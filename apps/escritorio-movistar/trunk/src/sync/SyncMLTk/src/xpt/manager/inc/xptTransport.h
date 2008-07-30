/*****************************************************************************/
/* module:         SyncML Communication Protocol Transport Binding header    */
/* file:           src/xpt/inc/xptTransport.h                                */
/* target system:  all                                                       */
/* target OS:      all                                                       */
/*****************************************************************************/


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
 * This include file contains the API specifications for pluggable transport
 * bindings for the SyncML Communication Services.
 */

#ifndef XPTTRANSPORT_H
#define XPTTRANSPORT_H

#include <xptdef.h>
#include <xpt.h>
#include <smldef.h>


#define XPT_ID SML_ERR_A_XPT_ERROR  // Starting trace message number

#ifndef TRACE_TO_STDOUT
 #include <libutil.h>               // For smlLibPrint() prototype
#endif

/*---------------------------------------------------------------------*/
/* 1.st exclude everything */
// %%% luz 2002-04-16: moved defines and undefs to below

/* And now lets define what we really whant. */
/* Currently only Windows (32-Bit) and Linux 
 * allow for dynamic loading. 
 * Dynamicbinding requires a file 'dynamicTransports' beeing
 * present which lists all DLL's to load (without the DLL suffix).
 * Empty lines or lines starting with a hash mark '#' are ignored.
 * Insert one transport dll per line only.
 */
// %%% luz 2002-04-16: added checking LINK_TRANSPORT_STATICALLY to allow for
//     statically linked transports even on Win/Linux
#if (defined (WIN32) || defined(linux)) && !defined(LINK_TRANSPORT_STATICALLY)
    #define INCLUDE_TRANSPORTS_DYNAMICALLY
    // %%% luz 2002-04-16: moved undefs here to allow INCLUDE_xxx predefs outside
    //     this file
    #undef INCLUDE_HTTP_STATICALLY
    #undef INCLUDE_OBEX_STATICALLY
    #undef INCLUDE_WSP_STATICALLY
