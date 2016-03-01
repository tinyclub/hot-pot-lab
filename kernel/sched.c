
#include <dim-sum/base.h>
#include <asm/asm-offsets.h>
#include <linux/kernel.h>
#include <dim-sum/hard_irq.h>
#include <dim-sum/sched.h>
#include <asm/current.h>
#include <asm/switch_to.h>
#include <dim-sum/magic.h>
#include <dim-sum/timer.h>
//#include <dim-sum/alloc_flags.h>
//#include <dim-sum/memory_alloc.h>
#include <dim-sum/mem.h>
#include <kapi/dim-sum/task.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <dim-sum/wait.h>
#include <asm/signal.h>
#include <linux/wait.h>


#define MAX_RT_PRIO 31
#define SCHED_MASK_SIZE ((MAX_RT_PRIO + BITS_PER_LONG - 1) / BITS_PER_LONG)
unsigned long sched_runqueue_mask[SCHED_MASK_SIZE] = { 0, };
struct list_head sched_runqueue_list[MAX_RT_PRIO+1];

#if 0
extern struct files_struct global_files;
extern fd_set global_open_fds;
#endif

union thread_union __attribute__((aligned (THREAD_SIZE))) init_thread_stack;
struct task_struct init_task;
#define INIT_SP		((unsigned long)&init_thread_stack + THREAD_START_SP)

struct list_head sched_all_task_list;

DEFINE_SPINLOCK(lock_all_task_list);

DEFINE_PER_CPU(struct thread_info *, all_thread_info);

extern void* AllocStack(void);
extern void FreeStack(void* Address);
/**
 * 释放任务结构
 */
void __put_task_struct(struct task_struct *tsk)
{
	list_del_init(&tsk->all_list);
	FreeStack(tsk->stack);
}

void * alloc_task_stack(int stack_size)
{
	void *ret;
	stack_size = stack_size & (~(THREAD_SIZE - 1));
	if (stack_size < THREAD_SIZE)
	{
		stack_size = THREAD_SIZE;
	}
	
	//ret = SysPageAlloc(PG_OPT_NOWMARK,  stack_size >> 12);
	ret = AllocStack();
	if (ret)
	{
		memset(ret, 0, stack_size);
	}
	
	return ret;
}


static void inline _add_to_runqueue_no_preempt(struct task_struct * p)
{
	int pri = p->prio;

	if (p->in_list)
		return;

	list_add_tail(&p->run_list,&(sched_runqueue_list[pri])); 
	p->in_list = 1;

    	set_bit(pri, ( long unsigned int *)sched_runqueue_mask);  
}

static inline void set_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	set_ti_thread_flag(tsk->thread_info, flag);
}
static inline void set_tsk_need_resched(struct task_struct *tsk)
{
	set_tsk_thread_flag(tsk, TIF_NEED_RESCHED);
}
static inline void _add_to_runqueue(struct task_struct * p)
{
 	int pri = p->prio;
	if (p->in_list)
		return;
	list_add_tail(&p->run_list,&(sched_runqueue_list[pri])); 
	p->in_list = 1;
	if (p->prio < current->prio)
	{
		set_tsk_need_resched(current);
	}
	set_bit(pri, ( long unsigned int *)sched_runqueue_mask);  
}

static inline void _del_from_runqueue(struct task_struct * p)
{
	int pri = p->prio;

	if (!p->in_list)
		return;

	if (p == &init_task)
	{
		BUG();
		while (1);
	}
	list_del(&p->run_list);
	p->run_list.next = NULL;

	if(list_empty(&(sched_runqueue_list[pri])))
	{
	    clear_bit(pri, ( long unsigned int *)sched_runqueue_mask);
	}
	p->in_list = 0;
}



asmlinkage void __sched PreemptSwitch(void)
{
	if (likely(preempt_count() || irqs_disabled()))
	{
		return;
	}
	
need_resched:
	add_preempt_count(PREEMPT_ACTIVE);

	TaskSwitch();

	sub_preempt_count(PREEMPT_ACTIVE);

	barrier();
	if (unlikely(test_thread_flag(TIF_NEED_RESCHED)))
		goto need_resched;
}

