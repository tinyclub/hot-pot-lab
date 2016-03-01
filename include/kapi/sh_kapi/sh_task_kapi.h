
#ifndef _SH_TASK_KAPI_H_
#define _SH_TASK_KAPI_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern int sh_ker_is_valid_task(int task_id);
extern int sh_ker_get_init_task_id(void);
extern int sh_ker_get_next_task_id(int prev_task_id);
extern char *sh_ker_get_task_name(int task_id);
extern unsigned long sh_ker_get_task_entry(int task_id);
extern int sh_ker_get_task_prio(int task_id);
extern int sh_ker_get_task_state(int task_id);
extern int sh_ker_suspend_task(int task_id);
extern int sh_ker_resume_task(int task_id);
extern void sh_ker_dump_task(int task_id);
struct task_struct *sh_ker_get_task(int task_id);

#endif

