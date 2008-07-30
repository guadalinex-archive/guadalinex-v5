/*************************************************************************/
/* module:          Communication Services, HTTP functions               */
/* file:            src/xpt/all/xpt-http.c                               */
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

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#include <xpttypes.h>
#include "xptihttp.h"

#include "xpt-tcp.h"
#include "xpt-http.h"
#include "xpt-auth.h"
#include <xpt.h>
#include <xptport.h>

#define CACHE_BUFFER_SIZE   2000  // Size of the transfer buffer
#define CHUNK_HEADER_SIZE   8     // size of a chunk header.
                      // must be greater than (CACHE_BUFFER_SIZE LOG 10) + 2
#define HTTP_STATUS_UNDEFINED -1
#define HTTP_DEFAULT_CONNECTION_TYPE  CLOSE // KEEP_ALIVE

// %%% luz: added ifdefs to allow pre-defining these values in the prefix file
// - HTTP language
#ifndef HTTP_LANGUAGE
  #define HTTP_LANGUAGE    "en"
#endif
// - server name
#ifndef SERVER_NAME
  #define SERVER_NAME      "HTTP SyncML Server/0.50.0.0"
#endif
// - client name
#ifndef CLIENT_NAME
  #if defined(__EPOC_OS__)
    #define CLIENT_NAME      "HTTP SyncML Client ["HTTP_LANGUAGE"] (EPOC; I)"
  #elif defined(__PALM_OS__)
    #define CLIENT_NAME      "HTTP SyncML Client ["HTTP_LANGUAGE"] (PalmOS; I)"
  #elif defined(WINCE)
    #define CLIENT_NAME      "HTTP SyncML Client ["HTTP_LANGUAGE"] (WinCE; I)"
  #elif defined(WIN32)
    #define CLIENT_NAME      "HTTP SyncML Client ["HTTP_LANGUAGE"] (Win32; I)"
  #elif defined(linux)
    #define CLIENT_NAME      "HTTP SyncML Client ["HTTP_LANGUAGE"] (Linux; I)"
  #endif
#endif
// - other HTTP headers
#define HTTP_VERSION     "HTTP/1.1"
#define HTTP_MIME_TYPES  "application/vnd.syncml+xml, application/vnd.syncml+wbxml, */*"
#define HTTP_CACHECONTROL "private"

#ifndef min
 #define min(a, b) ((a) < (b) ? (a) : (b))
#endif


/*************************************/
/* Type definition for object store  */
/*************************************/

typedef struct
{
   XptHmacInfo_t hmacInfo;
   char *pchHmac;
} HttpHmacInfo_t, *HttpHmacInfoPtr_t;

typedef struct
   {
   BufferSize_t cbSize;                          // size of this data structure

   char *pchRequest;                             // HTTP request: "RECEIVE", "SEND", "EXCHANGE" or "SERVER"
   char *pchURI;                                 // URL name
   char *pchHost;                                // Host IP address
   char *pchProxy;                               // Proxy IP address
   char *pchResponseType;                        // MIME type of data received
   char *pchRequestType;                         // MIME type of data xfered
   char *pchReferer;                             // Referenced URL
   char *pchFrom;                                // address of sender
   HttpHmacInfo_t sXSyncmlHmac;                  // HMAC Information

   BufferSize_t cbDataLength;                    // Length of real data
   Bool_t bEox;                                  // end-of-transmission flag

   enum {                                        // HTTP requests supported
      MODE_HTTP_SERVER       = 0,
      MODE_HTTP_GET          = 1,
      MODE_HTTP_PUT          = 2,
      MODE_HTTP_POST         = 3
      } fOpenMode;                               // Communication open mode

   enum
      {
      MODE_UNDEFINED = 0,
      MODE_READING = 1,
      MODE_WRITING = 2
      } fTransferMode;                          // Transfer mode

   enum
      {
      NOT_CHUNKED = 0,
      CHUNKED = 1
      } fTransferCoding;                         // data transfer coding

   enum
      {
      CLOSE = 0,
      KEEP_ALIVE = 1
      } fConnection;                             // connection type

   // enum
   //    {
   //    FLAT = 0,
   //    BASE64 = 1
   //    } fEncoding;                               // data encoding type

   TcpRc_t  iRc;                                 // Communication return code
   int iHttpStatus;                              // status of HTTP response
   SocketPtr_t pSession;                         // TCP/IP socket
   // unsigned char abBase64Data [4];               // needed by base64 encoding routine
   // BufferSize_t cbDocumentOffset;                // total # of document bytes read.
   BufferSize_t cbDataToRead;                    // # of bytes already read.
   BufferSize_t cbCacheSize;                     // size of data xfer buffer
   unsigned char pbCache [CACHE_BUFFER_SIZE];           // Data xfer buffer
   BufferSize_t cbCacheUsed;                     // number of bytes processed

   #ifdef STATIC_MEMORY_MANAGEMENT
   BufferSize_t cbStringHeapSize;                // Size of string heap buffer
   BufferSize_t cbStringHeapUsed;                // pointer to free string heap space
   char  pchStringBuffer [STATIC_MEMORY_SIZE];   // memory buffer for strings
   #endif

   } HttpBuffer_t, *HttpBufferPtr_t;


/* the following enumeration denotes the data encoding type */


/*****************************************************************************/
/*                                                                           */
/*                   String Management functions                             */
/*                                                                           */
/*****************************************************************************/

#ifdef STATIC_MEMORY_MANAGEMENT

#define makeString(s) newString ((s),p)
// %%%luz 2002-08-28: second argument p was missing here:
#define makeHmacString(s,p) newHmacString((s),(p))
#define freeString(s)

/*************************************************/
/* Function: copy a string into the string heap, */
/* return a pointer to the copied string         */
/*************************************************/
StringBuffer_t newString (CString_t pszString, HttpBufferPtr_t p)
   {
   StringBuffer_t rc = p->pchStringBuffer + p->cbStringHeapUsed;
   if (pszString != NULL)
      {
      StringLength_t cbStringLength = xppStrlen (pszString);
      if (p->cbStringHeapSize < cbStringLength + 1 + p->cbStringHeapUsed)
         rc = NULL;
      else
         {
         xppStrcpy (p->pchStringBuffer + p->cbStringHeapUsed, pszString);
         p->cbStringHeapUsed += cbStringLength + 1;
         }
      }
   else
      rc = NULL;
   return rc;
   }

// %%%luz 2002-08-28: second argument p was missing here
StringBuffer_t newHmacString(XptHmacInfoPtr_t pHmacInfo, HttpBufferPtr_t p)
{
	StringBuffer_t rc = p->pchStringBuffer + p->cbStringHeapUsed;
	StringLength_t cbStringLength;

	if ((pHmacInfo != NULL) && (pHmacInfo->username != NULL) && (pHmacInfo->mac != NULL))
	{
	  cbStringLength = xppStrlen("algorithm=") 
			+ xppStrlen( (pHmacInfo->algorithm != NULL) ? "MD5" : pHmacInfo->algorithm)
			+ xppStrlen(", username=") + xppStrlen (pHmacInfo->username)
			+ xppStrlen(", mac=") + xppStrlen (pHmacInfo->mac);

      if (p->cbStringHeapSize < cbStringLength + 1 + p->cbStringHeapUsed)
         rc = NULL;
      else
         {
         sprintf(p->pchStringBuffer + p->cbStringHeapUsed, "algorithm=%s, username=%s, mac=%s", 
					(pHmacInfo->algorithm == NULL) ? "MD5" : pHmacInfo->algorithm, 
					pHmacInfo->username, pHmacInfo->mac);
         p->cbStringHeapUsed += cbStringLength + 1;
         }
   }
   else
      rc = NULL;
   return rc;
	}

#else

static char *copyString(const char *string) {
   char *copy = xppMalloc(xppStrlen(string) + 1);
   if (!copy) return copy;    /* Unable to allocate storage */
   xppStrcpy(copy, string);
   return copy;
}

StringBuffer_t copyHmacString(XptHmacInfoPtr_t pHmacInfo)
{
	StringLength_t cbStringLength;
    char *copy;

	if ((pHmacInfo != NULL) && (pHmacInfo->username != NULL) && (pHmacInfo->mac != NULL))
	{
 		cbStringLength = xppStrlen("algorithm=") 
			+ xppStrlen( (pHmacInfo->algorithm == NULL) ? "MD5" : pHmacInfo->algorithm)
			+ xppStrlen(", username=\"") + xppStrlen (pHmacInfo->username)
			+ xppStrlen("\", mac=") + xppStrlen (pHmacInfo->mac);

		copy = xppMalloc(cbStringLength + 1);
		if (!copy) return copy;    /* Unable to allocate storage */

		sprintf(copy, "algorithm=%s, username=\"%s\", mac=%s", 
			(pHmacInfo->algorithm == NULL) ? "MD5" : pHmacInfo->algorithm, 
			pHmacInfo->username, pHmacInfo->mac);
    }
	else
	  copy = NULL;

    return copy;
}

/* No static memory management: use CRT memory heap */
#define makeString(s) ((s) ? copyString(s) : NULL)
// %%%luz 2002-08-28: second argument p was missing here (not really
//                    used in non-static memory management case):
#define makeHmacString(s,p) copyHmacString((s))
#define freeString(s) if ((s)) xppFree ((s))


#endif


/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                     private functions                                   **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

/* check if the handle that has been passed by the caller is correct. */
// Note: gcc cannot parse multi-line macros in files with DOS lineends
#define ISHEADERVALID(p) (((p) != NULL) && ((p)->cbSize == sizeof (HttpBuffer_t)))

/* compute the logarithm of an integer */
#define LOG(n) ((n)>10000?5:((n)>1000?4:((n)>100?3:((n)>10?2:1))))



/*********************************************/
/* Function: remove the "HTTP://<hostname>/" */
/* stuff from the specified URI              */
/* Returns: the resource name                */
/*********************************************/
const char *getResourceName (const char *pszURI)
   {
   const char *rc = NULL;
   if ((xppStrlen (pszURI) > 7) &&
       (pszURI [0] == 'H' || pszURI [0] == 'h') &&
       (pszURI [1] == 'T' || pszURI [1] == 't') &&
       (pszURI [2] == 'T' || pszURI [2] == 't') &&
       (pszURI [3] == 'P' || pszURI [3] == 'p') &&
       pszURI [4] == ':' && pszURI [5] == '/' && pszURI [6] == '/')
      rc = xppStrchr (pszURI+7, '/');

   if (rc == NULL)
      rc = pszURI;
   else
      rc ++;
   return rc;
   }

