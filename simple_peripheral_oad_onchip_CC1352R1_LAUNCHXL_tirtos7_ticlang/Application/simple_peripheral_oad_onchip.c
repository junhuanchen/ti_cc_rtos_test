/******************************************************************************

@file  multi_role.c

@brief This file contains the multi_role sample application for use
with the CC2650 Bluetooth Low Energy Protocol Stack.

Group: WCS, BTS
Target Device: cc13xx_cc26xx

******************************************************************************

 Copyright (c) 2013-2023, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************


*****************************************************************************/

/*********************************************************************
* INCLUDES
*/
#ifdef FREERTOS
#include <FreeRTOS.h>
#include <task.h>
//#include <pthread.h>
#include <mqueue.h>
#else
#include <string.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Task.h>
#endif

#ifdef FREERTOS
#include <stdarg.h>
#endif

#include <ti/display/Display.h>

#if (!(defined FREERTOS) && !(defined __TI_COMPILER_VERSION__)) && !(defined(__clang__))
#include <intrinsics.h>
#endif

#include <ti/drivers/GPIO.h>
#include <ti/drivers/utils/List.h>

#include <icall.h>
#include "util.h"
#include "onboard.h"
#include <bcomdef.h>
/* This Header file contains all BLE API and icall structure definition */
#include <icall_ble_api.h>

#include <devinfoservice.h>
#include <simple_gatt_profile.h>

// Used for OAD Reset Service APIs
#include "oad_reset_service.h"

#include <ti_drivers_config.h>
#include <board_key.h>

#include <menu/two_btn_menu.h>

#include "ti_ble_config.h"
#include "simple_peripheral_oad_onchip_menu.h"
#include "simple_peripheral_oad_onchip.h"

// Used for imgHdr_t structure
#include <common/cc26xx/oad/oad_image_header.h>
#include "oad.h"
#include <common/cc26xx/flash_interface/flash_interface.h>

#include "apps.h"

/*********************************************************************
* CONSTANTS
*/
// How often to perform periodic event (in msec)
#define MR_PERIODIC_EVT_PERIOD               5000

// Offset into the scanRspData string the software version info is stored
#define OAD_SOFT_VER_OFFSET                   15

// Task configuration
#define MR_TASK_PRIORITY                     1
#ifndef MR_TASK_STACK_SIZE
#ifdef FREERTOS
#define MR_TASK_STACK_SIZE                   2048
#else
#define MR_TASK_STACK_SIZE                   1024
#endif

#endif

// Application events
#define MR_EVT_CHAR_CHANGE         1
#define MR_EVT_KEY_CHANGE          2
#define MR_EVT_ADV_REPORT          3
#define MR_EVT_SCAN_ENABLED        4
#define MR_EVT_SCAN_DISABLED       5
#define MR_EVT_SVC_DISC            6
#define MR_EVT_ADV                 7
#define MR_EVT_PAIRING_STATE       8
#define MR_EVT_PASSCODE_NEEDED     9
#define MR_EVT_SEND_PARAM_UPDATE   10
#define MR_EVT_PERIODIC            11
#define MR_EVT_READ_RPA            12
#define MR_EVT_INSUFFICIENT_MEM    13
#define MR_CONN_EVT                14
#define MR_OAD_RESET_EVT           15


#define MR_OAD_QUEUE_EVT                     OAD_QUEUE_EVT       // Event_Id_01
#define MR_OAD_COMPLETE_EVT                  OAD_DL_COMPLETE_EVT // Event_Id_02
#define MR_OAD_NO_MEM_EVT                    OAD_OUT_OF_MEM_EVT  // Event_Id_03

// Internal Events for RTOS application
#define MR_ICALL_EVT                         ICALL_MSG_EVENT_ID // Event_Id_31
#define MR_QUEUE_EVT                         UTIL_QUEUE_EVENT_ID // Event_Id_30

#define MR_ALL_EVENTS                        (MR_ICALL_EVT             | \
                                              MR_QUEUE_EVT             | \
                                              MR_OAD_QUEUE_EVT         | \
                                              MR_OAD_COMPLETE_EVT      | \
                                              MR_OAD_NO_MEM_EVT)

// address string length is an ascii character for each digit +
#define MR_ADDR_STR_SIZE     15

// Supervision timeout conversion rate to miliseconds
#define CONN_TIMEOUT_MS_CONVERSION            10

// Spin if the expression is not true
#define MULTIROLE_ASSERT(expr) if (!(expr)) multi_role_spin();

/*********************************************************************
* GLOBAL VARIABLES
*/
#ifdef FREERTOS
mqd_t g_EventsQueueID;
#endif
// Display Interface
Display_Handle dispHandle = NULL;

/*********************************************************************
* LOCAL VARIABLES
*/

#define APP_EVT_EVENT_MAX  0x13
char *appEventStrings[] = {
  "APP_ZERO             ",
  "APP_CHAR_CHANGE      ",
  "APP_KEY_CHANGE       ",
  "APP_ADV_REPORT       ",
  "APP_SCAN_ENABLED     ",
  "APP_SCAN_DISABLED    ",
  "APP_SVC_DISC         ",
  "APP_ADV              ",
  "APP_PAIRING_STATE    ",
  "APP_SEND_PARAM_UPDATE",
  "APP_PERIODIC         ",
  "APP_READ_RPA         ",
  "APP_INSUFFICIENT_MEM ",
};

//reset connection handle
uint16_t resetConnHandle = LINKDB_CONNHANDLE_INVALID;
/*********************************************************************
* LOCAL VARIABLES
*/

// Entity ID globally used to check for source and/or destination of messages
ICall_EntityID selfEntity;

// Event globally used to post local events and pend on system and
// local events.
#ifdef FREERTOS
ICall_SyncHandle syncEvent;
#else
static ICall_SyncHandle syncEvent;
#endif
// Clock instances for internal periodic events.
static Clock_Struct clkPeriodic;
// Clock instance for RPA read events.
static Clock_Struct clkRpaRead;

// Memory to pass periodic event to clock handler
mrClockEventData_t periodicUpdateData =
{
  .event = MR_EVT_PERIODIC
};

// Memory to pass RPA read event ID to clock handler
mrClockEventData_t argRpaRead =
{
  .event = MR_EVT_READ_RPA
};
#ifdef FREERTOS
/*Non blocking queue */
 mqd_t g_POSIX_appMsgQueue;

#else
// Queue object used for app messages
static Queue_Struct appMsg;
static Queue_Handle appMsgQueue;
#endif

// Task configuration
#ifdef FREERTOS
typedef uint32_t * Task_Handle;
TaskHandle_t mrTask = NULL;
#else
Task_Struct mrTask;
#endif

#ifndef FREERTOS
#if defined __TI_COMPILER_VERSION__
#pragma DATA_ALIGN(mrTaskStack, 8)
#else
#pragma data_alignment=8
#endif
#ifndef FREERTOS
uint8_t mrTaskStack[MR_TASK_STACK_SIZE];
#endif
#endif

// Maximim PDU size (default = 27 octets)
static uint16 mrMaxPduSize;

#if (DEFAULT_DEV_DISC_BY_SVC_UUID == TRUE)
// Number of scan results filtered by Service UUID
static uint8_t numScanRes = 0;

// Scan results filtered by Service UUID
static scanRec_t scanList[DEFAULT_MAX_SCAN_RES];
#endif // DEFAULT_DEV_DISC_BY_SVC_UUID

// Discovered service start and end handle
static uint16_t svcStartHdl = 0;
static uint16_t svcEndHdl = 0;

// Value to write
uint8_t charVal = 0;

// Number of connected devices
uint8_t numConn = 0;

// Connection handle of current connection
uint16_t mrConnHandle = LINKDB_CONNHANDLE_INVALID;

// List to store connection handles for set phy command status's
static List_List setPhyCommStatList;

// List to store connection handles for queued param updates
static List_List paramUpdateList;

// Per-handle connection info
mrConnRec_t connList[MAX_NUM_BLE_CONNS];

// Variable used to store the number of messages pending once OAD completes
// The application cannot reboot until all pending messages are sent
static uint8_t numPendingMsgs = 0;
static bool oadWaitReboot = false;

// Flag to be stored in NV that tracks whether service changed
// indications needs to be sent out
static uint32_t  sendSvcChngdOnNextBoot = FALSE;

// Advertising handles
uint8 advHandle;

static bool mrIsAdvertising = false;
// Address mode
static GAP_Addr_Modes_t addrMode = DEFAULT_ADDRESS_MODE;

// Current Random Private Address
static uint8 rpa[B_ADDR_LEN] = {0};

// Initiating PHY
static uint8_t mrInitPhy = INIT_PHY_1M;

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void multi_role_init(void);
static void multi_role_scanInit(void);
static void multi_role_advertInit(void);

#ifdef FREERTOS
static void multi_role_taskFxn(void *a0);
#else//TI_RTOS
static void multi_role_taskFxn(UArg a0, UArg a1);
#endif

static uint8_t multi_role_processStackMsg(ICall_Hdr *pMsg);
static uint8_t multi_role_processGATTMsg(gattMsgEvent_t *pMsg);
static void multi_role_processAppMsg(mrEvt_t *pMsg);
static void multi_role_processCharValueChangeEvt(uint8_t paramID);
static void multi_role_processGATTDiscEvent(gattMsgEvent_t *pMsg);
static void multi_role_processPasscode(mrPasscodeData_t *pData);
static void multi_role_processPairState(mrPairStateData_t* pairingEvent);
static void multi_role_processGapMsg(gapEventHdr_t *pMsg);
static void multi_role_processParamUpdate(uint16_t connHandle);
static void multi_role_processAdvEvent(mrGapAdvEventData_t *pEventData);

static void multi_role_charValueChangeCB(uint8_t paramID);
status_t multi_role_enqueueMsg(uint8_t event, void *pData);
static void multi_role_handleKeys(uint8_t keys);
uint16_t multi_role_getConnIndex(uint16_t connHandle);
static void multi_role_keyChangeHandler(uint8_t keys);
static uint8_t multi_role_addConnInfo(uint16_t connHandle, uint8_t *pAddr,
                                      uint8_t role);
static void multi_role_performPeriodicTask(void);
#ifdef FREERTOS
static void multi_role_clockHandler(void * arg);
#else
static void multi_role_clockHandler(UArg arg);
#endif
static uint8_t multi_role_clearConnListEntry(uint16_t connHandle);
#if (DEFAULT_DEV_DISC_BY_SVC_UUID == TRUE)
static void multi_role_addScanInfo(uint8_t *pAddr, uint8_t addrType);
static bool multi_role_findSvcUuid(uint16_t uuid, uint8_t *pData,
                                      uint16_t dataLen);
#endif // DEFAULT_DEV_DISC_BY_SVC_UUID
static uint8_t multi_role_removeConnInfo(uint16_t connHandle);
static void multi_role_menuSwitchCb(tbmMenuObj_t* pMenuObjCurr,
                                       tbmMenuObj_t* pMenuObjNext);
static void multi_role_startSvcDiscovery(void);
#ifndef Display_DISABLE_ALL
static char* multi_role_getConnAddrStr(uint16_t connHandle);
#endif
static void multi_role_advCB(uint32_t event, void *pBuf, uintptr_t arg);
static void multi_role_scanCB(uint32_t evt, void* msg, uintptr_t arg);
static void multi_role_passcodeCB(uint8_t *deviceAddr, uint16_t connHandle,
                                  uint8_t uiInputs, uint8_t uiOutputs, uint32_t numComparison);
static void multi_role_pairStateCB(uint16_t connHandle, uint8_t state,
                                   uint8_t status);
static void multi_role_updateRPA(void);
static void multi_role_connEvtCB(Gap_ConnEventRpt_t *pReport);
static void multi_role_processConnEvt(Gap_ConnEventRpt_t *pReport);
void multi_role_processOadResetWriteCB(uint16_t connHandle, uint16_t bim_var);
static uint8_t multi_role_processL2CAPMsg(l2capSignalEvent_t *pMsg);
static void multi_role_processOadResetEvt(oadResetWrite_t *resetEvt);
static void multi_role_processCmdCompleteEvt(hciEvt_CmdComplete_t *pMsg);
static void multi_role_updatePHYStat(uint16_t eventCode, uint8_t *pMsg);

static status_t multi_role_setPhy(uint16_t connHandle, uint8_t allPhys,
                                        uint8_t txPhy, uint8_t rxPhy,
                                        uint16_t phyOpts);

/*********************************************************************
 * EXTERN FUNCTIONS
*/
extern void AssertHandler(uint8 assertCause, uint8 assertSubcause);

/*********************************************************************
* PROFILE CALLBACKS
*/

// GAP Bond Manager Callbacks
static gapBondCBs_t multi_role_BondMgrCBs =
{
  multi_role_passcodeCB, // Passcode callback
  multi_role_pairStateCB                  // Pairing state callback
};

// Simple GATT Profile Callbacks
static simpleProfileCBs_t multi_role_simpleProfileCBs =
{
  multi_role_charValueChangeCB // Characteristic value change callback
};

static oadResetWriteCB_t multi_role_oadResetCBs =
{
  multi_role_processOadResetWriteCB // Write Callback.
};

/*********************************************************************
* PUBLIC FUNCTIONS
*/

/*********************************************************************
 * @fn      multi_role_spin
 *
 * @brief   Spin forever
 *
 * @param   none
 */
static void multi_role_spin(void)
{
  volatile uint8_t x = 0;

  while(1)
  {
    x++;
  }
}

/*********************************************************************
* @fn      multi_role_createTask
*
* @brief   Task creation function for multi_role.
*
* @param   None.
*
* @return  None.
*/


