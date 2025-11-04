/*
 * Driver for the Renesas RZ/V2H, RZ/V2N DRP-AI unit
 *
 * Copyright (C) 2023-2024 Renesas Electronics Corporation
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
#ifndef R_DRP_LOCK_H
#define R_DRP_LOCK_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* The following macros are used in exclusion control of DRP-AI */
#define DRPFLAG_DRPAI_LOCK  (1)
#define DRPFLAG_DRP_LOCK    (2)

#define INTERRUPT_CONTEXT     (0)
#define PROCESS_CONTEXT       (1)


int R_DRP_LockDrpaiContStatus(unsigned long long *addr,
                              spinlock_t *lock,
                              unsigned int context,
                              unsigned int num);
int R_DRP_UnlockDrpaiContStatus(unsigned long long *addr,
                                spinlock_t *lock,
                                unsigned int context,
                                unsigned int num);
int R_DRP_IsDrpaiContStatusLocked(unsigned long long *addr, unsigned int num);
int R_DRP_LockResetContStatusBit(unsigned long long *addr,
                                 spinlock_t *lock,
                                 unsigned int context);
int R_DRP_UnlockResetContStatusBit(unsigned long long *addr,
                                   spinlock_t *lock,
                                   unsigned int context);
int R_DRP_RecordDrpaiActiveStatus(unsigned long long *addr,
                                  spinlock_t *lock,
                                  unsigned int context);
int R_DRP_RecordDrpaiInactiveStatus(unsigned long long *addr,
                                    spinlock_t *lock,
                                    unsigned int context);
int R_DRP_IsDrpaiHwStatusActive(unsigned long long *addr);
int R_DRP_InitializeSharedMemory(unsigned long long *addr);
int R_DRP_IsSharedMemoryInitialized(unsigned long long *addr);
int R_DRP_ClearFlagManagementArea(unsigned long long *addr);
int R_DRP_ClearHashArea(unsigned long long *addr);
struct reset_control * R_DRP_GetResetContPointerFromSharedMemory(unsigned long long *addr);
int R_DRP_SetResetContPointerToSharedMemory(unsigned long long *addr, struct reset_control * rstc);

int lock_drp(unsigned long long *addr, unsigned int num);
int unlock_drp(unsigned long long * addr, unsigned int num);
 
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* R_DRP_LOCK_H */
