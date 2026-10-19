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



#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-ioctl.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <media/videobuf-dma-contig.h>
#include <media/videobuf2-vmalloc.h>
#include <mt-plat/aee.h>
#include <linux/of.h>
#include <linux/proc_fs.h>	/* proc file use */
#include <linux/namei.h>
#include <linux/fs.h>
#include "di_hal.h"
#include "tvd_if.h"
#include "atc_capture.h"
#include "avin_hal.h"
#include "avin_log.h"
#include "avin_common.h"


#define AVIN_VERSION			"01.01.01"

#ifdef VSS_LOG_TAG
#undef VSS_LOG_TAG
#define VSS_LOG_TAG "[vss_avin]"
#endif
const char *vss_log_str[] = {"err", "warning", "info", "debug"};
int log_debug = VSS_LOG_INFO;
int vss_aee_debug = 0;
module_param(log_debug, int, 0644);
module_param(vss_aee_debug, int, 0644);

struct atc_avin_device *g_avin_dev;
struct mutex avin_lock;
bool avin_state = false;
uintptr_t di_add_arr[5];

static struct device *cap_dev;
static int g_io_fd[DISPLAY_BUFFER_NUM];
static int g_io_fd_num = 0;

typedef enum {
	CORE_TYPE_ARM1_CVBS,
	CORE_TYPE_ARM2_CVBS,
	CORE_TYPE_ARM2_AHD,
	CORE_TYPE_MAX
} CORE_TYPE_ENUM;

typedef enum {
	SOURCE_TYPE_VDI,
#ifdef NR_MODULE_EXIST
	SOURCE_TYPE_NR,
#endif
	SOURCE_TYPE_ARM2_DI,
	SOURCE_TYPE_ARM1_DI,
	SOURCE_TYPE_CVBS_MDP,
	SOURCE_TYPE_ISP,
	SOURCE_TYPE_AHD_MDP,
	SOURCE_TYPE_MAX
} SOURCE_TYPE_ENUM;

typedef struct {
	u32 base_address;
	u32 size;
	void *virt_addr;
} RESERVE_MEM_INFO;

