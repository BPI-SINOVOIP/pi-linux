/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2018-2020 Synaptics Incorporated */
#ifndef aio_h
#define aio_h (){}
#include "ctypes.h"
#pragma pack(1)
#ifdef __cplusplus
  extern "C" {
#endif
#ifndef _DOCC_H_BITOPS_
#define _DOCC_H_BITOPS_ (){}
    #define _bSETMASK_(b)                                      ((b)<32 ? (u32)(1<<((b)&31)) : 0)
    #define _NSETMASK_(msb,lsb)                                (_bSETMASK_((msb)+1)-_bSETMASK_(lsb))
    #define _bCLRMASK_(b)                                      (~_bSETMASK_(b))
    #define _NCLRMASK_(msb,lsb)                                (~_NSETMASK_(msb,lsb))
    #define _BFGET_(r,msb,lsb)                                 (_NSETMASK_((msb)-(lsb),0)&((r)>>(lsb)))
    #define _BFSET_(r,msb,lsb,v)                               do{ (r)&=_NCLRMASK_(msb,lsb); (r)|=_NSETMASK_(msb,lsb)&((v)<<(lsb)); }while(0)
#endif
#ifndef h_SPDIFRX_CTRL
#define h_SPDIFRX_CTRL (){}
    #define     RA_SPDIFRX_CTRL_CTRL1                          0x0000
    #define     RA_SPDIFRX_CTRL_CTRL2                          0x0004
    #define     RA_SPDIFRX_CTRL_CTRL3                          0x0008
    #define     RA_SPDIFRX_CTRL_CTRL4                          0x000C
    #define     RA_SPDIFRX_CTRL_CTRL5                          0x0010
    #define     RA_SPDIFRX_CTRL_CTRL6                          0x0014
    #define     RA_SPDIFRX_CTRL_CTRL7                          0x0018
    #define     RA_SPDIFRX_CTRL_CTRL8                          0x001C
    #define     RA_SPDIFRX_CTRL_CTRL9                          0x0020
    #define     RA_SPDIFRX_CTRL_CTRL10                         0x0024
    #define     RA_SPDIFRX_CTRL_CTRL11                         0x0028
    #define     RA_SPDIFRX_CTRL_CTRL12                         0x002C
    typedef struct SIE_SPDIFRX_CTRL {
    #define     w32SPDIFRX_CTRL_CTRL1                          {\
            UNSG32 uCTRL1_ERR0_CLR                             :  1;\
            UNSG32 uCTRL1_ERR1_CLR                             :  1;\
            UNSG32 uCTRL1_ERR2_CLR                             :  1;\
            UNSG32 uCTRL1_ERR3_CLR                             :  1;\
            UNSG32 uCTRL1_ERR4_CLR                             :  1;\
            UNSG32 uCTRL1_ERR5_CLR                             :  1;\
            UNSG32 uCTRL1_ERR0_EN                              :  1;\
            UNSG32 uCTRL1_ERR1_EN                              :  1;\
            UNSG32 uCTRL1_ERR2_EN                              :  1;\
            UNSG32 uCTRL1_ERR3_EN                              :  1;\
            UNSG32 uCTRL1_ERR4_EN                              :  1;\
            UNSG32 uCTRL1_ERR5_EN                              :  1;\
            UNSG32 uCTRL1_SW_LOCK_EN                           :  1;\
            UNSG32 uCTRL1_SW_LOCK                              :  1;\
            UNSG32 uCTRL1_LOCK_POL                             :  1;\
            UNSG32 uCTRL1_SW_TRIG                              :  5;\
            UNSG32 uCTRL1_SW_LOAD                              :  1;\
            UNSG32 uCTRL1_SW_TRIG_FIFO                         :  1;\
            UNSG32 uCTRL1_SW_RDEN_CTRL                         :  1;\
            UNSG32 uCTRL1_SW_RDEN                              :  1;\
            UNSG32 uCTRL1_AUTOCALIB                            :  1;\
            UNSG32 uCTRL1_OP_DISABLE                           :  1;\
            UNSG32 uCTRL1_OP_CTRL                              :  2;\
            UNSG32 uCTRL1_CLK_CTRL                             :  1;\
            UNSG32 uCTRL1_DATA_CTRL                            :  1;\
            UNSG32 RSVDx0_b30                                  :  2;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL1;
            struct w32SPDIFRX_CTRL_CTRL1;
          };
    #define     w32SPDIFRX_CTRL_CTRL2                          {\
            UNSG32 uCTRL2_REF_PULSE                            :  8;\
            UNSG32 uCTRL2_ZERO_COUNT                           :  8;\
            UNSG32 uCTRL2_ONE_COUNT                            :  8;\
            UNSG32 RSVDx4_b24                                  :  8;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL2;
            struct w32SPDIFRX_CTRL_CTRL2;
          };
    #define     w32SPDIFRX_CTRL_CTRL3                          {\
            UNSG32 uCTRL3_MARGIN1                              :  8;\
            UNSG32 uCTRL3_MARGIN2                              :  8;\
            UNSG32 uCTRL3_MARGIN3                              :  8;\
            UNSG32 uCTRL3_MARGIN4                              :  8;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL3;
            struct w32SPDIFRX_CTRL_CTRL3;
          };
    #define     w32SPDIFRX_CTRL_CTRL4                          {\
            UNSG32 uCTRL4_RD_PTR                               :  6;\
            UNSG32 uCTRL4_WR_PTR                               :  6;\
            UNSG32 uCTRL4_DIV_CNT_0                            :  8;\
            UNSG32 uCTRL4_DIV_CNT_1                            :  8;\
            UNSG32 RSVDxC_b28                                  :  4;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL4;
            struct w32SPDIFRX_CTRL_CTRL4;
          };
    #define     w32SPDIFRX_CTRL_CTRL5                          {\
            UNSG32 uCTRL5_POLL_REGS                            : 32;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL5;
            struct w32SPDIFRX_CTRL_CTRL5;
          };
    #define     w32SPDIFRX_CTRL_CTRL6                          {\
            UNSG32 uCTRL6_VAL_32K                              :  8;\
            UNSG32 uCTRL6_VAL_44K                              :  8;\
            UNSG32 uCTRL6_VAL_88K                              :  8;\
            UNSG32 uCTRL6_VAL_176K                             :  8;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL6;
            struct w32SPDIFRX_CTRL_CTRL6;
          };
    #define     w32SPDIFRX_CTRL_CTRL7                          {\
            UNSG32 uCTRL7_LOCKWAIT_0                           :  8;\
            UNSG32 uCTRL7_LOCKWAIT_1                           :  8;\
            UNSG32 RSVDx18_b16                                 : 16;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL7;
            struct w32SPDIFRX_CTRL_CTRL7;
          };
    #define     w32SPDIFRX_CTRL_CTRL8                          {\
            UNSG32 uCTRL8_FPLL_N_REGS                          : 24;\
            UNSG32 RSVDx1C_b24                                 :  8;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL8;
            struct w32SPDIFRX_CTRL_CTRL8;
          };
    #define     w32SPDIFRX_CTRL_CTRL9                          {\
            UNSG32 uCTRL9_FPLL_W_REGS                          : 29;\
            UNSG32 RSVDx20_b29                                 :  3;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL9;
            struct w32SPDIFRX_CTRL_CTRL9;
          };
    #define     w32SPDIFRX_CTRL_CTRL10                         {\
            UNSG32 uCTRL10_FPLL_THRESH0                        :  8;\
            UNSG32 uCTRL10_FPLL_THRESH1                        :  8;\
            UNSG32 uCTRL10_FPLL_GAIN                           :  6;\
            UNSG32 RSVDx24_b22                                 : 10;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL10;
            struct w32SPDIFRX_CTRL_CTRL10;
          };
    #define     w32SPDIFRX_CTRL_CTRL11                         {\
            UNSG32 uCTRL11_FPLL_ENABLE                         :  1;\
            UNSG32 uCTRL11_OPEN_LOOP                           :  1;\
            UNSG32 uCTRL11_AUTO_GAIN                           :  1;\
            UNSG32 uCTRL11_CLOSED_LOOP                         :  2;\
            UNSG32 uCTRL11_COAST                               :  1;\
            UNSG32 uCTRL11_RESOLUTION                          :  3;\
            UNSG32 RSVDx28_b9                                  : 23;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL11;
            struct w32SPDIFRX_CTRL_CTRL11;
          };
    #define     w32SPDIFRX_CTRL_CTRL12                         {\
            UNSG32 uCTRL12_FRAME_WIDTH                         :  2;\
            UNSG32 uCTRL12_DATAVALID_WIDTH                     :  3;\
            UNSG32 uCTRL12_OP_MODE                             :  2;\
            UNSG32 uCTRL12_RCV_MODE                            :  1;\
            UNSG32 uCTRL12_JUSTIFY_MODE                        :  1;\
            UNSG32 uCTRL12_L_R_SWITCH                          :  1;\
            UNSG32 uCTRL12_MUTE                                :  1;\
            UNSG32 uCTRL12_I2SFIFO_FLUSH                       :  1;\
            UNSG32 RSVDx2C_b12                                 : 20;\
          }
    union { UNSG32 u32SPDIFRX_CTRL_CTRL12;
            struct w32SPDIFRX_CTRL_CTRL12;
          };
    } SIE_SPDIFRX_CTRL;
    typedef union  T32SPDIFRX_CTRL_CTRL1
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL1;
                 } T32SPDIFRX_CTRL_CTRL1;
    typedef union  T32SPDIFRX_CTRL_CTRL2
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL2;
                 } T32SPDIFRX_CTRL_CTRL2;
    typedef union  T32SPDIFRX_CTRL_CTRL3
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL3;
                 } T32SPDIFRX_CTRL_CTRL3;
    typedef union  T32SPDIFRX_CTRL_CTRL4
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL4;
                 } T32SPDIFRX_CTRL_CTRL4;
    typedef union  T32SPDIFRX_CTRL_CTRL5
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL5;
                 } T32SPDIFRX_CTRL_CTRL5;
    typedef union  T32SPDIFRX_CTRL_CTRL6
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL6;
                 } T32SPDIFRX_CTRL_CTRL6;
    typedef union  T32SPDIFRX_CTRL_CTRL7
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL7;
                 } T32SPDIFRX_CTRL_CTRL7;
    typedef union  T32SPDIFRX_CTRL_CTRL8
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL8;
                 } T32SPDIFRX_CTRL_CTRL8;
    typedef union  T32SPDIFRX_CTRL_CTRL9
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL9;
                 } T32SPDIFRX_CTRL_CTRL9;
    typedef union  T32SPDIFRX_CTRL_CTRL10
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL10;
                 } T32SPDIFRX_CTRL_CTRL10;
    typedef union  T32SPDIFRX_CTRL_CTRL11
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL11;
                 } T32SPDIFRX_CTRL_CTRL11;
    typedef union  T32SPDIFRX_CTRL_CTRL12
          { UNSG32 u32;
            struct w32SPDIFRX_CTRL_CTRL12;
                 } T32SPDIFRX_CTRL_CTRL12;
    typedef union  TSPDIFRX_CTRL_CTRL1
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL1;
                   };
                 } TSPDIFRX_CTRL_CTRL1;
    typedef union  TSPDIFRX_CTRL_CTRL2
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL2;
                   };
                 } TSPDIFRX_CTRL_CTRL2;
    typedef union  TSPDIFRX_CTRL_CTRL3
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL3;
                   };
                 } TSPDIFRX_CTRL_CTRL3;
    typedef union  TSPDIFRX_CTRL_CTRL4
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL4;
                   };
                 } TSPDIFRX_CTRL_CTRL4;
    typedef union  TSPDIFRX_CTRL_CTRL5
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL5;
                   };
                 } TSPDIFRX_CTRL_CTRL5;
    typedef union  TSPDIFRX_CTRL_CTRL6
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL6;
                   };
                 } TSPDIFRX_CTRL_CTRL6;
    typedef union  TSPDIFRX_CTRL_CTRL7
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL7;
                   };
                 } TSPDIFRX_CTRL_CTRL7;
    typedef union  TSPDIFRX_CTRL_CTRL8
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL8;
                   };
                 } TSPDIFRX_CTRL_CTRL8;
    typedef union  TSPDIFRX_CTRL_CTRL9
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL9;
                   };
                 } TSPDIFRX_CTRL_CTRL9;
    typedef union  TSPDIFRX_CTRL_CTRL10
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL10;
                   };
                 } TSPDIFRX_CTRL_CTRL10;
    typedef union  TSPDIFRX_CTRL_CTRL11
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL11;
                   };
                 } TSPDIFRX_CTRL_CTRL11;
    typedef union  TSPDIFRX_CTRL_CTRL12
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_CTRL_CTRL12;
                   };
                 } TSPDIFRX_CTRL_CTRL12;
     SIGN32 SPDIFRX_CTRL_drvrd(SIE_SPDIFRX_CTRL *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SPDIFRX_CTRL_drvwr(SIE_SPDIFRX_CTRL *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SPDIFRX_CTRL_reset(SIE_SPDIFRX_CTRL *p);
     SIGN32 SPDIFRX_CTRL_cmp  (SIE_SPDIFRX_CTRL *p, SIE_SPDIFRX_CTRL *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SPDIFRX_CTRL_check(p,pie,pfx,hLOG) SPDIFRX_CTRL_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SPDIFRX_CTRL_print(p,    pfx,hLOG) SPDIFRX_CTRL_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SPDIFRX_STATUS
#define h_SPDIFRX_STATUS (){}
    #define     RA_SPDIFRX_STATUS_STATUS                       0x0000
    typedef struct SIE_SPDIFRX_STATUS {
    #define     w32SPDIFRX_STATUS_STATUS                       {\
            UNSG32 uSTATUS_PERIOD0                             :  8;\
            UNSG32 uSTATUS_PERIOD1                             :  8;\
            UNSG32 uSTATUS_ERR_0                               :  1;\
            UNSG32 uSTATUS_ERR_1                               :  1;\
            UNSG32 uSTATUS_ERR_2                               :  1;\
            UNSG32 uSTATUS_ERR_3                               :  1;\
            UNSG32 uSTATUS_ERR_4                               :  1;\
            UNSG32 uSTATUS_ERR_5                               :  1;\
            UNSG32 uSTATUS_BDET_LOCK                           :  1;\
            UNSG32 uSTATUS_I2SFIFO_OF                          :  1;\
            UNSG32 uSTATUS_FPLL_STAT                           :  1;\
            UNSG32 RSVDx0_b25                                  :  7;\
          }
    union { UNSG32 u32SPDIFRX_STATUS_STATUS;
            struct w32SPDIFRX_STATUS_STATUS;
          };
    } SIE_SPDIFRX_STATUS;
    typedef union  T32SPDIFRX_STATUS_STATUS
          { UNSG32 u32;
            struct w32SPDIFRX_STATUS_STATUS;
                 } T32SPDIFRX_STATUS_STATUS;
    typedef union  TSPDIFRX_STATUS_STATUS
          { UNSG32 u32[1];
            struct {
            struct w32SPDIFRX_STATUS_STATUS;
                   };
                 } TSPDIFRX_STATUS_STATUS;
     SIGN32 SPDIFRX_STATUS_drvrd(SIE_SPDIFRX_STATUS *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SPDIFRX_STATUS_drvwr(SIE_SPDIFRX_STATUS *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SPDIFRX_STATUS_reset(SIE_SPDIFRX_STATUS *p);
     SIGN32 SPDIFRX_STATUS_cmp  (SIE_SPDIFRX_STATUS *p, SIE_SPDIFRX_STATUS *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SPDIFRX_STATUS_check(p,pie,pfx,hLOG) SPDIFRX_STATUS_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SPDIFRX_STATUS_print(p,    pfx,hLOG) SPDIFRX_STATUS_cmp(p,0,  pfx,(void*)(hLOG),0,0)
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
    #define     w32PRIAUD_CLKDIV                               {\
            UNSG32 uCLKDIV_SETTING                             :  4;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32PRIAUD_CLKDIV;
            struct w32PRIAUD_CLKDIV;
          };
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
    #define     RA_DBG_TX_DEBUGLO                              0x0004
    typedef struct SIE_DBG_TX {
    #define     w32DBG_TX_DEBUGHI                              {\
            UNSG32 uDEBUGHI_DATAHI                             : 32;\
          }
    union { UNSG32 u32DBG_TX_DEBUGHI;
            struct w32DBG_TX_DEBUGHI;
          };
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
    #define     RA_DBG_RX_DEBUGHI                              0x0004
    #define     RA_DBG_RX_DEBUGLO                              0x0008
    typedef struct SIE_DBG_RX {
    #define     w32DBG_RX_DBG                                  {\
            UNSG32 uDBG_PORT_SEL                               :  2;\
            UNSG32 RSVDx0_b2                                   : 30;\
          }
    union { UNSG32 u32DBG_RX_DBG;
            struct w32DBG_RX_DBG;
          };
    #define     w32DBG_RX_DEBUGHI                              {\
            UNSG32 uDEBUGHI_DATAHI                             : 32;\
          }
    union { UNSG32 u32DBG_RX_DEBUGHI;
            struct w32DBG_RX_DEBUGHI;
          };
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
    #define        ACLK_ACLK_CTRL_clk_Enable_DISABLE                        0x0
    #define        ACLK_ACLK_CTRL_clk_Enable_ENABLE                         0x1
    #define        ACLK_ACLK_CTRL_src_sel_AVPLL_A3                          0x0
    #define        ACLK_ACLK_CTRL_src_sel_AVPLL_A4                          0x1
    #define        ACLK_ACLK_CTRL_src_sel_MCLK_EXT                          0x2
    #define        ACLK_ACLK_CTRL_clkSwitch_SRC_CLK                         0x0
    #define        ACLK_ACLK_CTRL_clkSwitch_DIV_CLK                         0x1
    #define        ACLK_ACLK_CTRL_clkD3Switch_DIV_NOR                       0x0
    #define        ACLK_ACLK_CTRL_clkD3Switch_DIV_3                         0x1
    #define        ACLK_ACLK_CTRL_clkSel_d2                                 0x1
    #define        ACLK_ACLK_CTRL_clkSel_d4                                 0x2
    #define        ACLK_ACLK_CTRL_clkSel_d6                                 0x3
    #define        ACLK_ACLK_CTRL_clkSel_d8                                 0x4
    #define        ACLK_ACLK_CTRL_clkSel_d12                                0x5
    #define        ACLK_ACLK_CTRL_sw_sync_rst_ASSERT                        0x0
    #define        ACLK_ACLK_CTRL_sw_sync_rst_DEASSERT                      0x1
    typedef struct SIE_ACLK {
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
    #define        PRI_PRIPORT_ENABLE_DISABLE                               0x0
    #define        PRI_PRIPORT_ENABLE_ENABLE                                0x1
    typedef struct SIE_PRI {
              SIE_PRIAUD                                       ie_PRIAUD;
              SIE_AUDCH                                        ie_TSD0_PRI;
              SIE_AUDCH                                        ie_TSD1_PRI;
              SIE_AUDCH                                        ie_TSD2_PRI;
              SIE_AUDCH                                        ie_TSD3_PRI;
              SIE_DBG_TX                                       ie_DBG;
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
    #define        SEC_SECPORT_ENABLE_DISABLE                               0x0
    #define        SEC_SECPORT_ENABLE_ENABLE                                0x1
    typedef struct SIE_SEC {
              SIE_PRIAUD                                       ie_SECAUD;
              SIE_AUDCH                                        ie_TSD0_SEC;
              SIE_DBG_TX                                       ie_DBG;
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
#ifndef h_MIC1
#define h_MIC1 (){}
    #define     RA_MIC1_MICCTRL                                0x0000
    #define     RA_MIC1_RSD0                                   0x000C
    #define     RA_MIC1_DBG                                    0x0010
    #define     RA_MIC1_RSD1                                   0x001C
    #define     RA_MIC1_RSD2                                   0x0020
    #define     RA_MIC1_RSD3                                   0x0024
    #define     RA_MIC1_MM_MODE                                0x0028
    #define     RA_MIC1_RXPORT                                 0x002C
    #define        MIC1_RXPORT_ENABLE_DISABLE                               0x0
    #define        MIC1_RXPORT_ENABLE_ENABLE                                0x1
    #define     RA_MIC1_RXDATA                                 0x0030
    #define     RA_MIC1_INTLMODE                               0x0034
    #define     RA_MIC1_HBRDMAP                                0x0038
    typedef struct SIE_MIC1 {
              SIE_PRIAUD                                       ie_MICCTRL;
              SIE_AUDCH                                        ie_RSD0;
              SIE_DBG_RX                                       ie_DBG;
              SIE_AUDCH                                        ie_RSD1;
              SIE_AUDCH                                        ie_RSD2;
              SIE_AUDCH                                        ie_RSD3;
    #define     w32MIC1_MM_MODE                                {\
            UNSG32 uMM_MODE_RCV_MASTER                         :  1;\
            UNSG32 uMM_MODE_WS_HIGH_PRD                        :  8;\
            UNSG32 uMM_MODE_WS_TOTAL_PRD                       :  8;\
            UNSG32 uMM_MODE_WS_INV                             :  1;\
            UNSG32 RSVDx28_b18                                 : 14;\
          }
    union { UNSG32 u32MIC1_MM_MODE;
            struct w32MIC1_MM_MODE;
          };
    #define     w32MIC1_RXPORT                                 {\
            UNSG32 uRXPORT_ENABLE                              :  1;\
            UNSG32 RSVDx2C_b1                                  : 31;\
          }
    union { UNSG32 u32MIC1_RXPORT;
            struct w32MIC1_RXPORT;
          };
    #define     w32MIC1_RXDATA                                 {\
            UNSG32 uRXDATA_HBR                                 :  1;\
            UNSG32 uRXDATA_TDM_HR                              :  1;\
            UNSG32 RSVDx30_b2                                  : 30;\
          }
    union { UNSG32 u32MIC1_RXDATA;
            struct w32MIC1_RXDATA;
          };
    #define     w32MIC1_INTLMODE                               {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx34_b5                                  : 27;\
          }
    union { UNSG32 u32MIC1_INTLMODE;
            struct w32MIC1_INTLMODE;
          };
    #define     w32MIC1_HBRDMAP                                {\
            UNSG32 uHBRDMAP_PORT0                              :  2;\
            UNSG32 uHBRDMAP_PORT1                              :  2;\
            UNSG32 uHBRDMAP_PORT2                              :  2;\
            UNSG32 uHBRDMAP_PORT3                              :  2;\
            UNSG32 RSVDx38_b8                                  : 24;\
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
    #define     RA_MIC2_RSD1                                   0x001C
    #define     RA_MIC2_RXPORT                                 0x0020
    #define        MIC2_RXPORT_ENABLE_DISABLE                               0x0
    #define        MIC2_RXPORT_ENABLE_ENABLE                                0x1
    #define     RA_MIC2_RXDATA                                 0x0024
    #define     RA_MIC2_INTLMODE                               0x0028
    #define     RA_MIC2_HBRDMAP                                0x002C
    typedef struct SIE_MIC2 {
              SIE_PRIAUD                                       ie_MICCTRL;
              SIE_AUDCH                                        ie_RSD0;
              SIE_DBG_RX                                       ie_DBG;
              SIE_AUDCH                                        ie_RSD1;
    #define     w32MIC2_RXPORT                                 {\
            UNSG32 uRXPORT_ENABLE                              :  1;\
            UNSG32 RSVDx20_b1                                  : 31;\
          }
    union { UNSG32 u32MIC2_RXPORT;
            struct w32MIC2_RXPORT;
          };
    #define     w32MIC2_RXDATA                                 {\
            UNSG32 uRXDATA_HBR                                 :  1;\
            UNSG32 uRXDATA_TDM_HR                              :  1;\
            UNSG32 RSVDx24_b2                                  : 30;\
          }
    union { UNSG32 u32MIC2_RXDATA;
            struct w32MIC2_RXDATA;
          };
    #define     w32MIC2_INTLMODE                               {\
            UNSG32 uINTLMODE_PORT0_EN                          :  1;\
            UNSG32 uINTLMODE_PORT1_EN                          :  1;\
            UNSG32 uINTLMODE_PORT2_EN                          :  1;\
            UNSG32 uINTLMODE_PORT3_EN                          :  1;\
            UNSG32 uINTLMODE_DUMMYDATA_EN                      :  1;\
            UNSG32 RSVDx28_b5                                  : 27;\
          }
    union { UNSG32 u32MIC2_INTLMODE;
            struct w32MIC2_INTLMODE;
          };
    #define     w32MIC2_HBRDMAP                                {\
            UNSG32 uHBRDMAP_PORT0                              :  2;\
            UNSG32 uHBRDMAP_PORT1                              :  2;\
            UNSG32 uHBRDMAP_PORT2                              :  2;\
            UNSG32 uHBRDMAP_PORT3                              :  2;\
            UNSG32 RSVDx2C_b8                                  : 24;\
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
#ifndef h_PDMCH
#define h_PDMCH (){}
    #define     RA_PDMCH_CTRL                                  0x0000
    #define        PDMCH_CTRL_ENABLE_DISABLE                                0x0
    #define        PDMCH_CTRL_ENABLE_ENABLE                                 0x1
    #define        PDMCH_CTRL_MUTE_MUTE_OFF                                 0x0
    #define        PDMCH_CTRL_MUTE_MUTE_ON                                  0x1
    #define        PDMCH_CTRL_LRSWITCH_SWITCH_OFF                           0x0
    #define        PDMCH_CTRL_LRSWITCH_SWITCH_ON                            0x1
    #define        PDMCH_CTRL_FLUSH_ON                                      0x1
    #define        PDMCH_CTRL_FLUSH_OFF                                     0x0
    #define     RA_PDMCH_CTRL2                                 0x0004
    typedef struct SIE_PDMCH {
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
    #define     RA_PDM_RXDATA                                  0x0004
    #define     RA_PDM_INTLMODE                                0x0008
    #define     RA_PDM_INTLMAP                                 0x000C
    #define     RA_PDM_PDM0                                    0x0010
    #define     RA_PDM_PDM1                                    0x0018
    #define     RA_PDM_PDM2                                    0x0020
    #define     RA_PDM_PDM3                                    0x0028
    #define     SET32PDMCH_CTRL_MUTE(r32,v)                    _BFSET_(r32, 1, 1,v)
    #define     SET32PDMCH_CTRL_ENABLE(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define     SET32IOSEL_PDM_GENABLE(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define     SET32PDMCH_CTRL2_RDLT(r32,v)                   _BFSET_(r32,15, 0,v)
    #define     SET32PDMCH_CTRL2_FDLT(r32,v)                   _BFSET_(r32,31,16,v)
    #define     SET32PDM_INTLMODE_PORT0_EN(r32,v)              _BFSET_(r32, 0, 0,v)
    #define     SET32PDM_INTLMODE_PORT1_EN(r32,v)              _BFSET_(r32, 1, 1,v)
    #define     SET32PDM_INTLMODE_PORT2_EN(r32,v)              _BFSET_(r32, 2, 2,v)
    #define     SET32PDM_INTLMODE_PORT3_EN(r32,v)              _BFSET_(r32, 3, 3,v)
    #define     SET32PDM_CTRL1_CLKDIV(r32,v)                   _BFSET_(r32, 3, 0,v)
    #define     SET32PDM_CTRL1_INVCLK_INT(r32,v)               _BFSET_(r32, 5, 5,v)
    #define     SET32PDM_CTRL1_RLSB(r32,v)                     _BFSET_(r32, 6, 6,v)
    #define     SET32PDM_CTRL1_RDM(r32,v)                      _BFSET_(r32, 9, 7,v)
    #define     SET32PDM_CTRL1_MODE(r32,v)                     _BFSET_(r32,10,10,v)
    #define     SET32PDM_CTRL1_LATCH_MODE(r32,v)               _BFSET_(r32,12,12,v)
    typedef struct SIE_PDM {
    #define     w32PDM_CTRL1                                   {\
            UNSG32 uCTRL1_CLKDIV                               :  4;\
            UNSG32 uCTRL1_INVCLK_OUT                           :  1;\
            UNSG32 uCTRL1_INVCLK_INT                           :  1;\
            UNSG32 uCTRL1_RLSB                                 :  1;\
            UNSG32 uCTRL1_RDM                                  :  3;\
            UNSG32 uCTRL1_MODE                                 :  1;\
            UNSG32 uCTRL1_SDR_CLKSEL                           :  1;\
            UNSG32 uCTRL1_LATCH_MODE                           :  1;\
            UNSG32 RSVDx0_b13                                  : 19;\
          }
    union { UNSG32 u32PDM_CTRL1;
            struct w32PDM_CTRL1;
          };
    #define     w32PDM_RXDATA                                  {\
            UNSG32 uRXDATA_INTL                                :  1;\
            UNSG32 RSVDx4_b1                                   : 31;\
          }
    union { UNSG32 u32PDM_RXDATA;
            struct w32PDM_RXDATA;
          };
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
              SIE_PDMCH                                        ie_PDM2;
              SIE_PDMCH                                        ie_PDM3;
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
#ifndef h_IOSEL
#define h_IOSEL (){}
    #define     RA_IOSEL_PRIMCLK                               0x0000
    #define     RA_IOSEL_MIC1MCLK                              0x0004
    #define     RA_IOSEL_PRIBCLK                               0x0008
    #define     RA_IOSEL_SECBCLK                               0x000C
    #define     RA_IOSEL_MIC1BCLK                              0x0010
    #define     RA_IOSEL_PRIFSYNC                              0x0014
    #define     RA_IOSEL_SECFSYNC                              0x0018
    #define     RA_IOSEL_MIC1FSYNC                             0x001C
    #define     RA_IOSEL_MIC2FSYNC                             0x0020
    #define     RA_IOSEL_MIC2BCLK                              0x0024
    #define     RA_IOSEL_PDM                                   0x0028
    typedef struct SIE_IOSEL {
    #define     w32IOSEL_PRIMCLK                               {\
            UNSG32 uPRIMCLK_SEL                                :  1;\
            UNSG32 RSVDx0_b1                                   : 31;\
          }
    union { UNSG32 u32IOSEL_PRIMCLK;
            struct w32IOSEL_PRIMCLK;
          };
    #define     w32IOSEL_MIC1MCLK                              {\
            UNSG32 uMIC1MCLK_SEL                               :  1;\
            UNSG32 RSVDx4_b1                                   : 31;\
          }
    union { UNSG32 u32IOSEL_MIC1MCLK;
            struct w32IOSEL_MIC1MCLK;
          };
    #define     w32IOSEL_PRIBCLK                               {\
            UNSG32 uPRIBCLK_SEL                                :  2;\
            UNSG32 uPRIBCLK_INV                                :  1;\
            UNSG32 RSVDx8_b3                                   : 29;\
          }
    union { UNSG32 u32IOSEL_PRIBCLK;
            struct w32IOSEL_PRIBCLK;
          };
    #define     w32IOSEL_SECBCLK                               {\
            UNSG32 uSECBCLK_SEL                                :  2;\
            UNSG32 uSECBCLK_INV                                :  1;\
            UNSG32 RSVDxC_b3                                   : 29;\
          }
    union { UNSG32 u32IOSEL_SECBCLK;
            struct w32IOSEL_SECBCLK;
          };
    #define     w32IOSEL_MIC1BCLK                              {\
            UNSG32 uMIC1BCLK_SEL                               :  2;\
            UNSG32 uMIC1BCLK_INV                               :  1;\
            UNSG32 RSVDx10_b3                                  : 29;\
          }
    union { UNSG32 u32IOSEL_MIC1BCLK;
            struct w32IOSEL_MIC1BCLK;
          };
    #define     w32IOSEL_PRIFSYNC                              {\
            UNSG32 uPRIFSYNC_SEL                               :  1;\
            UNSG32 uPRIFSYNC_INV                               :  1;\
            UNSG32 RSVDx14_b2                                  : 30;\
          }
    union { UNSG32 u32IOSEL_PRIFSYNC;
            struct w32IOSEL_PRIFSYNC;
          };
    #define     w32IOSEL_SECFSYNC                              {\
            UNSG32 uSECFSYNC_SEL                               :  1;\
            UNSG32 uSECFSYNC_INV                               :  1;\
            UNSG32 RSVDx18_b2                                  : 30;\
          }
    union { UNSG32 u32IOSEL_SECFSYNC;
            struct w32IOSEL_SECFSYNC;
          };
    #define     w32IOSEL_MIC1FSYNC                             {\
            UNSG32 uMIC1FSYNC_SEL                              :  1;\
            UNSG32 uMIC1FSYNC_INV                              :  1;\
            UNSG32 RSVDx1C_b2                                  : 30;\
          }
    union { UNSG32 u32IOSEL_MIC1FSYNC;
            struct w32IOSEL_MIC1FSYNC;
          };
    #define     w32IOSEL_MIC2FSYNC                             {\
            UNSG32 uMIC2FSYNC_SEL                              :  1;\
            UNSG32 uMIC2FSYNC_INV                              :  1;\
            UNSG32 RSVDx20_b2                                  : 30;\
          }
    union { UNSG32 u32IOSEL_MIC2FSYNC;
            struct w32IOSEL_MIC2FSYNC;
          };
    #define     w32IOSEL_MIC2BCLK                              {\
            UNSG32 uMIC2BCLK_SEL                               :  2;\
            UNSG32 uMIC2BCLK_INV                               :  1;\
            UNSG32 RSVDx24_b3                                  : 29;\
          }
    union { UNSG32 u32IOSEL_MIC2BCLK;
            struct w32IOSEL_MIC2BCLK;
          };
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
    #define     RA_AIO_SPDIF                                   0x0044
    #define     RA_AIO_MIC1                                    0x0054
    #define     RA_AIO_MIC2                                    0x0090
    #define     RA_AIO_PDM                                     0x00C0
    #define     RA_AIO_SPDIFRX_CTRL                            0x00F0
    #define     RA_AIO_SPDIFRX_STATUS                          0x0120
    #define     RA_AIO_IOSEL                                   0x0124
    #define     RA_AIO_IRQENABLE                               0x0150
    #define     RA_AIO_IRQSTS                                  0x0154
    #define     RA_AIO_PRISRC                                  0x0158
    #define     RA_AIO_SECSRC                                  0x015C
    #define     RA_AIO_PDM_MIC_SEL                             0x0160
    #define     RA_AIO_MCLKPRI                                 0x0164
    #define     RA_AIO_MCLKSEC                                 0x0168
    #define     RA_AIO_MCLKSPF                                 0x016C
    #define     RA_AIO_MCLKPDM                                 0x0170
    #define     RA_AIO_MCLKMIC1                                0x0174
    #define     RA_AIO_MCLKMIC2                                0x0178
    #define     RA_AIO_SW_RST                                  0x017C
    #define     RA_AIO_SAMP_CTRL                               0x0180
    #define     RA_AIO_SAMPINFO_REQ                            0x0184
    #define     RA_AIO_SCR                                     0x0188
    #define     RA_AIO_SCR1                                    0x018C
    #define     RA_AIO_SCR2                                    0x0190
    #define     RA_AIO_SCR3                                    0x0194
    #define     RA_AIO_SCR4                                    0x0198
    #define     RA_AIO_SCR5                                    0x019C
    #define     RA_AIO_SCR6                                    0x01A0
    #define     RA_AIO_STR                                     0x01A4
    #define     RA_AIO_STR1                                    0x01A8
    #define     RA_AIO_STR2                                    0x01AC
    #define     RA_AIO_STR3                                    0x01B0
    #define     RA_AIO_STR4                                    0x01B4
    #define     RA_AIO_STR5                                    0x01B8
    #define     RA_AIO_STR6                                    0x01BC
    #define     RA_AIO_ATR                                     0x01C0
    typedef struct SIE_AIO {
              SIE_PRI                                          ie_PRI;
              SIE_SEC                                          ie_SEC;
              SIE_SPDIF                                        ie_SPDIF;
              SIE_MIC1                                         ie_MIC1;
              SIE_MIC2                                         ie_MIC2;
              SIE_PDM                                          ie_PDM;
              SIE_SPDIFRX_CTRL                                 ie_SPDIFRX_CTRL;
              SIE_SPDIFRX_STATUS                               ie_SPDIFRX_STATUS;
              SIE_IOSEL                                        ie_IOSEL;
    #define     w32AIO_IRQENABLE                               {\
            UNSG32 uIRQENABLE_PRIIRQ                           :  1;\
            UNSG32 uIRQENABLE_SECIRQ                           :  1;\
            UNSG32 uIRQENABLE_MIC1IRQ                          :  1;\
            UNSG32 uIRQENABLE_MIC2IRQ                          :  1;\
            UNSG32 uIRQENABLE_SPDIFIRQ                         :  1;\
            UNSG32 uIRQENABLE_PDMIRQ                           :  1;\
            UNSG32 uIRQENABLE_SPDIFRXIRQ                       :  1;\
            UNSG32 RSVDx150_b7                                 : 25;\
          }
    union { UNSG32 u32AIO_IRQENABLE;
            struct w32AIO_IRQENABLE;
          };
    #define     w32AIO_IRQSTS                                  {\
            UNSG32 uIRQSTS_PRISTS                              :  1;\
            UNSG32 uIRQSTS_SECSTS                              :  1;\
            UNSG32 uIRQSTS_MIC1STS                             :  1;\
            UNSG32 uIRQSTS_MIC2STS                             :  1;\
            UNSG32 uIRQSTS_SPDIFSTS                            :  1;\
            UNSG32 uIRQSTS_PDMSTS                              :  1;\
            UNSG32 uIRQSTS_SPDIFRXSTS                          :  1;\
            UNSG32 RSVDx154_b7                                 : 25;\
          }
    union { UNSG32 u32AIO_IRQSTS;
            struct w32AIO_IRQSTS;
          };
    #define     w32AIO_PRISRC                                  {\
            UNSG32 uPRISRC_SEL                                 :  2;\
            UNSG32 uPRISRC_L0DATAMAP                           :  2;\
            UNSG32 uPRISRC_L1DATAMAP                           :  2;\
            UNSG32 uPRISRC_L2DATAMAP                           :  2;\
            UNSG32 uPRISRC_L3DATAMAP                           :  2;\
            UNSG32 uPRISRC_SLAVEMODE                           :  1;\
            UNSG32 RSVDx158_b11                                : 21;\
          }
    union { UNSG32 u32AIO_PRISRC;
            struct w32AIO_PRISRC;
          };
    #define     w32AIO_SECSRC                                  {\
            UNSG32 uSECSRC_SEL                                 :  2;\
            UNSG32 uSECSRC_L0DATAMAP                           :  2;\
            UNSG32 uSECSRC_L1DATAMAP                           :  2;\
            UNSG32 uSECSRC_L2DATAMAP                           :  2;\
            UNSG32 uSECSRC_L3DATAMAP                           :  2;\
            UNSG32 uSECSRC_SLAVEMODE                           :  1;\
            UNSG32 RSVDx15C_b11                                : 21;\
          }
    union { UNSG32 u32AIO_SECSRC;
            struct w32AIO_SECSRC;
          };
    #define     w32AIO_PDM_MIC_SEL                             {\
            UNSG32 uPDM_MIC_SEL_CTRL                           :  4;\
            UNSG32 RSVDx160_b4                                 : 28;\
          }
    union { UNSG32 u32AIO_PDM_MIC_SEL;
            struct w32AIO_PDM_MIC_SEL;
          };
              SIE_ACLK                                         ie_MCLKPRI;
              SIE_ACLK                                         ie_MCLKSEC;
              SIE_ACLK                                         ie_MCLKSPF;
              SIE_ACLK                                         ie_MCLKPDM;
              SIE_ACLK                                         ie_MCLKMIC1;
              SIE_ACLK                                         ie_MCLKMIC2;
    #define     w32AIO_SW_RST                                  {\
            UNSG32 uSW_RST_SPFRX                               :  1;\
            UNSG32 uSW_RST_REFCLK                              :  1;\
            UNSG32 RSVDx17C_b2                                 : 30;\
          }
    union { UNSG32 u32AIO_SW_RST;
            struct w32AIO_SW_RST;
          };
    #define     w32AIO_SAMP_CTRL                               {\
            UNSG32 uSAMP_CTRL_EN_I2STX1                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2STX2                        :  1;\
            UNSG32 uSAMP_CTRL_EN_SPDIFTX                       :  1;\
            UNSG32 uSAMP_CTRL_EN_SPDIFRX                       :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX1                        :  1;\
            UNSG32 uSAMP_CTRL_EN_I2SRX2                        :  1;\
            UNSG32 uSAMP_CTRL_EN_AUDTIMER                      :  1;\
            UNSG32 uSAMP_CTRL_EN_PDMRX1                        :  1;\
            UNSG32 RSVDx180_b8                                 : 24;\
          }
    union { UNSG32 u32AIO_SAMP_CTRL;
            struct w32AIO_SAMP_CTRL;
          };
    #define     w32AIO_SAMPINFO_REQ                            {\
            UNSG32 uSAMPINFO_REQ_I2STX1                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2STX2                        :  1;\
            UNSG32 uSAMPINFO_REQ_SPDIFTX                       :  1;\
            UNSG32 uSAMPINFO_REQ_SPDIFRX                       :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX1                        :  1;\
            UNSG32 uSAMPINFO_REQ_I2SRX2                        :  1;\
            UNSG32 uSAMPINFO_REQ_PDMRX1                        :  1;\
            UNSG32 RSVDx184_b7                                 : 25;\
          }
    union { UNSG32 u32AIO_SAMPINFO_REQ;
            struct w32AIO_SAMPINFO_REQ;
          };
    #define     w32AIO_SCR                                     {\
            UNSG32 uSCR_I2STX1                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR;
            struct w32AIO_SCR;
          };
    #define     w32AIO_SCR1                                    {\
            UNSG32 uSCR_I2STX2                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR1;
            struct w32AIO_SCR1;
          };
    #define     w32AIO_SCR2                                    {\
            UNSG32 uSCR_SPDIFTX                                : 32;\
          }
    union { UNSG32 u32AIO_SCR2;
            struct w32AIO_SCR2;
          };
    #define     w32AIO_SCR3                                    {\
            UNSG32 uSCR_SPDIFRX                                : 32;\
          }
    union { UNSG32 u32AIO_SCR3;
            struct w32AIO_SCR3;
          };
    #define     w32AIO_SCR4                                    {\
            UNSG32 uSCR_I2SRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR4;
            struct w32AIO_SCR4;
          };
    #define     w32AIO_SCR5                                    {\
            UNSG32 uSCR_I2SRX2                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR5;
            struct w32AIO_SCR5;
          };
    #define     w32AIO_SCR6                                    {\
            UNSG32 uSCR_PDMRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_SCR6;
            struct w32AIO_SCR6;
          };
    #define     w32AIO_STR                                     {\
            UNSG32 uSTR_I2STX1                                 : 32;\
          }
    union { UNSG32 u32AIO_STR;
            struct w32AIO_STR;
          };
    #define     w32AIO_STR1                                    {\
            UNSG32 uSTR_I2STX2                                 : 32;\
          }
    union { UNSG32 u32AIO_STR1;
            struct w32AIO_STR1;
          };
    #define     w32AIO_STR2                                    {\
            UNSG32 uSTR_SPDIFTX                                : 32;\
          }
    union { UNSG32 u32AIO_STR2;
            struct w32AIO_STR2;
          };
    #define     w32AIO_STR3                                    {\
            UNSG32 uSTR_SPDIFRX                                : 32;\
          }
    union { UNSG32 u32AIO_STR3;
            struct w32AIO_STR3;
          };
    #define     w32AIO_STR4                                    {\
            UNSG32 uSTR_I2SRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_STR4;
            struct w32AIO_STR4;
          };
    #define     w32AIO_STR5                                    {\
            UNSG32 uSTR_I2SRX2                                 : 32;\
          }
    union { UNSG32 u32AIO_STR5;
            struct w32AIO_STR5;
          };
    #define     w32AIO_STR6                                    {\
            UNSG32 uSTR_PDMRX1                                 : 32;\
          }
    union { UNSG32 u32AIO_STR6;
            struct w32AIO_STR6;
          };
    #define     w32AIO_ATR                                     {\
            UNSG32 uATR_TIMER                                  : 32;\
          }
    union { UNSG32 u32AIO_ATR;
            struct w32AIO_ATR;
          };
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
    typedef union  T32AIO_ATR
          { UNSG32 u32;
            struct w32AIO_ATR;
                 } T32AIO_ATR;
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
          { UNSG32 u32[7];
            struct {
            struct w32AIO_SCR;
            struct w32AIO_SCR1;
            struct w32AIO_SCR2;
            struct w32AIO_SCR3;
            struct w32AIO_SCR4;
            struct w32AIO_SCR5;
            struct w32AIO_SCR6;
                   };
                 } TAIO_SCR;
    typedef union  TAIO_STR
          { UNSG32 u32[7];
            struct {
            struct w32AIO_STR;
            struct w32AIO_STR1;
            struct w32AIO_STR2;
            struct w32AIO_STR3;
            struct w32AIO_STR4;
            struct w32AIO_STR5;
            struct w32AIO_STR6;
                   };
                 } TAIO_STR;
    typedef union  TAIO_ATR
          { UNSG32 u32[1];
            struct {
            struct w32AIO_ATR;
                   };
                 } TAIO_ATR;
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
