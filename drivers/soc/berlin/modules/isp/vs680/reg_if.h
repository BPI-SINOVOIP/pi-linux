#ifndef __REG_IF_H__
#define __REG_IF_H__

#define IS_ISP_CORE_INTR(intr_num) \
	((intr_num == ispDhubSemMap_TSB_ispIntr) || \
		(intr_num == ispDhubSemMap_TSB_miIntr))
#define IS_ISP_BE_DWP_INTR(intr_num) \
	(intr_num == ispDhubSemMap_TSB_dwrp_intr)

#define TX_EN_VALUE		0x1A3

/* BASE OFFSETS */
#define ISP_GLOBAL_BASE		0x40000
#define ISP_CORE_BASE		0x60000

/* GLOABL SPACE */
#define ISP_CORE_TOP_CTRL_REG	0x8000
#define ISP_CORE_TOP_CTRL_INIT	0x0

#define ISP_INT_STATUS		0x194
#define MI_IRQ_BIT		0x1
#define ISP_IRQ_BIT		0x2
#define ISP_INTR_TYPE_CORE	0x1
#define ISP_INRPT_TYPE_OTHERS	0x0

/* ISP_CORE SPACE */
#define ISP_MI_INTR_STS_REG	0x16D0
#define ISP_MI_INTR_CLR_REG	0x16D8
#define MI_MP_EOF		0x1

#define ISP_ISP_INTR_STS_REG	0x5c4
#define ISP_ISP_INTR_CLR_REG	0x5c8

/* ISP BE-DWP SPACE*/
#define ISP_BE_DWP_REG_BASE         0xC0000
#define ISP_BE_DWP_LGDC_REG_BASE    0x7000
#define ISP_BE_DWP_CFTG             0x24
#define ISP_BE_CLEAR_DWP_TG         0x2

#endif /* __REG_IF_H__ */
