#ifndef avio_h
#define avio_h (){}
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
#ifndef h_oneReg
#define h_oneReg (){}
    #define       boneReg_0x00000000                           32
    typedef struct SIE_oneReg {
    #define   SET32oneReg_0x00000000(r32,v)                    _BFSET_(r32,31, 0,v)
            UNSG32 u_0x00000000                                : 32;
    } SIE_oneReg;
     SIGN32 oneReg_drvrd(SIE_oneReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 oneReg_drvwr(SIE_oneReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void oneReg_reset(SIE_oneReg *p);
     SIGN32 oneReg_cmp  (SIE_oneReg *p, SIE_oneReg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define oneReg_check(p,pie,pfx,hLOG) oneReg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define oneReg_print(p,    pfx,hLOG) oneReg_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AVIO_REG
#define h_AVIO_REG (){}
    #define     RA_AVIO_REG_dummy                              0x0000
    typedef struct SIE_AVIO_REG {
              SIE_oneReg                                       ie_dummy[64];
    } SIE_AVIO_REG;
     SIGN32 AVIO_REG_drvrd(SIE_AVIO_REG *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AVIO_REG_drvwr(SIE_AVIO_REG *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AVIO_REG_reset(SIE_AVIO_REG *p);
     SIGN32 AVIO_REG_cmp  (SIE_AVIO_REG *p, SIE_AVIO_REG *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AVIO_REG_check(p,pie,pfx,hLOG) AVIO_REG_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AVIO_REG_print(p,    pfx,hLOG) AVIO_REG_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_OCCURENCE
#define h_OCCURENCE (){}
    #define       bOCCURENCE_AUTO_PUSH_CNT                     8
    typedef struct SIE_OCCURENCE {
    #define   SET32OCCURENCE_AUTO_PUSH_CNT(r32,v)              _BFSET_(r32, 7, 0,v)
    #define   SET16OCCURENCE_AUTO_PUSH_CNT(r16,v)              _BFSET_(r16, 7, 0,v)
            UNSG32 u_AUTO_PUSH_CNT                             :  8;
            UNSG32 RSVDx0_b8                                   : 24;
    } SIE_OCCURENCE;
     SIGN32 OCCURENCE_drvrd(SIE_OCCURENCE *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 OCCURENCE_drvwr(SIE_OCCURENCE *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void OCCURENCE_reset(SIE_OCCURENCE *p);
     SIGN32 OCCURENCE_cmp  (SIE_OCCURENCE *p, SIE_OCCURENCE *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define OCCURENCE_check(p,pie,pfx,hLOG) OCCURENCE_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define OCCURENCE_print(p,    pfx,hLOG) OCCURENCE_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_AVIO
#define h_AVIO (){}
    #define     RA_AVIO_cfgReg                                 0x0000
    #define     RA_AVIO_BCM_Q0                                 0x0100
    #define       bAVIO_BCM_Q0_mux                             5
    #define     RA_AVIO_BCM_Q1                                 0x0104
    #define       bAVIO_BCM_Q1_mux                             5
    #define     RA_AVIO_BCM_Q2                                 0x0108
    #define       bAVIO_BCM_Q2_mux                             5
    #define     RA_AVIO_BCM_Q3                                 0x010C
    #define       bAVIO_BCM_Q3_mux                             5
    #define     RA_AVIO_BCM_Q4                                 0x0110
    #define       bAVIO_BCM_Q4_mux                             5
    #define     RA_AVIO_BCM_Q5                                 0x0114
    #define       bAVIO_BCM_Q5_mux                             5
    #define     RA_AVIO_BCM_Q6                                 0x0118
    #define       bAVIO_BCM_Q6_mux                             5
    #define     RA_AVIO_BCM_Q7                                 0x011C
    #define       bAVIO_BCM_Q7_mux                             5
    #define     RA_AVIO_BCM_Q8                                 0x0120
    #define       bAVIO_BCM_Q8_mux                             5
    #define     RA_AVIO_BCM_Q9                                 0x0124
    #define       bAVIO_BCM_Q9_mux                             5
    #define     RA_AVIO_BCM_Q10                                0x0128
    #define       bAVIO_BCM_Q10_mux                            5
    #define     RA_AVIO_BCM_Q11                                0x012C
    #define       bAVIO_BCM_Q11_mux                            5
    #define     RA_AVIO_BCM_Q14                                0x0130
    #define       bAVIO_BCM_Q14_mux                            5
    #define     RA_AVIO_BCM_Q15                                0x0134
    #define       bAVIO_BCM_Q15_mux                            5
    #define     RA_AVIO_BCM_Q16                                0x0138
    #define       bAVIO_BCM_Q16_mux                            5
    #define     RA_AVIO_BCM_Q17                                0x013C
    #define       bAVIO_BCM_Q17_mux                            5
    #define     RA_AVIO_BCM_Q18                                0x0140
    #define       bAVIO_BCM_Q18_mux                            5
    #define     RA_AVIO_BCM_FULL_STS                           0x0144
    #define       bAVIO_BCM_FULL_STS_Q0                        1
    #define       bAVIO_BCM_FULL_STS_Q1                        1
    #define       bAVIO_BCM_FULL_STS_Q2                        1
    #define       bAVIO_BCM_FULL_STS_Q3                        1
    #define       bAVIO_BCM_FULL_STS_Q4                        1
    #define       bAVIO_BCM_FULL_STS_Q5                        1
    #define       bAVIO_BCM_FULL_STS_Q6                        1
    #define       bAVIO_BCM_FULL_STS_Q7                        1
    #define       bAVIO_BCM_FULL_STS_Q8                        1
    #define       bAVIO_BCM_FULL_STS_Q9                        1
    #define       bAVIO_BCM_FULL_STS_Q10                       1
    #define       bAVIO_BCM_FULL_STS_Q11                       1
    #define       bAVIO_BCM_FULL_STS_Q12                       1
    #define       bAVIO_BCM_FULL_STS_Q13                       1
    #define       bAVIO_BCM_FULL_STS_Q14                       1
    #define       bAVIO_BCM_FULL_STS_Q15                       1
    #define       bAVIO_BCM_FULL_STS_Q16                       1
    #define       bAVIO_BCM_FULL_STS_Q17                       1
    #define       bAVIO_BCM_FULL_STS_Q18                       1
    #define       bAVIO_BCM_FULL_STS_Q31                       1
    #define     RA_AVIO_BCM_EMP_STS                            0x0148
    #define       bAVIO_BCM_EMP_STS_Q0                         1
    #define       bAVIO_BCM_EMP_STS_Q1                         1
    #define       bAVIO_BCM_EMP_STS_Q2                         1
    #define       bAVIO_BCM_EMP_STS_Q3                         1
    #define       bAVIO_BCM_EMP_STS_Q4                         1
    #define       bAVIO_BCM_EMP_STS_Q5                         1
    #define       bAVIO_BCM_EMP_STS_Q6                         1
    #define       bAVIO_BCM_EMP_STS_Q7                         1
    #define       bAVIO_BCM_EMP_STS_Q8                         1
    #define       bAVIO_BCM_EMP_STS_Q9                         1
    #define       bAVIO_BCM_EMP_STS_Q10                        1
    #define       bAVIO_BCM_EMP_STS_Q11                        1
    #define       bAVIO_BCM_EMP_STS_Q12                        1
    #define       bAVIO_BCM_EMP_STS_Q13                        1
    #define       bAVIO_BCM_EMP_STS_Q14                        1
    #define       bAVIO_BCM_EMP_STS_Q15                        1
    #define       bAVIO_BCM_EMP_STS_Q16                        1
    #define       bAVIO_BCM_EMP_STS_Q17                        1
    #define       bAVIO_BCM_EMP_STS_Q18                        1
    #define       bAVIO_BCM_EMP_STS_Q31                        1
    #define     RA_AVIO_BCM_FLUSH                              0x014C
    #define       bAVIO_BCM_FLUSH_Q0                           1
    #define       bAVIO_BCM_FLUSH_Q1                           1
    #define       bAVIO_BCM_FLUSH_Q2                           1
    #define       bAVIO_BCM_FLUSH_Q3                           1
    #define       bAVIO_BCM_FLUSH_Q4                           1
    #define       bAVIO_BCM_FLUSH_Q5                           1
    #define       bAVIO_BCM_FLUSH_Q6                           1
    #define       bAVIO_BCM_FLUSH_Q7                           1
    #define       bAVIO_BCM_FLUSH_Q8                           1
    #define       bAVIO_BCM_FLUSH_Q9                           1
    #define       bAVIO_BCM_FLUSH_Q10                          1
    #define       bAVIO_BCM_FLUSH_Q11                          1
    #define       bAVIO_BCM_FLUSH_Q12                          1
    #define       bAVIO_BCM_FLUSH_Q13                          1
    #define       bAVIO_BCM_FLUSH_Q14                          1
    #define       bAVIO_BCM_FLUSH_Q15                          1
    #define       bAVIO_BCM_FLUSH_Q16                          1
    #define       bAVIO_BCM_FLUSH_Q17                          1
    #define       bAVIO_BCM_FLUSH_Q18                          1
    #define       bAVIO_BCM_FLUSH_Q31                          1
    #define     RA_AVIO_BCM_AUTOPUSH_CNT                       0x0150
    #define       bAVIO_BCM_AUTOPUSH_CNT_OCCURENCE             32
    #define     RA_AVIO_Q0                                     0x0154
    #define     RA_AVIO_Q1                                     0x0158
    #define     RA_AVIO_Q2                                     0x015C
    #define     RA_AVIO_Q3                                     0x0160
    #define     RA_AVIO_Q4                                     0x0164
    #define     RA_AVIO_Q5                                     0x0168
    #define     RA_AVIO_Q6                                     0x016C
    #define     RA_AVIO_Q7                                     0x0170
    #define     RA_AVIO_Q8                                     0x0174
    #define     RA_AVIO_Q9                                     0x0178
    #define     RA_AVIO_Q10                                    0x017C
    #define     RA_AVIO_Q11                                    0x0180
    #define     RA_AVIO_Q14                                    0x0184
    #define     RA_AVIO_Q15                                    0x0188
    #define     RA_AVIO_Q16                                    0x018C
    #define     RA_AVIO_Q17                                    0x0190
    #define     RA_AVIO_Q18                                    0x0194
    #define     RA_AVIO_BCM_AUTOPUSH                           0x0198
    #define       bAVIO_BCM_AUTOPUSH_Q0                        1
    #define       bAVIO_BCM_AUTOPUSH_Q1                        1
    #define       bAVIO_BCM_AUTOPUSH_Q2                        1
    #define       bAVIO_BCM_AUTOPUSH_Q3                        1
    #define       bAVIO_BCM_AUTOPUSH_Q4                        1
    #define       bAVIO_BCM_AUTOPUSH_Q5                        1
    #define       bAVIO_BCM_AUTOPUSH_Q6                        1
    #define       bAVIO_BCM_AUTOPUSH_Q7                        1
    #define       bAVIO_BCM_AUTOPUSH_Q8                        1
    #define       bAVIO_BCM_AUTOPUSH_Q9                        1
    #define       bAVIO_BCM_AUTOPUSH_Q10                       1
    #define       bAVIO_BCM_AUTOPUSH_Q11                       1
    #define       bAVIO_BCM_AUTOPUSH_Q12                       1
    #define       bAVIO_BCM_AUTOPUSH_Q13                       1
    #define       bAVIO_BCM_AUTOPUSH_Q14                       1
    #define       bAVIO_BCM_AUTOPUSH_Q15                       1
    #define       bAVIO_BCM_AUTOPUSH_Q16                       1
    #define       bAVIO_BCM_AUTOPUSH_Q17                       1
    #define       bAVIO_BCM_AUTOPUSH_Q18                       1
    #define     RA_AVIO_BCM_FULL_STS_STICKY                    0x019C
    #define       bAVIO_BCM_FULL_STS_STICKY_Q0                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q1                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q2                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q3                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q4                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q5                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q6                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q7                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q8                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q9                 1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q10                1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q11                1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q12                1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q13                1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q14                1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q15                1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q16                1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q17                1
    #define       bAVIO_BCM_FULL_STS_STICKY_Q18                1
    #define     RA_AVIO_BCM_ERROR                              0x01A0
    #define       bAVIO_BCM_ERROR_err                          1
    #define     RA_AVIO_BCM_LOG_ADDR                           0x01A4
    #define       bAVIO_BCM_LOG_ADDR_addr                      32
    #define     RA_AVIO_BCM_ERROR_DATA                         0x01A8
    #define       bAVIO_BCM_ERROR_DATA_data                    32
    typedef struct SIE_AVIO {
              SIE_AVIO_REG                                     ie_cfgReg;
    #define   SET32AVIO_BCM_Q0_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q0_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q0                                 {\
            UNSG32 uBCM_Q0_mux                                 :  5;\
            UNSG32 RSVDx100_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q0;
            struct w32AVIO_BCM_Q0;
          };
    #define   SET32AVIO_BCM_Q1_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q1_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q1                                 {\
            UNSG32 uBCM_Q1_mux                                 :  5;\
            UNSG32 RSVDx104_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q1;
            struct w32AVIO_BCM_Q1;
          };
    #define   SET32AVIO_BCM_Q2_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q2_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q2                                 {\
            UNSG32 uBCM_Q2_mux                                 :  5;\
            UNSG32 RSVDx108_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q2;
            struct w32AVIO_BCM_Q2;
          };
    #define   SET32AVIO_BCM_Q3_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q3_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q3                                 {\
            UNSG32 uBCM_Q3_mux                                 :  5;\
            UNSG32 RSVDx10C_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q3;
            struct w32AVIO_BCM_Q3;
          };
    #define   SET32AVIO_BCM_Q4_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q4_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q4                                 {\
            UNSG32 uBCM_Q4_mux                                 :  5;\
            UNSG32 RSVDx110_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q4;
            struct w32AVIO_BCM_Q4;
          };
    #define   SET32AVIO_BCM_Q5_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q5_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q5                                 {\
            UNSG32 uBCM_Q5_mux                                 :  5;\
            UNSG32 RSVDx114_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q5;
            struct w32AVIO_BCM_Q5;
          };
    #define   SET32AVIO_BCM_Q6_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q6_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q6                                 {\
            UNSG32 uBCM_Q6_mux                                 :  5;\
            UNSG32 RSVDx118_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q6;
            struct w32AVIO_BCM_Q6;
          };
    #define   SET32AVIO_BCM_Q7_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q7_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q7                                 {\
            UNSG32 uBCM_Q7_mux                                 :  5;\
            UNSG32 RSVDx11C_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q7;
            struct w32AVIO_BCM_Q7;
          };
    #define   SET32AVIO_BCM_Q8_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q8_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q8                                 {\
            UNSG32 uBCM_Q8_mux                                 :  5;\
            UNSG32 RSVDx120_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q8;
            struct w32AVIO_BCM_Q8;
          };
    #define   SET32AVIO_BCM_Q9_mux(r32,v)                      _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q9_mux(r16,v)                      _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q9                                 {\
            UNSG32 uBCM_Q9_mux                                 :  5;\
            UNSG32 RSVDx124_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q9;
            struct w32AVIO_BCM_Q9;
          };
    #define   SET32AVIO_BCM_Q10_mux(r32,v)                     _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q10_mux(r16,v)                     _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q10                                {\
            UNSG32 uBCM_Q10_mux                                :  5;\
            UNSG32 RSVDx128_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q10;
            struct w32AVIO_BCM_Q10;
          };
    #define   SET32AVIO_BCM_Q11_mux(r32,v)                     _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q11_mux(r16,v)                     _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q11                                {\
            UNSG32 uBCM_Q11_mux                                :  5;\
            UNSG32 RSVDx12C_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q11;
            struct w32AVIO_BCM_Q11;
          };
    #define   SET32AVIO_BCM_Q14_mux(r32,v)                     _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q14_mux(r16,v)                     _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q14                                {\
            UNSG32 uBCM_Q14_mux                                :  5;\
            UNSG32 RSVDx130_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q14;
            struct w32AVIO_BCM_Q14;
          };
    #define   SET32AVIO_BCM_Q15_mux(r32,v)                     _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q15_mux(r16,v)                     _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q15                                {\
            UNSG32 uBCM_Q15_mux                                :  5;\
            UNSG32 RSVDx134_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q15;
            struct w32AVIO_BCM_Q15;
          };
    #define   SET32AVIO_BCM_Q16_mux(r32,v)                     _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q16_mux(r16,v)                     _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q16                                {\
            UNSG32 uBCM_Q16_mux                                :  5;\
            UNSG32 RSVDx138_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q16;
            struct w32AVIO_BCM_Q16;
          };
    #define   SET32AVIO_BCM_Q17_mux(r32,v)                     _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q17_mux(r16,v)                     _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q17                                {\
            UNSG32 uBCM_Q17_mux                                :  5;\
            UNSG32 RSVDx13C_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q17;
            struct w32AVIO_BCM_Q17;
          };
    #define   SET32AVIO_BCM_Q18_mux(r32,v)                     _BFSET_(r32, 4, 0,v)
    #define   SET16AVIO_BCM_Q18_mux(r16,v)                     _BFSET_(r16, 4, 0,v)
    #define     w32AVIO_BCM_Q18                                {\
            UNSG32 uBCM_Q18_mux                                :  5;\
            UNSG32 RSVDx140_b5                                 : 27;\
          }
    union { UNSG32 u32AVIO_BCM_Q18;
            struct w32AVIO_BCM_Q18;
          };
    #define   SET32AVIO_BCM_FULL_STS_Q0(r32,v)                 _BFSET_(r32, 0, 0,v)
    #define   SET16AVIO_BCM_FULL_STS_Q0(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_FULL_STS_Q1(r32,v)                 _BFSET_(r32, 1, 1,v)
    #define   SET16AVIO_BCM_FULL_STS_Q1(r16,v)                 _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_FULL_STS_Q2(r32,v)                 _BFSET_(r32, 2, 2,v)
    #define   SET16AVIO_BCM_FULL_STS_Q2(r16,v)                 _BFSET_(r16, 2, 2,v)
    #define   SET32AVIO_BCM_FULL_STS_Q3(r32,v)                 _BFSET_(r32, 3, 3,v)
    #define   SET16AVIO_BCM_FULL_STS_Q3(r16,v)                 _BFSET_(r16, 3, 3,v)
    #define   SET32AVIO_BCM_FULL_STS_Q4(r32,v)                 _BFSET_(r32, 4, 4,v)
    #define   SET16AVIO_BCM_FULL_STS_Q4(r16,v)                 _BFSET_(r16, 4, 4,v)
    #define   SET32AVIO_BCM_FULL_STS_Q5(r32,v)                 _BFSET_(r32, 5, 5,v)
    #define   SET16AVIO_BCM_FULL_STS_Q5(r16,v)                 _BFSET_(r16, 5, 5,v)
    #define   SET32AVIO_BCM_FULL_STS_Q6(r32,v)                 _BFSET_(r32, 6, 6,v)
    #define   SET16AVIO_BCM_FULL_STS_Q6(r16,v)                 _BFSET_(r16, 6, 6,v)
    #define   SET32AVIO_BCM_FULL_STS_Q7(r32,v)                 _BFSET_(r32, 7, 7,v)
    #define   SET16AVIO_BCM_FULL_STS_Q7(r16,v)                 _BFSET_(r16, 7, 7,v)
    #define   SET32AVIO_BCM_FULL_STS_Q8(r32,v)                 _BFSET_(r32, 8, 8,v)
    #define   SET16AVIO_BCM_FULL_STS_Q8(r16,v)                 _BFSET_(r16, 8, 8,v)
    #define   SET32AVIO_BCM_FULL_STS_Q9(r32,v)                 _BFSET_(r32, 9, 9,v)
    #define   SET16AVIO_BCM_FULL_STS_Q9(r16,v)                 _BFSET_(r16, 9, 9,v)
    #define   SET32AVIO_BCM_FULL_STS_Q10(r32,v)                _BFSET_(r32,10,10,v)
    #define   SET16AVIO_BCM_FULL_STS_Q10(r16,v)                _BFSET_(r16,10,10,v)
    #define   SET32AVIO_BCM_FULL_STS_Q11(r32,v)                _BFSET_(r32,11,11,v)
    #define   SET16AVIO_BCM_FULL_STS_Q11(r16,v)                _BFSET_(r16,11,11,v)
    #define   SET32AVIO_BCM_FULL_STS_Q12(r32,v)                _BFSET_(r32,12,12,v)
    #define   SET16AVIO_BCM_FULL_STS_Q12(r16,v)                _BFSET_(r16,12,12,v)
    #define   SET32AVIO_BCM_FULL_STS_Q13(r32,v)                _BFSET_(r32,13,13,v)
    #define   SET16AVIO_BCM_FULL_STS_Q13(r16,v)                _BFSET_(r16,13,13,v)
    #define   SET32AVIO_BCM_FULL_STS_Q14(r32,v)                _BFSET_(r32,14,14,v)
    #define   SET16AVIO_BCM_FULL_STS_Q14(r16,v)                _BFSET_(r16,14,14,v)
    #define   SET32AVIO_BCM_FULL_STS_Q15(r32,v)                _BFSET_(r32,15,15,v)
    #define   SET16AVIO_BCM_FULL_STS_Q15(r16,v)                _BFSET_(r16,15,15,v)
    #define   SET32AVIO_BCM_FULL_STS_Q16(r32,v)                _BFSET_(r32,16,16,v)
    #define   SET16AVIO_BCM_FULL_STS_Q16(r16,v)                _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_FULL_STS_Q17(r32,v)                _BFSET_(r32,17,17,v)
    #define   SET16AVIO_BCM_FULL_STS_Q17(r16,v)                _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_FULL_STS_Q18(r32,v)                _BFSET_(r32,18,18,v)
    #define   SET16AVIO_BCM_FULL_STS_Q18(r16,v)                _BFSET_(r16, 2, 2,v)
    #define   SET32AVIO_BCM_FULL_STS_Q31(r32,v)                _BFSET_(r32,19,19,v)
    #define   SET16AVIO_BCM_FULL_STS_Q31(r16,v)                _BFSET_(r16, 3, 3,v)
    #define     w32AVIO_BCM_FULL_STS                           {\
            UNSG32 uBCM_FULL_STS_Q0                            :  1;\
            UNSG32 uBCM_FULL_STS_Q1                            :  1;\
            UNSG32 uBCM_FULL_STS_Q2                            :  1;\
            UNSG32 uBCM_FULL_STS_Q3                            :  1;\
            UNSG32 uBCM_FULL_STS_Q4                            :  1;\
            UNSG32 uBCM_FULL_STS_Q5                            :  1;\
            UNSG32 uBCM_FULL_STS_Q6                            :  1;\
            UNSG32 uBCM_FULL_STS_Q7                            :  1;\
            UNSG32 uBCM_FULL_STS_Q8                            :  1;\
            UNSG32 uBCM_FULL_STS_Q9                            :  1;\
            UNSG32 uBCM_FULL_STS_Q10                           :  1;\
            UNSG32 uBCM_FULL_STS_Q11                           :  1;\
            UNSG32 uBCM_FULL_STS_Q12                           :  1;\
            UNSG32 uBCM_FULL_STS_Q13                           :  1;\
            UNSG32 uBCM_FULL_STS_Q14                           :  1;\
            UNSG32 uBCM_FULL_STS_Q15                           :  1;\
            UNSG32 uBCM_FULL_STS_Q16                           :  1;\
            UNSG32 uBCM_FULL_STS_Q17                           :  1;\
            UNSG32 uBCM_FULL_STS_Q18                           :  1;\
            UNSG32 uBCM_FULL_STS_Q31                           :  1;\
            UNSG32 RSVDx144_b20                                : 12;\
          }
    union { UNSG32 u32AVIO_BCM_FULL_STS;
            struct w32AVIO_BCM_FULL_STS;
          };
    #define   SET32AVIO_BCM_EMP_STS_Q0(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16AVIO_BCM_EMP_STS_Q0(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_EMP_STS_Q1(r32,v)                  _BFSET_(r32, 1, 1,v)
    #define   SET16AVIO_BCM_EMP_STS_Q1(r16,v)                  _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_EMP_STS_Q2(r32,v)                  _BFSET_(r32, 2, 2,v)
    #define   SET16AVIO_BCM_EMP_STS_Q2(r16,v)                  _BFSET_(r16, 2, 2,v)
    #define   SET32AVIO_BCM_EMP_STS_Q3(r32,v)                  _BFSET_(r32, 3, 3,v)
    #define   SET16AVIO_BCM_EMP_STS_Q3(r16,v)                  _BFSET_(r16, 3, 3,v)
    #define   SET32AVIO_BCM_EMP_STS_Q4(r32,v)                  _BFSET_(r32, 4, 4,v)
    #define   SET16AVIO_BCM_EMP_STS_Q4(r16,v)                  _BFSET_(r16, 4, 4,v)
    #define   SET32AVIO_BCM_EMP_STS_Q5(r32,v)                  _BFSET_(r32, 5, 5,v)
    #define   SET16AVIO_BCM_EMP_STS_Q5(r16,v)                  _BFSET_(r16, 5, 5,v)
    #define   SET32AVIO_BCM_EMP_STS_Q6(r32,v)                  _BFSET_(r32, 6, 6,v)
    #define   SET16AVIO_BCM_EMP_STS_Q6(r16,v)                  _BFSET_(r16, 6, 6,v)
    #define   SET32AVIO_BCM_EMP_STS_Q7(r32,v)                  _BFSET_(r32, 7, 7,v)
    #define   SET16AVIO_BCM_EMP_STS_Q7(r16,v)                  _BFSET_(r16, 7, 7,v)
    #define   SET32AVIO_BCM_EMP_STS_Q8(r32,v)                  _BFSET_(r32, 8, 8,v)
    #define   SET16AVIO_BCM_EMP_STS_Q8(r16,v)                  _BFSET_(r16, 8, 8,v)
    #define   SET32AVIO_BCM_EMP_STS_Q9(r32,v)                  _BFSET_(r32, 9, 9,v)
    #define   SET16AVIO_BCM_EMP_STS_Q9(r16,v)                  _BFSET_(r16, 9, 9,v)
    #define   SET32AVIO_BCM_EMP_STS_Q10(r32,v)                 _BFSET_(r32,10,10,v)
    #define   SET16AVIO_BCM_EMP_STS_Q10(r16,v)                 _BFSET_(r16,10,10,v)
    #define   SET32AVIO_BCM_EMP_STS_Q11(r32,v)                 _BFSET_(r32,11,11,v)
    #define   SET16AVIO_BCM_EMP_STS_Q11(r16,v)                 _BFSET_(r16,11,11,v)
    #define   SET32AVIO_BCM_EMP_STS_Q12(r32,v)                 _BFSET_(r32,12,12,v)
    #define   SET16AVIO_BCM_EMP_STS_Q12(r16,v)                 _BFSET_(r16,12,12,v)
    #define   SET32AVIO_BCM_EMP_STS_Q13(r32,v)                 _BFSET_(r32,13,13,v)
    #define   SET16AVIO_BCM_EMP_STS_Q13(r16,v)                 _BFSET_(r16,13,13,v)
    #define   SET32AVIO_BCM_EMP_STS_Q14(r32,v)                 _BFSET_(r32,14,14,v)
    #define   SET16AVIO_BCM_EMP_STS_Q14(r16,v)                 _BFSET_(r16,14,14,v)
    #define   SET32AVIO_BCM_EMP_STS_Q15(r32,v)                 _BFSET_(r32,15,15,v)
    #define   SET16AVIO_BCM_EMP_STS_Q15(r16,v)                 _BFSET_(r16,15,15,v)
    #define   SET32AVIO_BCM_EMP_STS_Q16(r32,v)                 _BFSET_(r32,16,16,v)
    #define   SET16AVIO_BCM_EMP_STS_Q16(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_EMP_STS_Q17(r32,v)                 _BFSET_(r32,17,17,v)
    #define   SET16AVIO_BCM_EMP_STS_Q17(r16,v)                 _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_EMP_STS_Q18(r32,v)                 _BFSET_(r32,18,18,v)
    #define   SET16AVIO_BCM_EMP_STS_Q18(r16,v)                 _BFSET_(r16, 2, 2,v)
    #define   SET32AVIO_BCM_EMP_STS_Q31(r32,v)                 _BFSET_(r32,19,19,v)
    #define   SET16AVIO_BCM_EMP_STS_Q31(r16,v)                 _BFSET_(r16, 3, 3,v)
    #define     w32AVIO_BCM_EMP_STS                            {\
            UNSG32 uBCM_EMP_STS_Q0                             :  1;\
            UNSG32 uBCM_EMP_STS_Q1                             :  1;\
            UNSG32 uBCM_EMP_STS_Q2                             :  1;\
            UNSG32 uBCM_EMP_STS_Q3                             :  1;\
            UNSG32 uBCM_EMP_STS_Q4                             :  1;\
            UNSG32 uBCM_EMP_STS_Q5                             :  1;\
            UNSG32 uBCM_EMP_STS_Q6                             :  1;\
            UNSG32 uBCM_EMP_STS_Q7                             :  1;\
            UNSG32 uBCM_EMP_STS_Q8                             :  1;\
            UNSG32 uBCM_EMP_STS_Q9                             :  1;\
            UNSG32 uBCM_EMP_STS_Q10                            :  1;\
            UNSG32 uBCM_EMP_STS_Q11                            :  1;\
            UNSG32 uBCM_EMP_STS_Q12                            :  1;\
            UNSG32 uBCM_EMP_STS_Q13                            :  1;\
            UNSG32 uBCM_EMP_STS_Q14                            :  1;\
            UNSG32 uBCM_EMP_STS_Q15                            :  1;\
            UNSG32 uBCM_EMP_STS_Q16                            :  1;\
            UNSG32 uBCM_EMP_STS_Q17                            :  1;\
            UNSG32 uBCM_EMP_STS_Q18                            :  1;\
            UNSG32 uBCM_EMP_STS_Q31                            :  1;\
            UNSG32 RSVDx148_b20                                : 12;\
          }
    union { UNSG32 u32AVIO_BCM_EMP_STS;
            struct w32AVIO_BCM_EMP_STS;
          };
    #define   SET32AVIO_BCM_FLUSH_Q0(r32,v)                    _BFSET_(r32, 0, 0,v)
    #define   SET16AVIO_BCM_FLUSH_Q0(r16,v)                    _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_FLUSH_Q1(r32,v)                    _BFSET_(r32, 1, 1,v)
    #define   SET16AVIO_BCM_FLUSH_Q1(r16,v)                    _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_FLUSH_Q2(r32,v)                    _BFSET_(r32, 2, 2,v)
    #define   SET16AVIO_BCM_FLUSH_Q2(r16,v)                    _BFSET_(r16, 2, 2,v)
    #define   SET32AVIO_BCM_FLUSH_Q3(r32,v)                    _BFSET_(r32, 3, 3,v)
    #define   SET16AVIO_BCM_FLUSH_Q3(r16,v)                    _BFSET_(r16, 3, 3,v)
    #define   SET32AVIO_BCM_FLUSH_Q4(r32,v)                    _BFSET_(r32, 4, 4,v)
    #define   SET16AVIO_BCM_FLUSH_Q4(r16,v)                    _BFSET_(r16, 4, 4,v)
    #define   SET32AVIO_BCM_FLUSH_Q5(r32,v)                    _BFSET_(r32, 5, 5,v)
    #define   SET16AVIO_BCM_FLUSH_Q5(r16,v)                    _BFSET_(r16, 5, 5,v)
    #define   SET32AVIO_BCM_FLUSH_Q6(r32,v)                    _BFSET_(r32, 6, 6,v)
    #define   SET16AVIO_BCM_FLUSH_Q6(r16,v)                    _BFSET_(r16, 6, 6,v)
    #define   SET32AVIO_BCM_FLUSH_Q7(r32,v)                    _BFSET_(r32, 7, 7,v)
    #define   SET16AVIO_BCM_FLUSH_Q7(r16,v)                    _BFSET_(r16, 7, 7,v)
    #define   SET32AVIO_BCM_FLUSH_Q8(r32,v)                    _BFSET_(r32, 8, 8,v)
    #define   SET16AVIO_BCM_FLUSH_Q8(r16,v)                    _BFSET_(r16, 8, 8,v)
    #define   SET32AVIO_BCM_FLUSH_Q9(r32,v)                    _BFSET_(r32, 9, 9,v)
    #define   SET16AVIO_BCM_FLUSH_Q9(r16,v)                    _BFSET_(r16, 9, 9,v)
    #define   SET32AVIO_BCM_FLUSH_Q10(r32,v)                   _BFSET_(r32,10,10,v)
    #define   SET16AVIO_BCM_FLUSH_Q10(r16,v)                   _BFSET_(r16,10,10,v)
    #define   SET32AVIO_BCM_FLUSH_Q11(r32,v)                   _BFSET_(r32,11,11,v)
    #define   SET16AVIO_BCM_FLUSH_Q11(r16,v)                   _BFSET_(r16,11,11,v)
    #define   SET32AVIO_BCM_FLUSH_Q12(r32,v)                   _BFSET_(r32,12,12,v)
    #define   SET16AVIO_BCM_FLUSH_Q12(r16,v)                   _BFSET_(r16,12,12,v)
    #define   SET32AVIO_BCM_FLUSH_Q13(r32,v)                   _BFSET_(r32,13,13,v)
    #define   SET16AVIO_BCM_FLUSH_Q13(r16,v)                   _BFSET_(r16,13,13,v)
    #define   SET32AVIO_BCM_FLUSH_Q14(r32,v)                   _BFSET_(r32,14,14,v)
    #define   SET16AVIO_BCM_FLUSH_Q14(r16,v)                   _BFSET_(r16,14,14,v)
    #define   SET32AVIO_BCM_FLUSH_Q15(r32,v)                   _BFSET_(r32,15,15,v)
    #define   SET16AVIO_BCM_FLUSH_Q15(r16,v)                   _BFSET_(r16,15,15,v)
    #define   SET32AVIO_BCM_FLUSH_Q16(r32,v)                   _BFSET_(r32,16,16,v)
    #define   SET16AVIO_BCM_FLUSH_Q16(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_FLUSH_Q17(r32,v)                   _BFSET_(r32,17,17,v)
    #define   SET16AVIO_BCM_FLUSH_Q17(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_FLUSH_Q18(r32,v)                   _BFSET_(r32,18,18,v)
    #define   SET16AVIO_BCM_FLUSH_Q18(r16,v)                   _BFSET_(r16, 2, 2,v)
    #define   SET32AVIO_BCM_FLUSH_Q31(r32,v)                   _BFSET_(r32,19,19,v)
    #define   SET16AVIO_BCM_FLUSH_Q31(r16,v)                   _BFSET_(r16, 3, 3,v)
    #define     w32AVIO_BCM_FLUSH                              {\
            UNSG32 uBCM_FLUSH_Q0                               :  1;\
            UNSG32 uBCM_FLUSH_Q1                               :  1;\
            UNSG32 uBCM_FLUSH_Q2                               :  1;\
            UNSG32 uBCM_FLUSH_Q3                               :  1;\
            UNSG32 uBCM_FLUSH_Q4                               :  1;\
            UNSG32 uBCM_FLUSH_Q5                               :  1;\
            UNSG32 uBCM_FLUSH_Q6                               :  1;\
            UNSG32 uBCM_FLUSH_Q7                               :  1;\
            UNSG32 uBCM_FLUSH_Q8                               :  1;\
            UNSG32 uBCM_FLUSH_Q9                               :  1;\
            UNSG32 uBCM_FLUSH_Q10                              :  1;\
            UNSG32 uBCM_FLUSH_Q11                              :  1;\
            UNSG32 uBCM_FLUSH_Q12                              :  1;\
            UNSG32 uBCM_FLUSH_Q13                              :  1;\
            UNSG32 uBCM_FLUSH_Q14                              :  1;\
            UNSG32 uBCM_FLUSH_Q15                              :  1;\
            UNSG32 uBCM_FLUSH_Q16                              :  1;\
            UNSG32 uBCM_FLUSH_Q17                              :  1;\
            UNSG32 uBCM_FLUSH_Q18                              :  1;\
            UNSG32 uBCM_FLUSH_Q31                              :  1;\
            UNSG32 RSVDx14C_b20                                : 12;\
          }
    union { UNSG32 u32AVIO_BCM_FLUSH;
            struct w32AVIO_BCM_FLUSH;
          };
    #define   SET32AVIO_BCM_AUTOPUSH_CNT_OCCURENCE(r32,v)      _BFSET_(r32,31, 0,v)
    #define     w32AVIO_BCM_AUTOPUSH_CNT                       {\
            UNSG32 uBCM_AUTOPUSH_CNT_OCCURENCE                 : 32;\
          }
    union { UNSG32 u32AVIO_BCM_AUTOPUSH_CNT;
            struct w32AVIO_BCM_AUTOPUSH_CNT;
          };
              SIE_OCCURENCE                                    ie_Q0;
              SIE_OCCURENCE                                    ie_Q1;
              SIE_OCCURENCE                                    ie_Q2;
              SIE_OCCURENCE                                    ie_Q3;
              SIE_OCCURENCE                                    ie_Q4;
              SIE_OCCURENCE                                    ie_Q5;
              SIE_OCCURENCE                                    ie_Q6;
              SIE_OCCURENCE                                    ie_Q7;
              SIE_OCCURENCE                                    ie_Q8;
              SIE_OCCURENCE                                    ie_Q9;
              SIE_OCCURENCE                                    ie_Q10;
              SIE_OCCURENCE                                    ie_Q11;
              SIE_OCCURENCE                                    ie_Q14;
              SIE_OCCURENCE                                    ie_Q15;
              SIE_OCCURENCE                                    ie_Q16;
              SIE_OCCURENCE                                    ie_Q17;
              SIE_OCCURENCE                                    ie_Q18;
    #define   SET32AVIO_BCM_AUTOPUSH_Q0(r32,v)                 _BFSET_(r32, 0, 0,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q0(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q1(r32,v)                 _BFSET_(r32, 1, 1,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q1(r16,v)                 _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q2(r32,v)                 _BFSET_(r32, 2, 2,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q2(r16,v)                 _BFSET_(r16, 2, 2,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q3(r32,v)                 _BFSET_(r32, 3, 3,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q3(r16,v)                 _BFSET_(r16, 3, 3,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q4(r32,v)                 _BFSET_(r32, 4, 4,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q4(r16,v)                 _BFSET_(r16, 4, 4,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q5(r32,v)                 _BFSET_(r32, 5, 5,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q5(r16,v)                 _BFSET_(r16, 5, 5,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q6(r32,v)                 _BFSET_(r32, 6, 6,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q6(r16,v)                 _BFSET_(r16, 6, 6,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q7(r32,v)                 _BFSET_(r32, 7, 7,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q7(r16,v)                 _BFSET_(r16, 7, 7,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q8(r32,v)                 _BFSET_(r32, 8, 8,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q8(r16,v)                 _BFSET_(r16, 8, 8,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q9(r32,v)                 _BFSET_(r32, 9, 9,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q9(r16,v)                 _BFSET_(r16, 9, 9,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q10(r32,v)                _BFSET_(r32,10,10,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q10(r16,v)                _BFSET_(r16,10,10,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q11(r32,v)                _BFSET_(r32,11,11,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q11(r16,v)                _BFSET_(r16,11,11,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q12(r32,v)                _BFSET_(r32,12,12,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q12(r16,v)                _BFSET_(r16,12,12,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q13(r32,v)                _BFSET_(r32,13,13,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q13(r16,v)                _BFSET_(r16,13,13,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q14(r32,v)                _BFSET_(r32,14,14,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q14(r16,v)                _BFSET_(r16,14,14,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q15(r32,v)                _BFSET_(r32,15,15,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q15(r16,v)                _BFSET_(r16,15,15,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q16(r32,v)                _BFSET_(r32,16,16,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q16(r16,v)                _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q17(r32,v)                _BFSET_(r32,17,17,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q17(r16,v)                _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_AUTOPUSH_Q18(r32,v)                _BFSET_(r32,18,18,v)
    #define   SET16AVIO_BCM_AUTOPUSH_Q18(r16,v)                _BFSET_(r16, 2, 2,v)
    #define     w32AVIO_BCM_AUTOPUSH                           {\
            UNSG32 uBCM_AUTOPUSH_Q0                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q1                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q2                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q3                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q4                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q5                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q6                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q7                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q8                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q9                            :  1;\
            UNSG32 uBCM_AUTOPUSH_Q10                           :  1;\
            UNSG32 uBCM_AUTOPUSH_Q11                           :  1;\
            UNSG32 uBCM_AUTOPUSH_Q12                           :  1;\
            UNSG32 uBCM_AUTOPUSH_Q13                           :  1;\
            UNSG32 uBCM_AUTOPUSH_Q14                           :  1;\
            UNSG32 uBCM_AUTOPUSH_Q15                           :  1;\
            UNSG32 uBCM_AUTOPUSH_Q16                           :  1;\
            UNSG32 uBCM_AUTOPUSH_Q17                           :  1;\
            UNSG32 uBCM_AUTOPUSH_Q18                           :  1;\
            UNSG32 RSVDx198_b19                                : 13;\
          }
    union { UNSG32 u32AVIO_BCM_AUTOPUSH;
            struct w32AVIO_BCM_AUTOPUSH;
          };
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q0(r32,v)          _BFSET_(r32, 0, 0,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q0(r16,v)          _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q1(r32,v)          _BFSET_(r32, 1, 1,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q1(r16,v)          _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q2(r32,v)          _BFSET_(r32, 2, 2,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q2(r16,v)          _BFSET_(r16, 2, 2,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q3(r32,v)          _BFSET_(r32, 3, 3,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q3(r16,v)          _BFSET_(r16, 3, 3,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q4(r32,v)          _BFSET_(r32, 4, 4,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q4(r16,v)          _BFSET_(r16, 4, 4,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q5(r32,v)          _BFSET_(r32, 5, 5,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q5(r16,v)          _BFSET_(r16, 5, 5,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q6(r32,v)          _BFSET_(r32, 6, 6,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q6(r16,v)          _BFSET_(r16, 6, 6,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q7(r32,v)          _BFSET_(r32, 7, 7,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q7(r16,v)          _BFSET_(r16, 7, 7,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q8(r32,v)          _BFSET_(r32, 8, 8,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q8(r16,v)          _BFSET_(r16, 8, 8,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q9(r32,v)          _BFSET_(r32, 9, 9,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q9(r16,v)          _BFSET_(r16, 9, 9,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q10(r32,v)         _BFSET_(r32,10,10,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q10(r16,v)         _BFSET_(r16,10,10,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q11(r32,v)         _BFSET_(r32,11,11,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q11(r16,v)         _BFSET_(r16,11,11,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q12(r32,v)         _BFSET_(r32,12,12,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q12(r16,v)         _BFSET_(r16,12,12,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q13(r32,v)         _BFSET_(r32,13,13,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q13(r16,v)         _BFSET_(r16,13,13,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q14(r32,v)         _BFSET_(r32,14,14,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q14(r16,v)         _BFSET_(r16,14,14,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q15(r32,v)         _BFSET_(r32,15,15,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q15(r16,v)         _BFSET_(r16,15,15,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q16(r32,v)         _BFSET_(r32,16,16,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q16(r16,v)         _BFSET_(r16, 0, 0,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q17(r32,v)         _BFSET_(r32,17,17,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q17(r16,v)         _BFSET_(r16, 1, 1,v)
    #define   SET32AVIO_BCM_FULL_STS_STICKY_Q18(r32,v)         _BFSET_(r32,18,18,v)
    #define   SET16AVIO_BCM_FULL_STS_STICKY_Q18(r16,v)         _BFSET_(r16, 2, 2,v)
    #define     w32AVIO_BCM_FULL_STS_STICKY                    {\
            UNSG32 uBCM_FULL_STS_STICKY_Q0                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q1                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q2                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q3                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q4                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q5                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q6                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q7                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q8                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q9                     :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q10                    :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q11                    :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q12                    :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q13                    :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q14                    :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q15                    :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q16                    :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q17                    :  1;\
            UNSG32 uBCM_FULL_STS_STICKY_Q18                    :  1;\
            UNSG32 RSVDx19C_b19                                : 13;\
          }
    union { UNSG32 u32AVIO_BCM_FULL_STS_STICKY;
            struct w32AVIO_BCM_FULL_STS_STICKY;
          };
    #define   SET32AVIO_BCM_ERROR_err(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16AVIO_BCM_ERROR_err(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32AVIO_BCM_ERROR                              {\
            UNSG32 uBCM_ERROR_err                              :  1;\
            UNSG32 RSVDx1A0_b1                                 : 31;\
          }
    union { UNSG32 u32AVIO_BCM_ERROR;
            struct w32AVIO_BCM_ERROR;
          };
    #define   SET32AVIO_BCM_LOG_ADDR_addr(r32,v)               _BFSET_(r32,31, 0,v)
    #define     w32AVIO_BCM_LOG_ADDR                           {\
            UNSG32 uBCM_LOG_ADDR_addr                          : 32;\
          }
    union { UNSG32 u32AVIO_BCM_LOG_ADDR;
            struct w32AVIO_BCM_LOG_ADDR;
          };
    #define   SET32AVIO_BCM_ERROR_DATA_data(r32,v)             _BFSET_(r32,31, 0,v)
    #define     w32AVIO_BCM_ERROR_DATA                         {\
            UNSG32 uBCM_ERROR_DATA_data                        : 32;\
          }
    union { UNSG32 u32AVIO_BCM_ERROR_DATA;
            struct w32AVIO_BCM_ERROR_DATA;
          };
             UNSG8 RSVDx1AC                                    [84];
    } SIE_AVIO;
    typedef union  T32AVIO_BCM_Q0
          { UNSG32 u32;
            struct w32AVIO_BCM_Q0;
                 } T32AVIO_BCM_Q0;
    typedef union  T32AVIO_BCM_Q1
          { UNSG32 u32;
            struct w32AVIO_BCM_Q1;
                 } T32AVIO_BCM_Q1;
    typedef union  T32AVIO_BCM_Q2
          { UNSG32 u32;
            struct w32AVIO_BCM_Q2;
                 } T32AVIO_BCM_Q2;
    typedef union  T32AVIO_BCM_Q3
          { UNSG32 u32;
            struct w32AVIO_BCM_Q3;
                 } T32AVIO_BCM_Q3;
    typedef union  T32AVIO_BCM_Q4
          { UNSG32 u32;
            struct w32AVIO_BCM_Q4;
                 } T32AVIO_BCM_Q4;
    typedef union  T32AVIO_BCM_Q5
          { UNSG32 u32;
            struct w32AVIO_BCM_Q5;
                 } T32AVIO_BCM_Q5;
    typedef union  T32AVIO_BCM_Q6
          { UNSG32 u32;
            struct w32AVIO_BCM_Q6;
                 } T32AVIO_BCM_Q6;
    typedef union  T32AVIO_BCM_Q7
          { UNSG32 u32;
            struct w32AVIO_BCM_Q7;
                 } T32AVIO_BCM_Q7;
    typedef union  T32AVIO_BCM_Q8
          { UNSG32 u32;
            struct w32AVIO_BCM_Q8;
                 } T32AVIO_BCM_Q8;
    typedef union  T32AVIO_BCM_Q9
          { UNSG32 u32;
            struct w32AVIO_BCM_Q9;
                 } T32AVIO_BCM_Q9;
    typedef union  T32AVIO_BCM_Q10
          { UNSG32 u32;
            struct w32AVIO_BCM_Q10;
                 } T32AVIO_BCM_Q10;
    typedef union  T32AVIO_BCM_Q11
          { UNSG32 u32;
            struct w32AVIO_BCM_Q11;
                 } T32AVIO_BCM_Q11;
    typedef union  T32AVIO_BCM_Q14
          { UNSG32 u32;
            struct w32AVIO_BCM_Q14;
                 } T32AVIO_BCM_Q14;
    typedef union  T32AVIO_BCM_Q15
          { UNSG32 u32;
            struct w32AVIO_BCM_Q15;
                 } T32AVIO_BCM_Q15;
    typedef union  T32AVIO_BCM_Q16
          { UNSG32 u32;
            struct w32AVIO_BCM_Q16;
                 } T32AVIO_BCM_Q16;
    typedef union  T32AVIO_BCM_Q17
          { UNSG32 u32;
            struct w32AVIO_BCM_Q17;
                 } T32AVIO_BCM_Q17;
    typedef union  T32AVIO_BCM_Q18
          { UNSG32 u32;
            struct w32AVIO_BCM_Q18;
                 } T32AVIO_BCM_Q18;
    typedef union  T32AVIO_BCM_FULL_STS
          { UNSG32 u32;
            struct w32AVIO_BCM_FULL_STS;
                 } T32AVIO_BCM_FULL_STS;
    typedef union  T32AVIO_BCM_EMP_STS
          { UNSG32 u32;
            struct w32AVIO_BCM_EMP_STS;
                 } T32AVIO_BCM_EMP_STS;
    typedef union  T32AVIO_BCM_FLUSH
          { UNSG32 u32;
            struct w32AVIO_BCM_FLUSH;
                 } T32AVIO_BCM_FLUSH;
    typedef union  T32AVIO_BCM_AUTOPUSH_CNT
          { UNSG32 u32;
            struct w32AVIO_BCM_AUTOPUSH_CNT;
                 } T32AVIO_BCM_AUTOPUSH_CNT;
    typedef union  T32AVIO_BCM_AUTOPUSH
          { UNSG32 u32;
            struct w32AVIO_BCM_AUTOPUSH;
                 } T32AVIO_BCM_AUTOPUSH;
    typedef union  T32AVIO_BCM_FULL_STS_STICKY
          { UNSG32 u32;
            struct w32AVIO_BCM_FULL_STS_STICKY;
                 } T32AVIO_BCM_FULL_STS_STICKY;
    typedef union  T32AVIO_BCM_ERROR
          { UNSG32 u32;
            struct w32AVIO_BCM_ERROR;
                 } T32AVIO_BCM_ERROR;
    typedef union  T32AVIO_BCM_LOG_ADDR
          { UNSG32 u32;
            struct w32AVIO_BCM_LOG_ADDR;
                 } T32AVIO_BCM_LOG_ADDR;
    typedef union  T32AVIO_BCM_ERROR_DATA
          { UNSG32 u32;
            struct w32AVIO_BCM_ERROR_DATA;
                 } T32AVIO_BCM_ERROR_DATA;
    typedef union  TAVIO_BCM_Q0
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q0;
                   };
                 } TAVIO_BCM_Q0;
    typedef union  TAVIO_BCM_Q1
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q1;
                   };
                 } TAVIO_BCM_Q1;
    typedef union  TAVIO_BCM_Q2
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q2;
                   };
                 } TAVIO_BCM_Q2;
    typedef union  TAVIO_BCM_Q3
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q3;
                   };
                 } TAVIO_BCM_Q3;
    typedef union  TAVIO_BCM_Q4
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q4;
                   };
                 } TAVIO_BCM_Q4;
    typedef union  TAVIO_BCM_Q5
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q5;
                   };
                 } TAVIO_BCM_Q5;
    typedef union  TAVIO_BCM_Q6
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q6;
                   };
                 } TAVIO_BCM_Q6;
    typedef union  TAVIO_BCM_Q7
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q7;
                   };
                 } TAVIO_BCM_Q7;
    typedef union  TAVIO_BCM_Q8
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q8;
                   };
                 } TAVIO_BCM_Q8;
    typedef union  TAVIO_BCM_Q9
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q9;
                   };
                 } TAVIO_BCM_Q9;
    typedef union  TAVIO_BCM_Q10
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q10;
                   };
                 } TAVIO_BCM_Q10;
    typedef union  TAVIO_BCM_Q11
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q11;
                   };
                 } TAVIO_BCM_Q11;
    typedef union  TAVIO_BCM_Q14
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q14;
                   };
                 } TAVIO_BCM_Q14;
    typedef union  TAVIO_BCM_Q15
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q15;
                   };
                 } TAVIO_BCM_Q15;
    typedef union  TAVIO_BCM_Q16
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q16;
                   };
                 } TAVIO_BCM_Q16;
    typedef union  TAVIO_BCM_Q17
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q17;
                   };
                 } TAVIO_BCM_Q17;
    typedef union  TAVIO_BCM_Q18
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_Q18;
                   };
                 } TAVIO_BCM_Q18;
    typedef union  TAVIO_BCM_FULL_STS
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_FULL_STS;
                   };
                 } TAVIO_BCM_FULL_STS;
    typedef union  TAVIO_BCM_EMP_STS
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_EMP_STS;
                   };
                 } TAVIO_BCM_EMP_STS;
    typedef union  TAVIO_BCM_FLUSH
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_FLUSH;
                   };
                 } TAVIO_BCM_FLUSH;
    typedef union  TAVIO_BCM_AUTOPUSH_CNT
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_AUTOPUSH_CNT;
                   };
                 } TAVIO_BCM_AUTOPUSH_CNT;
    typedef union  TAVIO_BCM_AUTOPUSH
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_AUTOPUSH;
                   };
                 } TAVIO_BCM_AUTOPUSH;
    typedef union  TAVIO_BCM_FULL_STS_STICKY
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_FULL_STS_STICKY;
                   };
                 } TAVIO_BCM_FULL_STS_STICKY;
    typedef union  TAVIO_BCM_ERROR
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_ERROR;
                   };
                 } TAVIO_BCM_ERROR;
    typedef union  TAVIO_BCM_LOG_ADDR
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_LOG_ADDR;
                   };
                 } TAVIO_BCM_LOG_ADDR;
    typedef union  TAVIO_BCM_ERROR_DATA
          { UNSG32 u32[1];
            struct {
            struct w32AVIO_BCM_ERROR_DATA;
                   };
                 } TAVIO_BCM_ERROR_DATA;
     SIGN32 AVIO_drvrd(SIE_AVIO *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 AVIO_drvwr(SIE_AVIO *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void AVIO_reset(SIE_AVIO *p);
     SIGN32 AVIO_cmp  (SIE_AVIO *p, SIE_AVIO *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define AVIO_check(p,pie,pfx,hLOG) AVIO_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define AVIO_print(p,    pfx,hLOG) AVIO_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
