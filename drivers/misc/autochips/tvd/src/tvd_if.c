#if defined(__KERNEL__)
  #include <linux/types.h>
  #include <linux/module.h>
  #include <linux/spinlock.h>
  #include <linux/mutex.h>
  #include <linux/slab.h>
  #include <linux/irqreturn.h>
  #include <linux/interrupt.h>
  #include <linux/workqueue.h>
  #include <linux/err.h>
#elif defined(ATC_ARM2_LK_SUPPORT)
  #include <stdint.h>
  #include <string.h>
  #include "platform/mt_typedefs.h"
  #include "platform/mt_reg_base.h"
  #include "platform/mt_reg.h"
  #include "mt_gic.h"
#else
  #include "types.h"
#endif

#include "tvd_if.h"
#include "tvd_if_priv.h"
#include "tvd_hal.h"
#include "tvd_log.h"
#include "wch_if.h"

#if defined(__KERNEL__)
static struct workqueue_struct *_tvd_wq = NULL;
static struct work_struct _tvd_work;
static void (*_work_fn)(void) = NULL;
static void _work_func(struct work_struct* work)
{
	if (_work_fn) {
		_work_fn();
	}
}
static int worker_init(void(*pfn)(void))
{
	if (!_tvd_wq) {
		_tvd_wq = alloc_ordered_workqueue("tvd_wq", WQ_MEM_RECLAIM | WQ_HIGHPRI);
		if (IS_ERR_OR_NULL(_tvd_wq)) {
			tvd_err("create tvd work queue failed\n");
			_tvd_wq = NULL;
			return -1;
		}
		_work_fn = pfn;
		INIT_WORK(&_tvd_work, _work_func);
	} else {
		tvd_warn("the work queue is already exist\n");
	}
	return 0;
}

static int worker_call(void)
{
	if (!_tvd_wq) {
		tvd_err("tvd workqueue not exist\n");
		return -1;
	}
	queue_work(_tvd_wq, &_tvd_work);
	return 0;
}

static int worker_flush(void)
{
	if (!_tvd_wq) {
		tvd_err("tvd workqueue not exist\n");
		return -1;
	}
	flush_workqueue(_tvd_wq);
	return 0;
}

static void worker_deinit(void)
{
	if (_tvd_wq) {
		destroy_workqueue(_tvd_wq);
		_tvd_wq = NULL;
	}
}

