

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#include "sh_utils.h"
#include "sh_readline.h"
#include "sh_command.h"

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
static int filename_is_dir(const char * filename)
{
	struct stat c_file;   
	int hfind;
	
	/* 检查文件是否存在 */
	if(access(filename, F_OK) != 0)
	{
		return -1;
	} 

	hfind = stat(filename, &c_file);   
	if(hfind == -1) 
	{
		/* 如果出错 */
		return -1;
	}
	else if(S_ISDIR(c_file.st_mode))
	{
		/* 如果是一个目录 */
		return 1;
	}
	else
	{
		return 0;
	}
}

static void show_single_completion(char *input, int *cursor, int *len, 
											char *completions[], int completion_type)
{
	int this_cursor	= *cursor;	/* 当前光标位置 */
	int this_len	= *len;		/* 已输入字符串长度 */
	int i;

	/* 光标回退到第一个有效字符处 */
	for (i = this_cursor; i > 0; i--)
	{
		if (input[i - 1] == ' ')
		{
			break;
		}
		sh_write("\b", 1);
	}

	/* 拷贝补齐字符串到输入字符串缓存中，并输出补齐字符串 */
	strcpy((input + i), completions[0]);
	sh_write(completions[0], strlen(completions[0]));
	this_len = i + strlen(completions[0]);

	/* 
	 * 如果补齐字符串为shell命令目录名参数，补齐后多输出一个目录符号，
	 * 否则，多输出一个空格。
	 */
	if ((completion_type == CMD_FILENAME_ARG_COMPLETION) 
		&& filename_is_dir(completions[0]))
	{
		sh_write("/", 1);
		input[this_len] = '/';
	}
	else
	{
		sh_write(" ", 1);
		input[this_len] = ' ';
	}
	
	/* 填充结束符到输入字符串缓存中 */
	input[++this_len] = '\0';
	*cursor = *len = this_len;

	return;
}

static void show_all_completions(int completions_num, char *completions[], int completion_type)
{
	int i;
	char output_str[MAX_COMPLETION_STR_LEN]	= {0x0, 0};	/* 输出字符串缓存 */

	sh_write("\r\n", 2);
	for (i = 0; i < completions_num; i++)
	{
		if ((i != 0) && (i % MAX_SHOW_COMPLETIONS_NUM == 0))
		{
			int show_more;
	
			show_more = sh_show_more();
			sh_write("\r\n", 2);
			if (show_more == 0)
			{
				break;
			}
		}
	
		if (completion_type == CMD_FILENAME_ARG_COMPLETION)
		{
			char *seperater = strrchr(completions[i], '/');
			if (seperater)
			{
				char filename[MAX_COMPLETION_STR_LEN] = {0x0, 0};
				sprintf(filename, "%s", seperater + 1);
				if (filename_is_dir(completions[i]))
				{
					strcat(filename, "/");
				}
				sprintf(output_str, "%-20.20s", filename);
			}
			else
			{
				sprintf(output_str, "%-20.20s", completions[i]);
			}
		}
		else
		{
			sprintf(output_str, "%-20.20s", completions[i]);
		}
	
		sh_write(output_str, strlen(output_str));
	
		if (i % 5 == 4)
		{
			sh_write("\r\n", 2);
		}
		else
		{
			sh_write(" ", 1);
		}
	}
	
	if (i % 5 != 0)
	{
		sh_write("\r\n", 2);
	}

	return;
}

