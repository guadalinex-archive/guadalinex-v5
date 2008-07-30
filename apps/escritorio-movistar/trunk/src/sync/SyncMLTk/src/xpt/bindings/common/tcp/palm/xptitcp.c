/*************************************************************************/
/* module:          SyncML Communication Protocol selection module       */
/* file:            src/xpt/win/xptitcp.c                                */
/* target system:   win                                                  */
/* target OS:       win                                                  */
/*************************************************************************/

/**
 * This module contains Windows specific code for the
 * TCP/IP protocol handler for SyncML
 */

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#include "xptitcp.h"

// %%% luz:2003-04-17: Note this is Synthesis SySync farmework specific
#ifdef PROGRESS_EVENTS
  // we need SySync-specific include to allow progress monitoring
  #include "global_progress.h"
#endif

// # of active sockets
int _iNumSockets = 0;

// %%% 2002-09-28 luz added
Bool_t weOpenedNetLib = false;

#ifdef DEBUGHACKS
#warning "%%% This is hacky debug only!!"
int netlibopenerr;
int netlibtcperr;
// %%%
#endif

/********************************************************/
/* Function: implementation of the strrchr() c-function */
/********************************************************/

// %%% luz 2002-09-12: changed arguments from (char *,char)
//     to be compatible with PalmOS CW headers
// %%% luz 2002-09-12: changed name from strrchr, as this
//     causes name clashes.
char *StrRChr (const char *pszString, int ch)
{
  const char *p;

  for (p = pszString + strlen (pszString) - 1; p >= pszString; p --)
    if (*p == ch) break;
  if (p < pszString)
    p = NULL;
  return (char *)p;
} // StrRChr


/***********************************************************/
/* Function: Initialize the TCP/IP stack                   */
/* This must be done the first time the process is started */
/***********************************************************/
int _initTCPStack (void)
{
  int rc = 0;
  Err err;

  #ifdef DEBUGHACKS
  #warning "%%% This is hacky debug only!!"
  netlibopenerr=0;
  netlibtcperr=0;
  // %%%
  #endif

  /**********************************************************************/
  /* Load the Net.lib library to initialize the Berkeley IP stack       */
  /* The Library reference number must be stored in the global variable */
  /* AppNetRefnum that is defined in NetSocket.lib                      */
  /**********************************************************************/

  err = SysLibFind ("Net.lib", &AppNetRefnum);
  if (err != 0)
    rc = INVALID_SOCKET;

  /***********************************************************/
  /* Set the IP timeout. the global variable AppNetTimeout,  */
  /* defined in NetSocket.lib, is set accordingly.           */
  /***********************************************************/

  AppNetTimeout = SysTicksPerSecond () * TIMEOUT_SECONDS;

  /* Open the TCP-IP interface */
  if (rc == 0) {
    Word tcp_rc;
    err = NetLibOpen (AppNetRefnum, &tcp_rc);
    if (err != 0) {
      rc = INVALID_SOCKET;
      #ifdef DEBUGHACKS
      #warning "%%% This is hacky debug only!!"
      netlibopenerr=err;
      // %%%
      #endif
    }
    else if (tcp_rc != 0) {
      rc = SOCKET_ERROR;
      // %%% luz 2002-12-02: netlib is already open, even if there is an interface problem
      //     we need to close netlib again or else next retry will cause a netErrAlreadyOpen
      //     at the next attempt to connect!
      NetLibClose (AppNetRefnum, false);
      #ifdef DEBUGHACKS
      #warning "%%% This is hacky debug only!!"
      netlibtcperr=tcp_rc;
      // %%%
      #endif
    }
    // %%% 2002-09-28 luz added
    else {
      // all ok
      weOpenedNetLib=true; // remember for _termTCPStack
    }
  }
  return rc;
} // _initTCPStack


#ifdef PALM_SSL
// clean up SSL entirely
void sslCleanUp(void);
#endif

/****************************************/
/* Function: Clean-up the TCP/IP stack. */
/****************************************/
void _termTCPStack (Bool_t aImmediate)
{
  // #if EMULATION_LEVEL != EMULATION_NONE
  // %%% luz 2002-11-28 : test for weOpenedNetLib to prevent 
  //                      NetLibClose is not called if not opened before
  if (weOpenedNetLib) {
    NetLibClose (AppNetRefnum, aImmediate);
    weOpenedNetLib=false;
  }
  // if (AppNetRefnum)
  //    NetLibFinishCloseWait(AppNetRefnum);

  #ifdef PALM_SSL
  // release the SSL library if we used it
  sslCleanUp();
  #endif  
} // _termTCPStack


#ifdef PALM_SSL

