
/*
 *Department: ATC-SD-SD1
 *
 *Aurthor: Mingxu Wang(atc0027)
 *
 *Date: 2017-7-29
 */
#ifndef ATC_ARM2_LK_SUPPORT
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/sched.h>/*wake_up_process*/
#include <linux/kthread.h>/*kthread_create*/
#include <linux/err.h>/*IS_ERR,PTR_ERR*/
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>/*mutex*/
#include <mt-plat/aee.h>
#include "ac8257_addr_utils.h"
#else
#include <sys/types.h>
#include <platform/interrupts.h>
#include <string.h>
#include "platform/mt_typedefs.h"
#include "reserved_memory.h"
#include "dualarm.h"
#include <platform/mt_reg_base.h>
#include <platform/mt_irq.h>
#include "ac8257_addr_utils.h"
#include "mt_gic.h"
#endif
#include "wch_priv.h"
#include "wch_log.h"
#include "wch_if.h"
#include "wch_hal.h"
#include "avin_common.h"
#ifdef ATC_ARM2_LK_SUPPORT
#define IRQ_HANDLED INT_RESCHEDULE
#endif
#if WCH_DUMP_FIRST_20_FRAMES_BY_ICE
#define WCH_VDI_SKIP_BUFF_CNT 0
#define WCH_DI_SKIP_BUFF_CNT 0
#else
#define WCH_VDI_SKIP_BUFF_CNT 8
#define WCH_DI_SKIP_BUFF_CNT 5
#endif //WCH_DUMP_FIRST_20_FRAMES_BY_ICE

#define WCH_DBG_TIME  0

#if WCH_OPEN_SHADOW
#define WCH_WRITED_BUF_IDX_OFT		3U
#else
#define WCH_WRITED_BUF_IDX_OFT		2U
#endif
#define WCH_MAGIC_CHAR_NUM			4

AVIN_BUFFER_T wch_buf_stats[WCH_NUM][WCH_BUF_MAX_CNT];
static int buf_err_cnt[WCH_NUM];
static int wch_buf_index[WCH_NUM];
static int wch_vdi_send_flag = 0;
static int wch_vdi_send_idex = 0;
static int filed_flag_pre = -1;
static int filed_flag_cur = -1;

static u8 wch_di_triger_cnt = 0;
u32 g_wch_dbg_lvl = WCH_LOG_LVL_HAL;
const char *g_wch_log_level[] = {
	"[WCH] [ERR]",
	"[WCH] [WARN]",
	"[WCH] [INFO]",
	"[WCH] [HAL]",
	"[WCH] [DBG]",
	"[WCH] [IRQ]"
};
u8 garbage_log = 2;
static u32 _wch_reserve_base[WCH_NUM] = {
	0x60900000,
	0x60500000
};
static WCH_HW_INFO_T _wch_hw_info[WCH_NUM] = {
	{.state = WCH_HW_FREE},
	{.state = WCH_HW_FREE}
};

#ifndef ATC_ARM2_LK_SUPPORT
static unsigned int _wch_irq[WCH_NUM] = {
	0,
	0
};

static void __iomem *_wch_irq_clr_base[WCH_NUM] = {
	0,
	0
};

#else
static unsigned int _wch_irq[WCH_NUM] = {
	ARM9_WCH_VDI_IRQ_ID,
	ARM9_WCH_DI_IRQ_ID
};

static uintptr_t _wch_irq_clr_base[WCH_NUM] = {
	0x02800000 + ARM9_IO_BASE,
	0x02000000 + ARM9_IO_BASE
};

#endif
static const  char *_wch_state_name[WCH_HW_STATE_MAX] = {
	"WCH_HW_FREE",
	"WCH_HW_INIT",
	"WCH_HW_CONFIG",
	"WCH_HW_START",
	"WCH_HW_STOP"
};

const char *g_wch_name[WCH_NUM] = {
	"WCH_VDI",
	"WCH_DI"
};
#ifndef ATC_ARM2_LK_SUPPORT
static const char *_irq_name[WCH_NUM] = {
	"wch_vdi_ISR",
	"wch_di_ISR"
};
#endif
static const u8 _magic_char[WCH_NUM][WCH_MAGIC_CHAR_NUM] = {
	{'w', 'c', 'h', 'v'},
	{'w', 'c', 'h', 'd'},
};

static int wch_vdi_status = 0;
static void wch_hw_lock_init(WCH_HW_INFO_T * info)
{
#ifndef ATC_ARM2_LK_SUPPORT
	spin_lock_init(&info->wch_lock.lock);
	WCH_LOG(WCH_LOG_LVL_DBG, "wch_hw_lock_init %p \n", &info->wch_lock.lock);

#endif
	info->wch_lock.lock_cnt = 0;
	info->wch_lock.flags = 0;
}

