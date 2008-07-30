/*
** Obx Buffer management routines
*/

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

/* Note, debugging output only occurs in this module if DEBUGALL is used. */
#include <buffer.h>
#include <iConstants.h>
#include <stdlib.h>
#include <stdio.h>

#define  BUFFER_SIZE 1024

/*
** Private, common read.
*/
int iobxBufCommonRead( ObxBuffer *buf, void *data, int len, int peek );

/*
** Alloc a new buffer of specified length.
** Returns a newly allocated ObxBuffer or NULL on error.
** NOTE: len is ignored for now
*/
ObxBuffer   *iobxBufNew( int len ) {
	ObxBuffer      *buf;
#ifdef DEBUGALL
   OBXDBGFLOW(("iobxBufNew() entry, len=%d\n", len));
#endif
	
	if ( !(buf = (ObxBuffer *)malloc( sizeof( ObxBuffer ) )) ) {
      return NULL;
   }

#ifdef DEBUGALL
   OBXDBGBUF(("iobxBufNew() malloc, addr=0x%08x, len=%d.\n", buf, sizeof(ObxBuffer) ));
#endif
   buf->buffers = NULL;
	return iobxBufReset( buf );
}

/*
** Free all internal contents and the 'buf' itself.
** Pointer should not be used after calling.
*/
void        iobxBufFree( ObxBuffer *buf ) {
#ifdef DEBUGALL
   OBXDBGFLOW(("iobxBufFree() entry, buf=0x%08x\n", buf));
#endif
	if ( buf ) {
      iobxListFree( buf->buffers );
      free( buf );
   }
}

/*
** Clear internal state, allows buffer to be reused.
** Returns the reset ObxBuffer or NULL on error.
*/
ObxBuffer      *iobxBufReset( ObxBuffer *buf ) {
	void *managedbuf;
#ifdef DEBUGALL
   OBXDBGFLOW(("iobxBufReset() entry, buf=0x%08x\n", buf));
#endif
   if ( !buf ) {
      return NULL;
   }
   if ( buf ) {
      if ( buf->buffers ) {
         iobxListReset( buf->buffers );
      } else {
         if ( !(buf->buffers = iobxListNew()) ) {
            return NULL;
         }
      }
      if ( ( managedbuf = (void *)malloc( BUFFER_SIZE )) ) {
#ifdef DEBUGALL
         OBXDBGBUF(("iobxBufReset() malloc, addr=0x%08x, len=%d.\n", managedbuf, BUFFER_SIZE ));
#endif
         if ( (iobxListAppend( buf->buffers, managedbuf )) ) {
            buf->writeOffset = buf->readOffset = 0;
            buf->writeNode = buf->readNode = buf->buffers->head;
         } else {
            free( managedbuf );
            iobxListFree( buf->buffers );
            return NULL;
         }
      } else {
         iobxListFree( buf->buffers );
         return NULL;
      }
   }
	return buf;
}

/*
** Appends bytes from passed 'data' to the end of the
** buffer.  (copies data from 'data' into buffer).
** Returns 0 on success, !0 on error.
*/
int iobxBufWrite( ObxBuffer *buf, void *data, int len ) {
   int   length;
   int   thisCopy = 0;
   void  *source = data;
	void  *managedbuf;

#ifdef DEBUGALL
   OBXDBGFLOW(("iobxBufWrite() entry, buf=0x%08x\tlen=%d\n", buf, len));
   OBXDBGMEM(("iobxBufWrite()", data, len));
#endif

   if ( !buf ) {
      return -1;
   }
   length = len;
   while ( length > 0 ) {
      thisCopy = min( BUFFER_SIZE - buf->writeOffset, length );
      memcpy( (char *)buf->writeNode->data+buf->writeOffset, source, thisCopy );

      buf->writeOffset += thisCopy;
      
      // LEO:
      source = (void *)((char *)source+thisCopy);
      // (char *)source += thisCopy;
      
      
      
      length -= thisCopy;

      if ( length > 0 ) {
         // Need more space
         if ( ( managedbuf = (void *)malloc( BUFFER_SIZE )) ) {
#ifdef DEBUGALL
            OBXDBGBUF(("iobxBufWrite() malloc, addr=0x%08x, len=%d.\n", managedbuf, BUFFER_SIZE ));
#endif
            if ( (iobxListAppend( buf->buffers, managedbuf )) ) {
               buf->writeNode = buf->buffers->tail;
               buf->writeOffset = 0;
            } else {
               free( managedbuf );
               iobxListFree( buf->buffers );
               return -1;
            }
         } else {
            iobxListFree( buf->buffers );
            return -1;
         }
      }
   }
	return 0;
}

