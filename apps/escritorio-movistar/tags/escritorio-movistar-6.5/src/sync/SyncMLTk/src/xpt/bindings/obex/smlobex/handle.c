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

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#include <handle.h>
#include <stdlib.h>
#include <iConstants.h>
#include <transport.h>

/*
**************************************************************************
**
** Obex Handle
**
** ObxHandles are general handle structures for obex.  They are returned to
** callers when they request a 'handle'.
**
**************************************************************************
*/

/*
** Create a new handle.
*/
ObxHandle   *iobxHandleNew() {
   ObxHandle *newHandle = NULL;
   OBXDBGFLOW(("iobxHandleNew() entry.\n"));
   if ( (newHandle = (ObxHandle *)malloc( sizeof( ObxHandle ) )) ) {
      OBXDBGBUF(("iobxHandleNew() malloc, addr=0x%08x, len=%d.\n", newHandle, sizeof(ObxHandle) ));
      newHandle->transport = NULL;
      newHandle->maxPacketLen = OBEX_DEFAULT_PACKET_LENGTH;
      newHandle->lastRecv = 0;
      newHandle->lastSent = 0;
   }
   return newHandle;
}

/*
** Cleanup and destroy a passed handle.
*/
void        iobxHandleFree( ObxHandle *handle ) {
   OBXDBGFLOW(("iobxHandleFree() entry, handle=0x%08x\n", handle));
   if ( handle ) {
      if ( handle->transport ) {
         iobxTransportFree( handle->transport );
      }
      free( handle );
   }
}

/*
** Associate a transport with this handle.
*/
ObxRc          iobxHandleRegisterTransport( ObxHandle *handle, ObxTransport *transport ) {
   ObxRc rc = OBX_RC_ERR_BADPLIST;
   OBXDBGFLOW(("iobxHandleRegisterTransport() entry, handle=0x%08x\ttransport=0x%08x\n", handle, transport));
   if ( handle && transport ) {
      if ( handle->transport ) {
         iobxTransportFree( handle->transport );
      }
      handle->transport = transport;
   }
   return rc;
}




