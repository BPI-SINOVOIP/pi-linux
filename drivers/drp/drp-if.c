/*
 * Driver for the Renesas RZ/V2H, RZ/V2N DRP unit
 *
 * Copyright (C) 2023-2024 Renesas Electronics Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <asm/cacheflush.h>
#include <asm/current.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/version.h>
#include <linux/drp.h>    /* Header file for DRP-AI Driver */
#include "drp-core.h"     /* Header file for DRP-AI Core */
#include "lock_drp.h"     /* multi OS exclusion control */
#include <generated/autoconf.h>

// #define DRP_DRV_DEBUG_WAIT
#ifdef DRP_DRV_DEBUG_WAIT
#define DRP_DEBUG_WAIT(...) msleep(1100);
#else
#define DRP_DEBUG_WAIT(...)
#endif

#ifdef DRP_DRV_DEBUG
#define DRP_DRV_DEBUG_MODE        " (Debug Mode ON)"
#else
#define DRP_DRV_DEBUG_MODE        ""
#endif

#ifdef DRP_DRV_DEBUG_WAIT
#define DRP_DRV_DEBUG_WAIT_MODE   " (Debug Wait Mode ON)"
#else
#define DRP_DRV_DEBUG_WAIT_MODE   ""
#endif

/*Macro definitions*/
#define SYS_SIZE                    (1024)
#define SYS_DRP_BANK                (0x38)
#define SYS_MASK_DRP                (0x00000300)
#define SYS_SHIFT                   (24)

#define DRP_DRIVER_VERSION        "1.20 rel.3"
#define DRP_DEV_NUM                (1)
#define DRP_DRIVER_NAME            "drp"     /* Device name */
#define DRP_64BYTE_ALIGN           (0x3F)      /* Check 64-byte alignment */
#define DRP_STATUS_IDLE_RW         (10)
#define DRP_STATUS_ASSIGN          (11)
#define DRP_STATUS_READ_MEM        (13)
#define DRP_STATUS_WRITE           (15)

#define DRP_SGL_DRP_DESC_SIZE      (80)
#define DRP_DESC_CMD_SIZE          (16)
#define DRP_SINGLE_DESC_SIZE_BYTE  (64)  // DRP single descriptor size is 64Byte (16byte x 4 descriptor)
#define DRP_CMA_SIZE               ((DRP_SGL_DRP_DESC_SIZE * DRP_SEQ_NUM) + DRP_DESC_CMD_SIZE + 64)

#define DRP_MAX_PROCESS_CFG        (1)
#define MAX_SEM_TIMEOUT             (msecs_to_jiffies(1000))
#define DRP_IRQ_CHECK_ENABLE        (1)
#define DRP_IRQ_CHECK_DISABLE       (0)

#if 1 /* for CPG direct access (preliminary) */
#define CPG_SIZE                    (0x10000)
#define CPG_BASE_ADDRESS            (0x10420000uLL)
#endif

/* from drp-core.h */
#define DEVICE_RZV2MA               (0)
#define DEVICE_RZV2H                (1)

/* preliminary for V2H */
#undef CONFIG_ARCH_R9A09G011GBG
#undef CONFIG_ARCH_R9A09G055MA3GBG
#undef CONFIG_ARCH_R9A07G054

#if defined(CONFIG_ARCH_R9A09G056)
/* V2N conditional compilation */
#define DRP_CH_NUM                  (1)
#define AIMAC_CH_NUM                (1)
#define ENABLE_DRP_SUPPORT_SHARED_MEMORY
#undef CONFIG_DRP_SUPPORT_MULTI_OS
#else
/* V2H conditional compilation */
#define DRP_CH_NUM                  (2)
#endif

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
#define DRP_DRV_USE_SHARED_MEMORY_MODE   " (Use Shared Memory Mode ON)"
#else
#define DRP_DRV_USE_SHARED_MEMORY_MODE   ""
#endif

/* drp device channel no */
#if defined(CONFIG_ARCH_R9A09G056)
#define DRP_CH                      (0)     //(V2N) DRP-AI:0
#else
#define DRP_CH                      (1)     //(V2H) DRP-AI:0, DRP:1
#endif

#define VAL_40BIT_OVER  (0x10000000000uLL)
#define VAL_16M         (0x0000000001000000uLL)

/* A function called from the kernel */
static int drp_probe(struct platform_device *pdev);
static int drp_remove(struct platform_device *pdev);
static int drp_open(struct inode *inode, struct file *file);
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
static void drp_shutdown(struct platform_device *pdev);
#endif
static int drp_close(struct inode *inode, struct file *file);
static int drp_flush(struct file *file, fl_owner_t id);
static ssize_t  drp_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
static ssize_t  drp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static long drp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg); 
static unsigned int drp_poll( struct file* filp, poll_table* wait );
static irqreturn_t irq_drp_nmlint(int irq, void *dev);
static irqreturn_t irq_drp_errint(int irq, void *dev);

