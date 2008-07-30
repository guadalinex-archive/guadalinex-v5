/*************************************************************************/
/* module:          Communication Services, WSP HTTP Header Functions    */
/* file:            src/xpt/all/wsphttp.h                                */
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

#ifndef WSP_HTTP_H
#define WSP_HTTP_H

#include <xptTransport.h>
#include <wspdef.h>

/* Defines for binding parameters supplied by client on xptSelectProtocol() */
#define WSP_HTTP_ACCEPT          "accept"
#define WSP_HTTP_ACCEPT_CHARSET  "acceptCharset"
#define WSP_HTTP_ACCEPT_ENCODING "acceptEncoding"
#define WSP_HTTP_ACCEPT_LANGUAGE "acceptLanguage"
#define WSP_HTTP_FROM            "emailAddress"
#define WSP_HTTP_AUTH_USERID     "authUserid"
#define WSP_HTTP_AUTH_PASSWORD   "authPassword"
#define WSP_HTTP_AUTH_REALM      "authRealm"


#define NL "\n"
#define D_CACHE_CONTROL "no-store"
#define D_ACCEPT "application/vnd.syncml-xml, application/vnd.syncml-wbxml"
#define D_ACCEPT_CHARSET "UTF8"
#define D_ACCEPT_LANGUAGE "en"
#define D_USER_AGENT "WAP Client ["D_ACCEPT_LANGUAGE"] (WinNT; I)"


/**
 *  A structure to hold parameters related to HTTP Authorization Header Tag Info
 **/
typedef struct {

   char *authType;   /* Basic or Digest */
   char *userid;     /* Basic,Digest */
   char *password;   /* Basic */
   char *realm;      /* Basic, Digest */
   char *cnonce;     /* Digest */
   char *opaque;     /* Digest */

} WspAuthorization_t;

/**
 *  A structure to hold parameters related to HTTP WWW Authenticate Header Tag
 **/
typedef struct {

   char *authType;   /* Basic or Digest */
   char *realm;      /* Basic or Digest */
   char *domain;     /* Digest */
   char *nonce;      /* Digest */
   char *opaque;     /* Digest */
   char *stale;      /* Digest */
   char *algorithm;  /* Digest */
   char *qop;        /* Digest */

} WspWWWAuthenticate_t;

/**
 *  A structure to hold parameters related to HTTP Authentication-Info Header Tag
 **/
typedef struct {
   char *nextNonce;
   char *qop;
   char *rspAuth;
   char *cnonce;
   char *nc;

} WspAuthentication_t;

/**
 *  A structure to hold parameters related to HTTP Request Header Tags
 **/
typedef struct {
   char *accept;
   char *acceptCharset;
   char *from;
   char *acceptEncoding;
   char *acceptLanguage;
   char *host;
   char *referer;

   WspAuthorization_t *authorization;

   char *composedHeader;

} WspHttpRequestParms_t;

/**
 *  A structure to hold parameters related to HTTP Response Header Tags
 **/
typedef struct {
   char *cacheControl;
   char *location;
   char *retryAfter;

   WspWWWAuthenticate_t *authChallenge;
   WspAuthentication_t  *authVerification;

} WspHttpResponseParms_t;

/**
 *  A structure to hold parameters related to HTTP Header Tags
 **/
typedef struct {
   unsigned long           valid;
   WspHttpRequestParms_t  *request;
   WspHttpResponseParms_t *response;
} WspHttpParms_t;



/* Methods invoked from outside wsphttp.c */
unsigned int httpInitialize(const char *szSettings, WspHttpParms_t **parmPtr) WSP_SECTION;
const char *httpGetRequestHdrs(WspHttpParms_t *parmPtr, const char *contentType,
                                  const char *contentLength) WSP_SECTION;
const char *httpGetStaticRequestHdrs(WspHttpParms_t *parmPtr) WSP_SECTION;
void httpRelease(WspHttpParms_t *parmPtr) WSP_SECTION;
void httpReinitParms(WspHttpParms_t *parmPtr) WSP_SECTION;
unsigned int httpSetReferer(WspHttpParms_t *parmPtr, const char *referer) WSP_SECTION;
unsigned int httpUpdateAuthorization(WspHttpParms_t *parmPtr, const char *userid,
                                     const char *password, const char *realm) WSP_SECTION;
unsigned int httpParseResponseHdr(WspHttpParms_t *parmPtr, const char *rspHeader) WSP_SECTION;

/* Methods invoked only within wsphttp.c */
void addTag(char *buffer, const char *tagName, const char *tagValue) WSP_SECTION;
void auth(WspHttpParms_t *parmPtr) WSP_SECTION;
int calcStaticHeaderSize(WspHttpRequestParms_t *request) WSP_SECTION;
char *getRFC822Date(void) WSP_SECTION;
int getTagLen(const char *tagName, const char *tagValue) WSP_SECTION;
unsigned int initializeAuthChallenge(WspHttpResponseParms_t *response,
                                     char *tempValue) WSP_SECTION;
unsigned int initializeAuthorizationParms(WspAuthorization_t **auth) WSP_SECTION;
unsigned int initializeAuthVerification(WspHttpResponseParms_t *response,
                                        char *tempValue) WSP_SECTION;
unsigned int initializeRequestParms(WspHttpParms_t *parmPtr, const char *settings) WSP_SECTION;
unsigned int initializeResponseParms(WspHttpParms_t *parmPtr, const char *rspHeader) WSP_SECTION;
unsigned int parseAuthChallenge(WspWWWAuthenticate_t *auth, const char *source) WSP_SECTION;
unsigned int parseAuthorizationParms(WspHttpRequestParms_t *request, const char *settings) WSP_SECTION;
unsigned int parseAuthVerification(WspAuthentication_t *auth, const char *source) WSP_SECTION;
unsigned int parseRequestParms(WspHttpRequestParms_t *request, const char *settings) WSP_SECTION;
unsigned int parseResponseParms(WspHttpResponseParms_t *response, const char *rspHdr) WSP_SECTION;
void releaseChallengeParms(WspWWWAuthenticate_t *parms) WSP_SECTION;
void releaseAuthenticationParms(WspAuthentication_t *parms) WSP_SECTION;
void releaseAuthorizationParms(WspAuthorization_t *parms) WSP_SECTION;
void releaseRequestParms(WspHttpRequestParms_t *request) WSP_SECTION;
void releaseResponseParms(WspHttpResponseParms_t *response) WSP_SECTION;
unsigned int validateResponse(WspHttpParms_t *parms) WSP_SECTION;

#endif
