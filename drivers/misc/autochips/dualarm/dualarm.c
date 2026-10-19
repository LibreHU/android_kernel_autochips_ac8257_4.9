/*
* Copyright (c) 2016 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/of.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include "dualarm.h"

#include "dual_hal.h"
#include "types.h"
#include "ac8257.h"
#include "dualarm_log.h"
#include <asm/io.h>
#define DUALARM_DEV_DRIVER_NAME "dualarm-dev"
#define DEVICE_NAME "dualarm-dev"

typedef struct _module_info{
    int module_index;
    irq_handler_t handler;
    const char *name;
}module_info;



u32 g_dualarm_dbg_lvl = DUALARM_LOG_LVL_HAL;
u8 *g_dualarm_log_level[] = {
    "[Dualarm] [ERR]",
    "[Dualarm] [WARN]",
    "[Dualarm] [INFO]",
    "[Dualarm] [HAL]",
    "[Dualarm] [DBG]",
    "[Dualarm] [IRQ]",
};


typedef enum {
    DUALARM_BIM_BASE_ADDR = 0,
    ARM1_IO_BASE_ADDR,
    DUALARM_BASEADDR_NUM
} DUALARM_BASEADDR_ENUM;


typedef enum {
    ARM2_TO_ARM1_INT_0 = 0,
    ARM2_TO_ARM1_INT_1,
    ARM2_TO_ARM1_INT_2,
    ARM2_TO_ARM1_INT_3,
    ARM2_TO_ARM1_INT_NUM
} DUALARM_INT_ENUM;


static module_info g_arm2_modules[3];

unsigned int arm2_to_arm1_int_0 = 0;    //dualarm irq vector
unsigned int arm2_to_arm1_int_1 = 0;    //dualarm irq vector
#ifdef CONFIG_ATC_ARM2_LOG_BUFFER
unsigned int arm2_to_arm1_int_3;	//dualarm irq vector
#endif

#define __ioaddr(a) ((void __iomem *)(a))

void __iomem *arm2_emi_base;
void __iomem *io_reg_base;
void __iomem *dualarm_reg_base;

spinlock_t Dualarm_Lock;

UINT32 u4ARM2Start(void)
{
    //HAL_WRITE32(arm2_emi_base, ARM2_PCJUMP_CMD);
    void* pVMemAddr = ioremap(ARM2_EMI_OFFSET,16);

    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "u4ARM2Start enter!\n");

    *(UINT32 *)(pVMemAddr) = ARM2_PCJUMP_CMD;
    iounmap(pVMemAddr);

#ifdef trace_debug
    ARM1_IO_WRITE32(REG_ARM2_JTAG,  ARM2_JTAG_PINMUX);
#endif
    //reset arm2
    DUALARM_BIM_WRITE32(REG_ARM2_CONTROL, ARM2_CORE_RESET|ARM2_COREDEBUG_ENABLE|
            ARM2_CORECLK_ENABLE|ARM2_BLKGATE_BYPASS);

    return 0;
}

int  request_dualarm_irq(unsigned int module, irq_handler_t handler, unsigned long flags,
        const char *name, void *dev)
{
    if(module > (MAX_DULAARM_MODULE -1))
        return -1;

    g_arm2_modules[module].module_index = module;
    g_arm2_modules[module].handler = handler;
    g_arm2_modules[module].name = name;

    return 0;

}
EXPORT_SYMBOL(request_dualarm_irq);

int bacakcar_rsv_num = 0;
int bootlogo_rsv_num = 0;

static irqreturn_t dualarm_bcar_isr_handler(int irq, void *dev_id)
{
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm_bcar_isr_handler call\n");
    if(g_arm2_modules[MODULE_BCAR].handler != NULL){
        g_arm2_modules[MODULE_BCAR].handler(MODULE_BCAR,dev_id);}
    else {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "backcar handler is null\n");
    }

    return IRQ_HANDLED;
}

#ifdef CONFIG_ATC_ARM2_LOG_BUFFER
extern irqreturn_t arm2_log_buffer_log_handler(int irq, void *dev_id);
#endif

static irqreturn_t dualarm_disp_isr_handler(int irq, void *dev_id)
{
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm_disp_isr_handler call\n");
     if(g_arm2_modules[MODULE_DISPLAY].handler != NULL){
        g_arm2_modules[MODULE_DISPLAY].handler(MODULE_DISPLAY,dev_id);}
    else {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "display handler is null\n");
    }

    return IRQ_HANDLED;
}

static int dualarm_mmap(struct file *fp, struct vm_area_struct *vma)
{
#if 0
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    /* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
    if (remap_pfn_range(vma,
            vma->vm_start,
            vma->vm_pgoff,
            vma->vm_end - vma->vm_start,
            vma->vm_page_prot))
        return -EAGAIN;
#endif
    return 0;
}

static int dualarm_open(struct inode* inode, struct file *file)
{
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm open[kernel space]\n");
    return 0;
}

/*
static ssize_t dualarm_read(struct file *file, const char __user *in, size_t size, loff_t *off)
{
    int ret = 0;
    UINT32 module_id=0;
    UINT32 user_buf[4] = {0};
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm read[kernel space]\n");
    ret = copy_from_user((void *)&module_id, in, 1 * sizeof(UINT32));
    HWGetMessage(module_id, &user_buf[0], &user_buf[1], &user_buf[2], &user_buf[3]);
    ret = copy_to_user((void *)in, user_buf, 4 * sizeof(UINT32));

    return ret;
}

static ssize_t dualarm_write(struct file *file, const char __user *in, size_t size, loff_t *off)
{
    char msg[20] = {0};
    UINT32 data[4] = {0};
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm write[kernel space]\n");
    copy_from_user(msg, in, size - 1);
    sscanf(msg, "%d %d %d %d", &data[0], &data[1], &data[2], &data[3]);

    HWSendMessage(data[0], data[1], data[2], data[3]);
    return size;
}

static int dualarm_ioctl(struct file *filep,unsigned int cmd, unsigned long arg)
{

    BOOL ret = 0;
#if 0
    int err = 0;
    UINT32 module_id=0;
    UINT32 user_buf[4] = {0};
    int i = 0;
    u32 displayPhyAdr;
    //LOGO_BUF_INFO_T rLogoBufInfo;

    //DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm ioctl[kernel space 0x%08x] %d,%d\n",  cmd, (DUALARM_IOC_GETMESSAGE == cmd), (DUALARM_IOC_SENDMESSAGE == cmd));

    switch(cmd) {
        case DUALARM_IOC_GETMESSAGE: {
            copy_from_user((void *)module_id, (void *)arg, 1 * sizeof(UINT32));
            DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm iotcl[kernel space] module_id=%d\n", module_id);
            ret = HWGetMessage(module_id, &user_buf[0], &user_buf[1], &user_buf[2], &user_buf[3]);
            DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm iotcl[kernel space] &user_buf[0]=%d, &user_buf[1]=%d, &user_buf[2]=%d, &user_buf[3]=%d\n",
                user_buf[0], user_buf[1], user_buf[2], user_buf[3]);
            copy_to_user((void *)arg, user_buf, 4 * sizeof(UINT32));
        }
        break;
        case DUALARM_IOC_SENDMESSAGE: {
            copy_from_user(user_buf, (void *)arg, 4 * sizeof(UINT32));
            ret = HWSendMessage(user_buf[0], user_buf[1], user_buf[2], user_buf[3]);
            DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm iotcl[kernel space] &user_buf[0]=%d, &user_buf[1]=%d, &user_buf[2]=%d, &user_buf[3]=%d\n",
                user_buf[0], user_buf[1], user_buf[2], user_buf[3]);
        }
        break;
        default: {
            return -EINVAL;
        }
        break;
    }
#endif
    return ret;
}
*/
struct file_operations dualarm_fops = {
    .owner = THIS_MODULE,
    .open = dualarm_open,
    //.read = dualarm_read,
    //.write = dualarm_write,
    .mmap = dualarm_mmap,
    //.unlocked_ioctl = dualarm_ioctl,
};