static void wch_hw_spin_lock(WCH_HW_INFO_T * info)
{
#ifndef ATC_ARM2_LK_SUPPORT
	spin_lock_irqsave(&info->wch_lock.lock, info->wch_lock.flags);
#else

#endif
	info->wch_lock.lock_cnt++;
	WCH_LOG(WCH_LOG_LVL_IRQ, "wch_hw_spin_lock lock_cnt=%d \n", (s32)info->wch_lock.lock_cnt);
}

static void wch_hw_spin_unlock(WCH_HW_INFO_T * info)
{
#ifndef ATC_ARM2_LK_SUPPORT
	spin_unlock_irqrestore(&info->wch_lock.lock, info->wch_lock.flags);
#else

#endif
	info->wch_lock.lock_cnt--;
	WCH_LOG(WCH_LOG_LVL_IRQ, "wch_hw_spin_unlock lock_cnt=%d \n", (s32)info->wch_lock.lock_cnt);
}
#ifndef ATC_ARM2_LK_SUPPORT
void wch_hw_set_garbage_log(u8 log)
{
	garbage_log = log;
}

extern char _stext[];
extern char _etext[];

static void checkAddr (__u32 addr)
{
	__u32 start_addr = virt_to_phys(_stext);
	__u32 end_addr = virt_to_phys(_etext);

	if((addr >= start_addr) && (addr <= end_addr)) {
		WCH_LOG(WCH_LOG_LVL_DBG, "checkAddr: Addr = 0x%x\r\n",
				(unsigned int)addr);
		//TODO:
		panic("[MEM_INVALID]wch write address wch_error ");
	}
}
#if WCH_DUMP_BUFFER_ATTR
u32 wch_hw_get_dump_frame_cnt(WCH_ID_E wch_id)
{
	if (wch_id >= WCH_MAX)
	{
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d \n", wch_id);
		return 0;
	}

	return _wch_hw_info[wch_id].dump_frame_cnt;
}

u32 wch_hw_get_buf_cnt(WCH_ID_E wch_id)
{
	if (wch_id >= WCH_MAX)
	{
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d \n", wch_id);
		return 0;
	}

	return _wch_hw_info[wch_id].buf_info.buf_cnt;
}

void wch_hw_get_yc_addr(WCH_ID_E wch_id, uintptr_t *y_addr, uintptr_t *c_addr, u32 buf_idx)
{
	if (wch_id >= WCH_MAX ||
		(NULL == y_addr) ||
		(NULL == c_addr) ||
		(buf_idx >= _wch_hw_info[wch_id].buf_info.buf_cnt))
	{
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d, buf_idx: %u, y_addr: %p, c_addr: %p\n",
			wch_id, buf_idx, y_addr, c_addr);
		return;
	}

	*y_addr = _wch_hw_info[wch_id].buf_info.y_addr[buf_idx];
	*c_addr = _wch_hw_info[wch_id].buf_info.c_addr[buf_idx];
}

#endif

#endif

void wch_hw_enable(WCH_ID_E wch_id)
{
	if (wch_id >= WCH_MAX) {
		return;
	}
	wch_hal_enable(wch_id);
}
void wch_hw_disable(WCH_ID_E wch_id)
{
	if (wch_id >= WCH_MAX) {
		return;
	}
	wch_hal_disable(wch_id);
}

static void wch_hw_switch_buf(WCH_HW_INFO_T * info)
{
	uintptr_t y_addr = 0;
	uintptr_t c_addr = 0;
	WCH_BUF_INFO_T *buf_info = NULL;

	buf_info = &info->buf_info;

	y_addr = buf_info->y_addr[buf_info->buf_idx];
	c_addr = buf_info->c_addr[buf_info->buf_idx];

#ifndef ATC_ARM2_LK_SUPPORT
	checkAddr(y_addr);
	checkAddr(c_addr);
	wch_hal_set_yc_addr(info->wch_id, ARM1_CPUADDR_TO_DEVADDR_NOM4U(y_addr) >> 2, ARM1_CPUADDR_TO_DEVADDR_NOM4U(c_addr) >> 2);
#else
	wch_hal_set_yc_addr(info->wch_id, ARM2_CPUADDR_TO_DEVADDR_NOM4U(y_addr) >> 2, ARM2_CPUADDR_TO_DEVADDR_NOM4U(c_addr) >> 2);
	//wch_hal_set_yc_addr(info->wch_id, (y_addr >> 2), (c_addr >> 2));
#endif

	WCH_LOG(WCH_LOG_LVL_DBG, "y_addr=0x%x, c_addr=%x, buf_idx=%d \n",
		(u32)y_addr, (u32)c_addr, buf_info->buf_idx);

	buf_info->buf_idx++;

	if (buf_info->buf_idx == WCH_BUF_MAX_CNT){
		buf_info->buf_idx = 0;
	}
}

static u8 wch_hw_get_wchid_from_isrid(unsigned int int_vector, WCH_ID_E *wch_id)
{
	s32 i = 0;
	for (i = 0; i< WCH_NUM; i++) {
		if (int_vector == _wch_irq[_wch_hw_info[i].wch_id]){
			*wch_id = _wch_hw_info[i].wch_id;
			break;
		}
	}
	if (WCH_NUM == i) {
		WCH_LOG(WCH_LOG_LVL_ERR, "error int_vector=%d \n",  int_vector);
		return false;
	}
	WCH_LOG(WCH_LOG_LVL_DBG, "*wch_id=%d ,int_vector=%d \n", *wch_id, int_vector);
	return true;
}