#else
    #undef INCLUDE_TRANSPORTS_DYNAMICALLY
    // %%% luz 2002-04-16: disabled static defines, must be predefined
    /*    
    #define INCLUDE_HTTP_STATICALLY
    #define INCLUDE_OBEX_STATICALLY
    #define INCLUDE_WSP_STATICALLY
    */
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct xptTransportDescription {
   const char *shortName;        // Short name of protocol.  E.g., "HTTP" or "OBEX/IR"
   const char *description;      // Summary of protocol, suitable for display
                                 // E.g., "OBEX over infrared"
   unsigned int flags;           // Flags.  Any of XPT_CLIENT and/or XPT_SERVER
   void *privateTransportInfo;   // Field of use to the transport binding.
      // The only use the xpt service makes of this field is to pass it on
      // subsequent calls to functions of the transport binding to identify the
      // transport binding.  It is expected that the transport binding will set
      // this field to the address of a structure containing internal transport
      // binding information.


   // Each of the following function pointers are functions that must be
   // implemented by the transport binding.  In general, they correspond to
   // public function definitions in xpt.h.  The transport-specific function
   // is called by the xpt service when the public xpt function is called by
   // the application.  Since the public functions are described in xpt.h, the
   // arguments which are simply passed through from those calls are not
   // described again here.

   /**
    * FUNCTION: Called when xptSelectProtocol() is called, selecting this
    *           transport binding.
    *
    *  Select the communication protocol to start the communication
    *
    * IN: privateTransportInfo, pointer to the transport binding's private
    *         information about the transport, the value given in the
    *         privateTransportInfo field of the xptTransportDescription struct
    *         when the transport binding was registered.
    *
    *     metaInformation, passed directly from the xptSelectProtocol() call.
    *         Beware that a null pointer may have been specified to indicate
    *         that no meta information was provided.
    *
    *     flags, passed directly from the xptSelectProtocol() call.
    *
    * OUT: *pPrivateServiceInfo, pointer to the transport binding's private
    *         information for the newly allocated service instance.  This
    *         pointer will be passed on subsequent calls to the transport's
    *         functions to identify this service instance to the transport.
    */
   Ret_t (XPTAPI *selectProtocol)(void *privateTransportInfo,
                                  const char *metaInformation,
                                  unsigned int flags,
                                  void **pPrivateServiceInfo);


   /**
    * FUNCTION: Called when xptDeselectProtocol() is called, deselecting this
    *           transport binding.
    *
    *  Stop a communication service instance.
    *
    * IN: privateServiceInfo, pointer to the transport binding's private
    *         information about the service instance.  This is the same value
    *         that was returned by the "selectProtocol" function when the
    *         service instance was created.
    */
   Ret_t (XPTAPI *deselectProtocol)(void *privateServiceInfo);


   /**
    * FUNCTION: Called when xptOpenCommunication() is called, creating a
    *           connection instance.
    *
    *  Create a connection instance for the given service id.
    *
    * IN: privateServiceInfo, pointer to the transport binding's private
    *         information about the service instance.  This is the same value
    *         that was returned by the "selectProtocol" function when the
    *         service instance was created.
    *
    *     role, passed directly from the xptOpenCommunication() call.
    *
    * OUT: *pPrivateConnectionInfo, pointer to the transport binding's private
    *         information for the newly allocated connection instance.  This
    *         pointer will be passed on subsequent calls to the transport's
    *         functions to identify this connection instance to the transport.
    */
   Ret_t (XPTAPI *openCommunication)(void *privateServiceInfo,
                                     int role,
                                     void **pPrivateConnectionInfo);

   /**
    * FUNCTION: Called when xptCloseCommunication() is called, closing a
    *           connection instance.
    *
    *  Close a connection instance.
    *
    * IN: privateConnectionInfo, pointer to the transport binding's private
    *         information about the connection instance.  This is the same
    *         value that was returned by the "openCommunication" function when
    *         the connection instance was created.
    */
   Ret_t (XPTAPI *closeCommunication)(void *privateConnectionInfo);


   /**
    * FUNCTION: Called when xptBeginExchange() is called
    *
    *  Prepare for a document exchange.
    *
    * IN: privateConnectionInfo, pointer to the transport binding's private
    *         information about the connection instance.  This is the same
    *         value that was returned by the "openCommunication" function when
    *         the connection instance was created.
    */
   Ret_t (XPTAPI *beginExchange)(void *privateConnectionInfo);


   /**
    * FUNCTION: Called when xptEndExchange() is called
    *
    *  Clean up after a document exchange.
    *
    * IN: privateConnectionInfo, pointer to the transport binding's private
    *         information about the connection instance.  This is the same
    *         value that was returned by the "openCommunication" function when
    *         the connection instance was created.
    */
   Ret_t (XPTAPI *endExchange)(void *privateConnectionInfo);

   /**
    * FUNCTION: Called when xptReceiveData() is called
    *
    *  Read data
    *
    * IN: privateConnectionInfo, pointer to the transport binding's private
    *         information about the connection instance.  This is the same
    *         value that was returned by the "openCommunication" function when
    *         the connection instance was created.
    *
    *     buffer, passed directly from the xptReceiveData() call.
    *
    *     bufferLen, passed directly from the xptReceiveData() call.
    *
    * OUT:
    *     dataLen, passed directly from the xptReceiveData() call.
    *
    * NOTES:
    *
    *  If the transport provided a document length in the earlier call to
    *  xptGetDocumentInfo(), then the xpt interface layer will ensure that the
    *  application does not attempt to read more bytes than are available in
    *  the document.  A transport implementation that always returns the length
    *  does not, therefore, need to return any sort of "end of transmission"
    *  indication.
    *
    *  On the other hand, if the transport didn't provide a length in the
    *  earlier call to xptGetDocumentInfo(), then the xpt interface layer will
    *  simply pass the xptReceiveData() calls through to the transport, and the
    *  transport is responsible for indicating end-of-document by returning a
    *  dataLen of zero.  The xpt interface layer will ensure no subsequent
    *  calls to the receiveData function are made after the end of the document
    *  is reached.
    */
   Ret_t (XPTAPI *receiveData)(void *privateConnectionInfo,
                               void *buffer, size_t bufferLen,
                               size_t *dataLen);

   /**
    * FUNCTION: Called when xptSendData() is called
    *
    *  Send data
    *
    * IN: privateConnectionInfo, pointer to the transport binding's private
    *         information about the connection instance.  This is the same
    *         value that was returned by the "openCommunication" function when
    *         the connection instance was created.
    *
    *     buffer, passed directly from the xptSendData() call.
    *
    *     bufferLen, passed directly from the xptSendData() call.
    *
    *     bytesSent, passed directly from the xptSendData() call.
    *
    * NOTES:
    *
    *  If the application provided the document length in its earlier call to
    *  xptSetDocumentInfo(), the xpt interface layer will ensure that the
    *  application does not attempt to write more bytes than it said were in
    *  the document.  In this case, therefore, the transport implementation
    *  does not need to check for that overflow condition.
    *
    *  If the application did not provide the document length in its earlier
    *  call to xptSetDocumentInfo(), the xpt interface layer will simply pass
    *  through xptSendData() calls, and the transport is responsible for
    *  absorbing the document body until the application calls
    *  xptSendComplete() to indicate the end of the document.
    */
   Ret_t (XPTAPI *sendData)(void *privateConnectionInfo,
                            const void *buffer, size_t bufferLen,
                            size_t *bytesSent);

   /**
    * FUNCTION: Called when xptSendComplete() is called by the application,
    *           after sending the last byte of the document.
    *
    *  Complete send processing for an outgoing document.
    *
    * IN: privateConnectionInfo, pointer to the transport binding's private
    *         information about the connection instance.  This is the same
    *         value that was returned by the "openCommunication" function when
    *         the connection instance was created.
    *
    * NOTES:
    *
    *  This call gives the transport a chance to complete send processing for
    *  a document.  If it has not already been sent by the transport, the
    *  document should be physically sent at this time.
    */
   Ret_t (XPTAPI *sendComplete)(void *privateConnectionInfo);

   /**
    * FUNCTION: Called when xptSetDocumentInfo() is called
    *
    *  Provide document information for an outgoing document.
    *
    * IN: privateConnectionInfo, pointer to the transport binding's private
    *         information about the connection instance.  This is the same
    *         value that was returned by the "openCommunication" function when
    *         the connection instance was created.
    *
    *     pDoc, passed directly from the xptSetDocumentInfo() call.
    *
    * NOTES:
    *
    *  WARNING:  If the application doesn't know the length of the document
    *  body in advance, it is allowed to set the cbLength field of the
    *  XptCommunicationInfo_t structure to (size_t) -1.  The transport must
    *  be able to handle that.
    */
   Ret_t (XPTAPI *setDocumentInfo)(void *privateConnectionInfo,
                                   const XptCommunicationInfo_t *pDoc);


   /**
    * FUNCTION: Called when xptGetDocumentInfo() is called
    *
    *  Retrieve the document information associated with an incoming document.
    *
    * IN: privateConnectionInfo, pointer to the transport binding's private
    *         information about the connection instance.  This is the same
    *         value that was returned by the "openCommunication" function when
    *         the connection instance was created.
    *
    *     pDoc, passed directly from the xptGetDocumentInfo() call.
    *
    * NOTES:
    *
    *  If the transport doesn't know the length of the document when the call
    *  to xptGetDocumentInfo() is made, it is permitted to set the cbLength
    *  field of the XptCommunicationInfo_t structure to (size_t) -1.  Transport
    *  implementations are encouraged, however, to supply the document length
    *  in advance if possible.  This makes storage management easier for the
    *  application.
    */
   Ret_t (XPTAPI *getDocumentInfo)(void *privateConnectionInfo,
                                   XptCommunicationInfo_t *pDoc);



   // LEO:
   //
   // Para cancelar comunicaciones de forma asíncrona
   Ret_t (XPTAPI *cancelCommAsync)(void *privateConnectionInfo);


#ifdef __EPOC_OS__
   /**
    * FUNCTION: Called from xptCloseTLS()
	*/
   void (*resetBindingTLS)();
#endif
};

