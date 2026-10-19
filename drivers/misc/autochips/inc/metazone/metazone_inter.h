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

#ifndef _METAZONE_INTER_H_
#define _METAZONE_INTER_H_

/* ---------------------------------------------------------- */
typedef struct _MetaZone {
	u32 dwVersion;
	u32 dwSignature;
	u32 dwDataSize;		/* Metazone size (in bytes) */
	u32 dwValueOffset;
	u32 dwValueNum;
	u32 dwBinaryOffset;	/*  */
	u32 dwBinaryNum;	/* Number of binary data */
	u32 dwBinaryItemSize;	/* Max bytes of one item of binary data. */
	u32 dwReserveSize;      // Size of reserved area 
	u32 dwReserveOffset;    // offset of reserved 

} TMetaZone, *PTMetaZone;	/* limited size is 3 page size, that might be 3*512 bytes */

typedef struct MTZ_RES_HEADER {
	u32 dwAreNum;
	u32 dwRFlag[6];

} TMTZResHead, *PMTZResH;

typedef struct _Flag{
	u32 write_before;
	u32 write_after;
}Wflag;

typedef struct _CRC16{
    unsigned short crc_dword;
	unsigned short crc_binary;
	unsigned short crc_reserved;
	unsigned short align;
}CRCflag;

typedef enum {
   BACKUP_NONE,
   BACKUP_MTZ1,
   BACKUP_MTZ2
}backup_mtz;

typedef enum {
   MDP_FLAG_1 = 1,
   MDP_FLAG_2 = 2
}mdp_flag;

#define MTZ_MDP_SIZE_MAX        0x8000   /* 32 KB , single mdp data max size, now there are 2 group */
#define MTZ_MDP_OFFSET_Flag_1   0x40000  /* 256 KB , mdp data flag(1) start addr , offset to header*/
#define MTZ_MDP_OFFSET_Flag_2   0x48000  /* 256 KB + 32KB , mdp data flag(1) start addr , offset to header*/

#define METAZONE_SIGNATURE 0xabcdef01
#define METAZONE_SIZE_MAX  0x80000 //0x10000 metazone enhancement max size expend to 512KB
#define METAZONE_VERSION  0x00020000

#define MTZ_RES_HEADER_OFF (0x200)    //512B ,resevered header size
#define MTZ_RES_MAX_SIZE   (0x8000)   //32KB ,resevered max size
#define MTZ_RES_MAX_NUM    (5)        // 0 1 2 3 4 5 ,resevered has 6 area.

#define MTZ_DWORD_SIZE     (10)      //10B Dword size
#define MTZ_BINARY_SIZE    (206)     //206B Binary size

/******Metazone Write Flag ************/
#define MTZ_WRITE_FLAG_NONE          (0)
#define MTZ_WRITE_FLAG_FACTORYREST   (1)
#define MTZ_WRITE_FLAG_ENCRYPTION    (2)



#define MTZ_PARTITION_SIZE  (0x100000)  /*1MB*/
#define MTZ_CONFIG_AREA_SIZE    0x200    /* 512B , config area,include CRC flag */
#define MTZ_HERDER_OFF          0x200    /* 512B , partition head in metazone */

#define ERROR_INAVLID_METAZONE_SIGNATURE 100L
#define ERROR_INAVLID_METAZONE_SIZE      101L

u32 _MetaZone_Read(u32 u4Idx, u32 *pu4Data);
u32 _MetaZone_ReadBinary(u32 u4Idx, char *pbData, u32 u4Size);
u32 _MetaZone_ReadReserved(char *pbData, u32 u4Size);

u32 _MetaZone_Write(u32 u4Idx, u32 u4Data);
u32 _MetaZone_WriteBinary(u32 u4Idx, char *pbData, u32 u4Size);
u32 _MetaZone_WriteReserved(char *pbData, u32 u4Size);

u32  _MetaZone_SpecReadReserved(u32 u4Num, char *pbData, u32 offset, u32 u4Size);// new add for enhancement
u32  _MetaZone_SpecWrite(u32 u4Idx, u32 u4Data, u32 u4flage);// new add for enhancement
u32  _MetaZone_SpecWriteBinary(u32 u4Idx, char *pbData, u32 u4Size, u32 u4flage); // new add for enhancement
u32  _MetaZone_SpecWriteReserved(u32 u4Num, char *pbData, u32 offset, u32 u4Size, u32 u4flage);// new add for enhancement


#endif				/* _METAZONE_INTER_H_ */
