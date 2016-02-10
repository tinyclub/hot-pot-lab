/*
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Enhanced CPU detection and feature setting code by Mike Jagdis
 *  and Martin Mares, November 1997.
 */
#include <asm/pgtable_types.h>
#include <asm/asm-offsets.h>
.text

/* Physical address */
#define pa(X) ((X) - __PAGE_OFFSET)


#define __HEAD	.section ".head.text","ax"


#define ASM_NL	;
#define ALIGN	.align 4, 0x90
#define ENTRY(name)	\
	.globl name ASM_NL	\
	ALIGN ASM_NL	\
	name:


#define END(name)	\
	.size name, .-name


#define ENDPROC(name)	\
	.type	name, @function ASM_NL	\
	END(name)

	__HEAD
ENTRY(startup_32)
	movl pa(stack_start),%ecx
	leal -__PAGE_OFFSET(%ecx),%esp
/*
 * Clear BSS first so that there are no surprises...
 */ 
	cld
	xorl %eax,%eax
	movl $pa(__bss_start),%edi
	movl $pa(__bss_stop),%ecx
	subl %edi,%ecx
	shrl $2,%ecx
	rep ; stosl

	call start_kernel
/**
 * 不应该运行到这里，只能死循环挂住了。
 */
1:	jmp 1b


ENDPROC(startup_32)


.section __ex_table, "a"
.align	8
.long 0, 0
.long 0, 0
.previous


ENTRY(stack_start)
	.long init_thread_stack+THREAD_SIZE
