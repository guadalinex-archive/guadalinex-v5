/*************************************************************************/
/* module:          SyncML HTTP protocol driver                          */
/* file:            src/xpt/palm/xptihttp.h                              */
/* target system:   palm                                                 */
/* target OS:       palm                                                 */
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
 * This module contains Palm OS specific definitions for the
 * HTTP protocol handler for SyncML
 * invokes the miscellaneous protocol drivers.
 *
 */

#ifndef XPTIHTTP_H
#define XPTIHTTP_H

#define STATIC_MEMORY_MANAGEMENT    // This flag enables that certain strings
                                    // are stored in a reserved block of memory onside the
                                    // the instance object store. This is less flexible
                                    // than allocating secondary storage.
#define STATIC_MEMORY_SIZE 500      // # of bytes reserved in the object store
                                    // to store string values


/************/
/* Includes */
/************/

#ifdef USE_SDK_35
 #include <PalmOS.h>
#else
 #include <Pilot.h>
#endif

#ifndef Bool_t
#define Bool_t unsigned int
#endif

#include <StringMgr.h>
#include <DateTime.h>


/*******************************************************/
/* Memory and string management function substitutions */
/*******************************************************/

#define atol(s) StrAToI((s))
#define ltoa(i,a,b) (((b) == 16) ? StrIToH((a),(i)) : StrIToA((a),(i)))
#define itoa(i,a,b) (((b) == 16) ? StrIToH((a),(i)) : StrIToA((a),(i)))
#define sprintf StrPrintF


/******************************************************/
/* Function: return the current time (RFC 850 Format) */
/******************************************************/
const char * _getTime (char * pchBuffer) // in: buffer to a chunk of memory
   {
   const char *wdy [] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
   const char *mon [] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
   char achDist [8];

   DateTimeType dt;
   achDist [0] = '\0';
   TimSecondsToDateTime (TimGetSeconds (), &dt);

   sprintf ((char *) pchBuffer,
            "%s, %d %s %d %0.2d:%0.2d:%0.2d",
            wdy[dt.weekDay], dt.day, mon[dt.month], dt.year+1904,
            dt.hour, dt.minute, dt.second, achDist);
   return pchBuffer;
   }

#endif
