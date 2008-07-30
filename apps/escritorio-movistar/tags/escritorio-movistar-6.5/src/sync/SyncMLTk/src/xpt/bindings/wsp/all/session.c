/*************************************************************************/
/* module:          Communication Services, WSP Session Functions        */
/* file:            src/xpt/all/session.c                                */
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

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#include <session.h>
#include <xpt.h>           /* For the SML_ERR defines */
#include <wsputil.h>

#include <xptport.h>

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         Private Implementation Methods                  **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/


/**
 *  sessionCreate
 *       - allocates storage for a session structure, connects to server,
 *         populates session with connect results.
 *
 *  IN:    session      A pointer to a session structure
 *
 *  RETURN:
 *       An indication of whether or not the session was created.
 *
 *  Note:  According to the WSP spec the alias is informational, not negotiable.
 *         So, if the client has multiple addresses depending on the bearer, this
 *         would list the client's bearer addresses.  All the other capabilities are
 *         negotiable.
 **/
unsigned int sessionCreate(awsp_ConnectionHandle connHandle,
                        awsp_Capabilities *requestedCapabilities,
                        const char *staticClientHdrs,
                        WspSession_t **session)
{
   unsigned int rc  = SML_ERR_OK;
   awsp_Rc_t    aRc = AWSP_RC_OK;
   size_t       len = 0;
   char        *tmp = NULL;

   XPTDEBUG(("    sessionCreate(%lx, %lx, %lx, %lx)\n",
             (unsigned long) connHandle, (unsigned long) requestedCapabilities,
             (unsigned long) staticClientHdrs, (unsigned long) session));

   if (connHandle == NULL)
      return SML_ERR_A_XPT_NO_TRANSPORTS;

   if (session == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   rc = initializeSession(session);

   if (rc != SML_ERR_OK)
      return rc;

   len = xppStrlen(staticClientHdrs);
   if ((staticClientHdrs != NULL) && (len > 0)) {
      tmp = (char *) xppMalloc(len + 1);
      if (tmp != NULL)
         xppStrcpy(tmp, staticClientHdrs);
      (*session)->staticClientHeaders = tmp;
   }

   aRc = awsp_connect_req(connHandle,
                          &((*session)->sessionHandle),
                          (*session)->staticClientHeaders,
                          (len + 1),
                          requestedCapabilities);

   /* Can the connect RC indicate that the failure is because the SAP
    * was not opened in session mode?  If so, should we try to recover?        */
   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp connect request failed.");
      rc = SML_ERR_A_XPT_COMMUNICATION;
   } else
      rc = getConnectResponse(*session);

   if (rc != SML_ERR_OK)
      sessionRelease(*session);

   return rc;
}  /* End of sessionCreate() */

/**
 *  sessionDisconnect
 *       - disconnects the active session and releases its associated storage
 *
 *  IN:    session      A pointer to a session structure
 *
 **/
unsigned int sessionDisconnect(WspSession_t *session)
{
   unsigned int rc  = SML_ERR_OK;
   awsp_Rc_t aRc = AWSP_RC_OK;

   XPTDEBUG(("    sessionDisconnect(%lx)\n", (unsigned long) session));

   /* Maybe this should return OK - after all, the goal is reached */
   if (session == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   /* I'm not sure it's possible to have a session without a handle at this
    * point, so this check may be extraneous                                   */
   if (session->sessionHandle == NULL)
      return SML_ERR_OK;

   aRc = awsp_disconnect_req(session->sessionHandle,
                             AWSP_USERREQ,         /* Reason Code                 */
                             AWSP_FALSE,           /* Redirect Secure Session? No */
                             NULL,                 /* Redirect Addresses (none)   */
                             NULL,                 /* Error Header (none)         */
                             0,                    /* Error Header Length         */
                             NULL,                 /* Error Body (none)           */
                             0);                   /* Error Body Length           */

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp disconnect request failed.");
      rc = SML_ERR_A_XPT_COMMUNICATION;
   }

   sessionRelease(session);

   return rc;
} /* End of sessionDisconnect() */

/**
 *  sessionSuspend
 *       - suspends the active session, if it exists, and indicates it has
 *         been suspended.
 *
 *  IN:    session      A pointer to a session structure
 *
 *  RETURN:
 *       An indication of whether or not the suspension was successful.
 *
 **/