static int avin_mkdir(const char *name, umode_t mode)
{
	struct dentry *dentry;
	struct path path;
	int err;

	vss_printk(VSS_LOG_INFO, "enter\n");

	dentry = kern_path_create(AT_FDCWD, name, &path, LOOKUP_DIRECTORY);
	if (IS_ERR(dentry)) {
		vss_printk(VSS_LOG_ERR, "get dentry failed and error number is %ld and name is %s\n", PTR_ERR(dentry), name);
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

/**
 * avin_buffer_queue_setup : Callback function for buffer setup.
 * @vq: vb2_queue ptr
 * @format: v4l2 format
 * @nbuffers: ptr to number of buffers requested by application
 * @nplanes:: contains number of distinct video planes needed to hold a frame
 * @sizes[]: contains the size (in bytes) of each plane.
 * @alloc_ctxs: ptr to allocation context
 *
 * This callback function is called when reqbuf() is called to adjust
 * the buffer count and buffer size
 */
static int avin_buffer_queue_setup(struct vb2_queue *vq,
			unsigned int *nbuffers, unsigned int *nplanes,
			unsigned int sizes[], struct device *alloc_devs[])
{
	vss_printk(VSS_LOG_INFO, "enter\n");
	*nplanes = 1;
	sizes[0] = sizeof(struct capture_priv);
	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
}


/**
 * avin_buffer_queue : Callback function to add buffer to DMA queue
 * @vb: ptr to vb2_buffer
 */
static void avin_buffer_queue(struct vb2_buffer *vb)
{
	struct atc_avin_device *avin_dev = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer* v4l2_buf = to_vb2_v4l2_buffer(vb);
	struct avin_cap_buffer *buf = container_of(v4l2_buf, struct avin_cap_buffer, buf);
	struct capture_priv *cap_info;

	spin_lock(&avin_dev->slock);
	cap_info = (struct capture_priv *)vb2_plane_vaddr(vb, 0U);
	list_add_tail(&buf->list, &avin_dev->buf_queue);
	spin_unlock(&avin_dev->slock);

	if (!IS_ERR_OR_NULL(cap_info) && (cap_info->buf_index != (unsigned int)-1)) {
		/*set the buffer of specified index to free status*/
		di_set_output_buf_status(cap_info->buf_index, AVIN_BUF_FREE);
	}
}

/*
 * vb2 uses these to release the mutex when waiting in dqbuf.  I'm
 * not actually sure we need to do this (I'm not sure that vb2_dqbuf() needs
 * to be called with the mutex held), but better safe than sorry. IKnow heihei
 */
static void avin_wait_prepare(struct vb2_queue *vq)
{
	struct atc_avin_device *avin_dev = vb2_get_drv_priv(vq);

	mutex_unlock(&avin_dev->lock);
}

static void avin_wait_finish(struct vb2_queue *vq)
{
	struct atc_avin_device *avin_dev = vb2_get_drv_priv(vq);

	mutex_lock(&avin_dev->lock);
}


/**
 * avin_start_streaming : Starts the DMA engine for streaming
 * @vb: ptr to vb2_buffer
 * @count: number of buffers
 */
static int avin_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	int ret = 0;

	vss_printk(VSS_LOG_INFO, "enter and version is %s\n", AVIN_VERSION);

	mutex_lock(&avin_lock);
	avin_state = true;
	avin_tvd_init();
	avin_tvd_start();
	mutex_unlock(&avin_lock);

	vss_printk(VSS_LOG_INFO, "leave\n");
	return ret;
}

/**
 * avin_stop_streaming : Stop the DMA engine
 * @vq: ptr to vb2_queue
 *
 * This callback stops the DMA engine and any remaining buffers
 * in the DMA queue are released.
 */
static void avin_stop_streaming(struct vb2_queue *vq)
{
	struct atc_avin_device *avin_dev = vb2_get_drv_priv(vq);
	vss_printk(VSS_LOG_INFO, "enter\n");

	mutex_lock(&avin_lock);
	avin_state = false;
	avin_di_deinit();
	#ifdef NR_MODULE_EXIST
	avin_nr_deinit();
	#endif
	avin_tvd_stop();
	avin_tvd_deinit();
	mutex_unlock(&avin_lock);

	/* release all active buffers */
	spin_lock(&avin_dev->slock);
	while (!list_empty(&avin_dev->buf_queue)) {
		avin_dev->next_frm = list_entry(avin_dev->buf_queue.next, struct avin_cap_buffer, list);
		list_del(&avin_dev->next_frm->list);
		vb2_buffer_done(&avin_dev->next_frm->buf.vb2_buf, VB2_BUF_STATE_ERROR);
	}
	spin_unlock(&avin_dev->slock);
	vss_printk(VSS_LOG_INFO, "leave\n");
}


static struct vb2_ops video_qops = {
	.queue_setup = avin_buffer_queue_setup,
	.start_streaming = avin_start_streaming,
	.stop_streaming = avin_stop_streaming,
	.buf_queue = avin_buffer_queue,
	.wait_prepare = avin_wait_prepare,
	.wait_finish = avin_wait_finish,
};


/**
 * avin_g_fmt_vid_cap() - Set INPUT handler
 * @file: file ptr
 * @priv: file handle
 * @fmt: ptr to v4l2 format structure
 */
static int avin_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *format)
{
	return 0;
}

/**
 * avin_querycap() - QUERYCAP handler
 * @file: file ptr
 * @priv: file handle
 * @cap: ptr to v4l2_capability structure
 */
static int avin_querycap(struct file *file, void *priv, struct v4l2_capability *cap)
{
	strcpy(cap->driver, AVIN_DRIVER_NAME);
	strcpy(cap->card, AVIN_DRIVER_NAME);
	cap->version = KERNEL_VERSION(1, 0, 0);
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s", dev_name(cap_dev));
	return 0;
}


