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
**
** Defines public API's and constants for the OBEX transport.
**
** (See obexStructures.h for command/response constants.)
**
**************************************************************************
*/
#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#define EXPORT_OBEX_FUNCTIONS
#include <obex/obex.h>
#include <iConstants.h>
#include <handle.h>
#include <object.h>
#include <utils.h>
#include <buffer.h>
#include <transport.h>
#include <limits.h>
#include <assert.h>

#include <time.h>	/* for time() */

#ifndef min
 #define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
 #define max(a, b) ((a) > (b) ? (a) : (b))
#endif

OBEX_EXPORT void ObxInit() {
   OBXDBGFLOW(("ObxInit() entry.\n"));

   /*
   ** We've made some assumptions about the size of things...
   */
   assert( sizeof( short ) == 2 );
   assert( sizeof( int ) == 4 );


#ifdef _WIN32
   _initializeSockets();
#endif
}

OBEX_EXPORT ObxHandle* ObxHandleNew() {
   OBXDBGFLOW(("ObxHandleNew() entry.\n"));
   return iobxHandleNew();
}

OBEX_EXPORT void ObxHandleFree( ObxHandle *handle ) {
   OBXDBGFLOW(("ObxHandleFree() entry, handle=0x%08x\n", handle));
   iobxHandleFree( handle );
}

OBEX_EXPORT ObxTransport *ObxTransportGet( short preDefinedTransportId ) {
   OBXDBGFLOW(("ObxTransportGet() entry, id=%d\n", preDefinedTransportId));
   return iobxTransportGet( preDefinedTransportId );
}

OBEX_EXPORT void ObxTransportFree( ObxTransport *transport ) {
   OBXDBGFLOW(("ObxTransportFree() entry, transport=0x%08x\n", transport));
   iobxTransportFree( transport );
}

OBEX_EXPORT ObxRc ObxTransportRegister( ObxHandle *handle, ObxTransport *transport ) {
   OBXDBGFLOW(("ObxTransportRegister() entry, handle=0x%08x\ttransport=0x%08x\n", handle, transport));
   return ( handle == NULL ) ?
            OBX_RC_ERR_BADPLIST : (handle->transport = transport, OBX_RC_OK);
}

OBEX_EXPORT ObxRc ObxTransportInitialize( ObxHandle *handle, const char *meta ) {
   OBXDBGFLOW(("ObxTransportInitialize() entry, handle=0x%08x\n", handle));
   return ( handle == NULL || handle->transport == NULL ) ?
            OBX_RC_ERR_BADPLIST : (handle->transport->initialize)( meta );
}

OBEX_EXPORT ObxRc ObxTransportOpen( ObxHandle *handle ) {
   OBXDBGFLOW(("ObxTransportOpen() entry, handle=0x%08x\n", handle));
   return ( handle == NULL || handle->transport == NULL ) ?
            OBX_RC_ERR_BADPLIST : (handle->transport->open)( &handle->connectionId );
}

OBEX_EXPORT ObxRc ObxTransportClose( ObxHandle *handle ) {
   OBXDBGFLOW(("ObxTransportClose() entry, handle=0x%08x\n", handle));
   return ( handle == NULL || handle->transport == NULL ) ?
            OBX_RC_ERR_BADPLIST : (handle->transport->close)( &handle->connectionId );
}

OBEX_EXPORT ObxRc ObxTransportListen( ObxHandle *handle ) {
   OBXDBGFLOW(("ObxTransportListen() entry, handle=0x%08x\n", handle));
   return ( handle == NULL || handle->transport == NULL ) ?
            OBX_RC_ERR_BADPLIST : (handle->transport->listen)( &handle->connectionId );
}

OBEX_EXPORT ObxRc ObxTransportAccept( ObxHandle *handle ) {
   OBXDBGFLOW(("ObxTransportAccept() entry, handle=0x%08x\n", handle));
   return ( handle == NULL || handle->transport == NULL ) ?
            OBX_RC_ERR_BADPLIST : (handle->transport->accept)( &handle->connectionId );
}

OBEX_EXPORT ObxRc ObxTransportConnect( ObxHandle *handle ) {
   OBXDBGFLOW(("ObxTransportConnect() entry, handle=0x%08x\n", handle));
   return ( handle == NULL || handle->transport == NULL ) ?
            OBX_RC_ERR_BADPLIST : (handle->transport->connect)( &handle->connectionId );
}

OBEX_EXPORT ObxRc ObxTransportTerminate( ObxHandle *handle ) {
   OBXDBGFLOW(("ObxTransportTerminate() entry, handle=0x%08x\n", handle));
   return ( handle == NULL || handle->transport == NULL ) ?
            OBX_RC_ERR_BADPLIST : (handle->transport->terminate)();
}

OBEX_EXPORT ObxRc ObxObjectSend( ObxHandle *handle, ObxObject *object ) {
   OBXDBGFLOW(("ObxObjectSend() entry, handle=0x%08x\tobject=0x%08x\n", handle, object));
   return iobxObjectSendObject( object, handle );
}

#ifndef RAW_API_ONLY
OBEX_EXPORT ObxStream *ObxStreamNew() {
   ObxStream *stream;
   if ( (stream=(ObxStream *)malloc(sizeof( ObxStream ) )) ) {
      OBXDBGBUF(("ObxStreamNew() malloc, addr=0x%08x, len=%d.\n", stream, sizeof(ObxStream) ));
      stream->handle = NULL;
      stream->data = iobxBufNew( 10 );
      stream->request = iobxObjectNew();
      stream->state = STREAM_UNOPEN;
   }
   return stream;
}

OBEX_EXPORT ObxRc ObxStreamFree( ObxStream *stream ) {
   ObxRc rc = OBX_RC_OK;
   if ( stream ) {
      if ( stream->state != STREAM_CLOSED ) {
         OBXDBGERR(("[ERROR] ObxStreamFree() freeing stream before it was closed!\n"));
      }
      iobxBufFree( stream->data );
      stream->data = NULL;
      /*
      ** We can just release any buffers contained within this request, because
      ** the *only* element ever put on it's list is stream->data, which was
      ** just released.
      */
      iobxListRelease( iobxObjectGetHeaderList( stream->request ) );
      iobxObjectFree( stream->request );
      stream->request = NULL;
      free( stream );
   }
   return rc;
}

OBEX_EXPORT ObxRc ObxStreamingSend( ObxHandle *handle, ObxObject *request, ObxStream *stream ) {
   ObxRc          rc = OBX_RC_OK;
   ObxObject      *response;
   OBXDBGFLOW(("ObxStreamingSend() entry, handle=0x%08x\trequest=0x%08x\tstream=0x%08x\n", handle, request, stream));

   if ( handle && request && stream ) {

      if ( request->cmd == OBEX_CMD_GET ) {
         return OBX_RC_ERR_INAPPROPRIATE;
      }

      /*
      ** State must be UNOPEN when this is called.
      */
      if ( stream->state == STREAM_UNOPEN ) {
         response = iobxObjectNew();
         if ( request->cmd == OBEX_CMD_PUT ) {
            /*
            ** Setup stream, we're SENDING.
            */
            stream->cmd = OBEX_CMD_PUT;   /* Always */
            stream->state = STREAM_SENDING;
            stream->handle = handle;
            request->stream = TRUE;       /* request is part of a stream */
            /*
            ** Send all headers the user has provided in their initial request.
            */
            if ( (rc= ObxTransactionSend( stream->handle, request, response )) != OBX_RC_OK ) {
               OBXDBGERR(("[ERROR] ObxStreamingSend() unexpected rc calling ObxTransactionSend().\n"));
            } else {
               OBXDBGINFO(("ObxStreamingSend() success.\n"));
            }
         } else {
            OBXDBGERR(("[ERROR] ObxStreamingSend() Must be a PUT command!\n"));
            rc = OBX_RC_ERR_INAPPROPRIATE;
         }
         iobxObjectFree( response );
      } else {
         OBXDBGERR(("[ERROR] ObxStreamingSend() stream state was not STREAM_UNOPEN!\n"));
         rc = OBX_RC_ERR_INAPPROPRIATE;
      }
   } else {
      OBXDBGERR(("[ERROR] ObxStreamingSend() Bad plist on call\n"));
      rc = OBX_RC_ERR_BADPLIST;
   }
   return rc;
}

