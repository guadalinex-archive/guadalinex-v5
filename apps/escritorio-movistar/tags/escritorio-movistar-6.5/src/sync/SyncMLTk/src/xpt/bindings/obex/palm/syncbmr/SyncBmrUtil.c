/*************************************************************************/
/* module:                                                               */
/* file:                                                                 */
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

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#ifdef USE_SDK_35
 #include <PalmOS.h>
#else
 #include <Pilot.h>
#endif

#include <ErrorMgr.h>
#include <ExgMgr.h>
#include <StringMgr.h>
#include <SysEvtMgr.h>

#include "SyncBmr.h"
#include "SyncBmrUtil.h"


/***********************************************************************
 *                                                                     *
 * FUNCTION:    sbuFindSyncBmr                                         *
 *                                                                     *
 * DESCRIPTION: Verifies installation of SyncBmr.  SyncBmr is required *
 *              for successful usage of the PalmOS OBEX transport.     *
 *                                                                     *
 * PARAMETERS:                                                         *
 *                                                                     *
 *    Output:  theCardNum - the memory card where SyncBmr resides      *
 *             theDbID    - the unique database id of SyncBmr          *
 *                                                                     *
 * RETURNED:    nothing                                                *
 *                                                                     *
 ***********************************************************************/

Err sbuFindSyncBmr( UInt *theCardNum, LocalID *theDbID ) {

   UInt              cardNo;
   LocalID           dbID;
   DmSearchStateType searchState;

   DmGetNextDatabaseByTypeCreator( true, &searchState, sysFileTApplication,
                                   SYNC_BMR_ID, true, &cardNo, &dbID );

   ErrFatalDisplayIf( !dbID, "required component missing" );

   *theCardNum = cardNo;  // return the card number
   *theDbID    = dbID;    // return the database id

   return 0;

} // sbuFindSyncBmr()


/***********************************************************************
 *                                                                     *
 * FUNCTION:    sbuSendRequest                                         *
 *                                                                     *
 * DESCRIPTION: Sends a document and waits for a response.             *
 *                                                                     *
 * PARAMETERS:                                                         *
 *                                                                     *
 *    Input:  reqP  - information regarding the document to send       *
 *                                                                     *
 *    Output: respP - information regarding the document received      *
 *                                                                     *
 * RETURNED:                                                           *
 *                                                                     *
 ***********************************************************************/

Err sbuSendRequest( SyncBmrRequest_t **reqP, SyncBmrResponse_t **respP ) {

   UInt    cardNo;
   LocalID dbID;
   DWord   result;
   Err     error;

   SyncBmrData_t *dataP;

   sbuFindSyncBmr( &cardNo, &dbID );   // find SyncBmr

   // Allocate memory for the SyncBmr comm block
   dataP = (SyncBmrData_t *)MEM_ALLOC( sizeof( SyncBmrData_t ) );
   ErrFatalDisplayIf( !dataP, "memory allocation" );

   MemPtrSetOwner( dataP, 0 );              // set owner of storage to the system

   dataP->waitForResponse = true;           // indicate a response is expected
   dataP->requestP        = (void *)*reqP;  // indicate the document to send
   dataP->responseP       = NULL;           // indicate no document received yet

   // Cause SyncBmr to be programmatically launched
   error = SysAppLaunch( cardNo, dbID,
                         sysAppLaunchFlagNewGlobals,
                         sysAppLaunchSyncBmrSend,
                         (void *)dataP, &result );

   *respP = dataP->responseP;               // return document received

   MEM_FREE( dataP );                       // free SyncBmr comm block

   return 0;

} // sbuSendRequest()


/***********************************************************************
 *                                                                     *
 * FUNCTION:    sbuSendResponse                                        *
 *                                                                     *
 * DESCRIPTION: Send a document with no response expected              *
 *                                                                     *
 * PARAMETERS:                                                         *
 *                                                                     *
 *    Input: respP - information regarding document to send            *
 *                                                                     *
 * RETURNED:                                                           *
 *                                                                     *
 ***********************************************************************/

Err sbuSendResponse( SyncBmrResponse_t **respP ) {

   UInt    cardNo;
   LocalID dbID;
   DWord   result;
   Err     error;

   SyncBmrData_t *dataP;

   sbuFindSyncBmr( &cardNo, &dbID );         // find SyncBmr

   // Allocate memory for SyncBmr com block
   dataP = (SyncBmrData_t *)MEM_ALLOC( sizeof( SyncBmrData_t ) );
   ErrFatalDisplayIf( !dataP, "memory allocation" );

   MemPtrSetOwner( dataP, 0 );               // set owner of storage to the system

   dataP->waitForResponse = false;           // indicate no response expected
   dataP->requestP        = (void *)*respP;  // indicate document to send
   dataP->responseP       = NULL;            // indicate no document received

   // Cause SyncBmr to be programmatically launched
   error = SysAppLaunch( cardNo, dbID,
                         sysAppLaunchFlagNewGlobals,
                         sysAppLaunchSyncBmrSend,
                         (void *)dataP, &result );

   MEM_FREE( dataP );                        // free SyncBmr comm block

   return 0;

} // sbuSendResponse()


/***********************************************************************
 *                                                                     *
 * FUNCTION:    sbuReceiveResponse                                     *
 *                                                                     *
 * DESCRIPTION: Wait for a document to be received                     *
 *                                                                     *
 * PARAMETERS:                                                         *
 *                                                                     *
 *    Output: respP - information regarding document received          *
 *                                                                     *
 * RETURNED:                                                           *
 *                                                                     *
 ***********************************************************************/

Err sbuReceiveResponse( SyncBmrResponse_t **respP ) {

   UInt    cardNo;
   LocalID dbID;
   DWord   result;
   Err     error;

   SyncBmrData_t *dataP;

   sbuFindSyncBmr( &cardNo, &dbID );  // find SyncBmr

   // Allocate memory for SyncBmr comm block
   dataP = (SyncBmrData_t *)MEM_ALLOC( sizeof( SyncBmrData_t ) );
   ErrFatalDisplayIf( !dataP, "memory allocation" );

   MemPtrSetOwner( dataP, 0 );        // set owner of storage to system

   dataP->waitForResponse = true;     // indicate a response is expected
   dataP->requestP        = NULL;     // indicate no document to send
   dataP->responseP       = NULL;     // indicate no document received yet

   // Cause SyncBmr to be programmatically launched
   error = SysAppLaunch( cardNo, dbID,
                         sysAppLaunchFlagNewGlobals,
                         sysAppLaunchSyncBmrReceive,
                         (void *)dataP, &result );

   *respP = (SyncBmrResponse_t *)dataP->responseP;  // return received document

   MEM_FREE( dataP );                               // free SyncBmr comm block

   return 0;

} // sbuSendResponse()

