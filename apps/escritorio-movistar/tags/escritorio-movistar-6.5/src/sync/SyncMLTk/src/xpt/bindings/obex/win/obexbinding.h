
/*************************************************************************/
/* module:          SyncML OBEX binding API header file.                 */
/* file:            src/xpt/bindings/obex/win/obexbinding.h              */
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

/**
 * This include file contains the API specifications for the SyncML Communication Services.
 * Any exported routines (and non-exported entry points) are contained within this file.
 */

#ifndef OBEXBINDING_H
#define OBEXBINDING_H

#include <smldef.h>

#include <xptTransport.h>
#include <xpt.h>

#include <obex/obex.h>

#include <obexbindingerror.h>


#ifdef _WIN32
 #include <winsock.h>
 #include <io.h>
#else
 #include <netinet/in.h>
 #include <stdarg.h>
 #include <unistd.h>
#endif

/* TK: we use .def files instead of compiler dependant definitions */
 #define OBEX_BINDING_EXPORT

/*
**
** Memory Management Macros
**
*/

#define ALLOC_MEM(x)    malloc(x)
#define COPY_MEM(s,d,l) memcpy(d,s,l)
#define FREE_MEM(x)     if(x!=NULL) {free(x);x=NULL;}

/*
**
** Error Checking Macros
**
*/

// Note: gcc cannot parse multi-line macros in files with DOS lineends

// x-pntr, c-called from, v-variable
#define CHECK_ALLOC(x,c,v)  if(!x) { obxRecordError( OBX_ERRORMSG_MEMORY_ALLOC, c, v ); return OBX_RC_MEMORY_ERROR_ALLOC; }

// x-pntr, c-called from, v-variable
#define CHECK_PARM(x,c,v)  if(!x) { obxRecordError( OBX_ERRORMSG_NULL_POINTER, c, v ); return OBX_RC_MEMORY_NULL_PNTR; }

#define CHECK_RC(x)     if(x!=OBX_RC_OK) {return x;}


/*
**
** Constants
**
*/

#define OBX_DEFAULT_TIMEOUT            300         // Default OBEX response timeout, in seconds

#define HEADER_CREATOR_ID              0xcf

#define OBX_DEFAULT_CREATORID          0x53594d4c  // "SYML"

#define OBX_SYNCHRONOUS                0
#define OBX_REQUEST_ROLE_UNSPECIFIED   0x1
#define OBX_REQUEST_ROLE_SENDER        0x2
#define OBX_REQUEST_ROLE_RECEIVER      0x3
#define OBX_REQUEST_ROLE_LISTENER      0x4

#define OBEX_TRANS_IRDA		1
#define OBEX_TRANS_INET		2

#define OBEX_CMD_NULL      0
#define OBEX_COALESCE_NO   0
#define OBEX_COALESCE_YES  1

#define OBEX_CONNECTED_NO  0
#define OBEX_CONNECTED_YES 1

