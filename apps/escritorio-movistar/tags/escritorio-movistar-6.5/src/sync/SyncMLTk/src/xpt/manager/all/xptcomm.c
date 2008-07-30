/*************************************************************************/
/* module:          SyncML Communication Protocol selection module       */
/* file:            src/xpt/all/xptcomm.c                                */
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


/**
 * This file implements the SyncML transport protocol to invoke the
 * miscellaneous protocol drivers.  A transport protocol implementation may be
 * either statically linked or dynamically linked.  A dynamically-loaded
 * transport binding (e.g., a DLL on Windows, or a shared library on Unix) is
 * loaded by name when the xpt layer is initialized and thereafter is used just
 * like a statically-linked binding.
 */

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

//#define TRACE_TO_STDOUT
//#define TRACE

#include <xpt.h>
#include <xptTransport.h>
#include <xptcomm.h>
#include <utilities.h>
#include <xptport.h>

#ifdef __EPOC_OS__
#include "xpt_globals_epoc.h"
#endif

/***********************************************************************/
/* The following define statements determine which protocols should be */
/* statically linked with the xpt service.                             */
/***********************************************************************/
/*
#ifdef __PALM_OS__
  // If no transports are explicitly called for, compile them all in
  #if !defined(INCLUDE_HTTP_STATICALLY) && !defined(INCLUDE_OBEX_STATICALLY) \
      && !defined(INCLUDE_WSP_STATICALLY)
    #define INCLUDE_HTTP_STATICALLY
    #define INCLUDE_OBEX_STATICALLY
//  #define INCLUDE_WSP_STATICALLY
  #endif
#else
 // luz %%%: for statical link, transport are static linked, too
 #ifdef LINK_TRANSPORT_STATICALLY
  #if !defined(INCLUDE_HTTP_STATICALLY) && !defined(INCLUDE_OBEX_STATICALLY) \
      && !defined(INCLUDE_WSP_STATICALLY)
  #error No Transports included statically
  #endif
 #else
  #define INCLUDE_TRANSPORTS_DYNAMICALLY
 #endif
#endif

#define XPT_ID SML_ERR_A_XPT_ERROR  // Starting trace message number
*/
// %%% luz
#ifndef TRACE_TO_STDOUT
 #include <libutil.h>               // For smlLibPrint() prototype
#endif


// This is information that could go in the XptProtocolInfo stucture, but
// we put it in a different one to avoid defining all these internal fields
// in a public header file.
struct xptInternalTransportInfo {
   struct xptTransportDescription desc;
   int transportInstancesOpen;      // Count of the number of service id
                                    // handles open for this transport.
};


#define TransportInstanceControlBlockId  0x07020811

// An instance of this control block exists for each allocated
// XptServiceIDPtr_t handle.  In fact, such a handle is really a pointer to
// one of these control blocks.
struct TransportInstance {
   unsigned long controlBlockId;    // Helps us to verify that a handle is
                                    // really a pointer to one of these.

   void *privateInstanceInfo;       // Field of use to the transport implementation
   struct XptProtocolInfo *info;    // Pointer to info about the protocol
   int communicationHandlesOpen;    // Count of the number of communication
                                    // handles open under this transport
                                    // handle.
};


#define CommunicationInstanceControlBlockId  0x34243508

#define CI_STATE_READY_FOR_EXCHANGE       1 // Connection just opened
#define CI_STATE_EXPECTING_SET_DOC        2 // Client exchange just initiated
#define CI_STATE_EXPECTING_GET_DOC        3 // Server exchange just initiated
#define CI_STATE_SENDING                  4 // Okay to send data
#define CI_STATE_RECEIVING                5 // Okay to receive data
#define CI_STATE_EXPECTING_END_EXCHANGE   6 // Sending/receiving finished

// An instance of this control block exists for each allocated
// XptCommunicationID_t handle.  In fact, such a handle is really a pointer to
// one of these control blocks.
struct CommunicationInstance {
   unsigned long controlBlockId;    // Helps us to verify that a handle is
                                    // really a pointer to one of these.
   void *privateCommunicationInfo;  // Field of use to the transport implementation

   struct TransportInstance *transport;   // Pointer to associated transport
   int state;                       // Communication state.  We verify that
                                    // function calls are made only at times
                                    // they are valid so the transport
                                    // implementation doesn't have to.  The
                                    // value here is one of CI_STATE_*
   int role;                        // One of XPT_REQUEST_SENDER or
                                    // XPT_REQUEST_RECEIVER, indicating whether
                                    // the communication instance is in client
                                    // or server mode.
   size_t bytesRemaining;           // Number of bytes remaining to be sent,
                                    // if an outgoing document is currently
                                    // being sent, or the number of bytes
                                    // remaining to be read, if an incoming
                                    // document is currently being read, or
                                    // (size_t) -1 is the length is unknown.
};

/* Create a table containing initialization routines for statically-linked */
/* transport bindings.                                                     */

#ifdef INCLUDE_HTTP_STATICALLY
   Ret_t XPTAPI HTTP_initializeTransport(void);
#endif

#ifdef INCLUDE_OBEX_STATICALLY
   Ret_t XPTAPI obexInitializeTransport(void);
#endif

