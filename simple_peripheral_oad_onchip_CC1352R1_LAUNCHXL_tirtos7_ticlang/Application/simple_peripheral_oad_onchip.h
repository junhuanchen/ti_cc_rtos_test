/******************************************************************************
 * @file  multi_role.h
 *
 * @description Definitions and prototypes for the multi_role example
 *
 *
 Copyright (c) 2013-2016, Texas Instruments Incorporated
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
 *
 *****************************************************************************/

#ifndef MULTIROLE_H
#define MULTIROLE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
*  EXTERNAL VARIABLES
*/

/*********************************************************************
 * CONSTANTS
 */

// Maximum number of scan responses.
// Note: this value cannot be greater than the number of items reserved in
// mrMenuConnect (See multi_role_menu.c)
// This cannot exceed 27 (two-button menu's constraint)
#define DEFAULT_MAX_SCAN_RES                  15

// advertising PHY menu items
#define MR_ADV_LEGACY_PHY_1_MBPS    0
#define MR_ADV_EXT_PHY_1_MBPS       1
#define MR_ADV_EXT_PHY_CODED        2
  
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * FUNCTIONS
 */
/*
 * Task creation function for the Simple Peripheral.
 */
extern void multi_role_createTask(void);

/* Action for Menu: Enable Scanning */
bool multi_role_doDiscoverDevices(uint8_t index);

/* Action for Menu: Disable Scanning */
bool multi_role_doStopDiscovering(uint8_t index);

/* Actions for Menu: Init - Connect */
bool multi_role_doConnect(uint8_t index);

/* Action for Menu: Cancel Connecting */
bool multi_role_doCancelConnecting(uint8_t index);

/* Action for Menu: Select Connection */
bool multi_role_doSelectConn(uint8_t index);

bool multi_role_doAdvertise(uint8_t index);

/* Action for Menu: GATT Read */
bool multi_role_doGattRead(uint8_t index);

/* Action for Menu: GATT Write */
bool multi_role_doGattWrite(uint8_t index);

/* Actions for Menu: Init - Conn Update */
bool multi_role_doConnUpdate(uint8_t index);

/* Actions for Menu: Init - Disconnect */
bool multi_role_doDisconnect(uint8_t index);

/* Actions for Menu: Init - Advertise */
bool mr_doAdvertise(uint8_t index);

/* Action for Menu: Set Scanning PHY */
bool multi_role_doSetScanPhy(uint8_t index);

/* Action for Menu: Set Advertising PHY */
bool multi_role_doSetAdvPhy(uint8_t index);

/* Action for Menu: Set Initialize PHY */
bool multi_role_doSetInitPhy(uint8_t index);

/* Action for Menu: Set Connection PHY */
bool multi_role_doConnPhy(uint8_t index);

uint16_t multi_role_getConnIndex(uint16_t connHandle);

/*********************************************************************
*********************************************************************/

#include "ti_ble_config.h"
#include "gap_scanner.h"
#include "util.h"
#include "att.h"
#include "gatt.h"

// Discovery states
typedef enum {
  BLE_DISC_STATE_IDLE,                // Idle
  BLE_DISC_STATE_MTU,                 // Exchange ATT MTU size
  BLE_DISC_STATE_SVC,                 // Service discovery
  BLE_DISC_STATE_CHAR                 // Characteristic discovery
} discState_t;

// Row numbers for two-button menu
#define MR_ROW_SEPARATOR     (TBM_ROW_APP + 0)
#define MR_ROW_CUR_CONN      (TBM_ROW_APP + 1)
#define MR_ROW_ANY_CONN      (TBM_ROW_APP + 2)
#define MR_ROW_NON_CONN      (TBM_ROW_APP + 3)
#define MR_ROW_NUM_CONN      (TBM_ROW_APP + 4)
#define MR_ROW_CHARSTAT      (TBM_ROW_APP + 5)
#define MR_ROW_ADVERTIS      (TBM_ROW_APP + 6)
#define MR_ROW_MYADDRSS      (TBM_ROW_APP + 7)
#define MR_ROW_SECURITY      (TBM_ROW_APP + 8)
#define MR_ROW_RPA           (TBM_ROW_APP + 9)