struct xptTransportErrorInformation {
   long protocolErrorCode;          // Protocol-specific error code
   const char *errorMessage;        // Error message
};



/**
 * FUNCTION: xptRegisterTransport
 *  Register a transport binding for use by a syncML application
 *
 * PRE-Condition:
 *  A call (or calls) to this function is expected to be made from the
 *  transport bindings's "initializeTransport" function.  If the transport
 *  binding is dynamically loaded, the "initializeTransport" symbol should be
 *  exported from the loadable module (aka DLL).  It will be called by the xpt
 *  service when the xpt service is initializing the list of available
 *  transport bindings.  If no transports are registered by a dynamically
 *  loaded module, the module will be unloaded.  This uses less storage in
 *  cases where the module discovers that required hardware (e.g., an infrared
 *  port) is missing.  If the transport binding is statically linked with the
 *  application, the "initializeTransport" function should use a different
 *  symbol (to avoid conflicted with other statically linked transport
 *  bindings).  This function will be called by xpt to initialize the
 *  transport.
 *
 * POST-Condition:
 *  If successful, the new transport binding will be registered and available
 *  to applications for use.  The next call (if any) the transport binding
 *  should expect to get is to the transport's selectService() function.
 *
 *  The xpt service copies all information it needs from the provided
 *  xptTransportDescription structure, so the transport implementation can
 *  re-use or release the stucture on return.
 *
 * IN:
 *      pTransportDescription, pointer to a struct which contains information
 *      about the transport.  It contains descriptive fields, pointers to
 *      functions without the transport binding that will be called when the
 *      transport is used, and a private fields which is passed to the
 *      transport binding to identify the transport binding on subsequent calls.
 *
 * OUT:
 *
 * IN/OUT:
 *
 * RETURN: Ret_t, Return code. Refer to the return code list for details.
 *
 * NOTES:
 *
 *  The provided transport description stucture should be filled in with the
 *  following fields:
 *
 *    shortName   - The short name of the protocol.  Each available protocol
 *                  registers itself with a unique short name such as
 *                  "HTTP", "OBEX/IR", "OBEX/IP", "WSP", etc..
 *
 *    description - A one-sentence description of the protocol, suitable for
 *                  displaying in a list of available protocols from which the
 *                  user may choose.
 *
 *    flags       - Flags that provide additional information about the
 *                  protocol.  These are:
 *
 *                   XPT_CLIENT  - Indicates the protocol can support
 *                                 client-type connections (outgoing requests).
 *
 *                   XPT_SERVER  - Indicates the protocol can support
 *                                 server-type connections (incoming requests).
 *
 *                  At least one of XPT_CLIENT or XPT_SERVER must be set, but
 *                  not necessarily both.
 *
 *    privateTransportInfo
 *                - This field should be initialized to contain a value that
 *                  identifies the transport binding to the implementing code.
 *                  It is typically a pointer to a structure that contains
 *                  transport-specific information.  The only use the xpt
 *                  service makes of this field is to pass it on subsequent
 *                  calls to functions of the transport binding to identify the
 *                  transport binding.
 */
