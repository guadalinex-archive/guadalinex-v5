
/*************************************************************************/
/* module:          SyncML OBEX binding source file.                     */
/* file:            src/xpt/bindings/obex/win/obexbinding.c              */
/* target system:   all                                                  */
/* target OS:       all                                                  */
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

#define OBEX_BINDING_IS_EXPORTING 1

#include <xptTransport.h> // Mickey 2003-01-29
#include <obexbinding.h>
#include <iConstants.h>

#include <string.h>
#include <stdio.h>
#include <limits.h>

#include <handle.h> // Mickey 2003-01-29

#ifdef _WIN32
 #define WIN32_LEAN_AND_MEAN
 #include <windows.h>
 #define oSleep(sec)   SleepEx((sec)*1000, TRUE)
#else
 #include <unistd.h>
 #define oSleep sleep
#endif

/*********************************************************************************/
/***************************** Public Functions **********************************/
/*********************************************************************************/
#ifdef INCLUDE_OBEX_STATICALLY
#define initializeTransport obexInitializeTransport
#endif
/**
 *  Called by xpt when our DLL is loaded.  This function is required to
 *  call and register each api proto.
 */
XPTEXP1 Ret_t XPTAPI XPTEXP2 initializeTransport(void) {
   Ret_t rc1, rc2;

   XPTDEBUG(("OBX initializeTransport()\n"));

   ObxInit();

   rc1 = obxRegisterTcpObex();
   rc2 = obxRegisterIrObex();

   return rc1 ? rc1 : rc2;
}


/**
 * FUNCTION: Called when xptSelectProtocol() is called, selecting this
 *           transport binding.
 *
 *  Select the communication protocol to start the communication
 *
 * IN: privateTransportInfo, pointer to the transport binding's private
 *         information about the transport, the value given in the
 *         privateTransportInfo field of the xptTransportDescription struct
 *         when the transport binding was registered.
 *
 *     szSettings, passed directly from the xptSelectProtocol() call.
 *
 * OUT: *pPrivateServiceInfo, pointer to the transport binding's private
 *         information for the newly allocated service instance.  This
 *         pointer will be passed on subsequent calls to the transport's
 *         functions to identify this service instance to the transport.
 *
 * META:
 *          HOST=portSpec
 *             For example, 192.168.5.1 or howlandm.endicott.ibm.com
 *             If not specified, and using INET connections, the default is 'localhost'
 *          PORT=portSpec
 *             For example, 1122
 *             If not specified, and using INET connections, the default is 650 (OBEX_PORT constant
 *             in the code).  This is also the default if unable to convert to short.
 *          SERVICE=irServiceSpec
 *             For example, OBEX
 *             If not specified, and using IR connections, the default is 'OBEX'.
 *
 */

// **MHB** Investigate using ObxList instead of ObxSequenceNode

Ret_t XPTAPI obxSelectProtocol( void *privateTransportInfo,
                                const char *metaInformation,
                                unsigned int flags,
                                void **pPrivateServiceInfo ) {

   ObxServiceBlock *service;

   const char      *metaValue;
   char            *ckstr;
   unsigned long   result;
   size_t          metaLen;

   ObxRc rc;

   char *s = NULL;

   XPTDEBUG(("OBX obxSelectProtocol()\n"));

   CHECK_PARM( privateTransportInfo, "obxSelectProtocol()", "privateTransport Info" );
   CHECK_PARM( metaInformation,      "obxSelectProtocol()", "metaInformation" );

   service = (ObxServiceBlock *)ALLOC_MEM( sizeof( ObxServiceBlock ) );
   CHECK_ALLOC( service, "obxSelectProtocol()", "service" );
   *pPrivateServiceInfo = (void *)service;

   service->serviceName = NULL;
   service->connections = NULL;
   service->metadata    = NULL;
   service->obxHandle   = NULL;
   service->flags       = flags;
   service->transport   = (ObxTransportBlock *)privateTransportInfo;

   metaLen = strlen( metaInformation );
   s = (char *)ALLOC_MEM( metaLen + 1 );
   if( !s ) {
      obxRecordError( OBX_ERRORMSG_MEMORY_ALLOC, "obxSelectProtocol()", "s" );
      obxFreeService( service );
      return OBX_RC_MEMORY_ERROR_ALLOC;
   } // end if

   strncpy( s, metaInformation, metaLen );
   *( s + metaLen ) = '\0';

   service->metadata = s;    // Keep for subsequent call to ObxTransportInitialize()

   if ( service->transport->obxTransType == OBEX_TRANS_IRDA ) {
      metaValue = xptGetMetaInfoValue( metaInformation, "SERVICE", &metaLen );
      if ( !metaValue || metaLen == 0 ) {
         s = (char *)ALLOC_MEM( 5 );   // strlen('OBEX') + 1
         if( !s ) {
            obxRecordError( OBX_ERRORMSG_MEMORY_ALLOC, "obxSelectProtocol()","s" );
            obxFreeService( service );
            return OBX_RC_MEMORY_ERROR_ALLOC;
         }
         strcpy( s, "OBEX" );
      } else {
         s = (char *)ALLOC_MEM( metaLen+1 );
         if( !s ) {
            obxRecordError( OBX_ERRORMSG_MEMORY_ALLOC, "obxSelectProtocol()","s" );
            obxFreeService( service );
            return OBX_RC_MEMORY_ERROR_ALLOC;
         }
         strncpy( s, metaValue, metaLen );
         *(s+metaLen) = '\0';
         }
   } else if ( service->transport->obxTransType == OBEX_TRANS_INET ) {

      // HOST
      metaValue = xptGetMetaInfoValue( metaInformation, "HOST", &metaLen );
      if ( metaValue ) {
         s = (char *)ALLOC_MEM( metaLen+1 );
         if( !s ) {
            obxRecordError( OBX_ERRORMSG_MEMORY_ALLOC, "obxSelectProtocol()","s" );
            obxFreeService( service );
            return OBX_RC_MEMORY_ERROR_ALLOC;
         }
         strncpy( s, metaValue, metaLen );
         *(s+metaLen) = '\0';
      } else {
         s = (char *)ALLOC_MEM( 10 );  // strlen('localhost') + 1
         if( !s ) {
            obxRecordError( OBX_ERRORMSG_MEMORY_ALLOC, "obxSelectProtocol()","s" );
            obxFreeService( service );
            return OBX_RC_MEMORY_ERROR_ALLOC;
         }
         strcpy( s, "localhost" );
      }

      // PORT
      metaValue = xptGetMetaInfoValue( metaInformation, "PORT", &metaLen );
      if ( metaValue ) {
         result = strtoul( metaValue, &ckstr, 10 );
         // %%% luz:2002-05-28, corrected such that ok if strtoul reads entire value
         //     but does not need to end with NUL!
         if ( ckstr!=metaValue+metaLen || result > USHRT_MAX ) {
         // %%% original: if ( ckstr==metaValue || *ckstr || result > USHRT_MAX ) {
            service->port = OBEX_PORT;
         } else {
            service->port = (unsigned short)result;
         }
      } else {
         service->port = OBEX_PORT;
      }
   } else {
      obxRecordError( OBX_ERRORMSG_UNKNOWN_TRANSPORT_TYP );
      return OBX_RC_OBEX_INIT_FAILURE;
   }

   service->serviceName = s;

   // If Server, initialize transport.  Server will then be 'listening' for incoming connections.
   if ( ( service->flags & XPT_SERVER ) == XPT_SERVER ) {

      // Create OBEX Handle
      service->obxHandle = ObxHandleNew();
      if ( service->obxHandle == NULL ) {
         obxRecordError( OBX_ERRORMSG_OBEX_TRANSPORT_FAILURE, "OBEX Init" );
         return OBX_RC_OBEX_INIT_FAILURE;
      } // end if

      switch ( service->transport->obxTransType ) {
         case OBEX_TRANS_INET:
            rc = ObxTransportRegister( service->obxHandle, ObxTransportGet( DEFINED_TRANSPORT_INET ) );
            if ( rc != OBX_RC_OK ) {
               return OBX_RC_OBEX_INIT_FAILURE;
            } // end if
            break;
         case OBEX_TRANS_IRDA:
            rc = ObxTransportRegister( service->obxHandle, ObxTransportGet( DEFINED_TRANSPORT_IRDA ) );
            if ( rc != OBX_RC_OK ) {
               return OBX_RC_OBEX_INIT_FAILURE;
            } // end if
            break;
         default:
            return OBX_RC_OBEX_INIT_FAILURE;
      } // end switch

      if ( ObxTransportInitialize( service->obxHandle, service->metadata ) != OBX_RC_OK ) {
         return OBX_RC_OBEX_INIT_FAILURE;
      } // end if
      if ( ObxTransportOpen( service->obxHandle ) != OBX_RC_OK ) {
         return OBX_RC_OBEX_INIT_FAILURE;
      } // end if
      if ( ObxTransportListen( service->obxHandle ) != OBX_RC_OK ) {
         return OBX_RC_OBEX_INIT_FAILURE;
      } // end if

   } // end if

    return OBX_RC_OK;
}


/**
 * FUNCTION: Called when xptDeselectProtocol() is called, deselecting this
 *           transport binding.
 *
 *  Stop a communication service instance.
 *
 * IN: privateServiceInfo, pointer to the transport binding's private
 *         information about the service instance.  This is the same value
 *         that was returned by the "selectProtocol" function when the
 *         service instance was created.
 */

// **MHB** Close and DeRegister underlying OBEX protocol as well?

Ret_t XPTAPI obxDeselectProtocol( void *privateServiceInfo ) {

   ObxServiceBlock *service = (ObxServiceBlock *)privateServiceInfo;

   XPTDEBUG(("OBX obxDeselectProtocol()\n"));

   CHECK_PARM( service, "obxDeselectProtocol()", "service" );

   if ( ( service->flags & XPT_SERVER ) == XPT_SERVER ) {

      if ( ObxTransportClose( service->obxHandle ) != OBX_RC_OK ) {
         return OBX_RC_GENERAL_ERROR;
      } // end if
      if ( ObxTransportTerminate( service->obxHandle ) != OBX_RC_OK ) {
         return OBX_RC_GENERAL_ERROR;
      } // end if

   } // end if

   obxFreeService( service );    // Ignore rc

   return OBX_RC_OK;
}

/**
 * FUNCTION: Called when xptOpenCommunication() is called, creating a
 *           connection instance.
 *
 *  Create a connection instance for the given service id.
 *
 * IN: privateServiceInfo, pointer to the transport binding's private
 *         information about the service instance.  This is the same value
 *         that was returned by the "selectProtocol" function when the
 *         service instance was created.
 *
 *     role, passed directly from the xptOpenCommunication() call.
 *          either XPT_REQUEST_RECEIVER or XPT_REQUEST_SENDER
 *
 * OUT: *pPrivateConnectionInfo, pointer to the transport binding's private
 *         information for the newly allocated connection instance.  This
 *         pointer will be passed on subsequent calls to the transport's
 *         functions to identify this connection instance to the transport.
 */

// **MHB** Investigate using ObxList instead of ObxSequenceNode