#ifndef ATC_ARM2_LK_SUPPORT
static irqreturn_t wch_hw_Isr(int vector, void *dev_id)
{
#else
enum handler_return wch_hw_Isr(unsigned int vector)
{
#endif
	WCH_ID_E wch_id = WCH_VDI;
	WCH_HW_INFO_T * info = NULL;
	WCH_BUF_INFO_T *buf_info = NULL;
	int buf_idx = 0;
	u16 send_flag = 0;

	WCH_LOG(WCH_LOG_LVL_DBG, "wch_hw_Isr  interrupt = %d\n", vector);
	if (!wch_hw_get_wchid_from_isrid(vector, &wch_id)) {
		WCH_LOG(WCH_LOG_LVL_ERR, "get wch id fail interrupt = %d\n", vector);
		return IRQ_HANDLED;
	}
	wch_img_status_clear(wch_id, (uintptr_t)_wch_irq_clr_base[wch_id]);
	if(wch_id == WCH_VDI) {
		filed_flag_pre = wch_hal_check_filed_stauts(wch_id);
		if(filed_flag_pre == filed_flag_cur) {
			WCH_LOG(WCH_LOG_LVL_ERR, "irq is the same for before,will reutrn,filed_flag_pre = 0x%x,filed_flag_cur =0x%x\n",filed_flag_pre,filed_flag_cur);
			wch_img_status_enable(wch_id, (uintptr_t)_wch_irq_clr_base[wch_id]);
			return IRQ_HANDLED;
		}
		filed_flag_cur = filed_flag_pre;
	}

	info = &_wch_hw_info[wch_id];
	buf_info = &info->buf_info;
	info->irq_cnt ++;
	if (0 == (info->irq_cnt % 200)) {
		WCH_LOG(WCH_LOG_LVL_INFO, "WCH(%d) irq count(%d)\n", wch_id, info->irq_cnt);
    }
	wch_hw_spin_lock(info);

	if (WCH_HW_START != info->state) {
		WCH_LOG(WCH_LOG_LVL_DBG, "not start state: %s\n", _wch_state_name[info->state]);
		wch_img_status_enable(wch_id, (uintptr_t)_wch_irq_clr_base[wch_id]);
		wch_hw_spin_unlock(info);
		return IRQ_HANDLED;
	}

	if (wch_id == WCH_DI) {
		if (wch_di_triger_cnt <= 0) {
			WCH_LOG(WCH_LOG_LVL_WARN, "interrupt error,wch_di_triger_cnt:%d\n", wch_di_triger_cnt);
			wch_img_status_enable(wch_id, (uintptr_t)_wch_irq_clr_base[wch_id]);
			wch_hw_spin_unlock(info);
			return IRQ_HANDLED;
		} else {
			wch_di_triger_cnt--;
		}
#ifndef ATC_ARM2_LK_SUPPORT
		wch_hal_set_yc_addr(wch_id, ARM1_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER) >> 2, ARM1_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER + WCH_VDI_YBUF_SIZE) >> 2);
#else
		wch_hal_set_yc_addr(wch_id, ARM2_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER) >> 2, ARM2_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER + WCH_VDI_YBUF_SIZE) >> 2);
#endif
		info->get_wch_buf_info(&wch_buf_stats[wch_id][0]);
		//WCH_LOG(WCH_LOG_LVL_WARN, "interrupt call callback done \n");
		wch_img_status_enable(wch_id, (uintptr_t)_wch_irq_clr_base[wch_id]);
		wch_hw_spin_unlock(info);
		return IRQ_HANDLED;
	}


	if (info->skip_vsync_cnt > 0) {
		WCH_LOG(WCH_LOG_LVL_INFO, "interrupt = %d, need skip = %d\r\n",
			vector, (s32)info->skip_vsync_cnt);
		info->skip_vsync_cnt--;
#if WCH_OPEN_SHADOW
		if (0 == info->skip_vsync_cnt) {
			/*Before opening WCH shadow, we have switched a buffer to WCH. The first Buffer will be write immediately after WCH be enabled.
			   But after opening WCH shadow, the buffer being switched to WCH will be write in next interrupt.
			   So we need to switch a buffer to WCH before writing the first valid frame to buffer*/
			wch_hw_switch_buf(info);
		}
#endif

		wch_hw_spin_unlock(info);

	} else {
		if ((wch_id == WCH_VDI) && (wch_hal_check_filed_stauts(wch_id))) {
			wch_hw_spin_unlock(info);
			if (wch_vdi_send_flag) {
			buf_idx = (wch_vdi_send_idex + buf_info->buf_cnt - WCH_WRITED_BUF_IDX_OFT) % buf_info->buf_cnt;
				if (wch_buf_index[wch_id] != buf_idx) {
					WCH_LOG(WCH_LOG_LVL_INFO, "WCH(%d) SendIdex(%d) ExceptIdex(%d)\n", wch_id, buf_idx, wch_buf_index[wch_id]);
					wch_buf_index[wch_id] = buf_idx;
				}
				wch_buf_index[wch_id] = (wch_buf_index[wch_id] + 1) % buf_info->buf_cnt;
				atomic_set(&wch_buf_stats[wch_id][buf_idx].status, AVIN_BUF_BUSY);
				info->get_wch_buf_info(&wch_buf_stats[wch_id][buf_idx]);
			}
			wch_img_status_enable(wch_id, (uintptr_t)_wch_irq_clr_base[wch_id]);
			return IRQ_HANDLED;
		}
		if (buf_err_cnt[wch_id] == 0) {
			send_flag = 1;
		}
		if (atomic_read(&wch_buf_stats[wch_id][buf_info->buf_idx].status) == AVIN_BUF_FREE) {
			wch_hw_switch_buf(info);
			buf_idx = info->buf_info.buf_idx;
			wch_hw_spin_unlock(info);
			buf_err_cnt[wch_id] = 0;
		} else {
#ifndef ATC_ARM2_LK_SUPPORT
			wch_hal_set_yc_addr(wch_id, ARM1_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER) >> 2, ARM1_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER + WCH_VDI_YBUF_SIZE) >> 2);
