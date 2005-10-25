/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *	    Chong Kai Xiong <descender@phreaker.net>
 *	    Eric Anholt <anholt@FreeBSD.org>
 *
 * Extra Credits: MPlayer cpudetect hackers.
 *
 * $Id:
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* FIXME: clean this entire file up */

#include "lvconfig.h"

#if defined(VISUAL_ARCH_POWERPC)
#if defined(VISUAL_OS_DARWIN)
#include <sys/sysctl.h>
#else
#include <signal.h>
#include <setjmp.h>
#endif
#endif

#if defined(VISUAL_OS_NETBSD) || defined(VISUAL_OS_OPENBSD)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#endif

#if defined(VISUAL_OS_FREEBSD)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(VISUAL_OS_LINUX)
#include <signal.h>
#endif

#if defined(VISUAL_OS_WIN32)
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lv_log.h"
#include "lv_cpu.h"

static VisCPU _lv_cpu_caps;
static int _lv_cpu_initialized = FALSE;

static int has_cpuid (void);
static int cpuid (unsigned int ax, unsigned int *p);

/* The sigill handlers */
#if defined(VISUAL_ARCH_X86) //x86 (linux katmai handler check thing)
#if defined(VISUAL_OS_LINUX) && defined(_POSIX_SOURCE) && defined(X86_FXSR_MAGIC)
static void sigill_handler_sse( int signal, struct sigcontext sc )
{
	/* Both the "xorps %%xmm0,%%xmm0" and "divps %xmm0,%%xmm1"
	 * instructions are 3 bytes long.  We must increment the instruction
	 * pointer manually to avoid repeated execution of the offending
	 * instruction.
	 *
	 * If the SIGILL is caused by a divide-by-zero when unmasked
	 * exceptions aren't supported, the SIMD FPU status and control
	 * word will be restored at the end of the test, so we don't need
	 * to worry about doing it here.  Besides, we may not be able to...
	 */
	sc.eip += 3;

	_lv_cpu_caps.hasSSE=0;
}

static void sigfpe_handler_sse( int signal, struct sigcontext sc )
{
	if ( sc.fpstate->magic != 0xffff ) {
		/* Our signal context has the extended FPU state, so reset the
		 * divide-by-zero exception mask and clear the divide-by-zero
		 * exception bit.
		 */
		sc.fpstate->mxcsr |= 0x00000200;
		sc.fpstate->mxcsr &= 0xfffffffb;
	} else {
		/* If we ever get here, we're completely hosed.
		*/
	}
}
#endif
#endif /* VISUAL_OS_LINUX && _POSIX_SOURCE && X86_FXSR_MAGIC */

