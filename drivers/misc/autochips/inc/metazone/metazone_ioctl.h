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

#ifndef _METAZONE_IOCTL_H_
#define _METAZONE_IOCTL_H_

//#include "metazone.h"
#define MTZ_CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define FILE_DEVICE_MTZ             (0x00000022)
#define FILE_ANY_ACCESS             (0)
#define METHOD_BUFFERED             (0)


#define MZ_SUCCESS  0x00000000U
#define MZ_FAILURE  0x80000000U
#define MZ_NO_INIT  0x80000001U

typedef struct _METAZONE_INFO_T {
	unsigned int u4RdValueNum;	/* Max number of dword data in read only section. */
	unsigned int u4RdBinaryNum;	/* Max number of binary data in read only section. */
	unsigned int u4RdBinarySize;	/* Max size(in bytes) of binary data in read only section. */
	unsigned int u4WrValueNum;	/* Max number of dword data in writable section. */
	unsigned int u4WrBinaryNum;	/* Max number of binary data in writable section. */
	unsigned int u4WrBinarySize;	/* Max size(in bytes) of binary data.in writable section. */
	unsigned int u4FsValueNum;	/* Max number of dword data in file section. */
	unsigned int u4FsBinaryNum;	/* Max number of binary data in file section. */
	unsigned int u4FsBinarySize;	/* Max size(in bytes) of binary data in file section. */
} METAZONE_INFO_T, *PMETAZONE_INFO_T;



#define ERROR_LOG		(1)
#define INFO_LOG		(1)

#define META_RESERVE_SIZE (1024 - sizeof(TMetaZone))

#define MZ_BINARY_MAX_SIZE     100	/* Size in bytes */
typedef struct _MTZ_BINARY_T {
	unsigned int dwSize;		/* Real size of binary data */
	char bData[MZ_BINARY_MAX_SIZE];
} MTZ_BINARY_T, *PMTZ_BINARY_T;	/* limited size is 3 page size, that might be 3*512 bytes */


#define IOCTL_MZ_READ_VALUE				\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x201, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_WRITE_VALUE			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x202, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_READ_BINARY			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x203, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_WRITE_BINARY			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x204, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_READ_RESERVED			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x205, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_WRITE_RESERVED			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x206, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_READ_INFO				\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x208, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_FLUSH					\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x209, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_WRITE_LOGO				\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x210, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_READ_RESERVED_OFFSET			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x211, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_WRITE_RESERVED_OFFSET			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x212, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_READ_MDP			\
		((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x213, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_WRITE_MDP		\
		((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x214, METHOD_BUFFERED, FILE_ANY_ACCESS))


#define IOCTL_MZ_SPEC_WRITE_VALUE			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x215, METHOD_BUFFERED, FILE_ANY_ACCESS))
#define IOCTL_MZ_SPEC_WRITE_BINARY			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x216, METHOD_BUFFERED, FILE_ANY_ACCESS))
#define IOCTL_MZ_SPEC_READ_RESERVED			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x217, METHOD_BUFFERED, FILE_ANY_ACCESS))
#define IOCTL_MZ_SPEC_WRITE_RESERVED			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x218, METHOD_BUFFERED, FILE_ANY_ACCESS))

#define IOCTL_MZ_FACTORY_REST			\
	((int)MTZ_CTL_CODE(FILE_DEVICE_MTZ, 0x219, METHOD_BUFFERED, FILE_ANY_ACCESS))



typedef struct {
	unsigned int InSize;
	unsigned int OutSize;
	void *pInBuf;
	void *pOutBuf;
	unsigned int *pBytesReturned;
} WIN32_IOCTL_DATA;

#endif	/* _METAZONE_IOCTL_H_ */
