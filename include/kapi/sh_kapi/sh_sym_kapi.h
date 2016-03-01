
#ifndef _SH_SYM_KAPI_H_
#define _SH_SYM_KAPI_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern unsigned long sh_ker_lookup_addr(const char *name);
extern const char *sh_ker_lookup_name(unsigned long addr,
											unsigned long *symbolsize,
											unsigned long *offset,
											char *namebuf);
extern int sh_ker_lookup_size_offset(unsigned long addr,
											unsigned long *symbolsize,
											unsigned long *offset);

#endif

