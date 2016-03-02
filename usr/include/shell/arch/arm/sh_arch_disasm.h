
#ifndef _SH_ARCH_DISASM_H_
#define _SH_ARCH_DISASM_H_
/**********************************************************
 *                       Includers                        *
 **********************************************************/

/**********************************************************
 *                         Macro                          *
 **********************************************************/
typedef unsigned int INSTR;

#define INSTR_ALIGN	4

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/
extern unsigned long arch_dsm_instr(unsigned long addr, INSTR instr);

#endif

