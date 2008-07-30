/*************************************************************************/
/* module:          SyncML OBEX binding API source file.                 */
/* file:            obexbinding.c                                        */
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

/**
 * This include file contains the API implementations for the
 * SyncML Communication Services for OBEX on the Palm platform.
 */

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#include "xpt.h"
#include "xptTransport.h"

#include "obexbinding.h"

#include "SyncBmrUtil.h"

/**
 * FUNCTION: obexInitializeTransport ("initializeTransport")
 *
 *  Called by XPT layer to initialize and register the protocol.
 *
 */
Ret_t XPTAPI obexInitializeTransport(void) {
   obpRegisterTransport();            // register the Palm OBEX protocol
   return OBP_RC_OK;
} // initializeTransport()


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
 *     metaInformation, passed directly from the xptSelectProtocol() call.
 *         Beware that a null pointer may have been specified to indicate
 *         that no meta information was provided.
 *
 *     flags, passed directly from the xptSelectProtocol() call.
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
Ret_t XPTAPI obpSelectProtocol( void *privateTransportInfo,
                                const char *metaInformation,
                                unsigned int flags,
                                void **pPrivateServiceInfo ) {
   ObpServiceBlock *osb;

   osb = (ObpServiceBlock *)MEM_ALLOC( sizeof( ObpServiceBlock ) );
   ErrFatalDisplayIf( !osb, "memory allocation" );

   if ( !metaInformation ) {
      osb->metaInformation = (char *)MEM_ALLOC( STR_LEN( metaInformation ) );
      ErrFatalDisplayIf( !osb->metaInformation, "memory allocation" );
   } // end if
   else {
      osb->metaInformation = NULL;   // no meta data provided
   } // end else

   osb->flags           = flags;     // copy flags
   *pPrivateServiceInfo = osb;       // return pointer to service info

   return OBP_RC_OK;

} // obpSelectProtocol()


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
Ret_t XPTAPI obpDeselectProtocol( void *privateServiceInfo ) {

   ObpServiceBlock *osb = (ObpServiceBlock *)privateServiceInfo;

   ErrFatalDisplayIf( !osb, "null pointer" );

   MEM_FREE( osb->metaInformation );  // free meta data
   MEM_FREE( osb );                   // free service info

   return OBP_RC_OK;

} // obpDeselectProtocol()

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
Ret_t XPTAPI obpOpenCommunication( void *privateServiceInfo,
                                   int role,
                                   void **pPrivateConnectionInfo ) {

   ObpConnectionBlock *ocb;

   // Create a connection block
   ocb = (ObpConnectionBlock *)MEM_ALLOC( sizeof( ObpConnectionBlock ) );
   ErrFatalDisplayIf( !ocb, "memory allocation" );

   // Create a communication info block
   ocb->docInfo = (XptCommunicationInfo_t *)MEM_ALLOC( sizeof( XptCommunicationInfo_t) );
   ErrFatalDisplayIf( !ocb->docInfo, "memory allocation" );

   ocb->docInfo->cbSize = sizeof( XptCommunicationInfo_t );

   ocb->flags = role;

   ocb->incoming = (ObpBuffer *)MEM_ALLOC( sizeof( ObpBuffer ) );
   ErrFatalDisplayIf( !ocb->incoming, "memory allocation" );

   ocb->incoming->buf    = NULL;
   ocb->incoming->cursor = NULL;
   ocb->incoming->length = 0;

   ocb->outgoing = (ObpBuffer *)MEM_ALLOC( sizeof( ObpBuffer ) );
   ErrFatalDisplayIf( !ocb->outgoing, "memory allocation" );

   ocb->outgoing->buf    = NULL;
   ocb->outgoing->cursor = NULL;
   ocb->outgoing->length = 0;

   *pPrivateConnectionInfo = ocb;  // return pointer to caller

   return OBP_RC_OK;

} // opbOpenCommunication()


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
Ret_t XPTAPI obpCloseCommunication( void *privateConnectionInfo ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   ErrFatalDisplayIf( !ocb,           "null pointer" );
   ErrFatalDisplayIf( !ocb->incoming, "null pointer" );
   ErrFatalDisplayIf( !ocb->outgoing, "null pointer" );

   MEM_FREE( ocb->incoming->buf );   // release incoming data buffer
   MEM_FREE( ocb->outgoing->buf );   // release incoming data buffer

   MEM_FREE( ocb->incoming );        // release the incoming ObpBuffer
   MEM_FREE( ocb->outgoing );        // release the outgoing ObpBuffer
   MEM_FREE( ocb->docInfo );         // release the doc info block

   MEM_FREE( ocb );                  // release the connection block

   return OBP_RC_OK;

} // obpCloseCommunication()


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
Ret_t XPTAPI obpBeginExchange( void *privateConnectionInfo ) {

   return OBP_RC_OK;

} // opbBeginExchange()


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
Ret_t XPTAPI obpEndExchange( void *privateConnectionInfo ) {

   return OBP_RC_OK;

} // opbEndExchange()


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
Ret_t XPTAPI obpReceiveData( void *privateConnectionInfo,
                             void *buffer,
                             size_t bufferLen,
                             size_t *dataLen ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   ObpBuffer *in;

   unsigned long remainingLength;

   ErrFatalDisplayIf( !ocb,           "null pointer" );
   ErrFatalDisplayIf( !ocb->incoming, "null pointer" );

   in = ocb->incoming;

   remainingLength = in->length - ( in->cursor - in->buf );  // number of bytes remaining to receive
   remainingLength = remainingLength > 0 ? remainingLength : 0;

   if ( remainingLength > 0 ) {
      if ( bufferLen >= remainingLength ) {
         MEM_COPY( buffer, in->cursor, remainingLength );
         in->cursor += remainingLength;
         *dataLen    = remainingLength;
      } // end if
      else {
         MEM_COPY( buffer, in->cursor, bufferLen );
         in->cursor += bufferLen;
         *dataLen    = bufferLen;
      } // end else
   } // end if
   else {
      *dataLen = 0;
   } // end else

   return OBP_RC_OK;

} // obpReceiveData()


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
Ret_t XPTAPI obpSendData(void *privateConnectionInfo,
                         const void *dataBuffer,
                         size_t bufferLen,
                         size_t *bytesSent ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   ErrFatalDisplayIf( !ocb, "null pointer" );

   // Copy data to output buffer
   MEM_COPY( ocb->outgoing->cursor, dataBuffer, bufferLen );

   ocb->outgoing->cursor += bufferLen;  // move cursor to new position
   ocb->outgoing->length += bufferLen;  // update new data length
   *bytesSent             = bufferLen;  // always send every byte

   return OBP_RC_OK;

} // obpSendData()

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
Ret_t XPTAPI obpSendComplete( void *privateConnectionInfo ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   ErrFatalDisplayIf( !ocb, "null pointer" );

   if ( ocb->flags == XPT_REQUEST_SENDER ) {         // send document, then wait for response
      obpSendRequest( ocb );
   } // end if
   else if ( ocb->flags == XPT_REQUEST_RECEIVER ) {  // send document, then immediately return
      obpSendResponse( ocb );
   } // end else if

   return OBP_RC_OK;

} // obpSendComplete()


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
 */
