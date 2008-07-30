#ifndef OBEXCONSTANTS_H
#define OBEXCONSTANTS_H

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

#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE   1
#endif
/*
** Version
*/
#define OBEX_VERSION		            0x12           /* Version 1.2 */

//Mickey 2003.1.29
/*
** Target
*/
#define SYNCML_TARGET "SYNCML-SYNC"		/* UUID for SyncML. This is used to identify SyncML service on OBEX. */

/*
** Standard Obex headers
**
** Reserved and user defined headers
**   0x10 to 0x2F RESERVED this range includes all combinations of the upper 2 bits
**   0x30 to 0x3F USER DEFINED this range includes all combinations of the upper 2 bits
*/
#define OBEX_HEADER_COUNT	        0xC0 /* Number of objects (used by connect)  */
#define OBEX_HEADER_NAME	        0x01 /* Name of the object                   */
#define OBEX_HEADER_TYPE	        0x42 /* Type of the object                   */
#define OBEX_HEADER_TIME	        0x44 /* Last modification of object          */
#define OBEX_HEADER_TIME2	        0xC4 /* This value is also allowed for time  */
#define OBEX_HEADER_LENGTH	        0xC3 /* Total length of object               */
#define OBEX_HEADER_DESCRIPTION	    0x05 /* Description of object                */
#define OBEX_HEADER_TARGET	        0x46 /* Identifies the target for the object */
#define OBEX_HEADER_HTTP            0x47 /* An HTTP Header.                      */
#define OBEX_HEADER_BODY	        0x48 /* Data part of the object (body)       */
#define OBEX_HEADER_BODY_END	    0x49 /* Last data part of the obj (body end) */
#define OBEX_HEADER_WHO		        0x4A /* Identifies the sender of the object  */
#define OBEX_HEADER_CONNECTION	    0xCB /* Connection identifier                */
#define OBEX_HEADER_APP_PARM        0x4C /* extended application req & resp info */
#define OBEX_HEADER_AUTH_CHALLENGE  0x4D /* Challenge auth digest-challenge      */
#define OBEX_HEADER_AUTH_RESPONSE   0x4E /* Authentication digest-response       */
#define OBEX_HEADER_OBJECT          0x4F /* OBEX Object class of object          */
#define OBEX_HEADER_CREATORID       0xCF /* Creator id                           */

#define OBEX_HEADER_XSYNCMLHMAC     0x30 /* HMAC string for field x-syncml-hmac  */

/*
** Obex Header encoding schemes
*/
#define OBEX_HEADER_ENCODING_MASK        0xc0
#define OBEX_HEADER_ENCODING_UNICODE     0x00
#define OBEX_HEADER_ENCODING_BYTE_SEQ    0x40
#define OBEX_HEADER_ENCODING_BYTE        0x80
#define OBEX_HEADER_ENCODING_INT         0xc0

/*
** Obex Commands
**
** 0x06 to 0x0F Reserved not to be used w/out extension to this specification
** 0x10 to 0x1F User definable use as you please with peer application
*/
#define OBEX_CMD_CONNECT      0x80    /* Establish a connection with Obex peer*/
#define OBEX_CMD_DISCONNECT   0x81    /* Disconnect a connection.             */
#define OBEX_CMD_PUT	         0x02    /* Send a document to peer.             */
#define OBEX_CMD_GET		      0x03    /* Request a document from peer.        */
#define OBEX_CMD_COMMAND	   0x04    /* *RESERVED*                           */
#define OBEX_CMD_SETPATH	   0x85    /* Alter current directory of receiver. */
#define OBEX_CMD_ABORT		   0xFF    /* Abort current task.                  */
#define OBEX_CMD_FINAL		   0x80    /* Final packet Mask                    */

/*
** Command specific flags bit settings
*/
#define OBEX_SETPATH_BACKUP_LEVEL 0x80 /* backup level before applying (eq. to ../ on many systems) */
#define OBEX_SETPATH_NO_CREATE    0x40 /* Don’t create dir if it doesn't exist, ret error instead.  */