Ret_t XPTAPI obxOpenCommunication( void *privateServiceInfo,
                                   int role,
                                   void **pPrivateConnectionInfo ) {

   ObxServiceBlock    *service    = (ObxServiceBlock *)privateServiceInfo;
   ObxConnectionBlock *connection = NULL;

   int rc = OBX_RC_OK;

   XPTDEBUG(("OBX obxOpenCommunication()\n"));

   CHECK_PARM( privateServiceInfo, "obxOpenCommunication()", "privateServiceInfo" );

   connection = (ObxConnectionBlock *)ALLOC_MEM( sizeof( ObxConnectionBlock ));
   CHECK_ALLOC( connection, "obxOpenCommunication()", "connection" );

   connection->timeout        = OBX_DEFAULT_TIMEOUT;
   connection->role           = role;
   connection->docInfo        = NULL;
   connection->pchXSyncmlHmac = NULL;
   connection->dataToRead     = NULL;
   connection->dataToSend     = NULL;
   connection->obxHandle      = NULL;
   connection->service        = service;
   connection->connected      = OBEX_CONNECTED_NO;

   if ( ( rc = obxAddConnection( service, connection ) ) == OBX_RC_OK ) {
      *pPrivateConnectionInfo = (void *)connection;
      if ( ( service->flags & XPT_SERVER ) == XPT_SERVER ) {  // Running as Server.
         connection->obxHandle = service->obxHandle;
      } // end if
      else {                                                  // Running as Client.
         rc = obxRegisterTransport( connection );
      } // end else
   } // end if
   else {
      obxFreeConnection( connection );
      *pPrivateConnectionInfo = NULL;
   } // end else


/*
We move this paragraph, which is to make an OBEX connection, from function 
obxBeginExchange to here because all SyncML message exchanging in the same 
SyncML session must take place just in one OBEX connection. This function, 
obxOpenCommunication, can only be called once right in the start of every 
SyncML session using OBEX protocol.
*/ 
   if(rc==OBX_RC_OK) {
      switch ( connection->role ) {
         case XPT_REQUEST_RECEIVER:
            connection->currentRole = OBX_REQUEST_ROLE_RECEIVER;
            XPTDEBUG(("OBX obxOpenCommunication() initializing as RECEIVER (== Server Mode)\n"));
            rc = obxInitializeForServerMode( connection );  // accept()
            break;
         case XPT_REQUEST_SENDER:
            connection->currentRole = OBX_REQUEST_ROLE_SENDER;
            XPTDEBUG(("OBX obxOpenCommunication() initializing as SENDER (== Client Mode)\n"));
            rc = obxInitializeForClientMode( connection );  // connect() to peer
            break;
         default:
             XPTDEBUG(("OBX obxOpenCommunication() [ERROR] unknown role speciefied - bailing out\n"));
             return OBX_RC_ERR_TRANSPORT;
      }
   }

    return rc;
}


/**
 * FUNCTION: Called when xptCloseCommunication() is called, closing a
 *           connection instance.
 *
 *  Close a connection instance.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 *
 *     disp, passed directly from the xptOpenCommunication() call.
 */
Ret_t XPTAPI obxCloseCommunication( void *privateConnectionInfo ) {

   ObxObject *response = NULL;
   Ret_t rc;
   ObxConnectionBlock *ocb = (ObxConnectionBlock *)privateConnectionInfo;
   ObxIterator    *iterator;
   ObxHeader      *header;
   int connectionIDHeaderCount=0;

   XPTDEBUG(("OBX obxCloseCommunication()\n"));

   CHECK_PARM( ocb, "obxCloseCommunication()", "ocb" );

/*
We move this switch block that is to break up an OBEX connection, from function 
obxEndExchange to here because all SyncML message exchanging in the same 
SyncML session must take place just in one OBEX connection. This function, 
obxCloseCommunication, can only be called once right in the end of every 
SyncML session using OBEX protocol.
*/
    switch ( ocb->role ) {
    case XPT_REQUEST_RECEIVER:
        XPTDEBUG(("OBX obxCloseCommunication() waiting for DISCONNECT from client\n"));
        // 
            response = ObxObjectNew( ocb->obxHandle, OBEX_CMD_NULL);
            if ( response == NULL ) {
                return OBX_RC_OBEX_HEADER;
            }
            rc = ObxTransactionRecv( ocb->obxHandle, response, TRUE );
            if (rc != OBX_RC_OK) {
                ObxObjectFree(ocb->obxHandle, response);
                return rc;
            }

//Check if the header of ConnectionID carried by Disconnect command is legal.
            /* ok we received the Disconnect - now lets ript the headers apart */
            if ( !(iterator = ObxIteratorReset( ObxListGetIterator( ObxObjectGetHeaderList( response ) ) )) ) {
                return OBX_RC_ERR_MEMORY;
            }
            while ( ObxIteratorHasNext( iterator ) ) {
                if ( (header = (ObxHeader *)ObxIteratorNext( iterator )) ) {
                /*
                ** Which header is it ?
                    */
                    switch (header->identifier) {
                    case OBEX_HEADER_CONNECTION:
//Check if ConnectionID in the request command is equal to the one we've assigned in the response of Connect command.
                        XPTDEBUG(("OBX obxCloseCommunication() The ConnectionID carried in request command is %d and the one we assign is %d\n", header->value.fourBytevalue, ocb->obxHandle->OBEXConnectionID ));
                        if(header->value.fourBytevalue!=ocb->obxHandle->OBEXConnectionID)
                            return OBX_RC_ERR_SML_CONNECTIONID_HDR;
                        connectionIDHeaderCount++;
                        break;
                    }
                }
            }
            ObxIteratorFree( iterator );
            if(connectionIDHeaderCount!=1)
               return OBX_RC_ERR_SML_CONNECTIONID_HDR;
        
        break;
    case XPT_REQUEST_SENDER:
        XPTDEBUG(("OBX obxCloseCommunication() sending DISCONNECT to server\n"));
        if ((rc = obxSendObexDisconnect(ocb)) != OBX_RC_OK) {
            return rc;
        }
        break;
    default:
        XPTDEBUG(("OBX obxCloseCommunication() [ERROR] unknown role specified - bailing out\n"));
        return OBX_RC_ERR_TRANSPORT;
    }

   if ( ( ocb->service->flags & XPT_CLIENT ) == XPT_CLIENT ) {
      if ( ObxTransportClose( ocb->obxHandle ) != OBX_RC_OK ) {
         return OBX_RC_GENERAL_ERROR;
      } // end if
      if ( ObxTransportTerminate( ocb->obxHandle ) != OBX_RC_OK ) {
         return OBX_RC_GENERAL_ERROR;
      } // end if

   } // end if

   obxRemoveConnection( ocb );
   obxFreeConnection( ocb );

    return OBX_RC_OK;
}

/**
 * FUNCTION: Called when xptBeginExchange() is called
 *
 *  Prepare for a document exchange.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 */
Ret_t XPTAPI obxBeginExchange( void *privateConnectionInfo ) {
//See obxOpenCommunication.
   return OBX_RC_OK;
/*
   ObxConnectionBlock *ocb = (ObxConnectionBlock *)privateConnectionInfo;
    ObxRc rc = OBX_RC_OK;

   switch ( ocb->role ) {
      case XPT_REQUEST_RECEIVER:
         ocb->currentRole = OBX_REQUEST_ROLE_RECEIVER;
         XPTDEBUG(("OBX obxBeginExchange() initializing as RECEIVER (== Server Mode)\n"));
         rc = obxInitializeForServerMode( ocb );  // accept()
         break;
      case XPT_REQUEST_SENDER:
         ocb->currentRole = OBX_REQUEST_ROLE_SENDER;
         XPTDEBUG(("OBX obxBeginExchange() initializing as SENDER (== Client Mode)\n"));
         rc = obxInitializeForClientMode( ocb );  // connect() to peer
         break;
      default:
          XPTDEBUG(("OBX obxBeginExchange() [ERROR] unknown role speciefied - bailing out\n"));
          return OBX_RC_ERR_TRANSPORT;
   }

   return rc;
*/
}


/**
 * FUNCTION: Called when xptEndExchange() is called
 *
 *  Clean up after a document exchange.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 */
Ret_t XPTAPI obxEndExchange( void *privateConnectionInfo ) {
    
    ObxConnectionBlock *ocb = (ObxConnectionBlock *)privateConnectionInfo;

//See xptCloseCommunication.
/*
    ObxObject *response = NULL;
    Ret_t rc;
   XPTDEBUG(("OBX obxEndExchange()\n"));

   CHECK_PARM( ocb, "obxEndExchange()", "ocb" );

    switch ( ocb->role ) {
    case XPT_REQUEST_RECEIVER:
        XPTDEBUG(("OBX obxEndExchange() waiting for DISCONNECT from client\n"));
        // 
            response = ObxObjectNew( ocb->obxHandle, OBEX_CMD_NULL);
            if ( response == NULL ) {
                return OBX_RC_OBEX_HEADER;
            }
            rc = ObxTransactionRecv( ocb->obxHandle, response, TRUE );
            if (rc != OBX_RC_OK) {
                ObxObjectFree(ocb->obxHandle, response);
                return rc;
            }
        
        break;
    case XPT_REQUEST_SENDER:
        XPTDEBUG(("OBX obxEndExchange() sending DISCONNECT to server\n"));
        if ((rc = obxSendObexDisconnect(ocb)) != OBX_RC_OK) {
            return rc;
        }
        break;
    default:
        XPTDEBUG(("OBX obxEndExchange() [ERROR] unknown role specified - bailing out\n"));
        return OBX_RC_ERR_TRANSPORT;
    }
*/

   // Clean up any existing information contained in the ObxConnectionBlock
   obxResetConnection( ocb );
    return OBX_RC_OK;
}


/**
 * FUNCTION: Called when xptGetDocumentInfo() is called
 *
 *  Retrieve the document information associated with an incoming document.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 *
 *     pDoc, passed directly from the xptGetDocumentInfo() call.
 */
