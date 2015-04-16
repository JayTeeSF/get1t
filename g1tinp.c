/*

    Get 1T: retrieve relevant counts from n-gram database.
    Copyright (C) 2007 Tobias Hawker, Mary Gardiner and Andrew Bennetts

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include <stdlib.h>	// for malloc
#include <sys/types.h>	// types required by readdir, dirent
#include <dirent.h>	// for readdir, dirent
#include <assert.h> // for assert
#include <libgen.h>	// for dirname, basename
#include <string.h>	// for strncmp, strcmp, strcpy
#include <ctype.h>	// for isdigit
#ifndef NOZLIB
#include <zlib.h>
#endif

#include "get1t.h"
#include "g1ttok.h"
#include "g1tproc.h"
#include "g1tdebug.h"

// global variables from g1tmain
extern wordBhv doWords;
extern int n;
extern int doCooc;

// local functions
static void findFileHits(const char *path, xcCounters *counters, FILE *wildF);
static int isDBName(const char *name);
static int inpPrep(int needCopyBuf);
static int inpCleanup(void);

// local definitions
#define ONEK 1024
#define ONEMEG 1048576
#define blocksize 102400
#define inccount 100000
#ifdef NOZLIB
#define initialsize 512 * ONEMEG
#endif /* NOZLIB */

// filescope variables
#ifndef NOZLIB
static char *bufs[2] = {NULL, NULL};
static int curbuf;
#else /* NOZLIB */
static char *bigbuf = NULL;
static size_t bbsize = 0;
#endif /* NOZLIB */
// allocated here, used in g1tproc
/*
extern hshtbl **hitstarts;
extern long long **counthits;
*/
static char **tstarts = NULL;
char* originalline = NULL;
int originallinelength = 0;

int findHits(const char *datadir, xcCounters *counters, FILE *wildF)
{
	DIR *dir;
	struct dirent *dent;
	char path[ONEK];
	int result;

	inpPrep((wildF != NULL));

	// open the directory supplied
	dir = opendir(datadir);
	if(dir==NULL)
	{
		perror("ERROR - Problem opening data directory");
		return -1;
	}

	// iterate through the entries
	sprintf(path, "%s/.", datadir);
	fprintf(stderr, "Processing data from \'%s\'; Each . represents %d instances processed.\n", dirname(path), inccount);
	while((dent = readdir(dir)))
	{
		if (!isDBName(dent->d_name))
		{
//			fprintf(stderr, "-");
			continue;
		}
//		fprintf(stderr, "+");
		sprintf(path, "%s/%s", datadir, dent->d_name);
		findFileHits(path, counters, wildF);
	}
	result = closedir(dir);
	if(result != 0)
	{
		perror("WARNING - Problem encountered closing data dir");
	}

	inpCleanup();

	return 0;
}

/* checks that the supplied name exactly matches the pattern:
    vocab(_cs)?(.gz)?        for unigrams
    [0-9]gm-[0-9]{4}(.gz)?   for 2-5 grams
   Although regular expressions are not used, as it is simple enough as-is.
*/
int isDBName(const char *name)
{
	int i;

	//there are special cases for unigrams
	if(n == 1) 
	{
		if(strncmp("vocab", name, 5) != 0)
		{
			return 0;
		}
#ifndef NOZLIB
		if(name[5] == '\0' || (strcmp("_cs", name + 5) == 0))
		{
			return 1;
		}
#endif /* NOZLIB */
		if((strcmp(".gz", name + 5) == 0) || (strcmp("_cs.gz", name + 5) == 0))
		{
			return 1;
		}
		return 0;
	}

	// does it start with the pattern length?
	if(n != (name[0] - '0'))
	{
		return 0;
	}
	// are the next 3 characters "gm-"?
	if(strncmp("gm-", name + 1, 3) != 0)
	{
		return 0;
	}
	// and the following 4 characters all digits?
	for(i=4;i<8;i++)
	{
		if(!isdigit(name[i]))
		{
			return 0;
		}
	}
	// the acceptable endings depends on the read method
#ifndef NOZLIB
	// if we are using incremental decompression, no .gz is OK
	if(name[8] == '\0')
	{
		return 1;
	}
#endif /* NO ZLIB */
	// in either case, ".gz" is OK
	if(strcmp(".gz", name + 8) == 0)
	{
		return 1;
	}

	// if still here, the end was bad
	return 0;
}