OBEX_EXPORT ObxRc ObxStreamingRecv( ObxHandle *handle, ObxObject *request, ObxStream *stream ) {
   ObxRc rc = OBX_RC_OK;
   OBXDBGFLOW(("ObxStreamingRecv() entry, handle=0x%08x\trequest=0x%08x\tstream=0x%08x\n", handle, request, stream));

   if ( handle && request && stream ) {
      /*
      ** Stream must be UNOPENed when this is called.
      */
      if ( stream->state == STREAM_UNOPEN ) {

         /*
         ** Setup stream, we're receiving now.
         */
         stream->state = STREAM_RECEIVING;
         stream->handle = handle;
         request->stream = TRUE;    /* Request is part of a stream */
         /*
         ** Populate the users request object with a group of headers.
         ** This could be all the headers, or just the first group.
         */
         if ( (rc= ObxTransactionRecv( stream->handle, request, FALSE )) != OBX_RC_OK ) {
            OBXDBGERR(("[ERROR] ObxStreamingRecv() unexpected rc calling ObxTransactionRecv().\n"));
         } else {
            if ( request->cmd == OBEX_CMD_GET ) {
               return OBX_RC_ERR_NO_GET_SUPPORT;
            }
            /*
            ** Was that all the peer was sending on this transaction?
            */
            if ( request->cmd & OBEX_CMD_FINAL ) {
               /*
               ** Yep, all in one send.
               */
               stream->state = STREAM_FINISHED;
               rc = OBX_RC_STREAM_EOF;
               OBXDBGINFO(("ObxStreamingRecv() success, FINAL packet from peer.\n"));
            } else {
               /*
               ** Nope, more are expected.
               */
               OBXDBGINFO(("ObxStreamingRecv() success, more packets expected from peer..\n"));
            }
         }
      } else {
         OBXDBGERR(("[ERROR] ObxStreamingRecv() stream state was not STREAM_UNOPEN!\n"));
         rc = OBX_RC_ERR_INAPPROPRIATE;
      }
   } else {
      OBXDBGERR(("[ERROR] ObxStreamingRecv() Bad plist on call\n"));
      rc = OBX_RC_ERR_BADPLIST;
   }
   return rc;
}

OBEX_EXPORT ObxRc ObxStreamSend( ObxStream *stream, unsigned char *data, int len ) {
   ObxRc    rc = OBX_RC_OK;
   OBXDBGFLOW(("ObxStreamSend() entry, stream=0x%08x\tlen=%d\n",stream, len));
   if ( stream ) {
      /*
      ** Must be in SENDING state.
      */
      if ( stream->state == STREAM_SENDING ) {
         /*
         ** Append the new data to any existing data.
         ** If it's over the max packet length, auto flush for the user.
         */
         iobxBufWrite( stream->data, data, len );
         if ( iobxBufSize( stream->data ) > stream->handle->maxPacketLen ) {
            if ( (rc=ObxStreamFlush( stream )) != OBX_RC_OK ) {
               OBXDBGERR(("[ERROR] ObxStreamSend() unexpected rc from ObxStreamFlush()\n"));
            }
         }
      } else {
         OBXDBGERR(("[ERROR] ObxStreamSend() stream state was not STREAM_SENDING!\n"));
         rc = OBX_RC_ERR_INAPPROPRIATE;
      }
   } else {
      OBXDBGERR(("[ERROR] ObxStreamSend() Bad plist on call\n"));
      rc = OBX_RC_ERR_BADPLIST;
   }
   return rc;
}

OBEX_EXPORT ObxRc ObxStreamRecv( ObxStream *stream, ObxList *headerList ) {
   ObxRc       rc = OBX_RC_OK;
   ObxIterator *obxiterator;
   OBXDBGFLOW(("ObxStreamRecv() entry, stream=0x%08x\theaderList=0x%08x\n", stream, headerList));

   if ( stream && headerList ) {
      /*
      ** Must be in RECEIVING mode...
      */
      if ( stream->state == STREAM_RECEIVING ) {
         /*
         ** Reset the request object.  We release any headers contained within, since
         ** it can only be our buffer, otherwise, it would be free'ed during the reset.
         */
         iobxListRelease( iobxObjectGetHeaderList( stream->request ) );
         if ( (rc=iobxObjectReset( stream->request )) == OBX_RC_OK ) {
            stream->request->stream = TRUE;
            /*
            ** Receive a block of headers from the peer.
            */
            if ( (rc=ObxTransactionRecv( stream->handle, stream->request, FALSE )) == OBX_RC_OK ) {

               /*
               ** Hand over all the headers received to passed list.
               */
               if ( (obxiterator=iobxListGetIterator( iobxObjectGetHeaderList( stream->request ) )) ) {
                  iobxIteratorReset( obxiterator );
                  while ( iobxIteratorHasNext( obxiterator ) ) {
                     iobxListAppend( headerList, iobxIteratorNext( obxiterator ) );
                  }
                  /*
                  ** Remove from request list, don't free since we're giving them to 'headerList'.
                  */
                  iobxListRelease( iobxObjectGetHeaderList( stream->request ) );
                  iobxIteratorFree( obxiterator );
               } else {
                  OBXDBGERR(("[ERROR] ObxStreamRecv() unable to get iterator\n"));
                  rc = OBX_RC_ERR_UNSPECIFIED;
               }
               /*
               ** Peer done sending to us?
               */
               if ( stream->request->cmd & OBEX_CMD_FINAL ) {
                  OBXDBGINFO(("ObxStreamRecv() success, 'FINAL' from peer detected.\n"));
                  stream->state = STREAM_FINISHED;
                  rc = OBX_RC_STREAM_EOF;
               } else {
                  OBXDBGINFO(("ObxStreamRecv() success, but more data from peer expected.\n"));
               }
            } else {
               OBXDBGERR(("[ERROR] ObxStreamRecv() unexpected rc calling ObxTransactionRecv().\n"));
            }
         } else {
            OBXDBGERR(("[ERROR] ObxStreamRecv() unexpected rc on call to iobxObjectReset()\n"));
         }
      } else if ( stream->state == STREAM_FINISHED ) {
         OBXDBGINFO(("[WARNING] ObxStreamRecv() read after EOF detected.\n"));
         rc = OBX_RC_STREAM_EOF;
      } else {
         OBXDBGERR(("[ERROR] ObxStreamRecv() stream state was not STREAM_RECEIVING or STREAM_FINISHED!\n"));
         rc = OBX_RC_ERR_INAPPROPRIATE;
      }
   } else {
      OBXDBGERR(("[ERROR] ObxStreamRecv() Bad plist on call\n"));
      rc = OBX_RC_ERR_BADPLIST;
   }
   return rc;
}

