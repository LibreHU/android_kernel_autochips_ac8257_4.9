/* Copyright Statement:
 *
 * This software/firmware and related documentation ("AutoChips Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to AutoChips Inc. and/or its licensors. Without
 * the prior written permission of AutoChips inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of AutoChips Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * AutoChips Inc. (C) 2018. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
 * RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
 * SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
 * AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/miscdevice.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/namei.h>
#include "display_inc.h"
#include "log.h"
#include "vdp_hw.h"
#include "bringup.h"
#include "vdp_hal.h"
#include "pmx_hal.h"
#include "di_if.h"
#include "wch_if.h"
#include "mtk_sync.h"
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/clk.h>

#include "di_hal.h"

#define DI_IN_BUF_YSIZE      (720*576)
extern DI_PRM_T PALPARAM[4];

#define ATC_KERNEL_LINUX_LICENSE     "GPL"
#define ATC_DI_DRIVER_INFO	"ATC De-Interlance driver"

void __iomem *fmtf_reg;
void __iomem *vdo_8tap_reg;

void __iomem *vdof_reg;
void __iomem *clk_reg;

struct clk *di_clk;
struct clk *di_parent_clk;

unsigned int fmtf_irq;
unsigned int fmtr_irq;
unsigned long fbm_base;
unsigned long fbm_size;
void *fbm_va;
void *g_drvInst;
unsigned int g_src_width;
unsigned int g_src_height;
unsigned int singleChannelFlag;
#define DI_SINGLE_CHANNEL        (1 << 1)

struct di_device {
	struct device *dev;
	struct workqueue_struct *update_di_wq;
	struct mutex timeline_lock;
	struct sync_timeline *timeline_in;
	struct sync_timeline *timeline_out;
	int timeline_max_in;
	int timeline_max_out;
	di_cfg_bufs buf_list[BUFFER_NUM];
	int listIndex;
};
extern struct workqueue_struct *cvbs_wq;

extern void di_inital_vfy_param(int w, int h);
extern void msleep(unsigned int msecs);

int di_test_func(int param, int w, int h)
{
	int i = 0;
	di_params params = {0};
	uint32_t in_addr = 0x60800000;
	uint32_t out_addr = 0x60B00000;
	u32 y_addr[2] = {0}, c_addr[2] = {0};

	params.src_height = h;
	params.src_width = w;
	params.dst_height = h;
	params.dst_width = w;

	params.dst_rect.top = 0;
	params.dst_rect.bottom = h;
	params.dst_rect.left = 0;
	params.dst_rect.right = w;
	params.src_rect.top = 0;
	params.src_rect.bottom = h;
	params.src_rect.left = 0;
	params.src_rect.right = w;
	params.enable_hw = true;
	params.scan_line_mode = 1;
	di_hal_set_params(AVM, &params);

	di_hal_init(AVM);
	y_addr[0] = in_addr;
	y_addr[1] = in_addr + DI_IN_BUF_YSIZE * 2;
	c_addr[0] = in_addr + DI_IN_BUF_YSIZE;
	c_addr[1] = in_addr + DI_IN_BUF_YSIZE +  DI_IN_BUF_YSIZE * 2;
	di_read_file(y_addr[0], c_addr[0], 0, false);
	di_read_file(y_addr[1], c_addr[1], 1, false);
#if DI_DUMP_BUFFER_ATTR
	di_save_yc_addr(DI_IN_BUF, 0, y_addr[0], c_addr[0]);
	di_save_yc_addr(DI_IN_BUF, 1, y_addr[1], c_addr[1]);
#endif
	for (i = 0; i < 3; i++) {
		out_addr += DI_OUT_BUF_SIZE * (i%DI_BUF_MAX_CNT);
#if DI_DUMP_BUFFER_ATTR
		di_save_yc_addr(DI_OUT_BUF, i%DI_BUF_MAX_CNT, out_addr, DI_OUT_BUF_SIZE);
#endif
		di_work(AVM, (void *)y_addr, (void *)c_addr, out_addr, 1);
		msleep(100);
		pr_info("################ %d outaddr:%x\n", i, out_addr);
	}
	pr_info("##### w, h %d %d \n", w, h);
#if DI_DUMP_BUFFER_ATTR
	di_dump_buffer(DI_IN_BUF, 0);
	di_dump_buffer(DI_IN_BUF, 1);
	di_dump_buffer(DI_OUT_BUF, 0);
	di_dump_buffer(DI_OUT_BUF, 1);
	di_dump_register();
#endif
	di_hal_deinit(AVM);
	params.enable_hw = false;
	di_hal_set_params(AVM, &params);
	return 0;
}

static ssize_t di_file_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t di_file_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int ret = 0;
	unsigned int on = 0;
	unsigned int w = 0, h = 0;

	VDO_LOG(VDO_LOG_LVL_INFO, "*****************di test*****************\n");

	ret = sscanf(buf, "%u %u %u", &on, &w, &h);
	if (ret < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR,  "di_file_store sscanf error!\n");
		return -EINVAL;
	}

	di_test_func(on, w, h);

	return count;
}
static DEVICE_ATTR(di4file, S_IWUSR | S_IRUGO, di_file_show, di_file_store);

/*set enable 3dnr end*/
#ifndef ATC_ARM2_LK_SUPPORT
#if DI_DUMP_BUFFER_ATTR
static int di_mkdir(const char *name, umode_t mode)
{
	struct dentry *dentry = NULL;
	struct path path = {0};
	int err = 0;

	vss_printk(VSS_LOG_INFO, "enter\n");

	dentry = kern_path_create(AT_FDCWD, name, &path, LOOKUP_DIRECTORY);
	if (IS_ERR(dentry)) {
		vss_printk(VSS_LOG_ERR, "get dentry failed and error number is %ld\n", PTR_ERR(dentry));
		return PTR_ERR(dentry);
	}

	err = vfs_mkdir(path.dentry->d_inode, dentry, mode);
	if (err) {
		vss_printk(VSS_LOG_ERR, "create %s directory failed\n", name);
	}
	done_path_create(&path, dentry);
	vss_printk(VSS_LOG_INFO, "leave\n");
	return err;
}

