#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

#include <dim-sum/semaphore.h>
#include <asm/string.h>

//#include <usr/task.h>
//#include <usr/sem.h>

#define LWIP_STK_SIZE      1024

#define LWIP_TASK_MAX    5
#define LWIP_START_PRIO  5   //so priority of lwip tasks is from 5-9 

#define SYS_MBOX_NULL   0
#define SYS_SEM_NULL    0

#ifndef TCPIP_MBOX_SIZE
#define TCPIP_MBOX_SIZE	12
#endif

#define LWIP_MAX_MSGS		100
#define LWIP_Q_SIZE 		10              /* LwIP queue size */
#define LWIP_MAX_QS 		20              /* Max. LwIP queues */

#define TaskId unsigned long
typedef TaskId sys_thread_t;

//typedef SemId sys_sem_t;
//typedef MSG_Q_ID sys_mbox_t;

typedef struct
{
	MSG_Q_ID sys_mbox;
	unsigned char valid;
}
sys_mbox_t;


typedef struct
{
	SEM_ID sys_sem;
	unsigned char valid;
}
sys_sem_t;	

typedef struct
{
	SEM_ID sys_mutex;
	unsigned char valid;
}
sys_mutex_t;

extern void dim_sum_netdev_startup(void);

#endif

