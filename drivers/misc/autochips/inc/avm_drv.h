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

#ifndef AVM_DRV_H_
#define AVM_DRV_H_

/************  [ IOCTL Code ]  ***************/
#define AVM_BUFFER_MAX       3
#define AVM_CHANNEL_MAX      4

#define IOCTL_SET_FD         	  _IOW('M', 0x1, unsigned)
#define IOCTL_GET_PHY_ADDR        _IOR('M', 0x2, unsigned)

#endif