//Bool_t isBinaryMimeType (CString_t pszName)
//   {
//   static struct { CString_t name; Bool_t type; } mimetypes [] =
//      { { "application/vnd.syncml-xml", false },
//        { "application/vnd.syncml-wbxml", false },
//        { "text/html",  false },
//        { "text/plain", false },
//        { NULL, false } };
//   int n;
//
//   for (n = 0; mimetypes [n].name != NULL; n ++)
//      {
//      if (!xppStricmp (pszName, mimetypes [n].name))
//         break;
//      }
//   return mimetypes[n].type;
//   }



void writeGeneralRequestHeader (HttpBufferPtr_t p, char *pszHeader)
   {
   // Note: gcc cannot parse line continuation in files with DOS lineends (but they are not needed here!)
   sprintf (pszHeader + xppStrlen (pszHeader),
            "Cache-Control: "HTTP_CACHECONTROL"\r\n"
            "Connection: %s\r\n"
            "User-Agent: " CLIENT_NAME "\r\n"
            "Accept: "HTTP_MIME_TYPES"\r\n"
            "Accept-Language: "HTTP_LANGUAGE"\r\n"
            "Accept-Charset: utf-8\r\n",
            p->fConnection == KEEP_ALIVE ? "keep-alive" : "close");

   if ((p->pchReferer != NULL) && (p->pchReferer [0] != '\0'))
      sprintf (pszHeader + xppStrlen (pszHeader),
               "Referer: %s\r\n", p->pchReferer);

   if ((p->pchFrom != NULL) && (p->pchFrom [0] != '\0'))
      sprintf (pszHeader + xppStrlen (pszHeader),
               "From: %s\r\n", p->pchFrom);

   if ((p->pchHost != NULL) && (p->pchHost [0] != '\0'))
      sprintf (pszHeader + xppStrlen (pszHeader),
               "Host: %s\r\n", p->pchHost);
   return;
   }

void writeContentRequestHeader (HttpBufferPtr_t p, char *pszHeader)
   {
   if ((p->pchRequestType != NULL) && (p->pchRequestType [0] != '\0'))
      sprintf (pszHeader + xppStrlen (pszHeader),
               "Content-Type: %s\r\n", p->pchRequestType);

   if (p->cbDataLength > 0)
      sprintf (pszHeader + xppStrlen (pszHeader),
               "Content-Length: %ld\r\n", p->cbDataLength);

   /*******************************************************************/
   /* if there are data to send, determine the transfer encoding mode */
   /*******************************************************************/
   if (((p->fOpenMode == MODE_HTTP_POST) || (p->fOpenMode == MODE_HTTP_PUT)) &&
       (p->cbDataLength == 0) )
      p->fTransferCoding = CHUNKED; // enable chunked transfer coding
   else
      p->fTransferCoding = NOT_CHUNKED;

//    if ((p->pchRequestType == NULL) || !isBinaryMimeType (p->pchRequestType))
//       p->fEncoding = FLAT;
//    else
//       p->fEncoding = BASE64;

//   if ((p->fEncoding == BASE64) || (p->fTransferCoding == CHUNKED))
   if (p->fTransferCoding == CHUNKED)
      {
      sprintf (pszHeader+xppStrlen (pszHeader), "Transfer-Encoding: chunked\r\n");
//
//    if (p->fTransferCoding == CHUNKED)
//       sprintf (pszHeader + xppStrlen (pszHeader), " chunked");
//
//    if (p->fEncoding == BASE64)
//       sprintf (pszHeader + xppStrlen (pszHeader), " base64");
//
//    sprintf (pszHeader + xppStrlen (pszHeader), "\r\n");
      }
   if ((p->sXSyncmlHmac.pchHmac != NULL) && (p->sXSyncmlHmac.pchHmac[0] != '\0'))
	   sprintf (pszHeader + xppStrlen (pszHeader),
	            "x-syncml-hmac: %s\r\n", p->sXSyncmlHmac.pchHmac);
   return;
   }

void writeAuthorizationRequestHeader (HttpBufferPtr_t p,
                                      char *pszHeader, char *pszMode,
                                      HttpAuthenticationPtr_t auth,
                                      HttpAuthenticationDest_t dest)
   {
   HttpAuthenticationType_t fType = authGetType (auth, dest);
   const char *pszURI;

   const char *pszAuthType = (dest == DEST_PROXY) ?
                             "Proxy-Authorization" : "Authorization";
   const char *pszDigest = authGetDigest (auth, dest);

   if (fType == AUTH_BASIC)
      {
      sprintf (pszHeader, "%s: Basic %s\r\n",
               pszAuthType, pszDigest);
      }

   else if (fType == AUTH_DIGEST)
      {
      HttpAuthenticationData_t ad;
      HttpAuthenticationUserData_t ud;
      ad.cbSize = sizeof (ad);
      ud.cbSize = sizeof (ud);
      authGetAuthenticationData (auth, dest, &ad);
      authGetUserData (auth, dest, &ud);

      pszURI = getResourceName (p->pchURI);

      sprintf (pszHeader,
               "%s: Digest username=\"%s\", realm=\"%s\", "
               "nonce=\"%s\", uri=\"%s\", response=\"%s\"",
               pszAuthType, ud.szUser, ad.szRealm,
               ad.szNonce, pszURI, pszDigest);

      if (ad.szQop [0] != '\0')
         {
         sprintf (pszHeader + xppStrlen (pszHeader),
                  ", cnonce=\"%s\", nc=\"%s\", qop=\"%s\"",
                  ud.szCNonce, ud.szNC, ad.szQop);
         }

      if (ad.szOpaque [0] != '\0')
         sprintf (pszHeader + xppStrlen (pszHeader), ", opaque=\"%s\"", ad.szOpaque );

      sprintf (pszHeader + xppStrlen (pszHeader), "\r\n");
      }
   return;
   }

/***************************************************************/
/* Function: Write a HTTP request header into the buffer cache */
/***************************************************************/

void writeRequestHeader (HttpBufferPtr_t p, HttpAuthenticationPtr_t auth)
   {
   char *pszMode = "";
   char *pszHeader = (char *) p->pbCache;
   char *pfx1 = "http://", *pfx2 = "", *pfx3 = "/";
   const char *pszURI = getResourceName(p->pchURI);

   if (p->fOpenMode == MODE_HTTP_POST)     pszMode = "POST";
   else if (p->fOpenMode == MODE_HTTP_GET) pszMode = "GET";
   else if (p->fOpenMode == MODE_HTTP_PUT) pszMode = "PUT";

   /*************************************************************/
   /* Write the request header line. Use absolute URI addresses */
   /* if the connection goes via a proxy.                       */
   /*************************************************************/

   if (  (p->pchHost == NULL) || (p->pchHost [0] == '\0')
       || !xppStrcmp (p->pchURI, "*")
       || (p->pchProxy == NULL) || (p->pchProxy [0] == '\0') )
      pfx1 = "";
   else
      pfx2 = p->pchHost;

   if ((pszURI [0] == '/') || !xppStrcmp (p->pchURI, "*"))
      pfx3 = "";

   sprintf (pszHeader,
            "%s %s%s%s%s "HTTP_VERSION"\r\n",
            pszMode, pfx1, pfx2, pfx3, pszURI);

   /*************************************************/
   /* Write general header and content header data. */
   /*************************************************/

   writeGeneralRequestHeader (p, pszHeader);

   writeContentRequestHeader (p, pszHeader);

   if (authGetType (auth, DEST_SERVER) != AUTH_NONE)
      {
      authCalcDigest (auth, DEST_SERVER, pszURI,  pszMode);
      writeAuthorizationRequestHeader (p, pszHeader+xppStrlen(pszHeader), pszMode, auth, DEST_SERVER);
      }

   if (authGetType (auth, DEST_PROXY) != AUTH_NONE)
      {
      authCalcDigest (auth, DEST_PROXY, pszURI, pszMode);
      writeAuthorizationRequestHeader (p, pszHeader+xppStrlen(pszHeader), pszMode, auth, DEST_PROXY);
      }

   sprintf (pszHeader + xppStrlen (pszHeader), "\r\n");

   p->cbCacheUsed = xppStrlen (pszHeader);
   }



/****************************************************************/
/* Function: Write a HTTP response header into the buffer cache */
/****************************************************************/

void writeResponseHeader (HttpBufferPtr_t p,
                          int rcDocument,
                          const HttpReplyBufferPtr_t settings, // i: Response properties
                          const HttpAuthenticationPtr_t auth) // i: Authentication info
   {
   char achTime [50];
   StringBuffer_t pchHmac;

   _getTime (achTime);  // defined in xptihttp.h

   /******************************************/
   /* Write the HTTP header,                 */
   /* copy the header into the cache,        */
   /* but do not send the data.              */
   /******************************************/

   // Note: gcc cannot parse line continuation in files with DOS lineends (but they are not needed here!)
   sprintf ((char *) p->pbCache,
            HTTP_VERSION" %d \r\n"
            "Server: " SERVER_NAME "\r\n"
            "Date: %s\r\n"
            "Accept-Ranges: bytes\r\n"
            "Cache-Control: "HTTP_CACHECONTROL"\r\n"
            "Connection: %s\r\n",
            rcDocument, achTime,
            p->fConnection == KEEP_ALIVE ? "keep-alive" : "close");

   if (settings != NULL)
      {
      if ((settings->pszTime != NULL) && (settings->pszTime [0] != '\0'))
         sprintf ((char *) p->pbCache + xppStrlen ((char *)p->pbCache),
                  "Last-Modified: %s\r\n", settings->pszTime);

      if ((settings->pszType != NULL) && (settings->pszType [0] != '\0'))
         sprintf ((char *) p->pbCache + xppStrlen ((char *)p->pbCache),
                  "Content-Type: %s\r\n", settings->pszType);

      if (settings->cbLength > 0)
         {
         p->fTransferCoding = NOT_CHUNKED;
         sprintf ((char *) p->pbCache + xppStrlen ((char *)p->pbCache),
                  "Content-Length: %ld\r\n", settings->cbLength);
         }
      else
         p->fTransferCoding = CHUNKED;

//    if (isBinaryMimeType (settings->pszType))
//       p->fEncoding = BASE64;
//    else
//       p->fEncoding = FLAT;

//    if ((p->fTransferCoding == CHUNKED) || (p->fEncoding == BASE64))
      if (p->fTransferCoding == CHUNKED)
         {
         sprintf ((char *)(p->pbCache+xppStrlen ((char *)p->pbCache)), "Transfer-Encoding:");

//       if (p->fTransferCoding == CHUNKED)
            sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)), " chunked");