/* Internal function */
static int drp_regist_driver(void);
static int drp_regist_device(struct platform_device *pdev);
static void drp_unregist_driver(void);
static void drp_unregist_device(void);
static void drp_init_device(uint32_t ch);
static int8_t drp_cpg_reset(uint32_t ch);
static int8_t drp_stop_device(uint32_t ch);
static long drp_ioctl_assign(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_start(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_get_status(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_reset(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_set_seq(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_get_codec_area(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_get_opencva_area(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_set_drp_freq(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_read_drp_reg(struct file *filp, unsigned int cmd, unsigned long arg);
static long drp_ioctl_write_drp_reg(struct file *filp, unsigned int cmd, unsigned long arg);

static int drp_drp_cpg_init(void);

#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
static int drp_flag_test_and_set(unsigned int num);
static void drp_flag_clear(unsigned int num);
static int drp_flag_test(unsigned int num);
#endif

/* Linux device driver initialization */
static const unsigned int MINOR_BASE = 1;
static const unsigned int MINOR_NUM  = DRP_DEV_NUM;       /* Minor number */
static unsigned int drp_major;                    /* Major number (decided dinamically) */
static struct cdev drp_cdev;                      /* Character device object */
static struct class *drp_class = NULL;            /* class object */
struct device *drp_device_array[DRP_DEV_NUM];

/* for 40bit address */
#define ADR_CONV_MASK   (0x000000FFFF000000uLL)
#define ADR_LOW_24BIT   (0x00FFFFFFuL)
#define MAX_IODATA_NUM  (PARAM_ADDRESS_NUM)
#define ADRCONV_TBL_NUM  (256)
static uint64_t drp_adrconv_tbl[ADRCONV_TBL_NUM];
static uint64_t last_drp_config_address = 0;

/* for Config load skip */
#define LOAD_SKIP_OFFSET    (16)

/* for change DRP frequency */
#define DRP_DIVFIX_MIN      (2)
#define DRP_DIVFIX_MAX      (127)

/* for multi OS exclusion control */
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
static unsigned long long *drp_os_exclusion;
static resource_size_t drp_region_drp_multi_os_base_addr = 0;
static resource_size_t drp_region_drp_multi_os_size = 0;
#define DRPFLAG_DRP_USED    (0)
#define DRPFLAG_DRP_INIT    (1)
#define DRPFLAG_CLK_STOP    (2)
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
static unsigned long long *drp_shared_exclusion;
static resource_size_t drp_shared_base_addr = 0;
static resource_size_t drp_shared_size = 0;
static DEFINE_SPINLOCK(shared_mem_lock);
#endif
static int drp_irqnum_nmlint;
static int drp_irqnum_errint;

struct drp_priv {
    struct platform_device *pdev;
    const char *dev_name;
    spinlock_t lock;
    void __iomem *drp_base;
    struct semaphore sem;
    uint32_t drp_irq_flag;
    struct reset_control *rstc;
    refcount_t count;
    drp_status_t drp_status;
    /* Set first proc flag */
    uint32_t drp_first_proc_after_init;
};

struct drp_desc_info
{
    drp_seq_t seq;
    char* vaddr;
    uint64_t phyaddr;
    uint64_t drp_desc_adr_40bit;
    uint32_t drp_iodata_num;
    uint32_t drp_load_force;
    uint32_t drp_mindiv;
};

/* Virtual base address of register */
static void __iomem *drp_base_addr[DRP_CH_NUM];
#if defined(CONFIG_ARCH_R9A09G056)
static void __iomem *aimac_base_address[AIMAC_CH_NUM];
static resource_size_t aimac_size;
#endif

/* V2H CPG Control */
#define DRP_CPG_CTL (1)     //for CPG direct access (preliminary)
#ifdef DRP_CPG_CTL
static void __iomem *cpg_base_address;
static resource_size_t cpg_size;
#endif
static resource_size_t drp_size;

/* Virtual base address of register */
static resource_size_t drp_size;
static resource_size_t drp_region_codec_base_addr = 0;
static resource_size_t drp_region_codec_size = 0;
static resource_size_t drp_region_oca_base_addr = 0;
static resource_size_t drp_region_oca_size = 0;

/* handler table */
static struct file_operations s_mydevice_fops = {
    .open           = drp_open,
    .release        = drp_close,
    .write          = drp_write,
    .read           = drp_read,
    .unlocked_ioctl = drp_ioctl,
    .compat_ioctl   = drp_ioctl, /* for 32-bit App */
    .poll           = drp_poll,
    .flush          = drp_flush,
};

static const struct of_device_id drp_match[] = {

    { .compatible = "renesas,rzv2ma-drp",},
    { .compatible = "renesas,rzv2h-drp",},
    { .compatible = "renesas,rzv2n-drp",},
    { /* sentinel */ }
};
static struct platform_driver drp_platform_driver = {
    .driver = {
        .name   = "drp-rz",
        .of_match_table = drp_match,
    },
    .probe      = drp_probe,
    .remove     = drp_remove,
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    .shutdown   = drp_shutdown,
#endif
};

static struct drp_priv *drp_priv;
static DECLARE_WAIT_QUEUE_HEAD(read_q);
static DEFINE_SEMAPHORE(rw_sem);
static drp_data_t drp_data;
static uint32_t rw_status;
static uint32_t write_count;
static uint32_t read_count;

/* DRP single operation */
static drp_data_t proc[DRP_SEQ_NUM * 2];
/*DRP Descriptor*/
// 1. Load drpcfg
// 2. Set DRP core
// 3. Read DRP param
// 4. Start processing of DRP
// 5. Link descriptor
// 6. AIMAC descriptor
static unsigned char drp_single_desc_bin[] =
{
  0x00, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x07, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x88, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static drp_seq_t seq;
static drp_odif_intcnto_t odif_intcnto;

static int drp_probe(struct platform_device *pdev)
{
    int ret;

    ret = drp_regist_driver();
    if (0 != ret)
    {
        return ret;
    }

    ret = drp_regist_device(pdev);
    if (0 != ret)
    {
        drp_unregist_driver();
        return ret;
    }

    return ret;
}

static int drp_remove(struct platform_device *pdev)
{
    drp_unregist_driver();
    drp_unregist_device();

    return 0;
}

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
static void drp_shutdown(struct platform_device *pdev)
{
    int flag_ret = 0;
    DRP_DEBUG_PRINT("start.\n");

    drp_shared_exclusion = phys_to_virt(drp_shared_base_addr);
    flag_ret = R_DRP_IsSharedMemoryInitialized(drp_shared_exclusion);
    if(-1 == flag_ret)
    {
        // If not cleared shared memory, clear the shared memory
        if(0 != R_DRP_ClearHashArea(drp_shared_exclusion))
        {
            dev_err(&pdev->dev, "Failed to clear the shared memory region. Power off the board to reset the memory and then boot the board again.");
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,1)
        dcache_clean_inval_poc((unsigned long)drp_shared_exclusion, (unsigned long)drp_shared_exclusion + drp_shared_size);
#else
        __flush_dcache_area(drp_shared_exclusion, drp_shared_size);
#endif
        dev_info(&pdev->dev, "Shared memory region has cleared.");
    }
    else
    {
        // If cleared shared memory
        dev_info(&pdev->dev, "Shared memory region has already cleared.");
    }
    
    DRP_DEBUG_PRINT("end.\n");
    
    goto end;
end:
    return;
}
#endif

static int drp_open(struct inode *inode, struct file *file)
{
    int result = 0;
    struct drp_priv *priv = drp_priv;
    unsigned long flags;
    struct drp_desc_info *desc_info;
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    int flag_drp_used = -1;
    int flag_drp_init = -1;
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    int flag_drp_shared_used = -1;
    int flag_drp_shared_init = -1;
#endif

    DRP_DEBUG_PRINT("start.\n");
    DRP_DEBUG_PRINT("status1:%d\n", priv->drp_status.status);
    DRP_DEBUG_PRINT("status_rw1:%d\n", rw_status);

    if(unlikely(down_timeout(&priv->sem, MAX_SEM_TIMEOUT)))
    {
        result = -ETIMEDOUT;
        goto end;
    }

    DRP_DEBUG_WAIT();

    if(1 == refcount_read(&priv->count))
    {
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
        /* DRPFLAG_DRP_LOCK is set */
        flag_drp_shared_used = R_DRP_LockDrpaiContStatus(drp_shared_exclusion,
                                                         &shared_mem_lock,
                                                         PROCESS_CONTEXT,
                                                         DRPFLAG_DRP_LOCK);
        if(-1 == flag_drp_shared_used)
        {
            result = -EINPROGRESS;
            goto end;
        }
        if( 0 != flag_drp_shared_used)
        {
            result = -EADDRNOTAVAIL;
            goto end;
        }
#endif
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
        flag_drp_used = drp_flag_test_and_set(DRPFLAG_DRP_USED);
        if( -1 == flag_drp_used)
        {
            /* DRPFLAG_DRP_USED is already set */
            result = -EALREADY;
            goto end;
        }
        if( 0 != flag_drp_used)
        {
            /* OS shared memory unavailable */
            result = -ENODATA;
            goto end;
        }

        flag_drp_init = drp_flag_test_and_set(DRPFLAG_DRP_INIT);
        if( 0 != flag_drp_init)
        {
            /* DRPFLAG_DRP_INIT is already set */
            /* DO NOTHING */
        }
        else
        {
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
        flag_drp_shared_init = R_DRP_IsDrpaiHwStatusActive(drp_shared_exclusion);
        if(-1 == flag_drp_shared_init)
        {
            /* DRP-AI is stopped. therefore, initialize the DRP-AI first */
#endif
            /* Initialize CPG (DRP CPG On) */
            if(R_DRP_SUCCESS != drp_drp_cpg_init())
            {
                result = -EIO;
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
                drp_flag_clear(DRPFLAG_DRP_INIT);
#endif
                goto end;
            }

            /* Initialize DRP procedure */
            drp_init_device(DRP_CH);

            /* Finalize DRP procedure */
            if(R_DRP_SUCCESS != drp_stop_device(DRP_CH))
            {
                result = -EIO;
                DRP_DEBUG_PRINT("Reset failed\n");
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
                drp_flag_clear(DRPFLAG_DRP_INIT);
#endif
                goto end;
            }
            /* Reset DRP (CPG Reset)*/
            if(R_DRP_SUCCESS != drp_cpg_reset(DRP_CH))
            {
                result = -EIO;
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
                drp_flag_clear(DRPFLAG_DRP_INIT);
#endif
                goto end;
            }

            /* Initialize DRP procedure */
            drp_init_device(DRP_CH);

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
        }
#endif
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
            drp_flag_clear(DRPFLAG_CLK_STOP);
        }
#endif

        /* INIT -> IDLE */
        spin_lock_irqsave(&priv->lock, flags);
        priv->drp_status.status = DRP_STATUS_IDLE;
        /* Set first proc flag */
        priv->drp_first_proc_after_init = 1;
        spin_unlock_irqrestore(&priv->lock, flags);

        /* Initialization flag */
        rw_status = DRP_STATUS_IDLE_RW;
        
    }

    desc_info = kzalloc(sizeof(struct drp_desc_info), GFP_KERNEL);
    if (!desc_info)
    {
        result = -ENOMEM;
        goto end;
    }

    /* DRP single operation */
    desc_info->vaddr = NULL;
    desc_info->phyaddr = 0x0;
    file->private_data = desc_info;
    desc_info->drp_mindiv = 2;

    /* Increment reference count */
    refcount_inc(&priv->count);

    DRP_DEBUG_PRINT("status2:%d\n", priv->drp_status.status);
    DRP_DEBUG_PRINT("status_rw2:%d\n", rw_status);

    goto end;
end:
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    if(0 == result)
    {
        R_DRP_RecordDrpaiActiveStatus(drp_shared_exclusion,
                                      &shared_mem_lock,
                                      PROCESS_CONTEXT);
    }
#endif
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    if( 0 == flag_drp_used)
    {
        drp_flag_clear(DRPFLAG_DRP_USED);
    }
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    if( 0 == flag_drp_shared_used )
    {
        R_DRP_UnlockDrpaiContStatus(drp_shared_exclusion,
                                    &shared_mem_lock,
                                    PROCESS_CONTEXT,
                                    DRPFLAG_DRP_LOCK);
    }
#endif
    if((-ETIMEDOUT != result))
    {
        up(&priv->sem);
    }

    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static int drp_close(struct inode *inode, struct file *file)
{
    int result = 0;
    struct drp_priv *priv = drp_priv;
    unsigned long flags;
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    int flag_drp_used = -1;
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    int flag_drp_shared_used = -1;
    int flag_drp_shared_init = -1;
#endif

    DRP_DEBUG_PRINT("start.\n");

    if(unlikely(down_timeout(&priv->sem, MAX_SEM_TIMEOUT))) 
    {
        /* Note: this errno won't be returned to user*/
        result = -ETIMEDOUT;
        DRP_DEBUG_PRINT("API semaphore obtained failed\n");
        goto end;
    }

    DRP_DEBUG_WAIT();

    DRP_DEBUG_PRINT("status1:%d\n", priv->drp_status.status);
    DRP_DEBUG_PRINT("status_rw1:%d\n", rw_status);

    if(2 == refcount_read(&priv->count))
    {
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
        /* DRPFLAG_DRP_LOCK is set */
        flag_drp_shared_used = R_DRP_LockDrpaiContStatus(drp_shared_exclusion,
                                                         &shared_mem_lock,
                                                         PROCESS_CONTEXT,
                                                         DRPFLAG_DRP_LOCK);
        if(-1 == flag_drp_shared_used)
        {
            result = -EINPROGRESS;
            goto end;
        }
        if( 0 != flag_drp_shared_used)
        {
            result = -EADDRNOTAVAIL;
            goto end;
        }
#endif
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
        flag_drp_used = drp_flag_test_and_set(DRPFLAG_DRP_USED);
        if( 0 == flag_drp_used)
        {
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
        flag_drp_shared_init = R_DRP_IsDrpaiHwStatusActive(drp_shared_exclusion);
        if(0 == flag_drp_shared_init)
        {
            /* The DRP-AI is running. Therefore, stop the DRP-AI first. */
#endif
            /* Finalize DRP procedure */
            if(R_DRP_SUCCESS != drp_stop_device(DRP_CH))
            {
                DRP_DEBUG_PRINT("Reset failed\n");
            }
            /* Reset DRP (CPG Reset)*/
            if(R_DRP_SUCCESS != drp_cpg_reset(DRP_CH))
            {
                result = -EIO;
            }

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
            R_DRP_RecordDrpaiInactiveStatus(drp_shared_exclusion,
                                            &shared_mem_lock,
                                            PROCESS_CONTEXT);
        }
#endif
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
            drp_flag_clear(DRPFLAG_DRP_INIT);
        }
#endif
        /* IDLE -> INIT */
        /* RUN  -> INIT */
        spin_lock_irqsave(&priv->lock, flags);
        priv->drp_status.status = DRP_STATUS_INIT;
        priv->drp_status.err    = DRP_ERRINFO_SUCCESS;

        /* IDLE_RW */
        rw_status = DRP_STATUS_IDLE_RW;
        spin_unlock_irqrestore(&priv->lock, flags);
    }

    DRP_DEBUG_PRINT("status2:%d\n", priv->drp_status.status);
    DRP_DEBUG_PRINT("status_rw2:%d\n", rw_status);

    DRP_DEBUG_WAIT();

    goto end;
end:
    /* Decrement referenece count */
    refcount_dec(&priv->count);
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    if( 0 == flag_drp_used)
    {
        drp_flag_clear(DRPFLAG_DRP_USED);
    }
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    if( 0 == flag_drp_shared_used )
    {
        R_DRP_UnlockDrpaiContStatus(drp_shared_exclusion,
                                    &shared_mem_lock,
                                    PROCESS_CONTEXT,
                                    DRPFLAG_DRP_LOCK);
    }
#endif
    if((-ETIMEDOUT != result))
    {
        up(&priv->sem);
    if(file->private_data) 
    {
        DRP_DEBUG_PRINT("kfree is called\n");
        kfree(file->private_data);
        file->private_data = NULL;
    }
    }

    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static int drp_flush(struct file *file, fl_owner_t id)
{
    DRP_DEBUG_PRINT("start.\n");
    DRP_DEBUG_PRINT("end.\n");
    return 0;
}

static ssize_t  drp_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t result = 0;
    void *p_drp_cma = 0;
    uint64_t addr;

    DRP_DEBUG_PRINT("start.\n");

    if(unlikely(down_trylock(&rw_sem)))
    {
        result = -ERESTART;
        goto end;
    }

    DRP_DEBUG_PRINT("status_rw1:%d\n", rw_status);

    DRP_DEBUG_WAIT();

    /* Check status */
    if (!((DRP_STATUS_ASSIGN == rw_status) || (DRP_STATUS_WRITE == rw_status)))
    {
        result = -EACCES;
        goto end;
    }

    /* Check Argument */
    if (NULL == buf)
    {
        result = -EFAULT;
        goto end;
    }
    if (0 == count)
    {
        result = -EINVAL;
        goto end;
    }

    /* DRP_STATUS_ASSIGN -> DRP_STATUS_WRITE */
    /* DRP_STATUS_WRITE  -> DRP_STATUS_WRITE */
    rw_status = DRP_STATUS_WRITE;
    DRP_DEBUG_PRINT("status_rw2:%d\n", rw_status);

    /* Expand to DRP for CMA */
    addr = (uint64_t)drp_data.address;
    p_drp_cma = phys_to_virt(addr + (uint64_t)write_count);
    if (p_drp_cma == 0)
    {
        result = -EFAULT;
        goto end;
    }
    if ( !( drp_data.size >= (write_count + count) ) )
    {
        count = drp_data.size - write_count;
    }
    if (copy_from_user(p_drp_cma, buf, count))
    {
        result = -EFAULT;
        goto end;
    }
    write_count = write_count + count;

    /* DRP_STATUS_WRITE -> DRP_STATUS_IDLE_RW */
    if (drp_data.size <= write_count)
    {
        p_drp_cma = phys_to_virt(addr);
        if (p_drp_cma == 0)
        {
            result = -EFAULT;
            goto end;
        }
        // Write the data from the cache back to the main memory.
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,1)
        dcache_clean_inval_poc((unsigned long)p_drp_cma, (unsigned long)p_drp_cma + drp_data.size);
#else
        __flush_dcache_area(p_drp_cma, drp_data.size);
#endif
        rw_status = DRP_STATUS_IDLE_RW;
    }

    result = count;

    DRP_DEBUG_WAIT();
    goto end;
end:
    if(-ERESTART != result)
    {
        up(&rw_sem);
    }
    DRP_DEBUG_PRINT("status_rw3:%d\n", rw_status);
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static ssize_t drp_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t result = 0;
    void *p_drp_cma = 0;
    uint32_t i;
    uint64_t addr;

    DRP_DEBUG_PRINT("start.\n");

    if(unlikely(down_trylock(&rw_sem)))
    {
        result = -ERESTART;
        goto end;
    }

    DRP_DEBUG_PRINT("status_rw1:%d\n", rw_status);

    DRP_DEBUG_WAIT();

    /* Check status */
    if (!((DRP_STATUS_ASSIGN  == rw_status) ||
        (DRP_STATUS_READ_MEM  == rw_status)))
    {
        result = -EACCES;
        goto end;
    }

    /* Check Argument */
    if (NULL == buf)
    {
        result = -EFAULT;
        goto end;
    }
    if (0 == count)
    {
        result = -EINVAL;
        goto end;
    }

    /* DRP_STATUS_ASSIGN -> DRP_STATUS_READ_MEM */
    if (DRP_STATUS_ASSIGN == rw_status)
    {
        rw_status = DRP_STATUS_READ_MEM;
    }
    else
    {
        ; /* Do nothing */
    }
    DRP_DEBUG_PRINT("status_rw2:%d\n", rw_status);

    /* Read DRP-AI memory */
    if (DRP_STATUS_READ_MEM == rw_status)
    {
        addr = (uint64_t)drp_data.address;
        p_drp_cma = phys_to_virt(addr + (uint64_t)read_count);
        if (p_drp_cma == 0)
        {
            result = -EFAULT;
            goto end;
        }
        if ( !( drp_data.size >= (read_count + count) ) )
        {
            count = drp_data.size - read_count;
        }
        /* Copy arguments from kernel space to user space */
        if (copy_to_user(buf, p_drp_cma, count))
        {
            result = -EFAULT;
            goto end;
        }
        read_count = read_count + count;

        /* DRP_STATUS_READ -> DRP_STATUS_IDLE_RW */
        if (drp_data.size <= read_count)
        {
            rw_status = DRP_STATUS_IDLE_RW;
        }
        i = count;
    }
    else
    {
        ; /* Do nothing */
    }

    result = i;

    DRP_DEBUG_WAIT();
    goto end;
end:
    if(-ERESTART != result)
    {
        up(&rw_sem);
    }
    DRP_DEBUG_PRINT("status_rw3:%d\n", rw_status);
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static long drp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;

    switch (cmd) {
    case DRP_ASSIGN:
        DRP_DEBUG_PRINT("ioctl(DRP_ASSIGN)\n");
        result = drp_ioctl_assign(filp, cmd, arg);
        break;
    case DRP_START:
        DRP_DEBUG_PRINT("ioctl(DRP_START)\n");
        result = drp_ioctl_start(filp, cmd, arg);
        break;
    case DRP_RESET:
        DRP_DEBUG_PRINT("ioctl(DRP_RESET)\n");
        result = drp_ioctl_reset(filp, cmd, arg);
        break;
    case DRP_GET_STATUS:
        DRP_DEBUG_PRINT("ioctl(DRP_GET_STATUS)\n");
        result = drp_ioctl_get_status(filp, cmd, arg);
        break;
    case DRP_SET_SEQ:
        DRP_DEBUG_PRINT("ioctl(DRP_SET_SEQ)\n");
        result = drp_ioctl_set_seq(filp, cmd, arg);
        break;
    case DRP_GET_CODEC_AREA:
        DRP_DEBUG_PRINT("ioctl(DRP_GET_CODEC_AREA)\n");
        result = drp_ioctl_get_codec_area(filp, cmd, arg);
        break;
    case DRP_GET_OPENCVA_AREA:
        DRP_DEBUG_PRINT("ioctl(DRP_GET_OPENCVA_AREA)\n");
        result = drp_ioctl_get_opencva_area(filp, cmd, arg);
        break;
    case DRP_SET_DRP_MAX_FREQ:
        DRP_DEBUG_PRINT("ioctl(DRP_SET_DRP_FREQ)\n");
        result = drp_ioctl_set_drp_freq(filp, cmd, arg);
        break;
    case DRP_READ_DRP_REG:
        DRP_DEBUG_PRINT("[ioctl(DRP_READ_DRP_REG)]\n");
        result = drp_ioctl_read_drp_reg(filp, cmd, arg);
        break;
    case DRP_WRITE_DRP_REG:
        DRP_DEBUG_PRINT("[ioctl(DRP_WRITE_DRP_REG)]\n");
        result = drp_ioctl_write_drp_reg(filp, cmd, arg);
        break;
    default:
        DRP_DEBUG_PRINT("unsupported command %d\n", cmd);
        result = -EFAULT;
        break;
    }
    goto end;

end:
    return result;
}

static unsigned int drp_poll( struct file* filp, poll_table* wait )
{
    unsigned int retmask = 0;
    struct drp_priv *priv = drp_priv;
    unsigned long flags;

    DRP_DEBUG_PRINT("start.\n");

    spin_lock_irqsave(&priv->lock, flags);
    poll_wait( filp, &read_q,  wait );

    if (DRP_IRQ_CHECK_DISABLE == priv->drp_irq_flag)
    {
        /* Notify wakeup to user */
        retmask |= ( POLLIN  | POLLRDNORM );
    }
    spin_unlock_irqrestore(&priv->lock, flags);

    DRP_DEBUG_PRINT("end.\n");
    return retmask;
}
static irqreturn_t irq_drp_nmlint(int irq, void *dev)
{
    drp_odif_intcnto_t local_odif_intcnto;
    struct drp_priv *priv = drp_priv;
    unsigned long flags;

    DRP_DEBUG_PRINT("start.\n");
    DRP_DEBUG_PRINT("status1:%d\n", priv->drp_status.status);

    spin_lock_irqsave(&priv->lock, flags);

    /* DRP normal interrupt processing */
    R_DRP_DRP_Nmlint(drp_base_addr[0], 0, &local_odif_intcnto);

    odif_intcnto.ch0 += local_odif_intcnto.ch0;

    DRP_DEBUG_PRINT("ODIF_INTCNTO0 : 0x%08X\n", odif_intcnto.ch0);

    if (seq.num == odif_intcnto.ch0)
    {
        int32_t drp_ret;
         /* Internal state update */
        priv->drp_status.status = DRP_STATUS_IDLE;
        priv->drp_irq_flag  = DRP_IRQ_CHECK_DISABLE;

        /* Wake up the process */
        drp_ret = R_DRP_DRP_CLR_Nmlint(drp_base_addr[0], 0);
        if( 0 != drp_ret )
        {
            /* Internal state update(ERROR) */
            priv->drp_status.err    = DRP_ERRINFO_DRP_ERR;
        }

#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
        drp_flag_clear(DRPFLAG_DRP_USED);
#endif
        wake_up_interruptible( &read_q );
    }
    else if(seq.num < odif_intcnto.ch0)
    {
        /* Internal state update(ERROR) */
        priv->drp_status.err    = DRP_ERRINFO_DRP_ERR;
        priv->drp_status.status = DRP_STATUS_IDLE;
        priv->drp_irq_flag  = DRP_IRQ_CHECK_DISABLE;
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
        drp_flag_clear(DRPFLAG_DRP_USED);
#endif
        wake_up_interruptible( &read_q );
    }
    else
    {
        /* DO NOTHING */
    }
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    R_DRP_UnlockDrpaiContStatus(drp_shared_exclusion,
                                &shared_mem_lock,
                                INTERRUPT_CONTEXT,
                                DRPFLAG_DRP_LOCK);
#endif
    spin_unlock_irqrestore(&priv->lock, flags);

    DRP_DEBUG_PRINT("status2:%d\n", priv->drp_status.status);
    DRP_DEBUG_PRINT("end.\n");
    return IRQ_HANDLED;
}
static irqreturn_t irq_drp_errint(int irq, void *dev)
{
    struct drp_priv *priv = drp_priv;
    unsigned long flags;
    irqreturn_t ret = IRQ_HANDLED;
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    int flag_drp_shared_used;
#endif

    DRP_DEBUG_PRINT("start.\n");
    DRP_DEBUG_PRINT("status1:%d\n", priv->drp_status.status);

    spin_lock_irqsave(&priv->lock, flags);
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    flag_drp_shared_used = R_DRP_IsDrpaiContStatusLocked(drp_shared_exclusion, DRPFLAG_DRP_LOCK);
    if(-1 == flag_drp_shared_used)
    {
        ret = IRQ_HANDLED;
        goto end;
    }
    if(0 > flag_drp_shared_used)
    {
        ret = IRQ_NONE;
        goto end;
    }
#endif
    /* DRP error interrupt processing */
    R_DRP_DRP_Errint(drp_base_addr[0], 0);

    /* Internal state update */
    priv->drp_status.err    = DRP_ERRINFO_DRP_ERR;
    priv->drp_status.status = DRP_STATUS_IDLE;
    priv->drp_irq_flag  = DRP_IRQ_CHECK_DISABLE;
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    drp_flag_clear(DRPFLAG_DRP_USED);
#endif

    /* Wake up the process */
    wake_up_interruptible( &read_q );

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
end:
    if(0 == flag_drp_shared_used)
    {
        R_DRP_UnlockDrpaiContStatus(drp_shared_exclusion,
                                    &shared_mem_lock,
                                    INTERRUPT_CONTEXT,
                                    DRPFLAG_DRP_LOCK);
    } 
#endif
    spin_unlock_irqrestore(&priv->lock, flags);

    DRP_DEBUG_PRINT("status2:%d\n", priv->drp_status.status);
    DRP_DEBUG_PRINT("end.\n");
    return ret;
}

static int drp_regist_driver(void)
{
    int alloc_ret = 0;
    int cdev_err = 0;
    dev_t dev;
    int minor;
    int ptr_err;

    DRP_DEBUG_PRINT("start.\n");

    /* Get free major number. */
    alloc_ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRP_DRIVER_NAME);
    if (alloc_ret != 0) {
        pr_err("DRP Driver: alloc_chrdev_region = %d\n", alloc_ret);
        return -ENOMEM;
    }

    /* Save major number. */
    drp_major = MAJOR(dev);
    dev = MKDEV(drp_major, MINOR_BASE);

    /* Initialize cdev and registration handler table. */
    cdev_init(&drp_cdev, &s_mydevice_fops);
    drp_cdev.owner = THIS_MODULE;

    /* Registration cdev */
    cdev_err = cdev_add(&drp_cdev, dev, MINOR_NUM);
    if (cdev_err != 0) {
        pr_err("DRP Driver: cdev_add = %d\n", cdev_err);
        unregister_chrdev_region(dev, MINOR_NUM);
        return -ENOMEM;
    }

    /* Cleate class "/sys/class/drp/" */
    drp_class = class_create(THIS_MODULE, DRP_DRIVER_NAME);
    if (IS_ERR(drp_class)) {
        ptr_err = PTR_ERR(drp_class);
        pr_err("DRP Driver: class_create = %d\n", ptr_err);
        cdev_del(&drp_cdev);
        unregister_chrdev_region(dev, MINOR_NUM);
        return -ENOMEM;
    }

    /* Make "/sys/class/drp/drp*" */
    for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
        drp_device_array[minor - MINOR_BASE] =
        device_create(drp_class, NULL, MKDEV(drp_major, minor), NULL, DRP_DRIVER_NAME "%d", minor);
    }

    DRP_DEBUG_PRINT("end.\n");
    return 0;
}

static int drp_regist_device(struct platform_device *pdev)
{
    struct resource *res;
    struct resource reserved_res;
    struct device_node *np;
    int ret;
    struct drp_priv *priv;
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    int flag_ret;
#endif

    DRP_DEBUG_PRINT("start.\n");

    priv = devm_kzalloc(&pdev->dev, sizeof(struct drp_priv), GFP_KERNEL);
    if (!priv) {
        dev_err(&pdev->dev, "cannot allocate private data\n");
        return -ENOMEM;
    }

    /*Initialize DRP private structure*/
    platform_set_drvdata(pdev, priv);
    priv->pdev = pdev;
    priv->dev_name = dev_name(&pdev->dev);
    spin_lock_init(&priv->lock);
    sema_init(&priv->sem, DRP_DEV_NUM);
    priv->drp_irq_flag = DRP_IRQ_CHECK_DISABLE;
    drp_priv = priv;
    refcount_set(&priv->count, 1);

    if (of_device_is_compatible(pdev->dev.of_node, "renesas,rzv2h-drp"))
    {
        dev_info(&pdev->dev, "DRP Driver version : %s V2H\n", DRP_DRIVER_VERSION DRP_DRV_DEBUG_MODE DRP_DRV_DEBUG_WAIT_MODE);
    }
    else if (of_device_is_compatible(pdev->dev.of_node, "renesas,rzv2n-drp"))
    {
        dev_info(&pdev->dev, "DRP Driver version : %s V2N\n", DRP_DRIVER_VERSION DRP_DRV_DEBUG_MODE DRP_DRV_DEBUG_WAIT_MODE DRP_DRV_USE_SHARED_MEMORY_MODE);
    }
    else
    {
        /* Do Nothing */
    }

    /* Convert DRP base address from physical to virtual */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "cannot get resources (reg)\n");
        return -EINVAL;
    }
    priv->drp_base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!priv->drp_base) {
        dev_err(&pdev->dev, "cannot ioremap\n");
        return -EINVAL;
    }
    drp_base_addr[0] = priv->drp_base;
    drp_size = resource_size(res);
    dev_info(&pdev->dev, "DRP base address 0x%08llX, size 0x%08llX\n", res->start, drp_size);

#if defined(CONFIG_ARCH_R9A09G056)
    /* Convert AI-MAC base address from physical to virtual */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!res) {
        dev_err(&pdev->dev, "cannot get resources (reg)\n");
        return -EINVAL;
    }
    aimac_base_address[0] = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!aimac_base_address[0]) {
        dev_err(&pdev->dev, "cannot ioremap\n");
        return -EINVAL;
    }
    aimac_size = resource_size(res);
    dev_info(&pdev->dev, "AI-MAC base address 0x%016llX, size 0x%016llX\n", res->start, aimac_size);
#endif

    /* Registering an interrupt handler */
    drp_irqnum_nmlint = platform_get_irq(pdev, 0);
    ret = devm_request_irq(&pdev->dev, drp_irqnum_nmlint, irq_drp_nmlint, 0, "drp nmlint", priv);
    if (ret) {
        dev_err(&pdev->dev, "Failed to claim IRQ!\n");
        return ret;
    }

#if defined(CONFIG_ARCH_R9A09G056)
    /* V2N conditional compilation */
    drp_irqnum_errint = platform_get_irq(pdev, 1);
    ret = devm_request_irq(&pdev->dev, drp_irqnum_errint, irq_drp_errint, IRQF_SHARED | IRQF_TRIGGER_HIGH, "drp errint", priv);
    if (ret) {
        dev_err(&pdev->dev, "Failed to claim IRQ!\n");
        return ret;
    }
#else
    /* V2H conditional compilation */
    drp_irqnum_errint = platform_get_irq(pdev, 1);
    ret = devm_request_irq(&pdev->dev, drp_irqnum_errint, irq_drp_errint, 0, "drp errint", priv);
    if (ret) {
        dev_err(&pdev->dev, "Failed to claim IRQ!\n");
        return ret;
    }
#endif

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    drp_shared_exclusion = NULL;
    np = of_parse_phandle(pdev->dev.of_node, "memory-shared-for-drpai-ext-cont", 0);
    if (!np) 
    {
        dev_err(&pdev->dev, "No %s specified\n", "memory-shared-for-drpai-ext-cont");
        return -ENOMEM;
    }

    /* Convert memory region to a struct resource */
    flag_ret = of_address_to_resource(np, 0, &reserved_res);
    if (flag_ret) 
    {
        dev_err(&pdev->dev, "Failed to get shared memory used for DRP-AI exclustion control from device tree\n");
        return -ENOMEM;
    }

	drp_shared_base_addr = reserved_res.start;
    drp_shared_size = resource_size(&reserved_res);
    dev_info(&pdev->dev, "Shared memory region (DRP-AI exclustion cont.) start 0x%016llX, size 0x%08llX\n", drp_shared_base_addr, drp_shared_size);
    if(  64 > drp_shared_size )
    {
        dev_err(&pdev->dev, "Failed to initialize shared memory region used for DRP-AI exclustion control. It must be greater than or equal to 64byte.\n");
        return -EINVAL;
	}

    drp_shared_exclusion = phys_to_virt(drp_shared_base_addr);
    flag_ret = R_DRP_IsSharedMemoryInitialized(drp_shared_exclusion);
    if(-1 == flag_ret)
    {
        dev_info(&pdev->dev, "Shared memory region has already initialized by another driver.");
    }
    else if(0 != flag_ret)
    {
        dev_err(&pdev->dev, "Failed to initialize shared memory region used for DRP-AI exclustion control. 'drp_shared_exclusion' variable would be null.\n");
        return -ENODATA;
    }
    else
    {
        R_DRP_ClearFlagManagementArea(drp_shared_exclusion);
        R_DRP_InitializeSharedMemory(drp_shared_exclusion);
        dev_info(&pdev->dev, "Initilize shared memory region to be used in exclusive control of DRP-AI.");
    }

#endif

#ifdef DRP_CPG_CTL
    cpg_size = CPG_SIZE;
    cpg_base_address = ioremap(CPG_BASE_ADDRESS, cpg_size);
    if (cpg_base_address == 0)
    {
        pr_info("[%s: %d](pid: %d) failed to get cpg_base_address\n", __func__, __LINE__, current->pid);
        return -EINVAL;
    }
#endif
#ifndef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    /* Get reset controller info */
    priv->rstc = devm_reset_control_get(&pdev->dev, NULL);
    if (IS_ERR(priv->rstc))
    {
        dev_err(&pdev->dev, "Failed to get DRP CPG reset controller\n");
#if DRP_CPG_CTL
        iounmap(cpg_base_address);
#endif
        return PTR_ERR(priv->rstc);
    }
    else
    {
        DRP_DEBUG_PRINT("Get DRP CPG reset controller\n");      
    } 
#else
    flag_ret = R_DRP_LockResetContStatusBit(drp_shared_exclusion, &shared_mem_lock, PROCESS_CONTEXT);
    if(-1 == flag_ret)
    {
        dev_info(&pdev->dev, "Get CPG reset controller which is initilized by another driver.");
        priv->rstc = R_DRP_GetResetContPointerFromSharedMemory(drp_shared_exclusion);
    }
    else if(0 != flag_ret)
    {
        dev_info(&pdev->dev, "Failed to lock CPG reset controller bit. 'drp_shared_exclusion' variable is null or 'write control bit' is locked by another driver.");
#if DRP_CPG_CTL
        iounmap(cpg_base_address);
#endif
        return -EINVAL;
    }
    else
    {
        /* Get reset controller info */
        priv->rstc = devm_reset_control_get(&pdev->dev, NULL);
        if (IS_ERR(priv->rstc))
        {
            dev_err(&pdev->dev, "Failed to get DRP CPG reset controller\n");
            R_DRP_UnlockResetContStatusBit(drp_shared_exclusion, &shared_mem_lock, PROCESS_CONTEXT);

#if DRP_CPG_CTL
            iounmap(cpg_base_address);
#endif
            return PTR_ERR(priv->rstc);
        }

        dev_info(&pdev->dev, "Set CPG reset controller to shared memory.");
        R_DRP_SetResetContPointerToSharedMemory(drp_shared_exclusion, priv->rstc);
    }
#endif

    /* Status initialization */
    priv->drp_status.status = DRP_STATUS_INIT;

    {

        drp_region_codec_base_addr = 0;
        drp_region_codec_size = 0;
        drp_region_oca_base_addr = 0;
        drp_region_oca_size = 0;

        np = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
        if (!np) 
        {
            dev_err(&pdev->dev, "No %s specified\n", "memory-region");
        }
        else 
        {
           /* Convert memory region to a struct resource */
            ret = of_address_to_resource(np, 0, &reserved_res);
            if (ret) 
            {
                dev_err(&pdev->dev, "DRP(CODEC) memory was not assigned to the region\n");
            }
            else
            {
                drp_region_codec_base_addr = reserved_res.start;
                drp_region_codec_size = resource_size(&reserved_res);
                dev_info(&pdev->dev, "DRP(CODEC) memory region start 0x%016llX, size 0x%08llX\n", drp_region_codec_base_addr, drp_region_codec_size);
            }
        }

        np = of_parse_phandle(pdev->dev.of_node, "memory-oca-region", 0);
        if (!np) 
        {
            dev_err(&pdev->dev, "No %s specified\n", "memory-oca-region");
        }
        else 
        {
           /* Convert memory region to a struct resource */
            ret = of_address_to_resource(np, 0, &reserved_res);
            if (ret) 
            {
                dev_err(&pdev->dev, "DRP(OpenCVA) memory was not assigned to the region\n");
            }
            else
            {
                drp_region_oca_base_addr = reserved_res.start;
                drp_region_oca_size = resource_size(&reserved_res);
                dev_info(&pdev->dev, "DRP(OpenCVA) memory region start 0x%016llX, size 0x%08llX\n", drp_region_oca_base_addr, drp_region_oca_size);
            }
        }
        
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
        drp_os_exclusion = NULL;
        np = of_parse_phandle(pdev->dev.of_node, "memory-shared", 0);
        if (!np) 
        {
            dev_err(&pdev->dev, "No %s specified\n", "memory-shared");
        }
        else 
        {
           /* Convert memory region to a struct resource */
            ret = of_address_to_resource(np, 0, &reserved_res);
            if (ret) 
            {
                dev_err(&pdev->dev, "DRP(OS shared) memory was not assigned to the region\n");
            }
            else
            {
                drp_region_drp_multi_os_base_addr = reserved_res.start;
                drp_region_drp_multi_os_size = resource_size(&reserved_res);
                dev_info(&pdev->dev, "DRP(OS shared) memory region start 0x%016llX, size 0x%08llX\n", drp_region_drp_multi_os_base_addr, drp_region_drp_multi_os_size);
                if( drp_region_drp_multi_os_size >= 8 )
                {
                    drp_os_exclusion = phys_to_virt(drp_region_drp_multi_os_base_addr);
#ifdef CONFIG_DRP_INIT_SHARED_MEMORY
                    *drp_os_exclusion = 0;
#endif
                }
                else
                {
                    dev_err(&pdev->dev, "DRP(OS shared) memory is too small\n");
                }
            }
        }
#endif

    }

    DRP_DEBUG_PRINT("end.\n");
    return 0;
}

static void drp_unregist_driver(void)
{
    dev_t dev = MKDEV(drp_major, MINOR_BASE);
    int minor;

    DRP_DEBUG_PRINT("start.\n");

    /* Delete "/sys/class/mydevice/mydevice*". */
    for (minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
        device_destroy(drp_class, MKDEV(drp_major, minor));
    }

    /* Destroy "/sys/class/mydevice/". */
    class_destroy(drp_class);

    /* Delete cdev from kernel. */
    cdev_del(&drp_cdev);

    /* Unregistration */
    unregister_chrdev_region(dev, MINOR_NUM);
    DRP_DEBUG_PRINT("end.\n");
}

static void drp_unregist_device(void)
{
#if DRP_CPG_CTL
        iounmap(cpg_base_address);
#endif
}

static void drp_init_device(uint32_t ch)
{
    struct drp_priv *priv = drp_priv;
    unsigned long flags;
    DRP_DEBUG_PRINT("start.\n");


    spin_lock_irqsave(&priv->lock, flags);
    priv->drp_irq_flag = DRP_IRQ_CHECK_DISABLE;
    spin_unlock_irqrestore(&priv->lock, flags);
    
    (void)R_DRP_DRP_Open(drp_base_addr[0], 0, &priv->lock);
#if defined(CONFIG_ARCH_R9A09G056)
    (void)R_DRP_AIMAC_Open(aimac_base_address[0], 0);
#endif

    DRP_DEBUG_PRINT("end.\n");
}

static long drp_ioctl_assign(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    void *p_virt_address = 0;
    uint64_t addr, size;

    DRP_DEBUG_PRINT("start.\n");

    if(unlikely(down_trylock(&rw_sem)))
    {
        result = -ERESTART;
        goto end;
    }

    DRP_DEBUG_PRINT("status_rw1:%d\n", rw_status);

    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }

    /* Check status */
    if (DRP_STATUS_IDLE_RW != rw_status)
    {
        result = -EACCES;
        goto end;
    }

    /* Copy arguments from user space to kernel space */
    if (copy_from_user(&drp_data, (void __user *)arg, sizeof(drp_data_t)))
    {
        result = -EFAULT;
        goto end;
    }
    /* Check Argument */
    addr = (uint64_t)drp_data.address;
    size = (uint64_t)drp_data.size;
    if (0 != (addr & DRP_64BYTE_ALIGN))
    {
        result = -EINVAL;
        goto end;
    }
    /* Check size over */
    if (addr >= VAL_40BIT_OVER)
    {
        result = -EINVAL;
        goto end;
    }

    /* Data cache invalidate. DRP-AI W -> CPU R */
    addr = (uint64_t)drp_data.address;
    p_virt_address = phys_to_virt(addr);

    if (p_virt_address == 0)
    {
        result = -EFAULT;
        goto end;
    }
    // The cache is disabled for the initial access to read from memory.
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,1)
    dcache_inval_poc((unsigned long)p_virt_address, (unsigned long)p_virt_address + drp_data.size);
#else
    __inval_dcache_area(p_virt_address, drp_data.size);
#endif
    /* Initialization of read / write processing variables */
    rw_status   = DRP_STATUS_ASSIGN;
    write_count = 0;
    read_count  = 0;

    DRP_DEBUG_WAIT();
    goto end;
end:
    if(-ERESTART != result)
    {
        up(&rw_sem);
    }
    DRP_DEBUG_PRINT("status_rw2:%d\n", rw_status);
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static long drp_ioctl_start(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int result = 0;
    int i;
    int j;
    struct drp_priv *priv = drp_priv;
    unsigned long flags;
    struct drp_desc_info *desc_info = filp->private_data;
    int adrconv_cnt;
    char *param_addr;
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    int flag_drp_used = -1;
    int flag_drp_init = -1;
    int flag_drp_clkstop = -1;
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    int flag_drp_shared_used = -1;
    int flag_drp_shared_init = -1;
#endif

    DRP_DEBUG_PRINT("start.\n");

    if(unlikely(down_timeout(&priv->sem, MAX_SEM_TIMEOUT))) 
    {
        result = -ETIMEDOUT;
        goto end;
    }

    DRP_DEBUG_PRINT("status1:%d\n", priv->drp_status.status);

    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }
    if(NULL == desc_info->vaddr)
    {
        result = -EACCES;
        goto end;
    }

    /* Check status */
    spin_lock_irqsave(&priv->lock, flags);
    if (DRP_STATUS_RUN == priv->drp_status.status)
    {
        spin_unlock_irqrestore(&priv->lock, flags);
        result = -EBUSY;
        goto end;
    }
    spin_unlock_irqrestore(&priv->lock, flags);
    /* Copy arguments from user space to kernel space */
    if (copy_from_user(&proc[0], (void __user *)arg, sizeof(proc)))
    {
        result = -EFAULT;
        goto end;
    }

    spin_lock_irqsave(&priv->lock, flags);
    odif_intcnto.ch0 = 0;
    odif_intcnto.ch1 = 0;
    odif_intcnto.ch2 = 0;
    odif_intcnto.ch3 = 0;
    spin_unlock_irqrestore(&priv->lock, flags);

    /* Check Argument */
    for (i = 0; i < (desc_info->seq.num * 2); i++)
    {
        if (0 != (proc[i].address & DRP_64BYTE_ALIGN))
        {
            result = -EINVAL;
            goto end;
        }

        if( proc[i].address + proc[i].size >= VAL_40BIT_OVER )
        {
            result = -EINVAL;
            goto end;
        }

        if( proc[i].size > VAL_16M )
        {
            result = -EINVAL;
            goto end;
        }

        if( 0 == proc[i].size )
        {
            result = -EINVAL;
            goto end;
        }

        if( 0 == proc[i].address )
        {
            result = -EINVAL;
            goto end;
        }

    }

    /* DRP config address(32bit) and size settings to descriptor */
    /* (From here on, the code assumes seq.num == 1. It does not support seq.num >= 2.) */
    *(uint32_t*)(desc_info->vaddr + 4) =  (uint32_t)(proc[0].address & ADR_LOW_24BIT);
    *(uint32_t*)(desc_info->vaddr + 8) =  proc[0].size;

    /* DRP parameter address(32bit) and size settings to descriptor */
    *(uint32_t*)(desc_info->vaddr + 20) = (uint32_t)((proc[1].address & ADR_LOW_24BIT) | 0x02000000);
    *(uint32_t*)(desc_info->vaddr + 24) =  proc[1].size;

    /* LV disable */
    *(desc_info->vaddr + 51) = 0x0A;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,1)
    dcache_clean_inval_poc((unsigned long)desc_info->vaddr, (unsigned long)desc_info->vaddr + DRP_CMA_SIZE);
