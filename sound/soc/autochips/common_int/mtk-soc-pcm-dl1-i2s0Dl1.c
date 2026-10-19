/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *
 * Filename:
 * ---------
 *   mt_soc_pcm_dl1.c
 *
 * Project:
 * --------
 *    Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio Dl1 playback
 *
 * Author:
 * -------
 *   Shiyuan Dong
 *
 *------------------------------------------------------------------------------
 *
 *
 ******************************************************************************
 */

/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/

/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include <linux/dma-mapping.h>
#include <sound/pcm_params.h>

#include "mtk-auddrv-afe.h"
#include "mtk-auddrv-ana.h"
#include "mtk-auddrv-clk.h"
#include "mtk-auddrv-common.h"
#include "mtk-auddrv-def.h"
#include "mtk-auddrv-kernel.h"
#include "mtk-soc-afe-control.h"
#include "mtk-soc-pcm-common.h"
#include "mtk-soc-pcm-platform.h"

static struct afe_mem_control_t *pI2S0dl1MemControl;
static struct snd_dma_buffer Dl1I2S0_Playback_dma_buf;
static unsigned int mPlaybackDramState;

static bool vcore_dvfs_enable;

/*
 *    function implementation
 */
//static int mtk_I2S0dl1_probe(struct platform_device *pdev);
//static int mtk_pcm_I2S0dl1_close(struct snd_pcm_substream *substream);
//static int mtk_afe_I2S0dl1_probe(struct snd_soc_platform *platform);
//static int mtk_afe_I2S0dl1_remove(struct snd_soc_platform *platform);

static bool mPrepareDone;
static struct device *mDev;

static struct snd_pcm_hardware mtk_I2S0dl1_hardware = {
	.info = (SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_NO_PERIOD_WAKEUP |
		 SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_RESUME |
		 SNDRV_PCM_INFO_MMAP_VALID),
	.formats = SND_SOC_ADV_MT_FMTS,
	.rates = SOC_HIGH_USE_RATE,
	.rate_min = SOC_HIGH_USE_RATE_MIN,
	.rate_max = SOC_HIGH_USE_RATE_MAX,
	.channels_min = SOC_NORMAL_USE_CHANNELS_MIN,
	.channels_max = SOC_NORMAL_USE_CHANNELS_MAX,
	.buffer_bytes_max = SOC_HIFI_BUFFER_SIZE,
	.period_bytes_max = SOC_HIFI_BUFFER_SIZE,
	.periods_min = SOC_NORMAL_USE_PERIODS_MIN,
	.periods_max = SOC_NORMAL_USE_PERIODS_MAX,
	.fifo_size = 0,
};


static struct snd_pcm_hw_constraint_list constraints_sample_rates = {
	.count = ARRAY_SIZE(soc_high_supported_sample_rates),
	.list = soc_high_supported_sample_rates,
	/* TODO: KC: need check this!!!!!!!!!! */
	.mask = 0,
};

static int mtk_pcm_I2S0dl1_hw_free(struct snd_pcm_substream *substream)
{
	/* pr_debug("%s substream = %p\n", __func__, substream); */
	if (mPlaybackDramState == true) {
		AudDrv_Emi_Clk_Off();
		mPlaybackDramState = false;
	}

	return 0;
}

static int mtk_pcm_I2S0dl1_close(struct snd_pcm_substream *substream)
{
	pr_info("%s\n", __func__);

	if (is_irq_from_ext_module()) {
		ext_sync_signal_lock();
		ext_sync_signal_unlock();
	}

	if (mPrepareDone == true) {
		if ((DL1_DST_IO & I2S3) == I2S3) {
			SetupI2SConnection(false, Soc_Aud_I2S3, Soc_Aud_AFE_IO_Block_MEM_DL1);
			SetupAndEnableI2S(false, Soc_Aud_I2S3, substream);
		}

		if ((DL1_DST_IO & I2S1) == I2S1) {
			SetupI2SConnection(false, Soc_Aud_I2S1, Soc_Aud_AFE_IO_Block_MEM_DL1);
			SetupAndEnableI2S(false, Soc_Aud_I2S1, substream);
		}

		if ((DL1_DST_IO & PMIC_DAC) == PMIC_DAC) {
			SetupPmicDacConnection(false, Soc_Aud_AFE_IO_Block_MEM_DL1);
			SetupAndEnablePmicDac(false, substream);
		}
		RemoveMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);
		EnableAfe(false);
		mPrepareDone = false;
	}

	AudDrv_Clk_Off();
	vcore_dvfs(&vcore_dvfs_enable, true);

	return 0;
}

