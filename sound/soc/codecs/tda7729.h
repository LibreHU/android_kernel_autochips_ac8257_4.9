/*
 * tda7729.h -- TDA7729 ALSA SoC audio driver
 *
 * Copyright 2016 autochips.
 *
 * Author: quihua yin <qiuhua.yin@autochips.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __TDA7729_H__
#define __TDA7729_H__

/*******                proc debug                *********/
#define TDA7729_DEBUG_PROC_FILE     "tda7729_debug"
#define TDA7729_PROC_MODE_MAX       280

/*******                I2C format                *********/
#define TDA7729_CHIP_ID             0x44
#define IIC_MAX_TRANSFER_SIZE       8
#define TDA7729_ADDR_LENGTH         1
#define I2C_MASTER_CLOCK            300

/******* REG0 : input configuration / main selector *********/
#define REG0_IN_CFG0                0
#define AUDIO_AREA_M_SRC            "TDA7729MainIn_Src_Switch",0,0,3,7          //main source selecor
#define AUDIO_AREA_M_MD_SEL         "TDA7729MainMd_Cfg_Switch",0,3,1,1          //MD1/2 configuration for main selector
#define AUDIO_AREA_M_IGAIN          "TDA7729MainIn_Gain_Switch",0,4,1,1         //main source input gain select
#define AUDIO_AREA_ICFG             "TDA7729HwIn_Cfg_Switch",0,5,3,7            //input configuration

/******* REG1 : 2nd source selector /direct path *********/
#define REG0_IN_CFG1                1
#define AUDIO_AREA_SND_SRC          "TDA7729SndIn_Src_Switch",1,0,3,7           //2nd source selector
#define AUDIO_AREA_SND_MD_SEL       "TDA7729SndMd_Cfg_Switch",1,3,1,1           //MD 1/2 configuraton for 2nd selector
#define AUDIO_AREA_SND_IGAIN        "TDA7729SndIn_Gain_Switch",1,4,1,1          //2nd source input gain select
#define AUDIO_AREA_QD2_BYPASS       "TDA7729Qd2_Bypass_Switch",1,5,1,1          //QD2 Bypass (Front)
#define AUDIO_AREA_QD3_BYPASS       "TDA7729Qd3_Bypass_Switch",1,6,1,1          //QD3 Bypass (Rear)
#define AUDIO_AREA_QD4_BYPASS       "TDA7729Qd4_Bypass_Switch",1,7,1,1          //QD4 Bypass (subwoofer)

/******* REG2 : mixing source / mixding gain *********/
#define REG2_MIX_CFG0               2
#define AUDIO_AREA_MIX_SRC          "TDA7729Mix_Src_Switch",2,0,3,7             //mixing source selector
#define AUDIO_AREA_MIX_ATTEN        "TDA7729Mix_Atten_Switch",2,3,5,31          //mixing attenuator

/******* REG3 : mix control / level meter *********/
#define REG3_MIX_CFG1               3
#define AUDIO_AREA_MIX_FL           "TDA7729MixTo_Fl_Switch",3,0,1,1            //mix to front left
#define AUDIO_AREA_MIX_FR           "TDA7729MixTo_Fr_Switch",3,1,1,1            //mix to front right
#define AUDIO_AREA_MIX_RL           "TDA7729MixTo_Rl_Switch",3,2,1,1            //mix to rear left
#define AUDIO_AREA_MIX_RR           "TDA7729MixTo_Rr_Switch",3,3,1,1            //mix to rear right
#define AUDIO_AREA_REAR_SRC         "TDA7729Rear_Src_Switch",3,4,1,1            //rear speaker input configuration
#define AUDIO_AREA_LEVEL_CFG        "TDA7729Level_Cfg_Switch",3,6,1,1           //level meter reset
#define AUDIO_AREA_LEVEL_EN         "TDA7729Level_En_Switch",3,7,1,1            //level meter and beep enable