OBEX_EXPORT ObxRc ObxStreamFlush( ObxStream *stream ) {
   ObxRc       rc = OBX_RC_OK;
   ObxHeader   *header;
   ObxObject   *response;

   OBXDBGFLOW(("ObxStreamFlush() entry, stream=0x%08x\n", stream));
   if ( stream ) {
      /*
      ** State ok?
      */
      if ( stream->state == STREAM_SENDING ) {
         /*
         ** Any data to flush?
         */
         if ( iobxBufSize( stream->data ) ) {
            /*
            ** Reset the request object.  We release any headers contained within, since
            ** it can only be our buffer, otherwise, it would be free'ed during the reset.
            */
            iobxListRelease( iobxObjectGetHeaderList( stream->request ) );
            if ( (rc=iobxObjectReset( stream->request )) == OBX_RC_OK ) {
               /*
               ** Reset the stream object and create a local response object.
               */
               stream->request->stream = TRUE;
               stream->request->cmd = stream->cmd;
               response = iobxObjectNew();
               /*
               ** Create header of type BODY, set it's value, add the header to the request, send it.
               */
               if ( !(header = iobxHeaderNew( OBEX_HEADER_BODY )) ) {
                  OBXDBGERR(("[ERROR] ObxStreamFlush() Error creating header.\n"));
                  rc = OBX_RC_ERR_MEMORY;
               } else if ( (rc=iobxHeaderSetByteSequenceValue( header, stream->data )) != OBX_RC_OK ) {
                  OBXDBGERR(("[ERROR] ObxStreamFlush() unexpected rc calling iobxHeaderSetByteSequenceValue().\n"));
               } else if ( (rc=iobxObjectAddHeader( stream->request, header, stream->handle )) != OBX_RC_OK ) {
                  OBXDBGERR(("[ERROR] ObxStreamFlush() unexpected rc calling iobxObjectAddHeader().\n"));
               } else if ( (rc= ObxTransactionSend( stream->handle, stream->request, response )) != OBX_RC_OK ) {
                  OBXDBGERR(("[ERROR] ObxStreamFlush() unexpected rc calling ObxTransactionSend().\n"));
               } else {
                  OBXDBGINFO(("ObxStreamFlush() success.\n"));
                  /*
                  ** We play games here, a bit, to avoid having to reallocate our stream->data buffer
                  ** over and over.
                  */
                  iobxHeaderSetByteSequenceValue( header, NULL );
                  iobxHeaderFree( header );
               }
               /*
               ** Done with response.
               */
               iobxObjectFree( response );
            } else {
               OBXDBGERR(("[ERROR] ObxStreamFlush() unexpected rc on call to iobxObjectReset()\n"));
            }
         } else {
            OBXDBGINFO(("[WARNING] ObxStreamFlush() no data to flush, ignoring call.\n"));
         }
      } else {
         OBXDBGERR(("[ERROR] ObxStreamFlush() stream state was not STREAM_SENDING\n"));
         rc = OBX_RC_ERR_INAPPROPRIATE;
      }
   } else {
      OBXDBGERR(("[ERROR] ObxStreamFlush() Bad plist on call\n"));
      rc = OBX_RC_ERR_BADPLIST;
   }
   return rc;
}

OBEX_EXPORT ObxRc ObxStreamClose( ObxStream *stream ) {
   ObxRc       rc = OBX_RC_OK;
   ObxHeader   *header;
   ObxObject   *response;
   OBXDBGFLOW(("ObxStreamClose() entry, stream=0x%08x\n", stream));

   /*
   ** State ok?
   */
   if ( stream->state != STREAM_CLOSED ) {
      /*
      ** Flush data, if writing.
      */
      if ( stream->state == STREAM_SENDING ) {
         /*
         ** Flush any remaining data.
         */
         if ( (rc=ObxStreamFlush( stream )) == OBX_RC_OK ) {
            /*
            ** Reset the request object.  We release any headers contained within, since
            ** it can only be our buffer, otherwise, it would be free'ed during the reset.
            */
            iobxListRelease( iobxObjectGetHeaderList( stream->request ) );
            if ( (rc=iobxObjectReset( stream->request )) == OBX_RC_OK ) {
               /*
               ** Set the stream object up.. we're now closed.
               ** We clear the stream bit.  This will cause the object send, (called by
               ** ObxTransactionSend()) to set the final bit when it transmits.
               */
               stream->request->stream = FALSE;
               stream->request->cmd = stream->cmd;
               stream->state = STREAM_CLOSED;
               /*
               ** New up a response, create a header, BODY_END this time, set the value, flow it.
               */
               response = iobxObjectNew();
               if ( !(header = iobxHeaderNew( OBEX_HEADER_BODY_END )) ) {
                  OBXDBGERR(("[ERROR] ObxStreamClose() Error creating header.\n"));
                  rc = OBX_RC_ERR_MEMORY;
               } else if ( (rc=iobxHeaderSetByteSequenceValue( header, stream->data )) != OBX_RC_OK ) {
                  OBXDBGERR(("[ERROR] ObxStreamClose() unexpected rc calling iobxHeaderSetByteSequenceValue().\n"));
               } else if ( (rc=iobxObjectAddHeader( stream->request, header, stream->handle )) != OBX_RC_OK ) {
                  OBXDBGERR(("[ERROR] ObxStreamClose() unexpected rc calling iobxObjectAddHeader().\n"));
               } else if ( (rc= ObxTransactionSend( stream->handle, stream->request, response )) != OBX_RC_OK ) {
                  OBXDBGERR(("[ERROR] ObxStreamClose() unexpected rc calling ObxTransactionSend().\n"));
               } else {
                  OBXDBGINFO(("ObxStreamFlush() success.\n"));
                  /*
                  ** We play games here, a bit, to avoid having to reallocate our stream->data buffer
                  ** over and over.
                  */
                  iobxHeaderSetByteSequenceValue( header, NULL );
                  iobxHeaderFree( header );
               }
               /*
               ** Done with response.
               */
               iobxObjectFree( response );
            } else {
               OBXDBGERR(("[ERROR] ObxStreamFlush() unexpected rc on call to iobxObjectReset()\n"));
            }
         } else {
            OBXDBGERR(("[ERROR] ObxStreamClose() unexpected rc from ObxStreamFlush()\n"));
         }
      } else {
         /*
         ** must be a receiving stream, not much to do.
         */
         stream->state = STREAM_CLOSED;
       }
   } else {
      OBXDBGERR(("[ERROR] ObxStreamFlush() stream state already closed.\n"));
      rc = OBX_RC_ERR_INAPPROPRIATE;
   }
   return rc;
}

