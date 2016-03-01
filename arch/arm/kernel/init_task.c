/**
 * init任务定义
 */
//#include <linux/module.h>

#include <dim-sum/sched.h>
//#include <linux/init_task.h>
//#include <linux/fs.h>
//#include <linux/mqueue.h>


//#include <asm/uaccess.h>
//#include <asm/pgtable.h>

//static struct signal_struct init_signals = INIT_SIGNALS(init_signals);
//static struct sighand_struct init_sighand = INIT_SIGHAND(init_sighand);
/*
 * 初始任务结构
 *
 * 确保8K对齐，这是由链接过程来保证的
 */


//union thread_union __attribute__((aligned (8192))) init_thread_stack ;
	//__init_task_data __attribute__((__aligned__(THREAD_SIZE)));// =		{ INIT_THREAD_INFO(init_task) };

//struct task_struct init_task;// = INIT_TASK(init_task);

