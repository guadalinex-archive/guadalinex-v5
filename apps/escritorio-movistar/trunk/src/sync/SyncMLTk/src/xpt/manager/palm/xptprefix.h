/*************************************************************************/
/* module:          Prefix header for CodeWarrior                        */
/* file:            syncml/src/xpt/manager/palm/xptprefix.h              */
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

/*
 * This file isn't included by any source code.  It's intended to be used as
 * a Metrowerks CodeWarrior "prefix" header file, meaning that it contains
 * preprocessor definitions that apply to every file being compiled.  It is a
 * substitute for the -D option of most line-mode compilers (e.g., GCC).
 */

#ifndef XPTPREFIXES_H
#define XPTPREFIXES_H

#include <Us.Prefix.h>     /* Include the original prefix header file */

/* This suppresses the use of precompiled headers with CodeWarrior.  Not all  */
/* header files of the HTTP transport can stand alone, so using precompiled   */
/* headers causes errors building those header files.                         */
#define PILOT_PRECOMPILED_HEADERS_OFF

/* This causes xpt transports to expect to be statically linked with the xpt  */
/* manager code.                                                              */
#define LINK_TRANSPORT_STATICALLY 1

/* This causes debugging messages to be written during execution.             */
#define TRACE 1

/* This causes some ancient code in the HTTP transport to generate ANSI       */
/* prototypes.                                                                */
#define PROTOTYPES 1

/* Uncomment the following line to use Palm SDK 3.5 instead of SDK 3.1 */
/* #define USE_SDK_35 */

#ifdef USE_SDK_35
 /* Uncomment the following line to cause xpt debugging messages to be written
  * to the HostControl API instead of to the sml debugging log.  The HostControl
  * API allows the user to view the messages using the Reporter application,
  * which makes it especially easy to debug under the emulator.  The HostControl
  * API doesn't work prior to SDK 3.5, though.
  */
/* #define TRACE_TO_STDOUT 1 */
#endif

#endif