#else
			wch_hal_set_yc_addr(wch_id, ARM2_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER) >> 2, ARM2_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER + WCH_VDI_YBUF_SIZE) >> 2);
#endif
			wch_hw_spin_unlock(info);
			buf_idx = info->buf_info.buf_idx + 1;
			buf_err_cnt[wch_id] ++;
			if (buf_err_cnt[wch_id] >= garbage_log) {
				WCH_LOG(WCH_LOG_LVL_INFO, "WCH(%d) idex(%d) No free Buf %d Times\n", wch_id, buf_info->buf_idx, buf_err_cnt[wch_id]);
			}
		}
	}

	WCH_LOG(WCH_LOG_LVL_DBG, " interrupt = %d, buf_idx = %d, info->get_wch_buf_info: %p, info: %p\n",
		(s32)vector, (s32)buf_info->buf_idx, info->get_wch_buf_info, info);

	if (0 == info->skip_vsync_cnt) {
#if WCH_DUMP_BUFFER_ATTR
		info->dump_frame_cnt++;
#endif

#if WCH_DUMP_FIRST_20_FRAMES_BY_ICE
		if (WCH_BUF_MAX_CNT == info->dump_frame_cnt) {
			wch_hal_stop(wch_id);
			wch_hal_deinit(wch_id);
		}
#endif /*WCH_DUMP_FIRST_20_FRAMES_BY_ICE*/

		if (send_flag == 1) {
			if (WCH_VDI == wch_id) {
				wch_vdi_send_flag = 1;
				wch_vdi_send_idex = buf_idx;
			}
		} else {
			if (WCH_VDI == wch_id) {
				wch_vdi_send_flag = 0;
			}
		}
	}
	wch_img_status_enable(wch_id, (uintptr_t)_wch_irq_clr_base[wch_id]);
	return IRQ_HANDLED;
}

#ifndef ATC_ARM2_LK_SUPPORT
static s32 wch_hw_requst_irq(WCH_HW_INFO_T *info, irq_handler_t isr)
{
	unsigned int irq_vector = 0;

	WCH_LOG(WCH_LOG_LVL_IRQ,"wch_id = %d \n", info->wch_id);

	irq_vector = _wch_irq[info->wch_id];

	WCH_LOG_INFO(info->wch_id,"WchRequstIrq! irq_vector = %d \n", irq_vector);

	return request_irq(irq_vector, isr, 0, _irq_name[info->wch_id], NULL);
}

static void wch_hw_free_irq(WCH_HW_INFO_T * info)
{
	unsigned int irq_vector = 0;

	WCH_LOG(WCH_LOG_LVL_IRQ,"wch_id = %d \n", info->wch_id);

	irq_vector = _wch_irq[info->wch_id];

	WCH_LOG(WCH_LOG_LVL_IRQ,"WchRequstIrq! irq_vector = %d \n", irq_vector);

	free_irq(irq_vector, NULL);
}
#else
static s32 wch_hw_requst_irq(WCH_HW_INFO_T *info)
{
	unsigned int irq_vector = 0;

	WCH_LOG(WCH_LOG_LVL_IRQ,"wch_id = %d \n", info->wch_id);

	irq_vector = _wch_irq[info->wch_id];

	WCH_LOG_INFO(info->wch_id,"WchRequstIrq! irq_vector = %d \n", irq_vector);

    // FIXME
	// register_int_handler(irq_vector, isr, &_wch_irq[info->wch_id]);
	mt_irq_unmask(irq_vector);

	return 0;
}