void multi_role_createTask(void)
{

#ifdef FREERTOS
    BaseType_t xReturned;

    /* Create the task, storing the handle. */
    xReturned = xTaskCreate(
            multi_role_taskFxn,                     /* Function that implements the task. */
            "MULTI_ROLE_APP",                       /* Text name for the task. */
            MR_TASK_STACK_SIZE / sizeof(uint32_t),  /* Stack size in words, not bytes. */
            ( void * ) NULL,                        /* Parameter passed into the task. */
            MR_TASK_PRIORITY,                       /* Priority at which the task is created. */
            &mrTask );                              /* Used to pass out the created task's handle. */

    if(xReturned == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
    {
        /* Creation of FreeRTOS task failed */
        while(1);
    }
#else
    Task_Params taskParams;

      // Configure task
      Task_Params_init(&taskParams);
      taskParams.stack = mrTaskStack;
      taskParams.stackSize = MR_TASK_STACK_SIZE;
      taskParams.priority = MR_TASK_PRIORITY;
      Task_construct(&mrTask, multi_role_taskFxn, &taskParams, NULL);

#endif

}

/*********************************************************************
* @fn      multi_role_init
*
* @brief   Called during initialization and contains application
*          specific initialization (ie. hardware initialization/setup,
*          table initialization, power up notification, etc), and
*          profile initialization/setup.
*
* @param   None.
*
* @return  None.
*/
static void multi_role_init(void)
{
  BLE_LOG_INT_TIME(0, BLE_LOG_MODULE_APP, "APP : ---- init ", MR_TASK_PRIORITY);
  // Create the menu
  multi_role_build_menu();
  // ******************************************************************
  // N0 STACK API CALLS CAN OCCUR BEFORE THIS CALL TO ICall_registerApp
  // ******************************************************************
  // Register the current thread as an ICall dispatcher application
  // so that the application can send and receive messages.
  ICall_registerApp(&selfEntity, &syncEvent);

#ifdef LED_DEBUG
  /* Configure the LED pin */
  GPIO_setConfig(CONFIG_GPIO_GLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
  GPIO_setConfig(CONFIG_GPIO_RLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

  uint_least8_t activeLed;
  uint8_t blinkCnt = 16;

  if (blinkCnt < 12)
  {
    activeLed = CONFIG_GPIO_GLED;
  }
  else
  {
    activeLed = CONFIG_GPIO_RLED;
  }
  for(uint8_t numBlinks = 0; numBlinks < blinkCnt; ++numBlinks)
  {
    GPIO_toggle(activeLed);
    // Sleep for 100ms, sys-tick for BLE-Stack is 10us,
    // Task sleep is in # of ticks
    Task_sleep(10000);
  }
  GPIO_write(activeLed, 0);
#endif //LED_DEBUG

#ifdef USE_RCOSC
  RCOSC_enableCalibration();
#endif // USE_RCOSC

#ifdef FREERTOS
  Util_constructQueue(&g_POSIX_appMsgQueue);
#else
  // Create an RTOS queue for message from profile to be sent to app.
  appMsgQueue = Util_constructQueue(&appMsg);
#endif
  // Create one-shot clock for internal periodic events.
#ifdef FREERTOS
  Util_constructClock(&clkPeriodic,(void*) multi_role_clockHandler,
                        MR_PERIODIC_EVT_PERIOD, 0, false,
                        (void*)&periodicUpdateData);
#else

  Util_constructClock(&clkPeriodic, multi_role_clockHandler,
                      MR_PERIODIC_EVT_PERIOD, 0, false,
                      (UArg)&periodicUpdateData);
#endif

  uint8_t swVer[OAD_SW_VER_LEN];
  OAD_getSWVersion(swVer, OAD_SW_VER_LEN);

  // Set the Device Name characteristic in the GAP GATT Service
  // For more information, see the section in the User's Guide:
  // http://software-dl.ti.com/lprf/ble5stack-latest/
  GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, (void *)attDeviceName);

  // Configure GAP
  {
    uint16_t paramUpdateDecision = DEFAULT_PARAM_UPDATE_REQ_DECISION;

    // Pass all parameter update requests to the app for it to decide
    GAP_SetParamValue(GAP_PARAM_LINK_UPDATE_DECISION, paramUpdateDecision);
  }

  // Setup the GAP Bond Manager. For more information see the GAP Bond Manager
  // section in the User's Guide
  setBondManagerParameters();

  // Initialize GATT attributes
  GGS_AddService(GAP_SERVICE);                 // GAP
  GATTServApp_AddService(GATT_ALL_SERVICES);   // GATT attributes
  DevInfo_AddService();                        // Device Information Service
  SimpleProfile_AddService(GATT_ALL_SERVICES); // Simple GATT Profile

  Reset_addService((oadUsrAppCBs_t *)&multi_role_oadResetCBs);

  // Setup the SimpleProfile Characteristic Values
  // For more information, see the GATT and GATTServApp sections in the User's Guide:
  // http://software-dl.ti.com/lprf/ble5stack-latest/
  {
    uint8_t charValue1 = 1;
    uint8_t charValue2 = 2;
    uint8_t charValue3 = 3;
    uint8_t charValue4 = 4;
    uint8_t charValue5[SIMPLEPROFILE_CHAR5_LEN] = { 1, 2, 3, 4, 5 };

    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR1, sizeof(uint8_t),
                               &charValue1);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR2, sizeof(uint8_t),
                               &charValue2);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR3, sizeof(uint8_t),
                               &charValue3);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR4, sizeof(uint8_t),
                               &charValue4);
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR5, SIMPLEPROFILE_CHAR5_LEN,
                               charValue5);
  }

  // Register callback with SimpleGATTprofile
  SimpleProfile_RegisterAppCBs(&multi_role_simpleProfileCBs);

  // Start Bond Manager and register callback
  VOID GAPBondMgr_Register(&multi_role_BondMgrCBs);

  // Register with GAP for HCI/Host messages. This is needed to receive HCI
  // events. For more information, see the HCI section in the User's Guide:
  // http://software-dl.ti.com/lprf/ble5stack-latest/
  GAP_RegisterForMsgs(selfEntity);

  // Register for GATT local events and ATT Responses pending for transmission
  GATT_RegisterForMsgs(selfEntity);

  // Set default values for Data Length Extension
  // Extended Data Length Feature is already enabled by default
  {
    // Set initial values to maximum, RX is set to max. by default(251 octets, 2120us)
    // Some brand smartphone is essentially needing 251/2120, so we set them here.
  #define APP_SUGGESTED_PDU_SIZE 251 //default is 27 octets(TX)
  #define APP_SUGGESTED_TX_TIME 2120 //default is 328us(TX)

    // This API is documented in hci.h
    // See the LE Data Length Extension section in the BLE5-Stack User's Guide for information on using this command:
    // http://software-dl.ti.com/lprf/ble5stack-latest/
    HCI_LE_WriteSuggestedDefaultDataLenCmd(APP_SUGGESTED_PDU_SIZE, APP_SUGGESTED_TX_TIME);
  }

  // Initialize GATT Client, used by GAPBondMgr to look for RPAO characteristic for network privacy
  GATT_InitClient();

  // Init key debouncer
  Board_initKeys(multi_role_keyChangeHandler);

  // Register to receive incoming ATT Indications/Notifications
  GATT_RegisterForInd(selfEntity);

  // Initialize Connection List
  multi_role_clearConnListEntry(LINKDB_CONNHANDLE_ALL);

  // Setup the GAP Bond Manager
  BLE_LOG_INT_TIME(0, BLE_LOG_MODULE_APP, "APP : ---- call GAP_DeviceInit", GAP_PROFILE_PERIPHERAL | GAP_PROFILE_CENTRAL);
  //Initialize GAP layer for Peripheral and Central role and register to receive GAP events
  GAP_DeviceInit(GAP_PROFILE_PERIPHERAL | GAP_PROFILE_CENTRAL, selfEntity,
                 addrMode, &pRandomAddress);

  // The type of display is configured based on the BOARD_DISPLAY_USE...
  // preprocessor definitions
  dispHandle = Display_open(Display_Type_ANY, NULL);

  // Disable all items in the main menu
  tbm_setItemStatus(&mrMenuMain, MR_ITEM_NONE, MR_ITEM_ALL);

  // Init two button menu
  tbm_initTwoBtnMenu(dispHandle, &mrMenuMain, 5, multi_role_menuSwitchCb);

  uint8_t versionStr[OAD_SW_VER_LEN + 1];

  memcpy(versionStr, swVer, OAD_SW_VER_LEN);

  // Add in Null terminator
  versionStr[OAD_SW_VER_LEN] = 0;

  // Display Image version
  Display_print1(dispHandle, MR_ROW_SEPARATOR, 0, "== DLS On-chip OAD v%s ==",
                 versionStr);

  apps();

  /*
   * When switching from persistent app back to the user application for the
   * for the first time after an OAD the device must send a service changed
   * indication. This will cause any peers to rediscover services.
   *
   * To prevent sending a service changed IND on every boot, a flag is stored
   * in NV to determine whether or not the service changed IND needs to be
   * sent
   */
  uint8_t status = osal_snv_read(BLE_NVID_CUST_START,
                                  sizeof(sendSvcChngdOnNextBoot),
                                  (uint8 *)&sendSvcChngdOnNextBoot);
  if(status != SUCCESS)
  {
    /*
     * On first boot the NV item will not have yet been initialzed, and the read
     * will fail. Do a write to set the initial value of the flash in NV
     */
     osal_snv_write(BLE_NVID_CUST_START, sizeof(sendSvcChngdOnNextBoot),
                    (uint8 *)&sendSvcChngdOnNextBoot);
  }

}

int test_ble_flag = 0;

/*********************************************************************
* @fn      multi_role_taskFxn
*
* @brief   Application task entry point for the multi_role.
*
* @param   a0, a1 - not used.
*
* @return  None.
*/
#ifdef FREERTOS
static void multi_role_taskFxn(void* a0)
#else
static void multi_role_taskFxn(UArg a0, UArg a1)
#endif
{
  // Initialize application
  multi_role_init();

  // Application main loop
  for (;;)
  {
    // 这里可以做一个状态机的队列用于业务逻辑，将所有操作链接到这个文件
    // 串口或蓝牙数据缓冲区只用于接收状态机事件以及清除数据，供外部调用

    if (test_ble_flag == 1)
    {
      attReadReq_t req;
      uint8_t connIndex = multi_role_getConnIndex(mrConnHandle);

      // connIndex cannot be equal to or greater than MAX_NUM_BLE_CONNS
      MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

      req.handle = connList[connIndex].charHandle;
      GATT_ReadCharValue(mrConnHandle, &req, selfEntity);
      
      test_uart_puts("read char value\n");
      
      test_ble_flag = 0;
    }

    if (test_ble_flag > 1)
    {
      status_t status;
      uint8_t charVals[4] = { 0x33, 0x34, 0x35, 0x36 }; // Should be consistent with
                                                          // those in scMenuGattWrite
      attWriteReq_t req;

      uint8_t connIndex = multi_role_getConnIndex(mrConnHandle);

      req.pValue = GATT_bm_alloc(mrConnHandle, ATT_WRITE_REQ, 20, NULL);

      if ( req.pValue != NULL )
      {
          req.handle = connList[connIndex].charHandle;
          memset(req.pValue, 0, 20);
          req.len = 20;
          charVal = charVals[test_ble_flag - 2];
          // req.pValue[rand() % 20] = charVal;
          for (int i = 0; i < 20; i++)
          {
            req.pValue[i] = charVal;
          }
          req.sig = 0;
          req.cmd = 0;

          status = GATT_WriteCharValue(mrConnHandle, &req, selfEntity);
          if ( status != SUCCESS )
          {
              GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
          }

          test_uart_puts("write char value\n");
      }

      test_ble_flag = 0;
    }

    uint32_t events;

    // Waits for an event to be posted associated with the calling thread.
    // Note that an event associated with a thread is posted when a
    // message is queued to the message receive queue of the thread

#ifdef FREERTOS
    mq_receive(syncEvent, (char*)&events, sizeof(uint32_t), NULL);
#else
    events = Event_pend(syncEvent, Event_Id_NONE, MR_ALL_EVENTS,
                        ICALL_TIMEOUT_FOREVER); // event_31 + event_30
#endif

    if (events)
    {
      ICall_EntityID dest;
      ICall_ServiceEnum src;
      ICall_HciExtEvt *pMsg = NULL;

      // Fetch any available messages that might have been sent from the stack
      if (ICall_fetchServiceMsg(&src, &dest,
                                (void **)&pMsg) == ICALL_ERRNO_SUCCESS)
      {
        uint8_t safeToDealloc = TRUE;

        if ((src == ICALL_SERVICE_CLASS_BLE) && (dest == selfEntity))
        {
          ICall_Stack_Event *pEvt = (ICall_Stack_Event *)pMsg;

          // Check for BLE stack events first
          if (pEvt->signature != 0xffff)
          {
            // Process inter-task message
            safeToDealloc = multi_role_processStackMsg((ICall_Hdr *)pMsg);
          }
        }

        if (pMsg && safeToDealloc)
        {
          ICall_freeMsg(pMsg);
        }
      }

      // If RTOS queue is not empty, process app message.
      if (events & MR_QUEUE_EVT)
      {
#ifdef FREERTOS

          mrEvt_t *pMsg;
          do {
              pMsg = (mrEvt_t *)Util_dequeueMsg(g_POSIX_appMsgQueue);
              if (NULL != pMsg)
              {
                  // Process message.
                  multi_role_processAppMsg(pMsg);

                  // Free the space from the message.
                  ICall_free(pMsg);
              }
              else
              {
                  break;
              }
          }while(1);
#else
          while (!Queue_empty(appMsgQueue))
               {
                 mrEvt_t *pMsg = (mrEvt_t *)Util_dequeueMsg(appMsgQueue);
                 if (pMsg)
                 {
                   // Process message.
                   multi_role_processAppMsg(pMsg);

                   // Free the space from the message.
                   ICall_free(pMsg);
                 }
               }
#endif
      }
      // OAD events
      if(events & MR_OAD_NO_MEM_EVT)
      {
        // The OAD module is unable to allocate memory, print failure, cancel OAD
        Display_print0(dispHandle, MR_ROW_SEPARATOR, 0,
                        "OAD malloc fail, cancelling OAD");
        OAD_cancel();
#ifdef LED_DEBUG
        // Diplay is not enabled in persist app so use LED
        GPIO_write(CONFIG_GPIO_RLED, 1);
#endif //LED_DEBUG
      }
       // OAD queue processing
      if(events & MR_OAD_QUEUE_EVT)
      {
        // Process the OAD Message Queue
        uint8_t status = OAD_processQueue();

        // If the OAD state machine encountered an error, print it
        // Return codes can be found in oad_constants.h
        if(status == OAD_DL_COMPLETE)
        {
          Display_print0(dispHandle, MR_ROW_SEPARATOR, 0, "OAD DL Complete, wait for Enable");
        }
        else if(status == OAD_IMG_ID_TIMEOUT)
        {
          Display_print0(dispHandle, MR_ROW_SEPARATOR, 0, "ImgID Timeout, disconnecting");

          // This may be an attack, terminate the link,
          // Note HCI_DISCONNECT_REMOTE_USER_TERM seems to most closet reason for
          // termination at this state
          MAP_GAP_TerminateLinkReq(OAD_getactiveCxnHandle(), HCI_DISCONNECT_REMOTE_USER_TERM);
        }
        else if(status != OAD_SUCCESS)
        {
          Display_print1(dispHandle, MR_ROW_SEPARATOR, 0, "OAD Error: %d", status);
        }

      }

      if(events & MR_OAD_COMPLETE_EVT)
      {
        // Register for L2CAP Flow Control Events
        L2CAP_RegisterFlowCtrlTask(selfEntity);
      }

    }
  }
}

