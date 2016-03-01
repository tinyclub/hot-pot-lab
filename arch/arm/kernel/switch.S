#include "asm/linkage.h"
#include <asm/assembler.h>
#include "asm/asm-offsets.h"
#include <linux/linkage.h>

#include <asm/assembler.h>
#include <asm/asm-offsets.h>
#include <asm/errno.h>
#include <asm/thread_info.h>

.global _switch_s;
ENTRY(_switch_s)
	add	ip, r0, #TI_CPU_SAVE
	stmia	ip!, {r4 - sl, fp, sp, lr} 	@ Store most regs on stack
	add	r4, r1, #TI_CPU_SAVE
	ldmia	r4, {r4 - sl, fp, sp, pc}  	@ Load all regs saved previously
ENDPROC(_switch_s)