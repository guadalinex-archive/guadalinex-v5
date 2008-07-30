#ifndef OBEXERROR_H
#define OBEXERROR_H


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

#define OBX_RC_OK                            0

#define OBX_RC_ERR                           0x1000
#define OBX_RC_ERR_UNSPECIFIED               OBX_RC_ERR + 0x01
#define OBX_RC_ERR_TRANSPORT                 OBX_RC_ERR + 0x02
#define OBX_RC_ERR_BADPLIST                  OBX_RC_ERR + 0x03
#define OBX_RC_ERR_MEMORY                    OBX_RC_ERR + 0x04
#define OBX_RC_ERR_NOTINITIALIZED            OBX_RC_ERR + 0x05
#define OBX_RC_ERR_ALREADYINITIALIZED        OBX_RC_ERR + 0x06
#define OBX_RC_ERR_INAPPROPRIATE             OBX_RC_ERR + 0x07
#define OBX_RC_ERR_BADID                     OBX_RC_ERR + 0x08
#define OBX_RC_ERR_BADMETADATA               OBX_RC_ERR + 0x09

#define OBX_RC_ERR_SOCKET                    OBX_RC_ERR + 0x10
#define OBX_RC_ERR_SOCKET_LISTEN             OBX_RC_ERR + 0x11
#define OBX_RC_ERR_SOCKET_BIND               OBX_RC_ERR + 0x12
#define OBX_RC_ERR_SOCKET_CREATE             OBX_RC_ERR + 0x13
#define OBX_RC_ERR_SOCKET_CONNECT            OBX_RC_ERR + 0x14
#define OBX_RC_ERR_SOCKET_ALREADY_LISTENING  OBX_RC_ERR + 0x15
#define OBX_RC_ERR_SOCKET_NOT_LISTENING      OBX_RC_ERR + 0x16
#define OBX_RC_ERR_SOCKET_ACCEPT             OBX_RC_ERR + 0x17
#define OBX_RC_ERR_SOCKET_EOF                OBX_RC_ERR + 0x18

#define OBX_RC_STREAM_EOF                    OBX_RC_ERR + 0x19
#define OBX_RC_ERR_NO_GET_SUPPORT            OBX_RC_ERR + 0x20

#define OBX_RC_ERR_OBJ_HDR_NO_FIT            OBX_RC_ERR + 0x21
#define OBX_RC_ERR_OBJ_FRAGMENTED_SEND       OBX_RC_ERR + 0x22
#define OBX_RC_ERR_OBJ_BAD_HEADER            OBX_RC_ERR + 0x23

// Mickey 2003-01-29: These define errors that are caused because of no conforming to SyncML OBEX Binding Spec.
#define OBX_RC_ERR_SML_CONNECTIONID_HDR      OBX_RC_ERR + 0x24
#define OBX_RC_ERR_SML_WHO_HDR               OBX_RC_ERR + 0x25
#define OBX_RC_ERR_SML_TARGET_HDR               OBX_RC_ERR + 0x26
#define OBX_RC_ERR_SML_BODY_HDR              OBX_RC_ERR + 0x27
#define OBX_RC_ERR_SML_TYPE_HDR              OBX_RC_ERR + 0x28

//Suddenly get Abort command from OBEX client.
#define OBX_RC_ERR_GET_ABORT              OBX_RC_ERR + 0x29

#endif