//       if (p->fEncoding == BASE64)
//          sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)), " base64");

         sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)), "\r\n");
         }

    // %%%luz 2002-08-28: second argument p was missing here
	  pchHmac = makeHmacString(settings->pXSyncmlHmac,p);
      if ((pchHmac != NULL) && (pchHmac [0] != '\0'))
         sprintf ((char *) p->pbCache + xppStrlen ((char *)p->pbCache),
                  "x-syncml-hmac: %s\r\n", pchHmac);
	  freeString(pchHmac);

      }

   /************************************************/
   /* Check if the authentication info are present */
   /************************************************/

   if ((auth != NULL) && ((rcDocument == 401) || (rcDocument == 407))) // authentication required!
      {
      HttpAuthenticationDest_t dest = (rcDocument == 401) ? DEST_SERVER : DEST_PROXY;
      HttpAuthenticationType_t type = authGetType (auth, dest);
      if (type != AUTH_NONE)
         {
         const char *pszDelim = " ";
         const char *d2 = ", ";
         HttpAuthenticationData_t ad;
         ad.cbSize = sizeof (ad);
         authGetAuthenticationData (auth, dest, &ad);

         sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)),
                  "%s: %s",
                  (dest == DEST_PROXY) ? "Proxy-Authenticate" : "WWW-Authenticate",
                  (type == AUTH_DIGEST) ? "digest" : "basic");

         if (ad.szRealm[0] != '\0')
            {
            sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)),
                     "%srealm=\"%s\"", pszDelim, ad.szRealm); pszDelim = d2;
            }

         if (ad.szNonce[0] != '\0')
            {
            sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)),
                     "%snonce=\"%s\"", pszDelim, ad.szNonce); pszDelim = d2;
            }

         if (ad.szOpaque[0] != '\0')
            {
            sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)),
                     "%sopaque=\"%s\"", pszDelim, ad.szOpaque); pszDelim = d2;
            }

         if (ad.szQop[0] != '\0')
            {
            sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)),
                     "%sqop=\"%s\"", pszDelim, ad.szQop); pszDelim = d2;
            }
         if (ad.szDomain[0] != '\0')
            {
            sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)),
                     "%sdomain=\"%s\"", pszDelim, ad.szDomain); pszDelim = d2;
            }
         if (ad.bStale != false)
            {
            sprintf ((char *)(p->pbCache + xppStrlen ((char *)p->pbCache)),
                     "%sstale=true", pszDelim); pszDelim = d2;
            }
         sprintf ((char *) p->pbCache + xppStrlen ((char *)p->pbCache), "\r\n");
         }
      }

   /****************************************************************/
   /* Append the terminating empty line to the HTTP request header */
   /****************************************************************/
   sprintf ((char *) p->pbCache + xppStrlen ((char *)p->pbCache), "\r\n");

   p->cbCacheUsed = xppStrlen ((char *)p->pbCache);

   if (settings != NULL)
      p->fTransferMode = MODE_WRITING;
   else
      p->fTransferMode = MODE_UNDEFINED;
   }

/*******************************************************/
/* Function: parse one token from the specified line   */
/* Return the rest of the string                       */
/* The function changes the contents of pszLine!!      */
/*******************************************************/

StringBuffer_t splitToken (StringBuffer_t pszLine, // i: line
                           StringPtr_t ppszToken)  // o: ptr to extracted token
   {
   StringBuffer_t pszToken = pszLine;
   StringBuffer_t pszRest = NULL;

   /* skip leading blanks */
   while (pszToken [0] == ' ') pszToken ++;
   pszRest = pszToken;

   /* Now we have found the beginning of the token, so let us find the end of it */
   while ((pszRest [0] != '\0') && (pszRest [0] != ' ') && (pszRest [0] != ','))
      pszRest ++;
   if (pszRest [0] != '\0')
      {
      pszRest [0] = '\0'; // separate token from rest
      pszRest ++;
      while (pszRest [0] == ' ') pszRest ++;
      }
   *ppszToken = pszToken;
   return pszRest;
   }

// Note: gcc cannot parse multi-line macros in files with DOS lineends
#define SKIP_WHITESPACE(s) while (((s)[0] == ' ')||((s)[0] == ',')||((s)[0] == '\t')) (s) ++

/**************************************************************/
/* Function: parse one parameter from the specified line.     */
/* it is assumed that the parameter has the following format: */
/*           parm={ value | "value" }[,]                      */
/* Return the rest of the string, or NULL if here are no more */
/* data to handle                                             */
/* The function changes the contents of pszLine!!             */
/**************************************************************/
// %%% luz 2002-04-16: made static as same name is used for slightly different function in obexbinding.c
static StringBuffer_t splitParmValue (StringBuffer_t pszLine, // i: line
                               StringPtr_t ppszParm,  // o: ptr to extracted parameter
                               StringPtr_t ppszValue) // o: ptr to extracted parameter value
   {
   StringBuffer_t pszToken = pszLine;
   StringBuffer_t pszRest = NULL;

   if (pszToken == NULL) return NULL;
   /* skip leading blanks */
   SKIP_WHITESPACE (pszToken);
   pszRest = pszToken;
   *ppszParm = pszRest;
   if (pszToken [0] == '\0') return NULL;

   /* Find the delimiter */
   while ((*pszRest != '\0') && (*pszRest != '=') && (*pszRest != ' ') && (*pszRest != ','))
      pszRest ++;
   switch (*pszRest)
      {
      case '\0': //
         *ppszValue = pszRest;
         break;

      case ',':
      case ' ': // whitespace: there is no value assigned to this parameter
         *pszRest = '\0';
         *ppszValue = pszRest;
         pszRest ++;
         break;

      case '=':
         /* The value part may or may not be enclosed in quotes */
         *pszRest = '\0';
         pszRest ++;
         SKIP_WHITESPACE (pszRest);
         if (pszRest [0] == '\"')
            {
            *ppszValue = ++pszRest;
            while ((*pszRest != '\0') && (*pszRest != '\"'))
               pszRest ++;
            }
         else
            {
            *ppszValue = pszRest;
            while ((*pszRest != '\0') && (*pszRest != ' ') && (*pszRest != ','))
               pszRest ++;
            }
         if (*pszRest)
            { *pszRest = '\0'; pszRest ++; }
         break;
      }
   return pszRest;
   }



/*************************************************************************/
/* Function: Separate the first line from the specified multiline string */
/* Return the rest of the string.                                        */
/* The function changes the contents of pchLine!!                        */
/*************************************************************************/

StringBuffer_t splitLine (StringBuffer_t pchLine) // i: multiline string
   {
   StringBuffer_t ptr;
   StringBuffer_t pszSplit;

   ptr = xppStrchr ((char *)pchLine, '\n');
   if (ptr)
      {
      if (*(ptr-1) == '\r')
         pszSplit = ptr-1;
      else
         pszSplit = ptr;

      *pszSplit = '\0';
      ptr ++;
      }
   return ptr;
   }

void evaluateHdrHost (HttpBufferPtr_t p, StringBuffer_t pszValue)
   {
   freeString (p->pchHost);
   p->pchHost = makeString (pszValue);
   }

void evaluateHdrFrom (HttpBufferPtr_t p, StringBuffer_t pszValue)
   {
   freeString (p->pchFrom);
   p->pchFrom = makeString (pszValue);
   }

void evaluateHdrReferer (HttpBufferPtr_t p, StringBuffer_t pszValue)
   {
   freeString (p->pchReferer);
   p->pchReferer = makeString (pszValue);
   }
void evaluateHdrContentLength (HttpBufferPtr_t p, StringBuffer_t pszValue)
   {
   p->cbDataLength = (BufferSize_t) atol (pszValue);
   }

void evaluateHdrContentType (HttpBufferPtr_t p, StringBuffer_t pszValue)
   {
   freeString (p->pchResponseType);
   p->pchResponseType = makeString (pszValue);
   p->bEox = false; // we expect an attached document
   }

void evaluateHdrConnection (HttpBufferPtr_t p, StringBuffer_t pszValue)
   {
   if (!xppStricmp (pszValue, "close"))
      p->fConnection = CLOSE;
   else
      p->fConnection = KEEP_ALIVE;
   }

void evaluateHdrTransferEncoding (HttpBufferPtr_t p, StringBuffer_t pszValue)
   {
   CString_t pszToken = NULL;
   while ((pszValue != NULL) && (pszValue [0] != '\0'))
      {
      pszValue = splitToken (pszValue, &pszToken);
      if (!xppStricmp (pszToken, "chunked"))
         p->fTransferCoding = CHUNKED;
//    if (!xppStricmp (pszToken, "base64"))
//       p->fEncoding = BASE64;
      }
   p->bEox = false; // we expect an attached document
   }

void evaluateHdrXSyncmlHmac (HttpBufferPtr_t p, StringBuffer_t pszValue)
   {
    StringBuffer_t line;
    CString_t param = NULL;
    CString_t value = NULL;

	
	freeString (p->sXSyncmlHmac.pchHmac);
    p->sXSyncmlHmac.pchHmac = makeString (pszValue);
 
	line = p->sXSyncmlHmac.pchHmac;
	while (line != NULL)
	{
		line = splitParmValue(line, &param, &value);
		if ((param != NULL) && (param [0] != '\0'))
		{
			if (!xppStricmp (param, "algorithm"))
				p->sXSyncmlHmac.hmacInfo.algorithm = value;
			else if (!xppStricmp (param, "username"))
				p->sXSyncmlHmac.hmacInfo.username = value;
			else if (!xppStricmp (param, "mac"))
				p->sXSyncmlHmac.hmacInfo.mac = value;
		}
	}
	}

