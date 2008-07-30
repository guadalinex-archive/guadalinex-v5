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
#ifndef XPTAUTH_H
#define XPTAUTH_H

#include "xpttypes.h"

#define MAX_DIGEST_SIZE 80
#define MAX_REALM_SIZE	80
#define MAX_NONCE_SIZE	65
#define MAX_USER_SIZE	32
#define MAX_PWD_SIZE	32
#define MAX_CNONCE_SIZE 10
#define MAX_NC_SIZE	10
#define MAX_QOP_SIZE	10
#define MAX_OPAQUE_SIZE 32
#define MAX_DOMAIN_SIZE 256

/********************************************************/
/* Authentication info, Basic and Digest authentication */
/********************************************************/
typedef enum
	 {
	 AUTH_NONE = 0,
	 AUTH_BASIC = 1,
	 AUTH_DIGEST = 2
	 } HttpAuthenticationType_t;	    // required authentication

typedef enum
	 {
	 DEST_NONE = 0,
	 DEST_SERVER = 1,
	 DEST_PROXY  = 2
	 } HttpAuthenticationDest_t;	    // authentication destination

typedef struct
   {
   int cbSize;
   CString_t szUser;
   CString_t szPassword;
   CString_t szRealm;
   CString_t szCNonce;
   CString_t szNC;
   } HttpAuthenticationUserData_t, *HttpAuthenticationUserDataPtr_t;


typedef struct
   {
   int cbSize;
   CString_t szNonce;
   CString_t szNC;
   CString_t szQop;
   } HttpAuthenticationInfo_t, *HttpAuthenticationInfoPtr_t;

typedef struct
   {
   int cbSize;
   CString_t szRealm;
   CString_t szNonce;
   CString_t szOpaque;
   CString_t szDomain;
   CString_t szQop;
   Bool_t    bStale;
   } HttpAuthenticationData_t, *HttpAuthenticationDataPtr_t;

typedef void * HttpAuthenticationPtr_t;

HttpAuthenticationPtr_t authInit (void);
void authTerminate (HttpAuthenticationPtr_t auth);

Bool_t authSetDigest (HttpAuthenticationPtr_t auth,
		      HttpAuthenticationDest_t dest,
		      CString_t szDigest);
Bool_t authCalcDigest (HttpAuthenticationPtr_t auth,
		       HttpAuthenticationDest_t dest,
		       CString_t szURI,
		       CString_t szMode);
CString_t authGetDigest (HttpAuthenticationPtr_t auth,
			 HttpAuthenticationDest_t dest);

Bool_t authSetType (HttpAuthenticationPtr_t auth,
		    HttpAuthenticationDest_t dest,
		    HttpAuthenticationType_t type);
HttpAuthenticationType_t authGetType (HttpAuthenticationPtr_t auth,
				      HttpAuthenticationDest_t dest);


Bool_t authSetUserData (HttpAuthenticationPtr_t auth,
			HttpAuthenticationDest_t dest,
			HttpAuthenticationUserDataPtr_t pUserData);

Bool_t authGetUserData (HttpAuthenticationPtr_t auth,
			HttpAuthenticationDest_t dest,
			HttpAuthenticationUserDataPtr_t pUserData);

Bool_t authSetAuthenticationInfo (HttpAuthenticationPtr_t auth,
				  HttpAuthenticationDest_t dest,
				  HttpAuthenticationInfoPtr_t pAuthInfo);


Bool_t authSetAuthenticationData (HttpAuthenticationPtr_t auth,
				  HttpAuthenticationDest_t dest,
				  HttpAuthenticationDataPtr_t pAuthData);

Bool_t authGetAuthenticationData (HttpAuthenticationPtr_t auth,
				  HttpAuthenticationDest_t dest,
				  HttpAuthenticationDataPtr_t pAuthData);

#endif

/* eof */