static int avin_s_ctrl(struct file *file, void *priv, struct v4l2_control *ctrl)
{
	int ret = 0;
	vss_printk(VSS_LOG_INFO, "enter and id(%d) value(%d)\n", ctrl->id, ctrl->value);
	switch (ctrl->id) {
		case V4L2_CID_SET_FD:
			g_io_fd[g_io_fd_num] = ctrl->value & 0xFFFFFF;
			vss_printk(VSS_LOG_INFO, "fd is %d, index is %d[%d], status is %s\n",
				g_io_fd[g_io_fd_num], (ctrl->value >> 24 & 0x1F),g_io_fd_num, (ctrl->value >> 29) == 0?"free":"busy");
			if (g_io_fd_num == (ctrl->value >> 24 & 0x1F)) {
				di_set_output_buf_status(g_io_fd_num, (ctrl->value >> 29));
			} else {
				vss_printk(VSS_LOG_ERR,"the buffer index is not matched with fd index");
			}
			g_io_fd_num++;
			if (DISPLAY_BUFFER_NUM == g_io_fd_num){
				mutex_lock(&avin_lock);
				if (0 != avin_di_set_fd(g_io_fd, g_io_fd_num)) {
					vss_printk(VSS_LOG_ERR, "this may cause di fail\n");
				}
				mutex_unlock(&avin_lock);
			}
			g_io_fd_num = (g_io_fd_num % DISPLAY_BUFFER_NUM);
			break;
		case V4L2_CID_PROCESS_COLOR:
			ret = tvd_hw_color_process((ctrl->value >> 16), (ctrl->value & 0xFFFF));
			break;
		default:
			ret = -EINVAL;
			break;
	}
	vss_printk(VSS_LOG_INFO, "leave\n");
	return ret;
}

static int avin_open(struct file *file)
{
	vss_printk(VSS_LOG_INFO, "enter\n");
	vss_printk(VSS_LOG_INFO, "leave\n");
	return v4l2_fh_open(file);
}
static int avin_release(struct file *file)
{
	vss_printk(VSS_LOG_INFO, "enter\n");
	g_io_fd_num = 0;
	vss_printk(VSS_LOG_INFO, "leave\n");
	return vb2_fop_release(file);
}


/* avin capture ioctl operations */
static const struct v4l2_ioctl_ops avin_ioctl_ops = {
	.vidioc_querycap			= &avin_querycap,
	.vidioc_g_fmt_vid_cap		= &avin_g_fmt_vid_cap,

	.vidioc_reqbufs				= vb2_ioctl_reqbufs,
	.vidioc_create_bufs			= vb2_ioctl_create_bufs,
	.vidioc_querybuf			= vb2_ioctl_querybuf,
	.vidioc_qbuf				= vb2_ioctl_qbuf,
	.vidioc_dqbuf				= vb2_ioctl_dqbuf,
	.vidioc_expbuf				= vb2_ioctl_expbuf,
	.vidioc_streamon			= vb2_ioctl_streamon,
	.vidioc_streamoff			= vb2_ioctl_streamoff,

	.vidioc_s_ctrl				= &avin_s_ctrl,
};

/* avin file operations */
static struct v4l2_file_operations avin_fops = {
	.owner = THIS_MODULE,
	.open = avin_open,
	.release = avin_release,
	.unlocked_ioctl = video_ioctl2,
	.mmap = vb2_fop_mmap,
	.poll = vb2_fop_poll
};

static void avin_dev_release(struct v4l2_device *v4l2_dev)
{
	struct atc_avin_device *atc_dev = container_of(v4l2_dev, struct atc_avin_device, v4l2_dev);
	v4l2_device_unregister(&atc_dev->v4l2_dev);
	kfree(atc_dev);
}