unsigned int sessionSuspend(WspSession_t *session)
{
   unsigned int rc  = SML_ERR_OK;
   awsp_Rc_t aRc = AWSP_RC_OK;

   XPTDEBUG(("    sessionSuspend(%lx)\n", (unsigned long) session));

   /* Should return something like 'No active session to suspend'              */
   if (session == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (session->sessionHandle != NULL) {
      aRc = awsp_suspend_req(session->sessionHandle);
      if (aRc != AWSP_RC_OK) {
         setLastError(aRc, "WSP Binding awsp suspend request failed.");
         rc = SML_ERR_A_XPT_COMMUNICATION;
      } else
         session->sessionSuspended = AWSP_TRUE;
   }

   return rc;
}  /* End of sessionSuspend() */

/**
 *  sessionResume
 *       - resumes a suspended session.
 *
 *  IN:    session      A pointer to a session structure
 *
 *  RETURN:
 *       An indication of whether or not the resume was successful.
 *
 **/
unsigned int sessionResume(awsp_ConnectionHandle connHandle,
                        WspSession_t *session)
{
   unsigned int rc       = SML_ERR_OK;
   awsp_Rc_t aRc      = AWSP_RC_OK;

   XPTDEBUG(("    sessionResume(%lx, %lx)\n", (unsigned long) connHandle, (unsigned long) session));

   if (connHandle == NULL)
      return SML_ERR_A_XPT_NO_TRANSPORTS;

   /* Should return 'No session to resume' */
   if (session == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   /* Session is already active, which was goal, so return OK?    */
   if (session->sessionSuspended == AWSP_FALSE)
      return SML_ERR_A_XPT_COMMUNICATION;

   aRc = awsp_resume_req(connHandle,
                         session->staticClientHeaders,
                         xppStrlen(session->staticClientHeaders));

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp resume request failed.");
      return SML_ERR_A_XPT_COMMUNICATION;
   }

   rc = getResumeResponse(connHandle, session);

   if (rc == SML_ERR_OK)
      session->sessionSuspended = AWSP_FALSE;

   return rc;
}   /* End of sessionResume() */

unsigned int initializeSession(WspSession_t **oSession)
{
   unsigned int  rc      = SML_ERR_OK;
   WspSession_t *session = NULL;

   XPTDEBUG(("      initializeSession(%lx)\n", (unsigned long) oSession));

   if (oSession == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (*oSession != NULL)
      sessionDisconnect(*oSession);

   session = (WspSession_t *) xppMalloc(sizeof(WspSession_t));

   if (session == NULL)
      rc = SML_ERR_A_XPT_MEMORY;

   xppMemset(session, 0, sizeof(WspSession_t));
   *oSession = session;

   return rc;

} /* End of initializeSession() */

/**
 *  sessionRelease
 *       - releases all storage associated with the session.
 *       - nullifies the protocol structure's access to the session.
 *
 *  IN     pid             A pointer to a protocol handle structure
 *
 **/
void sessionRelease(WspSession_t *session) {
   if (session == NULL) return;

   XPTDEBUG(("    sessionRelease(%lx)\n", (unsigned long) session));

   releaseCapabilities(session);
   releaseStaticClientHeaders(session);
   releaseStaticServerHeaders(session);

   xppFree(session);
} /* End of sessionRelease() */

/**
 *  releaseCapabilities
 *       - releases all storage associated with the session's negotiated
 *         capabilities.
 *       - nullifies the session structure's access to the capabilities.
 *
 *  IN     session         A pointer to a session structure
 *
 **/
void releaseCapabilities(WspSession_t *session) {

   XPTDEBUG(("      releaseCapabilities(%lx)\n", (unsigned long) session));

   if (session == NULL) return;

   /**
    * The object was allocated by the WAP stack and deallocated when we
    * disconnected the session, so we just have to nullify our pointer to it.
    **/
   session->negotiatedCapabilities = NULL;

} /* End releaseCapabilities() */

unsigned int getConnectResponse(WspSession_t *session)
{
   unsigned int rc  = SML_ERR_OK;
   awsp_Rc_t aRc = AWSP_RC_OK;
   char *bufferPtr = NULL;
   size_t  bufSize   = 0;

   XPTDEBUG(("      getConnectResponse(%lx)\n", (unsigned long) session));


   aRc = awsp_get_connect_cnf(session->sessionHandle,
                              bufferPtr,
                              &bufSize,
                              &(session->negotiatedCapabilities));

   if (aRc == AWSP_RC_BUFFER_TOO_SMALL) {
      bufferPtr = (char *)xppMalloc(bufSize);
      if (bufferPtr != NULL) {
         xppMemset(bufferPtr, 0, bufSize);
         aRc = awsp_get_connect_cnf(session->sessionHandle,
                                    bufferPtr,
                                    &bufSize,
                                    &(session->negotiatedCapabilities));
         if (aRc != AWSP_RC_OK) {
            xppFree(bufferPtr);
            releaseCapabilities(session);
         } else {
            /* allocated buffer is used, so don't release it                   */
            initializeStaticServerHeaders(session, bufferPtr,bufSize);
         }
      } else
         rc = SML_ERR_A_XPT_MEMORY;

   } /* End of confirmation buffer acquisition */

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp connect confirmation failed.");
      rc = SML_ERR_A_XPT_COMMUNICATION;
   }

   return rc;
} /* End of getConnectResponse() */

unsigned int getResumeResponse(awsp_ConnectionHandle connHandle, WspSession_t *session)
{
   unsigned int rc      = SML_ERR_OK;
   awsp_Rc_t aRc      = AWSP_RC_OK;
   char     *hdrBuf   = NULL;
   size_t   hdrLength = 0;

   XPTDEBUG(("      getResumeResponse(%lx, %lx)\n", (unsigned long) connHandle, (unsigned long) session));

   aRc = awsp_get_resume_cnf(connHandle,
                            &(session->sessionHandle),
                            hdrBuf,
                            &hdrLength);

   if (aRc == AWSP_RC_BUFFER_TOO_SMALL) {
      hdrBuf = (char *) xppMalloc(hdrLength);
      if (hdrBuf != NULL) {
         xppMemset(hdrBuf, 0, hdrLength);
         aRc = awsp_get_resume_cnf(connHandle,
                                  &(session->sessionHandle),
                                  hdrBuf,
                                  &hdrLength);

         if (aRc != AWSP_RC_OK)
            xppFree(hdrBuf);
         else
            /* move static header info into static buffer */
            initializeStaticServerHeaders(session, hdrBuf, hdrLength);
      } else
         rc = SML_ERR_A_XPT_MEMORY;
   }

   if (aRc != AWSP_RC_OK) {
      setLastError(aRc, "WSP Binding awsp resume confirmation failed.");
      return SML_ERR_A_XPT_COMMUNICATION;
   }

   return rc;
} /* End getResumeResponse() */

void initializeStaticServerHeaders(WspSession_t *session,
                                   char *bufferPtr,
                                   size_t bufSize) {

   XPTDEBUG(("      initializeStaticServerHeaders(%lx, %lx, %lu)\n",
             (unsigned long) session, (unsigned long) bufferPtr, (unsigned long) bufSize));

   if (session->staticServerHeaders != NULL)
      releaseStaticServerHeaders(session);

   session->staticServerHeaders = bufferPtr;

} /* End initializeStaticServerHeaders */

/**
 *  releaseStaticServerHeaders
 *       - releases all storage associated with the session's server-specified
 *         'static' HTTP header.
 *       - nullifies the session structure's access to this header.
 *
 *  IN     session         A pointer to a session structure
 *
 **/
void releaseStaticServerHeaders(WspSession_t *session) {

   XPTDEBUG(("      releaseStaticServerHeaders(%lx)\n", (unsigned long) session));

   if (session == NULL) return;

   xppFree((void *)session->staticServerHeaders);
   session->staticServerHeaders = NULL;

} /* End releaseStaticServerHeaders() */

/**
 *  releaseStaticClientHeaders
 *       - releases all storage associated with the session's client-specified
 *         'static' HTTP header.
 *       - nullifies the session structure's access to this header.
 *
 *  IN     session         A pointer to a session structure
 *
 **/
void releaseStaticClientHeaders(WspSession_t *session) {

   XPTDEBUG(("      releaseStaticClientHeaders(%lx)\n", (unsigned long) session));

   if (session == NULL) return;

   xppFree((void *)session->staticClientHeaders);
   session->staticClientHeaders = NULL;

} /* End releaseStaticClientHeaders() */

/**
 *  sessionIsConnected
 *       - Determines if the service access point was opened for session
 *         connections, and whether we have established a valid session.
 *
 *  IN:    session      A pointer to a session structure
 *
 *  RETURN:
 *       AWSP_TRUE      We are in session mode
 *       AWSP_FALSE     We are in connectionless mode
 **/
awsp_BOOL sessionIsConnected(WspSession_t *session)
{

/* We can verify what port the SAP was opened with only if the SAP parms
   were specified by the app - how do we know what port was used with
   the default initializeSAP()?  I guess we should heed whatever rc comes
   back from the sessionCreate - and if the RC indicates that the port
   doesn't support sessions, we'll know that the SAP is connectionless..
   Presumably the session handle doesn't exist if the SAP was opened
   for connectionless communication...What if SAP was opened for Session
   but something prevented the session handle from being created?
   if (sapSessionMode == AWSP_FALSE)
      return AWSP_FALSE;
*/
   XPTDEBUG(("    sessionIsConnected(%lx)\n", (unsigned long) session));

   if ((session == NULL) || (session->sessionHandle == NULL))
      return AWSP_FALSE;

   return AWSP_TRUE;

} /* End sessionIsConnected() */

const char *sessionGetStaticServerHeaders(WspSession_t *session)
{
   XPTDEBUG(("    sessionGetStaticServerHeaders()\n"));
   if (session == NULL)
      return NULL;

   return session->staticServerHeaders;
} /* End sessionGetStaticServerHeaders() */