void findFileHits(const char *path, xcCounters *counters, FILE *wildF)
{
	// common local variables
	char *bufpos, *curpos;
	char dbuf[ONEK];
	long long curcount;
	int read, len, linenum, found;
	exactitem exact;
	setitem set;
	// divergent local variables
#ifndef NOZLIB
	gzFile gzF;
	int overhang;
#else	/* NOZLIB */
	char cmdbuf[TENK];
	FILE *pipe;
	size_t bufoff;
#endif	/* NOZLIB */

	// initialise: open the file/pipe
	strcpy(dbuf, path);
	fprintf(stderr, "%s: ", basename(dbuf));
#ifndef NOZLIB
	gzF = gzopen(path, "r");
	if (!gzF)
	{
		fprintf(stderr, "\nError opening \'%s\'", path);
		perror("");
		return;
	}
#else	/* NOZLIB */
	sprintf(cmdbuf, "gzip -dc %s", path);
	pipe = popen(cmdbuf, "r");
	if (pipe == NULL)
	{
		perror("Problem opening pipe to gzip");
		return;
	}

	// read all the data in
	fprintf(stderr, "buffering: ");
	bufpos = bigbuf;
	while(!feof(pipe) && !ferror(pipe))
	{
		// have we got room for another read?
		while(((bufpos - bigbuf) + blocksize) > bbsize)
		{
			// no, double the buffer size
			bufoff = bufpos - bigbuf;
			bigbuf = realloc(bigbuf, bbsize * 2);
			bbsize *= 2;
			bufpos = bigbuf + bufoff;
		}
		// read the block
		read = fread(bufpos, sizeof(char), blocksize, pipe);
		bufpos += read;
	}
	if(ferror(pipe))
	{
		perror("\nError reading from gzip process");
		pclose(pipe);
		return;
	}
	pclose(pipe);
	*bufpos = '\0';

	fprintf(stderr, "done.");
#endif	/* NOZLIB */
	
	// initialise for the loop
	linenum = 0;
#ifndef NOZLIB
	bufpos = bufs[curbuf];
	curpos = bufpos;
#else	/* NOZLIB */
	curpos = bigbuf;
#endif	/* NOZLIB */

	// do the entry processing loop
	fprintf(stderr, "Processing: ");
#ifndef NOZLIB
	while (1)
#else	/* NOZLIB */
	while (curpos < bufpos)
#endif	/* NOZLIB */
	{
		if(wildF != NULL)
		{
			len = cleanline(curpos, (bufpos - curpos), tstarts, &originalline, &originallinelength);
		}
		else
		{
			len = cleanline(curpos, (bufpos - curpos), tstarts, NULL, NULL);
		}
		if(len == (n + 1))
		{
#ifdef DEBUG
			int i;
			printf("Got input line of %d tokens:", len);
			for(i=0;i<len;i++)
			{
				printf(" %s", tstarts[i]);
			}
			printf("\n");
#endif
		}
		else
		{
#ifdef DEBUG
			printf("Got BAD line of %d tokens:", len);
			printf(" %s\n", tstarts[0]);
#endif
#ifndef NOZLIB
			// most likely due to needing more data
			// first move any partial line to the front of the alternative buffer
			overhang = bufpos - curpos;
			if (overhang > 0)
			{
				memcpy(bufs[1 - curbuf], curpos, overhang);
			}
			// switch active buffer
			curbuf = 1 - curbuf;
			// attempt to read from the compressed file
			read = gzread(gzF, bufs[curbuf] + overhang, blocksize);
			// was there an error?
			if (read == -1)
			{
				perror("\nProblem reading from file");
				return;
			}
			// have we hit the end of file?
			else if (read == 0)
			{
				break;
			}
			curpos = bufs[curbuf];
			bufpos = curpos + overhang + read;

			// now attempt to get the line again
			if(wildF != NULL)
			{
				len = cleanline(curpos, (bufpos - curpos), tstarts, &originalline, &originallinelength);
			}
			else
			{
				len = cleanline(curpos, (bufpos - curpos), tstarts, NULL, NULL);
			}
			if(len == (n + 1))
			{
#ifdef DEBUG
				printf("RETRY: Got input line of %d tokens:", len);
				for(i=0;i<len;i++)
				{
					printf(" %s", tstarts[i]);
				}
				printf("\n");
#endif
			}
			else
			{
				fprintf(stderr, "\nRETRY: Got BAD line of %d tokens: '%s'", len, tstarts[0]);
			}
#else	/* NOZLIB */
			curpos++;
			continue;
#endif	/* NOZLIB */
		}
		
		linenum++;
		if((linenum % inccount) == 0)
		{
			fprintf(stderr, ".");
		}
		/* parse the count value, we won't need it again */
		curcount = atoll(tstarts[n]);

		// add the current ngram count to the count of all ngrams
		counters->bigcount += curcount;

		// see if the words are of interest
		if(doWords != dontCount)
		{
			incrementWords(counters->wordTable, curcount, tstarts);
		}
		
#ifdef DEBUG
		printf("Beginning to update all queries matching this line\n");
#endif
		if (doCooc)
		{
			found = incrementCooc(counters->queries, curcount, tstarts);
		}
		else
		{
			found = incrementQueries(counters->queries, curcount,
					tstarts, 0, counters->queryLength,
					&exact, &set);
		}
		if (wildF != NULL && found > 0)
		{
			fprintf(wildF, "%s\n", originalline);
		}

#ifdef DEBUG
		printf("Query tree as of this line:\n");
		printQueries(counters->queries, counters->queryLength,
				&exact, &set);
#endif
		curpos = tstarts[n + 1];
	}

	fprintf(stderr, " done.\n");
#ifdef DEBUG
	fflush(stdout);
#endif
#ifndef NOZLIB
	gzclose(gzF);
#endif	/* NOZLIB */
}

