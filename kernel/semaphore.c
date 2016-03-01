#include <base/list.h>
#include <dim-sum/semaphore.h>
#include <dim-sum/mem.h>
#include <dim-sum/printk.h>
#include <asm/string.h>
#include <asm/current.h>
#include <dim-sum/hard_irq.h>
#include <dim-sum/sched.h>
#include <dim-sum/errno.h>

#define LOG printk

DEFINE_SPINLOCK(lock_sem_list);
static LIST_HEAD(all_semb);
static LIST_HEAD(all_semc);
static LIST_HEAD(all_semm);

void init_sem_lib(void)
{
	INIT_LIST_HEAD(&all_semb);
	INIT_LIST_HEAD(&all_semc);
	INIT_LIST_HEAD(&all_semm);
}

static void add_sem_to_list(struct list_head *sem, struct list_head *list)
{
	unsigned long flags;

	spin_lock_irqsave(&lock_sem_list, flags);
	list_add_tail(sem, list);
	spin_unlock_irqrestore(&lock_sem_list, flags);
}

static void delete_sem_from_list(struct list_head *sem)
{
	unsigned long flags;

	spin_lock_irqsave(&lock_sem_list, flags);
	list_del(sem);
	spin_unlock_irqrestore(&lock_sem_list, flags);
}

void* hal_malloc_align4(int size)
{
	unsigned long *tmp;
	unsigned long p = (unsigned long)dim_sum_mem_alloc(size + 8, MEM_NORMAL);
	unsigned long ret = (p >> 3 << 3) + 8;
	if (!p) {
		printk("%s: dim_sum_mem_alloc return NULL\n", __FUNCTION__);
		return NULL;
	}
	tmp = (unsigned long *)(ret - 4);
	*tmp = p;

	return (void *)ret;
}
void* hal_free_align4(void *p)
{
	unsigned long *tmp, *org_p;
	tmp = (void *)((unsigned long)p - 4);
	org_p = (void *)(*tmp);

	dim_sum_mem_free(org_p);

	return org_p;
}

struct semb_struct *alloc_semb_struct(void)
{
	struct semb_struct *ret = hal_malloc_align4(sizeof(struct semb_struct));
	if (ret)
	{
		memset(ret, 0, sizeof(struct semb_struct));
	}
	
	return ret;
}

SEM_ID ker_semBCreate_internal(int option, struct semb_struct *sem)
{
    if(InInterrupt())
    {
        LOG("can't create semB in_interrupt!\n");
        if (sem)
        {
          hal_free_align4(sem);
        }
        return 0;
    }
    if(!sem)
    {
        LOG("Can't get mem for semB");
        return 0;
    }

    sem->option = option;
	sem->owner  = (unsigned long)NULL;
    sem->magic  = SEM_MAGIC;
	spin_lock_init(&sem->lock);
    INIT_LIST_HEAD(&(sem->wait_list));
	atomic_set(&(sem->refcount), 1);

	add_sem_to_list(&sem->all_list, &all_semb);
    return (SEM_ID)sem;
}


SEM_ID ker_semBCreate(int option)
{
  struct semb_struct *sem = alloc_semb_struct();

	return ker_semBCreate_internal(option, sem);
}

int ker_semBDelete(SEM_ID semid)
{
	int ret = -1;
	unsigned long flags;
	struct task_struct *tsk;
    struct semb_wait_item *wait;
	struct list_head *list;
	struct semb_struct *sem = (struct semb_struct *)semid;
	
    if(!semid)
    {
         LOG("Delete NULL BSem!");
         return ERROR_SEMID_INVALID;
    }

    if (sem->magic != SEM_MAGIC)
    {
        return ERROR_SEMID_INVALID;    
    }

    if(InInterrupt())
    {       
         LOG("Delete BSem <0x%p> In ISR!",sem);
         return ERROR_SEM_IN_INTR;
    }

	delete_sem_from_list(&sem->all_list);

	spin_lock_irqsave(&sem->lock, flags);

    sem->magic = 0;
	while (!list_empty(&sem->wait_list))
    {
		list = sem->wait_list.next;
		wait = list_entry(list, struct semb_wait_item, list);
		list_del(list);
    	list->next = NULL;
		
		tsk = wait->task;
        wait->flag |= SEM_DELETED;

        try_to_wake_up(tsk);
    }

	spin_unlock_irqrestore(&sem->lock, flags);

	if (atomic_dec_and_test(&sem->refcount))
	{
		hal_free_align4(sem);
	}

	ret = 0;
    return ret;
}

int ker_semBTake_slow(struct semb_struct *sem, int timeout, unsigned long flags)
{
    struct task_struct *tsk;
    struct semb_wait_item wait;
	
    tsk = current;

RETRY:
    /* 未获得信号量，将线程挂起 */
    wait.task = tsk;
    wait.wait_sem = sem;
    wait.flag = SEM_NOT_GOT;
	if (sem->option & SEM_Q_PRIOITY)
	{
		struct list_head *list;
	    struct semb_wait_item *item;
	    int prio = current->prio;

	    for(list = sem->wait_list.prev; list != &(sem->wait_list); list = list->prev)
	    {
	        item = list_entry(list, struct semb_wait_item, list);
	        if(item->task->prio <= prio)
	        {
	            list_add(&wait.list, list);
	            goto out_list;
	        }
	    }
		list_add(&wait.list, &sem->wait_list);
	}
	else
	{
    	list_add_tail(&wait.list, &sem->wait_list);
	}

out_list:
    tsk->state = TASK_WAIT;

	atomic_inc(&sem->refcount);
	del_disable();
    spin_unlock_irqrestore(&sem->lock, flags);

	if (timeout == MAX_SCHEDULE_TIMEOUT)
	{
		TaskSwitch();
	}
    else
	{
		timeout = schedule_timeout(timeout); 
    }

    spin_lock_irqsave(&sem->lock, flags);

	if (atomic_dec_and_test(&sem->refcount))
	{
		spin_unlock_irqrestore(&sem->lock, flags);
		hal_free_align4(sem);

		del_enable();
		return ERROR_SEM_IS_UNREFED;
	}
	else
	if ((wait.flag & SEM_GOT_SEM))
	{
		spin_unlock_irqrestore(&sem->lock, flags);

		del_enable();
    	return 0;
	}
	else if (wait.flag & SEM_DELETED)
	{
		spin_unlock_irqrestore(&sem->lock, flags);
		del_enable();
		return ERROR_SEM_IS_DELETED;
	}
	else if (timeout)
	{
		list_del(&(wait.list));
        spin_unlock_irqrestore(&sem->lock, flags);
        del_enable();
		spin_lock_irqsave(&sem->lock, flags);
        goto RETRY;
	}
	else
	{
		list_del(&(wait.list));
		spin_unlock_irqrestore(&sem->lock, flags);

		del_enable();
    	return ERROR_SEM_TIMED_OUT;
	}
}


int ker_semBTake(SEM_ID semid, int timeout)
{
    unsigned long flags;
#define sem ((struct semb_struct *)semid)
    //struct semb_struct *sem = (struct semb_struct *)semid;

    if(!semid)
    {
         LOG("Get NULL BSem!");
         return ERROR_SEMID_INVALID;
    }
    if (sem->magic != SEM_MAGIC)
    {
        return ERROR_SEMID_INVALID;    
    }
    if(InInterrupt())
    {       
         LOG("Get BSem <0x%p> In ISR!",sem);
         return ERROR_SEM_IN_INTR;
    }

    spin_lock_irqsave(&sem->lock, flags);

    if(!sem->owner)
    {
        sem->owner = (unsigned long)current;
        spin_unlock_irqrestore(&sem->lock, flags);
        return 0;
    }

	return ker_semBTake_slow(sem, timeout, flags);
}
#undef sem