/*********************************************************************
* @fn      multi_role_processStackMsg
*
* @brief   Process an incoming stack message.
*
* @param   pMsg - message to process
*
* @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
*/
static uint8_t multi_role_processStackMsg(ICall_Hdr *pMsg)
{
  uint8_t safeToDealloc = TRUE;

  BLE_LOG_INT_INT(0, BLE_LOG_MODULE_APP, "APP : Stack msg status=%d, event=0x%x\n", pMsg->status, pMsg->event);

  switch (pMsg->event)
  {
    case GAP_MSG_EVENT:
      //multi_role_processRoleEvent((gapMultiRoleEvent_t *)pMsg);
      multi_role_processGapMsg((gapEventHdr_t*) pMsg);
      break;

    case GATT_MSG_EVENT:
      // Process GATT message
      safeToDealloc = multi_role_processGATTMsg((gattMsgEvent_t *)pMsg);
      break;

    case HCI_GAP_EVENT_EVENT:
    {
      // Process HCI message
      switch (pMsg->status)
      {
        case HCI_COMMAND_COMPLETE_EVENT_CODE:
        // Process HCI Command Complete Events here
        {
          multi_role_processCmdCompleteEvt((hciEvt_CmdComplete_t *) pMsg);
          break;
        }

        case HCI_BLE_HARDWARE_ERROR_EVENT_CODE:
          AssertHandler(HAL_ASSERT_CAUSE_HARDWARE_ERROR,0);
          break;

        // HCI Commands Events
        case HCI_COMMAND_STATUS_EVENT_CODE:
          {
            hciEvt_CommandStatus_t *pMyMsg = (hciEvt_CommandStatus_t *)pMsg;
            switch ( pMyMsg->cmdOpcode )
            {
              case HCI_LE_SET_PHY:
                {
                  if (pMyMsg->cmdStatus ==
                      HCI_ERROR_CODE_UNSUPPORTED_REMOTE_FEATURE)
                  {
                    Display_printf(dispHandle, MR_ROW_CUR_CONN, 0,
                            "PHY Change failure, peer does not support this");
                  }
                  else
                  {
                    Display_printf(dispHandle, MR_ROW_CUR_CONN, 0,
                                   "PHY Update Status: 0x%02x",
                                   pMyMsg->cmdStatus);
                  }
                }
                multi_role_updatePHYStat(HCI_LE_SET_PHY, (uint8_t *)pMsg);
                break;
              case HCI_DISCONNECT:
                break;

              default:
                {
                  Display_printf(dispHandle, MR_ROW_NON_CONN, 0,
                                 "Unknown Cmd Status: 0x%04x::0x%02x",
                                 pMyMsg->cmdOpcode, pMyMsg->cmdStatus);
                }
              break;
            }
          }
          break;

        // LE Events
        case HCI_LE_EVENT_CODE:
        {
          hciEvt_BLEPhyUpdateComplete_t *pPUC
            = (hciEvt_BLEPhyUpdateComplete_t*) pMsg;

          if (pPUC->BLEEventCode == HCI_BLE_PHY_UPDATE_COMPLETE_EVENT)
          {
            if (pPUC->status != SUCCESS)
            {

              Display_printf(dispHandle, MR_ROW_ANY_CONN, 0,
                             "%s: PHY Change failure",
                             multi_role_getConnAddrStr(pPUC->connHandle));
            }
            else
            {
              Display_printf(dispHandle, MR_ROW_ANY_CONN, 0,
                             "%s: PHY updated to %s",
                             multi_role_getConnAddrStr(pPUC->connHandle),
              // Only symmetrical PHY is supported.
              // rxPhy should be equal to txPhy.
                             (pPUC->rxPhy == PHY_UPDATE_COMPLETE_EVENT_1M) ? "1 Mbps" :
                             (pPUC->rxPhy == PHY_UPDATE_COMPLETE_EVENT_2M) ? "2 Mbps" :
                             (pPUC->rxPhy == PHY_UPDATE_COMPLETE_EVENT_CODED) ? "Coded" : "Unexpected PHY Value");
            }
          }

          break;
        }

        default:
          break;
      }

      break;
    }

    case L2CAP_SIGNAL_EVENT:
      // Process L2CAP signal
      safeToDealloc = multi_role_processL2CAPMsg((l2capSignalEvent_t *)pMsg);
      break;

    default:
      // do nothing
      break;
  }

  return (safeToDealloc);
}

/*********************************************************************
 * @fn      multi_role_processL2CAPMsg
 *
 * @brief   Process L2CAP messages and events.
 *
 * @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
 */
static uint8_t multi_role_processL2CAPMsg(l2capSignalEvent_t *pMsg)
{
  uint8_t safeToDealloc = TRUE;
  static bool firstRun = TRUE;

  switch (pMsg->opcode)
  {
    case L2CAP_NUM_CTRL_DATA_PKT_EVT:
    {
      /*
      * We cannot reboot the device immediately after receiving
      * the enable command, we must allow the stack enough time
      * to process and respond to the OAD_EXT_CTRL_ENABLE_IMG
      * command. This command will determine the number of
      * packets currently queued up by the LE controller.
      * BIM var is already set via OadPersistApp_processOadWriteCB
      */
      if(firstRun)
      {
        firstRun = false;

        // We only want to set the numPendingMsgs once
        numPendingMsgs = MAX_NUM_PDU - pMsg->cmd.numCtrlDataPktEvt.numDataPkt;

        // Wait until all PDU have been sent on cxn events
        Gap_RegisterConnEventCb(multi_role_connEvtCB,
                                  GAP_CB_REGISTER,
                                  GAP_CB_CONN_EVENT_ALL,
                                  resetConnHandle);
                                  //OAD_getactiveCxnHandle());
                                  //pMsg->connHandle);
                                  //0);

        /* Set the flag so that the connection event callback will
         * be processed in the context of a pending OAD reboot
         */
        oadWaitReboot = true;
      }

      break;
    }

    default:
      // do nothing
      break;
  }

  return (safeToDealloc);
}

/*********************************************************************
 * @fn      multi_role_processGapMsg
 *
 * @brief   GAP message processing function.
 *
 * @param   pMsg - pointer to event message structure
 *
 * @return  none
 */
static void multi_role_processGapMsg(gapEventHdr_t *pMsg)
{
  switch (pMsg->opcode)
  {
    case GAP_DEVICE_INIT_DONE_EVENT:
    {
      gapDeviceInitDoneEvent_t *pPkt = (gapDeviceInitDoneEvent_t *)pMsg;

      BLE_LOG_INT_TIME(0, BLE_LOG_MODULE_APP, "APP : ---- got GAP_DEVICE_INIT_DONE_EVENT", 0);
      if(pPkt->hdr.status == SUCCESS)
      {
        // Store the system ID
        uint8_t systemId[DEVINFO_SYSTEM_ID_LEN];

        // use 6 bytes of device address for 8 bytes of system ID value
        systemId[0] = pPkt->devAddr[0];
        systemId[1] = pPkt->devAddr[1];
        systemId[2] = pPkt->devAddr[2];

        // set middle bytes to zero
        systemId[4] = 0x00;
        systemId[3] = 0x00;

        // shift three bytes up
        systemId[7] = pPkt->devAddr[5];
        systemId[6] = pPkt->devAddr[4];
        systemId[5] = pPkt->devAddr[3];

        // Set Device Info Service Parameter
        DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);

        BLE_LOG_INT_INT(0, BLE_LOG_MODULE_APP, "APP : ---- start advert %d,%d\n", 0, 0);
        //Setup and start advertising
        multi_role_advertInit();

      }

      //Setup scanning
      multi_role_scanInit();

      mrMaxPduSize = pPkt->dataPktLen;

      // Enable "Discover Devices", "Set Scanning PHY", and "Set Address Type"
      // in the main menu
      tbm_setItemStatus(&mrMenuMain, MR_ITEM_STARTDISC | MR_ITEM_ADVERTISE | MR_ITEM_PHY, MR_ITEM_NONE);

      //Display initialized state status
      Display_printf(dispHandle, MR_ROW_NUM_CONN, 0, "Num Conns: %d", numConn);
      Display_printf(dispHandle, MR_ROW_NON_CONN, 0, "Initialized");
      Display_printf(dispHandle, MR_ROW_MYADDRSS, 0, "Multi-Role Address: %s",(char *)Util_convertBdAddr2Str(pPkt->devAddr));

      break;
    }

    case GAP_CONNECTING_CANCELLED_EVENT:
    {
      uint16_t itemsToEnable = MR_ITEM_STARTDISC| MR_ITEM_ADVERTISE | MR_ITEM_PHY;

      if (numConn > 0)
      {
        itemsToEnable |= MR_ITEM_SELECTCONN;
      }

      Display_printf(dispHandle, MR_ROW_NON_CONN, 0,
                     "Conneting attempt cancelled");

      // Enable "Discover Devices", "Connect To", and "Set Scanning PHY"
      // and disable everything else.
      tbm_setItemStatus(&mrMenuMain,
                        itemsToEnable, MR_ITEM_ALL & ~itemsToEnable);

      break;
    }

    case GAP_LINK_ESTABLISHED_EVENT:
    {
      uint16_t connHandle = ((gapEstLinkReqEvent_t*) pMsg)->connectionHandle;
      uint8_t role = ((gapEstLinkReqEvent_t*) pMsg)->connRole;
      uint8_t* pAddr      = ((gapEstLinkReqEvent_t*) pMsg)->devAddr;
      uint8_t  connIndex;
      uint32_t itemsToDisable = MR_ITEM_STOPDISC | MR_ITEM_CANCELCONN;
      uint8_t* pStrAddr;
      uint8_t i;
      uint8_t numConnectable = 0;

      BLE_LOG_INT_TIME(0, BLE_LOG_MODULE_APP, "APP : ---- got GAP_LINK_ESTABLISHED_EVENT", 0);
      // Add this connection info to the list
      connIndex = multi_role_addConnInfo(connHandle, pAddr, role);

      // connIndex cannot be equal to or greater than MAX_NUM_BLE_CONNS
      MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

      connList[connIndex].charHandle = 0;

      Util_startClock(&clkPeriodic);

      pStrAddr = (uint8_t*) Util_convertBdAddr2Str(connList[connIndex].addr);

      Display_printf(dispHandle, MR_ROW_NON_CONN, 0, "Connected to %s", pStrAddr);
      Display_printf(dispHandle, MR_ROW_NUM_CONN, 0, "Num Conns: %d", numConn);

      // Disable "Connect To" until another discovery is performed
      itemsToDisable |= MR_ITEM_CONNECT;

      // If we already have maximum allowed number of connections,
      // disable device discovery and additional connection making.
      if (numConn >= MAX_NUM_BLE_CONNS)
      {
        itemsToDisable |= MR_ITEM_STARTDISC;
      }

      for (i = 0; i < TBM_GET_NUM_ITEM(&mrMenuConnect); i++)
      {
        if (!memcmp(TBM_GET_ACTION_DESC(&mrMenuConnect, i), pStrAddr,
            MR_ADDR_STR_SIZE))
        {
          // Disable this device from the connection choices
          tbm_setItemStatus(&mrMenuConnect, MR_ITEM_NONE, 1 << i);
        }
        else if (TBM_IS_ITEM_ACTIVE(&mrMenuConnect, i))
        {
          numConnectable++;
        }
      }

      // Enable/disable Main menu items properly
      tbm_setItemStatus(&mrMenuMain,
                        MR_ITEM_ALL & ~(itemsToDisable), itemsToDisable);

      if (numConn < MAX_NUM_BLE_CONNS)
      {
        // Start advertising since there is room for more connections
        GapAdv_enable(advHandle, GAP_ADV_ENABLE_OPTIONS_USE_MAX, 0);
      }
      else
      {
        // Stop advertising since there is no room for more connections
        GapAdv_disable(advHandle);
      }

      break;
    }

    case GAP_LINK_TERMINATED_EVENT:
    {
      uint16_t connHandle = ((gapTerminateLinkEvent_t*) pMsg)->connectionHandle;
      uint8_t connIndex;
      uint32_t itemsToEnable = MR_ITEM_STARTDISC | MR_ITEM_ADVERTISE | MR_ITEM_PHY;
      uint8_t* pStrAddr;
      uint8_t i;
      uint8_t numConnectable = 0;

      BLE_LOG_INT_STR(0, BLE_LOG_MODULE_APP, "APP : GAP msg: status=%d, opcode=%s\n", 0, "GAP_LINK_TERMINATED_EVENT");
      // Mark this connection deleted in the connected device list.
      connIndex = multi_role_removeConnInfo(connHandle);

      // connIndex cannot be equal to or greater than MAX_NUM_BLE_CONNS
      MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

      pStrAddr = (uint8_t*) Util_convertBdAddr2Str(connList[connIndex].addr);

      Display_printf(dispHandle, MR_ROW_NON_CONN, 0, "%s is disconnected",
                     pStrAddr);
      Display_printf(dispHandle, MR_ROW_NUM_CONN, 0, "Num Conns: %d", numConn);

      for (i = 0; i < TBM_GET_NUM_ITEM(&mrMenuConnect); i++)
      {
        if (!memcmp(TBM_GET_ACTION_DESC(&mrMenuConnect, i), pStrAddr,
                     MR_ADDR_STR_SIZE))
        {
          // Enable this device in the connection choices
          tbm_setItemStatus(&mrMenuConnect, 1 << i, MR_ITEM_NONE);
        }

        if (TBM_IS_ITEM_ACTIVE(&mrMenuConnect, i))
        {
          numConnectable++;
        }
      }

      // Start advertising since there is room for more connections
      GapAdv_enable(advHandle, GAP_ADV_ENABLE_OPTIONS_USE_MAX , 0);

      if (numConn > 0)
      {
        // There still is an active connection to select
        itemsToEnable |= MR_ITEM_SELECTCONN;
      }

      // If no active connections
      if (numConn == 0)
      {
        // Stop periodic clock
        Util_stopClock(&clkPeriodic);
        tbm_setItemStatus(&mrMenuMain, TBM_ITEM_NONE, TBM_ITEM_ALL);
        tbm_setItemStatus(&mrMenuMain, MR_ITEM_NONE, MR_ITEM_ALL ^(MR_ITEM_STARTDISC | MR_ITEM_ADVERTISE | MR_ITEM_PHY));
      }

      // Enable/disable items properly.
      tbm_setItemStatus(&mrMenuMain,
                        itemsToEnable, MR_ITEM_ALL & ~itemsToEnable);

      // If we are in the context which the teminated connection was associated
      // with, go to main menu.
      if (connHandle == mrConnHandle)
      {
        tbm_goTo(&mrMenuMain);
      }

      break;
    }

    case GAP_UPDATE_LINK_PARAM_REQ_EVENT:
    {
      gapUpdateLinkParamReqReply_t rsp;
      gapUpdateLinkParamReqEvent_t *pReq = (gapUpdateLinkParamReqEvent_t *)pMsg;

      rsp.connectionHandle = pReq->req.connectionHandle;
      rsp.signalIdentifier = pReq->req.signalIdentifier;

      // Only accept connection intervals with slave latency of 0
      // This is just an example of how the application can send a response
      if(pReq->req.connLatency == 0)
      {
        rsp.intervalMin = pReq->req.intervalMin;
        rsp.intervalMax = pReq->req.intervalMax;
        rsp.connLatency = pReq->req.connLatency;
        rsp.connTimeout = pReq->req.connTimeout;
        rsp.accepted = TRUE;
      }
      else
      {
        rsp.accepted = FALSE;
      }

      // Send Reply
      VOID GAP_UpdateLinkParamReqReply(&rsp);

      break;
    }

     case GAP_LINK_PARAM_UPDATE_EVENT:
      {
        gapLinkUpdateEvent_t *pPkt = (gapLinkUpdateEvent_t *)pMsg;

        // Get the address from the connection handle
        linkDBInfo_t linkInfo;
        if (linkDB_GetInfo(pPkt->connectionHandle, &linkInfo) ==  SUCCESS)
        {

          if(pPkt->status == SUCCESS)
          {
            Display_printf(dispHandle, MR_ROW_CUR_CONN, 0,
                          "Updated: %s, connTimeout:%d",
                           Util_convertBdAddr2Str(linkInfo.addr),
                           linkInfo.connTimeout*CONN_TIMEOUT_MS_CONVERSION);
          }
          else
          {
            // Display the address of the connection update failure
            Display_printf(dispHandle, MR_ROW_CUR_CONN, 0,
                           "Update Failed 0x%h: %s", pPkt->opcode,
                           Util_convertBdAddr2Str(linkInfo.addr));
          }
        }
        // Check if there are any queued parameter updates
        mrConnHandleEntry_t *connHandleEntry = (mrConnHandleEntry_t *)List_get(&paramUpdateList);
        if (connHandleEntry != NULL)
        {
          // Attempt to send queued update now
          multi_role_processParamUpdate(connHandleEntry->connHandle);

          // Free list element
          ICall_free(connHandleEntry);
        }
        break;
      }

#if defined ( NOTIFY_PARAM_UPDATE_RJCT )
     case GAP_LINK_PARAM_UPDATE_REJECT_EVENT:
     {
       linkDBInfo_t linkInfo;
       gapLinkUpdateEvent_t *pPkt = (gapLinkUpdateEvent_t *)pMsg;

       // Get the address from the connection handle
       linkDB_GetInfo(pPkt->connectionHandle, &linkInfo);

       // Display the address of the connection update failure
       Display_printf(dispHandle, MR_ROW_CUR_CONN, 0,
                      "Peer Device's Update Request Rejected 0x%h: %s", pPkt->opcode,
                      Util_convertBdAddr2Str(linkInfo.addr));

       break;
     }
#endif

    default:
      break;
  }
}

