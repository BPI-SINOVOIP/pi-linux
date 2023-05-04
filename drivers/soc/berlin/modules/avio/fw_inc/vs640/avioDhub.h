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
    typedef struct SIE_SemaINTR {
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
    #define     RA_Semaphore_INTR                              0x0004
    #define     RA_Semaphore_mask                              0x0010
    #define     RA_Semaphore_thresh                            0x0014
    typedef struct SIE_Semaphore {
    #define     w32Semaphore_CFG                               {\
            UNSG32 uCFG_DEPTH                                  : 16;\
            UNSG32 RSVDx0_b16                                  : 16;\
          }
    union { UNSG32 u32Semaphore_CFG;
            struct w32Semaphore_CFG;
          };
              SIE_SemaINTR                                     ie_INTR[3];
    #define     w32Semaphore_mask                              {\
            UNSG32 umask_full                                  :  1;\
            UNSG32 umask_emp                                   :  1;\
            UNSG32 RSVDx10_b2                                  : 30;\
          }
    union { UNSG32 u32Semaphore_mask;
            struct w32Semaphore_mask;
          };
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
    typedef struct SIE_SemaQuery {
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
    #define        SemaQueryMap_ADDR_master_producer                        0x0
    #define        SemaQueryMap_ADDR_master_consumer                        0x1
    typedef struct SIE_SemaQueryMap {
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
    #define     RA_SemaHub_POP                                 0x0404
    #define     RA_SemaHub_empty                               0x0408
    #define     RA_SemaHub_full                                0x040C
    #define     RA_SemaHub_almostEmpty                         0x0410
    #define     RA_SemaHub_almostFull                          0x0414
    typedef struct SIE_SemaHub {
              SIE_SemaQuery                                    ie_counter[64];
              SIE_Semaphore                                    ie_cell[32];
    #define     w32SemaHub_PUSH                                {\
            UNSG32 uPUSH_ID                                    :  8;\
            UNSG32 uPUSH_delta                                 :  8;\
            UNSG32 RSVDx400_b16                                : 16;\
          }
    union { UNSG32 u32SemaHub_PUSH;
            struct w32SemaHub_PUSH;
          };
    #define     w32SemaHub_POP                                 {\
            UNSG32 uPOP_ID                                     :  8;\
            UNSG32 uPOP_delta                                  :  8;\
            UNSG32 RSVDx404_b16                                : 16;\
          }
    union { UNSG32 u32SemaHub_POP;
            struct w32SemaHub_POP;
          };
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
    #define     RA_FiFo_START                                  0x0004
    #define     RA_FiFo_CLEAR                                  0x0008
    #define     RA_FiFo_FLUSH                                  0x000C
    typedef struct SIE_FiFo {
    #define     w32FiFo_CFG                                    {\
            UNSG32 uCFG_BASE                                   : 20;\
            UNSG32 RSVDx0_b20                                  : 12;\
          }
    union { UNSG32 u32FiFo_CFG;
            struct w32FiFo_CFG;
          };
    #define     w32FiFo_START                                  {\
            UNSG32 uSTART_EN                                   :  1;\
            UNSG32 RSVDx4_b1                                   : 31;\
          }
    union { UNSG32 u32FiFo_START;
            struct w32FiFo_START;
          };
    #define     w32FiFo_CLEAR                                  {\
            UNSG32 uCLEAR_EN                                   :  1;\
            UNSG32 RSVDx8_b1                                   : 31;\
          }
    union { UNSG32 u32FiFo_CLEAR;
            struct w32FiFo_CLEAR;
          };
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
    typedef struct SIE_HBO {
              SIE_SemaHub                                      ie_FiFoCtl;
              SIE_FiFo                                         ie_FiFo[32];
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
    typedef struct SIE_LLDesFmt {
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
    typedef struct SIE_dHubCmdHDR {
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
    #define     RA_dHubCmd_HDR                                 0x0004
    typedef struct SIE_dHubCmd {
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
    #define        dHubChannel_CFG_intrCtl_cmdDone                          0x0
    #define        dHubChannel_CFG_intrCtl_chIdle                           0x1
    #define        dHubChannel_CFG_hScan_rastScan                           0x0
    #define        dHubChannel_CFG_hScan_invScan                            0x1
    #define        dHubChannel_CFG_vScan_rastScan                           0x0
    #define        dHubChannel_CFG_vScan_invScan                            0x1
    #define     RA_dHubChannel_ROB_MAP                         0x0004
    #define     RA_dHubChannel_AWQOS                           0x0008
    #define     RA_dHubChannel_ARQOS                           0x000C
    #define     RA_dHubChannel_AWPARAMS                        0x0010
    #define     RA_dHubChannel_ARPARAMS                        0x0014
    #define     RA_dHubChannel_START                           0x0018
    #define     RA_dHubChannel_CLEAR                           0x001C
    #define     RA_dHubChannel_FLUSH                           0x0020
    typedef struct SIE_dHubChannel {
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
    #define     w32dHubChannel_ROB_MAP                         {\
            UNSG32 uROB_MAP_ID                                 :  4;\
            UNSG32 RSVDx4_b4                                   : 28;\
          }
    union { UNSG32 u32dHubChannel_ROB_MAP;
            struct w32dHubChannel_ROB_MAP;
          };
    #define     w32dHubChannel_AWQOS                           {\
            UNSG32 uAWQOS_LO                                   :  4;\
            UNSG32 uAWQOS_HI                                   :  4;\
            UNSG32 RSVDx8_b8                                   : 24;\
          }
    union { UNSG32 u32dHubChannel_AWQOS;
            struct w32dHubChannel_AWQOS;
          };
    #define     w32dHubChannel_ARQOS                           {\
            UNSG32 uARQOS_LO                                   :  4;\
            UNSG32 uARQOS_HI                                   :  4;\
            UNSG32 RSVDxC_b8                                   : 24;\
          }
    union { UNSG32 u32dHubChannel_ARQOS;
            struct w32dHubChannel_ARQOS;
          };
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
    #define     w32dHubChannel_START                           {\
            UNSG32 uSTART_EN                                   :  1;\
            UNSG32 RSVDx18_b1                                  : 31;\
          }
    union { UNSG32 u32dHubChannel_START;
            struct w32dHubChannel_START;
          };
    #define     w32dHubChannel_CLEAR                           {\
            UNSG32 uCLEAR_EN                                   :  1;\
            UNSG32 RSVDx1C_b1                                  : 31;\
          }
    union { UNSG32 u32dHubChannel_CLEAR;
            struct w32dHubChannel_CLEAR;
          };
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
    #define     RA_dHubReg_PENDING                             0x0F44
    #define     RA_dHubReg_busRstEn                            0x0F48
    #define     RA_dHubReg_busRstDone                          0x0F4C
    #define     RA_dHubReg_flowCtl                             0x0F50
    #define     RA_dHubReg_axiCmdCol                           0x0F54
    #define     RA_dHubReg_axiMultiIdEn                        0x0F58
    typedef struct SIE_dHubReg {
              SIE_SemaHub                                      ie_SemaHub;
              SIE_HBO                                          ie_HBO;
              SIE_dHubChannel                                  ie_channelCtl[16];
    #define     w32dHubReg_BUSY                                {\
            UNSG32 uBUSY_ST                                    : 16;\
            UNSG32 RSVDxF40_b16                                : 16;\
          }
    union { UNSG32 u32dHubReg_BUSY;
            struct w32dHubReg_BUSY;
          };
    #define     w32dHubReg_PENDING                             {\
            UNSG32 uPENDING_ST                                 : 16;\
            UNSG32 RSVDxF44_b16                                : 16;\
          }
    union { UNSG32 u32dHubReg_PENDING;
            struct w32dHubReg_PENDING;
          };
    #define     w32dHubReg_busRstEn                            {\
            UNSG32 ubusRstEn_reg                               :  1;\
            UNSG32 RSVDxF48_b1                                 : 31;\
          }
    union { UNSG32 u32dHubReg_busRstEn;
            struct w32dHubReg_busRstEn;
          };
    #define     w32dHubReg_busRstDone                          {\
            UNSG32 ubusRstDone_reg                             :  1;\
            UNSG32 RSVDxF4C_b1                                 : 31;\
          }
    union { UNSG32 u32dHubReg_busRstDone;
            struct w32dHubReg_busRstDone;
          };
    #define     w32dHubReg_flowCtl                             {\
            UNSG32 uflowCtl_rAlpha                             :  8;\
            UNSG32 uflowCtl_wAlpha                             :  8;\
            UNSG32 RSVDxF50_b16                                : 16;\
          }
    union { UNSG32 u32dHubReg_flowCtl;
            struct w32dHubReg_flowCtl;
          };
    #define     w32dHubReg_axiCmdCol                           {\
            UNSG32 uaxiCmdCol_rCnt                             : 16;\
            UNSG32 uaxiCmdCol_wCnt                             : 16;\
          }
    union { UNSG32 u32dHubReg_axiCmdCol;
            struct w32dHubReg_axiCmdCol;
          };
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
    #define     RA_dHubCmd2D_DESC                              0x0004
    #define     RA_dHubCmd2D_START                             0x0008
    #define     RA_dHubCmd2D_CLEAR                             0x000C
    #define     RA_dHubCmd2D_HDR                               0x0010
    typedef struct SIE_dHubCmd2D {
    #define     w32dHubCmd2D_MEM                               {\
            UNSG32 uMEM_addr                                   : 32;\
          }
    union { UNSG32 u32dHubCmd2D_MEM;
            struct w32dHubCmd2D_MEM;
          };
    #define     w32dHubCmd2D_DESC                              {\
            UNSG32 uDESC_stride                                : 16;\
            UNSG32 uDESC_numLine                               : 13;\
            UNSG32 uDESC_hdrLoop                               :  2;\
            UNSG32 uDESC_interrupt                             :  1;\
          }
    union { UNSG32 u32dHubCmd2D_DESC;
            struct w32dHubCmd2D_DESC;
          };
    #define     w32dHubCmd2D_START                             {\
            UNSG32 uSTART_EN                                   :  1;\
            UNSG32 RSVDx8_b1                                   : 31;\
          }
    union { UNSG32 u32dHubCmd2D_START;
            struct w32dHubCmd2D_START;
          };
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
    #define     RA_dHubCmd2ND_DESC                             0x0004
    #define     RA_dHubCmd2ND_DESC_1D_ST                       0x0008
    #define     RA_dHubCmd2ND_DESC_1D_SZ                       0x000C
    #define     RA_dHubCmd2ND_DESC_2D_ST                       0x0010
    #define     RA_dHubCmd2ND_DESC_2D_SZ                       0x0014
    #define     RA_dHubCmd2ND_START                            0x0018
    #define     RA_dHubCmd2ND_CLEAR                            0x001C
    typedef struct SIE_dHubCmd2ND {
    #define     w32dHubCmd2ND_MEM                              {\
            UNSG32 uMEM_addr                                   : 32;\
          }
    union { UNSG32 u32dHubCmd2ND_MEM;
            struct w32dHubCmd2ND_MEM;
          };
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
    #define     w32dHubCmd2ND_DESC_1D_ST                       {\
            UNSG32 uDESC_1D_ST_step                            : 24;\
            UNSG32 RSVDx8_b24                                  :  8;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC_1D_ST;
            struct w32dHubCmd2ND_DESC_1D_ST;
          };
    #define     w32dHubCmd2ND_DESC_1D_SZ                       {\
            UNSG32 uDESC_1D_SZ_size                            : 24;\
            UNSG32 RSVDxC_b24                                  :  8;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC_1D_SZ;
            struct w32dHubCmd2ND_DESC_1D_SZ;
          };
    #define     w32dHubCmd2ND_DESC_2D_ST                       {\
            UNSG32 uDESC_2D_ST_step                            : 24;\
            UNSG32 RSVDx10_b24                                 :  8;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC_2D_ST;
            struct w32dHubCmd2ND_DESC_2D_ST;
          };
    #define     w32dHubCmd2ND_DESC_2D_SZ                       {\
            UNSG32 uDESC_2D_SZ_size                            : 24;\
            UNSG32 RSVDx14_b24                                 :  8;\
          }
    union { UNSG32 u32dHubCmd2ND_DESC_2D_SZ;
            struct w32dHubCmd2ND_DESC_2D_SZ;
          };
    #define     w32dHubCmd2ND_START                            {\
            UNSG32 uSTART_EN                                   :  1;\
            UNSG32 RSVDx18_b1                                  : 31;\
          }
    union { UNSG32 u32dHubCmd2ND_START;
            struct w32dHubCmd2ND_START;
          };
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
    typedef struct SIE_dHubQuery {
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
    #define     RA_dHubReg2D_CH_ST                             0x1440
    typedef struct SIE_dHubReg2D {
              SIE_dHubReg                                      ie_dHub;
              SIE_dHubCmd2D                                    ie_Cmd2D[16];
              SIE_dHubCmd2ND                                   ie_Cmd2ND[16];
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
    #define        avioDhubChMap_vpp128b_MV_R0                              0x0
    #define        avioDhubChMap_vpp128b_VMM_VX_R                           0x1
    #define        avioDhubChMap_vpp128b_Reserved0                          0x2
    #define        avioDhubChMap_vpp128b_MV_R1                              0x3
    #define        avioDhubChMap_vpp128b_1DSCL_W1                           0x4
    #define        avioDhubChMap_vpp128b_MetaData_RD                        0x5
    #define        avioDhubChMap_vpp128b_GFX_R1                             0x6
    #define        avioDhubChMap_vpp128b_DBEDR_RD_C                         0x7
    #define        avioDhubChMap_vpp128b_DBEDR_RD                           0x8
    #define        avioDhubChMap_vpp128b_1DSCL_W0                           0x9
    #define        avioDhubChMap_aio64b_MA0_R                               0x0
    #define        avioDhubChMap_aio64b_GFX2_R                              0x1
    #define        avioDhubChMap_aio64b_MA2_R                               0x2
    #define        avioDhubChMap_aio64b_MA3_R                               0x3
    #define        avioDhubChMap_aio64b_PDM_CH_W                            0x4
    #define        avioDhubChMap_aio64b_MIC1_CH_W                           0x5
    #define        avioDhubChMap_aio64b_MIC2_CH_W                           0x6
    #define        avioDhubChMap_aio64b_HDMI_R                              0x7
    #define        avioDhubChMap_aio64b_BCM_R                               0x8
    #define        avioDhubChMap_aio64b_MIC3_CH_W                           0x9
    #define        avioDhubChMap_aio64b_MIC4_CH_W                           0xA
    #define        avioDhubChMap_aio64b_MIC5_CH_W                           0xB
    #define        avioDhubChMap_aio64b_PDM_CH_R                            0xC
    #define        avioDhubChMap_aio64b_DMIC_PDM_CH_W                       0xD
    #define        avioDhubChMap_aio64b_SPDIF_R                             0xE
    #define        avioDhubChMap_aio64b_SPDIF_W                             0xF
    typedef struct SIE_avioDhubChMap {
            UNSG32 u_vpp128b                                   :  4;
            UNSG32 RSVDx0_b4                                   : 28;
            UNSG32 u_aio64b                                    :  4;
            UNSG32 RSVDx4_b4                                   : 28;
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
    #define        avioDhubTcmMap_vpp128bDhub_BANK0_START_ADDR 0x0
    #define        avioDhubTcmMap_vpp128bDhub_BANK0_SIZE       0x3C00
    #define        avioDhubTcmMap_vpp128bDhub_BANK1_START_ADDR 0x3C00
    #define        avioDhubTcmMap_vpp128bDhub_BANK1_SIZE       0x3C00
    #define        avioDhubTcmMap_vpp128bDhub_BANK2_START_ADDR 0x7800
    #define        avioDhubTcmMap_vpp128bDhub_BANK2_SIZE       0x3C00
    #define        avioDhubTcmMap_vpp128bDhub_BANK3_START_ADDR 0xB400
    #define        avioDhubTcmMap_vpp128bDhub_BANK3_SIZE       0x3C00
    #define        avioDhubTcmMap_aio64bDhub_BANK0_START_ADDR  0x0
    #define        avioDhubTcmMap_aio64bDhub_BANK0_SIZE        0x4A00
    #define     RA_avioDhubTcmMap_dummy                        0x0000
    typedef struct SIE_avioDhubTcmMap {
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
    #define        avioDhubSemMap_vpp128b_CH0_intr                          0x0
    #define        avioDhubSemMap_vpp128b_CH1_intr                          0x1
    #define        avioDhubSemMap_vpp128b_CH2_intr                          0x2
    #define        avioDhubSemMap_vpp128b_CH3_intr                          0x3
    #define        avioDhubSemMap_vpp128b_CH4_intr                          0x4
    #define        avioDhubSemMap_vpp128b_CH5_intr                          0x5
    #define        avioDhubSemMap_vpp128b_CH6_intr                          0x6
    #define        avioDhubSemMap_vpp128b_CH7_intr                          0x7
    #define        avioDhubSemMap_vpp128b_CH8_intr                          0x8
    #define        avioDhubSemMap_vpp128b_CH9_intr                          0x9
    #define        avioDhubSemMap_vpp128b_CH10_intr                         0xA
    #define        avioDhubSemMap_vpp128b_CH11_intr                         0xB
    #define        avioDhubSemMap_vpp128b_CH12_intr                         0xC
    #define        avioDhubSemMap_vpp128b_CH13_intr                         0xD
    #define        avioDhubSemMap_vpp128b_CH14_intr                         0xE
    #define        avioDhubSemMap_vpp128b_CH15_intr                         0xF
    #define        avioDhubSemMap_vpp128b_vpp_inr0                          0x10
    #define        avioDhubSemMap_vpp128b_vpp_inr1                          0x11
    #define        avioDhubSemMap_vpp128b_vpp_inr2                          0x12
    #define        avioDhubSemMap_vpp128b_vpp_inr3                          0x13
    #define        avioDhubSemMap_vpp128b_vpp_inr4                          0x14
    #define        avioDhubSemMap_vpp128b_vpp_inr5                          0x15
    #define        avioDhubSemMap_vpp128b_vpp_inr6                          0x16
    #define        avioDhubSemMap_vpp128b_vpp_inr7                          0x17
    #define        avioDhubSemMap_vpp128b_vpp_inr8                          0x18
    #define        avioDhubSemMap_vpp128b_vpp_inr9                          0x19
    #define        avioDhubSemMap_vpp128b_vpp_inr10                         0x1A
    #define        avioDhubSemMap_vpp128b_vpp_inr11                         0x1B
    #define        avioDhubSemMap_vpp128b_vpp_inr12                         0x1C
    #define        avioDhubSemMap_vpp128b_vpp_inr13                         0x1D
    #define        avioDhubSemMap_vpp128b_vpp_inr14                         0x1E
    #define        avioDhubSemMap_vpp128b_vpp_inr15                         0x1F
    #define        avioDhubSemMap_csi128b_CH0_intr                          0x0
    #define        avioDhubSemMap_csi128b_CH1_intr                          0x1
    #define        avioDhubSemMap_csi128b_CH2_intr                          0x2
    #define        avioDhubSemMap_csi128b_CH3_intr                          0x3
    #define        avioDhubSemMap_csi128b_CH4_intr                          0x4
    #define        avioDhubSemMap_csi128b_CH5_intr                          0x5
    #define        avioDhubSemMap_csi128b_CH6_intr                          0x6
    #define        avioDhubSemMap_csi128b_CH7_intr                          0x7
    #define        avioDhubSemMap_csi128b_CH8_intr                          0x8
    #define        avioDhubSemMap_csi128b_CH9_intr                          0x9
    #define        avioDhubSemMap_csi128b_CH10_intr                         0xA
    #define        avioDhubSemMap_csi128b_CH11_intr                         0xB
    #define        avioDhubSemMap_csi128b_CH12_intr                         0xC
    #define        avioDhubSemMap_csi128b_CH13_intr                         0xD
    #define        avioDhubSemMap_csi128b_CH14_intr                         0xE
    #define        avioDhubSemMap_csi128b_CH15_intr                         0xF
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
            UNSG32 u_vpp128b                                   :  5;
            UNSG32 RSVDx0_b5                                   : 27;
            UNSG32 u_csi128b                                   :  5;
            UNSG32 RSVDx4_b5                                   : 27;
            UNSG32 u_aio64b                                    :  5;
            UNSG32 RSVDx8_b5                                   : 27;
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
    typedef struct SIE_vppTcmEntry {
            UNSG32 u_dat                                       : 32;
    } SIE_vppTcmEntry;
     SIGN32 vppTcmEntry_drvrd(SIE_vppTcmEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 vppTcmEntry_drvwr(SIE_vppTcmEntry *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void vppTcmEntry_reset(SIE_vppTcmEntry *p);
     SIGN32 vppTcmEntry_cmp  (SIE_vppTcmEntry *p, SIE_vppTcmEntry *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define vppTcmEntry_check(p,pie,pfx,hLOG) vppTcmEntry_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define vppTcmEntry_print(p,    pfx,hLOG) vppTcmEntry_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_vpp128bDhub
#define h_vpp128bDhub (){}
    #define     RA_vpp128bDhub_tcm0                            0x0000
    #define     RA_vpp128bDhub_dHub0                           0x10000
    typedef struct SIE_vpp128bDhub {
              SIE_vppTcmEntry                                  ie_tcm0[13824];
             UNSG8 RSVD_tcm0                                   [10240];
              SIE_dHubReg2D                                    ie_dHub0;
             UNSG8 RSVDx11500                                  [60160];
    } SIE_vpp128bDhub;
     SIGN32 vpp128bDhub_drvrd(SIE_vpp128bDhub *p, UNSG32 base, SIGN32 mem, SIGN32 tst);
     SIGN32 vpp128bDhub_drvwr(SIE_vpp128bDhub *p, UNSG32 base, SIGN32 mem, SIGN32 tst, UNSG32 *pcmd);
       void vpp128bDhub_reset(SIE_vpp128bDhub *p);
     SIGN32 vpp128bDhub_cmp  (SIE_vpp128bDhub *p, SIE_vpp128bDhub *pie, char *pfx, void *hLOG, SIGN32 mem, SIGN32 tst);
    #define vpp128bDhub_check(p,pie,pfx,hLOG) vpp128bDhub_cmp(p,pie,pfx,(void*)(hLOG),0,0)
    #define vpp128bDhub_print(p,    pfx,hLOG) vpp128bDhub_cmp(p,0,  pfx,(void*)(hLOG),0,0)
#endif
#ifndef h_aio64bDhub
#define h_aio64bDhub (){}
    #define     RA_aio64bDhub_tcm0                             0x0000
    #define     RA_aio64bDhub_dHub0                            0x8000
    typedef struct SIE_aio64bDhub {
              SIE_vppTcmEntry                                  ie_tcm0[4736];
             UNSG8 RSVD_tcm0                                   [13824];
              SIE_dHubReg2D                                    ie_dHub0;
             UNSG8 RSVDx9500                                   [27392];
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