int ker_semBGive(SEM_ID semid)
{
    unsigned long flags;
#define sem ((struct semb_struct *)semid)
    //struct semb_struct *sem = (struct semb_struct *)semid;

	if(!semid)  return -1;
    if (sem->magic != SEM_MAGIC)
    {
        return ERROR_SEMID_INVALID;    
    }

    spin_lock_irqsave(&sem->lock, flags);


    if(list_empty(&sem->wait_list))
    {
        sem->owner = (unsigned long)NULL;
        spin_unlock_irqrestore(&sem->lock, flags);
        return 0;
    }
    else
    {
        struct task_struct *tsk;
    	struct semb_wait_item *wait;
		struct list_head *list;

		list = sem->wait_list.next;
		wait = list_entry(list, struct semb_wait_item, list);
		list_del(list);
    	//list->next = NULL;
		
		tsk = wait->task;
        sem->owner = (unsigned long)tsk;
        wait->flag |= SEM_GOT_SEM;

        try_to_wake_up(tsk);
        spin_unlock_irqrestore(&sem->lock, flags);


        return 0;
    }
}
#undef sem

struct semc_struct *alloc_semc_struct(void)
{
	struct semc_struct *ret = hal_malloc_align4(sizeof(struct semc_struct));
	if (ret)
	{
		memset(ret, 0, sizeof(struct semc_struct));
	}
	return ret;
}

SEM_ID init_semaphore_internal(int option, semaphore *sem, signed long num)
{
	sem->option = option;
    sem->count  = num;
    sem->magic  = SEM_MAGIC;
    spin_lock_init(&sem->lock);
    INIT_LIST_HEAD(&(sem->wait_list));
	atomic_set(&(sem->refcount), 1);

	add_sem_to_list(&sem->all_list, &all_semc);
    return (SEM_ID)sem;
}

void init_semaphore(semaphore *sem)
{
	init_semaphore_internal(0, sem, 1);
}

SEM_ID ker_semCCreate(int option, signed long num)
{
	struct semc_struct *sem;
    if(InInterrupt())
    {
        LOG("can't create semC in_interrupt, irq_count is %lu!\n", irq_count());
        return (SEM_ID)NULL;
    }

    sem = (struct semc_struct *)alloc_semc_struct();
    if(!sem)
    {
        LOG("Can't get mem for CSem");
        return (SEM_ID)NULL;
    }

    return init_semaphore_internal(option, sem, num);
}

int ker_semCDelete(SEM_ID semid)
{
	int ret = -1;
	unsigned long flags;
	struct task_struct *tsk;
    struct semc_wait_item *wait;
	struct list_head *list;
	struct semc_struct *sem = (struct semc_struct *)semid;
	
    if(!semid)
    {
         LOG("Delete NULL CSem!");
         return ERROR_SEMID_INVALID;
    }
    if (sem->magic != SEM_MAGIC)
    {
        return ERROR_SEMID_INVALID;    
    }
    if(InInterrupt())
    {       
         LOG("Delete CSem <0x%p> In ISR!",sem);
         return ERROR_SEM_IN_INTR;
    }

	delete_sem_from_list(&sem->all_list);
	
	spin_lock_irqsave(&sem->lock, flags);

    sem->magic = 0;
	while (!list_empty(&sem->wait_list))
    {
		list = sem->wait_list.next;
		wait = list_entry(list, struct semc_wait_item, list);
		list_del(list);
    	list->next = NULL;
		
		tsk = wait->task;
        wait->flag |= SEM_DELETED;

		try_to_wake_up(tsk);
    }

	spin_unlock_irqrestore(&sem->lock, flags);

	if (atomic_dec_and_test(&sem->refcount))
	{
		hal_free_align4(sem);
	}

	ret = 0;
    return ret;
}

int ker_semCTake_slow(struct semc_struct *sem, int timeout, unsigned long flags)
{
    struct task_struct *tsk;
    struct semc_wait_item wait;

    tsk = current;
RETRY:
    /* 未获得信号量，将线程挂起 */
    wait.task = tsk;
    wait.wait_sem = sem;
    wait.flag = SEM_NOT_GOT;

	if (sem->option & SEM_Q_PRIOITY)
	{
		struct list_head *list;
	    struct semc_wait_item *item;
	    int prio = current->prio;

	    for(list = sem->wait_list.prev; list != &(sem->wait_list); list = list->prev)
	    {
	        item = list_entry(list, struct semc_wait_item, list);
	        if(item->task->prio <= prio)
	        {
	            list_add(&wait.list, list);
	            goto out_list;
	        }
	    }
		list_add(&wait.list, &sem->wait_list);
	}
	else
	{
    	list_add_tail(&wait.list, &sem->wait_list);
	}

out_list:
    tsk->state = TASK_WAIT;
	atomic_inc(&sem->refcount);
	del_disable();
	spin_unlock_irqrestore(&sem->lock, flags);
	
	if (timeout == MAX_SCHEDULE_TIMEOUT)
	{
		TaskSwitch();
	}
    else
	{
		timeout = schedule_timeout(timeout); 
    }
    
	
	spin_lock_irqsave(&sem->lock, flags);

	if (atomic_dec_and_test(&sem->refcount))
	{
		spin_unlock_irqrestore(&sem->lock, flags);
		hal_free_align4(sem);

		del_enable();
		return ERROR_SEM_IS_UNREFED;
	}
    else 
	if ((wait.flag & SEM_GOT_SEM))
	{
		spin_unlock_irqrestore(&sem->lock, flags);

		del_enable();
    	return 0;
	}
	else if (wait.flag & SEM_DELETED)
	{
		spin_unlock_irqrestore(&sem->lock, flags);

		del_enable();
		return ERROR_SEM_IS_DELETED;
	}
	else if (timeout)
	{
		list_del(&(wait.list));
        spin_unlock_irqrestore(&sem->lock, flags);
		del_enable();
		spin_lock_irqsave(&sem->lock, flags);
        goto RETRY;
	}
	else
	{
		list_del(&(wait.list));
		spin_unlock_irqrestore(&sem->lock, flags);

		del_enable();
    	return ERROR_SEM_TIMED_OUT;
	}
}

int ker_semCTake(SEM_ID semid, int timeout)
{
    unsigned long flags;
#define sem ((struct semc_struct *)semid)
	//struct semc_struct *sem = (struct semc_struct *)semid;
    if(!semid)
    {
         LOG("Get NULL BSem!");
         return ERROR_SEMID_INVALID;
    }
    if (sem->magic != SEM_MAGIC)
    {
        return ERROR_SEMID_INVALID;    
    }
    if(InInterrupt())
    {       
         LOG("Get CSem <0x%p> In ISR!",sem);
         return ERROR_SEM_IN_INTR;
    }

    spin_lock_irqsave(&sem->lock, flags);

    if(sem->count > 0)
    {
        sem->count--;
        spin_unlock_irqrestore(&sem->lock, flags);
        return 0;
    }
	
	return ker_semCTake_slow(sem, timeout, flags);
}

#undef sem

void down(semaphore * sem)
{
	ker_semCTake((SEM_ID)sem, MAX_SCHEDULE_TIMEOUT);
}
/**
 * 释放信号量
 */
void up(semaphore * sem)
{
	ker_semCGive((SEM_ID)sem);
}

#undef sem


int ker_semCGive(SEM_ID semid)
{
    unsigned long flags;
#define sem ((struct semc_struct *)semid)
    //struct semc_struct *sem = (struct semc_struct *)semid;

	if(!semid)  return -1;
    if (sem->magic != SEM_MAGIC)
    {
        return ERROR_SEMID_INVALID;    
    }

    spin_lock_irqsave(&sem->lock, flags);

    if(list_empty(&sem->wait_list))
    {
        sem->count++;
        spin_unlock_irqrestore(&sem->lock, flags);
        return 0;
    }
    else
    {
        struct task_struct *tsk;
    	struct semc_wait_item *wait;
		struct list_head *list;

		list = sem->wait_list.next;
		wait = (struct semc_wait_item *)list_entry(list, struct semc_wait_item, list);
		list_del(list);
    	//list->next = NULL;
		
        tsk = wait->task;
        wait->flag = SEM_GOT_SEM;

        try_to_wake_up(tsk);
        spin_unlock_irqrestore(&sem->lock, flags);
        return 0;
    }
}
#undef sem