static void (*_irq_pfn)(void) = NULL;
static irqreturn_t _isr_func (int irq, void* dev)
{
	if (_irq_pfn) {
		_irq_pfn();
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}
static int irq_set_handler(unsigned int irq, void(*handler)(void))
{
	int ret;
	if (!handler) {
		return -1;
	}
	_irq_pfn = handler;
	ret = request_irq(irq, _isr_func, IRQF_TRIGGER_LOW, "tvd", NULL);
	if (ret < 0) {
		_irq_pfn = NULL;
		return ret;
	}
	return 0;
}
static void irq_clear_handler(unsigned int irq)
{
	if (_irq_pfn) {
		free_irq(irq, NULL);
		_irq_pfn = NULL;
	}
}
extern bool avin_state;
#else

typedef struct {int reserved;} spinlock_t;
#define spin_lock_init(lock) ((void)(lock))
#define spin_lock_irqsave(lock, flags) ((void)(lock), (void)(flags))
#define spin_unlock_irqrestore(lock, flags) ((void)(lock), (void)(flags))

struct mutex {int reserved;};
#define mutex_init(lock) ((void)(lock))
#define mutex_lock(lock) ((void)(lock))
#define mutex_unlock(lock) ((void)(lock))
#define mutex_destroy(lock) ((void)(lock))

#define vss_aee_print(...)

static void (*_work_fn)(void) = NULL;
static int worker_init(void(*pfn)(void))
{
	_work_fn = pfn;
	return 0;
}

static int worker_call(void)
{
	if (!_work_fn) {
		tvd_err("tvd workqueue not exist\n");
		return -1;
	}
	_work_fn();
	return 0;
}

static int worker_flush(void)
{
	if (!_work_fn) {
		tvd_err("tvd workqueue not exist\n");
		return -1;
	}
	return 0;
}

static void worker_deinit(void)
{
	if (_work_fn) {
		_work_fn = NULL;
	}
}

static void (*_irq_pfn)(void) = NULL;
#if defined(ATC_ARM2_LK_SUPPORT)
void tvd_irq_handler(u32 irq)
{
	if (_irq_pfn) {
		_irq_pfn();
	}
}
#else
extern void v_enable_bim_irq(u32 u4Id);
extern void v_disable_bim_irq(u32 u4Id);
void tvd_irq_function(u32 irq) //For ARM2 interrrupt
{
	if (_irq_pfn) {
		_irq_pfn();
	}
}
#endif

static int irq_set_handler(unsigned int irq, void(*handler)(void))
{
	if (!handler) {
		return -1;
	}
	_irq_pfn = handler;
#if defined(ATC_ARM2_LK_SUPPORT)
	mt_irq_unmask(irq);
#else
	v_enable_bim_irq(irq);
#endif
	return 0;
}
static void irq_clear_handler(unsigned int irq)
{
	if (_irq_pfn) {
#if defined(ATC_ARM2_LK_SUPPORT)
		mt_irq_mask(irq);
#else
		v_disable_bim_irq(irq);
#endif
		_irq_pfn = NULL;
	}
}
#define avin_state 1
#endif

#define TVD_VERSION			"01.01.01"

static const TVD_HW_RESOURCE* g_hw_resource = NULL;

static TVD_BUF_INFO_FUNC g_buf_info_callback = NULL;
static TVD_SOURCE_INFO_FUNC g_source_info_callback = NULL;

static int g_tvd_started = 0;
static SOURCE_INFO g_source_info = {0};
static spinlock_t g_info_lock;

static int g_wch_started = 0;
static struct mutex g_wch_state_lock;

static void tvd_worker_func(void);

void tvd_module_context_init(const TVD_HW_RESOURCE* resource)
{
	static TVD_HW_RESOURCE hw_resource;
	hw_resource = *resource;
	g_hw_resource = &hw_resource;

	tvd_info("TVD HW regbase: %p,%p,%p,%p,%p irq:%d reserved_mem:%X\n", 
			hw_resource.reg_base.tvd_base,
			hw_resource.reg_base.vdisys_base,
			hw_resource.reg_base.cvbs_base,
			hw_resource.reg_base.clk_base,
			hw_resource.reg_base.vdisys_dram_bridge_base,
			hw_resource.tvd_irq,
#ifdef CONFIG_ARM64
			hw_resource.tdc_dram_addr);
#else
			(unsigned int)hw_resource.tdc_dram_addr);
#endif

	g_buf_info_callback = NULL;
	g_source_info_callback = NULL;

	g_tvd_started = false;
	memset(&g_source_info, 0, sizeof(g_source_info));
	spin_lock_init(&g_info_lock);

	g_wch_started = 0;
	mutex_init(&g_wch_state_lock);

	worker_init(tvd_worker_func);
	tvd_hal_set_reg_base(&hw_resource.reg_base);
}

void tvd_module_context_deinit(void)
{
	worker_deinit();
	mutex_destroy(&g_wch_state_lock);
	g_hw_resource = NULL;
}

static int stop_wch(void)
{
	int ret = -1;

	tvd_info("%s enter\n", __FUNCTION__);

	mutex_lock(&g_wch_state_lock);
	if (!g_wch_started) {
		tvd_warn("%s leave, because the wch have not started\n", __FUNCTION__);
		mutex_unlock(&g_wch_state_lock);
		return 0;
	}

	ret = wch_hw_stop(WCH_VDI);
	if (ret) {
		tvd_err( "stop wch failed\n");
		vss_aee_print("stop wch failed");
		mutex_unlock(&g_wch_state_lock);
		return -1;
	}
	g_wch_started = 0;
	mutex_unlock(&g_wch_state_lock);
	tvd_info("%s leave\n", __FUNCTION__);
	return 0;
}

