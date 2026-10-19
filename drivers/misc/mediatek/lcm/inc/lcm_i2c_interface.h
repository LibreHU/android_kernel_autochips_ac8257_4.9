/*
 * Copyright (C) 2018 AutoChips Inc.
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

#ifndef __LCM_TRANSFER_IC_INTERFACE_H__
#define __LCM_TRANSFER_IC_INTERFACE_H__

#define LCM_INTERFACE_FATAL(string, args...)  \
	pr_crit("[F] [DISP][LCM] [%s: %d] "string, __FUNCTION__, __LINE__, ##args)
#define LCM_INTERFACE_ERROR(string, args...)  \
	pr_err("[E] [DISP][LCM] [%s: %d] "string, __FUNCTION__, __LINE__, ##args)
#define LCM_INTERFACE_INFO(string, args...)   pr_info("[I] [DISP][LCM] [%s] "string, __FUNCTION__, ##args)

#if defined(CONFIG_LCM_TRANSFER_IC_SUPPORT)
enum lcm_id_e {
	LCM_ID_SECONDARY = 0,
	LCM_ID_PRIMARY,
	LCM_ID_MAX
};

int interface_read_byte(int lcm_id, unsigned char addr, unsigned char cmd, unsigned char *out);
int interface_write_byte(int lcm_id, unsigned char addr, unsigned char cmd, unsigned char in);
int interface_write_buffer(int lcm_id, unsigned char addr, unsigned char *buffer, int len);
int interface_read_buffer(int lcm_id, unsigned char addr, unsigned char cmd, unsigned char *out, int len);
#else
static inline int interface_read_byte(unsigned char addr, unsigned char cmd, unsigned char *out)
{
	LCM_INTERFACE_ERROR("CONFIG_LCM_TRANSFER_IC_SUPPORT is not.\n");

	return -ENOSYS;
}
static inline int interface_write_byte(unsigned char addr, unsigned char cmd, unsigned char in)
{
	LCM_INTERFACE_ERROR("CONFIG_LCM_TRANSFER_IC_SUPPORT is not.\n");

	return -ENOSYS;
}
#endif
#endif
