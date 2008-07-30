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
** Obex Util Routines
**************************************************************************
*/
#include <utils.h>

#include <iConstants.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// luz %%%%
#ifdef __MWERKS__
#include <extras.h>
#define strnicmp _strnicmp
#endif

/* **************************************** */
/* ******* List Functions ***************** */
/* **************************************** */

/*
** Returns an empty list, ready for use.
*/
ObxList *iobxListNew() {
   ObxList *newList = NULL;
   OBXDBGFLOW(("iobxListNew() entry.\n"));
   if ( (newList = (ObxList *)malloc( sizeof( ObxList ) )) ) {
      OBXDBGBUF(("iobxListNew() malloc, addr=0x%08x, len=%d.\n", newList, sizeof(ObxList) ));
      newList->head = NULL;
      newList->tail = NULL;
      iobxListReset( newList );
   }
   return newList;
}

/*
** Free's the contents of a list and the list struct itself.
** WARNING: each element of the list is also free()'ed!
** Use ObxListRelease() to remove the elements without freeing them.
** The list should be considered invalid after this call.
*/
void iobxListFree( ObxList *list ) {
   OBXDBGFLOW(("iobxListFree() entry, list=0x%08x\n", list));
   if ( list ) {
      iobxListReset( list );
      free( list );
      list = NULL;
   }
}

/*
** Empty's the list, free()'ing all elements and resetting the list
** to it's initial state.
** The list is still viable and ready for use after this call.
** Use ObxListFree() to free the list itself.
*/
void iobxListReset( ObxList *list ) {
   ObxListNode *thisNode = NULL;
   ObxListNode *nextNode = NULL;
   OBXDBGFLOW(("iobxListReset() entry, list=0x%08x\n", list));
   if ( list ) {
      nextNode = list->head;
      while ( nextNode ) {
         thisNode = nextNode;
         nextNode->prev = NULL;
         nextNode = thisNode->next;
         if ( thisNode->data ) {
            free( thisNode->data );
            thisNode->data = NULL;
         }
         free( thisNode );
         thisNode = NULL;
      }
      list->head = NULL;
      list->tail = NULL;
   }
}

/*
** Removes all contents of a list.  They are not free'ed.  It's assumed
** that the caller is managing their storage.
** The list is still viable and ready for use after this call.
*/
void iobxListRelease( ObxList *list ) {
   ObxListNode *thisNode = NULL;
   ObxListNode *nextNode = NULL;
   OBXDBGFLOW(("iobxListRelease() entry, list=0x%08x\n", list));
   if ( list ) {
      nextNode = list->head;
      while ( nextNode ) {
         thisNode = nextNode;
         nextNode = thisNode->next;
         thisNode->data = NULL;  /* Note we're not freeing the data */
         free( thisNode );
         thisNode = NULL;
      }
      list->head = NULL;
      list->tail = NULL;
   }
}

