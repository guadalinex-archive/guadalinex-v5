/*************************************************************************/
/* module:          Communication Services, WSP Settings Functions       */
/* file:            src/xpt/all/settings.c                               */
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

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#include <settings.h>
#include <wsputil.h>
#include <wspdef.h>        /* For atoi on palm */
//#include <xptTransport.h>  /* For metadata parsing */
#include <xpt.h>           /* For SML_ERRs */

#include <xptport.h>

/*****************************************************************************/
/*****************************************************************************/
/**                                                                         **/
/**                         Private Implementation Methods                  **/
/**                                                                         **/
/*****************************************************************************/
/*****************************************************************************/

/**
 *  settingsInitialize
 *       - parses the settings string into WspSettings_t structure
 *       - updates protocol object to point to the settings structure
 *
 *  IN:    szSettings      A string containing WSP settings parameters
 *
 *  OUT:   settingPtr      Updated to point to a settings structure.
 *
 *  RETURN:
 *    An indication of whether the settings were successfully initialized.
 *    If any required settings are missing, the initialization will fail.
 *
 **/
unsigned int settingsInitialize(const char *szSettings, WspSettings_t **settingPtr) {

   unsigned int   rc          = SML_ERR_OK;
   WspSettings_t *settings    = NULL;
   char    *tempValue   = NULL;

   XPTDEBUG(("settingsInitialize(%s, %lx)\n", szSettings, (unsigned long) settingPtr));

   if (settingPtr == NULL)
      return SML_ERR_A_XPT_INVALID_PARM;

   settings = (WspSettings_t *) xppMalloc (sizeof(WspSettings_t));
   if (settings == NULL)
      return SML_ERR_A_XPT_MEMORY;

   xppMemset(settings, 0, sizeof(WspSettings_t));

   /*
    * Should we return 'All Parms processed', 'Required Parms and Some optional
    * parms processed', and 'Required Parms Missing' to give the user the choice
    * of letting it go without all the optional parms?  Or should we fail to
    * initialize if any of the inputted parms cannot be processed?
    *
    * Currently we only fail if we don't get all the required parms...
    */

   /* Process required settings */
   settings->host = getMetaInfoValue(szSettings, "syncServer");
   if (settings->host == NULL) {
      xppFree(settings);
      setLastError(SML_ERR_A_XPT_INVALID_PARM, "required wsp setting 'syncServer' is missing.");
      return SML_ERR_A_XPT_INVALID_PARM;
   } /* End missing syncServer setting */

   /* Some HTTP settings are required, too */
   rc = httpInitialize(szSettings, &(settings->httpParms));
   if (rc != SML_ERR_OK) {
      settingsRelease(settings);
      return SML_ERR_A_XPT_INVALID_PARM;
   }

   /* Process optional settings */
   tempValue = getMetaInfoValue(szSettings, "createSession");
   if ((tempValue != NULL) && (!xppStrcmp(tempValue, "1")))
      settings->createSession = AWSP_TRUE;
   else
      settings->createSession = AWSP_FALSE;
   xppFree(tempValue);

   settings->servAccessPtParms     = getSapParms(szSettings, settings->createSession);
   settings->requestedCapabilities = getCapParms(szSettings);

   /* Store parsed settings into protocol handle                               */
   *settingPtr = settings;

#ifdef WSP_DEBUG
   dumpSettings(settings);
#endif

   return rc;
} /* End settingsInitialize() */

/**
 *  settingsRelease
 *       - xppFrees storage for WspSettings_t structure
 *       - updates protocol object to nullify settings reference
 *
 *  IN:    settings        A pointer to a settings structure
 *
 **/
void settingsRelease(WspSettings_t *settings) {

   if (settings == NULL) return;

   XPTDEBUG(("settingsRelease(%lx)\n", (unsigned long) settings));

   httpRelease(settings->httpParms);
   releaseCapParms(settings->requestedCapabilities);
   releaseSapParms(settings->servAccessPtParms);

   xppFree(settings->host);
   xppFree(settings);

} /* End of settingsRelease() */


/**
 *  getSapParms
 *       - parses the settings string into an SapParameters structure
 *
 *  IN:    szSettings      A string containing WSP settings parameters
 *         sessionPort     An indication of whether the settings are
 *                         requesting a session or not.  This influences
 *                         the default port of the service access point.
 *  RETURN:
 *    A pointer to the SapParameters structure containing the parsed
 *    Service Access Point settings.
 *
 **/
