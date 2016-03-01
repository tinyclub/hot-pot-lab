

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_utils.h"
#include "sh_command.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern int ls_main(int argc, char *argv[]);
extern int cd_main(int argc, char *argv[]);
extern int pwd_main(int argc, char *argv[]);
extern int mkdir_main(int argc, char *argv[]);
extern int creat_main(int argc, char *argv[]);
extern int xby_test_cmd(int argc, char *argv[]);

/**********************************************************
 *                    Global Variables                    *
 **********************************************************/

/**********************************************************
 *                    Static Variables                    *
 **********************************************************/

/**********************************************************
 *                       Implements                       *
 **********************************************************/
static int sh_ls_cmd(int argc, char *argv[])
{
	return ls_main(argc, argv);
}

static int sh_cd_cmd(int argc, char *argv[])
{
	return cd_main(argc, argv);
}

static int sh_pwd_cmd(int argc, char *argv[])
{
	return pwd_main(argc, argv);
}

static int sh_mkdir_cmd(int argc, char *argv[])
{
	return mkdir_main(argc, argv);
}

static int sh_creat_cmd(int argc, char *argv[])
{
	return creat_main(argc, argv);
}

void _initialize_filesystem_cmds(void)
{
	register_shell_command("ls", sh_ls_cmd, 
		"List the file names of directory", 
		"ls [dirname|filename]", 
		"This command lists the file names of current directory \n\t"
		"or specified directory.", 
		sh_filename_completer);

	register_shell_command("cd", sh_cd_cmd,
		"Change the current directory", 
		"cd [dirname]", 
		"This command changes the specified directory as current directory.",
		sh_directory_completer);

	register_shell_command("pwd", sh_pwd_cmd,
		"Print the current directory", 
		"pwd", 
		"This command prints the current directory.",
		sh_noop_completer);

	register_shell_command("mkdir", sh_mkdir_cmd,
		"Create a new directory", 
		"mkdir dirname", 
		"This command creates a new directory.",
		sh_noop_completer);

	register_shell_command("creat", sh_creat_cmd,
		"Create a new file", 
		"creat filename", 
		"This command creates a new file.",
		sh_noop_completer);

	register_shell_command("xby_test", xby_test_cmd,
			"xby_test command", 
			"xby_test command", 
			"xby_test command",
			sh_filename_completer);
	
	return;
}

