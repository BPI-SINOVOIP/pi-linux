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
#include <linux/stddef.h>
#include <linux/device.h>
#include <linux/delay.h>
#include "lock_drp.h"

/** The hash value are generated with the following command.
 *    echo "CONFIG_DRP_SUPPORT_SHARED_MEMORY" | md5sum
 */
#define INIT_HASH_TOP_VAL                   (0x9bca83cf9c5b7f1e)
#define INIT_HASH_BOTTOM_VAL                (0xa30b890ea28c78e7)

#define WRITE_CONTROL_BIT     (0)
#define RESET_CONT_STATUS_BIT (3)
#define DRPAI_HW_STATUS_BIT   (4)

#define FLAG_MANAGE_MEMORY         (0x0)
#define RESET_CONT_STORE_MEMORY    (0x1)
#define HASH_TOP_STORE_MEMORY      (0x2)
#define HASH_BOTTOM_STORE_MEMORY   (0x3)

#define LOCK_TIMEOUT_US (1000)  // 1000us = 1ms
#define DELAY_TIME_US   (10)

int lock_drp(unsigned long long *addr, unsigned int num)
{
    int result = 0;
    unsigned long long old_value;

    /* Check Arguments. */
    if ((NULL == addr) || (((unsigned long long)addr & 0x7uLL) != 0) || (num >= 64))
    {
        result = -2;
    }
    else
    {
        /* set the specified bit. */
        old_value = __sync_fetch_and_or(addr, (1uLL << num));
        /* Check previous value. */
        if ((old_value & (1uLL << num)) != 0)
        {
          result = -1;
        }
    }

    return result;
}

int unlock_drp(unsigned long long *addr, unsigned int num)
{
    int result = 0;

    /* Check Arguments. */
    if ((NULL == addr) || (((unsigned long long)addr & 0x7uLL) != 0) || (num >= 64))
    {
        result = -2;
    }
    else
    {
        /* Clear the specified bit. */
        (void)__sync_fetch_and_and(addr, ~(1uLL << num));
    }
    return result;
}

static int lock_write_control(unsigned long long *addr)
{
    int result = 0;
    int elapsed_us_time = 0;

    while(0 != (__sync_fetch_and_or(addr, (1uLL << WRITE_CONTROL_BIT)) & (1uLL << WRITE_CONTROL_BIT)))
    {
        udelay(DELAY_TIME_US);

        elapsed_us_time += DELAY_TIME_US;
        if(elapsed_us_time > LOCK_TIMEOUT_US)
        {
            result = -1;
            goto end;
        }
    }

end:
    return result;
}

static int unlock_write_control(unsigned long long *addr)
{
    int result = 0;
    /* Clear the specified bit. */
    (void)__sync_fetch_and_and(addr, ~(1uLL << WRITE_CONTROL_BIT));

    return result;
}

/** Test and set the specified bit on shared memory.
 * @param *addr Base address for shared memory.
 * @param *lock Spinlock pointer.
 * @param context Indicates in which context this function was called.
 * @param num A bit to be set to 1.
 * @return Whether this function succeeded or not
 * @retval 0 Success
 * @retval -1 A specified bit has already set by another driver.
 * @retval -2 Invalid argument is set.
 * @retval -3 Failed to lock write control bit
 */
static int test_and_set_bit_on_shared_mem(unsigned long long *addr,
                                          spinlock_t *lock,
                                          unsigned int context,
                                          unsigned int num)
{
    int result = 0;
    int write_ctr_lock_status = -1;
    unsigned long flags;

    if (PROCESS_CONTEXT == context)
    {
        // To prevent interruption during critical section.
        spin_lock_irqsave(lock, flags);
    }

    /* Check Arguments. */
    if (NULL == addr) 
    {
        result = -2;
    }

    // Lock write control bit 
    write_ctr_lock_status = lock_write_control(addr);
    if(0 != write_ctr_lock_status)
    {
        // TIMEOUT
        result = -3;
        goto end;
    }

    // If RESET_CONT_STATUS_BIT is 1,
    // it means that reset controler has been initilized by a driver.
    if ((*(addr+FLAG_MANAGE_MEMORY) & ( 1uLL << num)) != 0)
    {
        result = -1;
        goto end;
    }
    *(addr+FLAG_MANAGE_MEMORY) = (*(addr+FLAG_MANAGE_MEMORY) | (0x1 << num));

    goto end;
end:
    if(0 == write_ctr_lock_status)
    {
        unlock_write_control(addr);
    }
    if (PROCESS_CONTEXT == context)
    {
        spin_unlock_irqrestore(lock, flags);
    }
    return result;
}