asmlinkage void __sched PreemptSwitchInIrq(void)
{
	if (likely(preempt_count() || !irqs_disabled()))
		return;

need_resched:
	add_preempt_count(PREEMPT_ACTIVE);

	local_irq_enable();
	TaskSwitch();
	local_irq_disable();

	sub_preempt_count(PREEMPT_ACTIVE);

	barrier();
	if (unlikely(test_thread_flag(TIF_NEED_RESCHED)))
		goto need_resched;
}

void ker_thread_entry(void);
struct task_struct *__switch_to(struct thread_info *prev,
	struct thread_info *new)
{
	struct task_struct *last;

	last = _switch_s(prev, new);

	return last;
}

asmlinkage void TaskSwitch(void)
{
	unsigned long flags;
	struct task_struct *prev, *next;
	struct list_head *list;
	int idx ;

	if (irqs_disabled())
	{
		printk("irqs_disabled!\n");
		BUG();
	}
	
	if (InHardInterrupt() 
		|| (preempt_count() & (~PREEMPT_ACTIVE))
		|| current->flags & (~((1 << TIF_NEED_RESCHED) | (1 << TIF_NEED_DELETE)))
		) 
	{
		printk("cannt switch task, %lx, %d, %d, %s\n", InHardInterrupt(), preempt_count(), current->flags, current->name);
		dump_stack();
		BUG();
	}

need_resched:
	preempt_disable();
	spin_lock_irqsave(&lock_all_task_list, flags);

	prev = current;
	if (!(preempt_count() & PREEMPT_ACTIVE)) {
		if (prev->state & (TASK_WAIT | TASK_DEADED | TASK_ZOMBIE))
		{
			_del_from_runqueue(prev);
		}
	}

	idx = find_first_bit(sched_runqueue_mask, MAX_RT_PRIO);
	if (idx > MAX_RT_PRIO)
	{
		BUG();
	}

	list = sched_runqueue_list[idx].next;
    	next = list_entry(list, struct task_struct, run_list);

	
	if (unlikely(prev == next)) {
		clear_tsk_need_resched(prev);
		spin_unlock_irq(&lock_all_task_list);
		preempt_enable_no_resched();
		goto same_process;
	}
	clear_tsk_need_resched(prev);
	
	next->prev_sched = prev;
	switch_to(prev, next, prev);
	barrier();
	/**
	 * 只能在切换后，释放上一个进程的结构。
	 */
	if (current->prev_sched && (current->prev_sched->state & TASK_DEADED))
	{
		put_task_struct(current->prev_sched);
		current->prev_sched = NULL;
	}

	spin_unlock_irq(&lock_all_task_list);
	preempt_enable_no_resched();

same_process:
	if (unlikely(test_thread_flag(TIF_NEED_RESCHED)))
		goto need_resched;

	return;
}

int __sched try_to_wake_up(struct task_struct *tsk)
{
	unsigned long flags;
	spin_lock_irqsave(&lock_all_task_list, flags);
	if (tsk->state & TASK_STOPPED)
	{
		tsk->state &= ~TASK_WAIT;
	}
	else
	{
		_add_to_runqueue(tsk);
		tsk->state = TASK_RUNNING;
	}
	spin_unlock_irqrestore(&lock_all_task_list, flags);

	return 0;
}


/*
 * The core wakeup function.  Non-exclusive wakeups (nr_exclusive == 0) just
 * wake everything up.  If it's an exclusive wakeup (nr_exclusive == small +ve
 * number) then we wake all the non-exclusive tasks and one exclusive task.
 *
 * There are circumstances in which we can try to wake a task which has already
 * started to run but is not in state TASK_RUNNING.  try_to_wake_up() returns
 * zero in this (rare) case, and we handle it by continuing to scan the queue.
 */
static void __wake_up_common(wait_queue_head_t *q, unsigned int mode,
			     int nr_exclusive, int sync, void *key)
{
	struct list_head *tmp, *next;

	list_for_each_safe(tmp, next, &q->task_list) {
		wait_queue_t *curr;
		unsigned flags;
		curr = list_entry(tmp, wait_queue_t, task_list);
		flags = curr->flags;
		if (curr->func(curr, mode, sync, key) &&
		    (flags & WQ_FLAG_EXCLUSIVE) &&
		    !--nr_exclusive)
			break;
	}
}