Ret_t XPTAPI obxGetDocumentInfo( void *privateConnectionInfo,
                                 XptCommunicationInfo_t *pDoc ) {

   ObxConnectionBlock *ocb = (ObxConnectionBlock *)privateConnectionInfo;

   Ret_t rc  = OBX_RC_OK;
    int length = 0;
    
    ObxObject *response = NULL;
    
    ObxObject *request = NULL;
    ObxIterator    *iterator;
    ObxHeader      *header;
    unsigned char  *someBuf, *buffer=NULL;
    ObxBuffer      *inboundbuffer,*bufferx;
    XptCommunicationInfo_t *docInfo = NULL;
    unsigned char  *param, *value;
    int bodyHeaderCount=0;
    int typeHeaderCount=0;
    int connectionIDHeaderCount=0;

   XPTDEBUG(("OBX obxGetDocumentInfo()\n"));

   CHECK_PARM( privateConnectionInfo, "obxGetDocumentInfo()", "privateConnectionInfo" );
   CHECK_PARM( pDoc,                  "obxGetDocumentInfo()", "pDoc" );

   if ( !ocb->docInfo  ) {
        if (ocb->role == XPT_REQUEST_RECEIVER) {
            /* we are Server  -> waiting for PUT */
            request = ObxObjectNew( ocb->obxHandle, OBEX_CMD_NULL);
            if ( request == NULL ) {
                return OBX_RC_OBEX_HEADER;
            }
            rc = ObxTransactionRecv( ocb->obxHandle, request, TRUE );
            if (rc != OBX_RC_OK) {
                ObxObjectFree(ocb->obxHandle, request);
                return rc;
            }
//We can only get commands of Put and Abort here.
            if( request->cmd != (OBEX_CMD_PUT|OBEX_CMD_FINAL) && request->cmd != (OBEX_CMD_ABORT|OBEX_CMD_FINAL) ) {
                ObxObjectFree(ocb->obxHandle, request);
                return OBX_RC_GENERAL_ERROR;
            }

            /* ok we received the PUT - now lets ript the headers apart */
            if ( !(iterator = ObxIteratorReset( ObxListGetIterator( ObxObjectGetHeaderList( request ) ) )) ) {
                return OBX_RC_ERR_MEMORY;
            }
            docInfo = (XptCommunicationInfo_t*)malloc(sizeof(XptCommunicationInfo_t));
            if (!docInfo) return OBX_RC_ERR_MEMORY;
            memset(docInfo, 0, sizeof(XptCommunicationInfo_t));
            docInfo->cbLength = -1;        
            while ( ObxIteratorHasNext( iterator ) ) {
                if ( (header = (ObxHeader *)ObxIteratorNext( iterator )) ) {
                /*
                ** Which header is it ?
                    */
                    switch (header->identifier) {
                    case OBEX_HEADER_CONNECTION:
//Check if ConnectionID in the request command is equal to the one we've assigned in the response of Connect command.
                        XPTDEBUG(("OBX obxGetDocumentInfo() The ConnectionID carried in request command is %d and the one we assign is %d\n", header->value.fourBytevalue, ocb->obxHandle->OBEXConnectionID ));
                        if(header->value.fourBytevalue!=ocb->obxHandle->OBEXConnectionID)
                            return OBX_RC_ERR_SML_CONNECTIONID_HDR;
                        connectionIDHeaderCount++;
                        break;
                    case OBEX_HEADER_TYPE:
                        length = ObxBufSize(header->value.byteSequenceValue);
//Need byte '0' to be a complete C string
//                        someBuf = (unsigned char *)malloc( length );
                        someBuf = (unsigned char *)malloc( length+1 );
                        memset(someBuf,0,length+1);
                        ObxBufRead( header->value.byteSequenceValue, someBuf, length );
                        strcpy(docInfo->mimeType, (const char *)someBuf); // %%% luz 2002-04-16: added cast
                        free( someBuf );
                        typeHeaderCount++;
                        break;
                    case OBEX_HEADER_LENGTH: 
                        docInfo->cbLength = header->value.fourBytevalue;
                        break;
                    case OBEX_HEADER_DESCRIPTION:
                        if (strlen(docInfo->mimeType) > 1) break;
                        length = ObxBufSize(header->value.unicodeValue);
                        someBuf = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.unicodeValue, someBuf, length );
                        inboundbuffer = ObxUnicodeToUTF8( (const unsigned char *)docInfo->mimeType, length ); // %%% luz 2002-04-16: added cast
                        ObxBufRead( inboundbuffer, someBuf, min( length, ObxBufSize( inboundbuffer ) ) );
                        ObxBufFree( inboundbuffer );
                        strcpy(docInfo->mimeType, (const char *)someBuf); // %%% luz 2002-04-16: added cast
                        free(someBuf);
                        break;
                    case OBEX_HEADER_NAME: 
                        length = ObxBufSize(header->value.unicodeValue);
                        someBuf = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.unicodeValue, someBuf, length );
                        inboundbuffer = ObxUnicodeToUTF8( someBuf, length );
                        ObxBufRead( inboundbuffer, someBuf, min( length, ObxBufSize( inboundbuffer ) ) );
                        ObxBufFree( inboundbuffer );
                        strcpy(docInfo->docName, (const char *)someBuf); // %%% luz 2002-04-16: added cast
                        free(someBuf);
                        break;
                    case OBEX_HEADER_XSYNCMLHMAC: 
                        length = ObxBufSize(header->value.unicodeValue);
                        someBuf = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.unicodeValue, someBuf, length );
                        inboundbuffer = ObxUnicodeToUTF8( someBuf, length );
                        ObxBufRead( inboundbuffer, someBuf, min( length, ObxBufSize( inboundbuffer ) ) );
                        ObxBufFree( inboundbuffer );
                        ocb->pchXSyncmlHmac = (char *)someBuf; // %%% luz 2002-04-16: added cast

                        docInfo->hmacInfo = (XptHmacInfo_t *)malloc(sizeof (XptHmacInfo_t));
                        while (someBuf != NULL)
                        {
                            someBuf = splitParmValue(someBuf, &param, &value);
                            if ((param != NULL) && (param [0] != '\0'))
                            {
                                if (!strcmp ((const char *)param, "algorithm")) // %%% luz 2002-04-16: added cast
                                    docInfo->hmacInfo->algorithm = (const char *) value; // %%% luz 2002-04-16: added cast
                                else if (!strcmp ((const char *)param, "username")) // %%% luz 2002-04-16: added cast
                                    docInfo->hmacInfo->username = (const char *) value; // %%% luz 2002-04-16: added cast
                                else if (!strcmp ((const char *)param, "mac")) // %%% luz 2002-04-16: added cast
                                    docInfo->hmacInfo->mac = (const char *) value; // %%% luz 2002-04-16: added cast
                            }
                        }
                        XPTDEBUG(("OBX obxGetDocumentInfo() HMAC: algorithm: '%s' username: '%s' mac: '%s'\n", 
                                   docInfo->hmacInfo->algorithm, docInfo->hmacInfo->username, docInfo->hmacInfo->mac ));
                        break;
// %%% luz 2003-07-07 - added Mickey 2003.1.28 code from client case to server, too
//Both OBEX_HEADER_BODY and OBEX_HEADER_BODY_END can carry payload data.
                    case OBEX_HEADER_BODY: 
                    case OBEX_HEADER_BODY_END:
                        length = ObxBufSize(header->value.byteSequenceValue);
// %%% luz 2003-07-07 - added Mickey 2003.1.28 code from client case to server, too
//There may be no OBEX_HEADER_LENGTH header in Put command.
                        docInfo->cbLength=length;
                        buffer = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.byteSequenceValue, buffer, length );
                        bodyHeaderCount++;
                        break;
                    default: break; /* ignore */
                    }
                }
            }
            ObxIteratorFree( iterator );
//We check Abort command here because we must check header of ConnectionID first.
            if( request->cmd == OBEX_CMD_ABORT ) {
                ObxObjectFree(ocb->obxHandle, request);
                return OBX_RC_ERR_GET_ABORT;
            }

//There must be data returned in Put command.
            if(bodyHeaderCount==0) {
               ObxObjectFree(ocb->obxHandle, request);
               return OBX_RC_ERR_SML_BODY_HDR;
            }
//There must be Type and ConnectionID header in Put command.
            if(typeHeaderCount!=1) {
               ObxObjectFree(ocb->obxHandle, request);
               return OBX_RC_ERR_SML_TYPE_HDR;
            }
            if(connectionIDHeaderCount!=1) {
               ObxObjectFree(ocb->obxHandle, request);
               return OBX_RC_ERR_SML_CONNECTIONID_HDR;
            }

            /* EOF - processing headers */
            ocb->docInfo = docInfo;
            
            ocb->dataToRead = (ObxBuf *)ALLOC_MEM( sizeof( ObxBuf ) );
            if (!ocb->dataToRead) {
                ObxObjectFree(ocb->obxHandle, request);
//                ObxObjectFree(ocb->obxHandle, response);                
                return OBX_RC_ERR_MEMORY;
            }
            ocb->dataToRead->buf = (unsigned char *)ALLOC_MEM( ocb->docInfo->cbLength );
            if (!ocb->dataToRead) {
                ObxObjectFree(ocb->obxHandle, request);
//                ObxObjectFree(ocb->obxHandle, response);                
                return OBX_RC_ERR_MEMORY;
            }
            ocb->dataToRead->cursor = ocb->dataToRead->buf;
            ocb->dataToRead->length = ocb->docInfo->cbLength;
            COPY_MEM(buffer, ocb->dataToRead->buf,  ocb->docInfo->cbLength );
            ObxObjectFree(ocb->obxHandle, request);
            
        } else if (ocb->role == XPT_REQUEST_SENDER) {
            /* we are a client -> using GET */
            if ( !(request = ObxObjectNew( ocb->obxHandle, OBEX_CMD_GET )) ) {
                ObxObjectFree(ocb->obxHandle, request);
                return rc;
            } else if ( !(response = ObxObjectNew(ocb->obxHandle, OBEX_CMD_NULL))) {
                ObxObjectFree(ocb->obxHandle, request);
                return rc;
            }

//We must embed ConnectionID header in Get command.
            header = ObxHeaderNew( OBEX_HEADER_CONNECTION );
            rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
            if ( rc == OBX_RC_OK ) {
                XPTDEBUG(("OBX obxGetDocumentInfo() Add header '%s' value '%x'\n", "OBEX_HEADER_CONNECTION", ocb->obxHandle->OBEXConnectionID ));
                rc = ObxHeaderSetIntValue( header, ocb->obxHandle->OBEXConnectionID );
                if ( rc == OBX_RC_OK ) {
                    rc = ObxObjectAddHeader( ocb->obxHandle, request, header );
                    if ( rc == OBX_RC_OK ) {
                        header = NULL; // we no longer own the header
                    } // end if
                } // end if
            } // end if
    
            // Clean up from an error
            if ( rc != OBX_RC_OK ) {
                obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxGetDocumentInfo()", "OBEX_HEADER_CONNECTION" );
                if ( header != NULL ) {
                    ObxHeaderFree( header );
                } // end if
                if ( request != NULL ) {
                    ObxObjectFree( ocb->obxHandle, request );
                } // end if
                return OBX_RC_OBEX_HEADER;
            } // end if

//Add Type header into Get command.
            header = ObxHeaderNew( OBEX_HEADER_TYPE );
            rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
            if ( rc == OBX_RC_OK ) {
                XPTDEBUG(("OBX obxGetDocumentInfo() Add header '%s' value '%s'\n", "OBEX_HEADER_TYPE", pDoc->mimeType ));
                bufferx = ObxBufNew( strlen( pDoc->mimeType ) + 1 );
                rc = ( bufferx == NULL ) ? OBX_RC_MEMORY_ERROR : OBX_RC_OK;
                if ( rc == OBX_RC_OK ) {
                    ObxBufWrite( bufferx, pDoc->mimeType, strlen( pDoc->mimeType ) + 1 );
                    rc = ObxHeaderSetByteSequenceValue( header, bufferx );

                    if ( rc == OBX_RC_OK ) {
                        bufferx = NULL;  // we no longer own the buffer
                        rc = ObxObjectAddHeader( ocb->obxHandle, request, header );
                        if ( rc == OBX_RC_OK ) {
                            header = NULL; // we no longer own the header
                        } // end if
                    } // end if
                } // end if
            } // end if
    
            // Clean up from an error
            if ( rc != OBX_RC_OK ) {
                obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxGetDocumentInfo()", "OBEX_HEADER_TYPE" );
                if ( bufferx != NULL ) {
                    ObxBufFree( bufferx );
                } // end if
                if ( header != NULL ) {
                    ObxHeaderFree( header );
                } // end if
                if ( request != NULL ) {
                    ObxObjectFree( ocb->obxHandle, request );
                } // end if
                return OBX_RC_OBEX_HEADER;
            } // end if

            if ((rc = ObxGetRecv( ocb->obxHandle, request, response)) != 0) {
                ObxObjectFree(ocb->obxHandle, request);
                ObxObjectFree(ocb->obxHandle, response);
                return rc;
            }
            /* ok we received the GET answer - now lets ript the headers apart */
            if ( !(iterator = ObxIteratorReset( ObxListGetIterator( ObxObjectGetHeaderList( response ) ) )) ) {
                ObxObjectFree(ocb->obxHandle, request);
                ObxObjectFree(ocb->obxHandle, response);
                return OBX_RC_ERR_MEMORY;
            }
            docInfo = (XptCommunicationInfo_t*)malloc(sizeof(XptCommunicationInfo_t));
            if (!docInfo) return OBX_RC_ERR_MEMORY;
            memset(docInfo, 0, sizeof(XptCommunicationInfo_t));
            docInfo->cbLength = -1;        
            while ( ObxIteratorHasNext( iterator ) ) {
                if ( (header = (ObxHeader *)ObxIteratorNext( iterator )) ) {
                /*
                ** Which header is it ?
                    */
                    switch (header->identifier) {
                    case OBEX_HEADER_TYPE:
                        length = ObxBufSize(header->value.byteSequenceValue);
                        someBuf = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.byteSequenceValue, someBuf, length );
                        strcpy(docInfo->mimeType, (const char *) someBuf); // %%% luz 2002-04-16: added cast
                        free( someBuf );
//                        typeHeaderCount++;
                        break;
                    case OBEX_HEADER_LENGTH: 
                        docInfo->cbLength = header->value.fourBytevalue;
                        break;
                    case OBEX_HEADER_DESCRIPTION:
                        if (strlen(docInfo->mimeType) > 1) break;
                        length = ObxBufSize(header->value.unicodeValue);
                        someBuf = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.unicodeValue, someBuf, length );
                        inboundbuffer = ObxUnicodeToUTF8( (const unsigned char *)docInfo->mimeType, length ); // %%% luz 2002-04-16: added cast
                        ObxBufRead( inboundbuffer, someBuf, min( length, ObxBufSize( inboundbuffer ) ) );
                        ObxBufFree( inboundbuffer );
                        strcpy(docInfo->mimeType, (const char *)someBuf); // %%% luz 2002-04-16: added cast
                        free(someBuf);
                        break;
                    case OBEX_HEADER_NAME: 
                        length = ObxBufSize(header->value.unicodeValue);
                        someBuf = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.unicodeValue, someBuf, length );
                        inboundbuffer = ObxUnicodeToUTF8( someBuf, length );
                        ObxBufRead( inboundbuffer, someBuf, min( length, ObxBufSize( inboundbuffer ) ) );
                        ObxBufFree( inboundbuffer );
                        strcpy(docInfo->docName, (const char*) someBuf); // %%% luz 2002-04-16: added cast
                        free(someBuf);
                        break;
                    case OBEX_HEADER_XSYNCMLHMAC: 
                        length = ObxBufSize(header->value.unicodeValue);
                        someBuf = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.unicodeValue, someBuf, length );
                        inboundbuffer = ObxUnicodeToUTF8( someBuf, length );
                        ObxBufRead( inboundbuffer, someBuf, min( length, ObxBufSize( inboundbuffer ) ) );
                        ObxBufFree( inboundbuffer );
                        ocb->pchXSyncmlHmac = (char *)someBuf; // %%% luz 2002-04-16: added cast

                        docInfo->hmacInfo = (XptHmacInfo_t *)malloc(sizeof (XptHmacInfo_t));
                        while (someBuf != NULL)
                        {
                            someBuf = splitParmValue(someBuf, &param, &value);
                            if ((param != NULL) && (param [0] != '\0'))
                            {
                                if (!strcmp ((const char *)param, "algorithm")) // %%% luz 2002-04-16: added cast
                                    docInfo->hmacInfo->algorithm = (const char *)value; // %%% luz 2002-04-16: added cast
                                else if (!strcmp ((const char *)param, "username")) // %%% luz 2002-04-16: added cast
                                    docInfo->hmacInfo->username = (const char *)value; // %%% luz 2002-04-16: added cast
                                else if (!strcmp ((const char *)param, "mac")) // %%% luz 2002-04-16: added cast
                                    docInfo->hmacInfo->mac = (const char *)value; // %%% luz 2002-04-16: added cast
                            }
                        }
                        XPTDEBUG(("OBX obxGetDocumentInfo() HMAC: algorithm: '%s' username: '%s' mac: '%s'\n", 
                                   docInfo->hmacInfo->algorithm, docInfo->hmacInfo->username, docInfo->hmacInfo->mac ));
                        break;
