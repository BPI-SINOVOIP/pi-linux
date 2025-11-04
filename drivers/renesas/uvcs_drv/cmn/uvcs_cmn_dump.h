/*************************************************************************/ /*
 UVCS Driver common module

 Copyright (C) 2015 - 2018 Renesas Electronics Corporation

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

#ifndef UVCS_CMN_DUMP_H
#define UVCS_CMN_DUMP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/******************************************************************************/
/*                      INCLUDE FILES                                         */
/******************************************************************************/

/******************************************************************************/
/*                      DUMP MACROS                                           */
/******************************************************************************/
#ifndef UVCS_DEBUG
#define UVCS_DEBUG (1)
#endif

/******************************************************************************/
/*                      DUMP FILE IDENTIFIER                                  */
/******************************************************************************/
#if (UVCS_DEBUG != 0)
enum {
	UVCS_C_API_C
};

enum {
	/* interface */
	UVCS_CAPI_DEIN = 0xF0001,
	UVCS_CAPI_OPEN = 0xF0002,	UVCS_CAPI_CLOS = 0xF0003,

	UVCS_CAPI_REQ0 = 0xF0010,	UVCS_CAPI_REQ1 = 0xF0011,
	UVCS_CAPI_EXE0 = 0xF0014,
	UVCS_CAPI_RUNV = 0xF0080,	UVCS_CAPI_RUNC = 0xF0081,
	UVCS_CAPI_RUNF = 0xF0082,
	UVCS_CAPI_ENDV = 0xF0088,	UVCS_CAPI_ENDC = 0xF0089,
	UVCS_CAPI_ENDF = 0xF008A,
	UVCS_CAPI_ENDT = 0xF008F,

	UVCS_CAPI_PEMP = 0xF0090,
	UVCS_CAPI_INTR = 0xF00A0,

	UVCS_CAPI_BAV0 = 0xF00B0,	UVCS_CAPI_BAV1 = 0xF00B1,
	UVCS_CAPI_BAV2 = 0xF00B2,	UVCS_CAPI_BAV3 = 0xF00B3,
	UVCS_CAPI_BAC0 = 0xF00B8,	UVCS_CAPI_BAC1 = 0xF00B9,
	UVCS_CAPI_BAC2 = 0xF00BA,	UVCS_CAPI_BAC3 = 0xF00BB,

	/* sim */
	UVCS_CASSERT = 0xFF000
};
#endif

#if (UVCS_DEBUG == 1)
	#define UVCS_CP3(a)		(((a) & 0xFFu) << 24u)
	#define UVCS_CP2(a)		(((a) & 0xFFu) << 16u)
	#define UVCS_CP1(a)		(((a) & 0xFFu) <<  8u)
	#define UVCS_CP0(a)		 ((a) & 0xFFu)
	extern void uvcs_c_dump_init(struct UVCS_C_INR_INFO *inr);
	extern void uvcs_c_dump(struct UVCS_C_INR_INFO *inr, UVCS_U32 c, UVCS_U32 p0, UVCS_U32 p1);
	#define UVCS_CMN_DUMP_INIT(inr) uvcs_c_dump_init((inr))
	#define UVCS_CMN_DUMP(inr, c, p0, p1) uvcs_c_dump((inr), (UVCS_U32)(c), (p0), (p1))
	#define UVCS_CMN_ASSERT(inr, cond)	\
				do {\
					if (!(cond)) {\
						uvcs_c_dump((inr), (UVCS_U32)UVCS_CASSERT, __LINE__, (UVCS_U32)UVCS_C_DUMP_ID);\
					} \
				} while (0)

#elif (UVCS_DEBUG == 2)
	#define UVCS_CP3(a)		(((a) & 0xFFu) << 24u)
	#define UVCS_CP2(a)		(((a) & 0xFFu) << 16u)
	#define UVCS_CP1(a)		(((a) & 0xFFu) <<  8u)
	#define UVCS_CP0(a)		 ((a) & 0xFFu)
	extern void uvcs_dump(UVCS_U32 c, UVCS_U32 p0, UVCS_U32 p1);
	extern void uvcs_c_dump(struct UVCS_C_INR_INFO *inr, UVCS_U32 c, UVCS_U32 p0, UVCS_U32 p1);
	#define UVCS_CMN_DUMP_INIT(inr) uvcs_c_dump(NULL, 0, 0, 0)
	#define UVCS_CMN_DUMP(inr, c, p0, p1) uvcs_c_dump((inr), (UVCS_U32)(c), (p0), (p1))
	#define UVCS_CMN_ASSERT(inr, cond)	\
				do {\
					if (!(cond)) {\
						uvcs_c_dump((inr), (UVCS_U32)UVCS_CASSERT, \
								__LINE__, (UVCS_CP3(UVCS_C_DUMP_ID)|0xFFu));\
					} \
				} while (0)

#else
	/* UVCS_DEBUG == 0 */
	#define UVCS_CP3(a)
	#define UVCS_CP2(a)
	#define UVCS_CP1(a)
	#define UVCS_CP0(a)
	#define UVCS_CMN_DUMP_INIT(inr)
	#define UVCS_CMN_DUMP(inr, c, p0, p1)
	#define UVCS_CMN_ASSERT(inr, cond)

#endif
/******************************************************************************/
/*                      VARIABLES                                             */
/******************************************************************************/
#if (UVCS_DEBUG == 1)
struct UVCS_C_DUMP_DATA {
	UVCS_U32	c;
	UVCS_U32	p0;
	UVCS_U32	p1;
	UVCS_U32	w;
};

struct UVCS_C_DUMP_INFO {
	UVCS_U32				 n;
	struct UVCS_C_DUMP_DATA	 d;
};
#endif

/******************************************************************************/
/*                      FORWARD DECLARATION                                   */
/******************************************************************************/

/******************************************************************************/
/*                      FUNCTIONS                                             */
/******************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UVCS_CMN_DUMP_H */