/**
 * __wake_up - wake up threads blocked on a waitqueue.
 * @q: the waitqueue
 * @mode: which threads
 * @nr_exclusive: how many wake-one or wake-many threads to wake up
 */
void fastcall __wake_up(wait_queue_head_t *q, unsigned int mode,
				int nr_exclusive, void *key)
{
	unsigned long flags;

	spin_lock_irqsave(&q->lock, flags);
	__wake_up_common(q, mode, nr_exclusive, 0, key);
	spin_unlock_irqrestore(&q->lock, flags);
}

/**
 * 非互斥进程由default_wake_function唤醒。它是try_to_wake_up的一个简单封装。
 */
int default_wake_function(wait_queue_t *curr, unsigned mode, int sync, void *key)
{
	struct task_struct *p = curr->task;
#ifdef xby_debug
	return try_to_wake_up(p, mode, sync);
#else
	return try_to_wake_up(p);
#endif
}


#define	SLEEP_ON_VAR					\
	unsigned long flags;				\
	wait_queue_t wait;				\
	init_waitqueue_entry(&wait, current);

#define SLEEP_ON_HEAD					\
	spin_lock_irqsave(&q->lock,flags);		\
	__add_wait_queue(q, &wait);			\
	spin_unlock(&q->lock);

#define	SLEEP_ON_TAIL					\
	spin_lock_irq(&q->lock);			\
	__remove_wait_queue(q, &wait);			\
	spin_unlock_irqrestore(&q->lock, flags);

/**
 * 该函数把当前进程的状态设置为TASK_UNINTERRUPTIBLE，并把它插入到特定的等待队列。
 * 然后，它调用调度程序，重新开始另外一个程序的执行。
 * 当睡眠进程被唤醒时，调度程序重新开始执行sleep_on函数，把该进程从等待队列中删除。
 */
void fastcall __sched __sleep_on(wait_queue_head_t *q, int state)
{
	SLEEP_ON_VAR

	current->state = state;

	SLEEP_ON_HEAD
	TaskSwitch();
	SLEEP_ON_TAIL
}

void fastcall __sched sleep_on(wait_queue_head_t *q)
{
	__sleep_on(q, TASK_UNINTERRUPTIBLE);
}

void interruptible_sleep_on(wait_queue_head_t *q)
{
	__sleep_on(q,TASK_INTERRUPTIBLE);
}

static int process_timeout(struct timer *timer)
{
	struct task_struct *p = (struct task_struct *)timer->data;

	try_to_wake_up(p);

	return 0;
}

signed long __sched schedule_timeout(signed long timeout)
{
	struct timer timer;
	unsigned long long start = 0;

	switch (timeout)
	{
	case MAX_SCHEDULE_TIMEOUT:
		TaskSwitch();
		goto out;
	case 0:/* 为了适合不等待获取信号量、消息队列的情况 */
		current->state = TASK_RUNNING;
		return 0;
	default:
		if (timeout < 0) {
			current->state = TASK_RUNNING;
			goto out;
		}
	}

	start = get_jiffies_64();
	timer_init(&timer);
	timer.data = (void*)current;
	timer.function = &process_timeout;

	del_disable();
	timer_set(&timer, (unsigned long long)timeout);

	TaskSwitch();

	timer_remove(&timer);

	del_enable();

	timeout = timeout - (signed long)(get_jiffies_64() - start);

	return timeout < 1 ? 0 : timeout;
 out:
	return MAX_SCHEDULE_TIMEOUT;
}

int ker_Sleep(int iMiniSec)
{
    set_task_state(current, TASK_WAIT);	
	schedule_timeout(iMiniSec);

	return 0;
}

void ker_change_prio_thread(struct task_struct *tsk, int prio)
{
	unsigned long flags;
	
	if (prio == tsk->prio)
	{
		return;
	}

	spin_lock_irqsave(&lock_all_task_list, flags);
	/**
	 * 此处需要用in_list进行判断，而不能判断进程状态是否为TASK_RUNNING
	 */
	if (tsk->in_list)
	{
		_del_from_runqueue(tsk);
		tsk->prio = prio;
		_add_to_runqueue(tsk);
	}
	else
	{
		tsk->prio = prio;
	}
	
	spin_unlock_irqrestore(&lock_all_task_list, flags);
}



#define get_task_struct(tsk)	do { atomic_inc(&(tsk)->usage); } while(0)