//Mickey 2003.1.28
//Both OBEX_HEADER_BODY and OBEX_HEADER_BODY_END can carry payload data.
                    case OBEX_HEADER_BODY: 
                    case OBEX_HEADER_BODY_END:
                        length = ObxBufSize(header->value.byteSequenceValue);
//Mickey 2003.1.28
//There may be no OBEX_HEADER_LENGTH header in Get command.
                        docInfo->cbLength=length;
                        buffer = (unsigned char *)malloc( length );
                        ObxBufRead( header->value.byteSequenceValue, buffer, length );
                        bodyHeaderCount++;
                        break;
                    default: break; /* ignore */
                    }
                }
            }
            ObxIteratorFree( iterator );
            /* EOF - processing headers */
//There must be data returned in the response of Get command.
            if(bodyHeaderCount==0) {
               ObxObjectFree(ocb->obxHandle, request);
               ObxObjectFree(ocb->obxHandle, response);                
               return OBX_RC_ERR_SML_BODY_HDR;
            }
//There must be Type header in Get command.
//            if(typeHeaderCount!=1) {
//               ObxObjectFree(ocb->obxHandle, request);
//               ObxObjectFree(ocb->obxHandle, response);                
//               return OBX_RC_ERR_SML_TYPE_HDR;
//            }

            ocb->docInfo = docInfo;
            
            ocb->dataToRead = (ObxBuf *)ALLOC_MEM( sizeof( ObxBuf ) );
            if (!ocb->dataToRead) {
                ObxObjectFree(ocb->obxHandle, request);
                ObxObjectFree(ocb->obxHandle, response);                
                return OBX_RC_ERR_MEMORY;
            }
            ocb->dataToRead->buf = (unsigned char *)ALLOC_MEM( ocb->docInfo->cbLength );
            if (!ocb->dataToRead->buf) {
                ObxObjectFree(ocb->obxHandle, request);
                ObxObjectFree(ocb->obxHandle, response);                
                return OBX_RC_ERR_MEMORY;
            }

            ocb->dataToRead->cursor = ocb->dataToRead->buf;
            ocb->dataToRead->length = ocb->docInfo->cbLength;
            COPY_MEM(buffer, ocb->dataToRead->buf,  ocb->docInfo->cbLength );

            ObxObjectFree(ocb->obxHandle, request);
            ObxObjectFree(ocb->obxHandle, response);
        } else {
            /* ERROR - this should not happen */
            pDoc->cbLength = -1;
            pDoc->cbSize   = -1;
            strcpy(pDoc->docName, "");
            strcpy(pDoc->mimeType, "");
            if (ocb->pchXSyncmlHmac != NULL)
                FREE_MEM(ocb->pchXSyncmlHmac);
            return OBX_RC_ERR_NOTINITIALIZED;
        }
    }
    strcpy( pDoc->docName,  ocb->docInfo->docName );
    strcpy( pDoc->mimeType, ocb->docInfo->mimeType );
    pDoc->hmacInfo = docInfo->hmacInfo;
    pDoc->cbLength = ocb->docInfo->cbLength;
    XPTDEBUG(("OBX obxGetDocumentInfo() name: '%s' mime: '%s' length: '%d'\n", pDoc->docName, pDoc->mimeType, pDoc->cbLength ));
    return OBX_RC_OK;
}


/**
 * FUNCTION: Called when xptReceiveData() is called
 *
 *  Read data
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 *
 *     buffer, passed directly from the xptReceiveData() call.
 *
 *     bufferLen, passed directly from the xptReceiveData() call.
 *
 * OUT:
 *     dataLen, passed directly from the xptReceiveData() call.
 */
Ret_t XPTAPI obxReceiveData(void *privateConnectionInfo,
                            void *buffer,
                            size_t bufferLen,
                            size_t *dataLen) {

   ObxConnectionBlock *ocb = (ObxConnectionBlock *)privateConnectionInfo;
   int   length = 0;

   XPTDEBUG(("OBX obxReceiveData()\n"));

   CHECK_PARM( privateConnectionInfo, "obxReceiveData()", "privateConnectionInfo" );
   CHECK_PARM( buffer,                "obxReceiveData()", "buffer" );

   if ( ocb->dataToRead == NULL  ) {  // no data to read yet...
      obxRecordError( OBX_ERRORMSG_BUFFER_LENGTH, bufferLen, "obxReceiveData()" );
      return OBX_RC_OBEX_DATA_LENGTH;
   } // end if

   // Can we satisfy the read?
   if ( bufferLen > ocb->dataToRead->length ) {
      // Less data on hand, than they are requesting.
      obxRecordError( OBX_ERRORMSG_BUFFER_LENGTH, bufferLen, "obxReceiveData()" );
      return OBX_RC_OBEX_DATA_LENGTH;
   } // end if

   length   = ( bufferLen < ocb->dataToRead->length ) ? bufferLen : ocb->dataToRead->length;
   *dataLen = length;

   if ( length > 0 ) {
      COPY_MEM( ocb->dataToRead->cursor, buffer, length );
      ocb->dataToRead->length -= length;
      ocb->dataToRead->cursor += length;
   } // end if

    return OBX_RC_OK;
}


/**
 * FUNCTION: Called when xptSetDocumentInfo() is called
 *
 *  Provide document information for an outgoing document.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 *
 *     pDoc, passed directly from the xptSetDocumentInfo() call.
 *
 * Note: It is possible that the transport not known the document length
 *       beforehand.  In this case, the document length will be set to
 *       (size_t) -1 by the caller of xptSetDocumentInfo().
 */
Ret_t XPTAPI obxSetDocumentInfo( void *privateConnectionInfo,
                                 const XptCommunicationInfo_t *pDoc ) {

   ObxConnectionBlock     *ocb       = (ObxConnectionBlock *)privateConnectionInfo;
   XptCommunicationInfo_t *docInfo   = NULL;
   XptHmacInfo_t          *pHmacInfo = pDoc->hmacInfo;

   XPTDEBUG(("OBX obxSetDocumentInfo()\n"));
   CHECK_PARM( privateConnectionInfo, "obxSetDocumentInfo()", "privateConnectionInfo" );
   CHECK_PARM( pDoc, "obxSetDocumentInfo()", "pDoc" );

   docInfo = (XptCommunicationInfo_t *)ALLOC_MEM( sizeof( XptCommunicationInfo_t ) );
   CHECK_ALLOC( docInfo, "obxSetDocumentInfo()", "docInfo" );

   strcpy( docInfo->docName, pDoc->docName );
   strcpy( docInfo->mimeType, pDoc->mimeType );

//Mickey 2003.1.14
   docInfo->cbSize = pDoc->cbSize;
   docInfo->cbLength = pDoc->cbLength;
   docInfo->hmacInfo = NULL;
   ocb->docInfo = docInfo;

   if (pHmacInfo != NULL) 
   {
        char *copy  = NULL;
        int  length = 0;

        if ((pHmacInfo->username != NULL) && (pHmacInfo->mac != NULL))
        {
            length = strlen("algorithm=") 
                   + strlen( (pHmacInfo->algorithm == NULL) ? "MD5" : pHmacInfo->algorithm)
                   + strlen(", username=\"") + strlen (pHmacInfo->username)
                   + strlen("\", mac=") + strlen (pHmacInfo->mac);

            copy = (char *)ALLOC_MEM( length + 1 );
        }

        if (copy) // we cannot use CHECK_ALLOC since we have to free docInfo before returning
        {
            sprintf(copy, "algorithm=%s, username=\"%s\", mac=%s", 
                (pHmacInfo->algorithm == NULL) ? "MD5" : pHmacInfo->algorithm, 
                pHmacInfo->username, pHmacInfo->mac);
            
            ocb->pchXSyncmlHmac = copy;   
        }
        else
        {
            FREE_MEM(docInfo);
            return OBX_RC_MEMORY_NULL_PNTR;
        }
        XPTDEBUG(("OBX obxSetDocumentInfo() hmac: '%s'\n", ocb->pchXSyncmlHmac ));
   }
   XPTDEBUG(("OBX obxSetDocumentInfo() name: '%s' mime: '%s' length: '%d'\n",
             docInfo->docName, docInfo->mimeType, docInfo->cbLength));

   return OBX_RC_OK;
}


