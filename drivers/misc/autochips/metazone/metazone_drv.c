/* Copyright Statement:
*
* This software/firmware and related documentation ("AutoChips Software") are
* protected under relevant copyright laws. The information contained herein is
* confidential and proprietary to AutoChips Inc. and/or its licensors. Without
* the prior written permission of AutoChips inc. and/or its licensors, any
* reproduction, modification, use or disclosure of AutoChips Software, and
* information contained herein, in whole or in part, shall be strictly
* prohibited.
*
* AutoChips Inc. (C) 2019. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AUTOCHIPS SOFTWARE")
* RECEIVED FROM AUTOCHIPS AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
* ON AN "AS-IS" BASIS ONLY. AUTOCHIPS EXPRESSLY DISCLAIMS ANY AND ALL
* WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
* NONINFRINGEMENT. NEITHER DOES AUTOCHIPS PROVIDE ANY WARRANTY WHATSOEVER WITH
* RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
* INCORPORATED IN, OR SUPPLIED WITH THE AUTOCHIPS SOFTWARE, AND RECEIVER AGREES
* TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
* RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
* OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN AUTOCHIPS
* SOFTWARE. AUTOCHIPS SHALL ALSO NOT BE RESPONSIBLE FOR ANY AUTOCHIPS SOFTWARE
* RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
* STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AUTOCHIPS'S
* ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE AUTOCHIPS SOFTWARE
* RELEASED HEREUNDER WILL BE, AT AUTOCHIPS'S OPTION, TO REVISE OR REPLACE THE
* AUTOCHIPS SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
* CHARGE PAID BY RECEIVER TO AUTOCHIPS FOR SUCH AUTOCHIPS SOFTWARE AT ISSUE.
*/



#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/sched.h>

#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <asm/io.h>

#include <asm/uaccess.h>
#include <asm/page.h>


#include "metazone_inter.h"
#include "metazone.h"
#include "metazone_ioctl.h"
#include <linux/compat.h>


#define KERNEL_STANDARD_API

#ifdef KERNEL_STANDARD_API
#include <linux/kthread.h>
//#include "x_os.h"	/* event operatioen API define header file */

static struct task_struct *mtz_thread_task;
#endif

#define MTZ_MODE_NAME		("MTZ")
#define MTZ_VER_MAJOR		(01)
#define MTZ_VER_MINOR		(01)
#define MTZ_VER_REV			(00)
#define MTZ_MISC_DVR_NAME	("mtz") // /dev/mtz
#define MTZ_MAX_SIZE		(0x200000)	// Phy storage media size for each metazone, in byte

#define MTZ_PART_HEAD_SIZE  (0x200) //512B
struct resvd_mem_info {
	unsigned long phys_addr;
	unsigned long virt_addr;
	unsigned long size;
};

unsigned long reser_phys_addr;
unsigned long reser_size;

typedef struct mtz_dev_info {
	struct miscdevice cdev;	/* Char device structure */
	struct device *dev;
	struct resvd_mem_info reserved_mem_info;
} mtzdev_info;

typedef struct {
	unsigned int InSize;
	unsigned int OutSize;
	u32 pInBuf;
	u32 pOutBuf;
	u32 pBytesReturned;
} MTZ_IOCTL_DATA_32;

struct resvd_mem_info *g_mtz_rsvmem_info = NULL;

#ifdef NAND_BOOT
int mtd_write_metazone(unsigned char *pbBuffer, unsigned int u4Size);
#endif

#define ATC_KERNEL_LINUX_LICENSE     "GPL"
#define MTZ_DRV_THREAD_STACK_SIZE	(2)

#ifdef KERNEL_STANDARD_API
#define MTZ_DRV_THREAD_PRIORITY			(81)
#else
#define MTZ_DRV_THREAD_PRIORITY			PRIORITY(PRIORITY_CLASS_REALTIME, PRIORITY_LAYER_TIME_CRITICAL, 0)
#endif

bool _fgWritableZoneInited = false;
void *_pMetaZone = NULL;
TMetaZone *_pMetaHeader = NULL;
TMTZResHead *_pMtzResHeader = NULL;
char *_pbReserve = NULL;
char *_pu4Value = NULL;
char *_u4BinaryStart = 0;
//char *_pMdpFlag1 = NULL;
//char *_pMdpFlag2 = NULL;
static bool _fgWritableZoneDirty;

#define USE_RESERVED_MEMORY			(1) //0:dynamic  1:static   reserved memory allocate in dts

#ifndef KERNEL_STANDARD_API
static HANDLE_T _hMtzFlushThread;
#endif

#define METAZONE_FS_SIZE			(0x10000)	/* 64KB */
#define METAZONE_FS_BINARY_OFFSET	(10240)

//static u32 _u4FsBinaryStart;
static void *_pFsMetaZone;
static bool _fgFileZoneInited;
static struct mutex _Lock;
static struct semaphore _rSema;
//static void *_hEvtFlushFinished;
#define FLUSH_EVENT				("MTZ_FLUSH")

#define Lock()		mutex_lock(&_Lock)
#define Unlock()	mutex_unlock(&_Lock)

#define MTZ_PARTITION  "/dev/block/platform/bootdevice/by-name/metazone"

Wflag p1 = {0,0};
Wflag p2 = {0,0};

static char *get_Wflag_addr(int part)
{
	char *ret = NULL;
	if (part == 1)
		ret = (char *)(_pMetaZone + MTZ_PARTITION_SIZE / 2 - MTZ_HERDER_OFF -  MTZ_CONFIG_AREA_SIZE);
	else if (part == 2)
		ret = (char *)(_pMetaZone + MTZ_PARTITION_SIZE - MTZ_HERDER_OFF -  MTZ_CONFIG_AREA_SIZE);

	return ret;
}

static int update_wflag(unsigned char sector,unsigned int offset)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	int ret = 0;
	char *pbBuffer = NULL;
	unsigned u4Size = sizeof(Wflag);

	fp = filp_open(MTZ_PARTITION, O_RDWR | O_SYNC, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open %s file error %d\n",MTZ_PARTITION, (int)PTR_ERR(fp));
		return -1;
	}
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (sector==1) {
		pbBuffer = get_Wflag_addr(1);
		if (offset==1)
			(p1.write_before)++;
		else if (offset==2)
			(p1.write_after)++;
		memcpy_toio(pbBuffer, &p1, sizeof(Wflag));
		pos = (MTZ_PARTITION_SIZE/2)-MTZ_CONFIG_AREA_SIZE;
	} else if (sector==2) {
		pbBuffer = get_Wflag_addr(2);
		if (offset==1)
			(p2.write_before)++;
		else if (offset==2)
			(p2.write_after)++;
		memcpy_toio(pbBuffer, &p2, sizeof(Wflag));
		pos = MTZ_PARTITION_SIZE-MTZ_CONFIG_AREA_SIZE;
	}
	ret = vfs_write(fp, pbBuffer, u4Size, &pos);
	if (ret < 0)
		pr_err("[MTZ] write wflag to emmc fail!\n");

	filp_close(fp, NULL);
	set_fs(old_fs);

	return 0;
}


static unsigned short calculate_CRC16(unsigned char* pbuff, unsigned short length)
{
	unsigned short shift, data=0, val=0;
	int i=0;
	shift = 0xffff;
	if(pbuff ==NULL || length <=0){
		pr_err("[MTZ] calculate_CRC input is invalid \n");
		return -1;
	}
	for(i = 0; i< length; i++){
		if(0 == (i%8))
		data = (*pbuff++) << 8;
		val = shift ^ data;
		shift = shift << 1;
		data = data << 1;
		if(val & 0x8000)
			shift = shift ^ 0x1021;
	}

	return shift;
}

static int recount_CRC_value(int type)
{
	unsigned short reCRC = 0;
	char *position = NULL;
	char *pbBuffer = NULL;
	char *pbCRCstart = (char *)(_pMetaZone + MTZ_PARTITION_SIZE / 2 - MTZ_HERDER_OFF -  MTZ_CONFIG_AREA_SIZE + sizeof(Wflag));

	switch(type){
		case 1:
			reCRC = calculate_CRC16((_pu4Value + MTZ_DWORD_SIZE) , (unsigned short)(_pMetaHeader->dwValueNum -1)*MTZ_DWORD_SIZE);
			position = pbCRCstart;
			pr_info("[MTZ] Dword CRC recount (%u)\n", reCRC);
			goto update;

		case 2:
			reCRC = calculate_CRC16((char *)_u4BinaryStart , (unsigned short)(((_pMetaHeader->dwBinaryItemSize)*2 + 6)
							* _pMetaHeader->dwBinaryNum));
			position = pbCRCstart + 2;
			pr_info("[MTZ] Binary CRC recount (%u)\n", reCRC);
			goto update;

		case 3:
			reCRC = calculate_CRC16(_pbReserve , (unsigned short)(_pMetaHeader->dwReserveSize));
			position = pbCRCstart + 4;
			pr_info("[MTZ] Reserved CRC recount (%u)\n", reCRC);
			goto update;

		default:
			pr_err("[MTZ] CRC recount type [%d] is wrong \n", type);
			return -1;
	}

update:

	pbBuffer = (char *)(&reCRC);
	memcpy_toio(position, pbBuffer, 2);

	return 0;
}

int Metazone_check_CRC_flag(void)
{
	unsigned short reCRC = 0;
	CRCflag CRC;
	char *pbCRCstart = (char *)(_pMetaZone + MTZ_PARTITION_SIZE / 2 - MTZ_HERDER_OFF -  MTZ_CONFIG_AREA_SIZE + sizeof(Wflag));

	memcpy(&CRC, pbCRCstart, sizeof(CRCflag));
	reCRC = calculate_CRC16((_pu4Value + MTZ_DWORD_SIZE), (unsigned short)(_pMetaHeader->dwValueNum - 1) * MTZ_DWORD_SIZE);
	if (CRC.crc_dword != reCRC) {
		pr_err("[MTZ]Check CRC dwValue is wrong:%d ,%d\n", CRC.crc_dword, reCRC);
		return -1;
	}

	reCRC = calculate_CRC16((char *)_u4BinaryStart, (unsigned short)((_pMetaHeader->dwBinaryNum) * MTZ_BINARY_SIZE));
	if (CRC.crc_binary != reCRC) {
		pr_err("[MTZ] Check CRC dwBinary is wrong \n");
		return -1;
	}

	reCRC = calculate_CRC16(_pbReserve, (unsigned short)(_pMetaHeader->dwReserveSize));
	if (CRC.crc_reserved != reCRC) {
		pr_err("[MTZ] Check CRC dwReserved is wrong \n");
		return -1;
	}

	return 0;
}