SapParameters *getSapParms(const char *szSettings, awsp_BOOL sessionPort) {
      awsp_BOOL hasWapSettings = AWSP_FALSE;
      char  *tempValue = NULL;
      SapParameters *parms   = NULL;

   XPTDEBUG(("  getSapParms(%s, %i)\n", szSettings, (int) sessionPort));

      parms = (SapParameters *) xppMalloc(sizeof(SapParameters));

      /**
       * Initialize default values to take for missing WAP parameters.
       * I'm assuming a session port, how do i determine if this should default
       * to a connectionless port (9200)??
       **/
      parms->bearerType    = AWSP_DEFAULT_BEARER;
      parms->addressType   = AWSP_DEFAULT_ADDRESS;
      parms->serverAddress = "";

      if (sessionPort)
         parms->serverPort    = 9201;
      else
         parms->serverPort    = 9200;

      parms->clientAddress = "";
      parms->clientPort    = parms->serverPort;

      tempValue = getMetaInfoValue(szSettings, "wapBearerType");
      if (tempValue != NULL) {
         parms->bearerType = atoi(tempValue);
         hasWapSettings = AWSP_TRUE;
      }
      xppFree(tempValue);

      tempValue = getMetaInfoValue(szSettings, "wapAddressType");
      if (tempValue != NULL) {
         parms->addressType = atoi(tempValue);
         hasWapSettings = AWSP_TRUE;
      }
      xppFree(tempValue);

      parms->serverAddress = getMetaInfoValue(szSettings, "wapServerAddr");
      if (parms->serverAddress != NULL)
         hasWapSettings = AWSP_TRUE;

      tempValue = getMetaInfoValue(szSettings, "wapServerPort");
      if (tempValue != NULL) {
         parms->serverPort = atoi(tempValue);
         hasWapSettings = AWSP_TRUE;
      }
      xppFree(tempValue);

      parms->clientAddress = getMetaInfoValue(szSettings, "wapClientAddr");
      if (parms->clientAddress != NULL)
         hasWapSettings = AWSP_TRUE;

      tempValue = getMetaInfoValue(szSettings, "wapClientPort");
      if (tempValue != NULL) {
         parms->clientPort = atoi(tempValue);
         hasWapSettings = AWSP_TRUE;
      }
      xppFree(tempValue);

      if (hasWapSettings == AWSP_FALSE) {
         xppFree(parms);
         parms = NULL;
      }

      return parms;
} /* End of getSapParms() */


/**
 *  releaseSapParms
 *       - xppFrees the storage associate with the input SapParameters structure.
 *       - nullifies the input pointer to the structure.
 *
 *  IN:    sapParms        Address of the pointer to the SapParameters structure
 *
 **/
void releaseSapParms(SapParameters *sapParms) {

   XPTDEBUG(("  releaseSapParms(%lx)\n", (unsigned long) sapParms));

   if (sapParms == NULL) return;

   if (!xppStrcmp(sapParms->serverAddress, ""))
      xppFree(sapParms->serverAddress);

   if (!xppStrcmp(sapParms->clientAddress, ""))
      xppFree(sapParms->clientAddress);

   xppFree(sapParms);

} /* End of releaseSapParms() */

/**
 *  getCapParms
 *       - parses the settings string into an awsp_Capabilities object to
 *         represent the requested capabilities.
 *
 *  IN:    szSettings      A string containing WSP settings parameters
 *  RETURN:
 *    A pointer to the awsp_Capabilities structure containing the parsed
 *    capabilities negotiation settings.
 *
 **/
