#ifndef aio_h
#define aio_h (){}
#include "ctypes.h"
#pragma pack(1)
#ifdef __cplusplus
  extern "C" {
#endif
#ifndef _DOCC_H_BITOPS_
#define _DOCC_H_BITOPS_ (){}
    #define _bSETMASK_(b)                                      ((b)<32 ? (1<<((b)&31)) : 0)
    #define _NSETMASK_(msb,lsb)                                (_bSETMASK_((msb)+1)-_bSETMASK_(lsb))
    #define _bCLRMASK_(b)                                      (~_bSETMASK_(b))
    #define _NCLRMASK_(msb,lsb)                                (~_NSETMASK_(msb,lsb))
    #define _BFGET_(r,msb,lsb)                                 (_NSETMASK_((msb)-(lsb),0)&((r)>>(lsb)))
    #define _BFSET_(r,msb,lsb,v)                               do{ (r)&=_NCLRMASK_(msb,lsb); (r)|=_NSETMASK_(msb,lsb)&((v)<<(lsb)); }while(0)
#endif
#ifndef h_SRAMPWR
#define h_SRAMPWR (){}
    #define     RA_SRAMPWR_ctrl                                0x0000
    #define       bSRAMPWR_ctrl_SD                             1
    #define        SRAMPWR_ctrl_SD_ON                                       0x0
    #define        SRAMPWR_ctrl_SD_SHUTDWN                                  0x1
    #define       bSRAMPWR_ctrl_DSLP                           1
    #define        SRAMPWR_ctrl_DSLP_ON                                     0x0
    #define        SRAMPWR_ctrl_DSLP_DEEPSLP                                0x1
    #define       bSRAMPWR_ctrl_SLP                            1
    #define        SRAMPWR_ctrl_SLP_ON                                      0x0
    #define        SRAMPWR_ctrl_SLP_SLEEP                                   0x1
    typedef struct SIE_SRAMPWR {
    #define   SET32SRAMPWR_ctrl_SD(r32,v)                      _BFSET_(r32, 0, 0,v)
    #define   SET16SRAMPWR_ctrl_SD(r16,v)                      _BFSET_(r16, 0, 0,v)
    #define   SET32SRAMPWR_ctrl_DSLP(r32,v)                    _BFSET_(r32, 1, 1,v)
    #define   SET16SRAMPWR_ctrl_DSLP(r16,v)                    _BFSET_(r16, 1, 1,v)
    #define   SET32SRAMPWR_ctrl_SLP(r32,v)                     _BFSET_(r32, 2, 2,v)
    #define   SET16SRAMPWR_ctrl_SLP(r16,v)                     _BFSET_(r16, 2, 2,v)
    #define     w32SRAMPWR_ctrl                                {\
            UNSG32 uctrl_SD                                    :  1;\
            UNSG32 uctrl_DSLP                                  :  1;\
            UNSG32 uctrl_SLP                                   :  1;\
            UNSG32 RSVDx0_b3                                   : 29;\
          }
    union { UNSG32 u32SRAMPWR_ctrl;
            struct w32SRAMPWR_ctrl;
          };
    } SIE_SRAMPWR;
    typedef union  T32SRAMPWR_ctrl
          { UNSG32 u32;
            struct w32SRAMPWR_ctrl;
                 } T32SRAMPWR_ctrl;
    typedef union  TSRAMPWR_ctrl
          { UNSG32 u32[1];
            struct {
            struct w32SRAMPWR_ctrl;
                   };
                 } TSRAMPWR_ctrl;
     SIGN32 SRAMPWR_drvrd(SIE_SRAMPWR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SRAMPWR_drvwr(SIE_SRAMPWR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SRAMPWR_reset(SIE_SRAMPWR *p);
     SIGN32 SRAMPWR_cmp  (SIE_SRAMPWR *p, SIE_SRAMPWR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SRAMPWR_check(p,pie,pfx,hLOG) SRAMPWR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SRAMPWR_print(p,    pfx,hLOG) SRAMPWR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SRAMRWTC
#define h_SRAMRWTC (){}
    #define     RA_SRAMRWTC_ctrl0                              0x0000
    #define       bSRAMRWTC_ctrl0_RF1P                         4
    #define       bSRAMRWTC_ctrl0_UHDRF1P                      4
    #define       bSRAMRWTC_ctrl0_RF2P                         8
    #define       bSRAMRWTC_ctrl0_UHDRF2P                      8
    #define       bSRAMRWTC_ctrl0_UHDRF2P_ULVT                 8
    #define     RA_SRAMRWTC_ctrl1                              0x0004
    #define       bSRAMRWTC_ctrl1_SHDMBSR1P                    4
    #define       bSRAMRWTC_ctrl1_SHDSBSR1P                    4
    #define       bSRAMRWTC_ctrl1_SHCMBSR1P_SSEG               4
    #define       bSRAMRWTC_ctrl1_SHCMBSR1P_USEG               4
    #define       bSRAMRWTC_ctrl1_SHCSBSR1P                    4
    #define       bSRAMRWTC_ctrl1_SHCSBSR1P_CUSTM              4
    #define       bSRAMRWTC_ctrl1_SPSRAM_WT0                   4
    #define       bSRAMRWTC_ctrl1_SPSRAM_WT1                   4
    #define     RA_SRAMRWTC_ctrl2                              0x0008
    #define       bSRAMRWTC_ctrl2_L1CACHE                      4
    #define       bSRAMRWTC_ctrl2_DPSR2P                       4
    #define       bSRAMRWTC_ctrl2_ROM                          8
    typedef struct SIE_SRAMRWTC {
    #define   SET32SRAMRWTC_ctrl0_RF1P(r32,v)                  _BFSET_(r32, 3, 0,v)
    #define   SET16SRAMRWTC_ctrl0_RF1P(r16,v)                  _BFSET_(r16, 3, 0,v)
    #define   SET32SRAMRWTC_ctrl0_UHDRF1P(r32,v)               _BFSET_(r32, 7, 4,v)
    #define   SET16SRAMRWTC_ctrl0_UHDRF1P(r16,v)               _BFSET_(r16, 7, 4,v)
    #define   SET32SRAMRWTC_ctrl0_RF2P(r32,v)                  _BFSET_(r32,15, 8,v)
    #define   SET16SRAMRWTC_ctrl0_RF2P(r16,v)                  _BFSET_(r16,15, 8,v)
    #define   SET32SRAMRWTC_ctrl0_UHDRF2P(r32,v)               _BFSET_(r32,23,16,v)
    #define   SET16SRAMRWTC_ctrl0_UHDRF2P(r16,v)               _BFSET_(r16, 7, 0,v)
    #define   SET32SRAMRWTC_ctrl0_UHDRF2P_ULVT(r32,v)          _BFSET_(r32,31,24,v)
    #define   SET16SRAMRWTC_ctrl0_UHDRF2P_ULVT(r16,v)          _BFSET_(r16,15, 8,v)
    #define     w32SRAMRWTC_ctrl0                              {\
            UNSG32 uctrl0_RF1P                                 :  4;\
            UNSG32 uctrl0_UHDRF1P                              :  4;\
            UNSG32 uctrl0_RF2P                                 :  8;\
            UNSG32 uctrl0_UHDRF2P                              :  8;\
            UNSG32 uctrl0_UHDRF2P_ULVT                         :  8;\
          }
    union { UNSG32 u32SRAMRWTC_ctrl0;
            struct w32SRAMRWTC_ctrl0;
          };
    #define   SET32SRAMRWTC_ctrl1_SHDMBSR1P(r32,v)             _BFSET_(r32, 3, 0,v)
    #define   SET16SRAMRWTC_ctrl1_SHDMBSR1P(r16,v)             _BFSET_(r16, 3, 0,v)
    #define   SET32SRAMRWTC_ctrl1_SHDSBSR1P(r32,v)             _BFSET_(r32, 7, 4,v)
    #define   SET16SRAMRWTC_ctrl1_SHDSBSR1P(r16,v)             _BFSET_(r16, 7, 4,v)
    #define   SET32SRAMRWTC_ctrl1_SHCMBSR1P_SSEG(r32,v)        _BFSET_(r32,11, 8,v)
    #define   SET16SRAMRWTC_ctrl1_SHCMBSR1P_SSEG(r16,v)        _BFSET_(r16,11, 8,v)
    #define   SET32SRAMRWTC_ctrl1_SHCMBSR1P_USEG(r32,v)        _BFSET_(r32,15,12,v)
    #define   SET16SRAMRWTC_ctrl1_SHCMBSR1P_USEG(r16,v)        _BFSET_(r16,15,12,v)
    #define   SET32SRAMRWTC_ctrl1_SHCSBSR1P(r32,v)             _BFSET_(r32,19,16,v)
    #define   SET16SRAMRWTC_ctrl1_SHCSBSR1P(r16,v)             _BFSET_(r16, 3, 0,v)
    #define   SET32SRAMRWTC_ctrl1_SHCSBSR1P_CUSTM(r32,v)       _BFSET_(r32,23,20,v)
    #define   SET16SRAMRWTC_ctrl1_SHCSBSR1P_CUSTM(r16,v)       _BFSET_(r16, 7, 4,v)
    #define   SET32SRAMRWTC_ctrl1_SPSRAM_WT0(r32,v)            _BFSET_(r32,27,24,v)
    #define   SET16SRAMRWTC_ctrl1_SPSRAM_WT0(r16,v)            _BFSET_(r16,11, 8,v)
    #define   SET32SRAMRWTC_ctrl1_SPSRAM_WT1(r32,v)            _BFSET_(r32,31,28,v)
    #define   SET16SRAMRWTC_ctrl1_SPSRAM_WT1(r16,v)            _BFSET_(r16,15,12,v)
    #define     w32SRAMRWTC_ctrl1                              {\
            UNSG32 uctrl1_SHDMBSR1P                            :  4;\
            UNSG32 uctrl1_SHDSBSR1P                            :  4;\
            UNSG32 uctrl1_SHCMBSR1P_SSEG                       :  4;\
            UNSG32 uctrl1_SHCMBSR1P_USEG                       :  4;\
            UNSG32 uctrl1_SHCSBSR1P                            :  4;\
            UNSG32 uctrl1_SHCSBSR1P_CUSTM                      :  4;\
            UNSG32 uctrl1_SPSRAM_WT0                           :  4;\
            UNSG32 uctrl1_SPSRAM_WT1                           :  4;\
          }
    union { UNSG32 u32SRAMRWTC_ctrl1;
            struct w32SRAMRWTC_ctrl1;
          };
    #define   SET32SRAMRWTC_ctrl2_L1CACHE(r32,v)               _BFSET_(r32, 3, 0,v)
    #define   SET16SRAMRWTC_ctrl2_L1CACHE(r16,v)               _BFSET_(r16, 3, 0,v)
    #define   SET32SRAMRWTC_ctrl2_DPSR2P(r32,v)                _BFSET_(r32, 7, 4,v)
    #define   SET16SRAMRWTC_ctrl2_DPSR2P(r16,v)                _BFSET_(r16, 7, 4,v)
    #define   SET32SRAMRWTC_ctrl2_ROM(r32,v)                   _BFSET_(r32,15, 8,v)
    #define   SET16SRAMRWTC_ctrl2_ROM(r16,v)                   _BFSET_(r16,15, 8,v)
    #define     w32SRAMRWTC_ctrl2                              {\
            UNSG32 uctrl2_L1CACHE                              :  4;\
            UNSG32 uctrl2_DPSR2P                               :  4;\
            UNSG32 uctrl2_ROM                                  :  8;\
            UNSG32 RSVDx8_b16                                  : 16;\
          }
    union { UNSG32 u32SRAMRWTC_ctrl2;
            struct w32SRAMRWTC_ctrl2;
          };
    } SIE_SRAMRWTC;
    typedef union  T32SRAMRWTC_ctrl0
          { UNSG32 u32;
            struct w32SRAMRWTC_ctrl0;
                 } T32SRAMRWTC_ctrl0;
    typedef union  T32SRAMRWTC_ctrl1
          { UNSG32 u32;
            struct w32SRAMRWTC_ctrl1;
                 } T32SRAMRWTC_ctrl1;
    typedef union  T32SRAMRWTC_ctrl2
          { UNSG32 u32;
            struct w32SRAMRWTC_ctrl2;
                 } T32SRAMRWTC_ctrl2;
    typedef union  TSRAMRWTC_ctrl0
          { UNSG32 u32[1];
            struct {
            struct w32SRAMRWTC_ctrl0;
                   };
                 } TSRAMRWTC_ctrl0;
    typedef union  TSRAMRWTC_ctrl1
          { UNSG32 u32[1];
            struct {
            struct w32SRAMRWTC_ctrl1;
                   };
                 } TSRAMRWTC_ctrl1;
    typedef union  TSRAMRWTC_ctrl2
          { UNSG32 u32[1];
            struct {
            struct w32SRAMRWTC_ctrl2;
                   };
                 } TSRAMRWTC_ctrl2;
     SIGN32 SRAMRWTC_drvrd(SIE_SRAMRWTC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SRAMRWTC_drvwr(SIE_SRAMRWTC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SRAMRWTC_reset(SIE_SRAMRWTC *p);
     SIGN32 SRAMRWTC_cmp  (SIE_SRAMRWTC *p, SIE_SRAMRWTC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SRAMRWTC_check(p,pie,pfx,hLOG) SRAMRWTC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SRAMRWTC_print(p,    pfx,hLOG) SRAMRWTC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_PRIAUD
#define h_PRIAUD (){}
    #define     RA_PRIAUD_CLKDIV                               0x0000
    #define       bPRIAUD_CLKDIV_SETTING                       4
    #define        PRIAUD_CLKDIV_SETTING_DIV1                               0x0
    #define        PRIAUD_CLKDIV_SETTING_DIV2                               0x1
    #define        PRIAUD_CLKDIV_SETTING_DIV4                               0x2
    #define        PRIAUD_CLKDIV_SETTING_DIV8                               0x3
    #define        PRIAUD_CLKDIV_SETTING_DIV16                              0x4
    #define        PRIAUD_CLKDIV_SETTING_DIV32                              0x5
    #define        PRIAUD_CLKDIV_SETTING_DIV64                              0x6
    #define        PRIAUD_CLKDIV_SETTING_DIV128                             0x7
    #define        PRIAUD_CLKDIV_SETTING_DIV256                             0x8
    #define        PRIAUD_CLKDIV_SETTING_DIV512                             0x9
    #define        PRIAUD_CLKDIV_SETTING_DIV1024                            0xA
    #define     RA_PRIAUD_CTRL                                 0x0004
    #define       bPRIAUD_CTRL_LEFTJFY                         1
    #define        PRIAUD_CTRL_LEFTJFY_LEFT                                 0x0
    #define        PRIAUD_CTRL_LEFTJFY_RIGHT                                0x1
    #define       bPRIAUD_CTRL_INVCLK                          1
    #define        PRIAUD_CTRL_INVCLK_NORMAL                                0x0
    #define        PRIAUD_CTRL_INVCLK_INVERTED                              0x1
    #define       bPRIAUD_CTRL_INVFS                           1
    #define        PRIAUD_CTRL_INVFS_NORMAL                                 0x0
    #define        PRIAUD_CTRL_INVFS_INVERTED                               0x1
    #define       bPRIAUD_CTRL_TLSB                            1
    #define        PRIAUD_CTRL_TLSB_MSB_FIRST                               0x0
    #define        PRIAUD_CTRL_TLSB_LSB_FIRST                               0x1
    #define       bPRIAUD_CTRL_TDM                             3
    #define        PRIAUD_CTRL_TDM_16DFM                                    0x0
    #define        PRIAUD_CTRL_TDM_18DFM                                    0x1
    #define        PRIAUD_CTRL_TDM_20DFM                                    0x2
    #define        PRIAUD_CTRL_TDM_24DFM                                    0x3
    #define        PRIAUD_CTRL_TDM_32DFM                                    0x4
    #define        PRIAUD_CTRL_TDM_8DFM                                     0x5
    #define       bPRIAUD_CTRL_TCF                             3
    #define        PRIAUD_CTRL_TCF_16CFM                                    0x0
    #define        PRIAUD_CTRL_TCF_24CFM                                    0x1
    #define        PRIAUD_CTRL_TCF_32CFM                                    0x2
    #define        PRIAUD_CTRL_TCF_8CFM                                     0x3
    #define       bPRIAUD_CTRL_TFM                             2
    #define        PRIAUD_CTRL_TFM_JUSTIFIED                                0x1
    #define        PRIAUD_CTRL_TFM_I2S                                      0x2
    #define       bPRIAUD_CTRL_TDMMODE                         1
    #define        PRIAUD_CTRL_TDMMODE_TDMMODE_OFF                          0x0
    #define        PRIAUD_CTRL_TDMMODE_TDMMODE_ON                           0x1
    #define       bPRIAUD_CTRL_TDMCHCNT                        3
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_2                             0x0
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_4                             0x1
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_6                             0x2
    #define        PRIAUD_CTRL_TDMCHCNT_CHCNT_8                             0x3
    #define       bPRIAUD_CTRL_TDMWSHIGH                       8
    #define       bPRIAUD_CTRL_TCF_MANUAL                      8
    #define     RA_PRIAUD_CTRL1                                0x0008
    #define       bPRIAUD_CTRL_TCF_MAN_MAR                     3
    #define       bPRIAUD_CTRL_TDM_MANUAL                      8
    #define       bPRIAUD_CTRL_PCM_MONO_CH                     1
    typedef struct SIE_PRIAUD {
    #define   SET32PRIAUD_CLKDIV_SETTING(r32,v)                _BFSET_(r32, 3, 0,v)
    #define   SET16PRIAUD_CLKDIV_SETTING(r16,v)                _BFSET_(r16, 3, 0,v)
    #define     w32PRIAUD_CLKDIV                               {\
            UNSG32 uCLKDIV_SETTING                             :  4;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32PRIAUD_CLKDIV;
            struct w32PRIAUD_CLKDIV;
          };
    #define   SET32PRIAUD_CTRL_LEFTJFY(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16PRIAUD_CTRL_LEFTJFY(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32PRIAUD_CTRL_INVCLK(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16PRIAUD_CTRL_INVCLK(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define   SET32PRIAUD_CTRL_INVFS(r32,v)                    _BFSET_(r32, 2, 2,v)
    #define   SET16PRIAUD_CTRL_INVFS(r16,v)                    _BFSET_(r16, 2, 2,v)
    #define   SET32PRIAUD_CTRL_TLSB(r32,v)                     _BFSET_(r32, 3, 3,v)
    #define   SET16PRIAUD_CTRL_TLSB(r16,v)                     _BFSET_(r16, 3, 3,v)
    #define   SET32PRIAUD_CTRL_TDM(r32,v)                      _BFSET_(r32, 6, 4,v)
    #define   SET16PRIAUD_CTRL_TDM(r16,v)                      _BFSET_(r16, 6, 4,v)
    #define   SET32PRIAUD_CTRL_TCF(r32,v)                      _BFSET_(r32, 9, 7,v)
    #define   SET16PRIAUD_CTRL_TCF(r16,v)                      _BFSET_(r16, 9, 7,v)
    #define   SET32PRIAUD_CTRL_TFM(r32,v)                      _BFSET_(r32,11,10,v)
    #define   SET16PRIAUD_CTRL_TFM(r16,v)                      _BFSET_(r16,11,10,v)
    #define   SET32PRIAUD_CTRL_TDMMODE(r32,v)                  _BFSET_(r32,12,12,v)
    #define   SET16PRIAUD_CTRL_TDMMODE(r16,v)                  _BFSET_(r16,12,12,v)
    #define   SET32PRIAUD_CTRL_TDMCHCNT(r32,v)                 _BFSET_(r32,15,13,v)
    #define   SET16PRIAUD_CTRL_TDMCHCNT(r16,v)                 _BFSET_(r16,15,13,v)
    #define   SET32PRIAUD_CTRL_TDMWSHIGH(r32,v)                _BFSET_(r32,23,16,v)
    #define   SET16PRIAUD_CTRL_TDMWSHIGH(r16,v)                _BFSET_(r16, 7, 0,v)
    #define   SET32PRIAUD_CTRL_TCF_MANUAL(r32,v)               _BFSET_(r32,31,24,v)
    #define   SET16PRIAUD_CTRL_TCF_MANUAL(r16,v)               _BFSET_(r16,15, 8,v)
    #define     w32PRIAUD_CTRL                                 {\
            UNSG32 uCTRL_LEFTJFY                               :  1;\
            UNSG32 uCTRL_INVCLK                                :  1;\
            UNSG32 uCTRL_INVFS                                 :  1;\
            UNSG32 uCTRL_TLSB                                  :  1;\
            UNSG32 uCTRL_TDM                                   :  3;\
            UNSG32 uCTRL_TCF                                   :  3;\
            UNSG32 uCTRL_TFM                                   :  2;\
            UNSG32 uCTRL_TDMMODE                               :  1;\
            UNSG32 uCTRL_TDMCHCNT                              :  3;\
            UNSG32 uCTRL_TDMWSHIGH                             :  8;\
            UNSG32 uCTRL_TCF_MANUAL                            :  8;\
          }
    union { UNSG32 u32PRIAUD_CTRL;
            struct w32PRIAUD_CTRL;
          };
    #define   SET32PRIAUD_CTRL_TCF_MAN_MAR(r32,v)              _BFSET_(r32, 2, 0,v)
    #define   SET16PRIAUD_CTRL_TCF_MAN_MAR(r16,v)              _BFSET_(r16, 2, 0,v)
    #define   SET32PRIAUD_CTRL_TDM_MANUAL(r32,v)               _BFSET_(r32,10, 3,v)
    #define   SET16PRIAUD_CTRL_TDM_MANUAL(r16,v)               _BFSET_(r16,10, 3,v)
    #define   SET32PRIAUD_CTRL_PCM_MONO_CH(r32,v)              _BFSET_(r32,11,11,v)
    #define   SET16PRIAUD_CTRL_PCM_MONO_CH(r16,v)              _BFSET_(r16,11,11,v)
    #define     w32PRIAUD_CTRL1                                {\
            UNSG32 uCTRL_TCF_MAN_MAR                           :  3;\
            UNSG32 uCTRL_TDM_MANUAL                            :  8;\
            UNSG32 uCTRL_PCM_MONO_CH                           :  1;\
            UNSG32 RSVDx8_b12                                  : 20;\
          }
    union { UNSG32 u32PRIAUD_CTRL1;
            struct w32PRIAUD_CTRL1;
          };
    } SIE_PRIAUD;
    typedef union  T32PRIAUD_CLKDIV
          { UNSG32 u32;
            struct w32PRIAUD_CLKDIV;
                 } T32PRIAUD_CLKDIV;
    typedef union  T32PRIAUD_CTRL
          { UNSG32 u32;
            struct w32PRIAUD_CTRL;
                 } T32PRIAUD_CTRL;
    typedef union  T32PRIAUD_CTRL1
          { UNSG32 u32;
            struct w32PRIAUD_CTRL1;
                 } T32PRIAUD_CTRL1;
    typedef union  TPRIAUD_CLKDIV
          { UNSG32 u32[1];
            struct {
            struct w32PRIAUD_CLKDIV;
                   };
                 } TPRIAUD_CLKDIV;
    typedef union  TPRIAUD_CTRL
          { UNSG32 u32[2];
            struct {
            struct w32PRIAUD_CTRL;
            struct w32PRIAUD_CTRL1;
                   };
                 } TPRIAUD_CTRL;
     SIGN32 PRIAUD_drvrd(SIE_PRIAUD *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 PRIAUD_drvwr(SIE_PRIAUD *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void PRIAUD_reset(SIE_PRIAUD *p);
     SIGN32 PRIAUD_cmp  (SIE_PRIAUD *p, SIE_PRIAUD *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define PRIAUD_check(p,pie,pfx,hLOG) PRIAUD_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define PRIAUD_print(p,    pfx,hLOG) PRIAUD_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AUDCH
#define h_AUDCH (){}
    #define     RA_AUDCH_CTRL                                  0x0000
    #define       bAUDCH_CTRL_ENABLE                           1
    #define        AUDCH_CTRL_ENABLE_DISABLE                                0x0
    #define        AUDCH_CTRL_ENABLE_ENABLE                                 0x1
    #define       bAUDCH_CTRL_MUTE                             1
    #define        AUDCH_CTRL_MUTE_MUTE_OFF                                 0x0
    #define        AUDCH_CTRL_MUTE_MUTE_ON                                  0x1
    #define       bAUDCH_CTRL_LRSWITCH                         1
    #define        AUDCH_CTRL_LRSWITCH_SWITCH_OFF                           0x0
    #define        AUDCH_CTRL_LRSWITCH_SWITCH_ON                            0x1
    #define       bAUDCH_CTRL_DEBUGEN                          1
    #define        AUDCH_CTRL_DEBUGEN_DISABLE                               0x0
    #define        AUDCH_CTRL_DEBUGEN_ENABLE                                0x1
    #define       bAUDCH_CTRL_FLUSH                            1
    #define        AUDCH_CTRL_FLUSH_ON                                      0x1
    #define        AUDCH_CTRL_FLUSH_OFF                                     0x0
    typedef struct SIE_AUDCH {
    #define   SET32AUDCH_CTRL_ENABLE(r32,v)                    _BFSET_(r32, 0, 0,v)
    #define   SET16AUDCH_CTRL_ENABLE(r16,v)                    _BFSET_(r16, 0, 0,v)
    #define   SET32AUDCH_CTRL_MUTE(r32,v)                      _BFSET_(r32, 1, 1,v)
    #define   SET16AUDCH_CTRL_MUTE(r16,v)                      _BFSET_(r16, 1, 1,v)
    #define   SET32AUDCH_CTRL_LRSWITCH(r32,v)                  _BFSET_(r32, 2, 2,v)
    #define   SET16AUDCH_CTRL_LRSWITCH(r16,v)                  _BFSET_(r16, 2, 2,v)
    #define   SET32AUDCH_CTRL_DEBUGEN(r32,v)                   _BFSET_(r32, 3, 3,v)
    #define   SET16AUDCH_CTRL_DEBUGEN(r16,v)                   _BFSET_(r16, 3, 3,v)
    #define   SET32AUDCH_CTRL_FLUSH(r32,v)                     _BFSET_(r32, 4, 4,v)
    #define   SET16AUDCH_CTRL_FLUSH(r16,v)                     _BFSET_(r16, 4, 4,v)
    #define     w32AUDCH_CTRL                                  {\
            UNSG32 uCTRL_ENABLE                                :  1;\
            UNSG32 uCTRL_MUTE                                  :  1;\
            UNSG32 uCTRL_LRSWITCH                              :  1;\
            UNSG32 uCTRL_DEBUGEN                               :  1;\
            UNSG32 uCTRL_FLUSH                                 :  1;\
            UNSG32 RSVDx0_b5                                   : 27;\
          }
    union { UNSG32 u32AUDCH_CTRL;
            struct w32AUDCH_CTRL;
          };
    } SIE_AUDCH;
    typedef union  T32AUDCH_CTRL
          { UNSG32 u32;
            struct w32AUDCH_CTRL;
                 } T32AUDCH_CTRL;
    typedef union  TAUDCH_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32AUDCH_CTRL;
                   };
                 } TAUDCH_CTRL;
     SIGN32 AUDCH_drvrd(SIE_AUDCH *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AUDCH_drvwr(SIE_AUDCH *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AUDCH_reset(SIE_AUDCH *p);
     SIGN32 AUDCH_cmp  (SIE_AUDCH *p, SIE_AUDCH *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AUDCH_check(p,pie,pfx,hLOG) AUDCH_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AUDCH_print(p,    pfx,hLOG) AUDCH_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DBG_TX
#define h_DBG_TX (){}
    #define     RA_DBG_TX_DEBUGHI                              0x0000
    #define       bDBG_TX_DEBUGHI_DATAHI                       32
    #define     RA_DBG_TX_DEBUGLO                              0x0004
    #define       bDBG_TX_DEBUGLO_DATALO                       32
    typedef struct SIE_DBG_TX {
    #define   SET32DBG_TX_DEBUGHI_DATAHI(r32,v)                _BFSET_(r32,31, 0,v)
    #define     w32DBG_TX_DEBUGHI                              {\
            UNSG32 uDEBUGHI_DATAHI                             : 32;\
          }
    union { UNSG32 u32DBG_TX_DEBUGHI;
            struct w32DBG_TX_DEBUGHI;
          };
    #define   SET32DBG_TX_DEBUGLO_DATALO(r32,v)                _BFSET_(r32,31, 0,v)
    #define     w32DBG_TX_DEBUGLO                              {\
            UNSG32 uDEBUGLO_DATALO                             : 32;\
          }
    union { UNSG32 u32DBG_TX_DEBUGLO;
            struct w32DBG_TX_DEBUGLO;
          };
    } SIE_DBG_TX;
    typedef union  T32DBG_TX_DEBUGHI
          { UNSG32 u32;
            struct w32DBG_TX_DEBUGHI;
                 } T32DBG_TX_DEBUGHI;
    typedef union  T32DBG_TX_DEBUGLO
          { UNSG32 u32;
            struct w32DBG_TX_DEBUGLO;
                 } T32DBG_TX_DEBUGLO;
    typedef union  TDBG_TX_DEBUGHI
          { UNSG32 u32[1];
            struct {
            struct w32DBG_TX_DEBUGHI;
                   };
                 } TDBG_TX_DEBUGHI;
    typedef union  TDBG_TX_DEBUGLO
          { UNSG32 u32[1];
            struct {
            struct w32DBG_TX_DEBUGLO;
                   };
                 } TDBG_TX_DEBUGLO;
     SIGN32 DBG_TX_drvrd(SIE_DBG_TX *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DBG_TX_drvwr(SIE_DBG_TX *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DBG_TX_reset(SIE_DBG_TX *p);
     SIGN32 DBG_TX_cmp  (SIE_DBG_TX *p, SIE_DBG_TX *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DBG_TX_check(p,pie,pfx,hLOG) DBG_TX_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DBG_TX_print(p,    pfx,hLOG) DBG_TX_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DBG_RX
#define h_DBG_RX (){}
    #define     RA_DBG_RX_DBG                                  0x0000
    #define       bDBG_RX_DBG_PORT_SEL                         2
    #define     RA_DBG_RX_DEBUGHI                              0x0004
    #define       bDBG_RX_DEBUGHI_DATAHI                       32
    #define     RA_DBG_RX_DEBUGLO                              0x0008
    #define       bDBG_RX_DEBUGLO_DATALO                       32
    typedef struct SIE_DBG_RX {
    #define   SET32DBG_RX_DBG_PORT_SEL(r32,v)                  _BFSET_(r32, 1, 0,v)
    #define   SET16DBG_RX_DBG_PORT_SEL(r16,v)                  _BFSET_(r16, 1, 0,v)
    #define     w32DBG_RX_DBG                                  {\
            UNSG32 uDBG_PORT_SEL                               :  2;\
            UNSG32 RSVDx0_b2                                   : 30;\
          }
    union { UNSG32 u32DBG_RX_DBG;
            struct w32DBG_RX_DBG;
          };
    #define   SET32DBG_RX_DEBUGHI_DATAHI(r32,v)                _BFSET_(r32,31, 0,v)
    #define     w32DBG_RX_DEBUGHI                              {\
            UNSG32 uDEBUGHI_DATAHI                             : 32;\
          }
    union { UNSG32 u32DBG_RX_DEBUGHI;
            struct w32DBG_RX_DEBUGHI;
          };
    #define   SET32DBG_RX_DEBUGLO_DATALO(r32,v)                _BFSET_(r32,31, 0,v)
    #define     w32DBG_RX_DEBUGLO                              {\
            UNSG32 uDEBUGLO_DATALO                             : 32;\
          }
    union { UNSG32 u32DBG_RX_DEBUGLO;
            struct w32DBG_RX_DEBUGLO;
          };
    } SIE_DBG_RX;
    typedef union  T32DBG_RX_DBG
          { UNSG32 u32;
            struct w32DBG_RX_DBG;
                 } T32DBG_RX_DBG;
    typedef union  T32DBG_RX_DEBUGHI
          { UNSG32 u32;
            struct w32DBG_RX_DEBUGHI;
                 } T32DBG_RX_DEBUGHI;
    typedef union  T32DBG_RX_DEBUGLO
          { UNSG32 u32;
            struct w32DBG_RX_DEBUGLO;
                 } T32DBG_RX_DEBUGLO;
    typedef union  TDBG_RX_DBG
          { UNSG32 u32[1];
            struct {
            struct w32DBG_RX_DBG;
                   };
                 } TDBG_RX_DBG;
    typedef union  TDBG_RX_DEBUGHI
          { UNSG32 u32[1];
            struct {
            struct w32DBG_RX_DEBUGHI;
                   };
                 } TDBG_RX_DEBUGHI;
    typedef union  TDBG_RX_DEBUGLO
          { UNSG32 u32[1];
            struct {
            struct w32DBG_RX_DEBUGLO;
                   };
                 } TDBG_RX_DEBUGLO;
     SIGN32 DBG_RX_drvrd(SIE_DBG_RX *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DBG_RX_drvwr(SIE_DBG_RX *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DBG_RX_reset(SIE_DBG_RX *p);
     SIGN32 DBG_RX_cmp  (SIE_DBG_RX *p, SIE_DBG_RX *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DBG_RX_check(p,pie,pfx,hLOG) DBG_RX_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DBG_RX_print(p,    pfx,hLOG) DBG_RX_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_ACLK
#define h_ACLK (){}
    #define     RA_ACLK_ACLK_CTRL                              0x0000
    #define       bACLK_ACLK_CTRL_clk_Enable                   1
    #define        ACLK_ACLK_CTRL_clk_Enable_DISABLE                        0x0
    #define        ACLK_ACLK_CTRL_clk_Enable_ENABLE                         0x1
    #define       bACLK_ACLK_CTRL_src_sel                      2
    #define        ACLK_ACLK_CTRL_src_sel_AVPLL_A3                          0x0
    #define        ACLK_ACLK_CTRL_src_sel_AVPLL_A4                          0x1
    #define        ACLK_ACLK_CTRL_src_sel_MCLK_EXT                          0x2
    #define       bACLK_ACLK_CTRL_clkSwitch                    1
    #define        ACLK_ACLK_CTRL_clkSwitch_SRC_CLK                         0x0
    #define        ACLK_ACLK_CTRL_clkSwitch_DIV_CLK                         0x1
    #define       bACLK_ACLK_CTRL_clkD3Switch                  1
    #define        ACLK_ACLK_CTRL_clkD3Switch_DIV_NOR                       0x0
    #define        ACLK_ACLK_CTRL_clkD3Switch_DIV_3                         0x1
    #define       bACLK_ACLK_CTRL_clkSel                       3
    #define        ACLK_ACLK_CTRL_clkSel_d2                                 0x1
    #define        ACLK_ACLK_CTRL_clkSel_d4                                 0x2
    #define        ACLK_ACLK_CTRL_clkSel_d6                                 0x3
    #define        ACLK_ACLK_CTRL_clkSel_d8                                 0x4
    #define        ACLK_ACLK_CTRL_clkSel_d12                                0x5
    #define       bACLK_ACLK_CTRL_sw_sync_rst                  1
    #define        ACLK_ACLK_CTRL_sw_sync_rst_ASSERT                        0x0
    #define        ACLK_ACLK_CTRL_sw_sync_rst_DEASSERT                      0x1
    typedef struct SIE_ACLK {
    #define   SET32ACLK_ACLK_CTRL_clk_Enable(r32,v)            _BFSET_(r32, 0, 0,v)
    #define   SET16ACLK_ACLK_CTRL_clk_Enable(r16,v)            _BFSET_(r16, 0, 0,v)
    #define   SET32ACLK_ACLK_CTRL_src_sel(r32,v)               _BFSET_(r32, 2, 1,v)
    #define   SET16ACLK_ACLK_CTRL_src_sel(r16,v)               _BFSET_(r16, 2, 1,v)
    #define   SET32ACLK_ACLK_CTRL_clkSwitch(r32,v)             _BFSET_(r32, 3, 3,v)
    #define   SET16ACLK_ACLK_CTRL_clkSwitch(r16,v)             _BFSET_(r16, 3, 3,v)
    #define   SET32ACLK_ACLK_CTRL_clkD3Switch(r32,v)           _BFSET_(r32, 4, 4,v)
    #define   SET16ACLK_ACLK_CTRL_clkD3Switch(r16,v)           _BFSET_(r16, 4, 4,v)
    #define   SET32ACLK_ACLK_CTRL_clkSel(r32,v)                _BFSET_(r32, 7, 5,v)
    #define   SET16ACLK_ACLK_CTRL_clkSel(r16,v)                _BFSET_(r16, 7, 5,v)
    #define   SET32ACLK_ACLK_CTRL_sw_sync_rst(r32,v)           _BFSET_(r32, 8, 8,v)
    #define   SET16ACLK_ACLK_CTRL_sw_sync_rst(r16,v)           _BFSET_(r16, 8, 8,v)
    #define     w32ACLK_ACLK_CTRL                              {\
            UNSG32 uACLK_CTRL_clk_Enable                       :  1;\
            UNSG32 uACLK_CTRL_src_sel                          :  2;\
            UNSG32 uACLK_CTRL_clkSwitch                        :  1;\
            UNSG32 uACLK_CTRL_clkD3Switch                      :  1;\
            UNSG32 uACLK_CTRL_clkSel                           :  3;\
            UNSG32 uACLK_CTRL_sw_sync_rst                      :  1;\
            UNSG32 RSVDx0_b9                                   : 23;\
          }
    union { UNSG32 u32ACLK_ACLK_CTRL;
            struct w32ACLK_ACLK_CTRL;
          };
    } SIE_ACLK;
    typedef union  T32ACLK_ACLK_CTRL
          { UNSG32 u32;
            struct w32ACLK_ACLK_CTRL;
                 } T32ACLK_ACLK_CTRL;
    typedef union  TACLK_ACLK_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32ACLK_ACLK_CTRL;
                   };
                 } TACLK_ACLK_CTRL;
     SIGN32 ACLK_drvrd(SIE_ACLK *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 ACLK_drvwr(SIE_ACLK *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void ACLK_reset(SIE_ACLK *p);
     SIGN32 ACLK_cmp  (SIE_ACLK *p, SIE_ACLK *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define ACLK_check(p,pie,pfx,hLOG) ACLK_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define ACLK_print(p,    pfx,hLOG) ACLK_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DMIC_CLK
#define h_DMIC_CLK (){}
    #define     RA_DMIC_CLK_DMIC_Reset                         0x0000
    #define       bDMIC_CLK_DMIC_Reset_Reset                   1
    #define     RA_DMIC_CLK_DMIC_Core_Clock_Config             0x0004
    #define       bDMIC_CLK_DMIC_Core_Clock_Config_Source_Sel  1
    #define       bDMIC_CLK_DMIC_Core_Clock_Config_Source_En   1
    #define       bDMIC_CLK_DMIC_Core_Clock_Config_Divider     10
    #define       bDMIC_CLK_DMIC_Core_Clock_Config_Enable_A    1
    #define       bDMIC_CLK_DMIC_Core_Clock_Config_Enable_B    1
    #define       bDMIC_CLK_DMIC_Core_Clock_Config_Enable_C    1
    #define       bDMIC_CLK_DMIC_Core_Clock_Config_Enable_D    1
    #define       bDMIC_CLK_DMIC_Core_Clock_Config_Enable_All  1
    #define     RA_DMIC_CLK_DMIC_EXTIFABC_Clock_Config         0x0008
    #define       bDMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerM 10
    #define       bDMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerN 5
    #define       bDMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerP 5
    #define     RA_DMIC_CLK_DMIC_EXTIFD_Clock_Config           0x000C
    #define       bDMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerM  10
    #define       bDMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerN  5
    #define       bDMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerP  5
    #define       bDMIC_CLK_DMIC_EXTIFD_Clock_Config_ABCClk_Select 1
    typedef struct SIE_DMIC_CLK {
    #define   SET32DMIC_CLK_DMIC_Reset_Reset(r32,v)            _BFSET_(r32, 0, 0,v)
    #define   SET16DMIC_CLK_DMIC_Reset_Reset(r16,v)            _BFSET_(r16, 0, 0,v)
    #define     w32DMIC_CLK_DMIC_Reset                         {\
            UNSG32 uDMIC_Reset_Reset                           :  1;\
            UNSG32 RSVDx0_b1                                   : 31;\
          }
    union { UNSG32 u32DMIC_CLK_DMIC_Reset;
            struct w32DMIC_CLK_DMIC_Reset;
          };
    #define   SET32DMIC_CLK_DMIC_Core_Clock_Config_Source_Sel(r32,v) _BFSET_(r32, 0, 0,v)
    #define   SET16DMIC_CLK_DMIC_Core_Clock_Config_Source_Sel(r16,v) _BFSET_(r16, 0, 0,v)
    #define   SET32DMIC_CLK_DMIC_Core_Clock_Config_Source_En(r32,v) _BFSET_(r32, 1, 1,v)
    #define   SET16DMIC_CLK_DMIC_Core_Clock_Config_Source_En(r16,v) _BFSET_(r16, 1, 1,v)
    #define   SET32DMIC_CLK_DMIC_Core_Clock_Config_Divider(r32,v) _BFSET_(r32,11, 2,v)
    #define   SET16DMIC_CLK_DMIC_Core_Clock_Config_Divider(r16,v) _BFSET_(r16,11, 2,v)
    #define   SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_A(r32,v) _BFSET_(r32,12,12,v)
    #define   SET16DMIC_CLK_DMIC_Core_Clock_Config_Enable_A(r16,v) _BFSET_(r16,12,12,v)
    #define   SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_B(r32,v) _BFSET_(r32,13,13,v)
    #define   SET16DMIC_CLK_DMIC_Core_Clock_Config_Enable_B(r16,v) _BFSET_(r16,13,13,v)
    #define   SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_C(r32,v) _BFSET_(r32,14,14,v)
    #define   SET16DMIC_CLK_DMIC_Core_Clock_Config_Enable_C(r16,v) _BFSET_(r16,14,14,v)
    #define   SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_D(r32,v) _BFSET_(r32,15,15,v)
    #define   SET16DMIC_CLK_DMIC_Core_Clock_Config_Enable_D(r16,v) _BFSET_(r16,15,15,v)
    #define   SET32DMIC_CLK_DMIC_Core_Clock_Config_Enable_All(r32,v) _BFSET_(r32,16,16,v)
    #define   SET16DMIC_CLK_DMIC_Core_Clock_Config_Enable_All(r16,v) _BFSET_(r16, 0, 0,v)
    #define     w32DMIC_CLK_DMIC_Core_Clock_Config             {\
            UNSG32 uDMIC_Core_Clock_Config_Source_Sel          :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Source_En           :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Divider             : 10;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_A            :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_B            :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_C            :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_D            :  1;\
            UNSG32 uDMIC_Core_Clock_Config_Enable_All          :  1;\
            UNSG32 RSVDx4_b17                                  : 15;\
          }
    union { UNSG32 u32DMIC_CLK_DMIC_Core_Clock_Config;
            struct w32DMIC_CLK_DMIC_Core_Clock_Config;
          };
    #define   SET32DMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerM(r32,v) _BFSET_(r32, 9, 0,v)
    #define   SET16DMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerM(r16,v) _BFSET_(r16, 9, 0,v)
    #define   SET32DMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerN(r32,v) _BFSET_(r32,14,10,v)
    #define   SET16DMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerN(r16,v) _BFSET_(r16,14,10,v)
    #define   SET32DMIC_CLK_DMIC_EXTIFABC_Clock_Config_DividerP(r32,v) _BFSET_(r32,19,15,v)
    #define     w32DMIC_CLK_DMIC_EXTIFABC_Clock_Config         {\
            UNSG32 uDMIC_EXTIFABC_Clock_Config_DividerM        : 10;\
            UNSG32 uDMIC_EXTIFABC_Clock_Config_DividerN        :  5;\
            UNSG32 uDMIC_EXTIFABC_Clock_Config_DividerP        :  5;\
            UNSG32 RSVDx8_b20                                  : 12;\
          }
    union { UNSG32 u32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
            struct w32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
          };
    #define   SET32DMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerM(r32,v) _BFSET_(r32, 9, 0,v)
    #define   SET16DMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerM(r16,v) _BFSET_(r16, 9, 0,v)
    #define   SET32DMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerN(r32,v) _BFSET_(r32,14,10,v)
    #define   SET16DMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerN(r16,v) _BFSET_(r16,14,10,v)
    #define   SET32DMIC_CLK_DMIC_EXTIFD_Clock_Config_DividerP(r32,v) _BFSET_(r32,19,15,v)
    #define   SET32DMIC_CLK_DMIC_EXTIFD_Clock_Config_ABCClk_Select(r32,v) _BFSET_(r32,20,20,v)
    #define   SET16DMIC_CLK_DMIC_EXTIFD_Clock_Config_ABCClk_Select(r16,v) _BFSET_(r16, 4, 4,v)
    #define     w32DMIC_CLK_DMIC_EXTIFD_Clock_Config           {\
            UNSG32 uDMIC_EXTIFD_Clock_Config_DividerM          : 10;\
            UNSG32 uDMIC_EXTIFD_Clock_Config_DividerN          :  5;\
            UNSG32 uDMIC_EXTIFD_Clock_Config_DividerP          :  5;\
            UNSG32 uDMIC_EXTIFD_Clock_Config_ABCClk_Select     :  1;\
            UNSG32 RSVDxC_b21                                  : 11;\
          }
    union { UNSG32 u32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
            struct w32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
          };
    } SIE_DMIC_CLK;
    typedef union  T32DMIC_CLK_DMIC_Reset
          { UNSG32 u32;
            struct w32DMIC_CLK_DMIC_Reset;
                 } T32DMIC_CLK_DMIC_Reset;
    typedef union  T32DMIC_CLK_DMIC_Core_Clock_Config
          { UNSG32 u32;
            struct w32DMIC_CLK_DMIC_Core_Clock_Config;
                 } T32DMIC_CLK_DMIC_Core_Clock_Config;
    typedef union  T32DMIC_CLK_DMIC_EXTIFABC_Clock_Config
          { UNSG32 u32;
            struct w32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
                 } T32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
    typedef union  T32DMIC_CLK_DMIC_EXTIFD_Clock_Config
          { UNSG32 u32;
            struct w32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
                 } T32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
    typedef union  TDMIC_CLK_DMIC_Reset
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CLK_DMIC_Reset;
                   };
                 } TDMIC_CLK_DMIC_Reset;
    typedef union  TDMIC_CLK_DMIC_Core_Clock_Config
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CLK_DMIC_Core_Clock_Config;
                   };
                 } TDMIC_CLK_DMIC_Core_Clock_Config;
    typedef union  TDMIC_CLK_DMIC_EXTIFABC_Clock_Config
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CLK_DMIC_EXTIFABC_Clock_Config;
                   };
                 } TDMIC_CLK_DMIC_EXTIFABC_Clock_Config;
    typedef union  TDMIC_CLK_DMIC_EXTIFD_Clock_Config
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CLK_DMIC_EXTIFD_Clock_Config;
                   };
                 } TDMIC_CLK_DMIC_EXTIFD_Clock_Config;
     SIGN32 DMIC_CLK_drvrd(SIE_DMIC_CLK *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DMIC_CLK_drvwr(SIE_DMIC_CLK *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DMIC_CLK_reset(SIE_DMIC_CLK *p);
     SIGN32 DMIC_CLK_cmp  (SIE_DMIC_CLK *p, SIE_DMIC_CLK *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DMIC_CLK_check(p,pie,pfx,hLOG) DMIC_CLK_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DMIC_CLK_print(p,    pfx,hLOG) DMIC_CLK_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SPDIF
#define h_SPDIF (){}
    #define     RA_SPDIF_CLKDIV                                0x0000
    #define       bSPDIF_CLKDIV_SETTING                        4
    #define        SPDIF_CLKDIV_SETTING_DIV1                                0x0
    #define        SPDIF_CLKDIV_SETTING_DIV2                                0x1
    #define        SPDIF_CLKDIV_SETTING_DIV4                                0x2
    #define        SPDIF_CLKDIV_SETTING_DIV8                                0x3
    #define        SPDIF_CLKDIV_SETTING_DIV16                               0x4
    #define        SPDIF_CLKDIV_SETTING_DIV32                               0x5
    #define        SPDIF_CLKDIV_SETTING_DIV64                               0x6
    #define        SPDIF_CLKDIV_SETTING_DIV128                              0x7
    #define        SPDIF_CLKDIV_SETTING_DIV256                              0x8
    #define        SPDIF_CLKDIV_SETTING_DIV512                              0x9
    #define        SPDIF_CLKDIV_SETTING_DIV1024                             0xA
    #define     RA_SPDIF_SPDIF                                 0x0004
    #define     RA_SPDIF_DBG                                   0x0008
    typedef struct SIE_SPDIF {
    #define   SET32SPDIF_CLKDIV_SETTING(r32,v)                 _BFSET_(r32, 3, 0,v)
    #define   SET16SPDIF_CLKDIV_SETTING(r16,v)                 _BFSET_(r16, 3, 0,v)
    #define     w32SPDIF_CLKDIV                                {\
            UNSG32 uCLKDIV_SETTING                             :  4;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32SPDIF_CLKDIV;
            struct w32SPDIF_CLKDIV;
          };
              SIE_AUDCH                                        ie_SPDIF;
              SIE_DBG_TX                                       ie_DBG;
    } SIE_SPDIF;
    typedef union  T32SPDIF_CLKDIV
          { UNSG32 u32;
            struct w32SPDIF_CLKDIV;
                 } T32SPDIF_CLKDIV;
    typedef union  TSPDIF_CLKDIV
          { UNSG32 u32[1];
            struct {
            struct w32SPDIF_CLKDIV;
                   };
                 } TSPDIF_CLKDIV;
     SIGN32 SPDIF_drvrd(SIE_SPDIF *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SPDIF_drvwr(SIE_SPDIF *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SPDIF_reset(SIE_SPDIF *p);
     SIGN32 SPDIF_cmp  (SIE_SPDIF *p, SIE_SPDIF *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SPDIF_check(p,pie,pfx,hLOG) SPDIF_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SPDIF_print(p,    pfx,hLOG) SPDIF_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_PRI
#define h_PRI (){}
    #define     RA_PRI_PRIAUD                                  0x0000
    #define     RA_PRI_TSD0_PRI                                0x000C
    #define     RA_PRI_TSD1_PRI                                0x0010
    #define     RA_PRI_TSD2_PRI                                0x0014
    #define     RA_PRI_TSD3_PRI                                0x0018
    #define     RA_PRI_DBG                                     0x001C
    #define     RA_PRI_PRIPORT                                 0x0024
    #define       bPRI_PRIPORT_ENABLE                          1
    #define        PRI_PRIPORT_ENABLE_DISABLE                               0x0
    #define        PRI_PRIPORT_ENABLE_ENABLE                                0x1
    typedef struct SIE_PRI {
              SIE_PRIAUD                                       ie_PRIAUD;
              SIE_AUDCH                                        ie_TSD0_PRI;
              SIE_AUDCH                                        ie_TSD1_PRI;
              SIE_AUDCH                                        ie_TSD2_PRI;
              SIE_AUDCH                                        ie_TSD3_PRI;
              SIE_DBG_TX                                       ie_DBG;
    #define   SET32PRI_PRIPORT_ENABLE(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16PRI_PRIPORT_ENABLE(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32PRI_PRIPORT                                 {\
            UNSG32 uPRIPORT_ENABLE                             :  1;\
            UNSG32 RSVDx24_b1                                  : 31;\
          }
    union { UNSG32 u32PRI_PRIPORT;
            struct w32PRI_PRIPORT;
          };
    } SIE_PRI;
    typedef union  T32PRI_PRIPORT
          { UNSG32 u32;
            struct w32PRI_PRIPORT;
                 } T32PRI_PRIPORT;
    typedef union  TPRI_PRIPORT
          { UNSG32 u32[1];
            struct {
            struct w32PRI_PRIPORT;
                   };
                 } TPRI_PRIPORT;
     SIGN32 PRI_drvrd(SIE_PRI *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 PRI_drvwr(SIE_PRI *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void PRI_reset(SIE_PRI *p);
     SIGN32 PRI_cmp  (SIE_PRI *p, SIE_PRI *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define PRI_check(p,pie,pfx,hLOG) PRI_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define PRI_print(p,    pfx,hLOG) PRI_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SEC
#define h_SEC (){}
    #define     RA_SEC_SECAUD                                  0x0000
    #define     RA_SEC_TSD0_SEC                                0x000C
    #define     RA_SEC_DBG                                     0x0010
    #define     RA_SEC_SECPORT                                 0x0018
    #define       bSEC_SECPORT_ENABLE                          1
    #define        SEC_SECPORT_ENABLE_DISABLE                               0x0
    #define        SEC_SECPORT_ENABLE_ENABLE                                0x1
    typedef struct SIE_SEC {
              SIE_PRIAUD                                       ie_SECAUD;
              SIE_AUDCH                                        ie_TSD0_SEC;
              SIE_DBG_TX                                       ie_DBG;
    #define   SET32SEC_SECPORT_ENABLE(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16SEC_SECPORT_ENABLE(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32SEC_SECPORT                                 {\
            UNSG32 uSECPORT_ENABLE                             :  1;\
            UNSG32 RSVDx18_b1                                  : 31;\
          }
    union { UNSG32 u32SEC_SECPORT;
            struct w32SEC_SECPORT;
          };
    } SIE_SEC;
    typedef union  T32SEC_SECPORT
          { UNSG32 u32;
            struct w32SEC_SECPORT;
                 } T32SEC_SECPORT;
    typedef union  TSEC_SECPORT
          { UNSG32 u32[1];
            struct {
            struct w32SEC_SECPORT;
                   };
                 } TSEC_SECPORT;
     SIGN32 SEC_drvrd(SIE_SEC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SEC_drvwr(SIE_SEC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SEC_reset(SIE_SEC *p);
     SIGN32 SEC_cmp  (SIE_SEC *p, SIE_SEC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SEC_check(p,pie,pfx,hLOG) SEC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SEC_print(p,    pfx,hLOG) SEC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_HDMI
#define h_HDMI (){}
    #define     RA_HDMI_HDAUD                                  0x0000
    #define     RA_HDMI_HDTSD                                  0x000C
    #define     RA_HDMI_DBG                                    0x0010
    #define     RA_HDMI_HDPORT                                 0x0018
    #define       bHDMI_HDPORT_TXSEL                           1
    typedef struct SIE_HDMI {
              SIE_PRIAUD                                       ie_HDAUD;
              SIE_AUDCH                                        ie_HDTSD;
              SIE_DBG_TX                                       ie_DBG;
    #define   SET32HDMI_HDPORT_TXSEL(r32,v)                    _BFSET_(r32, 0, 0,v)
    #define   SET16HDMI_HDPORT_TXSEL(r16,v)                    _BFSET_(r16, 0, 0,v)
    #define     w32HDMI_HDPORT                                 {\
            UNSG32 uHDPORT_TXSEL                               :  1;\
            UNSG32 RSVDx18_b1                                  : 31;\
          }
    union { UNSG32 u32HDMI_HDPORT;
            struct w32HDMI_HDPORT;
          };
    } SIE_HDMI;
    typedef union  T32HDMI_HDPORT
          { UNSG32 u32;
            struct w32HDMI_HDPORT;
                 } T32HDMI_HDPORT;
    typedef union  THDMI_HDPORT
          { UNSG32 u32[1];
            struct {
            struct w32HDMI_HDPORT;
                   };
                 } THDMI_HDPORT;
     SIGN32 HDMI_drvrd(SIE_HDMI *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 HDMI_drvwr(SIE_HDMI *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void HDMI_reset(SIE_HDMI *p);
     SIGN32 HDMI_cmp  (SIE_HDMI *p, SIE_HDMI *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define HDMI_check(p,pie,pfx,hLOG) HDMI_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define HDMI_print(p,    pfx,hLOG) HDMI_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MIC1
#define h_MIC1 (){}
    #define     RA_MIC1_MICCTRL                                0x0000
    #define     RA_MIC1_RSD0                                   0x000C
    #define     RA_MIC1_DBG                                    0x0010
    #define     RA_MIC1_RSD1                                   0x001C
    #define     RA_MIC1_MM_MODE                                0x0020
    #define       bMIC1_MM_MODE_RCV_MASTER                     1
    #define       bMIC1_MM_MODE_WS_HIGH_PRD                    8
    #define       bMIC1_MM_MODE_WS_TOTAL_PRD                   8
    #define       bMIC1_MM_MODE_WS_INV                         1
    #define     RA_MIC1_RXPORT                                 0x0024
    #define       bMIC1_RXPORT_ENABLE                          1
    #define        MIC1_RXPORT_ENABLE_DISABLE                               0x0
    #define        MIC1_RXPORT_ENABLE_ENABLE                                0x1
    #define     RA_MIC1_RXDATA                                 0x0028
    #define       bMIC1_RXDATA_HBR                             1
    #define       bMIC1_RXDATA_TDM_HR                          1
    #define     RA_MIC1_INTLMODE                               0x002C
    #define       bMIC1_INTLMODE_PORT0_EN                      1
    #define       bMIC1_INTLMODE_PORT1_EN                      1
    #define       bMIC1_INTLMODE_PORT2_EN                      1
    #define       bMIC1_INTLMODE_PORT3_EN                      1
    #define       bMIC1_INTLMODE_DUMMYDATA_EN                  1
    #define     RA_MIC1_HBRDMAP                                0x0030
    #define       bMIC1_HBRDMAP_PORT0                          2
    #define       bMIC1_HBRDMAP_PORT1                          2
    #define       bMIC1_HBRDMAP_PORT2                          2
    #define       bMIC1_HBRDMAP_PORT3                          2
    typedef struct SIE_MIC1 {
              SIE_PRIAUD                                       ie_MICCTRL;
              SIE_AUDCH                                        ie_RSD0;
              SIE_DBG_RX                                       ie_DBG;
              SIE_AUDCH                                        ie_RSD1;
    #define   SET32MIC1_MM_MODE_RCV_MASTER(r32,v)              _BFSET_(r32, 0, 0,v)
    #define   SET16MIC1_MM_MODE_RCV_MASTER(r16,v)              _BFSET_(r16, 0, 0,v)
    #define   SET32MIC1_MM_MODE_WS_HIGH_PRD(r32,v)             _BFSET_(r32, 8, 1,v)
    #define   SET16MIC1_MM_MODE_WS_HIGH_PRD(r16,v)             _BFSET_(r16, 8, 1,v)
    #define   SET32MIC1_MM_MODE_WS_TOTAL_PRD(r32,v)            _BFSET_(r32,16, 9,v)
    #define   SET32MIC1_MM_MODE_WS_INV(r32,v)                  _BFSET_(r32,17,17,v)
    #define   SET16MIC1_MM_MODE_WS_INV(r16,v)                  _BFSET_(r16, 1, 1,v)
    #define     w32MIC1_MM_MODE                                {\
            UNSG32 uMM_MODE_RCV_MASTER                         :  1;\
            UNSG32 uMM_MODE_WS_HIGH_PRD                        :  8;\
            UNSG32 uMM_MODE_WS_TOTAL_PRD                       :  8;\
            UNSG32 uMM_MODE_WS_INV                             :  1;\
            UNSG32 RSVDx20_b18                                 : 14;\
          }
    union { UNSG32 u32MIC1_MM_MODE;
            struct w32MIC1_MM_MODE;
          };
    #define   SET32MIC1_RXPORT_ENABLE(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16MIC1_RXPORT_ENABLE(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32MIC1_RXPORT                                 {\
            UNSG32 uRXPORT_ENABLE                              :  1;\
            UNSG32 RSVDx24_b1                                  : 31;\
          }
    union { UNSG32 u32MIC1_RXPORT;
            struct w32MIC1_RXPORT;
          };
    #define   SET32MIC1_RXDATA_HBR(r32,v)                      _BFSET_(r32, 0, 0,v)
    #define   SET16MIC1_RXDATA_HBR(r16,v)                      _BFSET_(r16, 0, 0,v)
    #define   SET32MIC1_RXDATA_TDM_HR(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16MIC1_RXDATA_TDM_HR(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define     w32MIC1_RXDATA                                 {\
            UNSG32 uRXDATA_HBR                                 :  1;\
            UNSG32 uRXDATA_TDM_HR                              :  1;\
            UNSG32 RSVDx28_b2                                  : 30;\
          }
    union { UNSG32 u32MIC1_RXDATA;
            struct w32MIC1_RXDATA;
          };
    #define   SET32MIC1_INTLMODE_PORT0_EN(r32,v)               _BFSET_(r32, 0, 0,v)
    #define   SET16MIC1_INTLMODE_PORT0_EN(r16,v)               _BFSET_(r16, 0, 0,v)
    #define   SET32MIC1_INTLMODE_PORT1_EN(r32,v)               _BFSET_(r32, 1, 1,v)
    #define   SET16MIC1_INTLMODE_PORT1_EN(r16,v)               _BFSET_(r16, 1, 1,v)
    #define   SET32MIC1_INTLMODE_PORT2_EN(r32,v)               _BFSET_(r32, 2, 2,v)
    #define   SET16MIC1_INTLMODE_PORT2_EN(r16,v)               _BFSET_(r16, 2, 2,v)
    #define   SET32MIC1_INTLMODE_PORT3_EN(r32,v)               _BFSET_(r32, 3, 3,v)
    #define   SET16MIC1_INTLMODE_PORT3_EN(r16,v)               _BFSET_(r16, 3, 3,v)
    #define   SET32MIC1_INTLMODE_DUMMYDATA_EN(r32,v)           _BFSET_(r32, 4, 4,v)
    #define   SET16MIC1_INTLMODE_DUMMYDATA_EN(r16,v)           _BFSET_(r16, 4, 4,v)
    #define     w32MIC1_INTLMODE                               {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx2C_b5                                  : 27;\
          }
    union { UNSG32 u32MIC1_INTLMODE;
            struct w32MIC1_INTLMODE;
          };
    #define   SET32MIC1_HBRDMAP_PORT0(r32,v)                   _BFSET_(r32, 1, 0,v)
    #define   SET16MIC1_HBRDMAP_PORT0(r16,v)                   _BFSET_(r16, 1, 0,v)
    #define   SET32MIC1_HBRDMAP_PORT1(r32,v)                   _BFSET_(r32, 3, 2,v)
    #define   SET16MIC1_HBRDMAP_PORT1(r16,v)                   _BFSET_(r16, 3, 2,v)
    #define   SET32MIC1_HBRDMAP_PORT2(r32,v)                   _BFSET_(r32, 5, 4,v)
    #define   SET16MIC1_HBRDMAP_PORT2(r16,v)                   _BFSET_(r16, 5, 4,v)
    #define   SET32MIC1_HBRDMAP_PORT3(r32,v)                   _BFSET_(r32, 7, 6,v)
    #define   SET16MIC1_HBRDMAP_PORT3(r16,v)                   _BFSET_(r16, 7, 6,v)
    #define     w32MIC1_HBRDMAP                                {\
            UNSG32 uHBRDMAP_PORT0                              :  2;\
            UNSG32 uHBRDMAP_PORT1                              :  2;\
            UNSG32 uHBRDMAP_PORT2                              :  2;\
            UNSG32 uHBRDMAP_PORT3                              :  2;\
            UNSG32 RSVDx30_b8                                  : 24;\
          }
    union { UNSG32 u32MIC1_HBRDMAP;
            struct w32MIC1_HBRDMAP;
          };
    } SIE_MIC1;
    typedef union  T32MIC1_MM_MODE
          { UNSG32 u32;
            struct w32MIC1_MM_MODE;
                 } T32MIC1_MM_MODE;
    typedef union  T32MIC1_RXPORT
          { UNSG32 u32;
            struct w32MIC1_RXPORT;
                 } T32MIC1_RXPORT;
    typedef union  T32MIC1_RXDATA
          { UNSG32 u32;
            struct w32MIC1_RXDATA;
                 } T32MIC1_RXDATA;
    typedef union  T32MIC1_INTLMODE
          { UNSG32 u32;
            struct w32MIC1_INTLMODE;
                 } T32MIC1_INTLMODE;
    typedef union  T32MIC1_HBRDMAP
          { UNSG32 u32;
            struct w32MIC1_HBRDMAP;
                 } T32MIC1_HBRDMAP;
    typedef union  TMIC1_MM_MODE
          { UNSG32 u32[1];
            struct {
            struct w32MIC1_MM_MODE;
                   };
                 } TMIC1_MM_MODE;
    typedef union  TMIC1_RXPORT
          { UNSG32 u32[1];
            struct {
            struct w32MIC1_RXPORT;
                   };
                 } TMIC1_RXPORT;
    typedef union  TMIC1_RXDATA
          { UNSG32 u32[1];
            struct {
            struct w32MIC1_RXDATA;
                   };
                 } TMIC1_RXDATA;
    typedef union  TMIC1_INTLMODE
          { UNSG32 u32[1];
            struct {
            struct w32MIC1_INTLMODE;
                   };
                 } TMIC1_INTLMODE;
    typedef union  TMIC1_HBRDMAP
          { UNSG32 u32[1];
            struct {
            struct w32MIC1_HBRDMAP;
                   };
                 } TMIC1_HBRDMAP;
     SIGN32 MIC1_drvrd(SIE_MIC1 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 MIC1_drvwr(SIE_MIC1 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void MIC1_reset(SIE_MIC1 *p);
     SIGN32 MIC1_cmp  (SIE_MIC1 *p, SIE_MIC1 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define MIC1_check(p,pie,pfx,hLOG) MIC1_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define MIC1_print(p,    pfx,hLOG) MIC1_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MIC2
#define h_MIC2 (){}
    #define     RA_MIC2_MICCTRL                                0x0000
    #define     RA_MIC2_RSD0                                   0x000C
    #define     RA_MIC2_DBG                                    0x0010
    #define     RA_MIC2_RXPORT                                 0x001C
    #define       bMIC2_RXPORT_ENABLE                          1
    #define        MIC2_RXPORT_ENABLE_DISABLE                               0x0
    #define        MIC2_RXPORT_ENABLE_ENABLE                                0x1
    #define     RA_MIC2_RXDATA                                 0x0020
    #define       bMIC2_RXDATA_HBR                             1
    #define       bMIC2_RXDATA_TDM_HR                          1
    #define     RA_MIC2_INTLMODE                               0x0024
    #define       bMIC2_INTLMODE_PORT0_EN                      1
    #define       bMIC2_INTLMODE_PORT1_EN                      1
    #define       bMIC2_INTLMODE_PORT2_EN                      1
    #define       bMIC2_INTLMODE_PORT3_EN                      1
    #define       bMIC2_INTLMODE_DUMMYDATA_EN                  1
    #define     RA_MIC2_HBRDMAP                                0x0028
    #define       bMIC2_HBRDMAP_PORT0                          2
    #define       bMIC2_HBRDMAP_PORT1                          2
    #define       bMIC2_HBRDMAP_PORT2                          2
    #define       bMIC2_HBRDMAP_PORT3                          2
    typedef struct SIE_MIC2 {
              SIE_PRIAUD                                       ie_MICCTRL;
              SIE_AUDCH                                        ie_RSD0;
              SIE_DBG_RX                                       ie_DBG;
    #define   SET32MIC2_RXPORT_ENABLE(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16MIC2_RXPORT_ENABLE(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32MIC2_RXPORT                                 {\
            UNSG32 uRXPORT_ENABLE                              :  1;\
            UNSG32 RSVDx1C_b1                                  : 31;\
          }
    union { UNSG32 u32MIC2_RXPORT;
            struct w32MIC2_RXPORT;
          };
    #define   SET32MIC2_RXDATA_HBR(r32,v)                      _BFSET_(r32, 0, 0,v)
    #define   SET16MIC2_RXDATA_HBR(r16,v)                      _BFSET_(r16, 0, 0,v)
    #define   SET32MIC2_RXDATA_TDM_HR(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16MIC2_RXDATA_TDM_HR(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define     w32MIC2_RXDATA                                 {\
            UNSG32 uRXDATA_HBR                                 :  1;\
            UNSG32 uRXDATA_TDM_HR                              :  1;\
            UNSG32 RSVDx20_b2                                  : 30;\
          }
    union { UNSG32 u32MIC2_RXDATA;
            struct w32MIC2_RXDATA;
          };
    #define   SET32MIC2_INTLMODE_PORT0_EN(r32,v)               _BFSET_(r32, 0, 0,v)
    #define   SET16MIC2_INTLMODE_PORT0_EN(r16,v)               _BFSET_(r16, 0, 0,v)
    #define   SET32MIC2_INTLMODE_PORT1_EN(r32,v)               _BFSET_(r32, 1, 1,v)
    #define   SET16MIC2_INTLMODE_PORT1_EN(r16,v)               _BFSET_(r16, 1, 1,v)
    #define   SET32MIC2_INTLMODE_PORT2_EN(r32,v)               _BFSET_(r32, 2, 2,v)
    #define   SET16MIC2_INTLMODE_PORT2_EN(r16,v)               _BFSET_(r16, 2, 2,v)
    #define   SET32MIC2_INTLMODE_PORT3_EN(r32,v)               _BFSET_(r32, 3, 3,v)
    #define   SET16MIC2_INTLMODE_PORT3_EN(r16,v)               _BFSET_(r16, 3, 3,v)
    #define   SET32MIC2_INTLMODE_DUMMYDATA_EN(r32,v)           _BFSET_(r32, 4, 4,v)
    #define   SET16MIC2_INTLMODE_DUMMYDATA_EN(r16,v)           _BFSET_(r16, 4, 4,v)
    #define     w32MIC2_INTLMODE                               {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx24_b5                                  : 27;\
          }
    union { UNSG32 u32MIC2_INTLMODE;
            struct w32MIC2_INTLMODE;
          };
    #define   SET32MIC2_HBRDMAP_PORT0(r32,v)                   _BFSET_(r32, 1, 0,v)
    #define   SET16MIC2_HBRDMAP_PORT0(r16,v)                   _BFSET_(r16, 1, 0,v)
    #define   SET32MIC2_HBRDMAP_PORT1(r32,v)                   _BFSET_(r32, 3, 2,v)
    #define   SET16MIC2_HBRDMAP_PORT1(r16,v)                   _BFSET_(r16, 3, 2,v)
    #define   SET32MIC2_HBRDMAP_PORT2(r32,v)                   _BFSET_(r32, 5, 4,v)
    #define   SET16MIC2_HBRDMAP_PORT2(r16,v)                   _BFSET_(r16, 5, 4,v)
    #define   SET32MIC2_HBRDMAP_PORT3(r32,v)                   _BFSET_(r32, 7, 6,v)
    #define   SET16MIC2_HBRDMAP_PORT3(r16,v)                   _BFSET_(r16, 7, 6,v)
    #define     w32MIC2_HBRDMAP                                {\
            UNSG32 uHBRDMAP_PORT0                              :  2;\
            UNSG32 uHBRDMAP_PORT1                              :  2;\
            UNSG32 uHBRDMAP_PORT2                              :  2;\
            UNSG32 uHBRDMAP_PORT3                              :  2;\
            UNSG32 RSVDx28_b8                                  : 24;\
          }
    union { UNSG32 u32MIC2_HBRDMAP;
            struct w32MIC2_HBRDMAP;
          };
    } SIE_MIC2;
    typedef union  T32MIC2_RXPORT
          { UNSG32 u32;
            struct w32MIC2_RXPORT;
                 } T32MIC2_RXPORT;
    typedef union  T32MIC2_RXDATA
          { UNSG32 u32;
            struct w32MIC2_RXDATA;
                 } T32MIC2_RXDATA;
    typedef union  T32MIC2_INTLMODE
          { UNSG32 u32;
            struct w32MIC2_INTLMODE;
                 } T32MIC2_INTLMODE;
    typedef union  T32MIC2_HBRDMAP
          { UNSG32 u32;
            struct w32MIC2_HBRDMAP;
                 } T32MIC2_HBRDMAP;
    typedef union  TMIC2_RXPORT
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_RXPORT;
                   };
                 } TMIC2_RXPORT;
    typedef union  TMIC2_RXDATA
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_RXDATA;
                   };
                 } TMIC2_RXDATA;
    typedef union  TMIC2_INTLMODE
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_INTLMODE;
                   };
                 } TMIC2_INTLMODE;
    typedef union  TMIC2_HBRDMAP
          { UNSG32 u32[1];
            struct {
            struct w32MIC2_HBRDMAP;
                   };
                 } TMIC2_HBRDMAP;
     SIGN32 MIC2_drvrd(SIE_MIC2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 MIC2_drvwr(SIE_MIC2 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void MIC2_reset(SIE_MIC2 *p);
     SIGN32 MIC2_cmp  (SIE_MIC2 *p, SIE_MIC2 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define MIC2_check(p,pie,pfx,hLOG) MIC2_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define MIC2_print(p,    pfx,hLOG) MIC2_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MIC3
#define h_MIC3 (){}
    #define     RA_MIC3_MICCTRL                                0x0000
    #define     RA_MIC3_RSD0                                   0x000C
    #define     RA_MIC3_DBG                                    0x0010
    #define     RA_MIC3_RSD1                                   0x001C
    #define     RA_MIC3_RSD2                                   0x0020
    #define     RA_MIC3_RSD3                                   0x0024
    #define     RA_MIC3_RXPORT                                 0x0028
    #define       bMIC3_RXPORT_ENABLE                          1
    #define        MIC3_RXPORT_ENABLE_DISABLE                               0x0
    #define        MIC3_RXPORT_ENABLE_ENABLE                                0x1
    #define     RA_MIC3_RXDATA                                 0x002C
    #define       bMIC3_RXDATA_HBR                             1
    #define       bMIC3_RXDATA_TDM_HR                          1
    #define     RA_MIC3_INTLMODE                               0x0030
    #define       bMIC3_INTLMODE_PORT0_EN                      1
    #define       bMIC3_INTLMODE_PORT1_EN                      1
    #define       bMIC3_INTLMODE_PORT2_EN                      1
    #define       bMIC3_INTLMODE_PORT3_EN                      1
    #define       bMIC3_INTLMODE_DUMMYDATA_EN                  1
    #define     RA_MIC3_HBRDMAP                                0x0034
    #define       bMIC3_HBRDMAP_PORT0                          2
    #define       bMIC3_HBRDMAP_PORT1                          2
    #define       bMIC3_HBRDMAP_PORT2                          2
    #define       bMIC3_HBRDMAP_PORT3                          2
    typedef struct SIE_MIC3 {
              SIE_PRIAUD                                       ie_MICCTRL;
              SIE_AUDCH                                        ie_RSD0;
              SIE_DBG_RX                                       ie_DBG;
              SIE_AUDCH                                        ie_RSD1;
              SIE_AUDCH                                        ie_RSD2;
              SIE_AUDCH                                        ie_RSD3;
    #define   SET32MIC3_RXPORT_ENABLE(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16MIC3_RXPORT_ENABLE(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32MIC3_RXPORT                                 {\
            UNSG32 uRXPORT_ENABLE                              :  1;\
            UNSG32 RSVDx28_b1                                  : 31;\
          }
    union { UNSG32 u32MIC3_RXPORT;
            struct w32MIC3_RXPORT;
          };
    #define   SET32MIC3_RXDATA_HBR(r32,v)                      _BFSET_(r32, 0, 0,v)
    #define   SET16MIC3_RXDATA_HBR(r16,v)                      _BFSET_(r16, 0, 0,v)
    #define   SET32MIC3_RXDATA_TDM_HR(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16MIC3_RXDATA_TDM_HR(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define     w32MIC3_RXDATA                                 {\
            UNSG32 uRXDATA_HBR                                 :  1;\
            UNSG32 uRXDATA_TDM_HR                              :  1;\
            UNSG32 RSVDx2C_b2                                  : 30;\
          }
    union { UNSG32 u32MIC3_RXDATA;
            struct w32MIC3_RXDATA;
          };
    #define   SET32MIC3_INTLMODE_PORT0_EN(r32,v)               _BFSET_(r32, 0, 0,v)
    #define   SET16MIC3_INTLMODE_PORT0_EN(r16,v)               _BFSET_(r16, 0, 0,v)
    #define   SET32MIC3_INTLMODE_PORT1_EN(r32,v)               _BFSET_(r32, 1, 1,v)
    #define   SET16MIC3_INTLMODE_PORT1_EN(r16,v)               _BFSET_(r16, 1, 1,v)
    #define   SET32MIC3_INTLMODE_PORT2_EN(r32,v)               _BFSET_(r32, 2, 2,v)
    #define   SET16MIC3_INTLMODE_PORT2_EN(r16,v)               _BFSET_(r16, 2, 2,v)
    #define   SET32MIC3_INTLMODE_PORT3_EN(r32,v)               _BFSET_(r32, 3, 3,v)
    #define   SET16MIC3_INTLMODE_PORT3_EN(r16,v)               _BFSET_(r16, 3, 3,v)
    #define   SET32MIC3_INTLMODE_DUMMYDATA_EN(r32,v)           _BFSET_(r32, 4, 4,v)
    #define   SET16MIC3_INTLMODE_DUMMYDATA_EN(r16,v)           _BFSET_(r16, 4, 4,v)
    #define     w32MIC3_INTLMODE                               {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx30_b5                                  : 27;\
          }
    union { UNSG32 u32MIC3_INTLMODE;
            struct w32MIC3_INTLMODE;
          };
    #define   SET32MIC3_HBRDMAP_PORT0(r32,v)                   _BFSET_(r32, 1, 0,v)
    #define   SET16MIC3_HBRDMAP_PORT0(r16,v)                   _BFSET_(r16, 1, 0,v)
    #define   SET32MIC3_HBRDMAP_PORT1(r32,v)                   _BFSET_(r32, 3, 2,v)
    #define   SET16MIC3_HBRDMAP_PORT1(r16,v)                   _BFSET_(r16, 3, 2,v)
    #define   SET32MIC3_HBRDMAP_PORT2(r32,v)                   _BFSET_(r32, 5, 4,v)
    #define   SET16MIC3_HBRDMAP_PORT2(r16,v)                   _BFSET_(r16, 5, 4,v)
    #define   SET32MIC3_HBRDMAP_PORT3(r32,v)                   _BFSET_(r32, 7, 6,v)
    #define   SET16MIC3_HBRDMAP_PORT3(r16,v)                   _BFSET_(r16, 7, 6,v)
    #define     w32MIC3_HBRDMAP                                {\
            UNSG32 uHBRDMAP_PORT0                              :  2;\
            UNSG32 uHBRDMAP_PORT1                              :  2;\
            UNSG32 uHBRDMAP_PORT2                              :  2;\
            UNSG32 uHBRDMAP_PORT3                              :  2;\
            UNSG32 RSVDx34_b8                                  : 24;\
          }
    union { UNSG32 u32MIC3_HBRDMAP;
            struct w32MIC3_HBRDMAP;
          };
    } SIE_MIC3;
    typedef union  T32MIC3_RXPORT
          { UNSG32 u32;
            struct w32MIC3_RXPORT;
                 } T32MIC3_RXPORT;
    typedef union  T32MIC3_RXDATA
          { UNSG32 u32;
            struct w32MIC3_RXDATA;
                 } T32MIC3_RXDATA;
    typedef union  T32MIC3_INTLMODE
          { UNSG32 u32;
            struct w32MIC3_INTLMODE;
                 } T32MIC3_INTLMODE;
    typedef union  T32MIC3_HBRDMAP
          { UNSG32 u32;
            struct w32MIC3_HBRDMAP;
                 } T32MIC3_HBRDMAP;
    typedef union  TMIC3_RXPORT
          { UNSG32 u32[1];
            struct {
            struct w32MIC3_RXPORT;
                   };
                 } TMIC3_RXPORT;
    typedef union  TMIC3_RXDATA
          { UNSG32 u32[1];
            struct {
            struct w32MIC3_RXDATA;
                   };
                 } TMIC3_RXDATA;
    typedef union  TMIC3_INTLMODE
          { UNSG32 u32[1];
            struct {
            struct w32MIC3_INTLMODE;
                   };
                 } TMIC3_INTLMODE;
    typedef union  TMIC3_HBRDMAP
          { UNSG32 u32[1];
            struct {
            struct w32MIC3_HBRDMAP;
                   };
                 } TMIC3_HBRDMAP;
     SIGN32 MIC3_drvrd(SIE_MIC3 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 MIC3_drvwr(SIE_MIC3 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void MIC3_reset(SIE_MIC3 *p);
     SIGN32 MIC3_cmp  (SIE_MIC3 *p, SIE_MIC3 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define MIC3_check(p,pie,pfx,hLOG) MIC3_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define MIC3_print(p,    pfx,hLOG) MIC3_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MIC4
#define h_MIC4 (){}
    #define     RA_MIC4_MICCTRL                                0x0000
    #define     RA_MIC4_RSD0                                   0x000C
    #define     RA_MIC4_DBG                                    0x0010
    #define     RA_MIC4_RSD1                                   0x001C
    #define     RA_MIC4_RSD2                                   0x0020
    #define     RA_MIC4_RSD3                                   0x0024
    #define     RA_MIC4_RXPORT                                 0x0028
    #define       bMIC4_RXPORT_ENABLE                          1
    #define        MIC4_RXPORT_ENABLE_DISABLE                               0x0
    #define        MIC4_RXPORT_ENABLE_ENABLE                                0x1
    #define     RA_MIC4_RXDATA                                 0x002C
    #define       bMIC4_RXDATA_HBR                             1
    #define       bMIC4_RXDATA_TDM_HR                          1
    #define     RA_MIC4_INTLMODE                               0x0030
    #define       bMIC4_INTLMODE_PORT0_EN                      1
    #define       bMIC4_INTLMODE_PORT1_EN                      1
    #define       bMIC4_INTLMODE_PORT2_EN                      1
    #define       bMIC4_INTLMODE_PORT3_EN                      1
    #define       bMIC4_INTLMODE_DUMMYDATA_EN                  1
    #define     RA_MIC4_HBRDMAP                                0x0034
    #define       bMIC4_HBRDMAP_PORT0                          2
    #define       bMIC4_HBRDMAP_PORT1                          2
    #define       bMIC4_HBRDMAP_PORT2                          2
    #define       bMIC4_HBRDMAP_PORT3                          2
    typedef struct SIE_MIC4 {
              SIE_PRIAUD                                       ie_MICCTRL;
              SIE_AUDCH                                        ie_RSD0;
              SIE_DBG_RX                                       ie_DBG;
              SIE_AUDCH                                        ie_RSD1;
              SIE_AUDCH                                        ie_RSD2;
              SIE_AUDCH                                        ie_RSD3;
    #define   SET32MIC4_RXPORT_ENABLE(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16MIC4_RXPORT_ENABLE(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32MIC4_RXPORT                                 {\
            UNSG32 uRXPORT_ENABLE                              :  1;\
            UNSG32 RSVDx28_b1                                  : 31;\
          }
    union { UNSG32 u32MIC4_RXPORT;
            struct w32MIC4_RXPORT;
          };
    #define   SET32MIC4_RXDATA_HBR(r32,v)                      _BFSET_(r32, 0, 0,v)
    #define   SET16MIC4_RXDATA_HBR(r16,v)                      _BFSET_(r16, 0, 0,v)
    #define   SET32MIC4_RXDATA_TDM_HR(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16MIC4_RXDATA_TDM_HR(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define     w32MIC4_RXDATA                                 {\
            UNSG32 uRXDATA_HBR                                 :  1;\
            UNSG32 uRXDATA_TDM_HR                              :  1;\
            UNSG32 RSVDx2C_b2                                  : 30;\
          }
    union { UNSG32 u32MIC4_RXDATA;
            struct w32MIC4_RXDATA;
          };
    #define   SET32MIC4_INTLMODE_PORT0_EN(r32,v)               _BFSET_(r32, 0, 0,v)
    #define   SET16MIC4_INTLMODE_PORT0_EN(r16,v)               _BFSET_(r16, 0, 0,v)
    #define   SET32MIC4_INTLMODE_PORT1_EN(r32,v)               _BFSET_(r32, 1, 1,v)
    #define   SET16MIC4_INTLMODE_PORT1_EN(r16,v)               _BFSET_(r16, 1, 1,v)
    #define   SET32MIC4_INTLMODE_PORT2_EN(r32,v)               _BFSET_(r32, 2, 2,v)
    #define   SET16MIC4_INTLMODE_PORT2_EN(r16,v)               _BFSET_(r16, 2, 2,v)
    #define   SET32MIC4_INTLMODE_PORT3_EN(r32,v)               _BFSET_(r32, 3, 3,v)
    #define   SET16MIC4_INTLMODE_PORT3_EN(r16,v)               _BFSET_(r16, 3, 3,v)
    #define   SET32MIC4_INTLMODE_DUMMYDATA_EN(r32,v)           _BFSET_(r32, 4, 4,v)
    #define   SET16MIC4_INTLMODE_DUMMYDATA_EN(r16,v)           _BFSET_(r16, 4, 4,v)
    #define     w32MIC4_INTLMODE                               {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx30_b5                                  : 27;\
          }
    union { UNSG32 u32MIC4_INTLMODE;
            struct w32MIC4_INTLMODE;
          };
    #define   SET32MIC4_HBRDMAP_PORT0(r32,v)                   _BFSET_(r32, 1, 0,v)
    #define   SET16MIC4_HBRDMAP_PORT0(r16,v)                   _BFSET_(r16, 1, 0,v)
    #define   SET32MIC4_HBRDMAP_PORT1(r32,v)                   _BFSET_(r32, 3, 2,v)
    #define   SET16MIC4_HBRDMAP_PORT1(r16,v)                   _BFSET_(r16, 3, 2,v)
    #define   SET32MIC4_HBRDMAP_PORT2(r32,v)                   _BFSET_(r32, 5, 4,v)
    #define   SET16MIC4_HBRDMAP_PORT2(r16,v)                   _BFSET_(r16, 5, 4,v)
    #define   SET32MIC4_HBRDMAP_PORT3(r32,v)                   _BFSET_(r32, 7, 6,v)
    #define   SET16MIC4_HBRDMAP_PORT3(r16,v)                   _BFSET_(r16, 7, 6,v)
    #define     w32MIC4_HBRDMAP                                {\
            UNSG32 uHBRDMAP_PORT0                              :  2;\
            UNSG32 uHBRDMAP_PORT1                              :  2;\
            UNSG32 uHBRDMAP_PORT2                              :  2;\
            UNSG32 uHBRDMAP_PORT3                              :  2;\
            UNSG32 RSVDx34_b8                                  : 24;\
          }
    union { UNSG32 u32MIC4_HBRDMAP;
            struct w32MIC4_HBRDMAP;
          };
    } SIE_MIC4;
    typedef union  T32MIC4_RXPORT
          { UNSG32 u32;
            struct w32MIC4_RXPORT;
                 } T32MIC4_RXPORT;
    typedef union  T32MIC4_RXDATA
          { UNSG32 u32;
            struct w32MIC4_RXDATA;
                 } T32MIC4_RXDATA;
    typedef union  T32MIC4_INTLMODE
          { UNSG32 u32;
            struct w32MIC4_INTLMODE;
                 } T32MIC4_INTLMODE;
    typedef union  T32MIC4_HBRDMAP
          { UNSG32 u32;
            struct w32MIC4_HBRDMAP;
                 } T32MIC4_HBRDMAP;
    typedef union  TMIC4_RXPORT
          { UNSG32 u32[1];
            struct {
            struct w32MIC4_RXPORT;
                   };
                 } TMIC4_RXPORT;
    typedef union  TMIC4_RXDATA
          { UNSG32 u32[1];
            struct {
            struct w32MIC4_RXDATA;
                   };
                 } TMIC4_RXDATA;
    typedef union  TMIC4_INTLMODE
          { UNSG32 u32[1];
            struct {
            struct w32MIC4_INTLMODE;
                   };
                 } TMIC4_INTLMODE;
    typedef union  TMIC4_HBRDMAP
          { UNSG32 u32[1];
            struct {
            struct w32MIC4_HBRDMAP;
                   };
                 } TMIC4_HBRDMAP;
     SIGN32 MIC4_drvrd(SIE_MIC4 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 MIC4_drvwr(SIE_MIC4 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void MIC4_reset(SIE_MIC4 *p);
     SIGN32 MIC4_cmp  (SIE_MIC4 *p, SIE_MIC4 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define MIC4_check(p,pie,pfx,hLOG) MIC4_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define MIC4_print(p,    pfx,hLOG) MIC4_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_MIC5
#define h_MIC5 (){}
    #define     RA_MIC5_MICCTRL                                0x0000
    #define     RA_MIC5_RSD0                                   0x000C
    #define     RA_MIC5_DBG                                    0x0010
    #define     RA_MIC5_RSD1                                   0x001C
    #define     RA_MIC5_RSD2                                   0x0020
    #define     RA_MIC5_RSD3                                   0x0024
    #define     RA_MIC5_RXPORT                                 0x0028
    #define       bMIC5_RXPORT_ENABLE                          1
    #define        MIC5_RXPORT_ENABLE_DISABLE                               0x0
    #define        MIC5_RXPORT_ENABLE_ENABLE                                0x1
    #define     RA_MIC5_RXDATA                                 0x002C
    #define       bMIC5_RXDATA_HBR                             1
    #define       bMIC5_RXDATA_TDM_HR                          1
    #define     RA_MIC5_INTLMODE                               0x0030
    #define       bMIC5_INTLMODE_PORT0_EN                      1
    #define       bMIC5_INTLMODE_PORT1_EN                      1
    #define       bMIC5_INTLMODE_PORT2_EN                      1
    #define       bMIC5_INTLMODE_PORT3_EN                      1
    #define       bMIC5_INTLMODE_DUMMYDATA_EN                  1
    #define     RA_MIC5_HBRDMAP                                0x0034
    #define       bMIC5_HBRDMAP_PORT0                          2
    #define       bMIC5_HBRDMAP_PORT1                          2
    #define       bMIC5_HBRDMAP_PORT2                          2
    #define       bMIC5_HBRDMAP_PORT3                          2
    typedef struct SIE_MIC5 {
              SIE_PRIAUD                                       ie_MICCTRL;
              SIE_AUDCH                                        ie_RSD0;
              SIE_DBG_RX                                       ie_DBG;
              SIE_AUDCH                                        ie_RSD1;
              SIE_AUDCH                                        ie_RSD2;
              SIE_AUDCH                                        ie_RSD3;
    #define   SET32MIC5_RXPORT_ENABLE(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16MIC5_RXPORT_ENABLE(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32MIC5_RXPORT                                 {\
            UNSG32 uRXPORT_ENABLE                              :  1;\
            UNSG32 RSVDx28_b1                                  : 31;\
          }
    union { UNSG32 u32MIC5_RXPORT;
            struct w32MIC5_RXPORT;
          };
    #define   SET32MIC5_RXDATA_HBR(r32,v)                      _BFSET_(r32, 0, 0,v)
    #define   SET16MIC5_RXDATA_HBR(r16,v)                      _BFSET_(r16, 0, 0,v)
    #define   SET32MIC5_RXDATA_TDM_HR(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16MIC5_RXDATA_TDM_HR(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define     w32MIC5_RXDATA                                 {\
            UNSG32 uRXDATA_HBR                                 :  1;\
            UNSG32 uRXDATA_TDM_HR                              :  1;\
            UNSG32 RSVDx2C_b2                                  : 30;\
          }
    union { UNSG32 u32MIC5_RXDATA;
            struct w32MIC5_RXDATA;
          };
    #define   SET32MIC5_INTLMODE_PORT0_EN(r32,v)               _BFSET_(r32, 0, 0,v)
    #define   SET16MIC5_INTLMODE_PORT0_EN(r16,v)               _BFSET_(r16, 0, 0,v)
    #define   SET32MIC5_INTLMODE_PORT1_EN(r32,v)               _BFSET_(r32, 1, 1,v)
    #define   SET16MIC5_INTLMODE_PORT1_EN(r16,v)               _BFSET_(r16, 1, 1,v)
    #define   SET32MIC5_INTLMODE_PORT2_EN(r32,v)               _BFSET_(r32, 2, 2,v)
    #define   SET16MIC5_INTLMODE_PORT2_EN(r16,v)               _BFSET_(r16, 2, 2,v)
    #define   SET32MIC5_INTLMODE_PORT3_EN(r32,v)               _BFSET_(r32, 3, 3,v)
    #define   SET16MIC5_INTLMODE_PORT3_EN(r16,v)               _BFSET_(r16, 3, 3,v)
    #define   SET32MIC5_INTLMODE_DUMMYDATA_EN(r32,v)           _BFSET_(r32, 4, 4,v)
    #define   SET16MIC5_INTLMODE_DUMMYDATA_EN(r16,v)           _BFSET_(r16, 4, 4,v)
    #define     w32MIC5_INTLMODE                               {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx30_b5                                  : 27;\
          }
    union { UNSG32 u32MIC5_INTLMODE;
            struct w32MIC5_INTLMODE;
          };
    #define   SET32MIC5_HBRDMAP_PORT0(r32,v)                   _BFSET_(r32, 1, 0,v)
    #define   SET16MIC5_HBRDMAP_PORT0(r16,v)                   _BFSET_(r16, 1, 0,v)
    #define   SET32MIC5_HBRDMAP_PORT1(r32,v)                   _BFSET_(r32, 3, 2,v)
    #define   SET16MIC5_HBRDMAP_PORT1(r16,v)                   _BFSET_(r16, 3, 2,v)
    #define   SET32MIC5_HBRDMAP_PORT2(r32,v)                   _BFSET_(r32, 5, 4,v)
    #define   SET16MIC5_HBRDMAP_PORT2(r16,v)                   _BFSET_(r16, 5, 4,v)
    #define   SET32MIC5_HBRDMAP_PORT3(r32,v)                   _BFSET_(r32, 7, 6,v)
    #define   SET16MIC5_HBRDMAP_PORT3(r16,v)                   _BFSET_(r16, 7, 6,v)
    #define     w32MIC5_HBRDMAP                                {\
            UNSG32 uHBRDMAP_PORT0                              :  2;\
            UNSG32 uHBRDMAP_PORT1                              :  2;\
            UNSG32 uHBRDMAP_PORT2                              :  2;\
            UNSG32 uHBRDMAP_PORT3                              :  2;\
            UNSG32 RSVDx34_b8                                  : 24;\
          }
    union { UNSG32 u32MIC5_HBRDMAP;
            struct w32MIC5_HBRDMAP;
          };
    } SIE_MIC5;
    typedef union  T32MIC5_RXPORT
          { UNSG32 u32;
            struct w32MIC5_RXPORT;
                 } T32MIC5_RXPORT;
    typedef union  T32MIC5_RXDATA
          { UNSG32 u32;
            struct w32MIC5_RXDATA;
                 } T32MIC5_RXDATA;
    typedef union  T32MIC5_INTLMODE
          { UNSG32 u32;
            struct w32MIC5_INTLMODE;
                 } T32MIC5_INTLMODE;
    typedef union  T32MIC5_HBRDMAP
          { UNSG32 u32;
            struct w32MIC5_HBRDMAP;
                 } T32MIC5_HBRDMAP;
    typedef union  TMIC5_RXPORT
          { UNSG32 u32[1];
            struct {
            struct w32MIC5_RXPORT;
                   };
                 } TMIC5_RXPORT;
    typedef union  TMIC5_RXDATA
          { UNSG32 u32[1];
            struct {
            struct w32MIC5_RXDATA;
                   };
                 } TMIC5_RXDATA;
    typedef union  TMIC5_INTLMODE
          { UNSG32 u32[1];
            struct {
            struct w32MIC5_INTLMODE;
                   };
                 } TMIC5_INTLMODE;
    typedef union  TMIC5_HBRDMAP
          { UNSG32 u32[1];
            struct {
            struct w32MIC5_HBRDMAP;
                   };
                 } TMIC5_HBRDMAP;
     SIGN32 MIC5_drvrd(SIE_MIC5 *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 MIC5_drvwr(SIE_MIC5 *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void MIC5_reset(SIE_MIC5 *p);
     SIGN32 MIC5_cmp  (SIE_MIC5 *p, SIE_MIC5 *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define MIC5_check(p,pie,pfx,hLOG) MIC5_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define MIC5_print(p,    pfx,hLOG) MIC5_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_PDMCH
#define h_PDMCH (){}
    #define     RA_PDMCH_CTRL                                  0x0000
    #define       bPDMCH_CTRL_ENABLE                           1
    #define        PDMCH_CTRL_ENABLE_DISABLE                                0x0
    #define        PDMCH_CTRL_ENABLE_ENABLE                                 0x1
    #define       bPDMCH_CTRL_MUTE                             1
    #define        PDMCH_CTRL_MUTE_MUTE_OFF                                 0x0
    #define        PDMCH_CTRL_MUTE_MUTE_ON                                  0x1
    #define       bPDMCH_CTRL_LRSWITCH                         1
    #define        PDMCH_CTRL_LRSWITCH_SWITCH_OFF                           0x0
    #define        PDMCH_CTRL_LRSWITCH_SWITCH_ON                            0x1
    #define       bPDMCH_CTRL_FLUSH                            1
    #define        PDMCH_CTRL_FLUSH_ON                                      0x1
    #define        PDMCH_CTRL_FLUSH_OFF                                     0x0
    #define     RA_PDMCH_CTRL2                                 0x0004
    #define       bPDMCH_CTRL2_RDLT                            16
    #define       bPDMCH_CTRL2_FDLT                            16
    typedef struct SIE_PDMCH {
    #define   SET32PDMCH_CTRL_ENABLE(r32,v)                    _BFSET_(r32, 0, 0,v)
    #define   SET16PDMCH_CTRL_ENABLE(r16,v)                    _BFSET_(r16, 0, 0,v)
    #define   SET32PDMCH_CTRL_MUTE(r32,v)                      _BFSET_(r32, 1, 1,v)
    #define   SET16PDMCH_CTRL_MUTE(r16,v)                      _BFSET_(r16, 1, 1,v)
    #define   SET32PDMCH_CTRL_LRSWITCH(r32,v)                  _BFSET_(r32, 2, 2,v)
    #define   SET16PDMCH_CTRL_LRSWITCH(r16,v)                  _BFSET_(r16, 2, 2,v)
    #define   SET32PDMCH_CTRL_FLUSH(r32,v)                     _BFSET_(r32, 3, 3,v)
    #define   SET16PDMCH_CTRL_FLUSH(r16,v)                     _BFSET_(r16, 3, 3,v)
    #define     w32PDMCH_CTRL                                  {\
            UNSG32 uCTRL_ENABLE                                :  1;\
            UNSG32 uCTRL_MUTE                                  :  1;\
            UNSG32 uCTRL_LRSWITCH                              :  1;\
            UNSG32 uCTRL_FLUSH                                 :  1;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32PDMCH_CTRL;
            struct w32PDMCH_CTRL;
          };
    #define   SET32PDMCH_CTRL2_RDLT(r32,v)                     _BFSET_(r32,15, 0,v)
    #define   SET16PDMCH_CTRL2_RDLT(r16,v)                     _BFSET_(r16,15, 0,v)
    #define   SET32PDMCH_CTRL2_FDLT(r32,v)                     _BFSET_(r32,31,16,v)
    #define   SET16PDMCH_CTRL2_FDLT(r16,v)                     _BFSET_(r16,15, 0,v)
    #define     w32PDMCH_CTRL2                                 {\
            UNSG32 uCTRL2_RDLT                                 : 16;\
            UNSG32 uCTRL2_FDLT                                 : 16;\
          }
    union { UNSG32 u32PDMCH_CTRL2;
            struct w32PDMCH_CTRL2;
          };
    } SIE_PDMCH;
    typedef union  T32PDMCH_CTRL
          { UNSG32 u32;
            struct w32PDMCH_CTRL;
                 } T32PDMCH_CTRL;
    typedef union  T32PDMCH_CTRL2
          { UNSG32 u32;
            struct w32PDMCH_CTRL2;
                 } T32PDMCH_CTRL2;
    typedef union  TPDMCH_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32PDMCH_CTRL;
                   };
                 } TPDMCH_CTRL;
    typedef union  TPDMCH_CTRL2
          { UNSG32 u32[1];
            struct {
            struct w32PDMCH_CTRL2;
                   };
                 } TPDMCH_CTRL2;
     SIGN32 PDMCH_drvrd(SIE_PDMCH *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 PDMCH_drvwr(SIE_PDMCH *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void PDMCH_reset(SIE_PDMCH *p);
     SIGN32 PDMCH_cmp  (SIE_PDMCH *p, SIE_PDMCH *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define PDMCH_check(p,pie,pfx,hLOG) PDMCH_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define PDMCH_print(p,    pfx,hLOG) PDMCH_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_PDM
#define h_PDM (){}
    #define     RA_PDM_CTRL1                                   0x0000
    #define       bPDM_CTRL1_CLKDIV                            4
    #define       bPDM_CTRL1_INVCLK_OUT                        1
    #define       bPDM_CTRL1_INVCLK_INT                        1
    #define       bPDM_CTRL1_CLKINT_SEL                        1
    #define       bPDM_CTRL1_RLSB                              1
    #define       bPDM_CTRL1_RDM                               3
    #define       bPDM_CTRL1_MODE                              1
    #define       bPDM_CTRL1_SDR_CLKSEL                        1
    #define       bPDM_CTRL1_LATCH_MODE                        1
    #define     RA_PDM_RXDATA                                  0x0004
    #define       bPDM_RXDATA_INTL                             1
    #define     RA_PDM_INTLMODE                                0x0008
    #define       bPDM_INTLMODE_PORT0_EN                       1
    #define       bPDM_INTLMODE_PORT1_EN                       1
    #define       bPDM_INTLMODE_PORT2_EN                       1
    #define       bPDM_INTLMODE_PORT3_EN                       1
    #define       bPDM_INTLMODE_DUMMYDATA_EN                   1
    #define     RA_PDM_INTLMAP                                 0x000C
    #define       bPDM_INTLMAP_PORT0                           2
    #define       bPDM_INTLMAP_PORT1                           2
    #define       bPDM_INTLMAP_PORT2                           2
    #define       bPDM_INTLMAP_PORT3                           2
    #define     RA_PDM_PDM0                                    0x0010
    #define     RA_PDM_PDM1                                    0x0018
    typedef struct SIE_PDM {
    #define   SET32PDM_CTRL1_CLKDIV(r32,v)                     _BFSET_(r32, 3, 0,v)
    #define   SET16PDM_CTRL1_CLKDIV(r16,v)                     _BFSET_(r16, 3, 0,v)
    #define   SET32PDM_CTRL1_INVCLK_OUT(r32,v)                 _BFSET_(r32, 4, 4,v)
    #define   SET16PDM_CTRL1_INVCLK_OUT(r16,v)                 _BFSET_(r16, 4, 4,v)
    #define   SET32PDM_CTRL1_INVCLK_INT(r32,v)                 _BFSET_(r32, 5, 5,v)
    #define   SET16PDM_CTRL1_INVCLK_INT(r16,v)                 _BFSET_(r16, 5, 5,v)
    #define   SET32PDM_CTRL1_CLKINT_SEL(r32,v)                 _BFSET_(r32, 6, 6,v)
    #define   SET16PDM_CTRL1_CLKINT_SEL(r16,v)                 _BFSET_(r16, 6, 6,v)
    #define   SET32PDM_CTRL1_RLSB(r32,v)                       _BFSET_(r32, 7, 7,v)
    #define   SET16PDM_CTRL1_RLSB(r16,v)                       _BFSET_(r16, 7, 7,v)
    #define   SET32PDM_CTRL1_RDM(r32,v)                        _BFSET_(r32,10, 8,v)
    #define   SET16PDM_CTRL1_RDM(r16,v)                        _BFSET_(r16,10, 8,v)
    #define   SET32PDM_CTRL1_MODE(r32,v)                       _BFSET_(r32,11,11,v)
    #define   SET16PDM_CTRL1_MODE(r16,v)                       _BFSET_(r16,11,11,v)
    #define   SET32PDM_CTRL1_SDR_CLKSEL(r32,v)                 _BFSET_(r32,12,12,v)
    #define   SET16PDM_CTRL1_SDR_CLKSEL(r16,v)                 _BFSET_(r16,12,12,v)
    #define   SET32PDM_CTRL1_LATCH_MODE(r32,v)                 _BFSET_(r32,13,13,v)
    #define   SET16PDM_CTRL1_LATCH_MODE(r16,v)                 _BFSET_(r16,13,13,v)
    #define     w32PDM_CTRL1                                   {\
            UNSG32 uCTRL1_CLKDIV                               :  4;\
            UNSG32 uCTRL1_INVCLK_OUT                           :  1;\
            UNSG32 uCTRL1_INVCLK_INT                           :  1;\
            UNSG32 uCTRL1_CLKINT_SEL                           :  1;\
            UNSG32 uCTRL1_RLSB                                 :  1;\
            UNSG32 uCTRL1_RDM                                  :  3;\
            UNSG32 uCTRL1_MODE                                 :  1;\
            UNSG32 uCTRL1_SDR_CLKSEL                           :  1;\
            UNSG32 uCTRL1_LATCH_MODE                           :  1;\
            UNSG32 RSVDx0_b14                                  : 18;\
          }
    union { UNSG32 u32PDM_CTRL1;
            struct w32PDM_CTRL1;
          };
    #define   SET32PDM_RXDATA_INTL(r32,v)                      _BFSET_(r32, 0, 0,v)
    #define   SET16PDM_RXDATA_INTL(r16,v)                      _BFSET_(r16, 0, 0,v)
    #define     w32PDM_RXDATA                                  {\
            UNSG32 uRXDATA_INTL                                :  1;\
            UNSG32 RSVDx4_b1                                   : 31;\
          }
    union { UNSG32 u32PDM_RXDATA;
            struct w32PDM_RXDATA;
          };
    #define   SET32PDM_INTLMODE_PORT0_EN(r32,v)                _BFSET_(r32, 0, 0,v)
    #define   SET16PDM_INTLMODE_PORT0_EN(r16,v)                _BFSET_(r16, 0, 0,v)
    #define   SET32PDM_INTLMODE_PORT1_EN(r32,v)                _BFSET_(r32, 1, 1,v)
    #define   SET16PDM_INTLMODE_PORT1_EN(r16,v)                _BFSET_(r16, 1, 1,v)
    #define   SET32PDM_INTLMODE_PORT2_EN(r32,v)                _BFSET_(r32, 2, 2,v)
    #define   SET16PDM_INTLMODE_PORT2_EN(r16,v)                _BFSET_(r16, 2, 2,v)
    #define   SET32PDM_INTLMODE_PORT3_EN(r32,v)                _BFSET_(r32, 3, 3,v)
    #define   SET16PDM_INTLMODE_PORT3_EN(r16,v)                _BFSET_(r16, 3, 3,v)
    #define   SET32PDM_INTLMODE_DUMMYDATA_EN(r32,v)            _BFSET_(r32, 4, 4,v)
    #define   SET16PDM_INTLMODE_DUMMYDATA_EN(r16,v)            _BFSET_(r16, 4, 4,v)
    #define     w32PDM_INTLMODE                                {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx8_b5                                   : 27;\
          }
    union { UNSG32 u32PDM_INTLMODE;
            struct w32PDM_INTLMODE;
          };
    #define   SET32PDM_INTLMAP_PORT0(r32,v)                    _BFSET_(r32, 1, 0,v)
    #define   SET16PDM_INTLMAP_PORT0(r16,v)                    _BFSET_(r16, 1, 0,v)
    #define   SET32PDM_INTLMAP_PORT1(r32,v)                    _BFSET_(r32, 3, 2,v)
    #define   SET16PDM_INTLMAP_PORT1(r16,v)                    _BFSET_(r16, 3, 2,v)
    #define   SET32PDM_INTLMAP_PORT2(r32,v)                    _BFSET_(r32, 5, 4,v)
    #define   SET16PDM_INTLMAP_PORT2(r16,v)                    _BFSET_(r16, 5, 4,v)
    #define   SET32PDM_INTLMAP_PORT3(r32,v)                    _BFSET_(r32, 7, 6,v)
    #define   SET16PDM_INTLMAP_PORT3(r16,v)                    _BFSET_(r16, 7, 6,v)
    #define     w32PDM_INTLMAP                                 {\
            UNSG32 uINTLMAP_PORT0                              :  2;\
            UNSG32 uINTLMAP_PORT1                              :  2;\
            UNSG32 uINTLMAP_PORT2                              :  2;\
            UNSG32 uINTLMAP_PORT3                              :  2;\
            UNSG32 RSVDxC_b8                                   : 24;\
          }
    union { UNSG32 u32PDM_INTLMAP;
            struct w32PDM_INTLMAP;
          };
              SIE_PDMCH                                        ie_PDM0;
              SIE_PDMCH                                        ie_PDM1;
    } SIE_PDM;
    typedef union  T32PDM_CTRL1
          { UNSG32 u32;
            struct w32PDM_CTRL1;
                 } T32PDM_CTRL1;
    typedef union  T32PDM_RXDATA
          { UNSG32 u32;
            struct w32PDM_RXDATA;
                 } T32PDM_RXDATA;
    typedef union  T32PDM_INTLMODE
          { UNSG32 u32;
            struct w32PDM_INTLMODE;
                 } T32PDM_INTLMODE;
    typedef union  T32PDM_INTLMAP
          { UNSG32 u32;
            struct w32PDM_INTLMAP;
                 } T32PDM_INTLMAP;
    typedef union  TPDM_CTRL1
          { UNSG32 u32[1];
            struct {
            struct w32PDM_CTRL1;
                   };
                 } TPDM_CTRL1;
    typedef union  TPDM_RXDATA
          { UNSG32 u32[1];
            struct {
            struct w32PDM_RXDATA;
                   };
                 } TPDM_RXDATA;
    typedef union  TPDM_INTLMODE
          { UNSG32 u32[1];
            struct {
            struct w32PDM_INTLMODE;
                   };
                 } TPDM_INTLMODE;
    typedef union  TPDM_INTLMAP
          { UNSG32 u32[1];
            struct {
            struct w32PDM_INTLMAP;
                   };
                 } TPDM_INTLMAP;
     SIGN32 PDM_drvrd(SIE_PDM *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 PDM_drvwr(SIE_PDM *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void PDM_reset(SIE_PDM *p);
     SIGN32 PDM_cmp  (SIE_PDM *p, SIE_PDM *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define PDM_check(p,pie,pfx,hLOG) PDM_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define PDM_print(p,    pfx,hLOG) PDM_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_DMIC
#define h_DMIC (){}
    #define     RA_DMIC_STATUS                                 0x0000
    #define       bDMIC_STATUS_Wake                            1
    #define       bDMIC_STATUS_PCM_FIFO_full                   1
    #define       bDMIC_STATUS_PDM_LFIFO_emp                   1
    #define       bDMIC_STATUS_PDM_RFIFO_emp                   1
    #define     RA_DMIC_CONTROL                                0x0004
    #define       bDMIC_CONTROL_Enable                         1
    #define       bDMIC_CONTROL_Enable_A                       1
    #define       bDMIC_CONTROL_Enable_B                       1
    #define       bDMIC_CONTROL_Enable_C                       1
    #define       bDMIC_CONTROL_Enable_D                       1
    #define       bDMIC_CONTROL_Run_A_L                        1
    #define       bDMIC_CONTROL_Run_A_R                        1
    #define       bDMIC_CONTROL_Run_B_L                        1
    #define       bDMIC_CONTROL_Run_B_R                        1
    #define       bDMIC_CONTROL_Run_C_L                        1
    #define       bDMIC_CONTROL_Run_C_R                        1
    #define       bDMIC_CONTROL_Run_D_L                        1
    #define       bDMIC_CONTROL_Run_D_R                        1
    #define       bDMIC_CONTROL_Wake_On_Sound                  4
    #define       bDMIC_CONTROL_Enable_DC_A                    1
    #define       bDMIC_CONTROL_Enable_DC_B                    1
    #define       bDMIC_CONTROL_Enable_DC_C                    1
    #define       bDMIC_CONTROL_Enable_DC_D                    1
    #define       bDMIC_CONTROL_Mono_A                         1
    #define       bDMIC_CONTROL_Mono_B                         1
    #define       bDMIC_CONTROL_Mono_C                         1
    #define       bDMIC_CONTROL_Mono_D                         1
    #define       bDMIC_CONTROL_PDMIn_D_LRSwp                  1
    #define       bDMIC_CONTROL_PDMIn_D_LJn_RJ                 1
    #define       bDMIC_CONTROL_PCM_LJn_RJ                     1
    #define     RA_DMIC_FLUSH                                  0x0008
    #define       bDMIC_FLUSH_PCMWR_FLUSH                      1
    #define       bDMIC_FLUSH_PDMRD_FLUSH                      1
    #define     RA_DMIC_DECIMATION_CONTROL                     0x000C
    #define       bDMIC_DECIMATION_CONTROL_CIC_Ratio_PCM       7
    #define       bDMIC_DECIMATION_CONTROL_CIC_Ratio_PCM_D     7
    #define       bDMIC_DECIMATION_CONTROL_PDM_Slots_Per_Frame 2
    #define       bDMIC_DECIMATION_CONTROL_FIR_Filter_Selection 2
    #define       bDMIC_DECIMATION_CONTROL_PDM_Bits_Per_Slot   5
    #define     RA_DMIC_MICROPHONE_CONFIGURATION               0x0010
    #define       bDMIC_MICROPHONE_CONFIGURATION_A_Left_Right_Swap 1
    #define       bDMIC_MICROPHONE_CONFIGURATION_A_Left_Right_Time_Order 1
    #define       bDMIC_MICROPHONE_CONFIGURATION_A_Store_PDM   1
    #define       bDMIC_MICROPHONE_CONFIGURATION_B_Left_Right_Swap 1
    #define       bDMIC_MICROPHONE_CONFIGURATION_B_Left_Right_Time_Order 1
    #define       bDMIC_MICROPHONE_CONFIGURATION_B_Store_PDM   1
    #define       bDMIC_MICROPHONE_CONFIGURATION_C_Left_Right_Swapz 1
    #define       bDMIC_MICROPHONE_CONFIGURATION_C_Left_Right_Time_Order 1
    #define       bDMIC_MICROPHONE_CONFIGURATION_C_Store_PDM   1
    #define       bDMIC_MICROPHONE_CONFIGURATION_D_Left_Right_Swap 1
    #define       bDMIC_MICROPHONE_CONFIGURATION_D_Left_Right_Time_Order 1
    #define       bDMIC_MICROPHONE_CONFIGURATION_D_Store_PDM   1
    #define       bDMIC_MICROPHONE_CONFIGURATION_D_PDM_from_ADMA 2
    #define     RA_DMIC_GAIN_MIC_PAIR_A                        0x0014
    #define       bDMIC_GAIN_MIC_PAIR_A_Gain_L                 10
    #define       bDMIC_GAIN_MIC_PAIR_A_Gain_R                 10
    #define     RA_DMIC_GAIN_MIC_PAIR_B                        0x0018
    #define       bDMIC_GAIN_MIC_PAIR_B_Gain_L                 10
    #define       bDMIC_GAIN_MIC_PAIR_B_Gain_R                 10
    #define     RA_DMIC_GAIN_MIC_PAIR_C                        0x001C
    #define       bDMIC_GAIN_MIC_PAIR_C_Gain_L                 10
    #define       bDMIC_GAIN_MIC_PAIR_C_Gain_R                 10
    #define     RA_DMIC_GAIN_MIC_PAIR_D                        0x0020
    #define       bDMIC_GAIN_MIC_PAIR_D_Gain_L                 10
    #define       bDMIC_GAIN_MIC_PAIR_D_Gain_R                 10
    #define     RA_DMIC_GAIN_RAMP_CONTROL                      0x0024
    #define       bDMIC_GAIN_RAMP_CONTROL_Step_Size            8
    #define       bDMIC_GAIN_RAMP_CONTROL_Step_Rate            3
    typedef struct SIE_DMIC {
    #define   SET32DMIC_STATUS_Wake(r32,v)                     _BFSET_(r32, 0, 0,v)
    #define   SET16DMIC_STATUS_Wake(r16,v)                     _BFSET_(r16, 0, 0,v)
    #define   SET32DMIC_STATUS_PCM_FIFO_full(r32,v)            _BFSET_(r32, 1, 1,v)
    #define   SET16DMIC_STATUS_PCM_FIFO_full(r16,v)            _BFSET_(r16, 1, 1,v)
    #define   SET32DMIC_STATUS_PDM_LFIFO_emp(r32,v)            _BFSET_(r32, 2, 2,v)
    #define   SET16DMIC_STATUS_PDM_LFIFO_emp(r16,v)            _BFSET_(r16, 2, 2,v)
    #define   SET32DMIC_STATUS_PDM_RFIFO_emp(r32,v)            _BFSET_(r32, 3, 3,v)
    #define   SET16DMIC_STATUS_PDM_RFIFO_emp(r16,v)            _BFSET_(r16, 3, 3,v)
    #define     w32DMIC_STATUS                                 {\
            UNSG32 uSTATUS_Wake                                :  1;\
            UNSG32 uSTATUS_PCM_FIFO_full                       :  1;\
            UNSG32 uSTATUS_PDM_LFIFO_emp                       :  1;\
            UNSG32 uSTATUS_PDM_RFIFO_emp                       :  1;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32DMIC_STATUS;
            struct w32DMIC_STATUS;
          };
    #define   SET32DMIC_CONTROL_Enable(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16DMIC_CONTROL_Enable(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32DMIC_CONTROL_Enable_A(r32,v)                _BFSET_(r32, 1, 1,v)
    #define   SET16DMIC_CONTROL_Enable_A(r16,v)                _BFSET_(r16, 1, 1,v)
    #define   SET32DMIC_CONTROL_Enable_B(r32,v)                _BFSET_(r32, 2, 2,v)
    #define   SET16DMIC_CONTROL_Enable_B(r16,v)                _BFSET_(r16, 2, 2,v)
    #define   SET32DMIC_CONTROL_Enable_C(r32,v)                _BFSET_(r32, 3, 3,v)
    #define   SET16DMIC_CONTROL_Enable_C(r16,v)                _BFSET_(r16, 3, 3,v)
    #define   SET32DMIC_CONTROL_Enable_D(r32,v)                _BFSET_(r32, 4, 4,v)
    #define   SET16DMIC_CONTROL_Enable_D(r16,v)                _BFSET_(r16, 4, 4,v)
    #define   SET32DMIC_CONTROL_Run_A_L(r32,v)                 _BFSET_(r32, 5, 5,v)
    #define   SET16DMIC_CONTROL_Run_A_L(r16,v)                 _BFSET_(r16, 5, 5,v)
    #define   SET32DMIC_CONTROL_Run_A_R(r32,v)                 _BFSET_(r32, 6, 6,v)
    #define   SET16DMIC_CONTROL_Run_A_R(r16,v)                 _BFSET_(r16, 6, 6,v)
    #define   SET32DMIC_CONTROL_Run_B_L(r32,v)                 _BFSET_(r32, 7, 7,v)
    #define   SET16DMIC_CONTROL_Run_B_L(r16,v)                 _BFSET_(r16, 7, 7,v)
    #define   SET32DMIC_CONTROL_Run_B_R(r32,v)                 _BFSET_(r32, 8, 8,v)
    #define   SET16DMIC_CONTROL_Run_B_R(r16,v)                 _BFSET_(r16, 8, 8,v)
    #define   SET32DMIC_CONTROL_Run_C_L(r32,v)                 _BFSET_(r32, 9, 9,v)
    #define   SET16DMIC_CONTROL_Run_C_L(r16,v)                 _BFSET_(r16, 9, 9,v)
    #define   SET32DMIC_CONTROL_Run_C_R(r32,v)                 _BFSET_(r32,10,10,v)
    #define   SET16DMIC_CONTROL_Run_C_R(r16,v)                 _BFSET_(r16,10,10,v)
    #define   SET32DMIC_CONTROL_Run_D_L(r32,v)                 _BFSET_(r32,11,11,v)
    #define   SET16DMIC_CONTROL_Run_D_L(r16,v)                 _BFSET_(r16,11,11,v)
    #define   SET32DMIC_CONTROL_Run_D_R(r32,v)                 _BFSET_(r32,12,12,v)
    #define   SET16DMIC_CONTROL_Run_D_R(r16,v)                 _BFSET_(r16,12,12,v)
    #define   SET32DMIC_CONTROL_Wake_On_Sound(r32,v)           _BFSET_(r32,16,13,v)
    #define   SET32DMIC_CONTROL_Enable_DC_A(r32,v)             _BFSET_(r32,17,17,v)
    #define   SET16DMIC_CONTROL_Enable_DC_A(r16,v)             _BFSET_(r16, 1, 1,v)
    #define   SET32DMIC_CONTROL_Enable_DC_B(r32,v)             _BFSET_(r32,18,18,v)
    #define   SET16DMIC_CONTROL_Enable_DC_B(r16,v)             _BFSET_(r16, 2, 2,v)
    #define   SET32DMIC_CONTROL_Enable_DC_C(r32,v)             _BFSET_(r32,19,19,v)
    #define   SET16DMIC_CONTROL_Enable_DC_C(r16,v)             _BFSET_(r16, 3, 3,v)
    #define   SET32DMIC_CONTROL_Enable_DC_D(r32,v)             _BFSET_(r32,20,20,v)
    #define   SET16DMIC_CONTROL_Enable_DC_D(r16,v)             _BFSET_(r16, 4, 4,v)
    #define   SET32DMIC_CONTROL_Mono_A(r32,v)                  _BFSET_(r32,21,21,v)
    #define   SET16DMIC_CONTROL_Mono_A(r16,v)                  _BFSET_(r16, 5, 5,v)
    #define   SET32DMIC_CONTROL_Mono_B(r32,v)                  _BFSET_(r32,22,22,v)
    #define   SET16DMIC_CONTROL_Mono_B(r16,v)                  _BFSET_(r16, 6, 6,v)
    #define   SET32DMIC_CONTROL_Mono_C(r32,v)                  _BFSET_(r32,23,23,v)
    #define   SET16DMIC_CONTROL_Mono_C(r16,v)                  _BFSET_(r16, 7, 7,v)
    #define   SET32DMIC_CONTROL_Mono_D(r32,v)                  _BFSET_(r32,24,24,v)
    #define   SET16DMIC_CONTROL_Mono_D(r16,v)                  _BFSET_(r16, 8, 8,v)
    #define   SET32DMIC_CONTROL_PDMIn_D_LRSwp(r32,v)           _BFSET_(r32,25,25,v)
    #define   SET16DMIC_CONTROL_PDMIn_D_LRSwp(r16,v)           _BFSET_(r16, 9, 9,v)
    #define   SET32DMIC_CONTROL_PDMIn_D_LJn_RJ(r32,v)          _BFSET_(r32,26,26,v)
    #define   SET16DMIC_CONTROL_PDMIn_D_LJn_RJ(r16,v)          _BFSET_(r16,10,10,v)
    #define   SET32DMIC_CONTROL_PCM_LJn_RJ(r32,v)              _BFSET_(r32,27,27,v)
    #define   SET16DMIC_CONTROL_PCM_LJn_RJ(r16,v)              _BFSET_(r16,11,11,v)
    #define     w32DMIC_CONTROL                                {\
            UNSG32 uCONTROL_Enable                             :  1;\
            UNSG32 uCONTROL_Enable_A                           :  1;\
            UNSG32 uCONTROL_Enable_B                           :  1;\
            UNSG32 uCONTROL_Enable_C                           :  1;\
            UNSG32 uCONTROL_Enable_D                           :  1;\
            UNSG32 uCONTROL_Run_A_L                            :  1;\
            UNSG32 uCONTROL_Run_A_R                            :  1;\
            UNSG32 uCONTROL_Run_B_L                            :  1;\
            UNSG32 uCONTROL_Run_B_R                            :  1;\
            UNSG32 uCONTROL_Run_C_L                            :  1;\
            UNSG32 uCONTROL_Run_C_R                            :  1;\
            UNSG32 uCONTROL_Run_D_L                            :  1;\
            UNSG32 uCONTROL_Run_D_R                            :  1;\
            UNSG32 uCONTROL_Wake_On_Sound                      :  4;\
            UNSG32 uCONTROL_Enable_DC_A                        :  1;\
            UNSG32 uCONTROL_Enable_DC_B                        :  1;\
            UNSG32 uCONTROL_Enable_DC_C                        :  1;\
            UNSG32 uCONTROL_Enable_DC_D                        :  1;\
            UNSG32 uCONTROL_Mono_A                             :  1;\
            UNSG32 uCONTROL_Mono_B                             :  1;\
            UNSG32 uCONTROL_Mono_C                             :  1;\
            UNSG32 uCONTROL_Mono_D                             :  1;\
            UNSG32 uCONTROL_PDMIn_D_LRSwp                      :  1;\
            UNSG32 uCONTROL_PDMIn_D_LJn_RJ                     :  1;\
            UNSG32 uCONTROL_PCM_LJn_RJ                         :  1;\
            UNSG32 RSVDx4_b28                                  :  4;\
          }
    union { UNSG32 u32DMIC_CONTROL;
            struct w32DMIC_CONTROL;
          };
    #define   SET32DMIC_FLUSH_PCMWR_FLUSH(r32,v)               _BFSET_(r32, 0, 0,v)
    #define   SET16DMIC_FLUSH_PCMWR_FLUSH(r16,v)               _BFSET_(r16, 0, 0,v)
    #define   SET32DMIC_FLUSH_PDMRD_FLUSH(r32,v)               _BFSET_(r32, 1, 1,v)
    #define   SET16DMIC_FLUSH_PDMRD_FLUSH(r16,v)               _BFSET_(r16, 1, 1,v)
    #define     w32DMIC_FLUSH                                  {\
            UNSG32 uFLUSH_PCMWR_FLUSH                          :  1;\
            UNSG32 uFLUSH_PDMRD_FLUSH                          :  1;\
            UNSG32 RSVDx8_b2                                   : 30;\
          }
    union { UNSG32 u32DMIC_FLUSH;
            struct w32DMIC_FLUSH;
          };
    #define   SET32DMIC_DECIMATION_CONTROL_CIC_Ratio_PCM(r32,v) _BFSET_(r32, 6, 0,v)
    #define   SET16DMIC_DECIMATION_CONTROL_CIC_Ratio_PCM(r16,v) _BFSET_(r16, 6, 0,v)
    #define   SET32DMIC_DECIMATION_CONTROL_CIC_Ratio_PCM_D(r32,v) _BFSET_(r32,13, 7,v)
    #define   SET16DMIC_DECIMATION_CONTROL_CIC_Ratio_PCM_D(r16,v) _BFSET_(r16,13, 7,v)
    #define   SET32DMIC_DECIMATION_CONTROL_PDM_Slots_Per_Frame(r32,v) _BFSET_(r32,15,14,v)
    #define   SET16DMIC_DECIMATION_CONTROL_PDM_Slots_Per_Frame(r16,v) _BFSET_(r16,15,14,v)
    #define   SET32DMIC_DECIMATION_CONTROL_FIR_Filter_Selection(r32,v) _BFSET_(r32,17,16,v)
    #define   SET16DMIC_DECIMATION_CONTROL_FIR_Filter_Selection(r16,v) _BFSET_(r16, 1, 0,v)
    #define   SET32DMIC_DECIMATION_CONTROL_PDM_Bits_Per_Slot(r32,v) _BFSET_(r32,22,18,v)
    #define   SET16DMIC_DECIMATION_CONTROL_PDM_Bits_Per_Slot(r16,v) _BFSET_(r16, 6, 2,v)
    #define     w32DMIC_DECIMATION_CONTROL                     {\
            UNSG32 uDECIMATION_CONTROL_CIC_Ratio_PCM           :  7;\
            UNSG32 uDECIMATION_CONTROL_CIC_Ratio_PCM_D         :  7;\
            UNSG32 uDECIMATION_CONTROL_PDM_Slots_Per_Frame     :  2;\
            UNSG32 uDECIMATION_CONTROL_FIR_Filter_Selection    :  2;\
            UNSG32 uDECIMATION_CONTROL_PDM_Bits_Per_Slot       :  5;\
            UNSG32 RSVDxC_b23                                  :  9;\
          }
    union { UNSG32 u32DMIC_DECIMATION_CONTROL;
            struct w32DMIC_DECIMATION_CONTROL;
          };
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_A_Left_Right_Swap(r32,v) _BFSET_(r32, 0, 0,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_A_Left_Right_Swap(r16,v) _BFSET_(r16, 0, 0,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_A_Left_Right_Time_Order(r32,v) _BFSET_(r32, 1, 1,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_A_Left_Right_Time_Order(r16,v) _BFSET_(r16, 1, 1,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_A_Store_PDM(r32,v) _BFSET_(r32, 2, 2,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_A_Store_PDM(r16,v) _BFSET_(r16, 2, 2,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_B_Left_Right_Swap(r32,v) _BFSET_(r32, 3, 3,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_B_Left_Right_Swap(r16,v) _BFSET_(r16, 3, 3,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_B_Left_Right_Time_Order(r32,v) _BFSET_(r32, 4, 4,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_B_Left_Right_Time_Order(r16,v) _BFSET_(r16, 4, 4,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_B_Store_PDM(r32,v) _BFSET_(r32, 5, 5,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_B_Store_PDM(r16,v) _BFSET_(r16, 5, 5,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_C_Left_Right_Swapz(r32,v) _BFSET_(r32, 6, 6,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_C_Left_Right_Swapz(r16,v) _BFSET_(r16, 6, 6,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_C_Left_Right_Time_Order(r32,v) _BFSET_(r32, 7, 7,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_C_Left_Right_Time_Order(r16,v) _BFSET_(r16, 7, 7,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_C_Store_PDM(r32,v) _BFSET_(r32, 8, 8,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_C_Store_PDM(r16,v) _BFSET_(r16, 8, 8,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_D_Left_Right_Swap(r32,v) _BFSET_(r32, 9, 9,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_D_Left_Right_Swap(r16,v) _BFSET_(r16, 9, 9,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_D_Left_Right_Time_Order(r32,v) _BFSET_(r32,10,10,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_D_Left_Right_Time_Order(r16,v) _BFSET_(r16,10,10,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_D_Store_PDM(r32,v) _BFSET_(r32,11,11,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_D_Store_PDM(r16,v) _BFSET_(r16,11,11,v)
    #define   SET32DMIC_MICROPHONE_CONFIGURATION_D_PDM_from_ADMA(r32,v) _BFSET_(r32,13,12,v)
    #define   SET16DMIC_MICROPHONE_CONFIGURATION_D_PDM_from_ADMA(r16,v) _BFSET_(r16,13,12,v)
    #define     w32DMIC_MICROPHONE_CONFIGURATION               {\
            UNSG32 uMICROPHONE_CONFIGURATION_A_Left_Right_Swap :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_A_Left_Right_Time_Order :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_A_Store_PDM       :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_B_Left_Right_Swap :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_B_Left_Right_Time_Order :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_B_Store_PDM       :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_C_Left_Right_Swapz :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_C_Left_Right_Time_Order :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_C_Store_PDM       :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_D_Left_Right_Swap :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_D_Left_Right_Time_Order :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_D_Store_PDM       :  1;\
            UNSG32 uMICROPHONE_CONFIGURATION_D_PDM_from_ADMA   :  2;\
            UNSG32 RSVDx10_b14                                 : 18;\
          }
    union { UNSG32 u32DMIC_MICROPHONE_CONFIGURATION;
            struct w32DMIC_MICROPHONE_CONFIGURATION;
          };
    #define   SET32DMIC_GAIN_MIC_PAIR_A_Gain_L(r32,v)          _BFSET_(r32, 9, 0,v)
    #define   SET16DMIC_GAIN_MIC_PAIR_A_Gain_L(r16,v)          _BFSET_(r16, 9, 0,v)
    #define   SET32DMIC_GAIN_MIC_PAIR_A_Gain_R(r32,v)          _BFSET_(r32,19,10,v)
    #define     w32DMIC_GAIN_MIC_PAIR_A                        {\
            UNSG32 uGAIN_MIC_PAIR_A_Gain_L                     : 10;\
            UNSG32 uGAIN_MIC_PAIR_A_Gain_R                     : 10;\
            UNSG32 RSVDx14_b20                                 : 12;\
          }
    union { UNSG32 u32DMIC_GAIN_MIC_PAIR_A;
            struct w32DMIC_GAIN_MIC_PAIR_A;
          };
    #define   SET32DMIC_GAIN_MIC_PAIR_B_Gain_L(r32,v)          _BFSET_(r32, 9, 0,v)
    #define   SET16DMIC_GAIN_MIC_PAIR_B_Gain_L(r16,v)          _BFSET_(r16, 9, 0,v)
    #define   SET32DMIC_GAIN_MIC_PAIR_B_Gain_R(r32,v)          _BFSET_(r32,19,10,v)
    #define     w32DMIC_GAIN_MIC_PAIR_B                        {\
            UNSG32 uGAIN_MIC_PAIR_B_Gain_L                     : 10;\
            UNSG32 uGAIN_MIC_PAIR_B_Gain_R                     : 10;\
            UNSG32 RSVDx18_b20                                 : 12;\
          }
    union { UNSG32 u32DMIC_GAIN_MIC_PAIR_B;
            struct w32DMIC_GAIN_MIC_PAIR_B;
          };
    #define   SET32DMIC_GAIN_MIC_PAIR_C_Gain_L(r32,v)          _BFSET_(r32, 9, 0,v)
    #define   SET16DMIC_GAIN_MIC_PAIR_C_Gain_L(r16,v)          _BFSET_(r16, 9, 0,v)
    #define   SET32DMIC_GAIN_MIC_PAIR_C_Gain_R(r32,v)          _BFSET_(r32,19,10,v)
    #define     w32DMIC_GAIN_MIC_PAIR_C                        {\
            UNSG32 uGAIN_MIC_PAIR_C_Gain_L                     : 10;\
            UNSG32 uGAIN_MIC_PAIR_C_Gain_R                     : 10;\
            UNSG32 RSVDx1C_b20                                 : 12;\
          }
    union { UNSG32 u32DMIC_GAIN_MIC_PAIR_C;
            struct w32DMIC_GAIN_MIC_PAIR_C;
          };
    #define   SET32DMIC_GAIN_MIC_PAIR_D_Gain_L(r32,v)          _BFSET_(r32, 9, 0,v)
    #define   SET16DMIC_GAIN_MIC_PAIR_D_Gain_L(r16,v)          _BFSET_(r16, 9, 0,v)
    #define   SET32DMIC_GAIN_MIC_PAIR_D_Gain_R(r32,v)          _BFSET_(r32,19,10,v)
    #define     w32DMIC_GAIN_MIC_PAIR_D                        {\
            UNSG32 uGAIN_MIC_PAIR_D_Gain_L                     : 10;\
            UNSG32 uGAIN_MIC_PAIR_D_Gain_R                     : 10;\
            UNSG32 RSVDx20_b20                                 : 12;\
          }
    union { UNSG32 u32DMIC_GAIN_MIC_PAIR_D;
            struct w32DMIC_GAIN_MIC_PAIR_D;
          };
    #define   SET32DMIC_GAIN_RAMP_CONTROL_Step_Size(r32,v)     _BFSET_(r32, 7, 0,v)
    #define   SET16DMIC_GAIN_RAMP_CONTROL_Step_Size(r16,v)     _BFSET_(r16, 7, 0,v)
    #define   SET32DMIC_GAIN_RAMP_CONTROL_Step_Rate(r32,v)     _BFSET_(r32,10, 8,v)
    #define   SET16DMIC_GAIN_RAMP_CONTROL_Step_Rate(r16,v)     _BFSET_(r16,10, 8,v)
    #define     w32DMIC_GAIN_RAMP_CONTROL                      {\
            UNSG32 uGAIN_RAMP_CONTROL_Step_Size                :  8;\
            UNSG32 uGAIN_RAMP_CONTROL_Step_Rate                :  3;\
            UNSG32 RSVDx24_b11                                 : 21;\
          }
    union { UNSG32 u32DMIC_GAIN_RAMP_CONTROL;
            struct w32DMIC_GAIN_RAMP_CONTROL;
          };
    } SIE_DMIC;
    typedef union  T32DMIC_STATUS
          { UNSG32 u32;
            struct w32DMIC_STATUS;
                 } T32DMIC_STATUS;
    typedef union  T32DMIC_CONTROL
          { UNSG32 u32;
            struct w32DMIC_CONTROL;
                 } T32DMIC_CONTROL;
    typedef union  T32DMIC_FLUSH
          { UNSG32 u32;
            struct w32DMIC_FLUSH;
                 } T32DMIC_FLUSH;
    typedef union  T32DMIC_DECIMATION_CONTROL
          { UNSG32 u32;
            struct w32DMIC_DECIMATION_CONTROL;
                 } T32DMIC_DECIMATION_CONTROL;
    typedef union  T32DMIC_MICROPHONE_CONFIGURATION
          { UNSG32 u32;
            struct w32DMIC_MICROPHONE_CONFIGURATION;
                 } T32DMIC_MICROPHONE_CONFIGURATION;
    typedef union  T32DMIC_GAIN_MIC_PAIR_A
          { UNSG32 u32;
            struct w32DMIC_GAIN_MIC_PAIR_A;
                 } T32DMIC_GAIN_MIC_PAIR_A;
    typedef union  T32DMIC_GAIN_MIC_PAIR_B
          { UNSG32 u32;
            struct w32DMIC_GAIN_MIC_PAIR_B;
                 } T32DMIC_GAIN_MIC_PAIR_B;
    typedef union  T32DMIC_GAIN_MIC_PAIR_C
          { UNSG32 u32;
            struct w32DMIC_GAIN_MIC_PAIR_C;
                 } T32DMIC_GAIN_MIC_PAIR_C;
    typedef union  T32DMIC_GAIN_MIC_PAIR_D
          { UNSG32 u32;
            struct w32DMIC_GAIN_MIC_PAIR_D;
                 } T32DMIC_GAIN_MIC_PAIR_D;
    typedef union  T32DMIC_GAIN_RAMP_CONTROL
          { UNSG32 u32;
            struct w32DMIC_GAIN_RAMP_CONTROL;
                 } T32DMIC_GAIN_RAMP_CONTROL;
    typedef union  TDMIC_STATUS
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_STATUS;
                   };
                 } TDMIC_STATUS;
    typedef union  TDMIC_CONTROL
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_CONTROL;
                   };
                 } TDMIC_CONTROL;
    typedef union  TDMIC_FLUSH
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_FLUSH;
                   };
                 } TDMIC_FLUSH;
    typedef union  TDMIC_DECIMATION_CONTROL
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_DECIMATION_CONTROL;
                   };
                 } TDMIC_DECIMATION_CONTROL;
    typedef union  TDMIC_MICROPHONE_CONFIGURATION
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_MICROPHONE_CONFIGURATION;
                   };
                 } TDMIC_MICROPHONE_CONFIGURATION;
    typedef union  TDMIC_GAIN_MIC_PAIR_A
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_MIC_PAIR_A;
                   };
                 } TDMIC_GAIN_MIC_PAIR_A;
    typedef union  TDMIC_GAIN_MIC_PAIR_B
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_MIC_PAIR_B;
                   };
                 } TDMIC_GAIN_MIC_PAIR_B;
    typedef union  TDMIC_GAIN_MIC_PAIR_C
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_MIC_PAIR_C;
                   };
                 } TDMIC_GAIN_MIC_PAIR_C;
    typedef union  TDMIC_GAIN_MIC_PAIR_D
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_MIC_PAIR_D;
                   };
                 } TDMIC_GAIN_MIC_PAIR_D;
    typedef union  TDMIC_GAIN_RAMP_CONTROL
          { UNSG32 u32[1];
            struct {
            struct w32DMIC_GAIN_RAMP_CONTROL;
                   };
                 } TDMIC_GAIN_RAMP_CONTROL;
     SIGN32 DMIC_drvrd(SIE_DMIC *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 DMIC_drvwr(SIE_DMIC *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void DMIC_reset(SIE_DMIC *p);
     SIGN32 DMIC_cmp  (SIE_DMIC *p, SIE_DMIC *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define DMIC_check(p,pie,pfx,hLOG) DMIC_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define DMIC_print(p,    pfx,hLOG) DMIC_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_IOSEL
#define h_IOSEL (){}
    #define     RA_IOSEL_PRIMCLK                               0x0000
    #define       bIOSEL_PRIMCLK_SEL                           1
    #define     RA_IOSEL_MIC1MCLK                              0x0004
    #define       bIOSEL_MIC1MCLK_SEL                          1
    #define     RA_IOSEL_PRIBCLK                               0x0008
    #define       bIOSEL_PRIBCLK_SEL                           2
    #define       bIOSEL_PRIBCLK_INV                           1
    #define     RA_IOSEL_SECBCLK                               0x000C
    #define       bIOSEL_SECBCLK_SEL                           2
    #define       bIOSEL_SECBCLK_INV                           1
    #define     RA_IOSEL_MIC1BCLK                              0x0010
    #define       bIOSEL_MIC1BCLK_SEL                          2
    #define       bIOSEL_MIC1BCLK_INV                          1
    #define     RA_IOSEL_PRIFSYNC                              0x0014
    #define       bIOSEL_PRIFSYNC_SEL                          1
    #define       bIOSEL_PRIFSYNC_INV                          1
    #define     RA_IOSEL_SECFSYNC                              0x0018
    #define       bIOSEL_SECFSYNC_SEL                          1
    #define       bIOSEL_SECFSYNC_INV                          1
    #define     RA_IOSEL_MIC1FSYNC                             0x001C
    #define       bIOSEL_MIC1FSYNC_SEL                         1
    #define       bIOSEL_MIC1FSYNC_INV                         1
    #define     RA_IOSEL_MIC2FSYNC                             0x0020
    #define       bIOSEL_MIC2FSYNC_SEL                         1
    #define       bIOSEL_MIC2FSYNC_INV                         1
    #define     RA_IOSEL_MIC2BCLK                              0x0024
    #define       bIOSEL_MIC2BCLK_SEL                          2
    #define       bIOSEL_MIC2BCLK_INV                          1
    #define     RA_IOSEL_PDM                                   0x0028
    #define       bIOSEL_PDM_GENABLE                           1
    typedef struct SIE_IOSEL {
    #define   SET32IOSEL_PRIMCLK_SEL(r32,v)                    _BFSET_(r32, 0, 0,v)
    #define   SET16IOSEL_PRIMCLK_SEL(r16,v)                    _BFSET_(r16, 0, 0,v)
    #define     w32IOSEL_PRIMCLK                               {\
            UNSG32 uPRIMCLK_SEL                                :  1;\
            UNSG32 RSVDx0_b1                                   : 31;\
          }
    union { UNSG32 u32IOSEL_PRIMCLK;
            struct w32IOSEL_PRIMCLK;
          };
    #define   SET32IOSEL_MIC1MCLK_SEL(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16IOSEL_MIC1MCLK_SEL(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32IOSEL_MIC1MCLK                              {\
            UNSG32 uMIC1MCLK_SEL                               :  1;\
            UNSG32 RSVDx4_b1                                   : 31;\
          }
    union { UNSG32 u32IOSEL_MIC1MCLK;
            struct w32IOSEL_MIC1MCLK;
          };
    #define   SET32IOSEL_PRIBCLK_SEL(r32,v)                    _BFSET_(r32, 1, 0,v)
    #define   SET16IOSEL_PRIBCLK_SEL(r16,v)                    _BFSET_(r16, 1, 0,v)
    #define   SET32IOSEL_PRIBCLK_INV(r32,v)                    _BFSET_(r32, 2, 2,v)
    #define   SET16IOSEL_PRIBCLK_INV(r16,v)                    _BFSET_(r16, 2, 2,v)
    #define     w32IOSEL_PRIBCLK                               {\
            UNSG32 uPRIBCLK_SEL                                :  2;\
            UNSG32 uPRIBCLK_INV                                :  1;\
            UNSG32 RSVDx8_b3                                   : 29;\
          }
    union { UNSG32 u32IOSEL_PRIBCLK;
            struct w32IOSEL_PRIBCLK;
          };
    #define   SET32IOSEL_SECBCLK_SEL(r32,v)                    _BFSET_(r32, 1, 0,v)
    #define   SET16IOSEL_SECBCLK_SEL(r16,v)                    _BFSET_(r16, 1, 0,v)
    #define   SET32IOSEL_SECBCLK_INV(r32,v)                    _BFSET_(r32, 2, 2,v)
    #define   SET16IOSEL_SECBCLK_INV(r16,v)                    _BFSET_(r16, 2, 2,v)
    #define     w32IOSEL_SECBCLK                               {\
            UNSG32 uSECBCLK_SEL                                :  2;\
            UNSG32 uSECBCLK_INV                                :  1;\
            UNSG32 RSVDxC_b3                                   : 29;\
          }
    union { UNSG32 u32IOSEL_SECBCLK;
            struct w32IOSEL_SECBCLK;
          };
    #define   SET32IOSEL_MIC1BCLK_SEL(r32,v)                   _BFSET_(r32, 1, 0,v)
    #define   SET16IOSEL_MIC1BCLK_SEL(r16,v)                   _BFSET_(r16, 1, 0,v)
    #define   SET32IOSEL_MIC1BCLK_INV(r32,v)                   _BFSET_(r32, 2, 2,v)
    #define   SET16IOSEL_MIC1BCLK_INV(r16,v)                   _BFSET_(r16, 2, 2,v)
    #define     w32IOSEL_MIC1BCLK                              {\
            UNSG32 uMIC1BCLK_SEL                               :  2;\
            UNSG32 uMIC1BCLK_INV                               :  1;\
            UNSG32 RSVDx10_b3                                  : 29;\
          }
    union { UNSG32 u32IOSEL_MIC1BCLK;
            struct w32IOSEL_MIC1BCLK;
          };
    #define   SET32IOSEL_PRIFSYNC_SEL(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16IOSEL_PRIFSYNC_SEL(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define   SET32IOSEL_PRIFSYNC_INV(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16IOSEL_PRIFSYNC_INV(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define     w32IOSEL_PRIFSYNC                              {\
            UNSG32 uPRIFSYNC_SEL                               :  1;\
            UNSG32 uPRIFSYNC_INV                               :  1;\
            UNSG32 RSVDx14_b2                                  : 30;\
          }
    union { UNSG32 u32IOSEL_PRIFSYNC;
            struct w32IOSEL_PRIFSYNC;
          };
    #define   SET32IOSEL_SECFSYNC_SEL(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16IOSEL_SECFSYNC_SEL(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define   SET32IOSEL_SECFSYNC_INV(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16IOSEL_SECFSYNC_INV(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define     w32IOSEL_SECFSYNC                              {\
            UNSG32 uSECFSYNC_SEL                               :  1;\
            UNSG32 uSECFSYNC_INV                               :  1;\
            UNSG32 RSVDx18_b2                                  : 30;\
          }
    union { UNSG32 u32IOSEL_SECFSYNC;
            struct w32IOSEL_SECFSYNC;
          };
    #define   SET32IOSEL_MIC1FSYNC_SEL(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16IOSEL_MIC1FSYNC_SEL(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32IOSEL_MIC1FSYNC_INV(r32,v)                  _BFSET_(r32, 1, 1,v)
    #define   SET16IOSEL_MIC1FSYNC_INV(r16,v)                  _BFSET_(r16, 1, 1,v)
    #define     w32IOSEL_MIC1FSYNC                             {\
            UNSG32 uMIC1FSYNC_SEL                              :  1;\
            UNSG32 uMIC1FSYNC_INV                              :  1;\
            UNSG32 RSVDx1C_b2                                  : 30;\
          }
    union { UNSG32 u32IOSEL_MIC1FSYNC;
            struct w32IOSEL_MIC1FSYNC;
          };
    #define   SET32IOSEL_MIC2FSYNC_SEL(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16IOSEL_MIC2FSYNC_SEL(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32IOSEL_MIC2FSYNC_INV(r32,v)                  _BFSET_(r32, 1, 1,v)
    #define   SET16IOSEL_MIC2FSYNC_INV(r16,v)                  _BFSET_(r16, 1, 1,v)
    #define     w32IOSEL_MIC2FSYNC                             {\
            UNSG32 uMIC2FSYNC_SEL                              :  1;\
            UNSG32 uMIC2FSYNC_INV                              :  1;\
            UNSG32 RSVDx20_b2                                  : 30;\
          }
    union { UNSG32 u32IOSEL_MIC2FSYNC;
            struct w32IOSEL_MIC2FSYNC;
          };
    #define   SET32IOSEL_MIC2BCLK_SEL(r32,v)                   _BFSET_(r32, 1, 0,v)
    #define   SET16IOSEL_MIC2BCLK_SEL(r16,v)                   _BFSET_(r16, 1, 0,v)
    #define   SET32IOSEL_MIC2BCLK_INV(r32,v)                   _BFSET_(r32, 2, 2,v)
    #define   SET16IOSEL_MIC2BCLK_INV(r16,v)                   _BFSET_(r16, 2, 2,v)
    #define     w32IOSEL_MIC2BCLK                              {\
            UNSG32 uMIC2BCLK_SEL                               :  2;\
            UNSG32 uMIC2BCLK_INV                               :  1;\
            UNSG32 RSVDx24_b3                                  : 29;\
          }
    union { UNSG32 u32IOSEL_MIC2BCLK;
            struct w32IOSEL_MIC2BCLK;
          };
    #define   SET32IOSEL_PDM_GENABLE(r32,v)                    _BFSET_(r32, 0, 0,v)
    #define   SET16IOSEL_PDM_GENABLE(r16,v)                    _BFSET_(r16, 0, 0,v)
    #define     w32IOSEL_PDM                                   {\
            UNSG32 uPDM_GENABLE                                :  1;\
            UNSG32 RSVDx28_b1                                  : 31;\
          }
    union { UNSG32 u32IOSEL_PDM;
            struct w32IOSEL_PDM;
          };
    } SIE_IOSEL;
    typedef union  T32IOSEL_PRIMCLK
          { UNSG32 u32;
            struct w32IOSEL_PRIMCLK;
                 } T32IOSEL_PRIMCLK;
    typedef union  T32IOSEL_MIC1MCLK
          { UNSG32 u32;
            struct w32IOSEL_MIC1MCLK;
                 } T32IOSEL_MIC1MCLK;
    typedef union  T32IOSEL_PRIBCLK
          { UNSG32 u32;
            struct w32IOSEL_PRIBCLK;
                 } T32IOSEL_PRIBCLK;
    typedef union  T32IOSEL_SECBCLK
          { UNSG32 u32;
            struct w32IOSEL_SECBCLK;
                 } T32IOSEL_SECBCLK;
    typedef union  T32IOSEL_MIC1BCLK
          { UNSG32 u32;
            struct w32IOSEL_MIC1BCLK;
                 } T32IOSEL_MIC1BCLK;
    typedef union  T32IOSEL_PRIFSYNC
          { UNSG32 u32;
            struct w32IOSEL_PRIFSYNC;
                 } T32IOSEL_PRIFSYNC;
    typedef union  T32IOSEL_SECFSYNC
          { UNSG32 u32;
            struct w32IOSEL_SECFSYNC;
                 } T32IOSEL_SECFSYNC;
    typedef union  T32IOSEL_MIC1FSYNC
          { UNSG32 u32;
            struct w32IOSEL_MIC1FSYNC;
                 } T32IOSEL_MIC1FSYNC;
    typedef union  T32IOSEL_MIC2FSYNC
          { UNSG32 u32;
            struct w32IOSEL_MIC2FSYNC;
                 } T32IOSEL_MIC2FSYNC;
    typedef union  T32IOSEL_MIC2BCLK
          { UNSG32 u32;
            struct w32IOSEL_MIC2BCLK;
                 } T32IOSEL_MIC2BCLK;
    typedef union  T32IOSEL_PDM
          { UNSG32 u32;
            struct w32IOSEL_PDM;
                 } T32IOSEL_PDM;
    typedef union  TIOSEL_PRIMCLK
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_PRIMCLK;
                   };
                 } TIOSEL_PRIMCLK;
    typedef union  TIOSEL_MIC1MCLK
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_MIC1MCLK;
                   };
                 } TIOSEL_MIC1MCLK;
    typedef union  TIOSEL_PRIBCLK
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_PRIBCLK;
                   };
                 } TIOSEL_PRIBCLK;
    typedef union  TIOSEL_SECBCLK
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_SECBCLK;
                   };
                 } TIOSEL_SECBCLK;
    typedef union  TIOSEL_MIC1BCLK
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_MIC1BCLK;
                   };
                 } TIOSEL_MIC1BCLK;
    typedef union  TIOSEL_PRIFSYNC
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_PRIFSYNC;
                   };
                 } TIOSEL_PRIFSYNC;
    typedef union  TIOSEL_SECFSYNC
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_SECFSYNC;
                   };
                 } TIOSEL_SECFSYNC;
    typedef union  TIOSEL_MIC1FSYNC
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_MIC1FSYNC;
                   };
                 } TIOSEL_MIC1FSYNC;
    typedef union  TIOSEL_MIC2FSYNC
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_MIC2FSYNC;
                   };
                 } TIOSEL_MIC2FSYNC;
    typedef union  TIOSEL_MIC2BCLK
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_MIC2BCLK;
                   };
                 } TIOSEL_MIC2BCLK;
    typedef union  TIOSEL_PDM
          { UNSG32 u32[1];
            struct {
            struct w32IOSEL_PDM;
                   };
                 } TIOSEL_PDM;
     SIGN32 IOSEL_drvrd(SIE_IOSEL *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 IOSEL_drvwr(SIE_IOSEL *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void IOSEL_reset(SIE_IOSEL *p);
     SIGN32 IOSEL_cmp  (SIE_IOSEL *p, SIE_IOSEL *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define IOSEL_check(p,pie,pfx,hLOG) IOSEL_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define IOSEL_print(p,    pfx,hLOG) IOSEL_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AIO
#define h_AIO (){}
    #define     RA_AIO_PRI                                     0x0000
    #define     RA_AIO_SEC                                     0x0028
    #define     RA_AIO_MIC1                                    0x0044
    #define     RA_AIO_MIC2                                    0x0078
    #define     RA_AIO_MIC4                                    0x00A4
    #define     RA_AIO_PDM                                     0x00DC
    #define     RA_AIO_DMIC_CLK                                0x00FC
    #define     RA_AIO_DMIC                                    0x010C
    #define     RA_AIO_IOSEL                                   0x0134
    #define     RA_AIO_IRQENABLE                               0x0160
    #define       bAIO_IRQENABLE_PRIIRQ                        1
    #define       bAIO_IRQENABLE_SECIRQ                        1
    #define       bAIO_IRQENABLE_MIC1IRQ                       1
    #define       bAIO_IRQENABLE_MIC2IRQ                       1
    #define       bAIO_IRQENABLE_MIC3IRQ                       1
    #define       bAIO_IRQENABLE_MIC4IRQ                       1
    #define       bAIO_IRQENABLE_MIC5IRQ                       1
    #define       bAIO_IRQENABLE_SPDIFIRQ                      1
    #define       bAIO_IRQENABLE_SPDIF1IRQ                     1
    #define       bAIO_IRQENABLE_HDMIIRQ                       1
    #define       bAIO_IRQENABLE_PDMIRQ                        1
    #define       bAIO_IRQENABLE_SPDIFRXIRQ                    1
    #define       bAIO_IRQENABLE_DMICIRQ                       1
    #define     RA_AIO_IRQSTS                                  0x0164
    #define       bAIO_IRQSTS_PRISTS                           1
    #define       bAIO_IRQSTS_SECSTS                           1
    #define       bAIO_IRQSTS_MIC1STS                          1
    #define       bAIO_IRQSTS_MIC2STS                          1
    #define       bAIO_IRQSTS_MIC3STS                          1
    #define       bAIO_IRQSTS_MIC4STS                          1
    #define       bAIO_IRQSTS_MIC5STS                          1
    #define       bAIO_IRQSTS_SPDIFSTS                         1
    #define       bAIO_IRQSTS_SPDIF1STS                        1
    #define       bAIO_IRQSTS_HDMISTS                          1
    #define       bAIO_IRQSTS_PDMSTS                           1
    #define       bAIO_IRQSTS_SPDIFRXSTS                       1
    #define       bAIO_IRQSTS_DMICSTS                          1
    #define     RA_AIO_PRISRC                                  0x0168
    #define       bAIO_PRISRC_SEL                              2
    #define       bAIO_PRISRC_L0DATAMAP                        2
    #define       bAIO_PRISRC_L1DATAMAP                        2
    #define       bAIO_PRISRC_L2DATAMAP                        2
    #define       bAIO_PRISRC_L3DATAMAP                        2
    #define       bAIO_PRISRC_SLAVEMODE                        1
    #define     RA_AIO_SECSRC                                  0x016C
    #define       bAIO_SECSRC_SEL                              2
    #define       bAIO_SECSRC_L0DATAMAP                        2
    #define       bAIO_SECSRC_L1DATAMAP                        2
    #define       bAIO_SECSRC_L2DATAMAP                        2
    #define       bAIO_SECSRC_L3DATAMAP                        2
    #define       bAIO_SECSRC_SLAVEMODE                        1
    #define     RA_AIO_PDM_MIC_SEL                             0x0170
    #define       bAIO_PDM_MIC_SEL_CTRL                        4
    #define     RA_AIO_MCLKPRI                                 0x0174
    #define     RA_AIO_MCLKSEC                                 0x0178
    #define     RA_AIO_MCLKPDM                                 0x017C
    #define     RA_AIO_MCLKMIC1                                0x0180
    #define     RA_AIO_MCLKMIC2                                0x0184
    #define     RA_AIO_SW_RST                                  0x0188
    #define       bAIO_SW_RST_SPFRX                            1
    #define       bAIO_SW_RST_REFCLK                           1
    #define       bAIO_SW_RST_MIC3                             1
    #define       bAIO_SW_RST_MIC4                             1
    #define       bAIO_SW_RST_MIC5                             1
    #define     RA_AIO_CLK_GATE_EN                             0x018C
    #define       bAIO_CLK_GATE_EN_MIC3                        1
    #define       bAIO_CLK_GATE_EN_MIC4                        1
    #define       bAIO_CLK_GATE_EN_MIC5                        1
    #define       bAIO_CLK_GATE_EN_GBL                         1
    #define     RA_AIO_EARC_ARC                                0x0190
    #define       bAIO_EARC_ARC_SEL                            1
    #define     RA_AIO_SAMP_CTRL                               0x0194
    #define       bAIO_SAMP_CTRL_EN_I2STX1                     1
    #define       bAIO_SAMP_CTRL_EN_I2STX2                     1
    #define       bAIO_SAMP_CTRL_EN_SPDIFTX                    1
    #define       bAIO_SAMP_CTRL_EN_SPDIFTX1                   1
    #define       bAIO_SAMP_CTRL_EN_HDMI                       1
    #define       bAIO_SAMP_CTRL_EN_HDMIARCTX                  1
    #define       bAIO_SAMP_CTRL_EN_SPDIFRX                    1
    #define       bAIO_SAMP_CTRL_EN_I2SRX1                     1
    #define       bAIO_SAMP_CTRL_EN_I2SRX2                     1
    #define       bAIO_SAMP_CTRL_EN_I2SRX3                     1
    #define       bAIO_SAMP_CTRL_EN_I2SRX4                     1
    #define       bAIO_SAMP_CTRL_EN_I2SRX5                     1
    #define       bAIO_SAMP_CTRL_EN_AUDTIMER                   1
    #define       bAIO_SAMP_CTRL_EN_PDMRX1                     1
    #define     RA_AIO_SAMPINFO_REQ                            0x0198
    #define       bAIO_SAMPINFO_REQ_I2STX1                     1
    #define       bAIO_SAMPINFO_REQ_I2STX2                     1
    #define       bAIO_SAMPINFO_REQ_HDMITX                     1
    #define       bAIO_SAMPINFO_REQ_HDMIARCTX                  1
    #define       bAIO_SAMPINFO_REQ_SPDIFTX                    1
    #define       bAIO_SAMPINFO_REQ_SPDIFTX1                   1
    #define       bAIO_SAMPINFO_REQ_SPDIFRX                    1
    #define       bAIO_SAMPINFO_REQ_I2SRX1                     1
    #define       bAIO_SAMPINFO_REQ_I2SRX2                     1
    #define       bAIO_SAMPINFO_REQ_I2SRX3                     1
    #define       bAIO_SAMPINFO_REQ_I2SRX4                     1
    #define       bAIO_SAMPINFO_REQ_I2SRX5                     1
    #define       bAIO_SAMPINFO_REQ_PDMRX1                     1
    #define     RA_AIO_SCR                                     0x019C
    #define       bAIO_SCR_I2STX1                              32
    #define     RA_AIO_SCR1                                    0x01A0
    #define       bAIO_SCR_I2STX2                              32
    #define     RA_AIO_SCR2                                    0x01A4
    #define       bAIO_SCR_HDMITX                              32
    #define     RA_AIO_SCR3                                    0x01A8
    #define       bAIO_SCR_HDMIARCTX                           32
    #define     RA_AIO_SCR4                                    0x01AC
    #define       bAIO_SCR_SPDIFTX                             32
    #define     RA_AIO_SCR5                                    0x01B0
    #define       bAIO_SCR_SPDIFTX1                            32
    #define     RA_AIO_SCR6                                    0x01B4
    #define       bAIO_SCR_SPDIFRX                             32
    #define     RA_AIO_SCR7                                    0x01B8
    #define       bAIO_SCR_I2SRX1                              32
    #define     RA_AIO_SCR8                                    0x01BC
    #define       bAIO_SCR_I2SRX2                              32
    #define     RA_AIO_SCR9                                    0x01C0
    #define       bAIO_SCR_I2SRX3                              32
    #define     RA_AIO_SCR10                                   0x01C4
    #define       bAIO_SCR_I2SRX4                              32
    #define     RA_AIO_SCR11                                   0x01C8
    #define       bAIO_SCR_I2SRX5                              32
    #define     RA_AIO_SCR12                                   0x01CC
    #define       bAIO_SCR_PDMRX1                              32
    #define     RA_AIO_STR                                     0x01D0
    #define       bAIO_STR_I2STX1                              32
    #define     RA_AIO_STR1                                    0x01D4
    #define       bAIO_STR_I2STX2                              32
    #define     RA_AIO_STR2                                    0x01D8
    #define       bAIO_STR_SPDIFTX                             32
    #define     RA_AIO_STR3                                    0x01DC
    #define       bAIO_STR_SPDIFTX1                            32
    #define     RA_AIO_STR4                                    0x01E0
    #define       bAIO_STR_HDMITX                              32
    #define     RA_AIO_STR5                                    0x01E4
    #define       bAIO_STR_HDMIARCTX                           32
    #define     RA_AIO_STR6                                    0x01E8
    #define       bAIO_STR_SPDIFRX                             32
    #define     RA_AIO_STR7                                    0x01EC
    #define       bAIO_STR_I2SRX1                              32
    #define     RA_AIO_STR8                                    0x01F0
    #define       bAIO_STR_I2SRX2                              32
    #define     RA_AIO_STR9                                    0x01F4
    #define       bAIO_STR_I2SRX3                              32
    #define     RA_AIO_STR10                                   0x01F8
    #define       bAIO_STR_I2SRX4                              32
    #define     RA_AIO_STR11                                   0x01FC
    #define       bAIO_STR_I2SRX5                              32
    #define     RA_AIO_STR12                                   0x0200
    #define       bAIO_STR_PDMRX1                              32
    #define     RA_AIO_ATR                                     0x0204
    #define       bAIO_ATR_TIMER                               32
    #define     RA_AIO_XFEED                                   0x0208
    #define       bAIO_XFEED_I2S1_LRCKIO_MODE                  2
    #define       bAIO_XFEED_I2S1_BCLKIO_MODE                  2
    #define       bAIO_XFEED_I2S2_LRCKIO_MODE                  2
    #define       bAIO_XFEED_I2S2_BCLKIO_MODE                  2
    #define       bAIO_XFEED_I2S3_LRCKIO_MODE                  1
    #define       bAIO_XFEED_I2S3_BCLKIO_MODE                  1
    #define       bAIO_XFEED_PDM_CLK_SEL                       3
    #define       bAIO_XFEED_PDMC_SEL                          1
    #define       bAIO_XFEED_PDM_SEL                           4
    #define     RA_AIO_DMIC_SRAMPWR                            0x020C
    typedef struct SIE_AIO {
              SIE_PRI                                          ie_PRI;
              SIE_SEC                                          ie_SEC;
              SIE_MIC1                                         ie_MIC1;
              SIE_MIC2                                         ie_MIC2;
              SIE_MIC4                                         ie_MIC4;
              SIE_PDM                                          ie_PDM;
              SIE_DMIC_CLK                                     ie_DMIC_CLK;
              SIE_DMIC                                         ie_DMIC;
              SIE_IOSEL                                        ie_IOSEL;
    #define   SET32AIO_IRQENABLE_PRIIRQ(r32,v)                 _BFSET_(r32, 0, 0,v)
    #define   SET16AIO_IRQENABLE_PRIIRQ(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define   SET32AIO_IRQENABLE_SECIRQ(r32,v)                 _BFSET_(r32, 1, 1,v)
    #define   SET16AIO_IRQENABLE_SECIRQ(r16,v)                 _BFSET_(r16, 1, 1,v)
    #define   SET32AIO_IRQENABLE_MIC1IRQ(r32,v)                _BFSET_(r32, 2, 2,v)
    #define   SET16AIO_IRQENABLE_MIC1IRQ(r16,v)                _BFSET_(r16, 2, 2,v)
    #define   SET32AIO_IRQENABLE_MIC2IRQ(r32,v)                _BFSET_(r32, 3, 3,v)
    #define   SET16AIO_IRQENABLE_MIC2IRQ(r16,v)                _BFSET_(r16, 3, 3,v)
    #define   SET32AIO_IRQENABLE_MIC3IRQ(r32,v)                _BFSET_(r32, 4, 4,v)
    #define   SET16AIO_IRQENABLE_MIC3IRQ(r16,v)                _BFSET_(r16, 4, 4,v)
    #define   SET32AIO_IRQENABLE_MIC4IRQ(r32,v)                _BFSET_(r32, 5, 5,v)
    #define   SET16AIO_IRQENABLE_MIC4IRQ(r16,v)                _BFSET_(r16, 5, 5,v)
    #define   SET32AIO_IRQENABLE_MIC5IRQ(r32,v)                _BFSET_(r32, 6, 6,v)
    #define   SET16AIO_IRQENABLE_MIC5IRQ(r16,v)                _BFSET_(r16, 6, 6,v)
    #define   SET32AIO_IRQENABLE_SPDIFIRQ(r32,v)               _BFSET_(r32, 7, 7,v)
    #define   SET16AIO_IRQENABLE_SPDIFIRQ(r16,v)               _BFSET_(r16, 7, 7,v)
    #define   SET32AIO_IRQENABLE_SPDIF1IRQ(r32,v)              _BFSET_(r32, 8, 8,v)
    #define   SET16AIO_IRQENABLE_SPDIF1IRQ(r16,v)              _BFSET_(r16, 8, 8,v)
    #define   SET32AIO_IRQENABLE_HDMIIRQ(r32,v)                _BFSET_(r32, 9, 9,v)
    #define   SET16AIO_IRQENABLE_HDMIIRQ(r16,v)                _BFSET_(r16, 9, 9,v)
    #define   SET32AIO_IRQENABLE_PDMIRQ(r32,v)                 _BFSET_(r32,10,10,v)
    #define   SET16AIO_IRQENABLE_PDMIRQ(r16,v)                 _BFSET_(r16,10,10,v)
    #define   SET32AIO_IRQENABLE_SPDIFRXIRQ(r32,v)             _BFSET_(r32,11,11,v)
    #define   SET16AIO_IRQENABLE_SPDIFRXIRQ(r16,v)             _BFSET_(r16,11,11,v)
    #define   SET32AIO_IRQENABLE_DMICIRQ(r32,v)                _BFSET_(r32,12,12,v)
    #define   SET16AIO_IRQENABLE_DMICIRQ(r16,v)                _BFSET_(r16,12,12,v)
    #define     w32AIO_IRQENABLE                               {\
            UNSG32 uIRQENABLE_PRIIRQ                           :  1;\
            UNSG32 uIRQENABLE_SECIRQ                           :  1;\
            UNSG32 uIRQENABLE_MIC1IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC2IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC3IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC4IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC5IRQ                          :  1;\
            UNSG32 uIRQENABLE_SPDIFIRQ                         :  1;\
            UNSG32 uIRQENABLE_SPDIF1IRQ                        :  1;\
            UNSG32 uIRQENABLE_HDMIIRQ                          :  1;\
            UNSG32 uIRQENABLE_PDMIRQ                           :  1;\
            UNSG32 uIRQENABLE_SPDIFRXIRQ                       :  1;\
            UNSG32 uIRQENABLE_DMICIRQ                          :  1;\
            UNSG32 RSVDx160_b13                                : 19;\
          }
    union { UNSG32 u32AIO_IRQENABLE;
            struct w32AIO_IRQENABLE;
          };
    #define   SET32AIO_IRQSTS_PRISTS(r32,v)                    _BFSET_(r32, 0, 0,v)
    #define   SET16AIO_IRQSTS_PRISTS(r16,v)                    _BFSET_(r16, 0, 0,v)
    #define   SET32AIO_IRQSTS_SECSTS(r32,v)                    _BFSET_(r32, 1, 1,v)
    #define   SET16AIO_IRQSTS_SECSTS(r16,v)                    _BFSET_(r16, 1, 1,v)
    #define   SET32AIO_IRQSTS_MIC1STS(r32,v)                   _BFSET_(r32, 2, 2,v)
    #define   SET16AIO_IRQSTS_MIC1STS(r16,v)                   _BFSET_(r16, 2, 2,v)
    #define   SET32AIO_IRQSTS_MIC2STS(r32,v)                   _BFSET_(r32, 3, 3,v)
    #define   SET16AIO_IRQSTS_MIC2STS(r16,v)                   _BFSET_(r16, 3, 3,v)
    #define   SET32AIO_IRQSTS_MIC3STS(r32,v)                   _BFSET_(r32, 4, 4,v)
    #define   SET16AIO_IRQSTS_MIC3STS(r16,v)                   _BFSET_(r16, 4, 4,v)
    #define   SET32AIO_IRQSTS_MIC4STS(r32,v)                   _BFSET_(r32, 5, 5,v)
    #define   SET16AIO_IRQSTS_MIC4STS(r16,v)                   _BFSET_(r16, 5, 5,v)
    #define   SET32AIO_IRQSTS_MIC5STS(r32,v)                   _BFSET_(r32, 6, 6,v)
    #define   SET16AIO_IRQSTS_MIC5STS(r16,v)                   _BFSET_(r16, 6, 6,v)
    #define   SET32AIO_IRQSTS_SPDIFSTS(r32,v)                  _BFSET_(r32, 7, 7,v)
    #define   SET16AIO_IRQSTS_SPDIFSTS(r16,v)                  _BFSET_(r16, 7, 7,v)
    #define   SET32AIO_IRQSTS_SPDIF1STS(r32,v)                 _BFSET_(r32, 8, 8,v)
    #define   SET16AIO_IRQSTS_SPDIF1STS(r16,v)                 _BFSET_(r16, 8, 8,v)
    #define   SET32AIO_IRQSTS_HDMISTS(r32,v)                   _BFSET_(r32, 9, 9,v)
    #define   SET16AIO_IRQSTS_HDMISTS(r16,v)                   _BFSET_(r16, 9, 9,v)
    #define   SET32AIO_IRQSTS_PDMSTS(r32,v)                    _BFSET_(r32,10,10,v)
    #define   SET16AIO_IRQSTS_PDMSTS(r16,v)                    _BFSET_(r16,10,10,v)
    #define   SET32AIO_IRQSTS_SPDIFRXSTS(r32,v)                _BFSET_(r32,11,11,v)
    #define   SET16AIO_IRQSTS_SPDIFRXSTS(r16,v)                _BFSET_(r16,11,11,v)
    #define   SET32AIO_IRQSTS_DMICSTS(r32,v)                   _BFSET_(r32,12,12,v)
    #define   SET16AIO_IRQSTS_DMICSTS(r16,v)                   _BFSET_(r16,12,12,v)
    #define     w32AIO_IRQSTS                                  {\
            UNSG32 uIRQSTS_PRISTS                              :  1;\
            UNSG32 uIRQSTS_SECSTS                              :  1;\
            UNSG32 uIRQSTS_MIC1STS                             :  1;\
            UNSG32 uIRQSTS_MIC2STS                             :  1;\
            UNSG32 uIRQSTS_MIC3STS                             :  1;\
            UNSG32 uIRQSTS_MIC4STS                             :  1;\
            UNSG32 uIRQSTS_MIC5STS                             :  1;\
            UNSG32 uIRQSTS_SPDIFSTS                            :  1;\
            UNSG32 uIRQSTS_SPDIF1STS                           :  1;\
            UNSG32 uIRQSTS_HDMISTS                             :  1;\
            UNSG32 uIRQSTS_PDMSTS                              :  1;\
            UNSG32 uIRQSTS_SPDIFRXSTS                          :  1;\
            UNSG32 uIRQSTS_DMICSTS                             :  1;\
            UNSG32 RSVDx164_b13                                : 19;\
          }
    union { UNSG32 u32AIO_IRQSTS;
            struct w32AIO_IRQSTS;
          };
    #define   SET32AIO_PRISRC_SEL(r32,v)                       _BFSET_(r32, 1, 0,v)
    #define   SET16AIO_PRISRC_SEL(r16,v)                       _BFSET_(r16, 1, 0,v)
    #define   SET32AIO_PRISRC_L0DATAMAP(r32,v)                 _BFSET_(r32, 3, 2,v)
    #define   SET16AIO_PRISRC_L0DATAMAP(r16,v)                 _BFSET_(r16, 3, 2,v)
    #define   SET32AIO_PRISRC_L1DATAMAP(r32,v)                 _BFSET_(r32, 5, 4,v)
    #define   SET16AIO_PRISRC_L1DATAMAP(r16,v)                 _BFSET_(r16, 5, 4,v)
    #define   SET32AIO_PRISRC_L2DATAMAP(r32,v)                 _BFSET_(r32, 7, 6,v)
    #define   SET16AIO_PRISRC_L2DATAMAP(r16,v)                 _BFSET_(r16, 7, 6,v)
    #define   SET32AIO_PRISRC_L3DATAMAP(r32,v)                 _BFSET_(r32, 9, 8,v)
    #define   SET16AIO_PRISRC_L3DATAMAP(r16,v)                 _BFSET_(r16, 9, 8,v)
    #define   SET32AIO_PRISRC_SLAVEMODE(r32,v)                 _BFSET_(r32,10,10,v)
    #define   SET16AIO_PRISRC_SLAVEMODE(r16,v)                 _BFSET_(r16,10,10,v)
    #define     w32AIO_PRISRC                                  {\
            UNSG32 uPRISRC_SEL                                 :  2;\
            UNSG32 uPRISRC_L0DATAMAP                           :  2;\
            UNSG32 uPRISRC_L1DATAMAP                           :  2;\
            UNSG32 uPRISRC_L2DATAMAP                           :  2;\
            UNSG32 uPRISRC_L3DATAMAP                           :  2;\
            UNSG32 uPRISRC_SLAVEMODE                           :  1;\
            UNSG32 RSVDx168_b11                                : 21;\
          }
    union { UNSG32 u32AIO_PRISRC;
            struct w32AIO_PRISRC;
          };
    #define   SET32AIO_SECSRC_SEL(r32,v)                       _BFSET_(r32, 1, 0,v)
    #define   SET16AIO_SECSRC_SEL(r16,v)                       _BFSET_(r16, 1, 0,v)
    #define   SET32AIO_SECSRC_L0DATAMAP(r32,v)                 _BFSET_(r32, 3, 2,v)
    #define   SET16AIO_SECSRC_L0DATAMAP(r16,v)                 _BFSET_(r16, 3, 2,v)
    #define   SET32AIO_SECSRC_L1DATAMAP(r32,v)                 _BFSET_(r32, 5, 4,v)
    #define   SET16AIO_SECSRC_L1DATAMAP(r16,v)                 _BFSET_(r16, 5, 4,v)
    #define   SET32AIO_SECSRC_L2DATAMAP(r32,v)                 _BFSET_(r32, 7, 6,v)
    #define   SET16AIO_SECSRC_L2DATAMAP(r16,v)                 _BFSET_(r16, 7, 6,v)
    #define   SET32AIO_SECSRC_L3DATAMAP(r32,v)                 _BFSET_(r32, 9, 8,v)
    #define   SET16AIO_SECSRC_L3DATAMAP(r16,v)                 _BFSET_(r16, 9, 8,v)
    #define   SET32AIO_SECSRC_SLAVEMODE(r32,v)                 _BFSET_(r32,10,10,v)
    #define   SET16AIO_SECSRC_SLAVEMODE(r16,v)                 _BFSET_(r16,10,10,v)
    #define     w32AIO_SECSRC                                  {\
            UNSG32 uSECSRC_SEL                                 :  2;\
            UNSG32 uSECSRC_L0DATAMAP                           :  2;\
            UNSG32 uSECSRC_L1DATAMAP                           :  2;\
            UNSG32 uSECSRC_L2DATAMAP                           :  2;\
            UNSG32 uSECSRC_L3DATAMAP                           :  2;\
            UNSG32 uSECSRC_SLAVEMODE                           :  1;\
            UNSG32 RSVDx16C_b11                                : 21;\
          }
    union { UNSG32 u32AIO_SECSRC;
            struct w32AIO_SECSRC;
          };
    #define   SET32AIO_PDM_MIC_SEL_CTRL(r32,v)                 _BFSET_(r32, 3, 0,v)
    #define   SET16AIO_PDM_MIC_SEL_CTRL(r16,v)                 _BFSET_(r16, 3, 0,v)
    #define     w32AIO_PDM_MIC_SEL                             {\
            UNSG32 uPDM_MIC_SEL_CTRL                           :  4;\
            UNSG32 RSVDx170_b4                                 : 28;\
          }
    union { UNSG32 u32AIO_PDM_MIC_SEL;
            struct w32AIO_PDM_MIC_SEL;
          };
              SIE_ACLK                                         ie_MCLKPRI;
              SIE_ACLK                                         ie_MCLKSEC;
              SIE_ACLK                                         ie_MCLKPDM;
              SIE_ACLK                                         ie_MCLKMIC1;
              SIE_ACLK                                         ie_MCLKMIC2;
    #define   SET32AIO_SW_RST_SPFRX(r32,v)                     _BFSET_(r32, 0, 0,v)
    #define   SET16AIO_SW_RST_SPFRX(r16,v)                     _BFSET_(r16, 0, 0,v)
    #define   SET32AIO_SW_RST_REFCLK(r32,v)                    _BFSET_(r32, 1, 1,v)
    #define   SET16AIO_SW_RST_REFCLK(r16,v)                    _BFSET_(r16, 1, 1,v)
    #define   SET32AIO_SW_RST_MIC3(r32,v)                      _BFSET_(r32, 2, 2,v)
    #define   SET16AIO_SW_RST_MIC3(r16,v)                      _BFSET_(r16, 2, 2,v)
    #define   SET32AIO_SW_RST_MIC4(r32,v)                      _BFSET_(r32, 3, 3,v)
    #define   SET16AIO_SW_RST_MIC4(r16,v)                      _BFSET_(r16, 3, 3,v)
    #define   SET32AIO_SW_RST_MIC5(r32,v)                      _BFSET_(r32, 4, 4,v)
    #define   SET16AIO_SW_RST_MIC5(r16,v)                      _BFSET_(r16, 4, 4,v)
    #define     w32AIO_SW_RST                                  {\
            UNSG32 uSW_RST_SPFRX                               :  1;\
            UNSG32 uSW_RST_REFCLK                              :  1;\
            UNSG32 uSW_RST_MIC3                                :  1;\
            UNSG32 uSW_RST_MIC4                                :  1;\
            UNSG32 uSW_RST_MIC5                                :  1;\
            UNSG32 RSVDx188_b5                                 : 27;\
          }
    union { UNSG32 u32AIO_SW_RST;
            struct w32AIO_SW_RST;
          };
    #define   SET32AIO_CLK_GATE_EN_MIC3(r32,v)                 _BFSET_(r32, 0, 0,v)
    #define   SET16AIO_CLK_GATE_EN_MIC3(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define   SET32AIO_CLK_GATE_EN_MIC4(r32,v)                 _BFSET_(r32, 1, 1,v)
    #define   SET16AIO_CLK_GATE_EN_MIC4(r16,v)                 _BFSET_(r16, 1, 1,v)
    #define   SET32AIO_CLK_GATE_EN_MIC5(r32,v)                 _BFSET_(r32, 2, 2,v)
    #define   SET16AIO_CLK_GATE_EN_MIC5(r16,v)                 _BFSET_(r16, 2, 2,v)
    #define   SET32AIO_CLK_GATE_EN_GBL(r32,v)                  _BFSET_(r32, 3, 3,v)
    #define   SET16AIO_CLK_GATE_EN_GBL(r16,v)                  _BFSET_(r16, 3, 3,v)
    #define     w32AIO_CLK_GATE_EN                             {\
            UNSG32 uCLK_GATE_EN_MIC3                           :  1;\
            UNSG32 uCLK_GATE_EN_MIC4                           :  1;\
            UNSG32 uCLK_GATE_EN_MIC5                           :  1;\
            UNSG32 uCLK_GATE_EN_GBL                            :  1;\
            UNSG32 RSVDx18C_b4                                 : 28;\
          }
    union { UNSG32 u32AIO_CLK_GATE_EN;
            struct w32AIO_CLK_GATE_EN;
          };
    #define   SET32AIO_EARC_ARC_SEL(r32,v)                     _BFSET_(r32, 0, 0,v)
    #define   SET16AIO_EARC_ARC_SEL(r16,v)                     _BFSET_(r16, 0, 0,v)
    #define     w32AIO_EARC_ARC                                {\
            UNSG32 uEARC_ARC_SEL                               :  1;\
            UNSG32 RSVDx190_b1                                 : 31;\
          }
    union { UNSG32 u32AIO_EARC_ARC;
            struct w32AIO_EARC_ARC;
          };
    #define   SET32AIO_SAMP_CTRL_EN_I2STX1(r32,v)              _BFSET_(r32, 0, 0,v)
    #define   SET16AIO_SAMP_CTRL_EN_I2STX1(r16,v)              _BFSET_(r16, 0, 0,v)
    #define   SET32AIO_SAMP_CTRL_EN_I2STX2(r32,v)              _BFSET_(r32, 1, 1,v)
    #define   SET16AIO_SAMP_CTRL_EN_I2STX2(r16,v)              _BFSET_(r16, 1, 1,v)
    #define   SET32AIO_SAMP_CTRL_EN_SPDIFTX(r32,v)             _BFSET_(r32, 2, 2,v)
    #define   SET16AIO_SAMP_CTRL_EN_SPDIFTX(r16,v)             _BFSET_(r16, 2, 2,v)
    #define   SET32AIO_SAMP_CTRL_EN_SPDIFTX1(r32,v)            _BFSET_(r32, 3, 3,v)
    #define   SET16AIO_SAMP_CTRL_EN_SPDIFTX1(r16,v)            _BFSET_(r16, 3, 3,v)
    #define   SET32AIO_SAMP_CTRL_EN_HDMI(r32,v)                _BFSET_(r32, 4, 4,v)
    #define   SET16AIO_SAMP_CTRL_EN_HDMI(r16,v)                _BFSET_(r16, 4, 4,v)
    #define   SET32AIO_SAMP_CTRL_EN_HDMIARCTX(r32,v)           _BFSET_(r32, 5, 5,v)
    #define   SET16AIO_SAMP_CTRL_EN_HDMIARCTX(r16,v)           _BFSET_(r16, 5, 5,v)
    #define   SET32AIO_SAMP_CTRL_EN_SPDIFRX(r32,v)             _BFSET_(r32, 6, 6,v)
    #define   SET16AIO_SAMP_CTRL_EN_SPDIFRX(r16,v)             _BFSET_(r16, 6, 6,v)
    #define   SET32AIO_SAMP_CTRL_EN_I2SRX1(r32,v)              _BFSET_(r32, 7, 7,v)
    #define   SET16AIO_SAMP_CTRL_EN_I2SRX1(r16,v)              _BFSET_(r16, 7, 7,v)
    #define   SET32AIO_SAMP_CTRL_EN_I2SRX2(r32,v)              _BFSET_(r32, 8, 8,v)
    #define   SET16AIO_SAMP_CTRL_EN_I2SRX2(r16,v)              _BFSET_(r16, 8, 8,v)
    #define   SET32AIO_SAMP_CTRL_EN_I2SRX3(r32,v)              _BFSET_(r32, 9, 9,v)
    #define   SET16AIO_SAMP_CTRL_EN_I2SRX3(r16,v)              _BFSET_(r16, 9, 9,v)
    #define   SET32AIO_SAMP_CTRL_EN_I2SRX4(r32,v)              _BFSET_(r32,10,10,v)
    #define   SET16AIO_SAMP_CTRL_EN_I2SRX4(r16,v)              _BFSET_(r16,10,10,v)
    #define   SET32AIO_SAMP_CTRL_EN_I2SRX5(r32,v)              _BFSET_(r32,11,11,v)
    #define   SET16AIO_SAMP_CTRL_EN_I2SRX5(r16,v)              _BFSET_(r16,11,11,v)
    #define   SET32AIO_SAMP_CTRL_EN_AUDTIMER(r32,v)            _BFSET_(r32,12,12,v)
    #define   SET16AIO_SAMP_CTRL_EN_AUDTIMER(r16,v)            _BFSET_(r16,12,12,v)
    #define   SET32AIO_SAMP_CTRL_EN_PDMRX1(r32,v)              _BFSET_(r32,13,13,v)
    #define   SET16AIO_SAMP_CTRL_EN_PDMRX1(r16,v)              _BFSET_(r16,13,13,v)
    #define     w32AIO_SAMP_CTRL                               {\
            UNSG32 uSAMP_CTRL_EN_I2STX1                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2STX2                        :  1;\
            UNSG32 uSAMP_CTRL_EN_SPDIFTX                       :  1;\
            UNSG32 uSAMP_CTRL_EN_SPDIFTX1                      :  1;\
            UNSG32 uSAMP_CTRL_EN_HDMI                          :  1;\
            UNSG32 uSAMP_CTRL_EN_HDMIARCTX                     :  1;\
            UNSG32 uSAMP_CTRL_EN_SPDIFRX                       :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX1                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX2                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX3                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX4                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX5                        :  1;\
            UNSG32 uSAMP_CTRL_EN_AUDTIMER                      :  1;\
            UNSG32 uSAMP_CTRL_EN_PDMRX1                        :  1;\
            UNSG32 RSVDx194_b14                                : 18;\
          }
    union { UNSG32 u32AIO_SAMP_CTRL;
            struct w32AIO_SAMP_CTRL;
          };
    #define   SET32AIO_SAMPINFO_REQ_I2STX1(r32,v)              _BFSET_(r32, 0, 0,v)
    #define   SET16AIO_SAMPINFO_REQ_I2STX1(r16,v)              _BFSET_(r16, 0, 0,v)
    #define   SET32AIO_SAMPINFO_REQ_I2STX2(r32,v)              _BFSET_(r32, 1, 1,v)
    #define   SET16AIO_SAMPINFO_REQ_I2STX2(r16,v)              _BFSET_(r16, 1, 1,v)
    #define   SET32AIO_SAMPINFO_REQ_HDMITX(r32,v)              _BFSET_(r32, 2, 2,v)
    #define   SET16AIO_SAMPINFO_REQ_HDMITX(r16,v)              _BFSET_(r16, 2, 2,v)
    #define   SET32AIO_SAMPINFO_REQ_HDMIARCTX(r32,v)           _BFSET_(r32, 3, 3,v)
    #define   SET16AIO_SAMPINFO_REQ_HDMIARCTX(r16,v)           _BFSET_(r16, 3, 3,v)
    #define   SET32AIO_SAMPINFO_REQ_SPDIFTX(r32,v)             _BFSET_(r32, 4, 4,v)
    #define   SET16AIO_SAMPINFO_REQ_SPDIFTX(r16,v)             _BFSET_(r16, 4, 4,v)
    #define   SET32AIO_SAMPINFO_REQ_SPDIFTX1(r32,v)            _BFSET_(r32, 5, 5,v)
    #define   SET16AIO_SAMPINFO_REQ_SPDIFTX1(r16,v)            _BFSET_(r16, 5, 5,v)
    #define   SET32AIO_SAMPINFO_REQ_SPDIFRX(r32,v)             _BFSET_(r32, 6, 6,v)
    #define   SET16AIO_SAMPINFO_REQ_SPDIFRX(r16,v)             _BFSET_(r16, 6, 6,v)
    #define   SET32AIO_SAMPINFO_REQ_I2SRX1(r32,v)              _BFSET_(r32, 7, 7,v)
    #define   SET16AIO_SAMPINFO_REQ_I2SRX1(r16,v)              _BFSET_(r16, 7, 7,v)
    #define   SET32AIO_SAMPINFO_REQ_I2SRX2(r32,v)              _BFSET_(r32, 8, 8,v)
    #define   SET16AIO_SAMPINFO_REQ_I2SRX2(r16,v)              _BFSET_(r16, 8, 8,v)
    #define   SET32AIO_SAMPINFO_REQ_I2SRX3(r32,v)              _BFSET_(r32, 9, 9,v)
    #define   SET16AIO_SAMPINFO_REQ_I2SRX3(r16,v)              _BFSET_(r16, 9, 9,v)
    #define   SET32AIO_SAMPINFO_REQ_I2SRX4(r32,v)              _BFSET_(r32,10,10,v)
    #define   SET16AIO_SAMPINFO_REQ_I2SRX4(r16,v)              _BFSET_(r16,10,10,v)
    #define   SET32AIO_SAMPINFO_REQ_I2SRX5(r32,v)              _BFSET_(r32,11,11,v)
    #define   SET16AIO_SAMPINFO_REQ_I2SRX5(r16,v)              _BFSET_(r16,11,11,v)
    #define   SET32AIO_SAMPINFO_REQ_PDMRX1(r32,v)              _BFSET_(r32,12,12,v)
    #define   SET16AIO_SAMPINFO_REQ_PDMRX1(r16,v)              _BFSET_(r16,12,12,v)
    #define     w32AIO_SAMPINFO_REQ                            {\
            UNSG32 uSAMPINFO_REQ_I2STX1                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2STX2                        :  1;\
            UNSG32 uSAMPINFO_REQ_HDMITX                        :  1;\
            UNSG32 uSAMPINFO_REQ_HDMIARCTX                     :  1;\
            UNSG32 uSAMPINFO_REQ_SPDIFTX                       :  1;\
            UNSG32 uSAMPINFO_REQ_SPDIFTX1                      :  1;\
            UNSG32 uSAMPINFO_REQ_SPDIFRX                       :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX1                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX2                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX3                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX4                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX5                        :  1;\
            UNSG32 uSAMPINFO_REQ_PDMRX1                        :  1;\
            UNSG32 RSVDx198_b13                                : 19;\
          }
    union { UNSG32 u32AIO_SAMPINFO_REQ;
            struct w32AIO_SAMPINFO_REQ;
          };
    #define   SET32AIO_SCR_I2STX1(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR                                     {\
            UNSG32 uSCR_I2STX1                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR;
            struct w32AIO_SCR;
          };
    #define   SET32AIO_SCR_I2STX2(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR1                                    {\
            UNSG32 uSCR_I2STX2                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR1;
            struct w32AIO_SCR1;
          };
    #define   SET32AIO_SCR_HDMITX(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR2                                    {\
            UNSG32 uSCR_HDMITX                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR2;
            struct w32AIO_SCR2;
          };
    #define   SET32AIO_SCR_HDMIARCTX(r32,v)                    _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR3                                    {\
            UNSG32 uSCR_HDMIARCTX                              : 32;\
          }
    union { UNSG32 u32AIO_SCR3;
            struct w32AIO_SCR3;
          };
    #define   SET32AIO_SCR_SPDIFTX(r32,v)                      _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR4                                    {\
            UNSG32 uSCR_SPDIFTX                                : 32;\
          }
    union { UNSG32 u32AIO_SCR4;
            struct w32AIO_SCR4;
          };
    #define   SET32AIO_SCR_SPDIFTX1(r32,v)                     _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR5                                    {\
            UNSG32 uSCR_SPDIFTX1                               : 32;\
          }
    union { UNSG32 u32AIO_SCR5;
            struct w32AIO_SCR5;
          };
    #define   SET32AIO_SCR_SPDIFRX(r32,v)                      _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR6                                    {\
            UNSG32 uSCR_SPDIFRX                                : 32;\
          }
    union { UNSG32 u32AIO_SCR6;
            struct w32AIO_SCR6;
          };
    #define   SET32AIO_SCR_I2SRX1(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR7                                    {\
            UNSG32 uSCR_I2SRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR7;
            struct w32AIO_SCR7;
          };
    #define   SET32AIO_SCR_I2SRX2(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR8                                    {\
            UNSG32 uSCR_I2SRX2                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR8;
            struct w32AIO_SCR8;
          };
    #define   SET32AIO_SCR_I2SRX3(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR9                                    {\
            UNSG32 uSCR_I2SRX3                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR9;
            struct w32AIO_SCR9;
          };
    #define   SET32AIO_SCR_I2SRX4(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR10                                   {\
            UNSG32 uSCR_I2SRX4                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR10;
            struct w32AIO_SCR10;
          };
    #define   SET32AIO_SCR_I2SRX5(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR11                                   {\
            UNSG32 uSCR_I2SRX5                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR11;
            struct w32AIO_SCR11;
          };
    #define   SET32AIO_SCR_PDMRX1(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_SCR12                                   {\
            UNSG32 uSCR_PDMRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR12;
            struct w32AIO_SCR12;
          };
    #define   SET32AIO_STR_I2STX1(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR                                     {\
            UNSG32 uSTR_I2STX1                                 : 32;\
          }
    union { UNSG32 u32AIO_STR;
            struct w32AIO_STR;
          };
    #define   SET32AIO_STR_I2STX2(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR1                                    {\
            UNSG32 uSTR_I2STX2                                 : 32;\
          }
    union { UNSG32 u32AIO_STR1;
            struct w32AIO_STR1;
          };
    #define   SET32AIO_STR_SPDIFTX(r32,v)                      _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR2                                    {\
            UNSG32 uSTR_SPDIFTX                                : 32;\
          }
    union { UNSG32 u32AIO_STR2;
            struct w32AIO_STR2;
          };
    #define   SET32AIO_STR_SPDIFTX1(r32,v)                     _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR3                                    {\
            UNSG32 uSTR_SPDIFTX1                               : 32;\
          }
    union { UNSG32 u32AIO_STR3;
            struct w32AIO_STR3;
          };
    #define   SET32AIO_STR_HDMITX(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR4                                    {\
            UNSG32 uSTR_HDMITX                                 : 32;\
          }
    union { UNSG32 u32AIO_STR4;
            struct w32AIO_STR4;
          };
    #define   SET32AIO_STR_HDMIARCTX(r32,v)                    _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR5                                    {\
            UNSG32 uSTR_HDMIARCTX                              : 32;\
          }
    union { UNSG32 u32AIO_STR5;
            struct w32AIO_STR5;
          };
    #define   SET32AIO_STR_SPDIFRX(r32,v)                      _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR6                                    {\
            UNSG32 uSTR_SPDIFRX                                : 32;\
          }
    union { UNSG32 u32AIO_STR6;
            struct w32AIO_STR6;
          };
    #define   SET32AIO_STR_I2SRX1(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR7                                    {\
            UNSG32 uSTR_I2SRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_STR7;
            struct w32AIO_STR7;
          };
    #define   SET32AIO_STR_I2SRX2(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR8                                    {\
            UNSG32 uSTR_I2SRX2                                 : 32;\
          }
    union { UNSG32 u32AIO_STR8;
            struct w32AIO_STR8;
          };
    #define   SET32AIO_STR_I2SRX3(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR9                                    {\
            UNSG32 uSTR_I2SRX3                                 : 32;\
          }
    union { UNSG32 u32AIO_STR9;
            struct w32AIO_STR9;
          };
    #define   SET32AIO_STR_I2SRX4(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR10                                   {\
            UNSG32 uSTR_I2SRX4                                 : 32;\
          }
    union { UNSG32 u32AIO_STR10;
            struct w32AIO_STR10;
          };
    #define   SET32AIO_STR_I2SRX5(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR11                                   {\
            UNSG32 uSTR_I2SRX5                                 : 32;\
          }
    union { UNSG32 u32AIO_STR11;
            struct w32AIO_STR11;
          };
    #define   SET32AIO_STR_PDMRX1(r32,v)                       _BFSET_(r32,31, 0,v)
    #define     w32AIO_STR12                                   {\
            UNSG32 uSTR_PDMRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_STR12;
            struct w32AIO_STR12;
          };
    #define   SET32AIO_ATR_TIMER(r32,v)                        _BFSET_(r32,31, 0,v)
    #define     w32AIO_ATR                                     {\
            UNSG32 uATR_TIMER                                  : 32;\
          }
    union { UNSG32 u32AIO_ATR;
            struct w32AIO_ATR;
          };
    #define   SET32AIO_XFEED_I2S1_LRCKIO_MODE(r32,v)           _BFSET_(r32, 1, 0,v)
    #define   SET16AIO_XFEED_I2S1_LRCKIO_MODE(r16,v)           _BFSET_(r16, 1, 0,v)
    #define   SET32AIO_XFEED_I2S1_BCLKIO_MODE(r32,v)           _BFSET_(r32, 3, 2,v)
    #define   SET16AIO_XFEED_I2S1_BCLKIO_MODE(r16,v)           _BFSET_(r16, 3, 2,v)
    #define   SET32AIO_XFEED_I2S2_LRCKIO_MODE(r32,v)           _BFSET_(r32, 5, 4,v)
    #define   SET16AIO_XFEED_I2S2_LRCKIO_MODE(r16,v)           _BFSET_(r16, 5, 4,v)
    #define   SET32AIO_XFEED_I2S2_BCLKIO_MODE(r32,v)           _BFSET_(r32, 7, 6,v)
    #define   SET16AIO_XFEED_I2S2_BCLKIO_MODE(r16,v)           _BFSET_(r16, 7, 6,v)
    #define   SET32AIO_XFEED_I2S3_LRCKIO_MODE(r32,v)           _BFSET_(r32, 8, 8,v)
    #define   SET16AIO_XFEED_I2S3_LRCKIO_MODE(r16,v)           _BFSET_(r16, 8, 8,v)
    #define   SET32AIO_XFEED_I2S3_BCLKIO_MODE(r32,v)           _BFSET_(r32, 9, 9,v)
    #define   SET16AIO_XFEED_I2S3_BCLKIO_MODE(r16,v)           _BFSET_(r16, 9, 9,v)
    #define   SET32AIO_XFEED_PDM_CLK_SEL(r32,v)                _BFSET_(r32,12,10,v)
    #define   SET16AIO_XFEED_PDM_CLK_SEL(r16,v)                _BFSET_(r16,12,10,v)
    #define   SET32AIO_XFEED_PDMC_SEL(r32,v)                   _BFSET_(r32,13,13,v)
    #define   SET16AIO_XFEED_PDMC_SEL(r16,v)                   _BFSET_(r16,13,13,v)
    #define   SET32AIO_XFEED_PDM_SEL(r32,v)                    _BFSET_(r32,17,14,v)
    #define     w32AIO_XFEED                                   {\
            UNSG32 uXFEED_I2S1_LRCKIO_MODE                     :  2;\
            UNSG32 uXFEED_I2S1_BCLKIO_MODE                     :  2;\
            UNSG32 uXFEED_I2S2_LRCKIO_MODE                     :  2;\
            UNSG32 uXFEED_I2S2_BCLKIO_MODE                     :  2;\
            UNSG32 uXFEED_I2S3_LRCKIO_MODE                     :  1;\
            UNSG32 uXFEED_I2S3_BCLKIO_MODE                     :  1;\
            UNSG32 uXFEED_PDM_CLK_SEL                          :  3;\
            UNSG32 uXFEED_PDMC_SEL                             :  1;\
            UNSG32 uXFEED_PDM_SEL                              :  4;\
            UNSG32 RSVDx208_b18                                : 14;\
          }
    union { UNSG32 u32AIO_XFEED;
            struct w32AIO_XFEED;
          };
              SIE_SRAMPWR                                      ie_DMIC_SRAMPWR;
    } SIE_AIO;
    typedef union  T32AIO_IRQENABLE
          { UNSG32 u32;
            struct w32AIO_IRQENABLE;
                 } T32AIO_IRQENABLE;
    typedef union  T32AIO_IRQSTS
          { UNSG32 u32;
            struct w32AIO_IRQSTS;
                 } T32AIO_IRQSTS;
    typedef union  T32AIO_PRISRC
          { UNSG32 u32;
            struct w32AIO_PRISRC;
                 } T32AIO_PRISRC;
    typedef union  T32AIO_SECSRC
          { UNSG32 u32;
            struct w32AIO_SECSRC;
                 } T32AIO_SECSRC;
    typedef union  T32AIO_PDM_MIC_SEL
          { UNSG32 u32;
            struct w32AIO_PDM_MIC_SEL;
                 } T32AIO_PDM_MIC_SEL;
    typedef union  T32AIO_SW_RST
          { UNSG32 u32;
            struct w32AIO_SW_RST;
                 } T32AIO_SW_RST;
    typedef union  T32AIO_CLK_GATE_EN
          { UNSG32 u32;
            struct w32AIO_CLK_GATE_EN;
                 } T32AIO_CLK_GATE_EN;
    typedef union  T32AIO_EARC_ARC
          { UNSG32 u32;
            struct w32AIO_EARC_ARC;
                 } T32AIO_EARC_ARC;
    typedef union  T32AIO_SAMP_CTRL
          { UNSG32 u32;
            struct w32AIO_SAMP_CTRL;
                 } T32AIO_SAMP_CTRL;
    typedef union  T32AIO_SAMPINFO_REQ
          { UNSG32 u32;
            struct w32AIO_SAMPINFO_REQ;
                 } T32AIO_SAMPINFO_REQ;
    typedef union  T32AIO_SCR
          { UNSG32 u32;
            struct w32AIO_SCR;
                 } T32AIO_SCR;
    typedef union  T32AIO_SCR1
          { UNSG32 u32;
            struct w32AIO_SCR1;
                 } T32AIO_SCR1;
    typedef union  T32AIO_SCR2
          { UNSG32 u32;
            struct w32AIO_SCR2;
                 } T32AIO_SCR2;
    typedef union  T32AIO_SCR3
          { UNSG32 u32;
            struct w32AIO_SCR3;
                 } T32AIO_SCR3;
    typedef union  T32AIO_SCR4
          { UNSG32 u32;
            struct w32AIO_SCR4;
                 } T32AIO_SCR4;
    typedef union  T32AIO_SCR5
          { UNSG32 u32;
            struct w32AIO_SCR5;
                 } T32AIO_SCR5;
    typedef union  T32AIO_SCR6
          { UNSG32 u32;
            struct w32AIO_SCR6;
                 } T32AIO_SCR6;
    typedef union  T32AIO_SCR7
          { UNSG32 u32;
            struct w32AIO_SCR7;
                 } T32AIO_SCR7;
    typedef union  T32AIO_SCR8
          { UNSG32 u32;
            struct w32AIO_SCR8;
                 } T32AIO_SCR8;
    typedef union  T32AIO_SCR9
          { UNSG32 u32;
            struct w32AIO_SCR9;
                 } T32AIO_SCR9;
    typedef union  T32AIO_SCR10
          { UNSG32 u32;
            struct w32AIO_SCR10;
                 } T32AIO_SCR10;
    typedef union  T32AIO_SCR11
          { UNSG32 u32;
            struct w32AIO_SCR11;
                 } T32AIO_SCR11;
    typedef union  T32AIO_SCR12
          { UNSG32 u32;
            struct w32AIO_SCR12;
                 } T32AIO_SCR12;
    typedef union  T32AIO_STR
          { UNSG32 u32;
            struct w32AIO_STR;
                 } T32AIO_STR;
    typedef union  T32AIO_STR1
          { UNSG32 u32;
            struct w32AIO_STR1;
                 } T32AIO_STR1;
    typedef union  T32AIO_STR2
          { UNSG32 u32;
            struct w32AIO_STR2;
                 } T32AIO_STR2;
    typedef union  T32AIO_STR3
          { UNSG32 u32;
            struct w32AIO_STR3;
                 } T32AIO_STR3;
    typedef union  T32AIO_STR4
          { UNSG32 u32;
            struct w32AIO_STR4;
                 } T32AIO_STR4;
    typedef union  T32AIO_STR5
          { UNSG32 u32;
            struct w32AIO_STR5;
                 } T32AIO_STR5;
    typedef union  T32AIO_STR6
          { UNSG32 u32;
            struct w32AIO_STR6;
                 } T32AIO_STR6;
    typedef union  T32AIO_STR7
          { UNSG32 u32;
            struct w32AIO_STR7;
                 } T32AIO_STR7;
    typedef union  T32AIO_STR8
          { UNSG32 u32;
            struct w32AIO_STR8;
                 } T32AIO_STR8;
    typedef union  T32AIO_STR9
          { UNSG32 u32;
            struct w32AIO_STR9;
                 } T32AIO_STR9;
    typedef union  T32AIO_STR10
          { UNSG32 u32;
            struct w32AIO_STR10;
                 } T32AIO_STR10;
    typedef union  T32AIO_STR11
          { UNSG32 u32;
            struct w32AIO_STR11;
                 } T32AIO_STR11;
    typedef union  T32AIO_STR12
          { UNSG32 u32;
            struct w32AIO_STR12;
                 } T32AIO_STR12;
    typedef union  T32AIO_ATR
          { UNSG32 u32;
            struct w32AIO_ATR;
                 } T32AIO_ATR;
    typedef union  T32AIO_XFEED
          { UNSG32 u32;
            struct w32AIO_XFEED;
                 } T32AIO_XFEED;
    typedef union  TAIO_IRQENABLE
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQENABLE;
                   };
                 } TAIO_IRQENABLE;
    typedef union  TAIO_IRQSTS
          { UNSG32 u32[1];
            struct {
            struct w32AIO_IRQSTS;
                   };
                 } TAIO_IRQSTS;
    typedef union  TAIO_PRISRC
          { UNSG32 u32[1];
            struct {
            struct w32AIO_PRISRC;
                   };
                 } TAIO_PRISRC;
    typedef union  TAIO_SECSRC
          { UNSG32 u32[1];
            struct {
            struct w32AIO_SECSRC;
                   };
                 } TAIO_SECSRC;
    typedef union  TAIO_PDM_MIC_SEL
          { UNSG32 u32[1];
            struct {
            struct w32AIO_PDM_MIC_SEL;
                   };
                 } TAIO_PDM_MIC_SEL;
    typedef union  TAIO_SW_RST
          { UNSG32 u32[1];
            struct {
            struct w32AIO_SW_RST;
                   };
                 } TAIO_SW_RST;
    typedef union  TAIO_CLK_GATE_EN
          { UNSG32 u32[1];
            struct {
            struct w32AIO_CLK_GATE_EN;
                   };
                 } TAIO_CLK_GATE_EN;
    typedef union  TAIO_EARC_ARC
          { UNSG32 u32[1];
            struct {
            struct w32AIO_EARC_ARC;
                   };
                 } TAIO_EARC_ARC;
    typedef union  TAIO_SAMP_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32AIO_SAMP_CTRL;
                   };
                 } TAIO_SAMP_CTRL;
    typedef union  TAIO_SAMPINFO_REQ
          { UNSG32 u32[1];
            struct {
            struct w32AIO_SAMPINFO_REQ;
                   };
                 } TAIO_SAMPINFO_REQ;
    typedef union  TAIO_SCR
          { UNSG32 u32[13];
            struct {
            struct w32AIO_SCR;
            struct w32AIO_SCR1;
            struct w32AIO_SCR2;
            struct w32AIO_SCR3;
            struct w32AIO_SCR4;
            struct w32AIO_SCR5;
            struct w32AIO_SCR6;
            struct w32AIO_SCR7;
            struct w32AIO_SCR8;
            struct w32AIO_SCR9;
            struct w32AIO_SCR10;
            struct w32AIO_SCR11;
            struct w32AIO_SCR12;
                   };
                 } TAIO_SCR;
    typedef union  TAIO_STR
          { UNSG32 u32[13];
            struct {
            struct w32AIO_STR;
            struct w32AIO_STR1;
            struct w32AIO_STR2;
            struct w32AIO_STR3;
            struct w32AIO_STR4;
            struct w32AIO_STR5;
            struct w32AIO_STR6;
            struct w32AIO_STR7;
            struct w32AIO_STR8;
            struct w32AIO_STR9;
            struct w32AIO_STR10;
            struct w32AIO_STR11;
            struct w32AIO_STR12;
                   };
                 } TAIO_STR;
    typedef union  TAIO_ATR
          { UNSG32 u32[1];
            struct {
            struct w32AIO_ATR;
                   };
                 } TAIO_ATR;
    typedef union  TAIO_XFEED
          { UNSG32 u32[1];
            struct {
            struct w32AIO_XFEED;
                   };
                 } TAIO_XFEED;
     SIGN32 AIO_drvrd(SIE_AIO *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AIO_drvwr(SIE_AIO *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AIO_reset(SIE_AIO *p);
     SIGN32 AIO_cmp  (SIE_AIO *p, SIE_AIO *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AIO_check(p,pie,pfx,hLOG) AIO_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AIO_print(p,    pfx,hLOG) AIO_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
