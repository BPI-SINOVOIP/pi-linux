#ifndef avioDhub_h
#define avioDhub_h (){}
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
#ifndef h_SemaINTR
#define h_SemaINTR (){}
    #define     RA_SemaINTR_mask                               0x0000
    #define       bSemaINTR_mask_empty                         1
    #define       bSemaINTR_mask_full                          1
    #define       bSemaINTR_mask_almostEmpty                   1
    #define       bSemaINTR_mask_almostFull                    1
    typedef struct SIE_SemaINTR {
    #define   SET32SemaINTR_mask_empty(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16SemaINTR_mask_empty(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32SemaINTR_mask_full(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16SemaINTR_mask_full(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define   SET32SemaINTR_mask_almostEmpty(r32,v)            _BFSET_(r32, 2, 2,v)
    #define   SET16SemaINTR_mask_almostEmpty(r16,v)            _BFSET_(r16, 2, 2,v)
    #define   SET32SemaINTR_mask_almostFull(r32,v)             _BFSET_(r32, 3, 3,v)
    #define   SET16SemaINTR_mask_almostFull(r16,v)             _BFSET_(r16, 3, 3,v)
    #define     w32SemaINTR_mask                               {\
            UNSG32 umask_empty                                 :  1;\
            UNSG32 umask_full                                  :  1;\
            UNSG32 umask_almostEmpty                           :  1;\
            UNSG32 umask_almostFull                            :  1;\
            UNSG32 RSVDx0_b4                                   : 28;\
          }
    union { UNSG32 u32SemaINTR_mask;
            struct w32SemaINTR_mask;
          };
    } SIE_SemaINTR;
    typedef union  T32SemaINTR_mask
          { UNSG32 u32;
            struct w32SemaINTR_mask;
                 } T32SemaINTR_mask;
    typedef union  TSemaINTR_mask
          { UNSG32 u32[1];
            struct {
            struct w32SemaINTR_mask;
                   };
                 } TSemaINTR_mask;
     SIGN32 SemaINTR_drvrd(SIE_SemaINTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SemaINTR_drvwr(SIE_SemaINTR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SemaINTR_reset(SIE_SemaINTR *p);
     SIGN32 SemaINTR_cmp  (SIE_SemaINTR *p, SIE_SemaINTR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SemaINTR_check(p,pie,pfx,hLOG) SemaINTR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SemaINTR_print(p,    pfx,hLOG) SemaINTR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_Semaphore
#define h_Semaphore (){}
    #define     RA_Semaphore_CFG                               0x0000
    #define       bSemaphore_CFG_DEPTH                         16
    #define     RA_Semaphore_INTR                              0x0004
    #define     RA_Semaphore_mask                              0x0010
    #define       bSemaphore_mask_full                         1
    #define       bSemaphore_mask_emp                          1
    #define     RA_Semaphore_thresh                            0x0014
    #define       bSemaphore_thresh_aFull                      2
    #define       bSemaphore_thresh_aEmp                       2
    typedef struct SIE_Semaphore {
    #define   SET32Semaphore_CFG_DEPTH(r32,v)                  _BFSET_(r32,15, 0,v)
    #define   SET16Semaphore_CFG_DEPTH(r16,v)                  _BFSET_(r16,15, 0,v)
    #define     w32Semaphore_CFG                               {\
            UNSG32 uCFG_DEPTH                                  : 16;\
            UNSG32 RSVDx0_b16                                  : 16;\
          }
    union { UNSG32 u32Semaphore_CFG;
            struct w32Semaphore_CFG;
          };
              SIE_SemaINTR                                     ie_INTR[3];
    #define   SET32Semaphore_mask_full(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16Semaphore_mask_full(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32Semaphore_mask_emp(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16Semaphore_mask_emp(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define     w32Semaphore_mask                              {\
            UNSG32 umask_full                                  :  1;\
            UNSG32 umask_emp                                   :  1;\
            UNSG32 RSVDx10_b2                                  : 30;\
          }
    union { UNSG32 u32Semaphore_mask;
            struct w32Semaphore_mask;
          };
    #define   SET32Semaphore_thresh_aFull(r32,v)               _BFSET_(r32, 1, 0,v)
    #define   SET16Semaphore_thresh_aFull(r16,v)               _BFSET_(r16, 1, 0,v)
    #define   SET32Semaphore_thresh_aEmp(r32,v)                _BFSET_(r32, 3, 2,v)
    #define   SET16Semaphore_thresh_aEmp(r16,v)                _BFSET_(r16, 3, 2,v)
    #define     w32Semaphore_thresh                            {\
            UNSG32 uthresh_aFull                               :  2;\
            UNSG32 uthresh_aEmp                                :  2;\
            UNSG32 RSVDx14_b4                                  : 28;\
          }
    union { UNSG32 u32Semaphore_thresh;
            struct w32Semaphore_thresh;
          };
    } SIE_Semaphore;
    typedef union  T32Semaphore_CFG
          { UNSG32 u32;
            struct w32Semaphore_CFG;
                 } T32Semaphore_CFG;
    typedef union  T32Semaphore_mask
          { UNSG32 u32;
            struct w32Semaphore_mask;
                 } T32Semaphore_mask;
    typedef union  T32Semaphore_thresh
          { UNSG32 u32;
            struct w32Semaphore_thresh;
                 } T32Semaphore_thresh;
    typedef union  TSemaphore_CFG
          { UNSG32 u32[1];
            struct {
            struct w32Semaphore_CFG;
                   };
                 } TSemaphore_CFG;
    typedef union  TSemaphore_mask
          { UNSG32 u32[1];
            struct {
            struct w32Semaphore_mask;
                   };
                 } TSemaphore_mask;
    typedef union  TSemaphore_thresh
          { UNSG32 u32[1];
            struct {
            struct w32Semaphore_thresh;
                   };
                 } TSemaphore_thresh;
     SIGN32 Semaphore_drvrd(SIE_Semaphore *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 Semaphore_drvwr(SIE_Semaphore *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void Semaphore_reset(SIE_Semaphore *p);
     SIGN32 Semaphore_cmp  (SIE_Semaphore *p, SIE_Semaphore *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define Semaphore_check(p,pie,pfx,hLOG) Semaphore_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define Semaphore_print(p,    pfx,hLOG) Semaphore_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SemaQuery
#define h_SemaQuery (){}
    #define     RA_SemaQuery_RESP                              0x0000
    #define       bSemaQuery_RESP_CNT                          16
    #define       bSemaQuery_RESP_PTR                          16
    typedef struct SIE_SemaQuery {
    #define   SET32SemaQuery_RESP_CNT(r32,v)                   _BFSET_(r32,15, 0,v)
    #define   SET16SemaQuery_RESP_CNT(r16,v)                   _BFSET_(r16,15, 0,v)
    #define   SET32SemaQuery_RESP_PTR(r32,v)                   _BFSET_(r32,31,16,v)
    #define   SET16SemaQuery_RESP_PTR(r16,v)                   _BFSET_(r16,15, 0,v)
    #define     w32SemaQuery_RESP                              {\
            UNSG32 uRESP_CNT                                   : 16;\
            UNSG32 uRESP_PTR                                   : 16;\
          }
    union { UNSG32 u32SemaQuery_RESP;
            struct w32SemaQuery_RESP;
          };
    } SIE_SemaQuery;
    typedef union  T32SemaQuery_RESP
          { UNSG32 u32;
            struct w32SemaQuery_RESP;
                 } T32SemaQuery_RESP;
    typedef union  TSemaQuery_RESP
          { UNSG32 u32[1];
            struct {
            struct w32SemaQuery_RESP;
                   };
                 } TSemaQuery_RESP;
     SIGN32 SemaQuery_drvrd(SIE_SemaQuery *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SemaQuery_drvwr(SIE_SemaQuery *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SemaQuery_reset(SIE_SemaQuery *p);
     SIGN32 SemaQuery_cmp  (SIE_SemaQuery *p, SIE_SemaQuery *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SemaQuery_check(p,pie,pfx,hLOG) SemaQuery_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SemaQuery_print(p,    pfx,hLOG) SemaQuery_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SemaQueryMap
#define h_SemaQueryMap (){}
    #define     RA_SemaQueryMap_ADDR                           0x0000
    #define       bSemaQueryMap_ADDR_byte                      2
    #define       bSemaQueryMap_ADDR_ID                        5
    #define       bSemaQueryMap_ADDR_master                    1
    #define        SemaQueryMap_ADDR_master_producer                        0x0
    #define        SemaQueryMap_ADDR_master_consumer                        0x1
    typedef struct SIE_SemaQueryMap {
    #define   SET32SemaQueryMap_ADDR_byte(r32,v)               _BFSET_(r32, 1, 0,v)
    #define   SET16SemaQueryMap_ADDR_byte(r16,v)               _BFSET_(r16, 1, 0,v)
    #define   SET32SemaQueryMap_ADDR_ID(r32,v)                 _BFSET_(r32, 6, 2,v)
    #define   SET16SemaQueryMap_ADDR_ID(r16,v)                 _BFSET_(r16, 6, 2,v)
    #define   SET32SemaQueryMap_ADDR_master(r32,v)             _BFSET_(r32, 7, 7,v)
    #define   SET16SemaQueryMap_ADDR_master(r16,v)             _BFSET_(r16, 7, 7,v)
    #define     w32SemaQueryMap_ADDR                           {\
            UNSG32 uADDR_byte                                  :  2;\
            UNSG32 uADDR_ID                                    :  5;\
            UNSG32 uADDR_master                                :  1;\
            UNSG32 RSVDx0_b8                                   : 24;\
          }
    union { UNSG32 u32SemaQueryMap_ADDR;
            struct w32SemaQueryMap_ADDR;
          };
    } SIE_SemaQueryMap;
    typedef union  T32SemaQueryMap_ADDR
          { UNSG32 u32;
            struct w32SemaQueryMap_ADDR;
                 } T32SemaQueryMap_ADDR;
    typedef union  TSemaQueryMap_ADDR
          { UNSG32 u32[1];
            struct {
            struct w32SemaQueryMap_ADDR;
                   };
                 } TSemaQueryMap_ADDR;
     SIGN32 SemaQueryMap_drvrd(SIE_SemaQueryMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SemaQueryMap_drvwr(SIE_SemaQueryMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SemaQueryMap_reset(SIE_SemaQueryMap *p);
     SIGN32 SemaQueryMap_cmp  (SIE_SemaQueryMap *p, SIE_SemaQueryMap *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SemaQueryMap_check(p,pie,pfx,hLOG) SemaQueryMap_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SemaQueryMap_print(p,    pfx,hLOG) SemaQueryMap_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_SemaHub
#define h_SemaHub (){}
    #define     RA_SemaHub_Query                               0x0000
    #define     RA_SemaHub_counter                             0x0000
    #define     RA_SemaHub_ARR                                 0x0100
    #define     RA_SemaHub_cell                                0x0100
    #define     RA_SemaHub_PUSH                                0x0400
    #define       bSemaHub_PUSH_ID                             8
    #define       bSemaHub_PUSH_delta                          8
    #define     RA_SemaHub_POP                                 0x0404
    #define       bSemaHub_POP_ID                              8
    #define       bSemaHub_POP_delta                           8
    #define     RA_SemaHub_empty                               0x0408
    #define       bSemaHub_empty_ST_0i                         1
    #define       bSemaHub_empty_ST_1i                         1
    #define       bSemaHub_empty_ST_2i                         1
    #define       bSemaHub_empty_ST_3i                         1
    #define       bSemaHub_empty_ST_4i                         1
    #define       bSemaHub_empty_ST_5i                         1
    #define       bSemaHub_empty_ST_6i                         1
    #define       bSemaHub_empty_ST_7i                         1
    #define       bSemaHub_empty_ST_8i                         1
    #define       bSemaHub_empty_ST_9i                         1
    #define       bSemaHub_empty_ST_10i                        1
    #define       bSemaHub_empty_ST_11i                        1
    #define       bSemaHub_empty_ST_12i                        1
    #define       bSemaHub_empty_ST_13i                        1
    #define       bSemaHub_empty_ST_14i                        1
    #define       bSemaHub_empty_ST_15i                        1
    #define       bSemaHub_empty_ST_16i                        1
    #define       bSemaHub_empty_ST_17i                        1
    #define       bSemaHub_empty_ST_18i                        1
    #define       bSemaHub_empty_ST_19i                        1
    #define       bSemaHub_empty_ST_20i                        1
    #define       bSemaHub_empty_ST_21i                        1
    #define       bSemaHub_empty_ST_22i                        1
    #define       bSemaHub_empty_ST_23i                        1
    #define       bSemaHub_empty_ST_24i                        1
    #define       bSemaHub_empty_ST_25i                        1
    #define       bSemaHub_empty_ST_26i                        1
    #define       bSemaHub_empty_ST_27i                        1
    #define       bSemaHub_empty_ST_28i                        1
    #define       bSemaHub_empty_ST_29i                        1
    #define       bSemaHub_empty_ST_30i                        1
    #define       bSemaHub_empty_ST_31i                        1
    #define     RA_SemaHub_full                                0x040C
    #define       bSemaHub_full_ST_0i                          1
    #define       bSemaHub_full_ST_1i                          1
    #define       bSemaHub_full_ST_2i                          1
    #define       bSemaHub_full_ST_3i                          1
    #define       bSemaHub_full_ST_4i                          1
    #define       bSemaHub_full_ST_5i                          1
    #define       bSemaHub_full_ST_6i                          1
    #define       bSemaHub_full_ST_7i                          1
    #define       bSemaHub_full_ST_8i                          1
    #define       bSemaHub_full_ST_9i                          1
    #define       bSemaHub_full_ST_10i                         1
    #define       bSemaHub_full_ST_11i                         1
    #define       bSemaHub_full_ST_12i                         1
    #define       bSemaHub_full_ST_13i                         1
    #define       bSemaHub_full_ST_14i                         1
    #define       bSemaHub_full_ST_15i                         1
    #define       bSemaHub_full_ST_16i                         1
    #define       bSemaHub_full_ST_17i                         1
    #define       bSemaHub_full_ST_18i                         1
    #define       bSemaHub_full_ST_19i                         1
    #define       bSemaHub_full_ST_20i                         1
    #define       bSemaHub_full_ST_21i                         1
    #define       bSemaHub_full_ST_22i                         1
    #define       bSemaHub_full_ST_23i                         1
    #define       bSemaHub_full_ST_24i                         1
    #define       bSemaHub_full_ST_25i                         1
    #define       bSemaHub_full_ST_26i                         1
    #define       bSemaHub_full_ST_27i                         1
    #define       bSemaHub_full_ST_28i                         1
    #define       bSemaHub_full_ST_29i                         1
    #define       bSemaHub_full_ST_30i                         1
    #define       bSemaHub_full_ST_31i                         1
    #define     RA_SemaHub_almostEmpty                         0x0410
    #define       bSemaHub_almostEmpty_ST_0i                   1
    #define       bSemaHub_almostEmpty_ST_1i                   1
    #define       bSemaHub_almostEmpty_ST_2i                   1
    #define       bSemaHub_almostEmpty_ST_3i                   1
    #define       bSemaHub_almostEmpty_ST_4i                   1
    #define       bSemaHub_almostEmpty_ST_5i                   1
    #define       bSemaHub_almostEmpty_ST_6i                   1
    #define       bSemaHub_almostEmpty_ST_7i                   1
    #define       bSemaHub_almostEmpty_ST_8i                   1
    #define       bSemaHub_almostEmpty_ST_9i                   1
    #define       bSemaHub_almostEmpty_ST_10i                  1
    #define       bSemaHub_almostEmpty_ST_11i                  1
    #define       bSemaHub_almostEmpty_ST_12i                  1
    #define       bSemaHub_almostEmpty_ST_13i                  1
    #define       bSemaHub_almostEmpty_ST_14i                  1
    #define       bSemaHub_almostEmpty_ST_15i                  1
    #define       bSemaHub_almostEmpty_ST_16i                  1
    #define       bSemaHub_almostEmpty_ST_17i                  1
    #define       bSemaHub_almostEmpty_ST_18i                  1
    #define       bSemaHub_almostEmpty_ST_19i                  1
    #define       bSemaHub_almostEmpty_ST_20i                  1
    #define       bSemaHub_almostEmpty_ST_21i                  1
    #define       bSemaHub_almostEmpty_ST_22i                  1
    #define       bSemaHub_almostEmpty_ST_23i                  1
    #define       bSemaHub_almostEmpty_ST_24i                  1
    #define       bSemaHub_almostEmpty_ST_25i                  1
    #define       bSemaHub_almostEmpty_ST_26i                  1
    #define       bSemaHub_almostEmpty_ST_27i                  1
    #define       bSemaHub_almostEmpty_ST_28i                  1
    #define       bSemaHub_almostEmpty_ST_29i                  1
    #define       bSemaHub_almostEmpty_ST_30i                  1
    #define       bSemaHub_almostEmpty_ST_31i                  1
    #define     RA_SemaHub_almostFull                          0x0414
    #define       bSemaHub_almostFull_ST_0i                    1
    #define       bSemaHub_almostFull_ST_1i                    1
    #define       bSemaHub_almostFull_ST_2i                    1
    #define       bSemaHub_almostFull_ST_3i                    1
    #define       bSemaHub_almostFull_ST_4i                    1
    #define       bSemaHub_almostFull_ST_5i                    1
    #define       bSemaHub_almostFull_ST_6i                    1
    #define       bSemaHub_almostFull_ST_7i                    1
    #define       bSemaHub_almostFull_ST_8i                    1
    #define       bSemaHub_almostFull_ST_9i                    1
    #define       bSemaHub_almostFull_ST_10i                   1
    #define       bSemaHub_almostFull_ST_11i                   1
    #define       bSemaHub_almostFull_ST_12i                   1
    #define       bSemaHub_almostFull_ST_13i                   1
    #define       bSemaHub_almostFull_ST_14i                   1
    #define       bSemaHub_almostFull_ST_15i                   1
    #define       bSemaHub_almostFull_ST_16i                   1
    #define       bSemaHub_almostFull_ST_17i                   1
    #define       bSemaHub_almostFull_ST_18i                   1
    #define       bSemaHub_almostFull_ST_19i                   1
    #define       bSemaHub_almostFull_ST_20i                   1
    #define       bSemaHub_almostFull_ST_21i                   1
    #define       bSemaHub_almostFull_ST_22i                   1
    #define       bSemaHub_almostFull_ST_23i                   1
    #define       bSemaHub_almostFull_ST_24i                   1
    #define       bSemaHub_almostFull_ST_25i                   1
    #define       bSemaHub_almostFull_ST_26i                   1
    #define       bSemaHub_almostFull_ST_27i                   1
    #define       bSemaHub_almostFull_ST_28i                   1
    #define       bSemaHub_almostFull_ST_29i                   1
    #define       bSemaHub_almostFull_ST_30i                   1
    #define       bSemaHub_almostFull_ST_31i                   1
    typedef struct SIE_SemaHub {
              SIE_SemaQuery                                    ie_counter[64];
              SIE_Semaphore                                    ie_cell[32];
    #define   SET32SemaHub_PUSH_ID(r32,v)                      _BFSET_(r32, 7, 0,v)
    #define   SET16SemaHub_PUSH_ID(r16,v)                      _BFSET_(r16, 7, 0,v)
    #define   SET32SemaHub_PUSH_delta(r32,v)                   _BFSET_(r32,15, 8,v)
    #define   SET16SemaHub_PUSH_delta(r16,v)                   _BFSET_(r16,15, 8,v)
    #define     w32SemaHub_PUSH                                {\
            UNSG32 uPUSH_ID                                    :  8;\
            UNSG32 uPUSH_delta                                 :  8;\
            UNSG32 RSVDx400_b16                                : 16;\
          }
    union { UNSG32 u32SemaHub_PUSH;
            struct w32SemaHub_PUSH;
          };
    #define   SET32SemaHub_POP_ID(r32,v)                       _BFSET_(r32, 7, 0,v)
    #define   SET16SemaHub_POP_ID(r16,v)                       _BFSET_(r16, 7, 0,v)
    #define   SET32SemaHub_POP_delta(r32,v)                    _BFSET_(r32,15, 8,v)
    #define   SET16SemaHub_POP_delta(r16,v)                    _BFSET_(r16,15, 8,v)
    #define     w32SemaHub_POP                                 {\
            UNSG32 uPOP_ID                                     :  8;\
            UNSG32 uPOP_delta                                  :  8;\
            UNSG32 RSVDx404_b16                                : 16;\
          }
    union { UNSG32 u32SemaHub_POP;
            struct w32SemaHub_POP;
          };
    #define   SET32SemaHub_empty_ST_0i(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16SemaHub_empty_ST_0i(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32SemaHub_empty_ST_1i(r32,v)                  _BFSET_(r32, 1, 1,v)
    #define   SET16SemaHub_empty_ST_1i(r16,v)                  _BFSET_(r16, 1, 1,v)
    #define   SET32SemaHub_empty_ST_2i(r32,v)                  _BFSET_(r32, 2, 2,v)
    #define   SET16SemaHub_empty_ST_2i(r16,v)                  _BFSET_(r16, 2, 2,v)
    #define   SET32SemaHub_empty_ST_3i(r32,v)                  _BFSET_(r32, 3, 3,v)
    #define   SET16SemaHub_empty_ST_3i(r16,v)                  _BFSET_(r16, 3, 3,v)
    #define   SET32SemaHub_empty_ST_4i(r32,v)                  _BFSET_(r32, 4, 4,v)
    #define   SET16SemaHub_empty_ST_4i(r16,v)                  _BFSET_(r16, 4, 4,v)
    #define   SET32SemaHub_empty_ST_5i(r32,v)                  _BFSET_(r32, 5, 5,v)
    #define   SET16SemaHub_empty_ST_5i(r16,v)                  _BFSET_(r16, 5, 5,v)
    #define   SET32SemaHub_empty_ST_6i(r32,v)                  _BFSET_(r32, 6, 6,v)
    #define   SET16SemaHub_empty_ST_6i(r16,v)                  _BFSET_(r16, 6, 6,v)
    #define   SET32SemaHub_empty_ST_7i(r32,v)                  _BFSET_(r32, 7, 7,v)
    #define   SET16SemaHub_empty_ST_7i(r16,v)                  _BFSET_(r16, 7, 7,v)
    #define   SET32SemaHub_empty_ST_8i(r32,v)                  _BFSET_(r32, 8, 8,v)
    #define   SET16SemaHub_empty_ST_8i(r16,v)                  _BFSET_(r16, 8, 8,v)
    #define   SET32SemaHub_empty_ST_9i(r32,v)                  _BFSET_(r32, 9, 9,v)
    #define   SET16SemaHub_empty_ST_9i(r16,v)                  _BFSET_(r16, 9, 9,v)
    #define   SET32SemaHub_empty_ST_10i(r32,v)                 _BFSET_(r32,10,10,v)
    #define   SET16SemaHub_empty_ST_10i(r16,v)                 _BFSET_(r16,10,10,v)
    #define   SET32SemaHub_empty_ST_11i(r32,v)                 _BFSET_(r32,11,11,v)
    #define   SET16SemaHub_empty_ST_11i(r16,v)                 _BFSET_(r16,11,11,v)
    #define   SET32SemaHub_empty_ST_12i(r32,v)                 _BFSET_(r32,12,12,v)
    #define   SET16SemaHub_empty_ST_12i(r16,v)                 _BFSET_(r16,12,12,v)
    #define   SET32SemaHub_empty_ST_13i(r32,v)                 _BFSET_(r32,13,13,v)
    #define   SET16SemaHub_empty_ST_13i(r16,v)                 _BFSET_(r16,13,13,v)
    #define   SET32SemaHub_empty_ST_14i(r32,v)                 _BFSET_(r32,14,14,v)
    #define   SET16SemaHub_empty_ST_14i(r16,v)                 _BFSET_(r16,14,14,v)
    #define   SET32SemaHub_empty_ST_15i(r32,v)                 _BFSET_(r32,15,15,v)
    #define   SET16SemaHub_empty_ST_15i(r16,v)                 _BFSET_(r16,15,15,v)
    #define   SET32SemaHub_empty_ST_16i(r32,v)                 _BFSET_(r32,16,16,v)
    #define   SET16SemaHub_empty_ST_16i(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define   SET32SemaHub_empty_ST_17i(r32,v)                 _BFSET_(r32,17,17,v)
    #define   SET16SemaHub_empty_ST_17i(r16,v)                 _BFSET_(r16, 1, 1,v)
    #define   SET32SemaHub_empty_ST_18i(r32,v)                 _BFSET_(r32,18,18,v)
    #define   SET16SemaHub_empty_ST_18i(r16,v)                 _BFSET_(r16, 2, 2,v)
    #define   SET32SemaHub_empty_ST_19i(r32,v)                 _BFSET_(r32,19,19,v)
    #define   SET16SemaHub_empty_ST_19i(r16,v)                 _BFSET_(r16, 3, 3,v)
    #define   SET32SemaHub_empty_ST_20i(r32,v)                 _BFSET_(r32,20,20,v)
    #define   SET16SemaHub_empty_ST_20i(r16,v)                 _BFSET_(r16, 4, 4,v)
    #define   SET32SemaHub_empty_ST_21i(r32,v)                 _BFSET_(r32,21,21,v)
    #define   SET16SemaHub_empty_ST_21i(r16,v)                 _BFSET_(r16, 5, 5,v)
    #define   SET32SemaHub_empty_ST_22i(r32,v)                 _BFSET_(r32,22,22,v)
    #define   SET16SemaHub_empty_ST_22i(r16,v)                 _BFSET_(r16, 6, 6,v)
    #define   SET32SemaHub_empty_ST_23i(r32,v)                 _BFSET_(r32,23,23,v)
    #define   SET16SemaHub_empty_ST_23i(r16,v)                 _BFSET_(r16, 7, 7,v)
    #define   SET32SemaHub_empty_ST_24i(r32,v)                 _BFSET_(r32,24,24,v)
    #define   SET16SemaHub_empty_ST_24i(r16,v)                 _BFSET_(r16, 8, 8,v)
    #define   SET32SemaHub_empty_ST_25i(r32,v)                 _BFSET_(r32,25,25,v)
    #define   SET16SemaHub_empty_ST_25i(r16,v)                 _BFSET_(r16, 9, 9,v)
    #define   SET32SemaHub_empty_ST_26i(r32,v)                 _BFSET_(r32,26,26,v)
    #define   SET16SemaHub_empty_ST_26i(r16,v)                 _BFSET_(r16,10,10,v)
    #define   SET32SemaHub_empty_ST_27i(r32,v)                 _BFSET_(r32,27,27,v)
    #define   SET16SemaHub_empty_ST_27i(r16,v)                 _BFSET_(r16,11,11,v)
    #define   SET32SemaHub_empty_ST_28i(r32,v)                 _BFSET_(r32,28,28,v)
    #define   SET16SemaHub_empty_ST_28i(r16,v)                 _BFSET_(r16,12,12,v)
    #define   SET32SemaHub_empty_ST_29i(r32,v)                 _BFSET_(r32,29,29,v)
    #define   SET16SemaHub_empty_ST_29i(r16,v)                 _BFSET_(r16,13,13,v)
    #define   SET32SemaHub_empty_ST_30i(r32,v)                 _BFSET_(r32,30,30,v)
    #define   SET16SemaHub_empty_ST_30i(r16,v)                 _BFSET_(r16,14,14,v)
    #define   SET32SemaHub_empty_ST_31i(r32,v)                 _BFSET_(r32,31,31,v)
    #define   SET16SemaHub_empty_ST_31i(r16,v)                 _BFSET_(r16,15,15,v)
    #define     w32SemaHub_empty                               {\
            UNSG32 uempty_ST_0i                                :  1;\
            UNSG32 uempty_ST_1i                                :  1;\
            UNSG32 uempty_ST_2i                                :  1;\
            UNSG32 uempty_ST_3i                                :  1;\
            UNSG32 uempty_ST_4i                                :  1;\
            UNSG32 uempty_ST_5i                                :  1;\
            UNSG32 uempty_ST_6i                                :  1;\
            UNSG32 uempty_ST_7i                                :  1;\
            UNSG32 uempty_ST_8i                                :  1;\
            UNSG32 uempty_ST_9i                                :  1;\
            UNSG32 uempty_ST_10i                               :  1;\
            UNSG32 uempty_ST_11i                               :  1;\
            UNSG32 uempty_ST_12i                               :  1;\
            UNSG32 uempty_ST_13i                               :  1;\
            UNSG32 uempty_ST_14i                               :  1;\
            UNSG32 uempty_ST_15i                               :  1;\
            UNSG32 uempty_ST_16i                               :  1;\
            UNSG32 uempty_ST_17i                               :  1;\
            UNSG32 uempty_ST_18i                               :  1;\
            UNSG32 uempty_ST_19i                               :  1;\
            UNSG32 uempty_ST_20i                               :  1;\
            UNSG32 uempty_ST_21i                               :  1;\
            UNSG32 uempty_ST_22i                               :  1;\
            UNSG32 uempty_ST_23i                               :  1;\
            UNSG32 uempty_ST_24i                               :  1;\
            UNSG32 uempty_ST_25i                               :  1;\
            UNSG32 uempty_ST_26i                               :  1;\
            UNSG32 uempty_ST_27i                               :  1;\
            UNSG32 uempty_ST_28i                               :  1;\
            UNSG32 uempty_ST_29i                               :  1;\
            UNSG32 uempty_ST_30i                               :  1;\
            UNSG32 uempty_ST_31i                               :  1;\
          }
    union { UNSG32 u32SemaHub_empty;
            struct w32SemaHub_empty;
          };
    #define   SET32SemaHub_full_ST_0i(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16SemaHub_full_ST_0i(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define   SET32SemaHub_full_ST_1i(r32,v)                   _BFSET_(r32, 1, 1,v)
    #define   SET16SemaHub_full_ST_1i(r16,v)                   _BFSET_(r16, 1, 1,v)
    #define   SET32SemaHub_full_ST_2i(r32,v)                   _BFSET_(r32, 2, 2,v)
    #define   SET16SemaHub_full_ST_2i(r16,v)                   _BFSET_(r16, 2, 2,v)
    #define   SET32SemaHub_full_ST_3i(r32,v)                   _BFSET_(r32, 3, 3,v)
    #define   SET16SemaHub_full_ST_3i(r16,v)                   _BFSET_(r16, 3, 3,v)
    #define   SET32SemaHub_full_ST_4i(r32,v)                   _BFSET_(r32, 4, 4,v)
    #define   SET16SemaHub_full_ST_4i(r16,v)                   _BFSET_(r16, 4, 4,v)
    #define   SET32SemaHub_full_ST_5i(r32,v)                   _BFSET_(r32, 5, 5,v)
    #define   SET16SemaHub_full_ST_5i(r16,v)                   _BFSET_(r16, 5, 5,v)
    #define   SET32SemaHub_full_ST_6i(r32,v)                   _BFSET_(r32, 6, 6,v)
    #define   SET16SemaHub_full_ST_6i(r16,v)                   _BFSET_(r16, 6, 6,v)
    #define   SET32SemaHub_full_ST_7i(r32,v)                   _BFSET_(r32, 7, 7,v)
    #define   SET16SemaHub_full_ST_7i(r16,v)                   _BFSET_(r16, 7, 7,v)
    #define   SET32SemaHub_full_ST_8i(r32,v)                   _BFSET_(r32, 8, 8,v)
    #define   SET16SemaHub_full_ST_8i(r16,v)                   _BFSET_(r16, 8, 8,v)
    #define   SET32SemaHub_full_ST_9i(r32,v)                   _BFSET_(r32, 9, 9,v)
    #define   SET16SemaHub_full_ST_9i(r16,v)                   _BFSET_(r16, 9, 9,v)
    #define   SET32SemaHub_full_ST_10i(r32,v)                  _BFSET_(r32,10,10,v)
    #define   SET16SemaHub_full_ST_10i(r16,v)                  _BFSET_(r16,10,10,v)
    #define   SET32SemaHub_full_ST_11i(r32,v)                  _BFSET_(r32,11,11,v)
    #define   SET16SemaHub_full_ST_11i(r16,v)                  _BFSET_(r16,11,11,v)
    #define   SET32SemaHub_full_ST_12i(r32,v)                  _BFSET_(r32,12,12,v)
    #define   SET16SemaHub_full_ST_12i(r16,v)                  _BFSET_(r16,12,12,v)
    #define   SET32SemaHub_full_ST_13i(r32,v)                  _BFSET_(r32,13,13,v)
    #define   SET16SemaHub_full_ST_13i(r16,v)                  _BFSET_(r16,13,13,v)
    #define   SET32SemaHub_full_ST_14i(r32,v)                  _BFSET_(r32,14,14,v)
    #define   SET16SemaHub_full_ST_14i(r16,v)                  _BFSET_(r16,14,14,v)
    #define   SET32SemaHub_full_ST_15i(r32,v)                  _BFSET_(r32,15,15,v)
    #define   SET16SemaHub_full_ST_15i(r16,v)                  _BFSET_(r16,15,15,v)
    #define   SET32SemaHub_full_ST_16i(r32,v)                  _BFSET_(r32,16,16,v)
    #define   SET16SemaHub_full_ST_16i(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define   SET32SemaHub_full_ST_17i(r32,v)                  _BFSET_(r32,17,17,v)
    #define   SET16SemaHub_full_ST_17i(r16,v)                  _BFSET_(r16, 1, 1,v)
    #define   SET32SemaHub_full_ST_18i(r32,v)                  _BFSET_(r32,18,18,v)
    #define   SET16SemaHub_full_ST_18i(r16,v)                  _BFSET_(r16, 2, 2,v)
    #define   SET32SemaHub_full_ST_19i(r32,v)                  _BFSET_(r32,19,19,v)
    #define   SET16SemaHub_full_ST_19i(r16,v)                  _BFSET_(r16, 3, 3,v)
    #define   SET32SemaHub_full_ST_20i(r32,v)                  _BFSET_(r32,20,20,v)
    #define   SET16SemaHub_full_ST_20i(r16,v)                  _BFSET_(r16, 4, 4,v)
    #define   SET32SemaHub_full_ST_21i(r32,v)                  _BFSET_(r32,21,21,v)
    #define   SET16SemaHub_full_ST_21i(r16,v)                  _BFSET_(r16, 5, 5,v)
    #define   SET32SemaHub_full_ST_22i(r32,v)                  _BFSET_(r32,22,22,v)
    #define   SET16SemaHub_full_ST_22i(r16,v)                  _BFSET_(r16, 6, 6,v)
    #define   SET32SemaHub_full_ST_23i(r32,v)                  _BFSET_(r32,23,23,v)
    #define   SET16SemaHub_full_ST_23i(r16,v)                  _BFSET_(r16, 7, 7,v)
    #define   SET32SemaHub_full_ST_24i(r32,v)                  _BFSET_(r32,24,24,v)
    #define   SET16SemaHub_full_ST_24i(r16,v)                  _BFSET_(r16, 8, 8,v)
    #define   SET32SemaHub_full_ST_25i(r32,v)                  _BFSET_(r32,25,25,v)
    #define   SET16SemaHub_full_ST_25i(r16,v)                  _BFSET_(r16, 9, 9,v)
    #define   SET32SemaHub_full_ST_26i(r32,v)                  _BFSET_(r32,26,26,v)
    #define   SET16SemaHub_full_ST_26i(r16,v)                  _BFSET_(r16,10,10,v)
    #define   SET32SemaHub_full_ST_27i(r32,v)                  _BFSET_(r32,27,27,v)
    #define   SET16SemaHub_full_ST_27i(r16,v)                  _BFSET_(r16,11,11,v)
    #define   SET32SemaHub_full_ST_28i(r32,v)                  _BFSET_(r32,28,28,v)
    #define   SET16SemaHub_full_ST_28i(r16,v)                  _BFSET_(r16,12,12,v)
    #define   SET32SemaHub_full_ST_29i(r32,v)                  _BFSET_(r32,29,29,v)
    #define   SET16SemaHub_full_ST_29i(r16,v)                  _BFSET_(r16,13,13,v)
    #define   SET32SemaHub_full_ST_30i(r32,v)                  _BFSET_(r32,30,30,v)
    #define   SET16SemaHub_full_ST_30i(r16,v)                  _BFSET_(r16,14,14,v)
    #define   SET32SemaHub_full_ST_31i(r32,v)                  _BFSET_(r32,31,31,v)
    #define   SET16SemaHub_full_ST_31i(r16,v)                  _BFSET_(r16,15,15,v)
    #define     w32SemaHub_full                                {\
            UNSG32 ufull_ST_0i                                 :  1;\
            UNSG32 ufull_ST_1i                                 :  1;\
            UNSG32 ufull_ST_2i                                 :  1;\
            UNSG32 ufull_ST_3i                                 :  1;\
            UNSG32 ufull_ST_4i                                 :  1;\
            UNSG32 ufull_ST_5i                                 :  1;\
            UNSG32 ufull_ST_6i                                 :  1;\
            UNSG32 ufull_ST_7i                                 :  1;\
            UNSG32 ufull_ST_8i                                 :  1;\
            UNSG32 ufull_ST_9i                                 :  1;\
            UNSG32 ufull_ST_10i                                :  1;\
            UNSG32 ufull_ST_11i                                :  1;\
            UNSG32 ufull_ST_12i                                :  1;\
            UNSG32 ufull_ST_13i                                :  1;\
            UNSG32 ufull_ST_14i                                :  1;\
            UNSG32 ufull_ST_15i                                :  1;\
            UNSG32 ufull_ST_16i                                :  1;\
            UNSG32 ufull_ST_17i                                :  1;\
            UNSG32 ufull_ST_18i                                :  1;\
            UNSG32 ufull_ST_19i                                :  1;\
            UNSG32 ufull_ST_20i                                :  1;\
            UNSG32 ufull_ST_21i                                :  1;\
            UNSG32 ufull_ST_22i                                :  1;\
            UNSG32 ufull_ST_23i                                :  1;\
            UNSG32 ufull_ST_24i                                :  1;\
            UNSG32 ufull_ST_25i                                :  1;\
            UNSG32 ufull_ST_26i                                :  1;\
            UNSG32 ufull_ST_27i                                :  1;\
            UNSG32 ufull_ST_28i                                :  1;\
            UNSG32 ufull_ST_29i                                :  1;\
            UNSG32 ufull_ST_30i                                :  1;\
            UNSG32 ufull_ST_31i                                :  1;\
          }
    union { UNSG32 u32SemaHub_full;
            struct w32SemaHub_full;
          };
    #define   SET32SemaHub_almostEmpty_ST_0i(r32,v)            _BFSET_(r32, 0, 0,v)
    #define   SET16SemaHub_almostEmpty_ST_0i(r16,v)            _BFSET_(r16, 0, 0,v)
    #define   SET32SemaHub_almostEmpty_ST_1i(r32,v)            _BFSET_(r32, 1, 1,v)
    #define   SET16SemaHub_almostEmpty_ST_1i(r16,v)            _BFSET_(r16, 1, 1,v)
    #define   SET32SemaHub_almostEmpty_ST_2i(r32,v)            _BFSET_(r32, 2, 2,v)
    #define   SET16SemaHub_almostEmpty_ST_2i(r16,v)            _BFSET_(r16, 2, 2,v)
    #define   SET32SemaHub_almostEmpty_ST_3i(r32,v)            _BFSET_(r32, 3, 3,v)
    #define   SET16SemaHub_almostEmpty_ST_3i(r16,v)            _BFSET_(r16, 3, 3,v)
    #define   SET32SemaHub_almostEmpty_ST_4i(r32,v)            _BFSET_(r32, 4, 4,v)
    #define   SET16SemaHub_almostEmpty_ST_4i(r16,v)            _BFSET_(r16, 4, 4,v)
    #define   SET32SemaHub_almostEmpty_ST_5i(r32,v)            _BFSET_(r32, 5, 5,v)
    #define   SET16SemaHub_almostEmpty_ST_5i(r16,v)            _BFSET_(r16, 5, 5,v)
    #define   SET32SemaHub_almostEmpty_ST_6i(r32,v)            _BFSET_(r32, 6, 6,v)
    #define   SET16SemaHub_almostEmpty_ST_6i(r16,v)            _BFSET_(r16, 6, 6,v)
    #define   SET32SemaHub_almostEmpty_ST_7i(r32,v)            _BFSET_(r32, 7, 7,v)
    #define   SET16SemaHub_almostEmpty_ST_7i(r16,v)            _BFSET_(r16, 7, 7,v)
    #define   SET32SemaHub_almostEmpty_ST_8i(r32,v)            _BFSET_(r32, 8, 8,v)
    #define   SET16SemaHub_almostEmpty_ST_8i(r16,v)            _BFSET_(r16, 8, 8,v)
    #define   SET32SemaHub_almostEmpty_ST_9i(r32,v)            _BFSET_(r32, 9, 9,v)
    #define   SET16SemaHub_almostEmpty_ST_9i(r16,v)            _BFSET_(r16, 9, 9,v)
    #define   SET32SemaHub_almostEmpty_ST_10i(r32,v)           _BFSET_(r32,10,10,v)
    #define   SET16SemaHub_almostEmpty_ST_10i(r16,v)           _BFSET_(r16,10,10,v)
    #define   SET32SemaHub_almostEmpty_ST_11i(r32,v)           _BFSET_(r32,11,11,v)
    #define   SET16SemaHub_almostEmpty_ST_11i(r16,v)           _BFSET_(r16,11,11,v)
    #define   SET32SemaHub_almostEmpty_ST_12i(r32,v)           _BFSET_(r32,12,12,v)
    #define   SET16SemaHub_almostEmpty_ST_12i(r16,v)           _BFSET_(r16,12,12,v)
    #define   SET32SemaHub_almostEmpty_ST_13i(r32,v)           _BFSET_(r32,13,13,v)
    #define   SET16SemaHub_almostEmpty_ST_13i(r16,v)           _BFSET_(r16,13,13,v)
    #define   SET32SemaHub_almostEmpty_ST_14i(r32,v)           _BFSET_(r32,14,14,v)
    #define   SET16SemaHub_almostEmpty_ST_14i(r16,v)           _BFSET_(r16,14,14,v)
    #define   SET32SemaHub_almostEmpty_ST_15i(r32,v)           _BFSET_(r32,15,15,v)
    #define   SET16SemaHub_almostEmpty_ST_15i(r16,v)           _BFSET_(r16,15,15,v)
    #define   SET32SemaHub_almostEmpty_ST_16i(r32,v)           _BFSET_(r32,16,16,v)
    #define   SET16SemaHub_almostEmpty_ST_16i(r16,v)           _BFSET_(r16, 0, 0,v)
    #define   SET32SemaHub_almostEmpty_ST_17i(r32,v)           _BFSET_(r32,17,17,v)
    #define   SET16SemaHub_almostEmpty_ST_17i(r16,v)           _BFSET_(r16, 1, 1,v)
    #define   SET32SemaHub_almostEmpty_ST_18i(r32,v)           _BFSET_(r32,18,18,v)
    #define   SET16SemaHub_almostEmpty_ST_18i(r16,v)           _BFSET_(r16, 2, 2,v)
    #define   SET32SemaHub_almostEmpty_ST_19i(r32,v)           _BFSET_(r32,19,19,v)
    #define   SET16SemaHub_almostEmpty_ST_19i(r16,v)           _BFSET_(r16, 3, 3,v)
    #define   SET32SemaHub_almostEmpty_ST_20i(r32,v)           _BFSET_(r32,20,20,v)
    #define   SET16SemaHub_almostEmpty_ST_20i(r16,v)           _BFSET_(r16, 4, 4,v)
    #define   SET32SemaHub_almostEmpty_ST_21i(r32,v)           _BFSET_(r32,21,21,v)
    #define   SET16SemaHub_almostEmpty_ST_21i(r16,v)           _BFSET_(r16, 5, 5,v)
    #define   SET32SemaHub_almostEmpty_ST_22i(r32,v)           _BFSET_(r32,22,22,v)
    #define   SET16SemaHub_almostEmpty_ST_22i(r16,v)           _BFSET_(r16, 6, 6,v)
    #define   SET32SemaHub_almostEmpty_ST_23i(r32,v)           _BFSET_(r32,23,23,v)
    #define   SET16SemaHub_almostEmpty_ST_23i(r16,v)           _BFSET_(r16, 7, 7,v)
    #define   SET32SemaHub_almostEmpty_ST_24i(r32,v)           _BFSET_(r32,24,24,v)
    #define   SET16SemaHub_almostEmpty_ST_24i(r16,v)           _BFSET_(r16, 8, 8,v)
    #define   SET32SemaHub_almostEmpty_ST_25i(r32,v)           _BFSET_(r32,25,25,v)
    #define   SET16SemaHub_almostEmpty_ST_25i(r16,v)           _BFSET_(r16, 9, 9,v)
    #define   SET32SemaHub_almostEmpty_ST_26i(r32,v)           _BFSET_(r32,26,26,v)
    #define   SET16SemaHub_almostEmpty_ST_26i(r16,v)           _BFSET_(r16,10,10,v)
    #define   SET32SemaHub_almostEmpty_ST_27i(r32,v)           _BFSET_(r32,27,27,v)
    #define   SET16SemaHub_almostEmpty_ST_27i(r16,v)           _BFSET_(r16,11,11,v)
    #define   SET32SemaHub_almostEmpty_ST_28i(r32,v)           _BFSET_(r32,28,28,v)
    #define   SET16SemaHub_almostEmpty_ST_28i(r16,v)           _BFSET_(r16,12,12,v)
    #define   SET32SemaHub_almostEmpty_ST_29i(r32,v)           _BFSET_(r32,29,29,v)
    #define   SET16SemaHub_almostEmpty_ST_29i(r16,v)           _BFSET_(r16,13,13,v)
    #define   SET32SemaHub_almostEmpty_ST_30i(r32,v)           _BFSET_(r32,30,30,v)
    #define   SET16SemaHub_almostEmpty_ST_30i(r16,v)           _BFSET_(r16,14,14,v)
    #define   SET32SemaHub_almostEmpty_ST_31i(r32,v)           _BFSET_(r32,31,31,v)
    #define   SET16SemaHub_almostEmpty_ST_31i(r16,v)           _BFSET_(r16,15,15,v)
    #define     w32SemaHub_almostEmpty                         {\
            UNSG32 ualmostEmpty_ST_0i                          :  1;\
            UNSG32 ualmostEmpty_ST_1i                          :  1;\
            UNSG32 ualmostEmpty_ST_2i                          :  1;\
            UNSG32 ualmostEmpty_ST_3i                          :  1;\
            UNSG32 ualmostEmpty_ST_4i                          :  1;\
            UNSG32 ualmostEmpty_ST_5i                          :  1;\
            UNSG32 ualmostEmpty_ST_6i                          :  1;\
            UNSG32 ualmostEmpty_ST_7i                          :  1;\
            UNSG32 ualmostEmpty_ST_8i                          :  1;\
            UNSG32 ualmostEmpty_ST_9i                          :  1;\
            UNSG32 ualmostEmpty_ST_10i                         :  1;\
            UNSG32 ualmostEmpty_ST_11i                         :  1;\
            UNSG32 ualmostEmpty_ST_12i                         :  1;\
            UNSG32 ualmostEmpty_ST_13i                         :  1;\
            UNSG32 ualmostEmpty_ST_14i                         :  1;\
            UNSG32 ualmostEmpty_ST_15i                         :  1;\
            UNSG32 ualmostEmpty_ST_16i                         :  1;\
            UNSG32 ualmostEmpty_ST_17i                         :  1;\
            UNSG32 ualmostEmpty_ST_18i                         :  1;\
            UNSG32 ualmostEmpty_ST_19i                         :  1;\
            UNSG32 ualmostEmpty_ST_20i                         :  1;\
            UNSG32 ualmostEmpty_ST_21i                         :  1;\
            UNSG32 ualmostEmpty_ST_22i                         :  1;\
            UNSG32 ualmostEmpty_ST_23i                         :  1;\
            UNSG32 ualmostEmpty_ST_24i                         :  1;\
            UNSG32 ualmostEmpty_ST_25i                         :  1;\
            UNSG32 ualmostEmpty_ST_26i                         :  1;\
            UNSG32 ualmostEmpty_ST_27i                         :  1;\
            UNSG32 ualmostEmpty_ST_28i                         :  1;\
            UNSG32 ualmostEmpty_ST_29i                         :  1;\
            UNSG32 ualmostEmpty_ST_30i                         :  1;\
            UNSG32 ualmostEmpty_ST_31i                         :  1;\
          }
    union { UNSG32 u32SemaHub_almostEmpty;
            struct w32SemaHub_almostEmpty;
          };
    #define   SET32SemaHub_almostFull_ST_0i(r32,v)             _BFSET_(r32, 0, 0,v)
    #define   SET16SemaHub_almostFull_ST_0i(r16,v)             _BFSET_(r16, 0, 0,v)
    #define   SET32SemaHub_almostFull_ST_1i(r32,v)             _BFSET_(r32, 1, 1,v)
    #define   SET16SemaHub_almostFull_ST_1i(r16,v)             _BFSET_(r16, 1, 1,v)
    #define   SET32SemaHub_almostFull_ST_2i(r32,v)             _BFSET_(r32, 2, 2,v)
    #define   SET16SemaHub_almostFull_ST_2i(r16,v)             _BFSET_(r16, 2, 2,v)
    #define   SET32SemaHub_almostFull_ST_3i(r32,v)             _BFSET_(r32, 3, 3,v)
    #define   SET16SemaHub_almostFull_ST_3i(r16,v)             _BFSET_(r16, 3, 3,v)
    #define   SET32SemaHub_almostFull_ST_4i(r32,v)             _BFSET_(r32, 4, 4,v)
    #define   SET16SemaHub_almostFull_ST_4i(r16,v)             _BFSET_(r16, 4, 4,v)
    #define   SET32SemaHub_almostFull_ST_5i(r32,v)             _BFSET_(r32, 5, 5,v)
    #define   SET16SemaHub_almostFull_ST_5i(r16,v)             _BFSET_(r16, 5, 5,v)
    #define   SET32SemaHub_almostFull_ST_6i(r32,v)             _BFSET_(r32, 6, 6,v)
    #define   SET16SemaHub_almostFull_ST_6i(r16,v)             _BFSET_(r16, 6, 6,v)
    #define   SET32SemaHub_almostFull_ST_7i(r32,v)             _BFSET_(r32, 7, 7,v)
    #define   SET16SemaHub_almostFull_ST_7i(r16,v)             _BFSET_(r16, 7, 7,v)
    #define   SET32SemaHub_almostFull_ST_8i(r32,v)             _BFSET_(r32, 8, 8,v)
    #define   SET16SemaHub_almostFull_ST_8i(r16,v)             _BFSET_(r16, 8, 8,v)
    #define   SET32SemaHub_almostFull_ST_9i(r32,v)             _BFSET_(r32, 9, 9,v)
    #define   SET16SemaHub_almostFull_ST_9i(r16,v)             _BFSET_(r16, 9, 9,v)
    #define   SET32SemaHub_almostFull_ST_10i(r32,v)            _BFSET_(r32,10,10,v)
    #define   SET16SemaHub_almostFull_ST_10i(r16,v)            _BFSET_(r16,10,10,v)
    #define   SET32SemaHub_almostFull_ST_11i(r32,v)            _BFSET_(r32,11,11,v)
    #define   SET16SemaHub_almostFull_ST_11i(r16,v)            _BFSET_(r16,11,11,v)
    #define   SET32SemaHub_almostFull_ST_12i(r32,v)            _BFSET_(r32,12,12,v)
    #define   SET16SemaHub_almostFull_ST_12i(r16,v)            _BFSET_(r16,12,12,v)
    #define   SET32SemaHub_almostFull_ST_13i(r32,v)            _BFSET_(r32,13,13,v)
    #define   SET16SemaHub_almostFull_ST_13i(r16,v)            _BFSET_(r16,13,13,v)
    #define   SET32SemaHub_almostFull_ST_14i(r32,v)            _BFSET_(r32,14,14,v)
    #define   SET16SemaHub_almostFull_ST_14i(r16,v)            _BFSET_(r16,14,14,v)
    #define   SET32SemaHub_almostFull_ST_15i(r32,v)            _BFSET_(r32,15,15,v)
    #define   SET16SemaHub_almostFull_ST_15i(r16,v)            _BFSET_(r16,15,15,v)
    #define   SET32SemaHub_almostFull_ST_16i(r32,v)            _BFSET_(r32,16,16,v)
    #define   SET16SemaHub_almostFull_ST_16i(r16,v)            _BFSET_(r16, 0, 0,v)
    #define   SET32SemaHub_almostFull_ST_17i(r32,v)            _BFSET_(r32,17,17,v)
    #define   SET16SemaHub_almostFull_ST_17i(r16,v)            _BFSET_(r16, 1, 1,v)
    #define   SET32SemaHub_almostFull_ST_18i(r32,v)            _BFSET_(r32,18,18,v)
    #define   SET16SemaHub_almostFull_ST_18i(r16,v)            _BFSET_(r16, 2, 2,v)
    #define   SET32SemaHub_almostFull_ST_19i(r32,v)            _BFSET_(r32,19,19,v)
    #define   SET16SemaHub_almostFull_ST_19i(r16,v)            _BFSET_(r16, 3, 3,v)
    #define   SET32SemaHub_almostFull_ST_20i(r32,v)            _BFSET_(r32,20,20,v)
    #define   SET16SemaHub_almostFull_ST_20i(r16,v)            _BFSET_(r16, 4, 4,v)
    #define   SET32SemaHub_almostFull_ST_21i(r32,v)            _BFSET_(r32,21,21,v)
    #define   SET16SemaHub_almostFull_ST_21i(r16,v)            _BFSET_(r16, 5, 5,v)
    #define   SET32SemaHub_almostFull_ST_22i(r32,v)            _BFSET_(r32,22,22,v)
    #define   SET16SemaHub_almostFull_ST_22i(r16,v)            _BFSET_(r16, 6, 6,v)
    #define   SET32SemaHub_almostFull_ST_23i(r32,v)            _BFSET_(r32,23,23,v)
    #define   SET16SemaHub_almostFull_ST_23i(r16,v)            _BFSET_(r16, 7, 7,v)
    #define   SET32SemaHub_almostFull_ST_24i(r32,v)            _BFSET_(r32,24,24,v)
    #define   SET16SemaHub_almostFull_ST_24i(r16,v)            _BFSET_(r16, 8, 8,v)
    #define   SET32SemaHub_almostFull_ST_25i(r32,v)            _BFSET_(r32,25,25,v)
    #define   SET16SemaHub_almostFull_ST_25i(r16,v)            _BFSET_(r16, 9, 9,v)
    #define   SET32SemaHub_almostFull_ST_26i(r32,v)            _BFSET_(r32,26,26,v)
    #define   SET16SemaHub_almostFull_ST_26i(r16,v)            _BFSET_(r16,10,10,v)
    #define   SET32SemaHub_almostFull_ST_27i(r32,v)            _BFSET_(r32,27,27,v)
    #define   SET16SemaHub_almostFull_ST_27i(r16,v)            _BFSET_(r16,11,11,v)
    #define   SET32SemaHub_almostFull_ST_28i(r32,v)            _BFSET_(r32,28,28,v)
    #define   SET16SemaHub_almostFull_ST_28i(r16,v)            _BFSET_(r16,12,12,v)
    #define   SET32SemaHub_almostFull_ST_29i(r32,v)            _BFSET_(r32,29,29,v)
    #define   SET16SemaHub_almostFull_ST_29i(r16,v)            _BFSET_(r16,13,13,v)
    #define   SET32SemaHub_almostFull_ST_30i(r32,v)            _BFSET_(r32,30,30,v)
    #define   SET16SemaHub_almostFull_ST_30i(r16,v)            _BFSET_(r16,14,14,v)
    #define   SET32SemaHub_almostFull_ST_31i(r32,v)            _BFSET_(r32,31,31,v)
    #define   SET16SemaHub_almostFull_ST_31i(r16,v)            _BFSET_(r16,15,15,v)
    #define     w32SemaHub_almostFull                          {\
            UNSG32 ualmostFull_ST_0i                           :  1;\
            UNSG32 ualmostFull_ST_1i                           :  1;\
            UNSG32 ualmostFull_ST_2i                           :  1;\
            UNSG32 ualmostFull_ST_3i                           :  1;\
            UNSG32 ualmostFull_ST_4i                           :  1;\
            UNSG32 ualmostFull_ST_5i                           :  1;\
            UNSG32 ualmostFull_ST_6i                           :  1;\
            UNSG32 ualmostFull_ST_7i                           :  1;\
            UNSG32 ualmostFull_ST_8i                           :  1;\
            UNSG32 ualmostFull_ST_9i                           :  1;\
            UNSG32 ualmostFull_ST_10i                          :  1;\
            UNSG32 ualmostFull_ST_11i                          :  1;\
            UNSG32 ualmostFull_ST_12i                          :  1;\
            UNSG32 ualmostFull_ST_13i                          :  1;\
            UNSG32 ualmostFull_ST_14i                          :  1;\
            UNSG32 ualmostFull_ST_15i                          :  1;\
            UNSG32 ualmostFull_ST_16i                          :  1;\
            UNSG32 ualmostFull_ST_17i                          :  1;\
            UNSG32 ualmostFull_ST_18i                          :  1;\
            UNSG32 ualmostFull_ST_19i                          :  1;\
            UNSG32 ualmostFull_ST_20i                          :  1;\
            UNSG32 ualmostFull_ST_21i                          :  1;\
            UNSG32 ualmostFull_ST_22i                          :  1;\
            UNSG32 ualmostFull_ST_23i                          :  1;\
            UNSG32 ualmostFull_ST_24i                          :  1;\
            UNSG32 ualmostFull_ST_25i                          :  1;\
            UNSG32 ualmostFull_ST_26i                          :  1;\
            UNSG32 ualmostFull_ST_27i                          :  1;\
            UNSG32 ualmostFull_ST_28i                          :  1;\
            UNSG32 ualmostFull_ST_29i                          :  1;\
            UNSG32 ualmostFull_ST_30i                          :  1;\
            UNSG32 ualmostFull_ST_31i                          :  1;\
          }
    union { UNSG32 u32SemaHub_almostFull;
            struct w32SemaHub_almostFull;
          };
             UNSG8 RSVDx418                                    [232];
    } SIE_SemaHub;
    typedef union  T32SemaHub_PUSH
          { UNSG32 u32;
            struct w32SemaHub_PUSH;
                 } T32SemaHub_PUSH;
    typedef union  T32SemaHub_POP
          { UNSG32 u32;
            struct w32SemaHub_POP;
                 } T32SemaHub_POP;
    typedef union  T32SemaHub_empty
          { UNSG32 u32;
            struct w32SemaHub_empty;
                 } T32SemaHub_empty;
    typedef union  T32SemaHub_full
          { UNSG32 u32;
            struct w32SemaHub_full;
                 } T32SemaHub_full;
    typedef union  T32SemaHub_almostEmpty
          { UNSG32 u32;
            struct w32SemaHub_almostEmpty;
                 } T32SemaHub_almostEmpty;
    typedef union  T32SemaHub_almostFull
          { UNSG32 u32;
            struct w32SemaHub_almostFull;
                 } T32SemaHub_almostFull;
    typedef union  TSemaHub_PUSH
          { UNSG32 u32[1];
            struct {
            struct w32SemaHub_PUSH;
                   };
                 } TSemaHub_PUSH;
    typedef union  TSemaHub_POP
          { UNSG32 u32[1];
            struct {
            struct w32SemaHub_POP;
                   };
                 } TSemaHub_POP;
    typedef union  TSemaHub_empty
          { UNSG32 u32[1];
            struct {
            struct w32SemaHub_empty;
                   };
                 } TSemaHub_empty;
    typedef union  TSemaHub_full
          { UNSG32 u32[1];
            struct {
            struct w32SemaHub_full;
                   };
                 } TSemaHub_full;
    typedef union  TSemaHub_almostEmpty
          { UNSG32 u32[1];
            struct {
            struct w32SemaHub_almostEmpty;
                   };
                 } TSemaHub_almostEmpty;
    typedef union  TSemaHub_almostFull
          { UNSG32 u32[1];
            struct {
            struct w32SemaHub_almostFull;
                   };
                 } TSemaHub_almostFull;
     SIGN32 SemaHub_drvrd(SIE_SemaHub *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 SemaHub_drvwr(SIE_SemaHub *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void SemaHub_reset(SIE_SemaHub *p);
     SIGN32 SemaHub_cmp  (SIE_SemaHub *p, SIE_SemaHub *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define SemaHub_check(p,pie,pfx,hLOG) SemaHub_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define SemaHub_print(p,    pfx,hLOG) SemaHub_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_FiFo
#define h_FiFo (){}
    #define     RA_FiFo_CFG                                    0x0000
    #define       bFiFo_CFG_BASE                               20
    #define     RA_FiFo_START                                  0x0004
    #define       bFiFo_START_EN                               1
    #define     RA_FiFo_CLEAR                                  0x0008
    #define       bFiFo_CLEAR_EN                               1
    #define     RA_FiFo_FLUSH                                  0x000C
    #define       bFiFo_FLUSH_EN                               1
    typedef struct SIE_FiFo {
    #define   SET32FiFo_CFG_BASE(r32,v)                        _BFSET_(r32,19, 0,v)
    #define     w32FiFo_CFG                                    {\
            UNSG32 uCFG_BASE                                   : 20;\
            UNSG32 RSVDx0_b20                                  : 12;\
          }
    union { UNSG32 u32FiFo_CFG;
            struct w32FiFo_CFG;
          };
    #define   SET32FiFo_START_EN(r32,v)                        _BFSET_(r32, 0, 0,v)
    #define   SET16FiFo_START_EN(r16,v)                        _BFSET_(r16, 0, 0,v)
    #define     w32FiFo_START                                  {\
            UNSG32 uSTART_EN                                   :  1;\
            UNSG32 RSVDx4_b1                                   : 31;\
          }
    union { UNSG32 u32FiFo_START;
            struct w32FiFo_START;
          };
    #define   SET32FiFo_CLEAR_EN(r32,v)                        _BFSET_(r32, 0, 0,v)
    #define   SET16FiFo_CLEAR_EN(r16,v)                        _BFSET_(r16, 0, 0,v)
    #define     w32FiFo_CLEAR                                  {\
            UNSG32 uCLEAR_EN                                   :  1;\
            UNSG32 RSVDx8_b1                                   : 31;\
          }
    union { UNSG32 u32FiFo_CLEAR;
            struct w32FiFo_CLEAR;
          };
    #define   SET32FiFo_FLUSH_EN(r32,v)                        _BFSET_(r32, 0, 0,v)
    #define   SET16FiFo_FLUSH_EN(r16,v)                        _BFSET_(r16, 0, 0,v)
    #define     w32FiFo_FLUSH                                  {\
            UNSG32 uFLUSH_EN                                   :  1;\
            UNSG32 RSVDxC_b1                                   : 31;\
          }
    union { UNSG32 u32FiFo_FLUSH;
            struct w32FiFo_FLUSH;
          };
    } SIE_FiFo;
    typedef union  T32FiFo_CFG
          { UNSG32 u32;
            struct w32FiFo_CFG;
                 } T32FiFo_CFG;
    typedef union  T32FiFo_START
          { UNSG32 u32;
            struct w32FiFo_START;
                 } T32FiFo_START;
    typedef union  T32FiFo_CLEAR
          { UNSG32 u32;
            struct w32FiFo_CLEAR;
                 } T32FiFo_CLEAR;
    typedef union  T32FiFo_FLUSH
          { UNSG32 u32;
            struct w32FiFo_FLUSH;
                 } T32FiFo_FLUSH;
    typedef union  TFiFo_CFG
          { UNSG32 u32[1];
            struct {
            struct w32FiFo_CFG;
                   };
                 } TFiFo_CFG;
    typedef union  TFiFo_START
          { UNSG32 u32[1];
            struct {
            struct w32FiFo_START;
                   };
                 } TFiFo_START;
    typedef union  TFiFo_CLEAR
          { UNSG32 u32[1];
            struct {
            struct w32FiFo_CLEAR;
                   };
                 } TFiFo_CLEAR;
    typedef union  TFiFo_FLUSH
          { UNSG32 u32[1];
            struct {
            struct w32FiFo_FLUSH;
                   };
                 } TFiFo_FLUSH;
     SIGN32 FiFo_drvrd(SIE_FiFo *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 FiFo_drvwr(SIE_FiFo *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void FiFo_reset(SIE_FiFo *p);
     SIGN32 FiFo_cmp  (SIE_FiFo *p, SIE_FiFo *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define FiFo_check(p,pie,pfx,hLOG) FiFo_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define FiFo_print(p,    pfx,hLOG) FiFo_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_HBO
#define h_HBO (){}
    #define     RA_HBO_FiFoCtl                                 0x0000
    #define     RA_HBO_ARR                                     0x0500
    #define     RA_HBO_FiFo                                    0x0500
    #define     RA_HBO_BUSY                                    0x0700
    #define       bHBO_BUSY_ST                                 32
    typedef struct SIE_HBO {
              SIE_SemaHub                                      ie_FiFoCtl;
              SIE_FiFo                                         ie_FiFo[32];
    #define   SET32HBO_BUSY_ST(r32,v)                          _BFSET_(r32,31, 0,v)
    #define     w32HBO_BUSY                                    {\
            UNSG32 uBUSY_ST                                    : 32;\
          }
    union { UNSG32 u32HBO_BUSY;
            struct w32HBO_BUSY;
          };
             UNSG8 RSVDx704                                    [252];
    } SIE_HBO;
    typedef union  T32HBO_BUSY
          { UNSG32 u32;
            struct w32HBO_BUSY;
                 } T32HBO_BUSY;
    typedef union  THBO_BUSY
          { UNSG32 u32[1];
            struct {
            struct w32HBO_BUSY;
                   };
                 } THBO_BUSY;
     SIGN32 HBO_drvrd(SIE_HBO *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 HBO_drvwr(SIE_HBO *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void HBO_reset(SIE_HBO *p);
     SIGN32 HBO_cmp  (SIE_HBO *p, SIE_HBO *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define HBO_check(p,pie,pfx,hLOG) HBO_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define HBO_print(p,    pfx,hLOG) HBO_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_LLDesFmt
#define h_LLDesFmt (){}
    #define     RA_LLDesFmt_mem                                0x0000
    #define       bLLDesFmt_mem_size                           16
    typedef struct SIE_LLDesFmt {
    #define   SET32LLDesFmt_mem_size(r32,v)                    _BFSET_(r32,15, 0,v)
    #define   SET16LLDesFmt_mem_size(r16,v)                    _BFSET_(r16,15, 0,v)
    #define     w32LLDesFmt_mem                                {\
            UNSG32 umem_size                                   : 16;\
            UNSG32 RSVDx0_b16                                  : 16;\
          }
    union { UNSG32 u32LLDesFmt_mem;
            struct w32LLDesFmt_mem;
          };
    } SIE_LLDesFmt;
    typedef union  T32LLDesFmt_mem
          { UNSG32 u32;
            struct w32LLDesFmt_mem;
                 } T32LLDesFmt_mem;
    typedef union  TLLDesFmt_mem
          { UNSG32 u32[1];
            struct {
            struct w32LLDesFmt_mem;
                   };
                 } TLLDesFmt_mem;
     SIGN32 LLDesFmt_drvrd(SIE_LLDesFmt *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 LLDesFmt_drvwr(SIE_LLDesFmt *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void LLDesFmt_reset(SIE_LLDesFmt *p);
     SIGN32 LLDesFmt_cmp  (SIE_LLDesFmt *p, SIE_LLDesFmt *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define LLDesFmt_check(p,pie,pfx,hLOG) LLDesFmt_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define LLDesFmt_print(p,    pfx,hLOG) LLDesFmt_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_dHubCmdHDR
#define h_dHubCmdHDR (){}
    #define     RA_dHubCmdHDR_DESC                             0x0000
    #define       bdHubCmdHDR_DESC_size                        16
    #define       bdHubCmdHDR_DESC_sizeMTU                     1
    #define       bdHubCmdHDR_DESC_semOpMTU                    1
    #define       bdHubCmdHDR_DESC_chkSemId                    5
    #define       bdHubCmdHDR_DESC_updSemId                    5
    #define       bdHubCmdHDR_DESC_interrupt                   1
    #define       bdHubCmdHDR_DESC_ovrdQos                     1
    #define       bdHubCmdHDR_DESC_disSem                      1
    #define       bdHubCmdHDR_DESC_qosSel                      1
    typedef struct SIE_dHubCmdHDR {
    #define   SET32dHubCmdHDR_DESC_size(r32,v)                 _BFSET_(r32,15, 0,v)
    #define   SET16dHubCmdHDR_DESC_size(r16,v)                 _BFSET_(r16,15, 0,v)
    #define   SET32dHubCmdHDR_DESC_sizeMTU(r32,v)              _BFSET_(r32,16,16,v)
    #define   SET16dHubCmdHDR_DESC_sizeMTU(r16,v)              _BFSET_(r16, 0, 0,v)
    #define   SET32dHubCmdHDR_DESC_semOpMTU(r32,v)             _BFSET_(r32,17,17,v)
    #define   SET16dHubCmdHDR_DESC_semOpMTU(r16,v)             _BFSET_(r16, 1, 1,v)
    #define   SET32dHubCmdHDR_DESC_chkSemId(r32,v)             _BFSET_(r32,22,18,v)
    #define   SET16dHubCmdHDR_DESC_chkSemId(r16,v)             _BFSET_(r16, 6, 2,v)
    #define   SET32dHubCmdHDR_DESC_updSemId(r32,v)             _BFSET_(r32,27,23,v)
    #define   SET16dHubCmdHDR_DESC_updSemId(r16,v)             _BFSET_(r16,11, 7,v)
    #define   SET32dHubCmdHDR_DESC_interrupt(r32,v)            _BFSET_(r32,28,28,v)
    #define   SET16dHubCmdHDR_DESC_interrupt(r16,v)            _BFSET_(r16,12,12,v)
    #define   SET32dHubCmdHDR_DESC_ovrdQos(r32,v)              _BFSET_(r32,29,29,v)
    #define   SET16dHubCmdHDR_DESC_ovrdQos(r16,v)              _BFSET_(r16,13,13,v)
    #define   SET32dHubCmdHDR_DESC_disSem(r32,v)               _BFSET_(r32,30,30,v)
    #define   SET16dHubCmdHDR_DESC_disSem(r16,v)               _BFSET_(r16,14,14,v)
    #define   SET32dHubCmdHDR_DESC_qosSel(r32,v)               _BFSET_(r32,31,31,v)
    #define   SET16dHubCmdHDR_DESC_qosSel(r16,v)               _BFSET_(r16,15,15,v)
    #define     w32dHubCmdHDR_DESC                             {\
            UNSG32 uDESC_size                                  : 16;\
            UNSG32 uDESC_sizeMTU                               :  1;\
            UNSG32 uDESC_semOpMTU                              :  1;\
            UNSG32 uDESC_chkSemId                              :  5;\
            UNSG32 uDESC_updSemId                              :  5;\
            UNSG32 uDESC_interrupt                             :  1;\
            UNSG32 uDESC_ovrdQos                               :  1;\
            UNSG32 uDESC_disSem                                :  1;\
            UNSG32 uDESC_qosSel                                :  1;\
          }
    union { UNSG32 u32dHubCmdHDR_DESC;
            struct w32dHubCmdHDR_DESC;
          };
    } SIE_dHubCmdHDR;
    typedef union  T32dHubCmdHDR_DESC
          { UNSG32 u32;
            struct w32dHubCmdHDR_DESC;
                 } T32dHubCmdHDR_DESC;
    typedef union  TdHubCmdHDR_DESC
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmdHDR_DESC;
                   };
                 } TdHubCmdHDR_DESC;
     SIGN32 dHubCmdHDR_drvrd(SIE_dHubCmdHDR *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 dHubCmdHDR_drvwr(SIE_dHubCmdHDR *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void dHubCmdHDR_reset(SIE_dHubCmdHDR *p);
     SIGN32 dHubCmdHDR_cmp  (SIE_dHubCmdHDR *p, SIE_dHubCmdHDR *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define dHubCmdHDR_check(p,pie,pfx,hLOG) dHubCmdHDR_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define dHubCmdHDR_print(p,    pfx,hLOG) dHubCmdHDR_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_dHubCmd
#define h_dHubCmd (){}
    #define     RA_dHubCmd_MEM                                 0x0000
    #define       bdHubCmd_MEM_addr                            32
    #define     RA_dHubCmd_HDR                                 0x0004
    typedef struct SIE_dHubCmd {
    #define   SET32dHubCmd_MEM_addr(r32,v)                     _BFSET_(r32,31, 0,v)
    #define     w32dHubCmd_MEM                                 {\
            UNSG32 uMEM_addr                                   : 32;\
          }
    union { UNSG32 u32dHubCmd_MEM;
            struct w32dHubCmd_MEM;
          };
              SIE_dHubCmdHDR                                   ie_HDR;
    } SIE_dHubCmd;
    typedef union  T32dHubCmd_MEM
          { UNSG32 u32;
            struct w32dHubCmd_MEM;
                 } T32dHubCmd_MEM;
    typedef union  TdHubCmd_MEM
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd_MEM;
                   };
                 } TdHubCmd_MEM;
     SIGN32 dHubCmd_drvrd(SIE_dHubCmd *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 dHubCmd_drvwr(SIE_dHubCmd *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void dHubCmd_reset(SIE_dHubCmd *p);
     SIGN32 dHubCmd_cmp  (SIE_dHubCmd *p, SIE_dHubCmd *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define dHubCmd_check(p,pie,pfx,hLOG) dHubCmd_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define dHubCmd_print(p,    pfx,hLOG) dHubCmd_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_dHubChannel
#define h_dHubChannel (){}
    #define     RA_dHubChannel_CFG                             0x0000
    #define       bdHubChannel_CFG_MTU                         4
    #define        dHubChannel_CFG_MTU_8byte                                0x0
    #define        dHubChannel_CFG_MTU_16byte                               0x1
    #define        dHubChannel_CFG_MTU_32byte                               0x2
    #define        dHubChannel_CFG_MTU_64byte                               0x3
    #define        dHubChannel_CFG_MTU_128byte                              0x4
    #define        dHubChannel_CFG_MTU_256byte                              0x5
    #define        dHubChannel_CFG_MTU_512byte                              0x6
    #define        dHubChannel_CFG_MTU_1024byte                             0x7
    #define        dHubChannel_CFG_MTU_2048byte                             0x8
    #define        dHubChannel_CFG_MTU_4096byte                             0x9
    #define       bdHubChannel_CFG_QoS                         1
    #define       bdHubChannel_CFG_selfLoop                    1
    #define       bdHubChannel_CFG_intrCtl                     1
    #define        dHubChannel_CFG_intrCtl_cmdDone                          0x0
    #define        dHubChannel_CFG_intrCtl_chIdle                           0x1
    #define       bdHubChannel_CFG_hScan                       1
    #define        dHubChannel_CFG_hScan_rastScan                           0x0
    #define        dHubChannel_CFG_hScan_invScan                            0x1
    #define       bdHubChannel_CFG_vScan                       1
    #define        dHubChannel_CFG_vScan_rastScan                           0x0
    #define        dHubChannel_CFG_vScan_invScan                            0x1
    #define     RA_dHubChannel_ROB_MAP                         0x0004
    #define       bdHubChannel_ROB_MAP_ID                      4
    #define     RA_dHubChannel_AWQOS                           0x0008
    #define       bdHubChannel_AWQOS_LO                        4
    #define       bdHubChannel_AWQOS_HI                        4
    #define     RA_dHubChannel_ARQOS                           0x000C
    #define       bdHubChannel_ARQOS_LO                        4
    #define       bdHubChannel_ARQOS_HI                        4
    #define     RA_dHubChannel_AWPARAMS                        0x0010
    #define       bdHubChannel_AWPARAMS_LOCK                   2
    #define       bdHubChannel_AWPARAMS_PROT                   3
    #define       bdHubChannel_AWPARAMS_USER                   6
    #define       bdHubChannel_AWPARAMS_CACHE                  3
    #define       bdHubChannel_AWPARAMS_USER_HI_EN             1
    #define     RA_dHubChannel_ARPARAMS                        0x0014
    #define       bdHubChannel_ARPARAMS_LOCK                   2
    #define       bdHubChannel_ARPARAMS_PROT                   3
    #define       bdHubChannel_ARPARAMS_USER                   6
    #define       bdHubChannel_ARPARAMS_CACHE                  4
    #define       bdHubChannel_ARPARAMS_USER_HI_EN             1
    #define     RA_dHubChannel_START                           0x0018
    #define       bdHubChannel_START_EN                        1
    #define     RA_dHubChannel_CLEAR                           0x001C
    #define       bdHubChannel_CLEAR_EN                        1
    #define     RA_dHubChannel_FLUSH                           0x0020
    #define       bdHubChannel_FLUSH_EN                        1
    typedef struct SIE_dHubChannel {
    #define   SET32dHubChannel_CFG_MTU(r32,v)                  _BFSET_(r32, 3, 0,v)
    #define   SET16dHubChannel_CFG_MTU(r16,v)                  _BFSET_(r16, 3, 0,v)
    #define   SET32dHubChannel_CFG_QoS(r32,v)                  _BFSET_(r32, 4, 4,v)
    #define   SET16dHubChannel_CFG_QoS(r16,v)                  _BFSET_(r16, 4, 4,v)
    #define   SET32dHubChannel_CFG_selfLoop(r32,v)             _BFSET_(r32, 5, 5,v)
    #define   SET16dHubChannel_CFG_selfLoop(r16,v)             _BFSET_(r16, 5, 5,v)
    #define   SET32dHubChannel_CFG_intrCtl(r32,v)              _BFSET_(r32, 6, 6,v)
    #define   SET16dHubChannel_CFG_intrCtl(r16,v)              _BFSET_(r16, 6, 6,v)
    #define   SET32dHubChannel_CFG_hScan(r32,v)                _BFSET_(r32, 7, 7,v)
    #define   SET16dHubChannel_CFG_hScan(r16,v)                _BFSET_(r16, 7, 7,v)
    #define   SET32dHubChannel_CFG_vScan(r32,v)                _BFSET_(r32, 8, 8,v)
    #define   SET16dHubChannel_CFG_vScan(r16,v)                _BFSET_(r16, 8, 8,v)
    #define     w32dHubChannel_CFG                             {\
            UNSG32 uCFG_MTU                                    :  4;\
            UNSG32 uCFG_QoS                                    :  1;\
            UNSG32 uCFG_selfLoop                               :  1;\
            UNSG32 uCFG_intrCtl                                :  1;\
            UNSG32 uCFG_hScan                                  :  1;\
            UNSG32 uCFG_vScan                                  :  1;\
            UNSG32 RSVDx0_b9                                   : 23;\
          }
    union { UNSG32 u32dHubChannel_CFG;
            struct w32dHubChannel_CFG;
          };
    #define   SET32dHubChannel_ROB_MAP_ID(r32,v)               _BFSET_(r32, 3, 0,v)
    #define   SET16dHubChannel_ROB_MAP_ID(r16,v)               _BFSET_(r16, 3, 0,v)
    #define     w32dHubChannel_ROB_MAP                         {\
            UNSG32 uROB_MAP_ID                                 :  4;\
            UNSG32 RSVDx4_b4                                   : 28;\
          }
    union { UNSG32 u32dHubChannel_ROB_MAP;
            struct w32dHubChannel_ROB_MAP;
          };
    #define   SET32dHubChannel_AWQOS_LO(r32,v)                 _BFSET_(r32, 3, 0,v)
    #define   SET16dHubChannel_AWQOS_LO(r16,v)                 _BFSET_(r16, 3, 0,v)
    #define   SET32dHubChannel_AWQOS_HI(r32,v)                 _BFSET_(r32, 7, 4,v)
    #define   SET16dHubChannel_AWQOS_HI(r16,v)                 _BFSET_(r16, 7, 4,v)
    #define     w32dHubChannel_AWQOS                           {\
            UNSG32 uAWQOS_LO                                   :  4;\
            UNSG32 uAWQOS_HI                                   :  4;\
            UNSG32 RSVDx8_b8                                   : 24;\
          }
    union { UNSG32 u32dHubChannel_AWQOS;
            struct w32dHubChannel_AWQOS;
          };
    #define   SET32dHubChannel_ARQOS_LO(r32,v)                 _BFSET_(r32, 3, 0,v)
    #define   SET16dHubChannel_ARQOS_LO(r16,v)                 _BFSET_(r16, 3, 0,v)
    #define   SET32dHubChannel_ARQOS_HI(r32,v)                 _BFSET_(r32, 7, 4,v)
    #define   SET16dHubChannel_ARQOS_HI(r16,v)                 _BFSET_(r16, 7, 4,v)
    #define     w32dHubChannel_ARQOS                           {\
            UNSG32 uARQOS_LO                                   :  4;\
            UNSG32 uARQOS_HI                                   :  4;\
            UNSG32 RSVDxC_b8                                   : 24;\
          }
    union { UNSG32 u32dHubChannel_ARQOS;
            struct w32dHubChannel_ARQOS;
          };
    #define   SET32dHubChannel_AWPARAMS_LOCK(r32,v)            _BFSET_(r32, 1, 0,v)
    #define   SET16dHubChannel_AWPARAMS_LOCK(r16,v)            _BFSET_(r16, 1, 0,v)
    #define   SET32dHubChannel_AWPARAMS_PROT(r32,v)            _BFSET_(r32, 4, 2,v)
    #define   SET16dHubChannel_AWPARAMS_PROT(r16,v)            _BFSET_(r16, 4, 2,v)
    #define   SET32dHubChannel_AWPARAMS_USER(r32,v)            _BFSET_(r32,10, 5,v)
    #define   SET16dHubChannel_AWPARAMS_USER(r16,v)            _BFSET_(r16,10, 5,v)
    #define   SET32dHubChannel_AWPARAMS_CACHE(r32,v)           _BFSET_(r32,13,11,v)
    #define   SET16dHubChannel_AWPARAMS_CACHE(r16,v)           _BFSET_(r16,13,11,v)
    #define   SET32dHubChannel_AWPARAMS_USER_HI_EN(r32,v)      _BFSET_(r32,14,14,v)
    #define   SET16dHubChannel_AWPARAMS_USER_HI_EN(r16,v)      _BFSET_(r16,14,14,v)
    #define     w32dHubChannel_AWPARAMS                        {\
            UNSG32 uAWPARAMS_LOCK                              :  2;\
            UNSG32 uAWPARAMS_PROT                              :  3;\
            UNSG32 uAWPARAMS_USER                              :  6;\
            UNSG32 uAWPARAMS_CACHE                             :  3;\
            UNSG32 uAWPARAMS_USER_HI_EN                        :  1;\
            UNSG32 RSVDx10_b15                                 : 17;\
          }
    union { UNSG32 u32dHubChannel_AWPARAMS;
            struct w32dHubChannel_AWPARAMS;
          };
    #define   SET32dHubChannel_ARPARAMS_LOCK(r32,v)            _BFSET_(r32, 1, 0,v)
    #define   SET16dHubChannel_ARPARAMS_LOCK(r16,v)            _BFSET_(r16, 1, 0,v)
    #define   SET32dHubChannel_ARPARAMS_PROT(r32,v)            _BFSET_(r32, 4, 2,v)
    #define   SET16dHubChannel_ARPARAMS_PROT(r16,v)            _BFSET_(r16, 4, 2,v)
    #define   SET32dHubChannel_ARPARAMS_USER(r32,v)            _BFSET_(r32,10, 5,v)
    #define   SET16dHubChannel_ARPARAMS_USER(r16,v)            _BFSET_(r16,10, 5,v)
    #define   SET32dHubChannel_ARPARAMS_CACHE(r32,v)           _BFSET_(r32,14,11,v)
    #define   SET16dHubChannel_ARPARAMS_CACHE(r16,v)           _BFSET_(r16,14,11,v)
    #define   SET32dHubChannel_ARPARAMS_USER_HI_EN(r32,v)      _BFSET_(r32,15,15,v)
    #define   SET16dHubChannel_ARPARAMS_USER_HI_EN(r16,v)      _BFSET_(r16,15,15,v)
    #define     w32dHubChannel_ARPARAMS                        {\
            UNSG32 uARPARAMS_LOCK                              :  2;\
            UNSG32 uARPARAMS_PROT                              :  3;\
            UNSG32 uARPARAMS_USER                              :  6;\
            UNSG32 uARPARAMS_CACHE                             :  4;\
            UNSG32 uARPARAMS_USER_HI_EN                        :  1;\
            UNSG32 RSVDx14_b16                                 : 16;\
          }
    union { UNSG32 u32dHubChannel_ARPARAMS;
            struct w32dHubChannel_ARPARAMS;
          };
    #define   SET32dHubChannel_START_EN(r32,v)                 _BFSET_(r32, 0, 0,v)
    #define   SET16dHubChannel_START_EN(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define     w32dHubChannel_START                           {\
            UNSG32 uSTART_EN                                   :  1;\
            UNSG32 RSVDx18_b1                                  : 31;\
          }
    union { UNSG32 u32dHubChannel_START;
            struct w32dHubChannel_START;
          };
    #define   SET32dHubChannel_CLEAR_EN(r32,v)                 _BFSET_(r32, 0, 0,v)
    #define   SET16dHubChannel_CLEAR_EN(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define     w32dHubChannel_CLEAR                           {\
            UNSG32 uCLEAR_EN                                   :  1;\
            UNSG32 RSVDx1C_b1                                  : 31;\
          }
    union { UNSG32 u32dHubChannel_CLEAR;
            struct w32dHubChannel_CLEAR;
          };
    #define   SET32dHubChannel_FLUSH_EN(r32,v)                 _BFSET_(r32, 0, 0,v)
    #define   SET16dHubChannel_FLUSH_EN(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define     w32dHubChannel_FLUSH                           {\
            UNSG32 uFLUSH_EN                                   :  1;\
            UNSG32 RSVDx20_b1                                  : 31;\
          }
    union { UNSG32 u32dHubChannel_FLUSH;
            struct w32dHubChannel_FLUSH;
          };
    } SIE_dHubChannel;
    typedef union  T32dHubChannel_CFG
          { UNSG32 u32;
            struct w32dHubChannel_CFG;
                 } T32dHubChannel_CFG;
    typedef union  T32dHubChannel_ROB_MAP
          { UNSG32 u32;
            struct w32dHubChannel_ROB_MAP;
                 } T32dHubChannel_ROB_MAP;
    typedef union  T32dHubChannel_AWQOS
          { UNSG32 u32;
            struct w32dHubChannel_AWQOS;
                 } T32dHubChannel_AWQOS;
    typedef union  T32dHubChannel_ARQOS
          { UNSG32 u32;
            struct w32dHubChannel_ARQOS;
                 } T32dHubChannel_ARQOS;
    typedef union  T32dHubChannel_AWPARAMS
          { UNSG32 u32;
            struct w32dHubChannel_AWPARAMS;
                 } T32dHubChannel_AWPARAMS;
    typedef union  T32dHubChannel_ARPARAMS
          { UNSG32 u32;
            struct w32dHubChannel_ARPARAMS;
                 } T32dHubChannel_ARPARAMS;
    typedef union  T32dHubChannel_START
          { UNSG32 u32;
            struct w32dHubChannel_START;
                 } T32dHubChannel_START;
    typedef union  T32dHubChannel_CLEAR
          { UNSG32 u32;
            struct w32dHubChannel_CLEAR;
                 } T32dHubChannel_CLEAR;
    typedef union  T32dHubChannel_FLUSH
          { UNSG32 u32;
            struct w32dHubChannel_FLUSH;
                 } T32dHubChannel_FLUSH;
    typedef union  TdHubChannel_CFG
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_CFG;
                   };
                 } TdHubChannel_CFG;
    typedef union  TdHubChannel_ROB_MAP
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_ROB_MAP;
                   };
                 } TdHubChannel_ROB_MAP;
    typedef union  TdHubChannel_AWQOS
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_AWQOS;
                   };
                 } TdHubChannel_AWQOS;
    typedef union  TdHubChannel_ARQOS
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_ARQOS;
                   };
                 } TdHubChannel_ARQOS;
    typedef union  TdHubChannel_AWPARAMS
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_AWPARAMS;
                   };
                 } TdHubChannel_AWPARAMS;
    typedef union  TdHubChannel_ARPARAMS
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_ARPARAMS;
                   };
                 } TdHubChannel_ARPARAMS;
    typedef union  TdHubChannel_START
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_START;
                   };
                 } TdHubChannel_START;
    typedef union  TdHubChannel_CLEAR
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_CLEAR;
                   };
                 } TdHubChannel_CLEAR;
    typedef union  TdHubChannel_FLUSH
          { UNSG32 u32[1];
            struct {
            struct w32dHubChannel_FLUSH;
                   };
                 } TdHubChannel_FLUSH;
     SIGN32 dHubChannel_drvrd(SIE_dHubChannel *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 dHubChannel_drvwr(SIE_dHubChannel *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void dHubChannel_reset(SIE_dHubChannel *p);
     SIGN32 dHubChannel_cmp  (SIE_dHubChannel *p, SIE_dHubChannel *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define dHubChannel_check(p,pie,pfx,hLOG) dHubChannel_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define dHubChannel_print(p,    pfx,hLOG) dHubChannel_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_dHubReg
#define h_dHubReg (){}
    #define     RA_dHubReg_SemaHub                             0x0000
    #define     RA_dHubReg_HBO                                 0x0500
    #define     RA_dHubReg_ARR                                 0x0D00
    #define     RA_dHubReg_channelCtl                          0x0D00
    #define     RA_dHubReg_BUSY                                0x0F40
    #define       bdHubReg_BUSY_ST                             16
    #define     RA_dHubReg_PENDING                             0x0F44
    #define       bdHubReg_PENDING_ST                          16
    #define     RA_dHubReg_busRstEn                            0x0F48
    #define       bdHubReg_busRstEn_reg                        1
    #define     RA_dHubReg_busRstDone                          0x0F4C
    #define       bdHubReg_busRstDone_reg                      1
    #define     RA_dHubReg_flowCtl                             0x0F50
    #define       bdHubReg_flowCtl_rAlpha                      8
    #define       bdHubReg_flowCtl_wAlpha                      8
    #define     RA_dHubReg_axiCmdCol                           0x0F54
    #define       bdHubReg_axiCmdCol_rCnt                      16
    #define       bdHubReg_axiCmdCol_wCnt                      16
    #define     RA_dHubReg_axiMultiIdEn                        0x0F58
    #define       bdHubReg_axiMultiIdEn_reg                    1
    typedef struct SIE_dHubReg {
              SIE_SemaHub                                      ie_SemaHub;
              SIE_HBO                                          ie_HBO;
              SIE_dHubChannel                                  ie_channelCtl[16];
    #define   SET32dHubReg_BUSY_ST(r32,v)                      _BFSET_(r32,15, 0,v)
    #define   SET16dHubReg_BUSY_ST(r16,v)                      _BFSET_(r16,15, 0,v)
    #define     w32dHubReg_BUSY                                {\
            UNSG32 uBUSY_ST                                    : 16;\
            UNSG32 RSVDxF40_b16                                : 16;\
          }
    union { UNSG32 u32dHubReg_BUSY;
            struct w32dHubReg_BUSY;
          };
    #define   SET32dHubReg_PENDING_ST(r32,v)                   _BFSET_(r32,15, 0,v)
    #define   SET16dHubReg_PENDING_ST(r16,v)                   _BFSET_(r16,15, 0,v)
    #define     w32dHubReg_PENDING                             {\
            UNSG32 uPENDING_ST                                 : 16;\
            UNSG32 RSVDxF44_b16                                : 16;\
          }
    union { UNSG32 u32dHubReg_PENDING;
            struct w32dHubReg_PENDING;
          };
    #define   SET32dHubReg_busRstEn_reg(r32,v)                 _BFSET_(r32, 0, 0,v)
    #define   SET16dHubReg_busRstEn_reg(r16,v)                 _BFSET_(r16, 0, 0,v)
    #define     w32dHubReg_busRstEn                            {\
            UNSG32 ubusRstEn_reg                               :  1;\
            UNSG32 RSVDxF48_b1                                 : 31;\
          }
    union { UNSG32 u32dHubReg_busRstEn;
            struct w32dHubReg_busRstEn;
          };
    #define   SET32dHubReg_busRstDone_reg(r32,v)               _BFSET_(r32, 0, 0,v)
    #define   SET16dHubReg_busRstDone_reg(r16,v)               _BFSET_(r16, 0, 0,v)
    #define     w32dHubReg_busRstDone                          {\
            UNSG32 ubusRstDone_reg                             :  1;\
            UNSG32 RSVDxF4C_b1                                 : 31;\
          }
    union { UNSG32 u32dHubReg_busRstDone;
            struct w32dHubReg_busRstDone;
          };
    #define   SET32dHubReg_flowCtl_rAlpha(r32,v)               _BFSET_(r32, 7, 0,v)
    #define   SET16dHubReg_flowCtl_rAlpha(r16,v)               _BFSET_(r16, 7, 0,v)
    #define   SET32dHubReg_flowCtl_wAlpha(r32,v)               _BFSET_(r32,15, 8,v)
    #define   SET16dHubReg_flowCtl_wAlpha(r16,v)               _BFSET_(r16,15, 8,v)
    #define     w32dHubReg_flowCtl                             {\
            UNSG32 uflowCtl_rAlpha                             :  8;\
            UNSG32 uflowCtl_wAlpha                             :  8;\
            UNSG32 RSVDxF50_b16                                : 16;\
          }
    union { UNSG32 u32dHubReg_flowCtl;
            struct w32dHubReg_flowCtl;
          };
    #define   SET32dHubReg_axiCmdCol_rCnt(r32,v)               _BFSET_(r32,15, 0,v)
    #define   SET16dHubReg_axiCmdCol_rCnt(r16,v)               _BFSET_(r16,15, 0,v)
    #define   SET32dHubReg_axiCmdCol_wCnt(r32,v)               _BFSET_(r32,31,16,v)
    #define   SET16dHubReg_axiCmdCol_wCnt(r16,v)               _BFSET_(r16,15, 0,v)
    #define     w32dHubReg_axiCmdCol                           {\
            UNSG32 uaxiCmdCol_rCnt                             : 16;\
            UNSG32 uaxiCmdCol_wCnt                             : 16;\
          }
    union { UNSG32 u32dHubReg_axiCmdCol;
            struct w32dHubReg_axiCmdCol;
          };
    #define   SET32dHubReg_axiMultiIdEn_reg(r32,v)             _BFSET_(r32, 0, 0,v)
    #define   SET16dHubReg_axiMultiIdEn_reg(r16,v)             _BFSET_(r16, 0, 0,v)
    #define     w32dHubReg_axiMultiIdEn                        {\
            UNSG32 uaxiMultiIdEn_reg                           :  1;\
            UNSG32 RSVDxF58_b1                                 : 31;\
          }
    union { UNSG32 u32dHubReg_axiMultiIdEn;
            struct w32dHubReg_axiMultiIdEn;
          };
             UNSG8 RSVDxF5C                                    [164];
    } SIE_dHubReg;
    typedef union  T32dHubReg_BUSY
          { UNSG32 u32;
            struct w32dHubReg_BUSY;
                 } T32dHubReg_BUSY;
    typedef union  T32dHubReg_PENDING
          { UNSG32 u32;
            struct w32dHubReg_PENDING;
                 } T32dHubReg_PENDING;
    typedef union  T32dHubReg_busRstEn
          { UNSG32 u32;
            struct w32dHubReg_busRstEn;
                 } T32dHubReg_busRstEn;
    typedef union  T32dHubReg_busRstDone
          { UNSG32 u32;
            struct w32dHubReg_busRstDone;
                 } T32dHubReg_busRstDone;
    typedef union  T32dHubReg_flowCtl
          { UNSG32 u32;
            struct w32dHubReg_flowCtl;
                 } T32dHubReg_flowCtl;
    typedef union  T32dHubReg_axiCmdCol
          { UNSG32 u32;
            struct w32dHubReg_axiCmdCol;
                 } T32dHubReg_axiCmdCol;
    typedef union  T32dHubReg_axiMultiIdEn
          { UNSG32 u32;
            struct w32dHubReg_axiMultiIdEn;
                 } T32dHubReg_axiMultiIdEn;
    typedef union  TdHubReg_BUSY
          { UNSG32 u32[1];
            struct {
            struct w32dHubReg_BUSY;
                   };
                 } TdHubReg_BUSY;
    typedef union  TdHubReg_PENDING
          { UNSG32 u32[1];
            struct {
            struct w32dHubReg_PENDING;
                   };
                 } TdHubReg_PENDING;
    typedef union  TdHubReg_busRstEn
          { UNSG32 u32[1];
            struct {
            struct w32dHubReg_busRstEn;
                   };
                 } TdHubReg_busRstEn;
    typedef union  TdHubReg_busRstDone
          { UNSG32 u32[1];
            struct {
            struct w32dHubReg_busRstDone;
                   };
                 } TdHubReg_busRstDone;
    typedef union  TdHubReg_flowCtl
          { UNSG32 u32[1];
            struct {
            struct w32dHubReg_flowCtl;
                   };
                 } TdHubReg_flowCtl;
    typedef union  TdHubReg_axiCmdCol
          { UNSG32 u32[1];
            struct {
            struct w32dHubReg_axiCmdCol;
                   };
                 } TdHubReg_axiCmdCol;
    typedef union  TdHubReg_axiMultiIdEn
          { UNSG32 u32[1];
            struct {
            struct w32dHubReg_axiMultiIdEn;
                   };
                 } TdHubReg_axiMultiIdEn;
     SIGN32 dHubReg_drvrd(SIE_dHubReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 dHubReg_drvwr(SIE_dHubReg *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void dHubReg_reset(SIE_dHubReg *p);
     SIGN32 dHubReg_cmp  (SIE_dHubReg *p, SIE_dHubReg *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define dHubReg_check(p,pie,pfx,hLOG) dHubReg_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define dHubReg_print(p,    pfx,hLOG) dHubReg_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_dHubCmd2D
#define h_dHubCmd2D (){}
    #define     RA_dHubCmd2D_MEM                               0x0000
    #define       bdHubCmd2D_MEM_addr                          32
    #define     RA_dHubCmd2D_DESC                              0x0004
    #define       bdHubCmd2D_DESC_stride                       16
    #define       bdHubCmd2D_DESC_numLine                      13
    #define       bdHubCmd2D_DESC_hdrLoop                      2
    #define       bdHubCmd2D_DESC_interrupt                    1
    #define     RA_dHubCmd2D_START                             0x0008
    #define       bdHubCmd2D_START_EN                          1
    #define     RA_dHubCmd2D_CLEAR                             0x000C
    #define       bdHubCmd2D_CLEAR_EN                          1
    #define     RA_dHubCmd2D_HDR                               0x0010
    typedef struct SIE_dHubCmd2D {
    #define   SET32dHubCmd2D_MEM_addr(r32,v)                   _BFSET_(r32,31, 0,v)
    #define     w32dHubCmd2D_MEM                               {\
            UNSG32 uMEM_addr                                   : 32;\
          }
    union { UNSG32 u32dHubCmd2D_MEM;
            struct w32dHubCmd2D_MEM;
          };
    #define   SET32dHubCmd2D_DESC_stride(r32,v)                _BFSET_(r32,15, 0,v)
    #define   SET16dHubCmd2D_DESC_stride(r16,v)                _BFSET_(r16,15, 0,v)
    #define   SET32dHubCmd2D_DESC_numLine(r32,v)               _BFSET_(r32,28,16,v)
    #define   SET16dHubCmd2D_DESC_numLine(r16,v)               _BFSET_(r16,12, 0,v)
    #define   SET32dHubCmd2D_DESC_hdrLoop(r32,v)               _BFSET_(r32,30,29,v)
    #define   SET16dHubCmd2D_DESC_hdrLoop(r16,v)               _BFSET_(r16,14,13,v)
    #define   SET32dHubCmd2D_DESC_interrupt(r32,v)             _BFSET_(r32,31,31,v)
    #define   SET16dHubCmd2D_DESC_interrupt(r16,v)             _BFSET_(r16,15,15,v)
    #define     w32dHubCmd2D_DESC                              {\
            UNSG32 uDESC_stride                                : 16;\
            UNSG32 uDESC_numLine                               : 13;\
            UNSG32 uDESC_hdrLoop                               :  2;\
            UNSG32 uDESC_interrupt                             :  1;\
          }
    union { UNSG32 u32dHubCmd2D_DESC;
            struct w32dHubCmd2D_DESC;
          };
    #define   SET32dHubCmd2D_START_EN(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16dHubCmd2D_START_EN(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32dHubCmd2D_START                             {\
            UNSG32 uSTART_EN                                   :  1;\
            UNSG32 RSVDx8_b1                                   : 31;\
          }
    union { UNSG32 u32dHubCmd2D_START;
            struct w32dHubCmd2D_START;
          };
    #define   SET32dHubCmd2D_CLEAR_EN(r32,v)                   _BFSET_(r32, 0, 0,v)
    #define   SET16dHubCmd2D_CLEAR_EN(r16,v)                   _BFSET_(r16, 0, 0,v)
    #define     w32dHubCmd2D_CLEAR                             {\
            UNSG32 uCLEAR_EN                                   :  1;\
            UNSG32 RSVDxC_b1                                   : 31;\
          }
    union { UNSG32 u32dHubCmd2D_CLEAR;
            struct w32dHubCmd2D_CLEAR;
          };
              SIE_dHubCmdHDR                                   ie_HDR[4];
    } SIE_dHubCmd2D;
    typedef union  T32dHubCmd2D_MEM
          { UNSG32 u32;
            struct w32dHubCmd2D_MEM;
                 } T32dHubCmd2D_MEM;
    typedef union  T32dHubCmd2D_DESC
          { UNSG32 u32;
            struct w32dHubCmd2D_DESC;
                 } T32dHubCmd2D_DESC;
    typedef union  T32dHubCmd2D_START
          { UNSG32 u32;
            struct w32dHubCmd2D_START;
                 } T32dHubCmd2D_START;
    typedef union  T32dHubCmd2D_CLEAR
          { UNSG32 u32;
            struct w32dHubCmd2D_CLEAR;
                 } T32dHubCmd2D_CLEAR;
    typedef union  TdHubCmd2D_MEM
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2D_MEM;
                   };
                 } TdHubCmd2D_MEM;
    typedef union  TdHubCmd2D_DESC
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2D_DESC;
                   };
                 } TdHubCmd2D_DESC;
    typedef union  TdHubCmd2D_START
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2D_START;
                   };
                 } TdHubCmd2D_START;
    typedef union  TdHubCmd2D_CLEAR
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2D_CLEAR;
                   };
                 } TdHubCmd2D_CLEAR;
     SIGN32 dHubCmd2D_drvrd(SIE_dHubCmd2D *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 dHubCmd2D_drvwr(SIE_dHubCmd2D *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void dHubCmd2D_reset(SIE_dHubCmd2D *p);
     SIGN32 dHubCmd2D_cmp  (SIE_dHubCmd2D *p, SIE_dHubCmd2D *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define dHubCmd2D_check(p,pie,pfx,hLOG) dHubCmd2D_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define dHubCmd2D_print(p,    pfx,hLOG) dHubCmd2D_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_dHubCmd2ND
#define h_dHubCmd2ND (){}
    #define     RA_dHubCmd2ND_MEM                              0x0000
    #define       bdHubCmd2ND_MEM_addr                         32
    #define     RA_dHubCmd2ND_DESC                             0x0004
    #define       bdHubCmd2ND_DESC_burst                       16
    #define       bdHubCmd2ND_DESC_chkSemId                    5
    #define       bdHubCmd2ND_DESC_updSemId                    5
    #define       bdHubCmd2ND_DESC_interrupt                   1
    #define       bdHubCmd2ND_DESC_ovrdQos                     1
    #define       bdHubCmd2ND_DESC_disSem                      1
    #define       bdHubCmd2ND_DESC_qosSel                      1
    #define     RA_dHubCmd2ND_DESC_1D_ST                       0x0008
    #define       bdHubCmd2ND_DESC_1D_ST_step                  24
    #define     RA_dHubCmd2ND_DESC_1D_SZ                       0x000C
    #define       bdHubCmd2ND_DESC_1D_SZ_size                  24
    #define     RA_dHubCmd2ND_DESC_2D_ST                       0x0010
    #define       bdHubCmd2ND_DESC_2D_ST_step                  24
    #define     RA_dHubCmd2ND_DESC_2D_SZ                       0x0014
    #define       bdHubCmd2ND_DESC_2D_SZ_size                  24
    #define     RA_dHubCmd2ND_START                            0x0018
    #define       bdHubCmd2ND_START_EN                         1
    #define     RA_dHubCmd2ND_CLEAR                            0x001C
    #define       bdHubCmd2ND_CLEAR_EN                         1
    typedef struct SIE_dHubCmd2ND {
    #define   SET32dHubCmd2ND_MEM_addr(r32,v)                  _BFSET_(r32,31, 0,v)
    #define     w32dHubCmd2ND_MEM                              {\
            UNSG32 uMEM_addr                                   : 32;\
          }
    union { UNSG32 u32dHubCmd2ND_MEM;
            struct w32dHubCmd2ND_MEM;
          };
    #define   SET32dHubCmd2ND_DESC_burst(r32,v)                _BFSET_(r32,15, 0,v)
    #define   SET16dHubCmd2ND_DESC_burst(r16,v)                _BFSET_(r16,15, 0,v)
    #define   SET32dHubCmd2ND_DESC_chkSemId(r32,v)             _BFSET_(r32,20,16,v)
    #define   SET16dHubCmd2ND_DESC_chkSemId(r16,v)             _BFSET_(r16, 4, 0,v)
    #define   SET32dHubCmd2ND_DESC_updSemId(r32,v)             _BFSET_(r32,25,21,v)
    #define   SET16dHubCmd2ND_DESC_updSemId(r16,v)             _BFSET_(r16, 9, 5,v)
    #define   SET32dHubCmd2ND_DESC_interrupt(r32,v)            _BFSET_(r32,26,26,v)
    #define   SET16dHubCmd2ND_DESC_interrupt(r16,v)            _BFSET_(r16,10,10,v)
    #define   SET32dHubCmd2ND_DESC_ovrdQos(r32,v)              _BFSET_(r32,27,27,v)
    #define   SET16dHubCmd2ND_DESC_ovrdQos(r16,v)              _BFSET_(r16,11,11,v)
    #define   SET32dHubCmd2ND_DESC_disSem(r32,v)               _BFSET_(r32,28,28,v)
    #define   SET16dHubCmd2ND_DESC_disSem(r16,v)               _BFSET_(r16,12,12,v)
    #define   SET32dHubCmd2ND_DESC_qosSel(r32,v)               _BFSET_(r32,29,29,v)
    #define   SET16dHubCmd2ND_DESC_qosSel(r16,v)               _BFSET_(r16,13,13,v)
    #define     w32dHubCmd2ND_DESC                             {\
            UNSG32 uDESC_burst                                 : 16;\
            UNSG32 uDESC_chkSemId                              :  5;\
            UNSG32 uDESC_updSemId                              :  5;\
            UNSG32 uDESC_interrupt                             :  1;\
            UNSG32 uDESC_ovrdQos                               :  1;\
            UNSG32 uDESC_disSem                                :  1;\
            UNSG32 uDESC_qosSel                                :  1;\
            UNSG32 RSVDx4_b30                                  :  2;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC;
            struct w32dHubCmd2ND_DESC;
          };
    #define   SET32dHubCmd2ND_DESC_1D_ST_step(r32,v)           _BFSET_(r32,23, 0,v)
    #define     w32dHubCmd2ND_DESC_1D_ST                       {\
            UNSG32 uDESC_1D_ST_step                            : 24;\
            UNSG32 RSVDx8_b24                                  :  8;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC_1D_ST;
            struct w32dHubCmd2ND_DESC_1D_ST;
          };
    #define   SET32dHubCmd2ND_DESC_1D_SZ_size(r32,v)           _BFSET_(r32,23, 0,v)
    #define     w32dHubCmd2ND_DESC_1D_SZ                       {\
            UNSG32 uDESC_1D_SZ_size                            : 24;\
            UNSG32 RSVDxC_b24                                  :  8;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC_1D_SZ;
            struct w32dHubCmd2ND_DESC_1D_SZ;
          };
    #define   SET32dHubCmd2ND_DESC_2D_ST_step(r32,v)           _BFSET_(r32,23, 0,v)
    #define     w32dHubCmd2ND_DESC_2D_ST                       {\
            UNSG32 uDESC_2D_ST_step                            : 24;\
            UNSG32 RSVDx10_b24                                 :  8;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC_2D_ST;
            struct w32dHubCmd2ND_DESC_2D_ST;
          };
    #define   SET32dHubCmd2ND_DESC_2D_SZ_size(r32,v)           _BFSET_(r32,23, 0,v)
    #define     w32dHubCmd2ND_DESC_2D_SZ                       {\
            UNSG32 uDESC_2D_SZ_size                            : 24;\
            UNSG32 RSVDx14_b24                                 :  8;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC_2D_SZ;
            struct w32dHubCmd2ND_DESC_2D_SZ;
          };
    #define   SET32dHubCmd2ND_START_EN(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16dHubCmd2ND_START_EN(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define     w32dHubCmd2ND_START                            {\
            UNSG32 uSTART_EN                                   :  1;\
            UNSG32 RSVDx18_b1                                  : 31;\
          }
    union { UNSG32 u32dHubCmd2ND_START;
            struct w32dHubCmd2ND_START;
          };
    #define   SET32dHubCmd2ND_CLEAR_EN(r32,v)                  _BFSET_(r32, 0, 0,v)
    #define   SET16dHubCmd2ND_CLEAR_EN(r16,v)                  _BFSET_(r16, 0, 0,v)
    #define     w32dHubCmd2ND_CLEAR                            {\
            UNSG32 uCLEAR_EN                                   :  1;\
            UNSG32 RSVDx1C_b1                                  : 31;\
          }
    union { UNSG32 u32dHubCmd2ND_CLEAR;
            struct w32dHubCmd2ND_CLEAR;
          };
    } SIE_dHubCmd2ND;
    typedef union  T32dHubCmd2ND_MEM
          { UNSG32 u32;
            struct w32dHubCmd2ND_MEM;
                 } T32dHubCmd2ND_MEM;
    typedef union  T32dHubCmd2ND_DESC
          { UNSG32 u32;
            struct w32dHubCmd2ND_DESC;
                 } T32dHubCmd2ND_DESC;
    typedef union  T32dHubCmd2ND_DESC_1D_ST
          { UNSG32 u32;
            struct w32dHubCmd2ND_DESC_1D_ST;
                 } T32dHubCmd2ND_DESC_1D_ST;
    typedef union  T32dHubCmd2ND_DESC_1D_SZ
          { UNSG32 u32;
            struct w32dHubCmd2ND_DESC_1D_SZ;
                 } T32dHubCmd2ND_DESC_1D_SZ;
    typedef union  T32dHubCmd2ND_DESC_2D_ST
          { UNSG32 u32;
            struct w32dHubCmd2ND_DESC_2D_ST;
                 } T32dHubCmd2ND_DESC_2D_ST;
    typedef union  T32dHubCmd2ND_DESC_2D_SZ
          { UNSG32 u32;
            struct w32dHubCmd2ND_DESC_2D_SZ;
                 } T32dHubCmd2ND_DESC_2D_SZ;
    typedef union  T32dHubCmd2ND_START
          { UNSG32 u32;
            struct w32dHubCmd2ND_START;
                 } T32dHubCmd2ND_START;
    typedef union  T32dHubCmd2ND_CLEAR
          { UNSG32 u32;
            struct w32dHubCmd2ND_CLEAR;
                 } T32dHubCmd2ND_CLEAR;
    typedef union  TdHubCmd2ND_MEM
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2ND_MEM;
                   };
                 } TdHubCmd2ND_MEM;
    typedef union  TdHubCmd2ND_DESC
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2ND_DESC;
                   };
                 } TdHubCmd2ND_DESC;
    typedef union  TdHubCmd2ND_DESC_1D_ST
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2ND_DESC_1D_ST;
                   };
                 } TdHubCmd2ND_DESC_1D_ST;
    typedef union  TdHubCmd2ND_DESC_1D_SZ
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2ND_DESC_1D_SZ;
                   };
                 } TdHubCmd2ND_DESC_1D_SZ;
    typedef union  TdHubCmd2ND_DESC_2D_ST
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2ND_DESC_2D_ST;
                   };
                 } TdHubCmd2ND_DESC_2D_ST;
    typedef union  TdHubCmd2ND_DESC_2D_SZ
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2ND_DESC_2D_SZ;
                   };
                 } TdHubCmd2ND_DESC_2D_SZ;
    typedef union  TdHubCmd2ND_START
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2ND_START;
                   };
                 } TdHubCmd2ND_START;
    typedef union  TdHubCmd2ND_CLEAR
          { UNSG32 u32[1];
            struct {
            struct w32dHubCmd2ND_CLEAR;
                   };
                 } TdHubCmd2ND_CLEAR;
     SIGN32 dHubCmd2ND_drvrd(SIE_dHubCmd2ND *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 dHubCmd2ND_drvwr(SIE_dHubCmd2ND *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void dHubCmd2ND_reset(SIE_dHubCmd2ND *p);
     SIGN32 dHubCmd2ND_cmp  (SIE_dHubCmd2ND *p, SIE_dHubCmd2ND *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define dHubCmd2ND_check(p,pie,pfx,hLOG) dHubCmd2ND_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define dHubCmd2ND_print(p,    pfx,hLOG) dHubCmd2ND_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_dHubQuery
#define h_dHubQuery (){}
    #define     RA_dHubQuery_RESP                              0x0000
    #define       bdHubQuery_RESP_ST                           16
    typedef struct SIE_dHubQuery {
    #define   SET32dHubQuery_RESP_ST(r32,v)                    _BFSET_(r32,15, 0,v)
    #define   SET16dHubQuery_RESP_ST(r16,v)                    _BFSET_(r16,15, 0,v)
    #define     w32dHubQuery_RESP                              {\
            UNSG32 uRESP_ST                                    : 16;\
            UNSG32 RSVDx0_b16                                  : 16;\
          }
    union { UNSG32 u32dHubQuery_RESP;
            struct w32dHubQuery_RESP;
          };
    } SIE_dHubQuery;
    typedef union  T32dHubQuery_RESP
          { UNSG32 u32;
            struct w32dHubQuery_RESP;
                 } T32dHubQuery_RESP;
    typedef union  TdHubQuery_RESP
          { UNSG32 u32[1];
            struct {
            struct w32dHubQuery_RESP;
                   };
                 } TdHubQuery_RESP;
     SIGN32 dHubQuery_drvrd(SIE_dHubQuery *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 dHubQuery_drvwr(SIE_dHubQuery *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void dHubQuery_reset(SIE_dHubQuery *p);
     SIGN32 dHubQuery_cmp  (SIE_dHubQuery *p, SIE_dHubQuery *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define dHubQuery_check(p,pie,pfx,hLOG) dHubQuery_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define dHubQuery_print(p,    pfx,hLOG) dHubQuery_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_dHubReg2D
#define h_dHubReg2D (){}
    #define     RA_dHubReg2D_dHub                              0x0000
    #define     RA_dHubReg2D_ARR                               0x1000
    #define     RA_dHubReg2D_Cmd2D                             0x1000
    #define     RA_dHubReg2D_ARR_2ND                           0x1200
    #define     RA_dHubReg2D_Cmd2ND                            0x1200
    #define     RA_dHubReg2D_BUSY                              0x1400
    #define       bdHubReg2D_BUSY_ST                           16
    #define     RA_dHubReg2D_CH_ST                             0x1440
    typedef struct SIE_dHubReg2D {
              SIE_dHubReg                                      ie_dHub;
              SIE_dHubCmd2D                                    ie_Cmd2D[16];
              SIE_dHubCmd2ND                                   ie_Cmd2ND[16];
    #define   SET32dHubReg2D_BUSY_ST(r32,v)                    _BFSET_(r32,15, 0,v)
    #define   SET16dHubReg2D_BUSY_ST(r16,v)                    _BFSET_(r16,15, 0,v)
    #define     w32dHubReg2D_BUSY                              {\
            UNSG32 uBUSY_ST                                    : 16;\
            UNSG32 RSVDx1400_b16                               : 16;\
          }
    union { UNSG32 u32dHubReg2D_BUSY;
            struct w32dHubReg2D_BUSY;
          };
             UNSG8 RSVDx1404                                   [60];
              SIE_dHubQuery                                    ie_CH_ST[16];
             UNSG8 RSVDx1480                                   [128];
    } SIE_dHubReg2D;
    typedef union  T32dHubReg2D_BUSY
          { UNSG32 u32;
            struct w32dHubReg2D_BUSY;
                 } T32dHubReg2D_BUSY;
    typedef union  TdHubReg2D_BUSY
          { UNSG32 u32[1];
            struct {
            struct w32dHubReg2D_BUSY;
                   };
                 } TdHubReg2D_BUSY;
     SIGN32 dHubReg2D_drvrd(SIE_dHubReg2D *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 dHubReg2D_drvwr(SIE_dHubReg2D *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void dHubReg2D_reset(SIE_dHubReg2D *p);
     SIGN32 dHubReg2D_cmp  (SIE_dHubReg2D *p, SIE_dHubReg2D *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define dHubReg2D_check(p,pie,pfx,hLOG) dHubReg2D_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define dHubReg2D_print(p,    pfx,hLOG) dHubReg2D_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_avioDhubChMap
#define h_avioDhubChMap (){}
    #define       bavioDhubChMap_aio64b                        4
    #define        avioDhubChMap_aio64b_MA0_R                               0x0
    #define        avioDhubChMap_aio64b_MA1_R                               0x1
    #define        avioDhubChMap_aio64b_MA2_R                               0x2
    #define        avioDhubChMap_aio64b_MA3_R                               0x3
    #define        avioDhubChMap_aio64b_MIC1_W0                             0x4
    #define        avioDhubChMap_aio64b_MIC1_W1                             0x5
    #define        avioDhubChMap_aio64b_MIC2_CH_W                           0x6
    #define        avioDhubChMap_aio64b_MA7_R                               0x7
    #define        avioDhubChMap_aio64b_BCM_R                               0x8
    #define        avioDhubChMap_aio64b_MA19_W                              0x9
    #define        avioDhubChMap_aio64b_MIC4_CH_W                           0xA
    #define        avioDhubChMap_aio64b_MA11_W                              0xB
    #define        avioDhubChMap_aio64b_PDM_CH_R                            0xC
    #define        avioDhubChMap_aio64b_MA13_R                              0xD
    #define        avioDhubChMap_aio64b_SPDIF_R                             0xE
    #define        avioDhubChMap_aio64b_SPDIF_W                             0xF
    typedef struct SIE_avioDhubChMap {
    #define   SET32avioDhubChMap_aio64b(r32,v)                 _BFSET_(r32, 3, 0,v)
    #define   SET16avioDhubChMap_aio64b(r16,v)                 _BFSET_(r16, 3, 0,v)
            UNSG32 u_aio64b                                    :  4;
            UNSG32 RSVDx0_b4                                   : 28;
    } SIE_avioDhubChMap;
     SIGN32 avioDhubChMap_drvrd(SIE_avioDhubChMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 avioDhubChMap_drvwr(SIE_avioDhubChMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void avioDhubChMap_reset(SIE_avioDhubChMap *p);
     SIGN32 avioDhubChMap_cmp  (SIE_avioDhubChMap *p, SIE_avioDhubChMap *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define avioDhubChMap_check(p,pie,pfx,hLOG) avioDhubChMap_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define avioDhubChMap_print(p,    pfx,hLOG) avioDhubChMap_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_avioDhubTcmMap
#define h_avioDhubTcmMap (){}
    #define        avioDhubTcmMap_aio64bDhub_BANK0_START_ADDR  0x0
    #define        avioDhubTcmMap_aio64bDhub_BANK0_SIZE        0x2A00
    #define     RA_avioDhubTcmMap_dummy                        0x0000
    #define       bavioDhubTcmMap_dummy_xxx                    1
    typedef struct SIE_avioDhubTcmMap {
    #define   SET32avioDhubTcmMap_dummy_xxx(r32,v)             _BFSET_(r32, 0, 0,v)
    #define   SET16avioDhubTcmMap_dummy_xxx(r16,v)             _BFSET_(r16, 0, 0,v)
    #define     w32avioDhubTcmMap_dummy                        {\
            UNSG32 udummy_xxx                                  :  1;\
            UNSG32 RSVDx0_b1                                   : 31;\
          }
    union { UNSG32 u32avioDhubTcmMap_dummy;
            struct w32avioDhubTcmMap_dummy;
          };
    } SIE_avioDhubTcmMap;
    typedef union  T32avioDhubTcmMap_dummy
          { UNSG32 u32;
            struct w32avioDhubTcmMap_dummy;
                 } T32avioDhubTcmMap_dummy;
    typedef union  TavioDhubTcmMap_dummy
          { UNSG32 u32[1];
            struct {
            struct w32avioDhubTcmMap_dummy;
                   };
                 } TavioDhubTcmMap_dummy;
     SIGN32 avioDhubTcmMap_drvrd(SIE_avioDhubTcmMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 avioDhubTcmMap_drvwr(SIE_avioDhubTcmMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void avioDhubTcmMap_reset(SIE_avioDhubTcmMap *p);
     SIGN32 avioDhubTcmMap_cmp  (SIE_avioDhubTcmMap *p, SIE_avioDhubTcmMap *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define avioDhubTcmMap_check(p,pie,pfx,hLOG) avioDhubTcmMap_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define avioDhubTcmMap_print(p,    pfx,hLOG) avioDhubTcmMap_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_avioDhubSemMap
#define h_avioDhubSemMap (){}
    #define       bavioDhubSemMap_aio64b                       5
    #define        avioDhubSemMap_aio64b_CH0_intr                           0x0
    #define        avioDhubSemMap_aio64b_CH1_intr                           0x1
    #define        avioDhubSemMap_aio64b_CH2_intr                           0x2
    #define        avioDhubSemMap_aio64b_CH3_intr                           0x3
    #define        avioDhubSemMap_aio64b_CH4_intr                           0x4
    #define        avioDhubSemMap_aio64b_CH5_intr                           0x5
    #define        avioDhubSemMap_aio64b_CH6_intr                           0x6
    #define        avioDhubSemMap_aio64b_CH7_intr                           0x7
    #define        avioDhubSemMap_aio64b_CH8_intr                           0x8
    #define        avioDhubSemMap_aio64b_CH9_intr                           0x9
    #define        avioDhubSemMap_aio64b_CH10_intr                          0xA
    #define        avioDhubSemMap_aio64b_CH11_intr                          0xB
    #define        avioDhubSemMap_aio64b_CH12_intr                          0xC
    #define        avioDhubSemMap_aio64b_CH13_intr                          0xD
    #define        avioDhubSemMap_aio64b_CH14_intr                          0xE
    #define        avioDhubSemMap_aio64b_CH15_intr                          0xF
    #define        avioDhubSemMap_aio64b_aio_intr0                          0x10
    #define        avioDhubSemMap_aio64b_aio_intr1                          0x11
    #define        avioDhubSemMap_aio64b_aio_intr2                          0x12
    #define        avioDhubSemMap_aio64b_aio_intr3                          0x13
    #define        avioDhubSemMap_aio64b_aio_intr4                          0x14
    #define        avioDhubSemMap_aio64b_aio_intr5                          0x15
    #define        avioDhubSemMap_aio64b_aio_intr6                          0x16
    #define        avioDhubSemMap_aio64b_aio_intr7                          0x17
    #define        avioDhubSemMap_aio64b_aio_intr8                          0x18
    #define        avioDhubSemMap_aio64b_aio_intr9                          0x19
    #define        avioDhubSemMap_aio64b_aio_intr10                         0x1A
    #define        avioDhubSemMap_aio64b_aio_intr11                         0x1B
    #define        avioDhubSemMap_aio64b_aio_intr12                         0x1C
    #define        avioDhubSemMap_aio64b_aio_intr13                         0x1D
    #define        avioDhubSemMap_aio64b_aio_intr14                         0x1E
    #define        avioDhubSemMap_aio64b_aio_intr15                         0x1F
    typedef struct SIE_avioDhubSemMap {
    #define   SET32avioDhubSemMap_aio64b(r32,v)                _BFSET_(r32, 4, 0,v)
    #define   SET16avioDhubSemMap_aio64b(r16,v)                _BFSET_(r16, 4, 0,v)
            UNSG32 u_aio64b                                    :  5;
            UNSG32 RSVDx0_b5                                   : 27;
    } SIE_avioDhubSemMap;
     SIGN32 avioDhubSemMap_drvrd(SIE_avioDhubSemMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 avioDhubSemMap_drvwr(SIE_avioDhubSemMap *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void avioDhubSemMap_reset(SIE_avioDhubSemMap *p);
     SIGN32 avioDhubSemMap_cmp  (SIE_avioDhubSemMap *p, SIE_avioDhubSemMap *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define avioDhubSemMap_check(p,pie,pfx,hLOG) avioDhubSemMap_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define avioDhubSemMap_print(p,    pfx,hLOG) avioDhubSemMap_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_vppTcmEntry
#define h_vppTcmEntry (){}
    #define       bvppTcmEntry_dat                             32
    typedef struct SIE_vppTcmEntry {
    #define   SET32vppTcmEntry_dat(r32,v)                      _BFSET_(r32,31, 0,v)
            UNSG32 u_dat                                       : 32;
    } SIE_vppTcmEntry;
     SIGN32 vppTcmEntry_drvrd(SIE_vppTcmEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 vppTcmEntry_drvwr(SIE_vppTcmEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void vppTcmEntry_reset(SIE_vppTcmEntry *p);
     SIGN32 vppTcmEntry_cmp  (SIE_vppTcmEntry *p, SIE_vppTcmEntry *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define vppTcmEntry_check(p,pie,pfx,hLOG) vppTcmEntry_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define vppTcmEntry_print(p,    pfx,hLOG) vppTcmEntry_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_aio64bDhub
#define h_aio64bDhub (){}
    #define     RA_aio64bDhub_tcm0                             0x0000
    #define     RA_aio64bDhub_dHub0                            0x10000
    typedef struct SIE_aio64bDhub {
              SIE_vppTcmEntry                                  ie_tcm0[10752];
             UNSG8 RSVD_tcm0                                   [22528];
              SIE_dHubReg2D                                    ie_dHub0;
             UNSG8 RSVDx11500                                  [60160];
    } SIE_aio64bDhub;
     SIGN32 aio64bDhub_drvrd(SIE_aio64bDhub *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 aio64bDhub_drvwr(SIE_aio64bDhub *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void aio64bDhub_reset(SIE_aio64bDhub *p);
     SIGN32 aio64bDhub_cmp  (SIE_aio64bDhub *p, SIE_aio64bDhub *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define aio64bDhub_check(p,pie,pfx,hLOG) aio64bDhub_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define aio64bDhub_print(p,    pfx,hLOG) aio64bDhub_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifdef __cplusplus
  }
#endif
#pragma  pack()
#endif
