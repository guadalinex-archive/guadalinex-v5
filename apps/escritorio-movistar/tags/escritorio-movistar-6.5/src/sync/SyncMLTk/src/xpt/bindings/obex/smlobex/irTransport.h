#ifndef OBEXIRTRANSPORT_H
#define OBEXIRTRANSPORT_H
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
**************************************************************************
** Ir Transport
**************************************************************************
*/
#include <iConstants.h>

#ifdef _WIN32
 #ifndef _WIN32_WINNT
  #define _WIN32_WINNT   /* Affects expansion of af_irda.h header file      */
 #endif
 #include <af_irda.h>
 #define sockaddr_irda _SOCKADDR_IRDA
 #define sir_family  irdaAddressFamily
 #define sir_name    irdaServiceName
 #define sir_addr    irdaDeviceID
#elif defined(__PALM_OS__)
  // %%% added by luz
  #error "IrDA not available for PalmOS yet"
#else
 #include <linux/types.h>
 #include <linux/irda.h>
 #define    MAX_DEVICES    10
#endif
#ifdef __cplusplus
extern "C" {
#endif


/*
** Defines a connection.
** This block is created by an open() call and is set as the 'connectionid' in
** the callers arg list.  It's returned on subsuquent calls to functions.
*/
typedef struct iobxIrConnectionBlock ObxIrConnectionBlock;
struct iobxIrConnectionBlock {
   int                  fd;               /* Socket FD (client)   */
   short                connected;        /* Do we have a peer?   */
   struct sockaddr_irda peer;             /* 'the other side'     */

};


/*
** Server block
*/
typedef struct iobxIrServerBlock ObxIrServerBlock;
struct iobxIrServerBlock {
   int                  fd;               /* Server socket.             */
   struct sockaddr_irda self;             /* ourselves                  */
   short                active;           /* Indicates an active server.*/
};


/*
** Meta block
*/
typedef struct iobxIrMetaBlock ObxIrMetaBlock;
struct iobxIrMetaBlock {
   char              *service;            // Ir Service name (i.e 'OBEX')
   short             valid;               // Was meta data valid?
};


/*
** Initialize the transport.  The inbound meta data will differ for each
** transport type.  Should be called once, prior to any other calls.
*/
ObxRc iobxIrTransportInitialize( const char *meta );

/*
** Clean up all internals
*/
ObxRc iobxIrTransportTerminate( void );

/*
** Create a connection
*/
ObxRc iobxIrTransportOpen( void **connectionid );

/* ************************************** */
/* When transport is acting as server     */
/* ************************************** */

/*
** Do any preperation for accepting inbound connections (i.e. acting as a server).
** For the INET transport this would include a bind() and listen().  Other transports
** may have other needs.
*/
ObxRc iobxIrTransportListen( void **connectionid );

/*
** Accept an inbound connection from a peer transport.  This call should block until
** a connection has been established.
** When a connection has been accepted, the passed 'connectionid' is set.  This will be
** provided by the caller on all subsuquent calls made against the active connection.
*/
ObxRc iobxIrTransportAccept( void **connectionid );

/* ************************************** */
/* When transport is acting as client     */
/* ************************************** */

/*
** Initiate a connection to a remote peer transport.
** When a connection has been created, the passed 'connectionid' is set.  This will be
** provided by the caller on all subsuquent calls made against the active connection.
*/
ObxRc iobxIrTransportConnect( void **connectionid );

/* ************************************** */
/* Functions used on connected transports */
/* ************************************** */

/*
** Send 'length' bytes of data from 'buf', set the actual number
** written in 'wrote'.
** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
*/
ObxRc iobxIrTransportSend( void **connectionid, const void *buf, int length, int *wrote, short allowShort );

/*
** Receive 'length' bytes of data and place into 'buf', set the actual
** number of bytes read in 'actual'.
** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
*/
ObxRc iobxIrTransportRecv( void **connectionid, void *buf, int length, int *actual, short allowShort );

/*
** Clean up all internals, subsuquent use of this 'connectionid' should result in an error.
** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
*/
ObxRc iobxIrTransportClose( void **connectionid );

/*
** Reach out and discover devices when connecting.
*/
ObxRc iobxIrDiscoverDevices( ObxIrConnectionBlock *conblk );


#ifdef __cplusplus
}
#endif


#endif
