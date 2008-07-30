/*************************************************************************/
/* module:          Communication Services, HTTP functions               */
/* file:            /src/xpt/all/xpt-http.h                              */
/* target system:   all                                                  */
/* target OS:       all                                                  */
/*************************************************************************/


/*
 * Copyright Notice
 * Copyright (c) Ericsson, IBM, Lotus, Matsushita Communication 
 * Industrial Co., LTD,Motorola, Nokia, Palm, Inc., Psion, 
 * Starfish Software (2001).
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
 * HTTP protocol services, function prototypes and return codes
 *
 *
 */

#ifndef XPT_HTTP_H
#define XPT_HTTP_H


/*********************/
/* Required includes */
/*********************/

#include "xpt.h"
#include "xpttypes.h"
#include "xpt-tcp.h"


#ifdef _cplusplus
extern "C" {
#endif

#define UNDEFINED_CONTENT_LENGTH  0x7FFFFFFFL

/*****************************/
/* Types and Data structures */
/*****************************/

typedef void * HttpHandle_t;   // instance handle



/********************************************************/
/* Authentication info, Basic and Digest authentication */
/********************************************************/
#include "xpt-auth.h"

typedef struct                   // document context
   {
   BufferSize_t cbSize;          // size of this structure
   CString_t pszURL;             // document name
   CString_t pszType;            // document MIME type
   CString_t pszHost;            // host name (optional)
   BufferSize_t cbLength;        // document length
   CString_t pszReferer;         // referenced URL (optional)
   CString_t pszRequest;         // type of the HTTP request
   CString_t pszFrom;            // sender of the document
   CString_t pszProxy;           // Proxy IP address
   XptHmacInfoPtr_t pXSyncmlHmac; // digest values for transport header field
   // %%% luz:2002-05-23: Auth support added
   HttpAuthenticationPtr_t auth;  // auth structure created by authInit()
   } HttpDocumentContext_t, *HttpDocumentContextPtr_t;


typedef struct                   // HTTP document reply
   {
   BufferSize_t cbSize;          // size of this structure
   CString_t pszTime;            // creation date of the replied document
   CString_t pszType;            // document MIME type
   BufferSize_t cbLength;        // document length
   HttpAuthenticationPtr_t auth; // authentication info
   XptHmacInfoPtr_t pXSyncmlHmac; // digest values for transport header field
   } HttpReplyBuffer_t, * HttpReplyBufferPtr_t;



/**************************/
/* Function return values */
/**************************/
typedef enum
   {
   HTTP_RC_RETRY         = -3,  // authentication required: resend the document
   HTTP_RC_HTTP          = -2,  // server error
   HTTP_RC_EOF           = -1,  // end of transmission
   HTTP_RC_OK            =  0,
   HTTP_RC_COMMUNICATION =  1,  // communication problem, reported by TCP/IP
   HTTP_RC_PARAMETER     =  2,  // one of the parameters was invalid
   HTTP_RC_NOT_ALLOWED   =  3,  // this function call is not allowed in this context
   // %%%luz:2003-04-17 added these extra codes
   HTTP_RC_TIMEOUT       =  4,  // timeout
   HTTP_RC_CERT_EXPIRED  =  5,  // https: certificate expired
   HTTP_RC_CERT_INVALID  =  6   // https: certificate invalid
   } HttpRc_t;



/**
 * FUNCTION: httpOpen
 *
 *  Opens a HTTP connection
 *
 * PRE-Condition:
 *
 *  The function is invoked if a client or server decides to process a HTTP request.
 *
 *  The TCP/IP socket that is passed to the service must have been opened via
 *  tcpOpen(), and the socket must be in the right mode: If a "SERVER" request
 *  is selected, this must be a server socket, if a "SEND", "RECEIVE", or "EXCHANGE"
 *  request is selected, a client socket must be passed to the function.
 *  Although the HTTP services utilize this socket, the socket itself must
 *  be opened and closed by the caller.
 *
 * POST-Condition:
 *  HTTP Clients: The HTTP header is transmitted to the server. If a "SEND" or "EXCHANGE"
 *     request was selected, the document that is specified in the 'settings'
 *     parameter can be transmitted to the host, using httpWrite().
 *     if a "RECEIVE" request was selected, no document is transmitted to the server.
 *     The application can directly call httpWait () to wait for the requested
 *     document.
 *
 *  HTTP Server: The client's request type (either "SEND", "EXCHANGE" or "RECEIVE"
 *     are expected) as well as the document properties that are sent from the
 *     client to the server ("EXCHANGE" and "SEND" requests) are returned in the
 *     document context structure that is referenced by the 'settings' parameter.
 *     If a pointer to an authentication information structure is passed to the
 *     function, the structure is updated with the client's authorization
 *     information (userID: passphrase).
 *
 * IN: p, instance handle
 *     pSession, potiner to an open TCP/IP socket.
 *     pszMode, HTTP request type.
 *              "SEND", "EXCHANGE", "RECEIVE" for HTTP clients
 *              "SERVER" for HTTP servers
 * IN/OUT:
 *      settings, pointer to a structure that denotes the properties of the document
 *                to be sent or being received.
 *      auth, authorization info, may ne NULL.
 *
 * RETURN: HttpRc_t, return code. Refer to the type definition above for details.
 *     If the return value is HTTP_RC_COMMUNICATION, further error information
 *     can be retrieved with the httpGetError() service;
 *
 */

HttpRc_t httpOpen (HttpHandle_t p,
                   SocketPtr_t pSession,
                   CString_t pszMode,
                   HttpDocumentContextPtr_t settings,
                   HttpAuthenticationPtr_t auth);


/**
 * FUNCTION: httpWrite
 *
 *  Write a chunk of data
 *
 * PRE-Condition:
 *  the HTTP communication has been opened via httpOpen(), and the protocol
 *  is in a state where incoming data is expected:
 *  HTTP clients: BEFORE httpWait() has been invoked
 *  HTTP Server:  AFTER httpReply () has been invoked
 *
 * POST-Condition:
 *  The data is transmitted to the communication partner.
 *
 * IN: p, instance handle
 *     pchDataBuffer, pointer to a block of allocated memory for the received data
 *     cbDataBufferSize, size of the memory block above
 *     bFinal, flag indicating if input buffer is the last block to send!
 *
 * OUT: pcbDataRead, pointer to a variable that is updated with the size of the
 *      received data block.
 *
 * RETURN: HttpRc_t, return code. Refer to the type definition above for details.
 *     If the return value is HTTP_RC_COMMUNICATION, further error information
 *     can be retrieved with the httpGetError() service;
 *
 */

HttpRc_t httpWrite (HttpHandle_t p,
                    DataBuffer_t pbBuffer,
                    BufferSize_t cbBufferSize,
                    Bool_t bFinal);

/**
 * FUNCTION: httpClose
 *
 *  Close an open HTTP communication
 *
 * PRE-Condition:
 *  A HTTP communication has been opened via httpOpen(), and the data exchange
 *  has been done.
 *
 * POST-Condition:
 *  The HTTP instance handle is invalidated, and all secondary storage and
 *  system resources are freed. The TCP/IP socked remains open.
 *
 * IN: p, instance handle
 *
 * RETURN: HttpRc_t, return code. Refer to the type definition above for details.
 *     If the return value is HTTP_RC_COMMUNICATION, further error information
 *     can be retrieved with the httpGetError() service;
 *
 *
 */

HttpRc_t httpClose (HttpHandle_t p);

/**
 * FUNCTION: httpRead
 *
 *  Read a chunk of data
 *
 * PRE-Condition:
 *  the HTTP communication has been opened via httpOpen(), and the protocol
 *  is in a state where incoming data is expected:
 *  HTTP clients: AFTER httpWait() has been invoked
 *  HTTP Server: BEFORE httpReply () has been invoked
 *
 *
 * POST-Condition:
 *  A part of the receiving document is copied to the specified data buffer.
 *  The size of the received data buffer is returned in the variable that
 *  is referenced with the pointer 'pcbDataRead'.
 *
 * IN: p, instance handle
 *     pchDataBuffer, pointer to a block of allocated memory for the received data
 *     cbDataBufferSize, size of the memory block above
 *
 * OUT: pcbDataRead, pointer to a variable that is updated with the size of the
 *      received data block.
 *
 * RETURN: HttpRc_t, return code. Refer to the type definition above for details.
 *     If the return value is HTTP_RC_COMMUNICATION, further error information
 *     can be retrieved with the httpGetError() service;
 *
 *
 */

HttpRc_t httpRead  (HttpHandle_t p,
                    DataBuffer_t pbDataBuffer,
                    BufferSize_t cbDataBufferSize,
                    BufferSizePtr_t pcbDataRead);

/**
 * FUNCTION: httpWait
 *
 *  Wait for the HTTP server response
 *
 * PRE-Condition:
 *  The HTTP instance has been opened in a client mode. A HTTP request has been
 *  selected that expects a response document from the server.
 *  This is in case of "RECEIVE" requests, immediately after httpOpen() has
 *  been invoked, or in case of "EXCHANGE" requests, after the posted document
 *  has been entirely transmitted to the server using the httpWrite() service. The application
 *  nust invoke the httpWait() function to wait for the response document.
 *
 * POST-Condition:
 *  When the function returns, the server started sending the response document.
 *  The document context (i.e. document length, creation date, MIME type) is returned
 *  to the caller in the pSettings parameter.
 *
 * IN: p, instance handle
 *     pSettings, response document properties.
 *
 * OUT: pSettings, response document properties.
 *      pAuth, pointer to an allocated authentication info structure, or NULL, if
 *             the caller does not want to examine the server's authentication
 *             data.
 *
 * RETURN: HttpRc_t, return code. Refer to the type definition above for details.
 *     If the return value is HTTP_RC_COMMUNICATION, further error information
 *     can be retrieved with the httpGetError() service;
 *
 *
 */

HttpRc_t httpWait  (HttpHandle_t p,
                    HttpDocumentContextPtr_t pSettings,
                    HttpAuthenticationPtr_t pAuth);

/**
 * FUNCTION: httpReply
 *
 *  Reply to a client HTTP request
 *
 * PRE-Condition:
 *  The HTTP instance has been opened in the server mode. A HTTP request has been
 *  initiated by the client. The service httpReply() is invoked after the client
 *  HTTP request has been completely received. Dependend on the type of request,
 *  This service issues the transmission of the HTTP response header.
 *
 * POST-Condition:
 *  Dependent on the type of HTTP request, the client expects a response document.
 *  If this is the case, the application can now start sending the document with
 *  the httpWrite() service.
 *
 * IN: p, instance handle
 *     rcDocument, HTTP return code of the request (i.e. 200 for OK)
 *     pSettings, response document properties. Dependent on the type of request,
 *                the structure elements can be filled:
 *                pszTime - creation date of the response document
 *                          ("RECEIVE", "EXCHANGE" requests, optional)
 *                pszType - MIME type of the response document
 *                          ("RECEIVE", "EXCHANGE" request, optional)
 *                cbLength - Length of the response document
 *                           (must be 0 for "SEND" requests, otherwise optional)
 *                rcDocument - HTTP return code of the request (i.e. 200 for OK)
 *     pAuth, pointer to a structure that contains authentication info (optional)
 *            The authentication structure contains the following elements:
 *            cbSize - size of dataauth structure.
 *            pbData, pointer to an allocated block of data that contains the
 *                    authentication data to be sent to the client.
 *            fType, authentication type, must be 0.
 *            cbDataLength, size of the data block above.
 *
 * OUT: pcbDataRead, pointer to a variable that is updated with the size of the
 *      received data block.
 *
 * RETURN: HttpRc_t, return code. Refer to the type definition above for details.
 *     If the return value is HTTP_RC_COMMUNICATION, further error information
 *     can be retrieved with the httpGetError() service;
 *
 *
 */

HttpRc_t httpReply (HttpHandle_t p,
                    int rcDocument,
                    const HttpReplyBufferPtr_t pSettings,
                    const HttpAuthenticationPtr_t pAuth);

/**
 * FUNCTION: httpGetError
 *
 *  Return the return code of the last error, that a TCP/IP protocol service returned.
 *
 * PRE-Condition:
 *  The previous HTTP protocol service function failed with the return value
 *  HTTP_RC_COMMUNICATION. The caller invokes this function to retrieve
 *  the return code TCP/IP service that failed.
 *
 * POST-Condition:
 *  -
 *
 *
 * IN: p, instance handle
 *
 * RETURN: TcpRc_t, Return code of the failing TCP/IP service
 *
 *
 */

TcpRc_t  httpGetError (HttpHandle_t p);

/**
 * FUNCTION: httpGetServerStatus
 *
 *  Return the HTTP return code in the HTTP response header.
 *
 * PRE-Condition:
 *  The previous HTTP protocol service function failed with the return value
 *  HTTP_RC_SERVER. The caller invokes this function to retrieve
 *  the HTTP return code. (i.e. rc=404 means "resource not found")
 *
 * POST-Condition:
 *  -
 *
 *
 * IN: p, instance handle
 *
 * RETURN: int Return code of the failing HTTP request
 *
 *
 */

int  httpGetServerStatus (HttpHandle_t p);

/**
 * FUNCTION: httpGetBufferSize
 *
 *  Return the number of Bytes that is required for the instance object
 *
 * PRE-Condition:
 *  -
 *
 * POST-Condition:
 *  The function returns the size of the data buffer that the caller must
 *  allocate for the object instance memory, that is required to call httpOpen().
 *
 * IN: -
 *
 * RETURN: required size of the HTTP instance handle in Bytes.
 *
 *
 */

BufferSize_t httpGetBufferSize (void);


/**
 * FUNCTION: httpIsEox
 *
 *  Check wether the HTTP document has been received completely.
 *
 * PRE-Condition:
 *  the HTTP communication has been opened via httpOpen(), and the protocol
 *  is in a state where incoming data is expected:
 *  HTTP clients: AFTER httpWait() has been invoked
 *  HTTP Server: BEFORE httpReply () has been invoked
 *
 *
 * POST-Condition:
 *  The caller uses the function to get the exit conditions for a while loop
 *  that receives an incoming HTTP document, for example:
 *    while (!httpIsEox (http_handle))  {
 *        httpRead (...);
 *
 *        // do something interesting
 *        }
 *
 * IN: p, instance handle
 *
 * RETURN: true, if the document has been read entirely. false if
 * further receive packets are expected.
 *
 */

Bool_t httpIsEox (HttpHandle_t p);

extern void __syncml_setSendPercent(int p);
extern void __syncml_setReceivePercent(int p);

#ifdef _cplusplus
}
#endif

#endif

