/*************************************************************************
    > File Name: drivers/misc/autochips/tvd_new/src/tvd_if_priv.h
    > Author: atc0176
    > Mail: xinquan.bian@autochips.com 
    > Created Time: Wed 10 Oct 2018 05:54:20 PM CST
 ************************************************************************/

#ifndef __TVD_IF_PRIV_H__
#define __TVD_IF_PRIV_H__

#include "tvd_hal.h"

typedef struct {
    TVD_REG_BASE reg_base;
    int tvd_irq;
    unsigned long tdc_dram_addr;
} TVD_HW_RESOURCE;

void tvd_module_context_init(const TVD_HW_RESOURCE* resource);
void tvd_module_context_deinit(void);

#endif

