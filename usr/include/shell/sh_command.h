
#ifndef _SH_COMMAND_H_
#define _SH_COMMAND_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_completion.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/
#define MAX_CMD_LEN		256			/* 最大命令长度 */
#define MAX_CMD_NUM		256			/* 最大命令个数 */
#define MAX_CMD_NAME	128			/* 最长命令名称 */
#define MAX_CMD_HELP	128			/* 最长命令帮助信息 */
#define MAX_CMD_DESC	1024		/* 最长命令描述信息 */
	
/* 定义shell命令执行函数返回值 */
#define RET_ERROR		-1			/* 执行shell命令错误 */
#define RET_OK			0			/* 执行shell命令成功 */

/* 定义最大显示shell命令数目 */
#define MAX_SHOW_SHELL_CMDS_NUM		20

/* 
 * 定义shell命令执行函数类型 
 * 参数1 -- 参数个数
 * 参数2 -- 参数字符串数组
 */
typedef int (sh_cmd_func_t)(int, char *[]);

/* 定义shell命令结构体 */
struct sh_cmd_t
{
	/* 下一条shell命令指针 */
	struct sh_cmd_t			*next;		

	/* shell命令名称 */
	char					*name;		

	/* shell命令执行函数指针 */
	sh_cmd_func_t			*func;		

	/* shell命令帮助信息 */
	char					*help;

	/* shell命令使用说明 */
	char					*usage;

	/* shell命令描述信息 */
	char					*desc;		

	/* shell命令参数补齐函数指针 */
	sh_cmd_arg_completer_t	*completer; 

	/* shell命令别名 */
	struct sh_cmd_t			*alias;	
};

/* 定义shell命令执行线程参数结构体 */
struct sh_cmd_run_t
{
	struct sh_cmd_t	*sh_cmd;	/* shell命令结构体 */
	int				argc;		/* 参数个数 */
	char			**argv;		/* 参数字符串数组 */
};

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern struct sh_cmd_t *register_shell_command(char *name, sh_cmd_func_t *func,
														char *help, char *usage, char *desc, 
														sh_cmd_arg_completer_t *completer); /* 注册shell命令 */
extern struct sh_cmd_t *register_alias_command(char *alias_name, char *old_name);	/* 注册别名命令 */
extern int unregister_shell_command(char *name);						/* 注销shell命令 */
extern void initialize_shell_cmds(void);									/* 初始化shell命令 */
extern int sh_cmd_loop(void);												/* shell命令事件循环 */
extern struct sh_cmd_t *get_sh_cmd_list(void);								/* 获取shell命令列表 */
extern int get_sh_cmd_count(void);										/* 获取shell命令数目 */
extern void sh_show_prompt(void);										/* 打印shell命令行提示符 */
extern int parse_cmd_args(char *cmd, char **argv[]);						/* 解析shell命令参数，含shell命令名 */
extern struct sh_cmd_t *lookup_shell_cmd(char *name);					/* 根据shell命令名称查找命令结构体 */
extern void sh_show_cmd_usage(char *cmd_name);						/* 打印shell命令使用说明 */

#endif

