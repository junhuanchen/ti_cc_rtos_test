
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART2.h>

/* Driver configuration */
#include "ti_drivers_config.h"

#include "simple_peripheral_oad_onchip.h"
/*
 * The following function is from good old K & R.
 */
// static void reverse(char s[])
// {
//     int i, j;
//     char c;

//     for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
//     {
//         c    = s[i];
//         s[i] = s[j];
//         s[j] = c;
//     }
// }

/*
 * The following function is from good old K & R.
 */
// static void itoa(int n, char s[])
// {
//     int i, sign;

//     if ((sign = n) < 0) /* record sign */
//     {
//         n = -n; /* make n positive */
//     }
//     i = 0;
//     do
//     {                          /* generate digits in reverse order */
//         s[i++] = n % 10 + '0'; /* get next digit */
//     } while ((n /= 10) > 0);   /* delete it */
//     if (sign < 0)
//     {
//         s[i++] = '-';
//     }
//     s[i] = '\0';
//     reverse(s);
// }

char input;
char tempStr[64] = "\r\nhello world\r\n";
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
    // "\r\n"
    bytesWritten = 0;
    while (bytesWritten == 0)
    {
        UART2_write(uart_1, "\r\n", 2, &bytesWritten);
    }
}

void test_uart_print(char *str, size_t len)
{
    bytesWritten = 0;
    while (bytesWritten == 0)
    {
        UART2_write(uart_1, str, len, &bytesWritten);
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
//    test_uart_print(buf, strlen(buf));
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

void test_uart_loop()
{

    extern mrConnRec_t connList[MAX_NUM_BLE_CONNS];
    extern ICall_EntityID selfEntity;
    extern uint16_t mrConnHandle;

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
        
        if (input == '0')
        {
            bytesWritten = 0;
            while (bytesWritten == 0)
            {
                status = UART2_write(uart_1, "hello world\r\n", 13, &bytesWritten);
                GPIO_toggle(CONFIG_GPIO_GLED);
            }
        }

        extern uint8_t charVal;
        
        if (input == '0')
        {
            // view charVal
            bytesWritten = 0;
            while (bytesWritten == 0)
            {
                sprintf(tempStr, "\r\ncharVal:%d\r\n", (int)charVal);
                status = UART2_write(uart_1, tempStr, strlen(tempStr), &bytesWritten);
                GPIO_toggle(CONFIG_GPIO_GLED);
            }

            char multi_role_tmpbuf[20] = { 0 };
            #define SIMPLEPROFILE_CHAR6                   5  // RW uint8 - Profile Characteristic 4 value
            SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR6, multi_role_tmpbuf);
            // view multi_role_tmpbuf
            sprintf(tempStr, "\r\nmulti_role_tmpbuf:%.*s\r\n", 20, multi_role_tmpbuf);
            test_uart_puts(tempStr);

            // Loop through connection
            for (int i = 0; i < MAX_NUM_BLE_CONNS; i++)
            {
                test_uart_puts("connHandle:");
                sprintf(tempStr, "%d", (int)connList[i].connHandle);
                test_uart_puts(tempStr);
            }
        }

        if (input == '8')
        {
            uint8_t connIndex = multi_role_getConnIndex(mrConnHandle);
            // connList[connIndex].discState == BLE_DISC_STATE_CHAR
            test_uart_puts("connHandle:");
            sprintf(tempStr, "%d", (int)connList[connIndex].connHandle);
            test_uart_puts(tempStr);
            // discExist == 1
            test_uart_puts("discExist:");
            sprintf(tempStr, "%d", (int)connList[connIndex].discExist);
            test_uart_puts(tempStr);
            return;
        }

        if (input == '7')
        {
            uint8_t connIndex = multi_role_getConnIndex(0);
            mrConnHandle = connList[connIndex].connHandle;

            if (connList[connIndex].charHandle == 0)
            {
                #define MR_EVT_SVC_DISC            6
                
                // Initiate service discovery
                multi_role_enqueueMsg(MR_EVT_SVC_DISC, NULL);
            }

            test_uart_puts("connHandle:");
            sprintf(tempStr, "%d", (int)connList[connIndex].connHandle);
            test_uart_puts(tempStr);
            return;
        }

        if (input == '6')
        {
            extern int test_ble_flag;
            test_ble_flag = rand() % 3 + 2;
            return;
        }

        if (input == '5')
        {
            extern int test_ble_flag;
            test_ble_flag = 1;
            return;
        }

        // if (input == '4')
        // {
        //     GapScan_enable(0, 100, 15);
        //     test_uart_puts("Discovering...\r\n");
        //     return;
        // }

        // if (input == '3')
        // {
        //     GapScan_disable();
        //     test_uart_puts("Stopped Discovering\r\n");
        //     return;
        // }

        // if (input == '2')
        // {
        //     GapScan_Evt_AdvRpt_t advRpt;

        //     GapScan_getAdvReport(0, &advRpt);

        //     static tmp[64] = {0};
        //     sprintf(tmp, "mac: %02x:%02x:%02x:%02x:%02x:%02x\r\n", advRpt.addr[0], advRpt.addr[1], advRpt.addr[2], advRpt.addr[3], advRpt.addr[4], advRpt.addr[5]);
        //     test_uart_puts(tmp);

        //     GapInit_connect(advRpt.addrType & 0x01, advRpt.addr, 0x01, 0);

        //     // multi_role_doConnUpdate(0);
        //     // test_uart_puts("Connection Update Request\r\n");
        //     return;
        // }

        if (input == '1')
        {
            // Temporarily disable advertising
            extern uint8_t advHandle;
            GapAdv_disable(advHandle);

            // Scanned device information record
            typedef struct
            {
            uint8_t addrType; // Peer Device's Address Type
            uint8_t addr[6];  // Peer Device Address
            } scanRec_t;

            scanRec_t advRpt; // 09:df:00:71:77:60 addrType 0
            advRpt.addrType = 0;
            advRpt.addr[0] = 0x09;
            advRpt.addr[1] = 0xdf;
            advRpt.addr[2] = 0x00;
            advRpt.addr[3] = 0x71;
            advRpt.addr[4] = 0x77;
            advRpt.addr[5] = 0x60;

            GapInit_connect(advRpt.addrType & 0x01, advRpt.addr, 0x01, 0);

            char tmp[64];
            sprintf(tmp, "Connecting to %02x:%02x:%02x:%02x:%02x:%02x\r\n", advRpt.addr[0], advRpt.addr[1], advRpt.addr[2], advRpt.addr[3], advRpt.addr[4], advRpt.addr[5]);
            test_uart_puts(tmp);

            // Re-enable advertising
            GapAdv_enable(advHandle, 0, 0);

            return;
        }

    }

}