/**
 * FUNCTION: Called when xptSendData() is called
 *
 *  Send data
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 *
 *     buffer, passed directly from the xptSendData() call.
 *
 *     bufferLen, passed directly from the xptSendData() call.
 *
 *     lastBlock, passed directly from the xptSendData() call.
 */
Ret_t XPTAPI obxSendData(void *privateConnectionInfo,
                         const void *dataBuffer,
                         size_t bufferLen,
                         size_t *bytesSent ) {

   XPTDEBUG(("OBX obxSendData()\n"));

   CHECK_PARM( privateConnectionInfo, "obxSendData()", "privateConnectionInfo" );
   CHECK_PARM( bytesSent,             "obxSendData()", "bytesSent" );
   CHECK_PARM( dataBuffer,            "obxSendData()", "dataBuffer" );

   *bytesSent = bufferLen; // We always take all they can give.

   return obxQueueBufferForSend( (ObxConnectionBlock *)privateConnectionInfo, dataBuffer, bufferLen );
}

/**
 * FUNCTION: Called when xptSendData() is called by the application, after
 *           sending the last byte of the document.
 *
 *  Complete send processing for an outgoing document.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 *
 * NOTES:
 *
 *  The xpt interface layer counts the number of bytes the application
 *  writes using the xptSendData() function.  When the transport
 *  implementation has written the last byte of the document, the xpt
 *  interface layer calls this function in the transport implementation to
 *  allow it to perform any desired completion processing.  The length of
 *  the document is known because it was specified by the application in the
 *  xptSetDocumentInfo() call.
 *
 *  Any error returned from sendComplete() is returned to the application
 *  as the result value of the application's call to xptSendData().
 *
 *  Note that this function call does NOT correspond to an xptSendComplete()
 *  function call available to the application.  Instead, it is called
 *  automatically by the xpt interface layer when the application has
 *  successfully written the last byte of the document.
 */

Ret_t XPTAPI obxSendComplete( void *privateConnectionInfo ) {

   ObxObject *object;
   ObxHeader *header;
   ObxBuffer *buffer;

    ObxObject *response = NULL;

    int   length = 0;
   Ret_t rc = OBX_RC_OK;

   ObxConnectionBlock *ocb = (ObxConnectionBlock *)privateConnectionInfo;

   XPTDEBUG(("OBX obxSendComplete()\n"));

   CHECK_PARM( privateConnectionInfo, "obxSendComplete()", "privateConnectionInfo" );

    /* check wether we are server beeing pulled with get or client pushing data with PUT */
    if (ocb->role == XPT_REQUEST_SENDER) {
      object = ObxObjectNew( ocb->obxHandle, OBEX_CMD_PUT );
    } else if (ocb->role == XPT_REQUEST_RECEIVER) {
//Every response of Get command must have final bit set.
//        object = ObxObjectNew( ocb->obxHandle, OBEX_RSP_CONTINUE );
        object = ObxObjectNew( ocb->obxHandle, OBEX_RSP_CONTINUE | OBEX_CMD_FINAL );
    } else {
        /* error - bail out! */
        return OBX_RC_ERR_NOTINITIALIZED;
    }
   if ( object == NULL ) {
        return OBX_RC_OBEX_HEADER;
    } // end if
    
//The following headers are needed only when OBEX client is make a PUT request.
    if (ocb->role == XPT_REQUEST_SENDER) {
//We must embed ConnectionID header in Put command.
        header = ObxHeaderNew( OBEX_HEADER_CONNECTION );
        rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
        if ( rc == OBX_RC_OK ) {
            XPTDEBUG(("OBX obxSendComplete() Add header '%s' value '%x'\n", "OBEX_HEADER_CONNECTION", ocb->obxHandle->OBEXConnectionID ));
            rc = ObxHeaderSetIntValue( header, ocb->obxHandle->OBEXConnectionID );
            if ( rc == OBX_RC_OK ) {
                rc = ObxObjectAddHeader( ocb->obxHandle, object, header );
                if ( rc == OBX_RC_OK ) {
                    header = NULL; // we no longer own the header
                } // end if
            } // end if
        } // end if
    
        // Clean up from an error
        if ( rc != OBX_RC_OK ) {
            obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendComplete()", "OBEX_HEADER_CONNECTION" );
            if ( header != NULL ) {
                ObxHeaderFree( header );
    } // end if
            if ( object != NULL ) {
                ObxObjectFree( ocb->obxHandle, object );
            } // end if
            return OBX_RC_OBEX_HEADER;
        } // end if

//These headers below are unnecessary in Put command.
/*    // Add header 'HEADER_CREATOR_ID'  (We do this for the palm we *may* be talking to)
   XPTDEBUG(("OBX obxSendComplete() Add header '%s' value '%x'\n", "HEADER_CREATOR_ID", (unsigned int)creatorId ));
   header = ObxHeaderNew( OBEX_HEADER_CREATORID );
   rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
   if ( rc == OBX_RC_OK ) {
      rc = ObxHeaderSetIntValue( header, creatorId );
      if ( rc == OBX_RC_OK ) {
         rc = ObxObjectAddHeader( ocb->obxHandle, object, header );
         if ( rc == OBX_RC_OK ) {
            header = NULL; // we no longer own the header
         } // end if
      } // end if
   } // end if

   // Clean up from an error
   if ( rc != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendComplete()", "OBEX_HEADER_CREATORID" );
      if ( header != NULL ) {
         ObxHeaderFree( header );
      } // end if
      if ( object != NULL ) {
         ObxObjectFree( ocb->obxHandle, object );
      } // end if
      return OBX_RC_OBEX_HEADER;
   } // end if
   */
   
   /*
   // %%% luz: temporarily re-enabled NAME header as we needed it for session ID (RespURI),
               but that's not standard-like.

   // Add header 'NAME'
   header = ObxHeaderNew( OBEX_HEADER_NAME );
   rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
   if ( rc == OBX_RC_OK ) {
      XPTDEBUG(("OBX obxSendComplete() Add header '%s' value '%s'\n", "OBEX_HEADER_NAME", ocb->docInfo->docName ));
      length = strlen( ocb->docInfo->docName ) + 1;
      // luz %%% cast added
      buffer = ObxUTF8ToUnicode( (const unsigned char *)ocb->docInfo->docName, length );
      rc = ( buffer == NULL ) ? OBX_RC_MEMORY_ERROR : OBX_RC_OK;
      if ( rc == OBX_RC_OK ) {
         rc = ObxHeaderSetUnicodeValue( header, buffer );
         if ( rc == OBX_RC_OK ) {
            buffer = NULL;  // we no longer own the buffer
            rc = ObxObjectAddHeader( ocb->obxHandle, object, header );
            if ( rc == OBX_RC_OK ) {
               header = NULL; // we no longer own the header
            } // end if
         } // end if
      } // end if
   } // end if
  
   // Clean up from an error
   if ( rc != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendComplete()", "OBEX_HEADER_NAME" );
      if ( buffer != NULL ) {
         ObxBufFree( buffer );
      } // end if
      if ( header != NULL ) {
         ObxHeaderFree( header );
      } // end if
      if ( object != NULL ) {
         ObxObjectFree( ocb->obxHandle, object );
      } // end if
      return OBX_RC_OBEX_HEADER;
   } // end if
   */
    
//We must embed Type header in Put command.
  // Add header 'MIME_TYPE'
  XPTDEBUG(("OBX obxSendComplete() Add header '%s' value '%s'\n", "OBEX_HEADER_TYPE", ocb->docInfo->mimeType ));
  header = ObxHeaderNew( OBEX_HEADER_TYPE );
  rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
  if ( rc == OBX_RC_OK ) {
    length = strlen( ocb->docInfo->mimeType ) + 1;
    buffer = ObxBufNew( length );
    rc = ( buffer == NULL ) ? OBX_RC_MEMORY_ERROR : OBX_RC_OK;
    if ( rc == OBX_RC_OK ) {
      ObxBufWrite( buffer, ocb->docInfo->mimeType, length );
      rc = ObxHeaderSetByteSequenceValue( header, buffer );
      if ( rc == OBX_RC_OK ) {
        buffer = NULL;  // we no longer own the buffer
        rc = ObxObjectAddHeader( ocb->obxHandle, object, header );
        if ( rc == OBX_RC_OK ) {
          header = NULL; // we no longer own the header
        } // end if
      } // end if
     } // end if
   } // end if
    
   // Clean up from an error
   if ( rc != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendComplete()", "OBEX_HEADER_TYPE" );
      if ( buffer != NULL ) {
          ObxBufFree( buffer );
      } // end if
      if ( header != NULL ) {
         ObxHeaderFree( header );
      } // end if
      if ( object != NULL ) {
         ObxObjectFree( ocb->obxHandle, object );
      } // end if
      return OBX_RC_OBEX_HEADER;
   } // end if
 } // end if (ocb->role == XPT_REQUEST_SENDER)

  // Add header 'LENGTH'
  XPTDEBUG(("OBX obxSendComplete() Add header '%s' value '%d'\n", "OBEX_HEADER_LENGTH", ocb->docInfo->cbLength ));
  header = ObxHeaderNew( OBEX_HEADER_LENGTH );
  rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
  if ( rc == OBX_RC_OK ) {
      rc = ObxHeaderSetIntValue( header, ocb->docInfo->cbLength );
      if ( rc == OBX_RC_OK ) {
            rc = ObxObjectAddHeader( ocb->obxHandle, object, header );
            if ( rc == OBX_RC_OK ) {
              header = NULL; // we no longer own the header
            } // end if
      } // end if
  } // end if

  if ( rc != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendComplete()", "OBEX_HEADER_LENGTH" );
      if ( header != NULL ) {
         ObxHeaderFree( header );
      } // end if
      if ( object != NULL ) {
         ObxObjectFree( ocb->obxHandle, object );
      } // end if
      return OBX_RC_OBEX_HEADER;
  } // end if
    
//These headers below are unnecessary in Put command.
/*    // Add header 'DESCRIPTION'
   XPTDEBUG(("OBX obxSendComplete() Add header '%s' value '%s'\n", "OBEX_HEADER_DESCRIPTION", ocb->docInfo->mimeType ));
   header = ObxHeaderNew( OBEX_HEADER_DESCRIPTION );
   rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
   if ( rc == OBX_RC_OK ) {
      length = strlen( ocb->docInfo->mimeType ) + 1;
      // luz %%%: cast added
      buffer = ObxUTF8ToUnicode( (const unsigned char *)ocb->docInfo->mimeType, length );
      rc = ( buffer == NULL ) ? OBX_RC_MEMORY_ERROR : OBX_RC_OK;
      if ( rc == OBX_RC_OK ) {
         rc = ObxHeaderSetUnicodeValue( header, buffer );
         if ( rc == OBX_RC_OK ) {
            buffer = NULL;  // we no longer own the buffer
            rc = ObxObjectAddHeader( ocb->obxHandle, object, header );
            if ( rc == OBX_RC_OK ) {
               header = NULL; // we no longer own the header
            } // end if
         } // end if
      } // end if
   } // end if

   // Clean up from an error
   if ( rc != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendComplete()", "OBEX_HEADER_DESCRIPTION" );
      if ( buffer != NULL ) {
         ObxBufFree( buffer );
      } // end if
      if ( header != NULL ) {
         ObxHeaderFree( header );
      } // end if
      if ( object != NULL ) {
         ObxObjectFree( ocb->obxHandle, object );
      } // end if
      return OBX_RC_OBEX_HEADER;
   } // end if

    // Add header 'XSYNCMLHMAC'
    if (ocb->pchXSyncmlHmac != NULL)
    {
        header = ObxHeaderNew( OBEX_HEADER_XSYNCMLHMAC );
        rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
        if ( rc == OBX_RC_OK ) {
            XPTDEBUG(("OBX obxSendComplete() Add header '%s' value '%s'\n", "OBEX_HEADER_XSYNCMLHMAC", ocb->pchXSyncmlHmac ));
            length = strlen( ocb->pchXSyncmlHmac ) + 1;
            buffer = ObxUTF8ToUnicode( (const unsigned char *)ocb->pchXSyncmlHmac, length ); // %%% luz 2002-04-16: added cast
            rc = ( buffer == NULL ) ? OBX_RC_MEMORY_ERROR : OBX_RC_OK;
            if ( rc == OBX_RC_OK ) {
                rc = ObxHeaderSetUnicodeValue( header, buffer );
                if ( rc == OBX_RC_OK ) {
                    buffer = NULL;  // we no longer own the buffer
                    rc = ObxObjectAddHeader( ocb->obxHandle, object, header );
                    if ( rc == OBX_RC_OK ) {
                        header = NULL; // we no longer own the header
                    } // end if
                } // end if
            } // end if
        } // end if
    
        // Clean up from an error
        if ( rc != OBX_RC_OK ) {
            obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendComplete()", "OBEX_HEADER_NAME" );
            if ( buffer != NULL ) {
                ObxBufFree( buffer );
            } // end if
            if ( header != NULL ) {
                ObxHeaderFree( header );
            } // end if
            if ( object != NULL ) {
                ObxObjectFree( ocb->obxHandle, object );
            } // end if
            return OBX_RC_OBEX_HEADER;
        } // end if
    } // end if (Header XSYNCMLHMAC)
    */
   // Add header 'BODY'
   XPTDEBUG(("OBX obxSendComplete() Add header '%s'\n", "OBEX_HEADER_BODY" ));
   header = ObxHeaderNew( OBEX_HEADER_BODY );
   rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
   if ( rc == OBX_RC_OK ) {
      length = obxGetBufferForSend( ocb, &buffer );
      rc = ( length > 0 ) ? OBX_RC_OK : OBX_RC_OBEX_HEADER;
      if ( rc == OBX_RC_OK ) {
         rc = ObxHeaderSetByteSequenceValue( header, buffer );
         if ( rc == OBX_RC_OK ) {
            buffer = NULL;  // we no longer own the buffer
            rc = ObxObjectAddHeader( ocb->obxHandle, object, header );
            if ( rc == OBX_RC_OK ) {
               header = NULL; // we no longer own the header
            } // end if
         } // end if
      } // end if
   } // end if

   // Clean up from an error
   if ( rc != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendComplete()", "OBEX_HEADER_BODY" );
      if ( buffer != NULL ) {
         ObxBufFree( buffer );
      } // end if
      if ( header != NULL ) {
         ObxHeaderFree( header );
      } // end if
      if ( object != NULL ) {
         ObxObjectFree( ocb->obxHandle, object );
      } // end if
      return OBX_RC_OBEX_HEADER;
   } // end if

    
    if (ocb->role == XPT_REQUEST_SENDER) {
        /* we are client -> use PUT */
        if ( ( response = ObxObjectNew( ocb->obxHandle, OBEX_CMD_NULL ) ) == NULL ) {
      rc = OBX_RC_GENERAL_ERROR;
   } // end else if
   // Flow obxPUT
   else if ( ObxTransactionSend( ocb->obxHandle, object, response ) != OBX_RC_OK ) {
      rc = OBX_RC_GENERAL_ERROR;
   } // end else if
   // Determine if obxPUT successful
   else if ( response->cmd != ( OBEX_CMD_FINAL + OBEX_RSP_SUCCESS ) ) {
      rc = OBX_RC_GENERAL_ERROR;
   } // end else if
        
    } else if (ocb->role == XPT_REQUEST_RECEIVER) {
        /* we are server -> wait for GET */
        rc = ObxGetSend(ocb->obxHandle, object, ocb->docInfo->mimeType);
    }

   // Clean any storage that remains allocated
   if ( response != NULL ) {
      ObxObjectFree( ocb->obxHandle, response );
   } // end if
   if ( buffer != NULL ) {
      ObxBufFree( buffer );
   } // end if
   if ( header != NULL ) {
      ObxHeaderFree( header );
   } // end if
   if ( object != NULL ) {
      ObxObjectFree( ocb->obxHandle, object );
   } // end if

   // We're done with docInfo now.. clear it so it can be used to receive data.
   if ( ocb->docInfo ) {
        FREE_MEM( ocb->docInfo->hmacInfo );
        FREE_MEM( ocb->docInfo );
    } // end if
    // since pchSyncmlHmac belongs to docInfo, clear it too
    if ( ocb->pchXSyncmlHmac ) {
        FREE_MEM( ocb->pchXSyncmlHmac );
   } // end if

   return rc;
}


