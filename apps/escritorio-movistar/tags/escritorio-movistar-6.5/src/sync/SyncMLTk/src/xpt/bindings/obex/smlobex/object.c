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
#include <object.h>

#include <iConstants.h>

#include <utils.h>
#include <buffer.h>

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

/*
** Creates and initializes a new ObxObject structure.
*/
ObxObject      *iobxObjectNew() {
   ObxObject *object = NULL;
   OBXDBGFLOW(("iobxObjectNew() entry.\n"));
   if ( (object = (ObxObject *)malloc( sizeof( ObxObject ) )) ) {
      OBXDBGBUF(("iobxObjectNew() malloc, addr=0x%08x, len=%d.\n", object, sizeof(ObxObject)));
      object->stream = 0;
      object->headers = NULL;
      object->meta.connectMeta = NULL;
      object->meta.setPathMeta = NULL; /* Not really needed, since union */
   }
   return object;
}

/*
** Destroys the passed ObxObject structure.  All internal structures and
** the ObxObject structure itself is free()'ed.
*/
void           iobxObjectFree( ObxObject *object ) {
   ObxIterator  *obxiterator = NULL;
   OBXDBGFLOW(("iobxObjectFree() entry, object=0x%08x\n", object));
   if ( object ) {
      if ( object->headers ) {
         if ( (obxiterator=iobxListGetIterator( object->headers )) ) {
            iobxIteratorReset( obxiterator );
            while ( iobxIteratorHasNext( obxiterator ) ) {
                ObxHeader *tempHeader =(ObxHeader *)iobxIteratorNext( obxiterator ) ;
               iobxHeaderFree( tempHeader);
               tempHeader = NULL;
            }
            iobxListRelease( object->headers );
            iobxListFree( object->headers );
            object->headers = NULL;
         }
         if ( obxiterator ) {
            iobxIteratorFree( obxiterator );
         }
      }
      /*
      ** Since it's a union, and both types are pointers to structs..
      ** we just free one.. it will get the other... :-)
      */
      if ( object->meta.connectMeta ) {
         free( (void *)object->meta.connectMeta );
         object->meta.connectMeta = NULL;
         object->meta.setPathMeta = NULL; /* Redundant... */
      }
      free( object );
   }
}

/*
** The passed ObxObject is cleared to an initial state.  Allows users to
** reuse objects.
*/
ObxRc          iobxObjectReset( ObxObject *object ) {
   ObxRc rc = OBX_RC_OK;
   OBXDBGFLOW(("iobxObjectReset() entry, object=0x%08x\n", object));
   if ( object ) {
      /*
      ** Free/setmeta
      */
      /*
      ** Since it's a union, and both types are pointers to structs..
      ** we just free one.. it will get the other... :-)
      */
      if ( object->meta.connectMeta ) {
         free( (void *)object->meta.connectMeta );
         object->meta.connectMeta = NULL;
         object->meta.setPathMeta = NULL; /* Redundant... */
      }
      object->cmd = 0;
      object->stream = 0;
      /*
      ** Recycle the headers list, if it exists, if not, create it.
      */
      if ( object->headers ) {
         //iobxListReset( object->headers );
          iobxListRelease( object->headers );
          iobxListFree( object->headers );
         if ( !(object->headers = iobxListNew()) ) {
            OBXDBGERR(("[ERROR] iobxObjectReset() failure creating list.\n"));
            rc = OBX_RC_ERR_MEMORY;
         }

      } else {
         if ( !(object->headers = iobxListNew()) ) {
            OBXDBGERR(("[ERROR] iobxObjectReset() failure creating list.\n"));
            rc = OBX_RC_ERR_MEMORY;
         }
      }

      object->state = STATE_BUILDING;
   } else {
      OBXDBGERR(("[ERROR] iobxObjectReset() bad plist on call.\n"));
      rc = OBX_RC_ERR_BADPLIST;
   }
   return rc;
}

