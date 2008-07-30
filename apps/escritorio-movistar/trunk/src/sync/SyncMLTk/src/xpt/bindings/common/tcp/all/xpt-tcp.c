/*************************************************************************/
/* module:          SyncML Communication Services, TCP/IP functions      */
/* file:            src/xpt/all/xpt-tcp.c                                */
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
 * Basic TCP/IP services for the SyncML communication services
 *
 */
#include <ctype.h>

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

// %%% luz:2003-04-17: Note this is Synthesis SySync farmework specific
#ifdef PROGRESS_EVENTS
  // we need SySync-specific include to allow progress monitoring
  #include "global_progress.h"
#endif

#define MAX_SOCKET_CONNECTIONS  10


#include "xptitcp.h" // internal OS specific implementation headers.

#include "xpttypes.h"
#include "xptport.h"
#include "xpt-tcp.h"


/****************************************************************/
/* Function: set up a sockaddr_in structure for a server socket */
/* The function returns TRUE if the function succeeds.          */
/****************************************************************/

#ifdef CONFIGURABLE_SERVER_ADDRESS
// this function must be defined externally to deliver the server socket address
extern unsigned long getHTTPServerSocketAddr(void);
#endif

Bool_t getINServerAddr (CString_t pszPortAddress,  // i: port address (i.e. "80")
                           struct sockaddr_in *pSocketAddress) // o: socket structure
   {
   Bool_t rc = true;
   int iPort = xppAtoi ((const char *) pszPortAddress);
   // %%% assigned twice, not needed, luz 2002-07-10
   // pSocketAddress->sin_addr.s_addr = INADDR_ANY;
   pSocketAddress->sin_family      = AF_INET;
   pSocketAddress->sin_port        = htons ((unsigned short) iPort);
   #ifdef CONFIGURABLE_SERVER_ADDRESS
   pSocketAddress->sin_addr.s_addr = getHTTPServerSocketAddr();    
   #else 
   pSocketAddress->sin_addr.s_addr = INADDR_ANY;
   #endif
   return rc;
   }


/****************************************************************/
/* Function: set up a sockaddr_in structure for a server socket */
/****************************************************************/

Bool_t getINSockAddr (StringBuffer_t pszIPAddress,// i: IP address (ie "192.168.5.0:80")
                      struct sockaddr_in *pSocketAddress, // o: socket structure
                      char *defaultPort) // i: default port name/number (%%% luz:2003-04-17 added)
{
  Bool_t rc = true;
  Ptr_t pszPort;
  struct hostent * hent;
  unsigned long *pAddr;
  unsigned short port;
  // %%% luz 2002-12-02
  StringBuffer_t p;
  char c;

  pSocketAddress->sin_family = AF_INET;
  pszPort = StrRChr ((char *) pszIPAddress, ':');
  if (pszPort == NULL)
  {
    port = xppAtoi (defaultPort);
  }
  else {
    *pszPort = '\0'; // split server name/IP from port specification
    port = xppAtoi (pszPort+1);
  }
  pSocketAddress->sin_port = htons ( port ) ;

  // %%% luz 2002-12-02 : added handling for IP addresses
  //     (gethostbyname fails on some systems, e.g. PalmOS when given a IP address)
  //     Obviously, the name lookup was hacked in later by someone, hence the
  //     parameter's name: "pszIPAddress" which should be called "pszHostName".
  // - check if string consists of digits and dots only, if yes, its an IP address
  p=pszIPAddress;
  while ((c=*p++)) {
    if (!isdigit(c) && c!='.') break;
  }
  if (c==0) {
    // pszIPAddress is a real IP address, no need to look up name
    pSocketAddress->sin_addr.s_addr = inet_addr(pszIPAddress);
    
  }
  else {
    // host name given, determine IP address
    hent = gethostbyname (pszIPAddress);
    if (hent == NULL) return false;
    pAddr = (unsigned long *)hent->h_addr_list[0];
    pSocketAddress->sin_addr.s_addr = *pAddr;
  }

  // restore original IP address
  if (pszPort != NULL)
    *pszPort = ':';
  return rc;
}


Bool_t getINSockFirewallAddr (TcpFirewallInfoPtr_t pFirewallInfo ,// i: IP address (ie "192.168.5.0:80")
                      struct sockaddr_in *pSocketAddress) // o: socket structure
   {
   Bool_t rc = true;
   struct hostent * hent;
   unsigned long *pAddr;

   pSocketAddress->sin_family = AF_INET;
   pSocketAddress->sin_port = htons( ((pFirewallInfo->serverPort > 0) ? pFirewallInfo->serverPort : xppAtoi (HTTP_PORT)));

   hent = gethostbyname( pFirewallInfo->serverName );
   if (hent == NULL) return false;
   pAddr = (unsigned long *)hent->h_addr_list[0];

   pSocketAddress->sin_addr.s_addr = *pAddr; // inet_addr (pszIPAddress);

   return rc;
}