static struct miscdevice dualarm_misc_dev = {
    MISC_DYNAMIC_MINOR,
    "dualarm-dev",
    &dualarm_fops,
};

static const struct of_device_id dualarm_of_match[] = {
    {.compatible = "autochips,dualarm-dev",},
    {},
};

MODULE_DEVICE_TABLE(of, dualarm_of_match);
static int dualarm_probe(struct platform_device * pdev)
{
    int ret = -EINVAL;
    struct device_node *node = pdev->dev.of_node;
    void __iomem *base_addr = NULL;

    if (!node) {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "get node fail \r\n");
        return -ENOMEM;
    }

    /* register for Dualarm */
    if (strcmp(node->name, "dualarm") != 0) {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Device node don't match the drive,device name[%s]\r\n",node->name);
        return -ENOMEM;
    }

    base_addr = of_iomap(node, DUALARM_BIM_BASE_ADDR);
    if (!base_addr) {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Unable to ioremap bim registers\n");
        return -ENOMEM;
    }
    dualarm_reg_base = base_addr;

    spin_lock_init(&Dualarm_Lock);

    base_addr = NULL;
    base_addr = of_iomap(node, ARM1_IO_BASE_ADDR);
    if (!base_addr) {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Unable to ioremap arm1 io base\n");
        return -ENOMEM;
    }
    io_reg_base = base_addr;

    arm2_to_arm1_int_0 = irq_of_parse_and_map(node, ARM2_TO_ARM1_INT_0);
    ret = request_irq(arm2_to_arm1_int_0, (irq_handler_t) dualarm_bcar_isr_handler, IRQF_TRIGGER_LOW, "DUALARM_ISR", NULL);
    if (ret) {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Unable to request IRQ, irq=%d\n", arm2_to_arm1_int_0);
        return ret;
    }
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "arm2_to_arm1_int_0 is %d\n", arm2_to_arm1_int_0);

    arm2_to_arm1_int_1 = irq_of_parse_and_map(node, ARM2_TO_ARM1_INT_1);
    ret = request_irq(arm2_to_arm1_int_1, (irq_handler_t) dualarm_disp_isr_handler, IRQF_TRIGGER_LOW, "DUALARM_ISR", NULL);
    if (ret) {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Unable to request IRQ, irq=%d\n", arm2_to_arm1_int_1);
        return ret;
    }
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "arm2_to_arm1_int_0 is %d\n", arm2_to_arm1_int_1);