/******* REG4 : soft mute / others *********/
#define REG4_OTHER_CFG0             4
#define AUDIO_AREA_SOFT_MUTE        "TDA7729SoftMute_En_Switch",4,0,1,1         //soft mute
#define AUDIO_AREA_MUTE_CFG         "TDA7729Mute_Ctl_Switch",4,1,1,1            //pin influence for mute
#define AUDIO_AREA_SOFT_MUTE_TIME   "TDA7729SoftMute_Time_Switch",4,2,2,3       //soft mute time
#define AUDIO_AREA_SUB_SRC          "TDA7729SubIn_Src_Switch",4,4,1,1           //subwoofer input configuration
#define AUDIO_AREA_SUB_EN           "TDA7729SubOut_En_Switch",4,5,1,1           //subwoofer enable(outl3&outr3)
#define AUDIO_AREA_FAST_CHARGE      "TDA7729FastCharge_En_Switch",4,6,1,1       //fast charge
#define AUDIO_AREA_ALIAS_FILTER     "TDA7729AliasFlt_En_Switch",4,7,1,1         //anti-alias filter

/******* REG5 : softstep 1 *********/
#define REG5_SOFT_STEP1             5
#define AUDIO_AREA_SOFT_STEP_LOUD   "TDA7729SoftStep_Loud_Switch",5,0,1,1       //loudness soft step
#define AUDIO_AREA_SOFT_STEP_VOL    "TDA7729SoftStep_Vol_Switch",5,1,1,1        //volume soft step
#define AUDIO_AREA_SOFT_STEP_TRE    "TDA7729SoftStep_Tre_Switch",5,2,1,1        //treble soft step
#define AUDIO_AREA_SOFT_STEP_MID    "TDA7729SoftStep_Mid_Switch",5,3,1,1        //middle soft step
#define AUDIO_AREA_SOFT_STEP_BASS   "TDA7729SoftStep_Bass_Switch",5,4,1,1       //bass soft step
#define AUDIO_AREA_SOFT_STEP_FL     "TDA7729SoftStep_Fl_Switch",5,5,1,1         //speaker lf soft step
#define AUDIO_AREA_SOFT_STEP_FR     "TDA7729SoftStep_Fr_Switch",5,6,1,1         //speaker rf soft step
#define AUDIO_AREA_SOFT_STEP_RL     "TDA7729SoftStep_Rl_Switch",5,7,1,1         //speaker lr soft step

/******* REG6 : softstep 2 *********/
#define REG6_SOFT_STEP2             6
#define AUDIO_AREA_SOFT_STEP_RR     "TDA7729SoftStep_Rr_Switch",6,0,1,1         //speaker rr soft step
#define AUDIO_AREA_SOFT_STEP_SUBL   "TDA7729SoftStep_SubL_Switch",6,1,1,1       //subwoofer left soft step
#define AUDIO_AREA_SOFT_STEP_SUBR   "TDA7729SoftStep_SubR_Switch",6,2,1,1       //subwoofer right soft step
#define AUDIO_AREA_SOFT_STEP_TIME   "TDA7729SoftStep_Time_Switch",6,3,1,1       //soft step time

/******* REG7 : loudness *********/
#define REG7_LOUD                   7
#define AUDIO_AREA_LOUD_ATTEN       "TDA7729Loud_Atten_Switch",7,0,4,15         //attenuation
#define AUDIO_AREA_LOUD_C_FRQ       "TDA7729Loud_CFreq_Switch",7,4,2,3          //center frequency
#define AUDIO_AREA_LOUD_BOOST       "TDA7729Loud_Boost_Switch",7,6,1,1          //high boost
#define AUDIO_AREA_LOUD_SOFT_STEP   "TDA7729Loud_SoftStep_Switch",7,7,1,1       //soft step action

/******* REG8 : volume / output gain *********/
#define REG8_VOL                    8
#define AUDIO_AREA_VOL              "TDA7729Vol_Cfg_Switch",8,0,5,16            //gain/attenuation
#define AUDIO_AREA_OUT_GAIN         "TDA7729OutGain_Cfg_Switch",8,6,1,1         //output gain
#define AUDIO_AREA_VOL_SOFT_STEP    "TDA7729Vol_SoftStep_Switch",8,7,1,1        //soft step action

/******* REG9 : treble filter *********/
#define REG9_TREBLE                 9
#define AUDIO_AREA_TRE_ATTEN        "TDA7729Tre_Atten_Switch",9,0,5,16          //gain/attenuation
#define AUDIO_AREA_TRE_C_FRQ        "TDA7729Tre_CFreq_Switch",9,5,2,3           //treble center frequency
#define AUDIO_AREA_TRE_SOFT_STEP    "TDA7729Tre_SoftStep_Switch",9,7,1,1        //soft step action

