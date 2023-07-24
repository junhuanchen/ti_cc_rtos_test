/*
 * Copyright (c) 2016-2022, Texas Instruments Incorporated
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
 *  ======== adcsinglechannel.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* POSIX Header files */
#include <pthread.h>

/* Driver Header files */
#include <ti/drivers/ADC.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* ADC sample count */
#define ADC_SAMPLE_COUNT (10)

#define THREADSTACKSIZE (768)

/* Inputs to the ADC on the CC32XX launchpads are downscaled by a factor of 0.42
 * Multiplying the conversion output with 2.365 will provide compensated output.
 */
#if DeviceFamily_PARENT == DeviceFamily_PARENT_CC32XX
    #define COMPENSATION_FACTOR 2365
#else
    #define COMPENSATION_FACTOR 1000
#endif

/* ADC conversion result variables */
uint16_t adcValue0;
uint32_t adcValue0MicroVolt;
uint16_t adcValue1[ADC_SAMPLE_COUNT];
uint32_t adcValue1MicroVolt[ADC_SAMPLE_COUNT];

void test_adc_init(void)
{
    pthread_t thread0, thread1;
    pthread_attr_t attrs;
    struct sched_param priParam;
    int retc;
    int detachState;

    /* Call driver init functions */
    ADC_init();
    test_uart_puts("Starting the ADC Single Channel example\n");

    ADC_Handle adc;
    ADC_Params params;
    int_fast16_t res;

    ADC_Params_init(&params);
    adc = ADC_open(CONFIG_ADC_0, &params);

    if (adc == NULL)
    {
        test_uart_puts("Error initializing CONFIG_ADC_0\n");
        while (1) {}
    }

    /* Blocking mode conversion */
    res = ADC_convert(adc, &adcValue0);

    if (res == ADC_STATUS_SUCCESS)
    {
        // Convert raw adcValue0 to compensated microvolts
        adcValue0MicroVolt = (COMPENSATION_FACTOR * ADC_convertRawToMicroVolts(adc, adcValue0)) / 1000;
        
        {
            char *tmpStr[64] = { 0 };
            sprintf(tmpStr, "CONFIG_ADC_0 raw result: %d\n", adcValue0);
            test_uart_puts(tmpStr);
        }
        
        {
            char *tmpStr[64] = { 0 };
            sprintf(tmpStr, "CONFIG_ADC_0 convert result: %d uV\n", adcValue0MicroVolt);
            test_uart_puts(tmpStr);
        }
    }
    else
    {
        test_uart_puts("CONFIG_ADC_0 convert failed\n");
    }

    ADC_close(adc);
    test_uart_puts("CONFIG_ADC_0 closed\n");

}
