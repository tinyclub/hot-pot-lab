

/**********************************************************
 *                       Includers                        *
 **********************************************************/
#include "sh_arch_disasm.h"
#include "sh_arch_register.h"
#include "sh_utils.h"
#include "sh_symbol.h"

/**********************************************************
 *                         Macro                          *
 **********************************************************/
/* 地址输出接口指针 */
typedef void (*PRTFUNCPTR)(unsigned int); /* ptr to function returning void */

/* ---------------- Output Functions --------------------- */
#define outc(h)   	(SH_PRINTF("%c", h))
#define outf(f,s) 	(SH_PRINTF(f, s))
#define outi(n)		(outf("#%ld", (unsigned long)n))
#define outx(n)   	(SH_PRINTF("0x%lx", (unsigned long)n))
#define outs(s)   	(SH_PRINTF("%s", s), (strlen(s)))

/* ---------------- Bit twiddlers ------------------------ */
#define bp(n)		(((unsigned int)1L << (n)))
#define bit(n)		(((unsigned int)(instr & bp(n))) >> (n))
#define bits(m,n)	(((unsigned int)(instr & (bp(n) - bp(m) + bp(n)))) >> (m))
#define ror(n,b) 	(((n) >> (b))|((n) << (32 - (b))))

/**********************************************************
 *                  Extern Declareation                   *
 **********************************************************/

/**********************************************************
 *                    Global Variables                    *
 **********************************************************/

/**********************************************************
 *                    Static Variables                    *
 **********************************************************/

/**********************************************************
 *                       Implements                       *
 **********************************************************/
/**
 * 地址输出接口
 */
LOCAL void lPrtAdrs(unsigned int address)
{
	char label_to_print_buf[128] = {0x0, };
	const char *label_to_print   = NULL;
	unsigned long offset         = 0;

	label_to_print = lookup_sym_name(address, label_to_print_buf);
	if (label_to_print)
	{
		lookup_size_offset(address, NULL, &offset);
	}

	if (label_to_print)
	{
		if (offset == 0)
		{
			SH_PRINTF("#0x%08x<%s>", address, label_to_print);
		}
		else
		{
			SH_PRINTF("#0x%08x<%s+0x%lx>", address, label_to_print, offset);
		}
	}
	else
	{
		SH_PRINTF("#0x%08x", address);
	}
	
	return;
}

/**
 * 输出寄存器名称
 */
LOCAL void reg(unsigned int regno, int ch)
{
	if (regno >= 0 && regno <= 15)
	{
		outs(arch_get_reg_name(regno));
	}
    else
    {
        outf("r%d", regno);
    }

    if (ch != 0)
    {
        outc(ch);
    }

    return;
}

/** 
 * 输出立即数
 */
LOCAL void outh(unsigned int n, unsigned int pos)
{
    /* ARM assembler immediate values are preceded by '#' */
    outc('#');

    if (!pos)
    {
		outc('-');
    }

    /* decimal values by default, precede hex values with '0x' */
    if (n < 10)
    {
        outf("%d", n);
    }
    else
    {
        outx(n);
    }

    return;
}

/**
 * 输出空格
 */
LOCAL void spacetocol9(int l)
{
    for ( ; l < 9 ; l++ )
    {
        outc(' ');
    }

    return;
}

/**
 * 输出寄存器组
 */
LOCAL void outregset(unsigned int instr)
{
    BOOL started = FALSE, string = FALSE;
    unsigned int i, first = 0, last = 0;

    /* display the start of list char */
    outc('{');

    /* check for presence of each register in list */
    for (i = 0; i < 16; i++)
    {
        if (bit(i))
        {
            /* register is in list */
            if (!started)
            {
                /* not currently doing a consecutive list of reg numbers */
                reg(i, 0);    /* print starting register */
                started = TRUE;
                first = last = i;
            }
            else if (i == last + 1) /* currently in a list */ 
            {
                string = TRUE;
                last = i;
            }
            else /* not consecutive */
            {
                if (i > last + 1 && string )
                {
                    outc((first == last - 1) ? ',' : '-');
                    reg(last, 0);
                    string = FALSE;
                }
                outc(',');
                reg(i, 0);
                first = last = i;
            }
        }
    } /* endfor */

    if (string)
    {
        outc((first == last - 1) ? ',' : '-');
        reg(last, 0);
    }

    outc('}');

    return;
} /* outregset() */

/**
 * 输出比较指令
 */
LOCAL int cond(unsigned int instr)
{
    const char* ccnames = "EQ\0\0NE\0\0CS\0\0CC\0\0MI\0\0PL\0\0VS\0\0VC\0\0"
                         "HI\0\0LS\0\0GE\0\0LT\0\0GT\0\0LE\0\0\0\0\0\0NV";

    return outs(ccnames + 4 * (int)bits(28, 31));
} 

