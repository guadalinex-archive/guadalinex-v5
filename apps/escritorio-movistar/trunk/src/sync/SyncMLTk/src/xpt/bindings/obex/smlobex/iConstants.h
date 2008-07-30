/*
** Defines internal obex constants.
*/

#ifndef OBEXINTERNALCONSTANTS_H
#define OBEXINTERNALCONSTANTS_H


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
#ifdef _WIN32
 #define WIN32_LEAN_AND_MEAN

 #include <windows.h>
 #include <winsock.h>
 #include <stdlib.h>
 #include <limits.h>

 #define serror() _describeWin32SocketError();
 #define sclose closesocket
 #define sread(fd, buf, len)  recv((fd), (buf), (len), 0)
 #define swrite(fd, buf, len) send((fd), (buf), (len), 0)
 #define sleep(seconds) SleepEx((seconds) * 1000, TRUE)
#elif defined(__PALM_OS__)
  // %%% added by luz
  #include <netinet_in.h>
  #include <sys_socket.h>
  // no special includes required, it seems
#else
 #include <unistd.h>
 #include <sys/ioctl.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <netdb.h>
 #include <limits.h>
 #include <stdlib.h>
 #include <math.h>
 #include <stdio.h>

 #define serror perror
 #define sclose close
 #define sread read
 #define swrite write

 #undef max
 #define max(a,b) ((a) > (b) ? (a) : (b))
 #undef min
 #define min(a,b) ((a) < (b) ? (a) : (b))

 #define SOCKET_ERROR   -1

#endif


/*
** Grab general obex constants.
*/
#include <obex/constants.h>
#include <buffer.h>

/*
** Debug output.
*/
#include <debug.h>

/*
** Errors
*/
#include <obex/error.h>

#define OBEX_HEADER_SEND_ALL                 INT_MAX

#define OBX_CONNECT_META_FLAGS               0x00
#define OBX_CONNECT_META_VERSION             OBEX_VERSION
#define OBX_CONNECT_META_MAX_PACKET_LENGTH   OBEX_DEFAULT_PACKET_LENGTH

#define OBX_SETPATH_META_CONSTANTS           0x00
#define OBX_SETPATH_META_FLAGS               0x00

/*
** Every frame begins thus
*/
struct iobxCommonFrameHeader {
	unsigned char  opcode;  /* 8 bits         */
	unsigned short length;  /* 16 bits        */
};

/*
** Connect header
*/
struct iobxConnectHeader {
	unsigned char  version; /* 8 bits         */
	unsigned char  flags;   /* 8 bits         */
	unsigned short mtu;     /* 16 bits        */
};

/*
** Stream
*/
typedef enum {
   STREAM_UNOPEN,
   STREAM_SENDING,
   STREAM_RECEIVING,
   STREAM_FINISHED,
   STREAM_CLOSED
} ObxStreamState;

struct iobxStream {
   ObxCommand     cmd;			      /* Command associated with the stream           */
   ObxHandle      *handle;          /* Where we get our fd                          */
   ObxBuffer      *data;            /* Data waiting to go...                        */
   ObxObject      *request;         /* Our, private request.                        */
   ObxStreamState state;            /* UNOPEN, SENDING, RECEIVING, FINISHED, CLOSED */
};

#endif
