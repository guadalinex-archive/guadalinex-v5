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

#include <sys_types.h>          /* For size_t */

#include <ExgMgr.h>
#include <StringMgr.h>
#include <SysEvtMgr.h>

#include "SyncBmr.h"

#include "obexbinding.h"

/***********************************************************************
 *                                                                     *
 *   Internal Constants                                                *
 *                                                                     *
 ***********************************************************************/

#define Version30  0x03000000   // PalmOS 3.0 required to run SyncBmr

#define ChunkSize  128          // default block size for Receive


/***********************************************************************
 *                                                                     *
 *   Global Variables                                                  *
 *                                                                     *
 ***********************************************************************/

Boolean globalWaiting = false;
void   *globalDataP   = NULL;

char globalTest[] = "Testing...";


/***********************************************************************
 *                                                                     *
 * FUNCTION:    RomVersionCompatible                                   *
 *                                                                     *
 * DESCRIPTION: Check that the ROM version meets your minimum          *
 *              requirement.  Warn if the app was switched to.         *
 *                                                                     *
 * PARAMETERS:  requiredVersion - minimum rom version required         *
 *                                (see sysFtrNumROMVersion in          *
 *                                SystemMgr.h for format)              *
 *              launchFlags - flags indicating how the application     *
 *                            was launched.  A warning is displayed    *
 *                            only if these flags indicate that the    *
 *                            app is launched normally.                *
 *                                                                     *
 * RETURNED:    zero if rom is compatible else an error code           *
 *                                                                     *
 ***********************************************************************/

static Err RomVersionCompatible( DWord requiredVersion, Word launchFlags ) {

   DWord romVersion;

   // See if we're on in minimum required version of the ROM or later.
   // The system records the version number in a feature.  A feature is a
   // piece of information which can be looked up by a creator and feature
   // number.
   FtrGet( sysFtrCreator, sysFtrNumROMVersion, &romVersion );
   if ( romVersion < requiredVersion ) {
      // If the user launched the app from the launcher, explain
      // why the app shouldn't run.  If the app was contacted for something
      // else, like it was asked to find a string by the system find, then
      // don't bother the user with a warning dialog.  These flags tell how
      // the app was launched to decided if a warning should be displayed.
      if ( ( launchFlags & ( sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp ) ) ==
           ( sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp ) ) {
         /************************************/
         /***** Notify ROM Incompatible! *****/
         /************************************/
         // Pilot 1.0 will continuously relaunch this app unless we switch to
         // another safe one.  The sysFileCDefaultApp is considered "safe".
         if ( romVersion < 0x02000000 )
            AppLaunchWithCommand( sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL );
      } // end if
      return( sysErrRomIncompatible );
   } // end if

   return 0;

} // RomVersionCompatible()


/***********************************************************************
 *                                                                     *
 * FUNCTION:     StartApplication                                      *
 *                                                                     *
 * DESCRIPTION:  Set up the initial state of the application.          *
 *                                                                     *
 * PARAMETERS:   None.                                                 *
 *                                                                     *
 * RETURNED:     Error Code.  Zero indicates no error.                 *
 *                                                                     *
 ***********************************************************************/

static Err StartApplication( void ) {

   globalWaiting = false;
   globalDataP   = NULL;

// ExgRegisterData( SYNC_BMR_ID, exgRegExtensionID, NULL );

   return 0;

} // StartApplication()


/***********************************************************************
 *                                                                     *
 * FUNCTION:     StopApplication                                       *
 *                                                                     *
 * DESCRIPTION:                                                        *
 *                                                                     *
 * PARAMETERS:   None.                                                 *
 *                                                                     *
 * RETURNED:     Error Code.  Zero indicates no error.                 *
 *                                                                     *
 ***********************************************************************/

static Err StopApplication( void ) {

   globalWaiting = false;
   globalDataP   = NULL;

// ExgRegisterData( SYNC_BMR_ID, exgRegExtensionID, NULL );

   return 0;

} // StopApplication()