static void multi_completions_completer(char *input, 
													int *cursor, int *len,
													int completions_num, 
													char *completions[])
{
	char *begin		= NULL;
	char *seperater	= NULL;
	char c;
	int i, j, l, same_char = 1;
	int this_len	= *len;		/* 已输入字符串长度 */

	seperater = strrchr(completions[0], '/');
	begin     = strrchr(input, '/');
	
	if (seperater == NULL && begin == NULL)
	{
		seperater = completions[0];
		if (strrchr(input, ' '))
		{
			begin = strrchr(input, ' ') + 1;
		}
		else
		{
			begin = input;
		}
	}
	else if (seperater != NULL && begin == NULL)
	{
		seperater = completions[0];
		begin     = strrchr(input, ' ') + 1;
	}
	else if (seperater == NULL && begin != NULL)
	{
		return;
	}
		 	
	l = seperater - completions[0] + strlen(begin);
	
	for (j = 0; ; j++)
	{
		c = *(completions[0] + l + j);
		if (c == '\0')
		{
			break;
		}
	
		for (i = 0; i < completions_num; i++)
		{
			if (c != *(completions[i] + l + j))
			{
				same_char = 0;
			}
		}
	
		if (same_char == 0)
		{
			break;
		}
	
		input[this_len++] = *(completions[0] + l + j);
		input[this_len]   = '\0';
		*cursor = *len = this_len;
	}

	return;
}

static void show_multi_completions(char *input, int *cursor, int *len, 
											int completions_num, char *completions[], 
											int completion_type)
{
	int show_all = 1;	/* 是否显示所有补齐字符串 */

	show_all = sh_show_all(completions_num, MAX_SHOW_COMPLETIONS_NUM);
	if (show_all)
	{
		show_all_completions(completions_num, completions, completion_type);
	}

	/* 多条补齐字符串输出后继续补齐操作 */
	multi_completions_completer(input, cursor, len, completions_num, completions);

	/* 重新输出命令行提示符和补齐之后的字符串 */
	sh_show_prompt();
	sh_write(input, *len);
	if (*cursor < *len)
	{
		*cursor = *len;
	}

	return;
}

static char *savestring(const char *s)
{
    return ((char *)strcpy((char *)malloc(strlen(s) + 1), (s)));
}

static char *sh_get_filename_completions(const char *text, int state)
{
    static DIR *directory		= NULL;
    static char *filename		= NULL;
    static char *dirname		= NULL;
    static char *users_dirname	= NULL;
    static int filename_len		= 0;
    char *temp					= NULL;
    int dirlen					= 0;
    struct dirent *entry		= NULL;

    /* If we don't have any state, then do some initialization. */
    if (state == 0)
    {
		/* If we were interrupted before closing the directory or reading
   		   all of its contents, close it. */
        if (directory)
        {
            closedir(directory);
            directory = (DIR *)NULL;
        }
        free(dirname);
        free(filename);
        free(users_dirname);

        filename = savestring(text);
        if (*text == 0)
        {
            text = "./";
        }
        dirname = savestring(text);

        temp = strrchr(dirname, '/');
        if (temp)
        {
            strcpy(filename, ++temp);
            *temp = '\0';
        }
        else
        {
            dirname[0] = '.';
            dirname[1] = '\0';
        }

        /* We aren't done yet.  We also support the "~user" syntax. */
        /* Save the version of the directory that the user typed. */
        users_dirname = savestring(dirname);

        directory	  = opendir(dirname);
        filename_len  = strlen(filename);
    }

    /* At this point we should entertain the possibility of hacking wildcarded
       filenames, like /usr/man/man<WILD>/te<TAB>.  If the directory name
       contains globbing characters, then build an array of directories, and
       then map over that list while completing. */
    /* *** UNIMPLEMENTED *** */ 
    /* Now that we have some state, we can read the directory. */

    entry = (struct dirent *)NULL;
    while (directory && (entry = readdir(directory)))
    {
		/* Special case for no filename.  If the user has disabled the
 		   `match-hidden-files' variable, skip filenames beginning with `.'.
 		   All other entries except "." and ".." match. */
        if (filename_len == 0)
        {
			if ((entry->d_name[0] != '.' )
				|| (entry->d_name[1] 
					&& (entry->d_name[1] != '.' || entry->d_name[2])))
			{
				break;
			}
        }
        else
        {
			if ((entry->d_name[0] == filename[0]) 
				&& (((int)(strlen(entry->d_name))) >= filename_len) 
					&& (strncmp(filename, entry->d_name, filename_len) == 0))
			{
				break;
			}
        }
    }

    if (entry == 0)
    {
        if (directory)
        {
            closedir(directory);
            directory = (DIR *)NULL;
        }
        if (dirname)
        {
            free(dirname);
            dirname = (char *)NULL;
        }
        if (filename)
        {
            free(filename);
            filename = (char *)NULL;
        }
        if (users_dirname)
        {
            free(users_dirname);
            users_dirname = (char *)NULL;
        }

		return (char *)NULL;
    }
    else
    {
        /* dirname && (strcmp (dirname, ".") != 0) */
        if (dirname && (dirname[0] != '.' || dirname[1]))
        {
            dirlen = strlen(users_dirname);
            temp = (char *)malloc(2 + dirlen + strlen(entry->d_name));
            strcpy(temp, users_dirname);
            /* Make sure that temp has a trailing slash here. */
            if (users_dirname[dirlen - 1] != '/')
            {
                temp[dirlen++] = '/';
			}
            strcpy(temp + dirlen, entry->d_name);
        }
        else
        {
            temp = savestring(entry->d_name);
		}

        return temp;
    }
}

