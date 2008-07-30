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

#include <header.h>

#include <buffer.h>
#include <handle.h>
#include <utils.h>
#include <object.h>

#include <iConstants.h>
#include <stdlib.h>

/*
** Create an empty header.
*/
ObxHeader   *iobxHeaderNew( unsigned char opcode ) {
   ObxHeader *header = NULL;
   OBXDBGFLOW(("iobxHeaderNew() entry, opcode=%02x\n", opcode));
   if ( (header = (ObxHeader *)malloc( sizeof( ObxHeader ) )) ) {
      OBXDBGBUF(("iobxHeaderNew() malloc, addr=0x%08x, len=%d.\n", header, sizeof(ObxHeader) ));
      header->identifier = opcode;
      header->state = STATE_BUILDING;
      header->value.byteSequenceValue = NULL;   // should clear'em all
   }
   return header;
}

/*
** The following functions set the value associated with the
** header.  Attempts to use a function call with an inappropriate opcode will result
** in a !OBX_RC_OK return code.
** Inbound argument for value is assuemd to be in host byte order.
*/
ObxRc       iobxHeaderSetIntValue( ObxHeader *header, unsigned int value ) {
   ObxRc rc = OBX_RC_OK;
   OBXDBGFLOW(("iobxHeaderSetIntValue() entry, header=0x%08x\tvalue=%d\n", header, value));
   if ( header && (header->identifier & OBEX_HEADER_ENCODING_MASK) == OBEX_HEADER_ENCODING_INT ) {
      header->value.fourBytevalue = value;
   } else {
      OBXDBGERR(("[ERROR] iobxHeaderSetIntValue() bad plist or inappropriate call for identifier encoding type.\n"));
      rc = OBX_RC_ERR_INAPPROPRIATE;
   }
   return rc;
}

ObxRc       iobxHeaderSetByteValue( ObxHeader *header, unsigned char value ) {
   ObxRc rc = OBX_RC_OK;
   OBXDBGFLOW(("iobxHeaderSetByteValue() entry, header=0x%08x\tvalue=%d\n", header, value));
   if ( header && (header->identifier & OBEX_HEADER_ENCODING_MASK) == OBEX_HEADER_ENCODING_BYTE ) {
      header->value.byteValue = value;
   } else {
      OBXDBGERR(("[ERROR] iobxHeaderSetByteValue() bad plist or inappropriate call for identifier encoding type.\n"));
      rc = OBX_RC_ERR_INAPPROPRIATE;
   }
   return rc;
}

ObxRc       iobxHeaderSetUnicodeValue( ObxHeader *header, ObxBuffer *value ) {
   ObxRc rc = OBX_RC_OK;
   OBXDBGFLOW(("iobxHeaderSetUnicodeValue() entry, header=0x%08x\tvalue=0x%08x\n", header, value));
   if ( header && (header->identifier & OBEX_HEADER_ENCODING_MASK) == OBEX_HEADER_ENCODING_UNICODE ) {
      header->value.unicodeValue = value;
   } else {
      OBXDBGERR(("[ERROR] iobxHeaderUnicodeValue() bad plist or inappropriate call for identifier encoding type.\n"));
      rc = OBX_RC_ERR_INAPPROPRIATE;
   }
   return rc;
}

ObxRc       iobxHeaderSetByteSequenceValue( ObxHeader *header, ObxBuffer *value ) {
   ObxRc rc = OBX_RC_OK;
   OBXDBGFLOW(("iobxHeaderSetByteSequenceValue() entry, header=0x%08x\tvalue=0x%08x\n", header, value));
   if ( header && (header->identifier & OBEX_HEADER_ENCODING_MASK) == OBEX_HEADER_ENCODING_BYTE_SEQ ) {
      header->value.byteSequenceValue = value;
   } else {
      OBXDBGERR(("[ERROR] iobxHeaderByteSequenceValue() bad plist or inappropriate call for identifier encoding type.\n"));
      rc = OBX_RC_ERR_INAPPROPRIATE;
   }
   return rc;
}

