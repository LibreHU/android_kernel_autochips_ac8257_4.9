/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/setup.h>

#if defined(CONFIG_MTK_PMIC) || defined(CONFIG_MTK_PMIC_NEW_ARCH)
#include <mt-plat/upmu_common.h>
#endif

#include "include/pmic_regulator.h"
#include "include/regulator_codegen.h"

#include <mtk_spm_internal.h>
#include <mtk_spm_suspend_internal.h>

#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
#include <sspm_define.h>
#include <sspm_timesync.h>
#endif

#include <mtk_power_gs_api.h>
#include <mtk_sspm.h>

#define WORLD_CLK_CNTCV_L        (0x10017008)
#define WORLD_CLK_CNTCV_H        (0x1001700C)

#ifdef CONFIG_FPGA_EARLY_PORTING
__attribute__ ((weak))
unsigned int pmic_read_interface_nolock(unsigned int RegNum, unsigned int *val,
	unsigned int MASK, unsigned int SHIFT)
{
	pr_info("NO %s !!!\n", __func__);
	return 0;
}

__attribute__ ((weak))
unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val,
	unsigned int MASK, unsigned int SHIFT)
{
	pr_info("NO %s !!!\n", __func__);
	return 0;
}
__attribute__ ((weak))
unsigned int pmic_config_interface_nolock(unsigned int RegNum, unsigned int val,
	unsigned int MASK, unsigned int SHIFT)
{
	pr_info("NO %s !!!\n", __func__);
	return 0;
}
#endif /* CONFIG_FPGA_EARLY_PORTING */

void spm_dump_world_clk_cntcv(void)
{
	u32 wlk_cntcv_l;
	u32 wlk_cntcv_h;

	/* SYS_TIMER counter value low and high */
	wlk_cntcv_l = _golden_read_reg(WORLD_CLK_CNTCV_L);
	wlk_cntcv_h = _golden_read_reg(WORLD_CLK_CNTCV_H);

	pr_info("[SPM] wlk_cntcv_l = 0x%x, wlk_cntcv_h = 0x%x\n",
		wlk_cntcv_l, wlk_cntcv_h);
}

void spm_set_sysclk_settle(void)
{
	u32 settle;

	/* SYSCLK settle = MD SYSCLK settle but set it again for MD PDN */
	settle = spm_read(SPM_CLK_SETTLE);

	/* md_settle is keyword for suspend status */
	aee_sram_printk("md_settle = %u, settle = %u\n",
		SPM_SYSCLK_SETTLE, settle);
	pr_info("[SPM] md_settle = %u, settle = %u\n",
		SPM_SYSCLK_SETTLE, settle);
}

#if SPM_PMIC_DEBUG
static void spm_dump_pmic_reg(void)
{
	unsigned int pmic_reg[] = {PMIC_RG_BUCK_VCORE_VOSEL_SLEEP_ADDR};
	unsigned int val = 0;
	unsigned int ret = 0;
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(pmic_reg); i++) {
		ret = pmic_read_interface_nolock(pmic_reg[i], &val, 0xffff, 0);
		aee_sram_printk("#@# %s(%d) pmic reg(0x%x) = 0x%x\n",
			__func__, __LINE__, pmic_reg[i], val);
		pr_info("[SPM] #@# %s(%d) pmic reg(0x%x) = 0x%x\n",
			__func__, __LINE__, pmic_reg[i], val);
	}
}