#else
    __flush_dcache_area(desc_info->vaddr, DRP_CMA_SIZE);
#endif

    /* Set AdrConv Table */
    /* drp_adrconv_tbl[Page] : 40-bit physical address associated for Page */

    /* Page 00 - 01:For DRP config address */
    adrconv_cnt = 0;
    drp_adrconv_tbl[adrconv_cnt++] = proc[0].address & ADR_CONV_MASK;
    drp_adrconv_tbl[adrconv_cnt++] = drp_adrconv_tbl[0] + VAL_16M;

    /* Page 02 - 03:For DRP parameter address */
    drp_adrconv_tbl[adrconv_cnt++] = proc[1].address & ADR_CONV_MASK;
    drp_adrconv_tbl[adrconv_cnt++] = drp_adrconv_tbl[2] + VAL_16M;

    /* Page 04 - 05:For Descriptor address */
    drp_adrconv_tbl[adrconv_cnt++] = desc_info->drp_desc_adr_40bit & ADR_CONV_MASK;
    drp_adrconv_tbl[adrconv_cnt++] = drp_adrconv_tbl[4] + VAL_16M;

    /* Page 06 -   :For DRP Input/Output data address in DRP parameters*/
    param_addr = (char *)phys_to_virt(proc[1].address);
    for( i = 0; i<desc_info->drp_iodata_num ; i++ )
    {
        uint64_t io_adr_start = desc_info->seq.iodata[i].address;
        uint64_t io_adr_end   = desc_info->seq.iodata[i].address + desc_info->seq.iodata[i].size - 1;
        uint32_t use_tbl_num  = (io_adr_end >> 24) - (io_adr_start >> 24) + 1;
        uint32_t set_addr;
        
        if( adrconv_cnt + use_tbl_num > ADRCONV_TBL_NUM )
        {
            result = -ENOMEM;
            goto end;
        }
        if( desc_info->seq.iodata[i].pos > proc[1].size - 4 )
        {
            result = -ENOSPC;
            goto end;
        }
        set_addr = (adrconv_cnt << 24) | (uint32_t)(io_adr_start & ADR_LOW_24BIT);
        param_addr[desc_info->seq.iodata[i].pos   ] = set_addr         & 0xFF;
        param_addr[desc_info->seq.iodata[i].pos +1] = (set_addr >> 8)  & 0xFF;
        param_addr[desc_info->seq.iodata[i].pos +2] = (set_addr >> 16) & 0xFF;
        param_addr[desc_info->seq.iodata[i].pos +3] = (set_addr >> 24) & 0xFF;
        for( j = 0; j < use_tbl_num; j++ )
        {
            drp_adrconv_tbl[adrconv_cnt++] = (io_adr_start & ADR_CONV_MASK) + j * VAL_16M;
        }
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,1)
    dcache_clean_inval_poc((unsigned long)param_addr, (unsigned long)param_addr + proc[1].size);