static void wch_hw_free_irq(WCH_HW_INFO_T * info)
{
	unsigned int irq_vector = 0;

	WCH_LOG_INFO(info->wch_id,"wch_id = %d \n", info->wch_id);

	irq_vector = _wch_irq[info->wch_id];

	WCH_LOG_INFO(info->wch_id,"WchRequstIrq! irq_vector = %d \n", irq_vector);

    // FIXME
    // mask_interrupt(irq_vector);
	mt_irq_mask(irq_vector);
}

#endif
void wch_hw_set_irq(WCH_ID_E wch_id, unsigned int irq)
{
	_wch_irq[wch_id] = irq;
}

void wch_hw_set_reserve_base(u32 reserve_base)
{
	_wch_reserve_base[WCH_VDI] = reserve_base;
}

static void wch_hw_alloc_buf(WCH_HW_INFO_T * info)
{
	u32 i = 0;
	WCH_BUF_INFO_T *buf_info = NULL;
	WCH_ID_E wch_id = WCH_VDI;
	u32 base_addr = 0;
	u32 y_buffer_size = 0;
	u32 c_buffer_size = 0;

	buf_info = &info->buf_info;
	wch_id = info->wch_id;

	if (WCH_VDI == wch_id) {
		base_addr = WCH_VDI_BASE;
		y_buffer_size = WCH_VDI_YBUF_SIZE;
		c_buffer_size = WCH_VDI_CBUF_SIZE;
	} else {
		base_addr = WCH_DI_BASE;
		y_buffer_size = WCH_DI_YBUF_SIZE;
		c_buffer_size = WCH_DI_CBUF_SIZE;
	}

	WCH_LOG_INFO(wch_id, "wch_hw_alloc_buf wch_id: %s,  base_addr: %x, y_buffer_size: %d, c_buffer_size: %d\n",
		g_wch_name[info->wch_id], (s32)base_addr, (s32)y_buffer_size, (s32)c_buffer_size);

	for (i = 0; i < WCH_BUF_MAX_CNT; i++) {
		buf_info->y_addr[i] = base_addr + (y_buffer_size + c_buffer_size) * i;
		buf_info->c_addr[i] = buf_info->y_addr[i] + y_buffer_size;
	}

	buf_info->buf_cnt = WCH_BUF_MAX_CNT;
	buf_info->buf_idx = 0;
}
#ifndef ATC_ARM2_LK_SUPPORT
u32 wch_hw_set_reg_base(WCH_ID_E wch_id, void *reg_base)
{
	if (wch_id >= WCH_MAX) {
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d \n", wch_id);
		return WCH_WRONG_ID;
	}

	wch_hal_set_reg_base(wch_id, reg_base);

	return WCH_SUCCESS;
}

u32 wch_hw_set_irq_clr_base(WCH_ID_E wch_id, void __iomem *reg_base)
{
	if (wch_id >= WCH_MAX) {
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d \n", wch_id);
		return WCH_WRONG_ID;
	}

	_wch_irq_clr_base[wch_id] = reg_base;

	return WCH_SUCCESS;
}

void wch_hw_set_clk_rst_base(WCH_ID_E wch_id, void __iomem *reg_base)
{
	wch_hal_set_clk_rst_base(wch_id, reg_base);
}
#endif
static void wch_hw_info_init(WCH_ID_E wch_id)
{
	u32 i = 0;
	WCH_HW_INFO_T *info = NULL;

	info = &_wch_hw_info[wch_id];
	memset(info, 0, sizeof(WCH_HW_INFO_T));

	for (i=0; i<WCH_MAGIC_CHAR_NUM; i++) {
		info->magic[i] = _magic_char[wch_id][i];
	}

	info->wch_id = wch_id;
	info->state = WCH_HW_FREE;
}

u32 wch_hw_init(WCH_ID_E wch_id)
{
	WCH_HW_INFO_T * info = NULL;

	if (wch_id >= WCH_MAX) {
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d \n", (s32)wch_id);
		return WCH_WRONG_ID;
	}

	info = &_wch_hw_info[wch_id];

	if (WCH_HW_FREE != info->state) {
		WCH_LOG(WCH_LOG_LVL_WARN,"%s is not free \n", g_wch_name[wch_id]);
		return WCH_STATE_ERROR;
	}

	WCH_LOG_INFO(wch_id, "wch_hw_init enter %s\n", g_wch_name[wch_id]);
	if (WCH_VDI == wch_id) {
		if (1 == wch_vdi_status) {
			info->state = WCH_HW_INIT;
			info->buf_info.buf_idx = 0;
			WCH_LOG(WCH_LOG_LVL_INFO,"wch_hw_init init succes %s \n", g_wch_name[wch_id]);
			return WCH_SUCCESS;
		}
	}
	wch_hw_info_init(wch_id);
	wch_hw_lock_init(info);
#ifdef ATC_ARM2_LK_SUPPORT
	if (WCH_VDI ==wch_id) {
			//_wch_reserve_base[wch_id] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(ARM1PHY2ARM2UCV(READ64(WCH_RSV_MEM_START_ADDR_4ARM2)));
			_wch_reserve_base[wch_id] = get_reserved_memory_node_addr("wch-vdi-reserved-memory");
			//_wch_reserve_base[wch_id] = (0x60900000 - 0x40000000);
	} else {
			//_wch_reserve_base[wch_id] = ARM2_CPUADDR_TO_DEVADDR_NOM4U(ARM1PHY2ARM2UCV(READ64(WCH_DI_DISP_RSV_MEM_START_ADDR_4ARM2)));
			_wch_reserve_base[wch_id] = get_reserved_memory_node_addr("wch-di-reserved-memory");
			//_wch_reserve_base[wch_id] = (0x60500000 - 0x40000000);
	}
#endif
	wch_hw_alloc_buf(info);

	wch_hal_init(wch_id);
	info->state = WCH_HW_INIT;

	WCH_LOG_INFO(wch_id,"wch_hw_init init succes %s \n", g_wch_name[wch_id]);

	return WCH_SUCCESS;
}