#ifdef INCLUDE_WSP_STATICALLY
   Ret_t XPTAPI wspInitializeTransport(void) WSP_SECTION;
#endif

// Ret_t XPTAPI initializeTestTransport(void);

#ifndef INCLUDE_TRANSPORTS_DYNAMICALLY
static const struct staticallyLinkedTransport {
   const char *name;
   Ret_t (XPTAPI *initFunction)(void);

} staticallyLinkedTransports[] XPT_DATA_SECTION = {

#ifdef INCLUDE_HTTP_STATICALLY
   {"HTTP", HTTP_initializeTransport},
#endif
#ifdef INCLUDE_OBEX_STATICALLY
   {"OBEX", obexInitializeTransport},
#endif
#ifdef INCLUDE_WSP_STATICALLY
   {"WSP", wspInitializeTransport},
#endif
   {NULL, NULL}      // Marker for end of address list
};

#endif
#ifndef __EPOC_OS__

static char const *noError  XPT_DATA_SECTION = "No error message provided";
static char const *noMemory XPT_DATA_SECTION = "Out of memory";

static struct XptProtocolInfo *availableTransports XPT_DATA_SECTION = NULL;
static int availableTransportCount XPT_DATA_SECTION = 0;
static int transportIdCounter XPT_DATA_SECTION = 0;

// On a multithreaded system, this information would be kept in a per-thread
// control block.
static XptErrorInformation_t lastError XPT_DATA_SECTION = {0, 0, "", "", 0, ""};

#endif
#ifdef __EPOC_OS__

#define noError TheXptGlobalsEpoc()->noError
#define noMemory TheXptGlobalsEpoc()->noMemory

#define availableTransports TheXptGlobalsEpoc()->availableTransports
#define availableTransportCount TheXptGlobalsEpoc()->availableTransportCount
#define transportIdCounter TheXptGlobalsEpoc()->transportIdCounter

#define lastError TheXptGlobalsEpoc()->lastError
#define closeBindingTLS TheXptGlobalsEpoc()->closeBindingTLS
#endif

// Prototypes for internal functions
static Ret_t initializeTransports(void) XPT_SECTION;
static struct TransportInstance *validateServiceId(XptServiceID_t id) XPT_SECTION;
static struct CommunicationInstance *validateCommunicationId(XptCommunicationID_t id) XPT_SECTION;
static Ret_t setLastErrorInfo(struct XptProtocolInfo *pi, const char *function, Ret_t rc) XPT_SECTION;


// Initialize all statically-linked transports and dynamically load any
// loadable modules implementing other transports.
static Ret_t initializeTransports(void) {
#ifndef __EPOC_OS__
   static Ret_t initializationStatus XPT_DATA_SECTION = SML_ERR_OK;
#endif
#ifdef __EPOC_OS__
#define initializationStatus TheXptGlobalsEpoc()->initializationStatus
#endif
#ifndef __EPOC_OS__
#ifndef INCLUDE_TRANSPORTS_DYNAMICALLY
   const struct staticallyLinkedTransport *transport;
#endif
#endif

   Ret_t exitStatus = SML_ERR_OK;

   // See if a previous initialization attempt failed.
   if (initializationStatus != SML_ERR_OK)
      return initializationStatus;

   // We assume single threaded, and thus no competition calling initializeTransports()

   XPTDEBUG(("Initializing transports\n"));

#ifndef INCLUDE_TRANSPORTS_DYNAMICALLY

   // First, initialize all the statically-linked transport implementations
   for (transport = staticallyLinkedTransports; transport->initFunction; ++transport) {
      Ret_t status;
      XPTDEBUG(("Initializing statically-linked transport %s\n", transport->name));
      status = transport->initFunction();
      if (status) {
         XPTDEBUG(("The initializeTransport() function of statically-linked transport %s failed with return status %d\n", transport->name, (int) status));
         exitStatus = status;
      }
   }

#endif
#ifdef INCLUDE_TRANSPORTS_DYNAMICALLY
   /* Now initialize any transports that are to be dynamically loaded         */

   // First read the names of the dynamic library files (e.g., DLLs).  This is
   // platform dependent, so call a platform dependent routine here.
   {
      char **libraries;

      libraries = getLibrariesToLoad();
      if (libraries) {
         char **p;

         // Try to load and initialize each library
         for (p=libraries; *p; ++p) {
            char *libname = *p;
            Ret_t (XPTAPI *initFunction)(void);
            Ret_t status;
            int transportsRegisteredSoFar;

            void *handle = loadLibrary(libname);
            if (!handle) continue;     // Error loading library

            initFunction = lookupLibrarySymbol("initializeTransport", 0, handle);
            if (!initFunction) {
               XPTDEBUG(("Dynamically-loaded transport library %s doesn't export an \"initializeTransport\" symbol\n", libname));
               unloadLibrary(handle);
               continue;
            }

            transportsRegisteredSoFar = availableTransportCount;

            XPTDEBUG(("Initializing dynamically-loaded transport in library %s\n", libname));
            status = initFunction();
            if (status) {
               XPTDEBUG(("The initializeTransport() function of dynamically-loaded transport in library %s failed with return status %d\n", libname, (int) status));
               exitStatus = status;
            }

            if (availableTransportCount == transportsRegisteredSoFar) {
               // The library registered no new transports.  We don't need it.
               unloadLibrary(handle);
            }
         }

         releaseLibraryList(libraries);
      }
   }
#endif

   if (!availableTransportCount)
      exitStatus = SML_ERR_A_XPT_NO_TRANSPORTS;

   initializationStatus = exitStatus;     // Save for future

   return exitStatus;
}

