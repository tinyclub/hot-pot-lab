#ifndef __ARM_MMU_H
#define __ARM_MMU_H

#include <asm/atomic.h>

typedef struct {
#ifdef CONFIG_CPU_HAS_ASID
	atomic64_t	id;
#else
	int		switch_pending;
#endif
	unsigned int	vmalloc_seq;
	unsigned long	sigpage;
} mm_context_t;

#endif
