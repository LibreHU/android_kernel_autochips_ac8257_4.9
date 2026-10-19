#ifndef _VDP_MDD_H
#define _VDP_MDD_H

#ifndef ATC_ARM2_LK_SUPPORT
#include "types.h"
#else
#include "platform/mt_typedefs.h"
#include "sys/types.h"
#include "di_types.h"

#endif
/*add by mtk94020 for vdp*/
#define g_u4PhysicalAddress (0x0C000000UL)


typedef void (*atc_dispc_isr_t)(void *arg, u32 phy_isr);
extern int atc_dispc_register_isr(atc_dispc_isr_t isr, void *arg);
extern int atc_dispc_unregister_isr(atc_dispc_isr_t isr, void *arg);
int VDP_Init_Param(void);
extern int VDP_IOControl(u32 dwCode, void *pBufIn,  void *pBufOut);

/*end add*/

struct atc_dispc_isr_data {
	atc_dispc_isr_t	isr;
	void			*arg;
};

typedef enum {
	VDP_SCAN_MODE_BLOCK = 0,
	VDP_SCAN_MODE_LINE = 1,
} VDP_SCAN_MODE_T;


typedef bool(*PFN_VDP_INIT)(u32 u4VdpId, u32 u4Width, u32 u4Height, u32 u4YAddr, u32 u4CAddr
	, RECT *prInRgn, RECT *prOutRgn, VDP_SCAN_MODE_T eScanMode, u32 u4DeintMode);
typedef bool(*PFN_VDP_HIDE)(u32 u4VdpId);
typedef void(*PFN_VDP_SET_DATA_PA)(u8 ucVdpId, u32 u4AddrY, u32 u4AddrC,  u32 u4DeintMode);



typedef struct {
	PFN_VDP_INIT			pfnVDP_Init;
	PFN_VDP_HIDE			pfnVDP_Hide;
	PFN_VDP_SET_DATA_PA     pfnVDP_SetDataPhyAddr;
} VDP_INTERFACE_T, *PVDP_INTERFACE_T;


PVDP_INTERFACE_T  VDP_QueryInterface(void);

#endif


