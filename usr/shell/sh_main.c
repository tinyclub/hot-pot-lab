
/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <dim-sum/task.h>

#include "sh_main.h"
#include "sh_command.h"
#include "sh_readline.h"

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
/* shell标题 */
static char *shell_title[] =
{
	"                                                    ",
	"##############################################",
	"#                                            #",
	"#  *   *   ***   *****  ****    ***   *****  #",
	"#  *   *  *   *    *    *   *  *   *    *    #",
	"#  *   *  *   *    *    *   *  *   *    *    #",
	"#  *****  *   *    *    ****   *   *    *    #",
	"#  *   *  *   *    *    *      *   *    *    #",
	"#  *   *  *   *    *    *      *   *    *    #",
	"#  *   *   ***     *    *       ***     *    #",
	"#                                            #",
	"##############################################",
    "                                              ",
	NULL
};

/**********************************************************
 *                       Implements                       *
 **********************************************************/
static void sh_show_title(void)
{
	unsigned int title_index	= 0;
	char *sh_title				= shell_title[title_index++];

	while (sh_title)
	{
		SH_PRINTF("\t%s\n", sh_title);
		sh_title = shell_title[title_index++];
	}

	return;
}

void initialize_shell(void)
{
	sh_show_title();
	initialize_shell_cmds();
	return;
}

int shell_task(void *argv)
{
	int ret = 0;

	ret = sh_cmd_loop();
	
	return ret;
}

int start_shell(char *str, int prio, void *arg)	
{
	/* 初始化shell */
	initialize_shell();

	/* 创建线程运行shell */
	return SysTaskCreate(shell_task, arg, str, prio, NULL, 0);	
}