static ssize_t di_dumpbuffer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	VDO_LOG(VDO_LOG_LVL_INFO, "*****************Dump Register Start*****************\n");
	di_mkdir("/data/di", 0755);
	di_dump_register();
	VDO_LOG(VDO_LOG_LVL_INFO, "*****************Dump Register End*******************\n");

	return 0;
}

static ssize_t di_dumpbuffer_store(struct device *dev, struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int ret = 0;
	unsigned char cmd[20] = "";
	unsigned int di_io = 0;    //default 0 mean in_buf, 1 mean out_buf, 2 mean in_buf&out_buf
	unsigned int dump_num = 5; //default 5
	unsigned int idx = 0;
	bool inCheck = false;
	bool outCheck = false;

	VDO_LOG(VDO_LOG_LVL_INFO, "di_dumpbuffer_store Parameters: [dumpDI]\n");

	ret = sscanf(buf, "%8s 0x%x %x", (char *)&cmd, (unsigned int *)&dump_num, (unsigned int *)&di_io);

	if (0 == di_io) {
		inCheck = true;
	} else if (1 == di_io) {
		outCheck = true;
	} else if (2 == di_io) {
		inCheck = true;
		outCheck = true;
	}

	VDO_LOG(VDO_LOG_LVL_INFO, "di_dumpbuffer_store: cmd[%s], dump_num[%x], di_io[%x]\n", cmd, dump_num, di_io);
	if (ret < 0) {
		VDO_LOG(VDO_LOG_LVL_ERR, "di_dumpbuffer_store sscanf error!\n");
		return -EINVAL;
	}
	di_mkdir("/data/di", 0755);

	if (strcmp(cmd, "dumpdi") == 0) {
		VDO_LOG(VDO_LOG_LVL_INFO, "*****************Dump Buffer Start*****************\n");
		for (; idx < dump_num; idx++) {
			if (inCheck && di_dump_buffer(DI_IN_BUF, idx)) {
				VDO_LOG(VDO_LOG_LVL_ERR, "di_dumpbuffer_store di_dump_buffer err! err_idx[%d]\n", idx);
				//return -EINVAL;
			}
			if (outCheck && di_dump_buffer(DI_OUT_BUF, idx)) {
				VDO_LOG(VDO_LOG_LVL_ERR, "di_dumpbuffer_store di_dump_buffer err! err_idx[%d]\n", idx);
				//return -EINVAL;
			}
		}
		VDO_LOG(VDO_LOG_LVL_INFO, "*****************Dump Buffer End*****************\n");
	} else if (strcmp(cmd, "wregVDO") == 0) {
		HAL_VDO_UNION_T *prVdofReg = (HAL_VDO_UNION_T *)vdof_reg;
		unsigned int reg = (dump_num & 0xff) >> 2;
		prVdofReg->au4Reg[reg] = di_io;

		VDO_LOG(VDO_LOG_LVL_WARN, "prVdofReg->au4Reg[%x] = %x\n", reg, di_io);
	} else if (strcmp(cmd, "rregVDO") == 0) {
		HAL_VDO_UNION_T *prVdofReg = (HAL_VDO_UNION_T *)vdof_reg;
		unsigned int reg = (dump_num & 0xff) >> 2;
		//prVdofReg->au4Reg[reg] = di_io;

		VDO_LOG(VDO_LOG_LVL_WARN, "prVdofReg->au4Reg[%x] = %x\n", reg, prVdofReg->au4Reg[reg]);
	} else if (strcmp(cmd, "wregFMT") == 0) {
		HAL_PMX_DISP_MAIN_UNION_T *prFmtfReg = (HAL_PMX_DISP_MAIN_UNION_T *)fmtf_reg;
		unsigned int reg = (dump_num & 0xff) >> 2;
		prFmtfReg->au4Reg[reg] = di_io;

		VDO_LOG(VDO_LOG_LVL_WARN, "prFmtfReg->au4Reg[%x] = %x\n", reg, di_io);
	} else if (strcmp(cmd, "rregFMT") == 0) {
		HAL_PMX_DISP_MAIN_UNION_T *prFmtfReg = (HAL_PMX_DISP_MAIN_UNION_T *)fmtf_reg;
		unsigned int reg = (dump_num & 0xff) >> 2;

		VDO_LOG(VDO_LOG_LVL_WARN, "prFmtfReg->au4Reg[%x] = %x\n", reg, prFmtfReg->au4Reg[reg]);
	}

	return count;
}
static DEVICE_ATTR(di4dump, S_IWUSR | S_IRUGO, di_dumpbuffer_show, di_dumpbuffer_store);
#endif
#endif

