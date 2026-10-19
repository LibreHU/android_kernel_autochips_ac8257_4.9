/*
* Copyright (c) 2016 AutoChips Inc.
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

#ifndef __BOOT_STATE_H__
#define __BOOT_STATE_H__

enum {
	STATUS_UNKNOWN,
	STATUS_BOOT_START,
	STATUS_BOOT_END,
	STATUS_SUSPEND_START,
	STATUS_SUSPEND_END,
	STATUS_RESUME_START,
	STATUS_RESUME_END,
	STATUS_SHUTDOWN_START,
	STATUS_SHUTDOWN_END,
	STATUS_LAST
};

extern int register_bs_notifer(struct notifier_block *nb);

extern int unregister_bs_notifer(struct notifier_block *nb);

extern int bs_notifer_call_chain(unsigned long val);

extern void  set_pdwnc_gpio_value(uint32_t u4Pin, uint32_t u4value);
#endif
