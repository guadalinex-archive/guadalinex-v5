/*************************************************************************/
/* module:          SyncML HTTP protocol driver                          */
/* file:            src/xpt/bindings/http/linux/xptihttp.h               */
/* target system:   linux                                                */
/* target OS:       linux                                                */
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
 * This module contains linux specific definitions for the
 * HTTP protocol handler for SyncML
 * invokes the miscellaneous protocol drivers.
 *
 */

#ifndef XPTIHTTP_H
#define XPTIHTTP_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "../all/xpttypes.h"

#define itoa(i,a,b) (((b) == 16) ? sprintf((a), "%x", (i)) : sprintf((a), "%d", (i)))
#define ltoa(i,a,b) (((b) == 16) ? sprintf((a), "%x", (i)) : sprintf((a), "%d", (i)))

CString_t _getTime (StringBuffer_t pchBuffer) // in: buffer to a chunk of memory
   {
   const char *wdy [] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
   const char *mon [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
   char achDist [8];
   struct tm * t;
   time_t tm;
   time (&tm);
   t = gmtime (&tm);

   if (t->tm_isdst == 0)
      achDist [0] = '\0';

   else if (t->tm_isdst > 0)
      {
      sprintf(achDist, "+%d", t->tm_isdst);
      }

   else
      sprintf(achDist, "%d", t->tm_isdst);

   sprintf ((char *) pchBuffer,
            "%s, %d %s %d %02d:%02d:%02d GMT%s",
            wdy[t->tm_wday], t->tm_mday, mon[t->tm_mon], t->tm_year+1900,
            t->tm_hour, t->tm_min, t->tm_sec, achDist);
   return pchBuffer;
   }


#endif
