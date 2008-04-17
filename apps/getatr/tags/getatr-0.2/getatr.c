/*
    Scan and print all the PC/SC devices available.
    Copyright (C) 2007 J. Félix Ontañón <fontanon@emergya.info>

    Totally based in pcsc_scan by Ludovic Rousseau.
    Copyright (C) 2001-2003  Ludovic Rousseau <ludovic.rousseau@free.fr>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#ifdef __APPLE__
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#else
#include <winscard.h>
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define TIMEOUT 1000    /* 1 second timeout */

/* command used to parse (on screen) the ATR */
#define ATR_PARSER "ATR_analysis"

void usage(void)
{
        printf("usage: getatr [-a] [-h]\n");
        printf("  -a : print ATR only\n");
        printf("  -h : this help\n");
} /* usage */


int main(int argc, char *argv[])
{
        int current_reader = 0;
        LONG rv;
        SCARDCONTEXT hContext;
        SCARD_READERSTATE_A *rgReaderStates_t = NULL;
        DWORD dwReaders, dwReadersOld;
        LPTSTR mszReaders = NULL;
        char *ptr, **readers = NULL;
        int nbReaders, i;
        char atr[MAX_ATR_SIZE*3+1];     /* ATR in ASCII */
	int opt;
	int atr_only = FALSE;

        while ((opt = getopt(argc, argv, "ah")) != EOF)
        {
                switch (opt)
                {
                        case 'a':
                                atr_only = TRUE;
                                break;

                        case 'h':
                                default:
                                usage();
                                return 1;
                                break;
                }
        }

        if (argc - optind != 0)
        {
                usage();
                return 1;
        }

        rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
        if (rv != SCARD_S_SUCCESS)
        {
                printf("SCardEstablishContext: Cannot Connect to Resource Manager %lX\n", rv);
                return 1;
        }
        /* free memory possibly allocated in a previous loop */
        if (NULL != readers)
        {
                free(readers);
                readers = NULL;
        }

        if (NULL != rgReaderStates_t)
        {
                free(rgReaderStates_t);
                rgReaderStates_t = NULL;
        }

        /* Retrieve the available readers list.
         *
         * 1. Call with a null buffer to get the number of bytes to allocate
         * 2. malloc the necessary storage
         * 3. call with the real allocated buffer
         */
        rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
        if (rv != SCARD_S_SUCCESS)
        {
                printf("SCardListReader: %lX\n", rv);
        }
        dwReadersOld = dwReaders;

        /* if non NULL we came back so free first */
        if (mszReaders)
        {
                free(mszReaders);
                mszReaders = NULL;
        }

        mszReaders = malloc(sizeof(char)*dwReaders);
        if (mszReaders == NULL)
        {
                printf("malloc: not enough memory\n");
                return 2;
        }

        rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
        if (rv != SCARD_S_SUCCESS)
        {
                printf("SCardListReader: %lX\n", rv);
        }

        /* Extract readers from the null separated string and get the total
         * number of readers */
        nbReaders = 0;
        ptr = mszReaders;
        while (*ptr != '\0')
        {
                ptr += strlen(ptr)+1;
                nbReaders++;
        }

        if (nbReaders == 0)
        {
                printf("ScardReader not found\n");
		return -1;
        }

        /* allocate the readers table */
        readers = calloc(nbReaders, sizeof(char *));
        if (NULL == readers)
        {
                printf("Not enough memory for readers table\n");
                return 3;
        }

        /* fill the readers table */
        nbReaders = 0;
        ptr = mszReaders;
        while (*ptr != '\0')
        {
                readers[nbReaders] = ptr;
                ptr += strlen(ptr)+1;
                nbReaders++;
        }

        /* allocate the ReaderStates table */
        rgReaderStates_t = calloc(nbReaders, sizeof(* rgReaderStates_t));
        if (NULL == rgReaderStates_t)
        {
                printf("Not enough memory for readers states\n");
                return -1;
        }

        /* Set the initial states to something we do not know
         * The loop below will include this state to the dwCurrentState
         */
        for (i=0; i<nbReaders; i++)
        {
                rgReaderStates_t[i].szReader = readers[i];
                rgReaderStates_t[i].dwCurrentState = SCARD_STATE_UNAWARE;
        }

        /* Wait endlessly for all events in the list of readers
         * We only stop in case of an error
         */
        rv = SCardGetStatusChange(hContext, TIMEOUT, rgReaderStates_t, nbReaders);

        for (current_reader=0; current_reader < nbReaders; current_reader++)
        {
               /* Specify the current reader's number and name */
	       if (!atr_only) {
               		printf("Reader %d: %s\n", current_reader, rgReaderStates_t[current_reader].szReader);
 	       		printf("  ATR: ");
	       }

        	/* Dump the ATR or None*/
	        if (rgReaderStates_t[current_reader].cbAtr > 0)
	        {
	                if (rgReaderStates_t[current_reader].cbAtr)
	                {
				for (i=0; i<rgReaderStates_t[current_reader].cbAtr; i++)
	                        sprintf(&atr[i*3], "%02X ",
	                        rgReaderStates_t[current_reader].rgbAtr[i]);
	
        	                atr[i*3-1] = '\0';
	                }
	                else
				atr[0] = '\0';
			
	                printf("%s\n", atr);
		}
		else
			printf("None\n");
	}
	return 0;
}