void evaluateHdrAuthorization (HttpBufferPtr_t p, StringBuffer_t pszValue,
                               HttpAuthenticationPtr_t auth,
                               HttpAuthenticationDest_t dest)
   {
   CString_t pszParm = NULL;
   CString_t pszToken = NULL;
   if (p->fOpenMode == MODE_HTTP_SERVER)
      {
      pszValue = splitToken (pszValue, &pszToken);
      if (!xppStricmp (pszToken, "Basic"))
         authSetDigest (auth, dest, pszValue);

      else if (!xppStricmp (pszToken, "Digest"))
         {
         CString_t pszDigest = NULL;
         HttpAuthenticationUserData_t aud;
         HttpAuthenticationData_t ad;

         xppMemset (&aud, 0, sizeof (aud)); aud.cbSize = sizeof (aud);
         xppMemset (&ad, 0, sizeof (ad)); ad.cbSize = sizeof (ad);

         while (pszValue != NULL)
            {
            pszValue = splitParmValue (pszValue, &pszToken, &pszParm);
            if (!xppStricmp (pszToken, "nonce"))
               ad.szNonce = pszParm;

            else if (!xppStricmp (pszToken, "nc"))
               aud.szNC = pszParm;

            else if (!xppStricmp (pszToken, "cnonce"))
               aud.szCNonce = pszParm;

            else if (!xppStricmp (pszToken, "qop"))
               ad.szQop = pszParm;

            else if (!xppStricmp (pszToken, "opaque"))
               ad.szOpaque = pszParm;

            else if (!xppStricmp (pszToken, "username"))
               aud.szUser = pszParm;

            else if (!xppStricmp (pszToken, "realm"))
               aud.szRealm = ad.szRealm = pszParm;

            else if (!xppStricmp (pszToken, "stale") && !xppStricmp (pszParm, "true"))
               ad.bStale = true;

            else if (!xppStricmp (pszToken, "response"))
               pszDigest = pszParm;
            }
         authSetUserData (auth, dest, &aud);
         authSetAuthenticationData (auth, dest, &ad);
         authSetDigest (auth, dest, pszDigest);
         }
      }
   return;
   }


void evaluateHdrAuthenticationInfo (HttpBufferPtr_t p,
                                    StringBuffer_t pszValue,
                                    HttpAuthenticationPtr_t auth,
                                    HttpAuthenticationDest_t dest) // will be overwritten!
   {
   HttpAuthenticationInfo_t ai;
   CString_t pszToken;
   CString_t pszParm;
   xppMemset (&ai, 0, sizeof (HttpAuthenticationInfo_t));
   ai.cbSize = sizeof (HttpAuthenticationInfo_t);

   if (p->fOpenMode != MODE_HTTP_SERVER)
      {
      /*************************************************************/
      /* Find ount if the Authentication info block belongs to the */
      /* server authentication or to the proxy authentication      */
      /*************************************************************/

      if (p->iHttpStatus == 407)
         dest = DEST_PROXY;
      else
         dest = DEST_SERVER;


      while (pszValue != NULL)
         {
         pszValue = splitParmValue (pszValue, &pszToken, &pszParm);
         if (!xppStricmp (pszToken, "nextnonce"))
            ai.szNonce = pszParm;

         else if (!xppStricmp (pszToken, "nc"))
            ai.szNC = pszParm;

         else if (!xppStricmp (pszToken, "qop"))
            ai.szQop = pszParm;
         }
      authSetAuthenticationInfo (auth, dest, &ai);
      }
   }


void evaluateHdrWWWAuthenticateInfo (HttpBufferPtr_t p,
                                     StringBuffer_t pszValue,
                                     HttpAuthenticationPtr_t auth,
                                     HttpAuthenticationDest_t dest)
   {
   CString_t pszToken = NULL;
   CString_t pszParm = NULL;
   // CString_t pszNonce = NULL;
   // CString_t pszOpaque = NULL;
   // CString_t pszRealm = NULL;
   // CString_t pszDomain = NULL;
   // CString_t pszQop = NULL;

   HttpAuthenticationType_t typ;
   HttpAuthenticationData_t ad;
   xppMemset (&ad, 0, sizeof (HttpAuthenticationData_t));
   ad.cbSize =  sizeof (HttpAuthenticationData_t);
   if (p->fOpenMode != MODE_HTTP_SERVER)
      {
      /*********************************/
      /* Parse the authentication type */
      /*********************************/
      pszValue = splitToken (pszValue, &pszToken);
      if (!xppStricmp (pszToken, "Basic"))
         typ = AUTH_BASIC;

      else if (!xppStricmp (pszToken, "Digest") )
         typ = AUTH_DIGEST;

      else return; // unsupported authentication type:
                   // the HTTP error 401 will be returned to the application

      /*******************************************/
      /* Get the other authentication parameters */
      /*******************************************/
      while (pszValue != NULL)
         {
         pszValue = splitParmValue (pszValue, &pszToken, &pszParm);

         if (!xppStricmp (pszToken, "qop"))
            {
            /********************************************/
            /* Check if one of the supported algorithms */
            /* is returned in the QOP parameter buffer. */
            /********************************************/
            unsigned int uQop = 0;
            char * pszAlgorithms = pszValue;
            while (pszAlgorithms != NULL)
               {
               pszAlgorithms = splitToken (pszAlgorithms, &pszToken);
               if (!xppStricmp (pszToken, "auth-int"))
                  uQop |= 2;
               if (!xppStricmp (pszToken, "auth"))
                  uQop |= 1;
               }

            if (uQop & 1) ad.szQop = "auth";
            else if (uQop & 2) ad.szQop = "auth-int";
            }
         if (!xppStricmp (pszToken, "realm"))
            ad.szRealm = pszParm;

         if (!xppStricmp (pszToken, "nonce"))
            ad.szNonce = pszParm;

         if (!xppStricmp (pszToken, "opaque"))
            ad.szOpaque = pszParm;

         if (!xppStricmp (pszToken, "stale") && !xppStricmp (pszValue, "true"))
            ad.bStale = true;
         else
            ad.bStale = false;

         if (!xppStricmp (pszToken, "domain"))
            ad.szDomain = pszParm;
         }
      authSetType (auth, dest, typ);
      authSetAuthenticationData (auth, dest, &ad);
      }
   }

#define EVALUATE_FIELD(f,n,p,t,v) if (!xppStricmp ((t),(f))) {(n)((p),(v)); }
#define EVALUATE_AUTH_FIELD(f,n,p,t,v,a,d) if (!xppStricmp ((t),(f))&&((a)!=NULL)) {(n)((p),(v),(a),(d));}




/*************************************************************************/
/* Function: Analyze one line from the HTTP header block, and update     */
/* the HTTP object store if something interesting was found.             */
/* In case of an empty string, the function returns true:                */
/* An empty string indicates the end of the HTTP header block            */
/*************************************************************************/

Bool_t evaluateHttpHeaderLine (HttpBufferPtr_t p,           // i: instance object
                               StringBuffer_t pszLine, // i: one HTTP header line
                               HttpAuthenticationPtr_t auth) // i/o: authorization info
   {
   Bool_t bFinished = false;
   CString_t pszToken = NULL;

   /*******************************************************/
   /* An empty line indicates the end of the HTTP header. */
   /*******************************************************/
   if (pszLine == NULL)
      return true;        // this should never occur.

   if  (pszLine [0] == '\0')
      {
      if (    (p->fOpenMode == MODE_HTTP_SERVER)
           || (p->bEox == false)
           || (p->iHttpStatus == 204) || (p->iHttpStatus == 205) || (p->iHttpStatus == 304)
           || (p->iHttpStatus == 100) || (p->iHttpStatus == 101) )

         bFinished = true;               // Empty Line indicates end of HTTP header!
      }
   else
      {
      pszLine = splitToken (pszLine, &pszToken); // get the keyword of the HTTP header line
      if (!pszToken)
         {
         bFinished = true; // this could be an error!!!
         }
      else
         {
         /*********************************************************/
         /* The first line of the HTTP header is handled this way */
         /*********************************************************/
         if ((p->pchRequest == NULL) && (p->fOpenMode == MODE_HTTP_SERVER))
            {
            p->pchRequest = makeString (pszToken);
            pszLine = splitToken (pszLine, &pszToken);
            if (pszToken != NULL)
               {
               const char *pszURL = getResourceName (pszToken);
               /*********************/
               /* Get the URL name. */
               /*********************/
               p->pchURI = makeString (pszURL);
               }
            }

         else if ((p->iHttpStatus == HTTP_STATUS_UNDEFINED) &&
                  (p->fOpenMode != MODE_HTTP_SERVER))
            {

            while (pszLine [xppStrlen(pszLine)-1] == ' ')
               pszLine [xppStrlen(pszLine)-1] = '\0'; /* cut trailing blanks */
            p->iHttpStatus = xppAtoi (pszLine);
            }

         /******************************************************/
         /* Look for the keywords that are interesting for me: */
         /* Host - IP address of the host                      */
         /* Referer - name of the referenced document          */
         /* Content-Length - size of the document to receive   */
         /* Content-Type - Document mime type                  */
         /* Connection - Connection type                       */
         /* Authorization -client authorization data           */
         /* Authentication-Info - server authentication data   */
         /*                                                    */
         /* If Content-Length or Transfer-Encoding is speci-   */
         /* fied, we recognize that a document is attached to  */
         /* the HTTP header.                                   */
         /*                                                    */
         /* ignore the other keywords                          */
         /******************************************************/

         else
            {
            EVALUATE_FIELD ("Host:", evaluateHdrHost, p, pszToken, pszLine)
            EVALUATE_FIELD ("From:", evaluateHdrFrom, p, pszToken, pszLine)
            EVALUATE_FIELD ("Referer:", evaluateHdrReferer, p, pszToken, pszLine)
            EVALUATE_FIELD ("Content-Length:", evaluateHdrContentLength, p, pszToken, pszLine)
            EVALUATE_FIELD ("Content-Type:", evaluateHdrContentType, p, pszToken, pszLine)
            EVALUATE_FIELD ("Connection:", evaluateHdrConnection, p, pszToken, pszLine)
            EVALUATE_FIELD ("Transfer-Encoding:", evaluateHdrTransferEncoding, p, pszToken, pszLine)
			EVALUATE_FIELD ("x-syncml-hmac:", evaluateHdrXSyncmlHmac, p, pszToken, pszLine)
            EVALUATE_AUTH_FIELD ("Authorization:", evaluateHdrAuthorization, p, pszToken, pszLine, auth, DEST_SERVER)
            EVALUATE_AUTH_FIELD ("Authentication-Info:", evaluateHdrAuthenticationInfo, p, pszToken, pszLine, auth, DEST_NONE)
            EVALUATE_AUTH_FIELD ("WWW-Authenticate:", evaluateHdrWWWAuthenticateInfo, p, pszToken, pszLine, auth, DEST_SERVER)
            EVALUATE_AUTH_FIELD ("Proxy-Authenticate:", evaluateHdrWWWAuthenticateInfo, p, pszToken, pszLine, auth, DEST_PROXY)
            }
         }
      }
   return bFinished;
   }




