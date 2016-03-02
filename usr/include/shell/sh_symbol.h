
#ifndef _SH_SYMBOL_H_
#define _SH_SYMBOL_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern unsigned long lookup_sym_addr(const char *name);
extern const char *lookup_sym_name(unsigned long addr, char *namebuf);
extern unsigned long lookup_sym_size(const char *name);
extern int lookup_size_offset(unsigned long addr, 
									unsigned long *symbolsize, 
									unsigned long *offset);
extern void _initialize_symbol_cmds(void);

#endif

