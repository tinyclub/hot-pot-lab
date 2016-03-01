

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_arch_mm.h"
#include "sh_arch_disasm.h"
#include "sh_memory.h"
#include "sh_utils.h"
#include "sh_command.h"
#include "sh_readline.h"
#include "sh_symbol.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern int sh_showmem_cmd(int argc, char **args);

/**********************************************************
 *                    Global Variables                    *
 **********************************************************/

/**********************************************************
 *                    Static Variables                    *
 **********************************************************/

/**********************************************************
 *                       Implements                       *
 **********************************************************/
static int width_is_valid(unsigned int width)
{
	return (width == 1 || width == 2
			|| width == 4 || width == 8) ? 1 : 0;
}

static inline void DISPLAY_BLANK(unsigned long width)
{
    char c = ' ';
	char fmt[16] = {0x0, };
	sprintf(fmt, "%%%luc ", width << 1);
	SH_PRINTF(fmt, c);
	return;
}

static inline int DISPLAY_MEMORY(unsigned long addr, unsigned long width)
{
	int ret = -1;
	int i   = 0;
	unsigned char *tempchar = NULL;
	unsigned long long data = 0;

	ret = sh_read_memory(addr, (char *)&data, width);
	if (ret == 0)
	{
		tempchar = (unsigned char *)&data;
		for (i = 0; i < width; i++)
		{
			SH_PRINTF("%02x", *tempchar);
			tempchar++;
		}
		SH_PRINTF(" ");
	}
	else
	{
		SH_PRINTF("Read <%ld> bytes memory from address <0x%lx> failed.\n", width, addr);
	}
	
	return ret;
}

static inline void DISPLAY_CHAR_REPRESENT(unsigned long addr, unsigned long len)
{
    int i, ret;
    unsigned char c = 0;
    
    SH_PRINTF("* ");
    for (i = 0; i < len; i++)
	{
		ret = sh_read_memory(addr++, &c, 1);
		if (ret != 0)
		{
			return;
		}
		if (isprint(c)) 
		{
			SH_PRINTF("%c", c);
		} 
		else 
		{
			SH_PRINTF(".");
		}
    }
    SH_PRINTF(" *");

	return;
}

static int display_memory_block(unsigned long addr, unsigned long width, 
										unsigned long start, unsigned long end)
{
	unsigned long units  = MEMORY_DISPLAY_BLOCK / width;
	unsigned long ustart = (start - addr) / width;
	unsigned long uend   = (end - addr) / width;
	int i, ret;

	if (uend == 0)
	{
		return 0;
	}

	SH_PRINTF("0X%08lX:\t", addr);
	for (i = 0; i < ustart; i++)
	{
		DISPLAY_BLANK(width);
	}

	for ( ; i < uend; i++)
	{
		ret = DISPLAY_MEMORY(addr + i * width, width);
		if (ret != 0)
		{
			SH_PRINTF("\n");
			return ret;
		}
	}

	for ( ; i < units; i++)
	{
		DISPLAY_BLANK(width);
	}

	SH_PRINTF("    ");
    DISPLAY_CHAR_REPRESENT(addr, MEMORY_DISPLAY_BLOCK);
	SH_PRINTF("\n");
	
	return 0;
}