/*
**
*/
int iobxBufRead( ObxBuffer *buf, void *data, int length ) {
#ifdef DEBUGALL
   OBXDBGFLOW(("iobxBufRead() entry, buf=0x%08x\tlen=%d\n", buf, length));
#endif
   return iobxBufCommonRead( buf, data, length, 0 );
}

/*
**
*/
int iobxBufPeek( ObxBuffer *buf, void *data, int length ) {
#ifdef DEBUGALL
   OBXDBGFLOW(("iobxBufPeek() entry, buf=0x%08x\tlen=%d\n", buf, length));
#endif
   return iobxBufCommonRead( buf, data, length, 1 );
}

/*
** Populates 'data' with 'len' bytes from the buffer.  Returns
** the actual number of bytes placed into 'data'.  This value can
** be less than 'len' if no additional bytes were available.
** Handles, peek or no peek.
*/
int iobxBufCommonRead( ObxBuffer *buf, void *data, int length, int peek ) {
   int                  read = 0;
   int                  empty = 0;
   int                  thisCopy = 0;
   void                 *target = data;
   struct iobxListNode  *nodePntr = NULL;

   int                  origReadOffset = buf->readOffset;
   struct iobxListNode  *origReadNode = buf->readNode;

#ifdef DEBUGALL
   OBXDBGFLOW(("iobxBufCommonRead() entry, buf=0x%08x\tdata=0x%08x\tlen=%d\n", buf, data, length));
#endif

   if ( !buf ) {
      return 0;
   }
   while ( !empty && read < length ) {
      thisCopy = min( BUFFER_SIZE - buf->readOffset, length-read );

      memcpy( target, (char *)buf->readNode->data+buf->readOffset, thisCopy );

      
      
      // LEO:
      target = (void *)((char *)target+thisCopy);
      // (char *)target += thisCopy;
      
      
      
      buf->readOffset += thisCopy;
      read += thisCopy;

      /*
      ** Done?
      */
      if ( read < length  ) {
         /*
         ** Need to advance to next buffer, or we're 'emtpy'.
         ** Messing with the list directly isn't quite kosher.. but it's quicker.
         */
         if ( buf->readNode->next ) {
            /*
            ** There is another buffer, thus, more data.
            */
            nodePntr = buf->readNode;
            buf->readNode = buf->readNode->next;

            if ( !peek ) {
               /*
               ** Free 'read data' buffer
               */
               if ( nodePntr->data ) {
                  free( nodePntr->data );
               }
               free( nodePntr );
            }

            buf->readNode->prev = NULL;
            buf->buffers->head = buf->readNode;

            buf->readOffset = 0;
         } else {
            empty = 1;
         }
      }
   }

   if ( peek ) {
      buf->readOffset = origReadOffset;
      buf->readNode = origReadNode;
   }

   return read;
}


/*
** Amount of unread bytes in buffer.
*/
int iobxBufSize( ObxBuffer *buf ) {
   int size = 0;
   struct iobxListNode  *nodePntr;

#ifdef DEBUGALL
   OBXDBGFLOW(("iobxBufSize() entry, buf=0x%08x\n", buf));
#endif

   if ( !buf ) {
      return 0;
   }
   nodePntr = buf->readNode;
   while ( nodePntr ) {
      if ( nodePntr == buf->readNode ) {
         if ( nodePntr == buf->writeNode ) {
            size += (buf->writeOffset - buf->readOffset);
         } else {
            size += (BUFFER_SIZE - buf->readOffset);
         }
      } else {
         if ( nodePntr == buf->writeNode ) {
            size += buf->writeOffset;
         } else {
            size += BUFFER_SIZE;
         }
      }
      nodePntr = nodePntr->next;
   }
   return size;
}