static long di_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	int ret = 0, i = 0, j = 0;
	physical_block phy_info = {0};
	struct di_device* di_dev = file->private_data;

	switch(cmd) {

	case DI_IOCTL_SET_PARAMS: {
		di_params param = {0};
		VDO_LOG(VDO_LOG_LVL_DBG, "DI_IOCTL_SET_PARAMS\n");

		if (copy_from_user(&param, (void __user *)arg, sizeof(di_params))) {
			VDO_LOG(VDO_LOG_LVL_ERR, "[DI]: copy_from_user failed! line:%d\n", __LINE__);
			return -EFAULT;
		}
		//to do: set di param to hal
		g_src_width = param.src_width;
		g_src_height = param.src_height;
		singleChannelFlag = (param.flag & DI_SINGLE_CHANNEL) >> 1;
		if (param.enable_hw) {
			ret = request_irq(fmtf_irq, dispfmt_vsync_isr, IRQF_TRIGGER_LOW, "dispfmt_isr", NULL);
			if (ret < 0) {
				VDO_LOG(VDO_LOG_LVL_ERR, "register irq fali ret=%d\n", ret);
				return ret;
			}
		} else {
			free_irq(fmtf_irq, NULL);
		}

		if (di_dev->update_di_wq && !param.enable_hw) {
			VDO_LOG(VDO_LOG_LVL_DBG,"flush workqueue \n");
			flush_workqueue(di_dev->update_di_wq);
			dev_ion_free();
			g_src_width = 0;
			g_src_height = 0;
		}


		ret = di_hal_set_params(AVM, &param);

		return ret;
	}

	case DI_IOCTL_CFG_BUFS: {
		//1. store di bufs
		di_cfg_bufs di_bufs;
		//to create fence for input and output
		struct fence_data data_in = {0}, data_out = {0};
		VDO_LOG(VDO_LOG_LVL_DBG, "DI_IOCTL_CFG_BUFS\n");
		if (copy_from_user(&di_bufs, (void __user *) arg, sizeof(di_cfg_bufs))) {
			VDO_LOG(VDO_LOG_LVL_ERR, "di_buf_config copy_from_user failed!\n");
			return -EFAULT;
		}
		for (i = 0; i < VIDEO_CHANNEL_NUM; i++) {
			VDO_LOG(VDO_LOG_LVL_DBG, " input fd[%d] is %d\n", i, di_bufs.input.bufs[i]);
			VDO_LOG(VDO_LOG_LVL_DBG, " output fd[%d] is %d\n", i, di_bufs.output.bufs[i]);
		}

		if (!di_is_cur_dev(AVM)) {
			VDO_LOG(VDO_LOG_LVL_ERR, "di hw not enable, can't work\n");
			return -EPERM;
		}
		data_in.fence = -1;
		data_in.value = ++(di_dev->timeline_max_in);
		ret = fence_create(di_dev->timeline_in, &data_in);
		if (ret != 0) {
			VDO_LOG(VDO_LOG_LVL_ERR, "create input Fence Object failed ret=%d!\n", ret);
		}
		di_bufs.input.release_fence = data_in.fence;


		data_out.fence = -1;
		data_out.value = ++(di_dev->timeline_max_out);
		ret = fence_create(di_dev->timeline_out, &data_out);
		if (ret != 0) {
			VDO_LOG(VDO_LOG_LVL_ERR, "create output Fence Object failed ret=%d!\n", ret);
		}
		di_bufs.output.release_fence = data_out.fence;

		VDO_LOG(VDO_LOG_LVL_DBG, "input pt=%d, out pt =%d!\n", data_in.value, data_out.value);

		memcpy(&di_dev->buf_list[di_dev->listIndex++], &di_bufs, sizeof(di_cfg_bufs));
		/* two buffer can show for avm*/
		if (di_dev->listIndex == BUFFER_NUM - 1) {//
			//do di operation
	                struct di_work_struct *di_work;
			di_work = kmalloc(sizeof(*di_work), GFP_KERNEL);
			if (!di_work) {
				VDO_LOG(VDO_LOG_LVL_ERR, "no memory to alloc \n");
				return -ENOMEM;
			}
			INIT_WORK(&(di_work->work_store_buf), di_work_func);
			if (singleChannelFlag) {
				di_ion_to_phyaddr(di_dev->buf_list[0].output.bufs[0], &phy_info);//get buf list 1st out
				di_work->work_buf_out[0][0] = (int)phy_info.addr;
				for (j = 0; j < BUFFER_NUM - 1; j++) {
						di_ion_to_phyaddr(di_dev->buf_list[j].input.bufs[0], &phy_info);//get buf list 2 input
						di_work->work_buf_inY[0][j] = phy_info.addr;
						di_work->work_buf_inC[0][j] = phy_info.addr + g_src_width * g_src_height;
					}
					di_work->top_filed_fst[0] = di_dev->buf_list[0].top_filed_fst[0];

			} else {
				/*To Add Note*/
				for (i = 0; i < VIDEO_CHANNEL_NUM; i++) {
#ifdef DI_TZG_OUTPUT_EN
					di_ion_to_phyaddr(di_dev->buf_list[0].output.bufs[0], &phy_info);//get buf list 1st out
					di_work->work_buf_out[0][0] = (int)phy_info.addr;
#else
					di_ion_to_phyaddr(di_dev->buf_list[0].output.bufs[i], &phy_info);//get buf list 1st out
					di_work->work_buf_out[i][0] = (int)phy_info.addr;
#endif
				}
				for (i = 0; i < VIDEO_CHANNEL_NUM; i++) {
					for (j = 0; j < BUFFER_NUM - 1; j++) {
						di_ion_to_phyaddr(di_dev->buf_list[j].input.bufs[i], &phy_info);//get buf list 2 input
						di_work->work_buf_inY[i][j] = phy_info.addr;
						di_work->work_buf_inC[i][j] = phy_info.addr + g_src_width * g_src_height;
					}
					di_work->top_filed_fst[i] = di_dev->buf_list[0].top_filed_fst[i];
				}
			}
			di_work->type = AVM;
			di_work->timeline_in = di_dev->timeline_in;
			di_work->timeline_out = di_dev->timeline_out;

			queue_work(di_dev->update_di_wq, &di_work->work_store_buf);

			memcpy(&di_dev->buf_list[0], &di_dev->buf_list[1], sizeof(di_cfg_bufs));
			di_dev->listIndex--;
		}

		//return release fence to user
		if (copy_to_user((void __user *)arg, &di_bufs, sizeof(di_cfg_bufs))) {
			VDO_LOG(VDO_LOG_LVL_ERR, "di_config copy_to_user failed!\n");
			return -EFAULT;
		}

		return 0;
	}
	default:
		VDO_LOG(VDO_LOG_LVL_ERR, "Not support cmcmd=0x%08x \r\n", (unsigned int)cmd);
		return -1;

	}

	return 0;
}
static int di_open(struct inode *inode, struct file *file)
{
	VDO_LOG(VDO_LOG_LVL_INFO, "%s %d \n", __func__, __LINE__);
	file->private_data = g_drvInst;

	return 0;
}
static int di_release(struct inode *inode, struct file *file)
{
	struct di_device *di_dev = (struct di_device *)file->private_data;
	di_params param;

	VDO_LOG(VDO_LOG_LVL_INFO, "%s %d \n", __func__, __LINE__);
	if (g_src_width != 0 || g_src_height != 0) {
		if (di_dev && di_dev->update_di_wq) {
			VDO_LOG(VDO_LOG_LVL_DBG, "flush workqueue \n");
			flush_workqueue(di_dev->update_di_wq);
			dev_ion_free();
		}
		free_irq(fmtf_irq, NULL);
		memset(&param, 0, sizeof(di_params));
		param.enable_hw = 0;
		di_hal_set_params(AVM, &param);
		g_src_width = 0;
		g_src_height = 0;
		VDO_LOG(VDO_LOG_LVL_INFO, "clean params success\n");
	}
	if (di_dev) {
		file->private_data = NULL;
	}
	return 0;
}

