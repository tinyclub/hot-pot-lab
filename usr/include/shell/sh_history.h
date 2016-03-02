
#ifndef _SH_HISTORY_H_
#define _SH_HISTORY_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/
#define MAX_HISTORY	32	/* 历史命令记录最大数目 */

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern void add_one_history(char *cmd);	/* 增加一条历史命令记录 */
extern void destroy_all_history(void);		/* 释放所有历史命令记录 */
extern char *get_prev_history(void);		/* 获取上一条历史命令记录 */
extern char *get_next_history(void);		/* 获取下一条历史命令记录 */

#endif
