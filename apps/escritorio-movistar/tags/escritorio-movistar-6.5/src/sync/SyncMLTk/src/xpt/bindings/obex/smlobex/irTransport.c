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

#include <irTransport.h>

#include <iConstants.h>
#include <utils.h>
#include <string.h>

/*
** Should a server socket be required, this is it's resting place.
*/
static ObxIrServerBlock obxServer;

/*
** Meta data, available to all
*/
static ObxIrMetaBlock obxMeta;

/*
** Initialize the transport.  The inbound meta data will differ for each
** transport type.  Should be called once, prior to any other calls.
** Inbound meta data format:
**
** SERVICE=OBEX
*/
ObxRc iobxIrTransportInitialize( const char *meta ) {
   ObxRc       rc = OBX_RC_OK;
   size_t      len; // %%% luz 2002-04-16: changed from int to size_t
   const char  *p = NULL;

   OBXDBGFLOW(("iobxIrTransportInitialize() entry, meta=%s\n", meta));

   /*
   ** Server initialization.
   */
   obxServer.active = 0;

   /*
   ** Host
   ** Use a constant 'OBEX' if none specified.
   */
   if ( rc == OBX_RC_OK ) {
      p = iobxGetMetaInfoValue( meta, "Service", &len );
      if ( !p ) {
         if ( (obxMeta.service = (char *)malloc( strlen(OBEX_DEFAULT_SERVICE)+1 )) ) {
            OBXDBGBUF(("iobxIrTransportInitialize() malloc, addr=0x%08x, len=%d.\n", obxMeta.service, strlen(OBEX_DEFAULT_SERVICE)+1 ));
            obxMeta.service[ strlen(OBEX_DEFAULT_SERVICE) ] = '\0';
            strncpy( obxMeta.service, OBEX_DEFAULT_SERVICE, strlen(OBEX_DEFAULT_SERVICE) );
         } else {
            OBXDBGERR(("[ERROR] iobxIrTransportInitialize() unable to allocate buffer of length=%d.\n", strlen(OBEX_DEFAULT_SERVICE)+1));
            rc = OBX_RC_ERR_MEMORY;
         }
      } else {
         if ( (obxMeta.service = (char *)malloc( len+1 )) ) {
            OBXDBGBUF(("iobxIrTransportInitialize() malloc, addr=0x%08x, len=%d.\n", obxMeta.service, len+1 ));
            obxMeta.service[ len ] = '\0';
            strncpy( obxMeta.service, p, len );
         } else {
            OBXDBGERR(("[ERROR] iobxIrTransportInitialize() unable to allocate buffer of length=%d.\n", len+1));
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
ObxRc iobxIrTransportTerminate( void ) {
   OBXDBGFLOW(("iobxIrTransportTerminate() entry.\n"));
   free(obxMeta.service);
   obxMeta.service = NULL;
   if (obxServer.active) {
      sclose(obxServer.fd);
      obxServer.active = FALSE;
   }
   return OBX_RC_OK;
}

/*
** Create a connection
*/
ObxRc iobxIrTransportOpen( void **connectionid ) {
   ObxRc                   rc = OBX_RC_OK;
   ObxIrConnectionBlock    *conblk = NULL;

   OBXDBGFLOW(("iobxIrTransportOpen() entry, connid=0x%08x\n", *connectionid));

   if ( (conblk = (ObxIrConnectionBlock *)malloc(sizeof(ObxIrConnectionBlock))) ) {
      OBXDBGBUF(("iobxIrTransportOpen() malloc, addr=0x%08x, len=%d.\n", conblk, sizeof(ObxIrConnectionBlock) ));
      memset( &conblk->peer, 0, sizeof( struct sockaddr_irda ) );
      *connectionid = conblk;
      conblk->connected = FALSE;
   } else {
      OBXDBGERR(("[ERROR] iobxIrTransportOpen() unable to allocate buffer of length=%d.\n", sizeof(ObxIrConnectionBlock)));
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
ObxRc iobxIrTransportListen( void **connectionid ) {
   ObxRc       rc = OBX_RC_OK;
   OBXDBGFLOW(("iobxIrTransportListen() entry, connid=0x%08x\n", *connectionid));
   if ( !obxServer.active ) {
      obxServer.fd = socket( AF_IRDA, SOCK_STREAM, 0 );
      if ( obxServer.fd >= 0 ) {
         memset( &obxServer.self, 0, sizeof(struct sockaddr_irda) );
#ifdef _WIN32
         obxServer.self.sir_family = AF_IRDA;
         strncpy( obxServer.self.sir_name, obxMeta.service, strlen( obxMeta.service ) );
#else
         obxServer.self.sir_family = AF_IRDA;
         memcpy( obxServer.self.sir_name, obxMeta.service, strlen( obxMeta.service ) );
         obxServer.self.sir_lsap_sel = LSAP_ANY;
#endif
         if ( !(bind( obxServer.fd, (struct sockaddr *) &obxServer.self, sizeof(struct sockaddr_irda))) ) {
            OBXDBGINFO(("iobxIrTransportListen() bind() completes.\n"));
            if ( !(listen( obxServer.fd, 25)) ) {
               OBXDBGINFO(("iobxIrTransportListen() listen() completes.\n"));
               obxServer.active = TRUE;
            } else {
               OBXDBGERR(("[ERROR] iobxIrTransportListen() socket listen fails.\n"));
               OBXDBGSOCKERR();
               rc = OBX_RC_ERR_SOCKET_LISTEN;
            }
         } else {
            OBXDBGERR(("[ERROR] iobxIrTransportListen() socket bind fails %d.\n"));
            OBXDBGSOCKERR();
            rc = OBX_RC_ERR_SOCKET_BIND;
         }
      } else {
         OBXDBGERR(("[ERROR] iobxIrTransportListen() socket create fails.\n"));
         OBXDBGSOCKERR();
		   rc = OBX_RC_ERR_SOCKET_CREATE;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxIrTransportListen() server socket already listening.\n"));
      OBXDBGSOCKERR();
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
ObxRc iobxIrTransportAccept( void **connectionid ) {
	int                  addrlen = sizeof( struct sockaddr_irda );
   ObxRc                rc = OBX_RC_OK;
   ObxIrConnectionBlock   *conblk = NULL;

   OBXDBGFLOW(("iobxIrTransportAccept() entry, connid=0x%08x\n", *connectionid));

   if ( obxServer.active ) {
      if ( *connectionid ) {
         conblk = *connectionid;
         if ( conblk->connected ) {
            conblk->connected = FALSE;
            sclose( conblk->fd );
         }
         OBXDBGFLOW(("iobxIrTransportAccept() calling accept().\n"));
         if ( (conblk->fd = accept( obxServer.fd, (struct sockaddr *)&conblk->peer, &addrlen )) >= 0 ) {
            OBXDBGINFO(("iobxIrTransportAccept() accept() returns.\n"));
            conblk->connected = TRUE;
         } else {
            OBXDBGERR(("[ERROR] iobxIrTransportAccept() socket accept fails.\n"));
            OBXDBGSOCKERR();
            rc = OBX_RC_ERR_SOCKET_ACCEPT;
         }
      } else {
         OBXDBGERR(("[ERROR] iobxIrTransportAccept() bad plist on call.\n"));
         rc = OBX_RC_ERR_BADPLIST;           /* No connection block  */
      }
   } else {
      OBXDBGERR(("[ERROR] iobxIrTransportAccept() socket not listening.\n"));
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
ObxRc iobxIrTransportConnect( void **connectionid ) {
   ObxRc                rc = OBX_RC_OK;
   ObxIrConnectionBlock   *conblk = NULL;

   OBXDBGFLOW(("iobxIrTransportConnect() entry, connid=0x%08x\n", *connectionid));

   if ( *connectionid ) {
      conblk = *connectionid;
      if ( conblk->connected ) {
         sclose( conblk->fd );
      }
      conblk->fd = socket( AF_IRDA, SOCK_STREAM, 0 );
      if ( conblk->fd >= 0 ) {
         memset( &conblk->peer, 0, sizeof(struct sockaddr_irda) );
         conblk->peer.sir_family = AF_IRDA;
         strncpy(conblk->peer.sir_name, obxMeta.service, sizeof(conblk->peer.sir_name) );
         if ( (rc=iobxIrDiscoverDevices( conblk )) == OBX_RC_OK ) {
            if ( connect( conblk->fd, (struct sockaddr *)&conblk->peer, sizeof(struct sockaddr_irda)) >= 0 ) {
               OBXDBGINFO(("iobxIrTransportConnect() connect() succeeds.\n"));
               conblk->connected = TRUE;
            } else {
               OBXDBGERR(("[ERROR] iobxIrTransportConnect() socket connect fails.\n"));
               OBXDBGSOCKERR();
               rc = OBX_RC_ERR_SOCKET_CONNECT;
            }
         } else {
            OBXDBGERR(("[ERROR] iobxIrTransportConnect() unexpected rc from iobxIrDiscoverDevices(), rc=%d.\n", rc));
         }
      } else {
         OBXDBGERR(("[ERROR] iobxIrTransportConnect() socket create fails.\n"));
         rc = OBX_RC_ERR_SOCKET_CREATE;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxIrTransportConnect() bad plist on call.\n"));
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
ObxRc iobxIrTransportSend( void **connectionid, const void *buf, int length, int *wrote, short allowShort ) {
   ObxRc                rc = OBX_RC_OK;
   ObxIrConnectionBlock   *conblk = NULL;
   int                  remainingSend = length;
   void                 *cursor;
   int                  sent;

   OBXDBGFLOW(("iobxIrTransportSend() entry, connid=0x%08x\tlength=%d\tallowShort=%d\n", *connectionid, length, allowShort));
   OBXDBGMEM(("iobxIrTransportSend()", buf, length));
   *wrote = 0;
   if ( *connectionid ) {
      conblk = *connectionid;
      cursor = (void *)buf;
      do {
         sent = swrite( conblk->fd, cursor, remainingSend );
         if ( sent != SOCKET_ERROR ) {
            OBXDBGINFO(("iobxIrTransportSend() socket send, %d bytes sent.\n", sent));
            remainingSend -= sent;
            
            
            // LEO:
            cursor = (void *)((unsigned char *)cursor+sent);
            // (unsigned char *)cursor += sent; /* Treat as bytes */
            
            
            
            *wrote += sent;
         } else {
            OBXDBGERR(("[ERROR] iobxIrTransportSend() socket send resulted in SOCKET_ERROR.\n"));
            OBXDBGSOCKERR();
            rc = OBX_RC_ERR_SOCKET;
         }
      } while ( allowShort == FALSE && remainingSend > 0 && rc == OBX_RC_OK );
   } else {
      OBXDBGERR(("[ERROR] iobxIrTransportSend() bad plist on call.\n"));
      rc = OBX_RC_ERR_BADPLIST;        /* No con block */
   }
   return rc;
}

/*
** Receive 'length' bytes of data and place into 'buf', set the actual
** number of bytes read in 'actual'.
** Note that the inbound 'connectionid' was created by either a connect() or accept() call.
*/
ObxRc iobxIrTransportRecv( void **connectionid, void *buf, int length, int *actual, short allowShort ) {
   ObxRc                rc = OBX_RC_OK;
   ObxIrConnectionBlock   *conblk = NULL;
   int                  remainingRead = length;
   void                 *cursor;
   int                  iread;

   OBXDBGFLOW(("iobxIrTransportRecv() entry, connid=0x%08x\tlength=%d\n", *connectionid, length));

   *actual = 0;
   if ( *connectionid ) {
      conblk = *connectionid;
      cursor = buf;
      do {
         iread = sread( conblk->fd, cursor, remainingRead );
         switch ( iread ) {
         case SOCKET_ERROR:
            OBXDBGERR(("[ERROR] iobxIrTransportRecv() socket send resulted in SOCKET_ERROR.\n"));
            OBXDBGSOCKERR();
            rc = OBX_RC_ERR_SOCKET;
            break;
         case 0:
            OBXDBGINFO(("iobxIrTransportRecv() socket read results in EOF.\n"));
            rc = OBX_RC_ERR_SOCKET_EOF;
            break;
         default:
            OBXDBGINFO(("iobxIrTransportRecv() socket read, %d bytes read.\n", iread));
            OBXDBGMEM(("iobxIrTransportRecv()", cursor, iread));
            remainingRead -= iread;
            
            
            // LEO:
            cursor = (void *)((unsigned char *)cursor+iread);
            // (unsigned char *)cursor += iread; /* Treat as bytes */
            
            
            
            *actual += iread;
            break;
         }
      } while ( allowShort == FALSE && remainingRead > 0 && rc == OBX_RC_OK );
   } else {
      OBXDBGERR(("[ERROR] iobxIrTransportRecv() bad plist on call.\n"));
      rc = OBX_RC_ERR_BADPLIST;        /* No con block */
   }
   return rc;
}

/*
** Clean up all internals, subsuquent use of this 'connectionid' should result in an error.
** Note that the inbound 'connectionid' was created by an open() call.
** Passing in a null connectionid is ignored.
*/
ObxRc iobxIrTransportClose( void **connectionid ) {
   ObxIrConnectionBlock   *conblk = NULL;
   OBXDBGFLOW(("iobxIrTransportClose() entry, connid=0x%08x\n", *connectionid));
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


/*
** Discover devices when attempting to connect.
*/
#ifdef _WIN32
/*
** Win32
*/
ObxRc iobxIrDiscoverDevices( ObxIrConnectionBlock *conblk ) {
   DEVICELIST    list;
   unsigned int  index;
   int           len = sizeof( list );
   int           scanCount = 0;
   int           maxScanSecs = 5;

   memset( &list, 0, sizeof( list ) );

   OBXDBGFLOW(("iobxIrDiscoverDevices() entry, conblk=0x%08x.\n", conblk));

   while ( list.numDevice == 0 && scanCount <= maxScanSecs ) {
       if ( getsockopt( conblk->fd, SOL_IRLMP, IRLMP_ENUMDEVICES, (char *)&list, &len) == SOCKET_ERROR ) {
           scanCount++;
       }
   }

   if (list.numDevice == 0) {
      OBXDBGERR(("[ERROR] iobxIrDiscoverDevices() No IR devices discovered.\n"));
      return OBX_RC_ERR_TRANSPORT;
   } else {
      OBXDBGINFO(("iobxIrDiscoverDevices() Discovered: %d device(s), as follows:\n", list.numDevice));
      for ( index=0; index<list.numDevice; index++) {
         OBXDBGINFO(("Device: %d\tname:%s\tdaddr:0x%08x\n",
                   index, list.Device[0].irdaDeviceName, list.Device[0].sir_addr));

      }

      for ( index=0; index <= 3; index++ ) {
         conblk->peer.sir_addr[index] = list.Device[0].sir_addr[index];
      }
   }

   return OBX_RC_OK;
}
#else
/*
** Non-Win32
*/
ObxRc iobxIrDiscoverDevices( ObxIrConnectionBlock *conblk ) {
	struct irda_device_list *list;
	unsigned char *buf;
	int len;
	int i;

   OBXDBGFLOW(("iobxIrDiscoverDevices() entry, conblk=0x%08x.\n", conblk));

	len = sizeof(struct irda_device_list) -
		      sizeof(struct irda_device_info) +
		      sizeof(struct irda_device_info) * MAX_DEVICES;

	buf = malloc( len );
   OBXDBGBUF(("iobxIrDiscoverDevices() malloc, addr=0x%08x, len=%d.\n", buf, len ));

	list = (struct irda_device_list *) buf;
	
	if ( getsockopt( conblk->fd, SOL_IRLMP, IRLMP_ENUMDEVICES, buf, &len ) ) {
      free( buf );
		return OBX_RC_ERR_TRANSPORT;
	}

	if ( len > 0 ) {
      OBXDBGINFO(("iobxIrDiscoverDevices() Discovered: %d device(s), as follows:\n", list->len));
      for (i=0; i<list->len; i++) {
         OBXDBGINFO(("Device: %d\tname:%s\n\tdaddr:0x%08x\tsaddr:0x%08x\n",
                     i, list->dev[i].info,  list->dev[i].daddr, list->dev[i].saddr ));
      }

		for (i=0;i<list->len;i++) {
			OBXDBGINFO(("  name:  %s\n", list->dev[i].info));
			OBXDBGINFO(("  daddr: 0x%08x\n", list->dev[i].daddr));
			OBXDBGINFO(("  saddr: 0x%08x\n", list->dev[i].saddr));
			OBXDBGINFO(("\n"));
         conblk->peer.sir_addr = list->dev[i].daddr;
         free( buf );
         return OBX_RC_OK;
		}
	}
   OBXDBGERR(("[ERROR] iobxIrDiscoverDevices() No IR devices discovered.\n"));
	return OBX_RC_ERR_TRANSPORT;
}
#endif