static struct TransportInstance *validateServiceId(XptServiceID_t id) {
   struct TransportInstance *ti = (struct TransportInstance *) id;
   if (!ti || ti->controlBlockId != TransportInstanceControlBlockId)
      return NULL;
   return ti;
}

static struct CommunicationInstance *validateCommunicationId(XptCommunicationID_t id) {
   struct CommunicationInstance *ci = (struct CommunicationInstance *) id;
   if (!ci || ci->controlBlockId != CommunicationInstanceControlBlockId)
      return NULL;
   return ci;
}

static Ret_t setLastErrorInfo(struct XptProtocolInfo *pi, const char *function, Ret_t rc) {
   // On a multithreaded system, this information would be kept in a per-thread
   // control block.
   if (pi) {
      lastError.protocolId = pi->id;
      lastError.shortProtocolName = pi->shortName;
   } else {
      lastError.protocolId = 0;
      lastError.shortProtocolName = "none";
   }
   // We don't need to make a copy of the name of the failing function, because
   // we know the caller will always provide a pointer to a static constant.
   lastError.failingFunction = function;
   lastError.status = rc;
   if (!lastError.errorMessage || !*lastError.errorMessage) {
      if (lastError.status == SML_ERR_A_XPT_MEMORY)
         lastError.errorMessage = noMemory;
      else
         lastError.errorMessage = noError;
   }

   return rc;  // This makes it easier to use setLastErrorInfo() on a return statement
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptRegisterTransport(const struct xptTransportDescription *pTransport) {
   struct xptInternalTransportInfo *iti;
   /* Validate the provided xptTransportDescription structure              */

   // First make sure the necessary pointers are non-null
   if (!pTransport->shortName || !pTransport->description ||
       !pTransport->selectProtocol || !pTransport->deselectProtocol ||
       !pTransport->openCommunication || !pTransport->closeCommunication ||
       !pTransport->beginExchange || !pTransport->endExchange ||
       !pTransport->receiveData || !pTransport->sendData ||
       !pTransport->sendComplete ||
       !pTransport->setDocumentInfo || !pTransport->getDocumentInfo)
      return setLastErrorInfo(NULL, "xptRegisterTransport", SML_ERR_WRONG_PARAM);

   // Make sure no flags other that XPT_CLIENT and XPT_SERVER are specified
   if (pTransport->flags & ~(XPT_CLIENT | XPT_SERVER))
      return setLastErrorInfo(NULL, "xptRegisterTransport", SML_ERR_WRONG_PARAM);

   // Make sure at least one of XPT_CLIENT and XPT_SERVER are specified
   if (!(pTransport->flags & (XPT_CLIENT | XPT_SERVER)))
      return setLastErrorInfo(NULL, "xptRegisterTransport", SML_ERR_WRONG_PARAM);

   iti = xppMalloc(sizeof(struct xptInternalTransportInfo));
   if (!iti)
      return setLastErrorInfo(NULL, "xptRegisterTransport", SML_ERR_A_XPT_MEMORY);

   iti->desc.shortName   = xppMalloc(xppStrlen(pTransport->shortName)+1);

   iti->desc.description = xppMalloc(xppStrlen(pTransport->description)+1);

   if (!iti->desc.shortName || !iti->desc.description) {
      if (iti->desc.shortName)   xppFree((void *) iti->desc.shortName);
      if (iti->desc.description) xppFree((void *) iti->desc.description);
      xppFree(iti);
      return setLastErrorInfo(NULL, "xptRegisterTransport", SML_ERR_A_XPT_MEMORY);
   }

   // Copy the short name
   xppStrcpy((char *) iti->desc.shortName, pTransport->shortName);

   // Copy the description
   xppStrcpy((char *) iti->desc.description, pTransport->description);

   // Copy the flags and other fields
   iti->desc.flags = pTransport->flags;
   iti->desc.privateTransportInfo = pTransport->privateTransportInfo;

   // Copy the callback function pointers
   iti->desc.selectProtocol      = pTransport->selectProtocol;
   iti->desc.deselectProtocol    = pTransport->deselectProtocol;
   iti->desc.openCommunication   = pTransport->openCommunication;
   iti->desc.closeCommunication  = pTransport->closeCommunication;
   iti->desc.beginExchange       = pTransport->beginExchange;
   iti->desc.endExchange         = pTransport->endExchange;
   iti->desc.receiveData         = pTransport->receiveData;
   iti->desc.sendData            = pTransport->sendData;
   iti->desc.sendComplete        = pTransport->sendComplete;
   iti->desc.setDocumentInfo     = pTransport->setDocumentInfo;
   iti->desc.getDocumentInfo     = pTransport->getDocumentInfo;


   // LEO:
   iti->desc.cancelCommAsync	 = pTransport->cancelCommAsync;



#ifdef __EPOC_OS__
   closeBindingTLS               = pTransport->resetBindingTLS;
#endif
   // Initialize count of open instance to zero
   iti->transportInstancesOpen = 0;

   // Now create the public version of the transport description structure
   availableTransports = xppRealloc(availableTransports,
               (availableTransportCount + 1) * sizeof(struct XptProtocolInfo));
   if (!availableTransports) {
      availableTransportCount = 0;  // We just lost all transport info
      return setLastErrorInfo(NULL, "xptRegisterTransport", SML_ERR_A_XPT_MEMORY);
                                    // This would be very bad
   }

   // Fill in the public version of the structure.  It shares the shortName
   // and description string with the internal structure.
   availableTransports[availableTransportCount].internal = iti;
   availableTransports[availableTransportCount].shortName = iti->desc.shortName;
   availableTransports[availableTransportCount].description = iti->desc.description;
   availableTransports[availableTransportCount].flags = iti->desc.flags;
   availableTransports[availableTransportCount].id = ++transportIdCounter;

   // Make the new element an official part of the list
   ++availableTransportCount;

   XPTDEBUG(("Registered new transport \"%s\": %s\n", iti->desc.shortName, iti->desc.description));

   return SML_ERR_OK;
}



XPTEXP1 Ret_t XPTAPI XPTEXP2 xptSetLastError(const struct xptTransportErrorInformation *info) {
   // On a multithreaded system, this information would be kept in a per-thread
   // control block.
   if (!info->errorMessage) return SML_ERR_WRONG_PARAM;

   lastError.protocolErrorCode = info->protocolErrorCode;

   if (lastError.errorMessage && *lastError.errorMessage &&
       lastError.errorMessage != noError && lastError.errorMessage != noMemory)
      xppFree((void *) lastError.errorMessage);

   if (info->errorMessage) {
      lastError.errorMessage = xppMalloc(xppStrlen(info->errorMessage) + 1);
      if (!lastError.errorMessage) {
         lastError.errorMessage = noMemory;
         return SML_ERR_A_XPT_MEMORY;
      }

      // Copy the error message, so caller doesn't need to save it.
      xppStrcpy((char *) lastError.errorMessage, info->errorMessage);
   }

   return SML_ERR_OK;
}


XPTEXP1 const char * XPTAPI XPTEXP2 xptGetMetaInfoValue(const char *metaInformation,
                                        const char *tag, size_t *valueLen) {
#define DELIMITERS   " \t"

   const char *p, *vs, *ve;
   int found = 0;

   p = metaInformation;
   do {
      const char *ts, *te;

      // Skip over delimiters (the strspn() function would be handy here)
      for (ts=p; *ts && xppStrchr(DELIMITERS, *ts); ++ts) ;
      if (!*ts) break;           // We've hit the end of the string

      // The variable p now points to the start of a tag name.  Find the end.
      // The strpbrk() function would be handy here...
      for (te=ts; *te && !xppStrchr(DELIMITERS "=", *te); ++te) ;

      // The variable end now points just beyond the last char of the tag name.
      // Find the beginning and end of the value (if any).
      if (*te != '=') {
         // If the keyword ends with anything but '=', there's no value.
         p = vs = ve = te;
      } else {
         vs = te + 1;
         // Tolerate a quoted (single or double) string as well for the value
         if (*vs == '"' || *vs == '\'') {
            int q = *vs;
            ve = xppStrchr(++vs, q);
            if (ve) {
               p = ve + 1;
            } else {
               // Missing end quote is terminated by end of string
               ve = xppStrchr(vs, '\0');
               p = ve;
            }
         } else {
            // No quotes.  Find next delimiter (end of word)
            for (ve=vs; *ve && !xppStrchr(DELIMITERS, *ve); ++ve) ;
            p = ve;              // For next time through the loop
         }
      }

      // See if this is the tag we care about
      found = te-ts == (int) xppStrlen(tag)  &&  !xppMemicmp(tag, ts, te-ts);
   } while (!found && *p);

   if (!found) return NULL;

   *valueLen = ve-vs;
   return vs;
}
#undef DELIMITERS


XPTEXP1 Ret_t XPTAPI XPTEXP2 xptGetProtocol(const char *protocolName,
                                            const struct XptProtocolInfo **pProtocolInfo) {
   const struct XptProtocolInfo *protocolList;
   int i, protocolCount;
   Ret_t rc;

   rc = xptGetProtocols(&protocolList, &protocolCount);
   if (rc != SML_ERR_OK) return rc;

   for (i = 0; i < protocolCount; ++i) {
      if (!xppStrcmp(protocolList[i].shortName, protocolName)) {
         *pProtocolInfo = protocolList + i;
         return SML_ERR_OK;
      }
   }

   *pProtocolInfo = NULL;
   return SML_ERR_A_XPT_INVALID_PROTOCOL;
}


#ifndef __EPOC_OS__
   static int initialized XPT_DATA_SECTION = 0;
#endif
#ifdef __EPOC_OS__
#define initialized TheXptGlobalsEpoc()->initialized
#endif

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptGetProtocols(const struct XptProtocolInfo **pProtocolList,
                              int *pProtocolCount) {
   if (!initialized) {
      Ret_t rc = initializeTransports();

      // If initialization fails, xptGetProtocols() also fails
      if (rc != SML_ERR_OK)
         return setLastErrorInfo(NULL, "xptGetProtocols", rc);

      initialized = 1;     // Initialization completed successfully
   }

   *pProtocolList = availableTransports;
   *pProtocolCount = availableTransportCount;

   return SML_ERR_OK;
}

// Clean up by unregistering transports (we'll have memory leaks otherwise!)
// %%% luz 2003-06-26
void xptCleanUp(void)
{
  int idx;
  struct xptInternalTransportInfo *iti;

  // remove internal transport structures  
  for (idx=0; idx<availableTransportCount; idx++) {
    iti = availableTransports[idx].internal;
    // Delete the strings
    xppFree((void *) iti->desc.shortName);
    xppFree((void *) iti->desc.description);
    // delete the internal struct itself
    xppFree(iti);
  }
  // remove transports table itself
  if (availableTransports) xppFree(availableTransports);
  // no transports available any more
  availableTransportCount=0;
  transportIdCounter=0;
  availableTransports=NULL;
  initialized=0; // not initialized any more

  // LEO:
   if (lastError.errorMessage && *lastError.errorMessage &&
       lastError.errorMessage != noError && lastError.errorMessage != noMemory)
   {
      xppFree((void *) lastError.errorMessage);
	  lastError.errorMessage = NULL;
   }

}


XPTEXP1 Ret_t XPTAPI XPTEXP2 xptSelectProtocol (XptProtocolId_t protocolId,
                                const char *metaInformation,
                                unsigned int flags,
                                XptServiceIDPtr_t pId) {
   struct XptProtocolInfo *pi;
   struct TransportInstance *ti;
   void *psi;
   Ret_t rc;

   // Find the transport with the specified protocol id
   for (pi = availableTransports; pi && pi->id != protocolId; ++pi) ;

   // It's an error if an unknown protocol id is specified
   if (!pi)
      return setLastErrorInfo(pi, "xptSelectProtocol", SML_ERR_WRONG_PARAM);

   XPTDEBUG(("Function xptSelectProtocol() entered, selecting transport \"%s\"\n", pi->shortName));

   // Validate flags.
   if (flags != XPT_CLIENT && flags != XPT_SERVER)
      return setLastErrorInfo(pi, "xptSelectProtocol", SML_ERR_WRONG_PARAM);

   // Make sure protocol supports the requested mode
   if (!(pi->flags & flags))
      return setLastErrorInfo(pi, "xptSelectProtocol", SML_ERR_A_XPT_INVALID_PROTOCOL);

   // Call through to the transport's implementation
   rc = pi->internal->desc.selectProtocol(pi->internal->desc.privateTransportInfo,
                                                metaInformation, flags, &psi);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's selectProtocol() returned error %d\n", (int) rc));
      return setLastErrorInfo(pi, "xptSelectProtocol", rc);
   }

   // Allocate a transport instance control block
   ti = xppMalloc(sizeof(struct TransportInstance));
   if (!ti) {
      // Tell the transport that we won't need the instance after all
      pi->internal->desc.deselectProtocol(psi);

      return setLastErrorInfo(pi, "xptSelectProtocol", SML_ERR_A_XPT_MEMORY);
   }

   // Initialize the transport instance control block
   ti->controlBlockId = TransportInstanceControlBlockId;
   ti->privateInstanceInfo = psi;
   ti->info = pi;
   ti->communicationHandlesOpen = 0;

   *pId = (void *) ti;

   ++pi->internal->transportInstancesOpen;

   XPTDEBUG(("Returning successfully from xptSelectProtocol() with new handle == %lx, Open instances == %d\n", (unsigned long) ti, pi->internal->transportInstancesOpen));

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptDeselectProtocol(XptServiceID_t id) {

   Ret_t rc;
   struct XptProtocolInfo *pi;

   struct TransportInstance *ti = validateServiceId(id);
   if (!ti)
      return setLastErrorInfo(NULL, "xptDeselectProtocol", SML_ERR_A_XPT_INVALID_ID);


   // We'll need this later after we free the TransportInstance control block
   pi = ti->info;

   XPTDEBUG(("Function xptDeselectProtocol() entered, deselecting handle %lx for transport \"%s\"\n", (unsigned long) ti, pi->shortName));

   // All open communication instances must be closed
   if (ti->communicationHandlesOpen)
      return setLastErrorInfo(pi, "xptDeselectProtocol", SML_ERR_A_XPT_IN_USE);

   // Call through to the transport's implementation
   rc = pi->internal->desc.deselectProtocol(ti->privateInstanceInfo);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's deselectProtocol() returned error %d\n", (int) rc));
      return setLastErrorInfo(ti->info, "xptDeselectProtocol", rc);
   }

   // Ensure that a later reference to this block fails to validate
   ti->controlBlockId = 0;

   // Release the storage
   xppFree(ti);

   --pi->internal->transportInstancesOpen;

   XPTDEBUG(("Returning successfully from xptDeselectProtocol().  Open instances == %d\n", pi->internal->transportInstancesOpen));

   return rc;
}

