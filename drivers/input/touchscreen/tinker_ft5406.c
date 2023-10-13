#include <linux/module.h>
#include <linux/ratelimit.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/kthread.h>
#include <asm/unaligned.h>
#define MAXIMUM_SUPPORTED_POINTS 10
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 480
enum edt_ver {
	EDT_M06,
	EDT_M09,
	EDT_M12,
	EV_FT,
	GENERIC_FT,
};
struct edt_reg_addr {
	int reg_threshold;
	int reg_report_rate;
	int reg_gain;
	int reg_offset;
	int reg_offset_x;
	int reg_offset_y;
	int reg_num_x;
	int reg_num_y;
};
struct edt_i2c_chip_data {
	int  max_support_points;
};
struct ft5406_regs {
	uint8_t device_mode;
	uint8_t gesture_id;
	uint8_t num_points;
	struct ft5406_touch {
		uint8_t xh;
		uint8_t xl;
		uint8_t yh;
		uint8_t yl;
		uint8_t res1;
		uint8_t res2;
	} point[MAXIMUM_SUPPORTED_POINTS];
};
struct ft5406 {
	struct i2c_client 	 	*client;
	struct input_dev       	*input_dev;
	void __iomem           	*ts_base;
	dma_addr_t		 		 bus_addr;
	struct task_struct     	*thread;
};
static int edt_ft5x06_ts_readwrite(struct i2c_client *client,
				   u16 wr_len, u8 *wr_buf,
				   u16 rd_len, u8 *rd_buf)
{
	struct i2c_msg wrmsg[2];
	int i = 0;
	int ret;
	if (wr_len) {
		wrmsg[i].addr  = client->addr;
		wrmsg[i].flags = 0;
		wrmsg[i].len = wr_len;
		wrmsg[i].buf = wr_buf;
		i++;
	}
	if (rd_len) {
		wrmsg[i].addr  = client->addr;
		wrmsg[i].flags = I2C_M_RD;
		wrmsg[i].len = rd_len;
		wrmsg[i].buf = rd_buf;
		i++;
	}
	ret = i2c_transfer(client->adapter, wrmsg, i);
	if (ret < 0)
		return ret;
	if (ret != i)
		return -EIO;
	return 0;
}
u_char read_reg(struct i2c_client *client, u_char addr) {
	u_char wbuf[1] = {addr}, rbuf[1];
	edt_ft5x06_ts_readwrite(client, 1, wbuf, 1, rbuf);
	return rbuf[0];
}
void write_reg(struct i2c_client *client, u_char addr, u_char val) {
	u_char wbuf[2] = {addr, val};
	edt_ft5x06_ts_readwrite(client, 1, wbuf, 0, NULL);
}
void read_touch_regs(struct i2c_client *client, struct ft5406_regs *regs){
	if(regs == NULL){
		dev_err(&(client->dev), "regs is NULL");
		return;
	}
	u_char cmd = 0;
	edt_ft5x06_ts_readwrite(client, 1, &cmd, sizeof(struct ft5406_regs), (u8 *) regs);
}
/* Thread to poll for touchscreen events
 * 
 */