struct semm_struct *alloc_semm_struct(void)
{
	struct semm_struct *ret = hal_malloc_align4(sizeof(struct semm_struct));
	if (ret)
	{
		memset(ret, 0, sizeof(struct semm_struct));
	}
	
	return ret;
}

SEM_ID ker_semMCreate_internal(int option, struct semm_struct *sem)
{

    if(InInterrupt())
    {
        LOG("can't create semM in_interrupt!\n");
        
        if (sem)
        {
        	hal_free_align4(sem);
        }
        return (SEM_ID)NULL;
    }
    if ( (option&SEM_INVERSION_SAFE) && 
         (!(option&SEM_Q_PRIOITY)) )
    {
        LOG("can't create semM Invalid Option!\n");
        if (sem)
        {
        	hal_free_align4(sem);
        }
        return (SEM_ID)NULL;
    }

    if(!sem)
    {
        LOG("Can't get mem for MSem");
        return (SEM_ID)NULL;
    }

	sem->owner   = (unsigned long)NULL;
	sem->recurse = 0;
	sem->option  = option;
	sem->magic   = SEM_MAGIC;
    INIT_LIST_HEAD(&(sem->wait_list));
    spin_lock_init(&sem->lock);
    INIT_LIST_HEAD(&(sem->wait_list));
	atomic_set(&(sem->refcount), 1);

	add_sem_to_list(&sem->all_list, &all_semm);
    return (SEM_ID)sem;
}

SEM_ID ker_semMCreate(int option)
{
	struct semm_struct *sem;
	sem = (struct semm_struct *)alloc_semm_struct();
	
	return ker_semMCreate_internal(option, sem);
}


int ker_semMDelete(SEM_ID semid)
{
	int ret = -1;
	unsigned long flags;
	struct task_struct *tsk;
    struct semm_wait_item *wait;
	struct list_head *list;
	struct semm_struct *sem = (struct semm_struct *)semid;
	
    if(!semid)
    {
         LOG("Delete NULL MSem!");
         return ERROR_SEMID_INVALID;
    }
    if (sem->magic != SEM_MAGIC)
    {
        return ERROR_SEMID_INVALID;    
    }
    if(InInterrupt())
    {       
         LOG("Delete MSem <0x%p> In ISR!",sem);
         return ERROR_SEM_IN_INTR;
    }

	delete_sem_from_list(&sem->all_list);
	
	spin_lock_irqsave(&sem->lock, flags);

    sem->magic = 0;
	while (!list_empty(&sem->wait_list))
    {
		list = sem->wait_list.next;
		wait = list_entry(list, struct semm_wait_item, list);
		list_del(list);
    	list->next = NULL;
		
		tsk = wait->task;
        wait->flag |= SEM_DELETED;

        try_to_wake_up(tsk);
    }

	spin_unlock_irqrestore(&sem->lock, flags);

	if (atomic_dec_and_test(&sem->refcount))
	{
		hal_free_align4(sem);
	}

	ret = 0;
    return ret;
}

extern void ker_change_prio_thread(struct task_struct *tsk, int prio);

int ker_semMTake_slow(struct semm_struct *sem, int timeout, unsigned long flags)
{
	struct task_struct *tsk;
    struct semm_wait_item wait;

	tsk = current;
RETRY:
    /* 未获得信号量，将线程挂起 */
    wait.task = tsk;
    wait.wait_sem = sem;
    wait.flag = SEM_NOT_GOT;
	if (sem->option & SEM_Q_PRIOITY)
	{
		struct list_head *list;
	    struct semm_wait_item *item;
	    int prio = current->prio;

	    for(list = sem->wait_list.prev; list != &(sem->wait_list); list = list->prev)
	    {
	        item = list_entry(list, struct semm_wait_item, list);
	        if(item->task->prio <= prio)
	        {
	            list_add(&wait.list, list);
	            goto out_list;
	        }
	    }
		list_add(&wait.list, &sem->wait_list);
	}
	else
	{
    	list_add_tail(&wait.list, &sem->wait_list);
	}
    //list_add_tail(&wait.list, &sem->wait_list);

out_list:

    if(sem->option & SEM_INVERSION_SAFE)
    {
    	struct task_struct *owner;
        owner = (struct task_struct *)sem->owner;
        if(owner->prio > current->prio)
        {
            ker_change_prio_thread(owner,current->prio);
        }
    }

    tsk->state = TASK_WAIT;
	atomic_inc(&sem->refcount);
    spin_unlock_irqrestore(&sem->lock, flags);

	if (timeout == MAX_SCHEDULE_TIMEOUT)
	{
		TaskSwitch();
	}
    else
	{
		timeout = schedule_timeout(timeout); 
    }
    
    spin_lock_irqsave(&sem->lock, flags);

	if (atomic_dec_and_test(&sem->refcount))
	{
		spin_unlock_irqrestore(&sem->lock, flags);
    	hal_free_align4(sem);
		
		del_enable();
		del_check();
		return ERROR_SEM_IS_UNREFED;
	}
    else 
	if ((wait.flag & SEM_GOT_SEM))
	{
		spin_unlock_irqrestore(&sem->lock, flags);
    	return 0;
	}
	else if (wait.flag & SEM_DELETED)
	{
		spin_unlock_irqrestore(&sem->lock, flags);

		del_enable();
		del_check();
		
		return ERROR_SEM_IS_DELETED;
	}
	else if (timeout)
	{
		list_del(&(wait.list));
        //spin_unlock_irqrestore(&sem->lock, flags);
        goto RETRY;
	}
	else
	{
		list_del(&(wait.list));
		spin_unlock_irqrestore(&sem->lock, flags);
		
		del_enable();
		del_check();
    	return ERROR_SEM_TIMED_OUT;
	}
}

//int ker_semMTake(SEM_ID semid, int timeout,char *s,int line)
int ker_semMTake(SEM_ID semid, int timeout)
{
    unsigned long flags;
#define sem ((struct semm_struct *)semid)
    //struct semm_struct *sem = (struct semm_struct *)semid;

    if(!semid)
    {
         //LOG("wb Get NULL MSem! %s %d",s,line);
         LOG(" Get NULL MSem!");
         return ERROR_SEMID_INVALID;
    }
    if (sem->magic != SEM_MAGIC)
    {
		LOG(" wrong  MSem!");
		return ERROR_SEMID_INVALID;    
    }
    if(InInterrupt())
    {       
         LOG("Get MSem <0x%p> In ISR!",sem);
         return ERROR_SEM_IN_INTR;
    }

	del_disable();

    spin_lock_irqsave(&sem->lock, flags);
    
    if(!sem->owner)
    {
        sem->owner = (unsigned long)current;
		sem->recurse = 0;
        spin_unlock_irqrestore(&sem->lock, flags);
        return 0;
    }

	if(sem->owner == (unsigned long)current)
    {
		sem->recurse++;
        spin_unlock_irqrestore(&sem->lock, flags);
        return 0;
    }

	return ker_semMTake_slow(sem, timeout, flags);
}
#undef sem