// ********************
// T.K. for GET we need two new functions - not the transaction Send/Recv pair.
// The scenario is like this:
// The server has created an Obexobject and wait for the client to request this
// object via get.
// server: ObxGetSend
//         -> this waits for a client GET request and sends the object and stores the clients answer in Answer
// client: ObxGetRecv
//         -> requests an Object from the server and stores it in getObj
//We added the third parameter, mimeType, in this function to check if the received object is what we want by checking Mime Type.
OBEX_EXPORT ObxRc ObxGetSend(ObxHandle *handle, ObxObject *getAnswer, char *mimeType) {
    short          done = FALSE;
    ObxRc          rc = OBX_RC_OK;
    ObxObject      *request = NULL;
    ObxIterator       *obxiterator = NULL;
    ObxList           *obxlist = NULL;
    ObxHeader         *obxheader = NULL;
    int typeHeaderCount=0;
    int connectionIDHeaderCount=0;
    int length;
    unsigned char *buffer;

    OBXDBGFLOW(("ObxGetSend() entry, handle=0x%08x\n"));
    
    if (!getAnswer) return OBX_RC_ERR_INAPPROPRIATE;
    if (!(request = iobxObjectNew())) {
        OBXDBGERR(("[ERROR] ObxGetSend() Error creating request object"));
        return OBX_RC_ERR_MEMORY;
    }
    
    while (!done) {
        OBXDBGINFO(("ObxGetSend() waiting for request\n"));
        if ((rc=iobxObjectReset(request)) != OBX_RC_OK) {
            OBXDBGERR(("[ERROR] ObxGetSend() iobxObjectReset failed with code 0x%x\n",rc));
            iobxObjectFree(request);
            return rc;
        }
        if ((rc=iobxObjectRecvObject(request, handle)) != OBX_RC_OK) {
            OBXDBGERR(("[ERROR] ObxGetSend() bad returncode from iobxObjectRecvObject\n"));
            iobxObjectFree(request);
            return rc;
        }
//Check if the headers of ConnectionID and Type carried by Get command are legal.
        if ( (obxlist=iobxObjectGetHeaderList( request )) ) {
            if ( (obxiterator=iobxListGetIterator( obxlist )) ) {
                iobxIteratorReset( obxiterator );
                while ( iobxIteratorHasNext( obxiterator ) ) {
                    if ( (obxheader = (ObxHeader *)iobxIteratorNext( obxiterator )) ) {
                        switch(obxheader->identifier) {
                        case OBEX_HEADER_CONNECTION:
                            OBXDBGINFO(("OBX ObxGetSend() The ConnectionID carried in request command is %d and the one we assign is %d\n", obxheader->value.fourBytevalue, handle->OBEXConnectionID ));
                            if(obxheader->value.fourBytevalue!=handle->OBEXConnectionID)
                                return OBX_RC_ERR_SML_CONNECTIONID_HDR;
                            connectionIDHeaderCount++;
                            break;
                        case OBEX_HEADER_TYPE:
                            length = iobxBufSize(obxheader->value.byteSequenceValue);
                            buffer = (unsigned char *)malloc( length+1 );
                            memset(buffer,0,length+1);
                            iobxBufRead( obxheader->value.byteSequenceValue, buffer, length );
                            OBXDBGINFO(("OBX ObxGetSend() Prefered Mime Type is \'%s\'\n", buffer ));
                            if(strcmp((const char *)buffer, (const char *)mimeType))
                                return OBX_RC_ERR_SML_TYPE_HDR;
                            free(buffer);
                            typeHeaderCount++;
                            break;
                        }
                    }
                }
                iobxIteratorFree( obxiterator );
            } else {
                OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxListGetIterator(), rc=%d\n", rc));
                return rc;
            }
        } else {
            OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxObjectGetHeaderList(), rc=%d\n", rc));
            return rc;
        }
        
//Every Get command must have final bit set.
/*        if (request->cmd != OBEX_CMD_GET) {
            if ((request->cmd & ~OBEX_CMD_FINAL) != OBEX_CMD_GET) {
                OBXDBGERR(("[ERROR] ObxGetSend() unexpected command received (0x%02x)\n",request->cmd));
                iobxObjectFree(request);
                return OBX_RC_ERR;
            }
        }
*/
        if (request->cmd != ( OBEX_CMD_GET | OBEX_CMD_FINAL) ) {
            OBXDBGERR(("[ERROR] ObxGetSend() unexpected command received (0x%02x)\n",request->cmd));
            iobxObjectFree(request);
            return OBX_RC_ERR;
        }

        if ((rc=iobxObjectSendObject(getAnswer, handle)) != OBX_RC_OK) {
            OBXDBGERR(("[ERROR] ObxGetSend() bad returncode from iobxObjectSendObject (0x%x)\n",rc));
            iobxObjectFree(request);
            return rc;
        }
//The last response of Get command have response code of 0xA0, i.e. OBEX_RSP_SUCCESS | OBEX_CMD_FINAL.
//        if (getAnswer->cmd & OBEX_CMD_FINAL) {
        if (getAnswer->cmd == (OBEX_RSP_SUCCESS | OBEX_CMD_FINAL)) {
            done = TRUE;
        }
    }
//There must be Type header carried in Get command.
    if(typeHeaderCount!=1)
        return OBX_RC_ERR_SML_TYPE_HDR;
//There must be ConnectionID header carried in Get command.
    if(connectionIDHeaderCount!=1)
        return OBX_RC_ERR_SML_CONNECTIONID_HDR;

    return rc;
}

OBEX_EXPORT ObxRc ObxGetRecv(ObxHandle *handle, ObxObject *request, ObxObject *responseOut ) {
    short          done = FALSE;
    ObxRc          rc = OBX_RC_OK;
    ObxObject      *response = NULL;
    ObxIterator    *obxiterator = NULL;
    ObxList        *obxlist = NULL;
    ObxHeader      *obxheader = NULL;
    ObxHeader      *bodyheader = NULL;
    void           *bytebuffer = NULL;
    int            dataLength = 0;
    OBXDBGFLOW(("ObxGetRecv() entry, handle=0x%08x\trequest=0x%08x\tresponseOut=0x%08x\n", handle, request, responseOut));
    
    if (!request)     return OBX_RC_ERR_INAPPROPRIATE;
    if (request->cmd != OBEX_CMD_GET) return OBX_RC_ERR_INAPPROPRIATE;

    if (!(response = iobxObjectNew())) {
        OBXDBGERR(("[ERROR] ObxGetRecv() Error creating response object"));
        return OBX_RC_ERR_MEMORY;
    }

//We re-write this while loop because the response code of Get command should be 0x90 for continue and 0xA0 for success, not 0x10 for continue and 0x90 for success.
    /* enter receiving loop */
    while ( !done ) {
        OBXDBGINFO(("ObxGetRecv() Sending GET request..\n"));
        if ( (rc=iobxObjectSendObject( request, handle )) == OBX_RC_OK ) {
            if ( (rc=iobxObjectReset( response )) == OBX_RC_OK ) {
                if ( (rc=iobxObjectRecvObject( response, handle )) == OBX_RC_OK ) {
//Response code must have final bit set.
                    if(!(response->cmd & OBEX_CMD_FINAL))
                        return OBX_RC_ERR_INAPPROPRIATE;
                    switch ( response->cmd & ~OBEX_CMD_FINAL ) {
                    case OBEX_RSP_CONTINUE:
                        OBXDBGINFO(("ObxGetRecv() got OBEX_RSP_CONTINUE from our send attempt.\n"));
                        
                        //-----------
                        /* Special Windows feature ....
                        * Check for Final Flag - if set the object is received complete
                        * Nomally we expect a OBEX_RSP_SUCCESS but ......
                        */
//                        if (response->cmd & OBEX_CMD_FINAL) {
//                            done = TRUE;
//                        }
                        break;
                        
                    case OBEX_RSP_SUCCESS:
                        // we are done!
                        done = TRUE;
                        OBXDBGINFO(("ObxGetRecv() got response: OBEX_RSP_SUCCESS from our send attempt.\n"));
                        break;
                    default:
                        OBXDBGERR(("[ERROR] ObxGetRecv() *unexpected* response (%02x) from our send attempt.\n", response->cmd));
                        return OBX_RC_ERR_INAPPROPRIATE;
//                        done = TRUE;
//                        break;
                    }

                    // ok - coalesce headers and request more data
                    //-----------
                    if ( (obxlist=iobxObjectGetHeaderList( response )) ) {
                        if ( (obxiterator=iobxListGetIterator( obxlist )) ) {
                            iobxIteratorReset( obxiterator );
                            while ( iobxIteratorHasNext( obxiterator ) ) {
                                if ( (obxheader = (ObxHeader *)iobxIteratorNext( obxiterator )) ) {
                                    if ( obxheader->identifier == OBEX_HEADER_BODY || obxheader->identifier == OBEX_HEADER_BODY_END ) {
                                        /* We may need to bind this data with some data that's already arrived. */
                                        if ( bodyheader ) {
                                            /* A header is already in the output request.  Append. */
                                            dataLength = iobxBufSize( obxheader->value.byteSequenceValue );
                                            if ( ( bytebuffer=(void *)malloc( dataLength )) ) {
                                                OBXDBGBUF(("ObxGetRecv() malloc, addr=0x%08x, len=%d.\n", bytebuffer, dataLength ));
                                                if ( iobxBufRead( obxheader->value.byteSequenceValue, bytebuffer, dataLength ) == dataLength) {
                                                    OBXDBGINFO(("ObxGetRecv() appending body to existing header.\n"));
                                                    if ( (iobxBufWrite( bodyheader->value.byteSequenceValue, bytebuffer, dataLength )) ) {
                                                        OBXDBGERR(("[ERROR] ObxGetRecv() Error appending to header.\n"));
                                                        return OBX_RC_ERR_TRANSPORT;
                                                    }
                                                } else {
                                                    OBXDBGERR(("[ERROR] ObxGetRecv() not enough data in buffer?.\n"));
                                                    return OBX_RC_ERR_MEMORY;
                                                }
                                                free( bytebuffer );
                                            } else {
                                                OBXDBGERR(("[ERROR] ObxGetRecv() creating byte buffer.\n"));
                                                return OBX_RC_ERR_MEMORY;
                                            }
                                            /* We don't want to add this one.. since we've combined with an existing one. */
                                            // T.K. we don't free it here, as it gets freed later on.
                                            // freeing it now will cause memory coruption. Just set the pointer
                                            // to NULL and it gets cleaned up in the end
                                            //iobxHeaderFree( obxheader );
                                            obxheader = NULL;
                                        } else {
                                            /* First one we've seen, so, we'll add it.*/
                                            bodyheader = obxheader;
                                        }
                                    }
                                    /*
                                    ** If we haven't coalesced this header we simply add it.
                                    ** Note that this could result in duplicate headers from the peer.  We make
                                    ** no effort to stop them from sending them... we're just a conduit.
                                    */
                                    if ( obxheader ) {
                                        if ( (rc=iobxObjectAddHeader( responseOut, obxheader, handle )) != OBX_RC_OK ) {
                                            OBXDBGERR(("[ERROR] ObxGetRecv() bad return code from iobxObjectAddHeader(), rc=%d\n", rc));
                                            return rc;
                                        }
                                        /*
                                        ** Remove this header from the 'request' since 'requestOut' now owns it.
                                        ** The itterator will be 'backed up' from the removed item.  All ready for
                                        ** the next pass through the loop.
                                        */
                                        if ( !(iobxListRemove( obxlist, obxiterator )) ) {
                                            OBXDBGERR(("[ERROR] ObxGetRecv() Error removing header from response.\n"));
                                            return OBX_RC_ERR_TRANSPORT;
                                        }
                                    }
                                } else {
                                    OBXDBGERR(("[ERROR] ObxGetRecv() bad return code from iobxIteratorNext(), rc=%d\n", rc));
                                    return rc;
                                }
                            } // eof while iterator has next
                              /*
                              ** Done with iterator
                            */
                            iobxIteratorFree( obxiterator );
                        } else {
                            OBXDBGERR(("[ERROR] ObxGetRecv() bad return code from iobxListGetIterator(), rc=%x\n", rc));
                            return rc;
                        }
                    } else {
                        OBXDBGERR(("[ERROR] ObxGetRecv() bad return code from iobxObjectGetHeaderList(), rc=%x\n", rc));
                        return rc;
                    }
                } else {
                    OBXDBGERR(("[ERROR] ObxGetRecv() bad return code from iobxObjectRecvObject(), rc=%x\n", rc));
                    done = TRUE;
                }
            } else {
                OBXDBGERR(("[ERROR] ObxGetRecv() bad return code from iobxObjectReset(), rc=%x\n", rc));
                done = TRUE;
            }
        } else {
            OBXDBGERR(("[ERROR] ObxGetRecv() bad return code from iobxObjectSendObject(), rc=%x\n", rc));
            done = TRUE;
        }
    }
//    responseOut->cmd = OBEX_RSP_SUCCESS | OBEX_CMD_FINAL;
    return rc;
}

