
#ifndef _SH_TASK_H_
#define _SH_TASK_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/
/* 定义任务状态 */
#define UNKNOWN_STATE	-1
#define TASK_RUNNING	0
#define TASK_WAIT		1
#define TASK_SUSPEND	2
#define TASK_STOPPED	4
#define TASK_DEADED		8
#define TASK_ZOMBIE		16
#define TASK_INIT		128

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern void _initialize_task_cmds(void);
extern int is_valid_task(int task_id);
extern int task_is_suspend(int task_id);
extern char *get_task_name(int task_id);

#endif