#else
    __flush_dcache_area(param_addr, proc[1].size);
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
        /* DRPFLAG_DRP_LOCK is set */
        flag_drp_shared_used = R_DRP_LockDrpaiContStatus(drp_shared_exclusion,
                                                         &shared_mem_lock,
                                                         PROCESS_CONTEXT,
                                                         DRPFLAG_DRP_LOCK);
        if(-1 == flag_drp_shared_used)
        {
            result = -EINPROGRESS;
            goto end;
        }
        if( 0 != flag_drp_shared_used)
        {
            result = -EADDRNOTAVAIL;
            goto end;
        }
#endif
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    flag_drp_used = drp_flag_test_and_set(DRPFLAG_DRP_USED);
    if( 0 != flag_drp_used)
    {
        /* DRPFLAG_DRP_USED is already set */
        result = -EALREADY;
        goto end;
    }

    flag_drp_init = drp_flag_test_and_set(DRPFLAG_DRP_INIT);
    if( 0 == flag_drp_init)
    {

        /* Initialize DRP procedure */
        drp_init_device(DRP_CH);

        /* Set first proc flag */
        spin_lock_irqsave(&priv->lock, flags);
        priv->drp_first_proc_after_init = 1;
        spin_unlock_irqrestore(&priv->lock, flags);

        drp_flag_clear(DRPFLAG_CLK_STOP);
    }

    flag_drp_clkstop = drp_flag_test(DRPFLAG_CLK_STOP);
    if( 0 != flag_drp_clkstop)
    {
        /* DRPFLAG_CLK_STOP is already set */
        result = -EOPNOTSUPP;
        goto end;
    }
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    flag_drp_shared_init = R_DRP_IsDrpaiHwStatusActive(drp_shared_exclusion);
    if(-1 == flag_drp_shared_init)
    {
        /* DRP-AI is stopped. Therefore, initialize the DRP-AI first */

        /* Initialize DRP procedure */
        drp_init_device(DRP_CH);

        /* Set first proc flag */
        spin_lock_irqsave(&priv->lock, flags);
        priv->drp_first_proc_after_init = 1;
        spin_unlock_irqrestore(&priv->lock, flags);

        R_DRP_RecordDrpaiActiveStatus(drp_shared_exclusion,
                                      &shared_mem_lock,
                                      PROCESS_CONTEXT);
    }
