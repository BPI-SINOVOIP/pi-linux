#ifndef __DRV_ISP_PRV_H__
#define __DRV_ISP_PRV_H__

#define ISP_CORE_SUB_MODULE "isp_core"
#define MAX_INTERRUPTS_PER_MODULE	2

typedef enum isp_core_int_names {
	ISP_INTR_MI,
	ISP_INTR_ISP,
	ISP_INTR_MAX,
} isp_interrupt_types;

typedef struct _isp_interrupts_t {
	unsigned int intr_status;
	struct semaphore sem;
	spinlock_t lock;
	bool enabled;
} isp_interrupts;

typedef struct _ISP_CORE_CTX_ {
	int irq_num;
	int IsrRegistered;
	void *mod_base;
	isp_interrupts intrs[MAX_INTERRUPTS_PER_MODULE];
	struct device *dev;
} ISP_CORE_CTX;

#endif /* __DRV_ISP_PRV_H__ */