int inpPrep(int needCopyBuf)
{
#ifndef NOZLIB
	int i;
#endif
	
	if(tstarts == NULL)
	{
		tstarts = malloc((n + 2) * sizeof(char *));
		assert(tstarts != NULL);
	}
	if (needCopyBuf)
	{
		originallinelength = ONEK;
		originalline = malloc(sizeof(char) * originallinelength);
	}
	else
	{
		originalline = NULL;
		originallinelength = 0;
	}
#ifndef NOZLIB
	for (i=0;i<2;i++)
	{
		if (bufs[i] == NULL)
		{
			bufs[i] = malloc(2 * blocksize);
			assert(bufs[i] != NULL);
		}
	}
	curbuf = 0;
#else /* NOZLIB */
	if(bigbuf == NULL)
	{
		bigbuf = malloc(initialsize * sizeof(char));
		assert(bigbuf != NULL);
		bbsize = initialsize * sizeof(char);
	}
#endif /* NOZLIB */
	procPrep();

	return 0;
}

int inpCleanup()
{
#ifndef NOZLIB
	int i;
#endif

	// clean up allocated buffers
#ifndef NOZLIB
	for (i=0; i<2; i++)
	{
		free(bufs[i]);
		bufs[i] = NULL;
	}
#else /* NOZLIB */
	free(bigbuf);
	bigbuf = NULL;
	bbsize = 0;
#endif /* NOZLIB */
	free(tstarts);
	tstarts = NULL;
	if (originalline != NULL)
	{
		free(originalline);
		originalline = NULL;
		originallinelength = 0;
	}

	return 0;
}
