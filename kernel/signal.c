#include <asm/signal.h>
#include <linux/linkage.h>
#include <asm/siginfo.h>

int do_signal(struct pt_regs *regs)
{
	return 0;
}

asmlinkage int sys_rt_sigsuspend(const sigset_t *mask, size_t size)
{
	return -38;
}

asmlinkage long sys_rt_sigaction(int sig,
		 const struct sigaction *act,
		 struct sigaction *oact,
		 size_t sigsetsize)
{
	return -38;
}

asmlinkage long sys_rt_sigprocmask(int how, sigset_t __user *set, sigset_t __user *oset, size_t sigsetsize)
{
	return -38;
}

asmlinkage long sys_rt_sigpending(sigset_t __user *set, size_t sigsetsize)
{
	return -38;
}

/**
 * Kill系统调用处理函数
 * pid>0 表示把sig信号发送到其PID==pid的进程所属的线程组。
 * pid==0表示把信号发送到与调用进程同组的进程所有线程组。
 * pid==-1表示把信号发送到所有进程。除了swapper。init和current
 * pid < -1把信号发送到进程组-pid中进程的所有线程线。
 * 虽然kill能够发送编号在32-64之间的实时信号。但是它不能确保把一个新的元素加入到目标进程的挂起信号队列。
 * 因此，发送实时信号需要通过rt_sigqueueinfo系统调用进行。
 */
asmlinkage long
sys_kill(int pid, int sig)
{
	return -38;
}

/**
 * Rt_sigqueueinfo的系统调用处理函数
 */
asmlinkage long
sys_rt_sigqueueinfo(int pid, int sig, siginfo_t *uinfo)
{
	return -38;
}

asmlinkage long
sys_rt_sigtimedwait(const sigset_t __user *uthese,
		    siginfo_t __user *uinfo,
		    const struct timespec __user *uts,
		    size_t sigsetsize)
{
	return -38;
}


