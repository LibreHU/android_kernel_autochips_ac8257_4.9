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
 */

#ifndef __MMDVFS_CONFIG_AC8257_H__
#define __MMDVFS_CONFIG_AC8257_H__

#include "mmdvfs_config_util.h"

/* Part I MMSVFS HW Configuration (OPP)*/
/* Define the number of mmdvfs, vcore and mm clks opps */

/* Max total MMDVFS opps of the profile support */
#define AC8257_MMDVFS_OPP_MAX 3

struct mmdvfs_profile_mask qos_apply_profiles[] = {
/* #ifdef MMDVFS_QOS_SUPPORT */
#if 1
	/* ISP for opp0 */
	{"ISP",
		SMI_BWC_SCEN_CAM_PV,
		MMDVFS_FINE_STEP_OPP0},
	{"ISP",
		SMI_BWC_SCEN_CAM_CP,
		MMDVFS_FINE_STEP_OPP0},
	{"ISP",
		SMI_BWC_SCEN_VR,
		MMDVFS_FINE_STEP_OPP0},
	/* ISP for opp1 */
	{"ISP",
		SMI_BWC_SCEN_CAM_PV,
		MMDVFS_FINE_STEP_OPP1},
	{"ISP",
		SMI_BWC_SCEN_CAM_CP,
		MMDVFS_FINE_STEP_OPP1},
	{"ISP",
		SMI_BWC_SCEN_VR,
		MMDVFS_FINE_STEP_OPP1},
	/* ISP for opp2 */
	{"ISP",
		SMI_BWC_SCEN_CAM_PV,
		MMDVFS_FINE_STEP_OPP2},
	{"ISP",
		SMI_BWC_SCEN_CAM_CP,
		MMDVFS_FINE_STEP_OPP2},
	{"ISP",
		SMI_BWC_SCEN_VR,
		MMDVFS_FINE_STEP_OPP2},
	/* ICFP for opp0 */
	{"ICFP",
		SMI_BWC_SCEN_ICFP,
		MMDVFS_FINE_STEP_OPP0},
	/* debug entry */
	{"DEBUG",
		0,
		MMDVFS_FINE_STEP_UNREQUEST },
#else
	/* debug entry */
	{"DEBUG",
		QOS_ALL_SCENARIO,
		0 },
#endif
};

/* Part II MMDVFS Scenario's Step Confuguration */

#define AC8257_MMDVFS_SENSOR_MIN (13000000)
#define AC8257_MMDVFS_SENSOR_MID (16000000)
/* A.1 [LP4 2-ch] Scenarios of each MM DVFS Step (force kicker) */
/* OPP 0 scenarios */
#define AC8257_MMDVFS_OPP0_NUM 1
struct mmdvfs_profile ac8257_mmdvfs_opp0_profiles[AC8257_MMDVFS_OPP0_NUM] = {
	{"ICFP", SMI_BWC_SCEN_ICFP, {0, 0, 0}, {0, 0, 0 } },
/*
 *	{"Camera Preview", SMI_BWC_SCEN_CAM_PV,
 *		{AC8257_MMDVFS_SENSOR_MID, 0, 0}, {0, 0, 0 } },
 *	{"Camera Capture", SMI_BWC_SCEN_CAM_CP,
 *		{AC8257_MMDVFS_SENSOR_MID, 0, 0}, {0, 0, 0 } },
 *	{"Video Recording", SMI_BWC_SCEN_VR,
 *		{AC8257_MMDVFS_SENSOR_MID, 0, 0}, {0, 0, 0 } },
 */
};

/* OPP 1 scenarios */
#define AC8257_MMDVFS_OPP1_NUM 0
struct mmdvfs_profile ac8257_mmdvfs_opp1_profiles[AC8257_MMDVFS_OPP1_NUM] = {
/*
 *	{"Camera Preview", SMI_BWC_SCEN_CAM_PV,
 *		{AC8257_MMDVFS_SENSOR_MIN, 0, 0}, {0, 0, 0 } },
 *	{"Camera Capture", SMI_BWC_SCEN_CAM_CP,
 *		{AC8257_MMDVFS_SENSOR_MIN, 0, 0}, {0, 0, 0 } },
 *	{"Video Recording", SMI_BWC_SCEN_VR,
 *		{AC8257_MMDVFS_SENSOR_MIN, 0, 0}, {0, 0, 0 } },
 */
};

/* OPP 2 scenarios */
#define AC8257_MMDVFS_OPP2_NUM 0
struct mmdvfs_profile ac8257_mmdvfs_opp2_profiles[AC8257_MMDVFS_OPP2_NUM] = {
/*
 *	{"Camera Preview", SMI_BWC_SCEN_CAM_PV, {0, 0, 0}, {0, 0, 0 } },
 *	{"Camera Capture", SMI_BWC_SCEN_CAM_CP, {0, 0, 0}, {0, 0, 0 } },
 *	{"Video Recording", SMI_BWC_SCEN_VR, {0, 0, 0}, {0, 0, 0 } },
 */
};

struct mmdvfs_step_to_qos_step legacy_to_qos_step[AC8257_MMDVFS_OPP_MAX] = {
	{0, 0},
	{1, 1},
	{2, 2},
};

struct mmdvfs_step_profile ac8257_step_profile[AC8257_MMDVFS_OPP_MAX] = {
	{0, ac8257_mmdvfs_opp0_profiles, AC8257_MMDVFS_OPP0_NUM, {0} },
	{1, ac8257_mmdvfs_opp1_profiles, AC8257_MMDVFS_OPP1_NUM, {0} },
	{2, ac8257_mmdvfs_opp2_profiles, AC8257_MMDVFS_OPP2_NUM, {0} },
};
#endif /* __MMDVFS_CONFIG_AC8257_H__ */
