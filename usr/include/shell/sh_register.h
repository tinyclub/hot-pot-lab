
#ifndef _SH_REGISTER_H_
#define _SH_REGISTER_H_
/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/
struct regset
{
	const char *name;	/* 寄存器的名字			  */
	int offset;			/* 该寄存器在缓存中的偏移量 */
	int size;			/* 该寄存器的大小 		  */
	int readable;		/*该寄存器是否可读*/
	int writeable;		/*该寄存器是否可写*/
	int ptrace_offset;	/*获取寄存器调用ptrace时输入的偏移量*/
};

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern void _initialize_register_cmds(void);
extern unsigned long sh_get_register(int task_id, int regno);
extern int sh_get_reg_no(const char *reg_name);
extern const char *sh_get_reg_name(int regno);

#endif