/*********************************************************************************/
/***************************** Private Functions *********************************/
/*********************************************************************************/

/*
** Add a connection to sequence in the provided service object.
** Ignores call if object already in the chain.
*/

// **MHB** Where is the comparison to check if object already in chain?
// **MHB** Investigate using ObxList instead of ObxSequenceNode.

static Ret_t obxAddConnection( ObxServiceBlock *service,
                               ObxConnectionBlock *connection ) {
   ObxSequenceNode *cursor;

   XPTDEBUG(("OBX obxAddConnection()\n"));

   CHECK_PARM( service, "obxAddConnection()", "service" );
   CHECK_PARM( connection, "obxAddConnection()", "connection" );

   cursor = (ObxSequenceNode *)ALLOC_MEM( sizeof( ObxSequenceNode ) );
   CHECK_ALLOC( cursor, "obxAddConnection()", "cursor" );

   cursor->node = connection;
   cursor->previous = NULL;
   cursor->next = service->connections;

   if ( service->connections && service->connections->next ) {
      service->connections->next->previous = cursor;
   }

   service->connections = cursor;

   return OBX_RC_OK;
}

/*
** Removes the connection from it's service object (it's ref'ed from con block).
** Any associated storage used to keep it in sequence within the service block
** is free'ed but it's the callers responsibility to free the connection block itself.
** Returns null if connection block is not found within the passed service.
*/

// **MHB** Investigate using ObxList instead of ObxSequenceNode.

static ObxConnectionBlock *obxRemoveConnection( ObxConnectionBlock *connection ) {

   ObxSequenceNode *cursor;
   ObxSequenceNode *next;

   ObxServiceBlock    *service = NULL;
   ObxConnectionBlock *ocb     = NULL;

   XPTDEBUG(("OBX obxRemoveConnection()\n"));

   if ( connection ) {
      service = connection->service;
      if ( service ) {
         cursor = service->connections;
         // Yea.. yea.. linear search.. but there shouldn't be
         // many connections anyway.
         while ( cursor && !ocb ) {
            next = cursor->next;
            if ( cursor->node == connection ) {
               ocb = connection;
               cursor->node = NULL;
               if ( cursor->next ) {   // Anybody down stream?
                  cursor->next->previous = cursor->previous;
               }
               if ( cursor->previous ) {  // Anybody up stream?
                  cursor->previous->next = cursor->next;
               }
               if ( cursor == service->connections ) {   // First on list?
                  service->connections = cursor->next;
               }
               FREE_MEM( cursor );
            }
            cursor = next;
         }
      }
   } else {
      // quiet
   }
   return ocb;
}

/*
** Wacks all storage associated with the passed connection construct.
** Note that it's up to the caller to ensure that the passed object has
** been removed from any ObxServiceBlock's.
*/

static Ret_t obxFreeConnection( ObxConnectionBlock *ocb ) {

   XPTDEBUG(("OBX obxFreeConnection()\n"));

   if ( ocb ) {

      obxResetConnection( ocb );

      if ( ( ocb->service->flags & XPT_CLIENT ) == XPT_CLIENT ) {
         if ( ocb->obxHandle ) {
            ObxHandleFree( ocb->obxHandle );
         } // end if
      } // end if

      FREE_MEM( ocb );
   } // end if

   return OBX_RC_OK;
}

static Ret_t obxResetConnection( ObxConnectionBlock *ocb ) {

   ObxSequenceNode *cursor = NULL;
   ObxSequenceNode *next   = NULL;
   ObxBuf          *obBuf  = NULL;

   XPTDEBUG(("OBX obxResetConnection()\n"));

   if ( ocb ) {
      if ( ocb->docInfo ) {
         FREE_MEM( ocb->docInfo->hmacInfo );
         FREE_MEM( ocb->docInfo );
      }

      if ( ocb->pchXSyncmlHmac ) {
         FREE_MEM (ocb->pchXSyncmlHmac );
      }

      if ( ocb->dataToRead ) {
         FREE_MEM( ocb->dataToRead->buf );
         FREE_MEM( ocb->dataToRead );
      }

      cursor = ocb->dataToSend;
      while ( cursor ) {
         next = cursor->next;
         obBuf = (ObxBuf *)cursor->node;
         if ( obBuf ) {
            FREE_MEM( obBuf->buf );
            FREE_MEM( obBuf );
         }
         FREE_MEM( cursor );
         cursor = next;
      }

      ocb->docInfo        = NULL;
      ocb->pchXSyncmlHmac = NULL;
      ocb->dataToRead     = NULL;
      ocb->dataToSend     = NULL;

   } // end if

   return OBX_RC_OK;

} // obxResetConnection()


/*
** Wacks all storage associated with the passed service construct.
** Including the storage in the passed construct.
*/

// **MHB** Investigate using ObxList instead of ObxSequenceNode