/*********************************************************************
* @fn      multi_role_scanInit
*
* @brief   Setup initial device scan settings.
*
* @return  None.
*/
static void multi_role_scanInit(void)
{
  uint8_t temp8;
  uint16_t temp16;

  // Setup scanning
  // For more information, see the GAP section in the User's Guide:
  // http://software-dl.ti.com/lprf/ble5stack-latest/

  // Register callback to process Scanner events
  GapScan_registerCb(multi_role_scanCB, NULL);

  // Set Scanner Event Mask
  GapScan_setEventMask(GAP_EVT_SCAN_ENABLED | GAP_EVT_SCAN_DISABLED |
                       GAP_EVT_ADV_REPORT);

  // Set Scan PHY parameters
  GapScan_setPhyParams(DEFAULT_SCAN_PHY, DEFAULT_SCAN_TYPE,
                       DEFAULT_SCAN_INTERVAL, DEFAULT_SCAN_WINDOW);

  // Set Advertising report fields to keep
  temp16 = ADV_RPT_FIELDS;
  GapScan_setParam(SCAN_PARAM_RPT_FIELDS, &temp16);
  // Set Scanning Primary PHY
  temp8 = DEFAULT_SCAN_PHY;
  GapScan_setParam(SCAN_PARAM_PRIM_PHYS, &temp8);
  // Set LL Duplicate Filter
  temp8 = SCANNER_DUPLICATE_FILTER;
  GapScan_setParam(SCAN_PARAM_FLT_DUP, &temp8);

  // Set PDU type filter -
  // Only 'Connectable' and 'Complete' packets are desired.
  // It doesn't matter if received packets are
  // whether Scannable or Non-Scannable, whether Directed or Undirected,
  // whether Scan_Rsp's or Advertisements, and whether Legacy or Extended.
  temp16 = SCAN_FLT_PDU_CONNECTABLE_ONLY | SCAN_FLT_PDU_COMPLETE_ONLY;
  GapScan_setParam(SCAN_PARAM_FLT_PDU_TYPE, &temp16);

  // Set initiating PHY parameters
  GapInit_setPhyParam(DEFAULT_INIT_PHY, INIT_PHYPARAM_CONN_INT_MIN,
					  INIT_PHYPARAM_MIN_CONN_INT);
  GapInit_setPhyParam(DEFAULT_INIT_PHY, INIT_PHYPARAM_CONN_INT_MAX,
					  INIT_PHYPARAM_MAX_CONN_INT);
}

/*********************************************************************
* @fn      multi_role_advertInit
*
* @brief   Setup initial advertisment and start advertising from device init.
*
* @return  None.
*/
static void multi_role_advertInit(void)
{
  uint8_t status = FAILURE;
  // Setup and start Advertising
  // For more information, see the GAP section in the User's Guide:
  // http://software-dl.ti.com/lprf/ble5stack-latest/


  BLE_LOG_INT_INT(0, BLE_LOG_MODULE_APP, "APP : ---- call GapAdv_create set=%d,%d\n", 1, 0);
  // Create Advertisement set #1 and assign handle
  GapAdv_create(&multi_role_advCB, &advParams1,
                &advHandle);

  // Load advertising data for set #1 that is statically allocated by the app
  GapAdv_loadByHandle(advHandle, GAP_ADV_DATA_TYPE_ADV,
                      sizeof(advData1), advData1);

  // Load scan response data for set #1 that is statically allocated by the app
  GapAdv_loadByHandle(advHandle, GAP_ADV_DATA_TYPE_SCAN_RSP,
                      sizeof(scanResData1), scanResData1);

  // Set event mask for set #1
  GapAdv_setEventMask(advHandle,
                      GAP_ADV_EVT_MASK_START_AFTER_ENABLE |
                      GAP_ADV_EVT_MASK_END_AFTER_DISABLE |
                      GAP_ADV_EVT_MASK_SET_TERMINATED);

  BLE_LOG_INT_TIME(0, BLE_LOG_MODULE_APP, "APP : ---- GapAdv_enable", 0);
  // Enable legacy advertising for set #1
  status = GapAdv_enable(advHandle, GAP_ADV_ENABLE_OPTIONS_USE_MAX , 0);

  if(status != SUCCESS)
  {
    mrIsAdvertising = false;
    Display_printf(dispHandle, MR_ROW_ADVERTIS, 0, "Error: Failed to Start Advertising!");
  }

  if (addrMode > ADDRMODE_RANDOM)
  {
    multi_role_updateRPA();
#ifdef FREERTOS
    // Create one-shot clock for RPA check event.
    Util_constructClock(&clkRpaRead, (void*)multi_role_clockHandler,
                        READ_RPA_PERIOD, 0, true,
                        (void*) &argRpaRead);

#else
    // Create one-shot clock for RPA check event.
    Util_constructClock(&clkRpaRead, multi_role_clockHandler,
                        READ_RPA_PERIOD, 0, true,
                        (UArg) &argRpaRead);
#endif
  }
}

/*********************************************************************
 * @fn      multi_role_advCB
 *
 * @brief   GapAdv module callback
 *
 * @param   pMsg - message to process
 */
static void multi_role_advCB(uint32_t event, void *pBuf, uintptr_t arg)
{
  mrGapAdvEventData_t *pData = ICall_malloc(sizeof(mrGapAdvEventData_t));

  if (pData)
  {
    pData->event = event;
    pData->pBuf = pBuf;

    if(multi_role_enqueueMsg(MR_EVT_ADV, pData) != SUCCESS)
    {
      ICall_free(pData);
    }
  }
}


/*********************************************************************
* @fn      multi_role_processGATTMsg
*
* @brief   Process GATT messages and events.
*
* @return  TRUE if safe to deallocate incoming message, FALSE otherwise.
*/
static uint8_t multi_role_processGATTMsg(gattMsgEvent_t *pMsg)
{
  // Get connection index from handle
  uint8_t connIndex = multi_role_getConnIndex(pMsg->connHandle);
  MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

  if (pMsg->method == ATT_FLOW_CTRL_VIOLATED_EVENT)
  {
    // ATT request-response or indication-confirmation flow control is
    // violated. All subsequent ATT requests or indications will be dropped.
    // The app is informed in case it wants to drop the connection.

    // Display the opcode of the message that caused the violation.
    Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "FC Violated: %d", pMsg->msg.flowCtrlEvt.opcode);
  }
  else if (pMsg->method == ATT_MTU_UPDATED_EVENT)
  {
    // MTU size updated
    OAD_setBlockSize(pMsg->msg.mtuEvt.MTU);
    Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "MTU Size: %d", pMsg->msg.mtuEvt.MTU);
  }


  // Messages from GATT server
  if (linkDB_Up(pMsg->connHandle))
  {
    if ((pMsg->method == ATT_READ_RSP)   ||
        ((pMsg->method == ATT_ERROR_RSP) &&
         (pMsg->msg.errorRsp.reqOpcode == ATT_READ_REQ)))
    {
      if (pMsg->method == ATT_ERROR_RSP)
      {
        Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Read Error %d", pMsg->msg.errorRsp.errCode);
      }
      else
      {
        // After a successful read, display the read value
        Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Read rsp: %d", pMsg->msg.readRsp.pValue[0]);
        // SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR, sizeof(uint8_t),
        //                           &pMsg->msg.readRsp.pValue[0]);
        SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR6, pMsg->msg.readRsp.len,
                                   pMsg->msg.readRsp.pValue);
      }

    }
    else if ((pMsg->method == ATT_WRITE_RSP)  ||
             ((pMsg->method == ATT_ERROR_RSP) &&
              (pMsg->msg.errorRsp.reqOpcode == ATT_WRITE_REQ)))
    {

      if (pMsg->method == ATT_ERROR_RSP)
      {
        Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Write Error %d", pMsg->msg.errorRsp.errCode);
      }
      else
      {
        // After a succesful write, display the value that was written and
        // increment value
        Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Write sent: %d", charVal);
      }

      tbm_goTo(&mrMenuPerConn);
    }
    else if (connList[connIndex].discState != BLE_DISC_STATE_IDLE)
    {
      multi_role_processGATTDiscEvent(pMsg);
    }
  } // Else - in case a GATT message came after a connection has dropped, ignore it.

  // Free message payload. Needed only for ATT Protocol messages
  GATT_bm_free(&pMsg->msg, pMsg->method);

  // It's safe to free the incoming message
  return (TRUE);
}

/*********************************************************************
 * @fn		multi_role_processParamUpdate
 *
 * @brief	Process connection parameters update
 *
 * @param	connHandle - connection handle to update
 *
 * @return	None.
 */
static void multi_role_processParamUpdate(uint16_t connHandle)
{
  gapUpdateLinkParamReq_t req;
  uint8_t connIndex;

  req.connectionHandle = connHandle;
#ifdef DEFAULT_SEND_PARAM_UPDATE_REQ
  req.connLatency = DEFAULT_DESIRED_SLAVE_LATENCY;
  req.connTimeout = DEFAULT_DESIRED_CONN_TIMEOUT;
  req.intervalMin = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
  req.intervalMax = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
#endif

  connIndex = multi_role_getConnIndex(connHandle);
  MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

  // Deconstruct the clock object
  Clock_destruct(connList[connIndex].pUpdateClock);
  // Free clock struct, only in case it is not NULL
  if (connList[connIndex].pUpdateClock != NULL)
  {
	ICall_free(connList[connIndex].pUpdateClock);
	connList[connIndex].pUpdateClock = NULL;
  }
  // Free ParamUpdateEventData, only in case it is not NULL
  if (connList[connIndex].pParamUpdateEventData != NULL)
  {
    ICall_free(connList[connIndex].pParamUpdateEventData);
  }

  // Send parameter update
  bStatus_t status = GAP_UpdateLinkParamReq(&req);

  // If there is an ongoing update, queue this for when the udpate completes
  if (status == bleAlreadyInRequestedMode)
  {
    mrConnHandleEntry_t *connHandleEntry = ICall_malloc(sizeof(mrConnHandleEntry_t));
    if (connHandleEntry)
    {
      connHandleEntry->connHandle = connHandle;

      List_put(&paramUpdateList, (List_Elem *)connHandleEntry);
    }
  }
}

/*********************************************************************
* @fn      multi_role_processAppMsg
*
* @brief   Process an incoming callback from a profile.
*
* @param   pMsg - message to process
*
* @return  None.
*/
static void multi_role_processAppMsg(mrEvt_t *pMsg)
{
  bool safeToDealloc = TRUE;

  if (pMsg->event <= APP_EVT_EVENT_MAX)
  {
    BLE_LOG_INT_STR(0, BLE_LOG_MODULE_APP, "APP : App msg status=%d, event=%s\n", 0, appEventStrings[pMsg->event]);
  }
  else
  {
    BLE_LOG_INT_INT(0, BLE_LOG_MODULE_APP, "APP : App msg status=%d, event=0x%x\n", 0, pMsg->event);
  }

  switch (pMsg->event)
  {
    case MR_EVT_CHAR_CHANGE:
    {
      multi_role_processCharValueChangeEvt(*(uint8_t*)(pMsg->pData));
      break;
    }

    case MR_EVT_KEY_CHANGE:
    {
      multi_role_handleKeys(*(uint8_t *)(pMsg->pData));
      break;
    }

    case MR_EVT_ADV_REPORT:
    {
      GapScan_Evt_AdvRpt_t* pAdvRpt = (GapScan_Evt_AdvRpt_t*) (pMsg->pData);
    
#if (DEFAULT_DEV_DISC_BY_SVC_UUID == TRUE)
      if (multi_role_findSvcUuid(SIMPLEPROFILE_SERV_UUID,
                                 pAdvRpt->pData, pAdvRpt->dataLen))
      {
        multi_role_addScanInfo(pAdvRpt->addr, pAdvRpt->addrType);
        Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Discovered: %s %d",
                       Util_convertBdAddr2Str(pAdvRpt->addr), pAdvRpt->addrType);
      }
#else // !DEFAULT_DEV_DISC_BY_SVC_UUID
      Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Discovered: %s addrType %d",
                    Util_convertBdAddr2Str(pAdvRpt->addr), pAdvRpt->addrType);
#endif // DEFAULT_DEV_DISC_BY_SVC_UUID
      
      test_uart_puts("Discovered: ");
      test_uart_puts(Util_convertBdAddr2Str(pAdvRpt->addr));
      test_uart_puts("\r\n");

      // Free scan payload data
      if (pAdvRpt->pData != NULL)
      {
        ICall_free(pAdvRpt->pData);
      }
      break;
    }

    case MR_EVT_SCAN_ENABLED:
    {
      // Disable everything but "Stop Discovering" on the menu
      tbm_setItemStatus(&mrMenuMain, MR_ITEM_STOPDISC,
                       (MR_ITEM_ALL & ~MR_ITEM_STOPDISC));
      Display_printf(dispHandle, MR_ROW_NON_CONN, 0, "Discovering...");

      break;
    }

    case MR_EVT_SCAN_DISABLED:
    {
      uint8_t numReport;
      uint8_t i;
      static uint8_t* pAddrs = NULL;
      uint8_t* pAddrTemp;
      uint16_t itemsToEnable = MR_ITEM_STARTDISC | MR_ITEM_ADVERTISE | MR_ITEM_PHY;
#if (DEFAULT_DEV_DISC_BY_SVC_UUID == TRUE)
      numReport = numScanRes;
#else // !DEFAULT_DEV_DISC_BY_SVC_UUID
      GapScan_Evt_AdvRpt_t advRpt;

      numReport = ((GapScan_Evt_End_t*) (pMsg->pData))->numReport;
#endif // DEFAULT_DEV_DISC_BY_SVC_UUID

      Display_printf(dispHandle, MR_ROW_NON_CONN, 0,
                     "%d devices discovered", numReport);

      if (numReport > 0)
      {
        // Also enable "Connect to"
        itemsToEnable |= MR_ITEM_CONNECT;
      }

      if (numConn > 0)
      {
        // Also enable "Work with"
        itemsToEnable |= MR_ITEM_SELECTCONN;
      }

      // Enable "Discover Devices", "Set Scanning PHY", and possibly
      // "Connect to" and/or "Work with".
      // Disable "Stop Discovering".
      tbm_setItemStatus(&mrMenuMain, itemsToEnable, MR_ITEM_STOPDISC);
      if (pAddrs != NULL)
      {
        ICall_free(pAddrs);
      }
      // Allocate buffer to display addresses
      pAddrs = ICall_malloc(numReport * MR_ADDR_STR_SIZE);
      if (pAddrs == NULL)
      {
        numReport = 0;
      }

      TBM_SET_NUM_ITEM(&mrMenuConnect, numReport);

      if (pAddrs != NULL)
      {
        pAddrTemp = pAddrs;
        for (i = 0; i < numReport; i++, pAddrTemp += MR_ADDR_STR_SIZE)
        {
  #if (DEFAULT_DEV_DISC_BY_SVC_UUID == TRUE)
          // Get the address from the list, convert it to string, and
          // copy the string to the address buffer
          memcpy(pAddrTemp, Util_convertBdAddr2Str(scanList[i].addr),
                 MR_ADDR_STR_SIZE);
  #else // !DEFAULT_DEV_DISC_BY_SVC_UUID
          // Get the address from the report, convert it to string, and
          // copy the string to the address buffer
          GapScan_getAdvReport(i, &advRpt);
          memcpy(pAddrTemp, Util_convertBdAddr2Str(advRpt.addr),
                 MR_ADDR_STR_SIZE);
  #endif // DEFAULT_DEV_DISC_BY_SVC_UUID

          // Assign the string to the corresponding action description of the menu
          TBM_SET_ACTION_DESC(&mrMenuConnect, i, pAddrTemp);
          tbm_setItemStatus(&mrMenuConnect, (1 << i) , TBM_ITEM_NONE);
        }

        // Disable any non-active scan results
        for(; i < DEFAULT_MAX_SCAN_RES; i++)
        {
          tbm_setItemStatus(&mrMenuConnect, TBM_ITEM_NONE, (1 << i));
        }
      }
      break;
    }

    case MR_EVT_SVC_DISC:
    {
      multi_role_startSvcDiscovery();
      break;
    }

    case MR_EVT_ADV:
    {
      multi_role_processAdvEvent((mrGapAdvEventData_t*)(pMsg->pData));
      break;
    }

    case MR_EVT_PAIRING_STATE:
    {
      multi_role_processPairState((mrPairStateData_t*)(pMsg->pData));
      break;
    }

    case MR_EVT_PASSCODE_NEEDED:
    {
      multi_role_processPasscode((mrPasscodeData_t*)(pMsg->pData));
      break;
    }

    case MR_EVT_SEND_PARAM_UPDATE:
    {
      // Extract connection handle from data
      uint16_t locConnHandle = *(uint16_t *)(((mrClockEventData_t *)pMsg->pData)->data);
      multi_role_processParamUpdate(locConnHandle);
      safeToDealloc = FALSE;
      break;
    }

    case MR_EVT_PERIODIC:
    {
      multi_role_performPeriodicTask();
      break;
    }

    case MR_EVT_READ_RPA:
    {
      multi_role_updateRPA();
      break;
    }

    case MR_EVT_INSUFFICIENT_MEM:
    {
      // We are running out of memory.
      Display_printf(dispHandle, MR_ROW_ANY_CONN, 0, "Insufficient Memory");

      // We might be in the middle of scanning, try stopping it.
      GapScan_disable();
      break;
    }

    case MR_CONN_EVT:
      multi_role_processConnEvt((Gap_ConnEventRpt_t *)(pMsg->pData));
      break;

    case MR_OAD_RESET_EVT:
      multi_role_processOadResetEvt((oadResetWrite_t *)(pMsg->pData));
      break;

    default:
      // Do nothing.
      break;
  }

  if ((safeToDealloc == TRUE) && (pMsg->pData != NULL))
  {
    ICall_free(pMsg->pData);
  }
}