static int sh_display_memory(unsigned long addr, unsigned long units, unsigned long width)
{
	unsigned long end_addr	= 0;
	unsigned long first_blk	= 0;
	unsigned long last_blk	= 0;
	unsigned long max_addr	= -1;
	int ret;
	
	addr = ALIGN(addr, width); /* 起始地址必须按width字节数对齐 */
	if (MEMORY_DISPLAY_BLOCK % width)
	{
		return -1;
	}

	if ((max_addr - (units * width)) < addr)
	{
		end_addr = max_addr;
	}
	else
	{
		end_addr = addr + (units * width);
	}

	first_blk = ALIGN(addr, MEMORY_DISPLAY_BLOCK);
	last_blk  = ALIGN(end_addr, MEMORY_DISPLAY_BLOCK);

	if (first_blk == last_blk)
	{
		ret = display_memory_block(first_blk, width, addr, end_addr);
		return ret;
	}

	/* first memory block may filled partially */
	ret = display_memory_block(first_blk, width, addr, first_blk + MEMORY_DISPLAY_BLOCK);
	if (ret != 0)
	{
		return ret;
	}

	first_blk += MEMORY_DISPLAY_BLOCK;
	while (first_blk < last_blk)
	{
		ret = display_memory_block(first_blk, width, first_blk, first_blk + MEMORY_DISPLAY_BLOCK);
		if (ret != 0)
		{
			return ret;

		}
		first_blk += MEMORY_DISPLAY_BLOCK;
	}

	/* last memory block may filled partially */
	display_memory_block(first_blk, width, first_blk, end_addr);
		
	return 0;
}

static int sh_read_mem_cmd(int argc, char *argv[])
{
	static unsigned long last_addr	= 0;	/* 记录上一次显示内存的结束地址 */
	/* 内存显示参数，address, units, width */
	unsigned long cmd_args[3]		= {0x0, 0x10, 1};
	unsigned long value				= 0;
	int i;

	if (argc > 4)
	{
		SH_PRINTF("Too many argument.\n");
		sh_show_cmd_usage(argv[0]);
		return RET_ERROR;
	}
	
	/* 无参数输入，默认从上一次内存地址处显示内存 */
	if (argc == 1)
	{
		if (last_addr == 0)
		{
			sh_show_cmd_usage(argv[0]);
			return RET_ERROR;
		}
		else
		{
			cmd_args[0] = last_addr;
		}
	}

	/* 解析内存显示参数 */
	for (i = 0; i < argc - 1; i++)
	{
		value = strtoul(argv[i + 1], NULL, 0);
		if (value == 0 && i == 0)
		{
			/* 内存地址输入错误 */
			SH_PRINTF("The address is invalid.\n");
			return RET_ERROR;
		}
		else
		{
			cmd_args[i] = value;
		}
	}

	last_addr = cmd_args[0] + cmd_args[1] * cmd_args[2];

	if (!width_is_valid(cmd_args[2]))
	{
		/* 内存显示单元字节数输入错误 */
		SH_PRINTF("The width must be [1, 2, 4, 8].\n");
		return RET_ERROR;
	}

	sh_display_memory(cmd_args[0], cmd_args[1], cmd_args[2]);
	
	return RET_OK;
}

