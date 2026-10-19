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

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/of_gpio.h>
#include <linux/mutex.h>

#define ARM2_FISHEYE_USE_CPU 3

#include "backcar_msg.h"
#include "backcar_log.h"
#include "backcardrv.h"
#ifdef CONFIG_PM_WAKELOCKS
#include <linux/pm_wakeup.h>
static struct wakeup_source bc_wake_lock;
#else
#include <linux/wakelock.h>
static struct wake_lock bc_wake_lock;
#endif

#include "metazone/metazone.h"
#include "metazone/metazone_ioctl.h"
//=========================================================
// declare dualarm interface functions
#define BACKCAR_VERSION            "01.01.03"
#define CONFIG_ATC_ARM2_ENABLE

#ifdef CONFIG_ATC_ARM2_ENABLE
#include "ac8257.h"
#include "dual_hal.h"
#include "dualarm.h"
#include <rsv_cpu_notifier.h>
#endif

#ifdef CONFIG_ATC_ARM2_ENABLE
extern int  request_dualarm_irq(unsigned int module, irq_handler_t handler, unsigned long flags,const char *name, void *dev);
#endif

extern int tvd_hw_stop(void);
extern int tvd_hw_deinit(void);
extern void nr_hw_deInit(void);
extern unsigned int di_hw_stop(void);
extern unsigned int di_hw_deinit(void);
extern int imgsensor_init(void);
extern signed int ISP_Init(void);
bool arm2_exit = false;
EXPORT_SYMBOL(arm2_exit);

u8 *backcar_log_level_str[] = {\
		"emerg",\
		"alert",\
		"crit",\
		"err",\
		"warning",\
		"notice",\
		"info",\
		"debug",\
	};\

#ifdef  LOG_TAG
#undef  LOG_TAG
#define LOG_TAG    "[BackCarDrv]"
#endif

/*memory information for a range of memory*/
struct memory_info {
	unsigned int start_addr;
	unsigned int size;
};

#define BACKCAR_NOTIFY_MCU_GPIO_HIGH       1
#define BACKCAR_NOTIFY_MCU_GPIO_LOW        0
#define METAZONE_FAST_AVM_MODE             0x10048

/************    [internal variables]    ***********************/
#ifdef CONFIG_ATC_ARM2_ENABLE
static wait_queue_head_t *g_ioctlwaitq = NULL;
static unsigned g_arm2backcarstatus = ARM2_STATUS_UNKNOWN;
#endif

static struct device_node *backcar_node = NULL;

#ifdef CONFIG_MEMBLK_RELEASE_POLICY
static struct device_node *arm2_backcar_ui_node = NULL;
static phys_addr_t vramsize = 0;
static phys_addr_t backcar_ui_base_pa = 0;
static struct device_node *arm2_backcar_mdp_node = NULL;
static phys_addr_t backcar_mdp_base_pa = 0;
static phys_addr_t backcar_mdp_size = 0;
static struct device_node *arm2_backcar_isp_node = NULL;
static phys_addr_t backcar_isp_base_pa = 0;
static phys_addr_t backcar_isp_mem_size = 0;
static bool is_free_reserved_memory = false;
#endif

static int backcar_sdbc_gpio_num = -1;
static int backcar_hdbc_gpio_num = -1;
static int backcar_hdavm_gpio_num = -1;
static int backcar_mcu_gpio_num = -1;

static DEFINE_MUTEX(g_ioctl_mutex);