// library
static Bool_t gSSLLibLoaded = false;
static Bool_t gSSLLibOpen = false;
static UInt16 gSSLlibRef = 0;
// connection
SslLib *gSSLContextTemplate = NULL;
SslContext *gSSLContext = NULL;

// clean up SSL stuff releated to
static void sslCleanUpConn(void) {
  // clean up current SSL connection
  if (gSSLContext) {
    SslContextDestroy(gSSLlibRef,gSSLContext);
    gSSLContext=NULL;
  }
  if (gSSLContextTemplate) {
    SslLibDestroy(gSSLlibRef,gSSLContextTemplate);
    gSSLContextTemplate=NULL;
  }
} // sslCleanUpConn


// clean up SSL entirely
void sslCleanUp(void)
{
  // clean up current context
  sslCleanUpConn();
  // clean up library
  if (gSSLLibOpen) {
    // lib is open, close it
    SslLibClose(gSSLlibRef);
    gSSLLibOpen=false;
  }
  // unload if we loaded it
  if (gSSLLibLoaded) {
    // we have loaded it, unload now
    SysLibRemove(gSSLlibRef);
    gSSLLibLoaded=false;
  }
} // sslCleanUp


// verify callback
static Int32 verifyCallbackFunc(
  SslCallback *aCallbackStruct,
  Int32 aCommand,
  Int32 aFlavor,
  void *aInfo
)
{
  Int32 ret = errNone; // assume ok
  
  switch (aCommand) {
    case sslCmdVerify:
      // default to failing on verify problems
      ret = aFlavor;
      // verify
      switch (aFlavor) {
        case sslErrVerifyNoTrustedRoot:
          // certificate could not be verified
          #ifdef PROGRESS_EVENTS
          if (GlobalNotifyProgressEvent(pev_ssl_notrust,0,0))
            ret=errNone; // user allows // %%%% evtl: sslErrOk ???
          #endif
          break;
        case sslErrVerifyNotAfter:
        case sslErrVerifyNotBefore:
          // certificate has expired
          #ifdef PROGRESS_EVENTS
          if (GlobalNotifyProgressEvent(pev_ssl_expired,0,0))
            ret=errNone; // user allows
          #endif
          break;
      } // switch
      break; 
    case sslCmdNew:
    case sslCmdReset:
    case sslCmdFree:
      break;
  }
  // return status
  return ret;
} // verifyCallbackFunc


// enable SSL for specified socket
TcpRc_t tcpEnableSSL(SocketPtr_t pSocket, Bool_t aConnected)
{
  TcpRc_t rc = TCP_RC_OK;
  SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;
  // SSL vars
  SslCallback verifyCallback;
  Err err;

  // we need to do things only after the connection is open
  if (!aConnected) return TCP_RC_OK;

  // connected, now we can go on
  if (lSocket == -1L)
      return TCP_RC_ERROR;
  // Note: this SSL implementation is only good for a single SSL socket
  // at a time
  if (gSSLContext)
    return TCP_RC_ERROR; // SSL context already open, must close socket first
  
  // secure client connection requested
  /* Part 1: Find and open the SSL library.
  * Note that you only have to do this if you’re
  * writing 68k code; ARM code can skip this step.
  */ 
  // - Find the SSL library
  if (!gSSLLibOpen) {
    if (SysLibFind(kSslDBName,&gSSLlibRef) != errNone)
    {
      if (SysLibLoad(kSslLibType,kSslLibCreator,&gSSLlibRef) != errNone) {
        // No SSL available
        return TCP_RC_ENOPROTOOPT;
      }
      gSSLLibLoaded = true;
    }
    // - open the SSL library
    if (SslLibOpen(gSSLlibRef) != errNone) {
      sslCleanUp();
      return TCP_RC_ENOPROTOOPT;
    }
    gSSLLibOpen = true;
  }
  /* Part 2: Create an SslLib object (which acts as an
  * ‘SSL context template’) and then use the object to ‘spawn’
  * an SslContext object. The latter object represents a
  * ‘real’ SSL context (or environment). All further
  * SSL operations will be in reference to this SslContext
  * object.
  */
  if (SslLibCreate(gSSLlibRef,&gSSLContextTemplate) != errNone) {
    sslCleanUpConn();
    return TCP_RC_ERROR;
  }
  if (SslContextCreate(gSSLlibRef,gSSLContextTemplate,&gSSLContext) != errNone) {
    sslCleanUpConn();
    return TCP_RC_ERROR;  
  }
  /* Part 3: Set the socket into the context. This is done by
  * calling a macro that sets the context’s ‘Socket’
  * attribute. Almost all context configuration that
  * you perform is done by setting the value of a
  * an attrbitute; and for this purpose, a number of
  * attribute-specific macros are provided.
  * We’re assuming that ‘socket’ is an existing
  * NetSocketRef object.
  */
  SslContextSet_Socket(gSSLlibRef,gSSLContext,lSocket);
  // Add a verify callback
  verifyCallback.callback = verifyCallbackFunc;
  SslContextSet_InfoCallback(gSSLlibRef,gSSLContext,&verifyCallback);
  SslContextSet_InfoInterest(gSSLlibRef,gSSLContext,sslFlgInfoCert);
  // Set some options
  // - allow all sorts of known SSL certificate defects
  SslContextSet_Compat(gSSLlibRef,gSSLContext,sslCompatAll);
  // - set client mode
  SslContextSet_Mode(gSSLlibRef,gSSLContext,sslModeSslClient);
  
  
  /* Part 4: Apply SSL to the socket. This is done by ‘opening’
  * a new SSL session.
  */
  err = SslOpen(
    gSSLlibRef,
    gSSLContext,
    sslOpenModeSsl | sslOpenUseDefaultTimeout,
    //sslOpenModeClear | sslOpenUseDefaultTimeout,
    0
  );
  if (err!=errNone) {
    // check error code
    sslCleanUpConn();
    return TCP_RC_ERROR;  
  }
  // open ok, we can use the socket now
  return TCP_RC_OK;
} // tcpEnableSSL