static int start_wch(unsigned width, unsigned height)
{
	WCH_CFG_T wch_cfg;
	int ret = -1;

	tvd_info("%s enter\n", __FUNCTION__);

	mutex_lock(&g_wch_state_lock);
	if (g_wch_started || !avin_state) {
		tvd_warn("%s leave, because the wch(state is %d) have started or avin(state is %d) is stop\n", __FUNCTION__, 
								g_wch_started, avin_state);
		mutex_unlock(&g_wch_state_lock);
		return 0;
	}

	wch_cfg.wch_id = WCH_VDI;
	wch_cfg.input_fmt = DATA_IN_FMT_YUV422;
	wch_cfg.src_width = width;
	wch_cfg.src_hight = height;
	wch_cfg.src_start_x = 0;
	wch_cfg.src_start_y_top = 0;
	wch_cfg.src_start_y_bot = 0;
	wch_cfg.input_data_type = INDATA_TYPE_I;
	wch_cfg.output_fmt = DATA_OUT_FMT_YUV420_2PLANES;
	wch_cfg.dst_width = width;
	wch_cfg.dst_height = height;
	wch_cfg.data_mode = DATA_MODE_BLOCK;
	wch_cfg.output_data_type = OUTDATA_TYPE_FRAME;
	wch_cfg.get_wch_buf_info = (get_wch_buffer_info)g_buf_info_callback;
	wch_cfg.uv_swap = 0;
	ret = wch_hw_config(&wch_cfg);
	if (ret) {
		tvd_err("config wch failed and ret is %d\n", ret);
		mutex_unlock(&g_wch_state_lock);
		return -1;
	}

	ret = wch_hw_start(WCH_VDI);
	if (ret) {
		tvd_err("start wch failed and ret is %d\n", ret);
		mutex_unlock(&g_wch_state_lock);
		return -1;
	}
	g_wch_started = true;
	mutex_unlock(&g_wch_state_lock);
	tvd_info("%s leave\n", __FUNCTION__);
	return 0;
}

#if defined(__KERNEL__)
extern int tvd_status; //XXX
#endif

static void tvd_worker_func(void)
{
	SOURCE_INFO source_info;
	unsigned long flags = 0;
	int ret = -1;

	if (!g_tvd_started) {
		tvd_warn("tvd not start.\n");
		return;
	}

	spin_lock_irqsave(&g_info_lock, flags);
	source_info = g_source_info;
	spin_unlock_irqrestore(&g_info_lock, flags);

	tvd_info("the signal state is %d\n", source_info.signal_state);

	switch(source_info.signal_state) {
		case SIGNAL_LOST:
		case SIGNAL_MODE_CHANGE_START:
			ret = stop_wch();
			break;
		case SIGNAL_STABLE:
		case SIGNAL_MODE_CHANGE_DONE:
			ret = start_wch(source_info.source_width, source_info.source_height);
			break;
		default:
			tvd_err("this signal status is error, reset tvd logic\n");
#if defined(__KERNEL__)
			tvd_status = 0;
#endif
			tvd_hal_vsync_irq_enable(0);
			tvd_hal_irq_enable(0);
			wch_hw_deinit(WCH_VDI);
			wch_hw_clear(WCH_VDI);
			tvd_hal_deinit();
			tvd_hal_init();
			wch_hw_init(WCH_VDI);
			tvd_hal_irq_enable(1);
			tvd_hal_vsync_irq_enable(1);
			break;
	}

	if (ret) {
		tvd_err("enable wch vdi failed\n");
		return;
	}

	if (g_source_info_callback) {
		g_source_info_callback(&source_info);
	}
}

static void tvd_get_source_size(int mode, SOURCE_INFO* source_info)
{
	source_info->source_width = 720;
	source_info->source_height = 480;

	switch (mode) {
	case TVD_MODE_PAL_N:
	case TVD_MODE_PAL:
		source_info->source_width = 720;
		source_info->source_height = 576;
		break;

	case TVD_MODE_PAL_M:
	case TVD_MODE_NTSC:
	case TVD_MODE_PAL_60:
	case TVD_MODE_NTSC443:
		source_info->source_width = 720;
		source_info->source_height = 480;
		break;

	case TVD_MODE_SECAM:
		source_info->source_width = 720;
		source_info->source_height = 480;
		break;

	default:
		break;
	}
	tvd_info("the width is %d, the height is %d\n", source_info->source_width, source_info->source_height);
}

