#ifndef OBEXOBJECT_H
#define OBEXOBJECT_H
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

#include <handle.h>
#include <header.h>

#define OBJECT_FSM_CHECK_PRECONDITONS    1
#define OBJECT_FSM_DETERMINE_LENGTHS     2
#define OBJECT_FSM_CHECK_FOR_FINAL       3
#define OBJECT_FSM_SEND_OPCODE           4
#define OBJECT_FSM_SEND_LENGTH           5
#define OBJECT_FSM_SEND_META             6
#define OBJECT_FSM_SEND_NONBODY          7
#define OBJECT_FSM_SEND_BODY             8
#define OBJECT_FSM_FINISH                9

#ifdef __cplusplus
extern "C" {
#endif

/*
** The current object is sent using the transport associated with the passed ObxHandle.
** This method will send any leading information, then iterate through the headers sending
** each of them.
** Note, this function will send what it can, it's up to the caller to deal with partial
** sends (by re invoking).
*/
ObxRc       iobxObjectSendObject( ObxObject *object, ObxHandle *handle );

/*
** The passed object is populated by reading from the transport layer associated with the
** passed ObxHandle object.
*/
ObxRc       iobxObjectRecvObject( ObxObject *object, ObxHandle *handle );


/*
** Creates and initializes a new ObxObject structure.
*/
ObxObject   *iobxObjectNew();

/*
** Destroys the passed ObxObject structure.  All internal structures and
** the ObxObject structure itself is free()'ed.
*/
void        iobxObjectFree( ObxObject *object );

/*
** The passed ObxObject is cleared to an initial state.  Allows users to
** reuse objects.
*/
ObxRc       iobxObjectReset( ObxObject *object );

/*
** Adds a header to the list of headers associated with the object.
*/
ObxRc       iobxObjectAddHeader( ObxObject *object, ObxHeader *header, ObxHandle *handle );

/*
** What's the prefix size of this object
*/
int         iobxObjectPrefixSize( ObxObject *object, ObxHandle *handle );

/*
** Requests the current list of headers be returned.  From this object users
** can get an iterator and manipulate the header list.
*/
ObxList     *iobxObjectGetHeaderList( ObxObject *object );

ObxRc iobxObjectSetConnectMeta( ObxObject *object, unsigned char version, unsigned char flags, unsigned short hbo_mtu );
ObxRc iobxObjectSetSetPathMeta( ObxObject *object, unsigned char flags, unsigned char constants );

#ifdef __cplusplus
}
#endif

#endif