/** Clear the specified bit on shared memory.
 * @param *addr Base address for shared memory.
 * @param *lock Spinlock pointer.
 * @param context Indicates in which context this function was called.
 * @param num A bit to be set to 0.
 * @return Whether this function succeeded or not
 * @retval 0 Success
 * @retval -2 Invalid argument is set.
 * @retval -3 Failed to lock write control bit
 */
static int clear_bit_on_shared_mem(unsigned long long *addr,
                                      spinlock_t *lock,
                                      unsigned int context,
                                      unsigned int num)
{
    int result = 0;
    int write_ctr_lock_status = -1;
    unsigned long flags;

    if (PROCESS_CONTEXT == context)
    {
        // To prevent interruption during critical section.
        spin_lock_irqsave(lock, flags);
    }

    /* Check Arguments. */
    if (NULL == addr)
    {
        result = -2;
        goto end;
    }

    // Lock write control bit 
    write_ctr_lock_status = lock_write_control(addr);
    if(0 != write_ctr_lock_status)
    {
        // TIMEOUT
        result = -3;
        goto end;
    }

    /* Clear the specified bit. */
    *(addr+FLAG_MANAGE_MEMORY) = (*(addr+FLAG_MANAGE_MEMORY) & ~(1uLL << num));

    goto end;
end:
    if(0 == write_ctr_lock_status)
    {
        unlock_write_control(addr);
    }
    if (PROCESS_CONTEXT == context)
    {
        spin_unlock_irqrestore(lock, flags);
    }
    return result;
}

/** Check a specifed bit on shared memory whether it is set.
 * @param *addr Base address for shared memory.
 * @param num A bit to be checked.
 * @return Whether this function succeeded or not
 * @retval 0 Success
 * @retval -1 A specified bit is cleared.
 * @retval -2 Invalid argument is set.
 */
static int is_shared_memory_bit_set(unsigned long long *addr, unsigned int num)
{
    int result = 0;

    /* Check Arguments. */
    if (NULL == addr)
    {
        result = -2;
        goto end;
    }

    if ((*addr & ( 0x1 << num)) != 0)
    {
        // num bit is used by driver.
        /* do nothing */
    }
    else
    {
        // num bit is not used by driver.
        result = -1;
        goto end;
    }
    goto end;
end:
    return result;
}

/*******************************************************************************
 * Public APIs
 *******************************************************************************/
/** Lock shared memory to use be used in exclusion control of DRP-AI
 * @param addr Base address for a shared memory.
 * @param lock A pointer for spinlock to prevent interrupts in process context 
 *             during performing this function.
 * @param context Specify whether this function is called by a process or an interrupt.
 *                If this function called by a process context, set PROCESS_CONTEXT.
 * @param num Bit number in the shared memory area that manages the driver status.
 *            Set DRPFLAG_DRP_LOCK(=0x2) for the DRP driver.
 *            Set DRPFLAG_DRPAI_LOCK(=0x1) for the DRP-AI driver.
 * @return Whether this function succeeded or not
 * @retval 0 Success
 * @retval -1 Failed to lock shared memory
 * @retval -2 Invalid argument is set.
 * @retval -3 Failed to lock write control bit
 */
