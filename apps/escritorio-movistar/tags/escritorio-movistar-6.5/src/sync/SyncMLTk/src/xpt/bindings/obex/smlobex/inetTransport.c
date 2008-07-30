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
** Inet Transport
**************************************************************************
*/

#include <inetTransport.h>

#include <iConstants.h>
#include <utils.h>
#include <string.h>

// %%% added by luz
#include <xptitcp.h>

/*
** Should a server socket be required, this is it's resting place.
*/
static ObxInetServerBlock obxServer;

/*
** Meta data, available to all
*/
static ObxInetMetaBlock obxMeta;

/*
** Initialize the transport.  The inbound meta data will differ for each
** transport type.  Should be called once, prior to any other calls.
** Inbound meta data format:
**
** HOST=1.2.3.4 [or foo.bis.baz] PORT=123
*/
ObxRc iobxInetTransportInitialize( const char *meta ) {
   ObxRc       rc = OBX_RC_OK;
   size_t      len; // %%% luz 2002-04-16: changed from int to size_t
   const char  *p = NULL;
   char        *buf = NULL;

   OBXDBGFLOW(("iobxInetTransportInitialize() entry, meta=%s\n", meta));

   /*
   ** Server initialization.
   */
   obxServer.active = 0;

   /*
   ** Port?
   */
   p = iobxGetMetaInfoValue( meta, "Port", &len );
   if ( !p || len > 5 ) {
      obxMeta.port = OBEX_PORT;      /* let it default */
   } else {
      if ( (buf = (char *)malloc( len+1 )) ) {
         OBXDBGBUF(("iobxInetTransportInitialize() malloc, addr=0x%08x, len=%d.\n", buf, len+1 ));
         strncpy( buf, p, len );
         obxMeta.port = atoi( buf );
         free( buf );
      } else {
         OBXDBGERR(("[ERROR] iobxInetTransportInitialize() unable to allocate buffer of length=%d.\n", len+1));
         rc = OBX_RC_ERR_MEMORY;
      }
   }

   /*
   ** Host
   ** For now, assume we HAVE to have a host specified.
   */
   if ( rc == OBX_RC_OK ) {
      p = iobxGetMetaInfoValue( meta, "Host", &len );
      if ( !p ) {
// %%% Calin: There may be no Host assigned, like when be a OBEX server.
//         rc = OBX_RC_ERR_BADMETADATA;
         if ( (obxMeta.host = (char *)malloc( strlen("localhost") + 1 ) ) ) {
            OBXDBGBUF(("iobxInetTransportInitialize() malloc, addr=0x%08x, len=%d.\n", obxMeta.host, strlen("localhost") + 1  ));
            strcpy( obxMeta.host, "localhost"  );
         } else {
            OBXDBGERR(("[ERROR] iobxInetTransportInitialize() unable to allocate buffer of length=%d.\n", strlen("localhost") + 1 ));
            rc = OBX_RC_ERR_MEMORY;
         }
      } else {
         if ( (obxMeta.host = (char *)malloc( len+1 )) ) {
            OBXDBGBUF(("iobxInetTransportInitialize() malloc, addr=0x%08x, len=%d.\n", obxMeta.host, len+1 ));
            obxMeta.host[ len ] = '\0';
            strncpy( obxMeta.host, p, len );
         } else {
            OBXDBGERR(("[ERROR] iobxInetTransportInitialize() unable to allocate buffer of length=%d.\n", len+1));
            rc = OBX_RC_ERR_MEMORY;
         }
      }
   }

   if ( rc == OBX_RC_OK ) {
      obxMeta.valid = TRUE;
   } else {
      obxMeta.valid = FALSE;
   }
   return rc;
}

/*
** Clean up all internals
*/
ObxRc iobxInetTransportTerminate( void ) {
   OBXDBGFLOW(("iobxInetTransportTerminate() entry.\n"));
   free(obxMeta.host);
   obxMeta.host = NULL;
   if (obxServer.active) {
      sclose(obxServer.fd);
      obxServer.active = FALSE;
   }
   return OBX_RC_OK;
}

