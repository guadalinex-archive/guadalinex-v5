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

#include <xptport.h>

#include "xpt-b64.h"
#include "digcalc.h"
#include "xpt-auth.h"


typedef struct _HttpDigest
   {
   int  cbSize;                    // size of this structure
   HttpAuthenticationType_t fType; // type of authorization, set by server
   HttpAuthenticationDest_t fDest; // destination: server or proxy?

   char achUser   [MAX_USER_SIZE];  // user name
   char achPassword [MAX_PWD_SIZE]; // Password
   char achDigest [MAX_DIGEST_SIZE];// digest value
   char achRealm  [MAX_REALM_SIZE]; // server realm value, set by server
   Bool_t bStale;                   // stale flag
   char achCNonce [MAX_CNONCE_SIZE];// Client Nonce.
   char achNonce [MAX_NONCE_SIZE];  // nonce value,
   char achQOp   [MAX_QOP_SIZE];    // selected algorithm
   char achNC    [MAX_NC_SIZE];     // incremented after each HTTP message.
   char achOpaque [MAX_OPAQUE_SIZE];// opaque value, set by server
   char achDomain [MAX_DOMAIN_SIZE];// domain Info
   } HttpDigest_t, *HttpDigestPtr_t;


/***********************/
/* Authentication Info */
/***********************/
typedef struct
   {
   int rfu;                     // not used.
   int cbCount;                 // # of authentication structures
   HttpDigestPtr_t digest [1];  // array of digest structures
   } HttpAuthentication_t;

HttpDigestPtr_t getDigest (HttpAuthenticationPtr_t handle,
                           HttpAuthenticationDest_t dest)
   {
   HttpAuthentication_t *auth = (HttpAuthentication_t *) handle;
   HttpDigestPtr_t dig = NULL;
   if (auth)
      {
      int n;
      for (n = 0; n < auth->cbCount; n ++)
         {
         if (auth->digest[n]->fDest == dest)
            {
            dig = auth->digest[n];
            break;
            }
         }
      }
   return dig;
   }


HttpAuthenticationPtr_t authInit (void)
   {
   /************************/
   /* Create objext stores */
   /************************/
   int cbAuthSize = sizeof (HttpAuthentication_t) + sizeof (HttpDigestPtr_t);
   HttpAuthentication_t * auth = (HttpAuthentication_t*) xppMalloc (cbAuthSize);
   if (auth == NULL) return NULL;
   xppMemset (auth, 0, cbAuthSize);

   /*******************************************/
   /* Create two digest structures:           */
   /* one to authenticate against the server, */
   /* and one to authenticate against a proxy */
   /*******************************************/
   auth->cbCount = 2;
   auth->digest [0] = (HttpDigestPtr_t) xppMalloc (sizeof (HttpDigest_t));
   auth->digest [1] = (HttpDigestPtr_t) xppMalloc (sizeof (HttpDigest_t));
   if ((auth->digest [0] == NULL) || (auth->digest [1] == NULL))
      {
      xppFree (auth);
      return NULL;
      }
   xppMemset (auth->digest [0], 0, sizeof (HttpDigest_t));
   xppMemset (auth->digest [1], 0, sizeof (HttpDigest_t));
   auth->digest[0]->fDest = DEST_SERVER;
   auth->digest[1]->fDest = DEST_PROXY;
   auth->digest[0]->cbSize = sizeof (HttpDigest_t);
   auth->digest[1]->cbSize = sizeof (HttpDigest_t);

   return (HttpAuthenticationPtr_t) auth;
   }


void authTerminate (HttpAuthenticationPtr_t handle)
   {
   HttpAuthentication_t *auth = (HttpAuthentication_t *) handle;
   int n;
   if (auth != NULL)
      {
      for (n = 0; n < auth->cbCount; n ++)
         {
         if ((auth->digest[n] != NULL) &&
             (auth->digest[n]->cbSize == sizeof (HttpAuthentication_t)))
            {
            auth->digest[n]->cbSize = 0;
            xppFree (auth->digest[n]);
            auth->digest[n] = NULL;
            }
         }
      auth->cbCount = 0;
      xppFree (auth);
      }
   return;
   }