/*
** Given the bytes sitting in the passed buffer, create a header.
** The buffer is 'consumed' by doing this.  It's assumed that the inbound
** buffer contains atleast the amount of data required to form the header and
** is positioned to begin header creation.
*/
ObxHeader   *iobxHeaderNewFromBuffer( ObxBuffer *buffer, ObxObject *object ) {
   ObxRc       rc = OBX_RC_OK;
   ObxHeader   *header = NULL;
   OBXDBGFLOW(("iobxHeaderNewFromBuffer() entry, buffer=0x%08x\tobject=0x%08x\n", buffer,object));

   if ( (header = (ObxHeader *)malloc( sizeof( ObxHeader ) )) ) {
      OBXDBGBUF(("iobxHeaderNewFromBuffer() malloc, addr=0x%08x, len=%d.\n", header, sizeof(ObxHeader) ));
      iobxBufRead( buffer, &header->identifier, 1 );
      header->state = STATE_BUILDING;
      OBXDBGINFO(("iobxHeaderNewFromBuffer() Creating header, opcode: %02x.\n", header->identifier ));

      /*
      ** What type of header is it?
      */
      switch ( header->identifier & OBEX_HEADER_ENCODING_MASK ) {
      case OBEX_HEADER_ENCODING_INT:
         rc = iobxHeaderAddIntBuffer( header, buffer );
         break;

      case OBEX_HEADER_ENCODING_BYTE:
         rc = iobxHeaderAddByteBuffer( header, buffer );
         break;

      case OBEX_HEADER_ENCODING_BYTE_SEQ:
         rc = iobxHeaderAddByteSequenceBuffer( header, buffer );
         break;

      case OBEX_HEADER_ENCODING_UNICODE:
         rc = iobxHeaderAddUnicodeBuffer( header, buffer );
         break;

      default:
         OBXDBGERR(("[ERROR] iobxHeaderNewFromBuffer() unknown type of encoding.\n"));
         rc = OBX_RC_ERR_INAPPROPRIATE;
         break;
      }

      if ( rc != OBX_RC_OK ) {
         free( header );
         header = NULL;
      }
   }
   return header;
}

/*
** The passed header is free()'ed.  All contents of the header are
** also free()'ed.
*/
void        iobxHeaderFree( ObxHeader *header ) {
   OBXDBGFLOW(("iobxHeaderFree() entry, header=0x%08x\n", header));
   if ( header ) {
      switch (header->identifier & OBEX_HEADER_ENCODING_MASK) {
      case OBEX_HEADER_ENCODING_BYTE_SEQ:
         if ( header->value.byteSequenceValue ) {
            iobxBufFree( header->value.byteSequenceValue );
         }
         break;

      case OBEX_HEADER_ENCODING_UNICODE:
         if ( header->value.unicodeValue ) {
            iobxBufFree( header->value.unicodeValue );
         }
         break;

      default:
         break;
      }
      free( header );
   }
}

