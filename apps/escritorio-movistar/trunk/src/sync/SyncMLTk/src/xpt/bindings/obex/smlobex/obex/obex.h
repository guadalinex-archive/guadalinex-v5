#ifndef OBEX_H
#define OBEX_H

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
**************************************************************************
**
** Defines public API's and constants for the Raw OBEX transport.
**
** See also:
**    obex/constants.h  : for command/response constants and public structures.
**    obex/errors.h     : for error codes.
**
**************************************************************************
*/
#include <obex/constants.h>
#include <obex/error.h>

/*
#ifdef _WIN32
 // luz %%%: seems to be needed
 #ifdef LINK_TRANSPORT_STATICALLY
  #define OBEX_EXPORT
 #else
  #ifdef EXPORT_OBEX_FUNCTIONS
   #define OBEX_EXPORT __declspec(dllexport)
  #else
   #define OBEX_EXPORT __declspec(dllimport)
  #endif
#else
 #define OBEX_EXPORT
#endif
*/
#define OBEX_EXPORT  /* we use .def files as dllim/export is a windows feature */

#ifdef __cplusplus
extern "C" {
#endif

/*
**************************************************************************
**
** Initialization.
**
**    This will take care of setting up any conversation independent aspects of Obex.
**************************************************************************
*/
OBEX_EXPORT void           ObxInit();

/*
**************************************************************************
**
** Handle manipulation.
**
** Routines in this group allow users to obtain and destroy an ObxHandle.
**
** ObxHandleNew():
**    Requests that a handle be returned.  Handles are used throughout the
**    various calls.
**
** ObxHandleFree():
**    Requests that the handle be cleaned up and disposed of.
**************************************************************************
*/
OBEX_EXPORT ObxHandle*     ObxHandleNew();
OBEX_EXPORT void           ObxHandleFree( ObxHandle *handle );

/*
**************************************************************************
**
** Obex Transport Interactions
**
** Routines in this group provide some direct interaction with the transport layer
** that was registered with the handle.  Although not confined to sockets, the transport
** API is based on sockets.  The expected behaviour is also based on the behaviour
** exhibited by sockets.  Implementers of custom transports should map the environment
** they are supporting to an API that satisfies the following.
**
** ObxTransportGet()
**    Returns a pre-defined transport.  See obex/constants.h for a list of available
**    transports.
**
** ObxTransportFree()
**    Disposes of a transport structure.
**
** ObxTransportRegister():
**    Define the transport structure to be used in operations on this handle.  This can be
**    a predefined transport that has been obtained via ObxTransportGet() or a custom transport
**    that conforms to the requirements specified in obxTransport.h.
**
** ObxTransportInitialize()
**    Calls through to the initialize() function call defined within the transport layer.
**    Use the passed meta data to initialize the transport associated with the passed
**    handle.  The format of the meta data will differ with each transport.
**
** ObxTransportOpen():
**    Calls through to the open() function call defined within the transport layer.
**    It's expected that general initialization tasks would be preformed in this call.
**    (Appropriate when acting as a client or server)
**
** ObxTransportClose():
**    Calls through to the close() function call defined within the transport layer.
**    It's expected that general termination tasks would be preformed in this call.
**    (Appropriate when acting as a client or server)
**
** ObxTransportListen()
**    Calls through to the listen() function call defined within the transport layer.
**    (Appropriate when acting as a server)
**
** ObxTransportAccept()
**    Calls through to the accept() function call defined within the transport layer.
**    It blocks, awaiting activity from a peer Obex.
**    (Appropriate when acting as a server)
**
** ObxTransportConnect()
**    Calls through to the connect() function call defined within the transport layer.
**    It initiates any connection activity with the remote peer.
**    (Appropriate when acting as a client)
**
** ObxTransportTerminate()
**    Calls through to the terminate() function call defined within the transport layer.
**    Allows the transport to clean up after itself.
**
**************************************************************************
*/
OBEX_EXPORT ObxTransport  *ObxTransportGet( short preDefinedTransportId );
OBEX_EXPORT void           ObxTransportFree( ObxTransport *transport );

OBEX_EXPORT ObxRc          ObxTransportRegister( ObxHandle *handle, ObxTransport *transport );

OBEX_EXPORT ObxRc          ObxTransportInitialize( ObxHandle *handle, const char *meta );

OBEX_EXPORT ObxRc          ObxTransportOpen( ObxHandle *handle );
OBEX_EXPORT ObxRc          ObxTransportClose( ObxHandle *handle );

OBEX_EXPORT ObxRc          ObxTransportListen( ObxHandle *handle );
OBEX_EXPORT ObxRc          ObxTransportAccept( ObxHandle *handle );

OBEX_EXPORT ObxRc          ObxTransportConnect( ObxHandle *handle );

OBEX_EXPORT ObxRc          ObxTransportTerminate( ObxHandle *handle );

/*
**************************************************************************
**
** Operations.
**
** Routines in this group allow for the flowing of requests to the peer and
** the handling of requests from the peer.
**
** ObxObjectSend():
**    Flows the passed object using the transport layer associated with the passed
**    handle to the peer.  The caller is responsible for ensuring that any responses
**    are handled (probably via a call to ObxObjectRecv()) that may be returning.
**    Note that this call can return OBX_RC_ERR_OBJ_FRAGMENTED_SEND which indicates
**    that only part of the object was sent.  The caller should call ObxObjectSend()
**    again after processing the response from the peer.
**
** ObxObjectRecv():
**    Wait for inbound request/response from the peer.  This could be a response
**    from a previously flowed response.. or something being initiated from a peer.
**    On success, the passed in ObxObject() is populated.
**
** ObxSendTransaction()
**    Warning: GET not supported at this time.  Attempts to send a GET will result in an
**             invalid return code, OBX_RC_ERR_INAPPROPRIATE.
**    Since a single 'transaction' may require numerous send/receives of objects,
**    this routine is designed to handle an entire transaction.
**    Takes care of flowing an entire, fully populated, request to the peer.  This
**    routine will not return until the entire transaction has taken place or an error
**    has occurred.
**    Contrast this with ObxObjectSend() which requires the caller to take care of
**    'continue responses' and other interactions with the peer that may have occurred.
**
**    -  The 'request' ObxObject is a request object, ready to flow to the peer.
**    -  The 'response' ObxObject is an object, provided by the caller, that will be populated
**       with the final response packet from the peer.  If not required, the caller can
**       pass a NULL pointer.  This will instruct the underlying infrastructure to not wait
**       for a response from the peer.  This is useful when flowing a DISCONNECT to the peer.
**    -  Errors will be returned via the ObxRc return.
**    -  The negotiated MTU returned on a CONNECT response is handled for the caller (it's
**       set into the handle for subsequent use).
**
** ObxRecvTransaction()
**    Warning: GET not supported at this time.  If an inbound transaction is found to be a GET,
**             this routine returns OBX_RC_ERR_NO_GET_SUPPORT and places the inbound request in
**             'request' but does no responses to the peer.  It's up to the application to deal
**             with this transaction with ObxObjectSend() and ObxObjectRecv().
**    Since a single 'transaction' may require numerous receive/replies of objects,
**    this routine is designed to handle an entire transaction.
**    Takes care of receiving an entire, fully populated, request from the peer.  This
**    routine will not return until the entire transaction has taken place or an error has
**    occurred.
**    Contrast this with ObxObjectRecv() which requires the caller to take care of creating
**    'continue responses' and other interactions with the peer that may have occurred.
**
**    -  The 'request' ObxObject is a request object, populated by interaction with the
**       peer.  It has been fully populated by the call.
**    -  Errors will be returned via the ObxRc return.
**
**    Setting coalesce to TRUE causes BODY/END_OF_BODY headers to be combined into a
**    single BODY header containing all information.  If this is false, all data will exist
**    within the returned object but there may be multiple BODY tags (and a single END_OF_BODY)
**    tag.  They will, of course, be in the order received from the peer.
**
**
**    The difference between ObxObjectSend()/ObxObjectRecv() and ObxSendTransaction()/
**    ObxRecvTransaction() can be represented with the following example:
**
**                    / ObxObjectSend() --- PUT (!final) ---> ObxObjectRecv() \
**                   /  ObxObjectRecv() <----- continue ----- ObxObjectSend()  \
**                  /   ObxObjectSend() --- PUT (!final) ---> ObxObjectRecv()   \
** ObxSendTransaction() ObxObjectRecv() <----- continue ----- ObxObjectSend() ObxRecvTransaction()
**                  \   ObxObjectSend() --- PUT (!final) ---> ObxObjectRecv()   /
**                   \  ObxObjectRecv() <----- continue ----- ObxObjectSend()  /
**                    \ ObxObjectSend() --- PUT (final)  ---> ObxObjectRecv() /
**                     \ObxObjectRecv() <----- success ------ ObxObjectSend()/
**
**
**************************************************************************
*/
OBEX_EXPORT ObxRc          ObxObjectSend( ObxHandle *handle, ObxObject *object );
OBEX_EXPORT ObxRc          ObxObjectRecv( ObxHandle *handle, ObxObject *object );

#ifndef RAW_API_ONLY
OBEX_EXPORT ObxRc ObxGetSend(ObxHandle *handle, ObxObject *getAnswer, char *mimeType);
OBEX_EXPORT ObxRc ObxGetRecv(ObxHandle *handle, ObxObject *request, ObxObject *responseOut );

OBEX_EXPORT ObxRc          ObxTransactionSend( ObxHandle *handle, ObxObject *request, ObxObject *response );
OBEX_EXPORT ObxRc          ObxTransactionRecv( ObxHandle *handle, ObxObject *request, short coalesce );
#endif

/*
**
** ObxStreamNew()
**    Returns a new stream object.
**    The ObxStream object, returned from this call, must be used on subsequent calls
**    involving the stream.
**
** ObxStreamingSend()
**    Warning: GET not supported at this time.  Attempts to send a GET will result in an
**             invalid return code, OBX_RC_ERR_INAPPROPRIATE.
**    Initiates a 'send' (i.e. PUT) to the remote peer.  The transaction is not
**    completed, thus allowing subsequent writes of data to be done.
**    Calls to ObxStreamSend() (after this call returns) can be used to send
**    additional data to the peer.  This data will be encoded in OBEX_HEADER_BODY
**    structures as it is sent.  Thus all headers, except BODY headers, must
**    be present in the initial, passed in, request object.  It is a requirement that
**    all non-body headers fit within a single MTU for this implementation.
**    This calls is only appropriate for OBEX_CMD_PUT requests that are initiated on
**    the local machine.  Other request types will result in an error return.
**       request  - A request containing initial headers to be sent to the peer.
**       stream   - A stream object that will be populated during the send.  Pass this token
**                  on all subsequent calls involving this transaction.
**
** ObxStreamingRecv()
**    Warning: GET not supported at this time.  If an inbound transaction is found to be a GET,
**             this routine returns OBX_RC_ERR_NO_GET_SUPPORT and places the inbound request in
**             'request' but does not respond to the peer.  It's up to the application to deal
**             with this transaction with ObxObjectSend() and ObxObjectRecv().
**    Waits for an inbound request from a peer.  When one comes in, all initial headers are
**    used to populate the passed 'request' object and the call returns.  Additional inbound
**    headers may be obtained via calls to ObxStreamRecv().
**    Returns
**       OBX_RC_OK         -  Success but more data is expected.  User should call ObxStreamRecv()
**                            to continue to accept data.
**       OBX_RC_STREAM_EOF -  Success, final bit from peer, no more data expected on stream.
**                            (i.e. it all fit within a single transmission).
**       OBX_RC_ERR_NO_GET_SUPPORT
**                         -  Inbound GET detected.
**
** ObxStreamSend()
**    Writes data into the stream.  Buffering may occur on the local side prior to actual sending
**    of data.
**
** ObxStreamRecv()
**    The passed list is appended with new, inbound headers, in the order received, from the peer.
**    Any existing data in the list is unmolested.  Note that this call can return after having
**    appended no additional data to the header list.  Returns OBX_RC_STREAM_EOF if transaction is
**    complete.
**    Returns
**       OBX_RC_OK         -  Success but more data is expected.  User should continue to call
**                            ObxStreamRecv() to accept data.
**       OBX_RC_STREAM_EOF -  Success, final bit from peer, no more data expected on stream.
**
** ObxStreamFlush()
**    Causes all queued headers, entered with the ObxStreamSend() call, to be sent to the peer.
**
** ObxStreamClose()
**    Closes the stream (flushes all remaining buffers, if writing).
**
** ObxStreamFree()
**    Returns storage associated with the passed stream object.
**
*/
#ifndef RAW_API_ONLY
OBEX_EXPORT ObxStream      *ObxStreamNew();

OBEX_EXPORT ObxRc          ObxStreamingSend( ObxHandle *handle, ObxObject *request, ObxStream *stream );
OBEX_EXPORT ObxRc          ObxStreamingRecv( ObxHandle *handle, ObxObject *request, ObxStream *stream );

OBEX_EXPORT ObxRc          ObxStreamSend( ObxStream *stream, unsigned char *data, int len );
OBEX_EXPORT ObxRc          ObxStreamRecv( ObxStream *stream, ObxList *headerList );

OBEX_EXPORT ObxRc          ObxStreamFlush( ObxStream *stream );
OBEX_EXPORT ObxRc          ObxStreamClose( ObxStream *stream );

OBEX_EXPORT ObxRc          ObxStreamFree( ObxStream *stream );
#endif


/*
**************************************************************************
**
** Methods to create and destroy an object
**
** ObxObjectNew():
**    Create a new object.  The passed command type is used to initialize it.
**    The caller will then add headers prior to flowing the request to the peer.
**
** ObxObjectNewResponse()
**    Forms an ObxObject for the given request object.  The response code
**    is indicated by the passed ObxCommand.
**    The caller is free to add appropriate headers prior to flowing the response
**    to the peer.
**
** ObxObjectSetConnectMeta()
** ObxObjectSetSetPathMeta()
**    Set's meta data specific to the OBEX_CMD_CONNECT and OBEX_CMD_SETPATH
**    objects.
**
** ObxObjectFree():
**    Return all memory associated with the passed object.
**
** ObxObjectReset()
**    Resets the internals of an ObxObject so it can be reused.  The command type
**    is used to determine if meta data objects need to be freed.
**
** ObxObjectAddHeader()
**    Add a ObxHeader to an ObxObject.
**
** ObxObjectGetHeaderList()
**    Retrieve the list of ObxHeaders from the object.
**
** ObxObjectGetCommand()
**    What command is associated with this object?  Returns the command
**    or -1 on error.
**
**************************************************************************
*/
OBEX_EXPORT ObxObject*     ObxObjectNew( ObxHandle *handle, ObxCommand cmd );
OBEX_EXPORT ObxObject*     ObxObjectNewResponse( ObxHandle *handle, ObxObject *request, ObxCommand cmd );

OBEX_EXPORT ObxRc          ObxObjectFree( ObxHandle *handle, ObxObject *object );
OBEX_EXPORT ObxRc          ObxObjectReset( ObxObject *object );
OBEX_EXPORT ObxRc          ObxObjectAddHeader( ObxHandle *handle, ObxObject *object, ObxHeader *header );
OBEX_EXPORT ObxList        *ObxObjectGetHeaderList( ObxObject *object );
OBEX_EXPORT ObxCommand     ObxObjectGetCommand( ObxObject *object );

/*
**************************************************************************
**
** Obex headers
**
** ObxHeaderNew()
**    Create a new header.
**
** ObxHeaderSetIntValue()
**    On headers where an int value is appropriate, this method sets the value.
**
** ObxHeaderSetByteValue()
**    On headers where an byte value is appropriate, this method sets the value.
**
** ObxHeaderSetUnicodeValue()
**    On headers where an unicode value is appropriate, this method sets the value.
**    Ownership of the passed ObxBuffer is assumed by the header.
**
** ObxHeaderSetByteSequenceValue()
**    On headers where an bytestream value is appropriate, this method sets the value.
**    Ownership of the passed ObxBuffer is assumed by the header.
**
** ObxHeaderNewFromBuffer()
**    Builds a header from a passed ObxBuffer object.  Note that any lengths within
**    the buffer are assumed to be in network byte order.
**    If autoAppend is TRUE, the passed object will be searched for existing headers
**    of the same type.  If found, the data will be appended to this header and a NULL
**    will be returned.
**
** ObxHeaderFree()
**    Frees all contents (and the header itself).
**
** ObxHeaderSize()
**    Returns the header size.  The size includes the opcode and any required lengths
**    in addition to the data itself.
**
** ObxAddHeader():
**    Associates the passed header with the passed object within the passed handle.
**
** ObxGetHeaderList():
**    Returns a list construct containing the headers.
**
**************************************************************************
*/
OBEX_EXPORT ObxHeader   *ObxHeaderNew( unsigned char opcode );

OBEX_EXPORT ObxRc       ObxHeaderSetIntValue( ObxHeader *header, unsigned int value );
OBEX_EXPORT ObxRc       ObxHeaderSetByteValue( ObxHeader *header, unsigned char value );
OBEX_EXPORT ObxRc       ObxHeaderSetUnicodeValue( ObxHeader *header, ObxBuffer *value );
OBEX_EXPORT ObxRc       ObxHeaderSetByteSequenceValue( ObxHeader *header, ObxBuffer *value );

OBEX_EXPORT ObxHeader   *ObxHeaderNewFromBuffer( ObxBuffer *buffer, ObxObject *object );
OBEX_EXPORT void        ObxHeaderFree( ObxHeader *header );
OBEX_EXPORT int         ObxHeaderSize( ObxHeader *header );

OBEX_EXPORT ObxRc       ObxHeaderAdd( ObxHandle *handle, ObxObject *object, ObxHeader *header );
OBEX_EXPORT ObxList*    ObxGetHeaderList( ObxHandle *handle, ObxObject *object );


/*
**************************************************************************
**
** Obex buffers
**
** ObxBufNew()
**    Alloc a new buffer of specified length.
**    Returns a newly allocated ObxBuffer or NULL on error.
**
** ObxBufFree()
**    Free all internal contents and the 'buf' itself.
**    Pointer 'buf' should not be used after calling this function.
**
** ObxBufReset()
**    Clear internal state, allows buffer to be reused.
**    Returns a reset ObxBuffer or NULL on error.
**
** ObxBufWrite()
**    Appends bytes from passed 'data' to the end of the
**    buffer.  (copies data from 'data' into buffer).
**    Returns 0 on success, !0 on error.
**
** ObxBufRead()
**    Populates 'data' with 'len' bytes from the buffer.  Returns
**    the actual number of bytes placed into 'data'.  This value can
**    be less than 'len' if no additional bytes were available.
**    Data is consumed by this call (not available for read again).
**
** ObxBufPeek()
**    Populates 'data' with 'len' bytes from the buffer.  Returns
**    the actual number of bytes placed into 'data'.  This value can
**    be less than 'len' if no additional bytes were available.
**    Data is NOT consumed by this call.
**
** ObxBufSize()
**    Number of unread bytes in buffer.
**
*/
OBEX_EXPORT ObxBuffer   *ObxBufNew( int len );
OBEX_EXPORT void        ObxBufFree( ObxBuffer *buf );
OBEX_EXPORT ObxBuffer   *ObxBufReset( ObxBuffer *buf );
OBEX_EXPORT int         ObxBufWrite( ObxBuffer *buf, void *data, int len );
OBEX_EXPORT int         ObxBufRead( ObxBuffer *buf, void *data, int len );
OBEX_EXPORT int         ObxBufPeek( ObxBuffer *buf, void *data, int len );
OBEX_EXPORT int         ObxBufSize( ObxBuffer *buf );

/*
**************************************************************************
**
** Obex lists
**
** ObxListNew()
**    Create a new list.
**
** ObxListFree()
**    Free's the contents of a list and the list struct itself.
**    WARNING: each element of the list is also free()'ed!
**    Use ObxListRelease() to remove the elements without freeing them.
**    The list should be considered invalid after this call.
**
** ObxListReset()
**    Empty's the list, free()'ing all elements and resetting the list
**    to it's initial state.
**    The list is still viable and ready for use after this call.
**    Use ObxListFree() to free the list itself.
**
** ObxListRelease()
**    Removes all contents of a list.  They are not free'ed.  It's assumed
**    that the caller is managing their storage.
**    The list is still viable and ready for use after this call.
**
** ObxListAppend()
**    Append a new data to the end of the list
**    On success, returns void * pointing to the data that was added.
**    Returns NULL on error.
**
** ObxListGetIterator()
**    Get an Iterator for the list.
**    Returns the newly created iterator or NULL on error.
**
** ObxListInsert()
**    Insert a new element into the list 'after' the current position of
**    the Iterator.
**    Inserts to the front of the list are done with an iterator that has been
**    'reset' (ObxIteratorReset()).
**    On success, returns pointer to the data that was added.
**    Returns NULL on error.
**
** ObxListRemove()
**    Remove the element being pointed to by the iterator.  Return this data
**    as a pointer to the caller.
**
**    The iterator is adjusted as follows:
**    1) The iterator is backed up to the previous node.
**    2) If the node being removed was the first node, the iterator is reset.
**
**    On success, returns pointer to the data that was removed.
**    Returns NULL on error.
**
**    Use ObxListDelete() to remove and free the data being managed by the
**    list.
**
** ObxListDelete()
**    Remove the element being pointed to by the iterator.  Free any data associated
**    with the node.
**
**    The iterator is adjusted as follows:
**    1) The iterator is backed up to the previous node.
**    2) If the node being removed was the first node, the iterator is reset.
**
**    Use ObxListRemove() to remove the data and return it to the caller.
**
** ObxIteratorReset()
**    Reset the enumeration cursor to the beginning of the list.
**    Returns the newly reset iterator or NULL on error.
**
** ObxIteratorFree()
**    Free storage associated with the Iterator.  Iterator should be considered
**    invalid after this call.
**
** ObxIteratorNext()
**    Requests the next element of data.  If at the end of the list, list is empty
**    or the passed iterator is NULL, NULL is returned.
**
** ObxIteratorHasNext()
**    Queries the existence of an element beyond the current element in
**    the list.  Returns 0 if one does not exist, non-zero otherwise.
**
**************************************************************************
*/
OBEX_EXPORT ObxList     *ObxListNew();
OBEX_EXPORT void        ObxListFree( ObxList *list );
OBEX_EXPORT void        ObxListReset( ObxList *list );
OBEX_EXPORT void        ObxListRelease( ObxList *list );
OBEX_EXPORT void        *ObxListAppend( ObxList *list, void *data );
OBEX_EXPORT void        *ObxListInsert( ObxList *list, const ObxIterator *iterator, void *data );
OBEX_EXPORT void        *ObxListRemove( ObxList *list, ObxIterator *iterator );
OBEX_EXPORT void        ObxListDelete( ObxList *list, ObxIterator *iterator );

OBEX_EXPORT ObxIterator *ObxListGetIterator( ObxList *list );
OBEX_EXPORT ObxIterator *ObxIteratorReset( ObxIterator *iterator );
OBEX_EXPORT void        ObxIteratorFree( ObxIterator *iterator );
OBEX_EXPORT void        *ObxIteratorNext( ObxIterator *iterator );
OBEX_EXPORT short       ObxIteratorHasNext( ObxIterator *iterator );

/*
**************************************************************************
**
** Misc conversion functions
**
** ObxUnicodeToUTF8()
**    Converts the passed buffer Unicode to UTF8.  The length indicates the
**    length of the unicode buffer.
**    The result is present in the ObxBuffer structure returned.
**    NULL is returned on error.
**
** ObxUTF8ToUnicode()
**    Converts the passed buffer, which points to a buffer containing
**    UTF8, to Unicode.  The length represents the length of the passed buffer.
**    The result is present in the ObxBuffer structure returned.  NULL is returned on error.
**
**************************************************************************
*/
OBEX_EXPORT ObxBuffer   *ObxUnicodeToUTF8( const unsigned char *unicodeBuffer, int ucLength );
OBEX_EXPORT ObxBuffer   *ObxUTF8ToUnicode( const unsigned char *utf8Buffer, int utf8Length );

#ifdef __cplusplus
}
#endif

#endif
