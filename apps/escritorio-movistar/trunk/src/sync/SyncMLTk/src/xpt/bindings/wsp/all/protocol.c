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

#include <protocol.h>
#include <wsphttp.h>

#include <xptport.h>

/**
 * Eyecatcher to validate WspProtocolHandle_t structure
 **/
#define WSP_PROTOCOL_HANDLE_VALID_FLAG 0x50535741L    // 'AWSP'

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         Private Implementation Methods                  **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/


Ret_t protocolCreateSession(WspProtocolHandle_t *pid) {

   Ret_t rc = SML_ERR_OK;

   XPTDEBUG(("protocolCreateSession(%lx)\n", (unsigned long) pid));

   if (pid == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   /**
    *    Connection-mode (session or connectionless) must be specified in szSettings?
    *    If connection-mode:
    */
   if ((pid->protocolSettings != NULL) &&
       (pid->protocolSettings->createSession == AWSP_TRUE))

      rc = sessionCreate(pid->connHandle,
                         pid->protocolSettings->requestedCapabilities,
                         httpGetStaticRequestHdrs(pid->protocolSettings->httpParms),
                         &(pid->session));

   /*
    * If the RC from sessionCreate is invalid_parm then we should convert it
    * to something else, since the parm to protocolCreateSession was ok
    */
   if (rc == SML_ERR_A_XPT_INVALID_PARM)
      rc = SML_ERR_A_XPT_COMMUNICATION;

   return rc;
}  /* End of protocolCreateSession() */

/**
 *  protocolTerminateSession
 *       - disconnects the active session and releases its associated storage
 *
 *  IN:    pid             A pointer to a protocol handle structure
 *
 **/
Ret_t protocolTerminateSession(WspProtocolHandle_t *pid) {

   Ret_t rc = SML_ERR_OK;

   XPTDEBUG(("protocolTerminateSession(%lx)\n", (unsigned long) pid));

   if (pid == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   /**
    * The syncml spec doesn't indicate that we are supporting methodAbort
    * requests from the client...
    *
    * Wait for a response to the pending transaction(s)
    * If (method(s) pending) && (!methodResult_ind(s) received)
    *    Invoke s_methodAbort_req
    *    Parameters:
    *       Transaction ID    - (from Method Request)
    *
    **/
   rc = sessionDisconnect(pid->session);

   if (rc != SML_ERR_OK) {
      /*
       * Since the disconnect cleans up the session regardless of the rc (i.e.
       * end result if no session), maybe we should just return OK
       */
   }

   pid->session = NULL;

   return rc;
} /* End of protocolTerminateSession() */

/**
 *  protocolSuspendSession
 *       - suspends the active session, if it exists, and indicates it has
 *         been suspended.
 *
 *  IN:    pid             A pointer to a protocol handle structure
 *
 *  RETURN:
 *       An indication of whether or not the suspension was successful.
 *
 **/
Ret_t protocolSuspendSession(WspProtocolHandle_t *pid) {

   Ret_t rc = SML_ERR_OK;

   XPTDEBUG(("protocolSuspendSession(%lx)\n", (unsigned long) pid));

   if (pid == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;
   /**
    * Wait for a response to the pending transaction(s)
    * If (method pending) && (!methodResult_ind received)
    *    Invoke s_methodAbort_req
    *    Parameters:
    *       Transaction ID    - (from Method Request)
    * Else
    *    <do nothing - no further transaction stuff pending>
    **/

   rc = sessionSuspend(pid->session);

   if (rc == SML_ERR_A_XPT_INVALID_PARM)
      rc = SML_ERR_A_XPT_COMMUNICATION;

   return rc;
}  /* End of protocolSuspendSession() */

Ret_t protocolResumeSession(WspProtocolHandle_t *pid)
{
   Ret_t rc = SML_ERR_OK;

   XPTDEBUG(("protocolResumeSession(%lx)\n", (unsigned long) pid));

   if (pid == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (pid->session->sessionSuspended == AWSP_FALSE) {
      /**
       * Return 'session already active'?  Or 'OK', since goal was to
       * have a session and we've got one?
       **/
   } else
      rc = sessionResume(pid->connHandle, pid->session);

   return rc;
}   /* End of protocolResumeSession() */

awsp_BOOL protocolIsHandleValid(WspProtocolHandle_t *handle) {

   XPTDEBUG(("protocolIsHandleValid(%lx)\n", (unsigned long) handle));

   if (handle == NULL)
      return AWSP_FALSE;

   if (handle->valid != WSP_PROTOCOL_HANDLE_VALID_FLAG)
      return AWSP_FALSE;

   return AWSP_TRUE;
}

/**
 *  protocolInitializeHandle
 *       - Allocates storage for protocol handle
 *       - Parses input settings and allocates the protocolSettings in the
 *         protocol handle structure.
 *
 *  IN:    szSettings      A string containing WSP settings parameters
 *         mode            An indication of whether this protocol instance
 *                         is operating as a client or a server.
 *
 *  OUT:   protocolHandle  Contains a handle to the WSP Protocol structure,
 *                         Unaltered if error occurs.
 *
 *  RETURN:
 *       A return code indicating whether the WSP protocol handle was
 *       successfully initialized.
 **/
Ret_t protocolInitializeHandle(const char *szSettings,
                               unsigned int mode,
                               void **protocolHandle) {

   Ret_t rc = SML_ERR_OK;
   WspProtocolHandle_t *pid = NULL;

   XPTDEBUG(("protocolInitializeHandle(%s, %u, %lx)\n", szSettings,
             mode, (unsigned long) protocolHandle));

   if (protocolHandle == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   pid = (void *) xppMalloc(sizeof(WspProtocolHandle_t));

   if (pid == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(pid, 0, sizeof(WspProtocolHandle_t));
   pid->valid = WSP_PROTOCOL_HANDLE_VALID_FLAG;
   pid->mode  = mode;

   rc = settingsInitialize(szSettings, (&(pid->protocolSettings)));

   if (rc == SML_ERR_OK)
      *protocolHandle = (void *) pid;
   else
      xppFree(pid);

   return rc;
} /* End of protocolInitializeHandle() */

/**
 *  protocolReleaseHandle
 *       - releases all storage associate with the protocol - the settings,
 *         the session, the connection, and the protocol structure itself.
 *       - closes the connection if required.
 *
 *  IN     pid             A pointer to a protocol handle structure
 *
 *  OUT    pid             The pointer to the handle has been nullified.
 *
 **/
void protocolReleaseHandle(WspProtocolHandle_t *pid) {

   XPTDEBUG(("protocolReleaseHandle(%lx)\n", (unsigned long) pid));

   if (pid == NULL) return;

   transRelease(pid->transaction);
   settingsRelease(pid->protocolSettings);
   sessionRelease(pid->session);
   protocolReleaseConnection(pid);
   pid->valid=0;              /* In case they try to reuse the freed pid */

   xppFree(pid);

} /* End of protocolReleaseHandle() */

/**
 *  protocolInitializeConnection
 *       - initializes the Service Access Point and obtains a connection
 *         handle to it.
 *       - updates the protocol structure to contain the connection handle.
 *
 *  IN:    pid             A pointer to a protocol handle structure
 *
 *  RETURN:
 *       An indication of whether or not the service access point, and therefore
 *       the physical connection, has been successfully established.
 **/
Ret_t protocolInitializeConnection(WspProtocolHandle_t *pid) {

   Ret_t rc = SML_ERR_OK;
   SapParameters *sapParmsPtr = NULL;

   XPTDEBUG(("protocolInitializeConnection(%lx)\n", (unsigned long) pid));

   if (pid == NULL) return SML_ERR_A_XPT_INVALID_PARM;

   if (pid->connHandle != NULL)
      protocolReleaseConnection(pid);

   if (pid->protocolSettings != NULL)
      sapParmsPtr = pid->protocolSettings->servAccessPtParms;

   /*
    * The following code should set a flag indicating if the SAP
    * was opened for session mode or not.  Questions is: How
    * do we know what the default mode is?
    */
   if (sapParmsPtr != NULL) {

      rc = initializeSAP(&(pid->connHandle),
                         sapParmsPtr->bearerType,
                         sapParmsPtr->addressType,
                         sapParmsPtr->serverAddress,
                         sapParmsPtr->serverPort,
                         sapParmsPtr->clientAddress,
                         sapParmsPtr->clientPort);

   } else
      rc = initializeSAPd(&(pid->connHandle));

   return rc;
} /* End of protocolInitializeConnection() */

/**
 *  protocolReleaseConnection
 *       - closes the Service Access Point and nullifies the connection
 *         handle to it.
 *
 *  IN:    pid             A pointer to a protocol handle structure
 *
 **/
void protocolReleaseConnection(WspProtocolHandle_t *pid) {

   XPTDEBUG(("protocolReleaseConnection(%lx)\n", (unsigned long) pid));

   if ((pid == NULL) || (pid->connHandle == NULL)) return;

   closeSAP(pid->connHandle);
   pid->connHandle = NULL;

} /* End of protocolReleaseConnection() */


/**
 *  protocolInitializeRequest
 *       - Builds the header information for the WSP request and, based on
 *         the method requested, sends the WSP request to the WAP server.
 *
 *  IN:    pid             A pointer to a protocol handle structure
 *         mode            Communication mode (send, receive, exchange, etc.)
 *         pDoc            A pointer to a structure that contains information
 *                         about the data to be transmitted.
 *                         For receive, this structure is also modified with
 *                         response information.
 *
 *  RETURN:
 *       An indication of the success or failure of the operation.
 *
 * Implementation Notes:
 *
 *  In composing the http request, the toolkit user indicates:
 *       method    (XptCommunicationMode)
 *       uri       (XptCommunicationInfoPtr->szName)
 *       mime type (XptCommunicationInfoPtr->szType)
 *
 *  Other information may have been specified in the szSettings on the
 *  selectProtocol, but that would be defined by the protocol.
 *
 *  We are required to support the HTTP Post method.  We can optionally
 *  support GET, PUT, HEAD, DELETE, TRACE and OPTIONS.
 *
 *  The header tags that WSP is required to support are listed in the
 *  SyncML HTTP specification.
 *
 **/
Ret_t protocolInitializeRequest(WspProtocolHandle_t *pid)
{
   Ret_t rc = SML_ERR_OK;

   XPTDEBUG(("protocolInitializeRequest(%lx)\n", (unsigned long) pid));

   /* Compose the transaction request.                                         */
   rc = transInitialize(&(pid->transaction));
   if (rc != SML_ERR_OK)
      return rc;

   if (pid->protocolSettings != NULL) {
      rc = transCreateRequest(pid->transaction,
                              pid->protocolSettings->httpParms,
                              pid->protocolSettings->host,
                             ((pid->session == NULL) ? AWSP_FALSE : AWSP_TRUE));
   } else {
      rc = transCreateRequest(pid->transaction,
                              NULL,
                              NULL,
                             ((pid->session == NULL) ? AWSP_FALSE : AWSP_TRUE));
   }

   if (rc != SML_ERR_OK){
      transRelease(pid->transaction);
      pid->transaction = NULL;
      return rc;
   }

   if (pid->mode == XPT_CLIENT)
      pid->transaction->method = "POST";
   else
      return -1;                          // What to do for server mode????

   /* We should flag that a transaction request is pending for tracking        */

   return rc;
} /* End of protocolInitializeRequest() */

void protocolReleaseRequest(WspProtocolHandle_t *pid)
{
   XPTDEBUG(("protocolReleaseRequest(%lx)\n", (unsigned long) pid));

   transRelease(pid->transaction);
   if (pid->protocolSettings != NULL)
      httpReinitParms(pid->protocolSettings->httpParms);

   pid->transaction = NULL;

   /* We should flag that a transaction request is complete for tracking        */

} /* End of protocolReleaseRequest() */


/**
 *  sendRequest
 *       - Invokes the appropriate abstract WSP methodInvoke according to
 *         the mode (session or connectionless).
 *
 *  IN:    pid             A pointer to a protocol handle structure
 *
 *  RETURN:
 *       An indication of whether the method invocation was successful
 **/
Ret_t sendRequest(WspProtocolHandle_t *pid) {

   Ret_t rc = SML_ERR_OK;

   XPTDEBUG(("  sendRequest(%lx)\n", (unsigned long) pid));

   if (pid == NULL) return SML_ERR_A_XPT_INVALID_PARM;


   /* We wait until now to compose the request-specific http headers so
    * we only do it once, when we're sure all the data has been set to its
    * final state */
   if (pid->protocolSettings != NULL)
      transBuildDynamicRequestHdr(pid->transaction, pid->protocolSettings->httpParms);

   /**
    *  Need to set flag that method is pending in case deselect occurs before response.
    *  Need to keep track of pending transaction id in case abort is required - can more
    *  than one transaction be pending in a single threaded machine?  If so, then need a
    *  table of pending transactions associated with a session, and deselect protcol needs
    *  to cancel them all...
    *  One request is made, needs to wait for response confirmation...
    **/

   if (sessionIsConnected(pid->session) != AWSP_TRUE)
      rc = transSendRequestNoSession(pid->connHandle, pid->transaction);
   else
      rc = transSendRequestOverSession(pid->session->sessionHandle, pid->transaction);

   if (rc != SML_ERR_OK)
      return rc;

   rc = processResponse(pid);

   return rc;
} /* End sendRequest() */


Ret_t processResponse(WspProtocolHandle_t *pid)
{
   Ret_t       rc          = SML_ERR_OK;
   const char *staticHdrs  = NULL;
   char       *mergedHdrs  = NULL;

   XPTDEBUG(("  processResponse(%lx)\n", (unsigned long) pid));

   if (pid == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if ((pid->transaction == NULL) || (pid->protocolSettings == NULL))
      return rc;

   staticHdrs = sessionGetStaticServerHeaders(pid->session);
   if (staticHdrs != NULL) {
      mergedHdrs = (char *) xppMalloc(xppStrlen(staticHdrs) + (pid->transaction->rspHdrSize));
      if (mergedHdrs != NULL) {
         xppStrcpy(mergedHdrs, staticHdrs);
         xppStrcpy(mergedHdrs + xppStrlen(staticHdrs) - 1, pid->transaction->rspHdr);
         rc = httpParseResponseHdr(pid->protocolSettings->httpParms, mergedHdrs);
      }
      xppFree(mergedHdrs);
   } else
      rc = httpParseResponseHdr(pid->protocolSettings->httpParms, pid->transaction->rspHdr);

   return rc;
} /* End processResponse() */


/**
 *  protocolSendRequestData
 *       - invokes abstract WSP method to send push/post data to server.
 *
 *  IN     pid             A pointer to the wsp protocol structure
 *         pbData          The data buffer to be sent.
 *         uDataSize       The size of the data buffer
 *         bLastBlock      An indication of whether or not this is the
 *                         last block of data to be sent as part of this
 *                         transaction.
 *
 *  RETURN:
 *       An indication of whether the data was successfully sent.
 **/
Ret_t protocolSendRequestData (WspProtocolHandle_t *pid,
                               const void *buffer,
                               size_t bufferLen,
                               size_t *bytesSent)
{
   Ret_t rc = SML_ERR_OK;

   XPTDEBUG(("protocolSendRequestData(%lx, %lx, %lu, %lx)\n",
             (unsigned long) pid, (unsigned long) buffer,
             (unsigned long) bufferLen, (unsigned long) bytesSent));

   if ((pid == NULL) || (buffer == NULL) || (bufferLen <= 0))
      return SML_ERR_A_XPT_INVALID_PARM;


   if (pid->transaction == NULL)
      return -1;                 /* invalid request                            */

   /* Verify that transaction state allows sendData - make sure they are not
    * trying to send more data after having sent the last block                */

   transAddRequestData(pid->transaction, buffer, bufferLen);

   /* For now we'll just tell them this - need to figure out if/when we're
    * lying.  Truth is this should be 0 until the last block, at which time
    * it would be the total accumulated.  However, I don't think the app
    * would be prepared to handle that...
    */
   *bytesSent = bufferLen;

   return rc;
} /* End protocolSendRequestData() */


Ret_t protocolSendComplete(WspProtocolHandle_t *pid)
{
   XPTDEBUG(("protocolSendComplete(%lx)\n", (unsigned long) pid));

   if (pid == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   return sendRequest(pid);    /* Have all data, do WSP request              */
} /* End protocolSendComplete() */

Ret_t protocolReadResponseData(WspProtocolHandle_t *pid,
                               void * pbData,
                               size_t uDataSize,
                               size_t *puDataRead)
{
   XPTDEBUG(("protocolReadResponseData(%lx, %lx, %lu, %lx)\n",
             (unsigned long) pid, (unsigned long) pbData,
             (unsigned long) uDataSize, (unsigned long) puDataRead));
   /* Verify that transaction state allows read                                */

   return transReadResponseData(pid->transaction, pbData, uDataSize, puDataRead);
} /* End protocolReadResponseData() */


Ret_t protocolSetRequestInfo(WspProtocolHandle_t *pid,
                 const XptCommunicationInfo_t *pDoc)
{
   XPTDEBUG(("protocolSetRequestInfo(%lx, %lx)\n", (unsigned long) pid, (unsigned long) pDoc));
   return transSetDocInfo(pid->transaction, pDoc);
} /* End of protocolSetRequestInfo() */

Ret_t protocolGetResponseInfo(WspProtocolHandle_t *pid,
                 XptCommunicationInfo_t *pDoc)
{
   XPTDEBUG(("protocolGetResponseInfo(%lx, %lx)\n", (unsigned long) pid, (unsigned long) pDoc));
   return transGetDocInfo(pid->transaction, pDoc);
} /* End of protocolGetResponseInfo() */