Bool_t authSetDigest (HttpAuthenticationPtr_t auth,
                      HttpAuthenticationDest_t dest,
                      CString_t szDigest)
   {
   Bool_t rc = true;
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/

   if ((dest != AUTH_NONE) && (auth != NULL) &&
       (szDigest != NULL) &&
       (xppStrlen (szDigest) < MAX_DIGEST_SIZE) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      xppStrcpy (dig->achDigest, szDigest);
   else
      rc = false;
   return rc;
   }

Bool_t authSetUserData (HttpAuthenticationPtr_t auth,
                        HttpAuthenticationDest_t dest,
                        HttpAuthenticationUserDataPtr_t pUserData)
   {
   Bool_t rc = true;
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/

   if ((dest != AUTH_NONE) && (auth != NULL) &&
       (pUserData != NULL) &&
       (pUserData->cbSize == sizeof (HttpAuthenticationUserData_t)) &&
       (pUserData->szUser != NULL) && (pUserData->szPassword != NULL) &&
       (!pUserData->szRealm || (xppStrlen (pUserData->szRealm) < MAX_REALM_SIZE)) &&
       (!pUserData->szCNonce || (xppStrlen (pUserData->szCNonce) < MAX_CNONCE_SIZE)) &&
       (!pUserData->szNC || (xppStrlen (pUserData->szNC) < MAX_NC_SIZE)) &&
       (xppStrlen (pUserData->szUser) < MAX_USER_SIZE) &&
       (xppStrlen (pUserData->szPassword) < MAX_PWD_SIZE) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      {
      xppStrcpy (dig->achUser, pUserData->szUser);
      xppStrcpy (dig->achPassword, pUserData->szPassword);
      if (pUserData->szRealm == NULL)
         xppStrcpy (dig->achRealm, "");
      else
         xppStrcpy (dig->achRealm, pUserData->szRealm);
      if (pUserData->szCNonce == NULL)
         xppStrcpy (dig->achCNonce, "");
      else
         xppStrcpy (dig->achCNonce, pUserData->szCNonce);
      if (pUserData->szNC == NULL)
         xppStrcpy (dig->achNC, "");
      else
         xppStrcpy (dig->achNC, pUserData->szNC);
      }
   else
      rc = false;
   return rc;
   }

Bool_t authSetType (HttpAuthenticationPtr_t auth,
                    HttpAuthenticationDest_t dest,
                    HttpAuthenticationType_t type)
   {
   Bool_t rc = true;
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/

   if ((dest != AUTH_NONE) && (auth != NULL) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      dig->fType = type;
   else
      rc = false;
   return rc;
   }




HttpAuthenticationType_t authGetType (HttpAuthenticationPtr_t auth,
                                      HttpAuthenticationDest_t dest)
   {
   HttpAuthenticationType_t rc = AUTH_NONE;
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/

   if ((dest != AUTH_NONE) && (auth != NULL) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      rc = dig->fType;
   return rc;
   }

CString_t authGetDigest (HttpAuthenticationPtr_t auth,
                         HttpAuthenticationDest_t dest)
   {
   CString_t rc = "";
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/

   if ((dest != AUTH_NONE) && (auth != NULL) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      rc = dig->achDigest;

   return rc;
   }


Bool_t authCalcDigest (HttpAuthenticationPtr_t auth,
                       HttpAuthenticationDest_t dest,
                       CString_t szURI, CString_t szMode)
   {
   Bool_t rc = true;
   HttpDigestPtr_t dig;
   HASHHEX ha1, ha2 = "";
   char szSeparator [2] = { ':', 0 };
   BufferSize_t cbDigestSize = MAX_DIGEST_SIZE;
   BufferSize_t cbLength;
   BufferSize_t cbOffset = 0; // <<<<< ????
   char abSave [4];
   int cbCopied = 0;

   /**************************/
   /* check entry conditions */
   /**************************/

   if ((dest != AUTH_NONE) && (auth != NULL) &&
       (szURI != NULL) && (szMode != NULL) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      {
      switch (dig->fType)
         {

         case AUTH_DIGEST:
            /*******************************************/
            /* Digest authentication:                  */
            /* compute the digest according to RFC2617 */
            /*******************************************/
            DigestCalcHA1 ("md5",
                           dig->achUser, dig->achRealm, dig->achPassword,
                           dig->achNonce, dig->achCNonce, ha1);
            DigestCalcResponse (ha1, dig->achNonce, dig->achNC,
                           dig->achCNonce, dig->achQOp, (char *)szMode,
                           (char *) szURI, ha2, dig->achDigest);
            rc = true;
            break;

         case AUTH_BASIC:
            /**************************************/
            /* Basic authentication:              */
            /* calculate a base64-encoding of the */
            /* string <user>:<password>           */
            /**************************************/
            abSave [0] = abSave [1] = abSave [2] = abSave [3] = '\0';
            cbLength = (unsigned long) xppStrlen (dig->achUser);
            cbCopied += base64Encode ((DataBuffer_t) dig->achDigest, cbDigestSize,
                                 (DataBuffer_t) dig->achUser, &cbLength, &cbOffset, 0, (unsigned char *)abSave);
            cbLength = 1;
            cbCopied += base64Encode ((DataBuffer_t) dig->achDigest+cbCopied, cbDigestSize-cbCopied,
                                 (DataBuffer_t) szSeparator, &cbLength, &cbOffset, 0, (unsigned char *)abSave);

            cbLength = (unsigned long) xppStrlen (dig->achPassword);
            cbCopied += base64Encode ((DataBuffer_t) dig->achDigest+cbCopied, cbDigestSize-cbCopied,
                                 (DataBuffer_t) dig->achPassword, &cbLength, &cbOffset, 1, (unsigned char *)abSave);
            dig->achDigest [cbCopied] = '\0';

            rc = true;
            break;
         default:
            rc = false;
         }
      }
   else
      rc = false;
   return rc;
   }



Bool_t authSetAuthenticationInfo (HttpAuthenticationPtr_t auth,
                                  HttpAuthenticationDest_t dest,
                                  HttpAuthenticationInfoPtr_t pAuthInfo)
   {
   Bool_t rc = true;
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/
   if ((dest != AUTH_NONE) && (auth != NULL) &&
       (pAuthInfo != NULL) &&
       (pAuthInfo->cbSize == sizeof (HttpAuthenticationInfo_t)) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      {
      if (pAuthInfo->szNonce != NULL)
         {
         if (xppStrlen(pAuthInfo->szNonce) < MAX_NONCE_SIZE)
            xppStrcpy (dig->achNonce, pAuthInfo->szNonce);
         else rc = false;
         }
      if (pAuthInfo->szNC != NULL)
         {
         if (xppStrlen(pAuthInfo->szNC) < MAX_NC_SIZE)
            xppStrcpy (dig->achNC, pAuthInfo->szNC);
         else rc = false;
         }
      if (pAuthInfo->szQop != NULL)
         {
         if (xppStrlen(pAuthInfo->szQop) < MAX_QOP_SIZE)
            xppStrcpy (dig->achQOp, pAuthInfo->szQop);
         else rc = false;
         }
      }
   else
      rc = false;
   return rc;
   }


Bool_t authSetAuthenticationData (HttpAuthenticationPtr_t auth,
                                  HttpAuthenticationDest_t dest,
                                  HttpAuthenticationDataPtr_t pAuthData)
   {
   Bool_t rc = true;
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/
   if ((dest != AUTH_NONE) && (auth != NULL) &&
       (pAuthData != NULL) &&
       (pAuthData->cbSize == sizeof (HttpAuthenticationData_t)) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      {
      dig->achRealm [0] = '\0';
      dig->achNonce [0] = '\0';
      dig->achOpaque [0] = '\0';
      dig->achDomain [0] = '\0';
      dig->achQOp [0] = '\0';
      dig->bStale = pAuthData->bStale;

      if (pAuthData->szRealm != NULL)
         {
         /****************************************************/
         /* If a Realm value has been preset by the user,    */
         /* check if it is correct. Return an error, if not. */
         /****************************************************/
         if (dig->achRealm [0] != '\0')
            {
            if (xppStricmp (dig->achRealm, pAuthData->szRealm))
               rc = false;
            }
         else
            {
            /****************************************************/
            /* The user did not set a realm value:              */
            /* It is assumed that the server realm value is OK. */
            /****************************************************/
            if (xppStrlen(pAuthData->szRealm) < MAX_REALM_SIZE)
               xppStrcpy (dig->achRealm, pAuthData->szRealm);
            else rc = false;
            }
         }

      if (pAuthData->szNonce != NULL)
         {
         if (xppStrlen(pAuthData->szNonce) < MAX_NONCE_SIZE)
            xppStrcpy (dig->achNonce, pAuthData->szNonce);
         else rc = false;
         }

      if (pAuthData->szOpaque != NULL)
         {
         if (xppStrlen(pAuthData->szOpaque) < MAX_OPAQUE_SIZE)
            xppStrcpy (dig->achOpaque, pAuthData->szOpaque);
         else rc = false;
         }
      if (pAuthData->szDomain != NULL)
         {
         if (xppStrlen(pAuthData->szDomain) < MAX_DOMAIN_SIZE)
            xppStrcpy (dig->achDomain, pAuthData->szDomain);
         else rc = false;
         }
      if (pAuthData->szQop != NULL)
         {
         if (xppStrlen(pAuthData->szQop) < MAX_QOP_SIZE)
            xppStrcpy (dig->achQOp, pAuthData->szQop);
         else rc = false;
         }
      }
   return false;
   }




Bool_t authGetUserData (HttpAuthenticationPtr_t auth,
                        HttpAuthenticationDest_t dest,
                        HttpAuthenticationUserDataPtr_t pUserData)
   {
   Bool_t rc = false;
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/
   if ((dest != AUTH_NONE) && (auth != NULL) &&
       (pUserData != NULL) &&
       (pUserData->cbSize == sizeof (HttpAuthenticationUserData_t)) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      {
      rc = true;
      pUserData->szUser = dig->achUser;
      pUserData->szPassword = dig->achPassword;
      pUserData->szRealm = dig->achRealm;
      pUserData->szCNonce = dig->achCNonce;
      pUserData->szNC  = dig->achNC;
      }
   return rc;
   }

Bool_t authGetAuthenticationData (HttpAuthenticationPtr_t auth,
                                  HttpAuthenticationDest_t dest,
                                  HttpAuthenticationDataPtr_t pAuthData)
   {
   Bool_t rc = false;
   HttpDigestPtr_t dig;

   /**************************/
   /* check entry conditions */
   /**************************/
   if ((dest != AUTH_NONE) && (auth != NULL) &&
       (pAuthData != NULL) &&
       (pAuthData->cbSize == sizeof (HttpAuthenticationData_t)) &&
       ((dig = getDigest (auth, dest)) != NULL) )
      {
      rc = true;
      pAuthData->szRealm = dig->achRealm;
      pAuthData->szNonce = dig->achNonce;
      pAuthData->szOpaque = dig->achOpaque;
      pAuthData->szDomain = dig->achDomain;
      pAuthData->szQop = dig->achQOp;
      pAuthData->bStale = dig->bStale;
      }
   return rc;
   }

/* eof */
