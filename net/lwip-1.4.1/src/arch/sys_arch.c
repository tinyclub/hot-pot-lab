#include "lwip/debug.h"

#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/tcpip.h"

#include <dim-sum/spinlock.h>
#include <dim-sum/timer.h>
#include <dim-sum/init.h>
#include <kapi/dim-sum/task.h>
#include "arch/sys_arch.h"
#include <asm/current.h>
#include <asm/div64.h>


#ifndef WAIT_FOREVER
#define WAIT_FOREVER (-1)
#endif

#define LWIP_MAX_TASKS  4          /* Number of LwIP tasks */

spinlock_t lwip_spin_lock;
static int sys_thread_no = 0;

const void * const pvNullPointer;

struct sys_timeouts {
  struct sys_timeo *next;
};

struct timeoutlist {
  struct sys_timeouts timeouts;
  TaskId tid;
};
static struct timeoutlist timeoutlist[LWIP_MAX_TASKS];

struct sys_timeouts timeouts;

void
sys_init(void)
{
	sys_thread_no = 0;
	spin_lock_init(&lwip_spin_lock);
}


/***************************** dim-sum msg box *******************************/
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	if (!size)
		size = 10;
	
	mbox->sys_mbox = ker_msgQCreate(LWIP_MAX_MSGS, size, 0);
	mbox->valid = 1;
	return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox)
{
	ker_MsgQDelete(mbox->sys_mbox);
}

void sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
	if(!msg)
		msg = (void*)&pvNullPointer;

	if (ker_msgQSend(mbox->sys_mbox, (char*)&msg, sizeof(void*), WAIT_FOREVER, 0))
		printk("%s %d failed\n", __FUNCTION__,__LINE__);
	
}

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	return ker_msgQSend(mbox->sys_mbox, (char*)&msg, sizeof(void*), 0, 0) ? ERR_MEM : ERR_OK;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	int wait;
	u64_t start, delta;

	if (timeout) 
		wait = timeout;
	else 
		wait = WAIT_FOREVER;

	start = SysUpTime();
	if (ker_msgQReceive(mbox->sys_mbox, (char*)msg, sizeof(void*), wait) <= 0) {
		return SYS_ARCH_TIMEOUT;
	}

	delta = SysUpTime() - start;
	do_div(delta, 1000000);
	return (u32_t)delta;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	return ker_msgQReceive(mbox->sys_mbox, (char*)msg, sizeof(void*), 0) ? SYS_MBOX_EMPTY : 0;
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
	if(mbox->valid == 0)
		return 0;
	else 
		return 1;
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	mbox->valid = 0;
	}

/*********************************** dim-sum M sem ************************************/
#define PN_SEM_VALID	1
#define PN_SEM_INVALID	0

err_t sys_mutex_new(sys_mutex_t *mutex)
	{
	mutex->sys_mutex = ker_semMCreate(0);
	if (mutex->sys_mutex) {
		mutex->valid = PN_SEM_VALID;
		return ERR_OK;
	} else {
		return ERR_MEM;
	}
}

void sys_mutex_free(sys_mutex_t *mutex)
{
	ker_semMDelete(mutex->sys_mutex);
}

void sys_mutex_lock(sys_mutex_t *mutex)
{
	ker_semMTake(mutex->sys_mutex, WAIT_FOREVER);
}

void sys_mutex_unlock(sys_mutex_t *mutex)
{
	ker_semMGive(mutex->sys_mutex);
}

int sys_mutex_valid(sys_mutex_t *mutex)
{
	if(mutex->valid == PN_SEM_VALID)
		return 1;
	else 
		return 0;
}

void sys_mutex_set_invalid(sys_mutex_t *mutex)
{
	mutex->valid = PN_SEM_INVALID;
}

/*********************************** dim-sum C se ************************************/
err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
	sem->sys_sem = ker_semCCreate(0, (int)count);
	if(sem->sys_sem){
		sem->valid = PN_SEM_VALID;
		return ERR_OK;
	} else {
		return ERR_MEM;
	}
}

void sys_sem_free(sys_sem_t *sem)
{
	ker_semCDelete(sem->sys_sem);
}

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	int wait, ret;
	u64_t start, delta;
	
	if (timeout) 
		wait = timeout;
	else 
		wait = WAIT_FOREVER;

	start = SysUpTime();
	ret = ker_semCTake(sem->sys_sem, wait);

	if (ret == -1) {
		return SYS_ARCH_TIMEOUT;
	}
	if (ret == 0)
		return 0;

	delta = SysUpTime() - start;
	do_div(delta, 1000000);
	
	return (u32_t)delta;
}

void sys_sem_signal(sys_sem_t *sem)
{
	ker_semCGive(sem->sys_sem);
}

int sys_sem_valid(sys_sem_t *sem)
{

	if(sem->valid == PN_SEM_VALID)
		return 1;
	else 
		return 0;
}

void sys_sem_set_invalid(sys_sem_t *sem)
{
	sem->valid = PN_SEM_INVALID;
}


/**************************************************************************************/
typedef int (*TASK_ENTRY)(void *data);
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	TaskId tid;
	TASK_ENTRY entry;

	if(sys_thread_no >= LWIP_MAX_TASKS)
	{
		printk("sys_thread_new: Max Sys. Tasks reached\n");
		return 0;
	}

	timeoutlist[sys_thread_no].timeouts.next = NULL;

	++sys_thread_no; /* next task created will be one lower to this one */

	entry = (TASK_ENTRY)thread;
	if ((tid = SysTaskCreate(entry, arg, (char *)name, prio, NULL, 0)) == 0) {
		printk("sys_thread_new: Task creation error (prio=%d)\n", prio);
		--sys_thread_no;
		while(1);
	}
	timeoutlist[sys_thread_no].tid = tid;
	
	return tid;
}

struct sys_timeouts* sys_arch_timeouts(void)
{
    int i;
    struct timeoutlist *tl;

    for(i = 0; i < sys_thread_no; i++)
    {
        tl = &timeoutlist[i];
        if(tl->tid == (TaskId)current)
        {
            return &(tl->timeouts);
        }
    }

    return &timeouts;
}

/*** 该函数为了消除tcpip.c:508中的告警 ***/
void __mem_free(void *p)
{
	dim_sum_mem_free(p);
}

/**********************************  LWIP INIT  ****************************************************/
static int __dim_sum_lwip_init(void *argv)
{
	tcpip_init(NULL, NULL);
	dim_sum_netdev_startup();

	printk("LWIP-1.4.1 TCP/IP initialized.\n");
	
	return 0;
}

int dim_sum_lwip_init(void)
{
	SysTaskCreate(__dim_sum_lwip_init, NULL, "ph_lwip_init", 1, NULL, 0);

	return 0;
}

