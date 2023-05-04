/*************************************************************************/ /*!
@File           sysconfig.c
@Title          System Configuration
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Configuration functions
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#include "pvrsrv_device.h"
#include "syscommon.h"
#include "sysinfo.h"
#include "sysconfig.h"
#include "physheap.h"
#if defined(SUPPORT_ION)
#include "ion_support.h"
#endif
#include "interrupt_support.h"
#include "rgx_bvnc_defs_km.h"
#include <linux/ioport.h>
#include <linux/of.h>

#define DEBUG_ENABLE_ACTIVE_PM 1

#include "gpu_tz.h"

static RGX_TIMING_INFORMATION	gsRGXTimingInfo;
static RGX_DATA					gsRGXData;
static PVRSRV_DEVICE_CONFIG 	gsDevices[1];
static PHYS_HEAP_FUNCTIONS		gsPhysHeapFuncs;
static PHYS_HEAP_CONFIG			gsPhysHeapConfig[2];

#if defined(PVR_DVFS)
extern struct devfreq_dev_profile img_devfreq_dev_profile;
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0))
struct clk *devm_clk_get_optional(struct device *dev, const char *id)
{
	struct clk *clk = devm_clk_get(dev, id);

	if (clk == ERR_PTR(-ENOENT))
		return NULL;

	return clk;
}
#endif

/*
   CPU to Device physical address translation
   */
	static
void UMAPhysHeapCpuPAddrToDevPAddr(IMG_HANDLE hPrivData,
		IMG_UINT32 ui32NumOfAddr,
		IMG_DEV_PHYADDR *psDevPAddr,
		IMG_CPU_PHYADDR *psCpuPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psDevPAddr[0].uiAddr = psCpuPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psDevPAddr[ui32Idx].uiAddr = psCpuPAddr[ui32Idx].uiAddr;
		}
	}
}

/*
   Device to CPU physical address translation
   */
	static
void UMAPhysHeapDevPAddrToCpuPAddr(IMG_HANDLE hPrivData,
		IMG_UINT32 ui32NumOfAddr,
		IMG_CPU_PHYADDR *psCpuPAddr,
		IMG_DEV_PHYADDR *psDevPAddr)
{
	PVR_UNREFERENCED_PARAMETER(hPrivData);

	/* Optimise common case */
	psCpuPAddr[0].uiAddr = psDevPAddr[0].uiAddr;
	if (ui32NumOfAddr > 1)
	{
		IMG_UINT32 ui32Idx;
		for (ui32Idx = 1; ui32Idx < ui32NumOfAddr; ++ui32Idx)
		{
			psCpuPAddr[ui32Idx].uiAddr = psDevPAddr[ui32Idx].uiAddr;
		}
	}
}

