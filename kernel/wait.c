#include <dim-sum/wait.h>
#include <base/list.h>


void __init_waitqueue_head(wait_queue_head_t *q, const char *name, struct lock_class_key *key)
{
	spin_lock_init(&q->lock);
	lockdep_set_class_and_name(&q->lock, key, name);
	INIT_LIST_HEAD(&q->task_list);
}

/**
 * add_wait_queue函数把一个非互斥进程插入等待队列链表的第一个位置
 */
void fastcall add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	unsigned long flags;

	wait->flags &= ~WQ_FLAG_EXCLUSIVE;
	spin_lock_irqsave(&q->lock, flags);
	__add_wait_queue(q, wait);
	spin_unlock_irqrestore(&q->lock, flags);
}


/**
 * 从等待队列链表中删除一个进程
 */
void fastcall remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	unsigned long flags;

	spin_lock_irqsave(&q->lock, flags);
	__remove_wait_queue(q, wait);
	spin_unlock_irqrestore(&q->lock, flags);
}