/*********************************************************************/
/* Function: determine the IP address of the communication partner   */
/* and write it into the specified string buffer (i.e."192.168.5.3") */
/*********************************************************************/

void getSenderAddress ( struct sockaddr_in *pSocket,  // i: socket structure
                       StringBuffer_t pchSenderAddress)    // o: IP address
   {
   /* int iPort = ntohs (pSocket->sin_port); */
   /* sprintf ((char *) pchSenderAddress, "%s:%d", inet_ntoa (pSocket->sin_addr), iPort); */
   xppStrcpy ((char *) pchSenderAddress, inet_ntoa (pSocket->sin_addr));
   return;
   }

TcpRc_t makeSocksConnection( SocketPtr_t pSocket, struct sockaddr_in *addr) {
    // TODO: ....
    int rc;
    int len;
                                        // Connect to the socks server
                                        //  an anonymous user id.
    char data[20];
    data[0] = 4;                      // Socks version 4
    data[1] = 1;                      // Socks command 1, CONNECT
                                      // Server connect port, high byte
    data[2] = (unsigned char)( (addr->sin_port & 0xff00) >> 8);
                                      // Server connect port, low byte
    data[3] = (unsigned char)( addr->sin_port & 0xff);
                                      // IP address of the external web server
    *((unsigned long *)(data+4)) = addr->sin_addr.s_addr;
    xppStrcpy((char *)&data[8], "anonymous");

                                        // send Socks connect request to Socks server
    rc = tcpSendData( pSocket, (unsigned char *)data, 18 );
    if( rc != TCP_RC_OK ) {
        return rc;
    }
    len = 8;
                                        // read reply from server
    rc = tcpReadData( pSocket, (unsigned char *)data, (BufferSizePtr_t)&len );
    if ( rc == TCP_RC_OK && data[1] != 90 ) {
        rc = TCP_RC_ECONNREFUSED;
    }

    return rc;
}


/*****************************************************************************/
/*                                                                           */
/*        Function: Open a TCP/IP connection (both client and server)        */
/*                                                                           */
/*****************************************************************************/

// %%% 2002-09-28 luz added: Macro to test if we need to init TCP stack
//     define it only if xptitcp.h has not defined an OS-sepcific version
#ifndef TCPINITIALIZED
// global flag
static Bool_t bFirstConnection = true;
// Macro
#define TCPINITIALIZED() (!bFirstConnection)
#define TCPINITDONE() bFirstConnection=false
#endif

#ifdef DEBUGHACKS
#warning "%%% This is hacky debug only!!"
int tcperrcode;
#endif

TcpRc_t tcpOpenConnection (CString_t pszPort,  // i: Port address (i.e."192.168.5.1:80")
                           SocketPtr_t pSocket,     // o: Pointer to generated socket
                           CString_t   pszOpenMode) // i: open Mode.
   {
       return tcpOpenConnectionEx( pszPort, pSocket, pszOpenMode, NULL );
   }