static void GPIO_Init(void)
{
	int ret = -1;
	int cur_gpio_val = 0;
	ret = gpio_request(backcar_sdbc_gpio_num, "BackCar_SDBC_GPIO");
	if (ret < 0) {
		BACKCAR_LOG(BACKCAR_ERR, "request backcar SDBC gpio bc_failed\n");
		//return;
	}

	if (0 != gpio_direction_input(backcar_sdbc_gpio_num)) {
		BACKCAR_LOG(BACKCAR_ERR, "set backcar SDBC gpio direction input bc_failed\n");
		//return;
	}

	ret = gpio_request(backcar_hdbc_gpio_num, "BackCar_HDBC_GPIO");
	if (ret < 0) {
		BACKCAR_LOG(BACKCAR_ERR, "request backcar HDBC gpio bc_failed\n");
		//return;
	}

	if (0 != gpio_direction_input(backcar_hdbc_gpio_num)) {
		BACKCAR_LOG(BACKCAR_ERR, "set backcar HDBC gpio direction input bc_failed\n");
		//return;
	}

	ret = gpio_request(backcar_hdavm_gpio_num, "BackCar_HDAVM_GPIO");
	if (ret < 0) {
		BACKCAR_LOG(BACKCAR_ERR, "request backcar HDAVM gpio bc_failed\n");
		//return;
	}

	if (0 != gpio_direction_input(backcar_hdavm_gpio_num)) {
		BACKCAR_LOG(BACKCAR_ERR, "set backcar HDAVM gpio direction input bc_failed\n");
		//return;
	}

	ret = gpio_request(backcar_mcu_gpio_num, "BackCar_MCU_GPIO");
	if (ret < 0) {
		BACKCAR_LOG(BACKCAR_ERR, "request backcar mcu gpio bc_failed\n");
		//return;
	}

	if (0 != gpio_direction_input(backcar_mcu_gpio_num)) {
		BACKCAR_LOG(BACKCAR_ERR, "set backcar mcu gpio direction input bc_failed\n");
		//return;
	}
	cur_gpio_val = gpio_get_value(backcar_mcu_gpio_num);
	gpio_direction_output(backcar_mcu_gpio_num, cur_gpio_val);
}

static unsigned GPIO_Get_value(void)
{
#ifndef BUILD_JAC_BSP
	unsigned val_sdbc = !!gpio_get_value(backcar_sdbc_gpio_num);
	unsigned val_hdbc = !!gpio_get_value(backcar_hdbc_gpio_num);
	unsigned val_hdavm = !!gpio_get_value(backcar_hdavm_gpio_num);
//    BACKCAR_LOG(BACKCAR_INFO,"lwn,not BUILD_JAC_BSP");
#else
    unsigned val_sdbc = 0;
    unsigned val_hdavm = 0;
	unsigned val_hdbc = !gpio_get_value(backcar_hdbc_gpio_num);
//    BACKCAR_LOG(BACKCAR_INFO,"lwn,BUILD_JAC_BSP");
#endif
	return ((val_hdavm << 2) | (val_hdbc << 1) | (val_sdbc << 0));
}

#ifdef CONFIG_ATC_ARM2_ENABLE
static irqreturn_t backcar_isr_handler(int irq, void *dev_id)
{
	unsigned int u4Param1, u4Param2, u4Param3;
	unsigned int u4ModuleMsgUnion, u4MessageID;

	BACKCAR_LOG(BACKCAR_INFO, "enter\n");

	backcar_get_message(&u4ModuleMsgUnion, &u4Param1, &u4Param2, &u4Param3);

	u4MessageID = GETMESSAGEID(u4ModuleMsgUnion);
	if (u4MessageID == MSG_ARM2_RESPONSE) {
		if (u4Param1 == ARM2_STATUS_NO_BACK_CAR||u4Param1 == ARM2_STATUS_BACKING_CAR) {
			g_arm2backcarstatus = u4Param1;
			BACKCAR_LOG(BACKCAR_INFO, "Rev ARM2 Responese and arm2 status is %d\n",g_arm2backcarstatus);
		} else {
			BACKCAR_LOG(BACKCAR_INFO, "Rev ARM2 Responese, but it is unnormal, the param1 is %d\n", u4Param1);
		}
	}

	if (NULL != g_ioctlwaitq) {
		wake_up(g_ioctlwaitq);
	}else{
		BACKCAR_LOG(BACKCAR_WARNING, "g_ioctlwaitq has been waked. but not by arm2 msg\n");
	}

	BACKCAR_LOG(BACKCAR_INFO, "leave and arm2 backcar status is %d\n",g_arm2backcarstatus);
	return IRQ_HANDLED;
}
#endif

struct memory_info g_info;

