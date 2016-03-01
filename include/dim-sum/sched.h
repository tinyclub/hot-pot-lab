#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#define PF_PTRACED		0x00000010

/*
 * cloning flags:
 */
#define CSIGNAL		0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM	0x00000100	/* set if VM shared between processes */
#define CLONE_FS	0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES	0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PTRACE	0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_VFORK	0x00004000	/* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT	0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD	0x00010000	/* Same thread group? */
#define CLONE_NEWNS	0x00020000	/* New namespace group? */
#define CLONE_SYSVSEM	0x00040000	/* share system V SEM_UNDO semantics */
#define CLONE_SETTLS	0x00080000	/* create a new TLS for the child */
#define CLONE_PARENT_SETTID	0x00100000	/* set the TID in the parent */
#define CLONE_CHILD_CLEARTID	0x00200000	/* clear the TID in the child */
#define CLONE_DETACHED		0x00400000	/* Unused, ignored */
#define CLONE_UNTRACED		0x00800000	/* set if the tracing process can't force CLONE_PTRACE on this clone */
#define CLONE_CHILD_SETTID	0x01000000	/* set the TID in the child */
#define CLONE_STOPPED		0x02000000	/* Start in stopped state */
#define CLONE_NEWUTS		0x04000000	/* New utsname group? */
#define CLONE_NEWIPC		0x08000000	/* New ipcs */
#define CLONE_NEWUSER		0x10000000	/* New user namespace */
#define CLONE_NEWPID		0x20000000	/* New pid namespace */
#define CLONE_NEWNET		0x40000000	/* New network namespace */
#define CLONE_IO		0x80000000	/* Clone io context */

/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE		5
/* Can be ORed in to make sure the process is reverted back to SCHED_NORMAL on fork */
#define SCHED_RESET_ON_FORK     0x40000000

#include <dim-sum/base.h>
#ifdef __KERNEL__
#include <linux/linkage.h>
//#include <asm/page.h>
#include <dim-sum/thread_info.h>
#include <asm/processor.h>
#include <fs/fs_struct.h>
#include <kapi/dim-sum/task.h>
#include <dim-sum/wait.h>

struct tty_struct;

union thread_union {
	struct thread_info thread_info;
	unsigned long stack[THREAD_SIZE/sizeof(long)];
};

typedef unsigned long cputime_t;

struct task_struct {
	volatile long 	state;	/* -1 unrunnable, 0 runnable, >0 stopped */
	void 		*stack;
	atomic_t 		usage;
	unsigned int 	flags;	/* per process flags, defined below */
	unsigned int 	ptrace;
#define TASK_NAME_LEN 16	
	char 		name[TASK_NAME_LEN];
	int 			prio, orig_prio;
	struct list_head 		run_list;
	struct list_head 		all_list;
	int			in_list;
	uid_t uid;
	uid_t euid;
	/**
	 * 进程PID
	 */
	pid_t pid;
	/**
	 * 组id
	 */
	gid_t gid;

	pid_t pgrp;
	uid_t fsuid;
	gid_t fsgid;

	//用户态寄存器现场
	struct pt_regs *uregs;
	//是否处于系统调用中
	int		in_syscall;
	struct thread_info *thread_info;

	/* 父进程子指针 */
	struct task_struct 		*p_pptr;
	struct task_struct 		*prev_sched;
	void 		*func;
	void 		*data;
	int			exit_code;
	int 			magic;
	/**
	 * 与当前任务相关的文件句柄结构，目前使用全局数组。
	 */
	struct files_struct	*files;
	/**
	 * 与文件系统相关的信息。如当前目录。
	 */
	struct fs_struct *fs;
	/**
	 * 文件系统在查找路径时使用，避免符号链接查找深度过深，导致死循环。
	 * link_count是__do_follow_link递归调用的层次。
	 * total_link_count调用__do_follow_link的总次数。
	 */
	int link_count, total_link_count;

	/**
	 * 链表头部。链表指向的所有元素都是进程创建的子进程。
	 */
	struct list_head children;	/* list of my children */
	/**
	 * 指向兄弟进程链表的下一个元素或前一个元素的指针。
	 */
	struct list_head sibling;	/* linkage in my parent's children list */
	
	int exit_signal;

	struct tty_struct *tty; /* NULL if no tty */
	int tty_old_pgrp;
	unsigned long signal;
	unsigned long blocked;	/* bitmap of masked signals */
	int session;
	int leader;
	unsigned long timeout;

	//任务运行时间
	cputime_t cutime, utime, stime, cstime;
	/**
	 * 在系统调用wait4中睡眠的进程的等待队列。
	 */
	wait_queue_head_t	wait_chldexit;
};

#define TASK_INIT 			128
#define TASK_RUNNING		0
#define TASK_WAIT			1
#define TASK_SUSPEND		2
#define TASK_STOPPED		4
#define TASK_DEADED		8
#define TASK_ZOMBIE		16

/**
 * 可中断的等待状态。
 */