void __noreturn do_exit_task(void)
{
	struct task_struct *tsk = current;
	unsigned long flags;

	spin_lock_irqsave(&lock_all_task_list, flags);
	tsk->state |= TASK_ZOMBIE;
	spin_unlock_irqrestore(&lock_all_task_list, flags);

	wake_up_interruptible(&init_task.wait_chldexit);

	TaskSwitch();

	BUG();
}

int send_sig(unsigned long sig,struct task_struct * p,int priv)
{
	return 0;
}

/*
 * kill_pg() sends a signal to a process group: this is what the tty
 * control characters do (^C, ^Z etc)
 */
int kill_pg(int pgrp, int sig, int priv)
{
	struct task_struct *p;
	int err,retval = -ESRCH;
	int found = 0;

	if (sig<0 || sig>32 || pgrp<=0)
		return -EINVAL;
	for_each_task(p) {
		if (p->pgrp == pgrp) {
			if ((err = send_sig(sig,p,priv)) != 0)
				retval = err;
			else
				found++;
		}
	}
	return(found ? 0 : retval);
}

int kill_proc(int pid, int sig, int priv)
{
 	struct task_struct *p;

	if (sig<0 || sig>32)
		return -EINVAL;
	for_each_task(p) {
		if (p && p->pid == pid)
			return send_sig(sig,p,priv);
	}
	return(-ESRCH);
}

/*
 * Determine if a process group is "orphaned", according to the POSIX
 * definition in 2.2.2.52.  Orphaned process groups are not to be affected
 * by terminal-generated stop signals.  Newly orphaned process groups are 
 * to receive a SIGHUP and a SIGCONT.
 * 
 * "I ask you, have you ever known what it is to be an orphan?"
 */
int is_orphaned_pgrp(int pgrp)
{
	struct task_struct *p;

	for_each_task(p) {
		if ((p->pgrp != pgrp) || 
		    (p->state == TASK_ZOMBIE) ||
		    (p->p_pptr->pid == 1))
			continue;
		if ((p->p_pptr->pgrp != pgrp) &&
		    (p->p_pptr->session == p->session))
			return 0;
	}
	return(1);	/* (sighing) "Often!" */
}
int session_of_pgrp(int pgrp)
{
	struct task_struct *p;
	int fallback;

	fallback = -1;
	for_each_task(p) {
 		if (p->session <= 0)
 			continue;
		if (p->pgrp == pgrp)
			return p->session;
		if (p->pid == pgrp)
			fallback = p->session;
	}
	return fallback;
}


static __noreturn void _do_exit(int code)
{
	do_exit_task();
}

void ker_thread_entry(void)
{
	struct task_struct *tsk = current;
	int (*func)(void *) = tsk->func;

	raw_spin_unlock(&lock_all_task_list);
	local_irq_enable();
	preempt_enable();
	func(tsk->data);

	do_exit_task();
}

