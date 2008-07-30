/*************************************************************************/
/* module:          Communication Services, WSP Transaction Functions    */
/* file:            src/xpt/all/transact.c                               */
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


#include <transact.h>
#include <wspdef.h>     /* For atoi on palm */
#include <wsputil.h>
#include <xptiwsp.h>    /* for getTransactionID */
#include <xpt.h>        /* For SML_ERRs */
#include <xptport.h>

#include <define.h>     /* For __PALM_OS__ definition on the Palm */

#ifdef __PALM_OS__
 #define sprintf StrPrintF
#endif

#include <stdio.h> /* for sprintf */


/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         Private Implementation Methods                  **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

/**
 *  transReadResponseData
 *       - Copies response body from transaction into provided input
 *         buffer.
 *
 *  IN     transaction     A pointer to the transaction structure
 *         pbData          The data buffer into which the body is copied
 *         uDataSize       The size of the data buffer
 *         puDataRead      A pointer to a size field that will be updated
 *                         to indicate how much data was copied into the
 *                         buffer.
 *
 *  OUT    transaction     The transaction has been updated to
 *                         remove all read data from the response cache.
 *         pbData          Response data has been copied into the buffer.
 *         puDataRead      The size field indicates much data was copied
 *                         into the buffer.
 *
 *  RETURN:
 *       An indication of whether the response body was successfully copied.
 **/
