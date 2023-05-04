#ifndef GPU_TA_H
#define GPU_TA_H
#include "pvrsrv_device.h"
#include "syscommon.h"
#include "sysinfo.h"
#include "sysconfig.h"
#include "physheap.h"
#include <linux/dma-buf.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/ion.h>
#else
#include PVR_ANDROID_ION_HEADER
#endif

#define FW_SIZE_ALIGN				   0x10000
#define FW_SIZE_MASK				   0xFFFF	 

#define FW_CODE_SIZE                   262144
#define FW_DATA_SIZE                   131072
#if defined(SUPPORT_TRUSTED_DEVICE)
PVRSRV_ERROR syna_PFN_TD_SEND_FW_IMAGE(IMG_HANDLE hSysData, PVRSRV_TD_FW_PARAMS *psTDFWParams);
PVRSRV_ERROR syna_PFN_TD_SET_POWER_PARAMS(IMG_HANDLE hSysData, PVRSRV_TD_POWER_PARAMS *psTDPowerParams);
PVRSRV_ERROR syna_PFN_TD_RGXSTART(IMG_HANDLE hSysData);
PVRSRV_ERROR syna_PFN_TD_RGXSTOP(IMG_HANDLE hSysData);
PVRSRV_ERROR init_tz(void *pvOSDevice);
phys_addr_t getSecureHeapPaddr (void);
#endif
#endif
