

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <dim-sum/task.h>
#include <sh_kapi.h>
 
#include "sh_utils.h"
#include "sh_command.h"
#include "sh_symbol.h"
#include "sh_task.h"

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
static char *task_state_text[] =
{
	"INIT",
	"RUNNING",
	"WAIT",
	"SUSPEND",
	"STOPPED",
	"DEADED",
	"ZOMBIE",
	"UNKNOWN"
};

/**********************************************************
 *                       Implements                       *
 **********************************************************/
/**
 * 是否是有效任务号
 */
int is_valid_task(int task_id)
{
	return sh_ker_is_valid_task(task_id);
}

/**
 * 获取初始任务号
 */
static int get_init_task_id(void)
{
	return sh_ker_get_init_task_id();
}

/**
 * 获取下一个任务号
 */
static int get_next_task_id(int prev_task_id)
{
	return sh_ker_get_next_task_id(prev_task_id);
}

/**
 * 获取任务名称
 */
char *get_task_name(int task_id)
{
	return sh_ker_get_task_name(task_id);
}

/**
 * 获取任务入口地址
 */
static unsigned long get_task_entry(int task_id)
{
	return sh_ker_get_task_entry(task_id);
}

/**
 * 获取任务优先级
 */
static int get_task_priority(int task_id)
{
	return sh_ker_get_task_prio(task_id);
}

/**
 * 获取任务状态
 */
static int get_task_state(int task_id)
{
	int task_state = UNKNOWN_STATE;

	task_state = sh_ker_get_task_state(task_id);

	if ((task_state & TASK_WAIT)
		&& (task_state & ~TASK_WAIT))
	{
		task_state &= ~TASK_WAIT;
	}
	
	return task_state;
}

/**
 * 打印任务栈桢
 */
static void dump_task(int task_id)
{
	sh_ker_dump_task(task_id);
	return;
}

/**
 * 暂停任务运行
 */
static int suspend_task(int task_id)
{
	return sh_ker_suspend_task(task_id);
}

/**
 * 恢复任务运行
 */
static int resume_task(int task_id)
{
	return sh_ker_resume_task(task_id);
}

/**
 * 获取任务状态字符串
 */
static char *get_task_state_text(int state)
{
	int state_idx = 0;

	switch (state)
	{
	case TASK_INIT: 
		state_idx = 0;
		break;
	case TASK_RUNNING:
		state_idx = 1;
		break;
	case TASK_WAIT: 
		state_idx = 2;
		break;
	case TASK_SUSPEND:
		state_idx = 3;
		break;
	case TASK_STOPPED: 
		state_idx = 4;
		break;
	case TASK_DEADED:
		state_idx = 5;
		break;
	case TASK_ZOMBIE:
		state_idx = 6;
		break;
	default:
		state_idx = 7;
		break;
	}

	return task_state_text[state_idx];
}

/**
 * 任务是否暂停运行
 */
int task_is_suspend(int task_id)
{
	int task_state = UNKNOWN_STATE;

	task_state = get_task_state(task_id);

	return (task_state == TASK_SUSPEND || task_state == TASK_STOPPED) ? 1 : 0;
}

/**
 * 输出任务信息标题
 */
static void show_task_info_title(void)
{
	SH_PRINTF("\n");
	SH_PRINTF("    PID       TASK_NAME       ENTRY          FUNCTION NAME       PRIO  STATE  \n");
	SH_PRINTF("---------- ---------------- ---------- ------------------------- ---- --------\n");
	return;
}

/**
 * 输出任务信息标题
 */
static void show_task_info(int task_id)
{
	char sym_name[128] = {0x0, };

	SH_PRINTF("0x%-8x %-16s 0x%-8lx %-25.25s %-4d %-8s\n",
		task_id,
		get_task_name(task_id),
		get_task_entry(task_id),
		lookup_sym_name(get_task_entry(task_id), sym_name),
		get_task_priority(task_id),
		get_task_state_text(get_task_state(task_id)));

	return;
}

static int sh_show_task_cmd(int argc, char *argv[])
{	
	if (argc > 2)
	{
		SH_PRINTF("Too many argument.\n");
		sh_show_cmd_usage(argv[0]);
		
		return RET_ERROR;
	}

	if (argc == 1)
	{
		int prev_task_id = 0, next_task_id = 0;
		
		prev_task_id = get_init_task_id();
		if (prev_task_id != 0)
		{
			show_task_info_title();
		}
		
		while (1)
		{
			show_task_info(prev_task_id);
			
			next_task_id = get_next_task_id(prev_task_id);
			if (next_task_id == 0)
			{
				SH_PRINTF("\n");
				break;
			}
			else
			{
				prev_task_id = next_task_id;
			}
		}
	}
	else
	{
		int task_id = 0;

		task_id = strtol(argv[1], NULL, 0);
		if (!is_valid_task(task_id))
		{
			SH_PRINTF("No such task <0x%x>.\n", task_id);
		}
		else
		{
			show_task_info_title();
			show_task_info(task_id);
			SH_PRINTF("\n");
		}
	}

	return RET_OK;
}

