

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <dim-sum/task.h>
#include <sys/wait.h>

#include "sh_command.h"
#include "sh_utils.h"
#include "sh_readline.h"
#include "sh_history.h"
#include "sh_filesystem.h"
#include "sh_network.h"
#include "sh_symbol.h"
#include "sh_task.h"
#include "sh_memory.h"
#include "sh_register.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/

/**********************************************************
 *                    Global Variables                    *
 **********************************************************/

/**********************************************************
 *                    Static Variables                    *
 **********************************************************/
static char *shell_prompt = "[dim-sum@hot pot]# ";	/* shell命令行提示符 */
static struct sh_cmd_t *sh_cmd_list = NULL;			/* shell命令列表 */
static int sh_cmd_count = 0;						/* shell命令个数 */
	

static void sort_sh_cmd_list(void)
{
	int i, j;
	struct sh_cmd_t *p = NULL, *q = NULL, *prev = NULL;

	for (i = 0; i < sh_cmd_count - 1; i++)
	{
		p	 = sh_cmd_list;
		prev = NULL;
		
		for (j = 0; j < sh_cmd_count - i - 1; j++)
		{
			q = p->next;
			if (q && (strcmp(p->name, q->name) > 0))
			{
				p->next = q->next;
				q->next = p;
				
				if (prev)
				{
					prev->next = q;
				}
				else
				{
					sh_cmd_list = q;
				}

				p = q;
			}
			
			prev = p;
			p = p->next;
		}
	}
}


void sh_show_cmd_usage(char *cmd_name)
{
	struct sh_cmd_t *p_cmd = NULL;

	p_cmd = lookup_shell_cmd(cmd_name);
	if (p_cmd)
	{
		SH_PRINTF("USAGE: %s\n", p_cmd->usage);
	}

	return;
}

void sh_show_prompt(void)
{
	SH_PRINTF("%s", shell_prompt);
	return;
}

struct sh_cmd_t *get_sh_cmd_list(void)
{
	return sh_cmd_list;
}

int get_sh_cmd_count(void)
{
	return sh_cmd_count;
}

static int del_shell_cmd(char *name)
{
	struct sh_cmd_t *cur_sh_cmd = NULL;
	struct sh_cmd_t *del_sh_cmd = NULL;

	cur_sh_cmd = sh_cmd_list;

	if (cur_sh_cmd && (strcmp(cur_sh_cmd->name, name) == 0))
	{
		sh_cmd_list = sh_cmd_list->next;
		sh_cmd_count--;
		free(cur_sh_cmd);
		return 0;
	}
	else
	{
		while (cur_sh_cmd && cur_sh_cmd->next)
		{
			if (strcmp(cur_sh_cmd->next->name, name) == 0)
			{
				del_sh_cmd		 = cur_sh_cmd->next;
				cur_sh_cmd->next = cur_sh_cmd->next->next;
				sh_cmd_count--;
				free(del_sh_cmd);
				return 0;
			}

			cur_sh_cmd = cur_sh_cmd->next;
		}
		
		return -1;
	}
}

static struct sh_cmd_t *add_cmd(char *name, sh_cmd_func_t *func, 
									char *help,	char *usage, char *desc, 
									sh_cmd_arg_completer_t *completer,
									struct sh_cmd_t *alias)
{
	struct sh_cmd_t *sh_cmd = NULL;

	sh_cmd = malloc(sizeof(struct sh_cmd_t));
	if (sh_cmd == NULL)
	{
		return NULL;
	}

	/* 先删除同名shell命令结构体 */
	del_shell_cmd(name);

	sh_cmd->next		= NULL;
	sh_cmd->name		= name;
	sh_cmd->func		= func;
	sh_cmd->help		= help;
	sh_cmd->usage		= usage;
	sh_cmd->desc		= desc;
	sh_cmd->completer	= completer;
	sh_cmd->alias		= alias;

	if (sh_cmd_list == NULL)
	{
		sh_cmd_list = sh_cmd;
	}
	else
	{
		sh_cmd->next = sh_cmd_list;
		sh_cmd_list  = sh_cmd;
	}

	sh_cmd_count++;

	return sh_cmd;
}

static struct sh_cmd_t *add_shell_cmd(char *name, sh_cmd_func_t *func,
											char *help,	char *usage, char *desc, 
											sh_cmd_arg_completer_t *completer)
{
	return add_cmd(name, func, help, usage, desc, completer, NULL);
}

static struct sh_cmd_t *add_alias_cmd(char *alias_name, char *old_name)
{
	struct sh_cmd_t *alias_cmd = NULL;
	struct sh_cmd_t *old_cmd	 = NULL;

