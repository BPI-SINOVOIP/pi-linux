#ifndef avioGbl_h
#define avioGbl_h (){}
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
#ifndef h_abipll
#define h_abipll (){}
    #define     RA_abipll_ctrlA                                0x0000
    #define       babipll_ctrlA_RESET                          1
    #define       babipll_ctrlA_BYPASS                         1
    #define       babipll_ctrlA_NEWDIV                         1
    #define       babipll_ctrlA_RANGE                          3
    #define     RA_abipll_ctrlB                                0x0004
    #define       babipll_ctrlB_SSMF                           4
    #define       babipll_ctrlB_SSMD                           3
    #define       babipll_ctrlB_SSE_RSVD                       1
    #define       babipll_ctrlB_SSE                            1
    #define       babipll_ctrlB_SSDS                           1
    #define     RA_abipll_ctrlC                                0x0008
    #define       babipll_ctrlC_DIVR                           6
    #define     RA_abipll_ctrlD                                0x000C
    #define       babipll_ctrlD_DIVFI                          9
    #define     RA_abipll_ctrlE                                0x0010
    #define       babipll_ctrlE_DIVFF                          24
    #define     RA_abipll_ctrlF                                0x0014
    #define       babipll_ctrlF_DIVQ                           5
    #define     RA_abipll_ctrlG                                0x0018
    #define       babipll_ctrlG_DIVQF                          3
    #define     RA_abipll_status                               0x001C
    #define       babipll_status_LOCK                          1
    #define       babipll_status_DIVACK                        1
    typedef struct SIE_abipll {
    #define   SET32abipll_ctrlA_RESET(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16abipll_ctrlA_RESET(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define   SET32abipll_ctrlA_BYPASS(r32,v)                  _BFSET_(r32, 1, 1,v)
    #define   SET16abipll_ctrlA_BYPASS(r16,v)                  _BFSET_(r16, 1, 1,v)
    #define   SET32abipll_ctrlA_NEWDIV(r32,v)                  _BFSET_(r32, 2, 2,v)
    #define   SET16abipll_ctrlA_NEWDIV(r16,v)                  _BFSET_(r16, 2, 2,v)
    #define   SET32abipll_ctrlA_RANGE(r32,v)                   _BFSET_(r32, 5, 3,v)
    #define   SET16abipll_ctrlA_RANGE(r16,v)                   _BFSET_(r16, 5, 3,v)
    #define     w32abipll_ctrlA                                {\
            UNSG32 uctrlA_RESET                                :  1;\
            UNSG32 uctrlA_BYPASS                               :  1;\
            UNSG32 uctrlA_NEWDIV                               :  1;\
            UNSG32 uctrlA_RANGE                                :  3;\
            UNSG32 RSVDx0_b6                                   : 26;\
          }
    union { UNSG32 u32abipll_ctrlA;
            struct w32abipll_ctrlA;
          };
    #define   SET32abipll_ctrlB_SSMF(r32,v)                    _BFSET_(r32, 3, 0,v)
    #define   SET16abipll_ctrlB_SSMF(r16,v)                    _BFSET_(r16, 3, 0,v)
    #define   SET32abipll_ctrlB_SSMD(r32,v)                    _BFSET_(r32, 6, 4,v)
    #define   SET16abipll_ctrlB_SSMD(r16,v)                    _BFSET_(r16, 6, 4,v)
    #define   SET32abipll_ctrlB_SSE_RSVD(r32,v)                _BFSET_(r32, 7, 7,v)
    #define   SET16abipll_ctrlB_SSE_RSVD(r16,v)                _BFSET_(r16, 7, 7,v)
    #define   SET32abipll_ctrlB_SSE(r32,v)                     _BFSET_(r32, 8, 8,v)
    #define   SET16abipll_ctrlB_SSE(r16,v)                     _BFSET_(r16, 8, 8,v)
    #define   SET32abipll_ctrlB_SSDS(r32,v)                    _BFSET_(r32, 9, 9,v)
    #define   SET16abipll_ctrlB_SSDS(r16,v)                    _BFSET_(r16, 9, 9,v)
    #define     w32abipll_ctrlB                                {\
            UNSG32 uctrlB_SSMF                                 :  4;\
            UNSG32 uctrlB_SSMD                                 :  3;\
            UNSG32 uctrlB_SSE_RSVD                             :  1;\
            UNSG32 uctrlB_SSE                                  :  1;\
            UNSG32 uctrlB_SSDS                                 :  1;\
            UNSG32 RSVDx4_b10                                  : 22;\
          }
    union { UNSG32 u32abipll_ctrlB;
            struct w32abipll_ctrlB;
          };
    #define   SET32abipll_ctrlC_DIVR(r32,v)                    _BFSET_(r32, 5, 0,v)
    #define   SET16abipll_ctrlC_DIVR(r16,v)                    _BFSET_(r16, 5, 0,v)
    #define     w32abipll_ctrlC                                {\
            UNSG32 uctrlC_DIVR                                 :  6;\
            UNSG32 RSVDx8_b6                                   : 26;\
          }
    union { UNSG32 u32abipll_ctrlC;
            struct w32abipll_ctrlC;
          };
    #define   SET32abipll_ctrlD_DIVFI(r32,v)                   _BFSET_(r32, 8, 0,v)
    #define   SET16abipll_ctrlD_DIVFI(r16,v)                   _BFSET_(r16, 8, 0,v)
    #define     w32abipll_ctrlD                                {\
            UNSG32 uctrlD_DIVFI                                :  9;\
            UNSG32 RSVDxC_b9                                   : 23;\
          }
    union { UNSG32 u32abipll_ctrlD;
            struct w32abipll_ctrlD;
          };
    #define   SET32abipll_ctrlE_DIVFF(r32,v)                   _BFSET_(r32,23, 0,v)
    #define     w32abipll_ctrlE                                {\
            UNSG32 uctrlE_DIVFF                                : 24;\
            UNSG32 RSVDx10_b24                                 :  8;\
          }
    union { UNSG32 u32abipll_ctrlE;
            struct w32abipll_ctrlE;
          };
    #define   SET32abipll_ctrlF_DIVQ(r32,v)                    _BFSET_(r32, 4, 0,v)
    #define   SET16abipll_ctrlF_DIVQ(r16,v)                    _BFSET_(r16, 4, 0,v)
    #define     w32abipll_ctrlF                                {\
            UNSG32 uctrlF_DIVQ                                 :  5;\
            UNSG32 RSVDx14_b5                                  : 27;\
          }
    union { UNSG32 u32abipll_ctrlF;
            struct w32abipll_ctrlF;
          };
    #define   SET32abipll_ctrlG_DIVQF(r32,v)                   _BFSET_(r32, 2, 0,v)
    #define   SET16abipll_ctrlG_DIVQF(r16,v)                   _BFSET_(r16, 2, 0,v)
    #define     w32abipll_ctrlG                                {\
            UNSG32 uctrlG_DIVQF                                :  3;\
            UNSG32 RSVDx18_b3                                  : 29;\
          }
    union { UNSG32 u32abipll_ctrlG;
            struct w32abipll_ctrlG;
          };
    #define   SET32abipll_status_LOCK(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16abipll_status_LOCK(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define   SET32abipll_status_DIVACK(r32,v)                 _BFSET_(r32, 1, 1,v)
    #define   SET16abipll_status_DIVACK(r16,v)                 _BFSET_(r16, 1, 1,v)
    #define     w32abipll_status                               {\
            UNSG32 ustatus_LOCK                                :  1;\
            UNSG32 ustatus_DIVACK                              :  1;\
            UNSG32 RSVDx1C_b2                                  : 30;\
          }
    union { UNSG32 u32abipll_status;
            struct w32abipll_status;
          };
    } SIE_abipll;
    typedef union  T32abipll_ctrlA
          { UNSG32 u32;
            struct w32abipll_ctrlA;
                 } T32abipll_ctrlA;
    typedef union  T32abipll_ctrlB
          { UNSG32 u32;
            struct w32abipll_ctrlB;
                 } T32abipll_ctrlB;
    typedef union  T32abipll_ctrlC
          { UNSG32 u32;
            struct w32abipll_ctrlC;
                 } T32abipll_ctrlC;
    typedef union  T32abipll_ctrlD
          { UNSG32 u32;
            struct w32abipll_ctrlD;
                 } T32abipll_ctrlD;
    typedef union  T32abipll_ctrlE
          { UNSG32 u32;
            struct w32abipll_ctrlE;
                 } T32abipll_ctrlE;
    typedef union  T32abipll_ctrlF
          { UNSG32 u32;
            struct w32abipll_ctrlF;
                 } T32abipll_ctrlF;
    typedef union  T32abipll_ctrlG
          { UNSG32 u32;
            struct w32abipll_ctrlG;
                 } T32abipll_ctrlG;
    typedef union  T32abipll_status
          { UNSG32 u32;
            struct w32abipll_status;
                 } T32abipll_status;
    typedef union  Tabipll_ctrlA
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlA;
                   };
                 } Tabipll_ctrlA;
    typedef union  Tabipll_ctrlB
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlB;
                   };
                 } Tabipll_ctrlB;
    typedef union  Tabipll_ctrlC
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlC;
                   };
                 } Tabipll_ctrlC;
    typedef union  Tabipll_ctrlD
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlD;
                   };
                 } Tabipll_ctrlD;
    typedef union  Tabipll_ctrlE
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlE;
                   };
                 } Tabipll_ctrlE;
    typedef union  Tabipll_ctrlF
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlF;
                   };
                 } Tabipll_ctrlF;
    typedef union  Tabipll_ctrlG
          { UNSG32 u32[1];
            struct {
            struct w32abipll_ctrlG;
                   };
                 } Tabipll_ctrlG;
    typedef union  Tabipll_status
          { UNSG32 u32[1];
            struct {
            struct w32abipll_status;
                   };
                 } Tabipll_status;
     SIGN32 abipll_drvrd(SIE_abipll *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 abipll_drvwr(SIE_abipll *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void abipll_reset(SIE_abipll *p);
     SIGN32 abipll_cmp  (SIE_abipll *p, SIE_abipll *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define abipll_check(p,pie,pfx,hLOG) abipll_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define abipll_print(p,    pfx,hLOG) abipll_cmp(p,0,  pfx,(void*)(hLOG),0,0)
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
#ifndef h_AVIO_debug_ctrl
#define h_AVIO_debug_ctrl (){}
    #define     RA_AVIO_debug_ctrl_Ctrl0                       0x0000
    #define       bAVIO_debug_ctrl_Ctrl0_debug_ctrl0           4
    typedef struct SIE_AVIO_debug_ctrl {
    #define   SET32AVIO_debug_ctrl_Ctrl0_debug_ctrl0(r32,v)    _BFSET_(r32, 3, 0,v)
    #define   SET16AVIO_debug_ctrl_Ctrl0_debug_ctrl0(r16,v)    _BFSET_(r16, 3, 0,v)
    #define     w32AVIO_debug_ctrl_Ctrl0                       {\
            UNSG32 uCtrl0_debug_ctrl0                          :  4;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32AVIO_debug_ctrl_Ctrl0;
            struct w32AVIO_debug_ctrl_Ctrl0;
          };
    } SIE_AVIO_debug_ctrl;
    typedef union  T32AVIO_debug_ctrl_Ctrl0
          { UNSG32 u32;
            struct w32AVIO_debug_ctrl_Ctrl0;
                 } T32AVIO_debug_ctrl_Ctrl0;
    typedef union  TAVIO_debug_ctrl_Ctrl0
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_debug_ctrl_Ctrl0;
                   };
                 } TAVIO_debug_ctrl_Ctrl0;
     SIGN32 AVIO_debug_ctrl_drvrd(SIE_AVIO_debug_ctrl *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AVIO_debug_ctrl_drvwr(SIE_AVIO_debug_ctrl *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AVIO_debug_ctrl_reset(SIE_AVIO_debug_ctrl *p);
     SIGN32 AVIO_debug_ctrl_cmp  (SIE_AVIO_debug_ctrl *p, SIE_AVIO_debug_ctrl *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AVIO_debug_ctrl_check(p,pie,pfx,hLOG) AVIO_debug_ctrl_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AVIO_debug_ctrl_print(p,    pfx,hLOG) AVIO_debug_ctrl_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AVPLL_WRAP
#define h_AVPLL_WRAP (){}
    #define     RA_AVPLL_WRAP_AVPLL_CLK1_CTRL                  0x0000
    #define       bAVPLL_WRAP_AVPLL_CLK1_CTRL_clkSwitch        1
    #define       bAVPLL_WRAP_AVPLL_CLK1_CTRL_clkD3Switch      1
    #define       bAVPLL_WRAP_AVPLL_CLK1_CTRL_clkSel           3
    #define        AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSel_d2                     0x1
    #define        AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSel_d4                     0x2
    #define        AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSel_d6                     0x3
    #define        AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSel_d8                     0x4
    #define        AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSel_d12                    0x5
    #define       bAVPLL_WRAP_AVPLL_CLK1_CTRL_clkEn            1
    #define     RA_AVPLL_WRAP_CTRL0                            0x0004
    #define       bAVPLL_WRAP_CTRL0_clk_div_bypass1            1
    #define       bAVPLL_WRAP_CTRL0_I2S_BCLKI_SEL              2
    #define       bAVPLL_WRAP_CTRL0_clk_sel0                   2
    #define       bAVPLL_WRAP_CTRL0_clk_sel1                   1
    #define       bAVPLL_WRAP_CTRL0_clkOut_sel                 1
    #define     RA_AVPLL_WRAP_APLL                             0x0008
    typedef struct SIE_AVPLL_WRAP {
    #define   SET32AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSwitch(r32,v) _BFSET_(r32, 0, 0,v)
    #define   SET16AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSwitch(r16,v) _BFSET_(r16, 0, 0,v)
    #define   SET32AVPLL_WRAP_AVPLL_CLK1_CTRL_clkD3Switch(r32,v) _BFSET_(r32, 1, 1,v)
    #define   SET16AVPLL_WRAP_AVPLL_CLK1_CTRL_clkD3Switch(r16,v) _BFSET_(r16, 1, 1,v)
    #define   SET32AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSel(r32,v)    _BFSET_(r32, 4, 2,v)
    #define   SET16AVPLL_WRAP_AVPLL_CLK1_CTRL_clkSel(r16,v)    _BFSET_(r16, 4, 2,v)
    #define   SET32AVPLL_WRAP_AVPLL_CLK1_CTRL_clkEn(r32,v)     _BFSET_(r32, 5, 5,v)
    #define   SET16AVPLL_WRAP_AVPLL_CLK1_CTRL_clkEn(r16,v)     _BFSET_(r16, 5, 5,v)
    #define     w32AVPLL_WRAP_AVPLL_CLK1_CTRL                  {\
            UNSG32 uAVPLL_CLK1_CTRL_clkSwitch                  :  1;\
            UNSG32 uAVPLL_CLK1_CTRL_clkD3Switch                :  1;\
            UNSG32 uAVPLL_CLK1_CTRL_clkSel                     :  3;\
            UNSG32 uAVPLL_CLK1_CTRL_clkEn                      :  1;\
            UNSG32 RSVDx0_b6                                   : 26;\
          }
    union { UNSG32 u32AVPLL_WRAP_AVPLL_CLK1_CTRL;
            struct w32AVPLL_WRAP_AVPLL_CLK1_CTRL;
          };
    #define   SET32AVPLL_WRAP_CTRL0_clk_div_bypass1(r32,v)     _BFSET_(r32, 0, 0,v)
    #define   SET16AVPLL_WRAP_CTRL0_clk_div_bypass1(r16,v)     _BFSET_(r16, 0, 0,v)
    #define   SET32AVPLL_WRAP_CTRL0_I2S_BCLKI_SEL(r32,v)       _BFSET_(r32, 2, 1,v)
    #define   SET16AVPLL_WRAP_CTRL0_I2S_BCLKI_SEL(r16,v)       _BFSET_(r16, 2, 1,v)
    #define   SET32AVPLL_WRAP_CTRL0_clk_sel0(r32,v)            _BFSET_(r32, 4, 3,v)
    #define   SET16AVPLL_WRAP_CTRL0_clk_sel0(r16,v)            _BFSET_(r16, 4, 3,v)
    #define   SET32AVPLL_WRAP_CTRL0_clk_sel1(r32,v)            _BFSET_(r32, 5, 5,v)
    #define   SET16AVPLL_WRAP_CTRL0_clk_sel1(r16,v)            _BFSET_(r16, 5, 5,v)
    #define   SET32AVPLL_WRAP_CTRL0_clkOut_sel(r32,v)          _BFSET_(r32, 6, 6,v)
    #define   SET16AVPLL_WRAP_CTRL0_clkOut_sel(r16,v)          _BFSET_(r16, 6, 6,v)
    #define     w32AVPLL_WRAP_CTRL0                            {\
            UNSG32 uCTRL0_clk_div_bypass1                      :  1;\
            UNSG32 uCTRL0_I2S_BCLKI_SEL                        :  2;\
            UNSG32 uCTRL0_clk_sel0                             :  2;\
            UNSG32 uCTRL0_clk_sel1                             :  1;\
            UNSG32 uCTRL0_clkOut_sel                           :  1;\
            UNSG32 RSVDx4_b7                                   : 25;\
          }
    union { UNSG32 u32AVPLL_WRAP_CTRL0;
            struct w32AVPLL_WRAP_CTRL0;
          };
              SIE_abipll                                       ie_APLL;
    } SIE_AVPLL_WRAP;
    typedef union  T32AVPLL_WRAP_AVPLL_CLK1_CTRL
          { UNSG32 u32;
            struct w32AVPLL_WRAP_AVPLL_CLK1_CTRL;
                 } T32AVPLL_WRAP_AVPLL_CLK1_CTRL;
    typedef union  T32AVPLL_WRAP_CTRL0
          { UNSG32 u32;
            struct w32AVPLL_WRAP_CTRL0;
                 } T32AVPLL_WRAP_CTRL0;
    typedef union  TAVPLL_WRAP_AVPLL_CLK1_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32AVPLL_WRAP_AVPLL_CLK1_CTRL;
                   };
                 } TAVPLL_WRAP_AVPLL_CLK1_CTRL;
    typedef union  TAVPLL_WRAP_CTRL0
          { UNSG32 u32[1];
            struct {
            struct w32AVPLL_WRAP_CTRL0;
                   };
                 } TAVPLL_WRAP_CTRL0;
     SIGN32 AVPLL_WRAP_drvrd(SIE_AVPLL_WRAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AVPLL_WRAP_drvwr(SIE_AVPLL_WRAP *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AVPLL_WRAP_reset(SIE_AVPLL_WRAP *p);
     SIGN32 AVPLL_WRAP_cmp  (SIE_AVPLL_WRAP *p, SIE_AVPLL_WRAP *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AVPLL_WRAP_check(p,pie,pfx,hLOG) AVPLL_WRAP_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AVPLL_WRAP_print(p,    pfx,hLOG) AVPLL_WRAP_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_avioGbl
#define h_avioGbl (){}
    #define     RA_avioGbl_AVPLL0_WRAP                         0x0000
    #define     RA_avioGbl_AVIO_debug_ctrl                     0x0028
    #define     RA_avioGbl_AVPLLA_CLK_EN                       0x002C
    #define       bavioGbl_AVPLLA_CLK_EN_ctrl                  6
    #define       bavioGbl_AVPLLA_CLK_EN_dbg_mux_sel           1
    #define     RA_avioGbl_SWPDWN_CTRL                         0x0030
    #define       bavioGbl_SWPDWN_CTRL_AVPLL0_PD               1
    #define       bavioGbl_SWPDWN_CTRL_AVPLL1_PD               1
    #define     RA_avioGbl_RWTC_31to0                          0x0034
    #define       bavioGbl_RWTC_31to0_value                    32
    #define     RA_avioGbl_RWTC_57to32                         0x0038
    #define       bavioGbl_RWTC_57to32_value                   26
    #define     RA_avioGbl_CTRL                                0x003C
    #define       bavioGbl_CTRL_AIODHUB_dyCG_en                1
    #define       bavioGbl_CTRL_AIODHUB_swCG_en                1
    #define       bavioGbl_CTRL_AIODHUB_CG_en                  1
    #define       bavioGbl_CTRL_INTR_EN                        4
    #define     RA_avioGbl_CTRL0                               0x0040
    #define       bavioGbl_CTRL0_I2S1_MCLK_OEN                 1
    #define       bavioGbl_CTRL0_I2S2_MCLK_OEN                 1
    #define       bavioGbl_CTRL0_I2S3_BCLK_OEN                 1
    #define       bavioGbl_CTRL0_I2S3_LRCLK_OEN                1
    #define       bavioGbl_CTRL0_PDM_CLK_OEN                   1
    #define       bavioGbl_CTRL0_dummy                         4
    #define     RA_avioGbl_AIO64DHUB_SRAMPWR                   0x0044
    #define     RA_avioGbl_SRAMRWTC                            0x0048
    typedef struct SIE_avioGbl {
              SIE_AVPLL_WRAP                                   ie_AVPLL0_WRAP;
              SIE_AVIO_debug_ctrl                              ie_AVIO_debug_ctrl;
    #define   SET32avioGbl_AVPLLA_CLK_EN_ctrl(r32,v)           _BFSET_(r32, 5, 0,v)
    #define   SET16avioGbl_AVPLLA_CLK_EN_ctrl(r16,v)           _BFSET_(r16, 5, 0,v)
    #define   SET32avioGbl_AVPLLA_CLK_EN_dbg_mux_sel(r32,v)    _BFSET_(r32, 6, 6,v)
    #define   SET16avioGbl_AVPLLA_CLK_EN_dbg_mux_sel(r16,v)    _BFSET_(r16, 6, 6,v)
    #define     w32avioGbl_AVPLLA_CLK_EN                       {\
            UNSG32 uAVPLLA_CLK_EN_ctrl                         :  6;\
            UNSG32 uAVPLLA_CLK_EN_dbg_mux_sel                  :  1;\
            UNSG32 RSVDx2C_b7                                  : 25;\
          }
    union { UNSG32 u32avioGbl_AVPLLA_CLK_EN;
            struct w32avioGbl_AVPLLA_CLK_EN;
          };
    #define   SET32avioGbl_SWPDWN_CTRL_AVPLL0_PD(r32,v)        _BFSET_(r32, 0, 0,v)
    #define   SET16avioGbl_SWPDWN_CTRL_AVPLL0_PD(r16,v)        _BFSET_(r16, 0, 0,v)
    #define   SET32avioGbl_SWPDWN_CTRL_AVPLL1_PD(r32,v)        _BFSET_(r32, 1, 1,v)
    #define   SET16avioGbl_SWPDWN_CTRL_AVPLL1_PD(r16,v)        _BFSET_(r16, 1, 1,v)
    #define     w32avioGbl_SWPDWN_CTRL                         {\
            UNSG32 uSWPDWN_CTRL_AVPLL0_PD                      :  1;\
            UNSG32 uSWPDWN_CTRL_AVPLL1_PD                      :  1;\
            UNSG32 RSVDx30_b2                                  : 30;\
          }
    union { UNSG32 u32avioGbl_SWPDWN_CTRL;
            struct w32avioGbl_SWPDWN_CTRL;
          };
    #define   SET32avioGbl_RWTC_31to0_value(r32,v)             _BFSET_(r32,31, 0,v)
    #define     w32avioGbl_RWTC_31to0                          {\
            UNSG32 uRWTC_31to0_value                           : 32;\
          }
    union { UNSG32 u32avioGbl_RWTC_31to0;
            struct w32avioGbl_RWTC_31to0;
          };
    #define   SET32avioGbl_RWTC_57to32_value(r32,v)            _BFSET_(r32,25, 0,v)
    #define     w32avioGbl_RWTC_57to32                         {\
            UNSG32 uRWTC_57to32_value                          : 26;\
            UNSG32 RSVDx38_b26                                 :  6;\
          }
    union { UNSG32 u32avioGbl_RWTC_57to32;
            struct w32avioGbl_RWTC_57to32;
          };
    #define   SET32avioGbl_CTRL_AIODHUB_dyCG_en(r32,v)         _BFSET_(r32, 0, 0,v)
    #define   SET16avioGbl_CTRL_AIODHUB_dyCG_en(r16,v)         _BFSET_(r16, 0, 0,v)
    #define   SET32avioGbl_CTRL_AIODHUB_swCG_en(r32,v)         _BFSET_(r32, 1, 1,v)
    #define   SET16avioGbl_CTRL_AIODHUB_swCG_en(r16,v)         _BFSET_(r16, 1, 1,v)
    #define   SET32avioGbl_CTRL_AIODHUB_CG_en(r32,v)           _BFSET_(r32, 2, 2,v)
    #define   SET16avioGbl_CTRL_AIODHUB_CG_en(r16,v)           _BFSET_(r16, 2, 2,v)
    #define   SET32avioGbl_CTRL_INTR_EN(r32,v)                 _BFSET_(r32, 6, 3,v)
    #define   SET16avioGbl_CTRL_INTR_EN(r16,v)                 _BFSET_(r16, 6, 3,v)
    #define     w32avioGbl_CTRL                                {\
            UNSG32 uCTRL_AIODHUB_dyCG_en                       :  1;\
            UNSG32 uCTRL_AIODHUB_swCG_en                       :  1;\
            UNSG32 uCTRL_AIODHUB_CG_en                         :  1;\
            UNSG32 uCTRL_INTR_EN                               :  4;\
            UNSG32 RSVDx3C_b7                                  : 25;\
          }
    union { UNSG32 u32avioGbl_CTRL;
            struct w32avioGbl_CTRL;
          };
    #define   SET32avioGbl_CTRL0_I2S1_MCLK_OEN(r32,v)          _BFSET_(r32, 0, 0,v)
    #define   SET16avioGbl_CTRL0_I2S1_MCLK_OEN(r16,v)          _BFSET_(r16, 0, 0,v)
    #define   SET32avioGbl_CTRL0_I2S2_MCLK_OEN(r32,v)          _BFSET_(r32, 1, 1,v)
    #define   SET16avioGbl_CTRL0_I2S2_MCLK_OEN(r16,v)          _BFSET_(r16, 1, 1,v)
    #define   SET32avioGbl_CTRL0_I2S3_BCLK_OEN(r32,v)          _BFSET_(r32, 2, 2,v)
    #define   SET16avioGbl_CTRL0_I2S3_BCLK_OEN(r16,v)          _BFSET_(r16, 2, 2,v)
    #define   SET32avioGbl_CTRL0_I2S3_LRCLK_OEN(r32,v)         _BFSET_(r32, 3, 3,v)
    #define   SET16avioGbl_CTRL0_I2S3_LRCLK_OEN(r16,v)         _BFSET_(r16, 3, 3,v)
    #define   SET32avioGbl_CTRL0_PDM_CLK_OEN(r32,v)            _BFSET_(r32, 4, 4,v)
    #define   SET16avioGbl_CTRL0_PDM_CLK_OEN(r16,v)            _BFSET_(r16, 4, 4,v)
    #define   SET32avioGbl_CTRL0_dummy(r32,v)                  _BFSET_(r32, 8, 5,v)
    #define   SET16avioGbl_CTRL0_dummy(r16,v)                  _BFSET_(r16, 8, 5,v)
    #define     w32avioGbl_CTRL0                               {\
            UNSG32 uCTRL0_I2S1_MCLK_OEN                        :  1;\
            UNSG32 uCTRL0_I2S2_MCLK_OEN                        :  1;\
            UNSG32 uCTRL0_I2S3_BCLK_OEN                        :  1;\
            UNSG32 uCTRL0_I2S3_LRCLK_OEN                       :  1;\
            UNSG32 uCTRL0_PDM_CLK_OEN                          :  1;\
            UNSG32 uCTRL0_dummy                                :  4;\
            UNSG32 RSVDx40_b9                                  : 23;\
          }
    union { UNSG32 u32avioGbl_CTRL0;
            struct w32avioGbl_CTRL0;
          };
              SIE_SRAMPWR                                      ie_AIO64DHUB_SRAMPWR;
              SIE_SRAMRWTC                                     ie_SRAMRWTC;
    } SIE_avioGbl;
    typedef union  T32avioGbl_AVPLLA_CLK_EN
          { UNSG32 u32;
            struct w32avioGbl_AVPLLA_CLK_EN;
                 } T32avioGbl_AVPLLA_CLK_EN;
    typedef union  T32avioGbl_SWPDWN_CTRL
          { UNSG32 u32;
            struct w32avioGbl_SWPDWN_CTRL;
                 } T32avioGbl_SWPDWN_CTRL;
    typedef union  T32avioGbl_RWTC_31to0
          { UNSG32 u32;
            struct w32avioGbl_RWTC_31to0;
                 } T32avioGbl_RWTC_31to0;
    typedef union  T32avioGbl_RWTC_57to32
          { UNSG32 u32;
            struct w32avioGbl_RWTC_57to32;
                 } T32avioGbl_RWTC_57to32;
    typedef union  T32avioGbl_CTRL
          { UNSG32 u32;
            struct w32avioGbl_CTRL;
                 } T32avioGbl_CTRL;
    typedef union  T32avioGbl_CTRL0
          { UNSG32 u32;
            struct w32avioGbl_CTRL0;
                 } T32avioGbl_CTRL0;
    typedef union  TavioGbl_AVPLLA_CLK_EN
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_AVPLLA_CLK_EN;
                   };
                 } TavioGbl_AVPLLA_CLK_EN;
    typedef union  TavioGbl_SWPDWN_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_SWPDWN_CTRL;
                   };
                 } TavioGbl_SWPDWN_CTRL;
    typedef union  TavioGbl_RWTC_31to0
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_RWTC_31to0;
                   };
                 } TavioGbl_RWTC_31to0;
    typedef union  TavioGbl_RWTC_57to32
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_RWTC_57to32;
                   };
                 } TavioGbl_RWTC_57to32;
    typedef union  TavioGbl_CTRL
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_CTRL;
                   };
                 } TavioGbl_CTRL;
    typedef union  TavioGbl_CTRL0
          { UNSG32 u32[1];
            struct {
            struct w32avioGbl_CTRL0;
                   };
                 } TavioGbl_CTRL0;
     SIGN32 avioGbl_drvrd(SIE_avioGbl *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 avioGbl_drvwr(SIE_avioGbl *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void avioGbl_reset(SIE_avioGbl *p);
     SIGN32 avioGbl_cmp  (SIE_avioGbl *p, SIE_avioGbl *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define avioGbl_check(p,pie,pfx,hLOG) avioGbl_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define avioGbl_print(p,    pfx,hLOG) avioGbl_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