#if defined(VISUAL_OS_WIN32)
LONG CALLBACK win32_sig_handler_sse(EXCEPTION_POINTERS* ep)
{
	if(ep->ExceptionRecord->ExceptionCode==EXCEPTION_ILLEGAL_INSTRUCTION){
		ep->ContextRecord->Eip +=3;
		_lv_cpu_caps.hasSSE=0;       
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif /* VISUAL_OS_WIN32 */


#if defined(VISUAL_ARCH_POWERPC) && !defined(VISUAL_OS_DARWIN)
static sigjmp_buf _lv_powerpc_jmpbuf;
static volatile sig_atomic_t _lv_powerpc_canjump = 0;

static void sigill_handler (int sig);

static void sigill_handler (int sig)
{
	if (!_lv_powerpc_canjump) {
		signal (sig, SIG_DFL);
		raise (sig);
	}

	_lv_powerpc_canjump = 0;
	siglongjmp (_lv_powerpc_jmpbuf, 1);
}

static void check_os_altivec_support( void )
{
#if defined(VISUAL_OS_DARWIN)
	int sels[2] = {CTL_HW, HW_VECTORUNIT};
	int has_vu = 0;
	size_t len = sizeof(has_vu);
	int err;

	err = sysctl (sels, 2, &has_vu, &len, NULL, 0);   

	if (err == 0)
		if (has_vu != 0)
			_lv_cpu_caps.hasAltiVec = 1;
#else /* !VISUAL_OS_DARWIN */
	/* no Darwin, do it the brute-force way */
	/* this is borrowed from the libmpeg2 library */
	signal (SIGILL, sigill_handler);
	if (sigsetjmp (_lv_powerpc_jmpbuf, 1)) {
		signal (SIGILL, SIG_DFL);
	} else {
		_lv_powerpc_canjump = 1;

		asm volatile
			("mtspr 256, %0\n\t"
			 "vand %%v0, %%v0, %%v0"
			 :
			 : "r" (-1));

		signal (SIGILL, SIG_DFL);
		_lv_cpu_caps.hasAltiVec = 1;
	}
#endif
}
#endif

/* If we're running on a processor that can do SSE, let's see if we
 * are allowed to or not.  This will catch 2.4.0 or later kernels that
 * haven't been configured for a Pentium III but are running on one,
 * and RedHat patched 2.2 kernels that have broken exception handling
 * support for user space apps that do SSE.
 */
static void check_os_katmai_support( void )
{
//	printf ("omg\n");
#if defined(VISUAL_ARCH_X86)
#if defined(VISUAL_OS_FREEBSD)
	int has_sse=0, ret;
	size_t len=sizeof(has_sse);

	ret = sysctlbyname("hw.instruction_sse", &has_sse, &len, NULL, 0);
	if (ret || !has_sse)
		_lv_cpu_caps.hasSSE=0;

#elif defined(VISUAL_OS_NETBSD) || defined(VISUAL_OS_OPENBSD)
	int has_sse, has_sse2, ret, mib[2];
	size_t varlen;

	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_SSE;
	varlen = sizeof(has_sse);

	ret = sysctl(mib, 2, &has_sse, &varlen, NULL, 0);
	if (ret < 0 || !has_sse) {
		_lv_cpu_caps.hasSSE=0;
	} else {
		_lv_cpu_caps.hasSSE=1;
	}

	mib[1] = CPU_SSE2;
	varlen = sizeof(has_sse2);
	ret = sysctl(mib, 2, &has_sse2, &varlen, NULL, 0);
	if (ret < 0 || !has_sse2) {
		_lv_cpu_caps.hasSSE2=0;
	} else {
		_lv_cpu_caps.hasSSE2=1;
	}
	_lv_cpu_caps.hasSSE = 0; /* FIXME ?!?!? */

#elif defined(VISUAL_OS_WIN32)
	LPTOP_LEVEL_EXCEPTION_FILTER exc_fil;
	if ( _lv_cpu_caps.hasSSE ) {
		exc_fil = SetUnhandledExceptionFilter(win32_sig_handler_sse);
		__asm __volatile ("xorps %xmm0, %xmm0");
		SetUnhandledExceptionFilter(exc_fil);
	}
#elif defined(VISUAL_OS_LINUX)
//	printf ("omg1\n");
//	printf ("omg2\n");
	struct sigaction saved_sigill;
	struct sigaction saved_sigfpe;

	/* Save the original signal handlers.
	*/
	sigaction( SIGILL, NULL, &saved_sigill );
	sigaction( SIGFPE, NULL, &saved_sigfpe );

#if 0		/* broken :( --nenolod */
	signal( SIGILL, (void (*)(int))sigill_handler_sse );
	signal( SIGFPE, (void (*)(int))sigfpe_handler_sse );
#endif

	/* Emulate test for OSFXSR in CR4.  The OS will set this bit if it
	 * supports the extended FPU save and restore required for SSE.  If
	 * we execute an SSE instruction on a PIII and get a SIGILL, the OS
	 * doesn't support Streaming SIMD Exceptions, even if the processor
	 * does.
	 */
	if ( _lv_cpu_caps.hasSSE ) {
		__asm __volatile ("xorps %xmm1, %xmm0");
	}

	/* Emulate test for OSXMMEXCPT in CR4.  The OS will set this bit if
	 * it supports unmasked SIMD FPU exceptions.  If we unmask the
	 * exceptions, do a SIMD divide-by-zero and get a SIGILL, the OS
	 * doesn't support unmasked SIMD FPU exceptions.  If we get a SIGFPE
	 * as expected, we're okay but we need to clean up after it.
	 *
	 * Are we being too stringent in our requirement that the OS support
	 * unmasked exceptions?  Certain RedHat 2.2 kernels enable SSE by
	 * setting CR4.OSFXSR but don't support unmasked exceptions.  Win98
	 * doesn't even support them.  We at least know the user-space SSE
	 * support is good in kernels that do support unmasked exceptions,
	 * and therefore to be safe I'm going to leave this test in here.
	 */
	if ( _lv_cpu_caps.hasSSE ) {
		//      test_os_katmai_exception_support();
	}

	/* Restore the original signal handlers.
	*/
	sigaction( SIGILL, &saved_sigill, NULL );
	sigaction( SIGFPE, &saved_sigfpe, NULL );

#else
//	printf ("hier dan3\n");
	/* We can't use POSIX signal handling to test the availability of
	 * SSE, so we disable it by default.
	 */
	_lv_cpu_caps.hasSSE=0;
#endif /* __linux__ */
//	printf ("hier dan\n");
#endif
//	printf ("hier dan ha\n");
}


static int has_cpuid (void)
{
#ifdef VISUAL_ARCH_X86
	int a, c;

	__asm __volatile
		("pushf\n"
		 "popl %0\n"
		 "movl %0, %1\n"
		 "xorl $0x200000, %0\n"
		 "push %0\n"
		 "popf\n"
		 "pushf\n"
		 "popl %0\n"
		 : "=a" (a), "=c" (c)
		 :
		 : "cc");

	return a != c;
#else
	return 0;
#endif
}

static int cpuid (unsigned int ax, unsigned int *p)
{
#ifdef VISUAL_ARCH_X86
	uint32_t flags;

	__asm __volatile
		("movl %%ebx, %%esi\n\t"
		 "cpuid\n\t"
		 "xchgl %%ebx, %%esi"
		 : "=a" (p[0]), "=S" (p[1]),
		 "=c" (p[2]), "=d" (p[3])
		 : "0" (ax));

	return VISUAL_OK;
#else
	return VISUAL_ERROR_CPU_INVALID_CODE;
#endif
}

/**
 * @defgroup VisCPU VisCPU
 * @{
 */

void visual_cpu_initialize ()
{
	uint32_t cpu_flags;
	unsigned int regs[4];
	unsigned int regs2[4];

	memset (&_lv_cpu_caps, 0, sizeof (VisCPU));

	/* Check for arch type */
#if defined(VISUAL_ARCH_MIPS)
	_lv_cpu_caps.type = VISUAL_CPU_TYPE_MIPS;
#elif defined(VISUAL_ARCH_ALPHA)
	_lv_cpu_caps.type = VISUAL_CPU_TYPE_ALPHA;
#elif defined(VISUAL_ARCH_SPARC)
	_lv_cpu_caps.type = VISUAL_CPU_TYPE_SPARC;
#elif defined(VISUAL_ARCH_X86)
	_lv_cpu_caps.type = VISUAL_CPU_TYPE_X86;
#elif defined(VISUAL_ARCH_POWERPC)
	_lv_cpu_caps.type = VISUAL_CPU_TYPE_POWERPC;
#else
	_lv_cpu_caps.type = VISUAL_CPU_TYPE_OTHER;
#endif

	/* Count the number of CPUs in system */
#if !defined(VISUAL_OS_WIN32) && !defined(VISUAL_OS_UNKNOWN)
	_lv_cpu_caps.nrcpu = sysconf (_SC_NPROCESSORS_ONLN);
	if (_lv_cpu_caps.nrcpu == -1)
		_lv_cpu_caps.nrcpu = 1;
#else
	_lv_cpu_caps.nrcpu = 1;
#endif
	
#if defined(VISUAL_ARCH_X86)
	/* No cpuid, old 486 or lower */
	if (has_cpuid () == 0)
		return;

	_lv_cpu_caps.cacheline = 32;
	
	/* Get max cpuid level */
	cpuid (0x00000000, regs);
	
	if (regs[0] >= 0x00000001) {
		unsigned int cacheline;

		cpuid (0x00000001, regs2);

		_lv_cpu_caps.x86cpuType = (regs2[0] >> 8) & 0xf;
		if (_lv_cpu_caps.x86cpuType == 0xf)
		    _lv_cpu_caps.x86cpuType = 8 + ((regs2[0] >> 20) & 255); /* use extended family (P4, IA64) */
		
		/* general feature flags */
		_lv_cpu_caps.hasTSC  = (regs2[3] & (1 << 8  )) >>  8; /* 0x0000010 */
		_lv_cpu_caps.hasMMX  = (regs2[3] & (1 << 23 )) >> 23; /* 0x0800000 */
		_lv_cpu_caps.hasSSE  = (regs2[3] & (1 << 25 )) >> 25; /* 0x2000000 */
		_lv_cpu_caps.hasSSE2 = (regs2[3] & (1 << 26 )) >> 26; /* 0x4000000 */
		_lv_cpu_caps.hasMMX2 = _lv_cpu_caps.hasSSE; /* SSE cpus supports mmxext too */

		cacheline = ((regs2[1] >> 8) & 0xFF) * 8;
		if (cacheline > 0)
			_lv_cpu_caps.cacheline = cacheline;
	}
	
	cpuid (0x80000000, regs);
	
	if (regs[0] >= 0x80000001) {

		cpuid (0x80000001, regs2);
		
		_lv_cpu_caps.hasMMX  |= (regs2[3] & (1 << 23 )) >> 23; /* 0x0800000 */
		_lv_cpu_caps.hasMMX2 |= (regs2[3] & (1 << 22 )) >> 22; /* 0x400000 */
		_lv_cpu_caps.has3DNow    = (regs2[3] & (1 << 31 )) >> 31; /* 0x80000000 */
		_lv_cpu_caps.has3DNowExt = (regs2[3] & (1 << 30 )) >> 30;
	}

	if (regs[0] >= 0x80000006) {
		cpuid (0x80000006, regs2);
		_lv_cpu_caps.cacheline = regs2[2] & 0xFF;
	}


#if defined(VISUAL_OS_LINUX) || defined(VISUAL_OS_FREEBSD) || defined(VISUAL_OS_NETBSD) || defined(VISUAL_OS_CYGWIN) || defined(VISUAL_OS_OPENBSD)
	if (_lv_cpu_caps.hasSSE)
		check_os_katmai_support ();

	if (!_lv_cpu_caps.hasSSE)
		_lv_cpu_caps.hasSSE2 = 0;
#else
	_lv_cpu_caps.hasSSE=0;
	_lv_cpu_caps.hasSSE2 = 0;
#endif
#endif /* VISUAL_ARCH_X86 */

#if VISUAL_ARCH_POWERPC
	check_os_altivec_support ();
#endif /* VISUAL_ARCH_POWERPC */

	visual_log (VISUAL_LOG_DEBUG, "CPU: Number of CPUs: %d", _lv_cpu_caps.nrcpu);
	visual_log (VISUAL_LOG_DEBUG, "CPU: type %d", _lv_cpu_caps.type);
	visual_log (VISUAL_LOG_DEBUG, "CPU: X86 type %d", _lv_cpu_caps.x86cpuType);
	visual_log (VISUAL_LOG_DEBUG, "CPU: cacheline %d", _lv_cpu_caps.cacheline);
	visual_log (VISUAL_LOG_DEBUG, "CPU: TSC %d", _lv_cpu_caps.hasTSC);
	visual_log (VISUAL_LOG_DEBUG, "CPU: MMX %d", _lv_cpu_caps.hasMMX);
	visual_log (VISUAL_LOG_DEBUG, "CPU: MMX2 %d", _lv_cpu_caps.hasMMX2);
	visual_log (VISUAL_LOG_DEBUG, "CPU: SSE %d", _lv_cpu_caps.hasSSE);
	visual_log (VISUAL_LOG_DEBUG, "CPU: SSE2 %d", _lv_cpu_caps.hasSSE2);
	visual_log (VISUAL_LOG_DEBUG, "CPU: 3DNow %d", _lv_cpu_caps.has3DNow);
	visual_log (VISUAL_LOG_DEBUG, "CPU: 3DNowExt %d", _lv_cpu_caps.has3DNowExt);
	visual_log (VISUAL_LOG_DEBUG, "CPU: AltiVec %d", _lv_cpu_caps.hasAltiVec);

	_lv_cpu_initialized = TRUE;
}

VisCPU *visual_cpu_get_caps ()
{
	if (_lv_cpu_initialized == FALSE)
		return NULL;

	return &_lv_cpu_caps;
}

/**
 * @}
 */