	old_cmd = lookup_shell_cmd(old_name);
	if (old_cmd == NULL)
	{
		del_shell_cmd(alias_name);
		return NULL;
	}

	alias_cmd = add_cmd(alias_name, old_cmd->func, old_cmd->help, old_cmd->usage, 
						old_cmd->desc, old_cmd->completer, old_cmd);

	return alias_cmd;
}

static int is_alias_cmd(struct sh_cmd_t *sh_cmd)
{
	return sh_cmd->alias ? 1 : 0 ;
}

struct sh_cmd_t *register_shell_command(char *name, sh_cmd_func_t *func,
													char *help,	char *usage, char *desc, 
													sh_cmd_arg_completer_t *completer)
{
	struct sh_cmd_t *sh_cmd = NULL;
	
	if (name == NULL || func == NULL || help == NULL || desc == NULL)
	{
		return NULL;
	}

	if ((strlen(name) >= MAX_CMD_NAME)
		|| (strlen(help) >= MAX_CMD_HELP)
		|| (strlen(desc) >= MAX_CMD_DESC))
	{
		return NULL;
	}

	if (sh_cmd_count >= MAX_CMD_NUM)
	{
		return NULL;
	}

	sh_cmd = add_shell_cmd(name, func, help, usage, desc, completer);
	if (!sh_cmd)
	{
		return NULL;
	}

	return sh_cmd;
}

struct sh_cmd_t *register_alias_command(char *alias_name, char *old_name)
{
	struct sh_cmd_t *sh_cmd = NULL;

	if (alias_name == NULL || old_name == NULL)
	{
		return NULL;
	}

	if (strlen(alias_name) >= MAX_CMD_NAME)
	{
		return NULL;
	}

	if (sh_cmd_count >= MAX_CMD_NUM)
	{
		return NULL;
	}
	
	sh_cmd = add_alias_cmd(alias_name, old_name);
	if (!sh_cmd)
	{
		return NULL;
	}

	return sh_cmd;
}

int unregister_shell_command(char *name)
{
	if (!del_shell_cmd(name))
	{
		return -1;
	}

	return 0;
}

struct sh_cmd_t *lookup_shell_cmd(char *name)
{
	struct sh_cmd_t *cur_sh_cmd = NULL;

	cur_sh_cmd = sh_cmd_list;

	while (cur_sh_cmd)
	{
		if (strcmp(cur_sh_cmd->name, name) == 0)
		{
			return cur_sh_cmd;
		}

		cur_sh_cmd = cur_sh_cmd->next;
	}
	
	return NULL;
}

static int get_sh_cmdline(char *cmdline)
{
	int len = 0;
	
	len = readline_echo(cmdline, MAX_CMD_LEN, shell_prompt);

	if (cmdline != NULL)
	{
		add_one_history(cmdline);
	}

	return len;
}

int parse_cmd_args(char *cmd, char **argv[])
{
	char *p 	= cmd;
	char *token = NULL;
	int argc	= 1;
	int arg_idx = 0;

	if ((cmd == NULL) || (*cmd == '\0'))
	{
		*argv = NULL;
		return 0;
	}

	/* 解析shell命令参数个数，含shell命令名 */
	while (*p != '\0')
	{
		if (isspace(*p))
		{
			argc++;
			while (isspace(*(++p)));
		}

		p++;
	}

	/* 解析shell命令参数，含shell命令名 */
	if (argc != 0)
	{
		*argv = (char **)malloc((argc + 1)* sizeof(char *));
		if (*argv == NULL)
		{
			return 0;
		}
		memset(*argv, 0x0, (argc + 1));

		/* 解析shell命令名 */
		token = strtok(cmd, " ");
		(*argv)[arg_idx++] = token;

		/* 解析shell参数 */
		while (token != NULL)
		{
			token = strtok(NULL, " ");
			(*argv)[arg_idx++] = token;
		}
	}

	return argc;
}

static int sh_cmd_run_task(void *data)
{
	struct sh_cmd_run_t *sh_cmd_run = (struct sh_cmd_run_t *)data;
	struct sh_cmd_t *sh_cmd			= NULL; 
	int ret;

	sh_cmd = sh_cmd_run->sh_cmd;
	ret = sh_cmd->func(sh_cmd_run->argc, sh_cmd_run->argv);
	
	return ret;
}

