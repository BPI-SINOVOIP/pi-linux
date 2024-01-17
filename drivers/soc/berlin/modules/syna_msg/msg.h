// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2019 Synaptics Incorporated */
#ifndef _MSG_H_
#define _MSG_H_

int syna_msg_callback_register(void* func);
int syna_msg_callback_unregister(void* func);

#endif //_MSG_H_