int ker_semMGive(SEM_ID semid)
{
    unsigned long flags;
#define sem ((struct semm_struct *)semid)
    //struct semm_struct *sem = (struct semm_struct *)semid;
    if(!semid)  return -1;
    if (sem->magic != SEM_MAGIC)
    {
        return ERROR_SEMID_INVALID;    
    }

	if(InInterrupt())
    {       
         LOG("Give MSem <0x%p> In ISR!",sem);
         return -1;
    }

    spin_lock_irqsave(&sem->lock, flags);

	if(sem->owner != (unsigned long)current) 
    {
        spin_unlock_irqrestore(&sem->lock, flags);
        return -1;  /* 必须由同一线程释放 */
    }

    if(sem->recurse > 0)
    {
        sem->recurse--;
        spin_unlock_irqrestore(&sem->lock, flags);

		del_enable();
		/**
		 * 此处可以不检查删除标志，因为还没有完全释放M信号量。删除计数不应该是0。
		 */
		//del_check();
        return 0;
    }

	if(sem->option & SEM_INVERSION_SAFE)
    {
        if(current->prio != current->orig_prio)
        {
            ker_change_prio_thread(current, current->orig_prio);
        }
    }

    if(list_empty(&sem->wait_list))
    {
        sem->owner = (unsigned long)NULL;
		sem->recurse = 0;
		
        spin_unlock_irqrestore(&sem->lock, flags);
		
		del_enable();
		del_check();
		
        return 0;
    }
    else
    {
        struct task_struct *tsk;
    	struct semm_wait_item *wait;
		struct list_head *list;

		list = sem->wait_list.next;
		wait = (struct semm_wait_item *)list_entry(list, struct semm_wait_item, list);
		list_del(list);
    	//list->next = NULL;

        tsk = wait->task;
        wait->flag |= SEM_GOT_SEM;

		sem->owner = (unsigned long)tsk;
		//sem->recurse = 0;
		
        try_to_wake_up(tsk);
        spin_unlock_irqrestore(&sem->lock, flags);

		del_enable();
		del_check();
		
        return 0;
    }
}
#undef sem

#define SEM_INFO_PRINTF printk

struct sem_task_wait{
	unsigned long id;
	char		name[TASK_NAME_LEN];
};

struct semb_info_show{
	unsigned long id;
	unsigned long owner;
	int option;
	int waits_tsks;
	int refcount;
};

#define MAX_SHOW_SEMB_NUM 100
int show_semb(unsigned long semb, int level)
{
	unsigned long flags;
	struct semb_struct * sem= (struct semb_struct *)semb;
	struct semb_info_show *semb_infos = NULL;
	struct sem_task_wait *waits = NULL;
	struct list_head *list, *list1;
	int semb_infos_count = 0;
	int idx = 0;
	int i;
	int ret = 0;

	if(level < 0)
		return -EINVAL;
	if(sem)
	{
		if (sem->magic != SEM_MAGIC)
		{
			return ERROR_SEMID_INVALID;    
		}
		semb_infos_count = 1;
	}
	else
	{
		if(level > 0)
		{
			SEM_INFO_PRINTF("Should specify semaphore id for detail information!\n");
			return -EINVAL;
		}
		semb_infos_count = MAX_SHOW_SEMB_NUM;
	}

	semb_infos = dim_sum_mem_alloc(
		sizeof(struct semb_info_show)*semb_infos_count, MEM_NORMAL);
	if(semb_infos == NULL)
	{
		ret = -ENOMEM;
		goto out;
	}
	memset(semb_infos, 0, sizeof(struct semb_info_show)*semb_infos_count);

	spin_lock_irqsave(&lock_sem_list, flags);
	list_for_each(list, &all_semb)
	{
		struct semb_struct *t = list_entry(list, struct semb_struct, all_list);

		if (t == sem || NULL == sem)
		{
			spin_lock(&t->lock);
			semb_infos[idx].id = (unsigned long)t;
			semb_infos[idx].owner = t->owner;
			semb_infos[idx].option = t->option;
			list_for_each(list1, &t->wait_list)
			{
				semb_infos[idx].waits_tsks++;
			}
			semb_infos[idx].refcount = t->refcount.counter;
			spin_unlock(&t->lock);
			idx++;
			if (idx >= semb_infos_count)
				break;
		}
	}
	spin_unlock_irqrestore(&lock_sem_list, flags);

	if(sem && level > 0 && semb_infos->waits_tsks > 0)
	{
		int c = 0;
		waits = dim_sum_mem_alloc(
			sizeof(struct sem_task_wait)*semb_infos->waits_tsks, MEM_NORMAL);
		memset(waits, 0, sizeof(struct sem_task_wait)*semb_infos->waits_tsks);
		if(waits == NULL)
		{
			ret = -ENOMEM;
			goto out;
		}
		if (sem->magic != SEM_MAGIC)
		{
			ret = ERROR_SEMID_INVALID;
			goto out;
		}
		spin_lock_irqsave(&sem->lock, flags);
		list_for_each(list1, &sem->wait_list)
		{
			struct semb_wait_item *wait = list_entry(list1, struct semb_wait_item, list);
			waits[c].id = (unsigned long)wait->task;
			memcpy(waits[c].name, wait->task->name, TASK_NAME_LEN);
			c++;
			if(c >= semb_infos->waits_tsks)
				break;
		}
		semb_infos->waits_tsks = c;
		spin_unlock_irqrestore(&sem->lock, flags);
	}

	SEM_INFO_PRINTF("ID           OWNER        OPTIONS     REF      WAIT\n");
	for (i = 0; i < idx; i++)
	{
		SEM_INFO_PRINTF("0x%-10lx 0x%-10lx 0x%-9x %-8d %-8d\n",
			semb_infos[i].id,
			semb_infos[i].owner,
			semb_infos[i].option,
			semb_infos[i].refcount,
			semb_infos[i].waits_tsks
		);
	}
	
	if(waits)
	{
		if(semb_infos->waits_tsks)
		{
			SEM_INFO_PRINTF("WAIT TASKS:\n");
			for (i = 0; i < semb_infos->waits_tsks; i++)
			{
				SEM_INFO_PRINTF("0x%lx[%s]\n", waits[i].id, waits[i].name);
			}
		}
	}

out:
	if(waits)
		dim_sum_mem_free(waits);
	if(semb_infos)
		dim_sum_mem_free(semb_infos);
	return ret;
}

struct semc_info_show{
	unsigned long id;
	unsigned long count;
	int option;
	int waits_tsks;
	int refcount;
};

#define MAX_SHOW_SEMC_NUM 100
int show_semc(unsigned long semc, int level)
{
	unsigned long flags;
	struct semc_struct * sem= (struct semc_struct *)semc;
	struct semc_info_show *semc_infos = NULL;
	struct sem_task_wait *waits = NULL;
	struct list_head *list, *list1;
	int semc_infos_count = 0;
	int idx = 0;
	int i;
	int ret = 0;

	if(level < 0)
		return -EINVAL;
	if(sem)
	{
		if (sem->magic != SEM_MAGIC)
		{
			return ERROR_SEMID_INVALID;    
		}
		semc_infos_count = 1;
	}
	else
	{
		if(level > 0)
		{
			SEM_INFO_PRINTF("Should specify semaphore id for detail information!\n");
			return -EINVAL;
		}
		semc_infos_count = MAX_SHOW_SEMC_NUM;
	}

	semc_infos = dim_sum_mem_alloc(
		sizeof(struct semc_info_show)*semc_infos_count, MEM_NORMAL);
	if(semc_infos == NULL)
	{
		ret = -ENOMEM;
		goto out;
	}
	memset(semc_infos, 0, sizeof(struct semc_info_show)*semc_infos_count);

	spin_lock_irqsave(&lock_sem_list, flags);
	list_for_each(list, &all_semc)
	{
		struct semc_struct *t = list_entry(list, struct semc_struct, all_list);

		if (t == sem || NULL == sem)
		{
			spin_lock(&t->lock);
			memset(&semc_infos[idx], 0, sizeof(struct semc_info_show));
			semc_infos[idx].id = (unsigned long)t;
			semc_infos[idx].count = t->count;
			semc_infos[idx].option = t->option;
			list_for_each(list1, &t->wait_list)
			{
				semc_infos[idx].waits_tsks++;
			}
			semc_infos[idx].refcount = t->refcount.counter;
			spin_unlock(&t->lock);
			idx++;
			if (idx >= semc_infos_count)
				break;
		}
	}
	spin_unlock_irqrestore(&lock_sem_list, flags);

	if(sem && level > 0 && semc_infos->waits_tsks > 0)
	{
		int c = 0;
		waits = dim_sum_mem_alloc(
			sizeof(struct sem_task_wait)*semc_infos->waits_tsks, MEM_NORMAL);
		memset(waits, 0, sizeof(struct sem_task_wait)*semc_infos->waits_tsks);
		if(waits == NULL)
		{
			ret = -ENOMEM;
			goto out;
		}
		if (sem->magic != SEM_MAGIC)
		{
			ret = ERROR_SEMID_INVALID;
			goto out;
		}
		spin_lock_irqsave(&sem->lock, flags);
		list_for_each(list1, &sem->wait_list)
		{
			struct semc_wait_item *wait = list_entry(list1, struct semc_wait_item, list);
			waits[c].id = (unsigned long)wait->task;
			memcpy(waits[c].name, wait->task->name, TASK_NAME_LEN);
			c++;
			if(c >= semc_infos->waits_tsks)
				break;
		}
		semc_infos->waits_tsks = c;
		spin_unlock_irqrestore(&sem->lock, flags);
	}

	SEM_INFO_PRINTF("ID           COUNT  OPTIONS     REF      WAIT\n");
	for (i = 0; i < idx; i++)
	{	
		SEM_INFO_PRINTF("0x%-10lx %-6ld 0x%-9x %-8d %-8d\n",
			semc_infos[i].id,
			semc_infos[i].count,
			semc_infos[i].option,
			semc_infos[i].refcount,
			semc_infos[i].waits_tsks
		);
	}
	
	if(waits)
	{
		if(semc_infos->waits_tsks)
		{
			SEM_INFO_PRINTF("WAIT TASKS:\n");
			for (i = 0; i < semc_infos->waits_tsks; i++)
			{
				SEM_INFO_PRINTF("0x%lx[%s]\n", waits[i].id, waits[i].name);
			}
		}
	}
	
out:
	if(waits)
		dim_sum_mem_free(waits);
	if(semc_infos)
		dim_sum_mem_free(semc_infos);
	return ret;
}