/***********************************************************************************/
/* Function: Some data have been received to the transfer buffer before, and it is */
/* assumed that the data contain HTTP header information. Go and analyze the block */
/* The function returns true if the end of the HTTP header has been detected.      */
/* Those parts of the incoming data that could not be processed remain in the      */
/* transfer buffer.                                                                */
/***********************************************************************************/

Bool_t evaluateHttpHeaderBlock (HttpBufferPtr_t p,            // i: instance object
                                HttpAuthenticationPtr_t auth) // i/o: authorization info
   {
   Bool_t bHeaderCompleted = false;         // end of HTTP header detected?
   Ptr_t pszEnd = (Ptr_t )(p->pbCache + p->cbCacheUsed); // end mark
   StringBuffer_t pszCurrent = (StringBuffer_t) p->pbCache;
   StringBuffer_t pszLine;

   /***********************************************/
   /* Loop thru the data in the transfer buffer,  */
   /* and parse the HTTP header data line-by-line */
   /***********************************************/
   p->pbCache [p->cbCacheUsed] = '\0';
   while ((pszCurrent != NULL) && (pszCurrent < (StringBuffer_t) pszEnd) && (bHeaderCompleted == false))
      {
      pszLine = pszCurrent;
      pszCurrent = splitLine (pszCurrent);
      if (pszCurrent == NULL)
         {
         /*********************************************/
         /* We are still waiting for additional data. */
         /* We must read the next block!              */
         /*********************************************/
         p->cbCacheUsed -= (BufferSize_t) (pszLine - (StringBuffer_t) p->pbCache);
         if (p->cbCacheUsed > 0)
            {
            xppMemmove (p->pbCache, pszLine, p->cbCacheUsed);
            p->pbCache [p->cbCacheUsed] = '\0';
            }
         }

      else if (evaluateHttpHeaderLine (p, pszLine, auth))
        {
          /* T.K. Microsoft IIS 4/5 hack                                          */
          /* IIS has the behavior of sending HTTP/1.1 100 Continue just           */
          /* before sending the HTTP/1.1 200 OK - so we 'peek' in the remaining   */
          /* buffer and see wether we are really finished or wether we need to go */
          /* further. */
          
          /* %%% luz 2002-05-23: this is the wrong place for that, as IIS might
                 choose to delay that "200 OK" header a little. We need to
                 continue reading (in readHttpHeader()) if status was "100 Continue"
                 and check if following data is "HTTP/1.1" again
          
		      int dummyInt = xppStrncmp(pszCurrent, "HTTP/1.1",8);
          if (dummyInt == 0) {
            p->iHttpStatus = HTTP_STATUS_UNDEFINED; // reset the unused status
          } else
          */

          {
            /****************************************************************************/
            /* The header finished here. Copy the non-processed data (this is the first */
            /* block of the data section) to the beginning of the cache, then return to */
            /* the caller                                                               */
            /****************************************************************************/
            p->cbCacheUsed = (BufferSize_t) (pszEnd - pszCurrent);

            if (p->cbCacheUsed > 0)
              xppMemmove (p->pbCache, pszCurrent, p->cbCacheUsed);
            bHeaderCompleted = true;
          }
        }
      }
   return bHeaderCompleted;
   }


#if SYDEBUG>1
// %%% luz
void DebugPrintf(const char *text, ...);
#endif

/****************************************************************************/
/* Function: read the HTTP header                                           */
/* The function waits for incoming data and processes the HTTP header data. */
/* When the function returns, the transfer buffer may already contain the   */
/* first bytes of the document!                                             */
/****************************************************************************/

TcpRc_t readHttpHeader (HttpBufferPtr_t p, // i: instance object
                        HttpAuthenticationPtr_t auth) // i/o: authorization info
   {
   BufferSize_t uBytesTransmitted;
   Bool_t bHeaderProcessed = false;
   /* %%% luz:2002-05-23: several changes (iispeek), see comments below */
   Bool_t iispeek;

   /****************/
   /* Set defaults */
   /****************/
// p->fEncoding = FLAT;
   p->fTransferCoding = NOT_CHUNKED;
   p->fTransferMode = MODE_READING;
   p->bEox = true;
   p->cbDataLength = UNDEFINED_CONTENT_LENGTH;

   freeString(p->sXSyncmlHmac.pchHmac);
   p->sXSyncmlHmac.pchHmac = NULL;
   p->sXSyncmlHmac.hmacInfo.mac = NULL;
   p->sXSyncmlHmac.hmacInfo.username = NULL;
   p->sXSyncmlHmac.hmacInfo.algorithm = NULL;

   /*****************************************************************/
   /* Go and receive all incoming TCP/IP packets                    */
   /* until the HTTP header information has been parsed completely. */
   /*****************************************************************/

   /* %%% luz:2002-05-23: several changes (iispeek), see comment below */
   iispeek=false; // no IIS peek done yet

   while ((bHeaderProcessed == false) && (p->iRc == TCP_RC_OK))
   {      
      /* get the next block of data */
      uBytesTransmitted = (BufferSize_t)(sizeof (p->pbCache) - p->cbCacheUsed);
      p->iRc = tcpReadData (p->pSession, p->pbCache+p->cbCacheUsed, &uBytesTransmitted);
      if (p->iRc == TCP_RC_OK) {
        p->cbCacheUsed += uBytesTransmitted;
        do {        
          // first check if we must peek for IIS before checking header
          if (iispeek) {
            // we have already processed a "100 continue" header.
            // - check if we have at least 7 chars to check if another header is following
            if (p->cbCacheUsed>7) {
              iispeek=false; // we can check now, peek ends here
              // NOTE: under some strange circumstances (special Auth software on IIS
              //       which somehow intervenes normal IIS response, we can have the 100 Continue
              //       followed by a HTTP 1.0 (!!). So we only check for "HTTP/1." here.
              if (xppStrncmp((StringBuffer_t)p->pbCache, "HTTP/1.",7)==0) {
                // - yes, parse it as if it was the first one
                #if SYDEBUG>1 && !defined(__PALM_OS__)
                // PalmOS has no %0.80s type formatting
                DebugPrintf("########### p->cbCacheUsed=%ld, p->pbCache[0..79]='%0.80s'",p->cbCacheUsed,p->pbCache);
                #endif
                p->iHttpStatus = HTTP_STATUS_UNDEFINED; // reset the unused status
              }
              else {
                // - no, this is NOT IIS 100 continue special case
                bHeaderProcessed=true; // we are done now
                break; // done with header (and some bytes in the cache)
              }
            }
            else {
              // else: iispeek remains pending, we need more data first
              break;
            }
          }
          // process header only if no iispeek is pending any more
          if (!iispeek) {
            bHeaderProcessed = evaluateHttpHeaderBlock (p, auth);
            /* T.K. Microsoft IIS 4/5 hack */
            /* %%% luz:2002-05-23: modified to work, moved here from
                   evaluateHttpHeaderBlock().
               IIS has the behavior of sending HTTP/1.1 100 Continue just
               before sending the HTTP/1.1 200 OK - so we continue reading after
               status==100 and see wether we are really finished or wether we need
               to go further. */


			// LEO:
#ifdef _DEBUG
			{
				char buff[256];
				sprintf(buff, "SYNCML HTTP STATUS: %d.\n", (int)p->iHttpStatus);
				OutputDebugString(buff);
			}
#endif



            if (bHeaderProcessed && p->iHttpStatus==100) {
              iispeek=true; // we need to do IIS peek
              bHeaderProcessed=false; // do not stop yet
            }
          } // if not peek pending
        } while(iispeek);
      } // if TCP read ok
      else {
        // %%% luz:2002-11-11 added to allow setting breakpoint on error
        break;
      }
   } // while

   /*************************************************************************/
   /* Dependent on the HTTP header settings,                                */
   /* the instance variable DataToRead must be set:                         */
   /* if CHUNKED transfer coding is selected, the variable keeps the number */
   /* of bytes that have not been for the current chunk, if NON_CHUNKED     */
   /* transfer coding is selected, the number of bytes of the HTTP document */
   /* itself is returned.                                                   */
   /*************************************************************************/
   if (p->iRc == TCP_RC_OK)
      {
      if (p->fTransferCoding == NOT_CHUNKED)
         p->cbDataToRead = p->cbDataLength;
      else
         p->cbDataToRead = 0L;
      }

   return p->iRc;
   }


HttpRc_t continueHttpRequest (HttpBufferPtr_t p,
                              HttpAuthenticationPtr_t auth) // i: ptr to authorization info
   {
   HttpRc_t rc = HTTP_RC_OK;

   p->cbDataLength = 0;
   writeRequestHeader (p, auth);
   p->iRc = tcpSendData (p->pSession, p->pbCache, p->cbCacheUsed);
   if (p->iRc != TCP_RC_OK)
      rc = HTTP_RC_COMMUNICATION;
   return rc;
   }


/********************************************************************/
/* Function: Create an instance of a HTTP object for a client       */
/* The HTTP doc type is denoted by pszMode.                         */
/********************************************************************/

