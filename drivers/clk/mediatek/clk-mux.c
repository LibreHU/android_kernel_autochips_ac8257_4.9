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


#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>

#include "clk-mtk.h"
#include "clk-mux.h"

#define WORKAROUND_318_WARNING	1

struct mtk_mux_upd_data {
	struct clk_hw hw;
	void __iomem *base;

	u32 mux_ofs;
	u32 upd_ofs;

	s8 mux_shift;
	s8 mux_width;
	s8 gate_shift;
	s8 upd_shift;

	spinlock_t *lock;
};
struct mtk_mux_clr_set_upd_data {
	struct clk_hw hw;
	void __iomem *base;

	u32 mux_ofs;
	u32 mux_set_ofs;
	u32 mux_clr_ofs;
	u32 upd_ofs;

	s8 mux_shift;
	s8 mux_width;
	s8 gate_shift;
	s8 upd_shift;

	spinlock_t *lock;
};

static inline struct mtk_mux_upd_data *to_mtk_mux_upd_data(struct clk_hw *hw)
{
	return container_of(hw, struct mtk_mux_upd_data, hw);
}

static inline struct mtk_mux_clr_set_upd_data
	*to_mtk_mux_clr_set_upd_data(struct clk_hw *hw)
{
	return container_of(hw, struct mtk_mux_clr_set_upd_data, hw);
}

static int mtk_mux_upd_enable(struct clk_hw *hw)
{
	struct mtk_mux_upd_data *mux = to_mtk_mux_upd_data(hw);
	u32 val, orig;
	unsigned long flags = 0;

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);

	val = clk_readl(mux->base + mux->mux_ofs);
	orig = val;

#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	/*add for CLK_CFG_8*/
	if (mux->mux_ofs == 0xC0) {
		val |= BIT(mux->gate_shift);
	} else {
#endif
		val &= ~BIT(mux->gate_shift);
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	}
#endif

	if (val != orig) {
		clk_writel(val, mux->base + mux->mux_ofs);

		if (mux->upd_shift >= 0)
			clk_writel(BIT(mux->upd_shift),
			mux->base + mux->upd_ofs);
	}

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);

	return 0;
}

static int mtk_mux_clr_set_upd_enable(struct clk_hw *hw)
{
	struct mtk_mux_clr_set_upd_data *mux = to_mtk_mux_clr_set_upd_data(hw);
	u32 val, orig;
	unsigned long flags = 0;

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);

	val = clk_readl(mux->base + mux->mux_ofs);
	orig = val;
	val &= ~BIT(mux->gate_shift);

	if (val != orig) {
		/*clk_writel(val, mux->base + mux->mux_ofs);*/
		clk_writel(BIT(mux->gate_shift), mux->base + mux->mux_clr_ofs);
		#if 0
		if (mux->upd_shift >= 0)
			clk_writel(BIT(mux->upd_shift),
			mux->base + mux->upd_ofs);
		#endif
	}

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);

	return 0;
}

static void mtk_mux_upd_disable(struct clk_hw *hw)
{
	struct mtk_mux_upd_data *mux = to_mtk_mux_upd_data(hw);
	u32 val, orig;
	unsigned long flags = 0;

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);

	val = clk_readl(mux->base + mux->mux_ofs);
	orig = val;
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	/*add for CLK_CFG_8*/
	if (mux->mux_ofs == 0xC0) {
		val &= ~BIT(mux->gate_shift);
	} else {
#endif
		val |= BIT(mux->gate_shift);
#ifdef CONFIG_ATC_AOSP_ENHANCEMENT
	}
#endif

	if (val != orig) {
		clk_writel(val, mux->base + mux->mux_ofs);

		if (mux->upd_shift >= 0)
			clk_writel(BIT(mux->upd_shift),
			mux->base + mux->upd_ofs);
	}

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);
}

static void mtk_mux_clr_set_upd_disable(struct clk_hw *hw)
{
	struct mtk_mux_clr_set_upd_data *mux = to_mtk_mux_clr_set_upd_data(hw);
	u32 val, orig;
	unsigned long flags = 0;

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);

	val = clk_readl(mux->base + mux->mux_ofs);
	orig = val;
	val |= BIT(mux->gate_shift);

	if (val != orig) {
		/*clk_writel(val, mux->base + mux->mux_ofs);*/
		clk_writel(BIT(mux->gate_shift), mux->base + mux->mux_set_ofs);
		#if 0
		if (mux->upd_shift >= 0)
			clk_writel(BIT(mux->upd_shift),
			mux->base + mux->upd_ofs);
		#endif
	}

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);
}