#define MODE_STABLE_COUNT		8U
#define SIGNAL_LOST_COUNT		1U
#define SIGNAL_STABLE_COUNT		8U
#define ASSUME_SIGNAL_STABLE_NUM	30
#define TVD3D_COMB_ENABLE   1

static unsigned int __lost_count = 0;
static int check_source_lost(void)
{
	int signal_lost = false;

	if (!(tvd_hal_vpres_on())) {
		__lost_count ++;
	} else {
		__lost_count = 0;
	}

	/*when continuous eight times which we detect signal lost, we assume the signal is lost*/
	if (__lost_count >= SIGNAL_LOST_COUNT) {
		signal_lost = true;
		__lost_count = 0;
	}
	return signal_lost;
}

static unsigned int __stable_count = 0;
/*check the signal/mode is stable or not*/
static int check_source_stable(unsigned int wait_type)
{
	int wait_signal_stable = ((wait_type & 0xF0) ? true : false);
	unsigned int stable_num = (wait_signal_stable ? SIGNAL_STABLE_COUNT : MODE_STABLE_COUNT);
	int source_stable = false;

	if (tvd_hal_sync_locked(!wait_signal_stable)) {
		__stable_count ++;
		tvd_info("the signal is locked temporarily\n");
	} else {
		__stable_count = 0;
		tvd_info("the status is not locked, so we reset count\n");
	}

	/*when continuous 8/16 times  which we detect locked, we assume the source is stable*/
	if (__stable_count > stable_num) {
		source_stable = true;
		__stable_count = 0;
	}
	return source_stable;
}

