

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <dim-sum/task.h>
#include <errno.h>

#include "sh_utils.h"
#include "sh_readline.h"
#include "sh_keymap.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern int read_serial_char(char *buf);

/**********************************************************
 *                    Global Variables                    *
 **********************************************************/

/**********************************************************
 *                    Static Variables                    *
 **********************************************************/
static char sh_last_char	= 0;	/* 最后输入的字符 */

/**********************************************************
 *                       Implements                       *
 **********************************************************/
int sh_show_all(int num, int max_num)
{
	int show_all = 1;
	
	if (num > max_num)
	{
		char msg[64] = {0x0, 0};
		char c;
		int ret;
		
		sprintf(msg, "Display all %d possibilities? (Y/N)", num);
		sh_write("\r\n", 2);
		sh_write(msg, strlen(msg));
		do
		{
			ret = sh_read(&c, 1);
			if(ret <= 0 && errno == ECONNRESET)
			{
				/* 读取字符错误 */
				return 0;
			}
		} while (!(c == 'y' || c == 'Y' || c == 'n' || c == 'N'));
		
		if (!(c == 'y' || c == 'Y'))
		{
			show_all = 0;
		}
	}

	return show_all;
}
int sh_show_more(void)
{
	int ret;
	int show_more = 1;
	char msg[64]  = {0x0, 0};
	char c;

	sprintf(msg, "%s", "--More--");
	sh_write(msg, strlen(msg));
	do
	{
		ret = sh_read(&c, 1);
		if(ret <= 0 && errno == ECONNRESET)
		{
			/* 读取字符错误 */
			return 0;
		}
	} while (!(c == 'q' || c == 'Q' || c == ' ' || c == '\r' || c == '\n'));

	if (c == 'q' || c == 'Q')
	{
		show_more = 0;
	}

	return show_more;	
}
void sh_trim_str(char *str)
{
	int len, i;
	char *cp = str;
	
	if (str == NULL)
	{
		return;
	}

	/* 删除字符串末尾的空格 */
	len = strlen(str);
	if (len)
	{
		cp += len - 1;
		while ((cp >= str) 
				&& ((*cp == '\n') || (*cp == '\t') || isspace(*cp)))
		{
			*cp = '\0';
			cp--;
		}
	}

	/* 计算字符串首部空格数 */
	cp = str;
	while (isspace(*cp) && (*cp != '\0'))
	{
		cp++;
	}

	/* 删除字符串首部的空格 */
	if (cp > str)
	{
		len = strlen(cp);
		for (i = 0; i < len + 1; i++)
		{
			str[i] = cp[i];
		}
	}

	return;
}

char sh_get_last_char(void)
{
	return sh_last_char;
}
void sh_set_last_char(char c)
{
	sh_last_char = c;
}

int sh_read(char *buf, size_t size)
{
	int ret = 0;
	while ((ret = read_serial_char(buf)) == 0)
	{
		SysSleep(1);
	}
	
	return ret;
}

void sh_write(char *buf, size_t size)
{	
	if (size <= 0)
	{
		/* 输出字节数错误 */
		return;
	}
	else if (size == 1)
	{
		/* 输出一个字符 */
		SH_PRINTF("%c", *buf);
	}
	else
	{
		/* 输出一个字符串 */
		SH_PRINTF("%s", buf);
	}
		
	return;
}

void sh_clean_input(char *input, int *cursor, int *len)
{
	int i;
	int this_cursor = *cursor;
	int this_len	= *len;

	/* 光标位置后的字符输出为空格 */
	if (this_cursor < this_len)
	{
		for (i = 0; i < this_len - this_cursor; i++)
		{
			sh_write(" ", 1);
		}
	}

	/* 光标回退到为0 */
	for (i = 0; i < this_len; i++)
	{
		sh_write("\b", 1);
	}

	/* 光标位置前的字符输出为空格 */
	for (i = 0; i < this_cursor; i++)
	{
		sh_write(" ", 1);
	}

	/* 光标回退到为0 */
	for (i = 0; i < this_cursor; i++)
	{
		sh_write("\b", 1);
	}

	/* 已输入字符串清零 */
	memset(input, 0x0, this_len);
	
	*cursor = 0;
	*len	= 0;
	
	return;
}

