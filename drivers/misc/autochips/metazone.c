/*
 * AutoChips metazone, kernel API (stock CONFIG_ATC_METAZONE: MTZ_Init, MetaZone_Read,
 * MetaZone_ReadBinary, MetaZone_SpecWriteBinary, MetaZone_Flush, /dev/mtz...).
 *
 * First step: the functions the vendor WLAN module (wlan_drv_gen4m.ko) imports, with the stock
 * prototypes and the stock error value (0x80000000), so that the module loads; the data itself is not
 * read yet (the WLAN driver then falls back to its NVRAM/default settings). The reading of the
 * metazone the LK loads into the autochips,metazone reserved memory, and /dev/mtz for rotationd, come
 * next.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>

#define MTZ_ERR		0x80000000	/* stock "failed" value */

int MetaZone_ReadBinary(u32 idx, void *buf, u32 len)
{
	pr_info_once("metazone: ReadBinary(0x%x) not implemented yet\n", idx);
	return MTZ_ERR;
}
EXPORT_SYMBOL(MetaZone_ReadBinary);

int MetaZone_SpecWriteBinary(u32 idx, void *buf, u32 len, u32 flag)
{
	pr_info_once("metazone: SpecWriteBinary(0x%x) not implemented yet\n", idx);
	return MTZ_ERR;
}
EXPORT_SYMBOL(MetaZone_SpecWriteBinary);

int MetaZone_Flush(u32 flag)
{
	return 0;
}
EXPORT_SYMBOL(MetaZone_Flush);
