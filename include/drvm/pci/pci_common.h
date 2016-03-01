
#ifndef _DIM_SUM_PCI_COMMON_H
#define _DIM_SUM_PCI_COMMON_H

#include <drvm/pci/pci_ids.h>
#include <drvm/drv_common.h>

/*
 * The PCI interface treats multi-function devices as independent
 * devices.  The slot/function address of each device is encoded
 * in a single byte as follows:
 *
 *	7:3 = slot
 *	2:0 = function
 */
#define PCI_DEVFN(slot,func)	((((slot) & 0x1f) << 3) | ((func) & 0x07))
#define PCI_SLOT(devfn)		(((devfn) >> 3) & 0x1f)
#define PCI_FUNC(devfn)		((devfn) & 0x07)


struct pci_controller 
{
	struct pci_controller *next;
	char    *io_start;
	char    *io_end;
	char    *mem_start;
	char    *mem_end;
	int     (*pci_conf_read)(int bus_no,int dev_no,int func_no,int off, int size, u32_t *val);
	int     (*pci_conf_write)(int bus_no,int dev_no,int func_no,int off, int size, u32_t val);
};

struct pci_device_id {
	u32_t vendor, device;		/* Vendor and device ID or PCI_ANY_ID*/
	u32_t subvendor, subdevice;	/* Subsystem ID's or PCI_ANY_ID */
	u32_t dev_class, class_mask;	/* (class,subclass,prog-if) triplet */
};

#define DEVICE_COUNT_RESOURCE	12

#ifdef CONFIG_PCI_RESOURCES_64BIT
typedef u64_t pci_resource_size_t;
#else
typedef u32_t pci_resource_size_t;
#endif

#define PCI_SPACE_INVALID  0

#define PCI_IO_SPACE       1
#define PCI_MEM_SPACE      2

typedef struct pci_space
{
	pci_resource_size_t start;
	pci_resource_size_t end;
	unsigned long flags;
}pci_space_t;

typedef struct pcibus_resource
{
    struct list_head res_list;
   	unsigned int	bus_no;		/* bus number */
	unsigned int	dev_no;		/*  */
    unsigned int	func_no;	
	unsigned int	irq_no;
	pci_space_t     space[DEVICE_COUNT_RESOURCE];
    struct pci_controller *p_horse; /**/
    struct pci_drv *p_drv; /**/
}pcibus_res_t;

struct pcibus_funcs
{
    struct bus_funcs common_bus_funcs;
};

typedef struct pci_drv
{
    struct list_head      drv_list;
    char  *drv_name;
    struct pci_device_id *id_table;

    struct pcibus_funcs bus_funcs;
    int  (*dev_create)(struct pcibus_resource *p_resource);
//    void (*dev_remove) (struct pcibus_resource *p_resource);

}pci_drv_t;

extern int reg_pci_drv(pci_drv_t *p_drv);
extern int unreg_pci_drv(pci_drv_t *p_drv);

extern int PciConfigGet8(dev_ctrl_t *p_dev,int off,u8_t *p_retval);
extern int PciConfigPut8(dev_ctrl_t *p_dev,int off,u8_t val);
extern int PciConfigGet16(dev_ctrl_t *p_dev,int off,u16_t *p_retval);
extern int PciConfigPut16(dev_ctrl_t *p_dev,int off,u16_t val);
extern int PciConfigGet32(dev_ctrl_t *p_dev,int off,u32_t *p_retval);
extern int PciConfigPut32(dev_ctrl_t *p_dev,int off,u32_t val);
extern pcibus_res_t *alloc_pcires(void);
extern int free_pcires(pcibus_res_t *p_res);
extern int reg_pci_res(pcibus_res_t *p_res);
extern int unreg_pci_res(pcibus_res_t *p_res);

extern int pcibusmm_init(void);

#endif //_DIM_SUM_PCI_COMMON_H