struct semm_info_show{
	unsigned long id;
	unsigned long owner;
	int recurse;
	int option;
	int waits_tsks;
	int refcount;
};

#define MAX_SHOW_SEMM_NUM 100
int show_semm(unsigned long semm, int level)
{
	unsigned long flags;
	struct semm_struct * sem= (struct semm_struct *)semm;
	struct semm_info_show *semm_infos = NULL;
	struct sem_task_wait *waits = NULL;
	struct list_head *list, *list1;
	int semm_infos_count = 0;
	int idx = 0;
	int i;
	int ret = 0;

	if(level < 0)
		return -EINVAL;
	if(sem)
	{
		if (sem->magic != SEM_MAGIC)
		{
			return ERROR_SEMID_INVALID;    
		}
		semm_infos_count = 1;
	}
	else
	{
		if(level > 0)
		{
			SEM_INFO_PRINTF("Should specify semaphore id for detail information!\n");
			return -EINVAL;
		}
		semm_infos_count = MAX_SHOW_SEMM_NUM;
	}

	semm_infos = dim_sum_mem_alloc(
		sizeof(struct semm_info_show)*semm_infos_count, MEM_NORMAL);
	if(semm_infos == NULL)
	{
		ret = -ENOMEM;
		goto out;
	}
	memset(semm_infos, 0, sizeof(struct semm_info_show)*semm_infos_count);

	spin_lock_irqsave(&lock_sem_list, flags);
	list_for_each(list, &all_semm)
	{
		struct semm_struct *t = list_entry(list, struct semm_struct, all_list);

		if (t == sem || NULL == sem)
		{
			spin_lock(&t->lock);
			memset(&semm_infos[idx], 0, sizeof(struct semm_info_show));
			semm_infos[idx].id = (unsigned long)t;
			semm_infos[idx].owner = t->owner;
			semm_infos[idx].recurse = t->recurse;
			semm_infos[idx].option = t->option;
			list_for_each(list1, &t->wait_list)
			{
				semm_infos[idx].waits_tsks++;
			}
			
			semm_infos[idx].refcount = t->refcount.counter;
			spin_unlock(&t->lock);
			idx++;
			if (idx >= semm_infos_count)
				break;
		}
	}
	spin_unlock_irqrestore(&lock_sem_list, flags);

	if(sem && level > 0 && semm_infos->waits_tsks > 0)
	{
		int c = 0;
		waits = dim_sum_mem_alloc(
			sizeof(struct sem_task_wait)*semm_infos->waits_tsks, MEM_NORMAL);
		memset(waits, 0, sizeof(struct sem_task_wait)*semm_infos->waits_tsks);
		if(waits == NULL)
		{
			ret = -ENOMEM;
			goto out;
		}
		if (sem->magic != SEM_MAGIC)
		{
			ret = ERROR_SEMID_INVALID;
			goto out;
		}
		spin_lock_irqsave(&sem->lock, flags);
		list_for_each(list1, &sem->wait_list)
		{
			struct semm_wait_item *wait = list_entry(list1, struct semm_wait_item, list);
			waits[c].id = (unsigned long)wait->task;
			memcpy(waits[c].name, wait->task->name, TASK_NAME_LEN);
			c++;
			if(c >= semm_infos->waits_tsks)
				break;
		}
		semm_infos->waits_tsks = c;
		spin_unlock_irqrestore(&sem->lock, flags);
	}

	SEM_INFO_PRINTF("ID           OWNER        RECURSE     OPTIONS     REF      WAIT\n");
	for (i = 0; i < idx; i++)
	{
		SEM_INFO_PRINTF("0x%-10lx 0x%-10lx 0x%-9x 0x%-9x %-8d %-8d\n",
			semm_infos[i].id,
			semm_infos[i].owner,
			semm_infos[i].recurse,
			semm_infos[i].option,
			semm_infos[i].refcount,
			semm_infos[i].waits_tsks
		);
	}

	if(waits)
	{
		if(semm_infos->waits_tsks)
		{
			SEM_INFO_PRINTF("WAIT TASKS:\n");
			for (i = 0; i < semm_infos->waits_tsks; i++)
			{
				SEM_INFO_PRINTF("0x%lx[%s]\n", waits[i].id, waits[i].name);
			}
		}
	}
out:
	if(waits)
		dim_sum_mem_free(waits);
	if(semm_infos)
		dim_sum_mem_free(semm_infos);
	return ret;
}

DEFINE_SPINLOCK(lock_msgq_list);
static LIST_HEAD(all_msgq);

extern void* hal_malloc_align4(int size);
extern void* hal_free_align4(void *p);

void init_msgq_listhead(void)
{ 
	INIT_LIST_HEAD(&all_msgq);
}

static void add_msgq_to_list(struct list_head *msgq, struct list_head *list)
{
	unsigned long flags;

	spin_lock_irqsave(&lock_msgq_list, flags);
	list_add_tail(msgq, list);
	spin_unlock_irqrestore(&lock_msgq_list, flags);
}

static void delete_msgq_from_list(struct list_head *msgq)
{
	unsigned long flags;

	spin_lock_irqsave(&lock_msgq_list, flags);
	list_del(msgq);
	spin_unlock_irqrestore(&lock_msgq_list, flags);
}

struct msgq_struct *alloc_msgq_struct(void)
{
	struct msgq_struct *ret = hal_malloc_align4(sizeof(struct msgq_struct));
	if (ret)
	{
		memset(ret, 0, sizeof(struct msgq_struct));
	}
	return ret;
}
void free_msgq_struct(struct msgq_struct *msgq)
{
	hal_free_align4(msgq);
}
void free_msgq_data(struct msgq_struct *msgq)
{
	hal_free_align4(msgq->data);
}

void free_msgq_nodes(struct msgq_struct *msgq)
{
	hal_free_align4(msgq->nodes);
}

void free_msgq_all(struct msgq_struct *msgq)
{
	free_msgq_nodes(msgq);
	free_msgq_data(msgq);
	free_msgq_struct(msgq);
}


