
#ifndef _DIM_SUM_DRV_COMMON_H
#define _DIM_SUM_DRV_COMMON_H

#include <asm/errno.h>
#include <base/list.h>

#include <dim-sum/base.h>
#include <dim-sum/err.h>
#include <string.h>
#include <linux/types.h>

typedef u8 u8_t;
typedef u16 u16_t;
typedef u32 u32_t;
typedef u64 u64_t;

#define	DEV_TYPE_NULL		0
#define	DEV_TYPE_STOR		0x50
#define	DEV_TYPE_MTD		0x70

typedef	struct dev_com
{
	unsigned int type;
}dev_com_t;

#define MAX_DEV_NAME 16
typedef struct dev_ctrl
{
	dev_com_t dev;		/*this must first*/
    struct list_head dev_list;
    char   dev_name[MAX_DEV_NAME];		/* device name */
    u32_t  dev_type;
    int    ref_cnt;
	u32_t  dev_no;   //device's sequence number each device is unique
	u32_t  qbuf_no;   //qbuf's sequence number each qbuf is unique
    struct bus_funcs   *p_bus_ops1;
    struct class_funcs *p_class_ops1;
    void *p_bus;
    void *p_class;
    int (* bhcache_uinit)(unsigned short dev );  
}dev_ctrl_t;

typedef dev_ctrl_t * dev_id_t;

typedef struct class_funcs
{
    //common ops
    int   (*read)(struct dev_ctrl  *dev_id,char *buf,int buf_size);
    int   (*write)(struct dev_ctrl *dev_id,char *buf,int buf_size);
    int   (*ioctl)(struct dev_ctrl *dev_id,int ctrl_cmd,void *p_cmd_args); 
    void  (*open)(struct dev_ctrl  *dev_id);
    void  (*close)(struct dev_ctrl *dev_id);
}class_funcs_t;

struct bus_funcs
{
    //PM
    void  (*suspend) (struct dev_ctrl  * dev_id,int pm_events);
    void  (*resume)  (struct dev_ctrl  * dev_id);
    void  (*shutdown)(struct dev_ctrl  * dev_id);
};




/* dev type macro define */

#define DEV_BUS_SHIFT  16
#define DEV_BUS_MASK   (~((1<<DEV_BUS_SHIFT) - 1))

#define DEV_CPU_BUS       (1 << DEV_BUS_SHIFT)
#define DEV_USB_BUS	 DEV_CPU_BUS + 1
#define DEV_TFFS_BUS	DEV_USB_BUS + 1
#define DEV_PCI_BUS       (2 << DEV_BUS_SHIFT)

#define MIN_BUS_TYPE    DEV_CPU_BUS
#define MAX_BUS_TYPE    DEV_PCI_BUS

#define DEV_CLASS_MASK  ((1<<DEV_BUS_SHIFT) - 1)
#define DEV_SERIAL_CLASS    1
#define DEV_NET_CLASS       2
#define DEV_STORAGE_CLASS   3
 

/**************************************************************************
*         BKM Global Funcation                   
**************************************************************************/

extern dev_id_t alloc_devctrl(char *name,int bus_type,int class_type);
extern int free_devctrl(dev_id_t p_dev);

#endif //_DIM_SUM_DRV_COMMON_H


