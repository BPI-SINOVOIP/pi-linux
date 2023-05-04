// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019-2020 Synaptics Incorporated */

#include "drv_vpp.h"
#include "drv_dhub.h"
#include "avio_core.h"
#include "avio_sub_module.h"
#include "drv_aio.h"

//#define ENABLE_AVIO_IOCTL_DEBUG

#ifdef ENABLE_AVIO_IOCTL_DEBUG
#define avio_ioctl_debug(...)     pr_debug(__VA_ARGS__)
#else
#define avio_ioctl_debug(...)
#endif

#define AVIO_IOCTL_GET_NDX(cmd)   ((cmd&0xF000) >> 12)
#define AVIO_IOCTL_GET_CMD(cmd)   (cmd&0x0FFF)

static AVIO_MODULE g_avio_module[AVIO_MODULE_TYPE_MAX];

static int avio_sub_module_ioctl_get_module_num(unsigned int cmd)
{
	int module_cmd = AVIO_IOCTL_GET_CMD(cmd);
	int module_num = AVIO_IOCTL_GET_NDX(cmd);
	int module_ndx = AVIO_MODULE_TYPE_MAX;
	int start_cmd, end_cmd;

	//check if the command do not belong to any sub modules
	switch (cmd) {
	case VPP_IOCTL_GPIO_WRITE:
			return -1;
	}

	//Check if the command is applicable to all sub modules
	switch (cmd) {
	case POWER_IOCTL_DISABLE_INT:
	case POWER_IOCTL_ENABLE_INT:
	case ISR_IOCTL_SET_AFFINITY:
		return AVIO_MODULE_TYPE_MAX;
	}

	//check if the command belong to specific sub modules
	switch (module_num) {
	case 0:
		//AVIO_MODULE_TYPE_CEC
		start_cmd = AVIO_IOCTL_GET_CMD(VPP_IOCTL_START);
		end_cmd = AVIO_IOCTL_GET_CMD(VPP_IOCTL_END);
		if (((module_cmd >= start_cmd) && (module_cmd <= end_cmd)) ||
			(module_cmd == VPP_IOCTL_INIT))
			module_ndx = AVIO_MODULE_TYPE_VPP;
		break;

	case 1:
		module_ndx = AVIO_MODULE_TYPE_MAX;
		break;

	case 2:
		start_cmd = AVIO_IOCTL_GET_CMD(AIP_IOCTL_START_CMD);
		end_cmd = AVIO_IOCTL_GET_CMD(AIP_IOCTL_SEMUP_CMD);
		if (((module_cmd >= start_cmd)) && (module_cmd <= end_cmd))
			module_ndx = AVIO_MODULE_TYPE_AIP;
		else
			module_ndx = AVIO_MODULE_TYPE_AOUT;
		break;

	case 3:
		start_cmd = AVIO_IOCTL_GET_CMD(APP_IOCTL_INIT_CMD);
		end_cmd = AVIO_IOCTL_GET_CMD(APP_IOCTL_GET_MSG_CMD);
		if ((module_cmd >= start_cmd) && (module_cmd <= end_cmd))
			module_ndx = AVIO_MODULE_TYPE_APP;
		else
			module_ndx = AVIO_MODULE_TYPE_AOUT;
		break;

	case 4:
		module_ndx = AVIO_MODULE_TYPE_VIP;
		break;

	case 5:
		//HDMIRX_IOCTL
		module_ndx = AVIO_MODULE_TYPE_AVIF;
		break;

	case 6:
		module_ndx = AVIO_MODULE_TYPE_AVIF;
		break;

	case 7:
		module_ndx = AVIO_MODULE_TYPE_OVP;
		break;

	case 8:
		//POWER_
		module_ndx = AVIO_MODULE_TYPE_MAX;
		break;
	}

	return module_ndx;
}

int avio_sub_module_probe(struct platform_device *pdev)
{
	int err = 0;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	/* Note: ensure is_probed flag is set before probe()
	 *     so that probe()->regiter() do not retrigger
	 *     init sequence for static sub-modules
	 */

	//Register AVIO driver as sub_module with framework
	g_avio_module[AVIO_MODULE_TYPE_AVIO].is_probed = 1;
	err = avio_module_avio_probe(pdev);

	if (!err) {
		//Register VPP driver as sub_module with framework
		g_avio_module[AVIO_MODULE_TYPE_VPP].is_probed = 1;
		err = avio_module_drv_vpp_probe(pdev);
	}

	if (!err) {
		//Register DHUB driver as sub_module with framework
		g_avio_module[AVIO_MODULE_TYPE_DHUB].is_probed = 1;
		err = avio_module_drv_dhub_probe(pdev);
	}

	if (!err) {
		//Register AIO driver as sub_module with framework
		g_avio_module[AVIO_MODULE_TYPE_AIO].is_probed = 1;
		err = avio_module_drv_aio_probe(pdev);
	}
	return err;
}