// Read a block from communication partner
TcpRc_t tcpReadData (
  SocketPtr_t     pSocket,   // i: Socket
  DataBuffer_t    pbBuffer,  // i: Data buffer
  BufferSizePtr_t pcbBufferSize) // io: size of data buffer / # of bytes reveived
{
  TcpRc_t rc = TCP_RC_OK;
  int iReceived;
  Err err;

  SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;

  if (lSocket == -1L)
    return TCP_RC_ERROR;
    
  /************************/
  /* Receive a data block */
  /************************/

  // Note: If we are using SSL, we need a different read call
  if (gSSLContext) {
    // SSL socket
    iReceived = SslRead(
      gSSLlibRef,
      gSSLContext,
      (void *)pbBuffer,
      (UInt16) *pcbBufferSize,
      &err
    );
    if (iReceived<0) iReceived=TCP_RC_ERROR; // general error
  }
  else {
    // standard socket
    iReceived = recv (lSocket, (char *)pbBuffer, (int) *pcbBufferSize, 0);
  }

  /************************************************/
  /* 0-byte packages indicate that the connection */
  /* was closed by the communication partner      */
  /************************************************/

  if (iReceived == 0) {
    pbBuffer [0] = '\0';
    *pcbBufferSize = 0L;
    rc = TCP_RC_EOF; // end-of-transmission
  }
  else if (iReceived < 0) {
    /* An error occurred */
    rc = CHKERROR (iReceived);
  }
  else {
    *pcbBufferSize = (BufferSize_t) iReceived; // return the # of bytes received.
    rc = TCP_RC_OK;
  }
  return rc;
} // tcpReadData



// Send a block of data to the communication partner.
TcpRc_t tcpSendData (
  SocketPtr_t        pSocket,      // i: socket
  const DataBuffer_t pbBuffer,     // i: data buffer
  BufferSize_t       cbBufferSize // i: size of data buffer
) {
  TcpRc_t rc = TCP_RC_OK;
  int iTcpRc;
  int sent;
  SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;
  Err err;

  if (lSocket == -1L)
    return TCP_RC_ERROR; // invalid socket

  sent=0;
  do {
    // Note: If we are using SSL, we need a different send call
    if (gSSLContext) {
      // SSL socket
      iTcpRc = SslWrite(
        gSSLlibRef,
        gSSLContext,
        (void *)(pbBuffer+sent),
        (UInt16) cbBufferSize-sent,
        &err
      );
      if (iTcpRc<0) iTcpRc=TCP_RC_ERROR; // general error
    }
    else {
      // standard socket
      iTcpRc = send (lSocket,  (char *)pbBuffer+sent, (int) cbBufferSize-sent, 0);
    }
    // check for error
    if (iTcpRc<0) break;
    // count sent bytes
    sent+=iTcpRc;
    iTcpRc=0; // ok in case we exit loop here
  } while(sent<cbBufferSize);

  rc = CHKERROR (iTcpRc);
  /* %%% not needed any more because we don't exit the loop before all data is sent
  if ((iTcpRc > 0) && (iTcpRc != (int) cbBufferSize))
    rc = TCP_RC_ERROR;
  */
  return rc;
} // tcpSendData


// called before socket is closed
void tcpBeforeSocketClose(SocketPtr_t pSocket)
{
  // clean up SSL connection
  sslCleanUpConn();
} // tcpBeforeSocketClose


#endif


/* eof */
