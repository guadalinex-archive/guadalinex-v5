#ifndef OBEXUTILS_H
#define OBEXUTILS_H

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

#include <iConstants.h>
#include <buffer.h>

#ifndef __PALM_OS__
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
**************************************************************************
** Obex Util Routines
**************************************************************************
*/

/*
** Obex List Node: A private node within a managed list
*/
struct iobxListNode {
   ObxListNode *next;
   ObxListNode *prev;
   void        *data;
};

/*
** Obex List: Manages an arbitrary list of buffers.
*/
struct iobxList {
   ObxListNode    *head;
   ObxListNode    *tail;
};

/*
** Obex Iterator: A cursor running through a list.
*/
struct iobxIterator {
   ObxList     *list;
   ObxListNode *cursor;
};


/* **************************************** */
/* ******* List Functions ***************** */
/* **************************************** */

/*
** Returns an empty list, ready for use.
*/
ObxList *iobxListNew();

/*
** Free's the contents of a list and the list struct itself.
** WARNING: each element of the list is also free()'ed!
** Use ObxListRelease() to remove the elements without freeing them.
** The list should be considered invalid after this call.
*/
void iobxListFree( ObxList *list );

/*
** Empty's the list, free()'ing all elements and resetting the list
** to it's initial state.
** The list is still viable and ready for use after this call.
** Use ObxListFree() to free the list itself.
*/
void iobxListReset( ObxList *list );

/*
** Removes all contents of a list.  They are not free'ed.  It's assumed
** that the caller is managing their storage.
** The list is still viable and ready for use after this call.
*/
void iobxListRelease( ObxList *list );

/*
** Append a new data to the end of the list
** On success, returns void * pointing to the data that was added.
** Returns NULL on error.
*/
void *iobxListAppend( ObxList *list, void *data );

/*
** Get an Iterator for the list.
** Returns the newly created iterator or NULL on error.
*/
ObxIterator *iobxListGetIterator( ObxList *list );

/*
** Insert a new element into the list 'after' the current position of
** the Iterator.
** Inserts to the front of the list are done with an iterator that has been
** 'reset' (ObxIteratorReset()).
** On success, returns pointer to the data that was added.
** Returns NULL on error.
*/
void *iobxListInsert( ObxList *list, const ObxIterator *iterator, void *data );

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
void *iobxListRemove( ObxList *list, ObxIterator *iterator );

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
void iobxListDelete( ObxList *list, ObxIterator *iterator );

/* **************************************** */
/* ******* Iterator Functions ************* */
/* **************************************** */

/*
** Reset the enumeration cursor to the beginning of the list.
** Returns the newly reset iterator or NULL on error.
*/
ObxIterator *iobxIteratorReset( ObxIterator *iterator );

/*
** Free storage associated with the Iterator.  Iterator should be considered
** invalid after this call.
*/
void iobxIteratorFree( ObxIterator *iterator );

/*
** Requests the next element of data.  If at the end of the list, list is empty
** or the passed iterator is NULL, NULL is returned.
*/
void *iobxIteratorNext( ObxIterator *iterator );

/*
** Queries the existance of an element beyond the current element in
** the list.  Returns 0 if one does not exist, non-zero otherwise.
*/
short iobxIteratorHasNext( ObxIterator *iterator );

/* **************************************** */
/* **** Misc and Converstion Functions **** */
/* **************************************** */

/*
** Converts the passed Unicode to UTF8.
** The result is present in the ObxBuffer structure returned.
** NULL is returned on error.
*/
ObxBuffer   *iobxUnicodeToUTF8( const unsigned char *unicodeBuffer, int unicodeLength );

/*
** Converts the passed buffer, which points to a buffer containing
** UTF8, to Unicode.  The length represents the length of the passed buffer.
** The result is present in the ObxBuffer structure returned.  NULL is returned on error.
*/
ObxBuffer   *iobxUTF8ToUnicode( const unsigned char *utf8, int utf8Length );

/*
** Parse a tag out of meta data.
*/
const char *iobxGetMetaInfoValue( const char *metaInformation, const char *tag, size_t *valueLen );

/*
** Load the inbound peerSocket structure (allocated by caller) with the required
** info using the provided name.
** On success, peerSocket structure is filled in and returns OBX_RC_OK.
*/
ObxRc iobxGetPeerAddr( const char *hostname, struct sockaddr_in *addr );

/*
** Initialize sockets
*/
#ifdef _WIN32
void _initializeSockets( void );
#endif


#ifdef __cplusplus
}
#endif

#endif