static u8 wch_hw_config_hal(PWCH_CFG_T wch_cfg)
{
	WCH_INPUT_INFO_T input_info;
	WCH_OUTPUT_INFO_T output_info;

	memset(&input_info, 0, sizeof(input_info));
	memset(&output_info, 0, sizeof(output_info));


	input_info.src_fmt		= wch_cfg->input_fmt;
	input_info.src_width	= wch_cfg->src_width;
	input_info.src_height	= wch_cfg->src_hight;

	/*next four is for wch_di only*/
	input_info.start_x		= wch_cfg->src_start_x;
	input_info.start_y_top	= wch_cfg->src_start_y_top;
	input_info.start_y_bot	= wch_cfg->src_start_y_bot;
	input_info.h_total		= wch_cfg->h_total;
	/*for wch_di only end*/

	input_info.input_data_type	= wch_cfg->input_data_type;

	wch_hal_set_input(wch_cfg->wch_id, &input_info);

	output_info.dst_fmt			= wch_cfg->output_fmt;
	output_info.data_mode		= wch_cfg->data_mode;
	output_info.plane_mode		= wch_cfg->plane_mode;
	output_info.dst_width		= wch_cfg->dst_width;
	output_info.dst_height		= wch_cfg->dst_height;
	output_info.dst_stride		= wch_cfg->dst_stride;
	output_info.input_data_type = wch_cfg->input_data_type;

	wch_hal_set_output(wch_cfg->wch_id, &output_info);

	return true;
}

u32 get_wch_garbage_addr(void)
{
	return WCH_GARBAGE_BUFFER;
}

u32 wch_hw_config(PWCH_CFG_T wch_cfg)
{
	WCH_ID_E wch_id = WCH_VDI;
	WCH_HW_INFO_T * info = NULL;

	if (NULL == wch_cfg) {
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hw_config param is NULL! \n");
		return WCH_INVALID_INPUT_PARAM;
	}

	if (sizeof(*wch_cfg) != sizeof(WCH_CFG_T)) {
		WCH_LOG(WCH_LOG_LVL_ERR, "param size error! \n");
		return WCH_INVALID_INPUT_PARAM;
	}

	wch_id = wch_cfg->wch_id;
	if (wch_id >= WCH_MAX)
	{
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d \n", wch_id);
		return WCH_WRONG_ID;
	}
	WCH_LOG_INFO(wch_id, " config %s\n", g_wch_name[wch_id]);
	info = &_wch_hw_info[wch_id];

	if ((WCH_HW_START == info->state) ||
		(WCH_HW_FREE == info->state) ||
		(WCH_HW_CONFIG == info->state)) {
		WCH_LOG(WCH_LOG_LVL_WARN, "state wrong: %s \n", _wch_state_name[info->state]);
		return WCH_STATE_ERROR;
	}

	if (0 == info->buf_info.buf_cnt) {
		WCH_LOG(WCH_LOG_LVL_WARN,"buffer count error %s \n", g_wch_name[wch_id]);
		return WCH_ALLOC_BUF_FAIL;
	}

	wch_hw_spin_lock(info);
	info->get_wch_buf_info= wch_cfg->get_wch_buf_info;

	WCH_LOG_INFO(wch_id, "get_wch_buf_info: %p\n", wch_cfg->get_wch_buf_info);
	wch_hw_spin_unlock(info);
	wch_hw_config_hal(wch_cfg);
    if (WCH_VDI == wch_id) {
	    wch_hw_switch_buf(info);
    } else {
#ifndef ATC_ARM2_LK_SUPPORT
	wch_hal_set_yc_addr(wch_id, ARM1_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER) >> 2, ARM1_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER + WCH_VDI_YBUF_SIZE) >> 2);
#else
	wch_hal_set_yc_addr(wch_id, ARM2_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER) >> 2, ARM2_CPUADDR_TO_DEVADDR_NOM4U(WCH_GARBAGE_BUFFER + WCH_VDI_YBUF_SIZE) >> 2);
