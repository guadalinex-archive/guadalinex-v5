/*************************************************************************/
/* module:          Communication Services, WSP Session Functions        */
/* file:            src/xpt/all/session.h                                */
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

#ifndef WSP_SESSION_H
#define WSP_SESSION_H

#include <awsp.h>

#include <xptTransport.h>
#include <wspdef.h>

/**
 *  A structure to hold WSP session-specific information.
 **/
typedef struct {
   awsp_SessionHandle      sessionHandle;
   awsp_Capabilities      *negotiatedCapabilities;
   const char             *staticServerHeaders;
   const char             *staticClientHeaders;
   awsp_BOOL               sessionSuspended;
} WspSession_t;

/* Methods invoked from outside of session.c */
unsigned int sessionCreate(awsp_ConnectionHandle connHandle,
                        awsp_Capabilities *requestedCapabilities,
                        const char *staticClientHdrs,
                        WspSession_t **session) WSP_SECTION;
unsigned int sessionSuspend(WspSession_t *session) WSP_SECTION;
unsigned int sessionResume(awsp_ConnectionHandle connHandle, WspSession_t *session) WSP_SECTION;
unsigned int sessionDisconnect(WspSession_t *session) WSP_SECTION;
awsp_BOOL sessionIsConnected(WspSession_t *session) WSP_SECTION;
void sessionRelease(WspSession_t *session) WSP_SECTION;
const char *sessionGetStaticServerHeaders(WspSession_t *session) WSP_SECTION;

/* Methods invoked only from within session.c */
unsigned int getConnectResponse(WspSession_t *session) WSP_SECTION;
unsigned int getResumeResponse(awsp_ConnectionHandle connHandle, WspSession_t *session) WSP_SECTION;
unsigned int initializeSession(WspSession_t **oSession) WSP_SECTION;
void initializeStaticServerHeaders(WspSession_t *session,char *bufferPtr,size_t bufSize) WSP_SECTION;
void releaseCapabilities(WspSession_t *session) WSP_SECTION;
void releaseStaticClientHeaders(WspSession_t *session) WSP_SECTION;
void releaseStaticServerHeaders(WspSession_t *session) WSP_SECTION;

#endif
