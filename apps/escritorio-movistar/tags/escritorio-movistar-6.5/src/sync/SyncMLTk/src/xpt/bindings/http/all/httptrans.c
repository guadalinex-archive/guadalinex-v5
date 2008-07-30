/*************************************************************************/
/* module:          Communication Services, HTTP functions               */
/* file:            src/xpt/bindings/http/all/httptrans.c                */
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

#include <httpdefs.h>
#include <httptrans.h>

// %%% luz: required for sprintf
#include <stdio.h>

#include "httpserverports.h"
#ifdef __EPOC_OS__
#include "http_globals_epoc.h"
#endif
#ifndef __EPOC_OS__
static HttpTransportInfo_t transportInfo;
#endif
#ifdef __EPOC_OS__
#define transportInfo TheHttpGlobalsEpoc()->transportInfo
#endif

#ifdef __PALM_OS__
 #define sprintf StrPrintF
#endif



// LEO:
#ifdef _WIN32
// #include "xptitcp.h" // internal OS specific implementation headers.
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif





/*---------------------------------*/
#ifdef LINK_TRANSPORT_STATICALLY
 #define initializeTransport HTTP_initializeTransport
#endif
XPTEXP1 Ret_t XPTAPI XPTEXP2 initializeTransport() {

    struct xptTransportDescription desc;

#ifdef __EPOC_OS__
	Ret_t err;
	err = httpOpenTLS();
	if (err != SML_ERR_OK)
		return SML_ERR_A_XPT_MEMORY;
#endif
    desc.shortName = "HTTP";
    desc.description = "HyperText Transfer Protocol over TCP";
    desc.flags = XPT_CLIENT | XPT_SERVER;
    desc.privateTransportInfo = &transportInfo;        // Transport Info

    desc.selectProtocol     = HTTP_selectProtocol;
    desc.deselectProtocol   = HTTP_deselectProtocol;
    desc.openCommunication  = HTTP_openCommunication;
    desc.closeCommunication = HTTP_closeCommunication;
    desc.beginExchange      = HTTP_beginExchange;
    desc.endExchange        = HTTP_endExchange;
    desc.receiveData        = HTTP_receiveData;
    desc.sendData           = HTTP_sendData;
    desc.sendComplete       = HTTP_sendComplete;
    desc.setDocumentInfo    = HTTP_setDocumentInfo;
    desc.getDocumentInfo    = HTTP_getDocumentInfo;
	

	// LEO:
	desc.cancelCommAsync	= HTTP_cancelCommAsync;


#ifdef __EPOC_OS__
    desc.resetBindingTLS    = httpCloseTLS;
#endif
    transportInfo.cbSize = sizeof( transportInfo );

    initializeServerPorts();

    return xptRegisterTransport( &desc );
}

void setError( unsigned long rc, const char *msg ) {
   struct xptTransportErrorInformation err;

   err.protocolErrorCode = rc;
   err.errorMessage = msg;

   xptSetLastError( &err );
}


void getFirewallInfo( TcpFirewallInfoPtr_t firewall, const char *buffer, int len ) {
    char *p;
    firewall->serverName = xppMalloc( len + 1 );
    xppMemcpy( firewall->serverName, buffer, len );
    firewall->serverName[len] = '\0';
    p = xppStrchr( firewall->serverName, ':' );
    if (p) {
        *p = '\0';
        firewall->serverPort = xppAtoi( p+1 );
    }
}