/**
 * 输出操作指令
 */
LOCAL void opcode(unsigned int instr, const char *op, char ch)
{
    int l;

    /* display the opcode */
    l = outs(op);

    /* display any condition code */
    l += cond(instr);

    /* display any suffix character */
    if (ch != 0)
    {
        outc(ch);
        l++;
    }

    /* pad with spaces to column 9 */
    spacetocol9(l);

    return;
} 


/* 以下为普通ARM指令反汇编操作相关接口 */

/**
 * 输出浮点寄存器
 */
LOCAL void freg(unsigned int rno, int ch)
{
    outf("f%d", rno);

    if (ch != 0)
    {
        outc(ch);
    }

    return;
}

/**
 * 寄存器位移操作
 */
LOCAL void shiftedreg(unsigned int instr)
{
    char* shiftname = "LSL\0LSR\0ASR\0ROR" + 4 * (int)bits(5, 6);

    /* display register name */
    reg(bits(0, 3), 0); /* offset is a (shifted) reg */

    if (bit(4))
    {
        /* register shift */
        outf(",%s ", shiftname);
        reg(bits(8, 11), 0);
    }
    else if (bits(5, 11) != 0)
    {
        /* immediate shift */
        if (bits(5, 11) == 3)
        {
            outs(",RRX");
        }
        else
        {
            outf(",%s ", shiftname);
            if (bits(5, 11) == 1 || bits(5, 11) == 2)
            {
                outi(32L);
            }
            else
            {
                outi(bits(7, 11));
            }
        }
    }

    return;
} 

/**
 * 输出地址
 */
LOCAL void outAddress( unsigned int instr, /* value of instruction */
    						unsigned int address, /* address */
    						int offset, /* any offset part of instruction */
   							PRTFUNCPTR prtAddress  /* routine to print addresses as symbols */ )
{
    if ( bits( 16, 19 ) == 15 && bit( 24 ) && !bit( 25 ) )
    {
        /* pc based, pre, imm */
        if ( !bit( 23 ) )
        {
            offset = -offset;
        }
        address = address + offset + 8;
        prtAddress( address );
    }
    else
    {
        outc( '[' );
        reg( bits( 16, 19 ), ( bit( 24 ) ? 0 : ']' ) );
        outc( ',' );

        if ( !bit( 25 ) )
        {
            /* offset is an immediate */
            outh( offset, bit( 23 ) );
        }
        else
        {
            if ( !bit( 23 ) )
            {
                outc( '-' );
            }
            shiftedreg( instr );
        }

        if ( bit( 24 ) )
        {
            outc( ']' );
            if ( bit( 21 ) )
            {
                outc( '!' );
            }
        }
    }

    return;
} /* outAddress() */

LOCAL void generic_cpdo(unsigned int instr)
{
    opcode( instr, "CDP", 0 );
    outf( "p%d,", bits( 8, 11 ) );
    outx( bits( 20, 23 ) );
    outc( ',' );

    outf( "c%d,", bits( 12, 15 ) ); /* CRd */
    outf( "c%d,", bits( 16, 19 ) ); /* CRn */
    outf( "c%d,", bits( 0, 3 ) );   /* CRm */
    outh( bits( 5, 7 ), 1 );

    return;
} /* generic_cpdo() */

LOCAL void generic_cprt(unsigned int instr)
{
    opcode( instr, ( bit( 20 ) ? "MRC" : "MCR" ), 0 );
    outf( "p%d,", bits( 8, 11 ) );
    outx( bits( 21, 23 ) );
    outc( ',' );
    reg( bits( 12, 15 ), ',' );

    outf( "c%d,", bits( 16, 19 ) ); /* CRn */
    outf( "c%d,", bits( 0, 3 ) );   /* CRm */
    outh( bits( 5, 7 ), 1 );

    return;
} /* generic_cprt() */

LOCAL void generic_cpdt( unsigned int instr,
              				unsigned int address,
              				PRTFUNCPTR prtAddress )
{
    opcode( instr, ( bit( 20 ) ? "LDC" : "STC" ), ( bit( 22 ) ? 'L' : 0 ) );
    outf( "p%d,", bits( 8, 11 ) );
    outf( "c%d,", bits( 12, 15 ) );

    outAddress( instr, address, 4 * bits( 0, 7 ), prtAddress );

    return;
} /* generic_cpdt() */