XPTDECLEXP1 Ret_t XPTAPI xptRegisterTransport(const struct xptTransportDescription *pTransport) XPT_SECTION;

/**
 * FUNCTION: xptSetLastError
 *  Provide transport-specific error information.
 *
 * PRE-Condition:
 *  Whenever a transport binding implementation is preparing to return a Ret_t
 *  value not equal to SML_ERR_OK or SML_ERR_A_XPT_EOX from one of the callback
 *  functions defined in the TransportDescription structure, it should  first
 *  call xptSetLastError() to provide transport-specific information to the xpt
 *  service.  If the application subsequently calls xptGetLastError(), the
 *  information provided on the call to xptSetLastError() will be combined with
 *  more general xpt information and provided to the application in a
 *  XptErrorInformation_t structure.
 *
 *  If the transport binding implementation supports calls on multiple threads,
 *  the call to xptSetLastError() should be made on the same thread that called
 *  the binding's callback function.
 *
 * POST-Condition:
 *  The provided error information is saved in preparation for the
 *  application's call to xptGetLastError().  If the platform is multithreaded,
 *  the xpt service will save the information in a per-thread storage area.
 *
 *  The xpt service copies all information it needs from the provided
 *  xptTransportErrorInformation structure, so the transport implementation can
 *  re-use or release the stucture on return, and any strings pointed to by
 *  the stucture.
 *
 * IN:  info, pointer to a structure containing platform-specific error
 *      information.  See NOTES for advice on what to put in each field of the
 *      stucture.
 *
 * OUT:
 *
 * IN/OUT:
 *
 * RETURN: Ret_t, Return code. Refer to the return code list for details.
 *
 * NOTES:
 *
 *  The provided transport error information stucture should be filled in with
 *  the following fields:
 *
 *    protocolErrorCode
 *                - An integer error code, which is expected to have a meaning
 *                  relevant only to the particular transport.  If the
 *                  transport documents its error values, the user could look
 *                  this value up in the provided documentation to diagnose
 *                  the problem further.
 *
 *    errorMessage
 *                - An error message, suitable for displaying, that describes
 *                  the cause of the error in as much detail as possible.  When
 *                  the xpt service forms the error message to return to the
 *                  application, it will have this general form:
 *                      transportName: functionName:  error-message
 *                  Where:
 *                      transportName  - is the short name of the transport.
 *                      functionName   - is the name of the xpt function that
 *                                       returned the error (e.g., "xptSendData").
 *                      error-message  - is the error message provided by the
 *                                       transport binding implementation.
 */