/*********************************************************************
 * @fn      multi_role_processAdvEvent
 *
 * @brief   Process advertising event in app context
 *
 * @param   pEventData
 */
static void multi_role_processAdvEvent(mrGapAdvEventData_t *pEventData)
{
  switch (pEventData->event)
  {
    case GAP_EVT_ADV_START_AFTER_ENABLE:
      BLE_LOG_INT_TIME(0, BLE_LOG_MODULE_APP, "APP : ---- GAP_EVT_ADV_START_AFTER_ENABLE", 0);
      mrIsAdvertising = true;
      Display_printf(dispHandle, MR_ROW_ADVERTIS, 0, "Adv Set %d Enabled",
                     *(uint8_t *)(pEventData->pBuf));
      break;

    case GAP_EVT_ADV_END_AFTER_DISABLE:
      mrIsAdvertising = false;
      Display_printf(dispHandle, MR_ROW_ADVERTIS, 0, "Adv Set %d Disabled",
                     *(uint8_t *)(pEventData->pBuf));
      break;

    case GAP_EVT_ADV_START:
      Display_printf(dispHandle, MR_ROW_ADVERTIS, 0, "Adv Started %d Enabled",
                     *(uint8_t *)(pEventData->pBuf));
      break;

    case GAP_EVT_ADV_END:
      Display_printf(dispHandle, MR_ROW_ADVERTIS, 0, "Adv Ended %d Disabled",
                     *(uint8_t *)(pEventData->pBuf));
      break;

    case GAP_EVT_ADV_SET_TERMINATED:
    {
      mrIsAdvertising = false;
#ifndef Display_DISABLE_ALL
      GapAdv_setTerm_t *advSetTerm = (GapAdv_setTerm_t *)(pEventData->pBuf);
#endif
      Display_printf(dispHandle, MR_ROW_ADVERTIS, 0, "Adv Set %d disabled after conn %d",
                     advSetTerm->handle, advSetTerm->connHandle );
    }
    break;

    case GAP_EVT_SCAN_REQ_RECEIVED:
      break;

    case GAP_EVT_INSUFFICIENT_MEMORY:
      break;

    default:
      break;
  }

  // All events have associated memory to free except the insufficient memory
  // event
  if (pEventData->event != GAP_EVT_INSUFFICIENT_MEMORY)
  {
    ICall_free(pEventData->pBuf);
  }
}

#if (DEFAULT_DEV_DISC_BY_SVC_UUID == TRUE)
/*********************************************************************
 * @fn      multi_role_findSvcUuid
 *
 * @brief   Find a given UUID in an advertiser's service UUID list.
 *
 * @return  TRUE if service UUID found
 */
static bool multi_role_findSvcUuid(uint16_t uuid, uint8_t *pData,
                                      uint16_t dataLen)
{
  uint8_t adLen;
  uint8_t adType;
  uint8_t *pEnd;

  if (dataLen > 0)
  {
    pEnd = pData + dataLen - 1;

    // While end of data not reached
    while (pData < pEnd)
    {
      // Get length of next AD item
      adLen = *pData++;
      if (adLen > 0)
      {
        adType = *pData;

        // If AD type is for 16-bit service UUID
        if ((adType == GAP_ADTYPE_16BIT_MORE) ||
            (adType == GAP_ADTYPE_16BIT_COMPLETE))
        {
          pData++;
          adLen--;

          // For each UUID in list
          while (adLen >= 2 && pData < pEnd)
          {
            // Check for match
            if ((pData[0] == LO_UINT16(uuid)) && (pData[1] == HI_UINT16(uuid)))
            {
              // Match found
              return TRUE;
            }

            // Go to next
            pData += 2;
            adLen -= 2;
          }

          // Handle possible erroneous extra byte in UUID list
          if (adLen == 1)
          {
            pData++;
          }
        }
        else
        {
          // Go to next item
          pData += adLen;
        }
      }
    }
  }

  // Match not found
  return FALSE;
}

/*********************************************************************
 * @fn      multi_role_addScanInfo
 *
 * @brief   Add a device to the scanned device list
 *
 * @return  none
 */
static void multi_role_addScanInfo(uint8_t *pAddr, uint8_t addrType)
{
  uint8_t i;

  // If result count not at max
  if (numScanRes < DEFAULT_MAX_SCAN_RES)
  {
    // Check if device is already in scan results
    for (i = 0; i < numScanRes; i++)
    {
      if (memcmp(pAddr, scanList[i].addr , B_ADDR_LEN) == 0)
      {
        return;
      }
    }

    // Add addr to scan result list
    memcpy(scanList[numScanRes].addr, pAddr, B_ADDR_LEN);
    scanList[numScanRes].addrType = addrType;

    // Increment scan result count
    numScanRes++;
  }
}
#endif // DEFAULT_DEV_DISC_BY_SVC_UUID

/*********************************************************************
 * @fn      multi_role_scanCB
 *
 * @brief   Callback called by GapScan module
 *
 * @param   evt - event
 * @param   msg - message coming with the event
 * @param   arg - user argument
 *
 * @return  none
 */
void multi_role_scanCB(uint32_t evt, void* pMsg, uintptr_t arg)
{
  uint8_t event;

  if (evt & GAP_EVT_ADV_REPORT)
  {
    event = MR_EVT_ADV_REPORT;
  }
  else if (evt & GAP_EVT_SCAN_ENABLED)
  {
    event = MR_EVT_SCAN_ENABLED;
  }
  else if (evt & GAP_EVT_SCAN_DISABLED)
  {
    event = MR_EVT_SCAN_DISABLED;
  }
  else if (evt & GAP_EVT_INSUFFICIENT_MEMORY)
  {
    event = MR_EVT_INSUFFICIENT_MEM;
  }
  else
  {
    return;
  }

  if(multi_role_enqueueMsg(event, pMsg) != SUCCESS)
  {
    ICall_free(pMsg);
  }

}

/*********************************************************************
* @fn      multi_role_charValueChangeCB
*
* @brief   Callback from Simple Profile indicating a characteristic
*          value change.
*
* @param   paramID - parameter ID of the value that was changed.
*
* @return  None.
*/
static void multi_role_charValueChangeCB(uint8_t paramID)
{
  uint8_t *pData;

  // Allocate space for the event data.
  if ((pData = ICall_malloc(sizeof(uint8_t))))
  {
    *pData = paramID;

    // Queue the event.
    if(multi_role_enqueueMsg(MR_EVT_CHAR_CHANGE, pData) != SUCCESS)
    {
      ICall_free(pData);
    }
  }
}

/*********************************************************************
 * @fn      multi_role_enqueueMsg
 *
 * @brief   Creates a message and puts the message in RTOS queue.
 *
 * @param   event - message event.
 * @param   state - message state.
 * @param   pData - message data pointer.
 *
 * @return  TRUE or FALSE
 */
status_t multi_role_enqueueMsg(uint8_t event, void *pData)
{
  uint8_t success;
  mrEvt_t *pMsg = ICall_malloc(sizeof(mrEvt_t));

  // Create dynamic pointer to message.
  if (pMsg)
  {
    pMsg->event = event;
    pMsg->pData = pData;

    // Enqueue the message.
#ifdef FREERTOS
    success = Util_enqueueMsg(g_POSIX_appMsgQueue, syncEvent, (uint8_t *)pMsg);
#else
    success = Util_enqueueMsg(appMsgQueue, syncEvent, (uint8_t *)pMsg);
#endif
    return (success) ? SUCCESS : FAILURE;
  }

  return(bleMemAllocError);
}

/*********************************************************************
 * @fn      multi_role_processCharValueChangeEvt
 *
 * @brief   Process a pending Simple Profile characteristic value change
 *          event.
 *
 * @param   paramID - parameter ID of the value that was changed.
 */
static void multi_role_processCharValueChangeEvt(uint8_t paramId)
{
  uint8_t newValue;

  switch(paramId)
  {
    case SIMPLEPROFILE_CHAR1:
      SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR1, &newValue);

      Display_printf(dispHandle, MR_ROW_CHARSTAT, 0, "Char 1: %d", (uint16_t)newValue);
      break;

    case SIMPLEPROFILE_CHAR3:
      SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR3, &newValue);

      Display_printf(dispHandle, MR_ROW_CHARSTAT, 0, "Char 3: %d", (uint16_t)newValue);
      break;

    default:
      // should not reach here!
      break;
  }
}

/*********************************************************************
 * @fn      multi_role_performPeriodicTask
 *
 * @brief   Perform a periodic application task. This function gets called
 *          every five seconds (MR_PERIODIC_EVT_PERIOD). In this example,
 *          the value of the third characteristic in the SimpleGATTProfile
 *          service is retrieved from the profile, and then copied into the
 *          value of the the fourth characteristic.
 *
 * @param   None.
 *
 * @return  None.
 */
static void multi_role_performPeriodicTask(void)
{
  uint8_t valueToCopy;

  // Call to retrieve the value of the third characteristic in the profile
  if (SimpleProfile_GetParameter(SIMPLEPROFILE_CHAR3, &valueToCopy) == SUCCESS)
  {
    // Call to set that value of the fourth characteristic in the profile.
    // Note that if notifications of the fourth characteristic have been
    // enabled by a GATT client device, then a notification will be sent
    // every time this function is called.
    SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR4, sizeof(uint8_t),
                               &valueToCopy);
  }
}

/*********************************************************************
 * @fn      multi_role_updateRPA
 *
 * @brief   Read the current RPA from the stack and update display
 *          if the RPA has changed.
 *
 * @param   None.
 *
 * @return  None.
 */
static void multi_role_updateRPA(void)
{
  uint8_t* pRpaNew;

  // Read the current RPA.
  pRpaNew = GAP_GetDevAddress(FALSE);

  if (memcmp(pRpaNew, rpa, B_ADDR_LEN))
  {
    // If the RPA has changed, update the display
    Display_printf(dispHandle, MR_ROW_RPA, 0, "RP Addr: %s",
                   Util_convertBdAddr2Str(pRpaNew));
    memcpy(rpa, pRpaNew, B_ADDR_LEN);
  }
}

/*********************************************************************
 * @fn      multi_role_clockHandler
 *
 * @brief   Handler function for clock timeouts.
 *
 * @param   arg - event type
 *
 * @return  None.
 */
#ifdef FREERTOS
static void multi_role_clockHandler(void * arg)
#else
static void multi_role_clockHandler(UArg arg)
#endif
{
  mrClockEventData_t *pData = (mrClockEventData_t *)arg;

  if (pData->event == MR_EVT_PERIODIC)
  {
    // Start the next period
    Util_startClock(&clkPeriodic);

    // Send message to perform periodic task
    multi_role_enqueueMsg(MR_EVT_PERIODIC, NULL);
  }
  else if (pData->event == MR_EVT_READ_RPA)
  {
    // Start the next period
    Util_startClock(&clkRpaRead);

    // Send message to read the current RPA
    multi_role_enqueueMsg(MR_EVT_READ_RPA, NULL);
  }
  else if (pData->event == MR_EVT_SEND_PARAM_UPDATE)
  {
    // Send message to app
    multi_role_enqueueMsg(MR_EVT_SEND_PARAM_UPDATE, pData);
  }
}

/*********************************************************************
* @fn      multi_role_keyChangeHandler
*
* @brief   Key event handler function
*
* @param   a0 - ignored
*
* @return  none
*/
static void multi_role_keyChangeHandler(uint8_t keys)
{
  uint8_t *pValue = ICall_malloc(sizeof(uint8_t));

  if (pValue)
  {
    *pValue = keys;

    multi_role_enqueueMsg(MR_EVT_KEY_CHANGE, pValue);
  }
}

/*********************************************************************
* @fn      multi_role_handleKeys
*
* @brief   Handles all key events for this device.
*
* @param   keys - bit field for key events. Valid entries:
*                 HAL_KEY_SW_2
*                 HAL_KEY_SW_1
*
* @return  none
*/
static void multi_role_handleKeys(uint8_t keys)
{
#ifndef FREERTOS
    uint32_t rtnVal = 0;
#endif
    if (keys & KEY_LEFT)
  {
#ifndef FREERTOS
    // Check if the key is still pressed
    if (GPIO_read(CONFIG_GPIO_BTN1) == 0)
    {
#endif
      tbm_buttonLeft();
#ifndef FREERTOS
    }
#endif
  }
  else if (keys & KEY_RIGHT)
  {
    // Check if the key is still pressed
#ifndef FREERTOS
    rtnVal = GPIO_read(CONFIG_GPIO_BTN2);
    if (rtnVal == 0)
    {
#endif
    tbm_buttonRight();
#ifndef FREERTOS
    }
#endif
  }
}