/*
** Using the passed transport, this method sends data associated with this header.
** The 'bytes' argument only has meaning with the 'body' header and is ignored by
** others (they send all bytes).  The 'body' header may need to fragment iff bytes
** is less than the number of bytes actually contained within the header.
** This argument indicates the number of bytes it's safe to send.  Another call will
** be made indicating that more (possibly the remainder) of the bytes may be sent.
** Numerous invocations may be made (enough to exaust the header).
*/
ObxRc       iobxHeaderSend( ObxHeader *header, ObxHandle *handle, int bytes ) {
   ObxRc          rc = OBX_RC_OK;
   int            intData;
   short          shortData;
   int            wrote;
   int            dataSize;
   int            sendLength;
   void           *bytebuffer;

   OBXDBGFLOW(("iobxHeaderSend() entry, header=0x%08x\thandle=0x%08x\tbytes=%d\n", header, handle, bytes));
   if ( handle->transport ) {
      if ( header->state != STATE_SENT && header->state != STATE_ERROR ) {
         switch ( header->identifier & OBEX_HEADER_ENCODING_MASK ) {

         /*
         ** 4 byte quantity - transmited in nbo
         */
         case OBEX_HEADER_ENCODING_INT:
            OBXDBGINFO(("iobxHeaderSend() 'INT' encoding detected.\n"));
            /*
            ** Identifier
            */
            if ( (rc=handle->transport->send( &handle->connectionId, &header->identifier, 1, &wrote, FALSE )) == OBX_RC_OK ) {
               OBXDBGMEM(("iobxHeaderSend()", &header->value.fourBytevalue, 4));
               intData = htonl( header->value.fourBytevalue );
               if ( (rc=handle->transport->send( &handle->connectionId, &intData,
                                                 sizeof(intData), &wrote, FALSE )) == OBX_RC_OK ) {
                  header->state = STATE_SENT;
               } else {
                  OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending data.\n"));
               }
            } else {
               OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending identifier.\n"));
            }
            break;

         /*
         ** 1 byte quantity
         */
         case OBEX_HEADER_ENCODING_BYTE:
            OBXDBGINFO(("iobxHeaderSend() 'BYTE' encoding detected.\n"));
            /*
            ** Identifier
            */
            if ( (rc=handle->transport->send( &handle->connectionId, &header->identifier, 1, &wrote, FALSE )) == OBX_RC_OK ) {
               /*
               ** Data
               */
               OBXDBGMEM(("iobxHeaderSend()", &header->value.byteValue, 1));
               if ( (rc=handle->transport->send( &handle->connectionId, &header->value.byteValue, 1, &wrote, FALSE )) == OBX_RC_OK ) {
                  header->state = STATE_SENT;
               } else {
                  OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending data.\n"));
               }
            } else {
               OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending identifier.\n"));
            }
            break;

         /*
         ** byte sequence, length prefixed with 2 byte unsigned integer,
         ** transmit length in nbo
         */
         case OBEX_HEADER_ENCODING_BYTE_SEQ:
            OBXDBGINFO(("iobxHeaderSend() 'BYTE SEQUENCE' encoding detected.\n"));
            /*
            ** Body header?, watch for fragmentation
            */
            dataSize = iobxBufSize( header->value.byteSequenceValue );
            if ( header->identifier == OBEX_HEADER_BODY || header->identifier == OBEX_HEADER_BODY_END ) {
               /*
               ** Can we handle all the data? (plus the opcode + len (i.e. 3) )
               */
               if ( bytes >= dataSize + 3 ) {
                  /* yep, it all fits. */
                  sendLength = dataSize;
                  /*header->identifier = OBEX_HEADER_BODY_END; */
                  header->state = STATE_SENT;
               } else {
                  sendLength = bytes - 3;
                  /* Nope, fragmenting */
                  /*header->identifier = OBEX_HEADER_BODY;*/
                  header->state = STATE_SENDING;
               }
            } else {
               header->state = STATE_SENT;
               sendLength = dataSize;
            }

            /*
            ** Identifier
            */
            if ( (rc=handle->transport->send( &handle->connectionId,
                                              &header->identifier,
                                              1,
                                              &wrote,
                                              FALSE )) == OBX_RC_OK ) {
               /*
               ** Length (must include opcode and length fields)
               */
               shortData = htons( (short)(sendLength+3) );
               if ( (rc=handle->transport->send( &handle->connectionId,
                                                 &shortData,
                                                 sizeof(shortData),
                                                 &wrote,
                                                 FALSE )) == OBX_RC_OK ) {
                  /*
                  ** Data, if any.
                  */
                  if ( sendLength > 0 ) {
                     if ( (bytebuffer = (void *)malloc(sendLength)) ) {
                        OBXDBGBUF(("iobxHeaderSend() malloc, addr=0x%08x, len=%d.\n", bytebuffer, sendLength ));
                        iobxBufRead( header->value.byteSequenceValue, bytebuffer, sendLength );
                        OBXDBGMEM(("iobxHeaderSend()", bytebuffer, sendLength));
                        if ( (rc=handle->transport->send( &handle->connectionId,
                                                          bytebuffer,
                                                          sendLength,
                                                          &wrote,
                                                          FALSE )) == OBX_RC_OK ) {
                        } else {
                           OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending data.\n"));
                        }
                        free( bytebuffer );
                     } else {
                        OBXDBGERR(("[ERROR] iobxHeaderSend() error getting byte buffer, len=%d.\n", sendLength));
                        rc = OBX_RC_ERR_MEMORY;
                     }
                  }
               } else {
                  OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending length.\n"));
               }
            } else {
               OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending identifier.\n"));
            }
            break;

         /*
         ** Null terminated unicode text sequence, length prefixed with 2 byte unsigned integer,
         ** transmit length in nbo
         */
         case OBEX_HEADER_ENCODING_UNICODE:
            OBXDBGINFO(("iobxHeaderSend() 'UNICODE' encoding detected.\n"));
            /*
            ** Identifier
            */
            if ( (rc=handle->transport->send( &handle->connectionId,
                                              &header->identifier,
                                              1,
                                              &wrote,
                                              FALSE )) == OBX_RC_OK ) {
               /*
               ** Length
               */
               dataSize = iobxBufSize( header->value.unicodeValue );
               shortData = htons( (short)(dataSize+3) );
               if ( (rc=handle->transport->send( &handle->connectionId,
                                                 &shortData,
                                                 sizeof(shortData),
                                                 &wrote,
                                                 FALSE )) == OBX_RC_OK ) {
                  /*
                  ** Data
                  */
                  if ( dataSize > 0 ) {
                     if ( (bytebuffer = (void *)malloc( dataSize )) ) {
                        OBXDBGBUF(("iobxHeaderSend() malloc, addr=0x%08x, len=%d.\n", bytebuffer, dataSize ));
                        iobxBufRead( header->value.unicodeValue, bytebuffer, dataSize );
                        OBXDBGMEM(("iobxHeaderSend()", bytebuffer, dataSize));
                        if ( (rc=handle->transport->send( &handle->connectionId,
                                                          bytebuffer,
                                                          dataSize,
                                                          &wrote,
                                                          FALSE )) == OBX_RC_OK ) {

                           header->state = STATE_SENT;
                        } else {
                           OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending data.\n"));
                        }
                        free( bytebuffer );
                     } else {
                        OBXDBGERR(("[ERROR] iobxHeaderSend() error getting byte buffer, len=%d.\n", dataSize));
                        rc = OBX_RC_ERR_MEMORY;
                     }
                  }
               } else {
                  OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending length.\n"));
               }
            } else {
               OBXDBGERR(("[ERROR] iobxHeaderSend() unexpected return code sending identifier.\n"));
            }
            break;

         default:
            OBXDBGERR(("[ERROR] iobxHeaderSend() unknown encoding type.\n"));
            rc = OBX_RC_ERR_BADID;
            break;
         }
      } else {
         OBXDBGINFO(("[WARNING] iobxHeaderSend() called to send, already sent, ignored.\n"));
      }
   } else {
      OBXDBGERR(("[ERROR] iobxHeaderSend() no transport defined within handle.\n"));
      rc = OBX_RC_ERR_NOTINITIALIZED;     /* No transport */
   }
   if ( rc != OBX_RC_OK ) {
      header->state = STATE_ERROR;
   }
   return rc;
}