XPTDECLEXP1 Ret_t XPTAPI xptSetLastError(const struct xptTransportErrorInformation *info) XPT_SECTION;


/**
 * FUNCTION: xptGetMetaInfoValue
 *  Parse a metaInformation string, returning the value corresponding to a
 *  particular tag.
 *
 * PRE-Condition:
 *  The metainformation string is expected to be a series of blank-delimited
 *  "tag=value" pairs.
 *
 * POST-Condition:
 *  This function searches for a particular tag, returning the address of the
 *  value corresponding to the tag.  A case-insensitive comparison is used when
 *  searching for the tag.  If the tag is not found, a null pointer is
 *  returned.  If the tag is found, a pointer to the first character of the
 *  tag's value is returned, and the length of the value is stored at the
 *  address of the provide size_t pointer.  If the tag is found, but has no
 *  value, a length of zero is returned.
 *
 * IN:  metaInformation, a pointer to the string containing the tag=value pairs.
 *      tag,             a pointer to tag being searched for.  The case is not
 *                       important in the search.
 *      valueLen,        the address of a field to contain the length of the
 *                       value on return.
 *
 * OUT: *valueLen,       If the return value is non-null, this is set to the
 *                       length of the located value.
 *
 * IN/OUT:
 *
 * RETURN: If the tag is found, a pointer to the first character of the tag's
 *         value is returned.  The *valueLen field will contain the length of
 *         the value.
 */
XPTDECLEXP1 const char * XPTAPI xptGetMetaInfoValue(
                                       const char *metaInformation,
                                       const char *tag, size_t *valueLen) XPT_SECTION;

/* The following macros and declarations for available for creating     */
/* debugging output.  To use them, enclose the printf-like argument     */
/* list in an extra set of parens.  E.g.,                               */
/*   XPTDEBUG(("The return value is %d\n", status))                     */
/* include the trailing newline, if one is desired.                     */
#ifdef TRACE
 #define XPTDEBUG(msg) xptDebug msg
#else
 #define XPTDEBUG(msg)
#endif

XPTDECLEXP1 void XPTAPI xptDebug(const char *format, ...) XPT_SECTION;

#ifdef __cplusplus
}
#endif

#endif
