/*
 *  boot阶段打印调试模块
 */
#include <linux/linkage.h>
#include <asm/assembler.h>

		.text

/*
 * !!!!!!  下面几个调试函数仅仅用于调试!!!!!!
 * 当内核打印函数不可用时，可调用这几个函数
 * 这些函数会破坏寄存器，因此可能会导致功能不正常。
 */

/*
 * 打印十六进制数字到串口等输出设备
 */

/*
 * 当没有开启MMU时，使用下面三个函数打印十六进制数字
 */
ENTRY(debug_printhex8_nommu) /* 输出8个字符到串口 */
		mov	r1, #8
		b	printhex_nommu
ENDPROC(debug_printhex8_nommu)

ENTRY(debug_printhex4_nommu)
		mov	r1, #4
		b	printhex_nommu
ENDPROC(debug_printhex4_nommu)

ENTRY(debug_printhex2_nommu)
		mov	r1, #2
printhex_nommu:	adr	r2, debug_printhex_buf
		add	r3, r2, r1
		mov	r1, #0
		strb	r1, [r3]
1:		and	r1, r0, #15
		mov	r0, r0, lsr #4
		cmp	r1, #10
		addlt	r1, r1, #'0'
		addge	r1, r1, #'a' - 10
		strb	r1, [r3, #-1]!
		teq	r3, r2
		bne	1b
		mov	r0, r2
		b	debug_printstr_nommu
ENDPROC(debug_printhex2_nommu)

/*
 * 当已经开启MMU后，使用下面三个函数打印十六进制数字
 */
ENTRY(debug_printhex8_mmu)
		mov	r1, #8
		b	printhex_mmu
ENDPROC(debug_printhex8_mmu)

ENTRY(debug_printhex4_mmu)
		mov	r1, #4
		b	printhex_mmu
ENDPROC(debug_printhex4_mmu)

ENTRY(debug_printhex2_mmu)
		mov	r1, #2
printhex_mmu:	adr	r2, debug_printhex_buf
		add	r3, r2, r1
		mov	r1, #0
		strb	r1, [r3]
1:		and	r1, r0, #15
		mov	r0, r0, lsr #4
		cmp	r1, #10
		addlt	r1, r1, #'0'
		addge	r1, r1, #'a' - 10
		strb	r1, [r3, #-1]!
		teq	r3, r2
		bne	1b
		mov	r0, r2
		b	debug_printstr_mmu
ENDPROC(debug_printhex2_mmu)

/* 打印十六进制时，使用的临时缓冲区 */
debug_printhex_buf:		.space 16

		.ltorg

