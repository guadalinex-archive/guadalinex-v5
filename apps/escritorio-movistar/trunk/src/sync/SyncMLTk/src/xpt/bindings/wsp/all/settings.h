/*************************************************************************/
/* module:          Communication Services, WSP Settings Functions       */
/* file:            src/xpt/all/settings.h                               */
/* target system:   all                                                  */
/* target OS:       all                                                  */
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
 * Version Label
 *
 * RELEASE ??? CANDIDATE ?
 * 13.06.2000
 */

#ifndef WSP_SETTINGS_H
#define WSP_SETTINGS_H

#include <awsp.h>
#include <wsphttp.h>

#include <xptTransport.h>
#include <wspdef.h>


/**
 *  A structure to hold parameters related to the Service Access Point initialization.
 **/
typedef struct {
 awsp_BEARER_TYPE       bearerType;
 awsp_ADDRESS_TYPE      addressType;
 char                  *serverAddress;
 unsigned short         serverPort;
 char                  *clientAddress;
 unsigned short         clientPort;
} SapParameters;

/**
 *  A structure to hold the WSP parameters passed to selectProtocol().
 **/
typedef struct {
      awsp_BOOL           createSession;
      char               *host;
      SapParameters      *servAccessPtParms;
      awsp_Capabilities  *requestedCapabilities;
      WspHttpParms_t     *httpParms;
} WspSettings_t;


/* Methods invoked from outside settings.c */
unsigned int settingsInitialize(const char *szSettings, WspSettings_t **settingPtr) WSP_SECTION;
void settingsRelease(WspSettings_t *settings) WSP_SECTION;

/* Methods invoked only within settings.c */
awsp_Capabilities *getCapParms(const char*szSettings) WSP_SECTION;
void releaseCapParms(awsp_Capabilities *capParms) WSP_SECTION;
SapParameters *getSapParms(const char *szSettings, awsp_BOOL sessionPort) WSP_SECTION;
void releaseSapParms(SapParameters *sapParms) WSP_SECTION;
const char *getAddressString(awsp_ADDRESS_TYPE type) WSP_SECTION;
const char *getBearerString(awsp_BEARER_TYPE type) WSP_SECTION;


#ifdef WSP_DEBUG
void dumpSap(SapParameters *sap) WSP_SECTION;
void dumpCapabilities(awsp_Capabilities *capabilities) WSP_SECTION;
void dumpSettings(WspSettings_t *settings) WSP_SECTION;
void dumpCharArray(char **arrayPtr, int arraySize) WSP_SECTION;
#endif

#endif
