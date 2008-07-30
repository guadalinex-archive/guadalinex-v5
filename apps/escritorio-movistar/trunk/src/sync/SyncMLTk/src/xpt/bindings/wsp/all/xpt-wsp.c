/*************************************************************************/
/* module:          Communication Services, WSP XPT API Functions        */
/* file:            src/xpt/all/xpt-wsp.c                                */
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

#define WSP_EXPORTING
#include <xpt-wsp.h>

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         API functions                                   **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

/**
 * FUNCTION: initializeTransport
 *
 * Register's this binding as a transport
 *
 **/

XPTEXP1 Ret_t XPTAPI XPTEXP2 initializeTransport(void)
{
   Ret_t rc = SML_ERR_OK;
   struct xptTransportDescription desc;

   desc.shortName            = "WSP";
   desc.description          = "WAP's Wireless Session Protocol";
   desc.flags                = XPT_CLIENT;
   desc.privateTransportInfo = NULL;
   desc.selectProtocol       = selectProtocol;
   desc.deselectProtocol     = deselectProtocol;
   desc.openCommunication    = openCommunication;
   desc.closeCommunication   = closeCommunication;
   desc.beginExchange        = beginExchange;
   desc.endExchange          = endExchange;
   desc.receiveData          = receiveData;
   desc.sendData             = sendData;
   desc.sendComplete         = sendComplete;
   desc.setDocumentInfo      = setDocumentInfo;
   desc.getDocumentInfo      = getDocumentInfo;

   rc = xptRegisterTransport(&desc);

   return rc;
} /* End initializeTransport() */

/**
 * FUNCTION: selectProtocol
 *
 * Open the physical connection to the server.
 *
 * szSettings parameters for WSP protocol:
 *
 * SyncML parameters (required):
 *    syncServer       = <ip or dns> of sync server to which syncML request is
 *                                   to be sent.
 *
 * Session parameters (Optional):
 *    createSession    = {0,1}   0 = connectionless, 1 = session, default = 1
 *
 * HTTP parameters (Optional)
 *    accept           = acceptable mime types,
 *                       default = "application/vnd.syncml-xml, application/vnd.syncml-wbxml"
 *    acceptCharset    = acceptable character sets,
 *                       default = "UTF8"
 *    acceptEncoding   = acceptable encodings
 *    acceptLanguage   = acceptable language
 *    emailAddress     = email address of user
 *    authUserid       = userid for basic/digest authentication
 *    authPassword     = password for basic authentication
 *    authRealm        = realm for basic/digest authentication
 *
 *
 * WAP Stack SAP Initialization Parameters (Optional):
 *    The presence of any one of the wap settings causes the use of any of the
 *    waps specified, and causes absent wap settings to default.  If no
 *    wap settings are present, the default WAP Stack SAP is initialized.
 *
 *    wapBearerType    = enum AWSP_BEARER_TYPE
 *    wapAddressType   = enum AWSP_ADDRESS_TYPE
 *    wapServerAddr    = <MSISDN>
 *    wapServerPort    = short
 *    wapClientAddr    = <MSISDN>
 *    wapClientPort    = short
 *
 * Capabilities Negotiation (Optional):
 *    The presence of any one of the capability settings causes the use of any of the
 *    capabilities specified, and causes absent capabilities settings to default.  If no
 *    capabilities settings are present, capabilities negotiation is bypassed.
 *    Note: Capabilities Negotiation occurs during openCommunication(), but the
 *          parameters must be presented in the szSettings string of selectProtocol().
 *
 *    capAlias                = <MSISDN>   (repeat setting for each alias)
 *    capClientSDUSize        = int
 *    capExtendedMethod       = string     (repeat setting for each alias)
 *    capHeaderCodePages      = string     (repeat setting for each code page)
 *    capMaxOutstandReq       = int
 *    capMaxOutstPush         = int
 *    capProtocolOption       = string     (repeat setting for each protocol option)
 *    capServerSDUSize        = int
 **/
Ret_t XPTAPI selectProtocol (void *privateTransportInfo,
                             const char *szSettings,
                             unsigned int flags,
                             void **pid)
{
   Ret_t rc = SML_ERR_OK;

   /* Need to reject server mode */

   /* Create the WSP protocol structure, initialize settings object(s).        */
   rc = protocolInitializeHandle(szSettings, flags, pid);

   return rc;
}  /* End of selectProtocol() method */


/**
 * FUNCTION: deselectProtocol
 *
 * Complete pending request and close physical connection to server.
 *
 */
Ret_t XPTAPI deselectProtocol (void *pid)
{
   if (!protocolIsHandleValid((WspProtocolHandle_t *) pid))
       return SML_ERR_A_XPT_INVALID_ID;
   /**
    * If the palm is a single-threaded environment and we last lost control
    * waiting for a synchronous response to any wsp request, then how in the
    * world does deselectProtocol() ever get invoked, if not as a callback
    * from the WSP stack aborting the pending request???????
    **/

   /* Disconnect the session and release its storage.                          */
   protocolTerminateSession((WspProtocolHandle_t *) pid);

   /* Close the Application Service Access Point and release its storage       */
   protocolReleaseConnection((WspProtocolHandle_t *) pid);

   /* Release protocol handle storage and any remaining objects                */
   protocolReleaseHandle((WspProtocolHandle_t *) pid);

   return SML_ERR_OK;
} /* End deselectProtocol() */