int sh_directory_completer(char *input, char **completions[])
{
	char **directory_completions = NULL;/* 目录名补齐字符串地址数组 */
	int state					= 0;	/* 目录名补齐状态，0为初始状态 */
	int completions_alloced		= 0;	/* 目录名补齐字符串地址数组分配内存数目 */
	int completions_num			= 0;	/* shell命令目录名参数字符串补齐数目 */

    completions_num      = 0;
	/* 
	 * 首先给文件名补齐字符串地址数组分配一个地址，
	 * 后续匹配文件名成功后，再追加分配内存。
	 */
    completions_alloced	 = 1;
    directory_completions = (char **)malloc(completions_alloced * sizeof(char *));

    state = 0;
    while (1)
    {
        char *p;
        p = sh_get_filename_completions(input, state);
        if (completions_num >= completions_alloced)
        {
        	/* 追加内存分配 */
            completions_alloced *= 2;
            directory_completions = (char **)realloc(directory_completions,
									completions_alloced * sizeof(char *));
        }
		
        if (p == NULL)
        {
            break;
        }


        state = 1;

		if (!filename_is_dir(p))
		{
			continue;
		}
		
        if (p[strlen(p) - 1] == '~')
        {
            continue;
        }

		directory_completions[completions_num++] = p;
    }

	if ((completions_num == 0) && (directory_completions))
	{
		free(directory_completions);
		directory_completions = (char **)NULL;
	}
	else
	{
		directory_completions[completions_num] = NULL;
		*completions = directory_completions;
	}

	return completions_num;
}

int sh_filename_completer(char *input, char **completions[])
{
	char **filename_completions = NULL;	/* 文件名补齐字符串地址数组 */
	int state					= 0;	/* 文件名补齐状态，0为初始状态 */
	int completions_alloced		= 0;	/* 文件名补齐字符串地址数组分配内存数目 */
	int completions_num			= 0;	/* shell命令文件名或目录名参数字符串补齐数目 */

    completions_num      = 0;
	/* 
	 * 首先给文件名补齐字符串地址数组分配一个地址，
	 * 后续匹配文件名成功后，再追加分配内存。
	 */
    completions_alloced	 = 1;
    filename_completions = (char **)malloc(completions_alloced * sizeof(char *));

    state = 0;
    while (1)
    {
        char *p;
        p = sh_get_filename_completions(input, state);
        if (completions_num >= completions_alloced)
        {
        	/* 追加内存分配 */
            completions_alloced *= 2;
            filename_completions = (char **)realloc(filename_completions,
									completions_alloced * sizeof(char *));
        }
		
        if (p == NULL)
        {
            break;
        }

        state = 1;
		
        if (p[strlen(p) - 1] == '~')
        {
            continue;
        }

		filename_completions[completions_num++] = p;
    }

	if ((completions_num == 0) && (filename_completions))
	{
		free(filename_completions);
		filename_completions = (char **)NULL;
	}
	else
	{
		filename_completions[completions_num] = NULL;
		*completions = filename_completions;
	}

	return completions_num;
}