// LEO:

XPTEXP1 Ret_t XPTAPI xptCancelCommunicationAsync(XptServiceID_t id, XptCommunicationID_t conn) 
{
   Ret_t rc;
   struct TransportInstance *ti = validateServiceId(id);
   struct CommunicationInstance *ci = validateCommunicationId(conn);

   if (!ti)
      return setLastErrorInfo(NULL, "xptCancelCommunicationAsync", SML_ERR_A_XPT_INVALID_ID);

   if (!ci)
      return setLastErrorInfo(NULL, "xptCancelCommunicationAsync", SML_ERR_A_XPT_INVALID_ID);

   rc = ti->info->internal->desc.cancelCommAsync(ci->privateCommunicationInfo);

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptOpenCommunication(XptServiceID_t id,
                                   int role,
                                   XptCommunicationIDPtr_t pConn) {
   Ret_t rc;

   void *pci;
   struct CommunicationInstance *ci;
   struct TransportInstance *ti = validateServiceId(id);
   if (!ti)
      return setLastErrorInfo(NULL, "xptOpenCommunication", SML_ERR_A_XPT_INVALID_ID);

   // Validate role
   if (role != XPT_REQUEST_SENDER && role != XPT_REQUEST_RECEIVER)
      return setLastErrorInfo(ti->info, "xptOpenCommunication", SML_ERR_WRONG_PARAM);

   XPTDEBUG(("Function xptOpenCommunication() entered, as %s\n", role == XPT_REQUEST_SENDER ? "sender" : "receiver\n"));

   // Call through to the transport's implementation
   rc = ti->info->internal->desc.openCommunication(ti->privateInstanceInfo, role, &pci);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's openCommunication() returned error %d\n", (int) rc));
      return setLastErrorInfo(ti->info, "xptOpenCommunication", rc);
   }

   // Allocate a communication instance control block
   ci = xppMalloc(sizeof(struct CommunicationInstance));
   if (!ci) {
      // Tell the communication that we won't need the instance after all
      ti->info->internal->desc.closeCommunication(pci);

      return setLastErrorInfo(ti->info, "xptOpenCommunication", SML_ERR_A_XPT_MEMORY);
   }

   // Initialize the communication instance control block
   ci->controlBlockId = CommunicationInstanceControlBlockId;
   ci->privateCommunicationInfo = pci;
   ci->transport = ti;
   ci->state = CI_STATE_READY_FOR_EXCHANGE;
   ci->role = role;

   *pConn = (void *) ci;

   ++ti->communicationHandlesOpen;

   XPTDEBUG(("Returning successfully from xptOpenCommunication() with new handle == %lx.  Open instances == %d\n", (unsigned long) ci, ti->communicationHandlesOpen));

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptCloseCommunication(XptCommunicationID_t conn) {
   Ret_t rc;

   struct TransportInstance *ti;
   struct CommunicationInstance *ci = validateCommunicationId(conn);
   if (!ci)
      return setLastErrorInfo(NULL, "xptCloseCommunication", SML_ERR_A_XPT_INVALID_ID);

   // We'll need this later after we free the CommunicationInstance control block
   ti = ci->transport;

   XPTDEBUG(("Function xptCloseCommunication() entered, closing handle %lx\n", (unsigned long) ci));

   // Call through to the transport's implementation
   rc = ti->info->internal->desc.closeCommunication(ci->privateCommunicationInfo);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's closeCommunication() returned error %d\n", (int) rc));
      return setLastErrorInfo(ti->info, "xptCloseCommunication", rc);
   }

   // Ensure that a later reference to this block fails to validate
   ci->controlBlockId = 0;

   // Release the storage
   xppFree(ci);

   --ti->communicationHandlesOpen;

   XPTDEBUG(("Returning successfully from xptCloseCommunication().  Open instances == %d\n", ti->communicationHandlesOpen));

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptBeginExchange(XptCommunicationID_t conn) {
   Ret_t rc;

   struct CommunicationInstance *ci = validateCommunicationId(conn);
   if (!ci)
      return setLastErrorInfo(NULL, "xptBeginExchange", SML_ERR_A_XPT_INVALID_ID);

   XPTDEBUG(("Function xptBeginExchange() entered for handle %lx\n", (unsigned long) ci));

   // Ensure the communication instance is in the proper state for this call
   if (ci->state != CI_STATE_READY_FOR_EXCHANGE)
      return setLastErrorInfo(ci->transport->info, "xptBeginExchange",
                              SML_ERR_A_XPT_INVALID_STATE);

   // Call through to the transport's implementation
   rc = ci->transport->info->internal->desc.beginExchange(ci->privateCommunicationInfo);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's beginExchange() returned error %d\n", (int) rc));
      return setLastErrorInfo(ci->transport->info, "xptBeginExchange", rc);
   }

   // Establish the new state
   ci->state = ci->role == XPT_REQUEST_SENDER ? CI_STATE_EXPECTING_SET_DOC :
                                        CI_STATE_EXPECTING_GET_DOC;

   XPTDEBUG(("Returning successfully from xptBeginExchange()\n"));

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptEndExchange(XptCommunicationID_t conn) {
   Ret_t rc;

   struct CommunicationInstance *ci = validateCommunicationId(conn);
   if (!ci)
      return setLastErrorInfo(NULL, "xptEndExchange", SML_ERR_A_XPT_INVALID_ID);

   XPTDEBUG(("Function xptEndExchange() entered for handle %lx\n", (unsigned long) ci));

   // Ensure the communication instance is in the proper state for this call
   if (ci->state != CI_STATE_EXPECTING_END_EXCHANGE)
      return setLastErrorInfo(ci->transport->info, "xptEndExchange",
                              SML_ERR_A_XPT_INVALID_STATE);

   // Call through to the transport's implementation
   rc = ci->transport->info->internal->desc.endExchange(ci->privateCommunicationInfo);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's endExchange() returned error %d\n", (int) rc));
      return setLastErrorInfo(ci->transport->info, "xptEndExchange", rc);
   }

   // Establish the new state
   ci->state = CI_STATE_READY_FOR_EXCHANGE;

   XPTDEBUG(("Returning successfully from xptEndExchange()\n"));

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptReceiveData(XptCommunicationID_t connection,
                             void *buffer, size_t bufferLen,
                             size_t *dataLen) {
   Ret_t rc;

   struct CommunicationInstance *ci = validateCommunicationId(connection);
   if (!ci)
      return setLastErrorInfo(NULL, "xptReceiveData", SML_ERR_A_XPT_INVALID_ID);

   XPTDEBUG(("Function xptReceiveData() entered for handle %lx, reading %ld bytes\n", (unsigned long) ci, (long) bufferLen));

   // Ensure the communication instance is in the proper state for this call
   if (ci->state != CI_STATE_RECEIVING)
      return setLastErrorInfo(ci->transport->info, "xptReceiveData",
                              SML_ERR_A_XPT_INVALID_STATE);

   if (!bufferLen)
      return setLastErrorInfo(ci->transport->info, "xptReceiveData",
                              SML_ERR_WRONG_PARAM);

   // Make sure the application isn't trying to read beyond the end of the
   // document.
   if (bufferLen > ci->bytesRemaining) {  // false if bytesRemaining == (size_t) -1
      bufferLen = ci->bytesRemaining;
      if (!bufferLen) {       // Application has read all the data
         *dataLen = 0;        // Generate an end-of-file condition
         // Establish the new state
         ci->state = ci->role == XPT_REQUEST_SENDER ? CI_STATE_EXPECTING_END_EXCHANGE :
                                              CI_STATE_EXPECTING_SET_DOC;
         return SML_ERR_OK;
      }
   }

   // Call through to the transport's implementation
   rc = ci->transport->info->internal->desc.receiveData(ci->privateCommunicationInfo,
                                                        buffer, bufferLen, dataLen);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's receiveData() returned error %d\n", (int) rc));
      return setLastErrorInfo(ci->transport->info, "xptReceiveData", rc);
   }

   // Account for the bytes provided by the transport
   if (ci->bytesRemaining != (size_t) -1) {
      ci->bytesRemaining -= *dataLen;
      if (ci->bytesRemaining && !*dataLen) {
         XPTDEBUG(("Transport returned incomplete document.  %lu more bytes expected.\n", (unsigned long) ci->bytesRemaining));
         // Let the application deal with it
      }
   }

   // Establish the new state
   if (ci->bytesRemaining == 0)
      ci->state = ci->role == XPT_REQUEST_SENDER ? CI_STATE_EXPECTING_END_EXCHANGE :
                                           CI_STATE_EXPECTING_SET_DOC;

   XPTDEBUG(("Returning successfully from xptReceiveData(), having read %ld bytes.\n", (long) *dataLen));

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptSendData(XptCommunicationID_t connection,
                                         const void *buffer, size_t bufferLen,
                                         size_t *bytesSent) {
   Ret_t rc;

   struct CommunicationInstance *ci = validateCommunicationId(connection);
   if (!ci)
      return setLastErrorInfo(NULL, "xptSendData", SML_ERR_A_XPT_INVALID_ID);

   XPTDEBUG(("Function xptSendData() entered for handle %lx, sending %ld bytes.\n", (unsigned long) ci, (long) bufferLen));

   // Ensure the communication instance is in the proper state for this call
   if (ci->state != CI_STATE_SENDING)
      return setLastErrorInfo(ci->transport->info, "xptSendData",
                              SML_ERR_A_XPT_INVALID_STATE);

   // Make sure the application isn't trying to write more data than is in the
   // document.
   if (bufferLen > ci->bytesRemaining)    // false if bytesRemaining == (size_t) -1
      return setLastErrorInfo(ci->transport->info, "xptSendData",
                              SML_ERR_WRONG_PARAM);

   // Call through to the transport's implementation
   rc = ci->transport->info->internal->desc.sendData(ci->privateCommunicationInfo,
                                                buffer, bufferLen, bytesSent);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's sendData() returned error %d\n", (int) rc));
      return setLastErrorInfo(ci->transport->info, "xptSendData", rc);
   }

   // Account for the bytes absorbed by the transport
   if (ci->bytesRemaining != (size_t) -1)
      ci->bytesRemaining -= *bytesSent;

   XPTDEBUG(("Returning successfully from xptSendData(), "
             "having written %ld bytes.\n", (long) *bytesSent));

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptSendComplete(XptCommunicationID_t connection) {
   Ret_t rc;

   struct CommunicationInstance *ci = validateCommunicationId(connection);
   if (!ci)
      return setLastErrorInfo(NULL, "xptSendComplete", SML_ERR_A_XPT_INVALID_ID);

   XPTDEBUG(("Function xptSendComplete() entered for handle %lx.\n", (unsigned long) ci));

   // Ensure the communication instance is in the proper state for this call.
   // Make sure the application wrote the exact number of expected bytes, if
   // it specified the number of bytes up front.
   if (ci->state != CI_STATE_SENDING ||
       (ci->bytesRemaining != (size_t) -1 && ci->bytesRemaining))
      return setLastErrorInfo(ci->transport->info, "xptSendComplete",
                              SML_ERR_A_XPT_INVALID_STATE);

   // Establish the new state
   ci->state = ci->role == XPT_REQUEST_SENDER ? CI_STATE_EXPECTING_GET_DOC :
                                        CI_STATE_EXPECTING_END_EXCHANGE;

   // Call through to the transport's implementation
   rc = ci->transport->info->internal->desc.sendComplete(ci->privateCommunicationInfo);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's sendComplete() returned error %d\n", (int) rc));
      return setLastErrorInfo(ci->transport->info, "xptSendComplete", rc);
   }

   XPTDEBUG(("Returning successfully from xptSendComplete()\n"));

   return rc;
}

