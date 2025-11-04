/*
 * Driver for the Renesas RZ/V2H, RZ/V2N DRP-AI unit
 *
 * Copyright (C) 2024 Renesas Electronics Corporation
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef R_DRPAI_LOCK_H
#define R_DRPAI_LOCK_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** The hash value are generated with the following command.
 *    echo "CONFIG_DRP_SUPPORT_SHARED_MEMORY" | md5sum
 */
#define INIT_HASH_TOP_VAL                   (0x9bca83cf9c5b7f1e)
#define INIT_HASH_BOTTOM_VAL                (0xa30b890ea28c78e7)

/* The following macros are used in exclusion control of DRP-AI */
#define DRPFLAG_DRPAI_LOCK  (1)
#define DRPFLAG_DRP_LOCK    (2)

#define INTERRUPT_CONTEXT     (0)
#define PROCESS_CONTEXT       (1)

int R_DRPAI_LockDrpaiContStatus(unsigned long long *addr,
                              spinlock_t *lock,
                              unsigned int context,
                              unsigned int num);
int R_DRPAI_UnlockDrpaiContStatus(unsigned long long *addr,
                                spinlock_t *lock,
                                unsigned int context,
                                unsigned int num);
int R_DRPAI_IsDrpaiContStatusLocked(unsigned long long *addr, unsigned int num);
int R_DRPAI_LockResetContStatusBit(unsigned long long *addr,
                                 spinlock_t *lock,
                                 unsigned int context);
int R_DRPAI_UnlockResetContStatusBit(unsigned long long *addr,
                                   spinlock_t *lock,
                                   unsigned int context);
int R_DRPAI_RecordDrpaiActiveStatus(unsigned long long *addr,
                                  spinlock_t *lock,
                                  unsigned int context);
int R_DRPAI_RecordDrpaiInactiveStatus(unsigned long long *addr,
                                    spinlock_t *lock,
                                    unsigned int context);
int R_DRPAI_IsDrpaiHwStatusActive(unsigned long long *addr);
int R_DRPAI_InitializeSharedMemory(unsigned long long *addr);
int R_DRPAI_ClearFlagManagementArea(unsigned long long *addr);
int R_DRPAI_ClearHashArea(unsigned long long *addr);
int R_DRPAI_IsSharedMemoryInitialized(unsigned long long *addr);
struct reset_control * R_DRPAI_GetResetContPointerFromSharedMemory(unsigned long long *addr);
int R_DRPAI_SetResetContPointerToSharedMemory(unsigned long long *addr, struct reset_control * rstc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* R_DRPAI_LOCK_H */