struct file_operations const di_fops = {
	.unlocked_ioctl = di_ioctl,
	.open = di_open,
	.release = di_release,
};

static struct miscdevice di_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "di",
	.mode = 0664,
	.fops = &di_fops
};


#ifndef ATC_ARM2_LK_SUPPORT
extern void __iomem *IO_BASE_BRINGUP;
#else
extern unsigned long IO_BASE_BRINGUP;
#endif

#ifndef NO_IRQ
#define NO_IRQ 0
#endif

static int di_probe(struct platform_device *pdev)
{
	unsigned int ret = -EINVAL;
	struct device *dev = &pdev->dev;
	struct device_node *nd = pdev->dev.of_node;
	struct di_device *pri_dev = NULL;

	VDO_LOG(VDO_LOG_LVL_INFO, "di_probe start \r\n");

	fmtf_irq = irq_of_parse_and_map(nd, 0);
	if (fmtf_irq == NO_IRQ) {
		VDO_LOG(VDO_LOG_LVL_ERR, "get irq failed = %d \r\n", fmtf_irq);
		return -1;
	}

	VDO_LOG(VDO_LOG_LVL_INFO, "fmtf_irq = %d \r\n", fmtf_irq);

	vdof_reg = of_iomap(nd, 0);//12002400
	if (!vdof_reg) {
		VDO_LOG(VDO_LOG_LVL_ERR, "get io base address vdof_reg = %p \r\n", vdof_reg);
		return -1;
	}
	VDO_LOG(VDO_LOG_LVL_INFO, "get io base address vdof_reg = %p pa=%p\r\n", vdof_reg, (void *)__pa(vdof_reg));

	fmtf_reg = of_iomap(nd, 1);//12001000
	if (!fmtf_reg) {
		VDO_LOG(VDO_LOG_LVL_ERR, "get io base address fmtf_reg = %p \r\n", fmtf_reg);
		return -1;
	}
	VDO_LOG(VDO_LOG_LVL_INFO, "get io base address fmtf_reg111 = %p pa=%p\r\n", fmtf_reg, (void *)__pa(fmtf_reg));

	IO_BASE_BRINGUP = of_iomap(nd, 2);//12000000
	if (!IO_BASE_BRINGUP) {
		VDO_LOG(VDO_LOG_LVL_ERR, "get io base address IO_BASE_BRINGUP = %p \r\n", IO_BASE_BRINGUP);
		return -1;
	}
	VDO_LOG(VDO_LOG_LVL_INFO, "get io base address IO_BASE_BRINGUP = %p pa=%p \r\n", IO_BASE_BRINGUP, (void *)__pa(IO_BASE_BRINGUP));

	vdo_8tap_reg = of_iomap(nd, 4);//12002100
	if (!vdo_8tap_reg) {
		VDO_LOG(VDO_LOG_LVL_ERR, "get io base address vdo_8tap_reg = %p \r\n", vdo_8tap_reg);
		return -1;
	}
	VDO_LOG(VDO_LOG_LVL_INFO, "get io base address vdo_8tap_reg = %p pa=%p \r\n", vdo_8tap_reg, (void *)__pa(vdo_8tap_reg));

	di_clk = devm_clk_get(dev, "di_clk_sel");
	if (IS_ERR(di_clk)) {
		VDO_LOG(VDO_LOG_LVL_ERR, "fail to get di clk\r\n");
		return PTR_ERR(di_clk);
	}
	/*syspll1_d2 univpll1_d4 univpll1_d8*/
	di_parent_clk = devm_clk_get(dev, "syspll1_d2");
	if (IS_ERR(di_parent_clk)) {
		VDO_LOG(VDO_LOG_LVL_ERR, "fail to get di parent clk univpll1_d4\r\n");
		return PTR_ERR(di_parent_clk);
	}

	ret = misc_register(&di_dev);

	bringupinit3365();
	VDO_LOG(VDO_LOG_LVL_INFO, "bringupinit3365 end \r\n");

	/*init di_param */
	di_init_params();
	ret = device_create_file(di_dev.this_device, &dev_attr_di4file);
	if (ret)
		VDO_LOG(VDO_LOG_LVL_ERR, "%s %d cannot create dev file %d\r\n", __func__, __LINE__, ret);

	//add for avm di
	pri_dev = kmalloc(sizeof(struct di_device), GFP_KERNEL);
	if (!pri_dev) {
		VDO_LOG(VDO_LOG_LVL_ERR, "no memory to alloc \n");
		return -ENOMEM;
	}


	//add for input & output timeline
	pri_dev->timeline_in = timeline_create("di_fence_input");
	if (!pri_dev->timeline_in) {
		VDO_LOG(VDO_LOG_LVL_ERR, "no memory to alloc \n");
		return -ENOMEM;
	}

	pri_dev->timeline_max_in = 0;

	pri_dev->timeline_out = timeline_create("di_fence_out");
	if (!pri_dev->timeline_out) {
		VDO_LOG(VDO_LOG_LVL_ERR, "no memory to alloc \n");
		return -ENOMEM;
	}

	pri_dev->timeline_max_out = 0;

	//add for create workqueue
	pri_dev->update_di_wq = create_singlethread_workqueue("di_workqueue");
	if (!pri_dev->update_di_wq) {
		VDO_LOG(VDO_LOG_LVL_ERR, "create workqueue fail\n");
	}
	pri_dev->listIndex = 0;
	g_drvInst = pri_dev;
	cvbs_wq = pri_dev->update_di_wq;

//end for add avm di
#if DI_DUMP_BUFFER_ATTR
	ret = device_create_file(di_dev.this_device, &dev_attr_di4dump);
	if (ret)
		VDO_LOG(VDO_LOG_LVL_ERR, "%s %d cannot create dev file %d\r\n", __func__, __LINE__, ret);
#endif

	return ret;


}

