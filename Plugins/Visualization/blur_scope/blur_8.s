.globl bscope_blur_8
	.type	 bscope_blur_8,@function
bscope_blur_8:
	pushl %ebp
	movl %esp,%ebp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl 20(%ebp),%edi
	movl %edi,%eax
	addl 8(%ebp),%eax
	leal 1(%eax),%esi
	movl %edi,%ecx
	imull 16(%ebp),%ecx
	subl $1,%ecx
	jc .L26
	leal 2(%eax),%ebx
	.align 4
.L27:
	movl %esi,%eax
	subl %edi,%eax
	movzbl (%eax),%edx
	movzbl -2(%ebx),%eax
	addl %eax,%edx
	movzbl (%ebx),%eax
	addl %eax,%edx
	movzbl (%edi,%esi),%eax
	addl %edx,%eax
	sarl $2,%eax
	cmpl $2,%eax
	jbe .L28
	addl $-2,%eax
.L28:
	movb %al,(%esi)
	incl %ebx
	incl %esi
	subl $1,%ecx
	jnc .L27
.L26:
	leal -12(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	leave
	ret