/***********************************************************************
 *                                                                     *
 * FUNCTION:                                                           *
 *                                                                     *
 * DESCRIPTION:                                                        *
 *                                                                     *
 * PARAMETERS:                                                         *
 *                                                                     *
 * RETURNED:                                                           *
 *                                                                     *
 ***********************************************************************/

static Err SyncBmrSendData( SyncBmrData_t *dataP ) {

   ExgSocketType exgSocket;

   SyncBmrRequest_t *reqP;

   unsigned long bytesSent = 0;

   Err errPut = 0, errSend = 0, errDisc = 0, errApp = 0;

   if ( !dataP ) return SyncBmrErrorDataPtr;

   reqP = dataP->requestP;

   if ( !reqP ) return SyncBmrErrorRequestPtr;

   // Important to init structure to zeros...
   MemSet( &exgSocket, sizeof( exgSocket ), 0 );

   exgSocket.target      = SYNC_BMR_ID;
   exgSocket.name        = reqP->name;
   exgSocket.description = reqP->type;
   exgSocket.type        = reqP->type;
   exgSocket.length      = reqP->length;

   errPut = ExgPut( &exgSocket );   // put data to destination

   if ( !errPut ) {
      bytesSent = ExgSend( &exgSocket, reqP->data, reqP->length, &errSend );
      errDisc   = ExgDisconnect( &exgSocket, errApp );
   } // end if

   return( errPut + errSend + errDisc + errApp ) ? SyncBmrErrorExgMgr : 0;

} // SyncBmrSendData()


/***********************************************************************
 *                                                                     *
 * FUNCTION:                                                           *
 *                                                                     *
 * DESCRIPTION:                                                        *
 *                                                                     *
 * PARAMETERS:                                                         *
 *                                                                     *
 * RETURNED:                                                           *
 *                                                                     *
 ***********************************************************************/

static Err SyncBmrReceiveData( SyncBmrData_t *dataP ) {

   globalDataP   = dataP;  // save until response received
   globalWaiting = true;   // indicate waiting for response
   AppEventLoop();         // wait until response received
   globalWaiting = false;  // indicate waiting for response

   return 0;

} // SyncBmrSendResponse()


/***********************************************************************
 *                                                                     *
 * FUNCTION:     ReceiveData                                           *
 *                                                                     *
 * DESCRIPTION:  Receives data using the Exg API                       *
 *                                                                     *
 * PARAMETERS:   exgSocketP, socket from the app code                  *
 *                           sysAppLaunchCmdExgReceiveData             *
 *                                                                     *
 * RETURNED:     error code or zero for no error.                      *
 *                                                                     *
 ***********************************************************************/

static Err ReceiveData( ExgSocketPtr exgSocketP ) {

   EventType newEvent;

   VoidHand dataH;
   char    *dataP;

   XptCommunicationInfo_t *docP;

   SyncBmrEventData_t     *eventDataP;

   unsigned long dataSize      = 0;
   unsigned long bufferSize    = 0;
   unsigned long bytesReceived = 0;

   Err error = 0;

   docP = (XptCommunicationInfo_t *)MEM_ALLOC( sizeof( XptCommunicationInfo_t ) );
   ErrFatalDisplayIf( !docP, "memory allocation" );

   // This block needs to belong to the system since it could be disposed when apps switch
   MemPtrSetOwner( docP, 0 );

   docP->cbLength = exgSocketP->length;

   STR_COPY( docP->docName,  exgSocketP->name );
   STR_COPY( docP->mimeType, exgSocketP->description );

   if ( exgSocketP->length )
      bufferSize = exgSocketP->length;  // if set, assume it's the max size
   else
      bufferSize = ChunkSize;           // allocate in pre-deteremined chunks

   // could use data size from header if available
   dataH = MemHandleNew( bufferSize );
   ErrFatalDisplayIf( !dataH, "memory allocation" );

   // This block needs to belong to the system since it could be disposed when apps switch
   MemHandleSetOwner( dataH, 0 );

   // accept will open a progress dialog and wait for your receive commands
   error = ExgAccept( exgSocketP );

   if ( !error ) {
      dataP = MemHandleLock( dataH );
      do {
         bytesReceived = ExgReceive( exgSocketP, &dataP[ dataSize ], bufferSize - dataSize, &error );
         if ( ( bytesReceived > 0 ) && !error ) {
            dataSize += bytesReceived;
            // resize block when we reach the limit of this one...
            if ( dataSize >= bufferSize ) {  // Note: dataSize > bufferSize is an error!
               MemHandleUnlock( dataH );
               error = MemHandleResize( dataH, bufferSize + ChunkSize );
               dataP = MemHandleLock( dataH );
               if ( !error ) bufferSize += ChunkSize;
            } // end if
         } // end if
      } while ( !error && ( bytesReceived > 0 ) );     // reading 0 bytes means EOF

      MemHandleUnlock( dataH );
      error = MemHandleResize( dataH, dataSize + 1 );  // shrink data buffer, if needed

      ExgDisconnect( exgSocketP, error );              // closes transfer dialog

      docP->cbLength = dataSize;                       // update length field with bytes received

   } // end if

   if ( error ) {
      // MHB: Handle error case
      ErrFatalDisplayIf( true, "SyncBmr Error..." );
   } // end else

   newEvent.eType = firstUserEvent;        // fire off a user event

   eventDataP = (SyncBmrEventData_t *)&newEvent.data;  // pointer to event data area

   eventDataP->dataH = dataH;              // save handle to document body
   eventDataP->docP  = docP;               // save pointer to document info

   EvtAddEventToQueue( &newEvent );        // add to event queue to signal end of processing

   return error;

} // ReceiveData()


