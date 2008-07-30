/*************************************************************************/
/* module:          Communication Services, Abstract WSP implementation  */
/* file:            src/xpt/awsp/awsp.c                                  */
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

#define WSP_EXPORTING
#include <awsp.h>

#include <xptTransport.h>  // For the XPTDEBUG macro
#include <xptport.h>

#define RESPONSE_BODY "This is a response to a syncml document"
//#define RESPONSE_HDR  "Content-Length: 39\nContent-Type: text/plain\n\n"
#define RESPONSE_HDR  "Content-Length: 39\nContent-Type: text/plain\nCache-Control: private\nLocation: http:\\\\www.ibm.com\\\nRetry-After: 30\nWWW-Authenticate: Basic realm=\"MyRealm\"\n\n"
//#define RESPONSE_HDR  "Content-Length: 39\nContent-Type: text/plain\nCache-Control: private\nLocation: http:\\\\www.ibm.com\\\nRetry-After: 30\nWWW-Authenticate: Digest  realm=\"MyRealm\",domain=\"www.ibm.com\",nonce=\"12A7536F48\", opaque=\"9A1B8C2D7E3F6\",stale=\"true\",  algorithm=\"MD5\",qop=\"auth\"\n\n"
//#define RESPONSE_HDR  "Content-Length: 39\nContent-Type: text/plain\nCache-Control: private\nLocation: http:\\\\www.ibm.com\\\nRetry-After: 30\nAuthentication-Info: nextnonce=\"12A7536F48\", rspauth=\"123456789\",cnonce=\"9A1B8C2D7E3F6\",nc=00000001\n\n"
#define RESPONSE_STATUS AWSP_SUCCESS
//#define RESPONSE_STATUS AWSP_INTERNAL_SERVER_ERROR


/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                     Prototypes of WSP functions                        **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_connect_req(awsp_ConnectionHandle connHandle,
                           awsp_SessionHandle   *sessionHandle,
                           const char           *clientHttpHeaders,
                           const size_t          clientHttpHeadersLength,
                           awsp_Capabilities    *requestedCapabilities)
{
   const char *eye = "SESSIONHANDLE";
   char *handle = NULL;

   XPTDEBUG(("awsp_connect_req(%lx, %lx, %s, %lu, %lx)\n",
          (unsigned long) connHandle, (unsigned long) sessionHandle,
          clientHttpHeaders, (unsigned long) clientHttpHeadersLength,
          (unsigned long) requestedCapabilities));

   handle = (char *) xppMalloc(xppStrlen(eye) + 1);
   xppStrcpy(handle, eye);

   *sessionHandle = (void *) handle;

   return AWSP_RC_OK;
} /* End awsp_connect_req() */

XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_get_connect_cnf(awsp_SessionHandle    sessionHandle,
                               const char           *serverHttpHeaders,
                               size_t               *serverHttpHeadersLength,
                               awsp_Capabilities   **negotiatedCapabilities)
{
   XPTDEBUG(("awsp_get_connect_cnf(%lx, %s, %lu, %lx)\n",
          (unsigned long) sessionHandle, serverHttpHeaders,
          (unsigned long) serverHttpHeadersLength,
          (unsigned long) negotiatedCapabilities));

   return AWSP_RC_OK;
} /* End awsp_get_connect_cnf() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_disconnect_req(awsp_SessionHandle sessionHandle,
                              int                reasonCode,
                              awsp_BOOL          redirectSecurity,
                              const char        *redirectAddresses[],
                              const char        *errorHeaders,
                              size_t             errorHeadersLength,
                              void              *errorBody,
                              size_t             errorBodyLength)
{
   XPTDEBUG(("awsp_disconnect_req(%lx, %d, %i, %lx, %s, %lu, %lx, %lu)\n",
          (unsigned long) sessionHandle, reasonCode, (int) redirectSecurity,
          (unsigned long) redirectAddresses, errorHeaders,
          (unsigned long) errorHeadersLength, (unsigned long) errorBody,
          (unsigned long) errorBodyLength));

   xppFree(sessionHandle);
   return AWSP_RC_OK;
} /* End awsp_disconnect_req() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_methodInvoke_req(awsp_SessionHandle sessionHandle,
                           unsigned long      clientTransactionID,
                           const char        *method,
                           const char        *requestURI,
                           const char        *requestHeaders,
                           size_t             requestHeadersLength,
                           void              *requestBody,
                           size_t             requestBodyLength)
{

   XPTDEBUG(("awsp_methodInvoke_req(%lx, %lu, %s, %s, %s, %lu, %lx, %lu)\n",
          (unsigned long) sessionHandle, clientTransactionID, method,
          requestURI, requestHeaders, (unsigned long) requestHeadersLength,
          (unsigned long) requestBody, (unsigned long) requestBodyLength));


   return AWSP_RC_OK;
} /* End awsp_methodInvoke_req() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_get_methodResult_ind(awsp_SessionHandle  sessionHandle,
                                    unsigned long       clientTransactionID,
                                    awsp_StatusCode_t  *status,
                                    char               *responseHeaders,
                                    size_t             *responseHeadersLength,
                                    void               *responseBody,
                                    size_t             *responseBodyLength)
{
   const char *body = RESPONSE_BODY;
   const char *hdrs = RESPONSE_HDR;
   awsp_Rc_t   rc   = AWSP_RC_OK;

   XPTDEBUG(("awsp_get_methodResult_ind(%lx, %lu, %lx, %s, %lu, %lx, %lu)\n",
          (unsigned long) sessionHandle, clientTransactionID, (unsigned long) status,
          responseHeaders, (unsigned long) responseHeadersLength,
          (unsigned long) responseBody, (unsigned long) responseBodyLength));

   if ((responseHeadersLength == NULL) || (responseBodyLength == NULL))
     return -1;

   if ((*responseHeadersLength < (xppStrlen(hdrs) + 1)) || (*responseBodyLength < xppStrlen(body)))
     rc = AWSP_RC_BUFFER_TOO_SMALL;

   *responseHeadersLength = xppStrlen(hdrs) + 1;
   *responseBodyLength    = xppStrlen(body);

   if (rc != AWSP_RC_OK)
     return rc;

   if ((responseHeaders == NULL) || (responseBody == NULL))
     return AWSP_RC_BUFFER_TOO_SMALL;

   *status                = RESPONSE_STATUS;
   xppStrcpy(responseHeaders, hdrs);
   xppMemcpy(responseBody, body, xppStrlen(body));

   return AWSP_RC_OK;
} /* End awsp_get_methodResult_ind() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_methodResult_rsp(awsp_SessionHandle sessionHandle,
                           unsigned long      clientTransactionID,
                           void              *acknowledgementHeaders,
                           size_t             acknowledgementHeadersLength)
{
   XPTDEBUG(("awsp_methodResult_rsp(%lx, %lu, %lx, %lu)\n",
          (unsigned long) sessionHandle, clientTransactionID,
          (unsigned long) acknowledgementHeaders,
          (unsigned long) acknowledgementHeadersLength));
   return AWSP_RC_OK;
} /* End awsp_methodResult_rsp() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_methodAbort_ind(awsp_SessionHandle sessionHandle,
                          unsigned long      transactionID,
                          awsp_ReasonCode_t  reasonCode)
{
   XPTDEBUG(("awsp_methodAbort_ind(%lx, %lu, %i)\n",
          (unsigned long) sessionHandle, transactionID, (int) reasonCode));
   return AWSP_RC_OK;
} /* End awsp_methodAbort_ind() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_confirmedPush_rsp(awsp_SessionHandle sessionHandle,
                            unsigned long      clientPushID,
                            void              *acknowledgementHeaders,
                            size_t             acknowledgementHeadersLength)
{
   XPTDEBUG(("awsp_confirmedPush_rsp(%lx, %lu, %lx, %lu)\n",
          (unsigned long) sessionHandle, clientPushID,
          (unsigned long) acknowledgementHeaders,
          (unsigned long) acknowledgementHeadersLength));
   return AWSP_RC_OK;
} /* End awsp_confirmedPush_rsp() */

XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_suspend_req(awsp_SessionHandle sessionHandle)
{
   XPTDEBUG(("awsp_suspend_req(%lx)\n", (unsigned long) sessionHandle));

   return AWSP_RC_OK;
} /* End awsp_suspend_req() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_resume_req(awsp_ConnectionHandle connHandle,
                          const char           *clientHeaders,
                          size_t                clientHeadersLength)
{
   XPTDEBUG(("awsp_resume_req(%lx, %s, %lu)\n",
          (unsigned long) connHandle, clientHeaders,
          (unsigned long) clientHeadersLength));

   return AWSP_RC_OK;
} /* End awsp_resume_req() */

XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_get_resume_cnf(awsp_ConnectionHandle connHandle,
                              awsp_SessionHandle    sessionHandle,
                              char                 *serverHeaders,
                              size_t               *serverHeadersLength)
{
   XPTDEBUG(("awsp_get_resume_cnf(%lx, %lx, %s, %lu)\n",
          (unsigned long) connHandle, (unsigned long) sessionHandle,
          serverHeaders, (unsigned long) serverHeadersLength));

   return AWSP_RC_OK;
} /* End awsp_get_resume_cnf() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_unit_methodInvoke_req(awsp_ConnectionHandle connHandle,
                                unsigned long         transactionID,
                                const char           *method,
                                const char           *requestURI,
                                const char           *requestHeaders,
                                size_t                requestHeadersLength,
                                void                 *requestBody,
                                size_t                requestBodyLength)
{
   XPTDEBUG(("awsp_unit_methodInvoke_req(%lx, %u, %s, %s, %s, %u, %lx, %u)\n",
          (unsigned long) connHandle, transactionID, method, requestURI,
          requestHeaders, (unsigned long) requestHeadersLength,
          (unsigned long) requestBody, (unsigned long) requestBodyLength));
   return AWSP_RC_OK;
} /* End awsp_unit_methodInvoke_req() */


XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_get_unit_methodResult_ind(awsp_ConnectionHandle connHandle,
                                         unsigned long         transactionID,
                                         awsp_StatusCode_t    *status,
                                         char                 *responseHeaders,
                                         size_t               *responseHeadersLength,
                                         void                 *responseBody,
                                         size_t               *responseBodyLength)
{
   const char *body = RESPONSE_BODY;
   const char *hdrs = RESPONSE_HDR;
   awsp_Rc_t   rc   = AWSP_RC_OK;

   XPTDEBUG(("awsp_get_unit_methodResult_ind(%lx, %lu, %lx, %s, %lu, %lx, %lu)\n",
          (unsigned long) connHandle, transactionID, (unsigned long) status,
          responseHeaders, (unsigned long) responseHeadersLength,
          (unsigned long) responseBody, (unsigned long) responseBodyLength));

   if ((responseHeadersLength == NULL) || (responseBodyLength == NULL))
     return -1;

   if ((*responseHeadersLength < (xppStrlen(hdrs) + 1)) || (*responseBodyLength < xppStrlen(body)))
     rc = AWSP_RC_BUFFER_TOO_SMALL;

   *responseHeadersLength = xppStrlen(hdrs) + 1;
   *responseBodyLength    = xppStrlen(body);

   if (rc != AWSP_RC_OK)
     return rc;

   if ((responseHeaders == NULL) || (responseBody == NULL))
      return AWSP_RC_BUFFER_TOO_SMALL;

   *status                = RESPONSE_STATUS;
   xppStrcpy(responseHeaders, hdrs);
   xppMemcpy(responseBody, body, xppStrlen(body));

   return AWSP_RC_OK;
} /* End awsp_get_unit_methodResult_ind() */

XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 initializeSAPd(awsp_ConnectionHandle *connectionHandle)
{
   const char *eye = "CONNHANDLE";
   char *handle = NULL;

   XPTDEBUG(("initializeSAPd(%lx)\n", (unsigned long) connectionHandle));

   handle = (char *) xppMalloc(xppStrlen(eye) + 1);
   xppStrcpy(handle, eye);

   *connectionHandle = (void *) handle;

   return AWSP_RC_OK;
} /* End initializeSAPd() */

XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 initializeSAP(awsp_ConnectionHandle *connectionHandle,
                        awsp_BEARER_TYPE       bearerType,
                        awsp_ADDRESS_TYPE      addressType,
                        const char            *serverAddress,
                        unsigned short         serverPort,
                        const char            *clientAddress,
                        unsigned short         clientPort)
{
   const char *eye = "CONNHANDLE";
   char *handle = NULL;

   XPTDEBUG(("initializeSAP(%lx, %i, %i, %s, %hu, %s, %hu)\n",
          (unsigned long) connectionHandle, (int) bearerType, (int) addressType,
          serverAddress, serverPort, clientAddress, clientPort));

   handle = (char *) xppMalloc(xppStrlen(eye) + 1);
   xppStrcpy(handle, eye);

   *connectionHandle = (void *) handle;

   return AWSP_RC_OK;
} /* End initializeSAP() */

XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 closeSAP(awsp_ConnectionHandle connectionHandle)
{

   XPTDEBUG(("closeSAP(%lx)\n", (unsigned long) connectionHandle));

   xppFree(connectionHandle);
   return AWSP_RC_OK;
} /* End closeSAP() */
