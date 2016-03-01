

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_keymap.h"
#include "sh_readline.h"
#include "sh_history.h"
#include "sh_completion.h"

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
/* 控制字符定义列表 */
static sh_key_map_t sh_key_map[] = 
{
	{CTRL('A'), 	ctrl_A_key_handler},
	{CTRL('B'), 	ctrl_B_key_handler},	/* 光标左移 */
	{CTRL('C'), 	ctrl_C_key_handler},	/* 结束输入 */
	{CTRL('D'), 	ctrl_D_key_handler},
	{CTRL('E'), 	ctrl_E_key_handler},	/* 光标移动到输入字符串末尾 */
	{CTRL('F'), 	ctrl_F_key_handler},	/* 光标右移 */
	{CTRL('G'), 	NULL},
	{CTRL('H'), 	ctrl_H_key_handler},	/* backspce键 */
	{CTRL('I'), 	ctrl_I_key_handler},	/* tab键 */
	{CTRL('J'), 	NULL},
	{CTRL('K'), 	ctrl_K_key_handler},
	{CTRL('L'), 	ctrl_L_key_handler},
	{CTRL('M'), 	NULL},
	{CTRL('N'), 	ctrl_N_key_handler},	/* pagedown键 */
	{CTRL('O'), 	NULL},
	{CTRL('P'), 	ctrl_P_key_handler},	/* pageup键 */
	{CTRL('Q'), 	NULL},
	{CTRL('R'), 	NULL},
	{CTRL('S'), 	NULL},
	{CTRL('T'), 	ctrl_T_key_handler},
	{CTRL('U'), 	ctrl_U_key_handler},
	{CTRL('V'), 	NULL},
	{CTRL('W'), 	ctrl_W_key_handler},	/* etb键 */
	{CTRL('X'), 	NULL},
	{CTRL('Y'), 	NULL},
	{CTRL('Z'), 	ctrl_Z_key_handler},
	{VK_DELETE, 	delete_key_handler},
	{0, 		NULL}
};

/**********************************************************
 *                       Implements                       *
 **********************************************************/
 
int esc_key_handler(void)
{
	char c	= 0;	 /* 读取字符 */
	int ret = 0;	 /* 返回值 */

esc_read:

	ret = sh_read(&c, 1);
	if(ret <= 0)
	{
		/* 读取字符错误 */
		c = 0;
		goto esc_ret;
	}

	/* 继续读取字符 */
	if (c == '[')
	{
		goto esc_read;
	}

	/* 将esc键值转换为控制字符 */
	switch (c)
	{
		case 'A':	 /* UP */
			c = CTRL('P');	
			break;
			
		case 'B':	 /* DOWN */
			c = CTRL('N');	
			break;
			
		case 'C':	 /* RIGHT */
			c = CTRL('F');	 
			break;		 
			
		case 'D':	 /* LEFT */
			c = CTRL('B');	
			break;
			
		case '3':	 /* DELETE */
			ret = sh_read(&c, 1); 
			if(ret <= 0)
			{
				/* 读取字符错误 */
				c = 0;
				goto esc_ret;
			}

			if (c == '~')
			{
				c = VK_DELETE;
			}
			break;
			
		default:
			c = 0;
	}

esc_ret: 

	return c;	 
}

sh_key_func_t *lookup_sh_key_func(int sh_key_value)
{
	int sh_key_map_index = 0;

	while (sh_key_map[sh_key_map_index].sh_key_value != 0)
	{
		if (sh_key_map[sh_key_map_index].sh_key_value == sh_key_value)
		{
			return sh_key_map[sh_key_map_index].sh_key_func;
		}

		sh_key_map_index++;
	}

	return NULL;
}

static void input_back(int back, char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;
	
	while (back--) 
	{
		if (this_len == this_cursor) 
		{
			input[--this_cursor] = 0;
			sh_write("\b \b", 3);
		} 
		else 
		{
			int i;
			int len;
			/* 光标位置向后移动一位 */
			this_cursor--; 
			/* 从光标所在位置开始，输入字符串依次向后移动一位 */
			for (i = this_cursor; i <= this_len; i++)
			{
				input[i] = input[i + 1];
			}
			/* 计算从光标所在位置字符串长度 */
			len = strlen(input + this_cursor);
			/* 输出退格符，并且光标位置向后移动一位，即覆盖要删除的字符 */
			sh_write("\b", 1);
			/* 输出从光标所在位置开始的字符串 */
			sh_write(input + this_cursor, len);
			/* 输出空格，覆盖最后一个字符 */
			sh_write(" ", 1);
			/* 光标回退 */
			for (i = 0; i <= len; i++)
			{
				sh_write("\b", 1);
			}
		}
		this_len--;
	}

	*cursor = this_cursor;
	*len	= this_len;
	
	return;
}

int ctrl_A_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_B_key_handler(char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	
	if (this_cursor) 
	{
		sh_write("\b", 1);
		this_cursor--;
	}

	*cursor = this_cursor;
	
	return RET_READ_CHAR_CONT;
}

int ctrl_C_key_handler(char *input, int *cursor, int *len)
{	
	sh_write("\a", 1);

	return RET_READ_CHAR_DONE;
}

int ctrl_D_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_E_key_handler(char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;

	if (this_cursor < this_len) 
	{
		sh_write(&input[this_cursor], this_len - this_cursor);
		this_cursor = this_len;
	}

	*cursor = this_cursor;
	*len	= this_len;
	
	return RET_READ_CHAR_CONT;
}