static int di_remove(struct platform_device *pdev)
{
	struct di_device *pri_dev = (struct di_device *)g_drvInst;
	ENTER();
	if (pri_dev) {
		if (pri_dev->update_di_wq) {
			flush_workqueue(pri_dev->update_di_wq);
			destroy_workqueue(pri_dev->update_di_wq);
			pri_dev->update_di_wq = NULL;
		}
		if (pri_dev->timeline_out) {
			timeline_destroy(pri_dev->timeline_out);
		}
		if (pri_dev->timeline_in) {
			timeline_destroy(pri_dev->timeline_in);
		}
		kfree(pri_dev);
		pri_dev = NULL;
	}

	g_drvInst = NULL;
#if DI_DUMP_BUFFER_ATTR
	device_remove_file(di_dev.this_device, &dev_attr_di4dump);
#endif
	LEAVE();
	return 0;
}

static const struct of_device_id di_of_ids[] = {
	{.compatible = "Autochips,de-interlance",},
	{}
};

static int di_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	clk_disable_unprepare(di_clk);
	VDO_LOG(VDO_LOG_LVL_INFO, "di_suspend after\n");
	return 0;
}

static int di_resume(struct platform_device *pdev)
{
	VDO_LOG(VDO_LOG_LVL_INFO, "di_resume done\n");
	WriteREG(VDPSYS_CG_CON0, 0x0);
	clk_prepare_enable(di_clk);
	return 0;
}

static struct platform_driver di_plt_drv = {
	.driver = {
		   .name = "Autochips-deinterlance",
		   .owner = THIS_MODULE,
		   .of_match_table = di_of_ids,
		   },
	.probe = di_probe,
	.remove = di_remove,
	.suspend = di_suspend,
	.resume = di_resume,
};

static int __init di_init(void)
{
	int ret;
	VDO_LOG(VDO_LOG_LVL_INFO, "di_init \n");
	di_thread_init();
	ret = platform_driver_register(&di_plt_drv);
	return ret;
}

static void __exit di_exit(void)
{
	VDO_LOG(VDO_LOG_LVL_INFO, "di_exit \n");
	di_thread_deinit();
	platform_driver_unregister(&di_plt_drv);

}
module_init(di_init);
module_exit(di_exit);

MODULE_DESCRIPTION(ATC_DI_DRIVER_INFO);
MODULE_LICENSE(ATC_KERNEL_LINUX_LICENSE);


