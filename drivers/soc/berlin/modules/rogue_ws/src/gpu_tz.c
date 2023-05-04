#if defined(SUPPORT_TRUSTED_DEVICE)
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>

#include "tee_client_api.h"
#include "gpu_tz.h"
#include "ion_support.h"
//#include "vz_support.h"
#include "interrupt_support.h"
#include "rgx_bvnc_defs_km.h"
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/dma-buf.h>
#include <linux/delay.h>
#include <linux/firmware.h>

#include "linux/ion.h"

enum {
	GPU_CMD_SEND_IMAGE,
	GPU_CMD_GET_CODE_SIZE,
	GPU_CMD_GET_DATA_SIZE,
	GPU_CMD_SET_FW_PARAMS,
	GPU_CMD_SET_POWER_PARAMS,
	GPU_CMD_SET_SLC_SIZE_IN_BYTES,
	GPU_CMD_SET_SLC_CACHE_LINE_SIZE_BITS,
	GPU_CMD_SET_PHYS_BUS_WIDTH,
	GPU_CMD_SET_bDevicePA0IsValid,
	GPU_CMD_SET_SLC_BANKS,
	GPU_CMD_SET_ui64DevErnsBrns,
	GPU_CMD_SET_ui64DevFeatures,
	GPU_CMD_SET_LAYOUT_MARS,
	GPU_CMD_RGX_START,
	GPU_CMD_RGX_STOP,
};

#define ION_AF_GFX_NONSECURE    (ION_A_NS | ION_A_FC)
#define ION_AF_GFX_SECURE       (ION_A_FS | ION_A_CG | ION_A_FC)
#define ALLOC_SIZE              1048576

static TEEC_Context             tee_context;
static TEEC_Session             tee_session;
static struct dma_buf           *fw_src_dma_buf;
static struct dma_buf           *fw_dma_buf;

static const TEEC_UUID GPU_TA_UUID =   {0x1316a183, 0x894d, 0x43fe, {0x98, 0x93, 0xbb, 0x94, 0x6a, 0xe1, 0x04, 0x36}};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
static unsigned long ion_dmabuf_get_phy(struct dma_buf *dmabuf)
{
	struct ion_buffer *buffer = dmabuf->priv;
	struct sg_table *table;
	struct page *page;

	table = buffer->sg_table;
	page = sg_page(table->sgl);
	return (unsigned long)PFN_PHYS(page_to_pfn(page));
}
#endif

phys_addr_t getSecureHeapPaddr(void)
{
	unsigned long ori = ion_dmabuf_get_phy(fw_dma_buf);
	if(ori & 0xFFFF) {
		printk(KERN_INFO "fw_dma_buf phy addr is align to 0xFFFF, change it to %lx\n", (ori+0xFFFF)&(~0xFFFF));
	}

	return (ori+0xFFFF)&(~0xFFFF);
}

static TEEC_Result getValueFromTA(int cmdId, int *size)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_OUTPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = 0xdeadbeef;

	result = TEEC_InvokeCommand(&tee_session, cmdId, &operation, NULL);

	*size = operation.params[0].value.a;

	return result;
}

static TEEC_Result setFwParams(int cmdId, IMG_UINT64 a, IMG_UINT64 b, IMG_UINT64 c, IMG_UINT32 d, IMG_UINT32 e)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT);
	operation.params[0].value.a = (IMG_UINT32)(a>>32);
	operation.params[0].value.b = (IMG_UINT32)(a&0xFFFFFFFF);
	operation.params[1].value.a = (IMG_UINT32)(b>>32);
	operation.params[1].value.b = (IMG_UINT32)(b&0xFFFFFFFF);
	operation.params[2].value.a = (IMG_UINT32)(c>>32);
	operation.params[2].value.b = (IMG_UINT32)(c&0xFFFFFFFF);
	operation.params[3].value.a = d;
	operation.params[3].value.b = e;

	result = TEEC_InvokeCommand(&tee_session, cmdId, &operation, NULL);

	return result;
}

