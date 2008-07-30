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

#include <debug.h>
#include <string.h>
#include <iConstants.h>


/*
** Direct inbound trace output to appropriate output.
** For now, this is just stdout.
*/
#if defined( DEBUGALL ) || defined( DEBUGFLOW ) || defined( DEBUGINFO ) || defined( DEBUGERROR )
// %%% luz: added declaration for LocalOutput
extern void localOutput(const char *fmt, va_list args);

void iobxDebug( const char *format, ... ) {
   va_list args;
   va_start(args, format);
   // luz %%%: added XPT-style output
   #ifdef TRACE_TO_STDOUT
   localOutput(format,args);
   #else
   // original, but bad
   vprintf(format, args);
   #endif
   va_end(args);
}
#endif

#if defined( _WIN32 ) && (defined( DEBUGALL ) || defined( DEBUGFLOW ) || defined( DEBUGINFO ) || defined( DEBUGERROR ))
void _describeWin32SocketError() {
   char *msgbuffer;
   long errcode;
   unsigned long rc;

   errcode = WSAGetLastError();
   rc = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       NULL,
                       errcode,
                       0,
                       (char *)
                       &msgbuffer,
                       0,
                       NULL
                      );
   if ( !rc ) {
      fprintf(stderr, "_describeWin32SocketError() Received error code %ld from FormatMessage() while"
                      " trying\nto format message for error code %ld: \n", GetLastError(), errcode);
   } else {
      iobxDebug( "Socket: (code %ld):  %s", errcode, msgbuffer );
      LocalFree( msgbuffer );
   }
}
#endif

#if defined( DEBUGALL )
void  outputStorage( const char *tagline, const void *mem, int len ) {
   #define SHOW_BYTES 8
   int show = len;
   int i,spc,toshow = 0;
   void *cursor = (void *)mem;
   void *cursorloop = (void *)mem;


   while ( show > 0 ) {
      toshow = min( SHOW_BYTES, show );

      iobxDebug("%s\t", tagline);
      cursorloop = cursor;
      spc = 0;
      for (i=0; i<SHOW_BYTES; i++) {
         if ( i < toshow ) {
            iobxDebug("%02x",*((char *)cursorloop));
            (char *)cursorloop += 1;
         } else {
            iobxDebug("  ");
         }
         if (++spc == 4) {
            spc = 0;
            iobxDebug(" ");
         }
      }

      iobxDebug("\t");
      cursorloop = cursor;
      spc = 0;
      for (i=0; i<SHOW_BYTES; i++) {
         if ( i < toshow ) {
            if ( *((char *)cursorloop) >= 0x20 && *((char *)cursorloop) <= 0x7E ) {
               iobxDebug("%c",*((char *)cursorloop));
            } else {
               iobxDebug(".");
            }
            (char *)cursorloop += 1;
         } else {
            iobxDebug(".");
         }
         if (++spc == 4) {
            spc = 0;
            iobxDebug(" ");
         }
      }

      (char *)cursor += 8;
      show -= 8;

      iobxDebug("\n");
   }
}
#endif