TcpRc_t tcpOpenConnectionEx (CString_t            pszPort,
                             SocketPtr_t          pSocket,
                             CString_t            pszOpenMode,
                             TcpFirewallInfoPtr_t pFirewallInfo ) {


  TcpRc_t rc = TCP_RC_OK;
  int iTcpRc;
  // %%% 2002-09-28 luz removed: static Bool_t bFirstConnection = true;
  SOCKET lSocket = -1L;
  struct sockaddr_in socket_address;
  char *defaultPort = HTTP_PORT;
  Bool_t useSSL = false;

  /*********************************************************/
  /* Initialize TCP/IP, if this function is invoked first. */
  /*********************************************************/

  #ifdef DEBUGHACKS
  #warning "%%% This is hacky debug only!!"
  tcperrcode=0;
  #endif

  // %%% 2002-09-28 luz removed: if (bFirstConnection == true) 
  if (!TCPINITIALIZED())
  {
    TCPINIT (rc);
    if (rc == TCP_RC_OK)
     TCPINITDONE();
    #ifdef DEBUGHACKS
    else {
     #warning "This is hacky debug only!!"
     tcperrcode=rc;
    }
    #endif
  }

  /**************************/
  /* Obtain a TCP/IP socket */
  /**************************/

  *pSocket = (Socket_t) 0L;
  if (rc == TCP_RC_OK) {
    lSocket = socket(PF_INET, SOCK_STREAM, 0);
    rc = CHKERROR ((int)lSocket);
    *pSocket = (Socket_t) lSocket;
  }

  /***************************************************************/
  /* Server socket handling: set the socket to the listener mode */
  /***************************************************************/
  if ((pszOpenMode [0] == 's') || (pszOpenMode [0] == 'S')) {
    /*****************************************/
    /* Bind the socket to the specified port */
    /*****************************************/
    if (rc == TCP_RC_OK) {
      getINServerAddr ((const char *)pszPort, &socket_address);
      iTcpRc = bind (lSocket, (struct sockaddr *)&socket_address, sizeof (socket_address));
      rc = CHKERROR (iTcpRc);
    }

    /***************************************/
    /* Set the socket to the listener mode */
    /***************************************/
    if (rc == TCP_RC_OK) {
       iTcpRc = listen (lSocket, MAX_SOCKET_CONNECTIONS);
       rc = CHKERROR (iTcpRc);
    }
  }

  /***********************************************/
  /* Client socket handling: connect to the host */
  /***********************************************/

  else if ((pszOpenMode [0] == 'c') || (pszOpenMode [0] == 'C')) {
  
    // %%% luz 2003-04-16 : added SSL support
    useSSL = pszOpenMode[1] && pszOpenMode[1]=='S';
    if (rc == TCP_RC_OK && useSSL) {
      // SSL preparation BEFORE opening the socket
      rc = tcpEnableSSL((SocketPtr_t)&lSocket,false);
      defaultPort=HTTPS_PORT;
    }

    if (rc == TCP_RC_OK) { // %%% luz:2003-04-16: added check for failure (was missing for client case)
      struct sockaddr_in target_socket_address;
      if ( pFirewallInfo && pFirewallInfo->type == TCP_FIREWALL_PROXY ) {
         getINSockFirewallAddr( pFirewallInfo, &socket_address );
         if (!getINSockAddr ((StringBuffer_t) pszPort, &target_socket_address, defaultPort)) {
             rc = TCP_RC_ERROR;
         }
      } else {
         if (!getINSockAddr ((StringBuffer_t) pszPort, &socket_address, defaultPort)) {
             rc = TCP_RC_ERROR;
         }
      }
      if ( rc == TCP_RC_OK ) {
         iTcpRc = connect (lSocket, (struct sockaddr *)&socket_address, sizeof (socket_address));
         rc = CHKERROR (iTcpRc);

         // %%% luz 2003-06-26 : added SSL init after opening socket
         if (rc == TCP_RC_OK && useSSL) {
           // SSL preparation AFTER opening the socket
           rc = tcpEnableSSL((SocketPtr_t)&lSocket,true);
           if (rc != TCP_RC_OK) {
             // SSL failed, close the socket
             closesocket (lSocket);
           }
         }
         
         if ( rc == TCP_RC_OK && pFirewallInfo && pFirewallInfo->type == TCP_FIREWALL_SOCKS ) {
             rc = makeSocksConnection( pSocket, &target_socket_address );
         }
      }
    }
  }
  else
    rc = TCP_RC_ERROR;

  return rc;
}


// %%% luz 2003-06-26: Added default tcpEnableSSL() here
/*****************************************************************************/
/*                                                                           */
/*             Function: Enable SSL for the  TCP/IP connection               */
/*                                                                           */
/*****************************************************************************/

// %%% luz 2003-06-26: If PLATFORM_TCPENABLESSL is defined, tcpReadData must be
//                     implemented in xptitcp.c
#ifndef PLATFORM_TCPENABLESSL

// enable SSL for socket
TcpRc_t tcpEnableSSL(SocketPtr_t pSocket, Bool_t aConnected) 
{
  return TCP_RC_ENOPROTOOPT; // not supported by default
}

#endif // not defined PLATFORM_TCPENABLESSL



/*****************************************************************************/
/*                                                                           */
/*             Function: Close the TCP/IP connection                         */
/*                                                                           */
/*****************************************************************************/

TcpRc_t tcpCloseConnection (SocketPtr_t pSocket) // i: socket to be closed
{
   TcpRc_t rc = TCP_RC_OK;
   int iTcpRc;
   SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;

   if (lSocket == -1L)
      return TCP_RC_ERROR;

   // %%% luz 2003-06-26: give platform code to do extra cleanup
   tcpBeforeSocketClose(pSocket);

   /********************/
   /* Close the socket */
   /********************/
   if (rc == TCP_RC_OK)
      {
      iTcpRc = closesocket (lSocket);
      rc = CHKERROR (iTcpRc);
      }

   if (pSocket) *pSocket = -1L; // reset the socket
   return rc;
}


