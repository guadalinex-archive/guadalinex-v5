/*************************************************************************/
/* module:          SyncML OBEX binding API header file.                 */
/* file:            obexbinding.h                                        */
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
 * This include file contains the API specifications for the
 * SyncML Communication Services for OBEX on the Palm platform.
 */

#ifndef OBEXBINDING_H
#define OBEXBINDING_H

#ifndef PILOT_PRECOMPILED_HEADERS_OFF
#define PILOT_PRECOMPILED_HEADERS_OFF
#endif

#include "xpt.h"
#include "xptTransport.h"

#include "SyncBmr.h"


/*************************************************************************
 *                                                                       *
 * Constants                                                             *
 *                                                                       *
 *************************************************************************/

#define OBP_RC_OK               0x00


/*************************************************************************
 *                                                                       *
 * Macros for memory management.                                         *
 *                                                                       *
 *************************************************************************/

#define MEM_ALLOC(x)    MemPtrNew((x))
#define MEM_COPY(d,s,l) MemMove((d),(s),(l))
#define MEM_FREE(x)     if((x)!=NULL){MemPtrFree((x));(x)=NULL;}

#define STR_COPY(d,s)   StrCopy((d),(s))
#define STR_LEN(s)      StrLen((s))

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************
 *                                                                       *
 * Support structures                                                    *
 *                                                                       *
 *************************************************************************/

struct _ObpBuffer {                  // container for document body
   unsigned char *buf;               // pointer to start of buffer
   unsigned char *cursor;            // next available byte in buffer
   size_t length;                    // length of data buffer
}; // struct _ObpBuffer

typedef struct _ObpBuffer ObpBuffer;

struct _ObpTransportBlock {          // used in obpRegisterTransport()
   int nothingHereYet;               // just a placeholder for now
};

typedef struct _ObpTransportBlock ObpTransportBlock;

struct _ObpServiceBlock {            // passed back from ObpSelectProtocol()
   unsigned int flags;               // XPT_Client and/or XPT_SERVER
   char *metaInformation;            // app-supplied meta data
}; // struct _ObpTransportBlock

typedef struct _ObpServiceBlock ObpServiceBlock;

struct _ObpConnectionBlock {         // passed back from ObpOpenCommunication()
   unsigned int flags;               // XPT_REQUEST_SENDER or XPT_REQUEST_RECEIVER
   XptCommunicationInfo_t *docInfo;  // document info for this exchange
   ObpBuffer *outgoing;              // pointer to outgoing document
   ObpBuffer *incoming;              // pointer to incoming document
}; // struct _ObpConnectionBlock

typedef struct _ObpConnectionBlock ObpConnectionBlock;


/*************************************************************************
 *                                                                       *
 * Transport-specific functions required by the XPT layer.               *
 *                                                                       *
 *************************************************************************/

/**
 * FUNCTION: obexInitializeTransport ("initializeTransport")
 *
 *  Called by xpt to initialize and register the protocol.
 *
 */
Ret_t XPTAPI obexInitializeTransport(void);

/**
 *
 * Functions required by XPT API
 *
 */

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
 *
 *     flags, passed directly from the xptSelectProtocol() call.
 *
 * OUT: *pPrivateServiceInfo, pointer to the transport binding's private
 *         information for the newly allocated service instance.  This
 *         pointer will be passed on subsequent calls to the transport's
 *         functions to identify this service instance to the transport.
 */
Ret_t XPTAPI obpSelectProtocol( void *privateTransportInfo,
                                const char *metaInformation,
                                unsigned int flags,
                                void **pPrivateServiceInfo );


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
Ret_t XPTAPI obpDeselectProtocol( void *privateServiceInfo );


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
 *
 * OUT: *pPrivateConnectionInfo, pointer to the transport binding's private
 *         information for the newly allocated connection instance.  This
 *         pointer will be passed on subsequent calls to the transport's
 *         functions to identify this connection instance to the transport.
 */
Ret_t XPTAPI obpOpenCommunication( void *privateServiceInfo,
                                   int role,
                                   void **pPrivateConnectionInfo );


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
 */
Ret_t XPTAPI obpCloseCommunication( void *privateConnectionInfo );


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
Ret_t XPTAPI obpBeginExchange(void *privateConnectionInfo);


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
Ret_t XPTAPI obpEndExchange(void *privateConnectionInfo);


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
Ret_t XPTAPI obpReceiveData(void *privateConnectionInfo,
                            void *buffer,
                            size_t bufferLen,
                            size_t *dataLen);


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
                         const void *buffer,
                         size_t bufferLen,
                         size_t *bytesSent);


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
Ret_t XPTAPI obpSendComplete(void *privateConnectionInfo);


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
                                 const XptCommunicationInfo_t *pDoc );


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
                                 XptCommunicationInfo_t *pDoc );


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
static Ret_t obpRegisterTransport( void );


/**
 * FUNCTION: Called to "deregister" the transport.
 */
static Ret_t obpDeregisterTransport( struct xptTransportDescription *xptBlock );


/*************************************************************************
 *                                                                       *
 * Functions that communicate to SyncBmr application                     *
 *                                                                       *
 *************************************************************************/

/**
 * FUNCTION: Called to send a document and wait for a response.
 */
static Ret_t obpSendRequest( void *privateConnectionInfo );


/**
 * FUNCTION: Called to send a document.
 */
static Ret_t obpSendResponse( void *privateConnectionInfo );


/**
 * FUNCTION: Called to receive an incoming document.
 */
static Ret_t obpReceiveResponse( void *privateConnectionInfo );


/*************************************************************************
 *                                                                       *
 * Internal Functions                                                    *
 *                                                                       *
 *************************************************************************/

/**
 * FUNCTION: Called to obtain a single document buffer.
 */
static Ret_t obpObtainBuffer( ObpBuffer *ob, size_t length );


/**
 * FUNCTION: Called to process outgoing request.
 */
static Ret_t obpProcessRequest( ObpConnectionBlock *ocb, SyncBmrRequest_t **reqP );


/**
 * FUNCTION: Called to process incoming response.
 */
static Ret_t obpProcessResponse( ObpConnectionBlock *ocb, SyncBmrResponse_t *respP );


#ifdef __cplusplus
}
#endif

#endif
