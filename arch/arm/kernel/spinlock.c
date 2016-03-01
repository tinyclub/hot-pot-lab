
#include <dim-sum/preempt.h>
#include <dim-sum/spinlock.h>

#ifdef CONFIG_SMP

static inline void _raw_spin_lock(spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
#ifdef CONFIG_CPU_32v6K
"	wfene\n"
#endif
"	strexeq	%0, %2, [%1]\n"
"	teqeq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	 __asm__ __volatile__("": : :"memory");
}


static inline void _raw_spin_unlock(spinlock_t *lock)
{
	__asm__ __volatile__("": : :"memory");

	__asm__ __volatile__(
"	str	%1, [%0]\n"
#ifdef CONFIG_CPU_32v6K
"	mcr	p15, 0, %1, c7, c10, 4\n" /* DSB */
"	sev"
#endif
	:
	: "r" (&lock->lock), "r" (0)
	: "cc");
}

static inline int __raw_spin_trylock(raw_spinlock_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	strexeq	%0, %2, [%1]"
	: "=&r" (tmp)
	: "r" (&lock->lock), "r" (1)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}
#endif


int __lockfunc _spin_trylock(spinlock_t *lock)
{

	preempt_disable();
	if (_raw_spin_trylock(lock))
	return 1;
	return 0;

}

void __lockfunc _spin_lock(spinlock_t *lock)
{

	preempt_disable();
	_raw_spin_lock(lock);

}

void __lockfunc _spin_unlock(spinlock_t *lock)
{

	_raw_spin_unlock(lock);
	preempt_enable();
}

unsigned long __lockfunc _spin_lock_irqsave(spinlock_t *lock)
{

	unsigned long flags;
	local_irq_save(flags);
        preempt_disable();
	_raw_spin_lock(lock);
	return flags;
	

}
void __lockfunc _spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags)
{
	_raw_spin_unlock(lock);
	local_irq_restore(flags);
	preempt_enable();
}

void __lockfunc _spin_unlock_irq(spinlock_t *lock)
{

	_raw_spin_unlock(lock);
	local_irq_enable();
	preempt_enable();

}

