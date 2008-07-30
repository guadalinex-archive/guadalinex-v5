#ifndef OBEXHEADER_H
#define OBEXHEADER_H


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

#ifdef __cplusplus
extern "C" {
#endif

/*
** There are two general ways to create a header:
**
** From a buffer containing data.
**
**    The primary use of this interface is by the transport layer.  Data inbound from
**    a peer obex is taken off the network and headers are created using this function
**    call.  The inbound data is assumed to be in network byte order with regard to lengths.
**
** Through seperate function calls.
**
**    This is the intended way for users to create header objects.
**
*/

/*
** Creates a new, empty header.
*/
ObxHeader *iobxHeaderNew( unsigned char opcode );

/*
** The following functions set the value associated with the
** header.  Attempts to use a function call with an inappropriate opcode or
** passing a null header pointer will result in a OBX_RC_ERROR_INAPPROPRIATE return code.
*/
ObxRc     iobxHeaderSetIntValue( ObxHeader *header, unsigned int value );
ObxRc     iobxHeaderSetByteValue( ObxHeader *header, unsigned char value );
ObxRc     iobxHeaderSetUnicodeValue( ObxHeader *header, ObxBuffer *value );
ObxRc     iobxHeaderSetByteSequenceValue( ObxHeader *header, ObxBuffer *value );

/*
** Given the bytes sitting in the passed buffer, create a header.
** The buffer is 'consumed' by doing this.  It's assumed that the inbound
** buffer contains atleast the amount of data required to form the header and
** is positioned to begin header creation.
*/
 ObxHeader   *iobxHeaderNewFromBuffer( ObxBuffer *buffer, ObxObject *object );

/*
** The passed header is free()'ed.  All contents of the header are also free()'ed.
*/
 void        iobxHeaderFree( ObxHeader *header );

/*
** Using the passed transport, this method sends data associated with this header.
** The 'bytes' argument only has meaning with the 'body' header and is ignored by
** others (they send all bytes).  The 'body' header may need to fragment iff bytes
** is less than the number of bytes actually contained within the header.
** This argument indicates the number of bytes it's safe to send.  Another call will
** be made indicating that more (possibly the remainder) of the bytes may be sent.
** Numerous invocations may be made (enough to exaust the header).
*/
ObxRc       iobxHeaderSend( ObxHeader *header, ObxHandle *handle, int bytes );

/*
** How large is this header right now?
** This is the size of the header if 'put on the wire' right now.  Including identifier,
** any lengths, data, etc.
*/
int         iobxHeaderSize( ObxHeader *header );

/*
** The following routines are called during the creation of the header using a buffer.
*/
ObxRc       iobxHeaderAddIntBuffer( ObxHeader *header, ObxBuffer *buf );
ObxRc       iobxHeaderAddByteBuffer( ObxHeader *header, ObxBuffer *buf );
ObxRc       iobxHeaderAddUnicodeBuffer( ObxHeader *header, ObxBuffer *buf );
ObxRc       iobxHeaderAddByteSequenceBuffer( ObxHeader *header, ObxBuffer *buf );

#ifdef __cplusplus
}
#endif

#endif
