/*************************************************************************/
/* module:                                                               */
/* file:            SyncBmr.h                                            */
/* target system:   Palm                                                 */
/* target OS:       PalmOS 3.0                                           */
/*************************************************************************/

/*
 * Copyright Notice
 * Copyright (c) Ericsson, IBM, Lotus, Matsushita Communication 
 * Industrial Co., Ltd., Motorola, Nokia, Openwave Systems, Inc., 
 * Palm, Inc., Psion, Starfish Software, Symbian, Ltd. (2001).
 * All Rights Reserved.
 * Implementation of all or part of any Specification may require 
 * licenses under third party intellectual property rights, 
 * including without limitation, patent rights (such a third party 
 * may or may not be a Supporter). The Sponsors of the Specification 
 * are not responsible and shall not be held responsible in any 
 * manner for identifying or failing to identify any or all such 
 * third party intellectual property rights.
 * 
 * THIS DOCUMENT AND THE INFORMATION CONTAINED HEREIN ARE PROVIDED 
 * ON AN "AS IS" BASIS WITHOUT WARRANTY OF ANY KIND AND ERICSSON, IBM, 
 * LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO. LTD, MOTOROLA, 
 * NOKIA, PALM INC., PSION, STARFISH SOFTWARE AND ALL OTHER SYNCML 
 * SPONSORS DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION 
 * HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF 
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
 * SHALL ERICSSON, IBM, LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO., 
 * LTD, MOTOROLA, NOKIA, PALM INC., PSION, STARFISH SOFTWARE OR ANY 
 * OTHER SYNCML SPONSOR BE LIABLE TO ANY PARTY FOR ANY LOSS OF 
 * PROFITS, LOSS OF BUSINESS, LOSS OF USE OF DATA, INTERRUPTION OF 
 * BUSINESS, OR FOR DIRECT, INDIRECT, SPECIAL OR EXEMPLARY, INCIDENTAL, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES OF ANY KIND IN CONNECTION WITH 
 * THIS DOCUMENT OR THE INFORMATION CONTAINED HEREIN, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH LOSS OR DAMAGE.
 * 
 * The above notice and this paragraph must be included on all copies 
 * of this document that are made.
 * 
 */

#ifndef SYNCBMR_H
#define SYNCBMR_H

#ifndef PILOT_PRECOMPILED_HEADERS_OFF
#define PILOT_PRECOMPILED_HEADERS_OFF
#endif

#include <ExgMgr.h>

#ifdef USE_SDK_35
 #include <PalmCompatibility.h>  /* For VoidHand */
#endif

#include "xpt.h"
#include "xptTransport.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
 *                                                                     *
 * Resource File Defines                                               *
 *                                                                     *
 ***********************************************************************/

#define ErrorAlert 1000
#define ErrorOK    0

/***********************************************************************
 *                                                                     *
 * Defines                                                             *
 *                                                                     *
 ***********************************************************************/

#define SYNC_BMR_ID 'SYML'                 // unique application id - Registered at www.palm.com

#define sysAppLaunchSyncBmrSend    0xFFFF  // launch code for data send
#define sysAppLaunchSyncBmrReceive 0xFFFE  // launch code for data receive

#define SyncBmrError               0xD000
#define SyncBmrErrorDataPtr        0xD002
#define SyncBmrErrorRequestPtr     0xD004
#define SyncBmrErrorResponsePtr    0xD008
#define SyncBmrErrorExgMgr         0xD010

/***********************************************************************
 *                                                                     *
 * Request and Response object                                         *
 *                                                                     *
 ***********************************************************************/

typedef struct {

   unsigned long  uniqueID;     // unique id

   unsigned long  sourceID;     // creatorID of source app
   unsigned long  targetID;     // creatorID of target app

   char           name[XPT_DOC_NAME_SIZE];  // document name, typically a filename and extension
   char           type[XPT_DOC_TYPE_SIZE];  // document MIME type

   void          *data;         // document pointer
   unsigned long  length;       // document length

} SyncBmrRequest_t, SyncBmrResponse_t;


/***********************************************************************
 *                                                                     *
 * Communication object                                                *
 *                                                                     *
 ***********************************************************************/

typedef struct {
   Boolean waitForResponse;  // is a response expected?
   void    *requestP;        // pointer to request  block (SyncBmrRequest_t)
   void    *responseP;       // pointer to response block (SyncBmrResponse_t)
} SyncBmrData_t;


/***********************************************************************
 *                                                                     *
 * Received document object                                            *
 *                                                                     *
 ***********************************************************************/

typedef struct {
   VoidHand dataH;     // pointer to document body received
   void    *docP;      // pointer to document info received
} SyncBmrEventData_t;

/***********************************************************************
 *                                                                     *
 *   Function Prototypes                                               *
 *                                                                     *
 ***********************************************************************/

static Boolean AppHandleEvent( EventPtr event );
static Err     ReceiveData( ExgSocketPtr exgSocketP );
static Err     RomVersionCompatible( DWord requiredVersion, Word launchFlags );
static Err     StartApplication( void );
static Err     StopApplication( void );
static Err     SyncBmrSendData( SyncBmrData_t *dataP );
static Err     SyncBmrReceiveData( SyncBmrData_t *dataP );
static void    AppEventLoop( void );


#ifdef __cplusplus
}
#endif

#endif
