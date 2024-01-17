#ifndef __ISP_DRIVER_H__
#define __ISP_DRIVER_H__

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>

#define ISP_DEVICE_NAME			"syna-isp"

#define isp_module_err_str(retVal)                  \
		(retVal ? "Failed" : "Done")
#define isp_module_is_invalid(mod)                  \
		((mod < 0 || mod >= ISP_MODULE_MAX))

typedef enum isp_module_names {
	ISP_MODULE_CORE,
	ISP_MODULE_DHUB,
	ISP_MODULE_MAX,
} isp_module_names;

typedef struct _isp_module_fops_t isp_module_fops;
typedef struct _isp_module_ctx_t {
	char *mod_name;
	void *mod_prv;
	atomic_t refcnt;
} isp_module_ctx;

typedef struct gpio_array_values_t {
	unsigned long *values;
	int nvalues;
} gpio_array_values;

typedef struct isp_device_t {
	struct cdev c_dev;
	struct class *dev_class;
	void __iomem *reg_base;
	resource_size_t reg_base_phys;
	resource_size_t reg_size;
	struct device *dev;
	struct gpio_descs *enable_gpios;
	gpio_array_values gpio_values;
	struct clk **isp_clks;
	struct proc_dir_entry *dev_procdir;
	struct mutex isp_mutex;

	isp_module_ctx mod_ctx[ISP_MODULE_MAX];
	struct i2c_adapter *i2c;
	u8 dev_addr;
} isp_device;

typedef struct isp_data_t {
	isp_device *isp_dev;
	isp_module_ctx *mod_ctx;
	int mod_type;
} isp_drv_data;

typedef int (*isp_mod_gen_fops)(isp_device *isp_dev, isp_module_ctx *mod_ctx);
typedef long (*isp_mod_ioctl)(isp_device *isp_dev, isp_module_ctx *mod_ctx,
			unsigned int cmd, unsigned long arg);

typedef struct _isp_module_fops_t {
	isp_mod_gen_fops probe;
	isp_mod_gen_fops open, close;
	isp_mod_ioctl ioctl;
	isp_mod_gen_fops exit;
	isp_mod_gen_fops suspend, resume;
} isp_module_fops;

typedef enum isp_ioctls_t {
	ISP_WAIT_FOR_EVENT,
	ISP_EVENT_ENABLE,
	ISP_EVENT_DISABLE,
	ISP_MODULE_CONFIG,
	ISP_SET_CLK,
	ISP_SELECT_SENSOR,
	ISP_SET_POWER,
	ISP_MODULE_IOCTL,
} ISP_IOCTLS;

typedef isp_module_fops* (*isp_mod_get_fops)(void);

#endif /* __ISP_DRIVER_H__ */