#if 0 //metazone enhancement
static int set_MDP_ready_flag(int dataflag) // set data ready flag in memory
{
    char *pReadyFlag1 = NULL;
	char *pReadyFlag2 = NULL;
    unsigned int readyflag = 1; // ready flag 1

    if(!_fgWritableZoneInited){
        return MZ_FAILURE;
	}

	pReadyFlag1 = (char *)((unsigned int)_pMetaZone + MTZ_PARTITION_SIZE/2 -MTZ_HERDER_OFF-
		                          MTZ_CONFIG_AREA_SIZE + sizeof(Wflag)+ sizeof(CRCflag));
	pReadyFlag2 = pReadyFlag1 + 4;

    switch(dataflag){
	  case MDP_FLAG_1:
	     Lock();
		 memcpy(pReadyFlag1, &readyflag, 4); //set MDP 1 ready flag
		 Unlock();
	     break;
      case MDP_FLAG_2:
		 Lock();
		 memcpy(pReadyFlag2, &readyflag, 4); //set MDP 2 ready flag
	     Unlock();
	     break;
	  default:
		 pr_err("[MTZ] set_MDP_ready_flag [%d] is invalid \n", dataflag);
		 return -1;
    }

	return 0;
}
#endif

#ifdef NAND_BOOT
int mtd_write_metazone(unsigned char *pbBuffer, unsigned int u4Size)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	int ret = 0;

	fp = filp_open("/dev/mtdblock14", O_RDWR, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open /dev/mtdblock14 file error\n");
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0x0;
	ret = vfs_write(fp, pbBuffer, u4Size, &pos);

	pos = 0x200000;
	ret = vfs_write(fp, pbBuffer, u4Size, &pos);

	filp_close(fp, NULL);
	set_fs(old_fs);

	return 0;
}

static int mtd_read_metazone(char *pbBuffer, unsigned long u4Size)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	int ret = 0;

	fp = filp_open("/dev/mtdblock14", O_RDWR, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open /dev/mtdblock14 error\n");
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0x0;
	ret = vfs_read(fp, pbBuffer, u4Size, &pos);
	if (ret != u4Size) {
		pr_err("[MTZ] read metazone from /dev/mtdblock14 fail\n");
		filp_close(fp, NULL);
		set_fs(old_fs);
		return -1;
	}

	filp_close(fp, NULL);
	set_fs(old_fs);

	return 0;
}

#else

static int update_CRC_flag(int i)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	int ret = 0;
	char *pbBuffer = NULL;
	char *pbCRCstart = NULL;
	unsigned u4Size = sizeof(CRCflag);

	pbBuffer = (char *)kmalloc(u4Size, GFP_KERNEL);
	if (!pbBuffer) {
		pr_err("[MTZ]kmalloc pbBuffer fail \n");
		return -1;
	}
	memset(pbBuffer, 0, u4Size);

	fp = filp_open(MTZ_PARTITION, O_RDWR | O_SYNC, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open %s file error\n",MTZ_PARTITION);
		kfree(pbBuffer);
		pbBuffer = NULL;
		return -1;
	}
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if(1 == i){
		pbCRCstart = (char *)(_pMetaZone + MTZ_PARTITION_SIZE/2 - MTZ_HERDER_OFF - MTZ_CONFIG_AREA_SIZE + sizeof(Wflag));
		memcpy_toio(pbBuffer, pbCRCstart, sizeof(CRCflag));

		pos = (MTZ_PARTITION_SIZE/2)-MTZ_CONFIG_AREA_SIZE+sizeof(Wflag);
		ret = vfs_write(fp, pbBuffer, u4Size, &pos);
	}
	else if(2 == i){
		pbCRCstart = (char *)(_pMetaZone + MTZ_PARTITION_SIZE/2 - MTZ_HERDER_OFF - MTZ_CONFIG_AREA_SIZE + sizeof(Wflag));
		memcpy_toio(pbBuffer, pbCRCstart, sizeof(CRCflag));

		pos = MTZ_PARTITION_SIZE-MTZ_CONFIG_AREA_SIZE+sizeof(Wflag);
		ret = vfs_write(fp, pbBuffer, u4Size, &pos);
	}

	filp_close(fp, NULL);
	set_fs(old_fs);
	kfree(pbBuffer);
	pbBuffer = NULL;

	return 0;

}


static int write_metazone1(const char *pbBuffer, unsigned long u4Size)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	int ret = 0;

	fp = filp_open(MTZ_PARTITION, O_RDWR | O_SYNC, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open %s file error\n",MTZ_PARTITION);
		return -1;
	}
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0x0; //512B
	ret = vfs_write(fp, pbBuffer, u4Size, &pos);

	filp_close(fp, NULL);
	set_fs(old_fs);

	return 0;
}

static int write_metazone2(const char *pbBuffer, unsigned long u4Size)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	int ret = 0;

	fp = filp_open(MTZ_PARTITION, O_RDWR | O_SYNC, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open %s file error\n",MTZ_PARTITION);
		return -1;
	}
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0x0 + (MTZ_PARTITION_SIZE / 2); // 512KB
	ret = vfs_write(fp, pbBuffer, u4Size, &pos);

	filp_close(fp, NULL);
	set_fs(old_fs);

	return 0;
}

static int mmc_write_metazone(const char *pbBuffer, unsigned long u4Size)
{
	/*before write to metazone 1*/
	update_wflag(1,1);

	write_metazone1(pbBuffer,u4Size);
	/*after write to metazone 1*/
	update_CRC_flag(1);

	update_wflag(1,2);

	pr_info("[MTZ] metazone update_wflag 1 \n");
	/*before write to metazone 2*/
	update_wflag(2,1);

	write_metazone2(pbBuffer,u4Size);
	/*after write to metazone 2*/
	update_CRC_flag(2);

	update_wflag(2,2);
	pr_info("[MTZ] metazone update_wflag 2 \n");

	return 0;
}


static int mmc_read_metazone(char *pbBuffer, unsigned long u4Size)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	int ret = 0;
	unsigned long size = u4Size;

	fp = filp_open(MTZ_PARTITION, O_RDWR, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open %s file error\n",MTZ_PARTITION);
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0x0;
	ret = vfs_read(fp, pbBuffer, size, &pos);
	if (ret != size) {
		pr_err("[MTZ] read metazone from %s fail\n",MTZ_PARTITION);
		filp_close(fp, NULL);
		set_fs(old_fs);
		return -1;
	}

	filp_close(fp, NULL);
	set_fs(old_fs);

	return 0;
}


#if 0
static int mmc_write_mdp(const char *pbBuffer, unsigned long u4Size)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	loff_t pos;
	int ret = 0;
	char* pMDPreadyflag = NULL;

	if (NULL==pbBuffer) {
        pr_err("[MTZ] mmc_write_mdp input is invalid\n");
		return -1;
	}

	fp = filp_open(MTZ_PARTITION, O_RDWR, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open %s file error\n",MTZ_PARTITION);
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pMDPreadyflag = (char *)((unsigned int)_pMetaZone + MTZ_PARTITION_SIZE/2 -
		                         MTZ_HERDER_OFF-MTZ_CONFIG_AREA_SIZE + sizeof(Wflag)+ sizeof(CRCflag));

	pos = MTZ_PARTITION_SIZE/4; // 256KB,  1/4 of partition
	ret = vfs_write(fp, pbBuffer, u4Size, &pos);

	pos = (MTZ_PARTITION_SIZE/2)-MTZ_CONFIG_AREA_SIZE+sizeof(Wflag)+sizeof(CRCflag); // the first 2 mdp ready flag
	ret = vfs_write(fp, pMDPreadyflag, 2*sizeof(int), &pos);

	pos = MTZ_PARTITION_SIZE/2 + MTZ_PARTITION_SIZE/4;  //768KB, 3/4 of partition
	ret = vfs_write(fp, pbBuffer, u4Size, &pos);

	pos = MTZ_PARTITION_SIZE-MTZ_CONFIG_AREA_SIZE+sizeof(Wflag)+sizeof(CRCflag); // the mdp ready flag backup
	ret = vfs_write(fp, pMDPreadyflag, 2*sizeof(int), &pos);

	filp_close(fp, NULL);
	set_fs(old_fs);

	return 0;
}
#endif

#endif

