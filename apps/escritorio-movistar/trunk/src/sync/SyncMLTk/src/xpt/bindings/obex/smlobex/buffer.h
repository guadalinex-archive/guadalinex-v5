#ifndef OBEXBUFFER_H
#define OBEXBUFFER_H
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
#include <obex/constants.h>
#include <utils.h>

/*
**************************************************************************
**
** Obex Buffer management
**
**************************************************************************
*/
struct iobxBuffer {
   struct iobxList      *buffers;

   struct iobxListNode  *writeNode;
   int                  writeOffset;

   struct iobxListNode  *readNode;
   int                  readOffset;
};


#ifdef __cplusplus
extern "C" {
#endif

/*
** Alloc a new buffer of specified length.
** Returns a newly allocated ObxBuffer or NULL on error.
*/
ObxBuffer *iobxBufNew( int len );

/*
** Free all internal contents and the 'buf' itself.
** Pointer 'buf' should not be used after calling this function.
*/
void iobxBufFree( ObxBuffer *buf );

/*
** Clear internal state, allows buffer to be reused.
** Returns a reset ObxBuffer or NULL on error.
*/
ObxBuffer *iobxBufReset( ObxBuffer *buf );

/*
** Appends bytes from passed 'data' to the end of the
** buffer.  (copies data from 'data' into buffer).
** Returns 0 on success, !0 on error.
*/
int iobxBufWrite( ObxBuffer *buf, void *data, int len );

/*
** Populates 'data' with 'len' bytes from the buffer.  Returns
** the actual number of bytes placed into 'data'.  This value can
** be less than 'len' if no additional bytes were available.
** Read consumes the data, Peek does not.
*/
int iobxBufRead( ObxBuffer *buf, void *data, int len );
int iobxBufPeek( ObxBuffer *buf, void *data, int len );

/*
** Amount of unread bytes in buffer.
*/
int iobxBufSize( ObxBuffer *buf );

#ifdef __cplusplus
}
#endif

#endif