#define ALIGNNUM 4 
static int msgalign(int size)
{
        return ((size+ALIGNNUM-1) & (~(ALIGNNUM-1)));
}  
MSG_Q_ID  ker_msgQCreate( int max_msgs, int msglen, int opt )
{
	struct msgq_struct *msgq;
	int size;
	struct msgq_node *node;
	char *data;
	int i;
	
    if ((max_msgs <= 0) || (msglen <=0))
    {
        return (MSG_Q_ID)NULL;
    }

	if(InInterrupt())
    {
        LOG("can't create MSGQ in_interrupt!\n");
        return (MSG_Q_ID)NULL;
    }

	msgq = alloc_msgq_struct();
	if(!msgq)
    {
        LOG("Can't get mem for MSGQ");
        goto out1;
    }
	memset(msgq, 0, sizeof(*msgq));

	size = msgalign(msglen) * max_msgs;
	msgq->data = hal_malloc_align4(size);
	if (msgq->data == NULL)
	{
		LOG("Can't get mem for MSGQ");
		goto out2;
	}
	memset(msgq->data, 0, size);

	size = sizeof(struct msgq_node) * max_msgs;
	msgq->nodes = hal_malloc_align4(size);
	if (msgq->nodes == NULL)
	{
		LOG("Can't get mem for MSGQ");
		goto out3;
	}
	memset(msgq->nodes, 0, size);
	
	msgq->option = opt;
	msgq->magic  = MSGQ_MAGIC;
	msgq->msg_size = msgalign(msglen);
	msgq->msg_num = max_msgs;
	msgq->msg_count = 0;

	spin_lock_init(&msgq->lock);
    INIT_LIST_HEAD(&(msgq->msg_list));
	INIT_LIST_HEAD(&(msgq->free_list));
	INIT_LIST_HEAD(&(msgq->wait_recv_list));
	INIT_LIST_HEAD(&(msgq->wait_send_list));

	for(node = (struct msgq_node*)(msgq->nodes), data = msgq->data, i = max_msgs-1; i >= 0; i--)
    {
        node->data = data;
        list_add_tail(&node->list, &msgq->free_list);
		data += msgq->msg_size;
        node++;
    }

	atomic_set(&(msgq->refcount), 1);

	add_msgq_to_list(&msgq->all_list, &all_msgq);
	return (MSG_Q_ID)msgq;

out3:
	free_msgq_data(msgq);
out2:
	free_msgq_struct(msgq);
		
out1:
	return (MSG_Q_ID)NULL;
}


int ker_MsgQDelete(MSG_Q_ID msgqid)
{
	int ret = ERROR_MSGQID_INVALID;
	unsigned long flags;
	struct task_struct *tsk;
    struct msgq_wait_send_item *wait_item_send;
	struct msgq_wait_recv_item *wait_item_recv;
	struct list_head *list;
	struct msgq_struct *msgq = (struct msgq_struct *)msgqid;
	
    if(!msgqid)
    {
         LOG("Delete NULL MSGQ!");
         return ERROR_MSGQID_INVALID;
    }

    if (msgq->magic != MSGQ_MAGIC)
    {
        return ERROR_MSGQID_INVALID;    
    }

    if(InInterrupt())
    {       
         LOG("Delete MSGQ <0x%lu> In ISR!",msgqid);
         return ERROR_MSGQ_IN_INTR;
    }

	delete_msgq_from_list(&msgq->all_list);

	spin_lock_irqsave(&msgq->lock, flags);

    msgq->magic = 0;
 
	while (!list_empty(&msgq->wait_send_list))
    {
		list = msgq->wait_send_list.next;
		wait_item_send = list_entry(list, struct msgq_wait_send_item, list);
		list_del(list);
    	list->next = NULL;
		
		tsk = wait_item_send->task;
        wait_item_send->flag |= MSGQ_DELETED;

        try_to_wake_up(tsk);
    }

	while (!list_empty(&msgq->wait_recv_list))
    {
		list = msgq->wait_recv_list.next;
		wait_item_recv = list_entry(list, struct msgq_wait_recv_item, list);
		list_del(list);
    	list->next = NULL;
		
		tsk = wait_item_recv->task;
        wait_item_recv->flag |= MSGQ_DELETED;

        try_to_wake_up(tsk);
    }
	
	spin_unlock_irqrestore(&msgq->lock, flags);

	if (atomic_dec_and_test(&msgq->refcount))
	{
		free_msgq_all(msgq);
	}

	ret = 0;
    return ret;
}

int  ker_msgQSend( MSG_Q_ID queue, char *msg, unsigned int msglen,
                           int wait, int pri )
{
	int ret = 0;
	unsigned long flags;
	struct msgq_struct *msgq = (struct msgq_struct *)queue;
	struct list_head *list;
	struct msgq_node *node;
	struct list_head *list_wait;
	struct msgq_wait_send_item wait_item;

	if ((msg == NULL)
		|| (msglen == 0))
	{
		return -1;
	}
		
	if (msgq == NULL)
	{
		return ERROR_MSGQID_INVALID;
	}

    if (msgq->magic != MSGQ_MAGIC)
    {
        return ERROR_MSGQID_INVALID;    
    }

	if (msgq->msg_size < msglen)
	{
		return ERROR_MSG_LENS_OVER;
	}

	if (InInterrupt() && wait)  /* 中断里面不能等待 */
	{
		return ERROR_MSGQ_IN_INTR;
	}

RETRY:
	spin_lock_irqsave(&msgq->lock, flags);
	if (!list_empty(&msgq->free_list))
	{
		list = msgq->free_list.next;
		node = list_entry(list, struct msgq_node, list);
		list_del(list);
    	//list->next = NULL;

		memcpy(node->data, msg, msglen);
		node->size = msglen;
		msgq->msg_count++;
		
		list_add_tail(list, &msgq->msg_list);

		if (!list_empty(&msgq->wait_recv_list))
		{
			struct msgq_wait_recv_item *wait;
			struct task_struct *tsk;
			list_wait = msgq->wait_recv_list.next;
			wait = list_entry(list_wait, struct msgq_wait_recv_item, list);
			list_del(list_wait);
    		list_wait->next = NULL;
			wait->flag |= MSGQ_TRY_AGAIN;

			tsk = wait->task;
			try_to_wake_up(tsk);
		}

		spin_unlock_irqrestore(&msgq->lock, flags);
	}
	else
	{
		if (InInterrupt())
		{
			spin_unlock_irqrestore(&msgq->lock, flags);

			return ERROR_MSGQ_IN_INTR;
		}
		else
		{
			if (wait)
			{
				current->state = TASK_WAIT;
				wait_item.flag = MSGQ_WAIT;
				wait_item.task = current;
				wait_item.wait_msgq = msgq;

				//list_add_tail(&wait_item.list, &msgq->wait_send_list);
				if (msgq->option & MSGQ_Q_PRIOITY)
				{
					struct list_head *list;
				    struct msgq_wait_send_item *item;
				    int prio = current->prio;

				    for(list = msgq->wait_send_list.prev; list != &(msgq->wait_send_list); list = list->prev)
				    {
				        item = list_entry(list, struct msgq_wait_send_item, list);
				        if(item->task->prio <= prio)
				        {
				            list_add(&wait_item.list, list);
				            goto out_list;
				        }
				    }
					list_add(&wait_item.list, &msgq->wait_send_list);
				}
				else
				{
			    	list_add_tail(&wait_item.list, &msgq->wait_send_list);
				}

out_list:
				atomic_inc(&msgq->refcount);
				del_disable();
				spin_unlock_irqrestore(&msgq->lock, flags);
				wait = schedule_timeout(wait);

				spin_lock_irqsave(&msgq->lock, flags);

				if (atomic_dec_and_test(&msgq->refcount))
				{
					spin_unlock_irqrestore(&msgq->lock, flags);
					
					free_msgq_all(msgq);

					del_enable();
					return ERROR_MSGQ_IS_UNREFED;
				}
				else 
				if (wait_item.flag & MSGQ_TRY_AGAIN)
				{
					spin_unlock_irqrestore(&msgq->lock, flags);
					del_enable();
					goto RETRY;
				}
				else if (wait_item.flag & MSGQ_DELETED)
				{
					spin_unlock_irqrestore(&msgq->lock, flags);

					del_enable();
					return ERROR_MSGQ_IS_DELETED;
				}
				else if (wait)
				{
					list_del(&wait_item.list);
					spin_unlock_irqrestore(&msgq->lock, flags);

					del_enable();
					goto RETRY;
				}
				else
				{
					list_del(&wait_item.list);//jkp
					spin_unlock_irqrestore(&msgq->lock, flags);

					del_enable();
					return ERROR_MSGQ_TIMED_OUT;
				}
			}
			else
			{
				spin_unlock_irqrestore(&msgq->lock, flags);

				return ERROR_MSGQ_FULL;
			}
		}
	}

	return ret;
}