/*********************************************************************
* @fn      multi_role_processGATTDiscEvent
*
* @brief   Process GATT discovery event
*
* @param   pMsg - pointer to discovery event stack message
*
* @return  none
*/
static void multi_role_processGATTDiscEvent(gattMsgEvent_t *pMsg)
{
  uint8_t connIndex = multi_role_getConnIndex(pMsg->connHandle);
  MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

  if (connList[connIndex].discState == BLE_DISC_STATE_MTU)
  {
    // MTU size response received, discover simple service
    if (pMsg->method == ATT_EXCHANGE_MTU_RSP)
    {
      uint8_t uuid[ATT_BT_UUID_SIZE] = { LO_UINT16(SIMPLEPROFILE_SERV_UUID),
                                         HI_UINT16(SIMPLEPROFILE_SERV_UUID) };

      connList[connIndex].discState = BLE_DISC_STATE_SVC;

      // Discovery simple service
      VOID GATT_DiscPrimaryServiceByUUID(pMsg->connHandle, uuid,
                                         ATT_BT_UUID_SIZE, selfEntity);
    }
  }
  else if (connList[connIndex].discState == BLE_DISC_STATE_SVC)
  {
    // Service found, store handles
    if (pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
        pMsg->msg.findByTypeValueRsp.numInfo > 0)
    {
      svcStartHdl = ATT_ATTR_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);
      svcEndHdl = ATT_GRP_END_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);
    }

    // If procedure complete
    if (((pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP) &&
         (pMsg->hdr.status == bleProcedureComplete))  ||
        (pMsg->method == ATT_ERROR_RSP))
    {
      if (svcStartHdl != 0)
      {
        attReadByTypeReq_t req;

        // Discover characteristic
        connList[connIndex].discState = BLE_DISC_STATE_CHAR;

        req.startHandle = svcStartHdl;
        req.endHandle = svcEndHdl;
        req.type.len = ATT_BT_UUID_SIZE;
        req.type.uuid[0] = LO_UINT16(SIMPLEPROFILE_CHAR6_UUID);
        req.type.uuid[1] = HI_UINT16(SIMPLEPROFILE_CHAR6_UUID);

        VOID GATT_DiscCharsByUUID(pMsg->connHandle, &req, selfEntity);
      }
    }
  }
  else if (connList[connIndex].discState == BLE_DISC_STATE_CHAR)
  {
    // Characteristic found, store handle
    if ((pMsg->method == ATT_READ_BY_TYPE_RSP) &&
        (pMsg->msg.readByTypeRsp.numPairs > 0))
    {
      uint8_t connIndex = multi_role_getConnIndex(mrConnHandle);

      // connIndex cannot be equal to or greater than MAX_NUM_BLE_CONNS
      MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

      // Store the handle of the simpleprofile characteristic 1 value
      connList[connIndex].charHandle
        = BUILD_UINT16(pMsg->msg.readByTypeRsp.pDataList[3],
                       pMsg->msg.readByTypeRsp.pDataList[4]);

      Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Simple Svc Found");
      
      // Now we can use GATT Read/Write
      tbm_setItemStatus(&mrMenuPerConn,
                        MR_ITEM_GATTREAD | MR_ITEM_GATTWRITE, MR_ITEM_NONE);
    }

    connList[connIndex].discState = BLE_DISC_STATE_IDLE;
    connList[connIndex].discExist = 1;
  }
}

/*********************************************************************
* @fn      multi_role_getConnIndex
*
* @brief   Translates connection handle to index
*
* @param   connHandle - the connection handle
*
 * @return  the index of the entry that has the given connection handle.
 *          if there is no match, MAX_NUM_BLE_CONNS will be returned.
*/
uint16_t multi_role_getConnIndex(uint16_t connHandle)
{
  uint8_t i;
  // Loop through connection
  for (i = 0; i < MAX_NUM_BLE_CONNS; i++)
  {
    // If matching connection handle found
    if (connList[i].connHandle == connHandle)
    {
      return i;
    }
  }

  // Not found if we got here
  return(MAX_NUM_BLE_CONNS);
}

#ifndef Display_DISABLE_ALL
/*********************************************************************
 * @fn      multi_role_getConnAddrStr
 *
 * @brief   Return, in string form, the address of the peer associated with
 *          the connHandle.
 *
 * @return  A null-terminated string of the address.
 *          if there is no match, NULL will be returned.
 */
static char* multi_role_getConnAddrStr(uint16_t connHandle)
{
  uint8_t i;

  for (i = 0; i < MAX_NUM_BLE_CONNS; i++)
  {
    if (connList[i].connHandle == connHandle)
    {
      return Util_convertBdAddr2Str(connList[i].addr);
    }
  }

  return NULL;
}
#endif

/*********************************************************************
 * @fn      multi_role_clearConnListEntry
 *
 * @brief   clear device list by connHandle
 *
 * @return  SUCCESS if connHandle found valid index or bleInvalidRange
 *          if index wasn't found. LINKDB_CONNHANDLE_ALL will always succeed.
 */
static uint8_t multi_role_clearConnListEntry(uint16_t connHandle)
{
  uint8_t i;
  // Set to invalid connection index initially
  uint8_t connIndex = MAX_NUM_BLE_CONNS;

  if(connHandle != LINKDB_CONNHANDLE_ALL)
  {
    connIndex = multi_role_getConnIndex(connHandle);
    // Get connection index from handle
    if(connIndex >= MAX_NUM_BLE_CONNS)
    {
      return bleInvalidRange;
    }
  }

  // Clear specific handle or all handles
  for(i = 0; i < MAX_NUM_BLE_CONNS; i++)
  {
    if((connIndex == i) || (connHandle == LINKDB_CONNHANDLE_ALL))
    {
      connList[i].connHandle = LINKDB_CONNHANDLE_INVALID;
      connList[i].charHandle = 0;
      connList[i].discState  =  0;
    }
  }

  return SUCCESS;
}


/************************************************************************
* @fn      multi_role_pairStateCB
*
* @param   connHandle - the connection handle
*
* @param   state - pairing state
*
* @param   status - status of pairing state
*
* @return  none
*/
static void multi_role_pairStateCB(uint16_t connHandle, uint8_t state,
                                   uint8_t status)
{
  mrPairStateData_t *pData = ICall_malloc(sizeof(mrPairStateData_t));

  // Allocate space for the event data.
  if (pData)
  {
    pData->state = state;
    pData->connHandle = connHandle;
    pData->status = status;

    // Queue the event.
    if (multi_role_enqueueMsg(MR_EVT_PAIRING_STATE, pData) != SUCCESS)
    {
      ICall_free(pData);
    }
  }
}

/*********************************************************************
* @fn      multi_role_passcodeCB
*
* @brief   Passcode callback.
*
* @param   deviceAddr - pointer to device address
*
* @param   connHandle - the connection handle
*
* @param   uiInputs - pairing User Interface Inputs
*
* @param   uiOutputs - pairing User Interface Outputs
*
* @param   numComparison - numeric Comparison 20 bits
*
* @return  none
*/
static void multi_role_passcodeCB(uint8_t *deviceAddr, uint16_t connHandle,
                                  uint8_t uiInputs, uint8_t uiOutputs,
                                  uint32_t numComparison)
{
  mrPasscodeData_t *pData = ICall_malloc(sizeof(mrPasscodeData_t));

  // Allocate space for the passcode event.
  if (pData)
  {
    pData->connHandle = connHandle;
    memcpy(pData->deviceAddr, deviceAddr, B_ADDR_LEN);
    pData->uiInputs = uiInputs;
    pData->uiOutputs = uiOutputs;
    pData->numComparison = numComparison;

    // Enqueue the event.
    if (multi_role_enqueueMsg(MR_EVT_PASSCODE_NEEDED, pData) != SUCCESS)
    {
      ICall_free(pData);
    }
  }
}

/*********************************************************************
* @fn      multi_role_processPairState
*
* @brief   Process the new paring state.
*
* @param   pairingEvent - pairing event received from the stack
*
* @return  none
*/
static void multi_role_processPairState(mrPairStateData_t *pPairData)
{
  uint8_t state = pPairData->state;
  uint8_t status = pPairData->status;

  switch (state)
  {
    case GAPBOND_PAIRING_STATE_STARTED:
      Display_printf(dispHandle, MR_ROW_SECURITY, 0, "Pairing started");
      break;

    case GAPBOND_PAIRING_STATE_COMPLETE:
      if (status == SUCCESS)
      {
        linkDBInfo_t linkInfo;

        Display_printf(dispHandle, MR_ROW_SECURITY, 0, "Pairing success");

        if (linkDB_GetInfo(pPairData->connHandle, &linkInfo) == SUCCESS)
        {
          // If the peer was using private address, update with ID address
          if ((linkInfo.addrType == ADDRTYPE_PUBLIC_ID ||
               linkInfo.addrType == ADDRTYPE_RANDOM_ID) &&
              !Util_isBufSet(linkInfo.addrPriv, 0, B_ADDR_LEN))

          {
            // Update the address of the peer to the ID address
            Display_printf(dispHandle, MR_ROW_NON_CONN, 0, "Addr updated: %s",
                           Util_convertBdAddr2Str(linkInfo.addr));

            // Update the connection list with the ID address
            uint8_t i = multi_role_getConnIndex(pPairData->connHandle);

            MULTIROLE_ASSERT(i < MAX_NUM_BLE_CONNS);
            memcpy(connList[i].addr, linkInfo.addr, B_ADDR_LEN);
          }
        }
      }
      else
      {
        Display_printf(dispHandle, MR_ROW_SECURITY, 0, "Pairing fail: %d", status);
      }
      break;

    case GAPBOND_PAIRING_STATE_ENCRYPTED:
      if (status == SUCCESS)
      {
        Display_printf(dispHandle, MR_ROW_SECURITY, 0, "Encryption success");
      }
      else
      {
        Display_printf(dispHandle, MR_ROW_SECURITY, 0, "Encryption failed: %d", status);
      }
      break;

    case GAPBOND_PAIRING_STATE_BOND_SAVED:
      if (status == SUCCESS)
      {
        Display_printf(dispHandle, MR_ROW_SECURITY, 0, "Bond save success");
      }
      else
      {
        Display_printf(dispHandle, MR_ROW_SECURITY, 0, "Bond save failed: %d", status);
      }

      break;

    default:
      break;
  }
}

/*********************************************************************
* @fn      multi_role_processPasscode
*
* @brief   Process the Passcode request.
*
* @return  none
*/
static void multi_role_processPasscode(mrPasscodeData_t *pData)
{
  // Display passcode to user
  if (pData->uiOutputs != 0)
  {
    Display_printf(dispHandle, MR_ROW_SECURITY, 0, "Passcode: %d",
                   B_APP_DEFAULT_PASSCODE);
  }

  // Send passcode response
  GAPBondMgr_PasscodeRsp(pData->connHandle, SUCCESS,
                         B_APP_DEFAULT_PASSCODE);
}

/*********************************************************************
 * @fn      multi_role_connEvtCB
 *
 * @brief   Connection event callback.
 *
 * @param pReport pointer to connection event report
 */
static void multi_role_connEvtCB(Gap_ConnEventRpt_t *pReport)
{
  // Enqueue the event for processing in the app context.
  if(multi_role_enqueueMsg(MR_CONN_EVT, pReport) != SUCCESS)
  {
    ICall_freeMsg(pReport);
  }
}

/*********************************************************************
 * @fn      multi_role_processConnEvt
 *
 * @brief   Process connection event.
 *
 * @param pReport pointer to connection event report
 */
static void multi_role_processConnEvt(Gap_ConnEventRpt_t *pReport)
{
  /* If we are waiting for an OAD Reboot, process connection events to ensure
   * that we are not waiting to send data before restarting
   */
  if(oadWaitReboot)
  {
    // Wait until all pending messages are sent
    if(numPendingMsgs == 0)
    {
      // Store the flag to indicate that a service changed IND will
      // be sent at the next boot
      sendSvcChngdOnNextBoot = TRUE;

      uint8_t status = osal_snv_write(BLE_NVID_CUST_START,
                                      sizeof(sendSvcChngdOnNextBoot),
                                      (uint8 *)&sendSvcChngdOnNextBoot);
      if(status != SUCCESS)
      {
        Display_print1(dispHandle, 5, 0, "SNV WRITE FAIL: %d", status);
      }

      // Reset the system
      SystemReset();
    }
    else
    {
      numPendingMsgs--;
    }
  }
  else
  {
    // Get index from handle
    uint8_t connIndex = multi_role_getConnIndex(pReport->handle);

    // If auto phy change is enabled
    if (connList[connIndex].isAutoPHYEnable == TRUE)
    {
      // Read the RSSI
      HCI_ReadRssiCmd(pReport->handle);
    }
  }
}

/*********************************************************************
 * @fn      multi_role_startSvcDiscovery
 *
 * @brief   Start service discovery.
 *
 * @return  none
 */
static void multi_role_startSvcDiscovery(void)
{
  uint8_t connIndex = multi_role_getConnIndex(mrConnHandle);

  // connIndex cannot be equal to or greater than MAX_NUM_BLE_CONNS
  MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

  attExchangeMTUReq_t req;

  // Initialize cached handles
  svcStartHdl = svcEndHdl = 0;

  connList[connIndex].discState = BLE_DISC_STATE_MTU;

  // Discover GATT Server's Rx MTU size
  req.clientRxMTU = mrMaxPduSize - L2CAP_HDR_SIZE;

  // ATT MTU size should be set to the minimum of the Client Rx MTU
  // and Server Rx MTU values
  VOID GATT_ExchangeMTU(mrConnHandle, &req, selfEntity);
}

/*********************************************************************
* @fn      multi_role_addConnInfo
*
* @brief   add a new connection to the index-to-connHandle map
*
* @param   connHandle - the connection handle
*
* @param   addr - pointer to device address
*
* @return  index of connection handle
*/
static uint8_t multi_role_addConnInfo(uint16_t connHandle, uint8_t *pAddr,
                                      uint8_t role)
{
  uint8_t i;

  for (i = 0; i < MAX_NUM_BLE_CONNS; i++)
  {
    if (connList[i].connHandle == LINKDB_CONNHANDLE_INVALID)
    {
      // Found available entry to put a new connection info in
      connList[i].connHandle = connHandle;
      memcpy(connList[i].addr, pAddr, B_ADDR_LEN);
      numConn++;

#ifdef DEFAULT_SEND_PARAM_UPDATE_REQ
      // If a peripheral, start the clock to send a connection parameter update
      if(role == GAP_PROFILE_PERIPHERAL)
      {
        // Allocate data to send through clock handler
        connList[i].pParamUpdateEventData = ICall_malloc(sizeof(mrClockEventData_t) +
                                                         sizeof(uint16_t));
        if(connList[i].pParamUpdateEventData)
        {
          // Set clock data
          connList[i].pParamUpdateEventData->event = MR_EVT_SEND_PARAM_UPDATE;
          *((uint16_t *)connList[i].pParamUpdateEventData->data) = connHandle;

          // Create a clock object and start
          connList[i].pUpdateClock
            = (Clock_Struct*) ICall_malloc(sizeof(Clock_Struct));

          if (connList[i].pUpdateClock)
          {
#ifdef FREERTOS
              Util_constructClock(connList[i].pUpdateClock,
                                              (void*)multi_role_clockHandler,
                                              SEND_PARAM_UPDATE_DELAY, 0, true,
                                              (void*) connList[i].pParamUpdateEventData);

#else
              Util_constructClock(connList[i].pUpdateClock,
                                  multi_role_clockHandler,
                                SEND_PARAM_UPDATE_DELAY, 0, true,
                                (UArg) connList[i].pParamUpdateEventData);
#endif
          }
          else
          {
            // Clean up
            ICall_free(connList[i].pParamUpdateEventData);
          }
        }
        else
        {
          // Memory allocation failed
          MULTIROLE_ASSERT(false);
        }
      }
#endif

      break;
    }
  }

  return i;
}