static int sh_modify_memory(unsigned long addr, unsigned long width)
{
	char *tempchar				  = NULL;
	char *pLine 				  = NULL;
	char line[32]				  = {0x0, };
	char excess 				  = 0;
	unsigned long long orig_value = 0;
	unsigned long long value	  = 0;
	int i, ret;
	
	for (addr = ALIGN(addr, width); ; addr = addr + width)
	{
		ret = sh_read_memory(addr, (char *)&orig_value, width);
		if (ret != 0)
		{
			SH_PRINTF("Read memory from address <0x%lx> failed.\n", addr);
			return ret;
		}

		/* 打印内存修改提示符 */
		SH_PRINTF("0X%lX:  0x", addr);
		tempchar = (unsigned char *)&orig_value;
		for (i = 0; i < width; i++)
		{
			SH_PRINTF("%02x", *tempchar);
			tempchar++;
		}
		SH_PRINTF(" - 0x");

		/* 读取字符串输入 */
		memset(line, 0x0, sizeof(line));
		readline_echo(line, sizeof(line), NULL);
		line[sizeof(line) - 1] = '\0';

		/* 删除字符串前的空格 */
		for (pLine = line; isspace(*pLine); ++pLine)
			;

		if (*pLine == '\0')
		{
			continue;
		}

		/* 解析字符串输入，如果存在无效字符，则退出 */
		if (sscanf(pLine, "%llx%1s", &value, &excess) != 1)
		{
			break;
		}

		/* 修改内存 */
		ret = sh_write_memory(addr, (unsigned char *)&value, width);
		if (ret != 0)
		{
			SH_PRINTF("Write <%ld> bytes memory to address <0x%lx> failed.\n", width, addr);
			return -1;
		}
	}
	
	SH_PRINTF("\n");
	
	return 0;
}
static int sh_write_mem_cmd(int argc, char *argv[])
{	
	/* 内存修改参数，address, width */
	unsigned long cmd_args[2] = {0x0, 4};
	unsigned long value = 0;
	int i;

	if (argc == 1)
	{
		sh_show_cmd_usage(argv[0]);
		return RET_ERROR;
	}

	if (argc > 3)
	{
		SH_PRINTF("Too many argument.\n");
		sh_show_cmd_usage(argv[0]);
		return RET_ERROR;
	}

	/* 解析内存修改参数 */
	for (i = 0; i < argc - 1; i++)
	{
		value = strtoul(argv[i + 1], NULL, 0);
		if (value == 0 && i == 0)
		{
			/* 内存地址输入错误 */
			SH_PRINTF("The address is invalid.\n");
			return RET_ERROR;
		}
		else
		{
			cmd_args[i] = value;
		}
	}

	if (!width_is_valid(cmd_args[1]))
	{
		/* 内存显示单元字节数输入错误 */
		SH_PRINTF("The width must be [1, 2, 4, 8].\n");
		return RET_ERROR;
	}

	sh_modify_memory(cmd_args[0], cmd_args[1]);
	
	return RET_OK;
}

static unsigned long do_disassemble(unsigned long addr, unsigned long count)
{
	int i;
	char namebuf[128]            = {0x0, };
	char label_to_print_buf[128] = {0x0, };
	const char *label_to_print   = NULL;
	INSTR instr                  = 0;

	for (i = 0; i < count; i++)
	{
		label_to_print = lookup_sym_name(addr, namebuf);
		if (label_to_print != NULL
			&& strcmp(label_to_print, label_to_print_buf) != 0)
		{
			SH_PRINTF("\n               %s:\n", label_to_print);
			strcpy(label_to_print_buf, label_to_print);
		}

		sh_read_memory(addr, (char *)&instr, sizeof(instr));
		addr += arch_dsm_instr(addr, instr);
	}

	SH_PRINTF("\n");

	return addr;
}

static int sh_disassemble_cmd(int argc, char *argv[])
{
	static unsigned long next_to_dsm = 0;
	static unsigned long dsm_count   = 10;
	unsigned long value              = 0;
	
	if (argc > 3)
	{
		SH_PRINTF("Too many argument.\n");
		sh_show_cmd_usage(argv[0]);
		return RET_ERROR;
	}

	if (argc > 1)
	{
		value = lookup_sym_addr(argv[1]);
		if (value == 0)
		{
			value = strtoul(argv[1], NULL, 0);
			if (value == 0)
			{
				SH_PRINTF("Address is invalid.\n");
				return RET_ERROR;
			}
		}
		next_to_dsm = value;
		next_to_dsm = ALIGN(next_to_dsm, INSTR_ALIGN);

		if (argc == 3)
		{
			value = strtoul(argv[2], NULL, 0);
			if (value != 0)
			{
				dsm_count = value;
			}
		}
	}
		
	if (next_to_dsm == 0)
	{
		sh_show_cmd_usage(argv[0]);
	}

	next_to_dsm = do_disassemble(next_to_dsm, dsm_count);
	
	return RET_OK;
}