static int get_rsv_mem_info(void)
{
	struct device_node *node = NULL;
	unsigned int property[4] = {0};

	BACKCAR_LOG(BACKCAR_INFO, "enter\n");

	node = of_find_compatible_node(NULL,NULL,"atc-wchdi-disp");
	if (node) {
		if (of_property_read_u32_array(node, "reg", (u32 *)property, 4)) {
			BACKCAR_LOG(BACKCAR_ERR, "get di out reserved memory node reg info bc_fail\n");
			return -1;
		}
		g_info.start_addr = property[1];
		g_info.size = property[3];
		BACKCAR_LOG(BACKCAR_INFO, "di out buffer pa:%x, size:%x\n", g_info.start_addr,g_info.size);
	} else {
		BACKCAR_LOG(BACKCAR_ERR, "find di out data reserved memory node bc_fail\n");
		return -1;
	}
	BACKCAR_LOG(BACKCAR_INFO, "leave\n");
	return 0;
}

static int backcar_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	BACKCAR_LOG(BACKCAR_INFO, "enter and pVma->vm_pgoff = 0x%lx", pVma->vm_pgoff);

	pVma->vm_pgoff = g_info.start_addr>>PAGE_SHIFT;

	if ((pVma->vm_end - pVma->vm_start) > g_info.size) {
		BACKCAR_LOG(BACKCAR_ERR, "mmap size range over flow, so bc_failed\n");
		return -EAGAIN;
	}

	if (remap_pfn_range(pVma, pVma->vm_start, pVma->vm_pgoff,pVma->vm_end - pVma->vm_start, pVma->vm_page_prot)) {
		BACKCAR_LOG(BACKCAR_ERR, "mmap di data memory bc_failed\n");
		return -EAGAIN;
	}
	BACKCAR_LOG(BACKCAR_INFO, "leave\n");
	return 0;
}

#ifdef CONFIG_ATC_ARM2_ENABLE
extern atomic_t arm2_running;
extern void cmdq_disable_all_clock(int flag);
extern void isp_larb_clock_off(void);
#endif

#ifdef CONFIG_MEMBLK_RELEASE_POLICY
extern int free_memblock_runtime(phys_addr_t phy_addr,phys_addr_t size);
#endif

#ifdef CONFIG_ATC_ARM2_ENABLE
static void backcar_force_stop_arm2(void)
{
	BACKCAR_LOG(BACKCAR_INFO, "backcar_force_stop_arm2 enter\n");
	BACKCAR_LOG(BACKCAR_INFO, "info arm2 backcar MSG_ANDROID_IPO\n");
	backcar_send_message(MSG_COMBINE(MODULE_BCAR, MSG_ANDROID_STOP_ARM2),0,0,0);
	BACKCAR_LOG(BACKCAR_INFO, "backcar_force_stop_arm2 leave\n");
}
#endif

static long backcar_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	u_long size = 0;
	int ret = 0;
	unsigned gpio_value = 0;
#ifdef CONFIG_ATC_ARM2_ENABLE
	wait_queue_head_t wait_queue_head;
	DECLARE_WAITQUEUE(cur_wait_queue, current);
