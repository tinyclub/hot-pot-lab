
#ifndef _DIM_SUM_NET_DRV_H
#define _DIM_SUM_NET_DRV_H

#include <drvm/drv_common.h>

typedef struct net_class_dev
{
    struct class_funcs gen_funcs;
    void    *dev_priv;
}net_dev_t;

static inline int NetDevInput (dev_ctrl_t *p_dev,char *buf,int buf_size)
{
    if (!p_dev)
    {
        return 0;
    }
    return p_dev->p_class_ops1->read(p_dev,buf,buf_size);
}

static inline int NetDevOutput (dev_ctrl_t *p_dev,char *buf,int buf_size)
{
    if (!p_dev)
    {
        return 0;
    }
    return p_dev->p_class_ops1->write(p_dev,buf,buf_size);
}

static inline int NetDevIoctl(dev_ctrl_t *p_dev,int ctrl_cmd,void *p_cmd_args)
{
    if (!p_dev)
    {
        return DIM_SUM_FAIL;
    }
    return p_dev->p_class_ops1->ioctl(p_dev,ctrl_cmd,p_cmd_args);
}

extern dev_ctrl_t *NetDevGet(char *name);
extern int NetDevPut(dev_ctrl_t * p_dev);
extern dev_id_t NetDevCreate(char *name,int priv_size,int bus_type,void *p_bus);
extern int NetDevDestroy(dev_id_t p_dev);
extern int reg_net_dev(dev_ctrl_t *p_dev);
extern int unreg_net_dev(dev_ctrl_t *p_dev);
extern int nwdmm_init(void);

#endif //_DIM_SUM_NET_DRV_H