int sh_read_memory(unsigned long memaddr, char *myaddr, int len)
{
	register int i;
    /* Round starting address down to longword boundary. */
    register long long addr = memaddr & -(long long)sizeof(long);
    /* Round ending address up; get number of longwords that makes. */
    register int count = (((memaddr + len) - addr) + sizeof (long) - 1)/ sizeof (long);
    /* Allocate buffer of that many longwords. */
    register long *buffer = (long *)malloc (count * sizeof (long));
	int err;

    /* Read all the longwords */
	for (i = 0; i < count; i++, addr += sizeof (long))
	{ 
		err = 0;
		__arch_read_safe((unsigned long)addr, buffer[i], err);
		if(err)
		{
			free(buffer);
			return -1;
		}
	}
	
    /* Copy appropriate bytes out of the buffer.  */
    memcpy(myaddr, (char *)buffer + (memaddr & (sizeof (long) - 1)), len);

    free(buffer);
	return 0;
}

int sh_write_memory(unsigned long memaddr, const unsigned char *to_write_buff, int len)
{
	register int i;
	register long long addr = memaddr & -(long long)sizeof(long);
	register int count		= (((memaddr + len) - addr) + sizeof(long) - 1) / sizeof(long);
	register long *buffer	= (long *)malloc(count * sizeof(long));
	int err;	

	err = 0;
	__arch_read_safe((unsigned long)addr, buffer[0], err);
	if (err)
	{
		free(buffer);
		return -1;
	}

	if (count > 1)
	{
		err = 0;
		__arch_read_safe((unsigned long)(addr + (count - 1)* sizeof(long)), buffer[count - 1], err);
		if(err)
		{
			free(buffer);
			return -1; 	
		}	
	}

	/* Copy data to be written over corresponding part of buffer */
	memcpy((char *)buffer + (memaddr & (sizeof(long) - 1)), to_write_buff, len);

	/* Write the entire buffer.  */
	for (i = 0; i < count; i++, addr += sizeof(long))
	{
		err = 0;
		__arch_write_safe(buffer[i], (long)addr, err);
		if(err)
		{
			free(buffer);
			return -1;
		}
	}

	free(buffer);
	return 0;
}

void _initialize_memory_cmds(void)
{
	register_shell_command("showmem", sh_showmem_cmd,
		"show system memory info", 
		"showmem/showmem zone|tiny", 
		"showmem -- show system memory info",
		sh_noop_completer);

	register_shell_command("read-mem", sh_read_mem_cmd, 
		"Read and display memory", 
		"read-mem [addr [units [width]]]", 
		"This command reads and displays the content of memory starting at the \n\t"
		"specified address. The range of width is [1, 2, 4, 8]. If the width \n\t"
		"input is not 1, 2, 4 or 8, the default width range 1 byte will be used. \n\t"
		"Example: display from 0xC000000, the units is 2 and the width is 4, the \n\t"
		"command will be \" read-mem 0xC0000000 2 4 \"",
		sh_noop_completer);

	register_alias_command("d", "read-mem");

	register_shell_command("write-mem", sh_write_mem_cmd, 
		"Modify memory", 
		"write-mem addr [width]", 
		"This command prompts to modify memory in byte, short word, word, or \n\t"
		"long word starting at the specified address. All memory values entered \n\t"
		"and displayed are in hexadecimal, with width range [1, 2, 4, 8]. If the \n\t"
		"width range input is not 1, 2, 4 or 8, a default width range 4 bytes will \n\t"
		"be used. If the numbers input is not any hexadecimal numbers, this command \n\t"
		"will be quit and the content of the specified address will not be modified. \n\t"
		"Example: modify from 0xC000000, the width is 2, the command will be \n\t"
		"\" write-mem 0xC0000000 2 \"",
		sh_noop_completer);

	register_alias_command("m", "write-mem");

	register_shell_command("disassemble", sh_disassemble_cmd, 
		"Disassemble", 
		"disassemble addr|funcname [count]", 
		"This command disassembles specified address or function name",
		sh_noop_completer);

	register_alias_command("l", "disassemble");

	return;
}