static Ret_t obxFreeService( ObxServiceBlock *block ) {
   ObxSequenceNode *cursor;
   ObxSequenceNode *next;

   XPTDEBUG(("OBX obxFreeService()\n"));

   if ( block ) {
      cursor = block->connections;
      while ( cursor ) {
         next = cursor->next;
         obxFreeConnection( (ObxConnectionBlock *)cursor->node );
         FREE_MEM( cursor );
         cursor = next;
      }
      FREE_MEM( block->serviceName );
      FREE_MEM( block->metadata );
      FREE_MEM( block );
   } else {
      // quiet
   }
   return OBX_RC_OK;
}

/*
**
** Register/Initialize/Open underlying Transport
**
*/
static Ret_t obxRegisterTransport( ObxConnectionBlock *ocb ) {

   ObxRc rc;

   XPTDEBUG(("OBX obxRegisterTransport()\n"));

   if ( ocb->obxHandle == NULL ) {
      // Create OBEX Handle
      ocb->obxHandle = ObxHandleNew();
      if ( ocb->obxHandle == NULL ) {
         obxRecordError( OBX_ERRORMSG_OBEX_TRANSPORT_FAILURE, "OBEX Init" );
         return OBX_RC_OBEX_TRANS_FAILURE;
      } // end if
   } // end if

   switch ( ocb->service->transport->obxTransType ) {
      case OBEX_TRANS_INET:
         rc = ObxTransportRegister( ocb->obxHandle, ObxTransportGet( DEFINED_TRANSPORT_INET ) );
         if ( rc != OBX_RC_OK ) {
            return OBX_RC_OBEX_TRANS_FAILURE;
         } // end if
         break;
      case OBEX_TRANS_IRDA:
         rc = ObxTransportRegister( ocb->obxHandle, ObxTransportGet( DEFINED_TRANSPORT_IRDA ) );
         if ( rc != OBX_RC_OK ) {
            return OBX_RC_OBEX_TRANS_FAILURE;
         } // end if
         break;
      default:
         rc = OBX_RC_OBEX_TRANS_FAILURE;
   } // end switch

   if ( ObxTransportInitialize( ocb->obxHandle, ocb->service->metadata ) != OBX_RC_OK ) {
      return OBX_RC_OBEX_TRANS_FAILURE;
   } // end if

   if ( ObxTransportOpen( ocb->obxHandle ) != OBX_RC_OK ) {
      return OBX_RC_OBEX_TRANS_FAILURE;
   } // end if

   // distinguish between client XPT_REQUEST_SENDER) and server (XPT_REQUEST_RECEIVER)
   switch ( ocb->role ) {
      case XPT_REQUEST_RECEIVER:
         // server - listen for a client request
   if ( ObxTransportListen( ocb->obxHandle ) != OBX_RC_OK ) {
      return OBX_RC_OBEX_TRANS_FAILURE;
   } // end if
         break;
      case XPT_REQUEST_SENDER:
         // client - nothing more to do here
         break;
      default:
          XPTDEBUG(("OBX obxRegisterTransport() [ERROR] unknown role speciefied - bailing out\n"));
          return OBX_RC_ERR_TRANSPORT;
   } // end switch

   return OBX_RC_OK;

} // obxRegisterTransport()

/*
** Register the TCP Obex binding.
*/
static Ret_t obxRegisterTcpObex() {
   struct xptTransportDescription xptBlock;
    ObxTransportBlock *tcpTransportBlock;

   XPTDEBUG(("OBX obxRegisterTcpObex()\n"));

   xptBlock.shortName                 = "OBEX/TCP";
   xptBlock.description               = "OBEX transport over TCP";

   xptBlock.flags                     = XPT_CLIENT | XPT_SERVER;

   xptBlock.selectProtocol            = obxSelectProtocol;
   xptBlock.deselectProtocol          = obxDeselectProtocol;
   xptBlock.openCommunication         = obxOpenCommunication;
   xptBlock.closeCommunication        = obxCloseCommunication;
   xptBlock.beginExchange             = obxBeginExchange;
   xptBlock.endExchange               = obxEndExchange;
   xptBlock.receiveData               = obxReceiveData;
   xptBlock.sendData                  = obxSendData;
   xptBlock.sendComplete              = obxSendComplete;
   xptBlock.setDocumentInfo           = obxSetDocumentInfo;
   xptBlock.getDocumentInfo           = obxGetDocumentInfo;



   // LEO:
   xptBlock.cancelCommAsync			  = NULL;




   tcpTransportBlock = (ObxTransportBlock*)ALLOC_MEM( sizeof(ObxTransportBlock) );
   CHECK_ALLOC( tcpTransportBlock, "obxRegisterTcpObex()", "tcpTransportBlock" );

   xptBlock.privateTransportInfo      = tcpTransportBlock;
   tcpTransportBlock->obxTransType    = OBEX_TRANS_INET;

   return xptRegisterTransport( &xptBlock );
}

/*
** Register the IR Obex binding.
*/
static Ret_t obxRegisterIrObex() {
   struct xptTransportDescription xptBlock;
    ObxTransportBlock *irTransportBlock;

   XPTDEBUG(("OBX obxRegisterIrObex()\n"));

   xptBlock.shortName                 = "OBEX/IR";
   xptBlock.description               = "OBEX transport over IR";

   xptBlock.flags                     = XPT_CLIENT | XPT_SERVER;

   xptBlock.selectProtocol            = obxSelectProtocol;
   xptBlock.deselectProtocol          = obxDeselectProtocol;
   xptBlock.openCommunication         = obxOpenCommunication;
   xptBlock.closeCommunication        = obxCloseCommunication;
   xptBlock.beginExchange             = obxBeginExchange;
   xptBlock.endExchange               = obxEndExchange;
   xptBlock.receiveData               = obxReceiveData;
   xptBlock.sendData                  = obxSendData;
   xptBlock.sendComplete              = obxSendComplete;
   xptBlock.setDocumentInfo           = obxSetDocumentInfo;
   xptBlock.getDocumentInfo           = obxGetDocumentInfo;


   // LEO:
   xptBlock.cancelCommAsync			  = NULL;



   irTransportBlock = (ObxTransportBlock*)ALLOC_MEM( sizeof(ObxTransportBlock) );
   CHECK_ALLOC( irTransportBlock, "obxRegisterIrObex()", "irTransportBlock" );

   xptBlock.privateTransportInfo      = irTransportBlock;
   irTransportBlock->obxTransType     = OBEX_TRANS_IRDA;

   return xptRegisterTransport( &xptBlock );
}

/*
** Flow obex handshaking for client mode
*/
static Ret_t obxInitializeForClientMode( ObxConnectionBlock *ocb ) {

   XPTDEBUG(("OBX obxInitializeForClientMode()\n"));

   obxResetConnection( ocb );

   // Connect to remote peer
   if ( ObxTransportConnect( ocb->obxHandle ) != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_TRANSPORT_FAILURE, "ObxTransportConnect()" );
      return OBX_RC_OBEX_INIT_FAILURE;
   } // end if

   // Flow OBEX CONNECT to server
   if ( obxSendObexConnect( ocb ) != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_TRANSPORT_FAILURE, "obxSendObexConnect()" );
      obxResetConnection( ocb );
      return OBX_RC_OBEX_INIT_FAILURE;
   } // end if

   return OBX_RC_OK;
}

/*
** Flow obex handshaking for server mode
** Really this means we put up an accept and block.
*/
static Ret_t obxInitializeForServerMode( ObxConnectionBlock *ocb ) {
   ObxObject *response = NULL;

   XPTDEBUG(("OBX obxInitializeForServerMode()\n"));

   obxResetConnection( ocb );

   XPTDEBUG(("OBX Putting up server accept()...\n"));

   if ( ObxTransportAccept( ocb->obxHandle ) != OBX_RC_OK ) {
      obxRecordError( OBX_ERRORMSG_OBEX_TRANSPORT_FAILURE, "ObxTransportAccept()" );
      return OBX_RC_SERVER_ACCEPT_FAILED;
   } // end if


   response = obxWaitForObexResponse( ocb );
   if ( ( response == NULL ) || ( response->cmd != OBEX_CMD_CONNECT ) ) {
       ObxObjectFree( ocb->obxHandle, response );
       return OBX_RC_GENERAL_ERROR;
   } // end if

   ObxObjectFree( ocb->obxHandle, response );
   return OBX_RC_OK;
}


/*
** Queue the inbound buffer for a later send.
*/
static Ret_t obxQueueBufferForSend( ObxConnectionBlock *connection,
                                    const void *buffer, size_t bufferLen ) {

   ObxBuf          *oBuf;
   ObxSequenceNode *cursor;
   ObxSequenceNode *newNode;

   Ret_t rc = OBX_RC_OK;

   XPTDEBUG(("OBX obxQueueBufferForSend()\n"));

   oBuf = (ObxBuf *)ALLOC_MEM( sizeof( ObxBuf ) );
   CHECK_ALLOC( oBuf, "obxQueueBufferForSend()", "oBuf" );

   oBuf->buf = (unsigned char *)ALLOC_MEM( bufferLen );
   oBuf->cursor = oBuf->buf;
   oBuf->length = bufferLen;
   COPY_MEM( buffer, oBuf->buf, bufferLen );

   newNode = (ObxSequenceNode *)ALLOC_MEM( sizeof( ObxSequenceNode ) );
   if ( !newNode ) {
      obxRecordError( OBX_ERRORMSG_MEMORY_ALLOC, "obxQueueBufferForSend()", "newNode" );
      FREE_MEM( oBuf );
      rc = OBX_RC_MEMORY_ERROR_ALLOC;
   } else {
      newNode->next = NULL;
      newNode->previous = NULL;
      newNode->node = oBuf;

      // Attach
      if ( !connection->dataToSend ) {
         // First one
         connection->dataToSend = newNode;
      } else {
         // Find end.
         cursor = connection->dataToSend;
//Mickey 2003.1.14
//         while ( !cursor->next ) {
         while ( cursor->next ) {
            cursor = cursor->next;
         }
         newNode->previous = cursor;
         cursor->next = newNode;
      }
   }
   return rc;
}

/*
** Form a single buffer with all data to send.  Assign buffer to body.
** Caller is responsible for free() of returned 'body' buffer.
** Returns buffer length if successful, 0 if not.
*/

// **MHB** Investigate using ObxList instead of ObxSequenceNode

static int obxGetBufferForSend( ObxConnectionBlock *connection, ObxBuffer **body ) {

   ObxBuffer         *buffer = NULL;
   ObxSequenceNode   *cursor = NULL;

   size_t            length  = 0;

   XPTDEBUG(("OBX obxGetBufferForSend()\n"));

   CHECK_PARM( connection, "obxGetBufferForSend()", "connection" );
   CHECK_PARM( body, "obxQueueBufferForSend()", "body" );

   // Calculate data length
   cursor = connection->dataToSend;
   while ( cursor != NULL ) {
      length += ((ObxBuf *)(cursor->node))->length;
      cursor  = cursor->next;
   } // end while

   // Create a new buffer
   buffer = ObxBufNew( length );
   if ( buffer == NULL ) {
      obxRecordError( OBX_ERRORMSG_MEMORY_ALLOC, "obxGetBufferForSend()", "buffer" );
      return 0;
   } // end if

   // Build buffer
   cursor = connection->dataToSend;
   while ( cursor != NULL ) {
      ObxBufWrite( buffer, (void *)((ObxBuf *)(cursor->node))->buf, ((ObxBuf *)(cursor->node))->length );
      cursor = cursor->next;
   } // end while

   *body = buffer;

   return length;
}