/*
** Append a new data to the end of the list
** On success, returns pointer to the data that was added.
** Returns NULL on error.
*/
void *iobxListAppend( ObxList *list, void *data ) {
   ObxListNode *newNode = NULL;
   OBXDBGFLOW(("iobxListAppend() entry, list=0x%08x\tdata=0x%08x\n", list, data));
   if ( (newNode = (ObxListNode *)malloc( sizeof( ObxListNode ) )) ) {
      OBXDBGBUF(("iobxListAppend() malloc, addr=0x%08x, len=%d.\n", newNode, sizeof(ObxListNode) ));
      newNode->data = data;
      if ( list->tail ) {
         list->tail->next = newNode;
         newNode->prev = list->tail;
         newNode->next = NULL;
         list->tail = newNode;
      } else {
         list->head = list->tail = newNode;
         newNode->next = newNode->prev = NULL;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxListAppend() malloc failure\n"));
      return NULL; /* malloc failure */
   }
   return newNode->data;
}

/*
** Get an Iterator for the list.
** Returns the newly created iterator or NULL on error.
*/
ObxIterator *iobxListGetIterator( ObxList *list ) {
   ObxIterator *newIterator = NULL;
   OBXDBGFLOW(("iobxListGetIterator() entry, list=0x%08x\n", list));
   if ( list ) {
      if ( (newIterator = (ObxIterator *)malloc( sizeof( ObxIterator ) )) ) {
          memset(newIterator, 0, sizeof(ObxIterator));
         OBXDBGBUF(("iobxListGetIterator() malloc, addr=0x%08x, len=%d.\n", newIterator, sizeof(ObxIterator) ));
         newIterator->list = list;
         iobxIteratorReset( newIterator );
      }
   }
   return newIterator;
}

/*
** Insert a new element into the list 'after' the current position of the Iterator.
** Inserts to the front of the list are done with an iterator that has been
** 'reset' (ObxIteratorReset()).
** On success, returns pointer to the data that was added.
** Returns NULL on error.
*/
void *iobxListInsert( ObxList *list, const ObxIterator *iterator, void *data ) {
   ObxListNode *newNode = NULL;
   OBXDBGFLOW(("iobxListInsert() entry, list=0x%08x\titerator=0x%08x\tdata=0x%08x\n", list, iterator, data));
   if ( list && iterator ) {
      if ( (newNode = (ObxListNode *)malloc( sizeof( ObxListNode ) )) ) {
         OBXDBGBUF(("iobxListInsert() malloc, addr=0x%08x, len=%d.\n", newNode, sizeof(ObxListNode) ));
         newNode->next = NULL;
         newNode->prev = NULL;
         newNode->data = data;
         if ( iterator->cursor ) {
            /*
            ** Insert after current cursor, this could be the list tail.
            */
            if ( iterator->cursor->prev ) {
               iterator->cursor->prev->next = newNode;
            }
            if ( iterator->cursor->next ) {
               iterator->cursor->next->prev = newNode;
            }
            if ( iterator->cursor == list->tail ) {
               list->tail = newNode;
            }
         } else {
            /*
            ** Insert before head
            */
            if ( list->head ) {
               /* others existed */
               newNode->next = list->head;
               newNode->prev = NULL;
               list->head->prev = newNode;
               list->head = newNode;
            } else {
               /* first one */
               list->head = list->tail = newNode;
               newNode->next = newNode->prev = NULL;
            }
         }
      } else {
         OBXDBGERR(("[ERROR] iobxListInsert() malloc failure\n"));
         return NULL;   /* malloc failure */
      }
   } else {
      OBXDBGERR(("[ERROR] iobxListInsert() Bad plist on call\n"));
      return NULL;      /* bad plist */
   }
   return newNode->data;
}

/*
** Remove the element being pointed to by the iterator.  Return this data
** as a pointer to the caller.
**
** The iterator is adjusted as follows:
** 1) The iterator is backed up to the previous node.
** 2) If the node being removed was the first node, the iterator is reset.
**
** On success, returns pointer to the data that was removed.
** Returns NULL on error.
**
** Use ObxListDelete() to remove and free the data being managed by the
** list.
*/
void *iobxListRemove( ObxList *list, ObxIterator *iterator ) {
   void *data = NULL;
   ObxListNode *newCursor = NULL;

   OBXDBGFLOW(("iobxListRemove() entry, list=0x%08x\titerator=0x%08x\n", list, iterator));

   if ( list && iterator && list == iterator->list ) {
      if ( iterator->cursor ) {
         data = iterator->cursor->data;
         newCursor = iterator->cursor->prev;

         if ( list->head == iterator->cursor ) {
            list->head = iterator->cursor->next;
         }
         if ( list->tail == iterator->cursor ) {
            list->tail = iterator->cursor->prev;
         }
         if ( iterator->cursor->prev ) {
            iterator->cursor->prev->next = iterator->cursor->next;
         }
         if ( iterator->cursor->next ) {
            iterator->cursor->next->prev = iterator->cursor->prev;
         }
         iterator->cursor->data = NULL;   /* Note we don't free data, */
         free( iterator->cursor );        /* but, we do free the node */
         iterator->cursor = newCursor;
      }
   }
   return data;
}

/*
** Remove the element being pointed to by the iterator.  Free any data associated
** with the node.
**
** The iterator is adjusted as follows:
** 1) The iterator is backed up to the previous node.
** 2) If the node being removed was the first node, the iterator is reset.
**
** Use ObxListRemove() to remove the data and return it to the caller.
*/
void iobxListDelete( ObxList *list, ObxIterator *iterator ) {
   void *data = NULL;
   OBXDBGFLOW(("iobxListDelete() entry, list=0x%08x\titerator=0x%08x\n", list, iterator));
   if ( (data = iobxListRemove( list, iterator ) )) {
      free( data );
      data = NULL;
   }
}

/* **************************************** */
/* ******* Iterator Functions ************* */
/* **************************************** */

/*
** Reset the enumeration cursor to the beginning of the list.
** Returns the newly reset iterator or NULL on error.
*/
ObxIterator *iobxIteratorReset( ObxIterator *iterator ) {
   OBXDBGFLOW(("iobxIteratorReset() entry, iterator=0x%08x\n", iterator));
   if ( iterator ) {
      iterator->cursor = NULL;
   }
   return iterator;
}

/*
** Free storage associated with the Iterator.  Iterator should be considered
** invalid after this call.
*/
void iobxIteratorFree( ObxIterator *iterator ) {
   OBXDBGFLOW(("iobxIteratorFree() entry, iterator=0x%08x\n", iterator));
   if ( iterator ) {
      free( iterator );
      iterator = NULL;
   }
}

/*
** Requests the next element of data.  If at the end of the list, list is empty
** or the passed iterator is NULL, NULL is returned.
*/
void *iobxIteratorNext( ObxIterator *iterator ) {
   void *data = NULL;
   ObxListNode *tempListNode = NULL;
   OBXDBGFLOW(("iobxIteratorNext() entry, iterator=0x%08x\n", iterator));
   if ( iterator ) {
      if ( iterator->cursor ) {
         iterator->cursor = iterator->cursor->next;
         if ( iterator->cursor ) {
            data = iterator->cursor->data;
         } else {
            // at end of list, let it return NULL
         }
      } else {
         if ( iterator->list->head ) {
            tempListNode = iterator->list->head;
            data = tempListNode->data;
            iterator->cursor = tempListNode;
         } else {
            // nothing in list, let it return null.
         }
      }
   }
   return data;
}

/*
** Queries the existance of an element beyond the current element in
** the list.  Returns 0 if one does not exist, non-zero otherwise.
*/
short iobxIteratorHasNext( ObxIterator *iterator ) {
   short hasMore = 0;
   OBXDBGFLOW(("iobxIteratorHasNext() entry, iterator=0x%08x\n", iterator));
   if ( iterator->cursor ) {
      if ( iterator->cursor->next != NULL ) {
         hasMore = 1;
      } else {
         // list not empty, but at the end.
      }
   } else {
      if ( iterator->list->head != NULL ) {
         hasMore = 1;
      } else {
         // list was empty.
      }
   }
   return hasMore;
}

/* **************************************** */
/* **** Misc and Converstion Functions **** */
/* **************************************** */

/*
** Converts the passed unicode buffer to UTF8.
** The result is present in the ObxBuffer structure returned.
** NULL is returned on error.
*/
ObxBuffer   *iobxUnicodeToUTF8( const unsigned char *unicodeBuffer, int unicodeLength ) {
   ObxBuffer         *buffer = NULL;
   unsigned char     utfchars[3];
   int               utflen;
   unsigned char     *cursor = (unsigned char *)unicodeBuffer;
   int               length = unicodeLength;
   int               unichar;

   OBXDBGFLOW(("iobxUnicodeToUTF8() entry\n"));
   OBXDBGMEM(("iobxUnicodeToUTF8()", (void*)unicodeBuffer, unicodeLength));

   /*
   ** Probable length of required buffer is unicodeLength/2...
   ** but the buffer will grow if needed.
   */
   if ( (buffer = iobxBufNew( length / 2 )) ) {
      while ( length ) {
         /*
         ** Form a unicode char, advance cursor, reduce remaining length.
         */
         unichar = cursor[0] << 8 | cursor[1];
         cursor += 2;
         length -= 2;

         /*
         ** Figure out how many output bytes we need, and convert the Unicode
         ** character into that many bytes of UTF-8.  We assume our unicode
         ** doesn't contain any chars in the range D800 - DFFF, which are
         ** really ucs-4 chars mapped to ucs-2.
         */
         if ( unichar < 0x80 ) {
            utflen = 1;
            utfchars[0] = unichar;
         } else if ( unichar < 0x800 ) {
            utflen = 2;
            utfchars[0] = 0xC0 | (unichar >> 6);
            utfchars[1] = 0x80 | (unichar & 0x3F);
         } else {
            utflen = 3;
            utfchars[0] = 0xE0 | (unichar >> 12);
            utfchars[1] = 0x80 | ((unichar >> 6) & 0x3F);
            utfchars[2] = 0x80 | (unichar & 0x3F);
         }

         /*
         ** Toss in ObxBuffer
         */
         iobxBufWrite( buffer, utfchars, utflen );
      }
   }
   return buffer;
}

/*
** Converts the passed buffer, which points to a null terminated
** UTF8 string to Unicode.  The result is present in the ObxBuffer structure
** returned.  NULL is returned on error.
*/
ObxBuffer   *iobxUTF8ToUnicode( const unsigned char *utf8Buffer, int utf8Length ) {
   ObxBuffer         *buffer = NULL;
   int               length = utf8Length;
   unsigned char     unibytes[2];
   int               unichar;
   unsigned char     *cursor = (unsigned char *)utf8Buffer;

   OBXDBGFLOW(("iobxUTF8ToUnicode() entry\n"));
   OBXDBGMEM(("iobxUTF8ToUnicode()", (void*)utf8Buffer, utf8Length));
   if ( (buffer = iobxBufNew( length * 2 )) ) {
      while ( length ) {
         /*
         ** Clear ucode char
         */
         unichar = 0;

         /*
         ** What are we dealing with?
         */
         if ( !(cursor[0] & 0x80) ) {
            /* single byte */
            unichar = cursor[0];
            cursor += 1;
            length -= 1;
         } else if ( (cursor[0] & 0xE0) == 0xC0 ) {
            /* double byte */
            unichar = ((cursor[0] & 0x1F) << 6) | (cursor[1] & 0x3F);
            cursor += 2;
            length -= 2;
         } else {
            /* triple byte */
            unichar = ((cursor[0] & 0x0F) << 12) | ((cursor[1] & 0x3F) << 6) | (cursor[2] & 0x3F);
            cursor += 3;
            length -= 3;
         }

         /*
         ** Toss in ObxBuffer
         */
         unibytes[0] = (unichar >> 8) & 0xFF;
         unibytes[1] = unichar & 0xFF;
         iobxBufWrite( buffer, unibytes, sizeof(unibytes) );
      }
   }
   return buffer;
}

/*
** Parse meta info (stolen from the code base of jph)
*/
const char *iobxGetMetaInfoValue( const char *metaInformation, const char *tag, size_t *valueLen ) {
#define DELIMITERS   " \t"

   const char *p, *vs, *ve;
   int found = 0;

   OBXDBGFLOW(("iobxGetMetaInfoValue() entry, meta='%s'\ttag='%s'\n" ,metaInformation, tag ));

   p = metaInformation;
   do {
      const char *ts, *te;

      // Skip over delimiters (the strspn() function would be handy here)
      for (ts=p; *ts && strchr(DELIMITERS, *ts); ++ts) ;
      if (!*ts) break;           // We've hit the end of the string

      // The variable p now points to the start of a tag name.  Find the end.
      // The strpbrk() function would be handy here...
      for (te=ts; *te && !strchr(DELIMITERS "=", *te); ++te) ;

      // The variable end now points just beyond the last char of the tag name.
      // Find the beginning and end of the value (if any).
      if (*te != '=') {
         // If the keyword ends with anything but '=', there's no value.
         p = vs = ve = te;
      } else {
         vs = te + 1;
         // Tolerate a quoted (single or double) string as well for the value
         if (*vs == '"' || *vs == '\'') {
            int q = *vs;
            ve = strchr(++vs, q);
            if (ve) {
               p = ve + 1;
            } else {
               // Missing end quote is terminated by end of string
               ve = strchr(vs, '\0');
               p = ve;
            }
         } else {
            // No quotes.  Find next delimiter (end of word)
            for (ve=vs; *ve && !strchr(DELIMITERS, *ve); ++ve) ;
            p = ve;              // For next time through the loop
         }
      }

      // See if this is the tag we care about
#ifdef _WIN32
      // luz %%%: memicmp doesn't exist!
      //found = te-ts == strlen(tag)  &&  !memicmp(tag, ts, te-ts);
      found = te-ts == (int) strlen(tag)  &&  !strnicmp(tag, ts, te-ts);
#else
      found = te-ts == strlen(tag)  &&  !strncmp(tag, ts, te-ts); /* TO DO, write case insensitive version */
#endif
   } while (!found && *p);

   if (!found) return NULL;

   *valueLen = ve-vs;
   return vs;
}
#undef DELIMITERS

/*
** Load the inbound peerSocket structure (allocated by caller) with the required
** info using the provided name.
** Returns OBEX_RC_xxxx ( see obexError.h for return codes )
** On success, peerSocket structure is filled in.
*/
ObxRc iobxGetPeerAddr( const char *hostname, struct sockaddr_in *addr ) {
   ObxRc rc = OBX_RC_OK;
   struct hostent *host;

   OBXDBGFLOW(("iobxGetPeerAddr() entry, hostname='%s'\taddr=0x%08x\n", hostname, addr ));
   if ( hostname && addr ) {
      // First try translating it as a dotted-decimal IP address
      addr->sin_addr.s_addr = inet_addr( hostname );
      if ( addr->sin_addr.s_addr != INADDR_NONE ) {
         return OBX_RC_OK;
      } else {
         if ( !(host = gethostbyname( hostname )) ) {
            OBXDBGERR(("[ERROR] iobxGetPeerAddr() gethostbyname() fails.\n"));
            return OBX_RC_ERR_SOCKET;
         }
         addr->sin_addr = *(struct in_addr *) host->h_addr;
      }
   } else {
      OBXDBGERR(("[ERROR] iobxGetPeerAddr() Bad plist on call\n"));
      rc = OBX_RC_ERR_BADPLIST;
   }
   return rc;
}

/*
**
*/
#ifdef _WIN32
void _initializeSockets( void ) {
   WORD requestedVersion = MAKEWORD(1, 1);  /* Version 1.1 */
   WSADATA wsaData;               /* Buffer for startup data */
   /* If the DLL only supports higher levels than we do, give up */
   if ( WSAStartup(requestedVersion, &wsaData) ) {
      fprintf(stderr, "The available sockets DLL only supports higher"
         " levels of the WINSOCK API than those supported by us.\n");
      exit(1);
   }
   /* If the DLL doesn't support the version we requested, give up. */
   if ( wsaData.wVersion != requestedVersion ) {
      WSACleanup();          /* We're "done" using sockets */
      fprintf(stderr, "The available sockets DLL does not support the"
         " WINSOCK API version required by us.\n");
      exit(1);
   }
}
#endif

