#ifndef _ASMARM_TLBFNS_H
#define _ASMARM_TLBFNS_H

//#include <dim-sum/mm_types.h>
struct vm_area_struct;

struct cpu_tlb_fns {
	void (*flush_user_range)(unsigned long, unsigned long, struct vm_area_struct *);
	void (*flush_kern_range)(unsigned long, unsigned long);
	unsigned long tlb_flags;
};

#endif /* _ASMARM_TLBFNS_H */

