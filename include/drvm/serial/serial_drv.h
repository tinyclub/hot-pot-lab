
#ifndef _DIM_SUM_SERIAL_DRV_H
#define _DIM_SUM_SERIAL_DRV_H

#include <drvm/drv_common.h>

typedef struct serial_class_dev
{
    struct   class_funcs common_funcs;
    int      baud_rate;
    void    *dev_priv;
}serial_dev_t;

static inline int SerialDevInput (dev_ctrl_t *p_dev,char *buf,int buf_size)
{
    if (!p_dev)
    {
        return 0;
    }
    return p_dev->p_class_ops1->read(p_dev,buf,buf_size);
}

static inline int SerialDevOutput (dev_ctrl_t *p_dev,char *buf,int buf_size)
{
    if (!p_dev)
    {
        return 0;
    }
    return p_dev->p_class_ops1->write(p_dev,buf,buf_size);
}

static inline int SerialDevIoctl(dev_ctrl_t *p_dev,int ctrl_cmd,void *p_cmd_args)
{
    if (!p_dev)
    {
        return DIM_SUM_FAIL;
    }
    return p_dev->p_class_ops1->ioctl(p_dev,ctrl_cmd,p_cmd_args);
}
//unreg
extern int reg_serial_dev(dev_ctrl_t *p_dev);
extern int unreg_serial_dev(dev_ctrl_t *p_dev);
extern dev_ctrl_t *SerialDevCreate(char *name,int priv_size,int bus_type,void *p_bus);
extern int SerialDevDestroy(dev_ctrl_t *p_dev);
extern dev_ctrl_t *SerialDevGet(char *name);
extern int SerialDevPut(dev_ctrl_t * p_dev);

extern int srdmm_init(void);
#endif //_DIM_SUM_PCI_H


