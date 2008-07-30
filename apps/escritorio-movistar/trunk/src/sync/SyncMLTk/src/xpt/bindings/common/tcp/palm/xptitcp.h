/*************************************************************************/
/* module:          SyncML Communication Protocol selection module       */
/* file:            src/xpt/palm/xptitcp.h                               */
/* target system:   palm                                                 */
/* target OS:       palm                                                 */
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
 * This module contains Palm OS specific definitions for the
 * TCP/IP protocol handler for SyncML
 * invokes the miscellaneous protocol drivers.
 *
 */

#ifndef XPTITCP_H
#define XPTITCP_H

#ifndef Bool_t
#define Bool_t unsigned int
#endif

#ifdef USE_SDK_35
 #include <PalmOS.h>
 #include <PalmCompatibility.h>
#else
 #include <Pilot.h>
#endif

#include <sys_socket.h>

#ifdef PALM_SSL
  // we need the SSL lib stuff
  #include <SslLib.h>
  // we have platform-specific send and receive functions for WINCE implemented in xptitcp.c
  #define PLATFORM_TCPREADDATA
  #define PLATFORM_TCPSENDDATA  
  // we also implement SSL in xptitcp.c
  #define PLATFORM_TCPENABLESSL
  // we also implement a function to clean up the socket before closing it
  #define PLATFORM_BEFORESOCKETCLOSE
#endif


#include "xpt-tcp.h"


#if EMULATION_LEVEL == EMULATION_NONE
//Err			errno = 0;
#endif

// socket definitions
typedef long SOCKET;
#define SOCKET_ERROR   -2
#define INVALID_SOCKET -1

#ifdef FOR_PALM_EMULATOR
#define TIMEOUT_SECONDS 120 // %%% luz 2002-11-12: more in emulator (seems to be less than specified when emulated)
#else
//#define TIMEOUT_SECONDS  30   // TCPIP timeout / second
#define TIMEOUT_SECONDS  60   // %%% luz: 30 seconds is too short, so we use 60
#endif

// # of active sockets
extern int _iNumSockets;

// %%% 2002-09-28 luz added
extern Bool_t weOpenedNetLib;

// %%% 2002-09-28 luz added: Macro to test if we need to init TCP stack
#define TCPINITIALIZED() (weOpenedNetLib)
#define TCPINITDONE() // not needed here

/***********************************************************/
/* Function: Initialize the TCP/IP stack                   */
/* This must be done the first time the process is started */
/***********************************************************/
int _initTCPStack (void);

/********************************************************/
/* Function: implementation of the strrchr() c-function */
/********************************************************/
char *StrRChr (const char *pszString, int ch);

/****************************************/
/* Function: Clean-up the TCP/IP stack. */
/****************************************/
void _termTCPStack (Bool_t aImmediate);

/**************************************************************************/
/* The following macros check if the TCP/IP stack needs to be initialized */
/**************************************************************************/
#define TCPINIT(r) (r) = _initTCPStack ()
#define TCPTERM() _termTCPStack (FALSE)
#define closesocket close

/********************************************************/
/* The following macro checks if the socket is in error */
/********************************************************/
#define CHKERROR(r) \
   (TcpRc_t)(((r)==SOCKET_ERROR||(r)==INVALID_SOCKET)?errno:TCP_RC_OK)

#endif
