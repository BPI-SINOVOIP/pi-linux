/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_MSG_H_
#define _DRV_MSG_H_
#include <linux/version.h>

typedef struct {
	/*Message ID*/
	unsigned int m_MsgID;
	/*Message 1st Parameter*/
	unsigned int m_Param1;
	/*Message 2nd Parameter*/
	unsigned int m_Param2;
} MV_CC_MSG_t, *pMV_CC_MSG_t;

#ifndef E_SUC
#define E_SUC               ( 0x00000000 )
#define E_ERR               ( 0x80000000 )

// generic error code macro
#define E_GEN_SUC( code ) ( E_SUC | E_GENERIC_BASE | ( code & 0x0000FFFF ) )
#define E_GEN_ERR( code )       ( E_ERR | E_GENERIC_BASE | ( code & 0x0000FFFF ) )

#define S_OK                E_GEN_SUC( 0x0000 ) // Success
#define S_FALSE             E_GEN_SUC( 0x0001 ) // Success but return false status
#define E_FAIL              E_GEN_ERR( 0x4005 ) // Unspecified failure
#define E_OUTOFMEMORY       E_GEN_ERR( 0x700E ) // Failed to allocate necessary memory

// error code base list
#define ERRORCODE_BASE          (0)
#define E_GENERIC_BASE          ( 0x0000 << 16 )
#define E_SYSTEM_BASE           ( 0x0001 << 16 )
#define E_DEBUG_BASE            ( 0x0002 << 16 )
#endif

typedef struct amp_message_queue {
	u32 q_length;
	u32 rd_number;
	u32 wr_number;
	MV_CC_MSG_t *pMsg;

	 int(*Add) (struct amp_message_queue * pMsgQ, MV_CC_MSG_t * pMsg);
	 int(*ReadTry) (struct amp_message_queue * pMsgQ,
			    MV_CC_MSG_t * pMsg);
	 int(*ReadFin) (struct amp_message_queue * pMsgQ);
	 int(*Destroy) (struct amp_message_queue * pMsgQ);
} AMPMsgQ_t;

int AMPMsgQ_DequeueRead(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg);
int AMPMsgQ_Fullness(AMPMsgQ_t * pMsgQ);
int AMPMsgQ_Init(AMPMsgQ_t * pAMPMsgQ, u32 q_length);
int AMPMsgQ_Destroy(AMPMsgQ_t * pMsgQ);
void AMPMsgQ_Post(AMPMsgQ_t * pMsgQ, int id);
int AMPMsgQ_Add(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg);
int AMPMsgQ_ReadTry(AMPMsgQ_t * pMsgQ, MV_CC_MSG_t * pMsg);
int AMPMsgQ_ReadFinish(AMPMsgQ_t * pMsgQ);

#endif //_DRV_MSG_H_