/*********************************************************************
 * @fn      multi_role_clearPendingParamUpdate
 *
 * @brief   clean pending param update request in the paramUpdateList list
 *
 * @param   connHandle - connection handle to clean
 *
 * @return  none
 */
void multi_role_clearPendingParamUpdate(uint16_t connHandle)
{
  List_Elem *curr;

  for (curr = List_head(&paramUpdateList); curr != NULL; curr = List_next(curr)) 
  {
    if (((mrConnHandleEntry_t *)curr)->connHandle == connHandle)
    {
      List_remove(&paramUpdateList, curr);
    }
  }
}

/*********************************************************************
 * @fn      multi_role_removeConnInfo
 *
 * @brief   Remove a device from the connected device list
 *
 * @return  index of the connected device list entry where the new connection
 *          info is removed from.
 *          if connHandle is not found, MAX_NUM_BLE_CONNS will be returned.
 */
static uint8_t multi_role_removeConnInfo(uint16_t connHandle)
{
  uint8_t connIndex = multi_role_getConnIndex(connHandle);

  if(connIndex < MAX_NUM_BLE_CONNS)
  {
    Clock_Struct* pUpdateClock = connList[connIndex].pUpdateClock;

    if (pUpdateClock != NULL)
    {
      // Stop and destruct the RTOS clock if it's still alive
      if (Util_isActive(pUpdateClock))
      {
        Util_stopClock(pUpdateClock);
      }

      // Destruct the clock object
      Clock_destruct(pUpdateClock);
      // Free clock struct
      ICall_free(pUpdateClock);
      // Free ParamUpdateEventData
      ICall_free(connList[connIndex].pParamUpdateEventData);
    }
    // Clear pending update requests from paramUpdateList
    multi_role_clearPendingParamUpdate(connHandle);
    // Clear Connection List Entry
    multi_role_clearConnListEntry(connHandle);
    numConn--;
  }

  return connIndex;
}

/*********************************************************************
* @fn      multi_role_doDiscoverDevices
*
* @brief   Respond to user input to start scanning
*
* @param   index - not used
*
* @return  TRUE since there is no callback to use this value
*/
bool multi_role_doDiscoverDevices(uint8_t index)
{
  (void) index;

#if (DEFAULT_DEV_DISC_BY_SVC_UUID == TRUE)
  // Scanning for DEFAULT_SCAN_DURATION x 10 ms.
  // The stack does not need to record advertising reports
  // since the application will filter them by Service UUID and save.

  // Reset number of scan results to 0 before starting scan
  numScanRes = 0;
  GapScan_enable(0, DEFAULT_SCAN_DURATION, 0);
#else // !DEFAULT_DEV_DISC_BY_SVC_UUID
  // Scanning for DEFAULT_SCAN_DURATION x 10 ms.
  // Let the stack record the advertising reports as many as up to DEFAULT_MAX_SCAN_RES.
  GapScan_enable(0, DEFAULT_SCAN_DURATION, DEFAULT_MAX_SCAN_RES);
#endif // DEFAULT_DEV_DISC_BY_SVC_UUID
  // Enable only "Stop Discovering" and disable all others in the main menu
  tbm_setItemStatus(&mrMenuMain, MR_ITEM_STOPDISC,
                    (MR_ITEM_ALL & ~MR_ITEM_STOPDISC));

  return (true);
}

/*********************************************************************
 * @fn      multi_role_doStopDiscovering
 *
 * @brief   Stop on-going scanning
 *
 * @param   index - item index from the menu
 *
 * @return  always true
 */
bool multi_role_doStopDiscovering(uint8_t index)
{
  (void) index;

  GapScan_disable();
  
  return (true);
}

/*********************************************************************
 * @fn      SimpleCentral_processCmdCompleteEvt
 *
 * @brief   Process an incoming OSAL HCI Command Complete Event.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void multi_role_processCmdCompleteEvt(hciEvt_CmdComplete_t *pMsg)
{
  uint8_t status = pMsg->pReturnParam[0];

  //Find which command this command complete is for
  switch (pMsg->cmdOpcode)
  {
    case HCI_READ_RSSI:
    {
      // Display RSSI value, if RSSI is higher than threshold, change to faster PHY
      if (status == SUCCESS)
      {
        uint16_t handle = BUILD_UINT16(pMsg->pReturnParam[1], pMsg->pReturnParam[2]);

        uint8_t index = multi_role_getConnIndex(handle);
        MULTIROLE_ASSERT(index < MAX_NUM_BLE_CONNS);

        connList[index].rssiArr[connList[index].rssiCntr++] =
                                                  (int8_t)pMsg->pReturnParam[3];
        connList[index].rssiCntr %= MR_MAX_RSSI_STORE_DEPTH;

        int16_t sum_rssi = 0;
        for(uint8_t cnt=0; cnt<MR_MAX_RSSI_STORE_DEPTH; cnt++)
        {
          sum_rssi += connList[index].rssiArr[cnt];
        }
        connList[index].rssiAvg = (uint32_t)(sum_rssi/MR_MAX_RSSI_STORE_DEPTH);

        Display_printf(dispHandle, MR_ROW_SEPARATOR, 0,
                       "RSSI:-%d, AVG RSSI:-%d",
                       (uint32_t)(-(int8_t)pMsg->pReturnParam[3]),
                       (uint32_t)(-sum_rssi/MR_MAX_RSSI_STORE_DEPTH));

        uint8_t phyRq = MR_PHY_NONE;
        uint8_t phyRqS = MR_PHY_NONE;
        uint8_t phyOpt = LL_PHY_OPT_NONE;

        if(connList[index].phyCngRq == FALSE)
        {
          if((connList[index].rssiAvg >= RSSI_2M_THRSHLD) &&
             (connList[index].currPhy != HCI_PHY_2_MBPS) &&
             (connList[index].currPhy != MR_PHY_NONE))
          {
            // try to go to higher data rate
            phyRqS = phyRq = HCI_PHY_2_MBPS;
          }
          else if((connList[index].rssiAvg < RSSI_2M_THRSHLD) &&
                  (connList[index].rssiAvg >= RSSI_1M_THRSHLD) &&
                  (connList[index].currPhy != HCI_PHY_1_MBPS) &&
                  (connList[index].currPhy != MR_PHY_NONE))
          {
            // try to go to legacy regular data rate
            phyRqS = phyRq = HCI_PHY_1_MBPS;
          }
          else if((connList[index].rssiAvg >= RSSI_S2_THRSHLD) &&
                  (connList[index].rssiAvg < RSSI_1M_THRSHLD) &&
                  (connList[index].currPhy != MR_PHY_NONE))
          {
            // try to go to lower data rate S=2(500kb/s)
            phyRqS = HCI_PHY_CODED;
            phyOpt = LL_PHY_OPT_S2;
            phyRq = BLE5_CODED_S2_PHY;
          }
          else if(connList[index].rssiAvg < RSSI_S2_THRSHLD )
          {
            // try to go to lowest data rate S=8(125kb/s)
            phyRqS = HCI_PHY_CODED;
            phyOpt = LL_PHY_OPT_S8;
            phyRq = BLE5_CODED_S8_PHY;
          }
          if((phyRq != MR_PHY_NONE) &&
             // First check if the request for this phy change is already not honored then don't request for change
             (((connList[index].rqPhy == phyRq) &&
               (connList[index].phyRqFailCnt < 2)) ||
              (connList[index].rqPhy != phyRq)))
          {
            //Initiate PHY change based on RSSI
            multi_role_setPhy(connList[index].connHandle, 0,
                                    phyRqS, phyRqS, phyOpt);
            connList[index].phyCngRq = TRUE;

            // If it a request for different phy than failed request, reset the count
            if(connList[index].rqPhy != phyRq)
            {
              // then reset the request phy counter and requested phy
              connList[index].phyRqFailCnt = 0;
            }

            if(phyOpt == LL_PHY_OPT_NONE)
            {
              connList[index].rqPhy = phyRq;
            }
            else if(phyOpt == LL_PHY_OPT_S2)
            {
              connList[index].rqPhy = BLE5_CODED_S2_PHY;
            }
            else
            {
              connList[index].rqPhy = BLE5_CODED_S8_PHY;
            }

          }
        } // end of if(connList[index].phyCngRq == FALSE)
      } // end of if (status == SUCCESS)
      break;
    }

    case HCI_LE_READ_PHY:
    {
      if (status == SUCCESS)
      {
        Display_printf(dispHandle, MR_ROW_SEPARATOR + 2, 0, "RXPh: %d, TXPh: %d",
                       pMsg->pReturnParam[3], pMsg->pReturnParam[4]);
      }
      break;
    }

    case HCI_LE_READ_LOCAL_RESOLVABLE_ADDRESS:
    {
      uint8_t* pRpaNew = &(pMsg->pReturnParam[1]);

      if (memcmp(pRpaNew, rpa, B_ADDR_LEN))
      {
        // If the RPA has changed, update the display
        Display_printf(dispHandle, MR_ROW_RPA, 0, "RP Addr: %s",
                       Util_convertBdAddr2Str(pRpaNew));
        memcpy(rpa, pRpaNew, B_ADDR_LEN);
      }
      break;
    }

    default:
      break;
  } // end of switch (pMsg->cmdOpcode)
}

/*********************************************************************
 * @fn      multi_role_doCancelConnecting
 *
 * @brief   Cancel on-going connection attempt
 *
 * @param   index - item index from the menu
 *
 * @return  always true
 */
bool multi_role_doCancelConnecting(uint8_t index)
{
  (void) index;

  GapInit_cancelConnect();

  return (true);
}

/*********************************************************************
* @fn      multi_role_doConnect
*
* @brief   Respond to user input to form a connection
*
* @param   index - index as selected from the mrMenuConnect
*
* @return  TRUE since there is no callback to use this value
*/
bool multi_role_doConnect(uint8_t index)
{
  // Temporarily disable advertising
  GapAdv_disable(advHandle);

#if (DEFAULT_DEV_DISC_BY_SVC_UUID == TRUE)
  GapInit_connect(scanList[index].addrType & MASK_ADDRTYPE_ID,
                  scanList[index].addr, mrInitPhy, 0);
#else // !DEFAULT_DEV_DISC_BY_SVC_UUID
  GapScan_Evt_AdvRpt_t advRpt;

  GapScan_getAdvReport(index, &advRpt);

  GapInit_connect(advRpt.addrType & MASK_ADDRTYPE_ID,
                  advRpt.addr, mrInitPhy, 0);
#endif // DEFAULT_DEV_DISC_BY_SVC_UUID

  // Re-enable advertising
  GapAdv_enable(advHandle, GAP_ADV_ENABLE_OPTIONS_USE_MAX , 0);

  // Enable only "Cancel Connecting" and disable all others in the main menu
  tbm_setItemStatus(&mrMenuMain, MR_ITEM_CANCELCONN,
                    (MR_ITEM_ALL & ~MR_ITEM_CANCELCONN));

  Display_printf(dispHandle, MR_ROW_NON_CONN, 0, "Connecting...");

  tbm_goTo(&mrMenuMain);

  return (true);
}

/*********************************************************************
 * @fn      multi_role_doSelectConn
 *
 * @brief   Select a connection to communicate with
 *
 * @param   index - item index from the menu
 *
 * @return  always true
 */
bool multi_role_doSelectConn(uint8_t index)
{
  uint32_t itemsToDisable = MR_ITEM_NONE;

  // index cannot be equal to or greater than MAX_NUM_BLE_CONNS
  MULTIROLE_ASSERT(index < MAX_NUM_BLE_CONNS);

  mrConnHandle  = connList[index].connHandle;

  if (connList[index].charHandle == 0)
  {
    // Initiate service discovery
    multi_role_enqueueMsg(MR_EVT_SVC_DISC, NULL);

    // Diable GATT Read/Write until simple service is found
    itemsToDisable = MR_ITEM_GATTREAD | MR_ITEM_GATTWRITE;
  }

  // Set the menu title and go to this connection's context
  TBM_SET_TITLE(&mrMenuPerConn, TBM_GET_ACTION_DESC(&mrMenuSelectConn, index));

  tbm_setItemStatus(&mrMenuPerConn, MR_ITEM_NONE, itemsToDisable);

  // Clear non-connection-related message
  Display_clearLine(dispHandle, MR_ROW_NON_CONN);

  tbm_goTo(&mrMenuPerConn);

  return (true);
}

/*********************************************************************
 * @fn      multi_role_doGattRead
 *
 * @brief   GATT Read
 *
 * @param   index - item index from the menu
 *
 * @return  always true
 */
bool multi_role_doGattRead(uint8_t index)
{
  attReadReq_t req;
  uint8_t connIndex = multi_role_getConnIndex(mrConnHandle);

  // connIndex cannot be equal to or greater than MAX_NUM_BLE_CONNS
  MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

  req.handle = connList[connIndex].charHandle;
  GATT_ReadCharValue(mrConnHandle, &req, selfEntity);

  return (true);
}

/*********************************************************************
 * @fn      multi_role_doGattWrite
 *
 * @brief   GATT Write
 *
 * @param   index - item index from the menu
 *
 * @return  always true
 */
bool multi_role_doGattWrite(uint8_t index)
{
  status_t status;
  uint8_t charVals[4] = { 0x00, 0x55, 0xAA, 0xFF }; // Should be consistent with
                                                    // those in scMenuGattWrite
  attWriteReq_t req;

  req.pValue = GATT_bm_alloc(mrConnHandle, ATT_WRITE_REQ, 1, NULL);

  if ( req.pValue != NULL )
  {
    uint8_t connIndex = multi_role_getConnIndex(mrConnHandle);

    // connIndex cannot be equal to or greater than MAX_NUM_BLE_CONNS
    MULTIROLE_ASSERT(connIndex < MAX_NUM_BLE_CONNS);

    req.handle = connList[connIndex].charHandle;
    req.len = 1;
    charVal = charVals[index];
    req.pValue[0] = charVal;
    req.sig = 0;
    req.cmd = 0;

    status = GATT_WriteCharValue(mrConnHandle, &req, selfEntity);
    if ( status != SUCCESS )
    {
      GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
    }
  }

  return (true);
}