extern int init_task_fs(struct task_struct *parent, struct task_struct *new);
int CreateTask(struct thread_param_t *param)
{
	int ret = 0;
	union thread_union *stack = NULL;
	unsigned long flags;
	int stack_size = param->stack_size;
	struct task_struct *tsk;

	if (stack_size == 0)
		stack_size = THREAD_SIZE;
	tsk = dim_sum_mem_alloc(sizeof(struct task_struct), MEM_NORMAL);
	if (tsk == NULL)
	{
		goto out;
	}
	memset(tsk, 0, sizeof(struct task_struct));
	if (param->stack)
	{
		stack = (union thread_union *)param->stack;
		memset(stack, 0, PAGE_SIZE);
	}
	else
	{
		stack = (union thread_union *)alloc_task_stack(stack_size);
		if (stack == NULL)
		{
			goto out;
		}
	}

	if (param->prio < 0 || param->prio >= MAX_RT_PRIO)
	{
		goto out;
	}

	stack->thread_info.task = tsk;
	tsk->thread_info = &stack->thread_info;
	strncpy(tsk->name, param->name, TASK_NAME_LEN);
      	tsk->magic = TASK_MAGIC; 
	tsk->state = TASK_INIT;
	tsk->prio = param->prio;
	tsk->orig_prio = param->prio;
	tsk->flags = 0;
	tsk->exit_code = 0;
	tsk->func = param->func;
	tsk->data = param->data;
	tsk->in_list = 0;
	tsk->pid = (int)tsk;//task_id++;
	
	stack->thread_info.preempt_count = 2;
#ifdef  CONFIG_ARM
	tsk->thread_info->cpu_context.fp = 0;
	tsk->thread_info->cpu_context.sp = (unsigned long)stack + THREAD_START_SP;
	tsk->thread_info->cpu_context.pc = (unsigned long)(&ker_thread_entry);  
#endif
	tsk->prev_sched = NULL;
	init_task_fs(current, tsk);


#ifdef CONFIG_SUPPORT_DEL_TASK
	tsk->del_count = 0;
#endif
	get_task_struct(tsk);
	INIT_LIST_HEAD(&tsk->run_list);
	INIT_LIST_HEAD(&tsk->all_list);
	INIT_LIST_HEAD(&tsk->children);
	INIT_LIST_HEAD(&tsk->sibling);
	init_waitqueue_head(&tsk->wait_chldexit);
	spin_lock_irqsave(&lock_all_task_list, flags);
	tsk->state = TASK_RUNNING;
	_add_to_runqueue(tsk);
	list_add_tail(&tsk->all_list, &sched_all_task_list);
	spin_unlock_irqrestore(&lock_all_task_list, flags);
	return (int)tsk;

out:
	if (tsk)
	{
		dim_sum_mem_free(tsk);
	}
	if ((param->stack != NULL)
		&& (stack != NULL))
	{
		dim_sum_pages_free(stack);
	}
	
	return ret;
}

extern void cpu_idle (void);

/*********************************************************************************/
/**
 * 调度初始化函数
 */
