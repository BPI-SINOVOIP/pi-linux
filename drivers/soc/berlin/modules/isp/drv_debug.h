#ifndef _DRV_DEBUG_H_
#define _DRV_DEBUG_H_

#define ISPDHUB_MODULE_TAG    "[ispdhub] "
#define ISPCORE_MODULE_TAG    "[ispcore] "
#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define isp_debug(...)		dev_dbg(__VA_ARGS__)
#define ispdhub_debug(...)	dev_dbg(hDhubCtx->dev, ISPDHUB_MODULE_TAG __VA_ARGS__)
#else
#define isp_debug(...)
#define ispdhub_debug(...)
#endif

#define isp_trace(...)		dev_info(__VA_ARGS__)
#define isp_info(...)		dev_info(__VA_ARGS__)
#define isp_error(...)		dev_err(__VA_ARGS__)

#define ispdhub_info(...)	dev_info(hDhubCtx->dev, ISPDHUB_MODULE_TAG __VA_ARGS__)
#define ispdhub_trace(...)	dev_info(hDhubCtx->dev, ISPDHUB_MODULE_TAG __VA_ARGS__)
#define ispdhub_error(...)	dev_err(hDhubCtx->dev, ISPDHUB_MODULE_TAG __VA_ARGS__)

#define ispcore_debug(...)	dev_dbg(hCoreCtx->dev, ISPCORE_MODULE_TAG __VA_ARGS__)
#define ispcore_info(...)	dev_info(hCoreCtx->dev, ISPCORE_MODULE_TAG __VA_ARGS__)
#define ispcore_trace(...)	dev_info(hCoreCtx->dev, ISPCORE_MODULE_TAG __VA_ARGS__)
#define ispcore_error(...)	dev_err(hCoreCtx->dev, ISPCORE_MODULE_TAG __VA_ARGS__)
#endif //_DRV_DEBUG_H_