awsp_Capabilities *getCapParms(const char *szSettings) {

      awsp_BOOL hasCapSettings = AWSP_FALSE;
      awsp_Capabilities *parms = NULL;
      char *tempValue = NULL;

   XPTDEBUG(("  getCapParms(%s)\n", szSettings));

      parms = (awsp_Capabilities *) xppMalloc(sizeof(awsp_Capabilities));

      /**
       * Initialize default values to take for missing Capabilities parameters.
       **/
      if (parms == NULL)
         return NULL;

      xppMemset(parms, 0, sizeof(awsp_Capabilities));

      /**
       * How to deal with retrieving the data for an array of values?  The
       * settingsGetParm method assumes only one of each value.  Also, it
       * is blank-delimitted, so any MSISDN numbers with embedded blanks need
       * to be quoted, and I need to strip the quotes...
       *
       * Not sure if/how awsp_Capabilities object can be constructed if the
       * number of elements in the array is not defined...
       **/

      tempValue = getMetaInfoValue(szSettings, "capAlias");
      if (tempValue != NULL) {
         parms->aliases = (char **) xppMalloc(10 * sizeof(char *));
         parms->aliases[0] = tempValue;
         parms->aliasesSz = 1;
         hasCapSettings = AWSP_TRUE;
      }

      tempValue = getMetaInfoValue(szSettings, "capClientSDUSize");
      if (tempValue != NULL) {
         parms->clientSDUsz = atoi(tempValue);
         hasCapSettings = AWSP_TRUE;
      }
      xppFree(tempValue);

      tempValue = getMetaInfoValue(szSettings, "capExtendedMethod");
      if (tempValue != NULL) {
         parms->extendedMethods = (char **) xppMalloc(10 * sizeof(char *));
         parms->extendedMethods[0] = tempValue;
         parms->extendedMethodsSz = 1;
         hasCapSettings = AWSP_TRUE;
      }

      tempValue = getMetaInfoValue(szSettings, "capHeaderCodePages");
      if (tempValue != NULL) {
         parms->headerCodePages = (char **) xppMalloc(10 * sizeof(char *));
         parms->headerCodePages[0] = tempValue;
         parms->headerCodePagesSz = 1;
         hasCapSettings = AWSP_TRUE;
      }

      tempValue = getMetaInfoValue(szSettings, "capMaxOutstandReq");
      if (tempValue != NULL) {
         parms->maxOutstandingMethodRequests = atoi(tempValue);
         hasCapSettings = AWSP_TRUE;
      }
      xppFree(tempValue);

      tempValue = getMetaInfoValue(szSettings, "capMaxOutstPush");
      if (tempValue != NULL) {
         parms->maxOutstandingPushRequests = atoi(tempValue);
         hasCapSettings = AWSP_TRUE;
      }
      xppFree(tempValue);

      tempValue = getMetaInfoValue(szSettings, "capProtocolOption");
      if (tempValue != NULL) {
         parms->protocolOptions = (char **) xppMalloc(10 * sizeof(char *));
         parms->protocolOptions[0] = tempValue;
         parms->protocolOptionsSz = 1;
         hasCapSettings = AWSP_TRUE;
      }

      tempValue = getMetaInfoValue(szSettings, "capServerSDUSize");
      if (tempValue != NULL) {
            parms->serverSDUsz = atoi(tempValue);
            hasCapSettings = AWSP_TRUE;
      }
      xppFree(tempValue);


      if (hasCapSettings == AWSP_FALSE) {
         xppFree(parms);
         parms = NULL;
      }

      return parms;

} /* End of getCapParms() */

/**
 *  releaseCapParms
 *       - xppFrees the storage associate with the input awsp_Capabilities structure.
 *       - nullifies the input pointer to the structure.
 *
 *  IN:    capParms        Address of the pointer to the awsp_Capabilities structure
 *
 **/
void releaseCapParms(awsp_Capabilities *capParms) {

   XPTDEBUG(("  releaseCapParms(%lx)\n", (unsigned long) capParms));

   if (capParms == NULL) return;

   /* Need to xppFree all elements of the arrays... */
   if (!xppStrcmp(capParms->aliases[0], "")) {
      xppFree(capParms->aliases[0]);              /* xppFree elements of array */
      xppFree(capParms->aliases);                 /* xppFree array itself      */
   }

   if (!xppStrcmp(capParms->extendedMethods[0], "")) {
      xppFree(capParms->extendedMethods[0]);      /* xppFree elements of array */
      xppFree(capParms->extendedMethods);         /* xppFree array itself      */
   }

   if (!xppStrcmp(capParms->headerCodePages[0], "")) {
      xppFree(capParms->headerCodePages[0]);      /* xppFree elements of array */
      xppFree(capParms->headerCodePages);         /* xppFree array itself      */
   }

   if (!xppStrcmp(capParms->protocolOptions[0], ""))  {
      xppFree(capParms->protocolOptions[0]);      /* xppFree elements of array */
      xppFree(capParms->protocolOptions);         /* xppFree array itself      */
   }

   xppFree(capParms);

} /* End of releaseCAPParms() */


