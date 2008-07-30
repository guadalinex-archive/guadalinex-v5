/*************************************************************************/
/* module:          Communication Services, WSP XPT API functions        */
/* file:            /src/xpt/all/xpt-wsp.h                               */
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
 * RELEASE RED CANDIDATE 1
 * 27.03.2000
 */


/**
 * WSP protocol services, function prototypes and return codes
 *
 *
 */

#ifndef XPT_WSP_H
#define XPT_WSP_H


/*********************/
/* Required includes */
/*********************/

#include <xptTransport.h>
#include <protocol.h>
#include <wspdef.h>

/*
#ifdef _WIN32
 #ifdef WSP_EXPORTING
  #define WSP_EXPORT __declspec(dllexport)
 #else
  #define WSP_EXPORT __declspec(dllimport)
 #endif
#else
 #define WSP_EXPORT
#endif
*/
#ifndef WSP_EXPORT
#define WSP_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Method to dynamically link binding
 **/

#ifdef LINK_TRANSPORT_STATICALLY
 #define initializeTransport wspInitializeTransport
#endif

WSP_EXPORT Ret_t XPTAPI initializeTransport(void) WSP_SECTION;

/**
 *  Methods required by the xptTransport.h binding interface.
 **/

Ret_t XPTAPI selectProtocol (void *privateTransportInfo,
                             const char *szSettings,
                             unsigned int flags,
                             void **pid) WSP_SECTION;

Ret_t XPTAPI deselectProtocol (void *pid) WSP_SECTION;

Ret_t XPTAPI openCommunication (void *pid,
                                int role,
                                void **pPrivateConnectionInfo) WSP_SECTION;

Ret_t XPTAPI closeCommunication (void *pid) WSP_SECTION;

Ret_t XPTAPI beginExchange(void *pid) WSP_SECTION;

Ret_t XPTAPI endExchange(void *pid) WSP_SECTION;


Ret_t XPTAPI receiveData(void *pid,
                         void *pbData,
                         size_t uDataSize,
                         size_t *puDataRead) WSP_SECTION;

Ret_t XPTAPI sendData(void *pid,
                      const void *buffer,
                      size_t bufferLen,
                      size_t *bytesSent) WSP_SECTION;

Ret_t XPTAPI sendComplete(void *pid) WSP_SECTION;

Ret_t XPTAPI setDocumentInfo(void *pid,
                             const XptCommunicationInfo_t *pDoc) WSP_SECTION;

Ret_t XPTAPI getDocumentInfo(void *pid,
                             XptCommunicationInfo_t *pDoc) WSP_SECTION;

#ifdef __cplusplus
}
#endif

#endif

