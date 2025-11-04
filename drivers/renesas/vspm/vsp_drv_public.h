/*************************************************************************/ /*
 * VSPM
 *
 * Copyright (C) 2015-2017 Renesas Electronics Corporation
 *
 * License        Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this
 * file under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * GPLv2:
 * If you wish to use this file under the terms of GPL, following terms are
 * effective.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */ /*************************************************************************/

#ifndef __VSP_DRV_PUBLIC_H__
#define __VSP_DRV_PUBLIC_H__

/* error code */
#define E_VSP_DEF_INH			(-100)
#define E_VSP_DEF_REG			(-101)
#define E_VSP_NO_MEM			(-102)
#define E_VSP_NO_INIT			(-103)
#define E_VSP_INVALID_STATE		(-105)
#define E_VSP_NO_CLK			(-107)
#define E_VSP_PARA_CH			(-150)

/* define channel bits */
#define VSP_RPF0_USE			(0x0001)
#define VSP_RPF1_USE			(0x0002)
#define VSP_RPF2_USE			(0x0004)
#define VSP_RPF3_USE			(0x0008)
#define VSP_RPF4_USE			(0x0010)

/* read outstanding of FCP */
#define FCP_MODE_64				(0x00000000)
#define FCP_MODE_16				(0x00000002)

/* public structure */
struct vsp_init_t {
	unsigned int ip_num;
};

struct vsp_open_t {
	struct platform_device *pdev;
};

struct vsp_status_t {
	unsigned int module_bits;
	unsigned int rpf_bits;
	unsigned int rpf_clut_bits;
	unsigned int wpf_rot_bits;
};

/* public functions */
long vsp_lib_init(struct vsp_init_t *param);
long vsp_lib_quit(void);
long vsp_lib_open(unsigned char ch, struct vsp_open_t *param);
long vsp_lib_close(unsigned char ch);
long vsp_lib_start(
	unsigned char ch,
	void *callback,
	struct vsp_start_t *param,
	void *userdata);
long vsp_lib_abort(unsigned char ch);
long vsp_lib_get_status(unsigned char ch, struct vsp_status_t *status);
long vsp_lib_suspend(unsigned char ch);
long vsp_lib_resume(unsigned char ch);

#endif