#endif

    /* Check whether DRP-AI is ready or not. */
    if(0 != R_DRP_DRP_IsActFieldOfDsccDctlZero(drp_base_addr[0]))
    {
        result = -EIO;
        goto end;
    }

    /* Init drp_status.err */
    spin_lock_irqsave(&priv->lock, flags);
    priv->drp_status.err = DRP_ERRINFO_SUCCESS;

    /* IDLE -> RUN */
    priv->drp_status.status = DRP_STATUS_RUN;
    priv->drp_irq_flag  = DRP_IRQ_CHECK_ENABLE;
    memcpy(&seq, &desc_info->seq, sizeof(drp_seq_t));
    spin_unlock_irqrestore(&priv->lock, flags);

    DRP_DEBUG_PRINT("status2:%d\n", priv->drp_status.status);

    /* Kick */
    {
        uint64_t last_descaddr;
        uint32_t noload_offset = 0;
        int32_t drp_ret;
        uint32_t first_proc;
        spin_lock_irqsave(&priv->lock, flags);
        first_proc = priv->drp_first_proc_after_init;
        spin_unlock_irqrestore(&priv->lock, flags);

        /* Check first proc */
        if( first_proc != 0 ){
            /* LOAD (Executed first time after initialization) */
        }
        else if( 0 != desc_info->drp_load_force )
        {
            /* LOAD (Forcefully) */
        }
        else
        {
            /* Get Descriptor Address (from DRP Register) */
            drp_ret = R_DRP_DRP_GetLastDescAddr(drp_base_addr[0], 4, &last_descaddr);   /* Page 04:Descriptor address page*/
            if( R_DRP_SUCCESS != drp_ret )
            {
                /* LOAD (Descriptor addres could no be obtained) */
            }
            else if( ( desc_info->phyaddr == last_descaddr ) || ( desc_info->phyaddr + LOAD_SKIP_OFFSET == last_descaddr ) )
            {
                /* Descriptor address matched the last one */
                if( last_drp_config_address == proc[0].address )
                {
                    /* DRP Config address matched the last one */
                    noload_offset = LOAD_SKIP_OFFSET; /* NOLOAD */
                }
            }
            else
            {
                /* LOAD (DRP Config address is not determined) */
            }
        }

#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
        irq_set_affinity( drp_irqnum_nmlint, cpu_online_mask );
        irq_set_affinity( drp_irqnum_errint, cpu_online_mask );
        (void)R_DRP_DRP_ResetDmaoffset(drp_base_addr[0], 0);
#endif
        (void)R_DRP_DRP_SetAdrConv(drp_base_addr[0], 0, &drp_adrconv_tbl[0]);
#if defined(CONFIG_ARCH_R9A09G056)
        (void)R_DRP_AIMAC_DisableAdrConv(aimac_base_address[0], 0, &drp_adrconv_tbl[0]);
#endif

        (void)R_DRP_SetFreq(drp_base_addr[0], 0, desc_info->drp_mindiv);

        /* Start DRP procedure */
        (void)R_DRP_DRP_Start(drp_base_addr[0], 0, ((desc_info->phyaddr & ADR_LOW_24BIT) | 0x04000000) + noload_offset);
#if defined(CONFIG_ARCH_R9A09G056)
        /* Start AIMAC with dummy descriptor */
        R_DRP_AIMAC_Start(aimac_base_address[0], 0, 
                          desc_info->phyaddr + (DRP_SINGLE_DESC_SIZE_BYTE - DRP_DESC_CMD_SIZE), 
                          desc_info->phyaddr + (DRP_SINGLE_DESC_SIZE_BYTE - DRP_DESC_CMD_SIZE));
#endif

        /* Clear first proc flag */
        spin_lock_irqsave(&priv->lock, flags);
        priv->drp_first_proc_after_init = 0;
        spin_unlock_irqrestore(&priv->lock, flags);

        /* Backup last config address */
        last_drp_config_address = proc[0].address;
    }
    DRP_DEBUG_WAIT();
    goto end;