Ret_t XPTAPI obpSetDocumentInfo( void *privateConnectionInfo,
                                 const XptCommunicationInfo_t *pDoc ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   ErrFatalDisplayIf( !ocb, "null pointer" );

   // Create a CommunicationInfo block if one does not exist
   if ( !ocb->docInfo ) {
      ocb->docInfo = (XptCommunicationInfo_t *)MEM_ALLOC( sizeof( XptCommunicationInfo_t) );
      ErrFatalDisplayIf( !ocb->docInfo, "memory allocation" );
      ocb->docInfo->cbSize = sizeof( XptCommunicationInfo_t );
   } // end if

   // Copy document information into newly-created ConnectionInfo block
   STR_COPY( ocb->docInfo->docName,  pDoc->docName );
   STR_COPY( ocb->docInfo->mimeType, pDoc->mimeType );
   ocb->docInfo->cbLength = pDoc->cbLength;

   ErrFatalDisplayIf( !ocb->outgoing, "null pointer" );

   MEM_FREE( ocb->outgoing->buf );

   ocb->outgoing->buf = (unsigned char *)MEM_ALLOC( ocb->docInfo->cbLength );

   ErrFatalDisplayIf( !ocb->outgoing->buf, "null pointer" );

   ocb->outgoing->cursor = ocb->outgoing->buf;
   ocb->outgoing->length = ocb->docInfo->cbLength;

   return OBP_RC_OK;

} // obpSetDocumentInfo()


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
Ret_t XPTAPI obpGetDocumentInfo( void *privateConnectionInfo,
                                 XptCommunicationInfo_t *pDoc ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   ErrFatalDisplayIf( !ocb, "null pointer" );

   if ( ocb->flags == XPT_REQUEST_RECEIVER ) {  // this caller is a "receiver"
      obpReceiveResponse( ocb );                // read the entire document
   } // end if

   // Copy document information into CommunicationInfo block
   STR_COPY( pDoc->docName,  ocb->docInfo->docName );
   STR_COPY( pDoc->mimeType, ocb->docInfo->mimeType );
   pDoc->cbLength = ocb->docInfo->cbLength;

   return OBP_RC_OK;

} // obpGetDocumentInfo()