/**
 * FUNCTION: openCommunication
 *
 * Send the SyncML document to the server.
 *
 * For HTTP GET operations, this has to return information that exists in the
 * server response header, so for GET this needs to do the methodInvoke and
 * wait for methodResponse.  However, the actual response body will have to
 * be cached for retrieval with the receiveData() method.
 *
 * The HTTP PUT/POST operations, only the http header is built and transmitted
 * in this method.  The user must invoke sendData() to transmit the body.
 *
 * WSP doesn't seem to be able to break these things apart.  However, given the
 * input parameters to this method, it would appear that at the very least the
 * HTTP header needs to be built here, even if we wait until sendData() to do
 * the methodInvoke request. Sooooo, the WSP binding either builds the http
 * header here, or sets up the objects so that send/receive can build it.
 *
 * One interesting note is that HTTP binding send/receive restrict what can be
 * done based on settings that are established at the time of the open (i.e.
 * if the header indicates GET the sendData() is disallowed).  Sooooo, building
 * the header here seems appropriate in this context, and then the send/receive
 * would have to actually invoke the appropriate WSP primitive to accomplish the
 * HTTP request.
 **/
Ret_t XPTAPI openCommunication (void *pid,
                                int role,
                                void **pPrivateConnectionInfo)
{
   Ret_t rc = SML_ERR_OK;

   if (!protocolIsHandleValid((WspProtocolHandle_t *) pid))
       return SML_ERR_A_XPT_INVALID_ID;

   /* Prepare Application Service Access Point for communication, if needed    */
   if (((WspProtocolHandle_t *)pid)->connHandle == NULL) {
      rc = protocolInitializeConnection(pid);
      if (rc != SML_ERR_OK) {
//         setError(pid, "openCommunication", rc, "protocolInitializeConnection", rc);
         return rc;
      }
   } /* End no connection handle */

   /* Establish session (create if it doesn't exist, resume if it's been suspended) */
   if ((((WspProtocolHandle_t *)pid)->session != NULL) &&
       (((WspProtocolHandle_t *)pid)->session->sessionSuspended == AWSP_TRUE)) {
      protocolResumeSession((WspProtocolHandle_t *)pid);
   } else {
      /* What if session create fails?  It could fail because the SAP is
       * sessionless...  Also, if user requested session and we cannot
       * establish the session, do we fail the open?                     */
      protocolCreateSession((WspProtocolHandle_t *)pid);
   }

   rc = protocolInitializeRequest((WspProtocolHandle_t *)pid);

   /* WSP only uses one structure... */
   if (pPrivateConnectionInfo != NULL)
      *pPrivateConnectionInfo = (void *) pid;

   return rc;
}

/**
 * FUNCTION: closeCommunication
 *
 * Complete the outstanding SyncML request
 *
 */

Ret_t XPTAPI closeCommunication (void *pid)
{

   if (!protocolIsHandleValid((WspProtocolHandle_t *) pid))
       return SML_ERR_A_XPT_INVALID_ID;

   /* Need to clean up anything pending - e.g. a put/post was requested on the open
    * but no sendData() was called to complete it.                             */

   protocolReleaseRequest(pid);

   /* Suspend the session until the next openCommunication()                   */
   /* Note:  If the suspension fails, the next open will handle it             */
   protocolSuspendSession(pid);

   return SML_ERR_OK;
} /* End of closeCommunication() */


Ret_t XPTAPI beginExchange(void *pid) {return SML_ERR_OK;}

Ret_t XPTAPI endExchange(void *pid) {return SML_ERR_OK;}


/**
 * FUNCTION: receiveData
 *
 * Read data from connection into buffer.
 *
 */
Ret_t XPTAPI receiveData (void *pid,
                          void * pbData,
                          size_t uDataSize,
                          size_t *puDataRead)
{
   Ret_t rc = SML_ERR_OK;

   if (!protocolIsHandleValid((WspProtocolHandle_t *) pid))
       return SML_ERR_A_XPT_INVALID_ID;

   /* Read whatever response data exists...                                    */
   rc = protocolReadResponseData((WspProtocolHandle_t *) pid, pbData, uDataSize, puDataRead);

   return rc;
} /* End of receiveData() */


/**
 * FUNCTION: sendData
 *
 * Write data to the connection
 *
 */
Ret_t XPTAPI sendData (void *pid,
                       const void *buffer,
                       size_t bufferLen,
                       size_t *bytesSent)
{
   Ret_t rc = SML_ERR_OK;

   if (!protocolIsHandleValid((WspProtocolHandle_t *) pid))
       return SML_ERR_A_XPT_INVALID_ID;

   rc = protocolSendRequestData((WspProtocolHandle_t *)pid, buffer, bufferLen, bytesSent);

   return rc;
} /* End of sendData() */

Ret_t XPTAPI sendComplete(void *pid)
{
   Ret_t rc = SML_ERR_OK;

   if (!protocolIsHandleValid((WspProtocolHandle_t *) pid))
       return SML_ERR_A_XPT_INVALID_ID;

   rc = protocolSendComplete((WspProtocolHandle_t *)pid);

   return rc;
} /* End sendComplete() */


Ret_t XPTAPI setDocumentInfo(void *pid, const XptCommunicationInfo_t *pDoc)
{
   Ret_t rc = SML_ERR_OK;

   if (!protocolIsHandleValid((WspProtocolHandle_t *) pid))
       return SML_ERR_A_XPT_INVALID_ID;

   rc = protocolSetRequestInfo((WspProtocolHandle_t *)pid, pDoc);

   return rc;
} /* End of setDocumentInfo() */

Ret_t XPTAPI getDocumentInfo(void *pid, XptCommunicationInfo_t *pDoc)
{
   Ret_t rc = SML_ERR_OK;

   if (!protocolIsHandleValid((WspProtocolHandle_t *) pid))
       return SML_ERR_A_XPT_INVALID_ID;

   rc = protocolGetResponseInfo((WspProtocolHandle_t *)pid, pDoc);

   return rc;
} /* End of getDocumentInfo() */