unsigned int transReadResponseData(WspTransaction_t *transaction,
                                void *pbData,
                                size_t uDataSize,
                                size_t *puDataRead)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("      transReadResponseData(%lx, %lx, %lu, %lu)\n",
             (unsigned long) transaction, (unsigned long) pbData,
             (unsigned long) uDataSize, (unsigned long) puDataRead));

   *puDataRead = 0;

   if ((transaction == NULL) || (pbData == NULL) || (puDataRead == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   if (uDataSize <= 0)
      return SML_ERR_A_XPT_INVALID_PARM;

   /**
    * Does this reponse need to distinguish between no data on a get vs.
    * put vs. post?
    **/
   if ((transaction == NULL) || (transaction->rspBody == NULL))
      return rc;                 /* No response data                           */

   /*
    * Copy the data to the provided buffer
    */

   if (uDataSize >= (transaction->rspBodySize)) {
      xppMemcpy(pbData, transaction->rspBody, transaction->rspBodySize);
      *puDataRead = transaction->rspBodySize;
//      return SML_ERR_A_XPT_EOX;  /* Indicate all data successfully read        */
      return SML_ERR_OK;         /* Indicate all data successfully read        */
   } else {                      /* Entire response will not fit in buffer     */
      xppMemcpy(pbData, transaction->rspBody, uDataSize);
      *puDataRead = uDataSize;
      adjustResponseBody(transaction, uDataSize);
      return SML_ERR_OK;         /* Indicate more data to be read              */
   } /* End buffer will not fit entire response body */

//   return SML_ERR_A_XPT_EOX;
   return SML_ERR_OK;         /* Indicate all data successfully read        */
} /* End transReadResponseData() */


/**
 *  transInitialize
 *       - allocates storage for a WSP transaction.
 *       - assigns the transaction id
 *
 *  OUT     transactionPtr    Updated to point to a transaction structure
 *
 *  RETURN
 *       An indicaton of whether the transaction structur was successfully
 *       allocated and initialized.
 *
 **/
unsigned int transInitialize(WspTransaction_t **transactionPtr)
{
   unsigned int  rc          = SML_ERR_OK;
   WspTransaction_t *transaction = NULL;

   XPTDEBUG(("      transInitialize(%lx)\n", (unsigned long) transactionPtr));

   if (transactionPtr == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (*transactionPtr != NULL) {
      transRelease(*transactionPtr);
   }

   transaction = (WspTransaction_t *) xppMalloc(sizeof(WspTransaction_t));

   if (transaction == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(transaction, 0, sizeof(WspTransaction_t));
   transaction->id = getTransactionID();

   *transactionPtr = transaction;

   return rc;
} /* End transInitialize() */

/**
 *  transRelease
 *       - releases all storage associated with the WSP transaction.
 *
 *  IN     transaction     A pointer to a transaction structure
 *
 **/
void transRelease(WspTransaction_t *transaction) {

   XPTDEBUG(("      transRelease(%lx)...\n", (unsigned long) transaction));

   if (transaction == NULL) return;

   releaseDocumentInfo(transaction);
   releaseTransactionRequestData(transaction);
   releaseTransactionResponseData(transaction);

   xppFree(transaction);

} /* End transRelease() */

/**
 *  releaseTransactionRequestData
 *       - releases all storage associated with the WSP transaction request.
 *
 *  IN     transaction     A pointer to the wsp transaction structure
 *
 **/
void releaseTransactionRequestData(WspTransaction_t *transaction) {


   XPTDEBUG(("        releaseTransactionRequestData(%lx)\n", (unsigned long) transaction));

   if (transaction == NULL) return;

/* method contains a static string, nothing to release */
   transaction->method = NULL;

   xppFree(transaction->uri);
   transaction->uri = NULL;

   xppFree(transaction->reqHdr);
   transaction->reqHdr = NULL;
   transaction->reqHdrSize = 0;

   xppFree(transaction->reqBody);
   transaction->reqBody = NULL;
   transaction->reqBodySize = 0;

} /* End releaseTransactioRequestData() */

/**
 *  releaseTransactionResponseData
 *       - releases all storage associated with the WSP transaction response.
 *
 *  IN     transaction     A pointer to the wsp transaction structure
 *
 **/
void releaseTransactionResponseData(WspTransaction_t *transaction) {

   XPTDEBUG(("        releaseTransactionResponseData(%lx)\n", (unsigned long) transaction));

   if (transaction == NULL) return;

   xppFree(transaction->rspHdr);
   transaction->rspHdr = NULL;
   transaction->rspHdrSize = 0;

   xppFree(transaction->rspBody);
   transaction->rspBody = NULL;
   transaction->rspBodySize = 0;

} /* End releaseTransactionResponseData() */


/**
 *  transSendRequestOverSession
 *       - Does an abstract WSP methodInvoke for a session connection
 *       - waits for method result
 *       - responds with method acknowledgement.
 *
 *  IN:    sessionHandle   A pointer to a session handle from the connect
 *         transaction     A pointer to a transaction structure which
 *                         contains the method request parameters.
 *
 *  OUT:   transaction     The transaction structure has been updated to
 *                         contain the method response information.
 *
 *  RETURN:
 *       An indication of whether the method invocation was successful
 **/
unsigned int transSendRequestOverSession(awsp_SessionHandle sessionHandle,
                                         WspTransaction_t  *transaction)
{
   unsigned int rc          = SML_ERR_OK;
   awsp_Rc_t   aRc          = AWSP_RC_OK;

   XPTDEBUG(("      transSendRequestOverSession(%lx, %lx)...\n",
             (unsigned long) sessionHandle, (unsigned long) transaction));

   if ((sessionHandle == NULL) || (transaction == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   /* Send WSP request and wait for response                                   */
   aRc = awsp_methodInvoke_req(sessionHandle,
                               transaction->id,
                               transaction->method,
                               transaction->uri,
                               transaction->reqHdr,
                               transaction->reqHdrSize,
                               transaction->reqBody,
                               transaction->reqBodySize);

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp method invoke request failed.");
      rc = SML_ERR_A_XPT_COMMUNICATION;
   } else
      rc = getMethodResponse(sessionHandle, transaction);

   /**
    * Need to verify under what conditions the response must be sent, and
    * decide which rc - the method invoke, method response, or method
    * confirmation rc - should be returned from this function...
    **/
   if (rc == SML_ERR_OK)
      /* We don't really care if our confirmation failed...          */
//      aRc = awsp_methodResult_rsp(sessionHandle,
      awsp_methodResult_rsp(sessionHandle,
                            transaction->id,
                            NULL,         /* No acknowledgement hdrs for now */
                            0);

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp method result response failed.");
      rc = SML_ERR_A_XPT_COMMUNICATION;
   }

   return rc;
} /* End transSendRequestOverSession() */


unsigned int getMethodResponse(awsp_SessionHandle sessionHandle,
                               WspTransaction_t  *transaction)
{
   unsigned int rc          = SML_ERR_OK;
   awsp_Rc_t   aRc         = AWSP_RC_OK;

   XPTDEBUG(("        getMethodResponse(%lx, %lx)\n",
             (unsigned long) sessionHandle, (unsigned long) transaction));

   if ((sessionHandle == NULL) || (transaction == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   /* What if our objects are deleted from under us while we wait???           */
   aRc = awsp_get_methodResult_ind(sessionHandle,
                                   transaction->id,
                                   &(transaction->status),
                                   NULL,
                                   &(transaction->rspHdrSize),
                                   NULL,
                                   &(transaction->rspBodySize));

   if (aRc == AWSP_RC_BUFFER_TOO_SMALL) {
      if (transaction->rspHdrSize != 0) {
         transaction->rspHdr = (char *)xppMalloc(1 + transaction->rspHdrSize);
         if (transaction->rspHdr == NULL)
            transaction->rspHdrSize = 0;
         else
            xppMemset(transaction->rspHdr, 0, 1 + transaction->rspHdrSize);
      }

      if (transaction->rspBodySize != 0) {
         transaction->rspBody = (void *) xppMalloc(transaction->rspBodySize);
         if (transaction->rspBody == NULL)
            transaction->rspBodySize = 0;
         else
            xppMemset(transaction->rspBody, 0, transaction->rspBodySize);
      }

      aRc = awsp_get_methodResult_ind(sessionHandle,
                                      transaction->id,
                                      &(transaction->status),
                                      transaction->rspHdr,
                                      &(transaction->rspHdrSize),
                                      transaction->rspBody,
                                      &(transaction->rspBodySize));
   } /* End buffer too small */

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp method result indicator failed.");
      rc = SML_ERR_A_XPT_COMMUNICATION;
   }

   /* Create a new document info object and update it with the post results */
   createDocumentInfo(transaction);
   updateDocInfoFromResponse(transaction, transaction->pDoc);

   return rc;
} /* End getMethodResponse() */

/**
 *  transSendRequestNoSession
 *       - Does a connectionless abstract WSP methodInvoke
 *       - waits for method result
 *
 *  IN:    connHandle      A pointer to a connection handle
 *         transaction     A pointer to a transaction structure which
 *                         contains the method request parameters.
 *
 *  OUT:   transaction     The transaction structure has been updated to
 *                         contain the method response information.
 *
 *  RETURN:
 *       An indication of whether the method invocation was successful
 **/
unsigned int transSendRequestNoSession(awsp_ConnectionHandle connHandle,
                                       WspTransaction_t     *transaction)
{
   unsigned int rc          = SML_ERR_OK;
   awsp_Rc_t   aRc         = AWSP_RC_OK;

   XPTDEBUG(("      transSendRequestNoSession(%lx, %lx)...\n",
             (unsigned long) connHandle, (unsigned long) transaction));

   if ((connHandle == NULL) || (transaction == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   /* Send WSP request and wait for response                                   */
   aRc = awsp_unit_methodInvoke_req(connHandle,
                                    transaction->id,
                                    transaction->method,
                                    transaction->uri,
                                    transaction->reqHdr,
                                    transaction->reqHdrSize,
                                    transaction->reqBody,
                                    transaction->reqBodySize);

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp method invoke request failed.");
      rc = SML_ERR_A_XPT_COMMUNICATION;
   } else
      rc = getUnitMethodResponse(connHandle, transaction);

   return rc;
} /* End transSendRequestNoSession() */

unsigned int getUnitMethodResponse(awsp_ConnectionHandle connHandle,
                                   WspTransaction_t     *transaction)
{
   unsigned int rc          = SML_ERR_OK;
   awsp_Rc_t   aRc         = AWSP_RC_OK;

   XPTDEBUG(("        getUnitMethodResponse(%lx, %lx)\n",
             (unsigned long) connHandle, (unsigned long) transaction));

   if ((connHandle == NULL) || (transaction == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   aRc = awsp_get_unit_methodResult_ind(connHandle,
                                        transaction->id,
                                        &(transaction->status),
                                        NULL,
                                        &(transaction->rspHdrSize),
                                        NULL,
                                        &(transaction->rspBodySize));

   if (aRc == AWSP_RC_BUFFER_TOO_SMALL) {
      if (transaction->rspHdrSize != 0)
         transaction->rspHdr = (char *)xppMalloc(transaction->rspHdrSize);
         if (transaction->rspHdr == NULL)
            transaction->rspHdrSize = 0;
         else
            xppMemset(transaction->rspHdr, 0, 1 + transaction->rspHdrSize);

      if (transaction->rspBodySize != 0)
         transaction->rspBody = (void *) xppMalloc(transaction->rspBodySize);
         if (transaction->rspBody == NULL)
            transaction->rspBodySize = 0;
         else
            xppMemset(transaction->rspBody, 0, 1 + transaction->rspBodySize);

      aRc = awsp_get_unit_methodResult_ind(connHandle,
                                           transaction->id,
                                           &(transaction->status),
                                           transaction->rspHdr,
                                           &(transaction->rspHdrSize),
                                           transaction->rspBody,
                                           &(transaction->rspBodySize));
   } /* End buffer too small */

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp unit method result indication failed.");
      rc = SML_ERR_A_XPT_COMMUNICATION;
   }

   /* Create a new document info object and update it with the post results */
   createDocumentInfo(transaction);
   updateDocInfoFromResponse(transaction, transaction->pDoc);

   return rc;
} /* End getUnitMethodResponse() */


/**
 *  transCreateRequest
 *       - Builds the http request uri and headers.
 *
 *  IN     transaction     A pointer to a transaction structure
 *         host            The host name of the sync server to which the
 *                         request is directed.
 *         pDoc            A pointer to a structure that contains information
 *                         about the data to be transmitted.
 *
 *  OUT    transaction     The transaction structure has been updated to
 *                         contain the request info.
 *
 *  RETURN:
 *       An indication of whether the URI was successfully composed.
 **/
unsigned int transCreateRequest(WspTransaction_t *transaction,
                                WspHttpParms_t   *httpParms,
                                char *host,
                                awsp_BOOL sessionMode)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("      transCreateRequest(%lx, %s, %i)...\n",
             (unsigned long) transaction, host, (int) sessionMode));

   if ((transaction == NULL) || (host == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   rc = composeRequestUri(transaction,
                          host);

   if (rc == SML_ERR_OK)
      rc = composeHttpHeader(transaction, httpParms, sessionMode);

   return rc;
} /* End of transCreateRequest() */

/**
 *  composeRequestUri
 *       - Builds uri from input host and document name
 *
 *  IN     transaction     A pointer to the wsp transaction structure
 *         host            The ip address or dns name of the host
 *
 *  OUT    transaction     The transaction structure has been updated to
 *                         contain the URI.
 *
 *  RETURN:
 *       An indication of whether the URI was successfully composed.
 **/
unsigned int composeRequestUri(WspTransaction_t *transaction,
                            const char *host)
{
   const char *HTTP     = "http://";
   const char *BKBK     = "//";
   int         uriSize  = 0;

   XPTDEBUG(("        composeRequestUri(%lx, %s)\n", (unsigned long) transaction, host));

   if ((transaction == NULL) || (host == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   uriSize  = xppStrlen(HTTP) + xppStrlen(host) + xppStrlen(BKBK) + 1;

   /* Allocate and initialize storage                                          */
   transaction->uri = (char *) xppMalloc(uriSize);

   if (transaction->uri == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(transaction->uri, 0 , uriSize);

   /* Build uri                                                                */
   xppStrcat(transaction->uri, HTTP);
   xppStrcat(transaction->uri, host);
   xppStrcat(transaction->uri, BKBK);

   return SML_ERR_OK;
} /* End composeRequestUri */

/**
 *  composeHttpHeader
 *       - Builds http request header
 *
 *  IN     transaction     A pointer to the wsp transaction structure
 *         sessionMode     An indication of whether the request is
 *                         going across an established session or whether
 *                         it is connectionless.  AWSP_TRUE means session.
 *
 *  OUT    transaction     The transaction structure has been updated to
 *                         contain the request header.
 *
 *  RETURN:
 *       An indication of whether the request header was successfully composed.
 *
 * Notes:
 *
 * Required Headers:
 *   General:           Cache-Control: no store
 *                      Cache-Control: private
 *   Request:           Accept: application/vnd.syncml-xml, application/vnd.syncml-wbxml
 *                      Accept-Charset: UTF-8
 *                      User-Agent: HTTP Client [en] (WinNT; I)
 *                      From:
 *                      Authorization:
 **/
unsigned int composeHttpHeader(WspTransaction_t *transaction, WspHttpParms_t *httpParms,
                               awsp_BOOL sessionMode)
{
   const char *tmpS = NULL;   /* temp for static request hdrs */
   size_t      lenS = 0;

   XPTDEBUG(("        composeHttpHeader(%lx, %lx, %i)\n",
             (unsigned long) transaction, (unsigned long) httpParms, (int) sessionMode));

   if (transaction == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;


   transaction->reqHdr = NULL;
   transaction->reqHdrSize = 0;

   /*
    * If not in session mode, request header must include 'static' headers
    * that session would have established
    */
   if (sessionMode == AWSP_FALSE) {
      tmpS = httpGetStaticRequestHdrs(httpParms);
      lenS = xppStrlen(tmpS);
      if ((tmpS != NULL) && (lenS > 0)) {
         transaction->reqHdr = (char *) xppMalloc(lenS + 1);
         transaction->reqHdrSize = lenS;
         if (transaction->reqHdr != NULL)
            xppStrcpy(transaction->reqHdr, tmpS);
      } /* End got static hdrs */
   } /* End not session mode */

   return SML_ERR_OK;
} /* End composeHttpHeader */

unsigned int transBuildDynamicRequestHdr(WspTransaction_t *transaction,
                                         WspHttpParms_t *httpParms)
{
   char       *tmp  = NULL;   /* temp for xppRealloc */
   const char *tmpR = NULL;   /* temp for dynamic request hdrs */
   size_t      lenR = 0;
   size_t      lenS = 0;
   char        cLen[32];

   XPTDEBUG(("        transBuildDynamicRequestHdr(%lx, %lx)\n",
            (unsigned long) transaction, (unsigned long) httpParms));

   if (transaction == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (transaction->pDoc != NULL) {
      xppMemset(cLen, 0, sizeof cLen);
      sprintf(cLen, "%lu", (unsigned long) transaction->pDoc->cbLength);
      tmpR = httpGetRequestHdrs(httpParms, transaction->pDoc->mimeType, cLen);
      lenR = xppStrlen(tmpR);

   } else {
      tmpR = httpGetRequestHdrs(httpParms, NULL, NULL);
      lenR = xppStrlen(tmpR);
   }


   if ((tmpR != NULL) && (lenR > 0)) {
      if (transaction->reqHdr == NULL) {
         transaction->reqHdr = (char *) xppMalloc(lenR + 1);
         if (transaction->reqHdr != NULL)
            xppStrcpy(transaction->reqHdr, tmpR);
         transaction->reqHdrSize = lenR;
      } else {
         /* Need to append dynamic to static.  Remember to remove last '\n' from static */
         lenS = xppStrlen(transaction->reqHdr);
         tmp = (char *) xppRealloc(transaction->reqHdr, lenS + lenR);
         if (tmp != NULL) {
            transaction->reqHdr = tmp;
            tmp = tmp + lenS - 1;      /* Bump pointer to last '\n' in static */
            xppStrcpy(tmp, tmpR);         /* Copy dynamic into buffer */
         }
      } /* End append dynamic to static */
   } /* End dynamic headers */

   return SML_ERR_OK;
} /* End transBuildDynamicRequestHdr() */

unsigned int transAddRequestData(WspTransaction_t *transaction,
                              const void *pbData,
                              size_t uDataSize)
{
   unsigned int rc = SML_ERR_OK;
   void *temp = NULL;

   XPTDEBUG(("      transAddRequestData(%lx, %lx, %lu)...\n",
            (unsigned long) transaction, (unsigned long) pbData, (unsigned long) uDataSize));

   if (transaction == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (pbData == NULL)
      return rc;

   if (transaction->reqBody == NULL) {          /* First block of data         */

      transaction->reqBody = (void *) xppMalloc(uDataSize);
      if (transaction->reqBody == NULL)
         return SML_ERR_A_XPT_MEMORY;           /* Failed to allocate storage  */

      /* Copy data to request buffer                                           */
      xppMemcpy(transaction->reqBody, pbData, uDataSize);
      transaction->reqBodySize = uDataSize;

   } else {                                     /* Accumulating data           */

      temp = (void *) xppRealloc(transaction->reqBody,
                              (transaction->reqBodySize) + uDataSize);
      if (temp != NULL) {
         transaction->reqBody = temp;
         xppMemcpy(((transaction->reqBodySize) + (char *)(transaction->reqBody)), pbData, uDataSize);
         transaction->reqBodySize = transaction->reqBodySize + uDataSize;
      } else {                                  /* Failed to allocate storage  */
         /**
          *What do I do if i can't allocate more storage?  I haven't sent
          * anything yet !!!
          **/
         rc = SML_ERR_A_XPT_COMMUNICATION;
      } /* End failed xppReallocation */

   } /* End of accumulating data */

   return rc;
} /* End of transAddRequestData() */


/**
 *  updateDocInfoFromResponse
 *       - Copies response data from HTTP response header into the
 *         communicaton info structure.
 *
 *  IN     transaction     A pointer to a transaction structure that
 *                         contains the completed transaction.
 *         pDoc            A pointer to a structure into which the
 *                         response data will be copied
 *
 *  OUT    pDoc            The structure has been updated with response
 *                         information.
 *
 **/
unsigned int updateDocInfoFromResponse(WspTransaction_t *transaction,
                                    XptCommunicationInfo_t *pDoc)
{
   unsigned int rc = SML_ERR_OK;
   const char *docName       = NULL;
   char       *tmp           = NULL;

   XPTDEBUG(("        updateDocInfoFromResponse(%lx, %lx)...\n",
             (unsigned long) transaction, (unsigned long) pDoc));

   if ((transaction == NULL) || (pDoc == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   tmp = getHeaderTag("Content-Length", transaction->rspHdr);
   if (tmp != NULL)
      pDoc->cbLength = (size_t) atoi(tmp);
   xppFree(tmp);

   tmp = getHeaderTag("Content-Type", transaction->rspHdr);

   if (tmp != NULL){
      if (xppStrlen(tmp) > XPT_DOC_TYPE_SIZE)
         xppStrncpy(pDoc->mimeType, tmp, XPT_DOC_TYPE_SIZE);
      else
         xppStrcpy(pDoc->mimeType, tmp);
   }

   xppFree(tmp);

   docName = "unknown";

   if (docName != NULL)
      xppStrncpy(pDoc->docName, docName, xppStrlen(docName));

   return rc;
} /* End of updateDocInfoFromResponse() */

/**
 *  adjustResponseData
 *       - Removes read data from the response body cache.
 *
 *  IN     transaction     A pointer to the transaction structure containing
 *                         the response body.
 *         size            The number of bytes of data that need to be
 *                         removed from the response body.
 *
 *  OUT    transaction     The response body of the transaction contains
 *                         only unread data.
 **/
void adjustResponseBody(WspTransaction_t *transaction, size_t size)
{

   XPTDEBUG(("        adjustResponseBody(%lx, %lu)\n",
             (unsigned long) transaction, (unsigned long) size));

   if ((transaction == NULL) || (transaction->rspBody == NULL) || (size <= 0))
      return;

   if (size >= transaction->rspBodySize) {   /* All data was read    */
      xppFree(transaction->rspBody);
      transaction->rspBody = NULL;
      transaction->rspBodySize = 0;
   } else {
      transaction->rspBodySize = transaction->rspBodySize - size;
      xppMemmove(transaction->rspBody, (char *)(transaction->rspBody)+size, transaction->rspBodySize);
      /* If xppRealloc fails we're ok - our 'unread' data is at beginning of original
       * buffer and our size has been corrected.                               */
      xppRealloc(transaction->rspBody, transaction->rspBodySize);
   } /* End reduce response body */

} /* End adjustResponseBody() */


unsigned int transSetDocInfo(WspTransaction_t *transaction,
                          const XptCommunicationInfo_t *pDoc)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("      transSetDocInfo(%lx, %lx)...\n",
             (unsigned long) transaction, (unsigned long) pDoc));

   if ((transaction == NULL) || (pDoc == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   rc = createDocumentInfo(transaction);

   if (rc != SML_ERR_OK)
      return rc;

   rc = copyDocumentInfo(transaction->pDoc, pDoc);
   rc = updateRequestUriWithDocName(transaction);

   if (rc != SML_ERR_OK)
      releaseDocumentInfo(transaction);

   return rc;
} /* End of transSetDocInfo() */

/*         docName         The path/name of the document on the host server. */
unsigned int updateRequestUriWithDocName(WspTransaction_t *transaction)
{
   int   docNameSize = 0;
   int   uriOldSize  = 0;
   char *temp        = NULL;

   XPTDEBUG(("        updateRequestUriWithDocName(%lx)\n", (unsigned long) transaction));

   if ((transaction == NULL) || (transaction->pDoc == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   if (transaction->uri == NULL)
      return SML_ERR_A_XPT_COMMUNICATION;

   if (transaction->pDoc->docName != NULL) {
      /*
       * Need to handle case where docname begins with '/', so that we
       * don't get '//' between uri and docname.
       */

      uriOldSize = xppStrlen(transaction->uri);
      docNameSize = xppStrlen(transaction->pDoc->docName);

      temp = (char *) xppRealloc(transaction->uri, uriOldSize + docNameSize + 1);

      if (temp == NULL)
         return SML_ERR_A_XPT_MEMORY;

      xppStrncat(temp + uriOldSize, transaction->pDoc->docName, docNameSize);

      transaction->uri = temp;

   } /* End docName */

   return SML_ERR_OK;
} /* End of updateRequestUriWithDocName() */

unsigned int transGetDocInfo(WspTransaction_t *transaction,
                          XptCommunicationInfo_t *pDoc)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("      transGetDocInfo(%lx, %lx)...\n",
             (unsigned long) transaction, (unsigned long) pDoc));

   if ((transaction == NULL) || (pDoc == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   rc = copyDocumentInfo(pDoc, transaction->pDoc);

   if (transaction->status != AWSP_SUCCESS) { /* If not HTTP response code 200 */
      reportHttpError(transaction);
      rc = SML_ERR_A_XPT_COMMUNICATION;
   }


   return rc;
} /* End of transGetDocInfo() */

unsigned int createDocumentInfo(WspTransaction_t *transaction)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("        createDocumentInfo(%lx)\n", (unsigned long) transaction));

   if (transaction == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (transaction->pDoc != NULL)
      releaseDocumentInfo(transaction);

   transaction->pDoc = (XptCommunicationInfo_t *) xppMalloc(sizeof(XptCommunicationInfo_t));

   if (transaction->pDoc == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(transaction->pDoc, 0, sizeof(XptCommunicationInfo_t));

   return rc;
} /* End of createDocumentInfo() */

unsigned int copyDocumentInfo(XptCommunicationInfo_t *dest,
                           const XptCommunicationInfo_t *source)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("        copyDocumentInfo(%lx, %lx)\n",
             (unsigned long) dest, (unsigned long) source));

   if ((dest == NULL) || (source == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   dest->cbSize = source->cbSize;
   dest->cbLength = source->cbLength;
   xppStrcpy(dest->mimeType, source->mimeType);
   xppStrcpy(dest->docName, source->docName);

   return rc;
} /* End of copyDocumentInfo() */

void releaseDocumentInfo(WspTransaction_t *transaction)
{
   XPTDEBUG(("        releaseDocumentInfo(%lx)\n", (unsigned long) transaction));
   if (transaction == NULL) return;

   xppFree(transaction->pDoc);

   transaction->pDoc = NULL;

} /* End of releaseDocumentInfo() */

void reportHttpError(WspTransaction_t *transaction)
{
   const char *msg = "HTTP 1.1 response is not 'OK' (200)";

   XPTDEBUG(("        reportHttpError(%lx)...\n", (unsigned long) transaction));

   if (transaction->status ==  AWSP_CONTINUE)
      msg = "HTTP 1.1 response code 100 - 'Continue'";
   else if (transaction->status ==  AWSP_SWITCH_PROTOCOL)
      msg = "HTTP 1.1 response code 101 - 'Switch Protocol'";
   else if (transaction->status ==  AWSP_CREATED)
      msg = "HTTP 1.1 response code 201 - 'Created'";
   else if (transaction->status ==  AWSP_ACCEPTED)
      msg = "HTTP 1.1 response code 202 - 'Accepted'";
   else if (transaction->status ==  AWSP_NON_AUTHORITATIVE_ANSWER)
      msg = "HTTP 1.1 response code 203 - 'Non-authoritative Answer'";
   else if (transaction->status ==  AWSP_NO_CONTENT)
      msg = "HTTP 1.1 response code 204 - 'No Content'";
   else if (transaction->status ==  AWSP_RESET_CONTENT)
      msg = "HTTP 1.1 response code 205 - 'Reset Content'";
   else if (transaction->status ==  AWSP_PARTIAL_CONTENT)
      msg = "HTTP 1.1 response code 206 - 'Partial Content'";
   else if (transaction->status ==  AWSP_MULTIPLE_CHOICES)
      msg = "HTTP 1.1 response code 300 - 'Multiple Choices'";
   else if (transaction->status ==  AWSP_MOVED_PERMANENTLY)
      msg = "HTTP 1.1 response code 301 - 'Moved Permanently'";
   else if (transaction->status ==  AWSP_MOVED_TEMPORARILY)
      msg = "HTTP 1.1 response code 302 - 'Moved Temporarily'";
   else if (transaction->status ==  AWSP_SEE_OTHER)
      msg = "HTTP 1.1 response code 303 - 'See Other'";
   else if (transaction->status ==  AWSP_NOT_MODIFIED)
      msg = "HTTP 1.1 response code 304 - 'Not Modified'";
   else if (transaction->status ==  AWSP_USE_PROXY)
      msg = "HTTP 1.1 response code 305 - 'Use Proxy'";
   else if (transaction->status ==  AWSP_BAD_REQUEST)
      msg = "HTTP 1.1 response code 400 - 'Bad Request'";
   else if (transaction->status ==  AWSP_UNAUTHORIZED)
      msg = "HTTP 1.1 response code 401 - 'Unauthorized'";
   else if (transaction->status ==  AWSP_PAYMENT_REQUIRED)
      msg = "HTTP 1.1 response code 402 - 'Payment Required'";
   else if (transaction->status ==  AWSP_FORBIDDEN)
      msg = "HTTP 1.1 response code 403 - 'Forbidden'";
   else if (transaction->status ==  AWSP_NOT_FOUND)
      msg = "HTTP 1.1 response code 404 - 'Not Found'";
   else if (transaction->status ==  AWSP_METHOD_NOT_ALLOWED)
      msg = "HTTP 1.1 response code 405 - 'Method Not Allowed'";
   else if (transaction->status ==  AWSP_NOT_ACCEPTABLE)
      msg = "HTTP 1.1 response code 406 - 'Not Acceptable'";
   else if (transaction->status ==  AWSP_PROXY_AUTH_REQUIRED)
      msg = "HTTP 1.1 response code 407 - 'Proxy Authentication Required'";
   else if (transaction->status ==  AWSP_REQUEST_TIMEOUT)
      msg = "HTTP 1.1 response code 408 - 'Request Timed Out'";
   else if (transaction->status ==  AWSP_CONFLICT)
      msg = "HTTP 1.1 response code 409 - 'Conflict'";
   else if (transaction->status ==  AWSP_GONE)
      msg = "HTTP 1.1 response code 410 - 'Gone'";
   else if (transaction->status ==  AWSP_LENGTH_REQUIRED)
      msg = "HTTP 1.1 response code 411 - 'Length Required'";
   else if (transaction->status ==  AWSP_PRECONDITION_FAILED)
      msg = "HTTP 1.1 response code 412 - 'Precondition Failed'";
   else if (transaction->status ==  AWSP_REQUEST_ENTITY_TOO_LARGE)
      msg = "HTTP 1.1 response code 413 - 'Request Entity Too Large'";
   else if (transaction->status ==  AWSP_REQUEST_URI_TOO_LARGE)
      msg = "HTTP 1.1 response code 414 - 'Request URI Too Large'";
   else if (transaction->status ==  AWSP_UNSUPPORTED_MEDIA_TYPE)
      msg = "HTTP 1.1 response code 415 - 'Unsupported Media Type'";
   else if (transaction->status ==  AWSP_INTERNAL_SERVER_ERROR)
      msg = "HTTP 1.1 response code 500 - 'Internal Server Error'";
   else if (transaction->status ==  AWSP_NOT_IMPLEMENTED)
      msg = "HTTP 1.1 response code 501 - 'Not Implemented'";
   else if (transaction->status ==  AWSP_BAD_GATEWAY)
      msg = "HTTP 1.1 response code 502 - 'Bad Gateway'";
   else if (transaction->status ==  AWSP_SERVICE_UNAVAILABLE)
      msg = "HTTP 1.1 response code 503 - 'Service Unavailable'";
   else if (transaction->status ==  AWSP_GATEWAY_TIMEOUT)
      msg = "HTTP 1.1 response code 504 - 'Gateway Timeout'";
   else if (transaction->status ==  AWSP_HTTP_VERSION_UNSUPPORTED)
      msg = "HTTP 1.1 response code 505 - 'HTTP Version is unsupported'";

   setLastError(transaction->status, msg);
   XPTDEBUG(("        reportHttpError() returning...\n"));

} /* End reportHttpError() */

