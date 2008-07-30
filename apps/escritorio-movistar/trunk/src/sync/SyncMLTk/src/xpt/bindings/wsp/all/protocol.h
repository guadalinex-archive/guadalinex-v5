/*************************************************************************/
/* module:          Communication Services, WSP Implementation Functions */
/* file:            src/xpt/all/protocol.h                               */
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
/*
 * Version Label
 *
 * RELEASE ??? CANDIDATE ?
 * 13.06.2000
 */


#ifndef WSP_PROTOCOL_H
#define WSP_PROTOCOL_H

#include <xpt.h>
#include <xptTransport.h>
#include <awsp.h>
#include <session.h>
#include <settings.h>
#include <transact.h>
#include <wspdef.h>

/**
 *  A structure to hold protocol connection information.
 **/

typedef struct {
   Long_t                     valid;
   awsp_ConnectionHandle      connHandle;
   unsigned int               mode;
   WspSession_t              *session;
   WspSettings_t             *protocolSettings;
   WspTransaction_t          *transaction;
} WspProtocolHandle_t;

/**
 * Public methods - used outside of handle.c
 *
 * I begin the method names with 'protocol' for ease in
 * identifying which 'class' (i.e. source file) they can be found in
 *
 **/

/* Session management */
Ret_t protocolCreateSession(WspProtocolHandle_t *pid) WSP_SECTION;
Ret_t protocolSuspendSession(WspProtocolHandle_t *pid) WSP_SECTION;
Ret_t protocolResumeSession(WspProtocolHandle_t *pid) WSP_SECTION;
Ret_t protocolTerminateSession(WspProtocolHandle_t *pid) WSP_SECTION;

/* Connection management */
Ret_t protocolInitializeConnection(WspProtocolHandle_t *pid) WSP_SECTION;
void protocolReleaseConnection(WspProtocolHandle_t *pid) WSP_SECTION;

/* Protocol Handle management */
Ret_t protocolInitializeHandle(const char *szSettings,
                               unsigned int mode,
                               void **protocolHandle) WSP_SECTION;
void protocolReleaseHandle(WspProtocolHandle_t *pid) WSP_SECTION;
awsp_BOOL protocolIsHandleValid(WspProtocolHandle_t *handle) WSP_SECTION;

/* Request processing */
Ret_t protocolInitializeRequest(WspProtocolHandle_t *pid) WSP_SECTION;
void protocolReleaseRequest(WspProtocolHandle_t *pid) WSP_SECTION;
Ret_t protocolSendRequestData(WspProtocolHandle_t *pid,
                              const void *buffer,
                              size_t bufferLen,
                              size_t *bytesSent) WSP_SECTION;
Ret_t protocolSendComplete(WspProtocolHandle_t *pid) WSP_SECTION;
Ret_t protocolReadResponseData(WspProtocolHandle_t *pid,
                               void *pbData,
                               size_t uDataSize,
                               size_t *puDataRead) WSP_SECTION;
Ret_t protocolSetRequestInfo(WspProtocolHandle_t *pid, const XptCommunicationInfo_t *pDoc) WSP_SECTION;
Ret_t protocolGetResponseInfo(WspProtocolHandle_t *pid, XptCommunicationInfo_t *pDoc) WSP_SECTION;


/* Private methods - only used within handle.c */
Ret_t sendRequest(WspProtocolHandle_t *pid) WSP_SECTION;
Ret_t processResponse(WspProtocolHandle_t *pid) WSP_SECTION;

#endif