int ctrl_F_key_handler(char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;
	
	if (this_cursor < this_len) 
	{
		sh_write(&input[this_cursor], 1);
		this_cursor++;
	}

	*cursor = this_cursor;
	*len	= this_len;
	
	return RET_READ_CHAR_CONT;
}

int ctrl_H_key_handler(char *input, int *cursor, int *len)
{
	int back		= 0;
	int this_len	= *len;
	int this_cursor = *cursor;

	if (this_len == 0 || this_cursor == 0)
	{
		sh_write("\a", 1);
		return RET_READ_CHAR_CONT;
	}

	back = 1;
	 
	if (back) 
	{
		input_back(back, input, cursor, len);
		return RET_READ_CHAR_CONT;
	}

	return RET_READ_CHAR_DONE;
}

int ctrl_I_key_handler(char *input, int *cursor, int *len)
{
	sh_completion_handler(input, cursor, len);
	return RET_READ_CHAR_CONT;
}

int ctrl_K_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_L_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_N_key_handler(char *input, int *cursor, int *len)
{
	char *history_input = NULL;

	sh_clean_input(input, cursor, len);
	
	history_input = get_next_history();
	if (history_input)
	{
		
		strcpy(input, history_input);
		*cursor = *len = strlen(input);
		sh_write(input, *len);
	}

	return RET_READ_CHAR_CONT;
}

int ctrl_P_key_handler(char *input, int *cursor, int *len)
{
	char *history_input = NULL;

	sh_clean_input(input, cursor, len);
	
	history_input = get_prev_history();
	if (history_input)
	{
		
		strcpy(input, history_input);
		*cursor = *len = strlen(input);
		sh_write(input, *len);
	}
	
	return RET_READ_CHAR_CONT;
}

int ctrl_T_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_U_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int ctrl_W_key_handler(char *input, int *cursor, int *len)
{
	int back		= 0;
	int nc			= *cursor;
	int this_cursor = *cursor;
	int this_len	= *len;
		
	if (this_len == 0 || this_cursor == 0)
	{
		return RET_READ_CHAR_CONT;
	}

	while (nc && input[nc - 1] == ' ') 
	{
		nc--;
		back++;
	}
	
	while (nc && input[nc - 1] != ' ') 
	{
		nc--;
		back++;
	}

	if (back) 
	{
		input_back(back, input, cursor, len);
		return RET_READ_CHAR_CONT;
	}

	return RET_READ_CHAR_DONE;
}

int ctrl_Z_key_handler(char *input, int *cursor, int *len)
{
	return RET_READ_CHAR_CONT;
}

int delete_key_handler(char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;
	
	if (this_len != this_cursor)
	{
		int i;
		int len;
		/* 从光标所在位置开始，输入字符串依次向后移动一位 */
		for (i = this_cursor; i < this_len; i++)
			input[i] = input[i + 1];
		/* 输入字符串最后一个字符清零 */
		input[this_len--] = '\0';
		/* 计算从光标所在位置字符串长度 */
		len = strlen(input + this_cursor);
		/* 输出从光标所在位置开始的字符串 */
		sh_write(input + this_cursor, len);
		/* 输出空格，覆盖最后一个字符 */
		sh_write(" ", 1);	
		/* 光标回退 */
	
		for (i = 0; i <= len; i++)
			sh_write("\b", 1);
	}

	*cursor = this_cursor;
	*len	= this_len;
	
	return RET_READ_CHAR_CONT;
}

int normal_char_type_handler(char c, char *input, int *cursor, int *len)
{
	int this_cursor = *cursor;
	int this_len	= *len;

	/* 回车 */
	if (c == '\n')
	{
		sh_write("\n", 1);
		return RET_READ_CHAR_DONE;
	}

	/* 换行 */
	if (c == '\r')
	{
		sh_write("\r\n", 2);
		return RET_READ_CHAR_DONE;
	}
	
	if (this_cursor == this_len) 
	{
		/* 
		 * 正常字符串输入，光标位置与输入字符串长度值相等，
		 * 在已输入字符串末尾插入新的字符
		 */
		 
		/* append to end of line */
		if ((short)c == 20 && this_cursor == 0)
		{
			return RET_READ_CHAR_DONE;
		}
	
		input[this_cursor] = c;
		if (this_len < (RL_BUF_SIZE - 1)) 
		{
			/* 未超出输入字符串最大长度，光标位置和输入字符串长度值加1 */
			this_len++;
			this_cursor++;
		} 
		else 
		{
			/* 超出输入字符串最大长度，返回最大输入字符串 */
			sh_write("\a", 1);
			return RET_READ_CHAR_DONE;
		}
	}
	else 
	{
		/* 
		 * 光标位置与输入字符串长度值不相等，主要操作是在已输入字符串
		 * 中间插入新的字符
		 */
		 
		/* insert */
		int i = 0;
		/* move everything one character to the right */
		if (this_len >= (RL_BUF_SIZE - 2)) 
		{
			this_len--;
		}
		
		for (i = this_len; i >= this_cursor; i--)
		{
			input[i + 1] = input[i];
		}
	
		/* sh_write what we've just added */
		input[this_cursor] = c;
		
		sh_write(&input[this_cursor], this_len - this_cursor + 1);
		for (i = 0; i < (this_len - this_cursor + 1); i++)
		{
			sh_write("\b", 1);
		}
	
		this_len++;
		this_cursor++;
	}

	*cursor = this_cursor;
	*len	= this_len;

	return RET_READ_CHAR_CONT;
}

