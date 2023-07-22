/*
 * Copyright (c) 2020, Texas Instruments Incorporated
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
 *  ======== uart2echo.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART2.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/*
 * The following function is from good old K & R.
 */
static void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c    = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/*
 * The following function is from good old K & R.
 */
static void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0) /* record sign */
    {
        n = -n; /* make n positive */
    }
    i = 0;
    do
    {                          /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0);   /* delete it */
    if (sign < 0)
    {
        s[i++] = '-';
    }
    s[i] = '\0';
    reverse(s);
}

char input;
char tempStr[64] = "\r\nhello world\r\n";
UART2_Handle uart_0;
UART2_Handle uart_1;
UART2_Params uartParams_0;
UART2_Params uartParams_1;
size_t bytesRead;
size_t bytesWritten = 0;

void test_uart_init(void)
{
    /* Create a UART where the default read and write mode is BLOCKING */
    UART2_Params_init(&uartParams_0);
    uartParams_0.baudRate = 9600;

    uart_0 = UART2_open(CONFIG_UART2_0, &uartParams_0);

    if (uart_0 == NULL)
    {
        /* UART2_open() failed */
        while (1) {}
    }

    UART2_Params_init(&uartParams_1);
    uartParams_1.baudRate = 115200;

    uart_1 = UART2_open(CONFIG_UART2_1, &uartParams_1);

    if (uart_1 == NULL)
    {
        /* UART2_open() failed */
        while (1) {}
    }

    UART2_write(uart_1, tempStr, strlen(tempStr), &bytesWritten);

}

void test_uart_loop(void)
{
    int status           = UART2_STATUS_SUCCESS;

    bytesRead = 0;
    status = UART2_readTimeout(uart_1, &input, 1, &bytesRead, 1000); // 1ms
    if (status == UART2_STATUS_SUCCESS)
    {
        bytesWritten = 0;
        sprintf(tempStr, "\r\ninput:%d\r\n", (int)input);
        UART2_write(uart_1, tempStr, strlen(tempStr), &bytesWritten);

        bytesWritten = 0;
        while (bytesWritten == 0)
        {
            status = UART2_write(uart_1, &input, 1, &bytesWritten);
        }

        // 发送给其他串口做接收测试
        bytesWritten = 0;
        while (bytesWritten == 0)
        {
            status = UART2_write(uart_0, &input, 1, &bytesWritten);
        }
    }

    bytesRead = 0;
    status = UART2_readTimeout(uart_0, &input, 1, &bytesRead, 1000); // 1ms
    if (status == UART2_STATUS_SUCCESS)
    {
        bytesWritten = 0;
        sprintf(tempStr, "\r\noutput:%d\r\n", (int)input);
        UART2_write(uart_1, tempStr, strlen(tempStr), &bytesWritten);

        GPIO_toggle(CONFIG_GPIO_LED_1);

    }
    
}
