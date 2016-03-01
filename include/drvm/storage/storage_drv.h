
#ifndef _DIM_SUM_STORAGE_DRV_H
#define _DIM_SUM_STORAGE_DRV_H

#include <drvm/drv_common.h>
#include <usr/sem.h>

#define BLKSSZGET 		0x0300
#define HDIO_GETGEO		0x0301	/* get device geometry */
struct hd_geometry {
      unsigned char heads;
      unsigned char sectors;
      unsigned short cylinders;
      unsigned long start;
};
typedef struct storage_class_dev
{
    struct   class_funcs common_funcs;
    unsigned int dbr_sector_offset; 	//fat 分区DBR起始扇区偏移地址
    unsigned long dev_block_nums;
    unsigned long lba;		/* number of blocks */
    unsigned long blksz;		/* block size */
    SemId dev_lock;
    int      (*blk_read)(struct dev_ctrl  *dev_id,int startBlk,int nBlks,char *pBuf);
    int      (*blk_write)(struct dev_ctrl  *dev_id,int startBlk,int nBlks,char *pBuf);
    void    *dev_priv;
}storage_dev_t;



static inline int StorageDevIoctl(dev_ctrl_t *p_dev,int ctrl_cmd,void *p_cmd_args)
{
    if (!p_dev)
    {
        return DIM_SUM_FAIL;
    }
    return p_dev->p_class_ops1->ioctl(p_dev,ctrl_cmd,p_cmd_args);
}

extern int StorageDevWrite (dev_ctrl_t *p_dev,int startBlk,int nBlks,char *pBuf);
extern int StorageDevRead (dev_ctrl_t *p_dev,int startBlk,int nBlks,char *pBuf);


extern int reg_storage_dev(dev_id_t p_dev);
extern int unreg_storage_dev(dev_id_t p_dev);
extern dev_ctrl_t *StorageDevCreate(char *name,int priv_size,int bus_type,void *p_bus);
extern int StorageDevDestroy(dev_ctrl_t *p_dev);
extern dev_ctrl_t *StorageDevGet(const char *name);
extern int StorageDevPut(dev_ctrl_t * p_dev);

extern int stdmm_init(void);
#endif //_DIM_SUM_NET_DRV_H


