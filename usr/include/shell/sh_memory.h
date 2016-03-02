
#ifndef _SH_MEMORY_H_
#define _SH_MEMORY_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/
#define MEMORY_DISPLAY_BLOCK	0x10UL	/* 定义内存每一行显示的字节数 */

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern void _initialize_memory_cmds(void);
extern int sh_read_memory(unsigned long memaddr, char *myaddr, int len);
extern int sh_write_memory(unsigned long memaddr, const unsigned char *to_write_buff, int len);

#endif