int R_DRP_LockDrpaiContStatus(unsigned long long *addr,
                              spinlock_t *lock,
                              unsigned int context,
                              unsigned int num)
{
    int result = 0;
    int write_ctr_lock_status = -1;
    unsigned long flags;
    unsigned int shift_val;

    if(PROCESS_CONTEXT == context)
    {
        // To prevent interruption during critical section.
        spin_lock_irqsave(lock, flags);
    }

    /* Check Arguments. */
    if (NULL == addr)
    {
        result = -2;
        goto end;
    }

    // Lock write control bit 
    write_ctr_lock_status = lock_write_control(addr);
    if(0 != write_ctr_lock_status)
    {
        // Failed to get write control
        // Error causes are TIMEOUT.
        result = -3;
        goto end;
    }

    // The following is bit manipulation example here.
    //
    // Case1: DRP Driver locked, DRPAI Driver try locking.
    // addr = 0b1100
    // num = DRPFLAG_DRPAI_LOCK (0b01) => ~(not operation) => 0b10 
    // 0x1 is shifted by 0b10 => 0b100
    // addr is masked by 0b100 => 0b0100. It means a locked status.
    //
    // Case2: nolocked, DRPAI Driver try locking
    // addr = 0b1000
    // num = DRPFLAG_DRPAI_LOCK (0b01) => ~(not operation) => 0b10 
    // 0x1 is shifted by 0b10 => 0b100
    // addr is masked by 0b100 => 0b0000. It means a no locked status.
    shift_val = (~num) & 0x3;
    if ((*(addr+FLAG_MANAGE_MEMORY) & (0x1 << shift_val)) != 0)
    {
        result = -1;
        goto end;
    }

    // Set 1 to DRPAI bit to get a DRPAI control.
    // If DRP bit is 1, DRPAI is used by DRP Driver.
    // When DRPAI is used by DRP Driver, this function returns -1
    // If DRPAI is used from DRPAI driver, this function returns 0. 
    // This is because it is assumed that this function is already protected 
    // by semaphores or spinlock or status transition management in the context in which it is called.
    *(addr+FLAG_MANAGE_MEMORY) = (*(addr+FLAG_MANAGE_MEMORY) |(0x1 << num));

    goto end;
end:
    if(0 == write_ctr_lock_status)
    {
        unlock_write_control(addr);
    }
    if (PROCESS_CONTEXT == context)
    {
        spin_unlock_irqrestore(lock, flags);
    }

    return result;
}

/** Unlock shared memory to be used in exclusion control of DRP-AI
 * @param addr Base address for a shared memory.
 * @param lock A pointer for spinlock to prevent interrupts in process context 
 *             during performing this function.
 * @param context Specify whether this function is called by a process or an interrupt.
 *                If this function called by a process context, set PROCESS_CONTEXT.
 * @param num Bit number in the shared memory area that manages the driver status.
 *            Set DRPFLAG_DRP_LOCK(=0x2) for the DRP driver.
 *            Set DRPFLAG_DRPAI_LOCK(=0x1) for the DRP-AI driver.
 * @return Whether this function succeeded or not
 * @retval 0 Success
 * @retval -2 Invalid argument is set.
 * @retval -3 Failed to lock write control bit
 */
int R_DRP_UnlockDrpaiContStatus(unsigned long long *addr,
                                spinlock_t *lock,
                                unsigned int context,
                                unsigned int num)
{
    return clear_bit_on_shared_mem(addr, lock, context, num);
}

/** Check whether DRPAI is used by someone.
 * @param addr Base address for a shared memory.
 * @param num Bit number in the shared memory area that manages the driver status.
 *            Set DRPFLAG_DRP_LOCK(=0x2) for the DRP driver.
 *            Set DRPFLAG_DRPAI_LOCK(=0x1) for the DRP-AI driver.
 * @return Status on shared memory for exculsion control of DRP-AI
 * @retval 0 Success. DRP-AI is used by a num bit Driver.
 * @retval 1 Nobody uses DRP-AI.
 * @retval -1 DRP-AI is not used by a num bit Driver
 * @retval -2 Invalid argument is set.
 * @note Only read the shared memory which is used by exclude control
 */
int R_DRP_IsDrpaiContStatusLocked(unsigned long long *addr, unsigned int num)
{
    int tilde_num;  
    int is_num_bit_locked = -1;
    int is_tilde_num_bit_locked = -1;

    // If num=DRPFLAG_DRP_LOCK(0x2), tilde_num becomes 0x1(=DRPFLAG_DRPAI_LOCK)
    // If num=DRPFLAG_DRPAI_LOCK(0x1), tilde_num becomes 0x2(=DRPFLAG_DRP_LOCK)
    tilde_num = (~num) & 0x3;

    is_num_bit_locked = is_shared_memory_bit_set(addr, num);
    is_tilde_num_bit_locked = is_shared_memory_bit_set(addr, tilde_num);

    // If both bits are not locked (DRPFLAG_DRPAI_LOCK bit is 0 and DRPFLAG_DRP_LOCK bit is 0),
    // nobody is using the DRP-AI.
    if((-1 == is_num_bit_locked) && (-1 == is_tilde_num_bit_locked))
    {
        is_num_bit_locked = 1;
    }

    return is_num_bit_locked;
}