static int mtk_pcm_I2S0dl1_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;

	mPlaybackDramState = false;

	pr_info("%s: mtk_I2S0dl1_hardware.buffer_bytes_max = %zu mPlaybackDramState = %d\n",
		__func__, mtk_I2S0dl1_hardware.buffer_bytes_max,
		mPlaybackDramState);
	runtime->hw = mtk_I2S0dl1_hardware;

	AudDrv_Clk_On();

	memcpy((void *)(&(runtime->hw)), (void *)&mtk_I2S0dl1_hardware,
	       sizeof(struct snd_pcm_hardware));
	pI2S0dl1MemControl = Get_Mem_ControlT(Soc_Aud_Digital_Block_MEM_DL1);

	ret = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &constraints_sample_rates);

	if (ret < 0) {
		pr_err("ret < 0 mtk_pcm_I2S0dl1_close\n");
		mtk_pcm_I2S0dl1_close(substream);
		return ret;
	}

	return 0;
}

static int mtk_pcm_I2S0dl1_hw_params(struct snd_pcm_substream *substream,
				     struct snd_pcm_hw_params *hw_params)
{
	int ret = 0;

	substream->runtime->dma_bytes = params_buffer_bytes(hw_params);

	pr_info("%s(), use DRAM\n", __func__);
	substream->runtime->dma_area = Dl1I2S0_Playback_dma_buf.area;
	substream->runtime->dma_addr = Dl1I2S0_Playback_dma_buf.addr;
	SetHighAddr(Soc_Aud_Digital_Block_MEM_DL1, true,
		    substream->runtime->dma_addr);
	mPlaybackDramState = true;
	AudDrv_Emi_Clk_On();

	set_mem_block(substream, hw_params, pI2S0dl1MemControl,
		      Soc_Aud_Digital_Block_MEM_DL1);

	pr_info("dma_bytes = %zu dma_area = %p dma_addr = 0x%lx\n",
		substream->runtime->dma_bytes, substream->runtime->dma_area,
		(long)substream->runtime->dma_addr);

	return ret;
}

static int mtk_pcm_I2S0dl1_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	pr_info("%s: mPrepareDone = %d, format = %d, sample rate = %d\n",
		__func__, mPrepareDone, runtime->format,
		substream->runtime->rate);

	if (mPrepareDone == false) {
		SetMemifSubStream(Soc_Aud_Digital_Block_MEM_DL1, substream);

		if (runtime->format == SNDRV_PCM_FORMAT_S32_LE ||
		    runtime->format == SNDRV_PCM_FORMAT_U32_LE) {
			SetMemIfFetchFormatPerSample(
				Soc_Aud_Digital_Block_MEM_DL1,
				AFE_WLEN_32_BIT_ALIGN_8BIT_0_24BIT_DATA);
		} else {
			SetMemIfFetchFormatPerSample(
				Soc_Aud_Digital_Block_MEM_DL1, AFE_WLEN_16_BIT);
		}

		if ((DL1_DST_IO & I2S3) == I2S3) {
			SetupI2SConnection(true, Soc_Aud_I2S3, Soc_Aud_AFE_IO_Block_MEM_DL1);
			SetupAndEnableI2S(true, Soc_Aud_I2S3, substream);
		}

		if ((DL1_DST_IO & I2S1) == I2S1) {
			SetupI2SConnection(true, Soc_Aud_I2S1, Soc_Aud_AFE_IO_Block_MEM_DL1);
			SetupAndEnableI2S(true, Soc_Aud_I2S1, substream);
		}

		if ((DL1_DST_IO & PMIC_DAC) == PMIC_DAC) {
			SetupPmicDacConnection(true, Soc_Aud_AFE_IO_Block_MEM_DL1);
			SetupAndEnablePmicDac(true, substream);
		}

		EnableAfe(true);
		mPrepareDone = true;
	}
	return 0;
}

