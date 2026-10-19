/*
 * tda7729.c -- TDA7729 ALSA SoC audio driver
 *
 * Copyright 2016 autochips.
 *
 * Author: Quihua yin <qiuhua.yin@autochips.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include "tda7729.h"


#define TDA_DRV_VER "TDA7729 v1.2"

static DEFINE_MUTEX(tda_Ctrl_Mutex);

struct tda7729_private {
    struct device *dev;
    struct i2c_client *client;
};
struct tda7729_private *tda7729;

u8 audio_i2c_buf[REG_MAX]= {
    0xfa,   //REG0
    0xfa,   //REG1
    0xff,   //REG2
    0xff,   //REG3
    0xf9,   //REG4
    0xff,   //REG5
    0x0f,   //REG6
    0x40,   //REG7
    0x59,   //REG8
    0x0f,   //REG9
    0x0f,   //REG10
    0x0f,   //REG11
    0x84,   //REG12
    0x00,   //REG13
    0x00,   //REG14
    0x00,   //REG15
    0x00,   //REG16
    0x00,   //REG17
    0x00    //REG18
};

#define mode_write(addr,val)                audio_i2c_buf[addr] = val
#define mode_read(addr)                     audio_i2c_buf[addr]
#define mode_mask(start,bitnum)             (((1 << (bitnum)) - 1) << start)
#define mode_bits_read(addr,start,bitnum)                   \
    ((mode_read(addr) & mode_mask(start, bitnum)) >> start)
#define mode_bits_write(addr, start, bitnum, val)           \
    mode_write(addr, (mode_read(addr) & ~(mode_mask(start, bitnum))) | ((val) << start))

static struct proc_dir_entry *tda7729_debug_proc_entry;
static s32 tda7729_read(u8 addr, u8 *data);
static s32 tda7729_write(u8 addr, u8 data);

#define max_min(num, min, max)  ((num < min) ? min : ((num > max) ? max : num))

#define TDA7729_MODE_SWITCH(xname, addr, start, bitnum, xmax) \
    SOC_SINGLE_EXT(xname, addr, ((start << 8)| bitnum), xmax, 0, \
        tda7729_mode_get, tda7729_mode_set)

static int tda7729_mode_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    u8 addr,start,bitnum,max;
    struct soc_mixer_control *mc;

    mutex_lock(&tda_Ctrl_Mutex);

    mc = (struct soc_mixer_control *)kcontrol->private_value;

    addr = mc->reg;
    start = (mc->shift >> 8) & 0xf;
    bitnum = mc->shift & 0xf;
    max = mc->max;

    ucontrol->value.integer.value[0] = mode_bits_read(addr, start, bitnum);

    pr_info("<tda7729_mode_get> -> reg: 0x%x(%d:%d) \n", addr, start, (start + bitnum - 1));

    mutex_unlock(&tda_Ctrl_Mutex);

    return 0;
}

static int tda7729_mode_set(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
    u8 value,addr,start,bitnum,max;
    struct soc_mixer_control *mc;

    mutex_lock(&tda_Ctrl_Mutex);
    mc = (struct soc_mixer_control *)kcontrol->private_value;
    addr = mc->reg;
    start = (mc->shift >> 8) & 0xf;
    bitnum = mc->shift & 0xf;
    max = mc->max;

    value = (u8)(ucontrol->value.integer.value[0] & 0xff);
    value = max_min(value, 0, max);

    pr_info("<tda7729_mode_set> -> reg: 0x%x(%d:%d) ,Val : 0x%x\n", addr, start, (start + bitnum - 1), value);
    mode_bits_write(addr, start, bitnum, value);
    tda7729_write(addr, mode_read(addr));
    mutex_unlock(&tda_Ctrl_Mutex);

    return 0;
}

static const struct snd_kcontrol_new tda7729_mode_controls[] = {
    TDA7729_MODE_SWITCH("TDA7729MainIn_Src_Switch",0,0,3,7),     //AUDIO_AREA_M_SRC
    TDA7729_MODE_SWITCH("TDA7729MainMd_Cfg_Switch",0,3,1,1),     //AUDIO_AREA_M_MD_SEL
    TDA7729_MODE_SWITCH("TDA7729MainIn_Gain_Switch",0,4,1,1),    //AUDIO_AREA_M_IGAIN
    TDA7729_MODE_SWITCH("TDA7729HwIn_Cfg_Switch",0,5,3,7),       //AUDIO_AREA_ICFG
    TDA7729_MODE_SWITCH("TDA7729SndIn_Src_Switch",1,0,3,7),      //AUDIO_AREA_SND_SRC
    TDA7729_MODE_SWITCH("TDA7729SndMd_Cfg_Switch",1,3,1,1),      //AUDIO_AREA_SND_MD_SEL
    TDA7729_MODE_SWITCH("TDA7729SndIn_Gain_Switch",1,4,1,1),     //AUDIO_AREA_SND_IGAIN
    TDA7729_MODE_SWITCH("TDA7729Qd2_Bypass_Switch",1,5,1,1),     //AUDIO_AREA_QD2_BYPASS
    TDA7729_MODE_SWITCH("TDA7729Qd3_Bypass_Switch",1,6,1,1),     //AUDIO_AREA_QD3_BYPASS
    TDA7729_MODE_SWITCH("TDA7729Qd4_Bypass_Switch",1,7,1,1),     //AUDIO_AREA_QD4_BYPASS
    TDA7729_MODE_SWITCH("TDA7729Mix_Src_Switch",2,0,3,7),        //AUDIO_AREA_MIX_SRC
    TDA7729_MODE_SWITCH("TDA7729Mix_Atten_Switch",2,3,5,31),     //AUDIO_AREA_MIX_ATTEN
    TDA7729_MODE_SWITCH("TDA7729MixTo_Fl_Switch",3,0,1,1),       //AUDIO_AREA_MIX_FL
    TDA7729_MODE_SWITCH("TDA7729MixTo_Fr_Switch",3,1,1,1),       //AUDIO_AREA_MIX_FR
    TDA7729_MODE_SWITCH("TDA7729MixTo_Rl_Switch",3,2,1,1),       //AUDIO_AREA_MIX_RL
    TDA7729_MODE_SWITCH("TDA7729MixTo_Rr_Switch",3,3,1,1),       //AUDIO_AREA_MIX_RR
    TDA7729_MODE_SWITCH("TDA7729Rear_Src_Switch",3,4,1,1),       //AUDIO_AREA_REAR_SRC
    TDA7729_MODE_SWITCH("TDA7729Level_Cfg_Switch",3,6,1,1),      //AUDIO_AREA_LEVEL_CFG
    TDA7729_MODE_SWITCH("TDA7729Level_En_Switch",3,7,1,1),       //AUDIO_AREA_LEVEL_EN
    TDA7729_MODE_SWITCH("TDA7729SoftMute_En_Switch",4,0,1,1),    //AUDIO_AREA_SOFT_MUTE
    TDA7729_MODE_SWITCH("TDA7729Mute_Ctl_Switch",4,1,1,1),       //AUDIO_AREA_MUTE_CFG
    TDA7729_MODE_SWITCH("TDA7729SoftMute_Time_Switch",4,2,2,3),  //AUDIO_AREA_SOFT_MUTE_TIME
    TDA7729_MODE_SWITCH("TDA7729SubIn_Src_Switch",4,4,1,1),      //AUDIO_AREA_SUB_SRC
    TDA7729_MODE_SWITCH("TDA7729SubOut_En_Switch",4,5,1,1),      //AUDIO_AREA_SUB_EN
    TDA7729_MODE_SWITCH("TDA7729FastCharge_En_Switch",4,6,1,1),  //AUDIO_AREA_FAST_CHARGE
    TDA7729_MODE_SWITCH("TDA7729AliasFlt_En_Switch",4,7,1,1),    //AUDIO_AREA_ALIAS_FILTER
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Loud_Switch",5,0,1,1),  //AUDIO_AREA_SOFT_STEP_LOUD
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Vol_Switch",5,1,1,1),   //AUDIO_AREA_SOFT_STEP_VOL
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Tre_Switch",5,2,1,1),   //AUDIO_AREA_SOFT_STEP_TRE
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Mid_Switch",5,3,1,1),   //AUDIO_AREA_SOFT_STEP_MID
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Bass_Switch",5,4,1,1),  //AUDIO_AREA_SOFT_STEP_BASS
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Fl_Switch",5,5,1,1),    //AUDIO_AREA_SOFT_STEP_FL
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Fr_Switch",5,6,1,1),    //AUDIO_AREA_SOFT_STEP_FR
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Rl_Switch",5,7,1,1),    //AUDIO_AREA_SOFT_STEP_RL
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Rr_Switch",6,0,1,1),    //AUDIO_AREA_SOFT_STEP_RR
    TDA7729_MODE_SWITCH("TDA7729SoftStep_SubL_Switch",6,1,1,1),  //AUDIO_AREA_SOFT_STEP_SUBL
    TDA7729_MODE_SWITCH("TDA7729SoftStep_SubR_Switch",6,2,1,1),  //AUDIO_AREA_SOFT_STEP_SUBR
    TDA7729_MODE_SWITCH("TDA7729SoftStep_Time_Switch",6,3,1,1),  //AUDIO_AREA_SOFT_STEP_TIME
    TDA7729_MODE_SWITCH("TDA7729Loud_Atten_Switch",7,0,4,15),    //AUDIO_AREA_LOUD_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Loud_CFreq_Switch",7,4,2,3),     //AUDIO_AREA_LOUD_C_FRQ
    TDA7729_MODE_SWITCH("TDA7729Loud_Boost_Switch",7,6,1,1),     //AUDIO_AREA_LOUD_BOOST
    TDA7729_MODE_SWITCH("TDA7729Loud_SoftStep_Switch",7,7,1,1),  //AUDIO_AREA_LOUD_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Vol_Cfg_Switch",8,0,5,31),       //AUDIO_AREA_VOL
    TDA7729_MODE_SWITCH("TDA7729OutGain_Cfg_Switch",8,6,1,1),    //AUDIO_AREA_OUT_GAIN
    TDA7729_MODE_SWITCH("TDA7729Vol_SoftStep_Switch",8,7,1,1),   //AUDIO_AREA_VOL_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Tre_Atten_Switch",9,0,5,31),     //AUDIO_AREA_TRE_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Tre_CFreq_Switch",9,5,2,3),      //AUDIO_AREA_TRE_C_FRQ
    TDA7729_MODE_SWITCH("TDA7729Tre_SoftStep_Switch",9,7,1,1),   //AUDIO_AREA_TRE_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Mid_Atten_Switch",10,0,5,31),   //AUDIO_AREA_MID_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Mid_Q_Switch",10,5,2,3),         //AUDIO_AREA_MID_Q
    TDA7729_MODE_SWITCH("TDA7729Mid_SoftStep_Switch",10,7,1,1),  //AUDIO_AREA_MID_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Bass_Atten_Switch",11,0,5,31),   //AUDIO_AREA_BASS_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Bass_Q_Switch",11,5,2,3),        //AUDIO_AREA_BASS_Q
    TDA7729_MODE_SWITCH("TDA7729Bass_SoftStep_Switch",11,7,1,1), //AUDIO_AREA_BASS_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Sub_CutFreq_Switch",12,0,2,3),   //AUDIO_AREA_SUB_CUT_FRQ
    TDA7729_MODE_SWITCH("TDA7729Sub_Phase_Switch",12,2,1,1),     //AUDIO_AREA_SUB_PHASE
    TDA7729_MODE_SWITCH("TDA7729Mid_CFreq_Switch",12,3,2,3),     //AUDIO_AREA_MID_C_FRQ
    TDA7729_MODE_SWITCH("TDA7729Bass_CFreq_Switch",12,5,2,3),    //AUDIO_AREA_BASS_C_FRQ
    TDA7729_MODE_SWITCH("TDA7729Bass_DC_Switch",12,7,1,1),       //AUDIO_AREA_BASS_DC
    TDA7729_MODE_SWITCH("TDA7729Lf_Atten_Switch",13,0,7,127),    //AUDIO_AREA_LF_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Lf_SoftStep_Switch",13,7,1,1),   //AUDIO_AREA_LF_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Rf_Atten_Switch",14,0,7,127),    //AUDIO_AREA_RF_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Rf_SoftStep_Switch",14,7,1,1),   //AUDIO_AREA_RF_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729LR_Atten_Switch",15,0,7,127),    //AUDIO_AREA_LR_ATTEN
    TDA7729_MODE_SWITCH("TDA7729LR_SoftStep_Switch",15,7,1,1),   //AUDIO_AREA_LR_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Rr_Atten_Switch",16,0,7,127),    //AUDIO_AREA_RR_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Rr_SoftStep_Switch",16,7,1,1),   //AUDIO_AREA_RR_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Subl_Atten_Switch",17,0,7,127),  //AUDIO_AREA_SUBL_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Subl_SoftStep_Switch",17,7,1,1), //AUDIO_AREA_SUBL_SOFT_STEP
    TDA7729_MODE_SWITCH("TDA7729Subr_Atten_Switch",18,0,7,127),  //AUDIO_AREA_SUBR_ATTEN
    TDA7729_MODE_SWITCH("TDA7729Subr_SoftStep_Switch",18,7,1,1)  //AUDIO_AREA_SUBR_SOFT_STEP
};

int tda7729_mode_controls_add(struct snd_soc_codec *codec)
{
    snd_soc_add_codec_controls(codec, tda7729_mode_controls, ARRAY_SIZE(tda7729_mode_controls));

    pr_info("<tda7729_mode_controls_add> ok! \n");

    return 0;
}

EXPORT_SYMBOL(tda7729_mode_controls_add);

int tda7729_mute_control(bool enable)
{
    enable = enable == 1 ? 0 : 1;

    mode_bits_write(REG4_OTHER_CFG0, 0, 1, enable);

    tda7729_write(REG4_OTHER_CFG0, mode_read(REG4_OTHER_CFG0));

    pr_info("<tda7729_mute_control> enable %d! \n", enable);

    return 0;
}
EXPORT_SYMBOL(tda7729_mute_control);


static ssize_t tda7729_debug_read_proc(struct file *file, char __user *page, size_t size, loff_t *ppos)
{
    s32 ret = 0;

    pr_info("<tda7729_debug_read_proc> ok!!!!!!!! \n");

    return ret;
}

static ssize_t tda7729_debug_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    //s32 ret = 0;
    u8 data;
    size_t mode_num;

    char mode_str[15] = { 0 };
    unsigned int para1,para2,para3;

    u8 mode_cfg[TDA7729_PROC_MODE_MAX];

    mode_num = (TDA7729_PROC_MODE_MAX < count) ? TDA7729_PROC_MODE_MAX : count;

    if (copy_from_user(mode_cfg, buffer, mode_num)) {
        pr_err("<tda7729_debug_write_proc> : fetch config mode fail !!! \n");
        return count;
    }

    if (sscanf(mode_cfg, "%s %x %x %x", (char *)&mode_str, &para1, &para2, &para3) == -1)
        return -1;

    pr_info("tda7729_debug_write_proc: cmd : (%s), para1(0x%x), para2(0x%x), para3(0x%x). \n",
                                            mode_str,para1, para2, para3 );

    if (strcmp(mode_str, "tda_write") == 0) {
        tda7729_write(para1, para2);
    }

    if (strcmp(mode_str, "tda_read") == 0) {
        tda7729_read(para1, &data);
    }

    pr_debug("<tda7729_debug_write_proc> ok!!!!!!!! \n");

    return count;
}

static const struct file_operations tda7729_debug_fops = {
    .owner = THIS_MODULE,
    .read = tda7729_debug_read_proc,
    .write = tda7729_debug_write_proc,
};

s32 tda7729_init_debug_node(void)
{
    tda7729_debug_proc_entry = proc_create(TDA7729_DEBUG_PROC_FILE, 0660, NULL, &tda7729_debug_fops);
    if (tda7729_debug_proc_entry == NULL) {
        pr_err("create_proc_entry %s FAILED!", TDA7729_DEBUG_PROC_FILE);
        return -1;
    }
    pr_debug("create_proc_entry %s SUCCESS. \n", TDA7729_DEBUG_PROC_FILE);
    return 0;
}

void tda7729_deinit_debug_node(void)
{
    if (tda7729_debug_proc_entry != NULL)
        remove_proc_entry(TDA7729_DEBUG_PROC_FILE, NULL);
}

static s32 tda7729_write(u8 addr, u8 data)
{
    s32 ret = 0;
    u8 tryCount = 0;

    struct i2c_msg msg;
    struct i2c_client *i2c_client = tda7729->client;
    u8 wr_buf[2];

    pr_info("<tda7729_write> -> addr: 0x%x, data = 0x%x \n", addr, data);

  #ifdef CONFIG_MTK_I2C_EXTENSION
    msg.addr = ((i2c_client->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG));
    msg.timing = I2C_MASTER_CLOCK;
    msg.ext_flag = 0;
  #else
    msg.addr = i2c_client->addr;
  #endif
    msg.flags = 0;
    msg.buf = wr_buf;
    msg.len = 2;

    wr_buf[0] = addr;
    wr_buf[1] = data;

    do {
        ret = i2c_transfer(i2c_client->adapter, &msg, 1);
        if(ret != 1)
        {
            msleep(20);
            tryCount++;
            pr_err("<tda7729_write>, i2c write fail, try %d times\n",tryCount);
        }
    } while (ret != 1 && tryCount < 200);

    return ret;
}

static s32 tda7729_read(u8 addr, u8 *data)
{
    int ret = 0;

    struct i2c_msg msgs[2];
    struct i2c_client *i2c_client = tda7729->client;

    u8 rd_buf;

    pr_info("<tda7729_read> -> addr: 0x%x \n", addr);

  #ifdef CONFIG_MTK_I2C_EXTENSION
    msgs[0].addr = ((i2c_client->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG));
    msgs[0].timing = I2C_MASTER_CLOCK;
    msgs[0].ext_flag = 0;
    msgs[1].addr = ((i2c_client->addr & I2C_MASK_FLAG) | (I2C_ENEXT_FLAG));
    msgs[1].timing = I2C_MASTER_CLOCK;
    msgs[1].ext_flag = 0;
  #else
    msgs[0].addr = i2c_client->addr;
    msgs[1].addr = i2c_client->addr;
  #endif
    msgs[0].flags = 0;
    msgs[0].buf = &rd_buf;
    msgs[0].len = 1;
    rd_buf = addr;

    msgs[1].flags = I2C_M_RD;
    msgs[1].buf = data;
    msgs[1].len = 1;

    ret = i2c_transfer(i2c_client->adapter, msgs, 2);

    pr_info("<tda7729_read> -> value : 0x%x \n", *data);

    return ret;
}

/*
static int tda7729_suspend(struct i2c_client * i2c_client, pm_message_t mesg)
{
    pr_info("<tda7729_suspend> -> done \n");

	return 0;
}
*/