PVRSRV_ERROR init_tz(void *pvOSDevice)
{
	TEEC_Result result;
	int nonSecurePoolID = -1;
	int securePoolID = -1;
	TEEC_Parameter parameter;
	TEEC_SharedMemory shm;
	TEEC_Property *property = NULL;
	const struct firmware *psFW = NULL;
	PVRSRV_ERROR res = PVRSRV_OK;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	struct ion_heap_data *hdata;
	int heap_num, i;
#endif

	result = TEEC_InitializeContext(NULL, &tee_context);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "TEEC_InitializeContext fail, result=%x\n", result);
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	res = request_firmware(&psFW, "ta/libgfx_img_linux.ta", pvOSDevice);
	if (res != 0) {
		printk(KERN_ERR "%s: request_firmware(ta/libgfx_img_linux.ta) failed (%d)", __func__, res);
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto cleanupContext;
	}

	shm.size = psFW->size;
	shm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	result = TEEC_AllocateSharedMemory(&tee_context, &shm);
	if (result != TEEC_SUCCESS || NULL == shm.buffer) {
		printk(KERN_ERR "AllocateShareMemory error, ret=0x%08x\n", result);
		res = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto cleanupFW;
	}

	memcpy(shm.buffer, psFW->data, psFW->size);

	parameter.memref.parent = &shm;
	parameter.memref.size = shm.size;
	parameter.memref.offset = 0;

	result = TEEC_RegisterTAExt(&tee_context, &parameter, TEEC_MEMREF_PARTIAL_INPUT, property);
	if (result != TEEC_SUCCESS) {
		if (result == TEEC_ERROR_ACCESS_CONFLICT)
			printk(KERN_ERR "This TA has been registed already\n");
		else
			printk(KERN_ERR "RegisterTAExt error, ret=0x%08x\n", result);

		res = PVRSRV_ERROR_INIT_FAILURE;
		goto cleanSharedBuffer;
	}

	result = TEEC_OpenSession(&tee_context, &tee_session, &GPU_TA_UUID, TEEC_LOGIN_USER, NULL, NULL, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "TEEC_OpenSession fail, result=%x\n", result);
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto cleanSharedBuffer;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	hdata = kmalloc(sizeof(*hdata) * ION_NUM_MAX_HEAPS, GFP_KERNEL);
	if (!hdata) {
		printk(KERN_ERR "tzd_ion_shm_init alloc mem failed\n");
		res = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto cleanSharedBuffer;
	}

	heap_num = ion_query_heaps_kernel(hdata, ION_NUM_MAX_HEAPS);

	for (i = 0; i < heap_num; i++) {
		if (!strcmp(hdata[i].name, "CMA-reserved"))
			nonSecurePoolID = 1 << hdata[i].heap_id;
	}

	for (i = 0; i < heap_num; i++) {
		if (!strcmp(hdata[i].name, "Secure"))
			securePoolID = 1 << hdata[i].heap_id;
	}

	kfree(hdata);

	if (nonSecurePoolID == -1 || securePoolID == -1) {
		printk(KERN_ERR "Can't find CMA-reserved or Secure Pool! %d %d", nonSecurePoolID, securePoolID);
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto cleanSharedBuffer;
	}

	fw_src_dma_buf = ion_alloc(ALLOC_SIZE, nonSecurePoolID, ION_NONCACHED);
	fw_dma_buf     = ion_alloc(FW_CODE_SIZE+FW_DATA_SIZE, securePoolID, ION_NONCACHED);
#else
	ion_get_heaps_mask_by_attr(ION_AF_GFX_NONSECURE, &nonSecurePoolID);
	ion_get_heaps_mask_by_attr(ION_AF_GFX_SECURE, &securePoolID);

	if (nonSecurePoolID == -1 || securePoolID == -1) {
		printk(KERN_ERR "Can not find CMA-reserved or Secure Pool, will assert!");
		res = PVRSRV_ERROR_INIT_FAILURE;
		goto cleanSharedBuffer;
	}

	fw_src_dma_buf = ion_alloc_dma_buf(ALLOC_SIZE, nonSecurePoolID, ION_NONCACHED);
	fw_dma_buf     = ion_alloc_dma_buf(FW_CODE_SIZE+FW_DATA_SIZE + 0xFFFF, securePoolID, ION_NONCACHED);
#endif

cleanSharedBuffer:
	TEEC_ReleaseSharedMemory(&shm);
cleanupFW:
	release_firmware(psFW);