//**************************************************************
OBEX_EXPORT ObxRc ObxTransactionSend( ObxHandle *handle, ObxObject *request, ObxObject *response ) {
   short          done = FALSE;
   ObxRc          rc = OBX_RC_OK;
   OBXDBGFLOW(("ObxTransactionSend() entry, handle=0x%08x\trequest=0x%08x\tresponse=0x%08x\n", handle, request, response));

   if ( request->cmd == OBEX_CMD_GET ) {
      rc = OBX_RC_ERR_INAPPROPRIATE;
   } else {
      while ( !done ) {
         OBXDBGINFO(("ObxTransactionSend() Sending object...\n"));
         if ( (rc=iobxObjectSendObject( request, handle )) == OBX_RC_OK ) {
            if ( response ) {
               if ( (rc=iobxObjectReset( response )) == OBX_RC_OK ) {
                  if ( (rc=iobxObjectRecvObject( response, handle )) == OBX_RC_OK ) {
                     switch ( response->cmd & ~OBEX_CMD_FINAL ) {
                     /*
                     ** We'll interperate success/continue as ... ok.. send more.. or ok.. we're done.
                     ** Again, the spec states that we should get a 'continue' when dealing with non
                     ** final packets.. but Win32 returns 'success'... we'll treat either as an
                     ** indication to keep going.
                     */
                     case OBEX_RSP_SUCCESS:
//When response of SUCCESS has received, this obex command should be terminated.
								done = TRUE;
                     case OBEX_RSP_CONTINUE:
                        OBXDBGINFO(("ObxTransactionSend() got response: OBEX_RSP_SUCCESS from our send attempt.\n"));
                        if ( request->cmd == OBEX_CMD_CONNECT ) {
                                    //handle->maxPacketLen = ntohs( response->meta.connectMeta->max_packet_length );
                                    handle->maxPacketLen = response->meta.connectMeta->max_packet_length;
                           OBXDBGINFO(("ObxTransactionSend() MTU from peer is %d.\n", handle->maxPacketLen));
                        }
                        /*
                        ** Here's the rub.. a continue could either indicate that the object we have
                        ** needs additional 'sending', (i.e. it wouldn't fit in the MTU) or we're dealing
                        ** with a stream.  So... we check to see if the object we're dealing with needs
                        ** to continue to be sent.
                        */
                        if ( request->state == STATE_SENT ) {
                           /*
                           ** In this situation, the object has been completely sent, but we got a
                           ** continue from the peer.  This indicates that more objects will follow
                           */
                           done = TRUE;
                        }
                        break;
                     default:
                        OBXDBGERR(("[ERROR] ObxTransactionSend() *unexpected* response (%02x) from our send attempt.\n", response->cmd));
                        done = TRUE;
                        break;
                     }
                  } else {
                     OBXDBGERR(("[ERROR] ObxTransactionSend() bad return code from iobxObjectRecvObject(), rc=%d\n", rc));
                     done = TRUE;
                  }
               } else {
                  OBXDBGERR(("[ERROR] ObxTransactionSend() bad return code from iobxObjectReset(), rc=%d\n", rc));
                  done = TRUE;
               }
            } else {
               // Response not expected, we return immediately.
               done = TRUE;
            }
         } else {
            OBXDBGERR(("[ERROR] ObxTransactionSend() bad return code from iobxObjectSendObject(), rc=%d\n", rc));
            done = TRUE;
         }
      }
   }
   return rc;
}