#define TASK_INTERRUPTIBLE		1
/**
 * 不可中断的等待状态。
 * 这种情况很少，但是有时也有用：比如进程打开一个设备文件，其相应的驱动程序在探测硬件设备时，就是这种状态。
 * 在探测完成前，设备驱动程序如果被中断，那么硬件设备的状态可能会处于不可预知状态。
 */
#define TASK_UNINTERRUPTIBLE	2
/**
 * 暂停状态。当收到SIGSTOP,SIGTSTP,SIGTTIN或者SIGTTOU信号后，会进入此状态。
 */
#define TASK_STOPPED		4
/**
 * 被跟踪状态。当进程被另外一个进程监控时，任何信号都可以把这个置于该
 */
#define TASK_TRACED		8
/**
 * 僵死状态。进程的执行被终止，但是，父进程还没有调用完wait4和waitpid来返回有关
 * 死亡进程的信息。在此时，内核不能释放相关数据结构，因为父进程可能还需要它。
 */
#define EXIT_ZOMBIE		16
/**
 * 在父进程调用wait4后，删除前，为避免其他进程在同一进程上也执行wait4调用
 * 将其状态由EXIT_ZOMBIE转为EXIT_DEAD，即僵死撤销状态。
 */
#define EXIT_DEAD		32

#define __set_task_state(tsk, state_value)		\
	do { (tsk)->state = (state_value); } while (0)
/**
 * 设置进程状态。同时会加上内存屏障。
 */
#define set_task_state(tsk, state_value)		\
	set_mb((tsk)->state, (state_value))

/**
 * 设置当前进程状态。同时会加上内存屏障。
 */
#define __set_current_state(state_value)			\
	do { current->state = (state_value); } while (0)
#define set_current_state(state_value)		\
	set_mb(current->state, (state_value))
	


#define task_thread_info(task)	((struct thread_info *)(task)->thread_info)

static inline int need_resched(void)
{
	return unlikely(test_thread_flag(TIF_NEED_RESCHED));
}

static inline void clear_tsk_thread_flag(struct task_struct *tsk, int flag)
{
	clear_ti_thread_flag(tsk->thread_info,flag);
}
static inline void clear_tsk_need_resched(struct task_struct *tsk)
{
	clear_tsk_thread_flag(tsk,TIF_NEED_RESCHED);
}

#define del_disable()
#define del_enable()
#define del_check()

#define TASK_NAME_LEN 16

struct thread_param_t{
	char name[TASK_NAME_LEN];
	int	prio;
	unsigned long *stack;
	int		stack_size;
	void *func;
	void *data;
};

#define	MAX_SCHEDULE_TIMEOUT ((long)(~0UL>>1))

extern void __put_task_struct(struct task_struct *tsk);
#define put_task_struct(tsk)	\
	do{															\
		if (atomic_dec_and_test(&(tsk)->usage))				\
			__put_task_struct(tsk);							\
	}while (0)


#define __sched		__attribute__((__section__(".sched.text")))

extern struct task_struct * _switch_s(struct thread_info *prev,
				   struct thread_info *next);

int CreateTask(struct thread_param_t *param);
int try_to_wake_up(struct task_struct *tsk);	
signed long schedule_timeout(signed long timeout);
void ker_change_prio_thread(struct task_struct *tsk, int prio);

extern asmlinkage void TaskSwitch(void);
extern asmlinkage void PreemptSwitchInIrq(void);

extern struct task_struct init_task;
#define next_task(p)	list_entry((p)->all_list.next, struct task_struct, all_list)

#define for_each_task(p) \
	list_for_each_entry(p, &(init_task.all_list), all_list)

extern void sched_init(void);
extern int send_sig(unsigned long sig,struct task_struct * p,int priv);
extern int kill_proc(int pid, int sig, int priv);
extern int kill_pg(int pgrp, int sig, int priv);
extern int ker_suspend_task(struct task_struct *tsk);
extern int ker_resume_task(struct task_struct *tsk);

/*
 * TASK is a pointer to the task whose backtrace we want to see (or NULL for current
 * task), SP is the stack pointer of the first frame that should be shown in the back
 * trace (or NULL if the entire call-chain of the task should be shown).
 */
extern void show_stack(struct task_struct *task, unsigned long *sp);

#ifndef __HAVE_THREAD_FUNCTIONS
/*
 * Return the address of the last usable long on the stack.
 *
 * When the stack grows down, this is just above the thread
 * info struct. Going any lower will corrupt the threadinfo.
 *
 * When the stack grows up, this is the highest address.
 * Beyond that position, we corrupt data on the next page.
 */
static inline unsigned long *end_of_stack(struct task_struct *p)
{
#ifdef CONFIG_STACK_GROWSUP
	return (unsigned long *)((unsigned long)task_thread_info(p) + THREAD_SIZE) - 1;
#else
	return (unsigned long *)(task_thread_info(p) + 1);
#endif
}
#endif

#endif /* __KERNEL__ */

#endif