XPTEXP1 Ret_t XPTAPI XPTEXP2 xptSetDocumentInfo(XptCommunicationID_t conn,
                                const XptCommunicationInfo_t *pDoc) {
   Ret_t rc;

   struct CommunicationInstance *ci = validateCommunicationId(conn);
   if (!ci)
      return setLastErrorInfo(NULL, "xptSetDocumentInfo", SML_ERR_A_XPT_INVALID_ID);

   XPTDEBUG(("Function xptSetDocumentInfo() entered for handle %lx\n", (unsigned long) ci));

   // Ensure the communication instance is in the proper state for this call
   if (ci->state != CI_STATE_EXPECTING_SET_DOC)
      return setLastErrorInfo(ci->transport->info, "xptSetDocumentInfo",
                              SML_ERR_A_XPT_INVALID_STATE);

   // Call through to the transport's implementation
   rc = ci->transport->info->internal->desc.setDocumentInfo(ci->privateCommunicationInfo,
                                                            pDoc);

   // If the call failed, fill in the last-error information
   if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's setDocumentInfo() returned error %d\n", (int) rc));
      return setLastErrorInfo(ci->transport->info, "xptSetDocumentInfo", rc);
   }

   // Establish the new state
   ci->state = CI_STATE_SENDING;
   ci->bytesRemaining = pDoc->cbLength;

   XPTDEBUG(("Returning successfully from xptSetDocumentInfo()\n"));

   return rc;
}


