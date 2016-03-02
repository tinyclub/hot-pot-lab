
#ifndef _SH_KEYMAP_H_
#define _SH_KEYMAP_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/
/* 
 * 定义控制键执行函数类型 
 * 参数1 -- 输入字符串
 * 参数2 -- 当前光标位置
 * 参数3 -- 当前输入字符串长度
 */
typedef int (sh_key_func_t)(char *, int *, int *);

/* 定义控制键结构体 */
typedef struct
{
	int 			 sh_key_value;	/* 控制键键值 */
	sh_key_func_t	*sh_key_func;	/* 控制键执行函数 */
}sh_key_map_t;
	
/* 定义字符值和键值 */
#ifndef VK_DELETE
#define VK_DELETE	0x7f		/* Delete键值 */
#endif
	
#ifndef VK_ESC
#define VK_ESC		0x1b		/* Esc键值 */
#endif

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern int esc_key_handler(void);								/* esc键值处理及转换控制字符 */
extern sh_key_func_t ctrl_A_key_handler;
extern sh_key_func_t ctrl_B_key_handler;
extern sh_key_func_t ctrl_C_key_handler;
extern sh_key_func_t ctrl_D_key_handler;
extern sh_key_func_t ctrl_E_key_handler;
extern sh_key_func_t ctrl_F_key_handler;
extern sh_key_func_t ctrl_H_key_handler;
extern sh_key_func_t ctrl_I_key_handler;
extern sh_key_func_t ctrl_K_key_handler;
extern sh_key_func_t ctrl_L_key_handler;
extern sh_key_func_t ctrl_N_key_handler;
extern sh_key_func_t ctrl_P_key_handler;
extern sh_key_func_t ctrl_T_key_handler;
extern sh_key_func_t ctrl_U_key_handler;
extern sh_key_func_t ctrl_W_key_handler;
extern sh_key_func_t ctrl_Z_key_handler;
extern sh_key_func_t delete_key_handler;
/* 普通字符输出接口 */
extern int normal_char_type_handler(char c, char *input, int *cursor, int *len);	
extern sh_key_func_t *lookup_sh_key_func(int sh_key_value);	/* 根据控制字符值查找对应的处理函数 */

#endif