static int mtk_mux_upd_is_enabled(struct clk_hw *hw)
{
	struct mtk_mux_upd_data *mux = to_mtk_mux_upd_data(hw);

	if (mux->gate_shift < 0)
		return true;

	return (clk_readl(mux->base + mux->mux_ofs) &
		BIT(mux->gate_shift)) == 0;
}

static int mtk_mux_clr_set_upd_is_enabled(struct clk_hw *hw)
{
	struct mtk_mux_clr_set_upd_data *mux = to_mtk_mux_clr_set_upd_data(hw);

	if (mux->gate_shift < 0)
		return true;

	return (clk_readl(mux->base + mux->mux_ofs) &
		BIT(mux->gate_shift)) == 0;
}

static u8 mtk_mux_upd_get_parent(struct clk_hw *hw)
{
	struct mtk_mux_upd_data *mux = to_mtk_mux_upd_data(hw);
	int num_parents = clk_hw_get_num_parents(hw);
	u32 mask = GENMASK(mux->mux_width - 1, 0);
	u32 val;

	val = clk_readl(mux->base + mux->mux_ofs) >> mux->mux_shift;
	val &= mask;

	if (val >= num_parents)
		return -EINVAL;

	return val;
}

static u8 mtk_mux_clr_set_upd_get_parent(struct clk_hw *hw)
{
	struct mtk_mux_clr_set_upd_data *mux = to_mtk_mux_clr_set_upd_data(hw);
	int num_parents = clk_hw_get_num_parents(hw);
	u32 mask = GENMASK(mux->mux_width - 1, 0);
	u32 val;

	val = clk_readl(mux->base + mux->mux_ofs) >> mux->mux_shift;
	val &= mask;

	if (val >= num_parents)
		return -EINVAL;

	return val;
}

static int mtk_mux_upd_set_parent(struct clk_hw *hw, u8 index)
{
	struct mtk_mux_upd_data *mux = to_mtk_mux_upd_data(hw);
	u32 mask = GENMASK(mux->mux_width - 1, 0);
	u32 val, orig;
	unsigned long flags = 0;

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);

	val = clk_readl(mux->base + mux->mux_ofs);
	orig = val;
	val &= ~(mask << mux->mux_shift);

	val |= index << mux->mux_shift;

	if (val != orig) {
		clk_writel(val, mux->base + mux->mux_ofs);

		if (mux->upd_shift >= 0)
			clk_writel(BIT(mux->upd_shift),
			mux->base + mux->upd_ofs);
	}

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);

	return 0;
}

static int mtk_mux_clr_set_upd_set_parent(struct clk_hw *hw, u8 index)
{
	struct mtk_mux_clr_set_upd_data *mux = to_mtk_mux_clr_set_upd_data(hw);
	u32 mask = GENMASK(mux->mux_width - 1, 0);
	u32 val, orig;
	unsigned long flags = 0;

	if (mux->lock)
		spin_lock_irqsave(mux->lock, flags);

	val = clk_readl(mux->base + mux->mux_ofs);
	orig = val;
	val &= ~(mask << mux->mux_shift);
	val |= index << mux->mux_shift;

	if (val != orig) {
		val = (mask << mux->mux_shift);
		writel(val, mux->base + mux->mux_clr_ofs);

		val = (index << mux->mux_shift);
		writel(val, mux->base + mux->mux_set_ofs);
		if (mux->upd_shift >= 0)
			clk_writel(BIT(mux->upd_shift),
			mux->base + mux->upd_ofs);
	}

	if (mux->lock)
		spin_unlock_irqrestore(mux->lock, flags);

	return 0;
}

const struct clk_ops mtk_mux_upd_ops = {
	.is_enabled = mtk_mux_upd_is_enabled,
	.get_parent = mtk_mux_upd_get_parent,
	.set_parent = mtk_mux_upd_set_parent,
};

const struct clk_ops mtk_mux_clr_set_upd_ops = {
	.is_enabled = mtk_mux_clr_set_upd_is_enabled,
	.get_parent = mtk_mux_clr_set_upd_get_parent,
	.set_parent = mtk_mux_clr_set_upd_set_parent,
};

const struct clk_ops mtk_mux_upd_gate_ops = {
	.enable = mtk_mux_upd_enable,
	.disable = mtk_mux_upd_disable,
	.is_enabled = mtk_mux_upd_is_enabled,
	.get_parent = mtk_mux_upd_get_parent,
	.set_parent = mtk_mux_upd_set_parent,
};

