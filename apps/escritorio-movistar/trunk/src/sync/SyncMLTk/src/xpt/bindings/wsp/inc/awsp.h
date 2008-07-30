/*************************************************************************/
/* module:          Communication Services, Abstract WSP API             */
/* file:            /src/xpt/all/awsp.h                                  */
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
 * RELEASE ???? CANDIDATE ?
 * 05.06.2000
 */


/**
 * WSP protocol services, function prototypes and return codes
 *
 *
 */

#ifndef AWSP_H
#define AWSP_H
/* 
#ifdef _WIN32
 #ifdef WSP_EXPORTING
  #define WSP_EXPORT __declspec(dllexport)
 #else
  #define WSP_EXPORT __declspec(dllimport)
 #endif
#else
 #define WSP_EXPORT
#endif
*/
#define WSP_EXPORT

/*********************/
/* Required includes */
/*********************/
#include <xptdef.h>


#ifdef __cplusplus
extern "C" {
#endif


/*****************************/
/* Types and Data structures */
/*****************************/

/*
 *  Structure containing the capabilities that can be negotiated during WSP
 *  capability negotiation.
 */

typedef struct {
                  /* List of alternate addresses for current service user      */
/*             char *aliases[]; */
            char **aliases;
            int    aliasesSz;

                  /* Size of the largest transaction service data unit which
                   * may be sent to the client                                 */
   unsigned int   clientSDUsz;

                  /* Set of extended HTTP 1.1 methods supported                */
/*            char *extendedMethods[]; */
            char **extendedMethods;
            int    extendedMethodsSz;

                  /* Set of supported header code page names                   */
/*            char *headerCodePages[]; */
            char **headerCodePages;
            int    headerCodePagesSz;

                  /* Maximum number of active Method requests                  */
   unsigned int   maxOutstandingMethodRequests;

                  /* Maximum number of active Push requests                    */
   unsigned int   maxOutstandingPushRequests;

                  /* Set of optional WSP facilities available for use          */
/*            char *protocolOptions[]; */
            char **protocolOptions;
            int    protocolOptionsSz;

                  /* Size of the largest transaction service data unit that
                   * may be sent to the server                                 */
   unsigned int   serverSDUsz;

} awsp_Capabilities;

typedef void *awsp_ConnectionHandle;
typedef void *awsp_SessionHandle;

typedef enum {
   AWSP_TRUE           = 1,
   AWSP_FALSE          = 0
} awsp_BOOL;

typedef enum {
   AWSP_DEFAULT_BEARER = -1,
   AWSP_USSD           = 0,
   AWSP_CSD            = 1,
   AWSP_CDPD           = 2,
   AWSP_SMS            = 3,
   AWSP_PACKET         = 4,
   AWSP_FLEX_REFLEX    = 5,
   AWSP_GUTS_RDATA     = 6,
   AWSP_GPRS           = 7,
   AWSP_SDS            = 8
   } awsp_BEARER_TYPE;

typedef enum {
   AWSP_DEFAULT_ADDRESS   = -1,
   AWSP_IPV4              = 0,        // RFC 791, header format
   AWSP_IPV6              = 1,        // RFC 1884, header format
   AWSP_GSM_MSISDN        = 2,        // GSM 03.40, SMS address format
   AWSP_IS_136_MSISDN     = 3,        // IS-136, address format
   AWSP_IS_637_MSISDN     = 4,        // IS-637, address format
   AWSP_IDEN_MSISDN       = 5,        // Motorola doc #68P81095E55-A, iDen SMS address format
   AWSP_FLEX_MSISDN       = 6,        // Motorola doc #68P81139B01, FLEX address format
   AWSP_PHS_MSISDN        = 7,        //
   AWSP_GSM_SERVICE_CODE  = 8,        // GSM 02.90, GSM USSD service code address format
   AWSP_TETRA_ITSI        = 9,        // ETS 300 392-1, TETRA SDS address format
   AWSP_TETRA_MSISDN      = 10        // ETS 300 392-1, TETRA SDS address format
   } awsp_ADDRESS_TYPE;


/**************************/
/* Function return values */
/**************************/


/* I'm not sure what these should be yet...put in OK as placeholder */
typedef enum {
   AWSP_RC_OK               = 0,    // Successful request
   AWSP_RC_BUFFER_TOO_SMALL = 4     // Buffer size of output parameter is too small to
                                    // receive the requested data.
} awsp_Rc_t;


typedef enum {
   AWSP_PROTOERR     =  0xE0,    /* Protocol error, illegal PDU                */
   AWSP_DISCONNECT   =  0xE1,    /* Session has been disconnected              */
   AWSP_SUSPEND      =  0xE2,    /* Session has been suspended                 */
   AWSP_RESUME       =  0xE3,    /* Session has been resumed                   */
   AWSP_CONGESTION   =  0xE4,    /* Peer is congested, can't process SDU       */
   AWSP_CONNECTERR   =  0xE5,    /* Session connect failed                     */
   AWSP_MRUEXCEEDED  =  0xE6,    /* Max Receive Unit size exceeded             */
   AWSP_MOREXCEEDED  =  0xE7,    /* Max Outstanding Requests exceeded          */
   AWSP_PEERREQ      =  0xE8,    /* Peer request                               */
   AWSP_NETERR       =  0xE9,    /* Network error                              */
   AWSP_USERREQ      =  0xEA,    /* User request                               */
   AWSP_USERRFS      =  0xEB,    /* No specific cause, no retries              */
   AWSP_USERPND      =  0xEC,    /* Push cannot be delivered to destination    */
   AWSP_USERDCR      =  0xED,    /* Push discarded due to resource shortage    */
   AWSP_USERDCU      =  0xEE     /* Content type cannot be processed           */
} awsp_ReasonCode_t ;

typedef enum {                  // HTTP Status code
   AWSP_CONTINUE                 =  0x10, // 100
   AWSP_SWITCH_PROTOCOL          =  0x11, // 101
   AWSP_SUCCESS                  =  0x20, // 200
   AWSP_CREATED                  =  0x21, // 201
   AWSP_ACCEPTED                 =  0x22, // 202
   AWSP_NON_AUTHORITATIVE_ANSWER =  0x23, // 203
   AWSP_NO_CONTENT               =  0x24, // 204
   AWSP_RESET_CONTENT            =  0x25, // 205
   AWSP_PARTIAL_CONTENT          =  0x26, // 206
   AWSP_MULTIPLE_CHOICES         =  0x30, // 300
   AWSP_MOVED_PERMANENTLY        =  0x31, // 301
   AWSP_MOVED_TEMPORARILY        =  0x32, // 302
   AWSP_SEE_OTHER                =  0x33, // 303
   AWSP_NOT_MODIFIED             =  0x34, // 304
   AWSP_USE_PROXY                =  0x35, // 305
   AWSP_BAD_REQUEST              =  0x40, // 400
   AWSP_UNAUTHORIZED             =  0x41, // 401
   AWSP_PAYMENT_REQUIRED         =  0x42, // 402
   AWSP_FORBIDDEN                =  0x43, // 403
   AWSP_NOT_FOUND                =  0x44, // 404
   AWSP_METHOD_NOT_ALLOWED       =  0x45, // 405
   AWSP_NOT_ACCEPTABLE           =  0x46, // 406
   AWSP_PROXY_AUTH_REQUIRED      =  0x47, // 407
   AWSP_REQUEST_TIMEOUT          =  0x48, // 408
   AWSP_CONFLICT                 =  0x49, // 409
   AWSP_GONE                     =  0x4A, // 410
   AWSP_LENGTH_REQUIRED          =  0x4B, // 411
   AWSP_PRECONDITION_FAILED      =  0x4C, // 412
   AWSP_REQUEST_ENTITY_TOO_LARGE =  0x4D, // 413
   AWSP_REQUEST_URI_TOO_LARGE    =  0x4E, // 414
   AWSP_UNSUPPORTED_MEDIA_TYPE   =  0x4F, // 415
   AWSP_INTERNAL_SERVER_ERROR    =  0x60, // 500
   AWSP_NOT_IMPLEMENTED          =  0x61, // 501
   AWSP_BAD_GATEWAY              =  0x62, // 502
   AWSP_SERVICE_UNAVAILABLE      =  0x63, // 503
   AWSP_GATEWAY_TIMEOUT          =  0x64, // 504
   AWSP_HTTP_VERSION_UNSUPPORTED =  0x65  // 505
} awsp_StatusCode_t;

/*
 * Connect Request
 *    IN:   connHandle              (required)  A Handle for the initialized service
 *                                              Access Point - see initializeSAP().
 *          sessionHandle           (required)  A pointer to receive the handle that
 *                                              represents the session created by this
 *                                              connection.
 *          clientHttpHeaders       (optional)  Static HTTP client headers that will
 *                                              apply to every client transaction
 *                                              during the session.  NULL if no static
 *                                              headers are defined for this session.
 *                                              Header should be in standard HTTP header
 *                                              format (i.e. tags are separated by CRLF,
 *                                              header ends with CRLF CRLF).
 *          clientHttpHeadersLength (optional)  Length of the clientHttpHeaders buffer.
 *                                              Required if clientHttpHeaders is not NULL.
 *          requestedCapabilities   (optional)  Capabilites Negotiation object containing
 *                                              capabilities requested for the session.
 *                                              NULL if no extended capabilities are needed.
 *  OUT:    sessionHandle                       A pointer to the handle that
 *                                              represents the session created by this
 *                                              connection.
 *
 * RETURN:  awsp_Rc_t               A return code indicating whether the connection
 *                                  was successful (i.e. is the session established or not).
 *
 * This method must block until the connect confirmation comes back from the server.
 *
 * Note to implementers:
 * The connect confirmation information needs to remain available until the
 * awsp_get_connect_cnf() method is invoked to retrieve it.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_connect_req(awsp_ConnectionHandle connHandle,
                           awsp_SessionHandle   *sessionHandle,
                           const char           *clientHttpHeaders,
                           const size_t          clientHttpHeadersLength,
                           awsp_Capabilities    *requestedCapabilities);

/*
 * Get Connect Confirmation Information
 *    IN:   sessionHandle                       The handle that represents the
 *                                              session.
 *          serverHttpHeaders                   A buffer to receive whatever static HTTP
 *                                              server headers that were returned with the
 *                                              connect confirmation.
 *                                              A NULL value should cause the method to return
 *                                              an 'insufficient buffer size' error, and
 *                                              serverHttpHeadersLength will have been updated
 *                                              to contain the required buffer size.
 *          serverHttpHeadersLength             Reference to a size_t variable that contains
 *                                              the length of the serverHttpHeaders buffer.
 *          negotiatedCapabilities              The address of a pointer to a Capabilities object.
 *                                              Required only when connect request included
 *                                              capabilities request.
 *
 *   OUT:   serverHttpHeaders                   The static HTTP server headers that will apply
 *                                              to every server transaction during the
 *                                              session.  NULL if no static headers were
 *                                              returned on the confirmation. Header should be
 *                                              in standard HTTP header format (i.e. tags are
 *                                              separated by CRLF, header ends with CRLF CRLF).
 *          serverHttpHeadersLength             Reference to a size_t variable that contains
 *                                              the length of the data returned in
 *                                              the serverHttpHeaders buffer.
 *          negotiatedCapabilities              The address has been updated to contain a
 *                                              pointer to the negotiated capabilities
 *                                              object containing what the server has
 *                                              agreed to for the session.
 *
 * RETURN:  awsp_Rc_t               A return code indicating whether the confirmation
 *                                  information has been returned in the passed parameters.
 *                                  Returns a 'buffer too small' response if the provided
 *                                  buffers are NULL, or are too small to receive the
 *                                  results.
 *
 *  This method is invoked to retrieve the information that was returned on the connect
 *  confirmation.  It is a non-blocking call - either the information is available
 *  or it is not.  This method should be invoked following successful return from
 *  awsp_connect_req().
 *  If the initial invocation of the method results in a 'buffer too small' error, the
 *  method should be able to be invoked again for the same transaction with the correct
 *  buffer sizes to retrieve the information.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_get_connect_cnf(awsp_SessionHandle    sessionHandle,
                               const char           *serverHttpHeaders,
                               size_t               *serverHttpHeadersLength,
                               awsp_Capabilities **negotiatedCapabilities);


/*
 * Disconnect Request
 *    IN:   sessionHandle           (required)        A Handle for the session that this
 *                                                    connection represents.
 *          reasonCode              (required)        Cause of the disconnection -
 *                                                    either a reason code or a status code
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
 *                                                    body information.  Optional, use only
 *                                                    if an Error Body will be provided to
 *                                                    expand upon the reason for the
 *                                                    disconnect.
 *          errorHeadersLength      (optional)        Length of Error Headers buffer.
 *                                                    Required if Error Headers is not NULL.
 *          errorBody               (optional)        Info to expand on reason code.
 *                                                    Required if Error Headers is not NULL
 *          errorBodyLength         (optional)        Length of Error Body buffer.
 *                                                    Required if Error Body is not NULL.
 *
 * RETURN:  awsp_Rc_t               A return code indicating whether the disconnect has
 *                                  been successfully sent to the server.
 *
 *  The client uses this method to request that the session be terminated..
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_disconnect_req(awsp_SessionHandle sessionHandle,
                              int                reasonCode,
                              awsp_BOOL          redirectSecurity,
                              const char        *redirectAddresses[],
                              const char        *errorHeaders,
                              size_t             errorHeadersLength,
                              void              *errorBody,
                              size_t             errorBodyLength);

/*
 * Disconnect Indicator in response to Request (callback)
 *    IN:   connHandle              (required)        A Handle for the initialized service
 *                                                    Access Point - see initializeSAP().
 *          reasonCode              (required)        Cause of the disconnection -
 *                                                    either a reason code or a status code
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
WSP_EXPORT awsp_Rc_t XPTAPI awsp_disconnect_ind(awsp_ConnectionHandle connHandle,
                         int                   reasonCode,
                         awsp_BOOL             redirectSecurity,
                         const char           *redirectAddresses[],
                         const char           *errorHeaders,
                         size_t                errorHeadersLength,
                         void                 *errorBody,
                         size_t                errorBodyLength);

/*
 ******************************************************************************
 *
 * Method Invocation Facility
 *       HTTP Methods and user-defined request/response model operations
 *
 ******************************************************************************
 */

/*
 * Method Invoke Request
 *    IN:   sessionHandle           (required)     A Handle for the session that this
 *                                                 transaction is executing in.
 *          clientTransactionID     (required)     Unique ID to distinguish between pending transactions
 *          method                  (required)     HTTP method or one of negotiated extended methods
 *          requestURI              (required)     The entity to which the operation applies -
 *          requestHeaders          (optional)     HTTP 1.1 headers
 *          requestHeadersLength    (optional)     Length of Request Headers buffer.  Required
 *                                                 if Request Headers is not NULL.
 *          requestBody             (conditional)  If Method transmits data, this contains
 *                                                 the data to be transmitted (i.e. for
 *                                                 PUT/POST, this is the SyncML message).
 *                                                 If method does not transmit data (i.e. GET)
 *                                                 then this should be NULL.
 *          requestBodyLength       (conditional)  Length of the Request Body buffer.  Required
 *                                                 if Request Body is not NULL.
 *
 *  This method must block until the server responds with the method result indication.
 *
 *  Note to implementers:
 *  The result information needs to remain available until the awsp_get_methodResult_ind()
 *  method is invoked to retrieve it.
 *
 *  Note:  The WAP specification allows new requests to be made before responses to existing
 *         requests are returned, and the responses are not required to come back in the
 *         same order as the requests were sent.  Based on this, it seems incorrect to make
 *         this API block and wait for the response.  This synchronous technique was chosen
 *         because of the Palm threading model, which we believe may be incapable of
 *         handling the multiple asynchronous request model.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_methodInvoke_req(awsp_SessionHandle sessionHandle,
                           unsigned long      clientTransactionID,
                           const char        *method,
                           const char        *requestURI,
                           const char        *requestHeaders,
                           size_t             requestHeadersLength,
                           void              *requestBody,
                           size_t             requestBodyLength);

/*
 * Get Method Result Indicator Information
 *    IN:   sessionHandle           (required)    A Handle for the session that this
 *                                                transaction is executing in.
 *          clientTransactionID     (required)    Unique ID to distinguish between pending transactions
 *          status                  (required)    A reference to an int field to receive the
 *                                                HTTP 1.1 status code
 *          responseHeaders         (conditional) A buffer to receive the HTTP 1.1 header, or
 *                                                NULL to query the buffer size needed to
 *                                                receive the header.
 *          responseHeadersLength   (conditional) Reference to a size_t variable that contains
 *                                                the length of the Response Headers buffer.
 *          responseBody            (conditional) A buffer to receive the Response data, or
 *                                                NULL to query the buffer size needed to
 *                                                receive the body.
 *          responseBodyLength      (conditional) Reference to a size_t variable that contains
 *                                                the length of the Reponse Body buffer.
 *   OUT:   status                  (required)    HTTP 1.1 status code
 *          responseHeaders         (conditional) If status includes response information,
 *                                                buffer has been updated to contain the
 *                                                HTTP 1.1 response header
 *          responseHeadersLength   (conditional) Updated to contain the length of the
 *                                                response header, or 0 if no response
 *                                                is available for this status.
 *          responseBody            (conditional) If status includes response information,
 *                                                buffer has been updated to contain the
 *                                                response data (or error info).
 *          responseBodyLength      (conditional) Updated to contain the length of the
 *                                                response body, or 0 if no response
 *                                                is available for this status.
 *
 * RETURN:  awsp_Rc_t               A return code indicating whether the indication
 *                                  information has been returned in the passed parameters.
 *                                  Returns a 'buffer too small' response if the provided
 *                                  buffers are NULL, or are too small to receive the
 *                                  results.
 *
 *  This method is invoked to retrieve the information that was returned on the method
 *  response indication. It is a non-blocking call - either the information is available
 *  or it is not.  This method should be invoked following successful return from
 *  awsp_methodInvoke_req().
 *
 *  Note to implementers:
 *  If the initial invocation of the method results in a 'buffer too small' error, the
 *  method should be able to be invoked again for the same transaction with the correct
 *  buffer sizes to retrieve the information.
 *
 *  Both the responseHeaders and responseBody buffers need to be valid and large enough
 *  to accomodate the data in order for any result data to be returned.  If either is
 *  missing or invalid, it should be assumed that this is a request for buffer size
 *  information.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_get_methodResult_ind(awsp_SessionHandle  sessionHandle,
                                    unsigned long       clientTransactionID,
                                    awsp_StatusCode_t  *status,
                                    char               *responseHeaders,
                                    size_t             *responseHeadersLength,
                                    void               *responseBody,
                                    size_t             *responseBodyLength);



/*
 * Method Result Response
 *    IN:   sessionHandle                  (required) A Handle for the session that this
 *                                                    transaction is executing in.
 *          clientTransactionID            (required) The transaction ID
 *          acknowledgementHeaders         (optional) Used to return info to the server.
 *          acknowledgementHeadersLength   (optional) Length of the acknowledgement
 *                                                    headers buffer.  Required if
 *                                                    buffer is not NULL.
 *
 * This method is invoked by the client to acknowledge that it has received the
 * server response to it's method invoke request.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_methodResult_rsp(awsp_SessionHandle sessionHandle,
                           unsigned long      clientTransactionID,
                           void              *acknowledgementHeaders,
                           size_t             acknowledgementHeadersLength);

/*
 * Method Abort Indicator (callback)
 *    IN:   sessionHandle           (required)     A Handle for the session that this
 *                                                 transaction is executing in.
 *          transactionID           (required)     A unique ID to identify this transaction.
 *          reasonCode              (required)     Reason for the abort
 *
 * This method is invoked by the WAP stack to notify the client that the
 * server has aborted the method invocation request.
 *
 * Again, it is unclear if/how the Palm and Wap stacks can handle callbacks.  This
 * is a placeholder for our need to be able to handle these notifications.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_methodAbort_ind(awsp_SessionHandle sessionHandle,
                          unsigned long      transactionID,
                          awsp_ReasonCode_t  reasonCode);


/*
 ******************************************************************************
 *
 * Push Facility
 *       Sends unsolicited data from server to client across session
 *       (no confirmation of data arrival)
 *
 ******************************************************************************
 */

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
WSP_EXPORT awsp_Rc_t XPTAPI awsp_push_ind(awsp_SessionHandle sessionHandle,
                   const char        *pushHeaders,
                   size_t             pushHeadersLength,
                   void              *pushBody,
                   size_t             pushBodyLength);


/*
 ******************************************************************************
 *
 * Confirmed Push Facility
 *       Sends unsolicited data from server to client across session
 *       (confirms client receipt of the data)
 *
 ******************************************************************************
 */

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
WSP_EXPORT awsp_Rc_t XPTAPI awsp_confirmedPush_ind(awsp_SessionHandle sessionHandle,
                            unsigned long      clientPushID,
                            const char        *pushHeaders,
                            size_t             pushHeadersLength,
                            void              *pushBody,
                            size_t             pushBodyLength);

/*
 * Confirmed Push Response
 *    IN:   sessionHandle           (required)        A Handle for the session that this
 *                                                    transaction is executing in.
 *          clientPushID            (required, equal) Unique Identified for this push transaction
 *          acknowledgementHeaders  (optional)        HTTP 1.1 headers to return data to server
 *          acknowledgementHeadersLength   (optional) Length of the Acknowledgement Headers
 *                                                    buffer.  Required if buffer is not NULL.
 *
 * This method is invoked by the client to acknowledge receipt of the pushed data from the
 * server.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_confirmedPush_rsp(awsp_SessionHandle sessionHandle,
                            unsigned long      clientPushID,
                            void              *acknowledgementHeaders,
                            size_t             acknowledgementHeadersLength);

/*
 ******************************************************************************
 *
 * Session Resume Facility
 *       Allows suspension and resumption of a session.  Can be used to
 *       switch to an alternate bearer network.
 *
 ******************************************************************************
 */

/*
 * Suspend Request
 *    IN:   sessionHandle           (required)  A Handle for the session that this
 *                                              transaction is executing in.
 *
 * RETURN:     awsp_Rc_t   indicates whether the suspend completed successfully.
 *
 * This method is invoked by the client to suspend the current session.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_suspend_req(awsp_SessionHandle sessionHandle);

/*
 * Suspend Indicator  (callback)
 *    IN:   sessionHandle           (required)  A Handle for the session that this
 *                                              transaction is executing in.
 *          reason                  (required)  The reason the session was suspended
 *
 * This method is invoked by the WAP stack to notify the client that the server
 * has requested that the session be suspended.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_suspend_ind(awsp_SessionHandle sessionHandle,
                      awsp_ReasonCode_t  reason);

/*
 * Resume Request
 *    IN:   connHandle              (required)  A Handle for the initialized service
 *                                              Access Point - see initializeSAP().
 *          clientHeaders           (optional)  HTTP 1.1 Headers to use throughout session
 *          clientHeadersLength     (optional)  Length of Client Headers buffer.  Required
 *                                              if buffer is not NULL.
 *
 * RETURN:  awsp_Rc_t               an indication of whether the resume was successful.
 *
 * This method is invoked by the client to resume a suspended session.
 * This method must block until the server confirmation that the session has resumed.
 *
 *  Note to implementers:
 *  The confirmation information needs to remain available until the awsp_get_resume_cnf()
 *  method is invoked to retrieve it.
 *
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_resume_req(awsp_ConnectionHandle connHandle,
                          const char           *clientHeaders,
                          size_t                clientHeadersLength);

/*
 * Get Resume Confirmation information.
 *
 *    IN:  connHandle              (required)    A Handle for the initialized service
 *                                               Access Point - see initializeSAP().
 *         sessionHandle           (required)    A pointer to receive the handle for
 *                                               the session that is being resumed.
 *         serverHeaders           (conditional) A buffer to receive the HTTP 1.1 static
 *                                               headers that the server wants to use
 *                                               for every request in the session.
 *                                               NULL to query the size the buffer needs
 *                                               to be
 *         serverHeadersLength     (conditional) A reference containing the length of the
 *                                               server headers buffer.
 *  OUT:   sessionHandle           (required)    A Handle for the session that is resumed.
 *         serverHeaders           (conditional) If buffer is sufficient for data, updated
 *                                               to contain the server headers.
 *         serverHeadersLength     (conditional) Updated to contain the length of the
 *                                               Server Headers data.
 *
 * RETURN:  awsp_Rc_t               A return code indicating whether the confirmation
 *                                  information has been returned in the passed parameters.
 *                                  Returns a 'buffer too small' response if the provided
 *                                  buffers are NULL, or are too small to receive the
 *                                  results.
 *
 *  This method is invoked to retrieve the information that was returned on the resume
 *  confirmation. It is a non-blocking call - either the information is available
 *  or it is not.  This method should be invoked following successful return from
 *  awsp_resume_req().
 *
 *  Note to implementers:
 *  If the initial invocation of the method results in a 'buffer too small' error, the
 *  method should be able to be invoked again for the same transaction with the correct
 *  buffer sizes to retrieve the information.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_get_resume_cnf(awsp_ConnectionHandle connHandle,
                              awsp_SessionHandle    sessionHandle,
                              char                 *serverHeaders,
                              size_t               *serverHeadersLength);


/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         WSP ConnectionLess Functions                    **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/


/*
 ******************************************************************************
 *
 * Method Invocation Facility
 *       HTTP Methods and user-defined request/response model operations
 *
 ******************************************************************************
 */

/*
 * Method Invoke Request
 *    IN:   connHandle              (required)     A Handle for the initialized service
 *                                                 Access Point - see initializeSAP().
 *          transactionID           (required)     A unique ID to identify this transaction.
 *          method                  (required)     The HTTP method for the request.
 *          requestURI              (required)
 *          requestHeaders          (optional)     If method type includes data, this
 *                                                 is the request header for the data.
 *          requestHeadersLength    (conditional)  The length of the request headers
 *                                                 buffer.  Required if the buffer is not NULL.
 *          requestBody             (conditional)  If method type includes data, this
 *                                                 is the data for the method.
 *          requestBodyLength       (conditional)  Length of Request Body buffer.
 *                                                 Required if buffer is not NULL.
 *
 * This method must block until the server methodResult indicator comes back.
 *
 *  Note to implementers:
 *  The indicator information needs to remain available until the
 *  awsp_get_unit_methodResult_ind() method is invoked to retrieve it.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_unit_methodInvoke_req(awsp_ConnectionHandle connHandle,
                                unsigned long         transactionID,
                                const char           *method,
                                const char           *requestURI,
                                const char           *requestHeaders,
                                size_t                requestHeadersLength,
                                void                 *requestBody,
                                size_t                requestBodyLength);

/*
 * Method Result Indicator
 *    IN:   connHandle              (required)     A Handle for the initialized service
 *                                                 Access Point - see initializeSAP().
 *          transactionID           (required)     A unique ID to identify this transaction.
 *          status                  (required)     An integer reference to receive the
 *                                                 HTTP status of the request
 *          responseHeaders         (conditional)  A buffer to receive the HTTP response
 *                                                 headers, or NULL to query how large
 *                                                 the buffer needs to be.
 *          responseHeadersLength   (conditional)  A reference containing the length of
 *                                                 the response headers buffer.
 *          responseBody            (conditional)  A buffer to receive the HTTP response
 *                                                 body, or NULL to query how large the
 *                                                 buffer needs to be.
 *          responseBodyLength      (conditional)  A reference containing the length of
 *                                                 the response body buffer.
 *   OUT:   status                  (required)     Updated to contain the HTTP response status
 *          responseHeaders         (conditional)  If buffer is large enough to hold response
 *                                                 header, buffer is updated to contain
 *                                                 response header
 *          responseHeadersLength   (conditional)  Length of response header
 *          responseBody            (conditional)  If buffer is large enough to hold response
 *                                                 body, buffer is updated to contain response
 *                                                 body.
 *          responseBodyLength      (conditional)  Length of response body
 *
 * RETURN:  awsp_Rc_t               A return code indicating whether the indication
 *                                  information has been returned in the passed parameters.
 *                                  Returns a 'buffer too small' response if the provided
 *                                  buffers are NULL, or are too small to receive the
 *                                  results.
 *
 *  This method is invoked to retrieve the information that was returned on the unit
 *  method response indication. It is a non-blocking call - either the information is
 *  available or it is not.  This method should be invoked following successful return from
 *  awsp_unit_methodInvoke_req().
 *
 *  Note to implementers:
 *  If the initial invocation of the method results in a 'buffer too small' error, the
 *  method should be able to be invoked again for the same transaction with the correct
 *  buffer sizes to retrieve the information.
 */
WSP_EXPORT awsp_Rc_t XPTAPI awsp_get_unit_methodResult_ind(awsp_ConnectionHandle connHandle,
                                         unsigned long         transactionID,
                                         awsp_StatusCode_t    *status,
                                         char                 *responseHeaders,
                                         size_t               *responseHeadersLength,
                                         void                 *responseBody,
                                         size_t               *responseBodyLength);



/*
 ******************************************************************************
 *
 * Push Facility
 *       Sends unsolicited data from server to client across session
 *       (no confirmation of data arrival)
 *
 ******************************************************************************
 */

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
WSP_EXPORT awsp_Rc_t XPTAPI awsp_unit_push_ind(awsp_ConnectionHandle connHandle,
                        unsigned long         pushID,
                        const char           *pushHeaders,
                        size_t                pushHeadersLength,
                        void                 *pushBody,
                        size_t                pushBodyLength);


/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**      Management Entity Functions for Service Access Point               **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

/*
 * Initialize the service access point for use.
 *    IN:
 *          connectionHandle        (required) A reference to a connection handle
 *                                             pointer to receive the connection
 *                                             handle
 *    OUT:
 *          connectionHandle        (required) Updated to contain a pointer to the
 *                                             connection handle for this service
 *                                             access point.
 *
 * RETURN:  awsp_Rc_t               A return code indicating whether the service
 *                                  access point has been successfully initialized.
 *
 *  This method is used to initialize the Service Access Point and prepare it for
 *  establishing a WSP session.  No input parameters are specified, which means the WAP
 *  stack should connect to the default server at the default port, using the
 *  default bearer.  It is assumed that the current client can be determined by the
 *  method, and the default client port should be used.
 */

WSP_EXPORT awsp_Rc_t XPTAPI initializeSAPd(awsp_ConnectionHandle *connectionHandle);

/*
 * Initialize the service access point for use.
 *    IN:
 *          connectionHandle        (required) A reference to a connection handle
 *                                             pointer to receive the connection
 *                                             handle
 *          bearerType              (required) Use DEFAULT type for default bearer
 *          addressType             (required) Use DEFAULT type for default bearer
 *          serverAddress           (optional) Uses addressing scheme of layer
 *                                  below it.  May be MSISDN, X.25, ip address,
 *                                  or other.
 *             MSISDN number = 3 digit country code +
 *                             2 digit national destination code +
 *                    (up to) 10 digit subscriber number
 *             X.25 X.121 #  = 3 digit country code +   \ this is the DNIC and
 *                             1 digit PSN +            / is optional
 *                    (up to) 10 digit NTN
 *             ip address
 *
 *          serverPort              (optional)
 *          clientAddress           (optional)
 *          clientPort              (optional)
 *    OUT:
 *          connectionHandle        (required) Updated to contain a pointer to the
 *                                             connection handle for this service
 *                                             access point.
 *
 * RETURN:  awsp_Rc_t               A return code indicating whether the service
 *                                  access point has been successfully initialized.
 *
 *  This method is used to initialize the Service Access Point and prepare it for
 *  establishing a WSP session.  It allows input parameters to be specified, for the
 *  case where the client application needs to specify access point information.
 */

WSP_EXPORT awsp_Rc_t XPTAPI initializeSAP(awsp_ConnectionHandle *connectionHandle,
                        awsp_BEARER_TYPE       bearerType,
                        awsp_ADDRESS_TYPE      addressType,
                        const char            *serverAddress,
                        unsigned short         serverPort,
                        const char            *clientAddress,
                        unsigned short         clientPort);

/*
 * Close the service access point after usage.
 *    IN:
 *          connectionHandle        (required)
 */
WSP_EXPORT awsp_Rc_t XPTAPI closeSAP(awsp_ConnectionHandle connectionHandle);


#ifdef __cplusplus
}
#endif

#endif