int  ker_msgQReceive( MSG_Q_ID queue, char *msgbuf, unsigned int buflen,
                              int wait )
{
	int ret = 0;
	unsigned long flags;
	struct msgq_struct *msgq = (struct msgq_struct *)queue;
	struct list_head *list;
	struct msgq_node *node;
	struct list_head *list_wait;
	struct msgq_wait_recv_item wait_item;

	//hal_printf("func %s, line %d, time %d\n", __FUNCTION__, __LINE__, wait);
	if (msgbuf == NULL)
	{
		return -1;
	}
	
	if (msgq == NULL)
	{
		return ERROR_MSGQID_INVALID;
	}

    if (msgq->magic != MSGQ_MAGIC)
    {
        return ERROR_MSGQID_INVALID;    
    }

	if (buflen <= 0)
	{
		return ERROR_BUF_LENS_UNDER;
	}

	if (InInterrupt() && wait)  /* 中断里面不能等待 */
	{
		return ERROR_MSGQ_IN_INTR;
	}
	
RETRY:
	spin_lock_irqsave(&msgq->lock, flags);

	if (!list_empty(&msgq->msg_list))
	{
		int copy_size = buflen;
		list = msgq->msg_list.next;
		node = list_entry(list, struct msgq_node, list);

		if (node->size < buflen)
			copy_size = node->size;
		
		list_del(list);
    	//list->next = NULL;

		memcpy(msgbuf, node->data, copy_size);
		
		list_add_tail(list, &msgq->free_list);

		msgq->msg_count--;

		if (!list_empty(&msgq->wait_send_list))
		{
			struct msgq_wait_send_item *wait;
			struct task_struct *tsk;
			list_wait = msgq->wait_send_list.next;
			wait = list_entry(list_wait, struct msgq_wait_send_item, list);
			list_del(list_wait);
    		list_wait->next = NULL;
			wait->flag |= MSGQ_TRY_AGAIN;

			tsk = wait->task;
			try_to_wake_up(tsk);
		}

		ret = copy_size;
		spin_unlock_irqrestore(&msgq->lock, flags);
	}
	else
	{
		if (InInterrupt())
		{
			spin_unlock_irqrestore(&msgq->lock, flags);

			return ERROR_MSGQ_IN_INTR;
		}
		else
		{
			if (wait)
			{
				current->state = TASK_WAIT;
				wait_item.flag = MSGQ_WAIT;
				wait_item.task = current;
				wait_item.wait_msgq = msgq;
				//list_add_tail(&wait_item.list, &msgq->wait_recv_list);
				if (msgq->option & MSGQ_Q_PRIOITY)
				{
					struct list_head *list;
				    struct msgq_wait_recv_item *item;
				    int prio = current->prio;

				    for(list = msgq->wait_recv_list.prev; list != &(msgq->wait_recv_list); list = list->prev)
				    {
				        item = list_entry(list, struct msgq_wait_recv_item, list);
				        if(item->task->prio <= prio)
				        {
				            list_add(&wait_item.list, list);
				            goto out_list;
				        }
				    }
					list_add(&wait_item.list, &msgq->wait_recv_list);
				}
				else
				{
			    	list_add_tail(&wait_item.list, &msgq->wait_recv_list);
				}

out_list:
				atomic_inc(&msgq->refcount);
				del_disable();
				spin_unlock_irqrestore(&msgq->lock, flags);
				wait = schedule_timeout(wait);

				spin_lock_irqsave(&msgq->lock, flags);

				if (atomic_dec_and_test(&msgq->refcount))
				{
					spin_unlock_irqrestore(&msgq->lock, flags);
					free_msgq_all(msgq);
					del_enable();
					return ERROR_MSGQ_IS_UNREFED;
				}
				else 
				if (wait_item.flag & MSGQ_TRY_AGAIN)
				{
					spin_unlock_irqrestore(&msgq->lock, flags);
					del_enable();
					goto RETRY;
				}
				else if (wait_item.flag & MSGQ_DELETED)
				{
					spin_unlock_irqrestore(&msgq->lock, flags);

					del_enable();
					return ERROR_MSGQ_IS_DELETED;
				}
				else if (wait)
				{
					list_del(&wait_item.list);
					spin_unlock_irqrestore(&msgq->lock, flags);
					del_enable();
					goto RETRY;
				}
				else
				{
					list_del(&wait_item.list); //jkp
					spin_unlock_irqrestore(&msgq->lock, flags);

					del_enable();
					return ERROR_MSGQ_TIMED_OUT;
				}
			}
			else
			{
				spin_unlock_irqrestore(&msgq->lock, flags);
				return ERROR_MSGQ_EMPTY;
			}
		}	
	}
	return ret;
}

int ker_msgQNumMsgs(MSG_Q_ID queue)
{
	struct msgq_struct *msgq = (struct msgq_struct *)queue;

	if (msgq == NULL)
	{
		return -1;
	}
    if (msgq->magic != MSGQ_MAGIC)
    {
        return ERROR_MSGQID_INVALID;    
    }

	return msgq->msg_count;
}

#define MSGQ_INFO_PRINTF printk

struct msgq_task_wait{
	unsigned long id;
	char		name[TASK_NAME_LEN];
};

struct msgq_msg_detail{
	char *addr;
	char	*data;
	int size;
};

struct msgq_info_show{
	unsigned long id;
	int option;
	int msg_size;
	int msg_num;
	int msg_count;
	int free_count;
	int wait_recv;
	int wait_send;
	int refcount;
};

