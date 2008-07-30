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


// %%% luz 2003-01-02: moved this from xptitcp.h to this new xptitcp.c file
//     as .h is included in more than one source file which leads to
//     multiply defined functions (linker errors)

// # of active sockets
static int _iNumSockets = 0;

// %%% luz 2002-12-19: inserted this as a function (was in the TCPTERM() macro before)
void _termTCPStack (int aImmediate)
{
  if (--_iNumSockets == 0) WSACleanup ();
}

// %%% luz 2003-01-02: made this a function too, to correctly encapsulate
//     the _iNumSockets static
void _initTCPStack (TcpRc_t *rcP)
{
  WSADATA wsadata;
  if ((_iNumSockets++ == 0) && WSAStartup (0x0101, &wsadata))
    *rcP=(TcpRc_t)(WSAGetLastError()-WSABASEERR);
} 



#ifdef WINCE_SSL

/* %%% accept-all version
// SSL validation function
static int CALLBACK validateCertificate (
 DWORD dwType, // in
 LPVOID pvArg, // in
 DWORD dwChainLen, // in
 LPBLOB pCertChain, // in
 DWORD dwFlags // in
)
{
  // just accept for now
  return SSL_ERR_OKAY;
} // validateCertificate
*/


///* from the internet, does not work yet

// Check certificate
static int CALLBACK validateCertificate(
 DWORD dwType, // in
 LPVOID pvArg, // in
 DWORD dwChainLen, // in
 LPBLOB pCertChain, // in
 DWORD dwFlags // in
)
{
  int ErrCode = SSL_ERR_OKAY;

  #ifdef _SSL_ORIG_SAMPLE_CODE
  PCCERT_CONTEXT pIssuerCertContext;
  DWORD dwValidityCheckFlags;
  HCERTSTORE hCASysStore = CertOpenSystemStore(0,"ROOT");

  PCCERT_CONTEXT pVSUCertContext =
    CertCreateCertificateContext(
      X509_ASN_ENCODING,
      pCertChain->pBlobData,
      pCertChain->cbSize
    );

  dwValidityCheckFlags =
    CERT_STORE_SIGNATURE_FLAG |
    CERT_STORE_TIME_VALIDITY_FLAG;

  pIssuerCertContext =
    CertGetIssuerCertificateFromStore(
      hCASysStore,
      pVSUCertContext,
      NULL,
      &dwValidityCheckFlags
    );

  if (pIssuerCertContext) {
    if (dwValidityCheckFlags & CERT_STORE_SIGNATURE_FLAG) {
      ErrCode = SSL_ERR_SIGNATURE;
    }
    else {
      if (dwValidityCheckFlags & CERT_STORE_TIME_VALIDITY_FLAG)
        ErrCode = SSL_ERR_CERT_EXPIRED;
      else
        ErrCode = SSL_ERR_OKAY;
    }
    CertFreeCertificateContext(pIssuerCertContext);
  }
  else
    ErrCode = SSL_ERR_FAILED;
  CertFreeCertificateContext(pVSUCertContext);
  CertCloseStore(hCASysStore,CERT_CLOSE_STORE_FORCE_FLAG);.
  
  #else
  HCERTSTORE hCASysStore;
  PCCERT_CONTEXT pVSUCertContext;
  DWORD                    myFlags=0;
  PCCERT_CHAIN_CONTEXT chainContextP;

  // chain params
  CERT_ENHKEY_USAGE        EnhkeyUsage;
  CERT_USAGE_MATCH         CertUsage;  
  CERT_CHAIN_PARA          ChainPara;

  // init chain params
  EnhkeyUsage.cUsageIdentifier = 0;
  EnhkeyUsage.rgpszUsageIdentifier=NULL;
  CertUsage.dwType = USAGE_MATCH_TYPE_AND;
  CertUsage.Usage  = EnhkeyUsage;
  ChainPara.cbSize = sizeof(CERT_CHAIN_PARA);
  ChainPara.RequestedUsage=CertUsage;
  // init chaincontext ptr
  chainContextP=NULL;
  // open cert store
  hCASysStore = CertOpenSystemStore(0,"ROOT");
  // get context for received certificate
  pVSUCertContext =
    CertCreateCertificateContext(
      X509_ASN_ENCODING,
      pCertChain->pBlobData,
      pCertChain->cbSize
    );
  // create chain
  if (!CertGetCertificateChain(
    NULL, // default engine
    pVSUCertContext, // our certificate 
    NULL, // checking for now
    NULL, // no additional store
    &ChainPara,
    myFlags, // no flags
    NULL, // reserved
    &chainContextP // receives chain context pointer
  )) {
    // failed creating chain
    ErrCode = SSL_ERR_FAILED;
  }
  else {
    switch(chainContextP->TrustStatus.dwErrorStatus) {
      case CERT_TRUST_NO_ERROR:
        // ok, fine
        ErrCode = SSL_ERR_OKAY;
        break;
      case CERT_TRUST_CTL_IS_NOT_TIME_VALID: 
      case CERT_TRUST_IS_NOT_TIME_NESTED: 
      case CERT_TRUST_IS_NOT_TIME_VALID:
        #ifdef PROGRESS_EVENTS
        if (!GlobalNotifyProgressEvent(pev_ssl_expired,0,0))
        #endif
          ErrCode = SSL_ERR_CERT_EXPIRED;
        break; 
      default:
        #ifdef PROGRESS_EVENTS
        if (!GlobalNotifyProgressEvent(pev_ssl_notrust,0,0))
        #endif
          ErrCode = SSL_ERR_FAILED;
        break;   
    } // End switch    
    // free chain
    CertFreeCertificateChain(chainContextP);
  }

  CertFreeCertificateContext(pVSUCertContext);
  CertCloseStore(hCASysStore,CERT_CLOSE_STORE_FORCE_FLAG);
  #endif


  return ErrCode;
} // validateCertificate
//*/


