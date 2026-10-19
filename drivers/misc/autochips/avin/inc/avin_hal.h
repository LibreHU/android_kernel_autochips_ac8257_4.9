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



#ifndef _AVIN_HAL_H_
#define _AVIN_HAL_H_

extern int avin_tvd_init(void);
extern int avin_tvd_start(void);
extern int avin_tvd_stop(void);
extern int avin_tvd_deinit(void);
extern int avin_nr_deinit(void);
extern int avin_di_init(unsigned int width ,unsigned int height);
extern int avin_di_deinit(void);
extern int avin_di_set_fd(int *pfd, int fd_num);

#endif

