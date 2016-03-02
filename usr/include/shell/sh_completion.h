
#ifndef _SH_COMPLETION_H_
#define _SH_COMPLETION_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/
/* 定义字符串补齐接口处理结果 */
#define HANDLE_COMPLETION_FAILED	-1
#define HANDLE_COMPLETION_DONE		0

/* 定义补齐字符串类型 */
#define UNKNOWN_COMPLETION			-1	/* 未定义补齐字符串类型 */
#define CMD_NAME_COMPLETION			0	/* shell命令名称补齐字符串 */
#define CMD_FILENAME_ARG_COMPLETION	1	/* shell命令文件名参数字符串 */
#define CMD_LOCATION_ARG_COMPLETION	2	/* shell命令地址参数字符串 */

#define MAX_SHOW_COMPLETIONS_NUM	60	/* 最大显示补齐字符串数目，每行5列，共计12行 */

#define MAX_COMPLETION_STR_LEN		256	/* 最大补齐字符串长度 */

/* 
 * 定义shell命令参数补齐执行函数类型
 * 参数1 -- 已输入的字符串
 * 参数2 -- shell命令参数补齐字符串数组地址
 */
typedef int (sh_cmd_arg_completer_t)(char *, char **[]);

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
/* shell输入字符串补齐处理接口 */
extern void sh_completion_handler(char *input, int *cursor, int *len);
/* shell命令文件名或目录名参数补齐处理接口 */
extern int sh_filename_completer(char *input, char **completions[]);
/* shell命令目录名参数补齐处理接口 */
extern int sh_directory_completer(char *input, char **completions[]);
/* shell命令地址参数补齐处理接口 */
extern int sh_location_completer(char *input, char **completions[]);
/* shell命令参数补齐空处理接口 */
extern int sh_noop_completer(char *input, char **completions[]);
/* shell命令命令参数补齐空处理接口 */
extern int sh_command_completer(char *input, char **completions[]);

#endif