int sh_cmd_loop(void)
{
	char cmd_line[MAX_CMD_LEN] = {0x0, };
	char **argv = NULL;
	int argc;
	struct sh_cmd_t *sh_cmd = NULL; 
	struct sh_cmd_run_t *sh_cmd_run = NULL;
#if 1
	TaskId child_id;
#else
    int ret;
#endif

	while (1)
	{
		/* 获取命令行输入 */
		get_sh_cmdline(cmd_line);
		if (cmd_line[0] == '\0')
		{
			continue;
		}

		/* 解析shell命令 */
		argc = parse_cmd_args(cmd_line, &argv);

		/* 查找shell命令 */
		sh_cmd = lookup_shell_cmd(cmd_line);
		if (sh_cmd == NULL)
		{
			SH_PRINTF("Can not find shell command <%s> !\n", cmd_line);
			continue;
		}

		sh_cmd_run 				= malloc(sizeof(struct sh_cmd_run_t));
		sh_cmd_run->argc		= argc;
		sh_cmd_run->argv		= argv;
		sh_cmd_run->sh_cmd		= sh_cmd;

#if 1
		child_id = SysTaskCreate(sh_cmd_run_task, (void *)sh_cmd_run, sh_cmd->name, 20, NULL, 0);
		wait4(child_id, NULL, 0, NULL);

		/* 释放命令参数分配内存 */
		if (argv != NULL)
		{
			free(argv);
			argv = NULL;
		}

		if (sh_cmd_run)
			free(sh_cmd_run);
#else
		/* 执行shell命令 */
		ret = sh_cmd->func(argc, argv);

		/* 释放命令参数分配内存 */
		if (argv != NULL)
		{
			free(argv);
			argv = NULL;
		}

		if (ret == RET_OK)
		{
			continue;
		}
		else if (ret == RET_EXIT)
		{
			break;
		}
#endif
	}

	return 0;
}

static int sh_help_cmd(int argc, char *argv[])
{
	struct sh_cmd_t *p_cmd = NULL;
	
	if (argc > 2)
	{
		SH_PRINTF("Too many argument.\n");
		sh_show_cmd_usage(argv[0]);
		return RET_ERROR;
	}
	
	if (argc == 1)
	{
		int show_num = 0;

		for (p_cmd = sh_cmd_list; p_cmd; p_cmd = p_cmd->next)
		{
			if (show_num != 0 && (show_num % MAX_SHOW_SHELL_CMDS_NUM == 0))
			{
				int show_more;
				show_more = sh_show_more();
				SH_PRINTF("\n");
				if (!show_more)
				{
					break;
				}
			}

			if (is_alias_cmd(p_cmd))
			{
				continue;
			}

			
			SH_PRINTF("%-30.30s ------ %s\n", p_cmd->usage, p_cmd->help);
			show_num++;
		}
		SH_PRINTF("\n");
		
		return RET_OK;
	}

	p_cmd = lookup_shell_cmd(argv[1]);
	if (p_cmd == NULL)
	{
		SH_PRINTF("No such shell command <%s>. Try to press \'help\'.\n", argv[1]);
		return RET_OK;
	}

	if (is_alias_cmd(p_cmd))
	{
		SH_PRINTF("Shell command <%s> is an alias command, please references command <%s>.\n",
			p_cmd->name, p_cmd->alias->name);
		return RET_OK;
	}

	SH_PRINTF("NAME\n");
	SH_PRINTF("\t%s\n", p_cmd->name);
	SH_PRINTF("\nUSAGE\n");
	SH_PRINTF("\t%s\n", p_cmd->usage);
	SH_PRINTF("\nDESCRIPTION\n");
	SH_PRINTF("\t%s\n", p_cmd->desc);
	SH_PRINTF("\n");

	return RET_OK;
}

void _initialize_comm_cmds(void)
{
	register_shell_command("help", sh_help_cmd, 
		"Print a list of routines.", 
		"help [command]", 
		"This command prints a list of the name of commonly used routines.", 
		sh_command_completer);

	register_alias_command("?", "help");
	
	return;
}

void initialize_shell_cmds(void)
{
	/* 初始化通用命令 */
	_initialize_comm_cmds();

	/* 初始化文件系统操作命令 */
	_initialize_filesystem_cmds();

	/* 初始化网络操作命令 */
	_initialize_network_cmds();

	/* 初始化符号操作命令 */
	_initialize_symbol_cmds();

	/* 初始化任务操作命令 */
	_initialize_task_cmds();

	/* 初始化内存操作命令 */
	_initialize_memory_cmds();

	/* 初始化寄存器操作命令 */
	_initialize_register_cmds();

	/* shell命令列表进行排序 */
	sort_sh_cmd_list();
	
	return;
}