/*
** Obex Responses
**
** The response code contains the HTTP status code (a 3 digit ASCII encoded positive integer)
** encoded in the low order 7 bits as an unsigned integer (the code in parentheses has the
** Final bit set). See the HTTP document for complete descriptions of each of these codes.
** The most commonly used response codes are 0x90 (0x10 Continue with Final bit set, used
** in responding to non-final request packets), and 0xA0 (0x20 Success w/Final bit set,
** used at end of successful operation).
** Note: 0x00 to 0x0F reserved
*/
#define OBEX_RSP_CONTINUE                 0x10  /*  Continue                                 */
#define OBEX_RSP_SWITCH_PRO		         0x11  /*  ????                                     */
#define OBEX_RSP_SUCCESS                  0x20  /*  OK, Success                              */
#define OBEX_RSP_CREATED                  0x21  /*  Created                                  */
#define OBEX_RSP_ACCEPTED                 0x22  /*  Accepted                                 */
#define OBEX_RSP_NON_AUTHOR_INFO          0x23  /*  Non-Authoritative Information            */
#define OBEX_RSP_NO_CONTENT               0x24  /*  No Content                               */
#define OBEX_RSP_RESET_CONTENT            0x25  /*  Reset Content                            */
#define OBEX_RSP_PARTIAL_CONTENT          0x26  /*  Partial Content                          */
#define OBEX_RSP_MULTIPLE_CHOICES         0x30  /*  Multiple Choices                         */
#define OBEX_RSP_MOVED_PERM               0x31  /*  Moved Permanently                        */
#define OBEX_RSP_MOVED_TEMP               0x32  /*  Moved temporarily                        */
#define OBEX_RSP_SEE_OTHER                0x33  /*  See Other                                */
#define OBEX_RSP_NOT_MODIFIED             0x34  /*  Not modified                             */
#define OBEX_RSP_USE_PROXY                0x35  /*  Use Proxy                                */
#define OBEX_RSP_BAD_REQUEST              0x40  /*  Bad Request - svr couldn’t understand    */
#define OBEX_RSP_UNAUTHORIZED             0x41  /*  Unauthorized                             */
#define OBEX_RSP_PAYMENT_REQUIRED         0x42  /*  Payment required                         */
#define OBEX_RSP_FORBIDDEN                0x43  /*  Forbidden - understood but refused       */
#define OBEX_RSP_NOT_FOUND                0x44  /*  Not Found                                */
#define OBEX_RSP_METHOD_NOT_ALLOWED       0x45  /*  Method not allowed                       */
#define OBEX_RSP_NOT_ACCEPTABLE           0x46  /*  Not Acceptable                           */
#define OBEX_RSP_PROXY_AUTH_REQ           0x47  /*  Proxy Authentication required            */
#define OBEX_RSP_REQUEST_TIME_OUT         0x48  /*  Request Time Out                         */
#define OBEX_RSP_CONFLICT                 0x49  /*  Conflict                                 */
#define OBEX_RSP_GONE                     0x4A  /*  Gone                                     */
#define OBEX_RSP_LENGTH_REQUIRED          0x4B  /*  Length Required                          */
#define OBEX_RSP_PRECONDITON_FAILED       0x4C  /*  Precondition failed                      */
#define OBEX_RSP_REQ_ENTITY_TOO_LARGE     0x4D  /*  Requested entity too large               */
#define OBEX_RSP_REQUEST_URL_TOO_LARGE    0x4E  /*  Request URL too large                    */
#define OBEX_RSP_UNSUPPORTED_MEDIA_TYP    0x4F  /*  Unsupported media type                   */
#define OBEX_RSP_INTERNAL_SERVER_ERROR    0x50  /*  Internal Server Error                    */
#define OBEX_RSP_NOT_IMPLEMENTED          0x51  /*  Not Implemented                          */
#define OBEX_RSP_BAD_GATEWAY              0x52  /*  Bad Gateway                              */
#define OBEX_RSP_SERVICE_UNAVAIL          0x53  /*  Service Unavailable                      */
#define OBEX_RSP_GATEWAY_TIMEOUT          0x54  /*  Gateway Timeout                          */
#define OBEX_RSP_HTTP_VERSION_UNSUPPORT   0x55  /*  HTTP version not supported               */
#define OBEX_RSP_DATABASE_FULL            0x60  /*  Database Full                            */
#define OBEX_RSP_DATABASE_LOCKED          0x61  /*  Database Locked                          */

/*
** Predefined transports
*/
#define DEFINED_TRANSPORT_IRDA            1         /* Obex over IrDA    */
#define DEFINED_TRANSPORT_INET            2         /* Obex over Inet    */
#define DEFINED_TRANSPORT_BLUETOOTH       3         /* Someday           */

/*
** Define some common defaults for our provided transport services.
*/
#define OBEX_PORT                         650
#define OBEX_DEFAULT_SERVICE              "OBEX"

/*
** MTU max and min
*/
#define OBEX_DEFAULT_PACKET_LENGTH        1024
#define OBEX_MINIMUM_PACKET_LENGTH        255

/*
** Internal states for objects and headers.
*/
typedef enum {
	STATE_BUILDING,
   STATE_SENDING,
   STATE_SENT,
   STATE_ERROR
} ObxSendState;