#if 0
static int mmc_compare_metazone(void)
{
	struct file *fp;
	mm_segment_t old_fs;
	loff_t pos = 0;
	int ret = 0;
	char *mtz_buf1 = NULL;
	char *mtz_buf2 = NULL;

	fp = filp_open("/dev/metazone", O_RDWR, 0644U);
	if (IS_ERR(fp)) {
		pr_err("[MTZ] open /dev/block/metazone file error\n");
		return -1;
	}

	// Allocate buffer for metazone read
	mtz_buf1 = kzalloc(MTZ_MAX_SIZE, GFP_KERNEL);
	mtz_buf2 = kzalloc(MTZ_MAX_SIZE, GFP_KERNEL);
	if (!mtz_buf1 || !mtz_buf2) {
		pr_err("[MTZ]allocat buffer for read metazone fail\r\n");
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	pos = 0x0;
	ret = vfs_read(fp, mtz_buf1, MTZ_MAX_SIZE, &pos);

	pos = 0x200000;
	ret = vfs_read(fp, mtz_buf2, MTZ_MAX_SIZE, &pos);

	filp_close(fp, NULL);
	set_fs(old_fs);

	// Compare, TODO

	// If they are diff, sync it. TODO

	// Release buffer
	kfree(mtz_buf1);
	kfree(mtz_buf2);
	mtz_buf1 = NULL;
	mtz_buf2 = NULL;

	return 0;
}
#endif

static bool MTZ_WriteWritableZone(void)
{
//	char *pbBuffer = NULL;

	if (_fgWritableZoneInited && _fgWritableZoneDirty) {
		//u32 *pu4Signature;
#if 0
		//(*((u32 *)(_pu4Value + 2)))++;
		Lock();
		pbBuffer = (char *)kmalloc(METAZONE_SIZE_MAX, GFP_KERNEL);
		if (pbBuffer) {
			int cpSize =
			    (_pMetaHeader->dwDataSize <
			     (METAZONE_SIZE_MAX-MTZ_HERDER_OFF)) ? _pMetaHeader->dwDataSize : (METAZONE_SIZE_MAX-MTZ_HERDER_OFF);
			memcpy(pbBuffer, _pMetaZone, cpSize);
		}
		_fgWritableZoneDirty = false;
		Unlock();
		if (!pbBuffer) {
			pr_err("[MTZ] Failed to kmalloc memory for Save MetaZone !!!\n");
			return false;
		}

#endif
		//pu4Signature = (u32 *) (pbBuffer + METAZONE_SIZE_MAX - 4);
		//*pu4Signature = METAZONE_SIGNATURE;
		Lock();
		_fgWritableZoneDirty = false;
		pr_info("[MTZ] flush metazone reserved memory to emmc.\n");
#ifdef NAND_BOOT
		mtd_write_metazone((unsigned char *)g_mtz_rsvmem_info->virt_addr, METAZONE_SIZE_MAX);
#else
		mmc_write_metazone((unsigned char *)g_mtz_rsvmem_info->virt_addr, METAZONE_SIZE_MAX);
#endif
		Unlock();

//		kfree(pbBuffer);
//		pbBuffer = NULL;
	}
	return true;
}


static bool MTZ_InitWritableZone(void)
{
#if !USE_RESERVED_MEMORY
	int ret = 0;
	PTMetaZone pMetazoen = NULL;
#endif

	if (!_fgWritableZoneInited) {
		pr_info("[MTZ] MTZ_InitWritableZone\n");

#if !USE_RESERVED_MEMORY

		if (g_mtz_rsvmem_info == NULL) {
			pr_err("[MTZ]reserved memory has not been initilized\r\n");
			return false;
		}
#ifdef NAND_BOOT
		ret = mtd_read_metazone((char *)g_mtz_rsvmem_info->virt_addr, g_mtz_rsvmem_info->size);
#else
		ret = mmc_read_metazone((char *)g_mtz_rsvmem_info->virt_addr, g_mtz_rsvmem_info->size);
#endif
		if (ret != 0) {
			pr_err("[MTZ]read metazone fail\r\n");
			return false;
		}

		pMetazoen = (PTMetaZone)(g_mtz_rsvmem_info->virt_addr + MTZ_HERDER_OFF);

		if (METAZONE_SIGNATURE != pMetazoen->dwSignature) {
			pr_err("[MTZ] MetaZone Signature is invalid!!!\r\n");
			return false;
		}
		if (METAZONE_VERSION != pMetazoen->dwVersion) {
			pr_err("[MTZ] MetaZone version is invalid!!!\r\n");
			return false;
		}
		if (METAZONE_SIZE_MAX < pMetazoen->dwDataSize) {
			pr_err("[MTZ] MetaZone siz is out of range!!!\r\n");
			return false;
		}
		if ((pMetazoen->dwReserveSize + (pMetazoen->dwValueNum * 4) +
			((pMetazoen->dwBinaryItemSize + 4) * pMetazoen->dwBinaryNum)) >= pMetazoen->dwDataSize) {
			pr_err("[MTZ] Calculated size is out of range!!!\n");
			return false;
		}
		pr_info("[MTZ] Writabe metazone size is %d.\n", pMetazoen->dwDataSize);

		_pMetaZone = (void *)(g_mtz_rsvmem_info->virt_addr + MTZ_HERDER_OFF);
		_pMetaHeader = (TMetaZone *) _pMetaZone;
		_pbReserve = (char *)((unsigned int) _pMetaZone + sizeof(TMetaZone));
		pr_info("[MTZ] sizeof(TMetaZone) is [%d] \n",sizeof(TMetaZone));
		_pu4Value = (unsigned int *) ((unsigned int) _pMetaZone + _pMetaHeader->dwValueOffset);
		_u4BinaryStart = (unsigned int) _pMetaZone + _pMetaHeader->dwBinaryOffset;

		//_pMdpFlag1 = (char *)((unsigned int) _pMetaZone + MTZ_MDP_OFFSET_Flag_1-MTZ_HERDER_OFF); // mdp data 1 addr
		//_pMdpFlag2 = (char *)((unsigned int) _pMetaZone + MTZ_MDP_OFFSET_Flag_2-MTZ_HERDER_OFF); // mdp data 2 addr

#else
		//_pMetaZone = (void *)MEMRSV_PHY_TO_VIRT(METAZONE_MEM_PA);
		_pMetaZone = (void *)(g_mtz_rsvmem_info->virt_addr + MTZ_HERDER_OFF); // already get address in metazone_probe
		_pMetaHeader = (TMetaZone *) _pMetaZone;
		_pbReserve = (char *)((char *) _pMetaZone + _pMetaHeader->dwReserveOffset);
		_pMtzResHeader = (TMTZResHead *)_pbReserve;
		_pu4Value = (char *) ((char *) _pMetaZone + _pMetaHeader->dwValueOffset);
		_u4BinaryStart = (char *)( (char *)_pMetaZone + _pMetaHeader->dwBinaryOffset);

		//_pMdpFlag1 = (char *)((unsigned int) _pMetaZone + MTZ_MDP_OFFSET_Flag_1-MTZ_HERDER_OFF); // mdp data 1 addr
		//_pMdpFlag2 = (char *)((unsigned int) _pMetaZone + MTZ_MDP_OFFSET_Flag_2-MTZ_HERDER_OFF); // mdp data 2 addr
#endif
		if ((METAZONE_SIGNATURE != _pMetaHeader->dwSignature)
			|| (METAZONE_VERSION != _pMetaHeader->dwVersion)) {
			pr_err("[MTZ] WritableZone signature or version is invalid.\n");
			return false;
		}
		_fgWritableZoneInited = true;

		pr_info("[MTZ] WritableZone Infomation:\n");
		pr_info("[MTZ] \tDataSize = 0x%x, Binary Item Max Size = %d\n",
			_pMetaHeader->dwDataSize, _pMetaHeader->dwBinaryItemSize);
		pr_info("[MTZ] \tDWORD Value Number = %d, offset = 0x%x\n", _pMetaHeader->dwValueNum,
			_pMetaHeader->dwValueOffset);
		pr_info("[MTZ] \tBinary Value Number = %d, offset = 0x%x\n",
			_pMetaHeader->dwBinaryNum, _pMetaHeader->dwBinaryOffset);
		pr_info("[MTZ] MTZ_InitWritableZone End\n");

	}
	return _fgWritableZoneInited;
}


static bool MTZ_InitFlashDriver(void)
{
	// Load metazone data from storage media;
	if (!MTZ_InitWritableZone()) {
		pr_err("[MTZ]read metazone from storage media fail\r\n");;
	}
	return _fgWritableZoneInited;
}

static int mtz_flush_thread_func(void *pvArg)
{
	pr_info("[MTZ]metazone flush thread start\n");
	while (true) {
		down(&_rSema);
		MTZ_WriteWritableZone();
		//if (_hEvtFlushFinished)
		//	x_event_set(_hEvtFlushFinished);
	}
	pr_info("[MTZ]metazone flush thread end\n");
	return 0;
}



/* ------------------------------------------------------------------------------ */
/* Local Functions */
/* ------------------------------------------------------------------------------ */


bool MTZ_Deinit(void)
{
	if (!_pMetaZone) {
#if !USE_RESERVED_MEMORY
		kfree(_pMetaZone);
#endif
		_pMetaZone = NULL;
		_fgWritableZoneInited = false;
	}

	if (!_pFsMetaZone) {
		kfree(_pFsMetaZone);
		_pFsMetaZone = NULL;
		_fgFileZoneInited = false;
	}

	#ifdef KERNEL_STANDARD_API
	if (mtz_thread_task) {
		kthread_stop(mtz_thread_task);
		mtz_thread_task = NULL;
	}
	#endif

	return true;
}

/* ----------------------------------------------------------------------------- */
/* FUNCTION */
/* MTZ_Init */
/* DESCRIPTION */
/* This function is used to init MetaZone Driver. */
/* PARAMETERS */
/* none */
/* RETURNS */
/* Return true for success; otherwise, the operation fails. */
/* RETURN VALUE LIST */
/* none */
/* ---------------------------------------------------------------------------- */
#ifdef KERNEL_STANDARD_API

int MTZ_Init(void)
{
	int ret = 0;
	char *t_name = "MTZ_Thread";
	struct sched_param param;

	pr_info("[MTZ] MTZ_Init.\n");

	mutex_init(&_Lock);
	sema_init(&_rSema, 0);
	//_hEvtFlushFinished = x_event_create(NULL, true, false, FLUSH_EVENT);

	#ifdef NAND_BOOT
	//If use NandFlash, we need init it.
	MTZ_InitFlashDriver();
	#endif

	// Load metazone data from storage media;
	if (!MTZ_InitWritableZone()) {
		pr_err("[MTZ]read metazone from storage media fail\r\n");;
	}

	mtz_thread_task = kthread_create(&mtz_flush_thread_func, NULL, t_name);
	if (IS_ERR(mtz_thread_task)) {
		pr_err("[MTZ]create metazone flush thread failed.\n");
		return -1;
	}

	param.sched_priority = MTZ_DRV_THREAD_PRIORITY;
	ret = sched_setscheduler_nocheck(mtz_thread_task, SCHED_RR, &param);
	wake_up_process(mtz_thread_task);

	pr_info("[MTZ] MTZ_Init End.\n");
	return METAZONE_SIGNATURE;
}

#else

int MTZ_Init(void)
{
	char *t_name = "MTZ_Thread";

	pr_info("[MTZ] MTZ_Init.\n");

	mutex_init(&_Lock);
	sema_init(&_rSema, 0);
	_hEvtFlushFinished = CreateEvent(NULL, true, false, FLUSH_EVENT);

	#ifdef NAND_BOOT
	//If use NandFlash, we need init it.
	MTZ_InitFlashDriver();
	#endif

	MTZ_InitWritableZone();
	if (!x_thread_create(&_hMtzFlushThread, t_name, MTZ_DRV_THREAD_STACK_SIZE,
				 MTZ_DRV_THREAD_PRIORITY, Flush_Thread, 0, NULL))
		pr_err("[MTZ] create metazone flush thread failed.\n");

	pr_info("[MTZ] MTZ_Init End.\n");
	return METAZONE_SIGNATURE;
}

#endif

/* ----------------------------------------------------------------------------- */
/* FUNCTION */
/* MTZ_Open */
/* DESCRIPTION */
/*  */
/* PARAMETERS */
/*  */
/* RETURNS */
/* Return true for success; otherwise, the operation fails. */
/* RETURN VALUE LIST */
/* none */
/* ---------------------------------------------------------------------------- */
unsigned long MTZ_Open(unsigned int context, unsigned int accessCode, unsigned int shareMode)
{
	if (MTZ_InitFlashDriver())
		return context;

	pr_err("[MTZ] MTZ_Open failed!\n");
	return 0;
}


/* ----------------------------------------------------------------------------- */
/* FUNCTION */
/* MTZ_Close */
/* DESCRIPTION */
/*  */
/* PARAMETERS */
/*  */
/* RETURNS */
/* Return true for success; otherwise, the operation fails. */
/* RETURN VALUE LIST */
/* none */
/* ---------------------------------------------------------------------------- */
bool MTZ_Close(unsigned int context)
{
	return true;
}

/* 0:Have/no data   1:Factory reset    2 ~ 15:NULL */
bool Metazone_Get_Flag(char *pFlag, int index)
{
	return (*pFlag >> index) & 0x01;
}

void Metazone_Set_Flag(char *pFlag, int index, bool flag)
{
	*pFlag &= ~(1<<index);
	*pFlag |= (flag<<index);
	pr_info("[MTZ]set flag is  %d %d \n", index, flag);
}

u32 MetaZone_Read(unsigned int u4Idx, unsigned int *pu4Data)
{
	pr_info("[MTZ] DW Read u4Idx(0x%x).\n", u4Idx);

	if (MZ_FS_IDX_START <= u4Idx) {
		return MZ_FAILURE;
	}

	if (MZ_WR_IDX_START <= u4Idx )
	{
		if (!_fgWritableZoneInited)
		{
			pr_err("[MTZ] DW Read Failed. (No MetaZone).\n");
			return (MZ_NO_INIT);
		}
		u4Idx -= MZ_WR_IDX_START;
		if (u4Idx < _pMetaHeader->dwValueNum)
		{
			char *pbflag = (char *)(_pu4Value + u4Idx * MTZ_DWORD_SIZE + 1);
			if(!Metazone_Get_Flag(pbflag, 0))
			{
				pr_err("[MTZ]DW read u4Idx(0x%x) no data!", u4Idx);
				return (MZ_FAILURE);
			}
			memcpy_fromio(pu4Data, (char *)(_pu4Value + u4Idx*MTZ_DWORD_SIZE + 2), sizeof(unsigned int));
			return (MZ_SUCCESS);
		}
		u4Idx += MZ_WR_IDX_START;
	}

	/* else.Send IOCTL to Flash driver to read DataZone. */

	pr_err("[MTZ] DW Read u4Idx(0x%x) out of range.\n", u4Idx);
	return MZ_FAILURE;
}
EXPORT_SYMBOL(MetaZone_Read);


unsigned int MetaZone_Write(unsigned int u4Idx, unsigned int u4Data)
{
	int ret = 0;
	pr_info("[MTZ] DW Write u4Idx(0x%x) data(0x%x)\n", u4Idx, u4Data);

	if (MZ_FS_IDX_START <= u4Idx) {
		return MZ_FAILURE;
	}

	if (Metazone_check_CRC_flag() != 0) {
		pr_err("[MTZ] Metazone CRC error, data broken.\n");
		ret = mmc_read_metazone((char *)g_mtz_rsvmem_info->virt_addr, g_mtz_rsvmem_info->size);
		if (ret != 0)
			pr_err("[MTZ]read metazone fail\r\n");

		return MZ_FAILURE;
	}

	if (MZ_WR_IDX_START <= u4Idx) {
		if (!_fgWritableZoneInited) {
			pr_err("[MTZ] DW Write Failed. (No MetaZone).\n");
			return MZ_NO_INIT;
		}
		u4Idx -= MZ_WR_IDX_START;
		if (u4Idx < _pMetaHeader->dwValueNum) {
			Lock();
			memcpy_toio((char *)(_pu4Value + u4Idx * MTZ_DWORD_SIZE + 2), &u4Data, sizeof(u32));
			Metazone_Set_Flag((char *)(_pu4Value + u4Idx * MTZ_DWORD_SIZE + 1), 0, true);
			_fgWritableZoneDirty = true;
			Unlock();
			recount_CRC_value(1); // recount Dword CRC value in memory

			return MZ_SUCCESS;
		}
		u4Idx += MZ_WR_IDX_START;
	}

	/* else Send IOCTL to Flash driver to write DataZone. */

	pr_err("[MTZ] DW Write u4Idx(0x%x) out of range.\n", u4Idx);
	return MZ_FAILURE;
}
EXPORT_SYMBOL(MetaZone_Write);


u32 MetaZone_ReadBinary(unsigned int u4Idx, char *pbData, unsigned int u4Size)
{

	u32 u4Tmp = 0;
	pr_info("[MTZ] Read Binary u4Idx(0x%x).\n", u4Idx);

	if (MZ_FS_IDX_START <= u4Idx) {
		pr_err("[MTZ] Read Binary Index Error: u4Idx(0x%x).\n", u4Idx);
		return MZ_FAILURE;
	}

	if (MZ_WR_IDX_START <= u4Idx) {
		if (!_fgWritableZoneInited)
			return MZ_NO_INIT;

		u4Idx -= MZ_WR_IDX_START;
		if (u4Idx < _pMetaHeader->dwBinaryNum)
		{
			char *pbflag = (char *)(_u4BinaryStart + u4Idx*(_pMetaHeader->dwBinaryItemSize * 2 + 6) + 1);
			if(!Metazone_Get_Flag(pbflag, 0))
			{
				pr_err("[MTZ] Read binary index u4Idx(0x%x) has no data!", u4Idx);
				return (MZ_FAILURE);
			}
			memcpy_toio(&u4Tmp, (char * )( _u4BinaryStart + (_pMetaHeader->dwBinaryItemSize * 2 + 6) * u4Idx + 2), sizeof(u32));
			printk("[metazone]u4Tmp(%d)\n", u4Tmp);
			if (u4Size > u4Tmp)
				u4Size = u4Tmp;
			memcpy_fromio(pbData,(char *)(_u4BinaryStart + (_pMetaHeader->dwBinaryItemSize*2+6) * u4Idx + 6), u4Size);
			return MZ_SUCCESS;
		}
		u4Idx += MZ_WR_IDX_START;
	}

	/* else Send IOCTL to Flash driver to read DataZone. */

	pr_err("[MTZ] Read Binary u4Idx(0x%x) out of range.\n", u4Idx);
	return MZ_FAILURE;
}
EXPORT_SYMBOL(MetaZone_ReadBinary);


unsigned int MetaZone_WriteBinary(unsigned int u4Idx, const char *pbData, unsigned int u4Size)
{
	int ret = 0;
	pr_info("[MTZ] Write Binary u4Idx(0x%x).\n", u4Idx);

	if (MZ_FS_IDX_START <= u4Idx) {
		pr_err("[MTZ] Write Binary Index Error: u4Idx(0x%x).\n", u4Idx);
		return MZ_FAILURE;
	}

	if (Metazone_check_CRC_flag() != 0) {
		pr_err("[MTZ] Metazone CRC error, data broken.\n");
		ret = mmc_read_metazone((char *)g_mtz_rsvmem_info->virt_addr, g_mtz_rsvmem_info->size);
		if (ret != 0)
			pr_err("[MTZ]read metazone fail\r\n");

		return MZ_FAILURE;
	}

	if (MZ_WR_IDX_START <= u4Idx) {
		if (!_fgWritableZoneInited) {
			pr_err("[MTZ] Write Binary Failed (No MetaZone).\n");
			return MZ_NO_INIT;
		}
		u4Idx -= MZ_WR_IDX_START;
		if (u4Idx < _pMetaHeader->dwBinaryNum) {
			if (u4Size > _pMetaHeader->dwBinaryItemSize)
				u4Size = _pMetaHeader->dwBinaryItemSize;
			Lock();
			memcpy_toio((char *) ( _u4BinaryStart + (_pMetaHeader->dwBinaryItemSize*2 + 6) * u4Idx + 2), &u4Size, sizeof(u32));
			memcpy_toio((char *)(_u4BinaryStart + (_pMetaHeader->dwBinaryItemSize*2 + 6) * u4Idx + 6), pbData, u4Size);
			Metazone_Set_Flag((char *)(_u4BinaryStart + (_pMetaHeader->dwBinaryItemSize*2 + 6) * u4Idx + 1), 0, true);
			_fgWritableZoneDirty = true;
			Unlock();
			recount_CRC_value(2);

			return MZ_SUCCESS;
		}
		u4Idx += MZ_WR_IDX_START;
	}

	/* else  Send IOCTL to Flash driver to read DataZone. */

	pr_err("[MTZ] Write Binary u4Idx(0x%x) out of range.\n", u4Idx);
	return MZ_FAILURE;
}
EXPORT_SYMBOL(MetaZone_WriteBinary);


static u32 _MetaZone_ReadInfo(PMETAZONE_INFO_T prInfo)
{
	memset((void *)prInfo, 0, sizeof(METAZONE_INFO_T));
	if (_fgWritableZoneInited) {
		prInfo->u4WrValueNum = _pMetaHeader->dwValueNum;
		prInfo->u4WrBinaryNum = _pMetaHeader->dwBinaryNum;
		prInfo->u4WrBinarySize = _pMetaHeader->dwBinaryItemSize;
		return MZ_SUCCESS;
	}

	return MZ_NO_INIT;
}


u32 MetaZone_ReadInfo(PMETAZONE_INFO_T prInfo)
{
	u32 u4Ret = 0;

	u4Ret = _MetaZone_ReadInfo(prInfo);
	prInfo->u4FsValueNum = MZ_FS_DWORD_NUM;
	prInfo->u4FsBinaryNum = MZ_FS_BINARY_NUM;
	prInfo->u4FsBinarySize = MZ_FS_BINARY_SIZE;
	return u4Ret;
}
EXPORT_SYMBOL(MetaZone_ReadInfo);

/*Discarding*/
/*unsigned int MetaZone_ReadReserved(char *pbData, unsigned int u4Size)
{
	pr_info("[MTZ] Read Reserved u4Size(%d).\n", u4Size);

	if (NULL==pbData) {
		pr_err("[MTZ] MetaZone_ReadReserved input is invalid\n");
		return MZ_FAILURE;
	}

	if (!_fgWritableZoneInited)
		return MZ_FAILURE;

	if(_pMtzResHeader->dwRFlag[0] == 0)
		return MZ_FAILURE;

	if (u4Size >  MTZ_RES_MAX_SIZE)
		u4Size =  MTZ_RES_MAX_SIZE;
	memcpy_toio(pbData, _pbReserve + MTZ_RES_HEADER_OFF, u4Size);
	return (u4Size);

}
EXPORT_SYMBOL(MetaZone_ReadReserved);



u32 MetaZone_WriteReserved(char *pbData, unsigned int u4Size)
{
	pr_info("[MTZ] Write Reserved u4Size(%d).\n", u4Size);

	if (NULL==pbData) {
		pr_err("[MTZ] MetaZone_WriteReserved input is invalid\n");
		return MZ_FAILURE;
	}

	if (!_fgWritableZoneInited)
		return MZ_FAILURE;

	if (u4Size >  MTZ_RES_MAX_SIZE)
		u4Size =  MTZ_RES_MAX_SIZE;

	Lock();
	memcpy_toio(_pbReserve + MTZ_RES_HEADER_OFF, pbData, u4Size);
	if(3 != _pMtzResHeader->dwRFlag[0])
	{
		_pMtzResHeader->dwRFlag[0] = 1;
	}
	_fgWritableZoneDirty = true;
	Unlock();
	recount_CRC_value(3);

	return MZ_SUCCESS;
}
EXPORT_SYMBOL(MetaZone_WriteReserved);


u32 MetaZone_ReadReserved_Offset(char *pbData,unsigned int offset, unsigned int u4Size)
{
	pr_info("[MTZ] Read Reserved offset(%08x) u4Size(%d).\n",offset, u4Size);

	if (NULL == pbData) {
		pr_err("[MTZ] MetaZone_ReadReserved_Offset input is invalid\n");
		return MZ_FAILURE;
	}
	if (!_fgWritableZoneInited)
		return MZ_FAILURE;

	if(offset> MTZ_RES_MAX_SIZE)
		return MZ_FAILURE;

	if(_pMtzResHeader->dwRFlag[0] == 0)
		return MZ_FAILURE;

	if((u4Size + offset) > MTZ_RES_MAX_SIZE)
		u4Size = MTZ_RES_MAX_SIZE - offset;

	memcpy_toio(pbData, _pbReserve + MTZ_RES_HEADER_OFF + offset, u4Size);

	return u4Size;
}
EXPORT_SYMBOL(MetaZone_ReadReserved_Offset);


u32 MetaZone_WriteReserved_Offset(char *pbData, unsigned int offset, unsigned int u4Size)
{
	pr_info("[MTZ] Write Reserved offset(%08x) u4Size(%d).\n",offset, u4Size);

	if (NULL == pbData) {
		pr_err("[MTZ] MetaZone_WriteReserved_Offset input is invalid\n");
		return MZ_FAILURE;
	}
	if (!_fgWritableZoneInited)
		return (MZ_FAILURE);

	if(offset > MTZ_RES_MAX_SIZE)
		return MZ_FAILURE;

	if((u4Size + offset) > MTZ_RES_MAX_SIZE)
		u4Size = MTZ_RES_MAX_SIZE - offset;

	Lock();
	memcpy_toio(_pbReserve + MTZ_RES_HEADER_OFF + offset, pbData, u4Size);
	if(3 != _pMtzResHeader->dwRFlag[0])
	{
		_pMtzResHeader->dwRFlag[0] = 1;
	}
	_fgWritableZoneDirty = true;
	Unlock();

	recount_CRC_value(3);

	return MZ_SUCCESS;
}
EXPORT_SYMBOL(MetaZone_WriteReserved_Offset);
*/

#if 0
unsigned int MetaZone_Read_MDP(unsigned int flag, char *pbData, unsigned int offset, unsigned int u4Size)
{
    u32 *pMDPReadyFlag1 = NULL;
	u32 *pMDPReadyFlag2 = NULL;
	pr_info("[MTZ] Read MDP data flag(%d) offset(%d) u4Size(%d).\n", flag, offset, u4Size);

	if (NULL==pbData) {
        pr_err("[MTZ] MetaZone_Read_MDP input is invalid\n");
		return MZ_FAILURE;
	}
	if (!_fgWritableZoneInited)
		return MZ_FAILURE;

	if(offset > MTZ_MDP_SIZE_MAX)
		return MZ_FAILURE;

	if((u4Size+offset) > MTZ_MDP_SIZE_MAX)
		u4Size = MTZ_MDP_SIZE_MAX - offset; //the real size for read

    pMDPReadyFlag1 = (u32 *)((unsigned int)_pMetaZone + MTZ_PARTITION_SIZE/2-MTZ_HERDER_OFF-MTZ_CONFIG_AREA_SIZE 
		                          + sizeof(Wflag)+ sizeof(CRCflag));
	pMDPReadyFlag2 = pMDPReadyFlag1 + 1;

	pr_info("[MTZ] MDP data ready flag1=(%d) flag2=(%d).\n", *pMDPReadyFlag1, *pMDPReadyFlag2);

	if(MDP_FLAG_1 == flag){
		if(1 != *pMDPReadyFlag1) // no data had been write into MDP flag1
			return MZ_FAILURE;
	    memcpy(pbData, _pMdpFlag1+offset, u4Size);
	}
	else if(MDP_FLAG_2 == flag){
		if(1 != *pMDPReadyFlag2) // no data had been write into MDP flag2
			return MZ_FAILURE;
		memcpy(pbData, _pMdpFlag2+offset, u4Size);
	}
	else
		return MZ_FAILURE; // invalid flag

	return u4Size;
}
EXPORT_SYMBOL(MetaZone_Read_MDP);


u32 MetaZone_Flush_MDP(int fgSync) // Just for write mdp API, internal use
{
	int ret = 0;
	pr_info("[MTZ] Flush MDP Sync(%d).\n", fgSync);

#ifndef NAND_BOOT
	ret = mmc_write_mdp(_pMdpFlag1, 2*MTZ_MDP_SIZE_MAX);
#endif

	if(ret){
        pr_err("[MTZ] Flush MDP data error.\n");
		return MZ_FAILURE;
	}
	pr_info("[MTZ] Flush MDP data success.\n");
	return MZ_SUCCESS;
}


u32 MetaZone_Write_MDP(unsigned int flag, char *pbData, unsigned int offset, unsigned int u4Size)
{
	pr_info("[MTZ] Write MDP data flag(%d) offset(%d) u4Size(%d).\n", flag, offset, u4Size);

	if (NULL==pbData) {
        pr_err("[MTZ] MetaZone_Write_MDP input is invalid\n");
		return MZ_FAILURE;
	}
	if (!_fgWritableZoneInited)
		return MZ_FAILURE;

	if(offset > MTZ_MDP_SIZE_MAX)
		return MZ_FAILURE;

	if((u4Size+offset) > MTZ_MDP_SIZE_MAX)
		u4Size = MTZ_MDP_SIZE_MAX - offset; //the real size for write

	if(MDP_FLAG_1 == flag){
		Lock();
	    memcpy(_pMdpFlag1+offset, pbData, u4Size);
		Unlock();
		set_MDP_ready_flag(MDP_FLAG_1);
	}
	else if(MDP_FLAG_2 == flag){
		Lock();
		memcpy(_pMdpFlag2+offset, pbData, u4Size);
		Unlock();
		set_MDP_ready_flag(MDP_FLAG_2);
	}
	else{
	    pr_err("[MTZ] Write MDP flag(%d) is invalid.\n",flag);
		return MZ_FAILURE; // invalid flag
	}

	if(MetaZone_Flush_MDP(1)){ // default flush,  flush MDP data to block 1 and 2
        return MZ_FAILURE;
	}

	return MZ_SUCCESS;
}
EXPORT_SYMBOL(MetaZone_Write_MDP);
#endif

u32 MetaZone_Flush(int fgSync)
{
	int ret = 0;
	pr_info("[MTZ] Flush metazone.bin Sync(%d).\n", fgSync);


	if (METAZONE_SIGNATURE != _pMetaHeader->dwSignature) {
		pr_err("[MTZ] MetaZone Signature is invalid!!!\r\n");
		ret = mmc_read_metazone((char *)g_mtz_rsvmem_info->virt_addr, g_mtz_rsvmem_info->size);
		if (ret != 0)
			pr_err("[MTZ]read metazone fail\r\n");

		return false;
	}

	up(&_rSema);
	return MZ_SUCCESS;
}
EXPORT_SYMBOL(MetaZone_Flush);


/*************************************************/
/*************************************************/
/***  The function below add for metazone enhancement   *****/
/*************************************************/
/*************************************************/


u32  MetaZone_SpecReadReserved(u32 u4Num, char *pbData, u32 offset, u32 u4Size)
{
	pr_info("[MTZ] Read Reserved areanum(%d) offset(%08x) u4Size(%d).\n", u4Num, offset, u4Size);
	if(NULL == pbData){
		pr_err("[MTZ] MetaZone_SpecReadReserved_Offset input is invalid\n");
		return MZ_FAILURE;
	}
	if (!_fgWritableZoneInited)
		return MZ_NO_INIT;

	if(offset > MTZ_RES_MAX_SIZE)
		return MZ_FAILURE;

	if((u4Num > MTZ_RES_MAX_NUM) || (u4Num < 0))
		return MZ_FAILURE;

	if(_pMtzResHeader->dwRFlag[u4Num] == 0)
		return MZ_FAILURE;

	if((u4Size + offset) > MTZ_RES_MAX_SIZE)
		u4Size = MTZ_RES_MAX_SIZE - offset;

	memcpy_fromio(pbData, (char *)(_pbReserve + MTZ_RES_HEADER_OFF + (u4Num * MTZ_RES_MAX_SIZE * 2) + offset), u4Size);
	return (MZ_SUCCESS);

}
EXPORT_SYMBOL(MetaZone_SpecReadReserved);


u32  MetaZone_SpecWriteReserved(u32 u4Num, char *pbData, u32 offset, u32 u4Size, u32 u4flage)
{
	int ret = 0;
	pr_info("[MTZ] Write Reserved areanum(%d) offset(%08x) u4Size(%d).u4flag(%d)\n", u4Num, offset, u4Size, u4flage);
	if(NULL == pbData){
		return MZ_FAILURE;
	}
	if (!_fgWritableZoneInited)
		return (MZ_NO_INIT);

	if(offset>MTZ_RES_MAX_SIZE)
		return MZ_FAILURE;

	if((u4Num > MTZ_RES_MAX_NUM) || (u4Num < 0))
		return MZ_FAILURE;

	if((u4Size+offset)>MTZ_RES_MAX_SIZE)
		u4Size = MTZ_RES_MAX_SIZE - offset;

	if (Metazone_check_CRC_flag() != 0) {
		pr_err("[MTZ] Metazone CRC error, data broken.\n");
		ret = mmc_read_metazone((char *)g_mtz_rsvmem_info->virt_addr, g_mtz_rsvmem_info->size);
		if (ret != 0)
			pr_err("[MTZ]read metazone fail\r\n");

		return MZ_FAILURE;
	}

	Lock();
	memcpy_toio(_pbReserve + MTZ_RES_HEADER_OFF + (u4Num * MTZ_RES_MAX_SIZE * 2) + offset, pbData, u4Size);
	if(3 != _pMtzResHeader->dwRFlag[u4Num])
	{
		_pMtzResHeader->dwRFlag[u4Num] = 1;
	}
	if(MTZ_WRITE_FLAG_FACTORYREST == u4flage)
	{
		_pMtzResHeader->dwRFlag[u4Num] = 3;
		memcpy_toio(_pbReserve + MTZ_RES_HEADER_OFF + (u4Num * MTZ_RES_MAX_SIZE * 2) + MTZ_RES_MAX_SIZE + offset, pbData, u4Size);
	}
	_fgWritableZoneDirty = true;
	Unlock();

	recount_CRC_value(3);


	return (MZ_SUCCESS);

}
EXPORT_SYMBOL(MetaZone_SpecWriteReserved);


u32  MetaZone_SpecWrite(u32 u4Idx, u32 u4Data, u32 u4flage)
{
	pr_info("[MTZ] DW Write u4idx:%x u4data:%x u4flag:%d \n",u4Idx,u4Data,u4flage);
	if (MZ_WR_IDX_START <= u4Idx)
	{
		if (!_fgWritableZoneInited)
		{
			return (MZ_NO_INIT);
		}
		u4Idx -= MZ_WR_IDX_START;
		if (u4Idx < _pMetaHeader->dwValueNum)
		{
			Lock();
			if(u4flage == MTZ_WRITE_FLAG_FACTORYREST)
			{
				memcpy_toio((char *)(_pu4Value + u4Idx * MTZ_DWORD_SIZE + 6), &u4Data, sizeof(u32));
				Metazone_Set_Flag((char *)(_pu4Value + u4Idx * MTZ_DWORD_SIZE + 1), 1, true);

			}

			memcpy_toio((char *)(_pu4Value + u4Idx * MTZ_DWORD_SIZE + 2), &u4Data, sizeof(u32));
			Metazone_Set_Flag((char *)(_pu4Value + u4Idx * MTZ_DWORD_SIZE + 1), 0, true);
			_fgWritableZoneDirty = true;
			Unlock();
			recount_CRC_value(1); // recount Dword CRC value in memory

			return (MZ_SUCCESS);
		}
		u4Idx += MZ_WR_IDX_START;

	}
	return (MZ_FAILURE);

}
EXPORT_SYMBOL(MetaZone_SpecWrite);


u32  MetaZone_SpecWriteBinary(u32 u4Idx, const char* pbData, u32 u4Size, u32 u4flage)
{
	pr_info("[MTZ] Binary Write u4idx:(%x) u4size:(%d) u4flag:(%d)\n",u4Idx,u4Size,u4flage);
	if (MZ_WR_IDX_START <= u4Idx)
	{
		if (!_fgWritableZoneInited)
		{
			return (MZ_NO_INIT);
		}
		u4Idx -= MZ_WR_IDX_START;
		if (u4Idx < _pMetaHeader->dwBinaryNum)
		{
			if (u4Size > _pMetaHeader->dwBinaryItemSize)
				u4Size = _pMetaHeader->dwBinaryItemSize;
			Lock();
			if(u4flage == MTZ_WRITE_FLAG_FACTORYREST)
			{
				memcpy_toio((char *)(_u4BinaryStart + (_pMetaHeader->dwBinaryItemSize * 2 + 6) * u4Idx + _pMetaHeader->dwBinaryItemSize + 6), pbData, u4Size);
				Metazone_Set_Flag((char *)(_u4BinaryStart + (_pMetaHeader->dwBinaryItemSize*2 + 6) * u4Idx  +1), 1, true);

			}

			memcpy_toio((char *)( _u4BinaryStart + (_pMetaHeader->dwBinaryItemSize * 2 + 6) * u4Idx + 2), &u4Size, sizeof(u32));
			memcpy_toio((char *)(_u4BinaryStart + (_pMetaHeader->dwBinaryItemSize * 2 + 6) * u4Idx + 6), pbData, u4Size);
			Metazone_Set_Flag((char *)(_u4BinaryStart + (_pMetaHeader->dwBinaryItemSize * 2 + 6) * u4Idx + 1), 0, true);
			_fgWritableZoneDirty = true;
			Unlock();
			recount_CRC_value(2); // recount Dword CRC value in memory
			printk("[metazone]after recount_CRC_value\n");

			return (MZ_SUCCESS);

		}
		u4Idx += MZ_WR_IDX_START;
	}
	return (MZ_FAILURE);

}
EXPORT_SYMBOL(MetaZone_SpecWriteBinary);



u32 MetaZone_Factory_Rest(void)
{

	u32 u4Idx = 0;
	char *pbflag = NULL;
	char *pvalue1 = NULL;
	char *pvalue2 = NULL;
	if (!_fgWritableZoneInited)
	{
		return (MZ_NO_INIT);
	}
	pr_info("[MTZ]enter Metazone Factory Reset.\n");

	/*----------reset dword -------------*/
	Lock();
	for(u4Idx = 1; u4Idx <_pMetaHeader->dwValueNum; u4Idx ++)
	{
		pbflag = (char *)(_pu4Value + u4Idx * MTZ_DWORD_SIZE + 1);
		if(Metazone_Get_Flag(pbflag, 1))
		{
			pr_info("[MTZ] Metazone Dword memcpy index:[%d].\n",u4Idx);
			memcpy_toio((_pu4Value + u4Idx * MTZ_DWORD_SIZE + 2), (_pu4Value + u4Idx * MTZ_DWORD_SIZE + 2 + 4 ), 4);

		}
	}

	pr_info("[MTZ]Metazone Dword Area Factory Reset End.\n");
	u4Idx = 0;
	pvalue2 = (char *)kmalloc(_pMetaHeader->dwBinaryItemSize, GFP_KERNEL);
	for(u4Idx = 0; u4Idx <_pMetaHeader->dwBinaryNum; u4Idx ++)
	{
		pbflag = (char *)(_u4BinaryStart + u4Idx * (_pMetaHeader->dwBinaryItemSize * 2 + 6) + 1);
		if(Metazone_Get_Flag(pbflag, 1))
		{
			pr_info("[MTZ] Metazone Binary memcpy index:[%d].\n",u4Idx);
			pvalue1 = (char *)(_u4BinaryStart + u4Idx * (_pMetaHeader->dwBinaryItemSize * 2 + 6) + 6);
			memcpy_fromio(pvalue2, (char *)(_u4BinaryStart + u4Idx * (_pMetaHeader->dwBinaryItemSize * 2 + 6) + 6 + _pMetaHeader->dwBinaryItemSize), _pMetaHeader->dwBinaryItemSize);
			memcpy_toio(pvalue1, pvalue2, _pMetaHeader->dwBinaryItemSize);
		}
	}

	pr_info("[MTZ] Metazone Binary Area Factory Reset End.\n");
	u4Idx = 0;
	for(u4Idx = 0; u4Idx <4; u4Idx ++)
	{
		if (_pMtzResHeader->dwRFlag[u4Idx] == 3)
		{
			pr_info("[MTZ] Metazone Reserve memcpy index:[%d].\n",u4Idx);
			memcpy_toio(_pbReserve + MTZ_RES_HEADER_OFF + (u4Idx * MTZ_RES_MAX_SIZE * 2),
					_pbReserve + MTZ_RES_HEADER_OFF + (u4Idx * MTZ_RES_MAX_SIZE * 2) + MTZ_RES_MAX_SIZE,
					MTZ_RES_MAX_SIZE);
		}
		else
		{
			pr_err("[MTZ] Metazone ResNum [%d] Not Need Factory Reset .\n",u4Idx);
		}
	}
	Unlock();
	kfree(pvalue2);
	recount_CRC_value(1);
	recount_CRC_value(2);
	recount_CRC_value(3);
	pr_info("[MTZ] Metazone Reserved Area Factory Reset End.\n");
	_fgWritableZoneDirty = true;

	MetaZone_Flush(1);
	pr_info("[MTZ] Metazone Factory Reset End.\n");
	return (MZ_SUCCESS);

}





/* ----------------------------------------------------------------------------- */
/* FUNCTION */
/* MTZ_IOControl */
/* DESCRIPTION */
/*  */
/* PARAMETERS */
/*  */
/* RETURNS */
/* Return true for success; otherwise, the operation fails. */
/* RETURN VALUE LIST */
/* none */
/* ---------------------------------------------------------------------------- */
bool MTZ_IOControl(int context, int code, u8 *pInBuffer, int inSize, u8 *pOutBuffer,
		   int outSize, int *pOutSize)
{
	u32 u4Ret = MZ_FAILURE;
	bool fgBlock = false;

	switch (code) {
	case IOCTL_MZ_READ_VALUE:
		if ((!pInBuffer) || (inSize != 4) || (!pOutBuffer) || (4 != outSize))
			return false;

		u4Ret = MetaZone_Read(*(u32 *) pInBuffer, (u32 *) pOutBuffer);
		if ((MZ_FAILURE != u4Ret) && pOutSize)
			*pOutSize = 4;
		break;

	case IOCTL_MZ_WRITE_VALUE:
		if ((!pInBuffer) || (inSize != 8))
			return false;

		u4Ret = MetaZone_Write(*(u32 *) pInBuffer, *((u32 *) pInBuffer + 1));
		break;

	case IOCTL_MZ_READ_BINARY:
		if ((!pInBuffer) || (inSize != 4) || (!pOutBuffer) || (!outSize) || (!pOutSize))
			return false;

		u4Ret = (int)MetaZone_ReadBinary(*(u32 *) pInBuffer, pOutBuffer, outSize);
		*pOutSize = u4Ret;
		break;

	case IOCTL_MZ_WRITE_BINARY:
		if ((!pInBuffer) || (inSize != 4) || (!pOutBuffer) || (!outSize))
			return false;

		u4Ret = MetaZone_WriteBinary(*(u32 *) pInBuffer, pOutBuffer, outSize);
		break;

	/*case IOCTL_MZ_READ_RESERVED:
		if ((!pOutBuffer) || (!outSize) || (!pOutSize))
			return false;

		u4Ret = (int)MetaZone_ReadReserved((char *)pOutBuffer, (u32) outSize);
		*pOutSize = u4Ret;
		break;

	case IOCTL_MZ_WRITE_RESERVED:
		if ((!pOutBuffer) || (!outSize))
			return false;

		u4Ret = MetaZone_WriteReserved(pOutBuffer, outSize);
		break;
	*/

	case IOCTL_MZ_READ_INFO:
		if ((!pOutBuffer) || (sizeof(METAZONE_INFO_T) != outSize))
			return false;

		u4Ret = MetaZone_ReadInfo((PMETAZONE_INFO_T) pOutBuffer);
		if ((MZ_NO_INIT != u4Ret) && pOutSize)
			*pOutSize = sizeof(METAZONE_INFO_T);
		break;

	/*case IOCTL_MZ_READ_RESERVED_OFFSET:
		if ((!pInBuffer) || (inSize != 4) || (!pOutBuffer) || (!outSize)|| (!pOutSize))
			return false;
		u4Ret = (int)MetaZone_ReadReserved_Offset((char *)pOutBuffer,*(u32 *) pInBuffer, (u32) outSize);
		*pOutSize = u4Ret;
		break;

	case IOCTL_MZ_WRITE_RESERVED_OFFSET:
		if ((!pInBuffer) || (inSize != 4) || (!pOutBuffer) || (!outSize))
			return false;
		u4Ret = MetaZone_WriteReserved_Offset((char *)pOutBuffer,*(u32 *) pInBuffer,(u32) outSize);
		break;

	case IOCTL_MZ_READ_MDP:
		if ((!pInBuffer) || (inSize != 8) || (!pOutBuffer) || (!outSize)|| (!pOutSize))
			return false;
		//u4Ret = (int)MetaZone_Read_MDP(*(u32 *) pInBuffer,(char *)pOutBuffer, *((u32 *) pInBuffer + 1), (u32) outSize);
		*pOutSize = u4Ret;
		break;

	case IOCTL_MZ_WRITE_MDP:
		if ((!pInBuffer) || (inSize != 8) || (!pOutBuffer) || (!outSize))
			return false;
		//u4Ret = MetaZone_Write_MDP(*(u32 *) pInBuffer, (char *)pOutBuffer, *((u32 *) pInBuffer + 1), (u32) outSize);
		break;
	*/

	case IOCTL_MZ_SPEC_WRITE_VALUE:  // add for metazone enhancement
		if ((!pInBuffer) || (inSize != 12))
			return false;

		u4Ret = MetaZone_SpecWrite(*(u32 *) pInBuffer, *((u32 *) pInBuffer + 1), *((u32 *) pInBuffer + 2));
		break;

	case IOCTL_MZ_SPEC_WRITE_BINARY:
		if ((!pInBuffer) || (inSize != 8) || (!pOutBuffer) || (!outSize))
			return false;

		u4Ret = MetaZone_SpecWriteBinary(*(u32 *) pInBuffer, pOutBuffer, outSize, *((u32 *) pInBuffer + 1));
		break;

	case IOCTL_MZ_SPEC_READ_RESERVED:
		if ((!pInBuffer) || (inSize != 8) || (!pOutBuffer) || (!outSize)|| (!pOutSize))
			return false;
		u4Ret = (int)MetaZone_SpecReadReserved(*(u32 *) pInBuffer,(char *)pOutBuffer,*((u32 *) pInBuffer + 1), (u32) outSize);
		*pOutSize = u4Ret;
		break;

	case IOCTL_MZ_SPEC_WRITE_RESERVED:
		if ((!pInBuffer) || (inSize != 12) || (!pOutBuffer) || (!outSize))
			return false;
		u4Ret = MetaZone_SpecWriteReserved(*(u32 *) pInBuffer,(char *)pOutBuffer,*((u32 *) pInBuffer + 1),(u32) outSize,*((u32 *) pInBuffer + 2));
		break;


	case IOCTL_MZ_FLUSH:
		if (pInBuffer)
			fgBlock = *(int *) pInBuffer;
		u4Ret = MetaZone_Flush(fgBlock);
		break;

	case IOCTL_MZ_FACTORY_REST:
		if (pInBuffer)
			fgBlock = *(int *) pInBuffer;
		u4Ret = MetaZone_Factory_Rest();
		break;

	default:
		break;
	}

	return (MZ_FAILURE == u4Ret) ? false : true;
}

static int mtz_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	void *private_data = NULL;

	private_data = (void *)MTZ_Open(1, 0, 0);

	/* FIXME */
	file->private_data = private_data;

	return ret;
}

