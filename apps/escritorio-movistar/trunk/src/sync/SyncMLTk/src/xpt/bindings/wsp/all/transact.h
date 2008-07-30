/*************************************************************************/
/* module:          Communication Services, WSP Transaction Functions    */
/* file:            src/xpt/all/transact.h                               */
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

#ifndef WSP_TRANSACT_H
#define WSP_TRANSACT_H

#include <awsp.h>
#include <xpt.h>     /* For communication info */
#include <wsphttp.h>

#include <xptTransport.h>
#include <wspdef.h>

/**
 *  A structure to hold WSP transaction-specific information.
 **/
typedef struct {

   unsigned long        id;

   /* Document Information */
   XptCommunicationInfo_t *pDoc;

   /* Request information */
   char                *method;
   char                *uri;
   char                *reqHdr;
   size_t               reqHdrSize;
   void                *reqBody;
//   char                *reqBody;
   size_t               reqBodySize;

   /* Response information */
   awsp_StatusCode_t    status;
   char                *rspHdr;
   size_t               rspHdrSize;
   void                *rspBody;
//   char                *rspBody;
   size_t               rspBodySize;
} WspTransaction_t;


/* Methods invoked from outside of transact.c */
unsigned int transAddRequestData(WspTransaction_t *transaction,
                                 const void       *pbData,
                                 size_t            uDataSize) WSP_SECTION;

unsigned int transCreateRequest(WspTransaction_t *transaction,
                                WspHttpParms_t   *httpParms,
                                char             *host,
                                awsp_BOOL         sessionMode) WSP_SECTION;

unsigned int transInitialize(WspTransaction_t **transaction) WSP_SECTION;

unsigned int transBuildDynamicRequestHdr(WspTransaction_t *transaction,
                                         WspHttpParms_t   *httpParms) WSP_SECTION;

unsigned int transSendRequestOverSession(awsp_SessionHandle sessionHandle,
                                         WspTransaction_t  *transaction) WSP_SECTION;

unsigned int transSendRequestNoSession(awsp_ConnectionHandle  connHandle,
                                       WspTransaction_t      *transaction) WSP_SECTION;

void transRelease(WspTransaction_t *transaction) WSP_SECTION;

unsigned int transReadResponseData(WspTransaction_t *transaction,
                                   void             *pbData,
                                   size_t            uDataSize,
                                   size_t           *puDataRead) WSP_SECTION;

unsigned int transSetDocInfo(WspTransaction_t             *transaction,
                             const XptCommunicationInfo_t *pDoc) WSP_SECTION;

unsigned int transGetDocInfo(WspTransaction_t       *transaction,
                             XptCommunicationInfo_t *pDoc) WSP_SECTION;


/* Methods invoked only from within transact.c */
void  adjustResponseBody(WspTransaction_t *transaction,
                         size_t            size) WSP_SECTION;

unsigned int composeHttpHeader(WspTransaction_t *transaction,
                               WspHttpParms_t   *httpParms,
                               awsp_BOOL         sessionMode) WSP_SECTION;

unsigned int composeRequestUri(WspTransaction_t *transaction,
                               const char       *host) WSP_SECTION;

unsigned int copyDocumentInfo(XptCommunicationInfo_t       *dest,
                              const XptCommunicationInfo_t *source) WSP_SECTION;

unsigned int createDocumentInfo(WspTransaction_t *transaction) WSP_SECTION;

unsigned int getMethodResponse(awsp_SessionHandle sessionHandle,
                               WspTransaction_t  *transaction) WSP_SECTION;

unsigned int getUnitMethodResponse(awsp_ConnectionHandle connHandle,
                                   WspTransaction_t     *transaction) WSP_SECTION;

void releaseDocumentInfo(WspTransaction_t *transaction) WSP_SECTION;

void releaseTransactionRequestData(WspTransaction_t *transaction) WSP_SECTION;

void releaseTransactionResponseData(WspTransaction_t *transaction) WSP_SECTION;

void reportHttpError(WspTransaction_t *transaction) WSP_SECTION;

unsigned int updateDocInfoFromResponse(WspTransaction_t       *transaction,
                                       XptCommunicationInfo_t *pDoc) WSP_SECTION;

unsigned int updateRequestUriWithDocName(WspTransaction_t *transaction) WSP_SECTION;

#endif
