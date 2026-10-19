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
#ifndef _DI_IF_H_
#define _DI_IF_H_

#include "display.h"
#include "di_hal.h"
enum {
	DI_SET_PARAM = 0,
	DI_NOT_SET_PARAM
};
extern struct ion_client *g_ion_client;

typedef struct _atc_di_device {
	int initaled;
	int configParam;
	struct OVERLAY_PARAM param;
	get_di_buffer getdibuffer;
} atc_di_device;
extern atc_di_device DI_INTER_PARAM;

extern struct ion_device *g_ion_device;

/*struct used to describe a physical block*/
typedef struct  _physical_block {
	uint32_t addr;     	/*the physical address of the block*/
	uint32_t size;     /*the length of block, in bytes, typically page aligned*/
} physical_block;

struct ion_handle  *di_ion_to_phyaddr(int fd, physical_block *phys_blocks);
void dev_ion_free(void);

typedef void(*DI_ADDRESS_FUNC) (void *buffer_info);
void di_get_addr_fun (void *buffer_info);
void di_start_wch(u16 src_width, u16 src_height, u16 dst_width, u16 dst_height, u16 stride);
void di_hw_triger(atc_di_device DI_INTER_PARAM);
void di_register_get_addr_fun(DI_ADDRESS_FUNC di_get_addr_fun);


#endif