/*
** Adds a header to the list of headers associated with the object.
*/
ObxRc          iobxObjectAddHeader( ObxObject *object, ObxHeader *newheader, ObxHandle *handle ) {
   ObxRc          rc = OBX_RC_OK;
   int            availLen = 0;
   ObxIterator    *iterator = NULL;
   ObxHeader      *header = NULL;

   OBXDBGFLOW(("iobxObjectAddHeader() entry, object=0x%08x\theader=0x%08x\thandle=0x%08x\n", object, newheader, handle));
   if ( object && newheader && handle ) {
      if ( object->state == STATE_BUILDING ) {
         if ( object->headers ) {
            if ( newheader->identifier == OBEX_HEADER_BODY || newheader->identifier == OBEX_HEADER_BODY_END ) {
               /*
               ** Just add it.
               */
               iobxListAppend( object->headers, newheader );
            } else {
               /*
               ** Will inbound non-body header fit within an MTU?
               */
               if ( (iterator = iobxListGetIterator( object->headers )) ) {
                  /*
                  ** Calc size
                  ** [MJH] Maybe I should just have an incrementing int or something.. avoid
                  ** iteration for each header...
                  */
                  iterator = iobxIteratorReset( iterator );
                  availLen = handle->maxPacketLen - iobxObjectPrefixSize( object, handle );
                  while ( iobxIteratorHasNext( iterator ) ) {
                     if ( (header = (ObxHeader *)iobxIteratorNext( iterator )) ) {

                        /*
                        ** Determine amount of non-body data we're holding.
                        */
                        if ( header->identifier != OBEX_HEADER_BODY && header->identifier != OBEX_HEADER_BODY_END ) {
                           availLen -= iobxHeaderSize( header );
                        }
                     }
                  }
               } else {
                  OBXDBGERR(("[ERROR] iobxObjectAddHeader() error calling iobxListGetIterator().\n"));
                  return OBX_RC_ERR_TRANSPORT;
               }

               iobxIteratorFree( iterator );

               if ( availLen >= iobxHeaderSize( newheader ) ) {
                  iobxListAppend( object->headers, newheader );
               } else {
                  OBXDBGERR(("[ERROR] iobxObjectAddHeader() inbound header would cause object size to exceed maximum packet length of %d.\n", handle->maxPacketLen));
                  rc = OBX_RC_ERR_OBJ_HDR_NO_FIT;
               }
            }
         } else {
            OBXDBGERR(("[ERROR] iobxObjectAddHeader() list object does not appear to be initialized.\n"));
            rc = OBX_RC_ERR_NOTINITIALIZED;
         }
      } else {
         OBXDBGERR(("[ERROR] iobxObjectAddHeader() object not 'building', inappropriate call.\n"));
         rc = OBX_RC_ERR_INAPPROPRIATE;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxObjectAddHeader() bad plist on call.\n"));
      rc = OBX_RC_ERR_BADPLIST;
   }
   return rc;
}

/*
** Requests the current list of headers be returned.  From this object users
** can get an iterator and manipulate the header list.
*/
ObxList        *iobxObjectGetHeaderList( ObxObject *object ) {
   OBXDBGFLOW(("iobxObjectGetHeaderList() entry, object=0x%08x\n", object));
   return (object ? object->headers : NULL);
}

/*
** What's the prefix size of this object?
*/
int         iobxObjectPrefixSize( ObxObject *object, ObxHandle *handle ) {
   int prefixSize = 0;

   if (   handle->lastRecv == OBEX_CMD_CONNECT || object->cmd == OBEX_CMD_CONNECT ) {
      /*
      ** This implies that we're responding to a connect, or connecting anew.
      ** In either case, our prefix length (because of the meta data) is the same.
      */
      prefixSize = 7;
   } else if ( object->cmd == OBEX_CMD_SETPATH ) {
      /*
      ** We're using setpath.  SetPath also has meta data, although it's of
      ** a diffrent format and length than the connect meta.
      */
      prefixSize = 5;
   } else {
      /*
      ** No meta data... just one byte of op code followed by two bytes of length.
      */
      prefixSize = 3;
   }
   return prefixSize;
}


/*
** The current object is sent using the transport associated with the passed ObxHandle.
** This method will send any leading information, then iterate through the headers sending
** each of them.
** Note, this function will send what it can, it's up to the caller to deal with partial
** sends (by reinvoking after receiving a continue response from the peer).
*/
ObxRc          iobxObjectSendObject( ObxObject *object, ObxHandle *handle ) {
   ObxRc          rc = OBX_RC_OK;
   int            nonBodyHeaderSize = 0;
   int            bodyHeaderSize = 0;
   int            prefixSize = 0;
   int            thisHeaderSize = 0;
   short          sendLength = 0;
   short          nboLength = 0;
   int            wrote = 0;
   ObxIterator    *iterator = NULL;
   ObxHeader      *header = NULL;
   short          done = FALSE;
   short          fsmState = OBJECT_FSM_CHECK_PRECONDITONS;

   OBXDBGFLOW(("iobxObjectSendObject() entry, object=0x%08x\thandle=0x%08x\n", object, handle));

   while ( rc == OBX_RC_OK && !done ) {
      switch ( fsmState ) {

      /*
      ** Ensure plist is ok and object is in the correct state.
      */
      case OBJECT_FSM_CHECK_PRECONDITONS:
         OBXDBGINFO(("iobxObjectSendObject() check preconditions\n"));
         if ( object && handle ) {
            if ( object->state == STATE_ERROR ) {
               OBXDBGERR(("[ERROR] iobxObjectSendObject() object is in STATE_ERROR, inappropriate call.\n"));
               rc = OBX_RC_ERR_INAPPROPRIATE;
            }
         } else {
            OBXDBGERR(("[ERROR] iobxObjectSendObject() bad plist on call.\n"));
            rc = OBX_RC_ERR_BADPLIST;
         }
         fsmState = OBJECT_FSM_DETERMINE_LENGTHS;
         break;

      /*
      ** Determine the prefix and non-body header lengths.
      ** Ensure that the non-body headers (and the prefix data) will fit within the
      ** maximum packet length.
      */
      case OBJECT_FSM_DETERMINE_LENGTHS:
         OBXDBGINFO(("iobxObjectSendObject() determine lengths\n"));

         prefixSize = iobxObjectPrefixSize( object, handle );

         /*
         ** Determine size of body headers and non-body headers.
         */
         nonBodyHeaderSize = 0;
         bodyHeaderSize = 0;
         if ( (iterator = iobxListGetIterator( object->headers )) ) {
            iterator = iobxIteratorReset( iterator );
            while ( iobxIteratorHasNext( iterator ) ) {
               if ( (header = (ObxHeader *)iobxIteratorNext( iterator )) ) {
                  if ( header->state != STATE_SENT ) {
                     if ( header->identifier == OBEX_HEADER_BODY || header->identifier == OBEX_HEADER_BODY_END ) {
                        bodyHeaderSize += iobxHeaderSize( header );
                     } else {
                        nonBodyHeaderSize += iobxHeaderSize( header );
                     }
                  }
               }
            }
            /*
            ** Will all non-body headers fit within this single packet?
            */
            if ( (prefixSize + nonBodyHeaderSize) > handle->maxPacketLen ) {
               OBXDBGERR(("[ERROR] iobxObjectSendObject() headers appear to exceed maximum packet length of %d.\n", handle->maxPacketLen));
               rc = OBX_RC_ERR_OBJ_BAD_HEADER;
            }

         } else {
            OBXDBGERR(("[ERROR] iobxObjectSendObject() unable to obtain header list iterator.\n"));
            rc = OBX_RC_ERR_NOTINITIALIZED;
         }
         fsmState = OBJECT_FSM_CHECK_FOR_FINAL;
         break;

      /*
      ** Set the final bit, if required.  Also establish the actual send length and
      ** set the object state as required.
      */
      case OBJECT_FSM_CHECK_FOR_FINAL:
         OBXDBGINFO(("iobxObjectSendObject() check for final\n"));

         /*
         ** We make allowences for Win32 here...
         */
         if ( nonBodyHeaderSize ) {
            /*
            ** Non-body headers exist and so do
            ** Clear final bit, must send body seperately.
            ** We bow to Win32 here... spec says that it's allowed, win32 won't allow it... chumps.
            */
            if ( bodyHeaderSize ) {
               /*
               ** Body headers will be sent on a susuquent call.
               */
//Every response must have final bit set.
//               object->cmd &= (~OBEX_CMD_FINAL);
               object->state = STATE_SENDING;
            } else {
               /*
               ** No body headers, we're done unless we're in a stream.
               */
//The last response of Get command have response code of 0xA0, i.e. OBEX_RSP_SUCCESS | OBEX_CMD_FINAL.
               if(handle->lastRecv == (OBEX_CMD_GET | OBEX_CMD_FINAL) ) {
               object->cmd = ( object->stream ?
//                                     (object->cmd & ~OBEX_CMD_FINAL) :   /* Stream, clear final, more to come. */
//                                     (object->cmd | OBEX_CMD_FINAL) );   /* Non Stream, set final */
                                     object->cmd :   /* Stream */
                                     (OBEX_RSP_SUCCESS | OBEX_CMD_FINAL) );   /* Non Stream */
                   object->state = STATE_SENT;
               } else {
                   object->cmd = ( object->stream ?
                                 (object->cmd & ~OBEX_CMD_FINAL) :   /* Stream, clear final, more to come. */
                                 (object->cmd | OBEX_CMD_FINAL) );   /* Non Stream, set final */
               object->state = STATE_SENT;
               }
            }
            sendLength = prefixSize + nonBodyHeaderSize;
         } else if ( prefixSize + bodyHeaderSize <= handle->maxPacketLen ) {
            /*
            ** Existing body content will fit in a single send.
            ** Set final bit, if not in a stream.  A stream implies that more
            ** data will be sent at a later date... but that will be a seperate
            ** object... in either case, this one's SENT.
            */
//The last response of Get command have response code of 0xA0, i.e. OBEX_RSP_SUCCESS | OBEX_CMD_FINAL.
            if(handle->lastRecv == (OBEX_CMD_GET | OBEX_CMD_FINAL) ) {
            object->cmd = ( object->stream ?
//                                  (object->cmd & ~OBEX_CMD_FINAL) :   /* Stream, clear final, more to come. */
//                                  (object->cmd | OBEX_CMD_FINAL) );   /* Non Stream, set final */
                                  object->cmd :   /* Stream */
                                  (OBEX_RSP_SUCCESS | OBEX_CMD_FINAL) );   /* Non Stream */
                object->state = STATE_SENT;
            } else {
               object->cmd = ( object->stream ?
                              (object->cmd & ~OBEX_CMD_FINAL) :   /* Stream, clear final, more to come. */
                              (object->cmd | OBEX_CMD_FINAL) );   /* Non Stream, set final */
            object->state = STATE_SENT;
            }
            sendLength = prefixSize + bodyHeaderSize;
         } else {
            /*
            ** We have body that will require multiple sends.
            ** Clear final bit.
            */
//Every response of Get command must have final bit set.
//            object->cmd &= (~OBEX_CMD_FINAL);
            object->state = STATE_SENDING;
            sendLength = handle->maxPacketLen;
         }
         fsmState = OBJECT_FSM_SEND_OPCODE;
         break;

      /*
      ** Send the opcode.
      */
      case OBJECT_FSM_SEND_OPCODE:
         OBXDBGINFO(("iobxObjectSendObject() *** outbound *** opcode = %02x\n", object->cmd));
         rc = handle->transport->send( &handle->connectionId, &object->cmd, 1, &wrote, FALSE );
         handle->lastSent = object->cmd;
         fsmState = OBJECT_FSM_SEND_LENGTH;
         break;

      /*
      ** Send the two byte length.
      */
      case OBJECT_FSM_SEND_LENGTH:
         OBXDBGINFO(("iobxObjectSendObject() sending length, %d\n", sendLength));
         nboLength = htons( sendLength );
         rc = handle->transport->send( &handle->connectionId, &nboLength,
                                       sizeof( nboLength ), &wrote, FALSE );
         /*
         ** Where to next?
         */
         if (    handle->lastRecv  == OBEX_CMD_CONNECT     /* Connect response */
              || object->cmd  == OBEX_CMD_CONNECT          /* Connect anew */
              || object->cmd  == OBEX_CMD_SETPATH ) {      /* SetPath anew */
            /*
            ** These situations (Connect response, connect, setpath) all
            ** have meta data send requirements.
            */
            fsmState = OBJECT_FSM_SEND_META;
         } else if ( nonBodyHeaderSize ) {
            /*
            ** If non body headers exist, we must send them.
            */
            fsmState = OBJECT_FSM_SEND_NONBODY;
         } else {
            /*
            ** No non-body headers, we're free to send body headers.
            */
            fsmState = OBJECT_FSM_SEND_BODY;
         }
         /*
         ** We've sent the opcode and length, reduce remaining send amount.
         */
         sendLength -= 3;
         break;

      /*
      ** Send the meta data associated with this object (if appropriate).
      */
      case OBJECT_FSM_SEND_META:
         OBXDBGINFO(("iobxObjectSendObject() sending meta\n"));
         if (    handle->lastRecv == OBEX_CMD_CONNECT || object->cmd == OBEX_CMD_CONNECT ) {
            /*
            ** We're either connecting, or responding to a connect in this case.
            */
            rc = handle->transport->send( &handle->connectionId, object->meta.connectMeta,
                                          sizeof(object->meta.connectMeta), &wrote, FALSE );
            sendLength -= sizeof(object->meta.connectMeta);
            OBXDBGINFO(("iobxObjectSendObject() sent meta for Connect / Connect response.\n"));
         } else if ( (object->cmd & ~OBEX_CMD_FINAL) == OBEX_CMD_SETPATH ) {
            rc = handle->transport->send( &handle->connectionId, object->meta.setPathMeta,
                                          sizeof(object->meta.setPathMeta), &wrote, FALSE );
            sendLength -= sizeof(object->meta.setPathMeta);
            OBXDBGINFO(("iobxObjectSendObject() sent meta for SetPath.\n"));
         } else {
            OBXDBGERR(("[ERROR] iobxObjectSendObject() FSM attempts to send meta data when it's inappropriate.\n"));
            rc = OBX_RC_ERR_UNSPECIFIED;
         }
         if ( nonBodyHeaderSize ) {
            /*
            ** If non body headers exist, we must send them.
            */
            fsmState = OBJECT_FSM_SEND_NONBODY;
         } else {
            /*
            ** No non-body headers, we're free to send body headers.
            */
            fsmState = OBJECT_FSM_SEND_BODY;
         }
         break;

      /*
      ** Send the non-body headers, if any, associated with this object.
      */
      case OBJECT_FSM_SEND_NONBODY:
         OBXDBGINFO(("iobxObjectSendObject() sending non body headers...\n"));
         iterator = iobxIteratorReset( iterator );
         while ( iobxIteratorHasNext( iterator ) && rc == OBX_RC_OK ) {
            if ( (header = (ObxHeader *)iobxIteratorNext( iterator )) ) {
               /*
               ** Send each non body header (if it's not already sent).
               ** If we send, reduce our available send ammount.
               */
               if ( header->state != STATE_SENT && header->identifier != OBEX_HEADER_BODY && header->identifier != OBEX_HEADER_BODY_END ) {
                  sendLength -= iobxHeaderSize( header );
                  OBXDBGINFO(("iobxObjectSendObject() sending non body header %02x.\n", header->identifier));
                  rc = iobxHeaderSend( header, handle, OBEX_HEADER_SEND_ALL );
               }
            }
         }
         fsmState = OBJECT_FSM_FINISH;
         break;

      /*
      ** Send the body header, if any, associated with this object.
      */
      case OBJECT_FSM_SEND_BODY:
         OBXDBGINFO(("iobxObjectSendObject() sending body header\n"));
         iterator = iobxIteratorReset( iterator );
         while ( iobxIteratorHasNext( iterator ) && sendLength > 0 ) {
            if ( (header = (ObxHeader *)iobxIteratorNext( iterator )) ) {
               if ( header->state != STATE_SENT ) {
                  if ( header->identifier == OBEX_HEADER_BODY || header->identifier == OBEX_HEADER_BODY_END ) {
                     OBXDBGINFO(("iobxObjectSendObject() sending body header %02x.\n", header->identifier));
                     thisHeaderSize = iobxHeaderSize( header );
                     rc = iobxHeaderSend( header, handle, sendLength );
                     sendLength -= thisHeaderSize;
                  }
               }
            }
         }
         fsmState = OBJECT_FSM_FINISH;
         break;

      /*
      ** Finish up.
      */
      case OBJECT_FSM_FINISH:
         iobxIteratorFree( iterator );
         done = TRUE;
         break;
      }
   }
   return rc;
}

/*
** The passed object is populated by reading from the transport layer associated with the
** passed ObxHandle object.
*/
ObxRc          iobxObjectRecvObject( ObxObject *object, ObxHandle *handle ) {
   ObxRc          rc = OBX_RC_OK;
   unsigned short packetLen;
   int            length, actual;
   ObxBuffer      *buffer;
   ObxConnectMeta *connectMeta;
   ObxSetPathMeta *setpathMeta;
   ObxHeader      *header;
   void           *bytebuffer;

   OBXDBGFLOW(("iobxObjectRecvObject() entry, object=0x%08x\thandle=0x%08x\n", object, handle));

   /*
   ** Inbound message format:
   **
   **          1 byte op-code        (i.e. OBX_CMD_CONNECT, OBX_CMD_PUT, OBX_CMD_GET, etc )
   **          2 byte packet length  (Entire message length, including the op-code and length bytes)
   **          ['n' meta bytes]      (optional bytes. CONNECT/SETPATH have 'extra' bytes, bone head design)
   **          'n' headers           (optional headers)
   */

   /*
   ** Read op code
   */
   if ( (rc=handle->transport->recv( &handle->connectionId, &object->cmd, 1, &actual, FALSE )) == OBX_RC_OK ) {
      handle->lastRecv = object->cmd;
      OBXDBGINFO(("iobxObjectRecvObject() *** inbound *** opcode = %02x\n", object->cmd));
      /*
      ** Read length
      */
      if ( (rc=handle->transport->recv( &handle->connectionId, &packetLen, 2, &actual, FALSE )) == OBX_RC_OK ) {
         length = ntohs( packetLen );
         OBXDBGINFO(("iobxObjectRecvObject() cmd length = %d\n", length));
         /*
         ** We've got the opcode and length bytes...
         */
         length -= 3;

         /*
         ** Meta bytes expected?  If so, gobble them first.  For some reason, the designers
         ** of Obex choose to put meta bytes in the frame instead of just using a variable length
         ** header.
         */
         if ( handle->lastSent == OBEX_CMD_CONNECT || object->cmd == OBEX_CMD_CONNECT ) {
            OBXDBGINFO(("iobxObjectRecvObject() inbound command CONNECT or last sent was CONNECT\n"));
            /*
            ** We're either CONNECTing, or receiving a response from a prior CONNECT, thus
            ** we read meta bytes.
            */
            if ( (connectMeta = (ObxConnectMeta *)malloc( sizeof(ObxConnectMeta) )) ) {
               OBXDBGBUF(("iobxObjectRecvObject() malloc, addr=0x%08x, len=%d.\n", connectMeta, sizeof(ObxConnectMeta)));

               if ( (rc=handle->transport->recv( &handle->connectionId, connectMeta, sizeof( ObxConnectMeta ), &actual, FALSE )) == OBX_RC_OK ) {
                  object->meta.connectMeta = connectMeta;
                  // MTU is still in NetworkByteOrder - we need to change that
                  object->meta.connectMeta->max_packet_length = ntohs(connectMeta->max_packet_length);
               } else {
                  OBXDBGERR(("[ERROR] iobxObjectRecvObject() error filling ObxConnectMeta structure!\n"));
                  return rc;
               }
            } else {
               OBXDBGERR(("[ERROR] iobxObjectRecvObject() error obtaining ObxConnectMeta structure!\n"));
               return OBX_RC_ERR_MEMORY;
            }
            length -= sizeof( ObxConnectMeta );
         } else if ( (object->cmd & ~OBEX_CMD_FINAL) == OBEX_CMD_SETPATH ) {
            OBXDBGINFO(("iobxObjectRecvObject() following SETPATH\n"));
            /*
            ** SetPath, too, causes us to expect meta bytes.
            */
            if ( (setpathMeta = (ObxSetPathMeta *)malloc( sizeof(ObxSetPathMeta) )) ) {
               OBXDBGBUF(("iobxObjectRecvObject() malloc, addr=0x%08x, len=%d.\n", setpathMeta, sizeof(ObxSetPathMeta)));
               if ( (rc=handle->transport->recv( &handle->connectionId, setpathMeta, sizeof( ObxSetPathMeta ), &actual, FALSE )) == OBX_RC_OK ) {
                  object->meta.setPathMeta = setpathMeta;
               } else {
                  OBXDBGERR(("[ERROR] iobxObjectRecvObject() error filling ObxSetPathMeta structure!\n"));
                  return rc;
               }
            } else {
               OBXDBGERR(("[ERROR] iobxObjectRecvObject() error obtaining ObxSetPathMeta structure!\n"));
               return OBX_RC_ERR_MEMORY;
            }
            length -= sizeof( ObxSetPathMeta );
         } else {
            /*
            ** inbound cmd has no meta associated with it.
            */
         }

         /*
         ** Any bytes expected?
         */
         if ( length > 0 ) {
            /*
            ** Read any header information, gobble all of them.
            */
            if ( (buffer = iobxBufNew( length )) ) {
               if ( !(bytebuffer=(void *)malloc(length)) ) {
                  OBXDBGERR(("[ERROR] iobxObjectRecvObject() error creating bytebuffer!\n"));
                  return OBX_RC_ERR_MEMORY;
               }
               OBXDBGBUF(("iobxObjectRecvObject() malloc, addr=0x%08x, len=%d.\n", bytebuffer, length));
               if ( (rc=handle->transport->recv( &handle->connectionId, bytebuffer, length, &actual, FALSE )) == OBX_RC_OK ) {
                  /*
                  ** Build headers from the buffer (if any).
                  */
                  iobxBufWrite( buffer, bytebuffer, actual );
                  while ( iobxBufSize( buffer ) > 0 && rc == OBX_RC_OK ) {
                     if ( (header=iobxHeaderNewFromBuffer( buffer, object )) ) {
                        rc = iobxObjectAddHeader( object, header, handle );
                     } else {
                        OBXDBGERR(("[ERROR] iobxObjectRecvObject() error creating header from buffer contents.\n"));
                        rc = OBX_RC_ERR_UNSPECIFIED;
                     }
                  }
               } else {
                  OBXDBGERR(("[ERROR] iobxObjectRecvObject() unexpected return code reading object data!\n"));
               }
               free( bytebuffer );
               free( buffer );
            } else {
               OBXDBGERR(("[ERROR] iobxObjectRecvObject() error obtaining obxBuffer!\n"));
               rc = OBX_RC_ERR_MEMORY;
            }
         } else {
            // No bytes expected
         }
      } else {
         OBXDBGERR(("[ERROR] iobxObjectRecvObject() unexpected return code reading object length!\n"));
      }
   } else {
      OBXDBGERR(("[ERROR] iobxObjectRecvObject() unexpected return code reading object opcode!\n"));
   }
   return rc;
}

ObxRc iobxObjectSetConnectMeta( ObxObject *object, unsigned char version, unsigned char flags, unsigned short hbo_mtu ) {
   ObxRc    rc = OBX_RC_OK;
   OBXDBGFLOW(("iobxObjectSetConnectMeta() entry, object=0x%08x\tobj->cmd=%02x\tversion=%02x\tflags=%02x\tmtu=%d\n",
             object,object->cmd,version,flags,hbo_mtu));

   if ( object->cmd == OBEX_CMD_CONNECT || (object->cmd & ~OBEX_CMD_FINAL) == OBEX_RSP_SUCCESS ) {
      if ( (object->meta.connectMeta = (ObxConnectMeta *)malloc( sizeof(ObxConnectMeta) )) ) {
         OBXDBGBUF(("iobxObjectSetConnectMeta() malloc, addr=0x%08x, len=%d.\n", object->meta.connectMeta, sizeof(ObxConnectMeta) ));
         object->meta.connectMeta->flags = flags;
         object->meta.connectMeta->version = version;
         object->meta.connectMeta->max_packet_length = htons( hbo_mtu );
      } else {
         OBXDBGERR(("[ERROR] iobxObjectSetConnectMeta() failure creating connectMeta structure.\n"));
         rc = OBX_RC_ERR_MEMORY;
      }
   } else {
      rc = OBX_RC_ERR_INAPPROPRIATE;
   }
   return rc;
}

ObxRc iobxObjectSetSetPathMeta( ObxObject *object, unsigned char flags, unsigned char constants ) {
   ObxRc    rc = OBX_RC_OK;
   OBXDBGFLOW(("iobxObjectSetSetPathMeta() entry, object=0x%08x\tflags=%02x\tconstants=%02x\n",object,flags,constants));
   if ( object->cmd == OBEX_CMD_SETPATH || (object->cmd & ~OBEX_CMD_FINAL) == OBEX_RSP_SUCCESS ) {
      if ( (object->meta.setPathMeta = (ObxSetPathMeta *)malloc( sizeof(ObxSetPathMeta) )) ) {
         OBXDBGBUF(("iobxObjectSetSetPathMeta() malloc, addr=0x%08x, len=%d.\n", object->meta.setPathMeta, sizeof(ObxSetPathMeta) ));
         object->meta.setPathMeta->constants = constants;
         object->meta.setPathMeta->flags = flags;
      } else {
         OBXDBGERR(("[ERROR] iobxObjectSetSetPathMeta() failure creating SetPath meta structure.\n"));
         rc = OBX_RC_ERR_MEMORY;
      }
   } else {
      rc = OBX_RC_ERR_INAPPROPRIATE;
   }
   return rc;
}

