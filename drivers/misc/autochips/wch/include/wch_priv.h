

#ifndef _WCH_PRIV_H_
#define _WCH_PRIV_H_

#ifndef ATC_ARM2_LK_SUPPORT
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/types.h>
#include <linux/io.h>/*__va,__pa*/
#else
#include "platform/mt_typedefs.h"
#endif
#include "wch_if.h"

#define WCH_DUMP_BUFFER_ATTR 				1
#define WCH_DUMP_FIRST_20_FRAMES_BY_ICE 	0
#define WCH_NUM								WCH_MAX

#if WCH_DUMP_FIRST_20_FRAMES_BY_ICE
#define WCH_BUF_MAX_CNT		20
#else
#define WCH_BUF_MAX_CNT		5
#endif //WCH_DUMP_FIRST_20_FRAMES_BY_ICE

/*WCH_VDI Buffer Info def*/
#define WCH_VDI_BUFFER_SIZE 	(720*288*3*WCH_BUF_MAX_CNT)
#define WCH_VDI_BASE			(_wch_reserve_base[WCH_VDI])
#define WCH_VDI_END 			(WCH_VDI_BASE + WCH_VDI_BUFFER_SIZE)
#define WCH_VDI_YBUF_SIZE		(720*576)
#define WCH_VDI_CBUF_SIZE		(720*288)/*MAX Support to 420*/

/* WCH_DI */
#define WCH_DI_BUFFER_SIZE 		(720*576*2*WCH_BUF_MAX_CNT)
#define WCH_DI_BASE				(_wch_reserve_base[WCH_DI])
#define WCH_DI_END 				(WCH_DI_BASE + WCH_DI_BUFFER_SIZE)
#define WCH_DI_YBUF_SIZE		(720*576)
#define WCH_DI_CBUF_SIZE		(720*576)  /*MAX Support to 422*/

#define WCH_GARBAGE_BUFFER		(WCH_VDI_END)
#define OFFSET_CPU2WCH		0x40000000
#define CPU_ADDR_2_WCH_ADDR(addr) (((addr) - OFFSET_CPU2WCH) >> 2)
typedef struct {
	u32 	buf_cnt;
	u32 	buf_idx;
	u32 	y_addr[WCH_BUF_MAX_CNT];
	u32 	c_addr[WCH_BUF_MAX_CNT];
} WCH_BUF_INFO_T, *PWCH_BUF_INFO_T;

typedef struct {
#ifndef ATC_ARM2_LK_SUPPORT
	spinlock_t 		lock;
#endif
	unsigned long 	flags;
	u32 			lock_cnt;
} WCH_LOCK_T;

typedef enum {
	WCH_HW_FREE = 0,
	WCH_HW_INIT,
	WCH_HW_CONFIG,
	WCH_HW_START,
	WCH_HW_STOP,
	WCH_HW_STATE_MAX
}WCH_HW_STATE_E;


typedef struct {
	unsigned char	magic[4];

	WCH_ID_E 				wch_id;
	WCH_HW_STATE_E  		state;
	WCH_LOCK_T 				wch_lock;
	WCH_BUF_INFO_T 			buf_info;
	u32 					skip_vsync_cnt;
	get_wch_buffer_info		get_wch_buf_info;
	u8 					show_isr_interval;
	u32 					irq_cnt;
	#if WCH_DUMP_BUFFER_ATTR
	u32 dump_frame_cnt;
	#endif
}WCH_HW_INFO_T;

void wch_hw_enable(WCH_ID_E wch_id);
void wch_hw_disable(WCH_ID_E wch_id);
#ifndef ATC_ARM2_LK_SUPPORT
void wch_hw_set_garbage_log(u8 log);
void wch_hw_set_io_base(void *io_base);
u32 wch_hw_set_reg_base(WCH_ID_E wch_id, void *reg_base);
u32 wch_hw_set_irq_clr_base(WCH_ID_E wch_id, void __iomem *reg_base);
void wch_hw_set_reserve_base(u32 reserve_base);
void wch_hw_set_irq(WCH_ID_E wch_id, unsigned int irq);
void wch_hw_set_clk_rst_base(WCH_ID_E wch_id, void __iomem *reg_base);

#if WCH_DUMP_BUFFER_ATTR
u32 wch_hw_get_dump_frame_cnt(WCH_ID_E wch_id);
u32 wch_hw_get_buf_cnt(WCH_ID_E wch_id);
void wch_hw_get_yc_addr(WCH_ID_E wch_id, uintptr_t *y_addr, uintptr_t *c_addr, u32 buf_idx);
#endif
#endif
#endif