HttpRc_t openClient (HttpBufferPtr_t p,    // io: allocated instance object
                     SocketPtr_t pSession, // i: ptr to created TCP/IP socket
                     CString_t pszMode,    // i: Type of HTTP document
                     HttpDocumentContextPtr_t settings, // i: document properties
                     HttpAuthenticationPtr_t auth) // i: ptr to authorization info
   {
   HttpRc_t rc = HTTP_RC_OK;

   p->pSession = pSession;
   /*****************************************************************/
   /* Split the host URL name into a Host name part and an URI part */
   /*****************************************************************/

   if (settings != NULL)
      {
      const char *pszAsterisk = "*", *pszEmpty = "";
      if ((settings->pszURL != NULL) && (settings->pszURL [0] != '\0'))
         p->pchURI  = makeString (settings->pszURL);
      else
         p->pchURI  = makeString (pszAsterisk);

      if (settings->pszHost != NULL)
         p->pchHost = makeString (settings->pszHost);
      else
         p->pchHost = makeString (pszEmpty);

      if (settings->pszProxy != NULL)
         p->pchProxy = makeString (settings->pszProxy);
      else
         p->pchProxy = makeString (pszEmpty);

      p->pchRequestType = makeString (settings->pszType);
      p->pchReferer = makeString (settings->pszReferer);
      p->pchFrom = makeString (settings->pszFrom);
      // %%%luz 2002-08-28: second argument p was missing here
	    p->sXSyncmlHmac.pchHmac = makeHmacString(settings->pXSyncmlHmac,p);
      }

   p->cbDataLength = settings->cbLength;
   p->pchRequest = makeString (pszMode);
   p->fConnection = HTTP_DEFAULT_CONNECTION_TYPE;

   /*************************************/
   /* Setup and Process the HTTP Header */
   /*************************************/

   if (!xppStrcmp (pszMode, "RECEIVE"))
      p->fOpenMode = MODE_HTTP_GET;
   else if (!xppStrcmp (pszMode, "EXCHANGE"))
      p->fOpenMode = MODE_HTTP_POST;
   else if (!xppStrcmp (pszMode, "SEND"))
      p->fOpenMode = MODE_HTTP_PUT;
   else
      rc = HTTP_RC_PARAMETER;

   if (rc == HTTP_RC_OK)
      writeRequestHeader (p, auth);

   /************************************/
   /* Send the HTTP header to the host */
   /************************************/
   if (rc == HTTP_RC_OK)
      {
      p->iRc = tcpSendData (p->pSession, p->pbCache, p->cbCacheUsed);
      if (p->iRc != TCP_RC_OK)
         {
         /*****************************************************/
         /* Probably, the TCP/IP connection is lost.          */
         /* Free all resources that have been allocated here. */
         /* The caller can re-establish the TCP/IP connection */
         /* and invoke the httpOpen() function again.         */
         /*****************************************************/

         rc = HTTP_RC_COMMUNICATION;

         freeString (p->pchRequest);
         freeString (p->pchURI);
         freeString (p->pchHost);
         freeString (p->pchProxy);
         freeString (p->pchRequestType);
         freeString (p->pchReferer);
         freeString (p->pchFrom);
		 freeString (p->sXSyncmlHmac.pchHmac);
         p->pchRequest = NULL;
         p->pchURI     = NULL;
         p->pchHost    = NULL;
         p->pchProxy   = NULL;
         p->pchRequestType = NULL;
         p->pchReferer     = NULL;
         p->pchFrom        = NULL;
		 p->sXSyncmlHmac.pchHmac = NULL;
		 p->sXSyncmlHmac.hmacInfo.mac = NULL;
		 p->sXSyncmlHmac.hmacInfo.username = NULL;
		 p->sXSyncmlHmac.hmacInfo.algorithm = NULL;

         #ifdef STATIC_MEMORY_MANAGEMENT
         p->cbStringHeapUsed = 0;
         #endif
         }
      p->cbCacheUsed = 0;
      }

   return rc;
   }




/**********************************************************************/
/* Function: Create an instance of a HTTP object for a HTTP server    */
/* Read the HTTP header information to find out what the client wants */
/* Note that pSession must point to a connection socket, that means,  */
/* there is already a client that connected to the server!            */
/**********************************************************************/

HttpRc_t openServer (HttpBufferPtr_t p, // i: allocated instance object
                     SocketPtr_t  pSession, // i:ptr to open connection socket
                     HttpDocumentContextPtr_t settings, //o: document settings
                     HttpAuthenticationPtr_t auth) // o: ptr to authorization info
   {
   HttpRc_t rc = HTTP_RC_OK;
   TcpRc_t iTcpRc;

   p->pSession = pSession;
   p->fOpenMode = MODE_HTTP_SERVER;
   p->fTransferCoding = CHUNKED;
   p->fConnection = HTTP_DEFAULT_CONNECTION_TYPE;

   /************************************/
   /* Process the complete HTTP header */
   /************************************/

   iTcpRc = readHttpHeader (p, auth);
   if (iTcpRc == TCP_RC_EOF)
      rc = HTTP_RC_EOF; // client closed the TCP/IP connection

   else if (iTcpRc != TCP_RC_OK)
      rc = HTTP_RC_COMMUNICATION; // TCP/IP communication error

   /*********************************************************/
   /* The HTTP header has been processed. Inform the caller */
   /* about the client request.                             */
   /*********************************************************/
   if (rc == HTTP_RC_OK)
      {
      settings->pszURL = p->pchURI;
      settings->pszHost = p->pchHost;
      settings->pszType = p->pchResponseType;
      settings->cbLength = p->cbDataLength;
      settings->pszReferer = p->pchReferer;
      settings->pszFrom = p->pchFrom;
      settings->pszRequest = p->pchRequest ? p->pchRequest : "undefined";
	  settings->pXSyncmlHmac = &(p->sXSyncmlHmac.hmacInfo);
      }
   return rc;
   }


/***********************************************************************/
/*                                                                     */
/*                Special data chunking functions                      */
/*                                                                     */
/***********************************************************************/


/**************************************************************************/
/* Function: send the last data chunk header to the communication partner */
/* The function is called if a Transfer-Encoding  mode is selected.       */
/* The document has been sent to the communication partner, now  we must  */
/* send a terminating chunk header ("0" | CRLF | CRLF") to inform the     */
/* receiver that the transmission completed.                              */
/* Returns: HTTP_RC_OK - transmission succeedded,                         */
/*          others     - a data transmission error occurred.              */
/**************************************************************************/
HttpRc_t writeLastDataChunk (HttpBufferPtr_t p)
   {
   HttpRc_t rc;
   const char *pszLastChunk = "0\r\n\r\n";
   p->iRc = tcpSendData (p->pSession, (DataBuffer_t) pszLastChunk,
                         (BufferSize_t) xppStrlen (pszLastChunk));
   if (p->iRc != 0)
      rc = HTTP_RC_COMMUNICATION;
   else
      rc = HTTP_RC_OK;

   return rc;
   }

/**********************************************************************/
/* Function: This is an extended atol () function. An additional base */
/* parameter specifies the base of the ascii-long transformation      */
/**********************************************************************/
unsigned long ascii2hex (const char *szAscii)
   {
   unsigned long rc = 0L;
   int iDigit;

   while ((szAscii [0] != '\0'))
      {
      if ((szAscii [0]>= '0')&&(szAscii [0]<= '9'))
         iDigit = (int)(szAscii [0] - '0');
      else if ((szAscii [0]>= 'A')&&(szAscii [0]<= 'Z'))
         iDigit = (int)(szAscii [0] - 'A')+10;
      else if ((szAscii [0]>= 'a')&&(szAscii [0]<= 'z'))
         iDigit = (int)(szAscii [0] - 'a')+10;
      else
         break;
      rc = 16 * rc + iDigit;
      szAscii ++;
      }
   return rc;
   }



/***************************************************************************/
/* Function: The function processed the data in the buffer cache that have */
/* been received from the communication partner as a chunk header.         */
/* The chunk header has the format "<l> [parms] CRLF" (<l> denotes the size*/
/* the data chunk that follows), or "0 CRLF CRLF" (the last chunk has been */
/* received). [parms] will be ignored.                                     */
/* The function parses the chunk size and sets the instance variable       */
/* cbDataToRead accordingly.                                               */
/* returns: 'false', if the chunk header has been parsed, and the variables*/
/* are updated, 'true' if the data in the cache is an incomplete chunk     */
/* header                                                                  */
/***************************************************************************/

Bool_t parseChunkHeader (HttpBufferPtr_t p)
   {
   Bool_t rc = true;
   unsigned int n;
   int m = 0;

   /******************************************/
   /* With the exception of the first chunk, */
   /* we must skip leading CRLF characters   */
   /******************************************/
   if ((p->pbCache [0] == '\r') && (p->pbCache [1] == '\n'))
      m = 2;

   /********************************/
   /* Check for the CRLF character */
   /********************************/
   for (n = m; (n < (p->cbCacheUsed-1)); n ++)
      {
      if ((p->pbCache [n] == '\r') && (p->pbCache [n+1] == '\n'))
         {
         rc = false;
         break;
         }
      }

   if (rc == false)
      {
      /************************************************/
      /* extract the first token of the chunk header: */
      /* this is the length of the chunk that follows */
      /************************************************/
      char *szChunkLength = (char *)p->pbCache+m;
      char *ptr;
      char chSave;
      while (szChunkLength [0] == ' ') szChunkLength ++;
      ptr = szChunkLength;
      while ((ptr [0] != ' ') &&
             (ptr [0] != '\0') &&
             (ptr [0] != '\r') &&
             (ptr [0] != ';')) ptr ++;
      chSave = *ptr;
      *ptr = '\0';

      p->cbDataToRead = (BufferSize_t) ascii2hex (szChunkLength);

      /**************************************************************/
      /* Special handling for the last chunk: after the last chunk, */
      /* (A chunk header with a chunk size of '0') there must be a  */
      /* second CRLF character sequence                             */
      /**************************************************************/
      if (p->cbDataToRead == 0L)
         {
         if ((p->cbCacheUsed < n + 4) ||
             (p->pbCache [n+2] != '\r') || (p->pbCache [n+3] != '\n'))
            {
            /***********************************************/
            /* not complete:                               */
            /* We wait for the last empty CRLF characters! */
            /***********************************************/
            *ptr = chSave;
            rc = true;
            }
         else
            n += 2;
         }

      /******************************************/
      /* Remove the chunk header from the cache */
      /******************************************/
      if (rc == false)
         {
         p->cbCacheUsed -= (n+2);
         if (p->cbCacheUsed > 0)
            xppMemmove (p->pbCache, p->pbCache + n + 2, p->cbCacheUsed);
         }
      }
   return rc;
   }


/**************************************************/
/*   Data encoding/decoding functions             */
/**************************************************/


BufferSize_t copyBlock (DataBuffer_t pbTarget,       // o: target buffer
                        BufferSize_t cbTargetSize,   // i: target buffer size
                        DataBuffer_t pbData,         // i: Data buffer
                        BufferSize_t *pcbDataLength) // i/o: non-processed Data size
   {
   BufferSize_t cbCopySize;

   cbCopySize = min (*pcbDataLength, cbTargetSize);
   xppMemcpy (pbTarget, pbData, cbCopySize);

   /********************************/
   /* Save the non-processed data. */
   /********************************/

   if (*pcbDataLength > cbCopySize)
      xppMemmove (pbData, pbData + cbCopySize, (*pcbDataLength)-cbCopySize);
   (*pcbDataLength) -= cbCopySize;

   return cbCopySize;
   }


BufferSize_t decodeBlock (DataBuffer_t pbTarget,       // o: target buffer
                          BufferSize_t cbTargetSize,   // i: target buffer size
                          DataBuffer_t pbData,         // i: data buffer
                          BufferSize_t *pcbDataLength) // i/o: Data buffer size
   {
   BufferSize_t cbCopySize;

   cbCopySize = min (*pcbDataLength, cbTargetSize);
   xppMemcpy (pbTarget, pbData, cbCopySize);

   if (*pcbDataLength > cbCopySize)
      xppMemmove (pbData, pbData + cbCopySize, (*pcbDataLength)-cbCopySize);
   (*pcbDataLength) -= cbCopySize;

   return cbCopySize;
   }