OBEX_EXPORT ObxRc ObxTransactionRecv( ObxHandle *handle, ObxObject *requestOut, short coalesce ) {
   short             done = FALSE;
   ObxRc             rc = OBX_RC_OK;
   ObxObject         *response = NULL;
   ObxObject         *request = NULL;
   ObxIterator       *obxiterator = NULL;
   ObxList           *obxlist = NULL;
   ObxHeader         *obxheader = NULL;
   ObxHeader         *bodyheader = NULL;
    ObxHeader         *header = NULL;
   void              *bytebuffer;
   int               dataLength;
    ObxBuffer         *obxbuffer;
    int targetHeaderCount=0;

   OBXDBGFLOW(("ObxTransactionRecv() entry, handle=0x%08x\trequest=0x%08x\n", handle, request));

   if ( !(response = iobxObjectNew()) || !(request = iobxObjectNew()) ) {
      OBXDBGERR(("[ERROR] ObxTransactionRecv() Error creating request/response objects.\n"));
      return OBX_RC_ERR_MEMORY;
   }

   while ( !done ) {
      if ( (rc=iobxObjectReset( request )) == OBX_RC_OK ) {
         if ( (rc=iobxObjectRecvObject( request, handle )) == OBX_RC_OK ) {
                requestOut->cmd = request->cmd;
            if ( request->cmd == OBEX_CMD_GET ) {
               /*
               ** They are on their own.
               */
               return OBX_RC_ERR_NO_GET_SUPPORT;
            }

            if ( (rc=iobxObjectReset( response )) == OBX_RC_OK ) {
               /*
               ** Acumulate or coalesce headers, in either case, we iterate all new inbound headers.
               */
               if ( (obxlist=iobxObjectGetHeaderList( request )) ) {
                  if ( (obxiterator=iobxListGetIterator( obxlist )) ) {
                     iobxIteratorReset( obxiterator );
                     while ( iobxIteratorHasNext( obxiterator ) ) {
                        if ( (obxheader = (ObxHeader *)iobxIteratorNext( obxiterator )) ) {
//OBEX_HEADER_BODY_END is also a kind of body chunk.
//                                    if ( obxheader->identifier == OBEX_HEADER_BODY || obxheader->identifier == OBEX_HEADER_BODY ) {
                                    if ( obxheader->identifier == OBEX_HEADER_BODY || obxheader->identifier == OBEX_HEADER_BODY_END ) {
                              /*
                              ** We may need to bind this data with some data that's already arrived.
                              */
                              if ( coalesce ) {
                                 if ( bodyheader ) {
                                    /*
                                    ** A header is already in the output request.  Append.
                                    */
                                    dataLength = iobxBufSize( obxheader->value.byteSequenceValue );
                                    if ( ( bytebuffer=(void *)malloc( dataLength )) ) {
                                       OBXDBGBUF(("ObxTransactionRecv() malloc, addr=0x%08x, len=%d.\n", bytebuffer, dataLength ));
                                       if ( iobxBufRead( obxheader->value.byteSequenceValue, bytebuffer, dataLength ) == dataLength) {
                                          OBXDBGINFO(("ObxTransactionRecv() appending body to existing header.\n"));
                                          if ( (iobxBufWrite( bodyheader->value.byteSequenceValue, bytebuffer, dataLength )) ) {
                                             OBXDBGERR(("[ERROR] ObxTransactionRecv() Error appending to header.\n"));
                                             return OBX_RC_ERR_TRANSPORT;
                                          }
                                       } else {
                                          OBXDBGERR(("[ERROR] ObxTransactionRecv() not enough data in buffer?.\n"));
                                          return OBX_RC_ERR_MEMORY;
                                       }
                                       free( bytebuffer );
                                    } else {
                                       OBXDBGERR(("[ERROR] ObxTransactionRecv() creating byte buffer.\n"));
                                       return OBX_RC_ERR_MEMORY;
                                    }
                                    /*
                                    ** We don't want to add this one.. since we've combined with an existing one.
                                    */
                                                // T.K. we don't free it here, as it gets freed later on.
                                                // freeing it now will cause memory coruption. Just set the pointer
                                                // to NULL and it gets cleaned up in the end
                                                //iobxHeaderFree( obxheader );
                                    obxheader = NULL;
                                 } else {
                                    /*
                                    ** First one we've seen, so, we'll add it.
                                    */
                                    bodyheader = obxheader;
                                 }
                                        }
                                    } else if ( obxheader->identifier == OBEX_HEADER_TARGET ) {
//Check if the Target header, which is sent from OBEX client in Connect command, conforms to the specified SyncML UUID.
                                        if ( request->cmd != OBEX_CMD_CONNECT ) {
                                            rc = OBX_RC_ERR_SML_TARGET_HDR;
                                            break;
                                        }
                                        targetHeaderCount++;
                                        if(targetHeaderCount>1) {
                                            rc = OBX_RC_ERR_SML_TARGET_HDR;
                                            break;
                                        }
                                        dataLength = iobxBufSize( obxheader->value.byteSequenceValue );
                                        if ( ( bytebuffer=(void *)malloc( dataLength+1 )) ) {
                                            memset(bytebuffer,0,dataLength+1);
                                            if ( iobxBufRead( obxheader->value.byteSequenceValue, bytebuffer, dataLength ) == dataLength) {
                                                if(strcmp(bytebuffer,SYNCML_TARGET)) {
                                                    rc = OBX_RC_ERR_SML_TARGET_HDR;
                                                    free( bytebuffer );
                                                    break;
                                                }
                                            } else {
                                                OBXDBGERR(("[ERROR] ObxTransactionRecv() not enough data in buffer?.\n"));
                                                free( bytebuffer );
                                                return OBX_RC_ERR_MEMORY;
                                            }
                                            free( bytebuffer );
                                        } else {
                                            OBXDBGERR(("[ERROR] ObxTransactionRecv() creating byte buffer.\n"));
                                            return OBX_RC_ERR_MEMORY;
                              }
                           }
                           /*
                           ** If we haven't coalesced this header we simply add it.
                           ** Note that this could result in duplicate headers from the peer.  We make
                           ** no effort to stop them from sending them... we're just a conduit.
                           */
                           if ( obxheader ) {
                              if ( (rc=iobxObjectAddHeader( requestOut, obxheader, handle )) != OBX_RC_OK ) {
                                 OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxObjectAddHeader(), rc=%d\n", rc));
                                 return rc;
                              }
                              /*
                              ** Remove this header from the 'request' since 'requestOut' now owns it.
                              ** The itterator will be 'backed up' from the removed item.  All ready for
                              ** the next pass through the loop.
                              */
                              if ( !(iobxListRemove( obxlist, obxiterator )) ) {
                                 OBXDBGERR(("[ERROR] ObxTransactionRecv() Error removing header from response.\n"));
                                 return OBX_RC_ERR_TRANSPORT;
                              }
                           }
                        } else {
                           OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxIteratorNext(), rc=%d\n", rc));
                           return rc;
                        }
                     }
                     /*
                     ** Done with iterator
                     */
                     iobxIteratorFree( obxiterator );
                  } else {
                     OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxListGetIterator(), rc=%d\n", rc));
                     return rc;
                  }
               } else {
                  OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxObjectGetHeaderList(), rc=%d\n", rc));
                  return rc;
               }

                    if(rc != OBX_RC_OK)
                        break;
               /*
               ** Inbound request a CONNECT?
               ** If so, we see if we can honor their proposed MTU, adjust up only if needed.
               */
               if ( request->cmd == OBEX_CMD_CONNECT ) {
                  /*
                  ** If one's set for us, we must respect our own (not climb above it).
                  */
                  handle->maxPacketLen = min( handle->maxPacketLen,
                                              max( OBEX_MINIMUM_PACKET_LENGTH,
                                                   request->meta.connectMeta->max_packet_length ) );
               }

               /*
               ** Do we send SUCCESS or CONTINUE?... build response.
               */
//                    requestOut->cmd = request->cmd;
               if ( request->cmd & OBEX_CMD_FINAL ) {
                  OBXDBGINFO(("ObxTransactionRecv() FINAL bit from peer.\n"));
                  response->cmd = (OBEX_RSP_SUCCESS | OBEX_CMD_FINAL);

                  /*
                  ** Response could be a response to a Connect or SetPath...
                  ** If so, we use our default meta data in the response
                  ** NOTE:
                  ** We could avoid this by using ObxObjectNewResponse() but
                  ** since we're 'inside' we can just do the adjustment ourselves
                  ** that way we can reuse the response object.
                  */
                  if ( request->cmd == OBEX_CMD_CONNECT ) {
//There must be Target header in Connect command.
                            if( targetHeaderCount ==0 ) {
                                rc=OBX_RC_ERR_SML_TARGET_HDR;
                                break;
                            }
                     if ( (rc=iobxObjectSetConnectMeta( response,
                                                       OBX_CONNECT_META_VERSION,
                                                       OBX_CONNECT_META_FLAGS,
                                                       handle->maxPacketLen )) != OBX_RC_OK ) {
                        OBXDBGERR(("[ERROR] ObxTransactionRecv() unexpected rc from iobxObjectSetConnectMeta(), rc=%d!\n", rc));
                        return rc;
                     }
//There must be headers of ConnectionID and Who in the response of Connection command.
//At the same time, ConnectionID must be saved for later use.
                            srand( (unsigned)time( NULL ) );
                            if ( !(header = iobxHeaderNew( OBEX_HEADER_CONNECTION )) ) {
                                OBXDBGERR(("[ERROR] ObxTransactionRecv() Error creating header.\n"));
                                rc = OBX_RC_ERR_MEMORY;
                            } else if ( (rc=ObxHeaderSetIntValue( header, (handle->OBEXConnectionID=rand()) )) != OBX_RC_OK ) {
                            	iobxHeaderFree(header);
                                OBXDBGERR(("[ERROR] ObxTransactionRecv() unexpected rc calling ObxHeaderSetIntValue().\n"));
                            } else if ( (rc=iobxObjectAddHeader( response, header, handle )) != OBX_RC_OK ) {
                                iobxHeaderFree(header);
                                OBXDBGERR(("[ERROR] ObxTransactionRecv() unexpected rc calling iobxObjectAddHeader().\n"));
                            }
                            header=NULL;
                            if( rc !=OBX_RC_OK )
                                return rc;

                            if ( !(header = iobxHeaderNew( OBEX_HEADER_WHO ))) {
                                OBXDBGERR(("[ERROR] ObxTransactionRecv() Error creating header.\n"));
                                rc = OBX_RC_ERR_MEMORY;
                            } else if ( !(obxbuffer = ObxBufNew( strlen( SYNCML_TARGET ) )) ) {
                                iobxHeaderFree(header);
                                OBXDBGERR(("[ERROR] ObxTransactionRecv() Error creating buffer.\n"));
                                rc = OBX_RC_ERR_MEMORY;
                            } else if((ObxBufWrite( obxbuffer, SYNCML_TARGET, strlen( SYNCML_TARGET )))) {
                                iobxHeaderFree(header);
                                ObxBufFree(obxbuffer);
                                OBXDBGERR(("[ERROR] ObxTransactionRecv() Error appending to header.\n"));                            
                                rc = OBX_RC_ERR_TRANSPORT;
                            } else if ( (rc=ObxHeaderSetByteSequenceValue( header, obxbuffer )) != OBX_RC_OK ) {
                            	ObxBufFree(obxbuffer);
                            	iobxHeaderFree(header);
                                OBXDBGERR(("[ERROR] ObxTransactionRecv() unexpected rc calling ObxHeaderSetByteSequenceValue().\n"));
                            } else if ( (rc=iobxObjectAddHeader( response, header, handle )) != OBX_RC_OK ) {
                            	ObxBufFree(obxbuffer);
                            	iobxHeaderFree(header);
                                OBXDBGERR(("[ERROR] ObxTransactionRecv() unexpected rc calling iobxObjectAddHeader().\n"));
                            }
                            obxbuffer=NULL;
                            header=NULL;
                            if( rc !=OBX_RC_OK )
                                return rc;
                  }
                  done = TRUE;
               } else {
                  OBXDBGINFO(("ObxTransactionRecv() no-FINAL bit from peer.\n"));
                  response->cmd = (OBEX_RSP_CONTINUE | OBEX_CMD_FINAL);
                  /*
                  ** If this is a streaming request, we could be done.
                  */
                  done = requestOut->stream;
               }

               /*
               ** Flow response, success or continue.
               */
               if ( (rc=iobxObjectSendObject( response, handle )) != OBX_RC_OK ) {
                  OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxObjectSendObject() rc=%d\n", rc));
                  done = TRUE;
               }
            } else {
               OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxObjectReset() rc=%d\n", rc));
               done = TRUE;
            }
         } else {
            OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxObjectRecvObject() rc=%d\n", rc));
            done = TRUE;
         }
      } else {
         OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxObjectReset() rc=%d\n", rc));
         done = TRUE;
      }
   }

//Send client a response with a code that indicates what's wrong
   switch(rc) {
   case OBX_RC_ERR_SML_TARGET_HDR:
      OBXDBGERR(("[ERROR] ObxTransactionRecv() received Target header error\n"));
      response->cmd = (OBEX_RSP_BAD_REQUEST | OBEX_CMD_FINAL);
      if ( (rc=iobxObjectSendObject( response, handle )) != OBX_RC_OK ) {
         OBXDBGERR(("[ERROR] ObxTransactionRecv() bad return code from iobxObjectSendObject() rc=%d\n", rc));
      }
   }

   iobxObjectFree( request );
   iobxObjectFree( response );
   return rc;
}
#endif /* RAW_API_ONLY */