// enable SSL for specified socket
TcpRc_t tcpEnableSSL(SocketPtr_t pSocket, Bool_t aConnected)
{
  TcpRc_t rc = TCP_RC_OK;
  int iTcpRc;
  SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;
  DWORD optval;

  // we need to do things only BEFORE the connection is open
  if (aConnected) return TCP_RC_OK;

  if (lSocket == -1L)
      return TCP_RC_ERROR;

  // secure client connection requested
  // - set SSL socket option
  optval = SO_SEC_SSL;
  iTcpRc = setsockopt (
    lSocket,
    SOL_SOCKET, // level: SSL is socket level option
    SO_SECURE, // optname: select secure socket
    &optval, // value: select SSL
    sizeof(optval) // value length
  );
  rc = CHKERROR (iTcpRc);
  if (rc==TCP_RC_OK) {
    // set certificate validation hook
    // - prepare hook info
    SSLVALIDATECERTHOOK hook;
    hook.HookFunc=validateCertificate;
    hook.pvArg=NULL;
    // - set hook
    iTcpRc = WSAIoctl(
      lSocket,
      SO_SSL_SET_VALIDATE_CERT_HOOK, // ioControlCode
      &hook, // input buffer = hook structure
      sizeof(hook), // size of hook structure
      NULL, // no output buffer
      0, // no size of output buffer
      NULL, // we don't need number of output bytes
      NULL, NULL // overlapped not supported
    );
    rc = CHKERROR (iTcpRc);
  }
  return rc;
}

#endif // WINCE_SSL


#ifdef WINCE

/*****************************************************************************/
/*                                                                           */
/*    Function: receive a block of data from the communication partner.      */
/*                                                                           */
/*****************************************************************************/

// time out after 5 seconds and 0 microseconds
const struct timeval tcp_timeout = { 5, 0 };

TcpRc_t tcpReadData (SocketPtr_t     pSocket,   // i: Socket
                     DataBuffer_t    pbBuffer,  // i: Data buffer
                     BufferSizePtr_t pcbBufferSize) // io: size of data buffer / # of bytes reveived
{
  TcpRc_t rc = TCP_RC_OK;
  int iReceived;
  fd_set  readfds; // %%% for luz timeout below
  #ifdef PROGRESS_EVENTS
  int timeouts=120; // abortable: 120 timeouts of 5 seconds = 10 minutes total
  #else
  int timeouts=24; // non-abortable: 24 timeouts of 5 seconds = 2 minutes total
  #endif

  SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;

  if (lSocket == -1L)
    return TCP_RC_ERROR;

  /************************/
  /* Receive a data block */
  /************************/

  // %%% luz changes to make sure receive aborts after a timeout
  do {
    // from a sample found on google groups:
    FD_ZERO (&readfds);
    FD_SET (lSocket, &readfds);
    if (select(0, &readfds, NULL, NULL, &tcp_timeout) == 1) {
     iReceived = recv (lSocket, (char *)pbBuffer, (int) *pcbBufferSize, 0);
     break;
    }
    else {
      // not ready for receive
      if (timeouts>0) {
        timeouts--;
        // %%% luz:2003-04-17: Note this is Synthesis SySync farmework specific
        #ifdef PROGRESS_EVENTS
        if (!GlobalNotifyProgressEvent(pev_nop,0,0))
          return TCP_RC_ECONNABORTED;  
        #endif
        continue;
      }
     // completely time out now
     return TCP_RC_ETIMEDOUT;
    }
  } while(true);

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


/*****************************************************************************/
/*                                                                           */
/*        Function: Send a block of data to the communication partner.       */
/*                                                                           */
/*****************************************************************************/

TcpRc_t tcpSendData (SocketPtr_t        pSocket,      // i: socket
                     const DataBuffer_t pbBuffer,     // i: data buffer
                     BufferSize_t       cbBufferSize) // i: size of data buffer
{
  TcpRc_t rc = TCP_RC_OK;
  int iTcpRc;
  fd_set  writefds;
  SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;

  if (lSocket == -1L)
    return TCP_RC_ERROR; // invalid socket

  // %%% luz changes to make sure receive aborts after a timeout
  // from a sample found on google groups:
  FD_ZERO (&writefds);
  FD_SET (lSocket, &writefds);
  if (select(0, NULL, &writefds, NULL, &tcp_timeout) == 1) {
    iTcpRc = send (lSocket,  (char *)pbBuffer, (int) cbBufferSize, 0);
  }
  else {
    // not ready for receive
    return TCP_RC_ETIMEDOUT;
  }   

  rc = CHKERROR (iTcpRc);
  if ((iTcpRc > 0) && (iTcpRc != (int) cbBufferSize))
    rc = TCP_RC_ERROR;
  return rc;
} // tcpSendData

#endif

/* eof */