#endif

	if (cmd != IOCTL_BC_DETECTGPIO) {
		//BACKCAR_LOG(BACKCAR_INFO, "cmd = 0x%x\n", cmd);
	}

	size = (cmd & IOCSIZE_MASK) >> IOCSIZE_SHIFT;

	if (cmd & IOC_IN) {
		if (!access_ok(VERIFY_READ, argp, size)) {
			BACKCAR_LOG(BACKCAR_ERR, "backcar_ioctl(): error 1, so bc_failed\n");
			return -EFAULT;
		}
	}

	if (cmd & IOC_OUT) {
		if (!access_ok(VERIFY_WRITE, argp, size)) {
			BACKCAR_LOG(BACKCAR_ERR, "backcar_ioctl(): error 2, so bc_failed\n");
			return -EFAULT;
		}
	}

	switch(cmd){
		case IOCTL_FSC_NOTIFY_APP_READY:
		{
			BACKCAR_LOG(BACKCAR_INFO, "execute IOCTL_FSC_NOTIFY_APP_READY ioctl and version is %s\n", BACKCAR_VERSION);
			mutex_lock(&g_ioctl_mutex);
			if (arm2_exit) {
				BACKCAR_LOG(BACKCAR_INFO, "wait arm2 already exit done\n");
				mutex_unlock(&g_ioctl_mutex);
				break;
			}
#ifdef CONFIG_ATC_ARM2_ENABLE
			init_waitqueue_head(&wait_queue_head);
			add_wait_queue(&wait_queue_head, &cur_wait_queue);
			g_ioctlwaitq = &wait_queue_head;
			set_current_state(TASK_INTERRUPTIBLE);

			BACKCAR_LOG(BACKCAR_INFO, "Inform Arm2, Arm1 Backcar apk is ready\n");
			#ifdef CONFIG_PM_WAKELOCKS
			__pm_stay_awake(&bc_wake_lock);
			#else
			wake_lock(&bc_wake_lock);
			#endif
			backcar_send_message(MSG_COMBINE(MODULE_BCAR, MSG_ANDROID_APP_READY),0,0,0);
			BACKCAR_LOG(BACKCAR_INFO, "wait arm2 interrupt before schedule\n");
			schedule();
			BACKCAR_LOG(BACKCAR_INFO, "wait arm2 interrupt after schedule\n");

			if (signal_pending(current)) {
				BACKCAR_LOG(BACKCAR_WARNING, "interrupted by system signal\n");
				remove_wait_queue(&wait_queue_head, &cur_wait_queue);
				set_current_state(TASK_RUNNING);
				g_ioctlwaitq = NULL;
				ret = -ERESTARTSYS;
				#ifdef CONFIG_PM_WAKELOCKS
				__pm_relax(&bc_wake_lock);
				#else
				wake_unlock(&bc_wake_lock);
				#endif
				mutex_unlock(&g_ioctl_mutex);
				break;
			}

			BACKCAR_LOG(BACKCAR_INFO, "wait arm2 interrupt done\n");
			isp_larb_clock_off();
			atomic_set(&arm2_running, 0);
			cmdq_disable_all_clock(1);
			rsv_cpu_notifier_call_chain(
					RSV_CPU_EVENT(RSV_CPU_EVENT_RELEASE, ARM2_FISHEYE_USE_CPU), NULL);
			#ifdef CONFIG_PM_WAKELOCKS
			__pm_relax(&bc_wake_lock);
			#else
			wake_unlock(&bc_wake_lock);
			#endif
			arm2_exit = true;
#else
			arm2_exit = true;
#endif
#ifdef CONFIG_MEMBLK_RELEASE_POLICY
			if(!is_free_reserved_memory){
				free_memblock_runtime((phys_addr_t)backcar_ui_base_pa,(phys_addr_t)vramsize);
				free_memblock_runtime((phys_addr_t)backcar_mdp_base_pa,(phys_addr_t)backcar_mdp_size);
				free_memblock_runtime((phys_addr_t)backcar_isp_base_pa,(phys_addr_t)backcar_isp_mem_size);
				is_free_reserved_memory = true;
				BACKCAR_LOG(BACKCAR_INFO, "free arm2 reserved memory done\n");
			}else{
				BACKCAR_LOG(BACKCAR_ERR, "arm2 backcar ui already free!");
			}
#endif
#ifdef CONFIG_ATC_ARM2_ENABLE
			remove_wait_queue(&wait_queue_head, &cur_wait_queue);
			set_current_state(TASK_RUNNING);
			ret = copy_to_user(argp, &g_arm2backcarstatus, sizeof(unsigned));
			if (ret != 0) {
				BACKCAR_LOG(BACKCAR_ERR, "copy arm2 backcar status to userspace bc_failed\n");
			}
			g_ioctlwaitq = NULL;
#endif

			mutex_unlock(&g_ioctl_mutex);
		}
		break;
		case IOCTL_BC_GPIOINIT:
		{
			GPIO_Init();
		}
		break;
		case IOCTL_BC_DETECTGPIO:
		{
			gpio_value = GPIO_Get_value();
			ret = copy_to_user(argp, &gpio_value, sizeof(gpio_value));
			if (ret != 0) {
				BACKCAR_LOG(BACKCAR_ERR, "copy gpio status to userspace bc_failed\n");
			}
		}
		break;
		case IOCTL_BC_FORCE_STOP_ARM2:
		{
#ifdef CONFIG_ATC_ARM2_ENABLE
			backcar_force_stop_arm2();
#endif
		}
		break;

		case IOCTL_MCU_BC_START_SUCCESS:
		{
			gpio_set_value_cansleep(backcar_mcu_gpio_num, BACKCAR_NOTIFY_MCU_GPIO_HIGH);
			BACKCAR_LOG(BACKCAR_INFO, "MCU Test: current gpio number = %d, gpio value = %d\n", backcar_mcu_gpio_num, gpio_get_value(backcar_mcu_gpio_num));

		}
		break;
		case IOCTL_MCU_BC_START_STOP:
		{
			gpio_set_value_cansleep(backcar_mcu_gpio_num, BACKCAR_NOTIFY_MCU_GPIO_LOW);
			BACKCAR_LOG(BACKCAR_INFO, "MCU Test: current gpio number = %d, gpio value = %d\n", backcar_mcu_gpio_num, gpio_get_value(backcar_mcu_gpio_num));
		}
		break;
		case IOCTL_GET_RSV_MEM_INFO:
		{
			ret = get_rsv_mem_info();
			if (0 != ret) {
				BACKCAR_LOG(BACKCAR_ERR, "get reserved memory info bc_failed\n");
			} else {
				if (copy_to_user(argp, &g_info, sizeof(g_info)) != 0) {
					BACKCAR_LOG(BACKCAR_ERR, "copy reserved memory infor to userspace bc_failed\n");
				}
			}
		}
		break;
		case IOCTL_MCU_STOP_BUFFER_GENERATOR:
		{
			ret = tvd_hw_stop();
			if (ret != 0) {
				BACKCAR_LOG(BACKCAR_ERR, "stop tvd bc_failed\n");
			}

			ret = tvd_hw_deinit();
			if (ret != 0) {
				BACKCAR_LOG(BACKCAR_ERR, "deinit tvd bc_failed\n");
			}

			nr_hw_deInit();
			ret = di_hw_stop();
			if (ret != 0) {
				BACKCAR_LOG(BACKCAR_ERR, "stop di bc_failed\n");
			}

			ret = di_hw_deinit();
			if (ret != 0) {
				BACKCAR_LOG(BACKCAR_ERR, "deinit di bc_failed\n");
			}
			BACKCAR_LOG(BACKCAR_INFO, "MCU Test: arm1. after stop buffer generator(tvd,nr,di);");
		}
		break;
		case IOCTL_GET_ARM2_STATUS:
		{
			unsigned int arm2_status = ARM2_STATUS_UNKNOWN;
			#ifdef CONFIG_ATC_ARM2_ENABLE
			arm2_status = arm2_exit ? ARM2_STATUS_NO_BACK_CAR : ARM2_STATUS_UNKNOWN;
			#else
			arm2_status = ARM2_STATUS_NO_BACK_CAR ;
			#endif
			ret = copy_to_user(argp, &arm2_status, sizeof(unsigned));
			if (ret != 0) {
				BACKCAR_LOG(BACKCAR_ERR, "copy arm2 status(%d) to userspace bc_failed\n", arm2_status);
			}
		}
		break;

		default:
			BACKCAR_LOG(BACKCAR_WARNING, "this ioctl(%d) is not supported", cmd);
	}

	return ret;
}