#endif
    }
	info->state = WCH_HW_CONFIG;

	WCH_LOG_INFO(wch_id, " config %s success\n", g_wch_name[wch_id]);

	return WCH_SUCCESS;
}

u32 wch_hw_set_di_buffer(WCH_ID_E wch_id, uintptr_t y_addr, uintptr_t c_addr)
{
	if (wch_id != WCH_DI){
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hw_set_di_buffer Param Error, wch_id: %d \n", (s32)wch_id);
		return WCH_WRONG_ID;
	}
#ifndef ATC_ARM2_LK_SUPPORT
    wch_hal_set_yc_addr(wch_id, ARM1_CPUADDR_TO_DEVADDR_NOM4U(y_addr) >> 2, ARM1_CPUADDR_TO_DEVADDR_NOM4U(c_addr) >> 2);
#else
    wch_hal_set_yc_addr(wch_id, ARM2_CPUADDR_TO_DEVADDR_NOM4U(y_addr) >> 2, ARM2_CPUADDR_TO_DEVADDR_NOM4U(c_addr) >> 2);
#endif
    wch_buf_stats[wch_id][0]. y_addr = y_addr;
    wch_buf_stats[wch_id][0]. c_addr = c_addr;
	return WCH_SUCCESS;
}

u32 wch_hw_start(WCH_ID_E wch_id)
{
	WCH_HW_INFO_T * info = NULL;
	u16 i = 0;
	if (wch_id >= WCH_MAX){
		WCH_LOG(WCH_LOG_LVL_ERR, "wch_hw_start Param Error, wch_id: %d \n", (s32)wch_id);
		return WCH_WRONG_ID;
	}

	info = &_wch_hw_info[wch_id];

	if (WCH_HW_START == info->state) {
		WCH_LOG(WCH_LOG_LVL_WARN, "alreay Started: %s \n \n",  g_wch_name[wch_id]);
		return WCH_SUCCESS;
	}

	if ((WCH_HW_STOP != info->state) &&
		(WCH_HW_CONFIG != info->state)) {
		WCH_LOG(WCH_LOG_LVL_WARN, "state error: %s \n", _wch_state_name[info->state]);
		return WCH_STATE_ERROR;
	}

	WCH_LOG_INFO(wch_id, " start %s\n", g_wch_name[wch_id]);

#if WCH_DUMP_BUFFER_ATTR
	info->dump_frame_cnt = 0;
#endif
	if (WCH_DI == wch_id) {
		info->skip_vsync_cnt = WCH_DI_SKIP_BUFF_CNT;
	} else {
		info->skip_vsync_cnt = WCH_VDI_SKIP_BUFF_CNT;
	}
	info->irq_cnt = 0;
	for (i = 0; i < WCH_BUF_MAX_CNT; i ++) {
		atomic_set(&wch_buf_stats[wch_id][i].status, AVIN_BUF_FREE);
		wch_buf_stats[wch_id][i].y_addr = info->buf_info.y_addr[i];
		wch_buf_stats[wch_id][i].c_addr = info->buf_info.c_addr[i];
		wch_buf_stats[wch_id][i].idex = i;
	}
	buf_err_cnt[wch_id] = 0;
	wch_buf_index[wch_id] = 0;
	/*request IRQ*/
	if (WCH_VDI == wch_id) {
		if (1 == wch_vdi_status) {
			wch_hw_spin_lock(info);
			wch_hal_enable(wch_id);
			info->state= WCH_HW_START;
			wch_hw_spin_unlock(info);
			WCH_LOG(WCH_LOG_LVL_INFO,"StartWch start success (%s)! \n", g_wch_name[wch_id]);
			return WCH_SUCCESS;
		}
	}
#ifndef ATC_ARM2_LK_SUPPORT
	if (0 != wch_hw_requst_irq(info, wch_hw_Isr)) {
		WCH_LOG(WCH_LOG_LVL_ERR, "requst isq error\n");
		return WCH_FAIL_UNKNOWN;
	}
#else
	if (0 != wch_hw_requst_irq(info)) {
		WCH_LOG(WCH_LOG_LVL_ERR, "requst isq error\n");
		return WCH_FAIL_UNKNOWN;
	}
#endif

	wch_hal_start(wch_id);
	wch_hw_spin_lock(info);
	info->state= WCH_HW_START;
	if (WCH_VDI == wch_id) {
		wch_vdi_status = 1;
		wch_hal_set_shadow(wch_id);
	} else {
		wch_di_triger_cnt = 0;
	}
	wch_hw_spin_unlock(info);

#if WCH_OPEN_SHADOW
	if (0 == info->skip_vsync_cnt) {
		/*Before opening WCH shadow, we have switched a buffer to WCH. The first Buffer will be write immediately after WCH be enabled.
		   But after opening WCH shadow, the buffer being switched to WCH will be write in next interrupt.
		   So we need to switch a buffer to WCH before writing the first valid frame to buffer*/

		wch_hw_switch_buf(info);
	}
#endif
	WCH_LOG_INFO(wch_id,"StartWch start success (%s)! \n", g_wch_name[wch_id]);

	return WCH_SUCCESS;
}