/***********************************************/
/* Function: encode and make a data chunk      */
/* returns false if no more data are to encode */
/***********************************************/

Bool_t encodeData (HttpBufferPtr_t p,
                   DataBuffer_t pbData,
                   BufferSize_t *pcbDataSize,
                   Bool_t bFinal)
   {
   BufferSize_t cbChunkSize;
   BufferSize_t cbBytesCopied = 0;
   BufferSize_t cbAvailableCacheSize;
   DataBuffer_t pbChunkHeader = p->pbCache + p->cbCacheUsed;

   /* First check if there are data to transmit at all. */
   if (*pcbDataSize == 0L)
      return false;           // no more data to transmit.


   if (p->fTransferCoding == CHUNKED)
      {
      /*******************************************/
      /* reserve some space for the chunk header */
      /*******************************************/

      xppMemset (pbChunkHeader, ' ', CHUNK_HEADER_SIZE-2);
      xppMemcpy (pbChunkHeader + CHUNK_HEADER_SIZE-2, "\r\n", 2);
      p->cbCacheUsed += CHUNK_HEADER_SIZE;
      cbAvailableCacheSize = p->cbCacheSize - p->cbCacheUsed - 2;
      }
   else
      {
      cbAvailableCacheSize = p->cbCacheSize - p->cbCacheUsed;
      }

   /************************************************************/
   /* encode the data and copy it to the transfer cache buffer */
   /* move the source data pointer to the non-processed rest   */
   /************************************************************/

   cbChunkSize = *pcbDataSize;
// switch (p->fEncoding)
//    {
//    case FLAT:
         cbBytesCopied = copyBlock (p->pbCache + p->cbCacheUsed,  // target buf
                                    cbAvailableCacheSize,         // bufsize
                                    pbData,                      // source data
                                    pcbDataSize);          // nonprocessed data
//       break;
//
//    case BASE64:
//       cbBytesCopied = base64Encode (p->pbCache + p->cbCacheUsed,
//                                     cbAvailableCacheSize,
//                                     pbData,
//                                     pcbDataSize,
//                                     &p->cbDocumentOffset,
//                                     (bFinal == true) ? 1 : 0, p->abBase64Data);
//       break;
//    }

   cbChunkSize = cbBytesCopied;
   p->cbCacheUsed += cbBytesCopied;


   /****************************************************************************/
   /* a block of data has been encoded. At this point of time, we received the */
   /* actual size of the data block. We can update the chunk header now.       */
   /****************************************************************************/

   if (p->fTransferCoding == CHUNKED)
      {
      char achTemp [12];
      char *ptr = achTemp;
      ltoa (cbChunkSize, achTemp, 16);
      while ((ptr [0] == '0') && (ptr [1] != '\0')) ptr ++; // remove leading 0's
      xppMemcpy (pbChunkHeader, ptr, xppStrlen (ptr));

      /********************************/
      /* append a CRLF character pair */
      /********************************/
      p->pbCache [p->cbCacheUsed++] = '\r';
      p->pbCache [p->cbCacheUsed++] = '\n';
      }

   return true;
   }


/******************************************************************/
/* Function: Remove the chunk headers and decode the data block   */
/* returns: true,  All expected data have been received!          */
/*          false, The cache has been processed, but further data */
/*                 are expected.                                  */
/******************************************************************/

Bool_t decodeData (HttpBufferPtr_t p,
                   DataBuffer_t pbData,
                   BufferSize_t *pcbDataSize)
   {
   Bool_t rc = false;
   Bool_t bComplete = false;

   /**********************************************/
   /* One IP block can contain zero, one or more */
   /* sequences of chunks and chunk headers.     */
   /**********************************************/

   while ((rc == false) && (bComplete == false))
      {
      if (p->cbDataToRead == 0)
         {
         /************************************************************/
         /* If non-chunked transfer coding is used, we are done now. */
         /* Otherwise, the next chunk header must be processed.      */
         /************************************************************/
         if (p->fTransferCoding == CHUNKED)
            bComplete = parseChunkHeader (p);
         /*******************************************************/
         /* if a chunk header with the chunk size of 0 has been */
         /* received, the document has been entirely received.  */
         /*******************************************************/

         if ((bComplete == false) && (p->cbDataToRead == 0))
            rc = true;
         }
      else
         {
         BufferSize_t cbBytesCopied = 0;
         BufferSize_t cbBytes = p->cbCacheUsed;

//       switch (p->fEncoding)
//          {
//          case FLAT:
               cbBytesCopied = decodeBlock (pbData,
                                            min (*pcbDataSize, p->cbDataToRead),
                                            p->pbCache,
                                            &p->cbCacheUsed);
//             break;
//
//          case BASE64:
//
//             if (p->fTransferCoding == CHUNKED)
//                {
//                cbDataToCopy = min (p->cbCacheUsed, p->cbDataToRead);
//                cbSourceDataProcessed = cbDataToCopy;
//                cbBytesCopied = base64Decode (pbData, *pcbDataSize, p->pbCache, &cbSourceDataProcessed);
//                if (cbDataToCopy != p->cbCacheUsed)
//                   {
//                   xppMemmove (p->pbCache + cbSourceDataProcessed,
//                            p->pbCache + cbDataToCopy,
//                            p->cbCacheUsed - cbDataToCopy);
//                   }
//                p->cbCacheUsed -= (cbDataToCopy - cbSourceDataProcessed);
//                }
//             else
//                {
//                cbDataToCopy = min (*pcbDataSize, p->cbDataToRead);
//                cbSourceDataProcessed = cbDataToCopy;
//                cbBytesCopied = base64Decode (pbData, cbDataToCopy, p->pbCache, &p->cbCacheUsed);
//                }
//
//             break;
//          }
         *pcbDataSize -= cbBytesCopied;
         pbData += cbBytesCopied;

         /******************************************/
         /* Check if we decoded the complete chunk */
         /******************************************/
         if (p->fTransferCoding == CHUNKED)
            p->cbDataToRead -= (cbBytes - p->cbCacheUsed);
         else
            p->cbDataToRead -= cbBytesCopied;

         if ((p->fTransferCoding != CHUNKED) && (p->cbDataToRead == 0))
            rc = true;
         else if ((p->cbCacheUsed == 0) || (*pcbDataSize == 0))

            bComplete = true;
         }
      }

   return rc;
   }





/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         API functions                                   **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/




/*****************************************************************************/
/*                                                                           */
/*           API Function: Start the transmission of a document              */
/*                                                                           */
/*****************************************************************************/

HttpRc_t httpOpen (HttpHandle_t handle,                // i: instance handle
                   SocketPtr_t pSession,               // i: TCP/IP socket
                   CString_t pszMode,                  // i: transmission mode
                   HttpDocumentContextPtr_t settings,  // io: Document context
                   HttpAuthenticationPtr_t auth)       // io: server authorization info
   {
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;

   if ((p == NULL) ||
       (settings == NULL)||
       (pSession == NULL) ||
       (pszMode == NULL))
      {
      return HTTP_RC_PARAMETER;
      }

   /******************************/
   /* initialize instance object */
   /******************************/
   xppMemset (p, 0, sizeof (HttpBuffer_t));
   p->cbSize = sizeof (HttpBuffer_t);
   p->cbCacheSize = CACHE_BUFFER_SIZE;
   p->iHttpStatus = HTTP_STATUS_UNDEFINED;

   #ifdef STATIC_MEMORY_MANAGEMENT
   p->cbStringHeapSize = STATIC_MEMORY_SIZE;
   #endif

   if (!xppStrcmp (pszMode, "SERVER"))
      return openServer (p, pSession, settings, auth);
   else
      return openClient (p, pSession, pszMode, settings, auth);
   }



/*****************************************************************************/
/*                                                                           */
/*      API Function: Send a chunk of data to the communication partner      */
/*                                                                           */
/*****************************************************************************/

HttpRc_t httpWrite (HttpHandle_t handle,       // i: instance handle
                    DataBuffer_t pbBuffer,     // i: data buffer
                    BufferSize_t cbBufferSize, // i: count of bytes to transmit
                    Bool_t bFinal)             // i: final block?
   {
   HttpRc_t rc;
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;

   // LEO:
   BufferSize_t cbInitSize=cbBufferSize;

   if (!ISHEADERVALID (p))
      return HTTP_RC_PARAMETER;

   if (p->iRc != 0)
      return HTTP_RC_COMMUNICATION;

   if (p->fTransferMode == MODE_READING)
      rc = HTTP_RC_NOT_ALLOWED;
   else p->fTransferMode = MODE_WRITING;

   /************************************************************************/
   /* The input data must be encoded and chunked. This could lead to cases */
   /* where the data must be transmitted by multiple TCP/IP packets.       */
   /************************************************************************/

   // LEO:

   // original:
   //
   //while ( encodeData (p, pbBuffer, &cbBufferSize, bFinal)
   //        && (p->iRc == TCP_RC_OK))
   //   {
   //   p->iRc = tcpSendData (p->pSession, p->pbCache, p->cbCacheUsed);
   //   p->cbCacheUsed = 0;
   //   }



   __syncml_setSendPercent(0);

   while ( encodeData (p, pbBuffer, &cbBufferSize, bFinal)
           && (p->iRc == TCP_RC_OK))
      {
		  p->iRc = tcpSendData (p->pSession, p->pbCache, p->cbCacheUsed);
		  p->cbCacheUsed = 0;

		 __syncml_setSendPercent((cbInitSize-cbBufferSize)*100/cbInitSize);
      }

	__syncml_setSendPercent(100);

   if (p->iRc != 0)
      rc = HTTP_RC_COMMUNICATION;
   else
      rc = HTTP_RC_OK;

   return rc;
   }




/*****************************************************************************/
/*                                                                           */
/*      API Function: Close the HTTP connection                              */
/*                                                                           */
/*****************************************************************************/

