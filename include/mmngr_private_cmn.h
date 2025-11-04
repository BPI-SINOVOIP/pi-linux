/*************************************************************************/ /*
 MMNGR

 Copyright (C) 2016 Renesas Electronics Corporation

 License        Dual MIT/GPLv2

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


 GPLv2:
 If you wish to use this file under the terms of GPL, following terms are
 effective.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/ /*************************************************************************/
#ifndef __MMNGR_PRIVATE_CMN_H__
#define __MMNGR_PRIVATE_CMN_H__

#define DEVFILE "/dev/rgnmm"

struct MM_PARAM {
	size_t size;
	unsigned long long	phy_addr;
	unsigned int	hard_addr;
	unsigned long	user_virt_addr;
	unsigned long	kernel_virt_addr;
	unsigned int flag;
};

struct MM_CACHE_PARAM {
	size_t offset;
	size_t len;
};

#define MM_IOC_MAGIC 'm'
#define MM_IOC_ALLOC	_IOWR(MM_IOC_MAGIC, 0, struct MM_PARAM)
#define MM_IOC_FREE	_IOWR(MM_IOC_MAGIC, 1, struct MM_PARAM)
#define MM_IOC_SET	_IOWR(MM_IOC_MAGIC, 2, struct MM_PARAM)
#define MM_IOC_GET	_IOWR(MM_IOC_MAGIC, 3, struct MM_PARAM)
#define MM_IOC_ALLOC_CO	_IOWR(MM_IOC_MAGIC, 4, struct MM_PARAM)
#define MM_IOC_FREE_CO	_IOWR(MM_IOC_MAGIC, 5, struct MM_PARAM)
#define MM_IOC_SHARE	_IOWR(MM_IOC_MAGIC, 6, struct MM_PARAM)
#define MM_IOC_FLUSH	_IOWR(MM_IOC_MAGIC, 7, struct MM_CACHE_PARAM)
#define MM_IOC_INVAL	_IOWR(MM_IOC_MAGIC, 8, struct MM_CACHE_PARAM)
/* virt to phys */
#define MM_IOC_VTOP     _IOWR(MM_IOC_MAGIC, 9, struct MM_PARAM)

#endif	/* __MMNGR_PRIVATE_CMN_H__ */