static Ret_t obxSendObexConnect( ObxConnectionBlock *ocb ) {
   ObxObject *request;
   ObxObject *response;
   ObxRc rc;
   ObxHeader *header;
   ObxBuffer *buffer;
   ObxIterator    *iterator;
   char who_buffer[sizeof(SYNCML_TARGET)];
   int connectionIDHeaderCount=0, whoHeaderCount=0;
   XPTDEBUG(("OBX obxSendObexConnect()\n"));

   request  = ObxObjectNew( ocb->obxHandle, OBEX_CMD_CONNECT );
   response = ObxObjectNew( ocb->obxHandle, OBEX_CMD_NULL );

   rc = ( ( request != NULL ) && ( response != NULL ) ) ? OBX_RC_OK : OBX_RC_GENERAL_ERROR;
   if ( rc == OBX_RC_OK ) {
//We must embed Target header in Connect command.
      header = ObxHeaderNew( OBEX_HEADER_TARGET );
      rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
      if ( rc == OBX_RC_OK ) {
          XPTDEBUG(("OBX obxSendObexConnect() Add header '%s' value '%s'\n", "OBEX_HEADER_TARGET", SYNCML_TARGET ));
          buffer = ObxBufNew( strlen(SYNCML_TARGET) );
          rc = ( buffer == NULL ) ? OBX_RC_MEMORY_ERROR : OBX_RC_OK;
          if ( rc == OBX_RC_OK ) {
              ObxBufWrite( buffer, SYNCML_TARGET, strlen(SYNCML_TARGET) );
              rc = ObxHeaderSetByteSequenceValue(header,buffer);

              if ( rc == OBX_RC_OK ) {
                  buffer = NULL;  // we no longer own the buffer
                  rc = ObxObjectAddHeader( ocb->obxHandle, request, header );
                  if ( rc == OBX_RC_OK ) {
                      header = NULL; // we no longer own the header
                  } // end if
              } // end if
          } // end if
      } // end if
    
      // Clean up from an error
      if ( rc != OBX_RC_OK ) {
          obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendObexConnect()", "OBEX_HEADER_TARGET" );
          if ( buffer != NULL ) {
              ObxBufFree( buffer );
          } // end if
          if ( header != NULL ) {
              ObxHeaderFree( header );
          } // end if
          if ( request != NULL ) {
              ObxObjectFree( ocb->obxHandle, request );
          } // end if
          return OBX_RC_OBEX_HEADER;
      } // end if

      rc  = ObxTransactionSend( ocb->obxHandle, request, response );
      if ( rc == OBX_RC_OK ) {
         rc = ( response->cmd == ( OBEX_CMD_FINAL + OBEX_RSP_SUCCESS ) ) ? OBX_RC_OK : OBX_RC_GENERAL_ERROR;
      } // end if
   } // end if

//Store ConnectionID and check if the string carried by Who header is the same with pre-defined UIID for SyncML.
   if ( !(iterator = ObxIteratorReset( ObxListGetIterator( ObxObjectGetHeaderList( response ) ) )) ) {
     ObxObjectFree( ocb->obxHandle, request );
     ObxObjectFree( ocb->obxHandle, response );
     return OBX_RC_ERR_MEMORY;
   }
   while ( ObxIteratorHasNext( iterator ) ) {
      if ( (header = (ObxHeader *)ObxIteratorNext( iterator )) ) {
      /*
      ** Which header is it ?
      */
         switch (header->identifier) {
            case OBEX_HEADER_CONNECTION:
               ocb->obxHandle->OBEXConnectionID=header->value.fourBytevalue;
               connectionIDHeaderCount++;
               break;
            case OBEX_HEADER_WHO:
               if(ObxBufSize( header->value.byteSequenceValue )!=sizeof(SYNCML_TARGET)-1) {
                  rc=OBX_RC_ERR_SML_WHO_HDR;
                  break;
               }
               memset(who_buffer,0,sizeof(SYNCML_TARGET));
               ObxBufRead(header->value.byteSequenceValue,who_buffer,sizeof(SYNCML_TARGET)-1);
               whoHeaderCount++;
               if(strcmp(who_buffer,SYNCML_TARGET)) {
                  rc=OBX_RC_ERR_SML_WHO_HDR;
               }
               break;
         }
      }
   }

   ObxObjectFree( ocb->obxHandle, request );
   ObxObjectFree( ocb->obxHandle, response );

   if(connectionIDHeaderCount!=1)
      rc=OBX_RC_ERR_SML_CONNECTIONID_HDR;
   if(whoHeaderCount!=1)
      rc=OBX_RC_ERR_SML_WHO_HDR;

   return rc;

} // obxSendObexConnect()


static Ret_t obxSendObexDisconnect( ObxConnectionBlock *ocb ) {

   ObxObject *request;
   ObxObject *response;
   ObxHeader      *header;

   ObxRc rc;

   XPTDEBUG(("OBX obxSendObexDisconnect()\n"));

   request  = ObxObjectNew( ocb->obxHandle, OBEX_CMD_DISCONNECT );
   response = ObxObjectNew( ocb->obxHandle, OBEX_CMD_NULL );
   rc = ( ( request != NULL ) && ( response != NULL ) ) ? OBX_RC_OK : OBX_RC_GENERAL_ERROR;
      if ( rc == OBX_RC_OK ) {
//We must embed ConnectionID header in Disconnect command.
      header = ObxHeaderNew( OBEX_HEADER_CONNECTION );
      rc = ( header == NULL ) ? OBX_RC_OBEX_HEADER : OBX_RC_OK;
      if ( rc == OBX_RC_OK ) {
          XPTDEBUG(("OBX obxSendObexDisconnect() Add header '%s' value '%x'\n", "OBEX_HEADER_CONNECTION", ocb->obxHandle->OBEXConnectionID ));
          rc = ObxHeaderSetIntValue( header, ocb->obxHandle->OBEXConnectionID );
          if ( rc == OBX_RC_OK ) {
              rc = ObxObjectAddHeader( ocb->obxHandle, request, header );
              if ( rc == OBX_RC_OK ) {
                  header = NULL; // we no longer own the header
              } // end if
          } // end if
      } // end if
    
      // Clean up from an error
      if ( rc != OBX_RC_OK ) {
         obxRecordError( OBX_ERRORMSG_OBEX_HEADER, "obxSendObexDisconnect()", "OBEX_HEADER_CONNECTION" );
         if ( header != NULL ) {
            ObxHeaderFree( header );
         } // end if
         if ( request != NULL ) {
            ObxObjectFree( ocb->obxHandle, request );
         } // end if
         return OBX_RC_OBEX_HEADER;
      } // end if

      rc = ObxTransactionSend( ocb->obxHandle, request, response );
   } // end if
   if (response->cmd != (OBEX_RSP_SUCCESS | OBEX_CMD_FINAL)) rc = OBX_RC_GENERAL_ERROR;
   ObxObjectFree( ocb->obxHandle, request );
   ObxObjectFree( ocb->obxHandle, response );

   return rc;

} // obxSendObexDisconnect()

static ObxObject *obxWaitForObexResponse( ObxConnectionBlock *ocb ) {

   ObxObject *response;

   ObxRc rc;

   XPTDEBUG(("OBX obxWaitForObexResponse()\n"));

   response = ObxObjectNew( ocb->obxHandle, OBEX_CMD_NULL );

   rc = ( response != NULL ) ? OBX_RC_OK : OBX_RC_GENERAL_ERROR;
   if ( rc == OBX_RC_OK ) {
      rc = ObxTransactionRecv( ocb->obxHandle, response, OBEX_COALESCE_YES );
      if ( rc != OBX_RC_OK ) {
         ObxObjectFree( ocb->obxHandle, response );
         response = NULL;
      } // end if
   } // end if

   return response;

} // obxWaitForObexResponse()

/*
** Record any errors.
** Supports only simple substitution: %s, %d for now
*/
static void obxRecordError( long errorCode, ... ) {

   struct xptTransportErrorInformation info;

   va_list        ap;
   char           *p;
   char           *sVal;
   int            iVal;
   char           isVal[25];        // Should be big enough for most numbers....
   const char     *fmt;
   char           message[256];     // Limits the msg length, but should be sufficent.
   char           *m;

   info.protocolErrorCode = errorCode;

   fmt = ERRORS[ errorCode ];
   m = message;

   va_start( ap, errorCode );
   for ( p=(char *)fmt; *p; p++ ) {
      if ( *p != '%' ) {
         *(m++) = *p;
         continue;
      }
      switch ( *++p ) {
      case  'd':
         iVal = va_arg( ap, int );
         iVal = sprintf(isVal, "%d", iVal);
         strcpy( m, isVal );
         m += iVal;
         break;
      case  's':
         for ( sVal=va_arg( ap, char * ); *sVal; sVal++ ) {
            *(m++) = *sVal;
         }
         break;
      default:
         *(m++) = *p;
         break;
      }
   }
   *m = '\0';
   va_end( ap );
   info.errorMessage = message;
   XPTDEBUG(("OBX obxRecordError(): Message= '%s'\n", message));
   xptSetLastError( &info );
}


/**************************************************************/
/* Function: parse one parameter from the specified line.     */
/* it is assumed that the parameter has the following format: */
/*           parm={ value | "value" }[,]                      */
/* Return the rest of the string, or NULL if here are no more */
/* data to handle                                             */
/* The function changes the contents of pszLine!!             */
/**************************************************************/

// Note: gcc cannot parse multi-line macros in files with DOS lineends
#define SKIP_WHITESPACE(s) while (((s)[0] == ' ')||((s)[0] == ',')||((s)[0] == '\t')) (s) ++

// %%% luz 2002-04-16: made static as same name is used for slightly different function in xpt-http.c
static unsigned char *splitParmValue (unsigned char *pszLine, // i: line
                               unsigned char **ppszParm,  // o: ptr to extracted parameter
                               unsigned char **ppszValue) // o: ptr to extracted parameter value
   {
   unsigned char *pszToken = pszLine;
   unsigned char *pszRest = NULL;

   if (pszToken == NULL) return NULL;
   /* skip leading blanks */
   SKIP_WHITESPACE (pszToken);
   pszRest = pszToken;
   *ppszParm = pszRest;
   if (pszToken [0] == '\0') return NULL;

   /* Find the delimiter */
   while ((*pszRest != '\0') && (*pszRest != '=') && (*pszRest != ' ') && (*pszRest != ','))
      pszRest ++;
   switch (*pszRest)
      {
      case '\0': //
         *ppszValue = pszRest;
         break;

      case ',':
      case ' ': // whitespace: there is no value assigned to this parameter
         *pszRest = '\0';
         *ppszValue = pszRest;
         pszRest ++;
         break;

      case '=':
         /* The value part may or may not be enclosed in quotes */
         *pszRest = '\0';
         pszRest ++;
         SKIP_WHITESPACE (pszRest);
         if (pszRest [0] == '\"')
            {
            *ppszValue = ++pszRest;
            while ((*pszRest != '\0') && (*pszRest != '\"'))
               pszRest ++;
            }
         else
            {
            *ppszValue = pszRest;
            while ((*pszRest != '\0') && (*pszRest != ' ') && (*pszRest != ','))
               pszRest ++;
            }
         if (*pszRest)
            { *pszRest = '\0'; pszRest ++; }
         break;
      }
   return pszRest;
   }
