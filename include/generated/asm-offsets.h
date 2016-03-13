#ifndef __ASM_OFFSETS_H__
#define __ASM_OFFSETS_H__
/*
 * DO NOT MODIFY.
 *
 * This file was generated by Kbuild
 *
 */


#define PAGE_SZ 4096 /* PAGE_SIZE	@ */
#define THREAD_START_SP 8088 /* (THREAD_SIZE -sizeof(struct pt_regs)-32)	@ */

#define _DMA_FROM_DEVICE 2 /* DMA_FROM_DEVICE	@ */
#define _DMA_TO_DEVICE 1 /* DMA_TO_DEVICE	@ */
#define TI_FLAGS 0 /* offsetof(struct thread_info, flags)	@ */
#define TI_PREEMPT 4 /* offsetof(struct thread_info, preempt_count)	@ */
#define TI_ADDR_LIMIT 8 /* offsetof(struct thread_info, addr_limit)	@ */
#define TI_TASK 12 /* offsetof(struct thread_info, task)	@ */
#define TI_EXEC_DOMAIN 16 /* offsetof(struct thread_info, exec_domain)	@ */
#define TI_CPU 20 /* offsetof(struct thread_info, cpu)	@ */
#define TI_CPU_DOMAIN 24 /* offsetof(struct thread_info, cpu_domain)	@ */
#define TI_CPU_SAVE 520 /* offsetof(struct thread_info, cpu_context)	@ */
#define TI_USED_CP 32 /* offsetof(struct thread_info, used_cp)	@ */
#define TI_TP_VALUE 48 /* offsetof(struct thread_info, tp_value)	@ */
#define TI_FPSTATE 56 /* offsetof(struct thread_info, fpstate)	@ */
#define TI_CPU_SAVE 520 /* offsetof(struct thread_info, cpu_context)	@ */
#define TI_VFPSTATE 200 /* offsetof(struct thread_info, vfpstate)	@ */

#define S_R0 0 /* offsetof(struct pt_regs, ARM_r0)	@ */
#define S_R1 4 /* offsetof(struct pt_regs, ARM_r1)	@ */
#define S_R2 8 /* offsetof(struct pt_regs, ARM_r2)	@ */
#define S_R3 12 /* offsetof(struct pt_regs, ARM_r3)	@ */
#define S_R4 16 /* offsetof(struct pt_regs, ARM_r4)	@ */
#define S_R5 20 /* offsetof(struct pt_regs, ARM_r5)	@ */
#define S_R6 24 /* offsetof(struct pt_regs, ARM_r6)	@ */
#define S_R7 28 /* offsetof(struct pt_regs, ARM_r7)	@ */
#define S_R8 32 /* offsetof(struct pt_regs, ARM_r8)	@ */
#define S_R9 36 /* offsetof(struct pt_regs, ARM_r9)	@ */
#define S_R10 40 /* offsetof(struct pt_regs, ARM_r10)	@ */
#define S_FP 44 /* offsetof(struct pt_regs, ARM_fp)	@ */
#define S_IP 48 /* offsetof(struct pt_regs, ARM_ip)	@ */
#define S_SP 52 /* offsetof(struct pt_regs, ARM_sp)	@ */
#define S_LR 56 /* offsetof(struct pt_regs, ARM_lr)	@ */
#define S_PC 60 /* offsetof(struct pt_regs, ARM_pc)	@ */
#define S_PSR 64 /* offsetof(struct pt_regs, ARM_cpsr)	@ */
#define S_OLD_R0 68 /* offsetof(struct pt_regs, ARM_ORIG_r0)	@ */
#define S_FRAME_SIZE 72 /* sizeof(struct pt_regs)	@ */

#define PROCINFO_INITFUNC 16 /* offsetof(struct proc_info_list, __cpu_flush)	@ */
#define PROCINFO_MM_MMUFLAGS 8 /* offsetof(struct proc_info_list, __cpu_mm_mmu_flags)	@ */
#define PROC_INFO_SZ 52 /* sizeof(struct proc_info_list)	@ */
#define PROCINFO_IO_MMUFLAGS 12 /* offsetof(struct proc_info_list, __cpu_io_mmu_flags)	@ */
#define SYS_ERROR0 10420224 /* 0x9f0000	@ */

#endif