static void sh_read_char(char *buf, int echo)
{
	char c = 0;							/* 读取字符 */
	int ret = 0;						/* 返回值 */
	int esc = 0;						/* escape键是否启用 */
	int this_cursor = 0;				/* 当前光标位置 */
	int this_len	= 0;				/* 当前输入字符串长度 */
	char input[RL_BUF_SIZE] = {0, };	/* 输入字符串缓存 */
	sh_key_func_t *sh_key_func = NULL;	/* 控制字符处理函数指针 */

	while (1)
	{
		ret = sh_read(&c, 1);
		if(ret <= 0)
		{
			/* 读取字符错误 */
			memset(input, 0x0, RL_BUF_SIZE);
			goto sh_read_char_ret;
		}

		/* escape键后续处理流程 */
		if (esc) 
		{
			c = esc_key_handler();
			esc = 0;
		}

		/* escape键 */
		if (c == VK_ESC) 
		{
			esc = 1;
			continue;
		}

		if (c == 0)
		{
			goto sh_read_char_ret;
		}

		sh_key_func = lookup_sh_key_func(c);
		if (sh_key_func)
		{
			/* 控制字符处理 */
			ret = sh_key_func(input, &this_cursor, &this_len);
			if (ret == RET_READ_CHAR_CONT)
			{
				continue;
			}
			else if (ret == RET_READ_CHAR_DONE)
			{
				goto sh_read_char_ret;
			}
		}
		else
		{
			/* 普通字符处理 */
			ret = normal_char_type_handler(c, input, &this_cursor, &this_len);
			if (ret == RET_READ_CHAR_CONT)
			{
				/* 回显读取到的字符 */
				if (echo)
				{
					sh_write(&c, 1);
				}

				sh_last_char = c;				
				continue;
			}
			else if (ret == RET_READ_CHAR_DONE)
			{
				goto sh_read_char_ret;
			}
		}
	}

sh_read_char_ret:
	strcpy(buf, input);
	return;
}

static char *readline(int echo)
{
	char *ptr	= NULL;
	int max_len	= 0;

	ptr = malloc(RL_BUF_SIZE);
	if (ptr == NULL)
	{
		return NULL;
	}
	memset(ptr, 0x0, RL_BUF_SIZE);

	sh_read_char(ptr, echo);

	sh_trim_str(ptr);
	
	max_len = strlen(ptr) + 1;
	if (max_len == 1)
	{
		/* len为表示输入的是回车 */
		free(ptr);
		return NULL;
	}

	return ptr;
}

int readline_echo(char *inputbuf, int maxsize, char *prompt)
{
	char *input	= NULL;
	int len		= 0;

	if (inputbuf == NULL)
	{
		return -1;
	}

	inputbuf[0] = '\0';

	if (prompt)
	{
		sh_write(prompt, strlen(prompt));
	}

	input = readline(INPUT_CHAR_ECHO);
	if (input == NULL)
	{
		return 0;
	}

	len = strlen(input);
	if (len >= maxsize)
	{
		strncpy(inputbuf, input, maxsize - 1);
		inputbuf[maxsize - 1] = '\0';
	}
	else
	{
		strcpy(inputbuf, input);
	}

	free(input);

	return strlen(inputbuf);
}

int readline_noecho(char *inputbuf, int maxsize, char *prompt)
{
	char *input	= NULL;
	int len		= 0;

	if (inputbuf == NULL)
	{
		return -1;
	}

	inputbuf[0] = '\0';

	if (prompt)
	{
		sh_write(prompt, strlen(prompt));
	}

	input = readline(INPUT_CHAR_NOECHO);
	if (input == NULL)
	{
		return 0;
	}

	len = strlen(input);
	if (len >= maxsize)
	{
		strncpy(inputbuf, input, maxsize - 1);
		inputbuf[maxsize - 1] = '\0';
	}
	else
	{
		strcpy(inputbuf, input);
	}

	free(input);

	return strlen(inputbuf);
}


