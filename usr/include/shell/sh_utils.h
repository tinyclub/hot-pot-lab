
#ifndef _SH_UTILS_H_
#define _SH_UTILS_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

/**********************************************************
 *                         Macro                          *
 **********************************************************/
#ifndef _PRINTF
#define _PRINTF		printf
#endif

#ifndef SH_PRINTF
#define SH_PRINTF	_PRINTF
#endif

/* 定义对齐方式 */
#define ALIGN(x, align)		({ typeof(x) __x = x; __x & ~(align - 1); })

/* 定义布尔类型 */
typedef int 	BOOL;
#define TRUE	1
#define FALSE	0

/* 定义静态类型 */
#define LOCAL static

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
 
#endif