static void *remap_lowmem(phys_addr_t start, phys_addr_t size)
{
	struct page **pages;
	phys_addr_t page_start;
	unsigned int page_count;
	pgprot_t prot;
	unsigned int i;
	void *vaddr;

	page_start = start - offset_in_page(start);
	page_count = DIV_ROUND_UP(size + offset_in_page(start), PAGE_SIZE);

	prot = pgprot_noncached(PAGE_KERNEL);

	pages = kmalloc_array(page_count, sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return NULL;

	for (i = 0; i < page_count; i++) {
		phys_addr_t addr = page_start + i * PAGE_SIZE;

		pages[i] = pfn_to_page(addr >> PAGE_SHIFT);
	}
	vaddr = vmap(pages, page_count, VM_MAP, prot);
	kfree(pages);
	if (!vaddr) {
		vss_printk(VSS_LOG_ERR, "%s: Failed to map %u pages\n", __func__, page_count);
		return NULL;
	}

	return vaddr + offset_in_page(start);
}

static int get_reserve_mem_info(char *node_name, RESERVE_MEM_INFO *mem_info)
{
	int ret = -1;
	u32 regs[4] = {0};
	struct device_node *node = NULL;

	vss_printk(VSS_LOG_INFO, "enter\n");
	node = of_find_compatible_node(NULL, NULL, node_name);
	if (IS_ERR_OR_NULL(node)) {
		vss_printk(VSS_LOG_ERR, "get %s device node failed\n", node_name);
		return ret;
	}

	ret = of_property_read_u32_array(node, "reg", regs, 4);
	if (0 != ret) {
		vss_printk(VSS_LOG_ERR, "get %s reserve memory information failed\n", node_name);
		return ret;
	}

	if (!IS_ERR_OR_NULL(mem_info)) {
		mem_info->base_address = regs[1];
		mem_info->size         = regs[3];
		mem_info->virt_addr    = remap_lowmem((phys_addr_t)regs[1], regs[3]);
		if (NULL == mem_info->virt_addr) {
			vss_printk(VSS_LOG_ERR, "ioremap the address(start:0x%08x, size = 0x%x) failed\n", regs[1], regs[3]);
			return -1;
		}
		vss_printk(VSS_LOG_INFO, "the base_address = 0x%08x, size = 0x%x of %s\n", regs[1], regs[3], node_name);
	}
	vss_printk(VSS_LOG_INFO, "leave\n");
	return 0;
}

void dump_module_buffer(char *module_name, void *virt_addr, u32 source_width, u32 source_hight, u32 offset)
{
	u8 y_file_name[256] = {'\0'};
	u8 c_file_name[256] = {'\0'};
	u32 y_size = source_width * source_hight;
	u32 c_size = y_size / 2;
	mm_segment_t old_fs;
	struct file *fp = NULL;
	loff_t pos;
	ssize_t ret;
	void *start_addr = virt_addr;
	u32 index;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	for (index = 0; index < 5; index++) {
		sprintf(y_file_name, "%s/y%03d.raw", module_name, index + 1);
		fp = filp_open(y_file_name, O_CREAT | O_RDWR | O_TRUNC, 0777);
		if (IS_ERR(fp)) {
			vss_printk(VSS_LOG_ERR, "create %s file failed\n", y_file_name);
			return;
		}

		pos = 0;
		ret = vfs_write(fp, (char __user *)start_addr, y_size, &pos);
		if (ret != y_size) {
			vss_printk(VSS_LOG_ERR, "write frame_count %d file failed\n", index + 1);
			filp_close(fp, NULL);
			return;
		}
		filp_close(fp, NULL);

		start_addr += offset;
		sprintf(c_file_name, "%s/c%03d.raw", module_name, index + 1);
		fp = filp_open(c_file_name, O_CREAT | O_RDWR | O_TRUNC, 0777);
		if (IS_ERR(fp)) {
			vss_printk(VSS_LOG_ERR, "create %s file failed\n", c_file_name);
			return;
		}

		pos = 0;
		ret = vfs_write(fp, (char __user *)start_addr, c_size, &pos);
		if (ret != c_size) {
			vss_printk(VSS_LOG_ERR, "write frame_count %d file failed\n", index + 1);
			filp_close(fp, NULL);
			return;
		}
		filp_close(fp, NULL);
		start_addr += offset/2;
	}
	set_fs(old_fs);
	return;
}

void dump_yuyv_buffer(char *module_name, void *virt_addr, u32 source_width, u32 source_hight, u32 offset)
{
	u8 file_name[256] = {'\0'};
	u32 size = source_width * source_hight*2;
	mm_segment_t old_fs;
	struct file *fp = NULL;
	loff_t pos;
	ssize_t ret;
	void *start_addr = virt_addr;
	u32 index;
	u32 dump_num = 0;

	vss_printk(VSS_LOG_INFO, "enter and source width = %d, source hight = %d", source_width, source_hight);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (strstr(module_name, "mdp")) {
		dump_num = 3;
	} else if (strstr(module_name, "isp")) {
		dump_num = 4;
	} else {
		dump_num = 5;
	}
	for (index = 0; index < dump_num; index++) {
		sprintf(file_name, "%s/yuyv%03d.raw", module_name, index + 1);
		fp = filp_open(file_name, O_CREAT | O_RDWR | O_TRUNC, 0777);
		if (IS_ERR(fp)) {
			vss_printk(VSS_LOG_ERR, "create %s file failed\n", file_name);
			return;
		}

		pos = 0;
		ret = vfs_write(fp, (char __user *)start_addr, size, &pos);
		if (ret != size) {
			vss_printk(VSS_LOG_ERR, "write frame_count %d file failed and (size, ret) is (%d, %d)\n", index + 1, size, ret);
			filp_close(fp, NULL);
			return;
		}
		filp_close(fp, NULL);
		start_addr += offset;
	}
	set_fs(old_fs);
	vss_printk(VSS_LOG_INFO, "leave");
	return;
}


static int  backcar_dump_show(struct seq_file *m, void *v)
{
	seq_printf(m, "backcar_dump_show\n");
	return 0;
}


static ssize_t  backcar_data_dump(struct file *file, const char *buffer, size_t count, loff_t *data)
{
	char buf[100]    = {'\0'};
	char core[10]    = {'\0'};
	CORE_TYPE_ENUM dump_type = CORE_TYPE_MAX;
	u32 buf_size = (count < (sizeof(buf) - 1)) ? (count) : (sizeof(buf) - 1);
	u32 src_width, src_height, dst_width, dst_height;
	static RESERVE_MEM_INFO mem_array[SOURCE_TYPE_MAX];
	u32 offset;
	ssize_t ret = -1;

	vss_printk(VSS_LOG_INFO, "enter\n");

	if (copy_from_user(buf, buffer, buf_size)) {
		vss_printk(VSS_LOG_ERR, "copy from user failed\n");
		return -EFAULT;
	}

	if (5 == sscanf(buf, "%9s %u %u %u %u", core, &src_width, &src_height, &dst_width, &dst_height)) {
		if (0 == strcmp(core, "arm1_cvbs")) {
			dump_type = CORE_TYPE_ARM1_CVBS;
		} else if (0 == strcmp(core, "arm2_cvbs")) {
			dump_type = CORE_TYPE_ARM2_CVBS;
		} else if (0 == strcmp(core, "arm2_ahd")) {
			dump_type = CORE_TYPE_ARM2_AHD;
		} else {
			vss_printk(VSS_LOG_ERR, "the input parameter is not suitable\n");
			return -EFAULT;
		}
	} else {
		vss_printk(VSS_LOG_ERR, "the input parameter number is not suitable\n");
		return -EFAULT;
	}

	/*create directory*/
	avin_mkdir("/data/backcar", 0755);
	if (CORE_TYPE_ARM2_AHD == dump_type) {
		avin_mkdir("/data/backcar/ahd", 0755);
		avin_mkdir("/data/backcar/ahd/isp", 0755);
		avin_mkdir("/data/backcar/ahd/mdp", 0755);
	} else {
		avin_mkdir("/data/backcar/cvbs", 0755);
		avin_mkdir("/data/backcar/cvbs/tvd", 0755);
#ifdef NR_MODULE_EXIST
		avin_mkdir("/data/backcar/cvbs/nr", 0755);
#endif
		avin_mkdir("/data/backcar/cvbs/di", 0755);
		if (CORE_TYPE_ARM1_CVBS != dump_type) {
			avin_mkdir("/data/backcar/cvbs/mdp", 0755);
		}
	}

	if (CORE_TYPE_ARM2_AHD == dump_type) {
		/*dump arm2 isp memory*/
		if (0 == mem_array[SOURCE_TYPE_ISP].virt_addr) {
			ret = get_reserve_mem_info("autochips,isp", &mem_array[SOURCE_TYPE_ISP]);
			if (ret) {
				vss_printk(VSS_LOG_ERR, "get isp reserved memory info failed\n");
				return ret;
			}
		}
		dump_yuyv_buffer("/data/backcar/ahd/isp", mem_array[SOURCE_TYPE_ISP].virt_addr, src_width, src_height, ALIGN(src_width*src_height*2, PAGE_SIZE));

		/*dump arm2 ahd mdp out memory*/
		if (0 == mem_array[SOURCE_TYPE_AHD_MDP].virt_addr) {
			ret = get_reserve_mem_info("autochips,mdp", &mem_array[SOURCE_TYPE_AHD_MDP]);
			if (ret) {
				vss_printk(VSS_LOG_ERR, "get mdp reserved memory info failed\n");
				return ret;
			}
		}
		dump_yuyv_buffer("/data/backcar/ahd/mdp", mem_array[SOURCE_TYPE_AHD_MDP].virt_addr, dst_width, dst_height, dst_width*dst_height*2);
	} else {
		/*dump wch vdi memory*/
		if (0 == mem_array[SOURCE_TYPE_VDI].virt_addr) {
			ret = get_reserve_mem_info("autochips,wch-vdi", &mem_array[SOURCE_TYPE_VDI]);
			if (ret) {
				vss_printk(VSS_LOG_ERR, "get wch vdi reserved memory info failed\n");
				return ret;
			}
		}
		dump_module_buffer("/data/backcar/cvbs/tvd", mem_array[SOURCE_TYPE_VDI].virt_addr, src_width, src_height, 720*576);

	#ifdef NR_MODULE_EXIST
		/*dump nr memory*/
		if (0 == mem_array[SOURCE_TYPE_NR].virt_addr) {
			ret = get_reserve_mem_info("autochips,nr", &mem_array[SOURCE_TYPE_NR]);
			if (ret) {
				vss_printk(VSS_LOG_ERR, "get nr reserved memory info failed\n");
				return ret;
			}
		}
		dump_module_buffer("/data/backcar/cvbs/nr", mem_array[SOURCE_TYPE_NR].virt_addr, src_width, src_height, 720*576);
	#endif

		switch (dump_type) {
			case CORE_TYPE_ARM1_CVBS:
				/*dump wch di which address is allocated by di heap*/
				if ((0 == mem_array[SOURCE_TYPE_ARM1_DI].virt_addr) && ((0 != di_add_arr[0]) && (0 != di_add_arr[1]) && (di_add_arr[1] > di_add_arr[0]))) {
					offset = di_add_arr[1] - di_add_arr[0];
					mem_array[SOURCE_TYPE_ARM1_DI].virt_addr = remap_lowmem(di_add_arr[0], offset * 5);
				}
				offset = di_add_arr[1] - di_add_arr[0];
				dump_yuyv_buffer("/data/backcar/cvbs/di", mem_array[SOURCE_TYPE_ARM1_DI].virt_addr, src_width, src_height, offset);
				break;
			case CORE_TYPE_ARM2_CVBS:
				/*dump wch di which address is allocated by wch di*/
				if (0 == mem_array[SOURCE_TYPE_ARM2_DI].virt_addr) {
					ret = get_reserve_mem_info("autochips,wch-di", &mem_array[SOURCE_TYPE_ARM2_DI]);
					if (ret) {
						vss_printk(VSS_LOG_ERR, "get di reserved memory info failed\n");
						return ret;
					}
				}
				dump_yuyv_buffer("/data/backcar/cvbs/di", mem_array[SOURCE_TYPE_ARM2_DI].virt_addr, src_width, src_height, 720*576*2);
				break;
			default:
				break;
		}

		/*dump arm2 cvbs mdp out memory*/
		if (CORE_TYPE_ARM2_CVBS == dump_type) {
			if (0 == mem_array[SOURCE_TYPE_CVBS_MDP].virt_addr) {
				ret = get_reserve_mem_info("autochips,mdp", &mem_array[SOURCE_TYPE_CVBS_MDP]);
				if (ret) {
					vss_printk(VSS_LOG_ERR, "get mdp reserved memory info failed\n");
					return ret;
				}
			}
			dump_yuyv_buffer("/data/backcar/cvbs/mdp", mem_array[SOURCE_TYPE_CVBS_MDP].virt_addr, dst_width, dst_height, dst_width*dst_height*2);
		}
	}
	vss_printk(VSS_LOG_INFO, "leave\n");
	return count;
}

static int backcar_open(struct inode *inode, struct file *file)
{
	return single_open(file, backcar_dump_show, NULL);
}

static  struct file_operations fbackcar_dump = {
	.owner = THIS_MODULE,
	.open  = backcar_open,
	.read  = seq_read,
	.write = backcar_data_dump,
	.release = single_release,
};


/**
 * avin_probe : This function probes the avin capture driver
 * @pdev: platform device pointer
 *
 * This creates device entries by register itself to the V4L2 driver and
 * initializes fields of each channel objects
 */
static __init int avin_probe(struct platform_device *pdev)
{
	struct atc_avin_device *avin_dev;
	struct vb2_queue *q;
	struct video_device *vfd;
	int ret;
	vss_printk(VSS_LOG_INFO, "enter\n");

	cap_dev = &pdev->dev;
	avin_dev = kzalloc(sizeof(*avin_dev), GFP_KERNEL);
	if (!avin_dev) {
		vss_printk(VSS_LOG_ERR, "alloc avin device failed\n");
		return -ENOMEM;
	}

	g_avin_dev = avin_dev;
	avin_state = false;
	memset(di_add_arr, 0, sizeof(di_add_arr));
	memset(g_io_fd, 0, sizeof(g_io_fd));
	g_io_fd_num = 0;

	/* initialize locks */
	spin_lock_init(&avin_dev->slock);
	mutex_init(&avin_dev->lock);
	mutex_init(&avin_lock);

	/* init dma queues */
	INIT_LIST_HEAD(&avin_dev->buf_queue);

	avin_dev->v4l2_dev.release = avin_dev_release;

	/* Initialize v4l2_dev and make dev->driver_data point to v4l2_dev.
	 *dev may be NULL in rare cases (ISA devices). In that case you
	 *must fill in the v4l2_dev->name field before calling this function. */
	ret = v4l2_device_register(&pdev->dev, &avin_dev->v4l2_dev);
	if (ret) {
		vss_printk(VSS_LOG_ERR, "v4l2_device_register failed\n");
		kfree(avin_dev);
		return ret;
	}

	/* Initialize vb2 queue */
	q = &avin_dev->vid_q;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	q->drv_priv = avin_dev;
	q->buf_struct_size = sizeof(struct avin_cap_buffer);
	q->ops = &video_qops;
	q->mem_ops = &vb2_vmalloc_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = 1;
	q->lock = &avin_dev->lock;
	ret = vb2_queue_init(q);
	if (ret) {
		vss_printk(VSS_LOG_ERR, "vb2_queue_init failed!\n");
		v4l2_device_put(&avin_dev->v4l2_dev);
		kfree(avin_dev);
		return ret;
	}

	/* finally start creating the device nodes */
	vfd = &avin_dev->video_dev;
	snprintf(vfd->name, sizeof(vfd->name), AVIN_DRIVER_NAME);
	vfd->fops = &avin_fops;
	vfd->ioctl_ops = &avin_ioctl_ops;
	vfd->release = video_device_release_empty;
	vfd->v4l2_dev = &avin_dev->v4l2_dev;
	vfd->queue = &avin_dev->vid_q;
	vfd->tvnorms = V4L2_STD_ALL;
	vfd->lock = &avin_dev->lock;
	video_set_drvdata(vfd, avin_dev);
	ret = video_register_device(vfd, VFL_TYPE_GRABBER, AVIN_DEVICE_ID_BASE);
	if (ret) {
		vss_printk(VSS_LOG_ERR, "register video device failed\n");
		video_unregister_device(&avin_dev->video_dev);
		v4l2_device_put(&avin_dev->v4l2_dev);
		return ret;
	}
	vss_printk(VSS_LOG_INFO, "the major is %d, the minor is %d\n", VIDEO_MAJOR, vfd->minor);
	platform_set_drvdata(pdev, avin_dev);
	return 0;
}


/**
 * avin_remove() - driver remove handler
 * @device: ptr to platform device structure
 *
 * The vidoe device is unregistered
 */
static int avin_remove(struct platform_device *pdev)
{
	struct atc_avin_device *avin_dev = platform_get_drvdata(pdev);

	if (!avin_dev) {
		vss_printk(VSS_LOG_WARNING, "avin device is null, return directly\n");
		return 0;
	}

	video_unregister_device(&avin_dev->video_dev);
	platform_set_drvdata(pdev, NULL);
	v4l2_device_put(&avin_dev->v4l2_dev);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
/**
 * avin_suspend: avin device suspend
 */
static int avin_suspend(struct device *dev)
{
#if 0
	struct common_obj *common;
	struct channel_obj *ch;
	int i;

	for (i = 0; i < AVIN_TYPE_MAX; i++) {
		/* Get the pointer to the channel object */
		ch = avin_dev.dev[i];
		common = &ch->common[AVIN_VIDEO_INDEX];

		if (!vb2_start_streaming_called(&common->buffer_queue)) {
			continue;
		}
	}
#endif
	return 0;
}

/*
 * avin_resume: avin device suspend
 */
static int avin_resume(struct device *dev)
{
#if 0
	struct common_obj *common;
	struct channel_obj *ch;
	int i;

	for (i = 0; i < AVIN_TYPE_MAX; i++) {
		/* Get the pointer to the channel object */
		ch = avin_dev.dev[i];
		common = &ch->common[AVIN_VIDEO_INDEX];

		if (!vb2_start_streaming_called(&common->buffer_queue)) {
			continue;
		}
	}
#endif
	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(avin_pm_ops, avin_suspend, avin_resume);

static __refdata struct platform_driver avin_driver = {
	.driver = {
		.name = AVIN_DRIVER_NAME,
		.owner = THIS_MODULE,
		.pm = &avin_pm_ops,
	},
	.probe = avin_probe,
	.remove = avin_remove,
};

static struct platform_device avin_capture_dev = {
	.name = AVIN_DRIVER_NAME,
	.id = -1,
};

static int __init avin_module_init(void)
{
	int ret = 0;
	vss_printk(VSS_LOG_INFO, "enter\n");
	ret = platform_device_register(&avin_capture_dev);
	if (ret) {
		vss_printk(VSS_LOG_ERR, "platform_device_register failed(%d)!\n", ret);
		return ret;
	}

	ret = platform_driver_register(&avin_driver);
	if (ret) {
		vss_printk(VSS_LOG_ERR, "platform_driver_register failed(%d)!\n", ret);
		platform_device_unregister(&avin_capture_dev);
		return ret;
	}

	proc_create("driver/dump_mem", 0, NULL, &fbackcar_dump);
	vss_printk(VSS_LOG_INFO, "success!\n");
	return 0;
}

static void __exit avin_module_exit(void)
{
	vss_printk(VSS_LOG_INFO, "enter\n");
	platform_driver_unregister(&avin_driver);
	platform_device_unregister(&avin_capture_dev);
	vss_printk(VSS_LOG_INFO, "leave\n");
}

module_init(avin_module_init);
module_exit(avin_module_exit);


MODULE_DESCRIPTION("Autochips Audio/Video Port Interface driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(AVIN_DRIVER_VERSION);