end:
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    if( ( 0 == flag_drp_used) && (0 != result) )
    {
        drp_flag_clear(DRPFLAG_DRP_USED);
    }
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    if( ( 0 == flag_drp_shared_used) && (0 != result) )
    {
        R_DRP_UnlockDrpaiContStatus(drp_shared_exclusion,
                                    &shared_mem_lock,
                                    PROCESS_CONTEXT,
                                    DRPFLAG_DRP_LOCK);
    }
#endif
    if(-ETIMEDOUT != result)
    {
        up(&priv->sem);
    }
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static long drp_ioctl_reset(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    struct drp_priv *priv = drp_priv;
    unsigned long flags;
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    int flag_drp_used = -1;
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    int flag_drp_shared_used = -1;
    int flag_drp_shared_init = -1;
#endif

    DRP_DEBUG_PRINT("start.\n");

    if(unlikely(down_timeout(&priv->sem, MAX_SEM_TIMEOUT))) 
    {
        result = -ETIMEDOUT;
        goto end;
    }

    DRP_DEBUG_PRINT("status1:   %d\n", priv->drp_status.status);
    DRP_DEBUG_PRINT("status_rw1:%d\n", rw_status);

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    /* DRPFLAG_DRP_LOCK is set */
    flag_drp_shared_used = R_DRP_LockDrpaiContStatus(drp_shared_exclusion,
                                                         &shared_mem_lock,
                                                         PROCESS_CONTEXT,
                                                         DRPFLAG_DRP_LOCK);
    if(-1 == flag_drp_shared_used)
    {
        result = -EINPROGRESS;
        goto end;
    }
    if( 0 != flag_drp_shared_used)
    {
        result = -EADDRNOTAVAIL;
        goto end;
    }
#endif

#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    flag_drp_used = drp_flag_test_and_set(DRPFLAG_DRP_USED);
    if( 0 != flag_drp_used)
    {
        /* DRPFLAG_DRP_USED is already set */
        result = -EALREADY;
        goto end;
    }
    drp_flag_clear(DRPFLAG_DRP_INIT);
#endif

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    flag_drp_shared_init = R_DRP_IsDrpaiHwStatusActive(drp_shared_exclusion);
    if(0 == flag_drp_shared_init)
    {
        /* The DRP-AI is running. Therefore, stop the DRP-AI first. */
#endif

    /* Finalize DRP procedure */
    if(R_DRP_SUCCESS != drp_stop_device(DRP_CH))
    {
        result = -EIO;
        DRP_DEBUG_PRINT("Reset failed\n");
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
        drp_flag_clear(DRPFLAG_DRP_INIT);
#endif
        goto end;
    }

    /* Reset DRP (CPG Reset)*/
    if(R_DRP_SUCCESS != drp_cpg_reset(DRP_CH))
    {
        result = -EIO;
        goto end;
    }

#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
        /* Normally, the DRP-AI status flag in shared memory should be deactivated. 
           However, for the sake of efficiency, 
           this function does not perform this deactivation here
           instead, it remains active. */
    }
#endif

    /* Initialize DRP procedure */
    drp_init_device(DRP_CH);

    /* Set first proc flag */
    spin_lock_irqsave(&priv->lock, flags);
    priv->drp_first_proc_after_init = 1;
    spin_unlock_irqrestore(&priv->lock, flags);

#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    (void)drp_flag_test_and_set(DRPFLAG_DRP_INIT);
    drp_flag_clear(DRPFLAG_CLK_STOP);
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    R_DRP_RecordDrpaiActiveStatus(drp_shared_exclusion,
                                  &shared_mem_lock,
                                  PROCESS_CONTEXT);
#endif
    /* Update internal state */
    spin_lock_irqsave(&priv->lock, flags);
    priv->drp_status.err    = DRP_ERRINFO_RESET;
    priv->drp_status.status = DRP_STATUS_IDLE;

    /* Wake up the process */
    wake_up_interruptible( &read_q );
    spin_unlock_irqrestore(&priv->lock, flags);

    DRP_DEBUG_PRINT("status2:   %d\n", priv->drp_status.status);
    DRP_DEBUG_PRINT("status_rw2:%d\n", rw_status);

    result = 0;

    DRP_DEBUG_WAIT();
    goto end;
end:
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
    if( 0 == flag_drp_used)
    {
        drp_flag_clear(DRPFLAG_DRP_USED);
    }
#endif
#ifdef ENABLE_DRP_SUPPORT_SHARED_MEMORY
    if( 0 == flag_drp_shared_used )
    {
        R_DRP_UnlockDrpaiContStatus(drp_shared_exclusion,
                                    &shared_mem_lock,
                                    PROCESS_CONTEXT,
                                    DRPFLAG_DRP_LOCK);
    }
#endif
    if(-ETIMEDOUT != result)
    {
        up(&priv->sem);
    }
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static long drp_ioctl_get_status(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    struct drp_priv *priv = drp_priv;
    unsigned long flags;
    drp_status_t local_drp_status;

    DRP_DEBUG_PRINT("start.\n");

    if(unlikely(down_timeout(&priv->sem, MAX_SEM_TIMEOUT))) 
    {
        result = -ETIMEDOUT;
        goto end;
    }
    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }

    /* Check status */
    spin_lock_irqsave(&priv->lock, flags);
    if (DRP_STATUS_RUN == priv->drp_status.status)
    {
        spin_unlock_irqrestore(&priv->lock, flags);
        result = -EBUSY;
        goto end;
    }

    /* Copy arguments from kernel space to user space */
    local_drp_status = priv->drp_status;
    spin_unlock_irqrestore(&priv->lock, flags);
    if (copy_to_user((void __user *)arg, &local_drp_status, sizeof(drp_status_t)))
    {
        result = -EFAULT;
        goto end;
    }

    /* Check DRP H/W error */
    if (DRP_ERRINFO_DRP_ERR == local_drp_status.err)
    {
        result = -EIO;
        goto end;
    }

    DRP_DEBUG_WAIT();
    goto end;
end:
    if(-ETIMEDOUT != result)
    {
        up(&priv->sem);
    }
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static int8_t drp_cpg_reset(uint32_t ch)
{
    int8_t retval = R_DRP_SUCCESS;
    struct drp_priv *priv = drp_priv;
    int r_data;
    int32_t i = 0;
    bool is_stop = false;

    DRP_DEBUG_PRINT("start.\n");

    r_data = reset_control_status(priv->rstc);
    DRP_DEBUG_PRINT("CPG reset_control_status before %d \n", r_data);

    /* Access reset controller interface */
    reset_control_reset(priv->rstc);

    /* Check reset status */
    i = 0;
    while((RST_MAX_TIMEOUT > i) && (false == is_stop))
    {
        udelay(1);
        i++;
        r_data = reset_control_status(priv->rstc);
        DRP_DEBUG_PRINT("CPG reset_control_status %d \n", r_data);
        if(CPG_RESET_SUCCESS == r_data)
        {
            is_stop = true;
            break;
        }
    }

    i = 0;
    while((RST_MAX_TIMEOUT > i) && (false == is_stop))
    {
        usleep_range(100, 200);
        i++;
        r_data = reset_control_status(priv->rstc);
        DRP_DEBUG_PRINT("CPG reset_control_status %d \n", r_data);
        if(CPG_RESET_SUCCESS == r_data)
        {
            is_stop = true;
            break;
        }
    }

    if(true != is_stop)
    {
        DRP_DEBUG_PRINT("CPG Reset failed. Reset Control Status: %d\n", r_data);
        goto err_reset;
    }

    goto end;

err_reset:
    retval = R_DRP_ERR_RESET;
    goto end;
end:
    DRP_DEBUG_PRINT("end.\n");

    return retval;
}

static int8_t drp_stop_device(uint32_t ch)
{
    int8_t retval = R_DRP_SUCCESS;
    struct drp_priv *priv = drp_priv;

    DRP_DEBUG_PRINT("start.\n");

    /* Stop DRP */
    if(R_DRP_SUCCESS != R_DRP_DRP_Stop(drp_base_addr[0], ch, &priv->lock)) 
    {
        retval = R_DRP_ERR_RESET;
        goto end;
    }

#if defined(CONFIG_ARCH_R9A09G056)
    /* Stop AIMAC */
    if(R_DRP_SUCCESS != R_DRP_AIMAC_Reset(aimac_base_address[0], ch))
    {
        retval = R_DRP_ERR_RESET;
        goto end;
    }
#endif

    goto end;
end:
    DRP_DEBUG_PRINT("end.\n");
    return retval;
}

static long drp_ioctl_set_seq(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    int i;
    struct drp_priv *priv = drp_priv;
    struct drp_desc_info *desc_info = filp->private_data;

    DRP_DEBUG_PRINT("start.\n");

    if(unlikely(down_timeout(&priv->sem, MAX_SEM_TIMEOUT))) 
    {
        result = -ETIMEDOUT;
        goto end;
    }
    DRP_DEBUG_PRINT("status1:%d\n", priv->drp_status.status);
    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }

    /* Check status */
    if (DRP_STATUS_RUN == priv->drp_status.status)
    {
        result = -EBUSY;
        goto end;
    }

    /* Copy arguments from user space to kernel space */
    if (copy_from_user(&desc_info->seq, (void __user *)arg, sizeof(drp_seq_t)))
    {
        result = -EFAULT;
        goto end;
    }

    /* Check Argument DRP Single */
    if (DRP_MAX_PROCESS_CFG != desc_info->seq.num)
    {
        result = -EINVAL;
        goto end;
    }
    if (0 != (desc_info->seq.address & DRP_64BYTE_ALIGN))
    {
        result = -EINVAL;
        goto end;
    }
    if (desc_info->seq.address >= VAL_40BIT_OVER)
    {
        result = -EINVAL;
        goto end;
    }
    for (i = 0; i < desc_info->seq.num; i++)
    {
        if( (desc_info->seq.order[i] & 0xFF) != DRP_EXE_DRP_40BIT )
        {
            result = -EINVAL;
            goto end;
        }
        if( (desc_info->seq.order[i] & DRP_OPMASK_FORCE_LOAD) == 0 )
        {
            desc_info->drp_load_force = 0;
        }
        else
        {
            /* Force Load */
            desc_info->drp_load_force = 1;
        }
    }

    /*Initialize DRPcfg descriptor*/
    desc_info->phyaddr = (uint64_t)desc_info->seq.address;
    desc_info->vaddr = (char *)phys_to_virt(desc_info->phyaddr);
    if (!desc_info->vaddr)
    {
        result = -EFAULT;
        goto end;
    }
    DRP_DEBUG_PRINT("dmabuf:0x%016llX, dmaphys:0x%016llX\n", desc_info->vaddr, desc_info->phyaddr);
    /* Deploy drp_single_desc */   
    for (i = 0; i < DRP_SEQ_NUM; i++)
    {
        memcpy(desc_info->vaddr + (DRP_SGL_DRP_DESC_SIZE * i), &drp_single_desc_bin[0], sizeof(drp_single_desc_bin));
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,1)
    dcache_clean_inval_poc((unsigned long)desc_info->vaddr, (unsigned long)desc_info->vaddr + DRP_CMA_SIZE);
#else
    __flush_dcache_area(desc_info->vaddr, DRP_CMA_SIZE);
#endif
    desc_info->drp_desc_adr_40bit = desc_info->seq.address;
    desc_info->drp_iodata_num = desc_info->seq.iodata_num;
    if (desc_info->drp_iodata_num>MAX_IODATA_NUM)
    {
        result = -EINVAL;
        goto end;
    }
    if( desc_info->drp_iodata_num > 0 )
    {
        for (i = 0; i < desc_info->drp_iodata_num; i++)
        {
            if( desc_info->seq.iodata[i].address + desc_info->seq.iodata[i].size >= VAL_40BIT_OVER )
            {
                result = -EINVAL;
                goto end;
            }
            if( desc_info->seq.iodata[i].size == 0 )
            {
                result = -EINVAL;
                goto end;
            }
        }
    }

    DRP_DEBUG_PRINT("status2:%d\n", priv->drp_status.status);

    DRP_DEBUG_WAIT();
end:
    if(-ETIMEDOUT != result)
    {
        up(&priv->sem);
    }
    DRP_DEBUG_PRINT("end.\n");
    return result;
}
static long drp_ioctl_get_codec_area(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    drp_data_t local_drp_data;
    DRP_DEBUG_PRINT("start.\n");

    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }

    local_drp_data.address = drp_region_codec_base_addr;
    local_drp_data.size    = drp_region_codec_size;

    if (copy_to_user((void __user *)arg, &local_drp_data, sizeof(drp_data_t)))
    {
        result = -EFAULT;
        goto end;
    }