static int tda7729_resume(struct device *dev)
{
    int addr;

    for (addr = 0; addr < REG_MAX; addr++)
    {
        tda7729_write(addr, mode_read(addr));
    }

    pr_info("<tda7729_resume> -> done \n");

    return 0;
}

static int tda7729_i2c_probe(struct i2c_client *i2c_client,
                 const struct i2c_device_id *id)
{
    int ret = 0;
    u8 data;

    pr_info("<tda7729_i2c_probe> driver verison : %s \n", TDA_DRV_VER);


    tda7729 = devm_kzalloc(&i2c_client->dev, sizeof(struct tda7729_private),
                   GFP_KERNEL);
    if (tda7729 == NULL)
        return -ENOMEM;

    tda7729->client = i2c_client;
    tda7729->dev = &i2c_client->dev;

    i2c_set_clientdata(i2c_client, tda7729);

    //init
    data = 0x58;
    mode_write(REG0_IN_CFG0, data);
    ret = tda7729_write(REG0_IN_CFG0, data);
    if (ret != 1) {//only care first cmd, to detect if tda7729 exist
        tda7729_init_debug_node();
        pr_info("<tda7729_i2c_probe> fail, i2c write timeout!!!!!!!! \n");
        return 0;
    }

    data = 0xff;
    mode_write(REG3_MIX_CFG1, data);
    tda7729_write(REG3_MIX_CFG1, data);

    data = 0x40;
    mode_write(REG7_LOUD, data);
    tda7729_write(REG7_LOUD, data);

    data = 0x59;
    mode_write(REG8_VOL, data);
    tda7729_write(REG8_VOL, data);

    data = 0x0f;
    mode_write(REG9_TREBLE, data);
    tda7729_write(REG9_TREBLE, data);

    data = 0x0f;
    mode_write(REG10_MIDDILE, data);
    tda7729_write(REG10_MIDDILE, data);

    data = 0x0f;
    mode_write(REG11_BASS, data);
    tda7729_write(REG11_BASS, data);

    data = 0x84;
    mode_write(REG12_OTHER_CFG1, data);
    tda7729_write(REG12_OTHER_CFG1, data);

    data = 0x0;
    mode_write(REG13_ATTEN_LF, data);
    mode_write(REG14_ATTEN_RF, data);
    mode_write(REG15_ATTEN_LR, data);
    mode_write(REG16_ATTEN_RR, data);

    tda7729_write(REG13_ATTEN_LF, data);
    tda7729_write(REG14_ATTEN_RF, data);
    tda7729_write(REG15_ATTEN_LR, data);
    tda7729_write(REG16_ATTEN_RR, data);

    data = 0x0;
    mode_write(REG5_SOFT_STEP1, data);
    tda7729_write(REG5_SOFT_STEP1, data);
    mode_write(REG6_SOFT_STEP2, data);
    tda7729_write(REG6_SOFT_STEP2, data);

    data = 0xf9;
    mode_write(REG4_OTHER_CFG0, data);
    tda7729_write(REG4_OTHER_CFG0, data);

    tda7729_init_debug_node();

    pr_info("<tda7729_i2c_probe> ok!!!!!!!! \n");

    return 0;
}

static int tda7729_i2c_remove(struct i2c_client *client)
{
    return 0;
}

static const struct dev_pm_ops tda7729_pm_ops = {
    .resume = tda7729_resume,
};

static const struct of_device_id tda7729_of_match[] = {
    { .compatible = "autochips,tda7729", },
    { }
};
MODULE_DEVICE_TABLE(of, tda7729_of_match);

static const struct i2c_device_id tda7729_id[] = {
    { "tda7729", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tda7729_id);

static struct i2c_driver tda7729_i2c_driver = {
    .driver = {
        .name = "tda7729",
        .owner = THIS_MODULE,
        .of_match_table = tda7729_of_match,
        .pm = &tda7729_pm_ops,
    },
    .id_table = tda7729_id,
    .probe =    tda7729_i2c_probe,
    .remove =   tda7729_i2c_remove,
};

module_i2c_driver(tda7729_i2c_driver);

MODULE_DESCRIPTION("ASoC TDA7729 driver");
MODULE_AUTHOR("AutoChips, AutoChips, <autochips.com>");
MODULE_LICENSE("GPL");