const struct clk_ops mtk_mux_clr_set_upd_gate_ops = {
	.enable = mtk_mux_clr_set_upd_enable,
	.disable = mtk_mux_clr_set_upd_disable,
	.is_enabled = mtk_mux_clr_set_upd_is_enabled,
	.get_parent = mtk_mux_clr_set_upd_get_parent,
	.set_parent = mtk_mux_clr_set_upd_set_parent,
};

struct clk * __init mtk_clk_register_mux_upd(const struct mtk_mux_upd *mu,
		void __iomem *base, spinlock_t *lock)
{
	struct clk *clk;
	struct mtk_mux_upd_data *mux;
	struct clk_init_data init;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return ERR_PTR(-ENOMEM);

	init.name = mu->name;
	init.flags = CLK_SET_RATE_PARENT;
#if WORKAROUND_318_WARNING
	init.parent_names = (const char **)mu->parent_names;
#else
	init.parent_names = mu->parent_names;
#endif
	init.num_parents = mu->num_parents;

	if (mu->gate_shift < 0)
		init.ops = &mtk_mux_upd_ops;
	else
		init.ops = &mtk_mux_upd_gate_ops;
	mux->base = base;
	mux->mux_ofs = mu->mux_ofs;
	mux->upd_ofs = mu->upd_ofs;
	mux->mux_shift = mu->mux_shift;
	mux->mux_width = mu->mux_width;
	mux->gate_shift = mu->gate_shift;
	mux->upd_shift = mu->upd_shift;
	mux->lock = lock;

	mux->hw.init = &init;

	clk = clk_register(NULL, &mux->hw);
	if (IS_ERR(clk))
		kfree(mux);

	return clk;
}

void __init mtk_clk_register_mux_upds(const struct mtk_mux_upd *mus,
		int num, void __iomem *base, spinlock_t *lock,
		struct clk_onecell_data *clk_data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < num; i++) {
		const struct mtk_mux_upd *mu = &mus[i];

		clk = mtk_clk_register_mux_upd(mu, base, lock);

		if (IS_ERR(clk)) {
			pr_notice("Failed to register clk %s: %ld\n",
					mu->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[mu->id] = clk;
	}
}

struct clk * __init mtk_clk_register_mux_clr_set_upd(
		const struct mtk_mux_clr_set_upd *mu,
		void __iomem *base, spinlock_t *lock)
{
	struct clk *clk;
	struct mtk_mux_clr_set_upd_data *mux;
	struct clk_init_data init;

	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		return ERR_PTR(-ENOMEM);

	init.name = mu->name;
	init.flags = (mu->flags) | CLK_SET_RATE_PARENT;
#if WORKAROUND_318_WARNING
	init.parent_names = (const char **)mu->parent_names;
#else
	init.parent_names = mu->parent_names;
#endif
	init.num_parents = mu->num_parents;

	if (mu->gate_shift < 0)
		init.ops = &mtk_mux_clr_set_upd_ops;
	else
		init.ops = &mtk_mux_clr_set_upd_gate_ops;

	mux->base = base;
	mux->mux_ofs = mu->mux_ofs;
	mux->mux_set_ofs = mu->mux_set_ofs;
	mux->mux_clr_ofs = mu->mux_clr_ofs;
	mux->upd_ofs = mu->upd_ofs;
	mux->mux_shift = mu->mux_shift;
	mux->mux_width = mu->mux_width;
	mux->gate_shift = mu->gate_shift;
	mux->upd_shift = mu->upd_shift;
	mux->lock = lock;

	mux->hw.init = &init;

	clk = clk_register(NULL, &mux->hw);
	if (IS_ERR(clk))
		kfree(mux);

	return clk;
}

void __init mtk_clk_register_mux_clr_set_upds(
		const struct mtk_mux_clr_set_upd *mus,
		int num, void __iomem *base, spinlock_t *lock,
		struct clk_onecell_data *clk_data)
{
	struct clk *clk;
	int i;

	for (i = 0; i < num; i++) {
		const struct mtk_mux_clr_set_upd *mu = &mus[i];

		clk = mtk_clk_register_mux_clr_set_upd(mu, base, lock);

		if (IS_ERR(clk)) {
			pr_notice("Failed to register clk %s: %ld\n",
					mu->name, PTR_ERR(clk));
			continue;
		}

		if (clk_data)
			clk_data->clks[mu->id] = clk;
	}
}