static int ft5406_thread(void *arg) {
	struct ft5406 *ts = (struct ft5406 *) arg;
	struct ft5406_regs regs;
	int x = 0, y = 0;
	int known_ids = 0;
	printk(KERN_ERR"%s\n", __FUNCTION__);
	while(!kthread_should_stop())
	{
		msleep_interruptible(17);
		read_touch_regs(ts->client, &regs);
		int i;
		int modified_ids = 0, released_ids;
		for(i = 0; i < regs.num_points; i++) {
			x = (((int)regs.point[i].xh & 0xf) << 8) + regs.point[i].xl;
			y = (((int)regs.point[i].yh & 0xf) << 8) + regs.point[i].yl;
			int touchid = (regs.point[i].yh >> 4) & 0xf;
			modified_ids |= 1 << touchid;
			input_mt_slot(ts->input_dev, touchid);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,  800 - x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,  480 - y);
		}
		released_ids = known_ids & ~modified_ids;
		for(i = 0; released_ids && i < MAXIMUM_SUPPORTED_POINTS; i++)
		{
			if(released_ids & (1<<i))
			{
				input_mt_slot(ts->input_dev, i);
				input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
				modified_ids &= ~(1 << i);
			}
		}
		known_ids = modified_ids;
		input_mt_report_pointer_emulation(ts->input_dev, true);
		input_sync(ts->input_dev);
	}
	return 0;
}
static int rpi_ft5406_ts_probe(struct i2c_client *client,
					 const struct i2c_device_id *id)
{
	int err = 0;
	struct device *dev = &(client->dev);
	struct ft5406 * ts;
	ts = devm_kzalloc(dev, sizeof(struct ft5406), GFP_KERNEL);
	if (!ts) {
			dev_err(dev, "Failed to allocate memory\n");
			return -ENOMEM;
	}
	ts->input_dev = input_allocate_device();
	ts->client    = client;
	printk(KERN_ERR"%s\n", __FUNCTION__);
	
	ts->input_dev->name = "FT5406 i2c driver";
	__set_bit(EV_KEY, ts->input_dev->evbit);
	__set_bit(EV_SYN, ts->input_dev->evbit);
	__set_bit(EV_ABS, ts->input_dev->evbit);
	input_set_abs_params(ts->input_dev,ABS_X,0,SCREEN_WIDTH,0,0);
    input_set_abs_params(ts->input_dev,ABS_Y,0,SCREEN_HEIGHT,0,0);
	// mt
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0,
			     SCREEN_WIDTH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0,
			     SCREEN_HEIGHT, 0, 0);
	input_mt_init_slots(ts->input_dev, MAXIMUM_SUPPORTED_POINTS, INPUT_MT_DIRECT);
	i2c_set_clientdata(client, ts);
	err = input_register_device(ts->input_dev);
	if (err) {
		dev_err(dev, "could not register input device, %d\n",
			err);
		goto out;
	}
	// create thread to poll the touch events
	ts->thread = kthread_run(ft5406_thread, ts, "ft5406");
	if(ts->thread == NULL)
	{
		dev_err(dev, "Failed to create kernel thread");
		err = -ENOMEM;
		goto out;
	}
	return 0;
out:
	if (ts->input_dev) {
		input_unregister_device(ts->input_dev);
		ts->input_dev = NULL;
	}
	return err;
}
static int rpi_ft5406_ts_remove(struct i2c_client *client)
{
	struct ft5406 *ts = (struct ft5406 *) i2c_get_clientdata(client);
	
	kthread_stop(ts->thread);
	if (ts->input_dev)
		input_unregister_device(ts->input_dev);
	
	return 0;
}

static const struct edt_i2c_chip_data rpi_ft5406_data = {
	.max_support_points = MAXIMUM_SUPPORTED_POINTS,
};
static const struct i2c_device_id rpi_ft5406_ts_id[] = {
	{ .name = "tinker-ft5406", .driver_data = (long)&rpi_ft5406_data },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, rpi_ft5406_ts_id);
static const struct of_device_id rpi_ft5406_of_match[] = {
	{ .compatible = "tinker_ft5406", .data = &rpi_ft5406_data },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rpi_ft5406_of_match);
static struct i2c_driver rpi_ft5406_ts_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "tinker_ft5406",
		.of_match_table = rpi_ft5406_of_match,
	},
	.id_table = rpi_ft5406_ts_id,
	.probe    = rpi_ft5406_ts_probe,
	.remove   = rpi_ft5406_ts_remove,
};
module_i2c_driver(rpi_ft5406_ts_driver);
MODULE_AUTHOR("xlq ");
MODULE_DESCRIPTION("rpi FT5x06 I2C Touchscreen Driver");
MODULE_LICENSE("GPL v2");