LOCAL char fp_dt_widthname( unsigned int instr )
{
    return "SDEP"[bit( 15 ) + 2 * bit( 22 )];
} /* fp_dt_widthname() */

LOCAL char fp_widthname( unsigned int instr )
{
    return "SDEP"[bit( 7 ) + 2 * bit( 19 )];
} /* fp_widthname() */

LOCAL char *fp_rounding( unsigned int instr )
{
    return "\0\0P\0M\0Z" + 2 * bits( 5, 6 );
} /* fp_rounding() */

LOCAL void fp_mfield( unsigned int instr )
{
    unsigned int r = bits( 0, 2 );

    if ( bit( 3 ) )
    {
        if ( r < 6 )
        {
            outi( r );
        }
        else
        {
            outs( ( r == 6 ? "#0.5" : "#10" ) );
        }
    }
    else
    {
        freg( r, 0 );
    }

    return;
} /* fp_mfield() */

LOCAL void fp_cpdo( unsigned int instr, BOOL *oddity )
{
    char* opset;
    int l;

    if ( bit( 15 ) )  /* unary */
    {
        opset = "MVF\0MNF\0ABS\0RND\0SQT\0LOG\0LGN\0EXP\0"
				"SIN\0COS\0TAN\0ASN\0ACS\0ATN\0URD\0NRM";
    }
    else
    {
        opset = "ADF\0MUF\0SUF\0RSF\0DVF\0RDF\0POW\0RPW\0"
				"RMF\0FML\0FDV\0FRD\0POL\0XX1\0XX2\0XX3";
    }

    l = outs( opset + 4 * bits( 20, 23 ) );
    l += cond( instr );
    outc( fp_widthname( instr ) );
    l++;
    l += outs( fp_rounding( instr ) );
    spacetocol9( l );
    freg( bits( 12, 14 ), ',' );  /* Fd */
    if ( !bit( 15 ) )
    {
        freg( bits( 16, 18 ), ',' );
    }  /* Fn */
    else if ( bits( 16, 18 ) != 0 )
            /* odd monadic (Fn != 0) ... */
    {
        *oddity = TRUE;
    }

    fp_mfield( instr );

    return;
} /* fp_cpdo() */

LOCAL void fp_cprt( unsigned int instr )
{
    int op = ( int ) bits( 20, 23 );

    if ( bits( 12, 15 ) == 15 )  /* ARM register = pc */
    {
        if ( ( op & 9 ) != 9 )
        {
            op = 4;
        }
        else
        {
            op = ( op >> 1 ) - 4;
        }
        opcode( instr, "CMF\0\0CNF\0\0CMFE\0CNFE\0???" + 5 * op, 0 );
        freg( bits( 16, 18 ), ',' );
        fp_mfield( instr );
        return;
    }
    else
    {
        int l;

        if ( op > 7 )
        {
            op = 7;
        }
        l = outs( "FLT\0FIX\0WFS\0RFS\0WFC\0RFC\0???\0???" + 4 * op );
        l += cond( instr );
        outc( fp_widthname( instr ) );
        l++;
        l += outs( fp_rounding( instr ) );
        spacetocol9( l );
        if ( bits( 20, 23 ) == 0 ) /* FLT */
        {
            freg( bits( 16, 18 ), ',' );
        }
        reg( bits( 12, 15 ), 0 );
        if ( bits( 20, 23 ) == 1 ) /* FIX */
        {
            outc( ',' );
            fp_mfield( instr );
        }
    }

    return;
} /* fp_cprt() */

LOCAL void fp_cpdt( unsigned int instr, unsigned int address, PRTFUNCPTR prtAddress )
{
    if ( !bit( 24 ) && !bit( 21 ) )
    {
        /* oddity: post and not writeback */
        generic_cpdt( instr, address, prtAddress );
    }
    else
    {
        opcode( instr,
                ( bit( 20 ) ? "LDF" : "STF" ),
                fp_dt_widthname( instr ) );
        freg( bits( 12, 14 ), ',' );
        outAddress( instr, address, 4 * bits( 0, 7 ), prtAddress );
    }

    return;
} /* fp_cpdt() */

LOCAL void fm_cpdt( unsigned int instr, unsigned int address, PRTFUNCPTR prtAddress )
{
    if ( !bit( 24 ) && !bit( 21 ) )
    {
        /* oddity: post and not writeback */
        generic_cpdt( instr, address, prtAddress );
        return;
    }
    opcode( instr, ( bit( 20 ) ? "LFM" : "SFM" ), 0 );
    freg( bits( 12, 14 ), ',' );
    {
        int count = ( int ) ( bit( 15 ) + 2 * bit( 22 ) );

        outf( "%d,", count == 0 ? 4 : count );
    }
    outAddress( instr, address, 4 * bits( 0, 7 ), prtAddress );

    return;
} /* fm_cpdt() */

