/* Paranormal - A highly customizable audio visualization library
 * Copyright (C) 2001  Jamie Gennis <jgennis@mindspring.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <glib.h>
#include "pncpu.h"

static guint cpu_caps = 0;
static gboolean pn_cpu_ready = FALSE;

#if defined PN_USE_CPU_IA32
#define pn_cpu_init_arch pn_cpu_init_ia32
void
pn_cpu_init_ia32 (void)
{
  gboolean AMD = FALSE;
  guint eax, ebx, ecx, edx;

  /* GCC won't acknowledge the fact that I'm clobbering ebx, so just save it */
#define CPUID __asm__ __volatile__ ("pushl %%ebx\n\tcpuid\n\tmovl %%ebx,%1\n\tpopl %%ebx" \
				    :"+a"(eax),"=r"(ebx),"=c"(ecx),"=d"(edx)::"ebx");

  eax = 0;
  CPUID;

  AMD = (ebx == 0x68747541) && (ecx == 0x444d4163) && (edx == 0x69746e65);

  eax = 1;
  CPUID;

  if (edx & (1 << 23))
    cpu_caps |= PN_CPU_CAP_MMX;

  if (edx & (1 << 25))
    {
      cpu_caps |= PN_CPU_CAP_MMXEXT;
      cpu_caps |= PN_CPU_CAP_SSE;
    }

  eax = 0x80000000;
  CPUID;

  if (eax >= 0x80000001)
    {
      eax = 0x80000001;
      CPUID;

      if (edx & (1 << 31))
	cpu_caps |= PN_CPU_CAP_3DNOW;
      if (AMD && (edx & (1 << 22)))
	cpu_caps |= PN_CPU_CAP_MMXEXT;
    }
#undef CPUID
}
#else /* Architecture */
void
pn_cpu_init_arch (void)
{
}
#endif /* Architecture */

void
pn_cpu_init (void)
{
  if (pn_cpu_ready)
    return;

  pn_cpu_init_arch ();

  pn_cpu_ready = TRUE;
}

guint
pn_cpu_get_caps (void)
{
  pn_cpu_init ();

  return cpu_caps;
}