/** After confirming that the reset control pointer can be used, 
 *  Lock shared memory to use be used in exclusion control of DRP-AI.
 * @param addr Base address for a shared memory.
 * @param lock A pointer for spinlock to prevent interrupts in process context 
 *             during performing this function.
 * @param context Specify whether this function is called by a process or an interrupt.
 *                If this function called by a process context, set PROCESS_CONTEXT.
 * @return Whether this function succeeded or not
 * @retval 0 Success
 * @retval -1 Another driver already has initlized the reset control pointer.
 * @retval -2 Invalid argument is set.
 * @retval -3 Failed to lock write control bit
*/
int R_DRP_LockResetContStatusBit(unsigned long long *addr,
                                 spinlock_t *lock,
                                 unsigned int context)
{
    return test_and_set_bit_on_shared_mem(addr, lock, context, RESET_CONT_STATUS_BIT);
}

/** Unlock shared memory to use be used in exclusion control of DRP-AI.
 * @param addr Base address for a shared memory.
 * @param lock A pointer for spinlock to prevent interrupts in process context 
 *             during performing this function.
 * @param context Specify whether this function is called by a process or an interrupt.
 *                If this function called by a process context, set PROCESS_CONTEXT.
 * @return Whether this function succeeded or not
 * @retval 0 Success
 * @retval -2 Invalid argument is set.
 * @retval -3 Failed to lock write control bit
*/
int R_DRP_UnlockResetContStatusBit(unsigned long long *addr,
                                   spinlock_t *lock,
                                   unsigned int context)
{
    return clear_bit_on_shared_mem(addr, lock, context, RESET_CONT_STATUS_BIT);
}

/** Record in a shared memory that DRP-AI is active.
 * @param addr Base address for a shared memory.
 * @param lock A pointer for spinlock to prevent interrupts in process context 
 *             during performing this function.
 * @param context Specify whether this function is called by a process or an interrupt.
 *                If this function called by a process context, set PROCESS_CONTEXT.
 * @return Whether this function succeeded or not
 * @retval 0 Successfully record that DRP-AI has been initialised.
 * @retval -1 Another driver already has initlized the DRP-AI HW.
 * @retval -2 Invalid argument is set.
 * @retval -3 Failed to lock write control bit
*/
int R_DRP_RecordDrpaiActiveStatus(unsigned long long *addr,
                                  spinlock_t *lock,
                                  unsigned int context)
{
    return test_and_set_bit_on_shared_mem(addr, lock, context, DRPAI_HW_STATUS_BIT);
}

/** Record in a shared memory that DRP-AI is inactive.
 * @param addr Base address for a shared memory.
 * @param lock A pointer for spinlock to prevent interrupts in process context 
 *             during performing this function.
 * @param context Specify whether this function is called by a process or an interrupt.
 *                If this function called by a process context, set PROCESS_CONTEXT.
 * @return Whether this function succeeded or not
 * @retval 0 Successfully record that DRP-AI has been stopped.
 * @retval -2 Invalid argument is set.
 * @retval -3 Failed to lock write control bit
*/
int R_DRP_RecordDrpaiInactiveStatus(unsigned long long *addr,
                                    spinlock_t *lock,
                                    unsigned int context)
{
    return clear_bit_on_shared_mem(addr, lock, context, DRPAI_HW_STATUS_BIT);
}

/** Check whether that DRP-AI is active.
 * @param addr Base address for a shared memory.
 * @return Whether this function succeeded or not
 * @retval 0 DRP-AI has already been initialised.
 * @retval -1 DRP-AI is stopped. Must initialise DRP-AI.
 * @retval -2 Invalid argument is set.
*/
int R_DRP_IsDrpaiHwStatusActive(unsigned long long *addr)
{
    return is_shared_memory_bit_set(addr, DRPAI_HW_STATUS_BIT);
}