end:
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static long drp_ioctl_get_opencva_area(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    drp_data_t local_drp_data;
    DRP_DEBUG_PRINT("start.\n");

    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }

    local_drp_data.address = drp_region_oca_base_addr;
    local_drp_data.size    = drp_region_oca_size;

    if (copy_to_user((void __user *)arg, &local_drp_data, sizeof(drp_data_t)))
    {
        result = -EFAULT;
        goto end;
    }

end:
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static long drp_ioctl_set_drp_freq(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    unsigned int divfix;
    struct drp_priv *priv = drp_priv;
    unsigned long flags;
    struct drp_desc_info *desc_info = filp->private_data;

    DRP_DEBUG_PRINT("start.\n");
    if(unlikely(down_timeout(&priv->sem, MAX_SEM_TIMEOUT))) 
    {
        result = -ETIMEDOUT;
        goto end;
    }
    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }

    if (copy_from_user(&divfix, (void __user *)arg, sizeof(unsigned int)))
    {
        result = -EFAULT;
        goto end;
    }

    if( (divfix < DRP_DIVFIX_MIN) || (divfix > DRP_DIVFIX_MAX) )
    {
        result = -EINVAL;
        goto end;
    }

    /* Check status */
    /* Get the internal state */
    spin_lock_irqsave(&priv->lock, flags);
    if (DRP_STATUS_RUN == priv->drp_status.status)
    {
        spin_unlock_irqrestore(&priv->lock, flags);
        result = -EBUSY;
        goto end;
    }
    spin_unlock_irqrestore(&priv->lock, flags);
    
    desc_info->drp_mindiv = divfix;

