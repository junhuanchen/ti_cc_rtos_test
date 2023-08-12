/*
 * apps.h
 *
 *  Created on: 2023年8月4日
 *      Author: juwan
 */

#ifndef APPLICATION_APPS_H_
#define APPLICATION_APPS_H_


#include <stdint.h>

/* POSIX Header files */
#include <pthread.h>

/* RTOS header files */
#include <ti/sysbios/BIOS.h>

#include <ti/drivers/Board.h>

extern void *mainThread(void *arg0);

/* Stack size in bytes */
#define THREADSTACKSIZE 1024

/*
 *  ======== main ========
 */
int apps(void)
{
    pthread_t thread;
    pthread_attr_t attrs;
    struct sched_param priParam;
    int retc;

    /* Initialize the attributes structure with default values */
    pthread_attr_init(&attrs);

    /* Set priority, detach state, and stack size attributes */
    priParam.sched_priority = 1;
    retc                    = pthread_attr_setschedparam(&attrs, &priParam);
    retc |= pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
    if (retc != 0)
    {
        /* failed to set attributes */
        while (1) {}
    }

    retc = pthread_create(&thread, &attrs, mainThread, NULL);
    if (retc != 0)
    {
        /* pthread_create() failed */
        while (1) {}
    }

    return (0);
}

#include "test_uart.h"
#include "simple_peripheral_oad_onchip.h"

#include <ti/drivers/NVS.h>

/* Buffer placed in RAM to hold bytes read from non-volatile storage. */
static char buffer[64];

/*
 * Some devices have a minimum FLASH write size of 4-bytes (1 word). Trying
 * to write a non-multiple of 4 amount of data will fail. This array is
 * rounded up (to next multiple of 4) to meet this requirement. Refer to NVS
 * driver documentation for more details.
 */
static const char signature[52] = {"SimpleLink SDK Non-Volatile Storage (NVS) Example."};

/*
 *  ======== mainThread ========
 */
void test_nvs_init()
{
    NVS_Handle nvsHandle;
    NVS_Attrs regionAttrs;
    NVS_Params nvsParams;

    NVS_init();

    NVS_Params_init(&nvsParams);
    nvsHandle = NVS_open(CONFIG_NVS_0, &nvsParams);

    if (nvsHandle == NULL)
    {
        test_uart_puts("NVS_open() failed.");

        return ;
    }

    test_uart_puts("\n");

    /*
     * This will populate a NVS_Attrs structure with properties specific
     * to a NVS_Handle such as region base address, region size,
     * and sector size.
     */
    NVS_getAttrs(nvsHandle, &regionAttrs);

    /* Display the NVS region attributes */
    // char tmp[64] = {0};
    // sprintf(tmp, "Region Base Address: 0x%x\n", regionAttrs.regionBase);
    // test_uart_puts(tmp);
    // sprintf(tmp, "Sector Size: 0x%x\n", regionAttrs.sectorSize);
    // test_uart_puts(tmp);
    // sprintf(tmp, "Region Size: 0x%x\n", regionAttrs.regionSize);
    // test_uart_puts(tmp);
    

    /*
     * Copy "sizeof(signature)" bytes from the NVS region base address into
     * buffer. An offset of 0 specifies the offset from region base address.
     * Therefore, the bytes are copied from regionAttrs.regionBase.
     */
    NVS_read(nvsHandle, 0, (void *)buffer, sizeof(signature));

    /*
     * Determine if the NVS region contains the signature string.
     * Compare the string with the contents copied into buffer.
     */
    if (strcmp((char *)buffer, (char *)signature) == 0)
    {

        /* Write signature directly from the NVS region to the UART console. */
        // test_uart_puts("%s", regionAttrs.regionBase);
        char tmp[64] = {0};
        sprintf(tmp, "%s", regionAttrs.regionBase);
        test_uart_puts(tmp);

        /* Erase the entire flash sector. */
        NVS_erase(nvsHandle, 0, regionAttrs.sectorSize);
    }
    else
    {

        /* The signature was not found in the NVS region. */
        test_uart_puts("Writing signature to flash...\n");

        /* Write signature to memory. The flash sector is erased prior
         * to performing the write operation. This is specified by
         * NVS_WRITE_ERASE.
         */
        NVS_write(nvsHandle, 0, (void *)signature, sizeof(signature), NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
    }

    NVS_close(nvsHandle);

}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    test_uart_init();
    // test_nvs_init();
    while (1)
    {
       test_uart_loop();
    }
}

#endif /* APPLICATION_APPS_H_ */
