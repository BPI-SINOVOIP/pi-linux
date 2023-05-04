/* SPDX-License-Identifier: GPL-2.0 */
/*
 * tzd trace points
 *
 * Copyright (C) 2021 Synaptics Incorporated
 *
 * Author: Jisheng Zhang <jszhang@kernel.org>
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM tzd

#if !defined(_TRACE_TZD_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_TZD_H

#include <linux/tracepoint.h>

TRACE_EVENT(tzd_smc_begin,
	TP_PROTO(u32 taskid, u32 cmd, u32 param0, u32 param1),
	TP_ARGS(taskid, cmd, param0, param1),

	TP_STRUCT__entry(
		__field(u32, taskid)
		__field(u32, cmd)
		__field(u32, param0)
		__field(u32, param1)
	),

	TP_fast_assign(
		__entry->taskid = taskid;
		__entry->cmd = cmd;
		__entry->param0 = param0;
		__entry->param1 = param1;
	),

	TP_printk("taskid=%d cmd=0x%x param0=0x%x param1=0x%x",
		  __entry->taskid, __entry->cmd,
		  __entry->param0, __entry->param1)
);

TRACE_EVENT(tzd_smc_end,
	TP_PROTO(u32 taskid, u32 cmd, u32 param0, u32 param1, u32 status),
	TP_ARGS(taskid, cmd, param0, param1, status),

	TP_STRUCT__entry(
		__field(u32, taskid)
		__field(u32, cmd)
		__field(u32, param0)
		__field(u32, param1)
		__field(u32, status)
	),

	TP_fast_assign(
		__entry->taskid = taskid;
		__entry->cmd = cmd;
		__entry->param0 = param0;
		__entry->param1 = param1;
		__entry->status = status;
	),

	TP_printk("taskid=%d cmd=0x%x param0=0x%x param1=0x%x status=0x%x",
		  __entry->taskid, __entry->cmd,
		  __entry->param0, __entry->param1, __entry->status)
);
#endif /* _TRACE_TZD_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE tzd_trace

/* This part must be outside protection */
#include <trace/define_trace.h>