#ifdef CONFIG_ATC_ARM2_LOG_BUFFER
	arm2_to_arm1_int_3 = irq_of_parse_and_map(node, ARM2_TO_ARM1_INT_3);
	ret = request_irq(arm2_to_arm1_int_3, (irq_handler_t)arm2_log_buffer_log_handler, IRQF_TRIGGER_LOW, "DUALARM_ISR", NULL);
	if (ret) {
		DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Unable to request IRQ, arm2_to_arm1_int_3 irq=%d\n", arm2_to_arm1_int_3);
		return ret;
	}

	DUALARM_LOG(DUALARM_LOG_LVL_INFO, "arm2_to_arm1_int_3 is %d\n", arm2_to_arm1_int_3);
#endif

    ret = misc_register(&dualarm_misc_dev);
    if (ret) {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "Unable to register \"dualarm\" misc device\n");
        return ret;
    }

    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm probe function leave\n");

    return ret;
}
static int /*__devexit*/ dualarm_remove(struct platform_device *dev)
{

    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm remove\n");
    misc_deregister(&dualarm_misc_dev);

    return 0;
}

//add by hyw for test
#define CKGEN_BASE_ADDRESS              (0x10005000)
#define CKGEN_REG_CKGEN_BACKUP0_CTRL0   (CKGEN_BASE_ADDRESS + 0x0E40)
#define CKGEN_REG_CKGEN_BACKUP0_CTRL1   (CKGEN_BASE_ADDRESS + 0x0E50)
#define CKGEN_REG_CKGEN_BACKUP0_CTRL2   (CKGEN_BASE_ADDRESS + 0x0E60)

#define RG_CVBSREFPLL_EN_MASK           (0x00000080)
#define RG_CVBSPLL_EN_MASK              (0x00000080)
#define RG_CVBSPLL_DIV2_EN_MASK         (0x00000100)

#define PLLGP2_BASE_ADDRESS             (0x12106000)
#define PLLGP2_REG_REG_ANA_PLLGP_CFG0   (PLLGP2_BASE_ADDRESS + 0x0010)
#define PLLGP2_REG_REG_ANA_PLLGP_CFG1   (PLLGP2_BASE_ADDRESS + 0x0014)
//=======

static int dualarm_suspend(struct device *dev)
{
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm suspend\n");
    return 0;
}

static int dualarm_resume(struct device *dev)
{
    DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm resume\n");
    //u4ARM2Start();
    return 0;
}

static const struct dev_pm_ops dualarm_dev_pm_ops = {
        SET_SYSTEM_SLEEP_PM_OPS(dualarm_suspend,dualarm_resume)
};

#define DRV_NAME "dualarm-dev"

static struct platform_driver dualarm_driver = {
    .probe = dualarm_probe,
    .remove = dualarm_remove,

    .driver = {
        .name = DRV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = dualarm_of_match,
        .pm = &dualarm_dev_pm_ops,
    },
};

static int __init dualarm_init(void)
{
    int ret = 0;

    /*Platform Driver initialization*/
    ret = platform_driver_register(&dualarm_driver);
    memset((void *)g_arm2_modules, 0x0, sizeof(g_arm2_modules));

    if(0 == ret) {
        DUALARM_LOG(DUALARM_LOG_LVL_INFO, "dualarm_init success\n");
    } else {
        DUALARM_LOG(DUALARM_LOG_LVL_ERR, "dualarm_init failed\n");
    }

    return ret;
}

static void __exit dualarm_exit(void)
{
    platform_driver_unregister(&dualarm_driver);
    free_irq(arm2_to_arm1_int_0, NULL);
    free_irq(arm2_to_arm1_int_1, NULL);
#ifdef CONFIG_ATC_ARM2_LOG_BUFFER
    free_irq(arm2_to_arm1_int_3, NULL);
#endif
}

module_init(dualarm_init);
module_exit(dualarm_exit);
EXPORT_SYMBOL(HWSendMessage);
EXPORT_SYMBOL(HWGetMessage);
EXPORT_SYMBOL(backcar_send_message);
EXPORT_SYMBOL(backcar_get_message);
EXPORT_SYMBOL(display_send_message);
EXPORT_SYMBOL(display_get_message);

MODULE_AUTHOR("ATC");
MODULE_DESCRIPTION("dualarm Driver");
MODULE_LICENSE("GPL");