static int sh_suspend_task_cmd(int argc, char *argv[])
{
	int task_id    = 0;
	int task_state = UNKNOWN_STATE;
	int ret;
	
	if (argc != 2)
	{
		if (argc > 2)
		{
			SH_PRINTF("Too many argument.\n");
		}
		
		sh_show_cmd_usage(argv[0]);

		return RET_ERROR;
	}

	task_id = strtol(argv[1], NULL, 0);
	if (!is_valid_task(task_id))
	{
		SH_PRINTF("No such task <0x%x>.\n", task_id);
	}

	task_state = get_task_state(task_id);
	if (task_state == TASK_DEADED
		|| task_state == TASK_ZOMBIE)
	{
		SH_PRINTF("Task <0x%x> has exited.\n", task_id);
	}
	else if (task_state == TASK_STOPPED
			|| task_state == TASK_SUSPEND)
	{
		SH_PRINTF("Task <0x%x> has been suspended.\n", task_id);
	}
	else if (task_state == TASK_RUNNING
			|| task_state == TASK_WAIT)
	{
		ret = suspend_task(task_id);
		if (ret < 0)
		{
			SH_PRINTF("Suspend task <0x%x> failed.\n", task_id);
		}
		else
		{
			SH_PRINTF("Suspend task <0x%x> successful.\n", task_id);
		}
	}
	else
	{
		SH_PRINTF("Can not suspend init task <0x%x>.\n", task_id);
	}

	return RET_OK;
}

static int sh_resume_task_cmd(int argc, char *argv[])
{
	int task_id    = 0;
	int task_state = UNKNOWN_STATE;
	int ret;
	
	if (argc != 2)
	{
		if (argc > 2)
		{
			SH_PRINTF("Too many argument.\n");
		}
		
		sh_show_cmd_usage(argv[0]);

		return RET_ERROR;
	}

	task_id = strtol(argv[1], NULL, 0);
	if (!is_valid_task(task_id))
	{
		SH_PRINTF("No such task <0x%x>.\n", task_id);
	}

	task_state = get_task_state(task_id);
	if (task_state == TASK_DEADED
		|| task_state == TASK_ZOMBIE)
	{
		SH_PRINTF("Task <0x%x> has exited.\n", task_id);
	}
	else if (task_state == TASK_RUNNING
			|| task_state == TASK_WAIT)
	{
		SH_PRINTF("Task <0x%x> is running.\n", task_id);
	}
	else if (task_state == TASK_STOPPED
			|| task_state == TASK_SUSPEND)
	{
		ret = resume_task(task_id);
		if (ret < 0)
		{
			SH_PRINTF("Resume task <0x%x> failed.\n", task_id);
		}
		else
		{
			SH_PRINTF("Resume task <0x%x> successful.\n", task_id);
		}
	}
	else
	{
		SH_PRINTF("Can not resume init task <0x%x>.\n", task_id);
	}

	return RET_OK;
}

static int sh_dump_task_cmd(int argc, char *argv[])
{
	int task_id    = 0;
	
	if (argc != 2)
	{
		if (argc > 2)
		{
			SH_PRINTF("Too many argument.\n");
		}
		
		sh_show_cmd_usage(argv[0]);

		return RET_ERROR;
	}
	
	task_id = strtol(argv[1], NULL, 0);
	if (!is_valid_task(task_id))
	{
		SH_PRINTF("No such task <0x%x>.\n", task_id);
	}

	if (!task_is_suspend(task_id))
	{
		SH_PRINTF("Task <0x%x> has not been suspended.\n", task_id);
	}
	else
	{
		dump_task(task_id);
	}

	return RET_OK;
}

unsigned long global_value1 = 0x12345678;
unsigned long long global_value2 = 0x1234567887654321;

static int test_task(void *data)
{
	int i = 0;
	while (1)
	{
		SH_PRINTF("this is test task, i = %d\n", i);
		sleep(1);
	}

	return 0;
}

static int test_cmd(int argc, char *argv[])
{
	SysTaskCreate(test_task, NULL, "test_task", 15, NULL, 0);

	return RET_OK;
}

void _initialize_task_cmds(void)
{
	register_shell_command("ps", sh_show_task_cmd, 
		"Show task information", 
		"ps [pid]", 
		"This command shows the information of task in the system.",
		sh_noop_completer);

	register_shell_command("suspend", sh_suspend_task_cmd, 
		"Suspend a task", 
		"suspend pid", 
		"This command suspends a specified task.",
		sh_noop_completer);

	register_shell_command("continue", sh_resume_task_cmd, 
		"Resume a task running", 
		"continue pid", 
		"This command resumes a specified task running.",
		sh_noop_completer);
	
	register_alias_command("c", "continue");

	register_shell_command("dump", sh_dump_task_cmd, 
		"Dump stack for specified task", 
		"dump pid", 
		"This command prints the dump stack of a specified task.",
		sh_noop_completer);

	register_shell_command("test", test_cmd, 
		"test task", 
		"test", 
		"test task.",
		sh_noop_completer);

	return;
}