static int mtz_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	void *private_data = file->private_data;

	if (private_data == NULL)
		return -1;

	MTZ_Close((unsigned int)private_data);

	file->private_data = NULL;

	return ret;
}


static long mtz_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	void *private_data = NULL;
	WIN32_IOCTL_DATA win_ioctl;
	void *pInBuf;
	void *pOutBuf;
	unsigned int *pBytesReturned;
	bool bRet = false;
	u8 alloc_in_buf = 0;
	u8 alloc_out_buf = 0;
	u8 alloc_byte_returned = 0;
	long ret = 0;
	unsigned long l_size = 0;
	pr_info("[MTZ] enter mtz_ioctl.\n");

	private_data = filp->private_data;
	if (!private_data) {
		pr_err("[MTZ] filp private_data error, can't get device handler.\n");
		ret = -2;
		goto ioctl_err;
	}
	if (copy_from_user((void *)&win_ioctl, (void *)arg, sizeof(win_ioctl))) {
		pr_err("[MTZ] copy source ioctl parameter failed.\n");
		ret = -4;
		goto ioctl_err;
	}
	pInBuf = win_ioctl.pInBuf;
	pOutBuf = win_ioctl.pOutBuf;
	pBytesReturned = win_ioctl.pBytesReturned;

	/***********************************************
	***** because of the CONFIG_CPU_SW_DOMAIN_PAN,    ****
	***** cann't use userspace pointer address(arg),modify code  *
	**** by Hui Yang @ 2018.03.08                                     ****
	************************************************/

	//if ((win_ioctl.InSize) && (((WIN32_IOCTL_DATA*)arg)->pInBuf)){
	if ((win_ioctl.InSize) && (win_ioctl.pInBuf)){
		win_ioctl.pInBuf = kzalloc(win_ioctl.InSize, GFP_KERNEL);
		if (win_ioctl.pInBuf){
			//l_size=copy_from_user(win_ioctl.pInBuf, ((WIN32_IOCTL_DATA*)arg)->pInBuf, win_ioctl.InSize);
			l_size=copy_from_user(win_ioctl.pInBuf, pInBuf, win_ioctl.InSize);
			alloc_in_buf = 1;
		} else {
			pr_err("[MTZ] copy source ioctl parameter (pInBuf) failed.\n");
			ret = -5;
			goto ioctl_err;
		}
	}

	//if ((win_ioctl.OutSize) && (((WIN32_IOCTL_DATA*)arg)->pOutBuf)){
	if ((win_ioctl.OutSize) && (win_ioctl.pOutBuf)){
		win_ioctl.pOutBuf = kzalloc(win_ioctl.OutSize, GFP_KERNEL);
		if (win_ioctl.pOutBuf){
			//l_size=copy_from_user(win_ioctl.pOutBuf, ((WIN32_IOCTL_DATA*)arg)->pOutBuf, win_ioctl.OutSize);
			l_size=copy_from_user(win_ioctl.pOutBuf, pOutBuf, win_ioctl.OutSize);
			alloc_out_buf = 1;
		} else {
			pr_err("[MTZ] copy source ioctl parameter (pOutBuf) failed.\n");
			ret = -6;
			goto ioctl_err;
		}
	}
	//if (((WIN32_IOCTL_DATA*)arg)->pBytesReturned){
	if (win_ioctl.pBytesReturned){
		win_ioctl.pBytesReturned = kzalloc(sizeof(unsigned int), GFP_KERNEL);
		if (win_ioctl.pBytesReturned){
			//l_size=copy_from_user(win_ioctl.pBytesReturned, ((WIN32_IOCTL_DATA*)arg)->pBytesReturned, sizeof(unsigned int));
			l_size=copy_from_user(win_ioctl.pBytesReturned, pBytesReturned, sizeof(unsigned int));
			alloc_byte_returned = 1;
		} else {
			pr_err("[MTZ] copy source ioctl parameter (pBytesReturned) failed.\n");
			ret = -7;
			goto ioctl_err;
		}
	}
	bRet = MTZ_IOControl((int)private_data,
				cmd,
				(u8 *) win_ioctl.pInBuf,
				win_ioctl.InSize,
				(u8 *) win_ioctl.pOutBuf,
				win_ioctl.OutSize, win_ioctl.pBytesReturned);

	/* Process data only in success */
	if (bRet){
		if (alloc_in_buf) {
			/*
			ret = copy_to_user((void *)((WIN32_IOCTL_DATA*)arg)->pInBuf,
								(void *)win_ioctl.pInBuf,
								win_ioctl.InSize);
			*/
			ret = copy_to_user((void *)pInBuf,
								(void *)win_ioctl.pInBuf,
								win_ioctl.InSize);
			if (ret) {
				pr_err("[MTZ] copy result to ioctl parameter (pBytesReturned) failed.\n");
				ret = -8;
				bRet = false;
				goto ioctl_err;
			}
		}

		if (alloc_out_buf) {
			/*
			ret = copy_to_user((void *)((WIN32_IOCTL_DATA*)arg)->pOutBuf,
								(void *)win_ioctl.pOutBuf,
							win_ioctl.OutSize);
							*/
			ret = copy_to_user((void *)pOutBuf,
								(void *)win_ioctl.pOutBuf,
							win_ioctl.OutSize);
			if (ret) {
				pr_err("[MTZ] copy result to ioctl parameter (pBytesReturned) failed.\n");
				ret = -9;
				bRet = false;
				goto ioctl_err;
			}
		}

		if (alloc_byte_returned) {
			/*
			ret = copy_to_user((void *)((WIN32_IOCTL_DATA*)arg)->pBytesReturned,
								(void *)win_ioctl.pBytesReturned,
								sizeof(unsigned int));
			*/
			ret = copy_to_user((void *)pBytesReturned,
								(void *)win_ioctl.pBytesReturned,
								sizeof(unsigned int));
			if (ret) {
				pr_err("[MTZ] copy result to ioctl parameter (pBytesReturned) failed.\n");
				ret = -10;
				bRet = false;
				goto ioctl_err;
			}
		}
	}


