#include <base/string.h>
#include <dim-sum/sched.h>
#include <dim-sum/printk.h>

#include <kapi/dim-sum/task.h>


#define DEF_TASK_STACK_SIZE (THREAD_SIZE)
#define DEF_STACK_ALIGN_SIZE 8
#define DEF_STACK_ALIGN_MASK (DEF_STACK_ALIGN_SIZE - 1)

int TaskStdGet( TaskId taskid,int stdFd)
{
	return 0;
}

int DefStdGet( int stdFd)
{
	return 0;
}

typedef  int (*TASK_FUNCTPTR)(void *data) ;
TaskId SysTaskCreate(int (*TaskFn)(void *data),
			void *pData,
			char *strName,
			int prio,
			unsigned long *pStackBase,
			int iStackSize
		)
{
 	struct thread_param_t param;
	TaskId ret = (TaskId)NULL;

	if(TaskFn==(TASK_FUNCTPTR)NULL)
	{
		printk("Create task error: TaskFn is NULL\n");
		return ret;
	}

	if ( ((pStackBase) && (!iStackSize)) ||
	     ( ((unsigned long)pStackBase) & DEF_STACK_ALIGN_MASK) )
	{
		printk("Create task error: Input Error Argument\n");
		return ret;
	}

	if (prio < 0 || prio >= 254)
	{
		printk("Create task error: prio is error\n");
		return ret;
	}

	if (strName)
	{
		int len = strlen(strName);
		if (len >= sizeof(param.name))
			len = sizeof(param.name) - 1;
		
		strncpy(param.name, strName, len);
		param.name[len] = 0;
	}
	else
	{
		strncpy(param.name, "[NULL]", 7);
	}
	param.prio  = prio;
	param.stack = pStackBase;
	if (!iStackSize)
	{
		param.stack_size = DEF_TASK_STACK_SIZE;
	}
	else
	{
		if (pStackBase)
		{
			param.stack_size = iStackSize;
		}
		else
		{
			param.stack_size = (iStackSize + DEF_STACK_ALIGN_MASK) & (~DEF_STACK_ALIGN_MASK);
		}
	}
	param.func = TaskFn;
	param.data = pData;
	ret = (TaskId)CreateTask(&param);
	if (ret == (TaskId)NULL)
	{
		printk("create task [%s] error\n", param.name);
	}
	else
	{
		//printk("create task [%s] OK\n", param.name);
	}
	return ret;
}
