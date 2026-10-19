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

#include "flashlight-core.h"

const struct flashlight_device_id flashlight_id[] = {
	/* {TYPE, CT, PART, "NAME", CHANNEL, DECOUPLE} */
	{0, 0, 0, "flashlights-rt4505", 0, 0},
};

const int flashlight_device_num =
	sizeof(flashlight_id) / sizeof(struct flashlight_device_id);