static unsigned int __vsync_count = 0;
static unsigned int __wait_type = 0x00;/*the high 4 bits indicate waiting signal stable, the low 4 bits indicate waiting mode stable*/
static unsigned int __signal_stable = 0;
static void tvd_isr_func (void)
{
	u32 irqstatus;
	u32 detail_mode;
	SOURCE_INFO source_info;
	unsigned long flags = 0;
	const char* signal_str[SIGNAL_MAX] = {"signal none", "signal stable", "signal lost", "mode change start", "mode change done", "signal error"};

	irqstatus = tvd_hal_irq_status();
	tvd_hal_irq_clear();

	//vss_printk(VSS_LOG_DEBUG, "the interrupt enable is 0x%08x, irq status is 0x%08x, vpress status is 0x%08x\n",
	//		TVD_READ32(TVD_INTR_EN), TVD_READ32(TVD_INTR_STA), TVD_READ32(REG_DFE_STA_00));
	//tvd_dbg("irq status is 0x%08x", irqstatus);

	tvd_hal_set_img_status(0);

	if (((irqstatus & TVD_INTR_STA_MODE) == TVD_INTR_STA_MODE) || 
		((irqstatus & TVD_INTR_STA_VPRES) == TVD_INTR_STA_VPRES)) {
		int vpress_irq = false;
		vpress_irq = (((irqstatus & TVD_INTR_STA_VPRES) == TVD_INTR_STA_VPRES) ? true : false);
		if ((!vpress_irq) && (!__signal_stable)) {
			tvd_warn("first mode irq force assign to vpress, after signal lost\n");
			vpress_irq = true;
		}

		/*we will discard all interrupts which are coming when we wait souce stable,
		*and discard the mode change interrupt when the signal is not stable
		*/
		if (__wait_type || ((!vpress_irq) && (!__signal_stable))) {
			tvd_warn("discard all irq which when the souce stable is not stable "
							"wait_type(0x%X), vpress_irq(%d), signal_stable(%d)\n",
					__wait_type, vpress_irq, __signal_stable);
		} else {
			tvd_info("this is %s\n", (vpress_irq ? "vpress irq" : "mode change irq"));
			__wait_type |= (vpress_irq ? (0xF0) : (0x0F));

			tvd_hal_vsync_irq_enable(1);
			/*this is mode change irq, we assume it is mode change start signal*/
			if ((!vpress_irq) && (SIGNAL_MODE_CHANGE_START != g_source_info.signal_state) &&
				(SIGNAL_LOST != g_source_info.signal_state) && (SIGNAL_NONE != g_source_info.signal_state)) {

				spin_lock_irqsave(&g_info_lock, flags);
				g_source_info.signal_state = SIGNAL_MODE_CHANGE_START;
				g_source_info.source_width = 0;
				g_source_info.source_height = 0;
				spin_unlock_irqrestore(&g_info_lock, flags);
				worker_call();
				__signal_stable = false;
			}
		}
	}

	if ((irqstatus & TVD_INTR_STA_VSYNC) == TVD_INTR_STA_VSYNC) {
		__vsync_count ++;

		if (check_source_lost()) {
			if (SIGNAL_NONE != g_source_info.signal_state || __vsync_count >= 3) {
				tvd_hal_vsync_irq_enable(0);
				tvd_info("the signal is lost\n");
				if ((SIGNAL_LOST != g_source_info.signal_state)) {
					spin_lock_irqsave(&g_info_lock, flags);
					g_source_info.signal_state = SIGNAL_LOST;
					g_source_info.source_width = 0;
					g_source_info.source_height = 0;
					spin_unlock_irqrestore(&g_info_lock, flags);
					worker_call();
				}
				__wait_type  = 0x00;
				__vsync_count = 0;
				__signal_stable = false;
			}
		} else if (!__signal_stable) {
			/*source_stable is true indicate that signal or mode is stable*/
			if (check_source_stable(__wait_type)) {
				tvd_hal_vsync_irq_enable(0);
				detail_mode = tvd_hal_mode_detect();
				if(TVD3D_COMB_ENABLE){
					tvd_hal_3d_setting(detail_mode, g_hw_resource->tdc_dram_addr);
				}else{
					tvd_hal_2d_setting(detail_mode);
				}
				tvd_hal_special_setting();
				//check_tvd_dma_addr(); //FIXME
				source_info.signal_state = ((__wait_type & 0xF0) ? SIGNAL_STABLE : SIGNAL_MODE_CHANGE_DONE);
				tvd_get_source_size(detail_mode, &source_info);
				tvd_info("%s\n", signal_str[source_info.signal_state]);
				if (g_source_info.signal_state != source_info.signal_state) {
					spin_lock_irqsave(&g_info_lock, flags);
					g_source_info = source_info;
					spin_unlock_irqrestore(&g_info_lock, flags);
					worker_call();
				}
				__wait_type = 0x00;
				__vsync_count = 0;
				__signal_stable = true;
			} else if (__vsync_count >= ASSUME_SIGNAL_STABLE_NUM) {
				//dump_tvd_reg();
				tvd_info("now start to reset tvd logic\n");
				/*reset tvd logic*/
				//TVD_CLR_BIT((0x130), BIT(4));
				//TVD_SET_BIT((0x130), BIT(4));
				spin_lock_irqsave(&g_info_lock, flags);
				g_source_info.signal_state = SIGNAL_ERROR;
				g_source_info.source_width = 0;
				g_source_info.source_height = 0;
				spin_unlock_irqrestore(&g_info_lock, flags);
				worker_call();
				__vsync_count = 0;
				__stable_count = 0;
			}
		} else {
			tvd_warn("the signal is already stable\n");
			tvd_hal_vsync_irq_enable(0);
			if (SIGNAL_STABLE != g_source_info.signal_state) {
				tvd_info("%s\n", signal_str[SIGNAL_STABLE]);
				spin_lock_irqsave(&g_info_lock, flags);
				g_source_info.signal_state = SIGNAL_STABLE;
				spin_unlock_irqrestore(&g_info_lock, flags);
				worker_call();
			}
			__wait_type  = 0x00;
			__vsync_count = 0;
		}
	}
	tvd_hal_set_img_status(1);
}

void tvd_callback_register(TVD_BUF_INFO_FUNC buf_func, TVD_SOURCE_INFO_FUNC source_func)
{
	tvd_info("%s enter\n", __FUNCTION__);
	g_buf_info_callback = buf_func;
	g_source_info_callback = source_func;
	tvd_info("%s leave\n", __FUNCTION__);
}

