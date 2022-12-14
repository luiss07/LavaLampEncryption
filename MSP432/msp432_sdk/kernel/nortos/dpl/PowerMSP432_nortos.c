/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ======== PowerMSP432_nortos.c ========
 */

#include <stdbool.h>
#include <stdint.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/dpl/SwiP.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerMSP432.h>

extern PowerMSP432_ModuleState PowerMSP432_module;

uintptr_t PowerMSP432_swiKey;


/*
 *  ======== PowerMSP432_deepSleepPolicy ========
 */
void PowerMSP432_deepSleepPolicy()
{
    uint32_t  constraints;
    bool      slept = false;
    uintptr_t key;
    uintptr_t swiKey;

    /* disable interrupts */
    key = HwiP_disable();

    swiKey = SwiP_disable();

    /* query the declared constraints */
    constraints = Power_getConstraintMask();

    /*
     *  Check if can go to a sleep state, starting with the deepest level.
     *  Do not go to a sleep state if a lesser sleep state is disallowed.
     */

     /* check if can go to DEEPSLEEP_1 */
    if ((constraints & ((1 << PowerMSP432_DISALLOW_SLEEP) |
                        (1 << PowerMSP432_DISALLOW_DEEPSLEEP_0) |
                        (1 << PowerMSP432_DISALLOW_DEEPSLEEP_1))) == 0) {

        /* go to DEEPSLEEP_1 */
        Power_sleep(PowerMSP432_DEEPSLEEP_1);

        /* set 'slept' to true*/
        slept = true;
    }

    /* if didn't sleep yet, now check if can go to DEEPSLEEP_0 */
    if (!slept && ((constraints & ((1 << PowerMSP432_DISALLOW_SLEEP) |
                        (1 << PowerMSP432_DISALLOW_DEEPSLEEP_0))) == 0)) {

        /* go to DEEPSLEEP_0 */
        Power_sleep(PowerMSP432_DEEPSLEEP_0);

        /* set 'slept' to true*/
        slept = true;
    }

    /* if didn't sleep yet, now check if can go to SLEEP */
    if (!slept && ((constraints & (1 << PowerMSP432_DISALLOW_SLEEP)) == 0)) {

        /* go to SLEEP */
        Power_sleep(PowerMSP432_SLEEP);

        /* set 'slept' to true*/
        slept = true;
    }

    SwiP_restore(swiKey);

    /* re-enable interrupts */
    HwiP_restore(key);

    /* if didn't sleep yet, just do WFI */
    if (!slept) {
        __asm(" wfi");
    }
}

/*
 *  ======== PowerMSP432_sleepPolicy ========
 */
void PowerMSP432_sleepPolicy()
{
    uint32_t  constraints;
    uintptr_t key;
    uintptr_t swiKey;
    bool      slept = false;

    /* disable interrupts */
    key = HwiP_disable();

    /*
     * Check if the Power policy has been disabled since we last checked.
     * Since we're in this policy function already, the policy must have
     * been enabled (with a valid policyFxn) when we were called, but
     * could have been disbled to short-circuit this function.
     * SemaphoreP_post() does this purposely (see comments in there).
     */
    if (!PowerMSP432_module.enablePolicy) {
        HwiP_restore(key);

        return;
    }

    swiKey = SwiP_disable();

    /* Query the declared constraints */
    constraints = Power_getConstraintMask();

    /* sleep, if there is no constraint prohibiting it */
    if ((constraints & (1 << PowerMSP432_DISALLOW_SLEEP)) == 0) {

        /* go to SLEEP */
        Power_sleep(PowerMSP432_SLEEP);

        /* set 'slept' to true*/
        slept = true;
    }

    SwiP_restore(swiKey);

    /* re-enable interrupts */
    HwiP_restore(key);

    /* if didn't sleep yet, just do WFI */
    if (!slept) {
        __asm(" wfi");
    }
}

/*
 *  ======== PowerMSP432_initPolicy ========
 */
void PowerMSP432_initPolicy()
{
}


/*
 *  ======== PowerMSP432_schedulerDisable ========
 */
void PowerMSP432_schedulerDisable()
{
    PowerMSP432_swiKey = SwiP_disable();
}

/*
 *  ======== PowerMSP432_schedulerRestore ========
 */
void PowerMSP432_schedulerRestore()
{
    SwiP_restore(PowerMSP432_swiKey);
}

/*
 *  ======== PowerMSP432_updateFreqs ========
 */
void PowerMSP432_updateFreqs(PowerMSP432_Freqs *freqs)
{
}
