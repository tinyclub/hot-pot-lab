
#ifndef __CC_H__
#define __CC_H__

#include <linux/time.h>
#include <dim-sum/spinlock_types.h>
#include <dim-sum/printk.h>
#include <dim-sum/semaphore.h>
#include <dim-sum/mem.h>


typedef unsigned char		u8_t;
typedef signed char			s8_t;
typedef unsigned short		u16_t;
typedef signed short		s16_t;
typedef unsigned int		u32_t;
typedef signed int			s32_t;
typedef unsigned long long	u64_t;
typedef u32_t			mem_ptr_t;


#if 0 //yangwei debug ??
#define PACK_STRUCT_FIELD(x) x __attribute__((packed))
#define PACK_STRUCT_STRUCT __attribute__((packed))
#else
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT
#endif

#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END


#define BYTE_ORDER	LITTLE_ENDIAN


#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"


#define LWIP_PLATFORM_DIAG(x)		do { printk x;} while (0)

#define LWIP_PLATFORM_ASSERT(x)  {printk("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); while(1);}


extern spinlock_t lwip_spin_lock;
#define SYS_ARCH_DECL_PROTECT(x) unsigned long x
#define SYS_ARCH_PROTECT(x)      spin_lock_irqsave(&lwip_spin_lock, x)
#define SYS_ARCH_UNPROTECT(x)    spin_unlock_irqrestore(&lwip_spin_lock, x)


/*** 将内存配置为MEM_LIBC_MALLOC=1 ，并将mem_malloc和mem_free重定义为dim-sum的内存函数 ***/
extern void __mem_free(void *p);
#define mem_malloc(x) dim_sum_mem_alloc(x, MEM_NORMAL)
#define mem_free	__mem_free

#endif /* __CC_H__ */

