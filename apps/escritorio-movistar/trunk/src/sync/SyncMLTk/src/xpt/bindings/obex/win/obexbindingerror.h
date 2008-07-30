/*************************************************************************/
/* module:          SyncML OBEX binding error header file.               */
/* file:            src/xpt/win/obex.obexbindingerror.h                  */
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
** Defines error codes used within the obex binding modules.
** Values are only valid WITHIN the obex bindings
*/
#ifndef OBEXBINDINGERROR_H
#define OBEXBINDINGERROR_H

// #define OBX_RC_OK                SML_ERR_OK

#define OBX_RC_OBEX_GENERAL_ERROR   SML_ERR_A_XPT_COMMUNICATION   // General obex error
#define OBX_RC_OBEX_INIT_FAILURE    SML_ERR_A_XPT_COMMUNICATION   // Failure during obex initialization
#define OBX_RC_COMMAND_FAILED       SML_ERR_A_XPT_COMMUNICATION   // Failure flowing an OBEX command
#define OBX_RC_SERVER_REG_FAILED    SML_ERR_A_XPT_COMMUNICATION   // Registration of the server failed

#define OBX_RC_SERVICE_ALREADY_REG  SML_ERR_A_XPT_COMMUNICATION   // Attempt to use a service name that is already registered.
#define OBX_RC_UNKNOWN_COMM_ROLE    SML_ERR_A_XPT_INVALID_STATE   // Unknown communction role.

#define OBX_RC_SOCKET               SML_ERR_A_XPT_COMMUNICATION   // General socket error
#define OBX_RC_SOCKET_BAD_HOST_NAME SML_ERR_A_XPT_COMMUNICATION
#define OBX_RC_SERVER_ACCEPT_FAILED SML_ERR_A_XPT_COMMUNICATION   // Failure when attempting an accept (in server mode)
#define OBX_RC_OBEX_TRANS_FAILURE   SML_ERR_A_XPT_COMMUNICATION   // General failure from transport layer

#define OBX_RC_OBEX_HEADER          SML_ERR_A_XPT_COMMUNICATION   // Error adding header to obex object
#define OBX_RC_OBEX_DATA_LENGTH     SML_ERR_A_XPT_COMMUNICATION   // Provided data != specified data length

#define OBX_RC_MEMORY_ERROR         SML_ERR_A_XPT_MEMORY          // Non specific memory error

#define OBX_RC_MEMORY_ERROR_ALLOC   SML_ERR_A_XPT_MEMORY          // Failure during Alloc
#define OBX_RC_MEMORY_NULL_PNTR     SML_ERR_A_XPT_INVALID_PARM    // Null pointer on function call

#define OBX_RC_GENERAL_ERROR        SML_ERR_A_XPT_COMMUNICATION   // Non specific general error


// ERROR handling

#define OBX_ERRORMSG_NULL_POINTER            0           // Null pointer passed to a function.
#define OBX_ERRORMSG_OBEX_HEADER             1           // Error adding a header to an obex object
#define OBX_ERRORMSG_OBEX_DATA_LENGTH        2           // Error adding a header to an obex object
#define OBX_ERRORMSG_UNKNOWN_TRANSPORT_TYP   3           // Uknown transport type
#define OBX_ERRORMSG_SOCKET_BAD_HOST_NAME    4           // Unable to resolve.
#define OBX_ERRORMSG_MEMORY_ALLOC            5           // Memory alloc error
#define OBX_ERRORMSG_OBEX_TRANSPORT_FAILURE  6           // Failure while attempting to invoke transport.
#define OBX_ERRORMSG_BUFFER_LENGTH           7           // Bad length provided
#define OBX_ERRORMSG_BAD_SERVICE_TYPE        8           // Bad service type

static const char *ERRORS[] = {
   "A null pointer was passed as an argument to function %s.  Variable name %s.",
   "An error occured in %s while adding header %s to an OBEX object.",
   "The length specified in the document information did not match the amount of data provided to send.",
   "The transport type was unknown.  Normally this is either OBEX_TRANS_INET or OBEX_TRANS_IRDA",
   "Unable to resolve host name '%s'.",
   "An error allocating memory in function %s for variable %s has occured.",
   "The OBEX transport layer returned an unexpected return code from function call %s.",
   "The specified buffer or document length of %d in function %s is inappropriate",
   "The service type was found to be bad in function %s."
};

#endif