/*********************************************************************
* @fn      multi_role_doConnUpdate
*
* @brief   Respond to user input to do a connection update
*
* @param   index - index as selected from the mrMenuConnUpdate
*
* @return  TRUE since there is no callback to use this value
*/
bool multi_role_doConnUpdate(uint8_t index)
{
  gapUpdateLinkParamReq_t params;

  (void) index; //may need to get the real connHandle?

  params.connectionHandle = mrConnHandle;
  params.intervalMin = DEFAULT_UPDATE_MIN_CONN_INTERVAL;
  params.intervalMax = DEFAULT_UPDATE_MAX_CONN_INTERVAL;
  params.connLatency = DEFAULT_UPDATE_SLAVE_LATENCY;

  linkDBInfo_t linkInfo;
  if (linkDB_GetInfo(mrConnHandle, &linkInfo) == SUCCESS)
  {
    if (linkInfo.connTimeout == DEFAULT_UPDATE_CONN_TIMEOUT)
    {
      params.connTimeout = DEFAULT_UPDATE_CONN_TIMEOUT + 200;
    }
    else
    {
      params.connTimeout = DEFAULT_UPDATE_CONN_TIMEOUT;
    }

    GAP_UpdateLinkParamReq(&params);

    Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Param update Request:connTimeout =%d",
                    params.connTimeout*CONN_TIMEOUT_MS_CONVERSION);
  }
  else
  {

    Display_printf(dispHandle, MR_ROW_CUR_CONN, 0,
                   "update :%s, Unable to find link information",
                   Util_convertBdAddr2Str(linkInfo.addr));
  }

  return (true);
}

/*********************************************************************
 * @fn      multi_role_setPhy
 *
 * @brief   Call the HCI set phy API and and add the handle to a
 *          list to match it to an incoming command status event
 */
static status_t multi_role_setPhy(uint16_t connHandle, uint8_t allPhys,
                                        uint8_t txPhy, uint8_t rxPhy,
                                        uint16_t phyOpts)
{
  // Allocate list entry to store handle for command status
  mrConnHandleEntry_t *connHandleEntry = ICall_malloc(sizeof(mrConnHandleEntry_t));

  if (connHandleEntry)
  {
    connHandleEntry->connHandle = connHandle;

    // Add entry to the phy command status list
    List_put(&setPhyCommStatList, (List_Elem *)connHandleEntry);

    // Send PHY Update
    HCI_LE_SetPhyCmd(connHandle, allPhys, txPhy, rxPhy, phyOpts);
  }

  return SUCCESS;
}

/*********************************************************************
 * @fn      multi_role_doConnPhy
 *
 * @brief   Set Connection PHY preference.
 *
 * @param   index - item number in MRMenu_connPhy list
 *
 * @return  always true
 */
bool multi_role_doConnPhy(uint8_t index)
{
  // Set Phy Preference on the current connection. Apply the same value
  // for RX and TX. For more information, see the LE 2M PHY section in the User's Guide:
  // http://software-dl.ti.com/lprf/ble5stack-latest/
  // Note PHYs are already enabled by default in build_config.opt in stack project.
  HCI_LE_SetPhyCmd(mrConnHandle, 0, MRMenu_connPhy[index].value, MRMenu_connPhy[index].value, 0);

  Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Connection PHY preference: %s",
                 TBM_GET_ACTION_DESC(&mrMenuConnPhy, index));

  return (true);
}

/*********************************************************************
 * @fn      multi_role_doSetInitPhy
 *
 * @brief   Set initialize PHY preference.
 *
 * @param   index - item number in MRMenu_initPhy list
 *
 * @return  always true
 */
bool multi_role_doSetInitPhy(uint8_t index)
{
  mrInitPhy = MRMenu_initPhy[index].value;
  Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Initialize PHY preference: %s",
                 TBM_GET_ACTION_DESC(&mrMenuInitPhy, index));

  return (true);
}

/*********************************************************************
 * @fn      multi_role_doSetScanPhy
 *
 * @brief   Set PHYs for scanning.
 *
 * @param   index - item number in MRMenu_scanPhy list
 *
 * @return  always true
 */
bool multi_role_doSetScanPhy(uint8_t index)
{
  // Set scanning primary PHY
  GapScan_setParam(SCAN_PARAM_PRIM_PHYS, &MRMenu_scanPhy[index].value);

  Display_printf(dispHandle, MR_ROW_NON_CONN, 0, "Primary Scan PHY: %s",
                 TBM_GET_ACTION_DESC(&mrMenuScanPhy, index));

  return (true);
}

/*********************************************************************
* @fn      multi_role_updatePHYStat
*
* @brief   Update the auto phy update state machine
*
* @param   connHandle - the connection handle
*
* @return  None
*/
static void multi_role_updatePHYStat(uint16_t eventCode, uint8_t *pMsg)
{
  uint8_t connIndex;

  switch (eventCode)
  {
    case HCI_LE_SET_PHY:
    {
      // Get connection handle from list
      mrConnHandleEntry_t *connHandleEntry =
                           (mrConnHandleEntry_t *)List_get(&setPhyCommStatList);

      if (connHandleEntry)
      {
        // Get index from connection handle
        connIndex = multi_role_getConnIndex(connHandleEntry->connHandle);

        ICall_free(connHandleEntry);

        // Is this connection still valid?
        if (connIndex < MAX_NUM_BLE_CONNS)
        {
          hciEvt_CommandStatus_t *pMyMsg = (hciEvt_CommandStatus_t *)pMsg;

          if (pMyMsg->cmdStatus == HCI_ERROR_CODE_UNSUPPORTED_REMOTE_FEATURE)
          {
            // Update the phychange request status for active RSSI tracking connection
            connList[connIndex].phyCngRq = FALSE;
            connList[connIndex].phyRqFailCnt++;
          }
        }
      }
      break;
    }

    // LE Event - a Phy update has completed or failed
    case HCI_BLE_PHY_UPDATE_COMPLETE_EVENT:
    {
      hciEvt_BLEPhyUpdateComplete_t *pPUC =
                                     (hciEvt_BLEPhyUpdateComplete_t*) pMsg;

      if(pPUC)
      {
        // Get index from connection handle
        connIndex = multi_role_getConnIndex(pPUC->connHandle);

        // Is this connection still valid?
        if (connIndex < MAX_NUM_BLE_CONNS)
        {
          // Update the phychange request status for active RSSI tracking connection
          connList[connIndex].phyCngRq = FALSE;

          if (pPUC->status == SUCCESS)
          {
            connList[connIndex].currPhy = pPUC->rxPhy;
          }
          if(pPUC->rxPhy != connList[connIndex].rqPhy)
          {
            connList[connIndex].phyRqFailCnt++;
          }
          else
          {
            // Reset the request phy counter and requested phy
            connList[connIndex].phyRqFailCnt = 0;
            connList[connIndex].rqPhy = 0;
          }
        }
      }

      break;
    }

    default:
      break;
  } // end of switch (eventCode)
}

/*********************************************************************
 * @fn      multi_role_doSetAdvPhy
 *
 * @brief   Set advertise PHY preference.
 *
 * @param   index - item number in MRMenu_advPhy list
 *
 * @return  always true
 */
bool multi_role_doSetAdvPhy(uint8_t index)
{
  uint16_t props;
  GapAdv_primaryPHY_t phy;
  bool isAdvActive = mrIsAdvertising;

  switch (MRMenu_advPhy[index].value)
  {
    case MR_ADV_LEGACY_PHY_1_MBPS:
        props = GAP_ADV_PROP_CONNECTABLE | GAP_ADV_PROP_SCANNABLE | GAP_ADV_PROP_LEGACY;
        phy = GAP_ADV_PRIM_PHY_1_MBPS;
    break;
    case MR_ADV_EXT_PHY_1_MBPS:
        props = GAP_ADV_PROP_CONNECTABLE;
        phy = GAP_ADV_PRIM_PHY_1_MBPS;
    break;
    case MR_ADV_EXT_PHY_CODED:
        props = GAP_ADV_PROP_CONNECTABLE;
        phy = GAP_ADV_PRIM_PHY_CODED_S2;
    break;
    default:
        return (false);
  }
  if (isAdvActive)
  {
    // Turn off advertising
    GapAdv_disable(advHandle);
  }
  GapAdv_setParam(advHandle,GAP_ADV_PARAM_PROPS,&props);
  GapAdv_setParam(advHandle,GAP_ADV_PARAM_PRIMARY_PHY,&phy);
  GapAdv_setParam(advHandle,GAP_ADV_PARAM_SECONDARY_PHY,&phy);
  if (isAdvActive)
  {
    // Turn on advertising
    GapAdv_enable(advHandle, GAP_ADV_ENABLE_OPTIONS_USE_MAX , 0);
  }

  Display_printf(dispHandle, MR_ROW_CUR_CONN, 0, "Advertise PHY preference: %s",
                 TBM_GET_ACTION_DESC(&mrMenuAdvPhy, index));

  return (true);
}

/*********************************************************************
* @fn      multi_role_doDisconnect
*
* @brief   Respond to user input to terminate a connection
*
* @param   index - index as selected from the mrMenuConnUpdate
*
* @return  always true
*/
bool multi_role_doDisconnect(uint8_t index)
{
  (void) index;

  // Disconnect
  GAP_TerminateLinkReq(mrConnHandle, HCI_DISCONNECT_REMOTE_USER_TERM);

  return (true);
}

/*********************************************************************
* @fn      multi_role_doAdvertise
*
* @brief   Respond to user input to terminate a connection
*
* @param   index - index as selected from the mrMenuConnUpdate
*
* @return  always true
*/
bool multi_role_doAdvertise(uint8_t index)
{
  (void) index;

  // If we're currently advertising
  if (mrIsAdvertising)
  {
    // Turn off advertising
    GapAdv_disable(advHandle);
  }
  // If we're not currently advertising
  else
  {
    if (numConn < MAX_NUM_BLE_CONNS)
    {
      // Start advertising since there is room for more connections
      GapAdv_enable(advHandle, GAP_ADV_ENABLE_OPTIONS_USE_MAX , 0);
    }
    else
    {
      Display_printf(dispHandle, MR_ROW_ADVERTIS, 0,
                     "At Maximum Connection Limit, Cannot Enable Advertisment");
    }
  }

  return (true);
}

/*********************************************************************
 * @fn      multi_role_menuSwitchCb
 *
 * @brief   Detect menu context switching
 *
 * @param   pMenuObjCurr - the current menu object
 * @param   pMenuObjNext - the menu object the context is about to switch to
 *
 * @return  none
 */
static void multi_role_menuSwitchCb(tbmMenuObj_t* pMenuObjCurr,
                                    tbmMenuObj_t* pMenuObjNext)
{
  // interested in only the events of
  // entering mrMenuConnect, mrMenuSelectConn, and mrMenuMain for now
  if (pMenuObjNext == &mrMenuConnect)
  {
    uint8_t i, j;
    uint32_t itemsToDisable = MR_ITEM_NONE;

    for (i = 0; i < TBM_GET_NUM_ITEM(&mrMenuConnect); i++)
    {
      for (j = 0; j < MAX_NUM_BLE_CONNS; j++)
      {
        if ((connList[j].connHandle != LINKDB_CONNHANDLE_INVALID) &&
            !memcmp(TBM_GET_ACTION_DESC(&mrMenuConnect, i),
                    Util_convertBdAddr2Str(connList[j].addr),
                    MR_ADDR_STR_SIZE))
        {
          // Already connected. Add to the set to be disabled.
          itemsToDisable |= (1 << i);
        }
      }
    }

    // Eventually only non-connected device addresses will be displayed.
    tbm_setItemStatus(&mrMenuConnect,
                      MR_ITEM_ALL & ~itemsToDisable, itemsToDisable);
  }
  else if (pMenuObjNext == &mrMenuSelectConn)
  {
    static uint8_t* pAddrs;
    uint8_t* pAddrTemp;

    if (pAddrs != NULL)
    {
      ICall_free(pAddrs);
    }

    // Allocate buffer to display addresses
    pAddrs = ICall_malloc(numConn * MR_ADDR_STR_SIZE);

    if (pAddrs == NULL)
    {
      TBM_SET_NUM_ITEM(&mrMenuSelectConn, 0);
    }
    else
    {
      uint8_t i;

      TBM_SET_NUM_ITEM(&mrMenuSelectConn, MAX_NUM_BLE_CONNS);

      pAddrTemp = pAddrs;

      // Add active connection info to the menu object
      for (i = 0; i < MAX_NUM_BLE_CONNS; i++)
      {
        if (connList[i].connHandle != LINKDB_CONNHANDLE_INVALID)
        {
          // This connection is active. Set the corresponding menu item with
          // the address of this connection and enable the item.
          memcpy(pAddrTemp, Util_convertBdAddr2Str(connList[i].addr),
                 MR_ADDR_STR_SIZE);
          TBM_SET_ACTION_DESC(&mrMenuSelectConn, i, pAddrTemp);
          tbm_setItemStatus(&mrMenuSelectConn, (1 << i), MR_ITEM_NONE);
          pAddrTemp += MR_ADDR_STR_SIZE;
        }
        else
        {
          // This connection is not active. Disable the corresponding menu item.
          tbm_setItemStatus(&mrMenuSelectConn, MR_ITEM_NONE, (1 << i));
        }
      }
    }
  }
  else if (pMenuObjNext == &mrMenuMain)
  {
    // Now we are not in a specific connection's context
    mrConnHandle = LINKDB_CONNHANDLE_INVALID;

    // Clear connection-related message
    Display_clearLine(dispHandle, MR_ROW_CUR_CONN);
  }
}

/*********************************************************************
 * @fn      multi_role_processOadResetEvt
 *
 * @brief   Process a write request to the OAD reset service
 *
 * @param   resetEvt - The oadResetWrite_t struct containing reset data
 *
 * @return  None.
 */
static void multi_role_processOadResetEvt(oadResetWrite_t *resetEvt)
{
  /* We cannot reboot the device immediately after receiving
   * the enable command, we must allow the stack enough time
   * to process and responsd to the OAD_EXT_CTRL_ENABLE_IMG
   * command. The current implementation will wait one cxn evt
   */
  // Register for L2CAP Flow Control Events
  L2CAP_RegisterFlowCtrlTask(selfEntity);

  resetConnHandle = resetEvt->connHandle;

  uint8_t status = FLASH_FAILURE;
  //read the image validation bytes and set it appropriately.
  imgHdr_t imgHdr = {0};
  if(flash_open())
  {
    status = readFlash(0x0, (uint8_t *)&imgHdr, OAD_IMG_HDR_LEN);
  }

  if ((FLASH_SUCCESS == status) && ( imgHdr.fixedHdr.imgVld != 0))
  {
    if ( OAD_evenBitCount(imgHdr.fixedHdr.imgVld) )
    {
      imgHdr.fixedHdr.imgVld = imgHdr.fixedHdr.imgVld << 1;
      writeFlash((uint32_t)FLASH_ADDRESS(0, IMG_VALIDATION_OFFSET),
                 (uint8_t *)&(imgHdr.fixedHdr.imgVld), sizeof(imgHdr.fixedHdr.imgVld));
    }
  }
}

/*********************************************************************
 * @fn      multi_role_processOadResetWriteCB
 *
 * @brief   Process a write request to the OAD reset service
 *
 * @param   connHandle - the connection Handle this request is from.
 * @param   bim_var    - bim_var to set before resetting.
 *
 * @return  None.
 */
void multi_role_processOadResetWriteCB(uint16_t connHandle,
                                      uint16_t bim_var)
{
  // Allocate memory for OAD EVT payload, the app task must free this later
  oadResetWrite_t *oadResetWriteEvt = ICall_malloc(sizeof(oadResetWrite_t));

  oadResetWriteEvt->connHandle = connHandle;
  oadResetWriteEvt->bim_var = bim_var;

  // This function will enqueue the messsage and wake the application
  multi_role_enqueueMsg(MR_OAD_RESET_EVT, (uint8_t *)oadResetWriteEvt);
}