#define CONNINDEX_INVALID  0xFF

// For storing the active connections
#define MR_RSSI_TRACK_CHNLS        1            // Max possible channels can be GAP_BONDINGS_MAX
#define MR_MAX_RSSI_STORE_DEPTH    5
#define MR_INVALID_HANDLE          0xFFFF
#define RSSI_2M_THRSHLD           -30           // -80 dB rssi
#define RSSI_1M_THRSHLD           -40           // -90 dB rssi
#define RSSI_S2_THRSHLD           -50           // -100 dB rssi
#define RSSI_S8_THRSHLD           -60           // -120 dB rssi
#define MR_PHY_NONE                LL_PHY_NONE  // No PHY set
#define AUTO_PHY_UPDATE            0xFF

/*********************************************************************
* TYPEDEFS
*/

// App event passed from profiles.
typedef struct
{
  uint8_t event;    // event type
  void *pData;   // event data pointer
} mrEvt_t;

// Container to store paring state info when passing from gapbondmgr callback
// to app event. See the pfnPairStateCB_t documentation from the gapbondmgr.h
// header file for more information on each parameter.
typedef struct
{
  uint8_t state;
  uint16_t connHandle;
  uint8_t status;
} mrPairStateData_t;

// Container to store passcode data when passing from gapbondmgr callback
// to app event. See the pfnPasscodeCB_t documentation from the gapbondmgr.h
// header file for more information on each parameter.
typedef struct
{
  uint8_t deviceAddr[B_ADDR_LEN];
  uint16_t connHandle;
  uint8_t uiInputs;
  uint8_t uiOutputs;
  uint32_t numComparison;
} mrPasscodeData_t;

// Scanned device information record
typedef struct
{
  uint8_t addrType;         // Peer Device's Address Type
  uint8_t addr[B_ADDR_LEN]; // Peer Device Address
} scanRec_t;

// Container to store information from clock expiration using a flexible array
// since data is not always needed
typedef struct
{
  uint8_t event;
  uint8_t data[];
} mrClockEventData_t;

// Container to store advertising event data when passing from advertising
// callback to app event. See the respective event in GapAdvScan_Event_IDs
// in gap_advertiser.h for the type that pBuf should be cast to.
typedef struct
{
  uint32_t event;
  void *pBuf;
} mrGapAdvEventData_t;

// List element for parameter update and PHY command status lists
typedef struct
{
  List_Elem elem;
  uint16_t  connHandle;
} mrConnHandleEntry_t;

// Connected device information
typedef struct
{
  uint16_t              connHandle;           // Connection Handle
  mrClockEventData_t*   pParamUpdateEventData;// pointer to paramUpdateEventData
  uint16_t              charHandle;           // Characteristic Handle
  uint8_t               addr[B_ADDR_LEN];     // Peer Device Address
  Clock_Struct*         pUpdateClock;         // pointer to clock struct
  uint8_t               discState;            // Per connection deiscovery state
  uint8_t               discExist;            // Per connection deiscovery state
  int8_t                rssiArr[MR_MAX_RSSI_STORE_DEPTH];
  uint8_t               rssiCntr;
  int8_t                rssiAvg;
  bool                  phyCngRq;                          // Set to true if PHY change request is in progress
  uint8_t               currPhy;
  uint8_t               rqPhy;
  uint8_t               phyRqFailCnt;                      // PHY change request count
  bool                  isAutoPHYEnable;                   // Flag to indicate auto phy change

} mrConnRec_t;

#ifdef __cplusplus
}
#endif

#endif /* MULTIROLE_H */