ioctl_err:
	if (alloc_in_buf) {
		kfree(win_ioctl.pInBuf);
		win_ioctl.pInBuf = NULL;
	}

	if (alloc_out_buf) {
		kfree(win_ioctl.pOutBuf);
		win_ioctl.pOutBuf = NULL;
	}

	if (alloc_byte_returned) {
		kfree(win_ioctl.pBytesReturned);
		win_ioctl.pBytesReturned = NULL;
	}

	return bRet ? 0 : (-1);
}

#ifdef CONFIG_COMPAT
static long mtz_ioctl_32(struct file *filp, unsigned int cmd, unsigned long arg)
{
	WIN32_IOCTL_DATA __user *mtz_ioctl_data;
	MTZ_IOCTL_DATA_32 mtz_ioctl_data32;
	if (copy_from_user(&mtz_ioctl_data32, (void __user *)arg, sizeof(mtz_ioctl_data32)))
		return -EFAULT;

	mtz_ioctl_data = compat_alloc_user_space(sizeof(*mtz_ioctl_data));
	if (!access_ok(VERIFY_WRITE, mtz_ioctl_data, sizeof(*mtz_ioctl_data))
		|| __put_user(mtz_ioctl_data32.InSize, &mtz_ioctl_data->InSize)
		|| __put_user(mtz_ioctl_data32.OutSize, &mtz_ioctl_data->OutSize)
		|| __put_user((void __user *)(unsigned long)mtz_ioctl_data32.pInBuf, &mtz_ioctl_data->pInBuf)
		|| __put_user((void __user *)(unsigned long)mtz_ioctl_data32.pOutBuf, &mtz_ioctl_data->pOutBuf)
		|| __put_user((void __user *)(unsigned long)mtz_ioctl_data32.pBytesReturned, &mtz_ioctl_data->pBytesReturned))
		return -EFAULT;

	if (!filp->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;
	pr_err("[MTZ] mtz_ioctl_32 into\n");
	return filp->f_op->unlocked_ioctl(filp, cmd, (unsigned long)mtz_ioctl_data);
}
#endif

static const struct file_operations mtz_fops = {
	.open			 = mtz_open,
	.release		 = mtz_release,
	.unlocked_ioctl	 = mtz_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	 = mtz_ioctl_32,
#endif
};

static int metazone_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	mtzdev_info *metazone_dev = NULL;
	int result = -1;
	unsigned int pra_dts[4]={0};
	unsigned long base_phys_addr, base_size;

	pr_info("[MTZ]enter metazone probe\r\n");

	if (!node) {
		pr_err("[MTZ] metazone probe fail because of no metazone device compatible dts node!\r\n");
		return -1;
	}

	metazone_dev = kzalloc(sizeof(mtzdev_info), GFP_KERNEL);
	if (!metazone_dev) {
		result = -ENOMEM;
		pr_err("[MTZ]metazone probe fail because of get alloc memory fail!\n");
		goto err_alloc_mem;
	}

	node = of_find_compatible_node(NULL,NULL,"autochips,metazone");
	if (node) {
		if (of_property_read_u32_array(node,"reg",(u32 *)pra_dts, 4)){
			pr_err("[MTZ] faild get reserved addr and size in dts!\r\n");
			goto err_free_mem;
		}
		base_phys_addr = (unsigned long)pra_dts[1];
		base_size = pra_dts[3];
		pr_info("[MTZ] metazone reserved memory start is 0x%x,size is 0x%x\n",(u32)base_phys_addr,(u32)base_size);
	}else {
		pr_err("[MTZ] fail to get reserved memory node in dts!\r\n");
		goto err_free_mem;
	}
	metazone_dev->reserved_mem_info.phys_addr = base_phys_addr;
	metazone_dev->reserved_mem_info.size      = base_size;
	metazone_dev->reserved_mem_info.virt_addr = (unsigned long)ioremap(metazone_dev->reserved_mem_info.phys_addr,
											metazone_dev->reserved_mem_info.size);


	if (0 == metazone_dev->reserved_mem_info.virt_addr) {
		pr_err("[MTZ]metazone fail in memremap!\r\n");
		goto err_free_mem;
	}

	metazone_dev->dev = &(pdev->dev);
	metazone_dev->cdev.name = MTZ_MISC_DVR_NAME;
	metazone_dev->cdev.minor = MISC_DYNAMIC_MINOR;
	metazone_dev->cdev.fops = &mtz_fops;

	platform_set_drvdata(pdev, metazone_dev);

	result = misc_register(&(metazone_dev->cdev));
	if (result != 0) {
		pr_err("[MTZ]metazone probe fail because of misc_register, error = %d\r\n", result);
		goto err_unset_drvdata;
	}

	g_mtz_rsvmem_info = &(metazone_dev->reserved_mem_info);

	// Init metazone
	MTZ_Init();

	pr_info("[MTZ]exit metazone probe\r\n");
	return 0;

err_unset_drvdata:
	platform_set_drvdata(pdev, NULL);

err_free_mem:
	if (0 != metazone_dev->reserved_mem_info.virt_addr) {
		iounmap((void *)metazone_dev->reserved_mem_info.virt_addr);
	}
	kfree(metazone_dev);
	metazone_dev = NULL;
err_alloc_mem:
	pr_err("[MTZ]metazone probe fail\r\n");

	return result;
}

