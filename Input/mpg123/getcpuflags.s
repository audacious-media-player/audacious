# KIMURA Takuhiro <kim@comtec.co.jp>
# Copyright 2002 Haavard Kvaalen <havardk@xmms.org>
	
# Get feature flags with cpuid
# void mpg123_getcpuid(unsigned int *fflags, unsigned int *efflags)
	
.text
	.align 4
.globl mpg123_getcpuflags
	.type	 mpg123_getcpuflags,@function
mpg123_getcpuflags:
	pushl %ebp
	movl %esp,%ebp
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushfl			# First test if cpuid is supported
	pushfl			# Check if the ID flag (bit 21 of eflags) sticks
	popl %eax		# Get eflags
	movl %eax,%ebx
	xorl $0x200000,%eax	# Flip bit 21
	pushl %eax
	popfl			# Get modified eflags to flag register
	pushfl
	popl %eax		# Get eflags again
	popfl			# Restore original eflags
	xorl %ebx,%eax
	je nocpuid
	xorl %eax,%eax
	cpuid			# Check if eax = 1 is supported
	xorl %edx,%edx
	cmp $1,%eax
	jl noflags
	movl $1,%eax		# Get feature flags
	cpuid
noflags:
	movl 8(%ebp),%eax
	movl %edx,(%eax)
	movl $0x80000000,%eax	# Check support for extended level cpuid
	cpuid
	xorl %edx,%edx
	cmp $0x80000001,%eax	# Get extended feature flags
	jl noeflags
	movl $0x80000001,%eax
	cpuid
noeflags:
	movl 12(%ebp),%eax
	movl %edx,(%eax)
	jmp done
nocpuid:
	xorl %edx,%edx
	movl 8(%ebp),%eax
	movl %edx,(%eax)
	movl 12(%ebp),%eax
	movl %edx,(%eax)
done:
	popl %ebx
	popl %ecx
	popl %edx
	leave
	ret