XPTEXP1 Ret_t XPTAPI XPTEXP2 xptGetDocumentInfo(XptCommunicationID_t conn,
                                XptCommunicationInfo_t *pDoc) {
   Ret_t rc;

   struct CommunicationInstance *ci = validateCommunicationId(conn);
   if (!ci)
      return setLastErrorInfo(NULL, "xptGetDocumentInfo", SML_ERR_A_XPT_INVALID_ID);

   XPTDEBUG(("Function xptGetDocumentInfo() entered for handle %lx\n", (unsigned long) ci));

   // Ensure the communication instance is in the proper state for this call
   if (ci->state != CI_STATE_EXPECTING_GET_DOC)
      return setLastErrorInfo(ci->transport->info, "xptGetDocumentInfo",
                              SML_ERR_A_XPT_INVALID_STATE);

   // Call through to the transport's implementation
   rc = ci->transport->info->internal->desc.getDocumentInfo(ci->privateCommunicationInfo,
                                                            pDoc);

   // %%% luz:2002-05-28: added handling server access denied
   if (rc == SML_ERR_A_XPT_ACCESS_DENIED) {
      XPTDEBUG(("Transport's getDocumentInfo() returned access denied error %x\n", (int) rc));
      setLastErrorInfo(ci->transport->info, "xptGetDocumentInfo", rc);
      // but we will return with data
   }
   // %%% end luz
   // If the call failed, fill in the last-error information
   else if (rc != SML_ERR_OK) {
      XPTDEBUG(("Transport's getDocumentInfo() returned error %x\n", (int) rc));
      return setLastErrorInfo(ci->transport->info, "xptGetDocumentInfo", rc);
   }

   // Establish the new state
   ci->state = CI_STATE_RECEIVING;
   ci->bytesRemaining = pDoc->cbLength;

   XPTDEBUG(("Returning successfully from xptGetDocumentInfo(), rc=%x\n",rc));

   return rc;
}

XPTEXP1 const XptErrorInformation_t * XPTAPI XPTEXP2 xptGetLastError(void) {
   // On a multithreaded system, this information would be kept in a per-thread
   // control block.
   return &lastError;
}


XPTEXP1 void XPTAPI XPTEXP2 xptDebug(const char *format, ...) {
   va_list args;

   va_start(args, format);

#ifdef TRACE_TO_STDOUT
   // Send to the platform local convenient output location.  On most
   // platforms, this is stdout.
   localOutput(format, args);

#else
   // We should direct the output to the normal toolkit trace output location.
   smlLibPrint("TRACE-XPT: ");   // Print a unique intro for this msg

   smlLibVprintf(format, args);
#endif

   va_end(args);
}