u32 wch_hw_stop(WCH_ID_E wch_id)
{
	WCH_HW_INFO_T * info = NULL;
	int i = 0;
	if (wch_id >= WCH_MAX){
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d \n", (s32)wch_id);
		return WCH_WRONG_ID;
	}

	info = &_wch_hw_info[wch_id];

	if (WCH_HW_STOP == info->state) {
		WCH_LOG(WCH_LOG_LVL_WARN, "alreay Stoped: %s \n \n",  g_wch_name[wch_id]);
		return WCH_SUCCESS;
	}
	for (i = 0; i < WCH_BUF_MAX_CNT; i ++) {
		atomic_set(&wch_buf_stats[wch_id][i].status, AVIN_BUF_BUSY);
	}

	WCH_LOG_INFO(wch_id, " stop %s\n", g_wch_name[wch_id]);
	wch_hw_spin_lock(info);

	if (WCH_HW_START != info->state){
		wch_hw_spin_unlock(info);
		WCH_LOG(WCH_LOG_LVL_WARN, "state wrong: %s \n", _wch_state_name[info->state]);
		return WCH_STATE_ERROR;
	}


#if WCH_DUMP_BUFFER_ATTR
	info->dump_frame_cnt = 0;
#endif
	if (WCH_VDI == wch_id) {
		wch_hal_disable(wch_id);
	}
	else {
		wch_hal_stop(wch_id);
	}
	info->state = WCH_HW_STOP;
	wch_hw_spin_unlock(info);
	if (WCH_DI == wch_id) {
		wch_hw_free_irq(info);
	}
	WCH_LOG_INFO(wch_id, " stop %s success\n", g_wch_name[wch_id]);

	return WCH_SUCCESS;
}


u32 wch_hw_deinit(WCH_ID_E wch_id)
{
	WCH_HW_INFO_T * info = NULL;

	if (wch_id >= WCH_MAX){
		WCH_LOG(WCH_LOG_LVL_ERR, "Param Error, wch_id: %d \n", (s32)wch_id);
		return WCH_WRONG_ID;
	}

	info = &_wch_hw_info[wch_id];

	if (WCH_HW_FREE == info->state) {
		WCH_LOG(WCH_LOG_LVL_WARN, "alreay Closed: %s \n",  g_wch_name[wch_id]);
		return WCH_SUCCESS;
	}

	WCH_LOG_INFO(wch_id, " deinit %s\n", g_wch_name[wch_id]);
	if (WCH_VDI == wch_id) {
		if (1 == wch_vdi_status) {
			wch_hw_spin_lock(info);
			info->state = WCH_HW_FREE;
			wch_hw_spin_unlock(info);
			WCH_LOG(WCH_LOG_LVL_INFO, "deinit %s success\n", g_wch_name[wch_id]);
			return WCH_SUCCESS;

		}
	}
	if (WCH_HW_START == info->state) {
		wch_hal_stop(wch_id);
	}
	wch_hal_deinit(wch_id);
	wch_hw_spin_lock(info);
	info->state = WCH_HW_FREE;
	wch_hw_spin_unlock(info);
	WCH_LOG_INFO(wch_id, " deinit %s success\n", g_wch_name[wch_id]);

	return WCH_SUCCESS;

}

u32 wch_hw_clear(WCH_ID_E wch_id)
{
	WCH_HW_INFO_T * info = NULL;
	info = &_wch_hw_info[wch_id];

	if ((WCH_VDI == wch_id) && (1 == wch_vdi_status)) {
		wch_hw_spin_lock(info);
		wch_hal_stop(wch_id);
		wch_hw_spin_unlock(info);
		wch_hw_free_irq(info);
		wch_hal_deinit(wch_id);
		WCH_LOG(WCH_LOG_LVL_INFO, "wch_hw_clear %s success\n", g_wch_name[wch_id]);
		wch_vdi_status = 0;
	}

	return WCH_SUCCESS;
}

u32 wch_set_di_triger_status(WCH_ID_E wch_id)
{
	if (wch_id != WCH_DI) {
		return WCH_WRONG_ID;
	}

	wch_di_triger_cnt ++;
	if (wch_di_triger_cnt > 1) {
		WCH_LOG(WCH_LOG_LVL_WARN, "Warnig: wch_di_triger_cnt:%d \n", wch_di_triger_cnt);
		//wch_di_triger_cnt = 1;
	}

	return WCH_SUCCESS;
}
#ifndef ATC_ARM2_LK_SUPPORT
EXPORT_SYMBOL(wch_hw_init);
EXPORT_SYMBOL(wch_hw_config);
EXPORT_SYMBOL(wch_hw_set_di_buffer);
EXPORT_SYMBOL(wch_hw_start);
EXPORT_SYMBOL(wch_hw_stop);
EXPORT_SYMBOL(wch_hw_deinit);
EXPORT_SYMBOL(wch_hw_clear);
EXPORT_SYMBOL(wch_set_di_triger_status);
#endif