PVRSRV_ERROR MrvlPrePowerState(IMG_HANDLE hSysData,
		PVRSRV_SYS_POWER_STATE eNewPowerState,
		PVRSRV_SYS_POWER_STATE eCurrentPowerState,
		IMG_BOOL bForced)
{
	if (eNewPowerState == PVRSRV_SYS_POWER_STATE_ON)
	{
		/* Enable clock, VS640's GFX Core Clock register have protected by TEE,
		   need call TA to open the clock. VS680's GFX Core Clock is normal*/
#ifdef CONFIG_GPU_VS680
		MRVL_SYS_DATA *pSysData = hSysData;
		clk_prepare_enable(pSysData->pCoreClk);
#else
		syna_TD_SET_GFX_CORE_CLOCK(1);
#endif
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR MrvlPostPowerState(IMG_HANDLE hSysData,
		PVRSRV_SYS_POWER_STATE eNewPowerState,
		PVRSRV_SYS_POWER_STATE eCurrentPowerState,
		IMG_BOOL bForced)
{
	if (eNewPowerState == PVRSRV_SYS_POWER_STATE_OFF)
	{
		/* Disable clock, VS640's GFX Core Clock register have protected by TEE,
		   need call TA to clock the clock. VS680's GFX Core Clock is normal*/
#ifdef CONFIG_GPU_VS680
		MRVL_SYS_DATA *pSysData = hSysData;
		clk_disable_unprepare(pSysData->pCoreClk);
#else
		syna_TD_SET_GFX_CORE_CLOCK(0);
#endif
	}

	return PVRSRV_OK;
}

#if defined(PVR_DVFS) || defined(SUPPORT_PDVFS)
static void SetFrequency(IMG_UINT32 ui32Frequency)
{
#ifdef CONFIG_GPU_VS680
	/*VS680 only have 700MHZ freq, do nothing for V680*/
#else
	int i32Count = _FoundFreqCount(ui32Frequency);
	int ui32Ret;
	if (i32Count >= 0)
	{
		/* Set frequency may fail, we need set correct clock speed here
		 * and remove it in pvr_dvfs_device.c */
		ui32Ret = cpm_set_core_mode("GFX3D", i32Count);
		if (ui32Ret != 0)
		{
			printk(KERN_ERR "pvr: cpm set clock failed, roll back to default. Err = %d\n", ui32Ret);
			gsRGXTimingInfo.ui32CoreClockSpeed = ((MRVL_SYS_DATA *)gsDevices[0].hSysData)->ui32DefaultClockSpeed;
		} else {
			gsRGXTimingInfo.ui32CoreClockSpeed = ui32Frequency;
		}

		printk(KERN_DEBUG "pvr: set freq %d, cpm count %d, current freq %d\n",
				ui32Frequency, i32Count,
				gsRGXTimingInfo.ui32CoreClockSpeed);
	} else {
		printk(KERN_ERR "pvr: can't found freq %d in opp table\n", ui32Frequency);
	}
#endif
}

static void SetVoltage(IMG_UINT32 ui32Voltage)
{
	/*Please call API to set Voltage*/
}
#endif

PVRSRV_ERROR SysDevInit(void *pvOSDevice, PVRSRV_DEVICE_CONFIG **ppsDevConfig)
{
	PVRSRV_ERROR eError = PVRSRV_OK;
	IMG_UINT32 ui32NextPhysHeapID = 0;
	IMG_INT32 i32Irq = -1;
	IMG_INT32 i32Status = PVRSRV_OK;
	IMG_UINT64 ui64ClockSpeed = 0;
	MRVL_SYS_DATA *pSysData = NULL;
	struct resource *pResource = NULL;
	struct clk *pCoreClk = NULL;
	struct clk *pSysClk = NULL;
	struct clk *pGfxAxiClk = NULL;
	struct platform_device *pPlatformDev = to_platform_device((struct device *)pvOSDevice);
	struct device_node *pDeviceNode = ((struct device *)pvOSDevice)->of_node;

	printk(KERN_INFO "Synaptics PVR init. Version 201803121700\n");

	if (gsDevices[0].pvOSDevice)
	{
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

#if defined(SUPPORT_TRUSTED_DEVICE)
	eError = init_tz(pvOSDevice);
	if (eError != PVRSRV_OK) {
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
#endif

	/*
	 * Setup information about physical memory heap(s) we have
	 */
	gsPhysHeapFuncs.pfnCpuPAddrToDevPAddr = UMAPhysHeapCpuPAddrToDevPAddr;
	gsPhysHeapFuncs.pfnDevPAddrToCpuPAddr = UMAPhysHeapDevPAddrToCpuPAddr;

	gsPhysHeapConfig[ui32NextPhysHeapID].pszPDumpMemspaceName = "SYNA_SYSMEM";
	gsPhysHeapConfig[ui32NextPhysHeapID].eType = PHYS_HEAP_TYPE_UMA;
	gsPhysHeapConfig[ui32NextPhysHeapID].psMemFuncs = &gsPhysHeapFuncs;
	gsPhysHeapConfig[ui32NextPhysHeapID].hPrivData = NULL;
	gsPhysHeapConfig[ui32NextPhysHeapID].ui32UsageFlags = PHYS_HEAP_USAGE_GPU_LOCAL;
	ui32NextPhysHeapID += 1;

	/*
	 * Setup RGX specific timing data
	 */
	/* Get sys/core clock */
	pSysClk     = devm_clk_get_optional(&pPlatformDev->dev, "gfx3dsys");
	pCoreClk    = devm_clk_get(&pPlatformDev->dev, "gfx3dcore");
	pGfxAxiClk  = devm_clk_get_optional(&pPlatformDev->dev, "gfxaxi");

	if (IS_ERR(pSysClk) || IS_ERR(pCoreClk) || IS_ERR(pGfxAxiClk)) {
		printk(KERN_ERR "pvr: get clock from DTS failed.\n");
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	ui64ClockSpeed = clk_get_rate(pCoreClk);

	if (!ui64ClockSpeed) {
		printk(KERN_ERR "pvr: get clockspeed from DTS failed.\n");
		return PVRSRV_ERROR_INVALID_DEVICE;
	} else {
		printk(KERN_ERR "pvr: ui64ClockSpeed=%lld.\n", ui64ClockSpeed);
	}
	gsRGXTimingInfo.ui32CoreClockSpeed        = ui64ClockSpeed;
#if defined(PVR_DVFS) || defined(SUPPORT_PDVFS) || DEBUG_ENABLE_ACTIVE_PM
	gsRGXTimingInfo.bEnableActivePM           = IMG_TRUE;
#else
	gsRGXTimingInfo.bEnableActivePM           = IMG_FALSE;
#endif
	gsRGXTimingInfo.bEnableRDPowIsland        = IMG_FALSE;
	gsRGXTimingInfo.ui32ActivePMLatencyms     = SYS_RGX_ACTIVE_POWER_LATENCY_MS;

	/*
	 *Setup RGX specific data
	 */
	gsRGXData.psRGXTimingInfo = &gsRGXTimingInfo;

#if defined(SUPPORT_TRUSTED_DEVICE)
	gsDevices[0].pfnTDSendFWImage	         = syna_PFN_TD_SEND_FW_IMAGE;
	gsDevices[0].pfnTDSetPowerParams         = syna_PFN_TD_SET_POWER_PARAMS;
	gsDevices[0].pfnTDRGXStart               = syna_PFN_TD_RGXSTART;
	gsDevices[0].pfnTDRGXStop                = syna_PFN_TD_RGXSTOP;

	gsPhysHeapConfig[ui32NextPhysHeapID].pszPDumpMemspaceName = "SYNA_SECURE_FW";
	gsPhysHeapConfig[ui32NextPhysHeapID].eType                = PHYS_HEAP_TYPE_LMA;
	gsPhysHeapConfig[ui32NextPhysHeapID].psMemFuncs           = &gsPhysHeapFuncs;
	gsPhysHeapConfig[ui32NextPhysHeapID].hPrivData            = NULL;
	gsPhysHeapConfig[ui32NextPhysHeapID].ui32UsageFlags       = PHYS_HEAP_USAGE_GPU_SECURE|PHYS_HEAP_USAGE_FW_CODE|PHYS_HEAP_USAGE_FW_PRIV_DATA;
	gsPhysHeapConfig[ui32NextPhysHeapID].sStartAddr.uiAddr    = getSecureHeapPaddr();
	gsPhysHeapConfig[ui32NextPhysHeapID].sCardBase.uiAddr     = getSecureHeapPaddr();
	gsPhysHeapConfig[ui32NextPhysHeapID].uiSize               = (FW_CODE_SIZE+FW_DATA_SIZE)*2;
	ui32NextPhysHeapID++;

	/* Device's physical heaps */
	gsDevices[0].pasPhysHeaps = gsPhysHeapConfig;
	gsDevices[0].ui32PhysHeapCount = ui32NextPhysHeapID;
	/* Device's physical heap IDs */
#else
	/* Virtualization support services needs to know which heap ID corresponds to FW */
	PVR_ASSERT(ui32NextPhysHeapID < ARRAY_SIZE(gsPhysHeapConfig));

	/* Device's physical heaps */
	gsDevices[0].pasPhysHeaps = gsPhysHeapConfig;
	gsDevices[0].ui32PhysHeapCount = ui32NextPhysHeapID;
#endif

	/*
	 * Setup device
	 */
	gsDevices[0].pvOSDevice				= pvOSDevice;
	gsDevices[0].pszName				= SYS_RGX_DEV_NAME;
	gsDevices[0].pszVersion				= NULL;

	/* Device setup information */
	pResource = platform_get_resource(pPlatformDev, IORESOURCE_MEM, 0);
	if (!pResource)
	{
		printk(KERN_ERR "pvr: read register info from DTS failed.\n");
		i32Status = PVRSRV_ERROR_INVALID_DEVICE;
		goto REMOVE_DEVICE;
	}

	gsDevices[0].sRegsCpuPBase.uiAddr   = pResource->start;
	gsDevices[0].ui32RegsSize           = resource_size(pResource);

	i32Irq = platform_get_irq(pPlatformDev, 0);
	if (i32Irq <= 0) {
		printk(KERN_ERR "pvr: read irq from DTS failed.\n");
		i32Status = PVRSRV_ERROR_INVALID_DEVICE;
		goto REMOVE_DEVICE;
	}
	gsDevices[0].ui32IRQ                = i32Irq;

	gsDevices[0].ui32NeedAllocFromDMAZone = of_property_read_bool(pDeviceNode, "alloc-from-dma-zone");

	printk(KERN_INFO "pvr: clockspeed:          %u\n", gsRGXTimingInfo.ui32CoreClockSpeed);
	printk(KERN_INFO "pvr: register base:       0x%llx\n", gsDevices[0].sRegsCpuPBase.uiAddr);
	printk(KERN_INFO "pvr: register size:       0x%x\n", gsDevices[0].ui32RegsSize);
	printk(KERN_INFO "pvr: irq:                 %d\n", gsDevices[0].ui32IRQ);
	printk(KERN_INFO "pvr: need alloc from dma zone: %s.\n", gsDevices[0].ui32NeedAllocFromDMAZone?"true":"false");

	gsDevices[0].eCacheSnoopingMode     = PVRSRV_DEVICE_SNOOP_NONE;

#if defined(PVR_DVFS) || DEBUG_ENABLE_ACTIVE_PM
	gsDevices[0].pfnPrePowerState       = MrvlPrePowerState;
	gsDevices[0].pfnPostPowerState      = MrvlPostPowerState;
#else
	gsDevices[0].pfnPrePowerState       = NULL;
	gsDevices[0].pfnPostPowerState      = NULL;
#endif

	/* No clock frequency either */
	gsDevices[0].pfnClockFreqGet        = NULL;

	gsDevices[0].hDevData               = &gsRGXData;
	gsDevices[0].bDevicePA0IsValid      = IMG_FALSE;

	gsDevices[0].pfnSysDevFeatureDepInit = NULL;
	/* Setup other system specific stuff */
#if defined(SUPPORT_ION) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
	i32Status = IonInit(NULL);
	if (i32Status != PVRSRV_OK)
		goto REMOVE_DEVICE;
#endif

#if defined(PVR_DVFS) || defined(SUPPORT_PDVFS)
	gsDevices[0].sDVFS.sDVFSDeviceCfg.bIdleReq               = IMG_FALSE;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.pasOPPTable            = NULL;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.ui32OPPTableSize       = 0;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.pfnSetFrequency        = SetFrequency;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.pfnSetVoltage          = SetVoltage;
	gsDevices[0].sDVFS.sDVFSDeviceCfg.ui32PollMs             = 200;
	gsDevices[0].sDVFS.sDVFSGovernorCfg.ui32UpThreshold      = 60;
	gsDevices[0].sDVFS.sDVFSGovernorCfg.ui32DownDifferential = 10;
#endif


	/* Save Synaptics private data here. */
	pSysData = (MRVL_SYS_DATA *)devm_kzalloc(&pPlatformDev->dev, sizeof(MRVL_SYS_DATA), GFP_KERNEL);
	if (!pSysData) {
		i32Status = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto DEINIT;
	}
	pSysData->pPlatformDev  = pPlatformDev;
	pSysData->pCoreClk      = pCoreClk;
	pSysData->pSysClk       = pSysClk;
	pSysData->pGfxAxiClk    = pGfxAxiClk;
	pSysData->ui32DefaultClockSpeed = gsRGXTimingInfo.ui32CoreClockSpeed;

	gsDevices[0].hSysData   = pSysData;

	if (clk_prepare_enable(pSysClk)) {
		i32Status = PVRSRV_ERROR_INVALID_DEVICE;
		printk(KERN_ERR "PVR GPU: Can not enable GFX Sys clock");
		goto DEINIT;
	}

	/* Always Enable GFX AXI clock */
	if (clk_prepare_enable(pGfxAxiClk)) {
		i32Status = PVRSRV_ERROR_INVALID_DEVICE;
		printk(KERN_ERR "PVR GPU: Can not enable GFX Axi clock");
		goto DISABLE_SYS_CLK;
	}

#if !defined(PVR_DVFS) && !DEBUG_ENABLE_ACTIVE_PM
	/* Enable CLK here if there is no PM */
	if (clk_prepare_enable(pCoreClk)) {
		i32Status = PVRSRV_ERROR_INVALID_DEVICE;
		printk(KERN_ERR "PVR GPU: Can not enable GFX Core clock");
		goto DISABLE_SYS_CLK;
	}
#endif

	*ppsDevConfig = &gsDevices[0];

	return PVRSRV_OK;

#if !defined(PVR_DVFS)
DISABLE_SYS_CLK:
	clk_disable_unprepare(pSysClk);
#endif
DEINIT:
#if defined(SUPPORT_ION) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
	IonDeinit();
#endif
REMOVE_DEVICE:
	gsDevices[0].pvOSDevice = NULL;
	return i32Status;
}

void SysDevDeInit(PVRSRV_DEVICE_CONFIG *psDevConfig)
{
	MRVL_SYS_DATA *pSysData = (MRVL_SYS_DATA *)(psDevConfig->hSysData);

#if defined(SUPPORT_ION) && (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
	IonDeinit();
#endif

	if (pSysData != NULL) {
#if !defined(PVR_DVFS) && !DEBUG_ENABLE_ACTIVE_PM
		clk_disable_unprepare(pSysData->pCoreClk);
#endif
		clk_disable_unprepare(pSysData->pSysClk);
		clk_disable_unprepare(pSysData->pGfxAxiClk);
		psDevConfig->hSysData = NULL;
	}

	psDevConfig->pvOSDevice = NULL;
}

PVRSRV_ERROR SysInstallDeviceLISR(IMG_HANDLE hSysData,
		IMG_UINT32 ui32IRQ,
		const IMG_CHAR *pszName,
		PFN_LISR pfnLISR,
		void *pvData,
		IMG_HANDLE *phLISRData)
{
	PVR_UNREFERENCED_PARAMETER(hSysData);
	return OSInstallSystemLISR(phLISRData, ui32IRQ, pszName, pfnLISR, pvData,
			SYS_IRQ_FLAG_TRIGGER_DEFAULT);
}

PVRSRV_ERROR SysUninstallDeviceLISR(IMG_HANDLE hLISRData)
{
	return OSUninstallSystemLISR(hLISRData);
}

PVRSRV_ERROR SysDebugInfo(PVRSRV_DEVICE_CONFIG *psDevConfig,
		DUMPDEBUG_PRINTF_FUNC *pfnDumpDebugPrintf,
		void *pvDumpDebugFile)
{
	PVR_UNREFERENCED_PARAMETER(psDevConfig);
	PVR_UNREFERENCED_PARAMETER(pfnDumpDebugPrintf);
	PVR_UNREFERENCED_PARAMETER(pvDumpDebugFile);
	return PVRSRV_OK;
}

/******************************************************************************
  End of file (sysconfig.c)
 ******************************************************************************/