OBEX_EXPORT ObxRc ObxObjectRecv( ObxHandle *handle, ObxObject *object ) {
   OBXDBGFLOW(("ObxObjectRecv() entry, handle=0x%08x\tobject=0x%08x\n", handle, object));
   return iobxObjectRecvObject( object, handle );
}

OBEX_EXPORT ObxObject* ObxObjectNew( ObxHandle *handle, ObxCommand cmd ) {
   ObxObject *object = NULL;
   OBXDBGFLOW(("ObxObjectNew() entry, handle=0x%08x\tcmd=%x.\n", handle, cmd));
   if ( (object = iobxObjectNew()) ) {
      if ( iobxObjectReset( object ) == OBX_RC_OK ) {
         object->cmd = cmd;

         switch ( cmd ) {
         case OBEX_CMD_CONNECT:
            if ( (iobxObjectSetConnectMeta( object,
                                            OBX_CONNECT_META_VERSION,
                                            OBX_CONNECT_META_FLAGS,
                                            handle->maxPacketLen )) != OBX_RC_OK ) {
               OBXDBGERR(("[ERROR] ObxObjectNew() unexpected rc from iobxObjectSetConnectMeta()\n"));
               free( object );
               object = NULL;
            }
            break;
         case OBEX_CMD_SETPATH:
            if ( (iobxObjectSetSetPathMeta( object,
                                            OBX_SETPATH_META_FLAGS,
                                            OBX_SETPATH_META_CONSTANTS )) != OBX_RC_OK ) {
               OBXDBGERR(("[ERROR] ObxObjectNew() unexpected rc from iobxObjectSetSetPathMeta()\n"));
               free( object );
               object = NULL;
            }
            break;
         }
      } else {
         free( object );
         object = NULL;
      }
   } else {
      OBXDBGERR(("[ERROR] ObxObjectNew() failure calling iobxObjectNew().\n"));
      object = NULL;
   }
   return object;
}

OBEX_EXPORT ObxObject*     ObxObjectNewResponse( ObxHandle *handle, ObxObject *request, ObxCommand cmd ) {
   ObxObject *object = NULL;
   OBXDBGFLOW(("ObxObjectNewResponse() entry, handle=0x%08x\trequest=0x%08x\tcmd=%x.\n", handle, request, cmd));
   if ( (object=ObxObjectNew( handle, cmd )) ) {
      if ( request->cmd == OBEX_CMD_CONNECT ) {
         if ( (iobxObjectSetConnectMeta( object,
                                         OBX_CONNECT_META_VERSION,
                                         OBX_CONNECT_META_FLAGS,
                                         handle->maxPacketLen )) != OBX_RC_OK ) {
            OBXDBGERR(("[ERROR] ObxObjectNewResponse() unexpected rc from iobxObjectSetConnectMeta()\n"));
            free( object );
            object = NULL;
         }
      }
   }
   return object;
}

OBEX_EXPORT ObxRc ObxObjectFree( ObxHandle *handle, ObxObject *object ) {
   OBXDBGFLOW(("ObxObjectFree() entry, handle=0x%08x\tobject=0x%08x\n", handle, object));
   iobxObjectFree( object );
   return OBX_RC_OK;
}

OBEX_EXPORT ObxRc ObxObjectReset( ObxObject *object ) {
   OBXDBGFLOW(("ObxObjectReset() entry, object=0x%08x\n", object));
   return iobxObjectReset( object );
}

OBEX_EXPORT ObxRc ObxObjectAddHeader( ObxHandle *handle, ObxObject *object, ObxHeader *header ) {
   OBXDBGFLOW(("ObxObjectAddHeader() entry, handle=0x%08x\tobject=0x%08x\theader=0x%08x\n", handle, object, header));
   return iobxObjectAddHeader( object, header, handle );
}

OBEX_EXPORT ObxList *ObxObjectGetHeaderList( ObxObject *object ) {
   OBXDBGFLOW(("ObxObjectGetHeaderList() entry, object=0x%08x\n", object));
   return iobxObjectGetHeaderList( object );
}

OBEX_EXPORT ObxCommand ObxObjectGetCommand( ObxObject *object ) {
   OBXDBGFLOW(("ObxObjectGetCommand() entry, object=0x%08x\n", object));
   return (object ? object->cmd : -1);
}


OBEX_EXPORT ObxHeader *ObxHeaderNew( unsigned char opcode ) {
   OBXDBGFLOW(("ObxHeaderNew() entry.\n"));
   return iobxHeaderNew( opcode );
}