/*************************************************************************
 *                                                                       *
 * Functions dealing with protocol initialization                        *
 *                                                                       *
 *************************************************************************/

/**
 * FUNCTION: Called when initializeTransport() is called.
 *
 *  Setup xptTransportDescription information and call xptRegisterTransport().
 */
static Ret_t obpRegisterTransport(void) {

   // MHB: Check for existence of SyncBmr and return an error if not present.

   struct xptTransportDescription xptBlock;  // input to xptRegisterTransport()

   ObpTransportBlock *otb;                   // private transport information

   xptBlock.shortName            = "OBEX/IR";
   xptBlock.description          = "OBEX transport over IR";

   xptBlock.flags                = XPT_CLIENT | XPT_SERVER;

   xptBlock.selectProtocol       = obpSelectProtocol;
   xptBlock.deselectProtocol     = obpDeselectProtocol;
   xptBlock.openCommunication    = obpOpenCommunication;
   xptBlock.closeCommunication   = obpCloseCommunication;
   xptBlock.beginExchange        = obpBeginExchange;
   xptBlock.endExchange          = obpEndExchange;
   xptBlock.receiveData          = obpReceiveData;
   xptBlock.sendData             = obpSendData;
   xptBlock.sendComplete         = obpSendComplete;
   xptBlock.setDocumentInfo      = obpSetDocumentInfo;
   xptBlock.getDocumentInfo      = obpGetDocumentInfo;

   // MHB: Currently, there is no "DeregisterTransport()"
   // MHB: function where this block can be released...
   otb = (ObpTransportBlock *)MEM_ALLOC( sizeof( ObpTransportBlock ) );
   ErrFatalDisplayIf( !otb, "memory allocation" );

   xptBlock.privateTransportInfo = otb;       // MHB: Any info required here?

   return xptRegisterTransport( &xptBlock );  // call XPT layer to register

} // obpRegisterTransport()


/**
 * FUNCTION: Called to deregister the transport.
 *
 *    NOTE: This function is not part of the existing specification
 *          and, therefore, is not called from anywhere.
 */
static Ret_t obpDeregisterTransport( struct xptTransportDescription *xptBlock ) {

   MEM_FREE( xptBlock->privateTransportInfo );  // free up the transport block

   return OBP_RC_OK;

} // obpDeregisterTransport()


/*************************************************************************
 *                                                                       *
 * Functions used to communicate with the SyncBmr application            *
 *                                                                       *
 *************************************************************************/

/**
 * FUNCTION: Called by obpSendComplete to tansmit request.
 *           This function blocks until a response has been
 *           been received.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 */
static Ret_t obpSendRequest( void *privateConnectionInfo ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   SyncBmrRequest_t  *reqP  = NULL;
   SyncBmrResponse_t *respP = NULL;

   obpProcessRequest( ocb, &reqP );    // validate and build request

   sbuSendRequest( &reqP, &respP );    // send request and wait for response

   obpProcessResponse( ocb, respP );   // validate and build response

   return OBP_RC_OK;

} // obpSendRequest()


/**
 * FUNCTION: Called by obpSendComplete to tansmit request.
 *           This function does not wait for a response.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 */
