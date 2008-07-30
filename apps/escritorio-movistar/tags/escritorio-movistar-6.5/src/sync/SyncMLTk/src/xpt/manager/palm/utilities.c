/*************************************************************************/
/* module:          SyncML Communication Protocol platform-dependent code*/
/* file:            src/xpt/palm/utilities.c                             */
/* target system:   Palm                                                 */
/* target OS:       Palm                                                 */
/*************************************************************************/

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

/**
 * This file implements the platform-dependent code for the xpt routing layer.
 */

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#include <xpt.h>
#include <xptTransport.h>

#ifdef TRACE_TO_STDOUT
 #include <HostControl.h>
#endif

#ifndef _MSL_CSTDARG
#include <unix_stdarg.h>
#endif

#include <utilities.h>
#include <xptport.h>

#if defined(TRACE_TO_STDOUT) && !defined(EXTERNAL_LOCALOUTPUT)
void localOutput(const char *format, va_list args) {
   static int initializedTrace XPT_DATA_SECTION = 0;
   char buffer[256];

   Int16 len = StrVPrintF(buffer, format, args);

   if (len >= sizeof buffer) {
      /* Bad news:  We overflowed the buffer.  Die now. */
      ErrDisplayFileLineMsg(__FILE__, (UInt16) __LINE__,
         "Overflowed msg buffer in utilities.c, localOutput()");
   }

   if (!initializedTrace) {
      HostTraceInit();
      initializedTrace = 1;
   }

   /* Call either HostTraceOutputT() or HostTraceOutputTL(), depending on     */
   /* whether or not the given string ends in a newline.  We don't support    */
   /* strings that have embedded newlines anywhere but at the end.            */
   len = StrLen(buffer);
   if (len) {
      if (buffer[len-1]=='\n') {
         buffer[len-1]='\0';
         HostTraceOutputTL(appErrorClass, buffer);
      } else {
         HostTraceOutputT(appErrorClass, buffer);
      }
   }
}
#endif


XPTEXP1 void * XPTAPI XPTEXP2 xppRealloc(void *ptr, size_t size) {
   Err rc;
   UInt32 currentSize;
   void *newMem;

   /* If original pointer is null, act like a normal malloc                */
   if (!ptr) return MemPtrNew(size);

   /* Try resizing original area.  This will always work if the new area   */
   /* is smaller.  It may or may not work if growing the area.             */
   rc = MemPtrResize(ptr, size);
   if (!rc) return ptr;             /* It worked.  Return same pointer.    */

   /* Resizing didn't work.  Allocate a new, larger, area, and copy the    */
   /* previous data from the old area.                                     */
   newMem = MemPtrNew(size);
   if (!newMem) return newMem;      /* Pass error to caller                */

   /* The original size must be smaller than the new size                  */
   currentSize = MemPtrSize(ptr);
   MemMove(newMem, ptr, currentSize);

   /* Release the old area.                                                */
   MemPtrFree(ptr);

   return newMem;
}
