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

#include "httpdefs.h"
#include <xpt.h>
#include "httpserverports.h"
#include "xptmutex.h"

#ifdef __EPOC_OS__
#include "http_globals_epoc.h"
#endif

//#define CLOSE_SOCKET_WHEN_ZERO
typedef struct _http_server_port_node {
    struct          _http_server_port_node *next;
    struct          _http_server_port_node *prev;
    StringBuffer_t  pszPort;
    Socket_t        serverSocket;
    int             inUse;
} HttpServerPortNode_t, *HttpServerPortNodePtr_t;

#ifndef __EPOC_OS__
MutexPtr_t listMutex;
HttpServerPortNodePtr_t firstNode = NULL;
#endif                                   
#ifdef __EPOC_OS__
#define listMutex TheHttpGlobalsEpoc()->listMutex
#define firstNode TheHttpGlobalsEpoc()->firstNode
#endif

void initializeServerPorts() {
    listMutex = newMutex();
}

void uninitializeServerPorts() {
    HttpServerPortNodePtr_t node = firstNode;
    HttpServerPortNodePtr_t nextNode = NULL;
    lockMutex( listMutex );
    // iterate list and close all sockets and free list;
    while (node != NULL) {
        xppFree( node->pszPort );
        tcpCloseConnection( &node->serverSocket );
        nextNode = node->next;
        xppFree( node );
    }
#ifndef __EPOC_OS__
    firstNode = NULL;
#else
	ResetFirstNode();
#endif
    unlockMutex( listMutex );
    freeMutex( listMutex );
}

HttpServerPortNodePtr_t findPort( CString_t   pszPort ) {

    HttpServerPortNodePtr_t node = firstNode;
    while (node != NULL) {
        if (xppStrcmp(pszPort, node->pszPort) == 0) {
            break;
        }
        node = node->next;
    }
    return node;
}

TcpRc_t selectServerSocket( CString_t   pszPort, SocketPtr_t serverSock ) {
    HttpServerPortNodePtr_t node = NULL;
    TcpRc_t rc = TCP_RC_OK;
    lockMutex( listMutex );
    node = findPort( pszPort );
    if (node == NULL) {
        node = xppMalloc( sizeof( HttpServerPortNode_t ) );
        xppMemset( node, '\0', sizeof( HttpServerPortNode_t ) );
        node->pszPort = xppMalloc( xppStrlen( pszPort) + 1 );
        xppStrcpy( node->pszPort, pszPort );
        rc = tcpOpenConnection( pszPort, &node->serverSocket, "s" );
        node->next = firstNode;
        firstNode = node;
    }
    if (node->inUse) {
        rc = TCP_RC_EADDRINUSE;
    } else {
        *serverSock = node->serverSocket;
        node->inUse = true;
    }
    unlockMutex( listMutex );
    return rc;
}

void        deselectServerSocket( CString_t   pszPort ) {
    HttpServerPortNodePtr_t node;
    lockMutex( listMutex );
    node = findPort( pszPort );
    if ( node ) {
        node->inUse = false;
#ifdef CLOSE_SOCKET_WHEN_ZERO
        if (node->useCount == 0) {
            if ( node == firstNode) {
                firstNode = node->next;
            } else {
                node->prev->next = node->next;
            }
            tcpCloseConnection( node->serverSocket );
        }
#endif
    }
    unlockMutex( listMutex );

}
