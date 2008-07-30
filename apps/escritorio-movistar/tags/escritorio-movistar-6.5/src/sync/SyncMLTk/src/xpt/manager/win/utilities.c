/*************************************************************************/
/* module:          SyncML Communication Protocol platform-dependent code*/
/* file:            src/xpt/all/win/utilities.c                          */
/* target system:   Windows                                              */
/* target OS:       Windows                                              */
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
 * This file implements the platform-dependent code for the xpt routing layer.
 */

#include "syncml_tk_prefix_file.h" // %%% luz: needed for precompiled headers in eVC++

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef WINCE // %%% luz adaptions for PocketPC
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <xpt.h>
#include <xptTransport.h>

#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

#include <utilities.h>

#ifndef WINCE // %%% luz adaptions for PocketPC

#define DESCRIBELASTWIN32ERROR(x)   describeWin32Error((x), GetLastError())

static void describeWin32Error(const char *msg, long errcode) {
   char *msgbuffer;
   DWORD rc = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
                            | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            NULL, errcode, 0, (char *) &msgbuffer, 0, NULL);
   if (!rc) {
      fprintf(stderr, "Received error code %ld from FormatMessage() while"
                      " trying\nto format message for error code %ld:  %s\n",
                      GetLastError(), errcode, msg);
      return;
   }

   fprintf(stderr, "%s (code %ld):  %s", msg, errcode, msgbuffer);
   LocalFree(msgbuffer);
}

static int findPropertyFile(const char *name, char *filename) {
   struct stat st;

   // First look in current directory
   strcpy(filename, name);
   if (!stat(filename, &st)) return 1;

#ifdef _WIN32
   // Now try the directory containing the executable program
   if (GetModuleFileName(NULL, filename, FILENAME_MAX)) {
      char *p = strrchr(filename, '\\');
      if (p && p+1-filename + strlen(name) < FILENAME_MAX) {
         strcpy(p+1, name);
         if (!stat(filename, &st)) return 1;
      }
   }
#endif

   return 0;      // Couldn't find the file
}

char **getLibrariesToLoad(void) {
   // Read the list of libraries to load from a "properties"-type file.
   char propertyFile[FILENAME_MAX];
   char line[FILENAME_MAX+1];
   FILE *file;

   char **list = NULL;
   size_t listLen  = 0;
   size_t listSize = 0;


   if (!findPropertyFile("dynamicTransports", propertyFile)) {
      fprintf(stderr, "Unable to find dynamic transport property file \"%s\"\n", "dynamicTransports");
      return NULL;
   }

   file = fopen(propertyFile, "r");
   if (!file) {
      fprintf(stderr, "Error opening dynamic transport property file \"%s\": %s\n", propertyFile, strerror(errno));
      return NULL;
   }

   while (fgets(line, sizeof line - 1, file)) {
      size_t lineLen = strlen(line);
      if (line[lineLen-1] != '\n') {
         fprintf(stderr, "Line too long in dynamic transport property file: %s...\n", line);
         fclose(file);
         return NULL;
      }

      // Ignore comment lines and empty lines
      if (*line == '#' || *line == '\n') continue;

      // Replace trailing newline with a null
      line[lineLen-1] = '\0';
      // The lineLen variable now includes the trailing null character

      // Make sure list is big enough to hold this new entry
      if (listLen + 1 >= listSize) {
         listSize += 5;
         list = realloc(list, listSize * sizeof(char *));
         if (!list) {
            perror("Unable to allocate storage for dynamic library list");
            fclose(file);
            return NULL;
         }
      }

      // Get storage for this new list entry
      list[listLen] = malloc(lineLen);
      if (!list[listLen]) {
         size_t i;
         perror("Unable to allocate storage for dynamic library name");
         fclose(file);
         for (i=0; i<listLen; ++i) free(list[i]);
         free(list);
         return NULL;
      }

      // Copy to item into list
      memcpy(list[listLen++], line, lineLen);
   }

   fclose(file);

   if (!list) list = malloc(sizeof(char *));

   list[listLen] = NULL;   // list is definitely big enough for this entry

   return list;
}


void releaseLibraryList(char **list) {
   char **p = list;

   if (!list) return;

   while (*p) {
      free(*p++);
   }

   free(list);
}

void *loadLibrary(const char *libname) {
   HMODULE handle = LoadLibrary(libname);
   if (!handle) {
      DESCRIBELASTWIN32ERROR("LoadLibrary");
      return NULL;
   }

   return handle;
}

void unloadLibrary(void *handle) {
   if (!FreeLibrary((HMODULE) handle))
      DESCRIBELASTWIN32ERROR("FreeLibrary");
}

void *lookupLibrarySymbol(const char *symbol, int arguments, void *handle) {
   FARPROC symbolAddr;

   // Try looking up plain symbol first
   symbolAddr = GetProcAddress((HMODULE) handle, symbol);

   if (!symbolAddr) {
      char symbolName[270];
      if (strlen(symbol) > 256) return NULL;

      // Try with a leading underscore (the cdecl calling convention)
      sprintf(symbolName, "_%s", symbol);

      symbolAddr = GetProcAddress((HMODULE) handle, symbolName);

      if (!symbolAddr) {
         // Try with a leading underscore and trailing decoration for the
         // stdcall calling convention (Microsoft version).
         sprintf(symbolName, "_%s@%d", symbol, arguments * 4);
         symbolAddr = GetProcAddress((HMODULE) handle, symbolName);

         if (!symbolAddr) {
            // Try with a no underscore and trailing decoration for the
            // stdcall calling convention (gcc version).
            sprintf(symbolName, "%s@%d", symbol, arguments * 4);
            symbolAddr = GetProcAddress((HMODULE) handle, symbolName);

            if (!symbolAddr) DESCRIBELASTWIN32ERROR("GetProcAddress");
         }
      }
   }

   return symbolAddr;
}

#endif

#if defined(TRACE_TO_STDOUT) && !defined(EXTERNAL_LOCALOUTPUT)
void localOutput(const char *format, va_list args) {
   vprintf(format, args);
}
#endif

/* eof */