int avio_sub_module_config(struct platform_device *pdev)
{
	int err = 0;
	int i;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->config)
			(*fops->config)(g_avio_module[i].pCtxData, pdev);
	}

	return err;
}

int avio_sub_module_ioctl_unlocked(unsigned int cmd, unsigned long arg, long *pRetVal)
{
	int i;
	int processedFlag = 0;
	int module_num;

	avio_ioctl_debug("avio_module:%s:%d\n", __func__, __LINE__);

	module_num = avio_sub_module_ioctl_get_module_num(cmd);

	if (module_num < 0) {
		//This command do not belong to any submodule - so no action
		//Aviod logging otherwise timing issue
		avio_ioctl_debug("avio_module:%s:%d: unhandled ioctl - %x, module:%x\n",
			__func__, __LINE__, cmd, module_num);
		return 0;
	} else if (module_num < AVIO_MODULE_TYPE_MAX) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[module_num].func;
		void *data = g_avio_module[module_num].pCtxData;

		//module specific command - invoke respective module
		if (fops->ioctl_unlocked)
			processedFlag = fops->ioctl_unlocked(
				data, cmd, arg, pRetVal);
		if (processedFlag)
			return processedFlag;
	}

	//Check if any sub-module can process this command
	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;
		void *data = g_avio_module[i].pCtxData;

		if (fops->ioctl_unlocked) {
			processedFlag = fops->ioctl_unlocked(
				data, cmd, arg, pRetVal);
			if ((module_num != AVIO_MODULE_TYPE_MAX) &&
				processedFlag) {
				//processed ioctl?, then return immediately
				break;
			}
		}
	}

	if (!processedFlag) {
		//ioctl not handled by nay module, log it for debug purpose
		//Aviod logging otherwise timing issue
		avio_ioctl_debug("avio_module:%s:%d: unhandled ioctl - %x, module:%x\n",
					__func__, __LINE__, cmd, module_num);
	}

	return processedFlag;
}

void avio_sub_module_enable_irq(void)
{
	int i;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->enable_irq)
			(*fops->enable_irq)(g_avio_module[i].pCtxData);
	}
}

void avio_sub_module_disable_irq(void)
{
	int i;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->disable_irq)
			(*fops->disable_irq)(g_avio_module[i].pCtxData);
	}
}

static void avio_sub_module_exit_n_module(int n)
{
	int i;

	for (i = 0; i < n; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->exit)
			(*fops->exit)(g_avio_module[i].pCtxData);
	}
}

int avio_sub_module_init(void)
{
	int err = 0;
	int i;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;
		char *mod_name = g_avio_module[i].module_name;

		if (!fops->init)
			continue;

		mod_name = mod_name ? mod_name : "UNKNOWN";
		err = (*fops->init)(g_avio_module[i].pCtxData);
		if (err == 0) {
			avio_trace("%s: init done\n", mod_name);
		} else {
			avio_trace("%s: init failed : %x\n", mod_name, err);
			avio_sub_module_exit_n_module(i);
			return err;
		}
	}

	return err;
}

int avio_sub_module_exit(void)
{
	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	avio_sub_module_exit_n_module(AVIO_MODULE_TYPE_MAX);

	return 0;
}

static void avio_sub_module_close_n_module(int n)
{
	int i;

	for (i = 0; i < n; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->close)
			(*fops->close)(g_avio_module[i].pCtxData);
	}
}

int avio_sub_module_open(void)
{
	int err = 0;
	int i;
	AVIO_MODULE_FUNC_TABLE *fops;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	//Initialize DHUB driver firstly
	i = AVIO_MODULE_TYPE_DHUB;
	fops = &g_avio_module[i].func;
	if (fops->open) {
		err = (*fops->open)(g_avio_module[i].pCtxData);
		if (err)
			return err;
	}

	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		fops = &g_avio_module[i].func;

		if ((i != AVIO_MODULE_TYPE_DHUB) && fops->open) {
			err = (*fops->open)(g_avio_module[i].pCtxData);
			//If error, close modules opened successfully
			if (err != 0)
				avio_sub_module_close_n_module(i);
		}
	}

	return err;
}

