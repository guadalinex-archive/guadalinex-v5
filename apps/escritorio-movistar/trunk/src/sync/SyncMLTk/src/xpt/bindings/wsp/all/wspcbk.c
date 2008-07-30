/*************************************************************************/
/* module:          Communication Services, AWSP Callback Functions      */
/* file:            src/xpt/all/wspcbk.c                                 */
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


#define WSP_EXPORTING
#include <awsp.h>
#include <xptdef.h>
#include <xpt-wsp.h>

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         WSP Callback methods                            **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

/*
 * Disconnect Indicator in response to Request (callback)
 *    IN:   connHandle              (required)        A Handle for the initialized service
 *                                                    Access Point - see initializeSAP().
 *          reasonCode              (required)        Cause of the disconnection
 *          redirectSecurity        (conditional)     Indicates if existing secure session
 *                                                    can be reused for redirection to new
 *                                                    server.
 *                                                    TRUE  = secure session can be reused.
 *                                                    FALSE = secure session cannot be reused,
 *                                                            new secure session must be
 *                                                            established with new server.
 *          redirectAddresses       (conditional)     Alternate address(es) to redirect
 *                                                    the request to
 *          errorHeaders            (optional)        HTTP 1.1 headers for the error
 *                                                    body information.
 *          errorHeadersLength      (optional)        Length of Error Headers buffer.
 *                                                    Required if Error Headers is not NULL.
 *          errorBody               (optional)        Info to expand on reason code.
 *                                                    Required if Error Headers is not NULL
 *          errorBodyLength         (optional)        Length of Error Body buffer.
 *                                                    Required if Error Body is not NULL.
 *
 *
 * This method would be invoked by the WAP stack to notify us, the client, that the server
 * has requested that our session be disconnected.
 *
 * It is here as a callback as a place-holder, since we need to be able to receive the
 * notification.  However, if callbacks are not feasible from the WAP implementations, or
 * on the Palm (single-threaded platform), then an alternative technique will be required.
 *
 */
XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_disconnect_ind(awsp_ConnectionHandle connHandle,
                         int                   reasonCode,
                         awsp_BOOL             redirectSecurity,
                         const char           *redirectAddresses[],
                         const char           *errorHeaders,
                         size_t                errorHeadersLength,
                         void                 *errorBody,
                         size_t                errorBodyLength)
{
   /**
    *
    * If there is a pending transaction this needs to terminate it with an error
    * somehow...
    *
    * The error information should be made available via getLastCommunicationError().
    *
    * How do we handle re-direct?  We have no way to tell the caller except in the
    * error block, so presumably we should keep track and when the caller tries to
    * reopen a SAP we convert to the redirect address?  This may work for temporary,
    * but how long is temporary?  And what about permanent??  Ideally a permanent
    * redirect should result in a change in either the app's SAP info or in the
    * default for the WAP stack, and we have no way to tell either of them to update
    * themselves...
    *
    **/
    return AWSP_RC_OK;

} /* End of awsp_disconnect_ind() */

/*
 * Push Indicator (callback)
 *    IN:   sessionHandle           (required)     A Handle for the session that this
 *                                                 transaction is executing in.
 *          pushHeaders             (conditional)  HTTP 1.1 headers
 *          pushHeadersLength       (conditional)  The length of the push headers
 *                                                 buffer.  Required if push headers is
 *                                                 not NULL.
 *          pushBody                (conditional)  Pushed data.
 *          pushBodyLength          (conditional)  The length of the Push Body buffer.
 *                                                 Required if push body is not NULL.
 *
 * This method is invoked by the WAP stack to notify the client that the
 * server is pushing data to it.
 *
 * Again, it is unclear if/how the Palm and Wap stacks can handle callbacks.  This
 * is a placeholder for our need to be able to handle these notifications.
 */
XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_push_ind(awsp_SessionHandle sessionHandle,
                   const char        *pushHeaders,
                   size_t             pushHeadersLength,
                   void              *pushBody,
                   size_t             pushBodyLength)
{
   /**
    *  I have NO idea how we communicate to the application that we have push data
    *  for it to receive and process....
    *
    *  Perhaps the application could register a routine to receive control on the
    *  push, but that would be another toolkit api that would be protocol-dependent.
    **/
   return AWSP_RC_OK;
} /* End of asp_push_ind() */

/*
 * Confirmed Push Indicator (callback)
 *    IN:   sessionHandle           (required)     A Handle for the session that this
 *                                                 transaction is executing in.
 *          clientPushID            (required)     Unique identifier for this push transaction
 *          pushHeaders             (conditional)  HTTP 1.1 headers
 *          pushHeadersLength       (conditional)  The length of the push headers
 *                                                 buffer.  Required if push headers is
 *                                                 not NULL.
 *          pushBody                (conditional)  Pushed data.
 *          pushBodyLength          (conditional)  The length of the Push Body buffer.
 *                                                 Required if push body is not NULL.
 *
 * This method is invoked by the WAP stack to notify the client that the
 * server is pushing data to it.
 *
 * Again, it is unclear if/how the Palm and Wap stacks can handle callbacks.  This
 * is a placeholder for our need to be able to handle these notifications.
 */
XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_confirmedPush_ind(awsp_SessionHandle sessionHandle,
                            unsigned long      clientPushID,
                            const char        *pushHeaders,
                            size_t             pushHeadersLength,
                            void              *pushBody,
                            size_t             pushBodyLength)
{
   /**
    *  I have NO idea how we communicate to the application that we have push data
    *  for it to receive and process...
    *
    *  At the very least, we need to respond with a push confirmation...
    **/
   return AWSP_RC_OK;
} /* End of awsp_confirmedPush_ind */

/*
 * Suspend Indicator  (callback)
 *    IN:   sessionHandle           (required)  A Handle for the session that this
 *                                              transaction is executing in.
 *          reason                  (required)  The reason the session was suspended
 *
 * This method is invoked by the WAP stack to notify the client that the server
 * has requested that the session be suspended.
 */
XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_suspend_ind(awsp_SessionHandle sessionHandle,
                      awsp_ReasonCode_t  reason)
{
  /**
   * All pending transactions should be cleaned up.
   *
   * Not sure if the connection handle is invalidated by this, but presumably it
   * should be.
   *
   * Also, and data not previously saved for the 'resume' should be saved here?
   *
   * Speaking of which, if the server can suspend than it can presumably resume,
   * so do we need a callback for a resume indicator????  Not clear from the spec
   * if the server can resume - looks like the client would resume if the provider
   * suspended...
   **/

   return AWSP_RC_OK;
} /* End of awsp_suspend_ind() */

/*
 * Push Indicator    (callback)
 *    IN:   connHandle              (required)    A Handle for the initialized service
 *                                                Access Point - see initializeSAP().
 *          pushID                  (required)    A unique identifier to identify he push
 *          pushHeaders             (conditional) Buffer containing the HTTP push headers
 *          pushHeadersLength       (conditional) Length of push headers buffer
 *          pushBody                (conditional) Buffer containing the push body
 *          pushBodyLength          (conditional) Length of push body buffer.
 *
 * This method is invoked by the WAP stack to notify the client that the server is
 * pushing data to it.
 */
XPTEXP1 awsp_Rc_t XPTAPI XPTEXP2 awsp_unit_push_ind(awsp_ConnectionHandle connHandle,
                        unsigned long         pushID,
                        const char           *pushHeaders,
                        size_t                pushHeadersLength,
                        void                 *pushBody,
                        size_t                pushBodyLength)
{
   /**
    *  I have NO idea how we communicate to the application that we have push data
    *  for it to receive and process....
    **/
   return AWSP_RC_OK;
} /* End of awsp_unit_push_ind() */