void sched_init(void)
{
	unsigned long flags;
	int i;

	INIT_LIST_HEAD(&sched_all_task_list);
	
	if (!irqs_disabled())
	{
		printk("my god, disable irq, please!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		local_irq_disable();
	}
	
	for(i = 0; i < sizeof(sched_runqueue_list) / sizeof(sched_runqueue_list[0]); i++)
	{
		INIT_LIST_HEAD(&(sched_runqueue_list[i]));
	}

	memset(sched_runqueue_mask, 0, sizeof(sched_runqueue_mask));

	/**
	 * 这里不能再初始化堆栈了!!!!!!
	 */
	//memset(&init_thread_stack, 0, sizeof(init_thread_stack));
	memset(&init_task, 0, sizeof(init_task));
	init_thread_stack.thread_info.task = &init_task;
	init_task.thread_info = &init_thread_stack.thread_info;

	strncpy(init_task.name, "idle", 5);
      	init_task.magic = TASK_MAGIC; 
	init_task.state = TASK_INIT;
	init_task.prio = MAX_RT_PRIO;
	init_task.orig_prio = MAX_RT_PRIO;
	init_task.flags = 0;
	init_task.exit_code = 0;
	init_task.func = &cpu_idle;
	init_task.data = NULL;
	init_task.in_list = 0;
	init_task.prev_sched = NULL;

	init_task_fs(NULL, &init_task);

#ifdef CONFIG_SUPPORT_DEL_TASK
	init_task.del_count = 0;
#endif	
	init_thread_stack.thread_info.preempt_count = 0;

	INIT_LIST_HEAD(&init_task.run_list);
	INIT_LIST_HEAD(&init_task.all_list);
	INIT_LIST_HEAD(&init_task.children);
	INIT_LIST_HEAD(&init_task.sibling);
	init_waitqueue_head(&init_task.wait_chldexit);
	get_task_struct(&init_task);
	
	spin_lock_irqsave(&lock_all_task_list, flags);
	init_task.state = TASK_RUNNING;
	_add_to_runqueue_no_preempt(&init_task);
	list_add_tail(&init_task.all_list, &sched_all_task_list);
	spin_unlock_irqrestore(&lock_all_task_list, flags);
}

asmlinkage __noreturn void sys_exit(int error_code)
{
	_do_exit((error_code&0xff)<<8);
}



asmlinkage pid_t sys_wait4(pid_t pid, int *stat_addr, int options, struct rusage *ru)
{
	int flag, retval;
	DECLARE_WAITQUEUE(wait, current);
	struct task_struct *p;
	struct list_head *_p;

	if (stat_addr) {
		flag = verify_area(VERIFY_WRITE, stat_addr, 4);
		if (flag)
			return flag;
	}
	add_wait_queue(&init_task.wait_chldexit,&wait);
repeat:
	flag=0;
	current->state = TASK_INTERRUPTIBLE;
	spin_lock(&lock_all_task_list);

 	list_for_each(_p,&sched_all_task_list) {
		p = list_entry(_p,struct task_struct, all_list);
		if (pid != 0) {//等待特定线程
			if (p->pid != pid) {
				continue;
			}
		} else if (!pid) {//等待任意子任务
			if (p->pgrp != current->pgrp)
				continue;
		} else if (pid != -1) {//等待特定进程组中的任务
			if (p->pgrp != -pid)
				continue;
		}

		flag = 1;
		switch (p->state) {
			case TASK_STOPPED:
				if (!p->exit_code)
					continue;
				if (!(options & WUNTRACED) && !(p->flags & PF_PTRACED))
					continue;
				if (stat_addr)
					put_fs_long((p->exit_code << 8) | 0x7f,
						stat_addr);
				p->exit_code = 0;
#ifdef xby_debug
				if (ru != NULL)
					getrusage(p, RUSAGE_BOTH, ru);
#endif
				retval = p->pid;
				goto end_wait4;
			case TASK_ZOMBIE:
				current->cutime += p->utime + p->cutime;
				current->cstime += p->stime + p->cstime;
#ifdef xby_debug
				current->mm->cmin_flt += p->mm->min_flt + p->mm->cmin_flt;
				current->mm->cmaj_flt += p->mm->maj_flt + p->mm->cmaj_flt;
				if (ru != NULL)
					getrusage(p, RUSAGE_BOTH, ru);
#endif
				flag = p->pid;
				if (stat_addr)
					put_fs_long(p->exit_code, stat_addr);
				list_del_init(&p->sibling);
				list_del_init(&p->all_list);
				put_task_struct(p);

				retval = flag;
				goto end_wait4;
			default:
				continue;
		}
	}

	if (flag) {
		
		retval = 0;
		if (options & WNOHANG)
			goto end_wait4;
		spin_unlock(&lock_all_task_list);
		current->state=TASK_INTERRUPTIBLE;
		TaskSwitch();
		current->signal &= ~(1<<(SIGCHLD-1));
		retval = -ERESTARTSYS;
		if (current->signal & ~current->blocked)
			goto end_wait4_unlocked;
		goto repeat;
	}
	retval = -ECHILD;
end_wait4:
	spin_unlock(&lock_all_task_list);
end_wait4_unlocked:
	remove_wait_queue(&current->wait_chldexit,&wait);
	return retval;
}

/*
 * sys_waitpid() remains for compatibility. waitpid() should be
 * implemented by calling sys_wait4() from libc.a.
 */
asmlinkage int sys_waitpid(pid_t pid,unsigned long * stat_addr, int options)
{
	return sys_wait4(pid, stat_addr, options, NULL);
}

asmlinkage long sys_getpid(void)
{
	return current->pid;
}

asmlinkage long sys_getuid(void)
{
	return current->uid;
}


/**
 * 暂停任务运行
 */
int ker_suspend_task(struct task_struct *tsk)
{
	int ret = -1;
	unsigned long flags;

	if (!tsk)
    {
        return -1;
    }

	spin_lock_irqsave(&lock_all_task_list, flags);
	_del_from_runqueue(tsk);
	tsk->state |= TASK_STOPPED;
	spin_unlock_irqrestore(&lock_all_task_list, flags);

	if (tsk == current)
		TaskSwitch();

	ret = 0;
	return ret;
}

/**
 * 恢复任务运行
 */
int ker_resume_task(struct task_struct *tsk)
{
	int ret = -1;
	unsigned long flags;

	if (!tsk)
    {
        return -1;
    }

	spin_lock_irqsave(&lock_all_task_list, flags);
	if (tsk->state & TASK_WAIT)
	{
		tsk->state &= ~TASK_STOPPED;
	}
	else
	{
		_add_to_runqueue(tsk);
		tsk->state = TASK_RUNNING;
	}
	spin_unlock_irqrestore(&lock_all_task_list, flags);

	ret = 0;
	return ret;
}