/******* REG10 : middle filter *********/
#define REG10_MIDDILE               10
#define AUDIO_AREA_MID_ATTEN        "TDA7729Mid_Atten_Switch",10,0,5,16         //gain/attenuation
#define AUDIO_AREA_MID_Q            "TDA7729Mid_Q_Switch",10,5,2,3              //middle q factor
#define AUDIO_AREA_MID_SOFT_STEP    "TDA7729Mid_SoftStep_Switch",10,7,1,1       //soft step action

/******* REG11 : bass filter *********/
#define REG11_BASS                  11
#define AUDIO_AREA_BASS_ATTEN       "TDA7729Bass_Atten_Switch",11,0,5,16        //gain / attenuation
#define AUDIO_AREA_BASS_Q           "TDA7729Bass_Q_Switch",11,5,2,3             //bass q factor
#define AUDIO_AREA_BASS_SOFT_STEP   "TDA7729Bass_SoftStep_Switch",11,7,1,1      //soft step action

/******* REG12 : subwoofer / middle / bass *********/
#define REG12_OTHER_CFG1            12
#define AUDIO_AREA_SUB_CUT_FRQ      "TDA7729Sub_CutFreq_Switch",12,0,2,3        //subwoofer cut-off frequency
#define AUDIO_AREA_SUB_PHASE        "TDA7729Sub_Phase_Switch",12,2,1,1          //subwoofer output phase
#define AUDIO_AREA_MID_C_FRQ        "TDA7729Mid_CFreq_Switch",12,3,2,3          //middle center frequency
#define AUDIO_AREA_BASS_C_FRQ       "TDA7729Bass_CFreq_Switch",12,5,2,3         //bass center frequency
#define AUDIO_AREA_BASS_DC          "TDA7729Bass_DC_Switch",12,7,1,1            //bass dc mode

/******* REG13 : lf speaker attenuation *********/
#define REG13_ATTEN_LF              13
#define AUDIO_AREA_LF_ATTEN         "TDA7729Lf_Atten_Switch",13,0,7,127         //gain/attenuation
#define AUDIO_AREA_LF_SOFT_STEP     "TDA7729Lf_SoftStep_Switch",13,7,1,1        //soft step action

/******* REG14 : rf speaker attenuation *********/
#define REG14_ATTEN_RF              14
#define AUDIO_AREA_RF_ATTEN         "TDA7729Rf_Atten_Switch",14,0,7,127         //gain/attenuation
#define AUDIO_AREA_RF_SOFT_STEP     "TDA7729Rf_SoftStep_Switch",14,7,1,1        //soft step action

/******* REG15 : lr speaker attenuation *********/
#define REG15_ATTEN_LR              15
#define AUDIO_AREA_LR_ATTEN         "TDA7729LR_Atten_Switch",15,0,7,127         //gain/attenuation
#define AUDIO_AREA_LR_SOFT_STEP     "TDA7729LR_SoftStep_Switch",15,7,1,1        //soft step action

/******* REG16 : rr speaker attenuation *********/
#define REG16_ATTEN_RR              16
#define AUDIO_AREA_RR_ATTEN         "TDA7729Rr_Atten_Switch",16,0,7,127         //gain/attenuation
#define AUDIO_AREA_RR_SOFT_STEP     "TDA7729Rr_SoftStep_Switch",16,7,1,1        //soft step action

/******* REG17 : subl speaker attenuation *********/
#define REG17_ATTEN_SUBL            17
#define AUDIO_AREA_SUBL_ATTEN       "TDA7729Subl_Atten_Switch",17,0,7,127       //gain/attenuation
#define AUDIO_AREA_SUBL_SOFT_STEP   "TDA7729Subl_SoftStep_Switch",17,7,1,1      //soft step action

/******* REG18 : subr speaker attenuation *********/
#define REG18_ATTEN_SUBR            18
#define AUDIO_AREA_SUBR_ATTEN       "TDA7729Subr_Atten_Switch",18,0,7,127       //gain/attenuation
#define AUDIO_AREA_SUBR_SOFT_STEP   "TDA7729Subr_SoftStep_Switch",18,7,1,1      //soft step action