static int backcar_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int backcar_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations backcardrv_fops = {
	.owner            = THIS_MODULE,
	.open = backcar_open,
	.release = backcar_release,
	.unlocked_ioctl   = backcar_ioctl,
	.mmap = backcar_mmap,
};

static struct miscdevice backcar_misc_dev = {
	MISC_DYNAMIC_MINOR,
	"backcardrv",
	&backcardrv_fops
};

static int backcar_probe(struct platform_device * pdev)
{
	int ret = 0;
#ifdef CONFIG_MEMBLK_RELEASE_POLICY
	unsigned int property[4];
#endif
	BACKCAR_LOG(BACKCAR_INFO, "enter\n");

	ret = misc_register(&backcar_misc_dev);
	if (ret) {
		BACKCAR_LOG(BACKCAR_ERR, "leave and register misc device bc_failed\n");
		return ret;
	}
#ifdef CONFIG_MEMBLK_RELEASE_POLICY
	arm2_backcar_ui_node = of_find_compatible_node(NULL, NULL, "autochips,arm2-backcar-ui");

	if(arm2_backcar_ui_node){
		if(of_property_read_u32_array(arm2_backcar_ui_node, "reg", (u32 *)property, 4)) {
			BACKCAR_LOG(BACKCAR_ERR, "[autochips] get arm2 backcar ui reserved memory node reg info fail\n");
		}
		BACKCAR_LOG(BACKCAR_INFO, "[autochips] get arm2 backcar ui reserved memory start is %x,size:%x\n",property[1],property[3]);
		backcar_ui_base_pa = property[1];
		vramsize = property[3];
	}else{
		BACKCAR_LOG(BACKCAR_INFO, "[autochips] fail to get arm2 backcar ui node\n");
	}

	arm2_backcar_mdp_node = of_find_compatible_node(NULL, NULL, "autochips,mdp");

	if(arm2_backcar_mdp_node){
		if(of_property_read_u32_array(arm2_backcar_mdp_node, "reg", (u32 *)property, 4)) {
			BACKCAR_LOG(BACKCAR_INFO, "[autochips] get arm2 backcar mdp reserved memory node reg info fail\n");
		}
		BACKCAR_LOG(BACKCAR_INFO, "[autochips] get arm2 backcar mdp reserved memory start is %x,size:%x\n",property[1],property[3]);
		backcar_mdp_base_pa = property[1];
		backcar_mdp_size = property[3];
	}else{
		BACKCAR_LOG(BACKCAR_INFO, "[autochips] fail to get arm2 backcar mdp node\n");
	}

	arm2_backcar_isp_node = of_find_compatible_node(NULL, NULL, "autochips,isp");
	if(arm2_backcar_isp_node){
		if(of_property_read_u32_array(arm2_backcar_isp_node, "reg", (u32 *)property, 4)) {
			BACKCAR_LOG(BACKCAR_INFO, "[autochips] get arm2 backcar isp reserved memory node reg info fail\n");
		}
		BACKCAR_LOG(BACKCAR_INFO, "[autochips] get arm2 backcar isp reserved memory start is %x,size:%x\n",property[1],property[3]);
		backcar_isp_base_pa = property[1];
		backcar_isp_mem_size = property[3];
	}else{
		BACKCAR_LOG(BACKCAR_INFO, "[autochips] fail to get arm2 backcar isp node\n");
	}
#endif
	BACKCAR_LOG(BACKCAR_INFO, "leave and minor number is %d\n",backcar_misc_dev.minor);
	return ret;
}