int avio_sub_module_close(void)
{
	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	avio_sub_module_close_n_module(AVIO_MODULE_TYPE_MAX);

	return 0;
}

int avio_sub_module_save_state(void)
{
	int err = 0;
	int i;

	for (i = AVIO_MODULE_TYPE_MAX - 1; i >= 0; i--) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->save_state) {
			avio_debug("Save state for %s\n", g_avio_module[i].module_name);
			err = (*fops->save_state)(g_avio_module[i].pCtxData);
			if (err) {
				avio_error("%s:save_state failed for module %d\n", __func__, i);
				return err;
			}
		}
	}

	return err;
}

int avio_sub_module_restore_state(void)
{
	int err = 0;
	int i;

	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->restore_state) {
			avio_debug("Restore state for %s\n", g_avio_module[i].module_name);
			err = (*fops->restore_state)(g_avio_module[i].pCtxData);
			if (err) {
				avio_error("%s:restore_state failed for module %d\n", __func__, i);
				return err;
			}
		}
	}

	return err;
}


void avio_sub_module_create_proc_file(struct proc_dir_entry *dev_procdir)
{
	int i;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->create_proc_file)
			(*fops->create_proc_file)(g_avio_module[i].pCtxData,
				dev_procdir);
	}
}

void avio_sub_module_remove_proc_file(struct proc_dir_entry *dev_procdir)
{
	int i;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);

	for (i = 0; i < AVIO_MODULE_TYPE_MAX; i++) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[i].func;

		if (fops->remove_proc_file)
			(*fops->remove_proc_file)(g_avio_module[i].pCtxData,
				dev_procdir);
	}

}

void avio_sub_module_register(AVIO_MODULE_TYPE sub_module, char *module_name,
			void *pCtxData, const AVIO_MODULE_FUNC_TABLE *p_func_table)
{

	if (sub_module < AVIO_MODULE_TYPE_MAX) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[sub_module].func;

		g_avio_module[sub_module].module_name = module_name;
		g_avio_module[sub_module].pCtxData = pCtxData;

		if (!p_func_table)
			return;

		memcpy(fops, p_func_table, sizeof(AVIO_MODULE_FUNC_TABLE));

		//If not yet probed - dynamic registration, then invoke init sequence
		if (!g_avio_module[sub_module].is_probed) {
			g_avio_module[sub_module].is_probed = 1;
			if (fops->config)
				(*fops->config)(g_avio_module[sub_module].pCtxData, NULL);
			if (fops->init)
				(*fops->init)(g_avio_module[sub_module].pCtxData);
			if (fops->open)
				(*fops->open)(g_avio_module[sub_module].pCtxData);
		}
	}
}

void avio_sub_module_unregister(AVIO_MODULE_TYPE sub_module)
{
	if (sub_module < AVIO_MODULE_TYPE_MAX) {
		AVIO_MODULE_FUNC_TABLE *fops = &g_avio_module[sub_module].func;

		//Invoke exit sequence for dynamically unloading
		if (fops->close)
			(*fops->close)(g_avio_module[sub_module].pCtxData);
		if (fops->exit)
			(*fops->exit)(g_avio_module[sub_module].pCtxData);

		//Remove fops
		memset(fops, 0, sizeof(AVIO_MODULE_FUNC_TABLE));

		//Allow re-regiser and re-probe again for dyanmic module load/unload.
		g_avio_module[sub_module].is_probed = 0;
	}
}

void *avio_sub_module_get_ctx(AVIO_MODULE_TYPE sub_module)
{
	void *pCtxData = NULL;

	if (sub_module < AVIO_MODULE_TYPE_MAX)
		pCtxData = g_avio_module[sub_module].pCtxData;

	return pCtxData;
}

int avio_sub_module_dhub_init(void)
{
	int err = 0;
	AVIO_MODULE_FUNC_TABLE *fops = NULL;

	avio_trace("avio_module:%s:%d\n", __func__, __LINE__);
	fops = &g_avio_module[AVIO_MODULE_TYPE_DHUB].func;
	if (fops->open) {
		err = (*fops->open)(g_avio_module[AVIO_MODULE_TYPE_DHUB].pCtxData);
		if (err)
			avio_trace("dhub open failed: %x\n", err);
	}
	return err;
}
EXPORT_SYMBOL(avio_sub_module_dhub_init);