cleanupContext:
	TEEC_FinalizeContext(&tee_context);
	return res;
}

PVRSRV_ERROR syna_PFN_TD_SEND_FW_IMAGE(IMG_HANDLE hSysData, PVRSRV_TD_FW_PARAMS *psTDFWParams)
{
	TEEC_Operation operation;
	TEEC_SharedMemory srcShm;
	TEEC_SharedMemory dstShm;
	TEEC_Result result;
	int codeSize;
	int dataSize;
	void *fw_src_data;
	void *cpuPAddr;

	if (psTDFWParams->uFWP.sMips.asFWPageTableAddr[1].uiAddr != 0 ||
	   psTDFWParams->uFWP.sMips.asFWPageTableAddr[2].uiAddr != 0 ||
	   psTDFWParams->uFWP.sMips.asFWPageTableAddr[3].uiAddr != 0) {
		printk(KERN_ERR "asFWPageTableAddr[1~3] must be zero!");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	setFwParams(GPU_CMD_SET_FW_PARAMS,
				psTDFWParams->uFWP.sMips.sGPURegAddr.uiAddr,
				psTDFWParams->uFWP.sMips.asFWPageTableAddr[0].uiAddr,
				psTDFWParams->uFWP.sMips.sFWStackAddr.uiAddr,
				psTDFWParams->uFWP.sMips.ui32FWPageTableLog2PageSize,
				psTDFWParams->uFWP.sMips.ui32FWPageTableNumPages);

	dma_buf_begin_cpu_access(fw_src_dma_buf, DMA_FROM_DEVICE);
	fw_src_data = dma_buf_kmap(fw_src_dma_buf, 0);
	cpuPAddr = (void *)ion_dmabuf_get_phy(fw_src_dma_buf);

	memcpy(fw_src_data, psTDFWParams->pvFirmware, psTDFWParams->ui32FirmwareSize);

	dma_buf_kunmap(fw_src_dma_buf, 0, fw_src_data);
	dma_buf_end_cpu_access(fw_src_dma_buf, DMA_FROM_DEVICE);

	srcShm.buffer = NULL;
	srcShm.phyAddr = cpuPAddr;
	srcShm.size = psTDFWParams->ui32FirmwareSize;
	srcShm.flags = TEEC_MEM_INPUT;
	result = TEEC_RegisterSharedMemory(&tee_context, &srcShm);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "TEEC_RegisterSharedMemory fail as:%d  (phyAddr=%llx  size=%d)\n",
				result, (unsigned long long)srcShm.phyAddr, (int)srcShm.size);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	dstShm.buffer  = NULL;
	dstShm.phyAddr = (void *)getSecureHeapPaddr();
	dstShm.size    = FW_CODE_SIZE+FW_DATA_SIZE;
	dstShm.flags   = TEEC_MEM_INPUT;
	result = TEEC_RegisterSharedMemory(&tee_context, &dstShm);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "TEEC_RegisterSharedMemory fail as:%d  (phyAddr=%llx  size=%d)\n",
				 result, (unsigned long long)dstShm.phyAddr, (int)dstShm.size);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_PARTIAL_INPUT,
											TEEC_MEMREF_PARTIAL_INPUT,
											TEEC_NONE,
											TEEC_NONE);

	operation.params[0].memref.parent = &srcShm;
	operation.params[0].memref.offset = 0;
	operation.params[0].memref.size   = psTDFWParams->ui32FirmwareSize;

	operation.params[1].memref.parent = &dstShm;
	operation.params[1].memref.offset = 0;
	operation.params[1].memref.size   = FW_CODE_SIZE+FW_DATA_SIZE;

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SEND_IMAGE, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SEND_IMAGE fail as:%d \n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(fw_src_dma_buf != NULL) {
		dma_buf_put(fw_src_dma_buf);
		fw_src_dma_buf = NULL;
	}

	/*get FW code size*/
	result = getValueFromTA(GPU_CMD_GET_CODE_SIZE, &codeSize);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_GET_CODE_SIZE fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*FW code size must be the same as FW_CODE_SIZE, otherwise MIPS can not bootup*/
	if (codeSize != FW_CODE_SIZE) {
		printk(KERN_ERR "Get wrong FW_CODE_SIZE from TA, expect:%d actual:%d", FW_CODE_SIZE, codeSize);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*get FW code size*/
	result = getValueFromTA(GPU_CMD_GET_DATA_SIZE, &dataSize);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_GET_DATA_SIZE fail as:%d\n",  result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/*FW code size must be the same as FW_DATA_SIZE, otherwise MIPS can not bootup*/
	if (dataSize != FW_DATA_SIZE) {
		printk(KERN_ERR "Get wrong FW_DATA_SIZE from TA, expect:%d actual:%d", FW_DATA_SIZE, dataSize);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_PFN_TD_SET_POWER_PARAMS(IMG_HANDLE hSysData, PVRSRV_TD_POWER_PARAMS *psTDPowerParams)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT, TEEC_VALUE_INPUT);

	if (psTDPowerParams->sPCAddr.uiAddr>>32         != 0 ||
		psTDPowerParams->sGPURegAddr.uiAddr>>32    != 0 ||
		psTDPowerParams->sBootRemapAddr.uiAddr>>32 != 0 ||
		psTDPowerParams->sCodeRemapAddr.uiAddr>>32 != 0 ||
		psTDPowerParams->sDataRemapAddr.uiAddr>>32 != 0) {
		printk(KERN_ERR "GPU PC/Reg/Boot/Code/Data address must within 32bits!");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.params[0].value.a = (IMG_UINT32)(psTDPowerParams->sPCAddr.uiAddr & 0xFFFFFFFF);
	operation.params[0].value.b = (IMG_UINT32)(psTDPowerParams->sGPURegAddr.uiAddr & 0xFFFFFFFF);
	operation.params[1].value.a = (IMG_UINT32)(psTDPowerParams->sBootRemapAddr.uiAddr & 0xFFFFFFFF);
	operation.params[1].value.b = (IMG_UINT32)(psTDPowerParams->sCodeRemapAddr.uiAddr & 0xFFFFFFFF);
	operation.params[2].value.a = (IMG_UINT32)(psTDPowerParams->sDataRemapAddr.uiAddr & 0xFFFFFFFF);
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_POWER_PARAMS, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_POWER_PARAMS fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = psTDPowerParams->SLC_SIZE_IN_BYTES;
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_SLC_SIZE_IN_BYTES, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_SLC_SIZE_IN_BYTES fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = psTDPowerParams->SLC_CACHE_LINE_SIZE_BITS;
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_SLC_CACHE_LINE_SIZE_BITS, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_SLC_CACHE_LINE_SIZE_BITS fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = psTDPowerParams->PHYS_BUS_WIDTH;
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_PHYS_BUS_WIDTH, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_PHYS_BUS_WIDTH fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = psTDPowerParams->bDevicePA0IsValid;
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_bDevicePA0IsValid, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_bDevicePA0IsValid fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = psTDPowerParams->SLC_BANKS;
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_SLC_BANKS, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_SLC_BANKS fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = (unsigned int)(psTDPowerParams->ui64DevErnsBrns>>32);
	operation.params[0].value.b = (unsigned int)(psTDPowerParams->ui64DevErnsBrns&0xFFFFFFFF);
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_ui64DevErnsBrns, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_ui64DevErnsBrns fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = (unsigned int)(psTDPowerParams->ui64DevFeatures>>32);
	operation.params[0].value.b = (unsigned int)(psTDPowerParams->ui64DevFeatures&0xFFFFFFFF);
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_ui64DevFeatures, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_ui64DevFeatures fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	operation.params[0].value.a = psTDPowerParams->LAYOUT_MARS;
	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_SET_LAYOUT_MARS, &operation, NULL);
	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_SET_LAYOUT_MARS fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_PFN_TD_RGXSTART(IMG_HANDLE hSysData)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_RGX_START, &operation, NULL);

	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_RGX_START fail as:%d\n", result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR syna_PFN_TD_RGXSTOP(IMG_HANDLE hSysData)
{
	TEEC_Result result;
	TEEC_Operation operation = {0};

	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	result = TEEC_InvokeCommand(&tee_session, GPU_CMD_RGX_STOP, &operation, NULL);

	if (result != TEEC_SUCCESS) {
		printk(KERN_ERR "GPU_CMD_RGX_STOP fail as:%d\n",  result);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}
#endif