int sh_location_completer(char *input, char **completions[])
{
	return 0;
}

int sh_noop_completer(char *input, char **completions[])
{
	return 0;
}

int sh_command_completer(char *input, char **completions[])
{
	char **command_completions	= NULL; /* 命令名称补齐字符串地址数组 */
	int completions_alloced		= 0;	/* 命令名补齐字符串地址数组分配内存数目 */
	int completions_num			= 0;	/* 命令名字符串补齐数目 */
	struct sh_cmd_t *p_cmd		= NULL;	/*  */
	
	completions_num = 0;
	/* 
	 * 首先给文件名补齐字符串地址数组分配一个地址，
	 * 后续匹配文件名成功后，再追加分配内存。
	 */
	completions_alloced  = 1;
	command_completions = (char **)malloc(completions_alloced * sizeof(char *));

	p_cmd = get_sh_cmd_list();
	while (1)
	{
		if (completions_num >= completions_alloced)
		{
			completions_alloced *= 2;
			command_completions = (char **)realloc((char *)command_completions,
											   (completions_alloced * sizeof(char *)));
		}

		if (p_cmd == NULL)
		{
			break;
		}

		if (!strncmp(p_cmd->name, input, strlen(input)))
		{
			command_completions[completions_num++] = savestring(p_cmd->name);
		}

		p_cmd = p_cmd->next;

	}
   
	if ((completions_num == 0) && (command_completions))
	{
		free(command_completions);
		command_completions = (char **)NULL;
	}
	else
	{
		command_completions[completions_num] = NULL;
		*completions = command_completions;
	}

	return completions_num;
}

static int sh_cmd_arg_completer(char *input, int *cursor, int *len)
{
	struct sh_cmd_t *p_cmd		= NULL;					/* shell命令结构体指针 */
	char cmd_line[MAX_CMD_LEN]	= {0x0, };				/* 复制shell已输入字符串并作解析 */
	char **argv					= NULL;					/* shell已输入字符串命令参数数组 */
	char *cmd_name				= NULL;					/* 输入字符串中第一个命令 */
	char **completions			= NULL;					/* 字符串补齐地址数组 */
	int completions_num			= 0;					/* 字符串补齐数目 */
	int completion_type			= UNKNOWN_COMPLETION;	/* shell命令参数类型 */
	char *text					= NULL;					/* shell命令参数字符串 */
	int i;
	
	/* 复制shell输入字符串并解析命令名称及参数 */
	strcpy(cmd_line, input);
	sh_trim_str(cmd_line);
	parse_cmd_args(cmd_line, &argv);

	/* 查找输入的第一个shell命令 */
	cmd_name = argv[0];
	
	p_cmd = lookup_shell_cmd(cmd_name);
	if (p_cmd == NULL || p_cmd->completer == NULL)
	{
		return HANDLE_COMPLETION_DONE;
	}

	text = strrchr(input, ' ') + 1;
	completions_num = p_cmd->completer(text, &completions);
	if (completions == NULL)
	{
		return HANDLE_COMPLETION_DONE;
	}

	/* shell命令参数补齐接口为文件名补齐接口时，需要扫描目录 */
	if (p_cmd->completer == sh_filename_completer)
	{
		completion_type = CMD_FILENAME_ARG_COMPLETION;
	}

	/* 根据字符串补齐数目以及补齐字符串地址进行处理 */
	if (completions_num == 0)
	{
		sh_write("\a", 1);
	}
	else if (completions_num == 1)
	{
		show_single_completion(input, cursor, len, completions, completion_type);
	}
	else if (sh_get_last_char() == CTRL('I'))
	{
		/* 连续Tab键处理 */
		show_multi_completions(input, cursor, len, completions_num, 
								completions, completion_type);
	}
	else
	{
		/* 单次Tab键处理 */
		sh_set_last_char(CTRL('I'));
		sh_write("\a", 1);
	}
	
	/* 释放命令参数分配内存 */
	if (argv != NULL)
	{
		free(argv);
		argv = NULL;
	}

	/* 释放字符串补齐地址数组内存 */
	if (completions)
	{
		for (i = 0; i < completions_num; i++)
		//for (completions_num = 0; completions[completions_num] != NULL; completions_num++)
		{
			free(completions[i]);
			completions[i] = NULL;
		}
		free(completions);
		completions = NULL;
	}
	
	return HANDLE_COMPLETION_DONE;
}