/*
** How large is this header right now?
** This is the size of the header if 'put on the wire' right now.  Including identifier,
** any lengths, data, etc.  If it's already 'sent' or in error, it returns zero.
*/
int         iobxHeaderSize( ObxHeader *header ) {
   int  size = 0;
   OBXDBGFLOW(("iobxHeaderSize() entry, header=0x%08x\n", header));
   if ( header->state != STATE_SENT && header->state != STATE_ERROR ) {
      size = 1;   /* for the identifier */
      switch ( header->identifier & OBEX_HEADER_ENCODING_MASK ) {
      case OBEX_HEADER_ENCODING_INT:
         size += 4;
         break;
      case OBEX_HEADER_ENCODING_BYTE:
         size += 1;
         break;
      case OBEX_HEADER_ENCODING_BYTE_SEQ:
         /* length field + data len */
         size += 2 + iobxBufSize( header->value.byteSequenceValue );
         break;
      case OBEX_HEADER_ENCODING_UNICODE:
         /* length field + data len */
         size += 2 + iobxBufSize( header->value.unicodeValue );
         break;
      }
   }
   return size;
}

/*
** The following routines are called by ObxHeaderNewHeader() once the header type has
** been determined.  The continue the population of the header (passed in) using the data
** contained in the buffer (passed in 'buf').
*/
ObxRc       iobxHeaderAddIntBuffer( ObxHeader *header, ObxBuffer *buf ) {
   int   theInt;

   OBXDBGFLOW(("iobxHeaderAddIntBuffer() entry, header=0x%08x\tbuf=0x%08x\n", header, buf));
   iobxBufRead( buf, &theInt, 4 );
   header->value.fourBytevalue = (int)ntohl(theInt);
   return OBX_RC_OK;
}

