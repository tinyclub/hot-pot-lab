

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_arch_register.h"
#include "sh_register.h"
#include "sh_utils.h"
#include "sh_command.h"
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

/**********************************************************
 *                       Implements                       *
 **********************************************************/
static int sh_read_regs_cmd(int argc, char *argv[])
{
	int task_id = 0;
	int i;
	unsigned long reg_val = 0;

	if (argc == 1)
	{
		sh_show_cmd_usage(argv[0]);
		return RET_ERROR;
	}

	task_id = strtoul(argv[1], NULL, 0);
	if (task_id == 0)
	{
		SH_PRINTF("Invalid task id.\n");
		return RET_ERROR;
	}

	if (!is_valid_task(task_id))
	{
		SH_PRINTF("No such task <0x%x>.\n", task_id);
		return RET_ERROR;
	}

	if (!task_is_suspend(task_id))
	{
		SH_PRINTF("Task <0x%x> has not been suspended.\n", task_id);
		return RET_ERROR;
	}

	if (argc == 2)
	{		
		SH_PRINTF("All key registers of task <name: %s> <ID: 0x%x>:\n", 
			get_task_name(task_id), task_id);
		
		for (i = 0; i < ARCH_REGS_NUM; i++)
		{
			reg_val = sh_get_register(task_id, i);
			SH_PRINTF("%-4s: 0x%08lx\t", sh_get_reg_name(i), reg_val);
			if (i % 3 == 2)
			{
				SH_PRINTF("\n");
			}
		}

		if (i % 3 != 0)
		{
			SH_PRINTF("\n");
		}
	}
	else
	{
		int regno;
		int valid_regs = 0;
		
		SH_PRINTF("Partial key registers of task <name: %s> <ID: 0x%x>:\n", 
			get_task_name(task_id), task_id);

		for (i = 2; i < argc; i++)
		{
			regno = sh_get_reg_no(argv[i]);
			if (regno < 0)
			{
				continue;
			}

			reg_val = sh_get_register(task_id, regno);
			SH_PRINTF("%-4s: 0x%08lx\t", sh_get_reg_name(regno), reg_val);

			if (valid_regs % 3 == 2)
			{
				SH_PRINTF("\n");
			}
			valid_regs++;
		}
		
		if (valid_regs % 3 != 0)
		{
			SH_PRINTF("\n");
		}
	}
	
	SH_PRINTF("\n");
	
	return RET_OK;
}

static int sh_write_regs_cmd(int argc, char *argv[])
{
	return RET_OK;
}

unsigned long sh_get_register(int task_id, int regno)
{
	return arch_get_register(task_id, regno);
}

const char *sh_get_reg_name(int regno)
{
	return arch_get_reg_name(regno);
}

int sh_get_reg_no(const char *reg_name)
{
	return arch_get_reg_no(reg_name);
}

void _initialize_register_cmds(void)
{
	register_shell_command("read-regs", sh_read_regs_cmd, 
		"Read and display the value of registers", 
		"read-regs pid [reg0] [reg1] ...", 
		"This command lists the value of all registers of specified registers. \n\t"
		"Argument \"reg\" is register name.",
		sh_noop_completer);

	register_alias_command("dregs", "read-regs");

	register_shell_command("modify-regs", sh_write_regs_cmd, 
		"Modify the value of registers", 
		"write-regs pid [reg0] [reg1] ...", 
		"This command modifies the value of all registers of specified registers."
		"Argument \"reg\" is register name.",
		sh_noop_completer);

	register_alias_command("mregs", "write-regs");

	return;
}
 