static int sh_cmd_name_completer(char *input, int *cursor, int *len)
{
	char **completions	= NULL;		/* 字符串补齐地址数组 */
	char *p_cmd			= input;	/* 指向输入字符串中第一个非空字符 */
	int completions_num	= 0;		/* 字符串补齐数目 */
	int i;

	/* 如果输入字符串为空，补齐操作结束 */
	if (p_cmd == NULL)
	{
		return HANDLE_COMPLETION_DONE;
	}

	/* 查找输入字符串中第一个非空字符 */
	while (isspace(*p_cmd) && (*p_cmd != '\0'))
	{
		p_cmd++;
	}

	/* 
	 * 首先根据输入字符串，判断是否匹配shell命令名称
	 * 如果匹配shell命令名称，根据字符串补齐数目以及补齐字符串地址进行处理。
	 */
	completions_num = sh_command_completer(p_cmd, &completions);
	if (completions_num == 0)
	{	 
		/*
		 * 如果输入字符串匹配单一shell命令失败，并且有效输入字符串中存在空格，
		 * 则需要进行shell命令参数补齐操作。否则，补齐操作结束。
		 */
		if (strrchr(p_cmd, ' ') == 0)
		{
			sh_write("\a", 1);
		}
		else
		{
			return HANDLE_COMPLETION_FAILED;
		}
	}
	else if (completions_num == 1)
	{
		show_single_completion(input, cursor, len, completions, CMD_NAME_COMPLETION);
	}
	else if (sh_get_last_char() == CTRL('I'))
	{
		/* 连续Tab键处理 */
		show_multi_completions(input, cursor, len, completions_num, 
								completions, CMD_NAME_COMPLETION);
	}
	else
	{
		/* 单次Tab键处理 */
		sh_set_last_char(CTRL('I'));
		sh_write("\a", 1);
	}

	/* 释放字符串补齐地址数组内存 */
	if (completions)
	{
		for (i = 0; i < completions_num; i++)
		//for (completions_num = 0; completions[completions_num] != NULL; completions_num++)
		{
			//SH_PRINTF("completions[%d] = 0x%lx\n", i, &completions[i]);
			free(completions[i]);
			completions[i] = NULL;
		}
		//SH_PRINTF("completions  = 0x%lx\n",  completions);

		free(completions);
		completions = NULL;
	}
	
	return HANDLE_COMPLETION_DONE;
}

void sh_completion_handler(char *input, int *cursor, int *len)
{
	int ret;

	/* shell命令名称补齐操作 */
	ret = sh_cmd_name_completer(input, cursor, len);
	if (ret == HANDLE_COMPLETION_DONE)
	{
		return;
	}

	/* shell命令参数补齐操作 */
	ret = sh_cmd_arg_completer(input, cursor, len);
	if (ret == HANDLE_COMPLETION_DONE)
	{
		return;
	}

	return;
}