static Ret_t obpSendResponse( void *privateConnectionInfo ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   SyncBmrRequest_t *reqP = NULL;

   obpProcessRequest( ocb, &reqP );  // validate and build request

   sbuSendResponse( &reqP );         // send request and do not wait for response

   return OBP_RC_OK;

} // obpSendResponse()


/**
 * FUNCTION: Called by obpGetDocumentInfo to receive request.
 *
 * IN: privateConnectionInfo, pointer to the transport binding's private
 *         information about the connection instance.  This is the same
 *         value that was returned by the "openCommunication" function when
 *         the connection instance was created.
 */
static Ret_t obpReceiveResponse( void *privateConnectionInfo ) {

   ObpConnectionBlock *ocb = (ObpConnectionBlock *)privateConnectionInfo;

   SyncBmrResponse_t *respP = NULL;

   sbuReceiveResponse( &respP );      // wait for an in coming response

   obpProcessResponse( ocb, respP );  // validate and build response

   return OBP_RC_OK;

} // obpReceiveRequest()


/*************************************************************************
 *                                                                       *
 * Internal Functions                                                    *
 *                                                                       *
 *************************************************************************/

/**
 * FUNCTION: Called to obtain an ObpBuffer block.
 */
static Ret_t obpObtainBuffer( ObpBuffer *ob, size_t length ) {

   ErrFatalDisplayIf( !ob, "null pointer" );

   MEM_FREE( ob->buf );                                 // free up existing buffer

   ob->buf = (unsigned char *)MEM_ALLOC( length );      // allocate actually data buffer
   ErrFatalDisplayIf( !ob->buf, "memory allocation" );

   ob->cursor = ob->buf;                // initialize cursor to start of data buffer
   ob->length = MemPtrSize( ob->buf );  // set the data buffer length

   return OBP_RC_OK;

} // obpObtainBuffer()


/**
 * FUNCTION: Called by obpSendRequest and obpSendResponse.
 *           Common code to provide validation and assembly of request.
 */
static Ret_t obpProcessRequest( ObpConnectionBlock *ocb, SyncBmrRequest_t **reqP ) {

   ErrFatalDisplayIf( !ocb,           "null pointer" );
   ErrFatalDisplayIf( !ocb->outgoing, "null pointer" );
   ErrFatalDisplayIf( !ocb->docInfo,  "null pointer" );

   *reqP = (SyncBmrResponse_t *)MEM_ALLOC( sizeof( SyncBmrRequest_t ) );
   ErrFatalDisplayIf( !*reqP, "memory allocation" );

   (*reqP)->targetID = 0x7FFFFFFF;                     // dummy id - not used
   (*reqP)->data     = ocb->outgoing->buf;             // point to outgoing data buffer
   (*reqP)->length   = ocb->docInfo->cbLength;         // total number of bytes for all objects

   STR_COPY( (*reqP)->name, ocb->docInfo->docName );   // filename and extension of data
   STR_COPY( (*reqP)->type, ocb->docInfo->mimeType );  // MIME type of data

   return OBP_RC_OK;

} // obpProcessRequest()

/**
 * FUNCTION: Called by obpSendRequest and obpReceiveResponse.
 *           Common code to provide validation and assembly of response.
 */
static Ret_t obpProcessResponse( ObpConnectionBlock *ocb, SyncBmrResponse_t *respP ) {

   ErrFatalDisplayIf( !ocb,           "null pointer" );
   ErrFatalDisplayIf( !ocb->docInfo,  "null pointer" );
   ErrFatalDisplayIf( !ocb->incoming, "null pointer" );

   ErrFatalDisplayIf( !respP,       "receive error" );
   ErrFatalDisplayIf( !respP->data, "null pointer" );

   STR_COPY( ocb->docInfo->docName,  respP->name );  // copy name from response
   STR_COPY( ocb->docInfo->mimeType, respP->type );  // copy type from response

   ocb->docInfo->cbLength = respP->length;           // copy length from response

   MEM_FREE( ocb->incoming->buf );                   // free up existing buffer

   ocb->incoming->buf    = respP->data;              // point to new data buffer
   ocb->incoming->cursor = respP->data;              // update cursor
   ocb->incoming->length = respP->length;            // copy length

   return OBP_RC_OK;

} // obpProcessResponse()


