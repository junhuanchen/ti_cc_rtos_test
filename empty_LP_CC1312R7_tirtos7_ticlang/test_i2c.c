/*
 * Copyright (c) 2018-2023, Texas Instruments Incorporated
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
 *  ======== i2ctmp.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

/* Driver configuration */
#include "ti_drivers_config.h"

#define TASKSTACKSIZE 640

/* Temperature result registers */
#define TMP11X_RESULT_REG 0x0000

/* I2C target addresses */
#define TMP11X_BASSENSORS_ADDR 0x48
#define TMP116_LAUNCHPAD_ADDR  0x49

/* Number of supported sensor iterations */
#define TMP_COUNT 2

/*
 * Data structure containing currently supported I2C TMP sensors.
 * Sensors are ordered by descending preference.
 */
static const struct
{
    uint8_t address;
    uint8_t resultReg;
    char *id;
} sensors[TMP_COUNT] = {{TMP11X_BASSENSORS_ADDR, TMP11X_RESULT_REG, "11X"},
                        {TMP116_LAUNCHPAD_ADDR, TMP11X_RESULT_REG, "116"}};

static uint8_t targetAddress;

static void i2cErrorHandler(I2C_Transaction *transaction);

#include "test_uart.h"

/*
 *  ======== mainThread ========
 */
void test_i2c_init(void)
{
    uint16_t sample;
    int16_t temperature;
    uint8_t txBuffer[1];
    uint8_t rxBuffer[2];
    int8_t i;
    I2C_Handle i2c;
    I2C_Params i2cParams;
    I2C_Transaction i2cTransaction;

    I2C_init();

    test_uart_puts("Starting the i2ctmp example\n");

    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c               = I2C_open(CONFIG_I2C_0, &i2cParams);
    if (i2c == NULL)
    {
        test_uart_puts("Error Initializing I2C\n");
        while (1) {}
    }
    else
    {
        test_uart_puts("I2C Initialized!\n");
    }

    /* Common I2C transaction setup */
    i2cTransaction.writeBuf   = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf    = rxBuffer;
    i2cTransaction.readCount  = 0;

    /*
     * Determine which I2C sensors are present by querying known I2C
     * target addresses.
     */
    for (i = TMP_COUNT - 1; i >= 0; i--)
    {
        i2cTransaction.targetAddress = sensors[i].address;
        txBuffer[0]                  = sensors[i].resultReg;

        if (I2C_transfer(i2c, &i2cTransaction))
        {
            targetAddress = sensors[i].address;
            // Display_printf(display,
                        //    0,
                        //    0,
                        //    "Detected TMP%s sensor with target"
                        //    " address 0x%x",
                        //    sensors[i].id,
                        //    sensors[i].address);
        }
        else
        {
            i2cErrorHandler(&i2cTransaction);
        }
    }

    /* If we never assigned a target address */
    if (targetAddress == 0)
    {
        test_uart_puts("Failed to detect a sensor!\n");
        I2C_close(i2c);
        while (1) {}
    }

    test_uart_puts("\nUsing last known sensor for samples.\n");
    i2cTransaction.targetAddress = targetAddress;

    /* Take 20 samples and print them out onto the console */
    i2cTransaction.readCount = 2;
    for (sample = 0; sample < 20; sample++)
    {
        if (I2C_transfer(i2c, &i2cTransaction))
        {
            /*
             * Extract degrees C from the received data;
             * see TMP sensor datasheet
             */
            temperature = (rxBuffer[0] << 8) | (rxBuffer[1]);
            temperature *= 0.0078125;

            // test_uart_puts("Sample %u: %d C", sample, temperature);
        }
        else
        {
            i2cErrorHandler(&i2cTransaction);
            /* Sleep for 1 second */
            sleep(1);
        }
    }

    I2C_close(i2c);

    test_uart_puts("I2C closed!\n");

}

/*
 *  ======== i2cErrorHandler ========
 */
static void i2cErrorHandler(I2C_Transaction *transaction)
{
    switch (transaction->status)
    {
        case I2C_STATUS_TIMEOUT:
            // test_uart_puts("I2C transaction timed out!");
            break;
        case I2C_STATUS_CLOCK_TIMEOUT:
            // test_uart_puts("I2C serial clock line timed out!");
            break;
        case I2C_STATUS_ADDR_NACK:
            // Display_printf(display, "I2C target address 0x%x not"
                        //    " acknowledged!",
                        //    transaction->targetAddress);
            break;
        case I2C_STATUS_DATA_NACK:
            // test_uart_puts("I2C data byte not acknowledged!");
            break;
        case I2C_STATUS_ARB_LOST:
            // test_uart_puts("I2C arbitration to another controller!");
            break;
        case I2C_STATUS_INCOMPLETE:
            // test_uart_puts("I2C transaction returned before completion!");
            break;
        case I2C_STATUS_BUS_BUSY:
            // test_uart_puts("I2C bus is already in use!");
            break;
        case I2C_STATUS_CANCEL:
            // test_uart_puts("I2C transaction cancelled!");
            break;
        case I2C_STATUS_INVALID_TRANS:
            // test_uart_puts("I2C transaction invalid!");
            break;
        case I2C_STATUS_ERROR:
            // test_uart_puts("I2C generic error!");
            break;
        default:
            // test_uart_puts("I2C undefined error case!");
            break;
    }
}
