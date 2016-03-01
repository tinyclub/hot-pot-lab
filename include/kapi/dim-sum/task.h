#ifndef _KAPI_PHOENIX_TASK_H
#define _KAPI_PHOENIX_TASK_H

#define TaskId unsigned long
typedef  int (*TASK_FUNCTPTR)(void *data) ;
extern TaskId SysTaskCreate(int (*TaskFn)(void *data),
			void *pData,
			char *strName,
			int prio,
			unsigned long *pStackBase,
			int iStackSize
		);

extern int SysSleep(int iMiniSec);
#endif