HttpRc_t httpClose (HttpHandle_t handle)          // i: instance handle
   {
   HttpRc_t rc = HTTP_RC_OK;
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;
   TcpRc_t iTcpRc = TCP_RC_OK;
   if (!ISHEADERVALID (p))
      return HTTP_RC_PARAMETER;

   /************************************************************************/
   /* PUT request: write the last data chunk, then wait for the HTTP reply */
   /************************************************************************/
   if ((p->fOpenMode == MODE_HTTP_PUT) && (p->fTransferMode == MODE_WRITING))
      {
      if (p->fTransferCoding == CHUNKED) writeLastDataChunk (p);
      iTcpRc = readHttpHeader (p, NULL);
      if ((iTcpRc == TCP_RC_OK) && (p->iHttpStatus >= 400))
         rc = HTTP_RC_HTTP;          // HTTP error
      }

   /******************************************/
   /* Server mode: write the last data chunk */
   /******************************************/
   else if ((p->fOpenMode == MODE_HTTP_SERVER) &&
            (p->fTransferMode == MODE_WRITING) &&
            (p->fTransferCoding == CHUNKED) )

      iTcpRc = writeLastDataChunk (p);


   if (iTcpRc == TCP_RC_EOF)     rc = HTTP_RC_EOF;  // client closed the connection
   else if (iTcpRc != TCP_RC_OK) rc = HTTP_RC_COMMUNICATION; // communication error

   freeString (p->pchRequest);
   freeString (p->pchURI);
   freeString (p->pchHost);
   freeString (p->pchProxy);
   freeString (p->pchRequestType);
   freeString (p->pchResponseType);
   freeString (p->pchReferer);
   freeString (p->sXSyncmlHmac.pchHmac);

   p->cbSize = 0;
   return rc;
   }





/*****************************************************************************/
/*                                                                           */
/*      API Function: Read a block of the HTTP document                      */
/*                                                                           */
/*****************************************************************************/


HttpRc_t httpRead (HttpHandle_t handle,           // i: instance handle
                   DataBuffer_t pbDataBuffer,     // io: pointer to data buffer
                   BufferSize_t cbDataBufferSize, // i: size of data buffer
                   BufferSizePtr_t pcbDataRead)   // io: Number of bytes read
   {
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;
   HttpRc_t rc = HTTP_RC_OK;
   Bool_t bBufferTrunctation = false;

   if (!ISHEADERVALID (p) || pbDataBuffer == NULL || pcbDataRead == NULL)
      return HTTP_RC_PARAMETER;

   /***************************/
   /* Check if we expect data */
   /***************************/
   if (p->fTransferMode == MODE_WRITING)
      rc = HTTP_RC_NOT_ALLOWED;
   else p->fTransferMode = MODE_READING;

   if (rc == HTTP_RC_OK)
      {
      *pcbDataRead = 0;
      /**************************************************/
      /* Process the data that remain in the cache,     */
      /* and check if there are further data to receive */
      /**************************************************/
      if ((p->bEox == false) && (p->cbCacheUsed > 0))
         {
         BufferSize_t cbDataProcessed = cbDataBufferSize;
         p->bEox = decodeData (p, pbDataBuffer, &cbDataProcessed);

         *pcbDataRead = cbDataBufferSize - cbDataProcessed;
         if (cbDataProcessed == 0) bBufferTrunctation = true;
         }

      /**************************************************************************/
      /* if there are further data to process, fill up the receive buffer cache */
      /**************************************************************************/
      if ((p->bEox == false) && (bBufferTrunctation == false))
         {
         BufferSize_t cbBytesTransmitted = (BufferSize_t) p->cbCacheSize - p->cbCacheUsed;

         p->iRc = tcpReadData (p->pSession,
                               p->pbCache+p->cbCacheUsed,
                               &cbBytesTransmitted);

         /******************************************/
         /* Check if the server closed the socket. */
         /* In some cases, this is not an error!   */
         /******************************************/

         if ((p->iRc == TCP_RC_EOF) && (p->cbDataLength == UNDEFINED_CONTENT_LENGTH))
            {
            p->iRc = TCP_RC_OK;
            p->bEox = true;
            }

         if (p->iRc != TCP_RC_OK)
            rc = HTTP_RC_COMMUNICATION;
         else
            {
            p->cbCacheUsed += cbBytesTransmitted;
// luz %%%: was commented out originally, re-enabled again
//          as hinted by devra5511@yahoo.com in syncml@yahoogroups.com at
//          Nov. 20th 2001.
            if (*pcbDataRead == 0)
             {
               BufferSize_t cbDataProcessed = cbDataBufferSize;
               p->bEox = decodeData (p, pbDataBuffer, &cbDataProcessed);
               *pcbDataRead = cbDataBufferSize - cbDataProcessed;
             }
            }
// end luz %%%
         }
      }
   return rc;
   }




/*****************************************************************************/
/*                                                                           */
/*   API Function: Wait for the server response (HTTP client function)       */
/*                                                                           */
/*****************************************************************************/

HttpRc_t httpWait (HttpHandle_t handle,               // i: instance handle
                   HttpDocumentContextPtr_t settings, // o: Document properties
                   HttpAuthenticationPtr_t auth)      // o: authentication info
   {
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;
   HttpRc_t rc = HTTP_RC_OK;

   if (!ISHEADERVALID (p) || (settings == NULL) ||
       (settings->cbSize != sizeof (HttpDocumentContext_t)))
      return HTTP_RC_PARAMETER;

   if ((p->fOpenMode == MODE_HTTP_SERVER) || (p->fTransferMode == MODE_READING))
      rc = HTTP_RC_NOT_ALLOWED;
   else
      {
      TcpRc_t iTcpRc = TCP_RC_OK;
      /*******************************************************/
      /* This is the first READ command of a POST request:   */
      /* if a chunked transfer encoding was selected,        */
      /* transmit the last chunk header. This will issue the */
      /* server to send the reply.                           */
      /*******************************************************/
      if (p->fTransferCoding == CHUNKED)
         iTcpRc = writeLastDataChunk (p);

      /*******************************************/
      /* Now we can wait for the server response */
      /*******************************************/
      if (iTcpRc == TCP_RC_OK)
         iTcpRc = readHttpHeader (p, auth);

      if (iTcpRc == TCP_RC_EOF)
         rc = HTTP_RC_EOF; // client closed the TCP/IP connection

      // %%% luz:2003-04-17: added these for timeout & SSL
      else if (iTcpRc == TCP_RC_ETIMEDOUT)
         rc = HTTP_RC_TIMEOUT; // TCP/IP timeout error
      else if (iTcpRc == TCP_TC_SLL_CERT_EXPIRED)
         rc = HTTP_RC_CERT_EXPIRED; // SSL certificate expired
      else if (iTcpRc == TCP_TC_SLL_CERT_INVALID)
         rc = HTTP_RC_CERT_INVALID; // SSL certificate invalid

      else if (iTcpRc != TCP_RC_OK)
         rc = HTTP_RC_COMMUNICATION; // TCP/IP communication error

      /***************************/
      /* Process the HTTP status */
      /***************************/
      else if (p->iHttpStatus != 200) switch (p->iHttpStatus)
         {
         case 100:
            /*****************************************/
            /* CONTINUE: send the HTTP header again. */
            /*****************************************/
            p->fTransferMode = MODE_UNDEFINED;
            if (p->cbCacheUsed < 0)
              rc = evaluateHttpHeaderBlock(p, auth);
            else
              rc = continueHttpRequest (p, auth);
            break;

         case 401:
         case 407:
            /**************************************************************/
            /* Authentication requrired: the caller will resend the data. */
            /**************************************************************/

            p->fTransferMode = MODE_UNDEFINED;
            rc = HTTP_RC_RETRY;
            break;

         default:
            if (p->iHttpStatus >= 400) rc = HTTP_RC_HTTP;
         }
      }

   /********************************************************************/
   /* Inform the caller about the document that is now being received. */
   /********************************************************************/
   if (rc == HTTP_RC_OK)
      {
      settings->pszURL = p->pchURI;
      settings->pszType = p->pchResponseType;
      settings->cbLength = (p->cbDataLength != UNDEFINED_CONTENT_LENGTH) ? p->cbDataLength : 0L;
      settings->pszReferer = p->pchReferer;
	  settings->pXSyncmlHmac = &(p->sXSyncmlHmac.hmacInfo);
      }
   return rc;
   }




/*****************************************************************************/
/*                                                                           */
/*   API Function: Inform the client that the document has been received     */
/*                                                 (HTTP Server function)    */
/*                                                                           */
/*****************************************************************************/

HttpRc_t httpReply (HttpHandle_t handle,                 // i: instance handle
                    int rcDocument,                      // i: document return code
                    const HttpReplyBufferPtr_t settings, // i: Response properties
                    const HttpAuthenticationPtr_t auth)  // i: Authentication info
   {
   HttpRc_t rc = HTTP_RC_OK;
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;

   if (!ISHEADERVALID (p) )
      return HTTP_RC_PARAMETER;

   if ((p->fOpenMode != MODE_HTTP_SERVER) || (p->fTransferMode == MODE_WRITING))
      rc = HTTP_RC_NOT_ALLOWED;


   if (rc == HTTP_RC_OK)
      {
      writeResponseHeader (p, rcDocument, settings, auth);

      p->iRc = tcpSendData (p->pSession, p->pbCache, xppStrlen ((char *)p->pbCache));
      p->cbCacheUsed = 0;

      if (p->iRc != TCP_RC_OK)
         rc = HTTP_RC_COMMUNICATION;
      }
   return rc;
   }




/*****************************************************************************/
/*                                                                           */
/*   API Function: Return the last HTTP communication error                  */
/*                                                                           */
/*****************************************************************************/

TcpRc_t httpGetError (HttpHandle_t handle) // i: instance handle
   {
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;
   if (!ISHEADERVALID (p))
      return TCP_RC_ERROR;

   return p->iRc;
   }


int  httpGetServerStatus (HttpHandle_t handle)
   {
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;
   if (!ISHEADERVALID (p))
      return 0;
   return p->iHttpStatus;
   }






/*****************************************************************************/
/*                                                                           */
/*   API Function: Check if the transmission of the document is finished     */
/*                                                                           */
/*****************************************************************************/

Bool_t httpIsEox (HttpHandle_t handle) // i: instance handle
   {
   HttpBufferPtr_t p = (HttpBufferPtr_t) handle;
   if (!ISHEADERVALID (p))
      return true;

   /***********************************************/
   /* Check if the HTTP status code was CONTINUE. */
   /* If so, resend the last request.             */
   /***********************************************/


   return p->bEox;
   }




/*****************************************************************************/
/*                                                                           */
/*   API Function: Return the required memory size for the instance object   */
/*                                                                           */
/*****************************************************************************/

BufferSize_t httpGetBufferSize (void)
   {
   return (BufferSize_t) sizeof (HttpBuffer_t);
   }

