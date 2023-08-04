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

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
   test_uart_init();
    while (1)
    {
       test_uart_loop();
    }
}

#endif /* APPLICATION_APPS_H_ */