end:
    if(-ETIMEDOUT != result)
    {
        up(&priv->sem);
    }
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static long drp_ioctl_read_drp_reg(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    drp_reg_t drp_reg;
    int32_t ret;

    DRP_DEBUG_PRINT("start.\n");

    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }

    if (copy_from_user(&drp_reg, (void __user *)arg, sizeof(drp_reg_t)))
    {
        result = -EFAULT;
        goto end;
    }

    if ((drp_size - sizeof(uint32_t)) < drp_reg.offset)
    {
        result = -EINVAL;
        goto end;
    }

    ret = R_DRP_DRP_RegRead(drp_base_addr[0], drp_reg.offset, &drp_reg.value);
    if (R_DRP_SUCCESS != ret)
    {
        result = -EFAULT;
        goto end;
    }

    if (copy_to_user((void __user *)arg, &drp_reg, sizeof(drp_reg_t)))
    {
        result = -EFAULT;
        goto end;
    }

    goto end;

end:
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static long drp_ioctl_write_drp_reg(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long result = 0;
    drp_reg_t drp_reg;

    DRP_DEBUG_PRINT("start.\n");

    /* Check NULL */
    if (0 == arg)
    {
        result = -EINVAL;
        goto end;
    }

    if (copy_from_user(&drp_reg, (void __user *)arg, sizeof(drp_reg_t)))
    {
        result = -EFAULT;
        goto end;
    }

    if ((drp_size - sizeof(uint32_t)) < drp_reg.offset)
    {
        result = -EINVAL;
        goto end;
    }

    R_DRP_DRP_RegWrite(drp_base_addr[0], drp_reg.offset, drp_reg.value);

    goto end;

end:
    DRP_DEBUG_PRINT("end.\n");
    return result;
}

static int drp_drp_cpg_init(void)
{
    int result;
    DRP_DEBUG_PRINT("start.\n");

    result =  R_DRP_SUCCESS;
#ifdef DRP_CPG_CTL
    initialize_cpg_drp(cpg_base_address);
#else
    // undefined
#endif
    
    DRP_DEBUG_PRINT("end.\n");

    return result;
}
#ifdef CONFIG_DRP_SUPPORT_MULTI_OS
static int drp_flag_test_and_set( unsigned int num )
{
    return lock_drp( drp_os_exclusion, num );
}

static void drp_flag_clear( unsigned int num )
{
    unlock_drp( drp_os_exclusion, num );
}

static int drp_flag_test( unsigned int num )
{
    int ret = lock_drp( drp_os_exclusion, num );
    if( 0 == ret)
    {
        unlock_drp( drp_os_exclusion, num );
    }
    return ret;
}
#endif

module_platform_driver(drp_platform_driver);
MODULE_DEVICE_TABLE(of, drp_match);
#if defined(CONFIG_ARCH_R9A09G056)
/* V2N conditional compilation */
MODULE_DESCRIPTION("RZ/V2N DRP driver");
#else
/* V2H conditional compilation */
MODULE_DESCRIPTION("RZ/V2H DRP driver");
#endif
MODULE_AUTHOR("Renesas Electronics Corporation");
MODULE_LICENSE("GPL v2");

