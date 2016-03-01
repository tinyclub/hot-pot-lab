

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <linux/kallsyms.h>

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
 
asmlinkage unsigned long sh_ker_lookup_addr(const char *name)
{
	return kallsyms_lookup_name(name);
}

asmlinkage const char *sh_ker_lookup_name(unsigned long addr,
												unsigned long *symbolsize,
												unsigned long *offset,
												char *namebuf)
{
	return kallsyms_lookup(addr, symbolsize, offset, NULL, namebuf);
}

asmlinkage int sh_ker_lookup_size_offset(unsigned long addr,
												unsigned long *symbolsize,
												unsigned long *offset)
{
	return kallsyms_lookup_size_offset(addr, symbolsize, offset);
}