static int mtk_pcm_I2S0dl1_stop(struct snd_pcm_substream *substream)
{
	/* struct afe_block_t *Afe_Block = &(pI2S0dl1MemControl->rBlock); */

	pr_info("%s\n", __func__);

	irq_remove_substream_user(
		substream, irq_request_number(Soc_Aud_Digital_Block_MEM_DL1));

	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1, false);

	ClearMemBlock(Soc_Aud_Digital_Block_MEM_DL1);

	return 0;
}

static int mtk_pcm_I2S0dl1_start(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	pr_info("%s\n", __func__);

	/* here to set interrupt */
	irq_add_substream_user(
		substream, irq_request_number(Soc_Aud_Digital_Block_MEM_DL1),
		substream->runtime->rate, substream->runtime->period_size);

	SetSampleRate(Soc_Aud_Digital_Block_MEM_DL1, runtime->rate);
	SetChannels(Soc_Aud_Digital_Block_MEM_DL1, runtime->channels);
	SetMemoryPathEnable(Soc_Aud_Digital_Block_MEM_DL1, true);

	EnableAfe(true);

	return 0;
}

static int mtk_pcm_I2S0dl1_trigger(struct snd_pcm_substream *substream, int cmd)
{

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
		return mtk_pcm_I2S0dl1_start(substream);
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		return mtk_pcm_I2S0dl1_stop(substream);
	}
	return -EINVAL;
}

static snd_pcm_uframes_t
mtk_pcm_I2S0dl1_pointer(struct snd_pcm_substream *substream)
{
	return get_mem_frame_index(substream, pI2S0dl1MemControl,
				   Soc_Aud_Digital_Block_MEM_DL1);
}

static int mtk_pcm_I2S0dl1_copy(struct snd_pcm_substream *substream,
				int channel, snd_pcm_uframes_t pos,
				void __user *dst, snd_pcm_uframes_t count)
{
	vcore_dvfs(&vcore_dvfs_enable, false);
	return mtk_memblk_copy(substream, channel, pos, dst, count,
			       pI2S0dl1MemControl,
			       Soc_Aud_Digital_Block_MEM_DL1);
}

static int mtk_pcm_I2S0dl1_silence(struct snd_pcm_substream *substream,
				   int channel, snd_pcm_uframes_t pos,
				   snd_pcm_uframes_t count)
{
	return 0; /* do nothing */
}

static void *dummy_page[2];

static struct page *mtk_I2S0dl1_pcm_page(struct snd_pcm_substream *substream,
					 unsigned long offset)
{
	return virt_to_page(dummy_page[substream->stream]); /* the same page */
}

static struct snd_pcm_ops mtk_I2S0dl1_ops = {
	.open = mtk_pcm_I2S0dl1_open,
	.close = mtk_pcm_I2S0dl1_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = mtk_pcm_I2S0dl1_hw_params,
	.hw_free = mtk_pcm_I2S0dl1_hw_free,
	.prepare = mtk_pcm_I2S0dl1_prepare,
	.trigger = mtk_pcm_I2S0dl1_trigger,
	.pointer = mtk_pcm_I2S0dl1_pointer,
	.copy = mtk_pcm_I2S0dl1_copy,
	.silence = mtk_pcm_I2S0dl1_silence,
	.page = mtk_I2S0dl1_pcm_page,
	.mmap = mtk_pcm_mmap,
};

static int mtk_afe_I2S0dl1_probe(struct snd_soc_platform *platform)
{
	pr_info("mtk_afe_I2S0dl1_probe\n");
	/* allocate dram */
	Dl1I2S0_Playback_dma_buf.area = dma_alloc_coherent(
		platform->dev, SOC_HIFI_BUFFER_SIZE,
		&Dl1I2S0_Playback_dma_buf.addr, GFP_KERNEL | GFP_DMA);
	if (!Dl1I2S0_Playback_dma_buf.area)
		return -ENOMEM;

	Dl1I2S0_Playback_dma_buf.bytes = SOC_HIFI_BUFFER_SIZE;
	pr_info("area = %p\n", Dl1I2S0_Playback_dma_buf.area);

	EnableI2SClock(Soc_Aud_I2S0);
	EnableI2SClock(Soc_Aud_I2S1);
	EnableI2SClock(Soc_Aud_I2S2);
	EnableI2SClock(Soc_Aud_I2S3);

	return 0;
}

