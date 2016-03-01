

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <sh_kapi.h>
 
#include "sh_utils.h"
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

unsigned long lookup_sym_addr(const char *name)
{
	return sh_ker_lookup_addr(name);
}

const char *lookup_sym_name(unsigned long addr, char *namebuf)
{
	if (namebuf)
	{
		return sh_ker_lookup_name(addr, NULL, NULL, namebuf);
	}
	else
	{
		return NULL;
	}
}

unsigned long lookup_sym_size(const char *name)
{
	unsigned long sym_addr = 0;
	unsigned long sym_size = 0;

	sym_addr = sh_ker_lookup_addr(name);

	if (sym_addr != 0)
	{
		sh_ker_lookup_size_offset(sym_addr, &sym_size, NULL);
	}

	return sym_size;
}

int lookup_size_offset(unsigned long addr, 
							unsigned long *symbolsize, 
							unsigned long *offset)
{
	return sh_ker_lookup_size_offset(addr, symbolsize, offset);
}

static int sh_lkup_cmd(int argc, char *argv[])
{
	unsigned long sym_addr = 0;

	if (argc != 2)
	{
		if (argc > 2)
		{
			SH_PRINTF("Too many argument.\n");
		}

		sh_show_cmd_usage(argv[0]);
		return RET_ERROR;
	}

	sym_addr = lookup_sym_addr(argv[1]);
	if (sym_addr == 0)
	{
		SH_PRINTF("No such symbol <%s> in system.\n", argv[1]);
	}
	else
	{
		SH_PRINTF("The address of symbol <%s> is <0x%lx>.\n", argv[1], sym_addr);
	}

	return RET_OK;	
}

static int sh_lkaddr_cmd(int argc, char *argv[])
{
	char sym_name[128]     = {0x0, };
	unsigned long sym_addr = 0;
	unsigned long offset   = 0;
	unsigned long sym_size = 0;

	if (argc != 2)
	{
		if (argc > 2)
		{
			SH_PRINTF("Too many argument.\n");
		}

		sh_show_cmd_usage(argv[0]);
		return RET_ERROR;
	}

	sym_addr = strtol(argv[1], NULL, 0);
	lookup_sym_name(sym_addr, sym_name);
	if (strlen(sym_name)
		&& lookup_size_offset(sym_addr, &sym_size, &offset))
	{
		SH_PRINTF("The symbol name of address <0x%lx> is <%s>, size <0x%lx>, offset <0x%lx>.\n",
			sym_addr, sym_name, sym_size, offset);
	}
	else
	{
		SH_PRINTF("No symbol for address <0x%lx> in system.\n", sym_addr);
	}

	return RET_OK;
}


void _initialize_symbol_cmds(void)
{
	register_shell_command("lkup", sh_lkup_cmd, 
		"List the address of symbol", 
		"lkup symbol", 
		"This command lists the address of symbol in the system.",
		sh_noop_completer);

	register_shell_command("lkaddr", sh_lkaddr_cmd, 
		"List the symbol information of address", 
		"lkaddr addr", 
		"This command lists the symbol information of address in the system.\n\t"
		"The symbol information includes the symbol name, the symbol size and \n\t"
		"the offset of address.",
		sh_noop_completer);

	return;
}

