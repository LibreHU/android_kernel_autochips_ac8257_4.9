#ifndef TVD_HAL_H
#define TVD_HAL_H

typedef struct {
    void* tvd_base;
    void* vdisys_base;
    void* cvbs_base;
    void* clk_base;
    void* vdisys_dram_bridge_base;
} TVD_REG_BASE;

typedef enum {
	TVD_MODE_PAL_N = 0,
	TVD_MODE_PAL,
	TVD_MODE_PAL_M,
	TVD_MODE_NTSC,
	TVD_MODE_SECAM,
	TVD_MODE_PAL_60,
	TVD_MODE_UNSTABLE,
	TVD_MODE_NTSC443,
	TVD_MODE_NONE,
} TVD_MODE_E;

typedef enum {
    TVD_INTR_STA_VPRES = (1<<0),
    TVD_INTR_STA_MODE = (1<<1),
    TVD_INTR_STA_VSYNC = (1<<9),
} TVD_INTR_STA_E;

typedef enum {
	TVD_COLOR_TYPE_Y_GAIN,
	TVD_COLOR_TYPE_U_COSGAIN,
	TVD_COLOR_TYPE_U_SINGAIN,
	TVD_COLOR_TYPE_V_COSGAIN,
	TVD_COLOR_TYPE_V_SINGAIN,
	TVD_COLOR_TYPE_Y_OFFSET,
	TVD_COLOR_TYPE_U_OFFSET,
	TVD_COLOR_TYPE_V_OFFSET,
} TVD_COLOR_TYPE_E;

void tvd_hal_set_reg_base(const TVD_REG_BASE* base);
void tvd_hal_init(void);
void tvd_hal_deinit(void);

void tvd_hal_irq_enable(int enable);
void tvd_hal_vsync_irq_enable(int enable);
int  tvd_hal_irq_status(void);
void tvd_hal_irq_clear(void);
void tvd_hal_set_img_status(int enable);

int  tvd_hal_vpres_on(void);
int  tvd_hal_sync_locked(int sel_ctg);
int  tvd_hal_mode_detect(void);

void tvd_hal_2d_setting(int mode);
void tvd_hal_3d_setting(int mode, unsigned long tdc_dram_addr);
void tvd_hal_special_setting(void);
void tvd_hal_color_process(int color_type, int value);

#endif