/*
** Create a connection
*/
ObxRc iobxInetTransportOpen( void **connectionid ) {
   ObxRc                rc = OBX_RC_OK;
   ObxInetConnectionBlock   *conblk = NULL;

   OBXDBGFLOW(("iobxInetTransportOpen() entry, connid=0x%08x\n", *connectionid));

   if ( (conblk = (ObxInetConnectionBlock *)malloc(sizeof(ObxInetConnectionBlock))) ) {
      OBXDBGBUF(("iobxInetTransportOpen() malloc, addr=0x%08x, len=%d.\n", conblk, sizeof(ObxInetConnectionBlock) ));
      memset( &conblk->peer, 0, sizeof( struct sockaddr_in ) );
      *connectionid = conblk;
      conblk->connected = FALSE;
   } else {
      OBXDBGERR(("[ERROR] iobxInetTransportOpen() unable to allocate buffer of length=%d.\n", sizeof(ObxInetConnectionBlock)));
      rc = OBX_RC_ERR_MEMORY;
   }

   return rc;
}

/* ************************************** */
/* When transport is acting as server     */
/* ************************************** */

/*
** Do any preperation for accepting inbound connections (i.e. acting as a server).
** For the INET transport this would include a bind() and listen().  Other transports
** may have other needs.
*/
ObxRc iobxInetTransportListen( void **connectionid ) {
   ObxRc       rc = OBX_RC_OK;

   OBXDBGFLOW(("iobxInetTransportListen() entry, connid=0x%08x\n", *connectionid));
   if ( !obxServer.active ) {
      obxServer.fd = socket( AF_INET, SOCK_STREAM, 0 );
      if ( obxServer.fd >= 0 ) {
         memset( &obxServer.self, 0, sizeof(struct sockaddr_in) );
         obxServer.self.sin_family = AF_INET;
         obxServer.self.sin_port = htons( (unsigned short)((obxMeta.port <= 0) ? OBEX_PORT : obxMeta.port) );
         obxServer.self.sin_addr.s_addr = INADDR_ANY;
         if ( !(bind( obxServer.fd, (struct sockaddr*) &obxServer.self, sizeof(struct sockaddr_in))) ) {
            OBXDBGINFO(("iobxInetTransportListen() bind() completes.\n"));
            if ( !(listen( obxServer.fd, 25)) ) {
               OBXDBGINFO(("iobxInetTransportListen() listen() completes.\n"));
               obxServer.active = TRUE;
            } else {
               OBXDBGERR(("[ERROR] iobxInetTransportListen() socket listen fails.\n"));
               OBXDBGSOCKERR();
               rc = OBX_RC_ERR_SOCKET_LISTEN;
            }
         } else {
            OBXDBGERR(("[ERROR] iobxInetTransportListen() socket bind fails.\n"));
            OBXDBGSOCKERR();
            rc = OBX_RC_ERR_SOCKET_BIND;
         }
      } else {
         OBXDBGERR(("[ERROR] iobxInetTransportListen() socket create fails.\n"));
         OBXDBGSOCKERR();
		   rc = OBX_RC_ERR_SOCKET_CREATE;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxInetTransportListen() server socket already listening.\n"));
      rc = OBX_RC_ERR_SOCKET_ALREADY_LISTENING;
   }
   return rc;
}

/*
** Accept an inbound connection from a peer transport.  This call should block until
** a connection has been established.
** When a connection has been accepted, the passed 'connectionid' is set.  This will be
** provided by the caller on all subsuquent calls made against the active connection.
*/
ObxRc iobxInetTransportAccept( void **connectionid ) {
	int                  addrlen = sizeof( struct sockaddr_in );
   ObxRc                rc = OBX_RC_OK;
   ObxInetConnectionBlock   *conblk = NULL;

   OBXDBGFLOW(("iobxInetTransportAccept() entry, connid=0x%08x\n", *connectionid));

   if ( obxServer.active ) {
      if ( *connectionid ) {
         conblk = *connectionid;
         if ( conblk->connected ) {
            conblk->connected = FALSE;
            sclose( conblk->fd );
         }
         OBXDBGINFO(("iobxInetTransportAccept() calling accept().\n"));
         if ( (conblk->fd = accept( obxServer.fd, (struct sockaddr *)&conblk->peer, &addrlen )) >= 0 ) {
            OBXDBGINFO(("iobxInetTransportAccept() accept() returns.\n"));
            conblk->connected = TRUE;
         } else {
            OBXDBGERR(("[ERROR] iobxInetTransportAccept() socket accept fails.\n"));
            OBXDBGSOCKERR();
            rc = OBX_RC_ERR_SOCKET_ACCEPT;
         }
      } else {
         OBXDBGERR(("[ERROR] iobxInetTransportAccept() bad plist on call.\n"));
         rc = OBX_RC_ERR_BADPLIST;           /* No connection block  */
      }
   } else {
      OBXDBGERR(("[ERROR] iobxInetTransportAccept() socket not listening.\n"));
      rc = OBX_RC_ERR_SOCKET_NOT_LISTENING;  /* listen() not called  */
   }
   return rc;
}

/* ************************************** */
/* When transport is acting as client     */
/* ************************************** */

/*
** Initiate a connection to a remote peer transport.
** When a connection has been created, the passed 'connectionid' is set.  This will be
** provided by the caller on all subsuquent calls made against the active connection.
*/
ObxRc iobxInetTransportConnect( void **connectionid ) {
   ObxRc                rc = OBX_RC_OK;
   ObxInetConnectionBlock   *conblk = NULL;

   OBXDBGFLOW(("iobxInetTransportConnect() entry, connid=0x%08x\n", *connectionid));

   if ( *connectionid ) {
      conblk = *connectionid;
      if ( conblk->connected ) {
         sclose( conblk->fd );
      }
      conblk->fd = socket( AF_INET, SOCK_STREAM, 0 );
      if ( conblk->fd >= 0 ) {
         memset( &conblk->peer, 0, sizeof(struct sockaddr_in) );
         conblk->peer.sin_family = AF_INET;
         conblk->peer.sin_port = htons( (unsigned short)((obxMeta.port <= 0) ? OBEX_PORT : obxMeta.port) );
         if ( (rc = iobxGetPeerAddr( obxMeta.host, &conblk->peer )) == OBX_RC_OK ) {
            OBXDBGINFO(("iobxInetTransportConnect() attempting connect() host=%s\tport=%d.\n", obxMeta.host, obxMeta.port));
            if ( connect( conblk->fd, (struct sockaddr *)&conblk->peer, sizeof(struct sockaddr_in)) >= 0 ) {
               OBXDBGINFO(("iobxInetTransportConnect() connect() succeeds.\n"));
               conblk->connected = TRUE;
            } else {
               OBXDBGERR(("[ERROR] iobxInetTransportConnect() socket connect fails.\n"));
               OBXDBGSOCKERR();
               rc = OBX_RC_ERR_SOCKET_CONNECT;
            }
         } else {
            OBXDBGERR(("[ERROR] iobxInetTransportConnect() unexpected rc from iobxGetPeerAddr().\n"));
         }
      } else {
         OBXDBGERR(("[ERROR] iobxInetTransportConnect() socket create fails.\n"));
         rc = OBX_RC_ERR_SOCKET_CREATE;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxInetTransportConnect() bad plist on call.\n"));
      rc = OBX_RC_ERR_BADPLIST;           /* no con block   */
   }
   return rc;
}

/* ************************************** */
/* Functions used on connected transports */
/* ************************************** */

/*
** Send 'length' bytes of data from 'buf', set the actual number
** written in 'wrote'.
** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
*/
ObxRc iobxInetTransportSend( void **connectionid, const void *buf, int length, int *wrote, short allowShort ) {
   ObxRc                rc = OBX_RC_OK;
   ObxInetConnectionBlock   *conblk = NULL;
   int                  remainingSend = length;
   void                 *cursor;
   int                  sent;

   OBXDBGFLOW(("iobxInetTransportSend() entry, connid=0x%08x\tlength=%d\tallowShort=%d\n", *connectionid, length, allowShort));
   OBXDBGMEM(("iobxInetTransportSend()", buf, length));
   *wrote = 0;
   if ( *connectionid ) {
      conblk = *connectionid;
      cursor = (void *)buf;
      do {
         sent = swrite( conblk->fd, cursor, remainingSend );
         if ( sent != SOCKET_ERROR ) {
            OBXDBGINFO(("iobxInetTransportSend() socket send, %d bytes sent.\n", sent));
            remainingSend -= sent;
            
            // LEO:
            cursor = (void *)((unsigned char *)cursor+sent);
            // (unsigned char *)cursor += sent; /* Treat as bytes */
            
            *wrote += sent;
         } else {
            OBXDBGERR(("[ERROR] iobxInetTransportSend() socket send resulted in SOCKET_ERROR.\n"));
            OBXDBGSOCKERR();
            rc = OBX_RC_ERR_SOCKET;
         }
      } while ( allowShort == FALSE && remainingSend > 0 && rc == OBX_RC_OK );
   } else {
      OBXDBGERR(("[ERROR] iobxInetTransportSend() bad plist on call.\n"));
      rc = OBX_RC_ERR_BADPLIST;        /* No con block */
   }
   return rc;
}

/*
** Receive 'length' bytes of data and place into 'buf', set the actual
** number of bytes read in 'actual'.
** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
*/
ObxRc iobxInetTransportRecv( void **connectionid, void *buf, int length, int *actual, short allowShort ) {
   ObxRc                rc = OBX_RC_OK;
   ObxInetConnectionBlock   *conblk = NULL;
   int                  remainingRead = length;
   void                 *cursor;
   int                  iread;

   OBXDBGFLOW(("iobxInetTransportRecv() entry, connid=0x%08x\tlength=%d\n", *connectionid, length));

   *actual = 0;
   if ( *connectionid ) {
      conblk = *connectionid;
      cursor = buf;
      do {
         iread = sread( conblk->fd, cursor, remainingRead );
         switch ( iread ) {
         case SOCKET_ERROR:
            OBXDBGERR(("[ERROR] iobxInetTransportRecv() socket send resulted in SOCKET_ERROR.\n"));
            OBXDBGSOCKERR();
            rc = OBX_RC_ERR_SOCKET;
            break;
         case 0:
            OBXDBGINFO(("iobxInetTransportRecv() socket read results in EOF.\n"));
            rc = OBX_RC_ERR_SOCKET_EOF;
            break;
         default:
            OBXDBGINFO(("iobxInetTransportRecv() socket read, %d bytes read.\n", iread));
            OBXDBGMEM(("iobxInetTransportRecv()", cursor, iread));
            remainingRead -= iread;
            
            
            // LEO:
            cursor = (void *)((unsigned char *)cursor+iread);
            // (unsigned char *)cursor += iread; /* Treat as bytes */
            
            
            
            *actual += iread;
            break;
         }
      } while ( allowShort == FALSE && remainingRead > 0 && rc == OBX_RC_OK );
   } else {
      OBXDBGERR(("[ERROR] iobxInetTransportRecv() bad plist on call.\n"));
      rc = OBX_RC_ERR_BADPLIST;        /* No con block */
   }
   return rc;
}

/*
** Clean up all internals, subsuquent use of this 'connectionid' should result in an error.
** Note that the inbound 'connectionid' was created by an open() call.
** Passing in a null connectionid is ignored.
*/
ObxRc iobxInetTransportClose( void **connectionid ) {
   ObxInetConnectionBlock   *conblk = NULL;
   OBXDBGFLOW(("iobxInetTransportClose() entry, connid=0x%08x\n", *connectionid));
   if ( connectionid ) {
      conblk = *connectionid;
      if ( conblk ) {
         if ( conblk->connected ) {
            sclose( conblk->fd );
         }
         free( conblk );
         *connectionid = NULL;
      }
   }
   return OBX_RC_OK;
}

