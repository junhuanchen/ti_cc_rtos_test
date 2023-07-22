/*
 * Copyright (c) 2015-2019, Texas Instruments Incorporated
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
 *  ======== watchdog.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/Watchdog.h>

/* Driver configuration */
#include "ti_drivers_config.h"

#define TIMEOUT_MS 1000
#define SLEEP_US   500000

/*
 *  ======== watchdogCallback ========
 */
void watchdogCallback(uintptr_t watchdogHandle)
{
    Power_reset();
}

/*
 *  ======== gpioButtonIsr ========
 */
void gpioButtonIsr0(uint_least8_t index)
{
    GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);

    // while (1) {}
}

/*
 *  ======== gpioButtonIsr ========
 */
void gpioButtonIsr1(uint_least8_t index)
{
    GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);

    // while (1) {} // this will trigger the watchdog
}

struct _test_empty_
{
    /* data */
} test_empty;

void test_wdt_init()
{

    Watchdog_Handle watchdogHandle;
    Watchdog_Params params;
    uint32_t reloadValue;
    
    Watchdog_init();
    /* Open a Watchdog driver instance */
    Watchdog_Params_init(&params);
    params.callbackFxn    = (Watchdog_Callback)watchdogCallback;
    params.debugStallMode = Watchdog_DEBUG_STALL_ON;
    params.resetMode      = Watchdog_RESET_ON;

    watchdogHandle = Watchdog_open(CONFIG_WATCHDOG_0, &params);
    if (watchdogHandle == NULL)
    {
        /* Error opening Watchdog */
        while (1) {}
    }

    reloadValue = Watchdog_convertMsToTicks(watchdogHandle, TIMEOUT_MS);

    if (reloadValue != 0)
    {
        Watchdog_setReload(watchdogHandle, reloadValue);
    }

}

void test_wdt_loop()
{

    /*
        * Disabling power policy will prevent the device from entering
        * low power state. The device will stay awake when the CPU is
        * idle.
        */
    Power_disablePolicy();

    /* Sleep for SLEEP_US before clearing the watchdog */
    usleep(SLEEP_US);
    Watchdog_clear(watchdogHandle);
    // GPIO_toggle(CONFIG_GPIO_LED_0);
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    /*
        * Enabling power policy will allow the device to enter a low
        * power state when the CPU is idle. How the watchdog peripheral
        * behaves in a low power state is device specific.
        */
    Power_enablePolicy();

    /* Sleep for SLEEP_US before clearing the watchdog */
    usleep(SLEEP_US);
    Watchdog_clear(watchdogHandle);
    // GPIO_toggle(CONFIG_GPIO_LED_0);
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);

}

void test_main_init()
{
    /* Call driver init functions */
    GPIO_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0_INPUT, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0_INPUT, gpioButtonIsr0);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_1_INPUT, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_GPIO_BUTTON_1_INPUT, gpioButtonIsr1);

    /* Turn on CONFIG_GPIO_LED_0 and enable CONFIG_GPIO_BUTTON_0_INPUT interrupt */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
    GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0_INPUT);
    GPIO_enableInt(CONFIG_GPIO_BUTTON_1_INPUT);

    test_wdt_init();
}

void test_main_loop()
{

    // test_wdt_loop();

}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    test_main_init();
    while (1)
    {
        test_main_loop();
    }
}
