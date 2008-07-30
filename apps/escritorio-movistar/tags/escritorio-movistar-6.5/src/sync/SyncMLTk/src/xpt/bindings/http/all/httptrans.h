/*************************************************************************/
/* module:          Communication Services, HTTP functions               */
/* file:            /src/xpt/bindings/http/httptrans.h                   */
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
 * HTTP protocol services, function prototypes and return codes
 *
 *
 */

#ifndef XPT_HTTP_TRANS_H
#define XPT_HTTP_TRANS_H


/*********************/
/* Required includes */
/*********************/

#include <xptTransport.h>
#include "xpttypes.h"
#include "xpt-http.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _http_transport_info {
    BufferSize_t                  cbSize;        // size of this structure
} HttpTransportInfo_t,  *HttpTransportInfoPtr_t;

struct _http_transport_service_client_info {
    TcpFirewallInfo_t       firewallInfo;
    StringBuffer_t          proxyString;
    StringBuffer_t          pchServerAddress;
    // %%% luz:2003-04-16 added SSL support
    Bool_t                  useSSL;
};

struct _http_transport_service_server_info {
    StringBuffer_t                pchSenderAddress;
    char *                        pchServerPort;
    Socket_t                      serverSocket;
};

typedef struct _http_transport_service_info {
    BufferSize_t                         cbSize;    // size of this structure
    Bool_t                               socketClient;
    union {
        struct _http_transport_service_client_info clientInfo;
        struct _http_transport_service_server_info serverInfo;
    } info;

} HttpTransportServiceInfo_t,  *HttpTransportServiceInfoPtr_t;

struct _http_transport_conn_client_info {
    char tmp;
};

struct _http_transport_conn_server_info {
    char tmp;
};

typedef enum  { HTTP_TRANSPORT_OPEN, HTTP_TRANSPORT_SENDING, HTTP_TRANSPORT_RECEIVING, HTTP_TRANSPORT_CLOSED } HttpTransportConnState_t ;

typedef struct _http_transport_conn_info {
    BufferSize_t                  cbSize;        // size of this structure
    Bool_t                        smlClient;
    Socket_t                      socket;
    HttpTransportServiceInfoPtr_t pServiceInfo;
    HttpHandle_t                  http;
    HttpDocumentContext_t         docContext;

    union {
        struct _http_transport_conn_client_info clientInfo;
        struct _http_transport_conn_server_info serverInfo;
    } info;
} HttpTransportConnInfo_t,  *HttpTransportConnInfoPtr_t;



Ret_t XPTAPI  HTTP_selectProtocol(void *privateTransportInfo,
                                     const char *metaInformation,
                                     unsigned int flags,
                                     void **pPrivateServiceInfo);

Ret_t XPTAPI HTTP_deselectProtocol(void *privateServiceInfo);


Ret_t XPTAPI HTTP_openCommunication(void *privateServiceInfo,
                                    int role,
                                    void **pPrivateConnectionInfo);

Ret_t XPTAPI  HTTP_closeCommunication(void *privateConnectionInfo);

Ret_t XPTAPI  HTTP_beginExchange(void *privateConnectionInfo);

Ret_t XPTAPI  HTTP_endExchange(void *privateConnectionInfo);

Ret_t XPTAPI  HTTP_receiveData(void *privateConnectionInfo,
                               void *buffer, size_t bufferLen,
                               size_t *dataLen);

Ret_t XPTAPI  HTTP_sendData(void *privateConnectionInfo,
                            const void *buffer, size_t bufferLen,
                            size_t *bytesSent);

Ret_t XPTAPI  HTTP_sendComplete(void *privateConnectionInfo);

Ret_t XPTAPI  HTTP_setDocumentInfo(void *privateConnectionInfo,
                                   const XptCommunicationInfo_t *pDoc);


Ret_t XPTAPI  HTTP_getDocumentInfo(void *privateConnectionInfo,
                                   XptCommunicationInfo_t *pDoc);


// LEO:
Ret_t XPTAPI  HTTP_cancelCommAsync(void *privateConnectionInfo);




#ifdef __cplusplus
}
#endif

#endif