OBEX_EXPORT ObxRc ObxHeaderSetIntValue( ObxHeader *header, unsigned int value ) {
   OBXDBGFLOW(("ObxHeaderSetIntValue() entry, header=0x%08x\tvalue=0x%08x\n",header,value));
   return iobxHeaderSetIntValue( header, value );
}

OBEX_EXPORT ObxRc ObxHeaderSetByteValue( ObxHeader *header, unsigned char value ) {
   OBXDBGFLOW(("ObxHeaderSetByteValue() entry, header=0x%08x\tvalue=%x\n",header,value));
   return iobxHeaderSetByteValue( header, value );
}

OBEX_EXPORT ObxRc ObxHeaderSetUnicodeValue( ObxHeader *header, ObxBuffer *value ) {
   OBXDBGFLOW(("ObxHeaderSetUnicodeValue() entry, header=0x%08x\tvalue(buffer)=0x%08x\n",header,value));
   return iobxHeaderSetUnicodeValue( header, value );
}

OBEX_EXPORT ObxRc ObxHeaderSetByteSequenceValue( ObxHeader *header, ObxBuffer *value ) {
   OBXDBGFLOW(("ObxHeaderSetByteSequenceValue() entry, header=0x%08x\tvalue(buffer)=0x%08x\n",header,value));
   return iobxHeaderSetByteSequenceValue( header, value );
}

OBEX_EXPORT ObxHeader *ObxHeaderNewFromBuffer( ObxBuffer *buffer, ObxObject *object ) {
   OBXDBGFLOW(("ObxHeaderNewFromBuffer() entry, buffer=0x%08x\n",buffer));
   return iobxHeaderNewFromBuffer( buffer, object );
}

OBEX_EXPORT void ObxHeaderFree( ObxHeader *header ) {
   OBXDBGFLOW(("ObxHeaderFree() entry, header=0x%08x\n",header));
   iobxHeaderFree( header );
}

OBEX_EXPORT int ObxHeaderSize( ObxHeader *header ) {
   OBXDBGFLOW(("ObxHeaderSize() entry, header=0x%08x\n",header));
   return iobxHeaderSize( header );
}

OBEX_EXPORT ObxRc ObxHeaderAdd( ObxHandle *handle, ObxObject *object, ObxHeader *header ) {
   OBXDBGFLOW(("ObxHeaderAdd() entry, handle=0x%08x\tobject=0x%08x\theader=0x%08x\n", handle, object, header));
   return iobxObjectAddHeader( object, header, handle );
}

OBEX_EXPORT ObxList *ObxGetHeaderList( ObxHandle *handle, ObxObject *object ) {
   OBXDBGFLOW(("ObxGetHeaderList() entry, handle=0x%08x\tobject=0x%08x\n", handle, object));
   return iobxObjectGetHeaderList( object );
}

OBEX_EXPORT ObxBuffer   *ObxBufNew( int len ) {
   OBXDBGFLOW(("ObxBufNew() entry, len=%d\n", len));
   return iobxBufNew( len );
}

OBEX_EXPORT void        ObxBufFree( ObxBuffer *buf ) {
   OBXDBGFLOW(("ObxBufFree() entry, buf=0x%08x\n", buf));
   iobxBufFree( buf );
}

OBEX_EXPORT ObxBuffer   *ObxBufReset( ObxBuffer *buf ) {
   OBXDBGFLOW(("ObxBufReset() entry, buf=0x%08x\n", buf));
   return iobxBufReset( buf );
}

OBEX_EXPORT int         ObxBufWrite( ObxBuffer *buf, void *data, int len ) {
   OBXDBGFLOW(("ObxBufWrite() entry, buf=0x%08x\tdata=0x%08x\tlen=%d\n", buf, data, len));
   OBXDBGMEM(("ObxBufWrite()", data, len));
   return iobxBufWrite( buf, data, len );
}

OBEX_EXPORT int         ObxBufPeek( ObxBuffer *buf, void *data, int len ) {
   OBXDBGFLOW(("ObxBufPeek() entry, buf=0x%08x\tdata=0x%08x\tlen=%d\n", buf, data, len));
   return iobxBufPeek( buf, data, len );
}

OBEX_EXPORT int         ObxBufRead( ObxBuffer *buf, void *data, int len ) {
   OBXDBGFLOW(("ObxBufRead() entry, buf=0x%08x\tdata=0x%08x\tlen=%d\n", buf, data, len));
   return iobxBufRead( buf, data, len );
}

OBEX_EXPORT int         ObxBufSize( ObxBuffer *buf ) {
   OBXDBGFLOW(("ObxBufRead() entry, buf=0x%08x\n", buf));
   return iobxBufSize( buf );
}


OBEX_EXPORT ObxList *ObxListNew() {
   OBXDBGFLOW(("ObxListNew() entry.\n"));
   return iobxListNew();
}

OBEX_EXPORT void ObxListFree( ObxList *list ) {
   OBXDBGFLOW(("ObxListFree() entry, list=0x%08x\n", list));
   iobxListFree( list );
}

OBEX_EXPORT void ObxListReset( ObxList *list ) {
   OBXDBGFLOW(("ObxListReset() entry, list=0x%08x\n", list));
   iobxListReset( list );
}

OBEX_EXPORT void ObxListRelease( ObxList *list ) {
   OBXDBGFLOW(("ObxListRelease() entry, list=0x%08x\n", list));
   iobxListRelease( list );
}

OBEX_EXPORT void *ObxListAppend( ObxList *list, void *data ) {
   OBXDBGFLOW(("ObxListAppend() entry, list=0x%08x\n", list));
   return iobxListAppend( list, data );
}

OBEX_EXPORT ObxIterator *ObxListGetIterator( ObxList *list ) {
   OBXDBGFLOW(("ObxListGetIterator() entry, list=0x%08x\n", list));
   return iobxListGetIterator( list );
}

OBEX_EXPORT void *ObxListInsert( ObxList *list, const ObxIterator *iterator, void *data ) {
   OBXDBGFLOW(("ObxListInsert() entry, list=0x%08x\titerator=0x%08x\n", list,iterator));
   return iobxListInsert( list, iterator, data );
}

OBEX_EXPORT void *ObxListRemove( ObxList *list, ObxIterator *iterator ) {
   OBXDBGFLOW(("ObxListRemove() entry, list=0x%08x\titerator=0x%08x\n", list,iterator));
   return iobxListRemove( list, iterator );
}

OBEX_EXPORT void ObxListDelete( ObxList *list, ObxIterator *iterator ) {
   OBXDBGFLOW(("ObxListDelete() entry, list=0x%08x\titerator=0x%08x\n", list,iterator));
   iobxListDelete( list, iterator );
}

OBEX_EXPORT ObxIterator *ObxIteratorReset( ObxIterator *iterator ) {
   OBXDBGFLOW(("ObxIteratorReset() entry, iterator=0x%08x\n", iterator));
   return iobxIteratorReset( iterator );
}

OBEX_EXPORT void ObxIteratorFree( ObxIterator *iterator ) {
   OBXDBGFLOW(("ObxIteratorFree() entry, iterator=0x%08x\n", iterator));
   iobxIteratorFree( iterator );
}

OBEX_EXPORT void *ObxIteratorNext( ObxIterator *iterator ) {
   OBXDBGFLOW(("ObxIteratorNext() entry, iterator=0x%08x\n", iterator));
   return iobxIteratorNext( iterator );
}

OBEX_EXPORT short ObxIteratorHasNext( ObxIterator *iterator ) {
   OBXDBGFLOW(("ObxIteratorHasNext() entry, iterator=0x%08x\n", iterator));
   return iobxIteratorHasNext( iterator );
}

OBEX_EXPORT ObxBuffer *ObxUnicodeToUTF8( const unsigned char *uc, int len ) {
   OBXDBGFLOW(("ObxUnicodeToUTF8() entry."));
   return iobxUnicodeToUTF8( uc, len );
}

OBEX_EXPORT ObxBuffer *ObxUTF8ToUnicode( const unsigned char *utf8, int len ) {
   OBXDBGFLOW(("ObxUTF8ToUnicode() entry."));
   return iobxUTF8ToUnicode( utf8, len );
}