#ifdef __cplusplus
extern "C" {
#endif


/*
**
** Support structures
**
*/

struct _ObxSequenceNode {
   struct _ObxSequenceNode *next;
   struct _ObxSequenceNode *previous;
   void                    *node;
};

typedef struct _ObxSequenceNode ObxSequenceNode;

struct _ObxBuffer {
   unsigned char *buf;
   unsigned char *cursor;
   size_t length;
};

typedef struct _ObxBuffer ObxBuf;

typedef short ObxTransportType;

/*
** Represents a transport binding.  ( i.e. Obex/TCP or Obex/IR )
** This is actually managed by the xpt layer above us.. we provide a
** structure when we register ourselves.
*/
struct _ObxTransportBlock {
   ObxTransportType     obxTransType;         // Transport type
};

typedef struct _ObxTransportBlock ObxTransportBlock;

/*
** A service block.
** Created when protocol is selected, destroyed when it's deselected.
** It contains information specific to this protocol.
*/
struct _ObxServiceBlock {
   char                *serviceName;         // What host to connect to or irda service to register as
   unsigned short      port;                 // Used if connecting via inet
   ObxSequenceNode     *connections;         // Any active connections associated with this protocol
   ObxTransportBlock   *transport;           // Mom-ma
   char                *metadata;            // **MHB** metadata passed in via xptSelectProtocol()
   ObxHandle           *obxHandle;           // **MHB** handle when acting as server
   int                 flags;                // XPT_CLIENT or XPT_SERVER (from xpt.h)
};

typedef struct _ObxServiceBlock ObxServiceBlock;

/*
** A structure representing an obex connection.
** Created when communication is opened, destroyed when it's closed.  Also
** destroyed should the protocol be deselected.
** It contains information specific to this connection.
*/
struct _ObxConnectionBlock {
   ObxHandle         *obxHandle;           // Obex handle.
   int               role;                 // Role, as opened, either: XPT_REQUEST_SENDER or XPT_REQUEST_RECEIVER
   int               currentRole;          // Current role: either: XPT_REQUEST_SENDER or XPT_REQUEST_RECEIVER
   int               timeout;              // Current timeout to use, in seconds
   XptCommunicationInfo_t *docInfo;        // Doc info for this connection
   char              *pchXSyncmlHmac;      // persistent data store for the hmac string that may be referred in docInfo;
   ObxBuf            *dataToRead;          // Data, inbound (single ObxBuffer, all put data arrives at once)
   ObxSequenceNode   *dataToSend;          // Data, outbound (sequence of ObxBuffer's)
   ObxServiceBlock   *service;             // Mom-ma...
   int               connected;            // Connected (1) or not (0).
};

typedef struct _ObxConnectionBlock ObxConnectionBlock;


/*********************************************************************************/
/***************************** Public Functions **********************************/
/*********************************************************************************/

#ifdef LINK_TRANSPORT_STATICALLY
 #define initializeTransport obexInitializeTransport
#endif


/*
******************************************************************
** Exported
******************************************************************
*/

/**
 * FUNCTION: initializeTransport
 *
 *  Called by xpt when our DLL is loaded.  This function is required to
 *  call and register each api proto.
 *
 */
OBEX_BINDING_EXPORT Ret_t XPTAPI initializeTransport();


/*
******************************************************************
** Functional prototypes of all api's inbound to the obex binding.
** Privately defined.
******************************************************************
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
Ret_t XPTAPI obxSelectProtocol( void *privateTransportInfo,
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
Ret_t XPTAPI obxDeselectProtocol( void *privateServiceInfo );

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
Ret_t XPTAPI obxOpenCommunication( void *privateServiceInfo,
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
Ret_t XPTAPI obxCloseCommunication( void *privateConnectionInfo );


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
Ret_t XPTAPI obxBeginExchange(void *privateConnectionInfo);


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
Ret_t XPTAPI obxEndExchange(void *privateConnectionInfo);

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
Ret_t XPTAPI obxSendData(void *privateConnectionInfo,
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
Ret_t XPTAPI obxSendComplete(void *privateConnectionInfo);


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
Ret_t XPTAPI obxSetDocumentInfo( void *privateConnectionInfo,
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
Ret_t XPTAPI obxGetDocumentInfo( void *privateConnectionInfo,
                                 XptCommunicationInfo_t *pDoc );


/*********************************************************************************/
/***************************** Private Functions *********************************/
/*********************************************************************************/

/*
** Associate a connection to the passed service
*/
static Ret_t obxAddConnection( ObxServiceBlock *service, ObxConnectionBlock *connection );

/*
** Removes the connection from it's service object.
** Any associated storage used to keep it in sequence within the service block
** is free'ed but it's the callers responsibility to free the connection block itself.
** Returns null if connection block is not found within service referenced by the connection.
*/
static ObxConnectionBlock * obxRemoveConnection( ObxConnectionBlock *connection );

/*
** Wacks all storage associated with the passed connection construct.
** Note that it's up to the caller to ensure that the passed object has
** been removed from any ObxServiceBlock's.
*/
static Ret_t obxFreeConnection( ObxConnectionBlock *connection );

/*
** Wacks all storage associated with the passed service construct.
** Including the storage in the passed construct.
*/
static Ret_t obxFreeService( ObxServiceBlock *block );

/*
** Register the TCP Obex binding.
*/
static Ret_t obxRegisterTcpObex();

/*
** Register the IR Obex binding.
*/
static Ret_t obxRegisterIrObex();

/*
** Flow obex handshaking for client mode
*/
static Ret_t obxInitializeForClientMode( ObxConnectionBlock *cblock );

/*
** Flow obex handshaking for server mode
*/
static Ret_t obxInitializeForServerMode( ObxConnectionBlock *cblock );

/*
** Queue the inbound buffer for a later send.
*/
static Ret_t obxQueueBufferForSend( ObxConnectionBlock *connection, const void *buffer, size_t bufferLen );

/*
** Form a single buffer with all data to send.
*/
static int obxGetBufferForSend( ObxConnectionBlock *connection, ObxBuffer **body );

/*
** Register the underlying OBEX transporta
*/

static Ret_t obxRegisterTransport( ObxConnectionBlock *ocb );

// static Ret_t obxWaitForDisconnect( ObxConnectionBlock *ocb );

static Ret_t obxResetConnection( ObxConnectionBlock *ocb );

static Ret_t obxSendObexConnect( ObxConnectionBlock *ocb );


static Ret_t obxSendObexDisconnect( ObxConnectionBlock *ocb );

static ObxObject *obxWaitForObexResponse( ObxConnectionBlock *ocb );

/*
** Record errors with xpt layer
*/
static void obxRecordError( long errorCode, ... );

/*
* Parse the value string of the header field XSyncmlHmac
*/
static unsigned char *splitParmValue (unsigned char *pszLine, // i: line
                               unsigned char **ppszParm,  // o: ptr to extracted parameter
                               unsigned char **ppszValue); // o: ptr to extracted parameter value

#ifdef __cplusplus
}
#endif

#endif

