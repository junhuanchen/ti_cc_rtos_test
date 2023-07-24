/*
 * Copyright (c) 2017-2022, Texas Instruments Incorporated
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
 *  ======== fatsdraw.c ========
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if defined(__IAR_SYSTEMS_ICC__)
    /*
     *  Prevent time() macro in time.h from being used instead of our
     *  generated time() function.
     */
    #define _NO_DEFINITIONS_IN_HEADER_FILES 1
#endif

#include <time.h>

#if defined(__IAR_SYSTEMS_ICC__)
    #undef _NO_DEFINITIONS_IN_HEADER_FILES
#endif

#include <third_party/fatfs/ff.h>

// #include <ti/display/Display.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SDFatFS.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* Buffer size used for the file copy process */
#ifndef CPY_BUFF_SIZE
    #define CPY_BUFF_SIZE 2048
#endif

/* String conversion macro */
#define STR_(n) #n
#define STR(n)  STR_(n)

/* Drive number used for FatFs */
#define DRIVE_NUM 0

const char inputfile[]  = STR(DRIVE_NUM) ":input.txt";
const char outputfile[] = STR(DRIVE_NUM) ":output.txt";

const char textarray[]
    __attribute__((aligned(4))) = "***********************************************************************\r\n"
                                  "0         1         2         3         4         5         6         7\r\n"
                                  "01234567890123456789012345678901234567890123456789012345678901234567890\r\n"
                                  "This is some text to be inserted into the inputfile if there isn't\r\n"
                                  "already an existing file located on the media.\r\n"
                                  "If an inputfile already exists, or if the file was already once\r\n"
                                  "generated, then the inputfile will NOT be modified.\r\n"
                                  "***********************************************************************\r\n";

// static Display_Handle display;

unsigned char cpy_buff[CPY_BUFF_SIZE + 1] __attribute__((aligned(4)));

FIL src;
FIL dst;

#include "test_uart.h"

/*
 *  ======== printDrive ========
 *  Function to print drive information such as the total disk space
 *  This function was created by referencing FatFs's API documentation
 *  http://elm-chan.org/fsw/ff/en/getfree.html
 *
 *  This function call may take a while to process, depending on the size of
 *  SD Card used.
 */
void printDrive(const char *driveNumber, FATFS **fatfs)
{
    FRESULT fresult;
    DWORD freeClusterCount;
    DWORD totalSectorCount;
    DWORD freeSectorCount;

    test_uart_puts("Reading disk information...\r\n");

    fresult = f_getfree(driveNumber, &freeClusterCount, fatfs);
    if (fresult)
    {
        test_uart_puts("Error getting the free cluster count from the FatFs object\r\n");
        while (1) {}
    }
    else
    {
        test_uart_puts("done\r\n");

        /* Get total sectors and free sectors */
        totalSectorCount = ((*fatfs)->n_fatent - 2) * (*fatfs)->csize;
        freeSectorCount  = freeClusterCount * (*fatfs)->csize;

        char tmpStr[64] = { 0 };
        sprintf(tmpStr, "Total Disk size: %10lu KiB\r\nFree Disk space: %10lu KiB\r\n",
                totalSectorCount / 2,
                freeSectorCount / 2);
        test_uart_puts(tmpStr);
    }
}

#include "test_uart.h"

void test_fatsd_init()
{
    FRESULT fresult;

    /* Variables to keep track of the file copy progress */
    unsigned int bytesRead    = 0;
    unsigned int bytesWritten = 0;
    unsigned int filesize;
    unsigned int totalBytesCopied = 0;

    SDFatFS_Handle sdfatfsHandle;

    SDFatFS_init();

    /* Mount and register the SD Card */
    sdfatfsHandle = SDFatFS_open(CONFIG_SD_0, DRIVE_NUM);
    if (sdfatfsHandle == NULL)
    {
        test_uart_puts("Error starting the SD card\r\n");
        while (1) {}
    }
    else
    {
        test_uart_puts("Drive 0 is mounted\r\n");
    }

    printDrive(STR(DRIVE_NUM), &(dst.obj.fs));

    /* Try to open the source file */
    fresult = f_open(&src, inputfile, FA_READ);
    if (fresult != FR_OK)
    {
        test_uart_puts("Creating a new file\r\n");

        /* Open file for both reading and writing */
        fresult = f_open(&src, inputfile, FA_CREATE_NEW | FA_READ | FA_WRITE);
        if (fresult != FR_OK)
        {
            while (1) {}
        }

        f_write(&src, textarray, strlen(textarray), &bytesWritten);
        f_sync(&src);

        /* Reset the internal file pointer */
        f_lseek(&src, 0);
        test_uart_puts("done\r\n");
    }
    else
    {
        test_uart_puts("Using existing\r\n");
    }

    /* Create a new file object for the file copy */
    fresult = f_open(&dst, outputfile, FA_CREATE_ALWAYS | FA_WRITE);
    if (fresult != FR_OK)
    {
        test_uart_puts("Error opening...\r\n");
        while (1) {}
    }
    else
    {
        test_uart_puts("Starting file copy\r\n");
    }

    /*  Copy the contents from the src to the dst */
    while (true)
    {
        /*  Read from source file */
        fresult = f_read(&src, cpy_buff, CPY_BUFF_SIZE, &bytesRead);
        if (fresult || bytesRead == 0)
        {
            break; /* Error or EOF */
        }

        /*  Write to dst file */
        fresult = f_write(&dst, cpy_buff, bytesRead, &bytesWritten);
        if (fresult || bytesWritten < bytesRead)
        {
            test_uart_puts("Disk Full\r\n");
            break; /* Error or Disk Full */
        }

        /*  Update the total number of bytes copied */
        totalBytesCopied += bytesWritten;
    }

    f_sync(&dst);

    /* Get the filesize of the source file */
    filesize = f_size(&src);

    /* Close both inputfile[] and outputfile[] */
    f_close(&src);
    f_close(&dst);

    // Display_printf(display,
    //                0,
    //                0,
    //                "File \"%s\" (%u B) copied to \"%s\" (Wrote %u B)\r\n",
    //                inputfile,
    //                filesize,
    //                outputfile,
    //                totalBytesCopied);

    /* Now output the outputfile[] contents onto the console */
    fresult = f_open(&dst, outputfile, FA_READ);
    if (fresult != FR_OK)
    {
        test_uart_puts("Error opening...\r\n");
        while (1) {}
    }

    /* Print file contents */
    while (true)
    {
        /* Read from output file */
        fresult = f_read(&dst, cpy_buff, CPY_BUFF_SIZE, &bytesRead);
        if (fresult || bytesRead == 0)
        {
            break; /* Error or EOF */
        }
        cpy_buff[bytesRead] = '\0';
        /* Write output */
        test_uart_puts(cpy_buff);
    }

    /* Close the file */
    f_close(&dst);

    printDrive(STR(DRIVE_NUM), &(dst.obj.fs));

    /* Stopping the SDCard */
    SDFatFS_close(sdfatfsHandle);

    test_uart_puts("Drive 0 unmounted\r\n");

}

void test_fatsd_loop()
{
    
}

/*
 *  ======== fatfs_getFatTime ========
 */
int32_t fatfs_getFatTime(void)
{
    /*
     *  FatFs uses this API to get the current time in FatTime format.  User's
     *  must implement this function based on their system's timekeeping
     *  mechanism.  See FatFs documentation for details on FatTime format.
     */
    /* Jan 1 2017 00:00:00 */
    return (0x4A210000);
}