/***********************************************************************
 *                                                                     *
 * FUNCTION:     AppHandleEvent                                        *
 *                                                                     *
 * DESCRIPTION:  Handles processing of events for the application.     *
 *                                                                     *
 * PARAMETERS:   event   - the most recent event.                      *
 *                                                                     *
 * RETURNED:     True if the event is handled, false otherwise.        *
 *                                                                     *
 ***********************************************************************/

static Boolean AppHandleEvent( EventPtr eventP ) {

   Boolean handled = false;

   VoidHand dataH;

   char    *dataP;

   EventType newEvent;

   SyncBmrData_t      *syncP;
   SyncBmrResponse_t  *respP;
   SyncBmrEventData_t *eventDataP;

   XptCommunicationInfo_t *docP;

   switch ( eventP->eType ) {

      case firstUserEvent:

         eventDataP = (SyncBmrEventData_t *)&eventP->data;    // pointer to event data area

         dataH = (VoidHand)eventDataP->dataH;                  // get handle to document body
         docP  = (XptCommunicationInfo_t *)eventDataP->docP;  // get pointer to document info

         if ( dataH && docP ) {

            // Create a response object
            respP = (SyncBmrResponse_t *)MEM_ALLOC( sizeof( SyncBmrResponse_t ) );
            ErrFatalDisplayIf( !respP, "memory allocation" );

            MemPtrSetOwner( respP, 0 );

            // Create a buffer to contain document body
            respP->data = (void *)MEM_ALLOC( MemHandleSize( dataH ) );
            ErrFatalDisplayIf( !respP->data, "memory allocation" );

            MemPtrSetOwner( respP->data, 0 );

            // Copy document body to response block
            dataP = MemHandleLock( dataH );

            MEM_COPY( respP->data, dataP, MemHandleSize( dataH ) );

            respP->length = docP->cbLength;

            STR_COPY( respP->name, docP->docName );
            STR_COPY( respP->type, docP->mimeType );

            MemHandleUnlock( dataH );  // unlock the storage first ...
            MemHandleFree( dataH );    // ... then get rid of it

            MEM_FREE( docP );          // Get rid of temporary doc info

            syncP = (SyncBmrData_t *)globalDataP;  // retrieve the sync block from global storage
            syncP->responseP = respP;               // point to the new response block

         } // end if
         else {
            // MHB: Handle error case
            ErrFatalDisplayIf( true, "SyncBmr Error..." );
         } // end else

         newEvent.eType = appStopEvent;    // tell the app it's time to exit
         EvtAddEventToQueue( &newEvent );  // add to event queue to signal end of processing

         handled = true;

         break;

      default:
         break;

   } // end switch

   return handled;

} // AppHandleEvent()


