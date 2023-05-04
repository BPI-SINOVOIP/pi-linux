#ifndef __UAPI_SYNA_BERLIN_CC__
#define __UAPI_SYNA_BERLIN_CC__

#include <linux/ioctl.h>
#include <linux/types.h>

#define BERLIN_CC_DYNAMIC_ID    (0x80000000)
#define BERLIN_CC_START_ID      (0x80000000)

#define CC_REG                  1
#define CC_FREE                 2
#define CC_INQUIRY              3
#define CC_UPDATE               4

struct berlin_cc_info {
	unsigned int  m_ServiceID;
	unsigned char m_SrvLevel;
	unsigned char m_SrvType;
	unsigned char m_Pad1;
	unsigned char m_Pad2;
	unsigned char m_Data[24];
};

struct berlin_cc_data {
	struct berlin_cc_info cc;
	unsigned int cmd;
};

#define BERLIN_IOC_CC           _IOWR('c', 1, struct berlin_cc_data)

#endif