static int mtk_afe_I2S0dl1_remove(struct snd_soc_platform *platform)
{
	pr_info("mtk_afe_I2S0dl1_remove\n");

	DisableI2SClock(Soc_Aud_I2S0);
	DisableI2SClock(Soc_Aud_I2S1);
	DisableI2SClock(Soc_Aud_I2S2);
	DisableI2SClock(Soc_Aud_I2S3);

	return 0;
}

static struct snd_soc_platform_driver mtk_I2S0dl1_soc_platform = {
	.ops = &mtk_I2S0dl1_ops,
	.probe = mtk_afe_I2S0dl1_probe,
	.remove = mtk_afe_I2S0dl1_remove,
};

/* supend and resume function */
static int mtk_I2S0dl1_suspend(struct device *device)
{
	pr_warn("%s\n", __func__);

	DisableI2SClock(Soc_Aud_I2S0);
	DisableI2SClock(Soc_Aud_I2S1);
	DisableI2SClock(Soc_Aud_I2S2);
	DisableI2SClock(Soc_Aud_I2S3);
	return 0;
}

static int mtk_I2S0dl1_resume(struct device *device)
{
	pr_warn("%s", __func__);

	EnableI2SClock(Soc_Aud_I2S0);
	EnableI2SClock(Soc_Aud_I2S1);
	EnableI2SClock(Soc_Aud_I2S2);
	EnableI2SClock(Soc_Aud_I2S3);
	return 0;
}

const struct dev_pm_ops mtk_I2S0dl1_pm_ops = {
	.suspend = mtk_I2S0dl1_suspend,
	.resume = mtk_I2S0dl1_resume,
	.freeze = NULL,
	.thaw = NULL,
	.poweroff = NULL,
	.restore = NULL,
	.restore_noirq = NULL,
};

static int mtk_I2S0dl1_probe(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);

	pdev->dev.coherent_dma_mask = DMA_BIT_MASK(64);
	if (!pdev->dev.dma_mask)
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;

	if (pdev->dev.of_node)
		dev_set_name(&pdev->dev, "%s", MT_SOC_I2S0DL1_PCM);

	pr_debug("%s: dev name %s\n", __func__, dev_name(&pdev->dev));

	mDev = &pdev->dev;

	return snd_soc_register_platform(&pdev->dev, &mtk_I2S0dl1_soc_platform);
}

static int mtk_I2S0dl1_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_soc_pcm_dl1_i2s0Dl1_of_ids[] = {
	{
		.compatible = "mediatek,mt_soc_pcm_dl1_i2s0dl1",
	},
	{} };
#endif

static struct platform_driver mtk_I2S0dl1_driver = {
	.driver = {

			.name = MT_SOC_I2S0DL1_PCM,
			.owner = THIS_MODULE,
#ifdef CONFIG_OF
			.of_match_table = mt_soc_pcm_dl1_i2s0Dl1_of_ids,
#endif
			.pm = &mtk_I2S0dl1_pm_ops,
		},
	.probe = mtk_I2S0dl1_probe,
	.remove = mtk_I2S0dl1_remove,
};

#ifndef CONFIG_OF
static struct platform_device *soc_mtkI2S0dl1_dev;
#endif

static int __init mtk_I2S0dl1_soc_platform_init(void)
{
	int ret;

	pr_debug("%s\n", __func__);
#ifndef CONFIG_OF
	soc_mtkI2S0dl1_dev = platform_device_alloc(MT_SOC_I2S0DL1_PCM, -1);
	if (!soc_mtkI2S0dl1_dev)
		return -ENOMEM;

	ret = platform_device_add(soc_mtkI2S0dl1_dev);
	if (ret != 0) {
		platform_device_put(soc_mtkI2S0dl1_dev);
		return ret;
	}
#endif

	ret = platform_driver_register(&mtk_I2S0dl1_driver);

	return ret;
}
module_init(mtk_I2S0dl1_soc_platform_init);

static void __exit mtk_I2S0dl1_soc_platform_exit(void)
{
	platform_driver_unregister(&mtk_I2S0dl1_driver);
}
module_exit(mtk_I2S0dl1_soc_platform_exit);

MODULE_DESCRIPTION("AFE PCM module platform driver");
MODULE_LICENSE("GPL");