typedef int                               ObxRc;
typedef unsigned char                     ObxCommand;
typedef struct iobxListNode               ObxListNode;
typedef struct iobxList                   ObxList;
typedef struct iobxIterator               ObxIterator;
typedef struct iobxHandle                 ObxHandle;
typedef struct iobxCommonFrameHeader      ObxCommonFrameHeader;
typedef struct iobxConnectHeader          ObxConnectHeader;
typedef struct iobxHeader                 ObxHeader;
typedef struct iobxTransport              ObxTransport;
typedef struct iobxObject                 ObxObject;
typedef struct iobxBuffer                 ObxBuffer;
typedef struct iobxConnectMeta            ObxConnectMeta;
typedef struct iobxSetPathMeta            ObxSetPathMeta;
typedef struct iobxStream                 ObxStream;

/*
**************************************************************************
**
** Meta information
**
** Some headers have 'meta' data that is appropriate.
** Two cases of these involve the CONNECT request and SETPATH request.
**
**************************************************************************
*/
struct iobxConnectMeta {
   unsigned char  version;
   unsigned char  flags;
   short          max_packet_length;
};

struct iobxSetPathMeta {
   unsigned char  flags;
   unsigned char  constants;
};

typedef union {
      ObxConnectMeta *connectMeta;  /* Fields associated with a connect object.        */
      ObxSetPathMeta *setPathMeta;  /* Fields associated with a set path object.       */
   } ObjectMeta;

/*
**************************************************************************
**
** Obex Object
**
** Represents an obex object (i.e. connect request, response ect.).
** Obex objects consist of indentifiers and a list of headers.
**
**************************************************************************
*/
struct iobxObject {
   ObxList        *headers;         /* List of headers being sent or received.   */
   ObxCommand     cmd;			      /* Command associated with this object.      */
   ObxSendState   state;            /* Building,sending,sent                     */
   ObjectMeta     meta;             /* Some commands have meta data associated   */
   short          stream;           /* Streaming object.                         */
};

/*
**************************************************************************
**
** Obex Header
**
** Obex headers are structures containing all required info to form a header...
** bytes.. len..identiers.. etc.  Each object can contain several of these.
**
**************************************************************************
*/
struct iobxHeader {
	unsigned char  identifier;          /* Header id                        */
   ObxSendState   state;               /* private: Building,sending,sent   */
   union {
      unsigned int   fourBytevalue;    /* OBEX_HEADER_ENCODING_INT         */
      unsigned char  byteValue;        /* OBEX_HEADER_ENCODING_BYTE        */
      ObxBuffer      *unicodeValue;    /* OBEX_HEADER_ENCODING_UNICODE     */
      ObxBuffer      *byteSequenceValue; /* OBEX_HEADER_ENCODING_BYTE_SEQ  */
   } value;                            /* Depends on header encoding       */
};

/*
**************************************************************************
** Obex transport
**
** Defines a structure representing a base transport to be used.
** Several pre-defined transports are available.  Exposing this struct allows
** authors to register custom transports instead of using a pre-defined
** transport block.
**
**************************************************************************
*/
struct iobxTransport {

   /* ************************************** */
   /* Init/terminate transport wide          */
   /* ************************************** */

   /*
   ** Initialize the transport.  The inbound meta data will differ for each
   ** transport type.  Should be called once, prior to any other calls.
   */
   ObxRc (*initialize)( const char *meta );

   /*
   ** Clean up all internals
   */
   ObxRc (*terminate)( void );

   /*
   ** Create a connection
   */
   ObxRc (*open)( void **connectionid );

   /* ************************************** */
   /* When transport is acting as server     */
   /* ************************************** */

   /*
   ** Do any preperation for accepting inbound connections (i.e. acting as a server).
   ** For the INET transport this would include a bind() and listen().  Other transports
   ** may have other needs.
   */
   ObxRc (*listen)( void **connectionid );

   /*
   ** Accept an inbound connection from a peer transport.  This call should block until
   ** a connection has been established.
   ** When a connection has been accepted, the passed 'connectionid' is set.  This will be
   ** provided by the caller on all subsuquent calls made against the active connection.
   */
   ObxRc (*accept)( void **connectionid );

   /* ************************************** */
   /* When transport is acting as client     */
   /* ************************************** */

   /*
   ** Initiate a connection to a remote peer transport.
   ** When a connection has been created, the passed 'connectionid' is set.  This will be
   ** provided by the caller on all subsuquent calls made against the active connection.
   */
   ObxRc (*connect)( void **connectionid );

   /* ************************************** */
   /* Functions used on connected transports */
   /* ************************************** */

   /*
   ** Send 'length' bytes of data from 'buf', set the actual number
   ** written in 'wrote'.
   ** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
   */
   ObxRc (*send)( void **connectionid, const void *buf, int length, int *wrote, short allowShort );

   /*
   ** Receive 'length' bytes of data and place into 'buf', set the actual
   ** number of bytes read in 'actual'.
   ** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
   */
   ObxRc (*recv)( void **connectionid, void *buf, int length, int *actual, short allowShort );

   /*
   ** Clean up all internals, subsuquent use of this 'connectionid' should result in an error.
   ** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
   */
   ObxRc (*close)( void **connectionid );
};


#endif