/*****************************************************************************/
/*                                                                           */
/*    Function: Wait until a client connected to this server socket.         */
/*                                                                           */
/*****************************************************************************/

TcpRc_t tcpWaitforConnections (SocketPtr_t    pSocket,          // i: server socket
                               SocketPtr_t    pNewSocket,       // o: Connection socket
                               StringBuffer_t pchSenderAddress) // o: Client IP address
   {
   TcpRc_t rc = TCP_RC_OK;
   struct sockaddr_in socket_address;
   SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;

   if ((lSocket == -1L) || (lSocket == 0L))
      return TCP_RC_ERROR;

   else
      {
      /**************************************/
      /* Wait for a client to connect to me */
      /**************************************/
      int iSocketAddressLength = sizeof (socket_address);
      *pNewSocket = (long) accept (lSocket, (struct sockaddr *)&socket_address, &iSocketAddressLength);
      rc = CHKERROR (*pNewSocket);
      if (rc == TCP_RC_OK)
         {
         getSenderAddress (&socket_address, pchSenderAddress);
         }
      else
         {
         // avoid follow-up problems and set output variables to a default value
         *pNewSocket = (long) 0;
         pchSenderAddress [0] = '\0';
         }
      }
   return rc;
   }

/*****************************************************************************/
/*                                                                           */
/*    Function: receive a block of data from the communication partner.      */
/*                                                                           */
/*****************************************************************************/

// %%% luz 2003-06-26: If PLATFORM_TCPREADDATA is defined, tcpReadData must be
//                     implemented in xptitcp.c
#ifndef PLATFORM_TCPREADDATA

TcpRc_t tcpReadData (SocketPtr_t     pSocket,   // i: Socket
                     DataBuffer_t    pbBuffer,  // i: Data buffer
                     BufferSizePtr_t pcbBufferSize) // io: size of data buffer / # of bytes reveived
   {
   TcpRc_t rc = TCP_RC_OK;
   int iReceived;

   SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;

   if (lSocket == -1L)
      return TCP_RC_ERROR;

   /************************/
   /* Receive a data block */
   /************************/

   iReceived = recv (lSocket, (char *)pbBuffer, (int) *pcbBufferSize, 0);

   /************************************************/
   /* 0-byte packages indicate that the connection */
   /* was closed by the communication partner      */
   /************************************************/

   if (iReceived == 0)
      {
      pbBuffer [0] = '\0';
      *pcbBufferSize = 0L;
      rc = TCP_RC_EOF; // end-of-transmission
      }
   else if (iReceived < 0)
      {
      /*********************/
      /* An error occurred */
      /*********************/
      rc = CHKERROR (iReceived);
      }
   else
      {
      *pcbBufferSize = (BufferSize_t) iReceived; // return the # of bytes received.
      rc = TCP_RC_OK;
      }

   return rc;
}

#endif // not defined PLATFORM_TCPREADDATA


/*****************************************************************************/
/*                                                                           */
/*        Function: Send a block of data to the communication partner.       */
/*                                                                           */
/*****************************************************************************/

// %%% luz 2003-06-26: If PLATFORM_TCPSENDDATA is defined, tcpReadData must be
//                     implemented in xptitcp.c
#ifndef PLATFORM_TCPSENDDATA

TcpRc_t tcpSendData (SocketPtr_t        pSocket,      // i: socket
                     const DataBuffer_t pbBuffer,     // i: data buffer
                     BufferSize_t       cbBufferSize) // i: size of data buffer
   {
   TcpRc_t rc = TCP_RC_OK;
   int iTcpRc;
   SOCKET lSocket = pSocket ? (SOCKET) *pSocket : -1L;

   if (lSocket == -1L)
      return TCP_RC_ERROR; // invalid socket

   iTcpRc = send (lSocket,  (char *)pbBuffer, (int) cbBufferSize, 0);

   rc = CHKERROR (iTcpRc);
   if ((iTcpRc > 0) && (iTcpRc != (int) cbBufferSize))
       rc = TCP_RC_ERROR;
   return rc;
}

#endif // not defined PLATFORM_TCPSENDDATA


#ifndef PLATFORM_BEFORESOCKETCLOSE

// Nop in standard case
void tcpBeforeSocketClose(SocketPtr_t pSocket)
{
  // nop
}

#endif // not defined PLATFORM_BEFORESOCKETCLOSE