ObxRc       iobxHeaderAddByteBuffer( ObxHeader *header, ObxBuffer *buf ) {
   OBXDBGFLOW(("iobxHeaderAddByteBuffer() entry, header=0x%08x\tbuf=0x%08x\n", header, buf));
   iobxBufRead( buf, &header->value.byteValue, 1 );
   return OBX_RC_OK;
}

ObxRc       iobxHeaderAddUnicodeBuffer( ObxHeader *header, ObxBuffer *buf ) {
   ObxRc          rc = OBX_RC_OK;
   short          nboLength;
   short          hboLength = 0;
   ObxBuffer      *buffer = NULL;
   void           *bytebuffer;

   OBXDBGFLOW(("iobxHeaderAddUnicodeBuffer() entry, header=0x%08x\tbuf=0x%08x\n", header, buf));

   /* Glean length */
   if ( iobxBufRead( buf, &nboLength, 2 ) != 2 ) {
      OBXDBGERR(("[ERROR] iobxHeaderAddUnicodeBuffer() not enough bytes in header?\n"));
      return OBX_RC_ERR_MEMORY;
   }
   hboLength = ntohs( nboLength ) - 3;   /* len included opcode and length field */

   if ( (buffer = iobxBufNew( hboLength )) ) {
      if ( (bytebuffer=(void *)malloc( hboLength )) ) {
         OBXDBGBUF(("iobxHeaderAddUnicodeBuffer() malloc, addr=0x%08x, len=%d.\n", bytebuffer, hboLength ));
         if ( iobxBufRead( buf, bytebuffer, hboLength ) == hboLength ) {
            iobxBufWrite( buffer, bytebuffer, hboLength );
            header->value.unicodeValue = buffer;
         } else {
            OBXDBGERR(("[ERROR] iobxHeaderAddUnicodeBuffer() not enough bytes in header?\n"));
            rc = OBX_RC_ERR_MEMORY;
         }
         free( bytebuffer );
      } else {
         OBXDBGERR(("[ERROR] iobxHeaderAddUnicodeBuffer() error obtaining bytebuffer.\n"));
         rc = OBX_RC_ERR_MEMORY;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxHeaderAddUnicodeBuffer() error obtaining obex buffer.\n"));
      rc = OBX_RC_ERR_MEMORY;
   }
   return rc;
}

ObxRc       iobxHeaderAddByteSequenceBuffer( ObxHeader *header, ObxBuffer *buf ) {
   ObxRc          rc = OBX_RC_OK;
   short          nboLength;
   short          hboLength = 0;
   ObxBuffer      *buffer = NULL;
   void           *bytebuffer;

   OBXDBGFLOW(("iobxHeaderAddByteSequenceBuffer() entry, header=0x%08x\tbuf=0x%08x\n", header, buf));

   /* Glean length */
   if ( iobxBufRead( buf, &nboLength, 2 ) != 2 ) {
      OBXDBGERR(("[ERROR] iobxHeaderAddByteSequenceBuffer() not enough bytes in header?\n"));
      return OBX_RC_ERR_MEMORY;
   }
   hboLength = ntohs( nboLength ) - 3;   /* len included opcode and length field */

   if ( (buffer = iobxBufNew( hboLength )) ) {
      if ( (bytebuffer=(void *)malloc( hboLength )) ) {
         OBXDBGBUF(("iobxHeaderAddByteSequenceBuffer() malloc, addr=0x%08x, len=%d.\n", bytebuffer, hboLength ));
         if ( iobxBufRead( buf, bytebuffer, hboLength ) == hboLength ) {
            iobxBufWrite( buffer, bytebuffer, hboLength );
            header->value.unicodeValue = buffer;
         } else {
            OBXDBGERR(("[ERROR] iobxHeaderAddByteSequenceBuffer() not enough bytes in header?\n"));
            rc = OBX_RC_ERR_MEMORY;
         }
         free( bytebuffer );
      } else {
         OBXDBGERR(("[ERROR] iobxHeaderAddByteSequenceBuffer() error obtaining bytebuffer.\n"));
         rc = OBX_RC_ERR_MEMORY;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxHeaderAddByteSequenceBuffer() error obtaining obex buffer.\n"));
      rc = OBX_RC_ERR_MEMORY;
   }
   return rc;
}
