/*************************************************************************/
/* module:          Communication Services, WSP HTTP Header Functions    */
/* file:            src/xpt/all/wsphttp.c                                */
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


#include <wsphttp.h>
#include <wsputil.h>
#include <xpt.h>        /* For SML_ERRs */
#include <xptport.h>

/**
 * Eyecatcher to validate WspProtocolHandle_t structure
 **/
#define WSP_HTTP_HANDLE_VALID_FLAG 0x48505357L    // 'WSPH'

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         Private Implementation Methods                  **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/


/**
 * HTTP Settings parameters:
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
 **/


unsigned int httpInitialize(const char *szSettings, WspHttpParms_t **parmPtr)
{
   unsigned int rc = SML_ERR_OK;
   WspHttpParms_t *httpParms = NULL;

   XPTDEBUG(("    httpInitialize(%s, %lx)\n", szSettings, (unsigned long) parmPtr));

   if (parmPtr == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (*parmPtr != NULL) {
      httpRelease(*parmPtr);
      *parmPtr = NULL;
   }

   httpParms = (WspHttpParms_t *) xppMalloc(sizeof(WspHttpParms_t));
   if (httpParms == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(httpParms, 0, sizeof(WspHttpParms_t));
   httpParms->valid = WSP_HTTP_HANDLE_VALID_FLAG;

   rc = initializeRequestParms(httpParms, szSettings);

   if (rc != SML_ERR_OK) {
      httpRelease(httpParms);
      return rc;
   }

   *parmPtr = httpParms;

   return rc;

} /* End httpInitialize */

void httpRelease(WspHttpParms_t *parmPtr)
{
   XPTDEBUG(("    httpRelease(%lx)\n", (unsigned long) parmPtr));

   if ((parmPtr == NULL) || (parmPtr->valid != WSP_HTTP_HANDLE_VALID_FLAG))
      return;

   releaseRequestParms(parmPtr->request);
   releaseResponseParms(parmPtr->response);
   parmPtr->request  = NULL;
   parmPtr->response = NULL;
   parmPtr->valid = 0;

   xppFree(parmPtr);

} /* End httpRelease */

/*
 * Unfortunately there are 2 kinds of data maintained in this object -
 * 'static' data that is specified at the time the protocol is selected
 * (i.e. settings metadata) and dynamic data that gathers as transactions
 * are being processed.  When a transaction completes, the dynamic
 * data must be removed/reset.  That is the purpose of this method.
 */

void httpReinitParms(WspHttpParms_t *parmPtr) {

   XPTDEBUG(("    httpReinitParms(%lx)\n", (unsigned long) parmPtr));

   if ((parmPtr == NULL) || (parmPtr->valid != WSP_HTTP_HANDLE_VALID_FLAG))
      return;

   if (parmPtr->request != NULL) {
      xppFree(parmPtr->request->composedHeader);
      parmPtr->request->composedHeader = NULL;

      if (parmPtr->request->authorization != NULL) {
         xppFree(parmPtr->request->authorization->realm);
         parmPtr->request->authorization->realm = NULL;

         xppFree(parmPtr->request->authorization->cnonce);
         parmPtr->request->authorization->cnonce = NULL;

         xppFree(parmPtr->request->authorization->opaque);
         parmPtr->request->authorization->opaque = NULL;
      } /* End have authorization parms */
   } /* End have request parms */

   releaseResponseParms(parmPtr->response);
   parmPtr->response = NULL;

   xppFree(parmPtr);

} /* End httpReinitParms() */


/*
 * Note: This shares a buffer with httpGetRequestHdrs, so the returned value
 *       can no longer be used once httpGetRequestHdrs is invoked.
 */
const char *httpGetStaticRequestHdrs(WspHttpParms_t *parmPtr)
{
   unsigned int hdrSize = 0;
   XPTDEBUG(("    httpGetStaticRequestHdrs(%lx)\n", (unsigned long) parmPtr));

   if ((parmPtr == NULL) || (parmPtr->request == NULL))
      return NULL;

   hdrSize = calcStaticHeaderSize(parmPtr->request);

   if (parmPtr->request->composedHeader != NULL)
      parmPtr->request->composedHeader = (char *) xppRealloc(parmPtr->request->composedHeader, hdrSize);
   else
      parmPtr->request->composedHeader = (char *) xppMalloc(hdrSize);

   if (parmPtr->request->composedHeader == NULL)
      return NULL;

   xppMemset(parmPtr->request->composedHeader, 0, hdrSize);

   addTag(parmPtr->request->composedHeader, "Cache-Control", D_CACHE_CONTROL);
   addTag(parmPtr->request->composedHeader, "Accept", parmPtr->request->accept);
   addTag(parmPtr->request->composedHeader, "Accept-Charset", parmPtr->request->acceptCharset);
   addTag(parmPtr->request->composedHeader, "From", parmPtr->request->from);
   addTag(parmPtr->request->composedHeader, "User-Agent", D_USER_AGENT);
   addTag(parmPtr->request->composedHeader, "Accept-Encoding", parmPtr->request->acceptEncoding);
   addTag(parmPtr->request->composedHeader, "Accept-Language", parmPtr->request->acceptLanguage);
   addTag(parmPtr->request->composedHeader, "host", parmPtr->request->host);
   xppStrcat(parmPtr->request->composedHeader, NL);

   XPTDEBUG(("    httpGetStaticRequestHdrs() response = %s\n", parmPtr->request->composedHeader));

   return parmPtr->request->composedHeader;
} /* End httpGetStaticRequestHdrs */

/*
 * Note: This shares a buffer with httpGetStaticRequestHdrs, so the returned value
 *       can no longer be used once httpGetStaticRequestHdrs is invoked.
 */
const char *httpGetRequestHdrs(WspHttpParms_t *parmPtr, const char *contentType,
                                  const char *contentLength)
{
   int headerSize = 0;
   char *date     = getRFC822Date();

   XPTDEBUG(("    httpGetRequestHdrs(%lx, %s, %s)\n", (unsigned long) parmPtr,
             contentType, contentLength));

   if ((parmPtr == NULL) || (parmPtr->request == NULL))
      return NULL;

   headerSize += getTagLen("Content-Type", contentType);
   headerSize += getTagLen("Content-Length", contentLength);
   headerSize += getTagLen("Date", date);
   headerSize += getTagLen("Referer: ", parmPtr->request->referer);
   headerSize += xppStrlen(NL);
   headerSize += 1;              /* For null terminator */

   if (parmPtr->request->composedHeader != NULL)
      parmPtr->request->composedHeader = (char *) xppRealloc(parmPtr->request->composedHeader, headerSize);
   else
      parmPtr->request->composedHeader = (char *) xppMalloc(headerSize);

   if (parmPtr->request->composedHeader == NULL)
      return NULL;

   xppMemset(parmPtr->request->composedHeader, 0, headerSize);

   addTag(parmPtr->request->composedHeader, "Content-Type", contentType);
   addTag(parmPtr->request->composedHeader, "Content-Length", contentLength);
   addTag(parmPtr->request->composedHeader, "Date", date);
   addTag(parmPtr->request->composedHeader, "Referer", parmPtr->request->referer);
   xppStrcat(parmPtr->request->composedHeader, NL);

   XPTDEBUG(("    httpGetRequestHdrs() response = %s\n", parmPtr->request->composedHeader));

   return parmPtr->request->composedHeader;
} /* End httpGetRequestHdrs */

char *getRFC822Date(void)
{
   return "Fri, 14 Jul 2000 00:00:00 GMT";
} /* End getRFC822Date() */

/*
 * Note: If the update fails, the previous authentication information is lost.
 */
unsigned int httpUpdateAuthorization(WspHttpParms_t *parmPtr, const char *userid,
                                     const char *password, const char *realm)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    httpUpdateAuthorization(%lx, %s, %s, %s)\n",
              (unsigned long) parmPtr, userid, password, realm));

   if (parmPtr == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if ((parmPtr->request == NULL) || (parmPtr->request->authorization == NULL))
      return SML_ERR_A_XPT_COMMUNICATION;

   rc = updateStringField(&(parmPtr->request->authorization->userid), userid);
   if (rc == SML_ERR_OK) {

      rc = updateStringField(&(parmPtr->request->authorization->password), password);
      if (rc == SML_ERR_OK) {

         rc = updateStringField(&(parmPtr->request->authorization->realm), realm);

      } /* End successful password update */
   } /* End successful userid update */

   if (rc != SML_ERR_OK) {
      xppFree(parmPtr->request->authorization->userid);
      parmPtr->request->authorization->userid = NULL;
      xppFree(parmPtr->request->authorization->password);
      parmPtr->request->authorization->password = NULL;
      xppFree(parmPtr->request->authorization->realm);
      parmPtr->request->authorization->realm = NULL;
   } /* End update(s) failed) */

   return rc;
} /* End httpSetAuthenticationInfo */