/** Initialize a shared memory.
 * @param addr Base address for a shared memory.
 * @return Whether this function succeeded or not
 * @retval 0 Successfully initialize a shared memory. Shared memory has not yet been initialized,
 *           but the HASH values were successfully written.
 * @retval -2 Invalid argument is set.
 * @note Write HASH to HASH_TOP_STORE_MEMORY and HASH_BOTTOM_STORE_MEMORY.
*/
int R_DRP_InitializeSharedMemory(unsigned long long *addr)
{
    int result = 0;

    /* Check Arguments. */
    if (NULL == addr) 
    {
        result = -2;
        goto end;
    }

    *(addr+HASH_TOP_STORE_MEMORY) = 0uLL;
    *(addr+HASH_BOTTOM_STORE_MEMORY) = 0uLL;
    *(addr+HASH_TOP_STORE_MEMORY) = (*(addr+HASH_TOP_STORE_MEMORY) | INIT_HASH_TOP_VAL);
    *(addr+HASH_BOTTOM_STORE_MEMORY) = (*(addr+HASH_BOTTOM_STORE_MEMORY) | INIT_HASH_BOTTOM_VAL);

    goto end;
end:
    return result;
}

/** Clear the flag management area in a shared memory.
 * @param addr Base address for a shared memory.
 * @return Whether this function succeeded or not
 * @retval 0 Successfully clear a shared memory. Write 0 to shared memory.
 * @retval -2 Invalid argument is set.
 * @note Write 0 to FLAG_MANAGE_MEMORY area.
*/
int R_DRP_ClearFlagManagementArea(unsigned long long *addr)
{
    int result = 0;

    /* Check Arguments. */
    if (NULL == addr) 
    {
        result = -2;
        goto end;
    }

    *(addr+FLAG_MANAGE_MEMORY) = 0uLL;

    goto end;
end:
    return result;
}

/** Clear the hash area in a shared memory.
 * @param addr Base address for a shared memory.
 * @return Whether this function succeeded or not
 * @retval 0 Successfully clear a shared memory. Write 0 to shared memory.
 * @retval -2 Invalid argument is set.
 * @note Write 0 to FLAG_MANAGE_MEMORY area. Write 0 to HASH_TOP_STORE_MEMORY and HASH_BOTTOM_STORE_MEMORY.
*/
int R_DRP_ClearHashArea(unsigned long long *addr)
{
    int result = 0;

    /* Check Arguments. */
    if (NULL == addr) 
    {
        result = -2;
        goto end;
    }

    *(addr+HASH_TOP_STORE_MEMORY) = 0uLL;
    *(addr+HASH_BOTTOM_STORE_MEMORY) = 0uLL;

    goto end;
end:
    return result;
}

/** Check whether a shared memory is initilized with HASH.
 * @param addr Base address for a shared memory.
 * @retval 0 Shared memory has not yet been initialized,
 * @retval -1 Shared memory has already been initialized
 * @retval -2 Invalid argument is set.
 */
int R_DRP_IsSharedMemoryInitialized(unsigned long long *addr)
{
    int result = 0;
    /* Check Arguments. */
    if (NULL == addr) 
    {
        result = -2;
        goto end;
    }

    // If memory has been initilized, returns -1 as error.
    if ((*(addr+HASH_TOP_STORE_MEMORY) == INIT_HASH_TOP_VAL) && (*(addr+HASH_BOTTOM_STORE_MEMORY) == INIT_HASH_BOTTOM_VAL))
    {
        result = -1;
        goto end;
    }
end:
    return result;
}

/** Get a reset controller pointer from shared memory.
 * @param addr Base address for a shared memory.
 * @return A pointer to the reset controller set in shared memory.
 * @retval a reset controller pointer
 */
struct reset_control * R_DRP_GetResetContPointerFromSharedMemory(unsigned long long *addr)
{
    return (struct reset_control*)*((addr + RESET_CONT_STORE_MEMORY));
}

/** Set a reset controller pointer to shared memory.
 * @param addr Base address for a shared memory.
 * @param rstc A pointer to be set in shared memory.
 * @return Always returns 0
 * @retval 0 always returns 0
 */
int R_DRP_SetResetContPointerToSharedMemory(unsigned long long *addr, struct reset_control * rstc)
{
    void ** p_rstc;

    p_rstc = (void**)(addr + RESET_CONT_STORE_MEMORY);
    *p_rstc = rstc;
    return 0;
}
