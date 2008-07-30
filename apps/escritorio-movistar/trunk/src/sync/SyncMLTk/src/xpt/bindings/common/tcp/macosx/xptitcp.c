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

// %%% luz:2003-04-17: Note this is Synthesis SySync framework specific
#ifdef PROGRESS_EVENTS
  // we need SySync-specific include to allow progress monitoring
  #include "global_progress.h"
#endif


/* eof */
