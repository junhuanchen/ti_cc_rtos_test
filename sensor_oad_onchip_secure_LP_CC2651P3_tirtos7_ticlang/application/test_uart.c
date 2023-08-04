
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
char tempStr[256] = "\r\nhello world\r\n";
UART2_Handle uart_0;
UART2_Handle uart_1;
UART2_Params uartParams_0;
UART2_Params uartParams_1;
size_t bytesRead;
size_t bytesWritten = 0;

void test_uart_puts(char *str)
{
    bytesWritten = 0;
    while (bytesWritten == 0)
    {
        UART2_write(uart_1, str, strlen(str), &bytesWritten);
    }
}

//void test_uart_printf(const char *format, ...)
//{
//    char buf[256];
//    va_list args;
//    va_start(args, format);
//    vsprintf(buf, format, args);
//    va_end(args);
//
//    test_uart_puts(buf, strlen(buf));
//}

void test_uart_init(void)
{

    UART2_Params_init(&uartParams_1);
    uartParams_1.baudRate = 115200;

    uart_1 = UART2_open(CONFIG_DISPLAY_UART, &uartParams_1);

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
            GPIO_toggle(CONFIG_GPIO_GLED);
        }
    }

}
