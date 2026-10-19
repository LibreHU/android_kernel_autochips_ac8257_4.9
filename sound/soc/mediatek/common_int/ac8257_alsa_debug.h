/*
* Copyright (c) 2019 AutoChips Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

/**
 * @File   : ac8257_alsa_debug.h
 * @Author : shiyuan.dong@autochips.com
 */

#ifndef ALSA_DEBUG_H
#define ALSA_DEBUG_H

#include <linux/device.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#define PLAYBACK 1
#define CAPTURE 0

void alsa_create_debug_file(struct device *dev);
unsigned int write_file(char *tmp_file, unsigned int dev_id,
	unsigned int is_dl, const unsigned char *wbuf, unsigned int length);
#endif /* ALSA_DEBUG_H */