void tvd_callback_unregister(void)
{
	tvd_info("%s enter\n", __FUNCTION__);
	g_buf_info_callback = NULL;
	g_source_info_callback = NULL;
	tvd_info("%s leave\n", __FUNCTION__);
}

int tvd_hw_init(void)
{
	u32 detail_mode;
	int ret = -1;

	tvd_info("%s enter and version is %s\n", __FUNCTION__, TVD_VERSION);

	if (!g_hw_resource) {
		tvd_err("tvd module context not init!\n");
		return -1;
	}

	ret = irq_set_handler(g_hw_resource->tvd_irq, tvd_isr_func);
	if (ret) {
		tvd_err("Unable to request IRQ, irq=%d\n", g_hw_resource->tvd_irq);
		return ret;
	}

	tvd_hal_init();
	g_tvd_started = false;
	g_source_info.signal_state = SIGNAL_NONE;
	__lost_count = 0;
	__stable_count = 0;
	__vsync_count = 0;
	__wait_type = 0x00;
	__signal_stable = 0;

	ret = wch_hw_init(WCH_VDI);
	if (ret) {
		tvd_err("init wch failed and ret is %d\n", ret);
		return -1;
	}

	detail_mode = tvd_hal_mode_detect();
	if(TVD3D_COMB_ENABLE){
		tvd_hal_3d_setting(detail_mode, g_hw_resource->tdc_dram_addr);
	}else{
		tvd_hal_2d_setting(detail_mode);
	}
	
	tvd_hal_special_setting();
	//check_tvd_dma_addr(); //FIXME

	tvd_info("%s leave\n", __FUNCTION__);
	return TVD_SUCCESS;
}

int tvd_hw_deinit(void)
{
	int ret = -1;

	tvd_info("%s enter\n", __FUNCTION__);
	if (!g_hw_resource) {
		return -1;
	}

	g_tvd_started = false;
	tvd_hal_vsync_irq_enable(0);
	tvd_hal_irq_enable(0);

	ret = wch_hw_deinit(WCH_VDI);
	if (ret) {
		tvd_err("deinit wch failed and ret is %d\n", ret);
		return -1;
	}
#if defined(ATC_ARM2_LK_SUPPORT)
	wch_hw_clear(WCH_VDI);
#endif
	irq_clear_handler(g_hw_resource->tvd_irq);

	worker_flush();

	tvd_hal_deinit();
	g_wch_started = false;

	tvd_info("%s leave\n", __FUNCTION__);
	return TVD_SUCCESS;
}

int tvd_hw_start(void)
{
	tvd_info("%s enter\n", __FUNCTION__);
	if (!g_hw_resource) {
		return -1;
	}

	g_tvd_started = true;
	g_source_info.signal_state = SIGNAL_NONE;
	tvd_hal_irq_enable(1);
	tvd_hal_vsync_irq_enable(1);
	tvd_info("%s leave\n", __FUNCTION__);
	return TVD_SUCCESS;
}

int tvd_hw_stop(void)
{
	tvd_info("%s enter\n", __FUNCTION__);
	if (!g_hw_resource) {
		return -1;
	}

	tvd_hal_vsync_irq_enable(0);
	tvd_hal_irq_enable(0);
	stop_wch();
	g_tvd_started = false;

	tvd_info("%s leave\n", __FUNCTION__);
	return TVD_SUCCESS;
}

#if 0
int tvd_hw_config(TVD_COLOR_PROCESS_PARAMETER_T args)
{
	return TVD_SUCCESS;
}
#endif

#if defined(__KERNEL__)
int tvd_hw_clear(void)
{
	if (!g_hw_resource) {
		return -1;
	}

	tvd_status = 0; //XXX
	wch_hw_clear(WCH_VDI);

	return TVD_SUCCESS;
}
#endif

int tvd_hw_color_process(int color_type, int value)
{
	if (!g_hw_resource) {
		return -1;
	}

	tvd_hal_color_process(color_type, value);
	return TVD_SUCCESS;
}