static int disasm_32(unsigned long address, INSTR instr, PRTFUNCPTR prtAddress)
{
	BOOL oddity = FALSE;

	switch ( bits( 24, 27 ) )
	{
		case 0:
			if ( bit( 23 ) == 1 && bits( 4, 7 ) == 9 )
			{
				/* Long Multiply */
				opcode( instr,
						bit( 21 ) ?
							( bit( 22 ) ? "SMLAL" : "UMLAL" ) :
							( bit( 22 ) ? "SMULL" : "UMULL" ),
						bit( 20 ) ?
							'S' :
							0 );
				reg( bits( 12, 15 ), ',' );
				reg( bits( 16, 19 ), ',' );
				reg( bits( 0, 3 ), ',' );
				reg( bits( 8, 11 ), 0 );
				break;
			}
			
		/* **** Drop through */
		case 1:
		case 2:
		case 3:
			if ( bit( 4 ) && bit( 7 ) )
			{
				if ( !bit( 5 ) && !bit( 6 ) )
				{
					if ( bits( 22, 27 ) == 0 )
					{
						opcode( instr,
								( bit( 21 ) ? "MLA" : "MUL" ),
								( bit( 20 ) ? 'S' : 0 ) );
						reg( bits( 16, 19 ), ',' );
						reg( bits( 0, 3 ), ',' );
						reg( bits( 8, 11 ), 0 );
						if ( bit( 21 ) )
						{
							outc( ',' );
							reg( bits( 12, 15 ), 0 );
						}
						break;
					}
					
					if ( bits( 23, 27 ) == 2 && bits( 8, 11 ) == 0 )
					{
						/* Swap */
						opcode( instr, "SWP", ( bit( 22 ) ? 'B' : 0 ) );
						reg( bits( 12, 15 ), ',' );
						reg( bits( 0, 3 ), ',' );
						outc( '[' );
						reg( bits( 16, 19 ), ']' );
						break;
					}
				}
				else
				{
					if ( !bit( 25 ) && ( bit( 20 ) || !bit( 6 ) ) )
					{
						int l;

						l = outs( bit( 20 ) ? "LDR" : "STR" );
						l += cond( instr );
						if ( bit( 6 ) )
						{
							outc( 'S' );
							outc( bit( 5 ) ? 'H' : 'B' );
							l += 2;
						}
						else
						{
							outc( 'H' );
							l++;
						}
						
						spacetocol9( l );
						reg( bits( 12, 15 ), ',' );
						outc( '[' );
						reg( bits( 16, 19 ), 0 );

						if ( bit( 24 ) )
						{
							outc( ',' );
						}
						else
						{
							outc( ']' );
							outc( ',' );
						}
						
						if ( bit( 22 ) )
						{
							outh( bits( 0, 3 ) + ( bits( 8, 11 ) << 4 ),
							bit( 23 ) );
						}
						else
						{
							if ( !bit( 23 ) )
							{
								outc( '-' );
							}
							reg( bits( 0, 3 ), 0 );
						}
						
						if ( bit( 24 ) )
						{
							outc( ']' );
							if ( bit( 21 ) )
							{
								outc( '!' );
							}
						}
						break;
					}
				}
			}

			if ( bits( 4, 27 ) == 0x12fff1 )
			{
				opcode( instr, "BX", 0 );
				reg( bits( 0, 3 ), 0 );
				break;
			}

			if ( instr == 0xe1a00000L )
			{
				opcode( instr, "NOP", 0 );
				break;
			}
			
		{
			/* data processing */
			int op = ( int ) bits( 21, 24 );
			const char* opnames = "AND\0EOR\0SUB\0RSB\0ADD\0ADC\0SBC\0RSC\0"
									"TST\0TEQ\0CMP\0CMN\0ORR\0MOV\0BIC\0MVN";

			if ( op >= 8 && op < 12 && !bit( 20 ) )
			{
				if ( ( op & 1 ) == 0 )
				{
					opcode( instr, "MRS", 0 );
					reg( bits( 12, 15 ), ',' );
					
					if ( op == 8 )
					{
						outs( "cpsr" );
					}
					else
					{
						outs( "spsr" );
					}
					oddity = ( bits( 0, 11 ) != 0 || bits( 16, 19 ) != 15 );
					break;
				}
				else
				{
					int rn = ( int ) bits( 16, 19 );
					char* rname = NULL;
					char* part  = NULL;
				#if 1
					if (op == 9)
					{
						rname = "cpsr";
					}
					else
					{
						rname = "spsr";
					}
				#else
					rname = op == 9 ? "cpsr" : "spsr";
				#endif
				
					part = rn == 1 ?
									"ctl" :
									rn ==
										8 ?
										"flg" :
											rn ==
												9 ?
												"all" :
												"?";

					opcode( instr, "MSR", 0 );
					outs( rname );
					outf( "_%s,", part );
					oddity = bits( 12, 15 ) != 15;
				}
			}
			else
			{
				int ch = ( !bit( 20 ) ) ?
							0 :
							( op >= 8 && op < 12 ) ?
								( bits( 12, 15 ) == 15 ? 'P' : 0 ) :
								'S';
				opcode( instr, opnames + 4 * op, ch );

				if ( !( op >= 8 && op < 12 ) )
				{
					/* not TST TEQ CMP CMN */
					/* print the dest reg */
					reg( bits( 12, 15 ), ',' );
				}

				if ( op != 13 && op != 15 )
				{
					/* not MOV MVN */
					reg( bits( 16, 19 ), ',' );
				}
			}

			if ( bit( 25 ) )
			{
				/* rhs is immediate */
				int shift = 2 * ( int ) bits( 8, 11 );
				int operand = ror( bits( 0, 7 ), shift );

				outh( operand, 1 );
			}
			else
			{
				/* rhs is a register */
				shiftedreg( instr );
			}
		}
		break;


		case 0xA:
		case 0xB:
			opcode( instr, ( bit( 24 ) ? "BL" : "B" ), 0 );
			{
				int offset = ( ( ( int ) bits( 0, 23 ) ) << 8 ) >> 6; /* sign extend and * 4 */

				address += offset + 8;
				prtAddress( address );
			}
			break;


		case 6:
		case 7:
			/*
   			 * Cope with the case where register shift register is specified
   			 * as this is an undefined instruction rather than an LDR or STR
   			 */
			if ( bit( 4 ) )
			{
				outs( "Undefined Instruction" );
				break;
			}
			
		/* ***** Drop through to always LDR / STR case */
		case 4:
		case 5:
		{
			int l;

			l = outs( bit( 20 ) ? "LDR" : "STR" );
			l += cond( instr );
			if ( bit( 22 ) )
			{
				outc( 'B' );
				l++;
			}
			if ( !bit( 24 ) && bit( 21 ) )	/* post, writeback */
			{
				outc( 'T' );
				l++;
			}
			spacetocol9( l );
			reg( bits( 12, 15 ), ',' );
			outAddress( instr, address, bits( 0, 11 ), prtAddress );
			break;
		}

		case 8:
		case 9:
		{
			int l;

			l = outs( bit( 20 ) ? "LDM" : "STM" );
			l += cond( instr );
			l += outs( "DA\0\0IA\0\0DB\0\0IB" + 4 * ( int ) bits( 23, 24 ) );
			spacetocol9( l );
			reg( bits( 16, 19 ), 0 );
			if ( bit( 21 ) )
			{
				outc( '!' );
			}
			outc( ',' );
			outregset( instr );
			if ( bit( 22 ) )
			{
				outc( '^' );
			}
			break;
		}


		case 0xF:
			opcode( instr, "SWI", 0 );
			{
				int swino = bits( 0, 23 );
				outx( swino );
			}
			break;

		case 0xE:
			if ( bit( 4 ) == 0 )
			{
				/* CP data processing */
				switch ( bits( 8, 11 ) )
				{
					case 1:
						fp_cpdo( instr, &oddity );
						break;

					default:
						generic_cpdo( instr );
						break;
				}
			}
			else
			{
				/* CP reg to/from ARM reg */
				switch ( bits( 8, 11 ) )
				{
					case 1:
						fp_cprt( instr );
						break;

					default:
						generic_cprt( instr );
						break;
				}
			}
			break;


		case 0xC:
		case 0xD:
			switch ( bits( 8, 11 ) )
			{
				case 1:
					fp_cpdt( instr, address, prtAddress );
					break;
					
				case 2:
					fm_cpdt( instr, address, prtAddress );
					break;
					
				default:
					generic_cpdt( instr, address, prtAddress );
					break;
			}
			break;

		default:
			outs( "EQUD    " );
			outx( instr );
			break;
	} /* endswitch */

	if ( oddity )
	{
		outs( " ; (?)" );
	}

	outc( '\n' );

	return 4;
}

unsigned long arch_dsm_instr(unsigned long addr, INSTR instr)
{
	SH_PRINTF("0x%08lx\t%08lx\t", addr, (unsigned long)instr);

	return disasm_32(addr, instr, lPrtAdrs);
}

