/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Shi.Ma <shi.ma@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DT_BINDINGS_MMC_AC8257_H
#define _DT_BINDINGS_MMC_AC8257_H

#define MSDC_EMMC               (0)
#define MSDC_SD                 (1)
#define MSDC_SDIO               (2)

#define MSDC_CD_HIGH            (1)
#define MSDC_CD_LOW             (0)

#define MSDC0_CLKSRC_26MHZ      (0)
#define MSDC0_CLKSRC_400MHZ     (1)
#define MSDC0_CLKSRC_182MHZ     (2)
#define MSDC0_CLKSRC_80MHZ      (3)
#define MSDC0_CLKSRC_312MHZ     (4)
#define MSDC0_CLKSRC_272MHZ     (5)
#define MSDC0_CLKSRC_250MHZ     (6)
#define MSDC0_CLKSRC_156MHZ     (7)

#define MSDC1_CLKSRC_26MHZ      (0)
#define MSDC1_CLKSRC_200MHZ     (1)
#define MSDC1_CLKSRC_208MHZ     (2)
#define MSDC1_CLKSRC_182MHZ     (3)
#define MSDC1_CLKSRC_136MHZ     (4)
#define MSDC1_CLKSRC_156MHZ     (5)
#define MSDC1_CLKSRC_48MHZ      (6)
#define MSDC1_CLKSRC_91MHZ      (7)

#define MSDC_SMPL_RISING        (0)
#define MSDC_SMPL_FALLING       (1)
#define MSDC_SMPL_SEPARATE      (2)

#define MSDC_DRIVE_2mA  0
#define MSDC_DRIVE_4mA  1
#define MSDC_DRIVE_6mA  2
#define MSDC_DRIVE_8mA  3
#define MSDC_DRIVE_10mA 4
#define MSDC_DRIVE_12mA 5
#define MSDC_DRIVE_14mA 6
#define MSDC_DRIVE_16mA 7


#endif /* _DT_BINDINGS_MMC_MT6761_H */