#define REG_MAX                     19


/********************************* const data define  ********************************************/

/******* ALl related *********/
enum tda7729_fun_ctl {
    FUN_ON,
    FUN_OFF
};

/******* REG00 related *********/
enum tda7729_main_input {
    MAIN_IN0,
    MAIN_IN1,
    MAIN_IN2,
    MAIN_IN3,
    MAIN_IN4,
    MAIN_IN5,
    MAIN_IN6,
    MAIN_IN7
};

enum tda7729_main_md_cfg {
    MAIN_MD1,
    MAIN_MD2
};

enum tda7729_main_in_gain {
    MAIN_IN_0DB,
    MAIN_IN_3DB
};

enum tda7729_main_in_cfg {
    MAIN_IN_CFG0,
    MAIN_IN_CFG1,
    MAIN_IN_CFG2,
    MAIN_IN_CFG3,
    MAIN_IN_CFG4,
    MAIN_IN_CFG5,
    MAIN_IN_CFG6,
    MAIN_IN_CFG7
};

/******* REG01 related *********/
enum tda7729_snd_input {
    SND_IN0,
    SND_IN1,
    SND_IN2,
    SND_IN3,
    SND_IN4,
    SND_IN5,
    SND_IN6,
    SND_IN7
};

enum tda7729_snd_md_cfg {
    SND_MD1,
    SND_MD2
};

enum tda7729_snd_in_gain {
    SND_IN_0DB,
    SND_IN_3DB
};

/******* REG02 related *********/
enum tda7729_mix_input {
    MIX_IN0,
    MIX_IN1,
    MIX_IN2,
    MIX_IN3,
    MIX_IN4,
    MIX_IN5,
    MIX_IN6,
    MIX_IN7
};

//0db, -1db, -2db, ......, -31db
u8 mix_attenuator_table[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

/******* REG03 related *********/
enum tda7729_rear_input {
    REAR_DIRECT_OR_SND_SRC,
    REAR_MAIN_SIGNAL
};

enum tda7729_level_meter_ctl {
    LEVEL_METER_NORMAL,
    LEVEL_METER_RESET
};

/******* REG04 related *********/
enum tda7729_soft_mute_ctl_mode {
    CTL_PIN_AND_IIC,
    CTL_IIC
};

enum tda7729_soft_mute_time {
    TIME_0_48_MS,
    TIME_0_96_MS,
    TIME_7_68_MS,
    TIME_15_36_MS
};

enum tda7729_sub_in {
    SUB_FROM_INPUT_MUX,
    SUB_FROM_BASS_OUTPUT
};

/******* REG06 related *********/
enum tda7729_soft_step_time {
    SOFT_STEP_5MS,
    SOFT_STEP_10MS
};

/******* REG07 related *********/
//  0db, -1db, -2db, ......, -15db
u8 loud_attenuator_table[16] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf
};

enum tda7729_loud_c_freq {
    LOUD_CFREQ_FLAT,
    LOUD_CFREQ_400HZ,
    LOUD_CFREQ_800HZ,
    LOUD_CFREQ_2400HZ
};

/******* REG08 related *********/
 // -15db ~ 0db ~ +15db
 // -15  -14  -13  -12  -11  -10  -9   -8   -7   -6   -5   -4   -3   -2  -1    0
 //  1    2    3    4    5    6    7    8    9   10   11   12   13   14  15
u8 vol_attenuator_table[31] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
    0x1e,0x1d,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10
};

enum tda7729_out_gain {
    OUT_GAIN_1DB,
    OUT_GAIN_0DB
};

/******* REG09 related *********/
 // -15db ~ 0db ~ +15db
 // -15  -14  -13  -12  -11  -10  -9   -8   -7   -6   -5   -4   -3   -2  -1    0
 //  1    2    3    4    5    6    7    8    9   10   11   12   13   14  15
u8 treble_attenuator_table[31] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
    0x1e,0x1d,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10
};

enum tda7729_treble_c_freq {
    TRE_CFREQ_10_0KHZ,
    TRE_CFREQ_12_5KHZ,
    TRE_CFREQ_15_0KHZ,
    TRE_CFREQ_17_5KHZ
};