/***********************************************************************
 *                                                                     *
 * FUNCTION:     AppEventLoop                                          *
 *                                                                     *
 * DESCRIPTION:  A simple loop that obtains events from the Event      *
 *               Manager and passes them on to various applications    *
 *               and system event handlers before passing them on to   *
 *               FrmHandleEvent for default processing.                *
 *                                                                     *
 * PARAMETERS:   None.                                                 *
 *                                                                     *
 * RETURNED:     Nothing.                                              *
 *                                                                     *
 ***********************************************************************/

static void AppEventLoop( void ) {

   EventType event;
   Word      error;

   do {
      // Get the next available event.
      EvtGetEvent( &event, evtWaitForever );

      // Give the system a chance to handle the event.
      if ( !SysHandleEvent( &event ) )

         // Give the menu a chance to handle the event
         if ( !MenuHandleEvent( 0, &event, &error ) )

            // Give the application a chance to handle the event.
            if ( !AppHandleEvent( &event ) )

               // Let the form object provide default handling of the event.
               FrmHandleEvent( FrmGetActiveForm(), &event );

   } while ( event.eType != appStopEvent );

} // AppEventLoop()


/***********************************************************************
 *                                                                     *
 * FUNCTION:        PilotMain                                          *
 *                                                                     *
 * DESCRIPTION: This function is the equivalent of a main() function   *
 *              in standard "C". It is called by the Emulator to begin *
 *              execution of this application.                         *
 *                                                                     *
 * PARAMETERS:  cmd         - cmd specifying how to launch the app.    *
 *              cmdPBP      - parameter block for the command.         *
 *              launchFlags - flags used to configure the launch.      *
 *                                                                     *
 * RETURNED:    Any applicable error code.                             *
 *                                                                     *
 ***********************************************************************/

DWord PilotMain( Word cmd, Ptr cmdPBP, Word launchFlags ) {

   Err error = 0;

   // This app makes use of PalmOS 3.0 features.  It will crash if
   // run on an earlier version of PalmOS.  Detect and warn if
   // this happens, then exit.

   error = RomVersionCompatible( Version30, launchFlags );

   if ( !error ) {

      if ( cmd == sysAppLaunchCmdNormalLaunch ) {
         FrmAlert( ErrorAlert );
//       StartApplication();
//       AppEventLoop();
//       StopApplication();
      } // end if

      else if ( cmd == sysAppLaunchCmdSyncNotify ) {
         // register our extension on syncNotify so we do
         // not need to be run before we can receive data.
         ExgRegisterData( SYNC_BMR_ID, exgRegExtensionID, NULL );
      } // end else if

      else if ( cmd == sysAppLaunchCmdExgAskUser ) {
         ((ExgAskParamPtr)cmdPBP)->result = exgAskOk;
      } // end else if

      else if ( cmd == sysAppLaunchCmdExgReceiveData ) {
         // MHB: Handle spurious data conditions...
         error = ReceiveData( (ExgSocketPtr)cmdPBP );
      } // end else if

      else if ( cmd == sysAppLaunchSyncBmrSend ) {
         SyncBmrData_t *dataP = (SyncBmrData_t *)cmdPBP;
         StartApplication();
         error = SyncBmrSendData( dataP );
         if ( !error ) {
            if ( dataP->waitForResponse ) {
               globalDataP   = dataP;  // save until response received
               globalWaiting = true;   // indicate waiting for response
               AppEventLoop();         // wait until response received
               globalWaiting = false;  // no longer waiting for response
            } // end if
            else {
               // MHB: Anything need to be done here?
            } // end else
         } // end if
         else {
            if ( dataP->waitForResponse ) {
               // MHB: Anything need to be done here?
            } // end if
            else {
               // MHB: Anything need to be done here?
            } // end else
         } // end else
         StopApplication();
      } // end else if

      else if ( cmd == sysAppLaunchSyncBmrReceive ) {
         SyncBmrData_t *dataP = (SyncBmrData_t *)cmdPBP;
         StartApplication();
         error = SyncBmrReceiveData( dataP );
         StopApplication();
      } // end else if

   } // end if

   return error;

} // end PilotMain()