#define MAX_SHOW_MSGQ_NUM 100
int show_msgq(unsigned long amsgq, int level)
{
	unsigned long flags;
	struct msgq_struct * msgq= (struct msgq_struct *)amsgq;
	struct msgq_info_show *msgq_infos = NULL;
	struct msgq_task_wait *send_waits = NULL;
	struct msgq_task_wait *recv_waits = NULL;
	struct msgq_msg_detail *msg_detail = NULL;
	char * msg_data = NULL;
	struct list_head *list, *list1;
	int msgq_infos_count = 0;
	int idx = 0;
	int i,j;
	int ret = 0;

	if(level < 0)
		return -1;
	if(msgq)
	{
		if (msgq->magic != MSGQ_MAGIC)
		{
			return ERROR_MSGQID_INVALID;    
		}
		msgq_infos_count = 1;
	}
	else
	{
		if(level > 0)
		{
			MSGQ_INFO_PRINTF("Should specify message queue id for detail information!\n");
			return -EINVAL;
		}
		msgq_infos_count = MAX_SHOW_MSGQ_NUM;
	}

	msgq_infos = dim_sum_mem_alloc(
		sizeof(struct msgq_info_show)*msgq_infos_count, MEM_NORMAL);
	if(msgq_infos == NULL)
	{
		ret = -ENOMEM;
		goto out;
	}
	memset(msgq_infos, 0, sizeof(struct msgq_info_show)*msgq_infos_count);

	spin_lock_irqsave(&lock_msgq_list, flags);
	list_for_each(list, &all_msgq)
	{
		struct msgq_struct *t = list_entry(list, struct msgq_struct, all_list);

		if (t == msgq || NULL == msgq)
		{
			int c = 0;
			spin_lock(&t->lock);
			msgq_infos[idx].id = (unsigned long)t;
			msgq_infos[idx].option = t->option;
			msgq_infos[idx].msg_size = t->msg_size;
			msgq_infos[idx].msg_num= t->msg_num;
			msgq_infos[idx].refcount = t->refcount.counter;
			list_for_each(list1, &t->msg_list)
			{
				c++;
			}
			msgq_infos[idx].msg_count = c;
			c = 0;
			list_for_each(list1, &t->free_list)
			{
				c++;
			}
			msgq_infos[idx].free_count = c;
			c = 0;
			list_for_each(list1, &t->wait_send_list)
			{
				c++;
			}
			msgq_infos[idx].wait_send = c;
			c = 0;
			list_for_each(list1, &t->wait_recv_list)
			{
				c++;
			}
			msgq_infos[idx].wait_recv = c;			
			spin_unlock(&t->lock);
			idx++;
			if (idx >= msgq_infos_count)
				break;
		}
	}
	spin_unlock_irqrestore(&lock_msgq_list, flags);

	if(msgq && level > 0)
	{
		if(msgq_infos->wait_send)
		{
			int c = 0;
			send_waits = dim_sum_mem_alloc(
				sizeof(struct msgq_task_wait)*msgq_infos->wait_send, MEM_NORMAL);
			if(send_waits == NULL)
			{
				ret = -ENOMEM;
				goto out;
			}
			memset(send_waits, 0, sizeof(struct msgq_task_wait)*msgq_infos->wait_send);
			if (msgq->magic != MSGQ_MAGIC)
			{
				ret = ERROR_MSGQID_INVALID;
				goto out;
			}
			spin_lock_irqsave(&msgq->lock, flags);
			list_for_each(list1, &msgq->wait_send_list)
			{
				struct msgq_wait_send_item *wait_item = list_entry(list1, struct msgq_wait_send_item, list);
				send_waits[c].id = (unsigned long)wait_item->task;
				memcpy(send_waits[c].name, wait_item->task->name, TASK_NAME_LEN);
				c++;
				if(c >= msgq_infos->wait_send)
					break;
			}
			msgq_infos->wait_send = c;
			spin_unlock_irqrestore(&msgq->lock, flags);
		}

		if(msgq_infos->wait_recv)
		{
			int c = 0;
			recv_waits = dim_sum_mem_alloc(
				sizeof(struct msgq_task_wait)*msgq_infos->wait_recv, MEM_NORMAL);
			if(recv_waits == NULL)
			{
				ret = -ENOMEM;
				goto out;
			}
			memset(recv_waits, 0, sizeof(struct msgq_task_wait)*msgq_infos->wait_recv);
			if (msgq->magic != MSGQ_MAGIC)
			{
				ret = ERROR_MSGQID_INVALID;
				goto out;
			}
			spin_lock_irqsave(&msgq->lock, flags);
			list_for_each(list1, &msgq->wait_recv_list)
			{
				struct msgq_wait_recv_item *wait_item = list_entry(list1, struct msgq_wait_recv_item, list);
				recv_waits[c].id = (unsigned long)wait_item->task;
				memcpy(recv_waits[c].name, wait_item->task->name, TASK_NAME_LEN);
				c++;
				if(c >= msgq_infos->wait_recv)
					break;
			}
			msgq_infos->wait_recv = c;
			spin_unlock_irqrestore(&msgq->lock, flags);
		}
	}

	if(msgq && level > 1 && msgq_infos->msg_count)
	{
		int c = 0;
		char *data = NULL;
		msg_detail = dim_sum_mem_alloc(
			sizeof(struct msgq_msg_detail)*msgq_infos->msg_count, MEM_NORMAL);
		if(msg_detail == NULL)
		{
			ret = -ENOMEM;
			goto out;
		}
		memset(msg_detail, 0, sizeof(struct msgq_msg_detail)*msgq_infos->msg_count);

		msg_data = dim_sum_mem_alloc(
			msgq->msg_size*msgq_infos->msg_count, MEM_NORMAL);
		if(msg_data == NULL)
		{
			ret = -ENOMEM;
			goto out;
		}
		memset(msg_data, 0, msgq->msg_size*msgq_infos->msg_count);
		data = msg_data;
		if (msgq->magic != MSGQ_MAGIC)
		{
			ret = ERROR_MSGQID_INVALID;
			goto out;
		}
		spin_lock_irqsave(&msgq->lock, flags);
		list_for_each(list1, &msgq->msg_list)
		{
			struct msgq_node *msg_item = list_entry(list1, struct msgq_node, list);
			msg_detail[c].addr = msg_item->data;
			msg_detail[c].data = data;
			msg_detail[c].size = msg_item->size;
			memcpy(msg_detail[c].data, msg_item->data, msg_item->size);
			data = data + msgq->msg_size;
			c++;
			if(c >= msgq_infos->msg_count)
				break;
		}
		msgq_infos->msg_count = c;
		spin_unlock_irqrestore(&msgq->lock, flags);
	}

	MSGQ_INFO_PRINTF("ID           OPTIONS     SIZE     NUM     REF     MSG     FREE    WAITRECV     WAITSEND\n");
	for (i = 0; i < idx; i++)
	{
		MSGQ_INFO_PRINTF("0x%-10lx 0x%-8x  %-5d    %-5d   %-5d   %-5d   %-5d   %-5d        %-5d\n",
			msgq_infos[i].id,
			msgq_infos[i].option,
			msgq_infos[i].msg_size,
			msgq_infos[i].msg_num,
			msgq_infos[i].refcount,
			msgq_infos[i].msg_count,
			msgq_infos[i].free_count,
			msgq_infos[i].wait_recv,
			msgq_infos[i].wait_send
		);
		MSGQ_INFO_PRINTF("\n");
	}
	if(recv_waits)
	{
		MSGQ_INFO_PRINTF("WAIT RECV MESSAGE TASKS:\n");
		for (i = 0; i < msgq_infos->wait_recv; i++)
		{
			MSGQ_INFO_PRINTF("0x%lx[%s]\n", recv_waits[i].id, recv_waits[i].name);
		}
	}
	if(send_waits)
	{
		MSGQ_INFO_PRINTF("WAIT SEND MESSAGE TASKS:\n");
		for (i = 0; i < msgq_infos->wait_send; i++)
		{
			MSGQ_INFO_PRINTF("0x%lx[%s]\n", send_waits[i].id, send_waits[i].name);
		}
	}
	if(msg_detail && msg_data)
	{
		MSGQ_INFO_PRINTF("MESSAGE DETAILS:\n");
		for (i = 0; i < msgq_infos->msg_count; i++)
		{
			MSGQ_INFO_PRINTF("Message[%d] Addr : 0x%p, Size : 0x%x",i, msg_detail[i].addr, msg_detail[i].size);
			for (j = 0; j < msg_detail[i].size; j++)
			{
				if ((j % 16) == 0)
					MSGQ_INFO_PRINTF ("\n");
				if ((j % 4) == 0)
					MSGQ_INFO_PRINTF (" 0x");

				MSGQ_INFO_PRINTF ("%02x", msg_detail[i].data[j] & 0xff);
			}
			MSGQ_INFO_PRINTF ("\n");
		}
	}

out:
	if(msg_data)
		dim_sum_mem_free(msg_data);
	if(msg_detail)
		dim_sum_mem_free(msg_detail);
	if(recv_waits)
		dim_sum_mem_free(recv_waits);
	if(send_waits)
		dim_sum_mem_free(send_waits);
	if(msgq_infos)
		dim_sum_mem_free(msgq_infos);
	return ret;
}