static int metazone_remove(struct platform_device *pdev)
{
	mtzdev_info *metazone_dev = platform_get_drvdata(pdev);

	if (0 != metazone_dev->reserved_mem_info.virt_addr)
		iounmap((void *)metazone_dev->reserved_mem_info.virt_addr);

	//of_reserved_mem_device_release(&(pdev->dev));

	misc_deregister(&(metazone_dev->cdev));

	platform_set_drvdata(pdev, NULL);

	kfree(metazone_dev);

	metazone_dev = NULL;

	return 0;
}

static const struct of_device_id metazone_of_ids[] = {
	{.compatible = "autochips,metazone",},
	{}
};

static struct platform_driver metazone_platform_drv = {
	.driver = {
		.name = "atc-metazone",
		.owner = THIS_MODULE,
		.of_match_table = metazone_of_ids,
		},
	.probe = metazone_probe,
	.remove = metazone_remove,
};


static int __init mtz_init(void)
{
	int ret = 0;
	struct device_node *node = NULL;

	//MOD_VERSION_INFO(MTZ_MODE_NAME, MTZ_VER_MAJOR, MTZ_VER_MINOR, MTZ_VER_REV);

	pr_info("[MTZ]enter metazone init\r\n");

	node = of_find_compatible_node(NULL, NULL, "autochips,metazone");
	if (!node) {
		pr_err("[MTZ]metazone init fail in find autochips,metazone dts compatible node!!\r\n");
		ret = -ENOMEM;
		goto err_node;
	}


	ret = platform_driver_register(&metazone_platform_drv);
	if (ret) {
		pr_err("[MTZ]metazone init fail in platform_driver_register, error = %d\r\n", ret);
		goto err_node;
	}

	pr_info("[MTZ]exit metazone init\r\n");

	return 0;

err_node:
	pr_err("[MTZ]exit metazone init with error\r\n");
	return ret;
}

static void __exit mtz_exit(void)
{
	MTZ_Deinit();

	pr_info("[MTZ]enter mtz exit!\r\n");

	platform_driver_unregister(&metazone_platform_drv);

	pr_info("[MTZ]exit mtz ok!\r\n");
}

subsys_initcall(mtz_init);
module_exit(mtz_exit);

/*
EXPORT_SYMBOL(MetaZone_Flush);
EXPORT_SYMBOL(MetaZone_Read);
EXPORT_SYMBOL(MetaZone_Write);
EXPORT_SYMBOL(MetaZone_ReadBinary);
EXPORT_SYMBOL(MetaZone_WriteBinary);
EXPORT_SYMBOL(MetaZone_ReadReserved);
EXPORT_SYMBOL(MetaZone_WriteReserved);
EXPORT_SYMBOL(MetaZone_ReadInfo);
*/

MODULE_DESCRIPTION("atc metazone driver");
MODULE_LICENSE(ATC_KERNEL_LINUX_LICENSE);

