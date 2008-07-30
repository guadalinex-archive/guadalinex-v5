/*************************************************************************/
/* module:          SyncML Communication Protocol selection module       */
/* file:            src/xpt/win/xptitcp.h                                */
/* target system:   win                                                  */
/* target OS:       win                                                  */
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


/**
 * This module contains Windows specific definitions for the
 * TCP/IP protocol handler for SyncML
 * invokes the miscellaneous protocol drivers.
 *
 */

#ifndef XPTITCP_H
#define XPTITCP_H

#include <string.h>

#if __IBMC__||__IBMCPP__
#include <windows.h>
#else
#include <winsock.h>
#endif

#ifdef WINCE_SSL
#include <sslsock.h>
#include <wincrypt.h>
#endif

#ifdef WINCE
  // we have platform-specific send and receive functions for WINCE implemented in xptitcp.c
  #define PLATFORM_TCPREADDATA
  #define PLATFORM_TCPSENDDATA
#endif
#ifdef WINCE_SSL
  // we also implement SSL in xptitcp.c
  #define PLATFORM_TCPENABLESSL
#endif

#include "xpt-tcp.h"

// %%% luz 2002-09-12: source now uses StrRChr to allow redefinition on other platforms as needed
#define StrRChr strrchr


/**************************************************************************/
/* The following macros check if the TCP/IP stack needs to be initialized */
/**************************************************************************/
void _termTCPStack (int aImmediate);
void _initTCPStack (TcpRc_t *rcP);
#define TCPINIT(r) _initTCPStack(&r) // %%% now calls function (see xptitcp.c)
#define TCPTERM() _termTCPStack(true) // %%% now calls function (see xptitcp.c)


/********************************************************/
/* The following macro checks if the socket is in error */
/********************************************************/
#define CHKERROR(r) \
   (TcpRc_t)(((r)==SOCKET_ERROR||(r)==INVALID_SOCKET)?(WSAGetLastError()-WSABASEERR):TCP_RC_OK)


#endif

/* eof */