Ret_t XPTAPI  HTTP_selectProtocol(void *privateTransportInfo,
                                     const char *metaInformation,
                                     unsigned int flags,
                                     void **pPrivateServiceInfo) {

    Ret_t ret = SML_ERR_OK;
    TcpRc_t rc;

    HttpTransportServiceInfoPtr_t info = NULL;


    if ( privateTransportInfo == NULL || pPrivateServiceInfo == NULL ) {
        setError( 1, "One of privateTransportInfo or privateServiceInfo pointer was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }

    info = ( HttpTransportServiceInfoPtr_t )xppMalloc( sizeof( HttpTransportServiceInfo_t ) );
    xppMemset( info, '\0', sizeof( HttpTransportServiceInfo_t ) );
    info->cbSize = sizeof( HttpTransportServiceInfo_t );

    info->socketClient = (flags & XPT_CLIENT);

    if ( info->socketClient ) {
        // initialte a client connection
        const char *ptr;
        size_t  len;

        ptr = xptGetMetaInfoValue( metaInformation, "HOST", &len );
        if (!ptr) {
            xppFree(info);
            setError( 1, "HOST information is not defined in select metaInformation" );
            return SML_ERR_A_XPT_COMMUNICATION;
        }

        info->info.clientInfo.pchServerAddress = xppMalloc( len + 1 );
        xppMemcpy( info->info.clientInfo.pchServerAddress, ptr, len );
        info->info.clientInfo.pchServerAddress[len] = '\0';

        info->info.clientInfo.firewallInfo.type = TCP_FIREWALL_DIRECT;

        ptr = xptGetMetaInfoValue( metaInformation, "SOCKSHOST", &len );
        if (ptr) {
            info->info.clientInfo.firewallInfo.type = TCP_FIREWALL_SOCKS;
            getFirewallInfo( &info->info.clientInfo.firewallInfo, ptr, len );
        } else {
            ptr = xptGetMetaInfoValue( metaInformation, "PROXYHOST", &len );
            if (ptr) {
                info->info.clientInfo.firewallInfo.type = TCP_FIREWALL_PROXY;
                getFirewallInfo( &info->info.clientInfo.firewallInfo, ptr, len );
                // Keep the proxy String, for use later.
                info->info.clientInfo.proxyString = xppMalloc( len + 1 );
                xppMemcpy( info->info.clientInfo.proxyString, ptr, len );
                info->info.clientInfo.proxyString[len] = '\0';
            }
        }
        
        // %%% luz:2003-04-16 added SSL support
        // default to no SSL
        info->info.clientInfo.useSSL=false;
        ptr = xptGetMetaInfoValue( metaInformation, "SSL", &len );
        if (ptr) {
          if (*ptr!='0') {
            // SSL requested
            info->info.clientInfo.useSSL=true;
          }
        }
  
    } else {
        // initiate a server connection
        const char *ptr;
        size_t  len;

        info->info.serverInfo.pchServerPort = xppMalloc(8);

        ptr = xptGetMetaInfoValue( metaInformation, "SERVERPORT", &len );
        if (!ptr) {
            xppStrcpy( info->info.serverInfo.pchServerPort, "80" );
        } else {
            if (len >= 8) {
                xppFree(info);
                setError( 1, "SERVERPORT information is invalid in select metaInformation" );
                return SML_ERR_A_XPT_COMMUNICATION;
            }
            xppMemcpy( info->info.serverInfo.pchServerPort, ptr, len );
            info->info.serverInfo.pchServerPort[len] = '\0';
        }
        rc = selectServerSocket( info->info.serverInfo.pchServerPort, &info->info.serverInfo.serverSocket );
        if ( TCP_RC_OK == rc ) {
            info->info.serverInfo.pchSenderAddress = xppMalloc( 256 );
        } else {
            char msg[ 64 ];
            sprintf( msg, "TCP error setting up listen sock for port %s", info->info.serverInfo.pchServerPort );
            setError( rc, msg );
            xppFree( info->info.serverInfo.pchServerPort );
            info->info.serverInfo.pchServerPort = NULL;
            xppFree( info );
            info = NULL;
            ret = SML_ERR_A_XPT_COMMUNICATION;
        }

    }

    *pPrivateServiceInfo = info;
    return ret;
}

Ret_t XPTAPI HTTP_deselectProtocol(void *privateServiceInfo) {
    HttpTransportServiceInfoPtr_t info = ( HttpTransportServiceInfoPtr_t )privateServiceInfo;
    if ( privateServiceInfo == NULL ) {
        setError( 1, "privateServiceInfo was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }
    if ( info->socketClient ) {
        if ( info->info.clientInfo.firewallInfo.type != TCP_FIREWALL_DIRECT ) {
            if ( info->info.clientInfo.firewallInfo.serverName ) {
                xppFree( info->info.clientInfo.firewallInfo.serverName );
                info->info.clientInfo.firewallInfo.serverName = NULL;
            }
        }
    } else {
        if (info->info.serverInfo.pchServerPort) {
            deselectServerSocket( info->info.serverInfo.pchServerPort );
            xppFree( info->info.serverInfo.pchServerPort );
            info->info.serverInfo.pchServerPort = NULL;
        }
        if ( info->info.serverInfo.pchSenderAddress ) {
            xppFree( info->info.serverInfo.pchSenderAddress );
            info->info.serverInfo.pchSenderAddress = NULL;
        }
    }
    xppFree( privateServiceInfo );
    return SML_ERR_OK;
}

Ret_t XPTAPI HTTP_openCommunication(void *privateServiceInfo,
                                    int role,
                                    void **pPrivateConnectionInfo) {

    Ret_t rc = SML_ERR_OK;
    HttpTransportConnInfoPtr_t info = NULL;

    if ( privateServiceInfo == NULL || pPrivateConnectionInfo == NULL ) {
        setError( 1, "One of privateServiceInfo or privateConnectionInfo pointer was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }

    info = (HttpTransportConnInfoPtr_t)xppMalloc( sizeof( HttpTransportConnInfo_t ) );

    if (info == NULL) {
        setError( 2, "Unable to allocate memory for connection Info block" );
        return SML_ERR_A_XPT_MEMORY;
    }
    info->cbSize = sizeof( HttpTransportConnInfo_t );
    info->pServiceInfo = ( HttpTransportServiceInfoPtr_t )privateServiceInfo;
    info->smlClient = (role == XPT_REQUEST_SENDER);

    if ( info->pServiceInfo->socketClient ) {
        rc = tcpOpenConnectionEx(
          info->pServiceInfo->info.clientInfo.pchServerAddress,
          &info->socket,
          // %%% luz:2003-04-16 added SSL support with special "cS" open mode.
          info->pServiceInfo->info.clientInfo.useSSL ? "cS" : "c",
          &info->pServiceInfo->info.clientInfo.firewallInfo
        );
        if ( rc != TCP_RC_OK ) {
            setError( rc, "TCP error Unable to establish connection with server" );
        }
    } else {
        rc = tcpWaitforConnections ( &info->pServiceInfo->info.serverInfo.serverSocket, &info->socket , info->pServiceInfo->info.serverInfo.pchSenderAddress );
        if ( rc != TCP_RC_OK ) {
            setError( rc, "TCP error Error accepting connection from client" );
        }
    }
    if ( rc != TCP_RC_OK ) {
        rc = SML_ERR_A_XPT_COMMUNICATION;
        xppFree( info );
        info = NULL;
    }

    *pPrivateConnectionInfo = info;
    return rc;
}

Ret_t XPTAPI  HTTP_closeCommunication(void *privateConnectionInfo) {

    HttpTransportConnInfoPtr_t connInfo = ( HttpTransportConnInfoPtr_t )privateConnectionInfo;
    if ( connInfo == NULL ) {
        setError( 1, "privateConnectionInfo was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }
#ifdef OLD
    if ( !connInfo->pServiceInfo->socketClient && connInfo->state == HTTP_TRANSPORT_RECEIVING ) {
        // if server mode and didn't send response......
        HttpReplyBuffer_t replyDoc;
        xppMemset( &replyDoc, '\0', sizeof( replyDoc ) );
        replyDoc.cbSize = sizeof( replyDoc );
        replyDoc.cbLength = 0;
        replyDoc.pszType  = "text/text";
        httpReply( connInfo->http, 500, &replyDoc, NULL );
    }
#endif
    tcpCloseConnection( &connInfo->socket );
    xppFree( privateConnectionInfo );
    return SML_ERR_OK;
}

Ret_t XPTAPI  HTTP_beginExchange(void *privateConnectionInfo) {
    HttpTransportConnInfoPtr_t connInfo = ( HttpTransportConnInfoPtr_t )privateConnectionInfo;
    if ( connInfo == NULL ) {
        setError( 1, "privateConnectionInfo was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }

    connInfo->http = xppMalloc( httpGetBufferSize() );
    xppMemset( connInfo->http, '\0', httpGetBufferSize() );

    return SML_ERR_OK;
}

Ret_t XPTAPI  HTTP_endExchange(void *privateConnectionInfo) {
    HttpTransportConnInfoPtr_t connInfo = ( HttpTransportConnInfoPtr_t )privateConnectionInfo;
    if (connInfo->http) {
        httpClose( connInfo->http );
        xppFree( connInfo->http );
        connInfo->http = NULL;
    }
    return SML_ERR_OK;
}

Ret_t XPTAPI  HTTP_receiveData(void *privateConnectionInfo,
                               void *buffer,
                               size_t bufferLen,
                               size_t *dataLen) {

    HttpTransportConnInfoPtr_t connInfo = ( HttpTransportConnInfoPtr_t )privateConnectionInfo;
    HttpRc_t rc;
    if ( privateConnectionInfo == NULL || buffer == NULL || dataLen == NULL ) {
        setError( 1, "One of privateConnectionInfo, dataBuffer or datLen pointer was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }
    *dataLen = 0;
    if ( !httpIsEox( connInfo->http ) ) {
        rc = httpRead( connInfo->http, buffer, bufferLen, (BufferSize_t *)dataLen );
        if ( HTTP_RC_OK != rc ) {
            setError( rc, "HTTP error reading data." );
            return SML_ERR_A_XPT_COMMUNICATION;
        }
    }

    return SML_ERR_OK;
}

Ret_t XPTAPI  HTTP_sendData(void *privateConnectionInfo,
                            const void *buffer,
                            size_t bufferLen,
                            size_t *bytesSent ){

    HttpTransportConnInfoPtr_t connInfo = ( HttpTransportConnInfoPtr_t )privateConnectionInfo;
    HttpRc_t rc;
    *bytesSent = 0;
    if ( privateConnectionInfo == NULL || buffer == NULL) {
        setError( 1, "One of privateConnectionInfo or dataBuffer was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }
    rc = httpWrite( connInfo->http, (void *)buffer, bufferLen, false );
    if ( HTTP_RC_OK != rc ) {
        setError( rc, "HTTP error sending data." );
        return SML_ERR_A_XPT_COMMUNICATION;
    }

    *bytesSent = bufferLen;
    return SML_ERR_OK;
}

Ret_t XPTAPI  HTTP_sendComplete(void *privateConnectionInfo) {
#ifdef BASE64_ENCODING
    HttpTransportConnInfoPtr_t connInfo = ( HttpTransportConnInfoPtr_t )privateConnectionInfo;
    HttpRc_t rc = httpWrite( connInfo->http, (void *)NULL, 0, true );
    if ( HTTP_RC_OK != rc ) {
        setError( rc, "HTTP error sending data." );
        return SML_ERR_A_XPT_COMMUNICATION;
    }
#endif
    return SML_ERR_OK;
}

Ret_t XPTAPI  HTTP_setDocumentInfo(void *privateConnectionInfo,
                                   const XptCommunicationInfo_t *pDoc) {
    HttpRc_t hrc;
    HttpTransportConnInfoPtr_t connInfo = ( HttpTransportConnInfoPtr_t )privateConnectionInfo;


    if ( connInfo == NULL || pDoc == NULL ) {
        setError( 1, "One of privateConnectionInfo or XptCommunicationInfo pointer was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }


    if (connInfo->smlClient) {

        HttpDocumentContext_t doc;


        xppMemset( &doc, '\0', sizeof( doc ) );
        doc.cbSize   = sizeof( doc );
        doc.cbLength = pDoc->cbLength != -1 ? pDoc->cbLength : 0;
        doc.pszType  = pDoc->mimeType;
        doc.pszURL   = pDoc->docName;
        doc.pszProxy = connInfo->pServiceInfo->info.clientInfo.proxyString;
        doc.pszHost  = connInfo->pServiceInfo->info.clientInfo.pchServerAddress;
		doc.pXSyncmlHmac = pDoc->hmacInfo;
        // %%% luz:2002-05-23: Auth support added
        doc.auth = pDoc->auth;  // auth structure created by authInit()

        hrc = httpOpen( connInfo->http, &connInfo->socket, "EXCHANGE", &doc, doc.auth /* %%% luz:2002-05-23, was NULL before (no auth support for client) */ );

        if ( HTTP_RC_OK != hrc ) {
            setError( hrc, "HTTP error starting exchange with server" );
            return SML_ERR_A_XPT_COMMUNICATION;
        }
    } else {

        HttpReplyBuffer_t replyDoc;

        xppMemset( &replyDoc, '\0', sizeof( replyDoc ) );
        replyDoc.cbSize = sizeof( replyDoc );
        replyDoc.cbLength = pDoc->cbLength != -1 ? pDoc->cbLength : 0;
        replyDoc.pszType  = pDoc->mimeType;
		replyDoc.pXSyncmlHmac = pDoc->hmacInfo;

        hrc = httpReply( connInfo->http, 200, &replyDoc, NULL );

        if ( HTTP_RC_OK != hrc ) {
            setError( hrc, "HTTP error seding reply to client" );
            return SML_ERR_A_XPT_COMMUNICATION;
        }
    }

    return SML_ERR_OK;
}

Ret_t XPTAPI  HTTP_getDocumentInfo(void *privateConnectionInfo,
                                      XptCommunicationInfo_t *pDoc){

    HttpRc_t hrc;
    HttpDocumentContext_t doc;
    int len;

    HttpTransportConnInfoPtr_t connInfo = ( HttpTransportConnInfoPtr_t )privateConnectionInfo;
    if ( connInfo == NULL || pDoc == NULL ) {
        setError( 1, "One of privateConnectionInfo or XptCommunicationInfo pointer was NULL" );
        return SML_ERR_A_XPT_INVALID_PARM;
    }

    xppMemset( &doc, '\0', sizeof( doc ) );
    doc.cbSize   = sizeof( doc );

    // %%% luz:2002-05-23: Auth support added
    doc.auth = pDoc->auth;  // auth structure created by authInit()

    if (connInfo->smlClient) {
        hrc = httpWait( connInfo->http, &doc, doc.auth /* %%% luz:2002-05-23, was NULL before (no auth support for client) */  );
        // %%% luz:2002-05-28: added handling for HTTP 401/407 error case
        if ( hrc == HTTP_RC_RETRY) {
            setError( hrc, "HTTP server access denied, must retry sending with proper credentials" );
            // do not modify document info, just return
            return SML_ERR_A_XPT_ACCESS_DENIED;
        }
        if ( HTTP_RC_OK != hrc ) {
            setError( hrc, "HTTP error waiting for a response from the server" );
            return SML_ERR_A_XPT_COMMUNICATION;
        }

    } else {

        hrc = httpOpen( connInfo->http, &connInfo->socket, "SERVER", &doc, NULL );
        if ( HTTP_RC_OK != hrc ) {
            setError( hrc, "HTTP error establishing a HTTP session with the client" );
            return SML_ERR_A_XPT_COMMUNICATION;
        }
    }
	  pDoc->hmacInfo = doc.pXSyncmlHmac;
	  // %%% luz:2002-05-23: Auth support added
    pDoc->auth = doc.auth; // return auth structure, NULL if none

    pDoc->cbLength = ((doc.cbLength >= 0) && (doc.cbLength != UNDEFINED_CONTENT_LENGTH))  ? doc.cbLength : -1;
    len = xppStrlen( doc.pszURL );
    if ( len > XPT_DOC_NAME_SIZE ) {
      char *msg = xppMalloc( len + 64 );
      sprintf( msg, "Returned document length is too large for supplied buffer: %s", doc.pszURL );
      setError( 5, msg );
      xppFree( msg );
      return SML_ERR_A_XPT_COMMUNICATION;
    }
    xppMemcpy( pDoc->docName, doc.pszURL, len+1 );

    // %%% discovered by luz 2002-04-xx: for some uninvestigated reason,
    // doc.pszType can be NULL here sometimes.
    // RTK Maint.Rel.4 adds own handling here, but
    // will xppMemCpy work ok with UNDEFINED_CONTENT_LENGTH???

    // %%% modified code by luz 2002-05-23
    if (doc.pszType) {
      len = xppStrlen( doc.pszType );
      if ( len > XPT_DOC_TYPE_SIZE ) {
  		  char *base_msg = "Returned document mime type is too large for supplied buffer";
        char *msg = xppMalloc( len + 64 );
  		  if (msg) {
           sprintf( msg, "%s: %s", base_msg, doc.pszType );
           setError( 5, msg );
           xppFree( msg );
  		  } else {
  			   setError( 5, base_msg);
  		  }
        return SML_ERR_A_XPT_COMMUNICATION;
      }
      xppMemcpy( pDoc->mimeType, doc.pszType, len+1 );
    } else {
      // no MIME-type
      pDoc->mimeType[0]=0;
    }

    /* original code:
    if (doc.pszType) {
      len = xppStrlen( doc.pszType );
    } else {
      len = UNDEFINED_CONTENT_LENGTH;
    }
    if ( len > XPT_DOC_TYPE_SIZE ) {
		  char *base_msg = "Returned document mime type is too large for supplied buffer";
      char *msg = xppMalloc( len + 64 );
		  if (msg) {
         sprintf( msg, "%s: %s", base_msg, doc.pszType );
         setError( 5, msg );
         xppFree( msg );
		  } else {
			   setError( 5, base_msg);
		  }
      return SML_ERR_A_XPT_COMMUNICATION;
    }
    xppMemcpy( pDoc->mimeType, doc.pszType, len+1 );
    */
    
    return SML_ERR_OK;
}

// LEO:

Ret_t XPTAPI  HTTP_cancelCommAsync(void *privateConnectionInfo){
	HttpTransportConnInfoPtr_t info = (HttpTransportConnInfoPtr_t)privateConnectionInfo;
#ifdef _WIN32
	if (info->socket!=INVALID_SOCKET)
#else
	if (info->socket!=-1)
#endif
	{
#ifdef _WIN32
		shutdown(info->socket, SD_SEND);
		closesocket(info->socket);
#else
		shutdown(info->socket, 1);
		close(info->socket);
#endif
#ifdef _WIN32
		info->socket = INVALID_SOCKET;
#else
		info->socket = -1;
#endif
	}
	return SML_ERR_OK;
}

