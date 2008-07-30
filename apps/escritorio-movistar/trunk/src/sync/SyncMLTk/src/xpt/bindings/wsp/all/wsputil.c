/*************************************************************************/
/* module:          Communication Services, WSP Utility Functions        */
/* file:            src/xpt/all/wsputil.c                                */
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

#include <wsputil.h>
#include <xpt.h>           /* For the SML_ERR defines */
#include <xptTransport.h>  /* For callback functions  */
#include <xptport.h>

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         Private Implementation Methods                  **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

char *getMetaInfoValue(const char *szSettings, const char *szTag)
{
   char          *retValue       = NULL;
   const char    *tempValue      = NULL;
   size_t         tempSize       = 0;

   XPTDEBUG(("    getMetaInfoValue(%s, %s)\n", szSettings, szTag));

   if ((szSettings == NULL) || (szTag == NULL))
      return retValue;

//   tempValue = getTokenValue(szSettings, ' ', '=', szTag, &tempSize);
   tempValue = xptGetMetaInfoValue(szSettings, szTag, &tempSize);
   if ((tempValue != NULL) && (tempSize >= 0)) {
      retValue = (char *) xppMalloc(tempSize + 1);
      if (retValue != NULL) {
         xppMemcpy(retValue, tempValue, tempSize);
         retValue[tempSize] = '\0';
      }
   }

   return retValue;
}


/**
 *  getHeaderTag
 *       - Parses an HTTP header tag value out of a HTTP header.
 *
 *  IN     tagName         The name of the tag whose value is required.
 *         rspHeader       The buffer containing the http header that is
 *                         to be searched for the tag.
 *
 *  RETURN
 *       A string containing the value of the requested header tag, or
 *       NULL if the tag was not found.
 *
 **/
char *getHeaderTag(const char *tagName, const char *rspHeader)
{
   char          *retValue       = NULL;
   const char    *tempValue      = NULL;
   size_t         tempSize       = 0;

   XPTDEBUG(("    getHeaderTag(%s, %s)\n", tagName, rspHeader));

   if ((tagName == NULL) || (rspHeader == NULL))
      return retValue;

   tempValue = getTokenValue(rspHeader, '\n', ':', tagName, &tempSize);

   if ((tempValue != NULL) && (tempSize >= 0)) {
      retValue = (char *) xppMalloc(tempSize + 1);
      if (retValue != NULL) {
         xppMemset(retValue, 0, tempSize + 1);
         xppStrncpy(retValue, tempValue, tempSize);
      }
   }

   return retValue;
} /* End of getHeaderTag() */

char *getHeaderParmValue(const char *parmName, const char *header)
{
   char          *retValue       = NULL;
   const char    *tempValue      = NULL;
   size_t         tempSize       = 0;

   XPTDEBUG(("    getHeaderParmValue(%s, %s)\n", parmName, header));

   if ((parmName == NULL) || (header == NULL))
      return retValue;

   tempValue = getTokenValue(header, ',', '=', parmName, &tempSize);
   if ((tempValue != NULL) && (tempSize >= 0)) {
      retValue = (char *) xppMalloc(tempSize + 1);
      if (retValue != NULL) {
         xppMemset(retValue, 0, tempSize + 1);
         xppStrncpy(retValue, tempValue, tempSize);
      }
   }

   return retValue;
}


unsigned int updateStringField(char **field, const char *updateValue)
{
   unsigned int rc = SML_ERR_OK;

   XPTDEBUG(("    updateStringField(%lx, %s)\n", (unsigned long) field, updateValue));

   if (field == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   if (*field != NULL) {
      xppFree(*field);
      *field = NULL;
   }

   if (updateValue != NULL) {
      *field = (char *) xppMalloc(xppStrlen(updateValue) + 1);
      if (*field == NULL)
         return SML_ERR_A_XPT_MEMORY;
      xppStrcpy(*field, updateValue);
   }

   return rc;
} /* End updateStringField() */


const char *getTokenValue(const char  *sourceString,
                          int          tokenSeparator,
                          int          valueSeparator,
                          const char  *tokenName,
                          size_t *valueSize) {
#define WHITESPACE   "   "

   const char *p, *vs, *ve;
   int found = 0;

   XPTDEBUG(("    getTokenValue(%s, %c, %c, %s, %lx) ", sourceString, tokenSeparator,
          valueSeparator, tokenName, (unsigned long) valueSize));

   p = sourceString;
   do {
      const char *ts, *te;

      /* Skip over delimiters and whitespace */
      for (ts=p; *ts == tokenSeparator || (*ts && xppStrchr(WHITESPACE, *ts)); ++ts) ;
      if (!*ts) break;           /* We've hit the end of the string */

      /* The variable p now points to the start of a tag name.  Find the end. */
      for (te=ts; *te && *te != tokenSeparator && *te != valueSeparator &&
                  !xppStrchr(WHITESPACE, *te); ++te) ;

      /* The variable end now points just beyond the last char of the tag name.
       * Find the beginning and end of the value (if any).  First, skip over
       * white space.                                                         */
      for (vs=te; *vs && xppStrchr(WHITESPACE, *vs); ++vs) ;

      if (*vs != valueSeparator) {
         /* If the tag ends with anything but the value separator, there's no value. */
         p = ve = vs;
      } else {
         vs = vs + 1;
         for (ve=vs; *ve && *ve != tokenSeparator &&
                     !xppStrchr(WHITESPACE, *te); ++ve) ;
         p = ve;                 /* For next time through the loop */
      }

      /* See if this is the tag we care about */
      found = te-ts == (int)xppStrlen(tokenName)  &&  !xppMemcmp(tokenName, ts, (te-ts));
   } while (!found && *p);

   if (!found) return NULL;

   *valueSize = ve-vs;

   XPTDEBUG(("returns tag found; valueSize(%lu)\n", (unsigned long) *valueSize));

   return vs;
#undef WHITESPACE
}


unsigned int setLastError(long protocolErrorCode, const char *errorMsg)
{
   Ret_t rc = SML_ERR_OK;
   struct xptTransportErrorInformation info;

   XPTDEBUG(("    setLastError(%ld, %s)\n", protocolErrorCode, errorMsg));

   info.protocolErrorCode = protocolErrorCode;
   info.errorMessage = errorMsg;

   rc = xptSetLastError(&info);

   return rc;
} /* End setLastError */