/*
 * Note: If the set fails, the previous referer information is lost.
 */
unsigned int httpSetReferer(WspHttpParms_t *parmPtr, const char *referer)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    httpSetRefered(%lx, %s)\n", (unsigned long) parmPtr, referer));

   if (parmPtr == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (parmPtr->request == NULL)
      return SML_ERR_OK;

   rc = updateStringField(&(parmPtr->request->referer), referer);

   return rc;
} /* End httpSetReferer */

unsigned int httpParseResponseHdr(WspHttpParms_t *parmPtr, const char *rspHeader)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    httpParseResponseHdr(%lx, %s)\n", (unsigned long) parmPtr, rspHeader));

   rc = initializeResponseParms(parmPtr, rspHeader);

   if (rc != SML_ERR_OK)
      return rc;

   rc = validateResponse(parmPtr);

   return rc;
} /* End httpParseResponseHdr */

unsigned int initializeRequestParms(WspHttpParms_t *parmPtr, const char *settings)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    initializeRequestParms(%lx, %s)\n", (unsigned long) parmPtr, settings));

   if (parmPtr == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (parmPtr->request != NULL)
      releaseRequestParms(parmPtr->request);

   parmPtr->request = (WspHttpRequestParms_t *) xppMalloc(sizeof(WspHttpRequestParms_t));

   if (parmPtr->request == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(parmPtr->request, 0, sizeof(WspHttpRequestParms_t));
   rc = parseRequestParms(parmPtr->request, settings);

   if (rc != SML_ERR_OK) {
      releaseRequestParms(parmPtr->request);
      parmPtr->request = NULL;
   }

   return rc;
} /* End initializeRequestParms() */

unsigned int parseRequestParms(WspHttpRequestParms_t *request, const char *settings)
{

   unsigned int   rc             = SML_ERR_OK;

   XPTDEBUG(("    parseRequestParms(%lx, %s)\n", (unsigned long) request, settings));

   if (request == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (settings == NULL)
      return rc;

   request->accept = getMetaInfoValue(settings, WSP_HTTP_ACCEPT);
   request->acceptCharset = getMetaInfoValue(settings, WSP_HTTP_ACCEPT_CHARSET);
   request->acceptEncoding = getMetaInfoValue(settings, WSP_HTTP_ACCEPT_ENCODING);
   request->acceptLanguage = getMetaInfoValue(settings, WSP_HTTP_ACCEPT_LANGUAGE);
   request->from = getMetaInfoValue(settings, WSP_HTTP_FROM);

   rc = parseAuthorizationParms(request, settings);

   if (rc != SML_ERR_OK)
      return rc;

   /* If client didn't specify certain header tags, we need to default them.
    * Note that we use dynamic storage so we don't have to constantly
    * distinguish between default and specified values when managing the
    * object
    */
   if (request->accept == NULL) {
      request->accept = (char *) xppMalloc(xppStrlen(D_ACCEPT) + 1);
      if (request->accept != NULL)
         xppStrcpy(request->accept,D_ACCEPT);
   }

   if (request->acceptCharset == NULL) {
      request->acceptCharset = (char *) xppMalloc(xppStrlen(D_ACCEPT_CHARSET) + 1);
      if (request->acceptCharset != NULL)
         xppStrcpy(request->acceptCharset, D_ACCEPT_CHARSET);
   }

   return rc;
} /* End parseRequestParms() */

unsigned int parseAuthorizationParms(WspHttpRequestParms_t *request, const char *settings)
{
   unsigned int   rc             = SML_ERR_OK;
   unsigned int   validAuthParms = 0;
   char    *tempValue      = NULL;

   XPTDEBUG(("    parseAuthorizationParms(%lx, %s)\n", (unsigned long) request, settings));

   if ((request == NULL) || (settings == NULL))
      return rc;

   /* We can't do any auth without a userid, so if no userid is
    * present then ignore other auth parms.  We also need the realm.
    * But since the password isn't required for Digest, we don't
    * care if it's present or not at this time
    */
   tempValue = getMetaInfoValue(settings, WSP_HTTP_AUTH_USERID);

   if (tempValue == NULL)
      return rc;

   rc = initializeAuthorizationParms(&(request->authorization));

   if (rc != SML_ERR_OK) {
      xppFree(tempValue);
      return rc;
   }

   request->authorization->userid = tempValue;
   if (request->authorization->userid != NULL) {
      validAuthParms = 1;
      request->authorization->realm = getMetaInfoValue(settings, WSP_HTTP_AUTH_REALM);
      if (request->authorization->realm != NULL)
         request->authorization->password = getMetaInfoValue(settings, WSP_HTTP_AUTH_PASSWORD);
      else
         validAuthParms = 0;
   } /* End valid userid */

   if (!validAuthParms) {
      releaseAuthorizationParms(request->authorization);
      rc = SML_ERR_A_XPT_INVALID_PARM;
   }

   return rc;
} /* End parseAuthorizationParms() */

unsigned int initializeAuthorizationParms(WspAuthorization_t **auth)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    initializeAuthorizationParms(%lx)\n", (unsigned long) auth));

   if (auth == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (*auth != NULL)
      releaseAuthorizationParms(*auth);

   *auth = (WspAuthorization_t *) xppMalloc(sizeof(WspAuthorization_t));

   if (*auth == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(*auth, 0, sizeof(WspAuthorization_t));

   return rc;
} /* End initializeAuthorization() */

unsigned int initializeResponseParms(WspHttpParms_t *parmPtr, const char *rspHeader)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    initializeResponseParms(%lx, %s)\n", (unsigned long) parmPtr, rspHeader));

   if (parmPtr == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (parmPtr->response != NULL)
      releaseResponseParms(parmPtr->response);

   parmPtr->response = (WspHttpResponseParms_t *) xppMalloc(sizeof(WspHttpResponseParms_t));

   if (parmPtr->response == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(parmPtr->response, 0, sizeof(WspHttpResponseParms_t));

   rc = parseResponseParms(parmPtr->response, rspHeader);

/* I think we may want to leave whatever response info we can process so it
 * can be queried for 'why it failed'.  It'll be cleaned up on the next
 * request anyway
 */
/*
   if (rc != SML_ERR_OK) {
      releaseResponseParms(parmPtr->response);
      parmPtr->response = NULL;
   }
*/

   return rc;
} /* End initializeResponseParms() */

unsigned int parseResponseParms(WspHttpResponseParms_t *response, const char *rspHdr)
{

   unsigned int rc = SML_ERR_OK;
   char  *tempValue = NULL;

   XPTDEBUG(("    parseResponseParms(%lx, %s)\n", (unsigned long) response, rspHdr));

   if (response == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (rspHdr == NULL)
      return rc;

   /* Binding MUST support                                                     */
   /* Binding must honor                                                       */
   /* Cache-control: private                                                   */
   response->cacheControl = getHeaderTag("Cache-Control", rspHdr);

   /* Binding optionally supports                                              */
   /* Either Binding does redirect automatically, or reflects to client        */
   /* Location: "URI"                                                          */
   response->location = getHeaderTag("Location", rspHdr);

   /* Binding optionally supports                                              */
   /* Either Binding does retry automatically, or reflects to client           */
   /* Retry-After: <date|secs>                                                 */
   response->retryAfter = getHeaderTag("Retry-After", rspHdr);

   /* Binding MUST support                                                     */
   /* Binding must verify successful client auth                               */
   /* Authentication-Info: <info>                                               */

   tempValue = getHeaderTag("Authentication-Info", rspHdr);

   if (tempValue != NULL)
      rc = initializeAuthVerification(response, tempValue);

   xppFree(tempValue);

   if (rc != SML_ERR_OK)
      return rc;           /* Keep whatever response info we have...???        */

   /* Binding MUST support                                                     */
   /* Binding must reply to challenge with Authorization                       */
   /* WWW-Authenticate: <auth challenge>                                       */
   tempValue = getHeaderTag("WWW-Authenticate", rspHdr);

   if (tempValue != NULL)
      rc = initializeAuthChallenge(response, tempValue);

   xppFree(tempValue);

   return rc;
} /* End parseResponseParms() */

unsigned int initializeAuthVerification(WspHttpResponseParms_t *response,
                                        char *tempValue)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    initializeAuthVerification(%lx, %s)\n", (unsigned long) response, tempValue));

   if (response == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   releaseAuthenticationParms(response->authVerification);

   if (tempValue == NULL)
      return rc;

   response->authVerification = (WspAuthentication_t *) xppMalloc(sizeof(WspAuthentication_t));
   if (response->authVerification == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(response->authVerification, 0, sizeof(WspAuthentication_t));
   rc = parseAuthVerification(response->authVerification, tempValue);

   return rc;
} /* End initializeAuthVerification() */

unsigned int parseAuthVerification(WspAuthentication_t *auth, const char *source)
{
   XPTDEBUG(("    parseAuthVerification(%lx, %s)\n", (unsigned long) auth, source));

   if ((auth == NULL) || (source == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   auth->nextNonce = getHeaderParmValue("nextnonce", source);
   auth->qop       = getHeaderParmValue("qop", source);
   auth->rspAuth   = getHeaderParmValue("rspauth", source);
   auth->cnonce    = getHeaderParmValue("cnonce", source);
   auth->nc        = getHeaderParmValue("nc", source);

   return SML_ERR_OK;
} /* End parseAuthVerification() */

unsigned int initializeAuthChallenge(WspHttpResponseParms_t *response,
                                     char *tempValue)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    initializeAuthChallenge(%lx, %s)\n", (unsigned long) response, tempValue));

   if (response == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   releaseChallengeParms(response->authChallenge);

   if (tempValue == NULL)
      return rc;

   response->authChallenge = (WspWWWAuthenticate_t *) xppMalloc(sizeof(WspWWWAuthenticate_t));
   if (response->authChallenge == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(response->authChallenge, 0, sizeof(WspWWWAuthenticate_t));
   rc = parseAuthChallenge(response->authChallenge, tempValue);

   return rc;
} /* End initializeAuthChallenge() */

unsigned int parseAuthChallenge(WspWWWAuthenticate_t *auth, const char *source)
{
   unsigned int rc         = SML_ERR_OK;
   const char  *sPtr       = source;
   const char  *ePtr       = NULL;
   int          authLen    = 0;

   XPTDEBUG(("    parseAuthChallenge(%lx, %s)\n", (unsigned long) auth, source));

   if ((auth == NULL) || (source == NULL))
      return SML_ERR_A_XPT_INVALID_PARM;

   ePtr = sPtr + xppStrlen(source);

   auth->authType = getMetaInfoValue(source, "Basic");
   XPTDEBUG(("    parseAuthChallenge() auth->authType = %lx\n", (unsigned long) auth->authType));
   if ((auth->authType != NULL) && (!xppStrcmp(auth->authType, ""))) {
      xppFree(auth->authType);
      auth->authType = (char *) xppMalloc (xppStrlen("Basic") + 1);
      if (auth->authType != NULL)
         xppStrcpy(auth->authType, "Basic");
   } else {
      auth->authType = getMetaInfoValue(source, "Digest");
      if ((auth->authType != NULL) && (!xppStrcmp(auth->authType, ""))) {
         xppFree(auth->authType);
         auth->authType = (char *) xppMalloc (xppStrlen("Basic") + 1);
         if (auth->authType != NULL)
            xppStrcpy(auth->authType, "Digest");
      }
   }

   if (auth->authType == NULL)
      return SML_ERR_A_XPT_COMMUNICATION;      /* Unsupported auth type */

   authLen = xppStrlen(auth->authType);
   /* Bump buffer pointer past auth type in source */
   while ((sPtr < ePtr) && (xppStrncmp(sPtr, auth->authType, authLen) !=0)) {
      sPtr++;
   }

   if (!xppStrncmp(sPtr, auth->authType, authLen)) {
      sPtr += authLen;
      while ((sPtr < ePtr) && (!xppStrncmp(sPtr, " ", 1)))
         sPtr++;
      if (sPtr >= ePtr)
         sPtr = source;
   } else
      sPtr = source;

   auth->realm = getHeaderParmValue("realm", sPtr);

   if (!xppStrcmp(auth->authType, "Digest")) {
      auth->domain = getHeaderParmValue("domain", sPtr);
      auth->nonce  = getHeaderParmValue("nonce", sPtr);
      auth->opaque = getHeaderParmValue("opaque", sPtr);
      auth->stale  = getHeaderParmValue("stale", sPtr);
      auth->algorithm = getHeaderParmValue("algorithm", sPtr);
      auth->qop    = getHeaderParmValue("qop", sPtr);
   } /* End Digest Authentication parms */

   XPTDEBUG(("    parseAuthChallenge() returns %u\n", rc));

   return rc;
} /* End parseAuthChallenge() */


unsigned int validateResponse(WspHttpParms_t *parms)
{

   XPTDEBUG(("    validateResponse(%lx)\n", (unsigned long) parms));
   /* Accept
    * Verify that server response complies, else reject response               */

   /* Accept-Charset
    * Verify that server response complies, else reject response               */

   /* Accept-Encoding
    * Verify any server encoding complies, else reject response
    * It's up to client to decode - binding is just conduit                    */

   /* Accept-Language
    * Verify any server encoding complies, else reject response
    * It's up to client to decode - binding is just conduit                    */


   return SML_ERR_OK;
} /* End validateResponse() */

void releaseRequestParms(WspHttpRequestParms_t *request)
{

   XPTDEBUG(("    releaseRequestParms(%lx)\n", (unsigned long) request));

   if (request == NULL)
      return;

   xppFree(request->accept);
   xppFree(request->acceptCharset);
   xppFree(request->from);
   xppFree(request->acceptEncoding);
   xppFree(request->acceptLanguage);
   xppFree(request->host);
   xppFree(request->referer);
   xppFree(request->composedHeader);

   releaseAuthorizationParms(request->authorization);

   xppFree(request);

} /* End releaseRequestParms() */

void releaseAuthorizationParms(WspAuthorization_t *parms)
{
   XPTDEBUG(("    releaseAuthorizationParms(%lx)\n", (unsigned long) parms));

   if (parms == NULL)
      return;

   xppFree(parms->authType);
   xppFree(parms->userid);
   xppFree(parms->password);
   xppFree(parms->realm);
   xppFree(parms->cnonce);
   xppFree(parms->opaque);

   xppFree(parms);

} /* End releaseAuthorizationParms() */

void releaseResponseParms(WspHttpResponseParms_t *response)
{

   XPTDEBUG(("    releaseResponseParms(%lx)\n", (unsigned long) response));

   if (response == NULL)
      return;

   xppFree(response->cacheControl);
   xppFree(response->location);
   xppFree(response->retryAfter);

   releaseChallengeParms(response->authChallenge);
   releaseAuthenticationParms(response->authVerification);

   xppFree(response);

} /* End releaseResponseParms() */

void releaseChallengeParms(WspWWWAuthenticate_t *parms)
{

   XPTDEBUG(("    releaseChallengeParms(%lx)\n", (unsigned long) parms));

   if (parms == NULL)
      return;

   xppFree(parms->authType);
   xppFree(parms->realm);
   xppFree(parms->domain);
   xppFree(parms->nonce);
   xppFree(parms->opaque);
   xppFree(parms->stale);
   xppFree(parms->algorithm);
   xppFree(parms->qop);

   xppFree(parms);

} /* End releaseChallengeParms() */

void releaseAuthenticationParms(WspAuthentication_t *parms)
{

   XPTDEBUG(("    releaseAuthenticationParms(%lx)\n", (unsigned long) parms));

   if (parms == NULL)
      return;

   xppFree(parms->nextNonce);
   xppFree(parms->qop);
   xppFree(parms->rspAuth);
   xppFree(parms->cnonce);
   xppFree(parms->nc);

   xppFree(parms);

} /* End releaseAuthenticationParms() */


/*
 * Returns a pointer to the address in the buffer where the next token
 * could begin, i.e. points to immediately after the end of the tag
 * that was just added to the buffer
 */
void addTag(char *buffer, const char *tagName, const char *tagValue)
{

   if ((buffer == NULL) || (tagName == NULL) || (tagValue == NULL))
      return;

   xppStrcat(buffer, tagName);
   xppStrcat(buffer, ": ");
   xppStrcat(buffer, tagValue);
   xppStrcat(buffer, NL);

   return;
} /* End addTag() */

int calcStaticHeaderSize(WspHttpRequestParms_t *request)
{
   int length = 0;

   XPTDEBUG(("    calcStaticHeaderSize(%lx)\n", (unsigned long) request));

   length += getTagLen("Cache-Control", D_CACHE_CONTROL);
   length += getTagLen("Accept", request->accept);
   length += getTagLen("Accept-Charset", request->acceptCharset);
   length += getTagLen("From", request->from);
   length += getTagLen("User-Agent", D_USER_AGENT);
   length += getTagLen("Accept-Encoding", request->acceptEncoding);
   length += getTagLen("Accept-Language", request->acceptLanguage);
   length += getTagLen("host", request->host);

   length += xppStrlen(NL);
   length += 1;            /* For null terminator */

   return length;
} /* End calcStaticHeaderSize() */

int getTagLen(const char *tagName, const char *tagValue)
{
   int length = 0;

   if ((tagName == NULL) || (tagValue == NULL))
      return 0;

   length = length + xppStrlen(tagName) + xppStrlen(": ") + xppStrlen(tagValue) + xppStrlen(NL);

   return length;
} /* end getTagLen() */


/**
 * 1.  Parse settings parms on selectProtocol to:
 *       - establish client-specific parms
 *       - initialize default values for non-specified parms
 * 2.  Compose request header for invoke requests, given content-type,
 *     content-length
 *       - who handles referer?  We should, but how does this class know
 *         it's needed?
 * 3.  Parse response header
 * 3.5 Provide caller methods for verifying response, i.e.
 *       - caller sees 401, wants to set auth info and reget request headers
 *       - caller wants to verify response authinfo?
 *       - tell caller of auth failures (same as challenge?)
 *       - tell caller of other reject conditions
 * 4.  Accept changes to request auth info?  If we don't prime with
 *     the auth info and get challenged, the caller will need to compose
 *     a response to the challenge.
 **/


void auth(WspHttpParms_t *parmPtr)
{
   if (parmPtr->request->authorization != NULL)  {
      /* Binding MUST support, Specified by client, or not included            */
      /* Verify domain/realm before sending                                    */
      if (parmPtr->request->authorization->authType != NULL) {
         xppStrcat(parmPtr->request->composedHeader, "Authorization: ");
         xppStrcat(parmPtr->request->composedHeader, parmPtr->request->authorization->authType);
         xppStrcat(parmPtr->request->composedHeader, " ");
         if (!xppStrcmp(parmPtr->request->authorization->authType, "BASIC")) {
         /* base64-encode userid:password */
            xppStrcat(parmPtr->request->composedHeader, parmPtr->request->authorization->userid);
            xppStrcat(parmPtr->request->composedHeader, ":");
            xppStrcat(parmPtr->request->composedHeader, parmPtr->request->authorization->password);
            xppStrcat(parmPtr->request->composedHeader, NL);
         } else if (!xppStrcmp(parmPtr->request->authorization->authType, "DIGEST")) {
            xppStrcat(parmPtr->request->composedHeader, " username=");
            xppStrcat(parmPtr->request->composedHeader, parmPtr->request->authorization->userid);
            xppStrcat(parmPtr->request->composedHeader, " realm=");
            xppStrcat(parmPtr->request->composedHeader, parmPtr->request->authorization->realm);
            xppStrcat(parmPtr->request->composedHeader, "nonce = \"" "\" ");
            xppStrcat(parmPtr->request->composedHeader, "uri=\"transaction->uri\" ");
            xppStrcat(parmPtr->request->composedHeader, "response=\"request-digest\" ");
            xppStrcat(parmPtr->request->composedHeader, "algorithm = \"<MD5|MD5-sess|token>\" ");
            xppStrcat(parmPtr->request->composedHeader, "cnonce=\"");
            xppStrcat(parmPtr->request->composedHeader, parmPtr->request->authorization->cnonce);
            xppStrcat(parmPtr->request->composedHeader, "\" ");
            xppStrcat(parmPtr->request->composedHeader, "opaque = \"");
            xppStrcat(parmPtr->request->composedHeader, parmPtr->request->authorization->opaque);
            xppStrcat(parmPtr->request->composedHeader, "\" ");
            xppStrcat(parmPtr->request->composedHeader, "qop=\"<auth|auth-int|token>\" ");
            xppStrcat(parmPtr->request->composedHeader, "nc=<nonceCountValue> ");
            xppStrcat(parmPtr->request->composedHeader, NL);
         } /* End digest auth  */
      } /* End valid auth type */
   } /* End authorization */

} /* End auth() */