static int backcar_remove(struct platform_device *dev)
{
	BACKCAR_LOG(BACKCAR_INFO, "enter\n");
	misc_deregister(&backcar_misc_dev);
	BACKCAR_LOG(BACKCAR_INFO, "leave\n");
	return 0;
}

#include <linux/pm.h>
static int backcar_suspend(struct platform_device* pdev, pm_message_t state)
{
#ifdef CONFIG_ATC_ARM2_ENABLE
	BACKCAR_LOG(BACKCAR_INFO, "enter state.event = %d, arm2_running = %d\n", state.event, atomic_read(&arm2_running));
	if (state.event == PM_EVENT_SUSPEND && 0 != atomic_read(&arm2_running)) {
		backcar_ioctl(NULL, IOCTL_BC_FORCE_STOP_ARM2, 0);
		backcar_ioctl(NULL, IOCTL_FSC_NOTIFY_APP_READY, 0);
	}
#endif
	return 0;
}

static int backcar_resume(struct platform_device* pdev)
{
	BACKCAR_LOG(BACKCAR_INFO, "enter\n");
	return 0;
}

static struct platform_driver backcar_driver = {

	.driver = {
		.name = "backcar-dev",
		.owner = THIS_MODULE,
	},
	.probe = backcar_probe,
	.remove = backcar_remove,
	.suspend = backcar_suspend,
	.resume = backcar_resume,
};