const char *getBearerString(awsp_BEARER_TYPE type) {
   const char *retValue = "DEFAULT";

   switch(type) {
      case AWSP_USSD:
         retValue = "USSD";
         break;
      case AWSP_CSD:
         retValue = "CSD";
         break;
      case AWSP_CDPD:
         retValue = "CDPD";
         break;
      case AWSP_SMS:
         retValue = "SMS";
         break;
      case AWSP_PACKET:
         retValue = "PACKET";
         break;
      case AWSP_FLEX_REFLEX:
         retValue = "FLEX, REFLEX";
         break;
      case AWSP_GUTS_RDATA:
         retValue = "GUTS RDATA";
         break;
      case AWSP_GPRS:
         retValue = "GPRS";
         break;
      case AWSP_SDS:
         retValue = "SDS";
         break;
      default:
         break;
   } /* End bearer switch */

   return retValue;
} /* End getBearerString() */


const char *getAddressString(awsp_ADDRESS_TYPE type) {
   const char *retValue = "DEFAULT";

   switch(type) {
      case AWSP_IPV4:
         retValue = "IP Version 4";
         break;
      case AWSP_IPV6:
         retValue = "IP Version 6";
         break;
      case AWSP_GSM_MSISDN:
         retValue = "GSM SMS";
         break;
      case AWSP_IS_136_MSISDN:
         retValue = "IS-136";
         break;
      case AWSP_IS_637_MSISDN:
         retValue = "IS-637";
         break;
      case AWSP_IDEN_MSISDN:
         retValue = "iDen SMS";
         break;
      case AWSP_FLEX_MSISDN:
         retValue = "FLEX";
         break;
      case AWSP_PHS_MSISDN:
         retValue = "PHS";
         break;
      case AWSP_GSM_SERVICE_CODE:
         retValue = "GSM USSD";
         break;
      case AWSP_TETRA_ITSI:
         retValue = "TETRA SDS ITSI";
         break;
      case AWSP_TETRA_MSISDN:
         retValue = "TETRA SDS MSISDN";
         break;
      default:
         break;
   } /* End address switch */

   return retValue;
} /* End getAddressString() */


#ifdef WSP_DEBUG

void dumpSettings(WspSettings_t *settings) {

   if (settings == NULL)
      return;

   XPTDEBUG(("  Meta Data Settings: \n"));
   XPTDEBUG(("    createSession:  %s\n", (settings->createSession) ? "true":"false"));
   XPTDEBUG(("    host         :  %s\n", settings->host));
   dumpSap(settings->servAccessPtParms);
   dumpCapabilities(settings->requestedCapabilities);

} /* End dumpSettings() */

void dumpCapabilities(awsp_Capabilities *capabilities) {

   if (capabilities == NULL)
      return;

   XPTDEBUG(("    Requested Capabilities: \n"));

   XPTDEBUG(("      Aliases           :\n"));
   dumpCharArray(capabilities->aliases, capabilities->aliasesSz);

   XPTDEBUG(("      Client SDU size   : %u\n", capabilities->clientSDUsz));

   XPTDEBUG(("      Extended Methods  :\n"));
   dumpCharArray(capabilities->extendedMethods, capabilities->extendedMethodsSz);

   XPTDEBUG(("      Header Code Pages :\n"));
   dumpCharArray(capabilities->headerCodePages, capabilities->headerCodePagesSz);

   XPTDEBUG(("      Protocol Options  :\n"));
   dumpCharArray(capabilities->protocolOptions, capabilities->protocolOptionsSz);

   XPTDEBUG(("      Server SDU size   : %u\n", capabilities->serverSDUsz));
   XPTDEBUG(("      Max Outstanding Method Requests : %u\n", capabilities->maxOutstandingMethodRequests));
   XPTDEBUG(("      Max Outstanding Push Requests   : %u\n", capabilities->maxOutstandingPushRequests));

} /* End dumpCapabilities() */

void dumpSap(SapParameters *sap) {

   if (sap == NULL)
      return;

   XPTDEBUG(("    Service Access Point Parameters: \n"));
   XPTDEBUG(("      Bearer Type    : %s\n", getBearerString(sap->bearerType)));
   XPTDEBUG(("      Address Type   : %s\n", getAddressString(sap->addressType)));
   XPTDEBUG(("      Server Address : %s\n", sap->serverAddress));
   XPTDEBUG(("      Server Port    : %hu\n", sap->serverPort));
   XPTDEBUG(("      Client Address : %s\n", sap->clientAddress));
   XPTDEBUG(("      Client Port    : %hu\n", sap->clientPort));

} /* End dumpSap() */


void dumpCharArray(char **arrayPtr, int arraySize) {
   int i = 0;
   for (; i < arraySize; i++) {
      if (i > 0)
         XPTDEBUG((", "));
      XPTDEBUG(("%s", *(arrayPtr + i)));
   } /* End for */

   XPTDEBUG(("\n"));
} /* End dumpCharArray() */

#endif

