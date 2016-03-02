
#ifndef _SH_READLINE_H_
#define _SH_READLINE_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_utils.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/
#ifndef CTRL
#define CTRL(c) (c - '@')
#endif

#define RL_BUF_SIZE			256	/* 输入字符串最大长度 */

#define INPUT_CHAR_NOECHO	0	/* 回显输入的字符 */
#define INPUT_CHAR_ECHO		1	/* 回显输入的字符 */

#define RET_READ_CHAR_DONE	0	/* 读取字符结束 */
#define RET_READ_CHAR_CONT	1	/* 继续读取字符 */

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern void readline_handler_install(void);							/* 设置shell读取字符串接口 */
extern void readline_handler_remove(void);							/* 移除shell读取字符串接口 */
extern int readline_echo(char *inputbuf, int maxsize, char *prompt);	/* 读取字符串输入并回显 */
extern int readline_noecho(char *inputbuf, int maxsize, char *prompt);/* 读取字符串输入不回显 */
extern int sh_read(char *buf, size_t size);								/* 从输入终端读取字符或字符串 */
extern void sh_write(char *buf, size_t size);								/* 向输出终端输出字符或字符串 */
extern void sh_clean_input(char *input, int *cursor, int *len);		/* 消除已输入的字符串 */
extern char sh_get_last_char(void);									/* 获取shell最后输入的字符 */
extern void sh_set_last_char(char c);									/* 设置shell最后输入的字符 */
extern void sh_trim_str(char *str);										/* 删除字符串前后空格 */
extern int sh_show_more(void);										/* 是否继续显示字符串 */
extern int sh_show_all(int num, int max_num);							/* 是否显示所有字符串 */

#endif