static struct platform_device backcar_device = {

	.name = "backcar-dev",
	.id = -1,

};

static int __init backcar_drv_init(void)
{
	int ret = 0;

	BACKCAR_LOG(BACKCAR_INFO, "enter\n");

	ret = platform_device_register(&backcar_device);
	if (ret) {
		BACKCAR_LOG(BACKCAR_ERR, "register backcar device bc_failed\n");
		return ret;
	}

	ret = platform_driver_register(&backcar_driver);
	if (ret) {
		BACKCAR_LOG(BACKCAR_ERR, "register backcar dirver bc_failed\n");
		platform_device_unregister(&backcar_device);
		return ret;
	}

#ifdef CONFIG_ATC_ARM2_ENABLE
	ret = request_dualarm_irq(MODULE_BCAR, backcar_isr_handler, 0, "BACKCAR_ISR", NULL);
	if (ret) {
		BACKCAR_LOG(BACKCAR_ERR, "request dualarm irq bc_failed\n");
		platform_device_unregister(&backcar_device);
		platform_driver_unregister(&backcar_driver);
		return ret;
	}
#endif

	backcar_node = of_find_compatible_node(NULL, NULL, "mediatek,ac8257-backcar");
	if (backcar_node) {
		backcar_sdbc_gpio_num = of_get_named_gpio(backcar_node, "backcar_sdbc_gpio", 0);
		if (!gpio_is_valid(backcar_sdbc_gpio_num)) {
			BACKCAR_LOG(BACKCAR_ERR,  "get backcar SDBC gpio number bc_failed\n");
		}

		backcar_hdbc_gpio_num = of_get_named_gpio(backcar_node, "backcar_hdbc_gpio", 0);
		if (!gpio_is_valid(backcar_hdbc_gpio_num)) {
			BACKCAR_LOG(BACKCAR_ERR,  "get backcar HDBC gpio number bc_failed\n");
		}

		backcar_hdavm_gpio_num = of_get_named_gpio(backcar_node, "backcar_hdavm_gpio", 0);
		if (!gpio_is_valid(backcar_hdavm_gpio_num)) {
			BACKCAR_LOG(BACKCAR_ERR,  "get backcar HDAVM gpio number bc_failed\n");
		}

		backcar_mcu_gpio_num = of_get_named_gpio(backcar_node, "backcar_mcu_notify_gpio", 0);
		if (!gpio_is_valid(backcar_mcu_gpio_num)) {
			BACKCAR_LOG(BACKCAR_ERR, "get backcar mcu gpio number bc_failed\n");
		}
		BACKCAR_LOG(BACKCAR_INFO, "gpio number SDBC %u, HDBC %u, HDAVM %u, mcu %u\n", 
				backcar_sdbc_gpio_num, backcar_hdbc_gpio_num, backcar_hdavm_gpio_num, backcar_mcu_gpio_num);
	}

	//GPIO_Init();
	//BACKCAR_LOG(BACKCAR_WARNING, "GPIO_Get_value() = %X\n", GPIO_Get_value());

	imgsensor_init();
	ISP_Init();

	#ifdef CONFIG_PM_WAKELOCKS
	wakeup_source_init(&bc_wake_lock, "bc_lock_wakelock");
	#else
	wake_lock_init(&bc_wake_lock, WAKE_LOCK_SUSPEND, "bc_lock_wakelock");
	#endif
	BACKCAR_LOG(BACKCAR_INFO, "leave\n");
	return ret;
}

static void __exit backcar_drv_exit(void)
{
	BACKCAR_LOG(BACKCAR_INFO, "enter\n");
	platform_driver_unregister(&backcar_driver);
	platform_device_unregister(&backcar_device);
	BACKCAR_LOG(BACKCAR_INFO, "leave\n");
}


module_init(backcar_drv_init);
module_exit(backcar_drv_exit);
MODULE_LICENSE("GPL");