static void spm_dump_ldo_status(void)
{
	int i, j;
	int en = 0, ret = 0;
	int voltage_reg = 0;
	int voltage = 0;
	const int *pVoltage, *pVoltidx;

	pr_info("[SPM]********** BUCK/LDO status dump [1:ON,0:OFF]**********\n");

	for (i = 0; i < mt_bucks_size; i++) {
		if (mt_bucks[i].da_en_cb != NULL)
			en = (mt_bucks[i].da_en_cb)();
		else
			en = -1;

		if (mt_bucks[i].da_vol_cb != NULL) {
			voltage_reg = (mt_bucks[i].da_vol_cb)();
			voltage = mt_bucks[i].desc.min_uV +
				mt_bucks[i].desc.uV_step *
				voltage_reg;
		} else {
			voltage_reg = -1;
			voltage = -1;
		}
		pr_info("[SPM]%s   status:%d     voltage:%duv    voltage_reg:%d\n"
			   , mt_bucks[i].desc.name
			   , en, voltage, voltage_reg);
	}

	/*for (i = 0; i < ARRAY_SIZE(mt_ldos); i++) {*/
	for (i = 0; i < mt_ldos_size; i++) {
		if (mt_ldos[i].da_en_cb != NULL)
			en = (mt_ldos[i].da_en_cb)();
		else
			en = -1;

		if (mt_ldos[i].desc.n_voltages != 1) {
			if (mt_ldos[i].da_vol_cb != NULL) {
				voltage_reg = (mt_ldos[i].da_vol_cb)();
				if (mt_ldos[i].pvoltages != NULL) {
					pVoltage = (const int *)
						mt_ldos[i].pvoltages;
					pVoltidx = (const int *)
						mt_ldos[i].idxs;
				/*HW LDO sequence issue, we need to change it */
				for (j = 0;
				     j < mt_ldos[i].desc.n_voltages; j++) {
					if (pVoltidx[j] == voltage_reg) {
						ret = j;
						break;
					}
				}
					voltage = pVoltage[ret];
				} else {
					voltage =
					    mt_ldos[i].desc.min_uV +
					    mt_ldos[i].desc.uV_step *
					    voltage_reg;
				}
			} else {
				voltage_reg = -1;
				voltage = -1;
			}
		} else {
			voltage = mt_ldos[i].desc.fixed_uV;
			voltage_reg = -1;
		}

		pr_info("[SPM]%s   status:%d     voltage:%duv    voltage_reg:%d\n"
			   , mt_ldos[i].desc.name, en, voltage, voltage_reg);
	}
}
#endif /* SPM_PMIC_DEBUG */

#if !defined(CONFIG_FPGA_EARLY_PORTING)
static int mt_power_gs_dump_suspend_count = 2;
#endif
void spm_suspend_pre_process(struct pwr_ctrl *pwrctrl)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	int ret;
	struct spm_data spm_d;
	unsigned int spm_opt = 0;

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef CONFIG_MTK_TIMER_TIMESYNC
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h,
		&spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h,
		&spm_d.u.suspend.sys_src_clk_l);
#endif
	spm_d.u.suspend.spm_opt = spm_opt;

	ret = spm_to_sspm_command(SPM_SUSPEND, &spm_d);
	if (ret < 0) {
		aee_sram_printk("ret %d", ret);
		pr_info("[SPM] ret %d", ret);
	}
#endif
#if SPM_PMIC_DEBUG
	spm_dump_pmic_reg();
#endif /* SPM_PMIC_DEBUG */

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (slp_dump_golden_setting || --mt_power_gs_dump_suspend_count >= 0)
		mt_power_gs_dump_suspend(slp_dump_golden_setting_type);
#endif

	/* dvfsrc_md_scenario_update(1); */

#if SPM_PMIC_DEBUG
	spm_dump_pmic_reg();
	spm_dump_ldo_status();
#endif /* SPM_PMIC_DEBUG */
}

void spm_suspend_post_process(struct pwr_ctrl *pwrctrl)
{
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	int ret;
	struct spm_data spm_d;

	/* dvfsrc_md_scenario_update(0); */

#if SPM_PMIC_DEBUG
	spm_dump_pmic_reg();
#endif /* SPM_PMIC_DEBUG */

	memset(&spm_d, 0, sizeof(struct spm_data));

#ifdef CONFIG_MTK_TIMER_TIMESYNC
	sspm_timesync_ts_get(&spm_d.u.suspend.sys_timestamp_h,
		&spm_d.u.suspend.sys_timestamp_l);
	sspm_timesync_clk_get(&spm_d.u.suspend.sys_src_clk_h,
		&spm_d.u.suspend.sys_src_clk_l);
#endif
	ret = spm_to_sspm_command(SPM_RESUME, &spm_d);
	if (ret < 0) {
		aee_sram_printk("ret %d", ret);
		pr_info("[SPM] ret %d", ret);
	}
#else
	/* dvfsrc_md_scenario_update(0); */

#if SPM_PMIC_DEBUG
	spm_dump_pmic_reg();
#endif /* SPM_PMIC_DEBUG */

#if SPM_PMIC_EN
	/* VCORE 0.6V */
	pmic_config_interface(PMIC_RG_BUCK_VCORE_VOSEL_SLEEP_ADDR, 0x10,
			PMIC_RG_BUCK_VCORE_VOSEL_SLEEP_MASK,
			PMIC_RG_BUCK_VCORE_VOSEL_SLEEP_SHIFT);

#endif /* SPM_PMIC_EN */

#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
}

