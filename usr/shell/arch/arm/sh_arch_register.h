
#ifndef _SH_ARCH_REGISTER_H_
#define _SH_ARCH_REGISTER_H_

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_register.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/
#define ARCH_REGS_NUM	16

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern unsigned long arch_get_register(int task_id, int regno);
extern const char *arch_get_reg_name(int regno);
extern int arch_get_reg_no(const char *reg_name);

#endif