/******* REG10 related *********/
 // -15db ~ 0db ~ +15db
 // -15  -14  -13  -12  -11  -10  -9   -8   -7   -6   -5   -4   -3   -2  -1    0
 //  1    2    3    4    5    6    7    8    9   10   11   12   13   14  15
u8 middle_attenuator_table[31] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
    0x1e,0x1d,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10
};

enum tda7729_middle_q_factor {
    MID_Q_0_5,
    MID_Q_1_25,
    MID_Q_1_5,
    MID_Q_2_0
};

/******* REG11 related *********/
 // -15db ~ 0db ~ +15db
 // -15  -14  -13  -12  -11  -10  -9   -8   -7   -6   -5   -4   -3   -2  -1    0
 //  1    2    3    4    5    6    7    8    9   10   11   12   13   14  15
u8 bass_attenuator_table[31] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
    0x1e,0x1d,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,0x10
};

enum tda7729_bass_q_factor {
    BASS_Q_1_0,
    BASS_Q_1_25,
    BASS_Q_1_5,
    BASS_Q_2_0
};

/******* REG12 related *********/
enum tda7729_sub_cut_freq {
    SUB_CUT_FREQ_FLAT,
    SUB_CUT_FREQ_80HZ,
    SUB_CUT_FREQ_120HZ,
    SUB_CUT_FREQ_160HZ
};

enum tda7729_sub_out_phase {
    SUB_PHASE_180DEG,
    SUB_PHASE_0DEG
};

enum tda7729_mid_c_freq {
    MID_C_FREQ_500HZ,
    MID_C_FREQ_1000HZ,
    MID_C_FREQ_1500HZ,
    MID_C_FREQ_2500HZ
};

enum tda7729_bass_c_freq {
    BASS_C_FREQ_60HZ,
    BASS_C_FREQ_80HZ,
    BASS_C_FREQ_100HZ,
    BASS_C_FREQ_200HZ
};

/******* REG13 - REG16 related *********/
u8 speaker_attenuator_table[81] = {
    0x7F, // mute// 0
    0x5F, // -79dB
    0x5E, // -78dB
    0x5D, // -77dB
    0x5C, // -76dB
    0x5B, // -75dB
    0x5A, // -74dB
    0x59, // -73dB
    0x58, // -72dB
    0x57, // -71dB
    0x56, // -70dB// 10
    0x55, // -69dB
    0x54, // -68dB
    0x53, // -67dB
    0x52, // -66dB
    0x51, // -65dB
    0x50, // -64dB
    0x4F, // -63dB
    0x4E, // -62dB
    0x4D, // -61dB
    0x4C, // -60dB// 20
    0x4B, // -59dB
    0x4A, // -58dB
    0x49, // -57dB
    0x48, // -56dB
    0x47, // -55dB
    0x46, // -54dB
    0x45, // -53dB
    0x44, // -52dB
    0x43, // -51dB
    0x42, // -50dB// 30
    0x41, // -49dB
    0x40, // -48dB
    0x3F, // -47dB
    0x3E, // -46dB
    0x3D, // -45dB
    0x3C, // -44dB
    0x3B, // -43dB
    0x3A, // -42dB
    0x39, // -41dB
    0x38, // -40dB// 40
    0x37, // -39dB
    0x36, // -38dB
    0x35, // -37dB
    0x34, // -36dB
    0x33, // -35dB
    0x32, // -34dB
    0x31, // -33dB
    0x30, // -32dB
    0x2F, // -31dB
    0x2E, // -30dB// 50
    0x2D, // -29dB
    0x2C, // -28dB
    0x2B, // -27dB
    0x2A, // -26dB
    0x29, // -25dB
    0x28, // -24dB
    0x27, // -23dB
    0x26, // -22dB
    0x25, // -21dB
    0x24, // -20dB// 60
    0x23, // -19dB
    0x22, // -18dB
    0x21, // -17dB
    0x20, // -16dB
    0x1F, // -15dB
    0x1E, // -14dB
    0x1D, // -13dB
    0x1C, // -12dB
    0x1B, // -11dB
    0x1A, // -10dB// 70
    0x19, // -9dB
    0x18, // -8dB
    0x17, // -7dB
    0x16, // -6dB
    0x15, // -5dB
    0x14, // -4dB
    0x13, // -3dB
    0x12, // -2dB
    0x11, // -1dB
    0x10, // +0dB// 80
};

#endif
