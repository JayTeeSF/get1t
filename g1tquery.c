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
#include <string.h>	// for strcpy, strcmp
#include <assert.h>	// for assert

#include "g1tdebug.h"
#include "get1t.h"
#include "g1thash.h"
#include "g1ttok.h"

// globals from g1tmain
extern int printIndividualCounts;
extern int n;
extern int doCooc;
extern wordBhv doWords;
extern int checkWords;

// local functions
static int addQuery(xcCounters *counters, char **wstarts);
static int addTokPair(xcCounters *counters, char **wstarts);

// local definitions
#define qinccount 10000

xcCounters *makeCounters(const char *searchfilename)
{
	xcCounters *counters;
	FILE *inF;
	char *linebuf;
	char *readline;
	size_t lbsize;
	int len, lines;
	char mybuf[TENK];
	char **wstarts;

	// allocate places for everything
	counters = malloc(sizeof(xcCounters));
	counters->bigcount = 0;
	if (doCooc)
	{
		counters->queryLength = 2;
		checkWords = 1;
	}
	else
	{
		counters->queryLength = n;
		checkWords = 0;
	}

	if(doWords != dontCount)
	{
		counters->wordTable = hshinit(hashexact, rehashexact, exactcmp, dupexact, freeexact, 0);
	}
	else
	{
		counters->wordTable = NULL;
	}

	if (n > 1) {
		counters->queries = hshinit(hashset, rehashset, setcmp, dupset, freeset, 0);
	} else {
		counters->queries = hshinit(hashexact, rehashexact, exactcmp, dupexact, freeexact, 0);
	}
		

	if(printIndividualCounts)
	{
		counters->wildMatches = hshinit(hashset, rehashset, setcmp, dupset, freeset, 0);
	}
	else
	{
		counters->wildMatches = NULL;
	}

	//initialise query-reading loop
	linebuf = malloc(TENK);
	lbsize = 0;
	lines = 0;

	//allocate 2 extra wstarts, one for the final ending and one because cleanline() expects it
	wstarts = malloc((counters->queryLength + 2) * sizeof(char *));
	strcpy(mybuf, searchfilename);
	fprintf(stderr, "Reading queries from \'%s\': ", mybuf);
	inF = fopen(searchfilename, "r");
	while(!(feof(inF) || ferror(inF)))
	{
		readline = fgets(linebuf, TENK, inF);
		/* did we get any data? */
		if(readline == NULL || feof(inF))
		{
#ifdef DEBUG
			printf("Breaking. readline was %p\n", readline);
#endif
			break;
		}

		lines ++;
		if(lines % qinccount == 0)
		{
			fprintf(stderr, ".");
		}
		
#ifdef DEBUG
		printf("\nProcessing query '%s'\n", linebuf);
#endif
		len = cleanline(linebuf, TENK, wstarts, NULL, NULL);
		if (len != counters->queryLength)
		{
			fprintf(stderr, "\nQuery wrong length (%d)\n", len);
			continue;
		}
	
		if (doCooc)
		{
			addTokPair(counters, wstarts);
		}
		else
		{
			addQuery(counters, wstarts);
		}
	}
	fclose(inF);
	free(wstarts);
	free(linebuf);
	fprintf(stderr, " done.\n");
	
	return counters;
}

int addQuery(xcCounters *counters, char **wstarts)
{
	exactitem exact;
	setitem set;
	setptr setp;
	int i;
	void* nextLevel = NULL;
	/* The hash table we're inserting into. Gets deeper
	 * with each iteration */
	hshtbl* currentQueryLevel = counters->queries;

#ifdef CURRENTDEBUG
			printf("Counting from 0 to %d\n", n);
#endif
	for(i=0;i<n;i++)
	{
		if (i <= n - 2)
		{
#ifdef CURRENTDEBUG
			printf("Searching for word");
			printf(" '%s' in pos %d...", wstarts[i], i);
#endif
			set.search = wstarts[i];
			nextLevel = hshfind(currentQueryLevel, &set);
			if (nextLevel == NULL)
			{
#ifdef CURRENTDEBUG
				printf("... not found\n");
#endif
				if (i < n - 2)
				{
					set.pl.isterminal = 0;
				}
				else /* i == n - 2 */
				{
					set.pl.isterminal = 1;
				}
				setp = hshinsert(currentQueryLevel, &set);
				if (setp == NULL)
				{
					printf("Failure to insert ");
					printf("into hash table.");
					printf(" Aborting.");
					assert(0);
				}
#ifdef DEBUG
				printf("Hash inserted at %p into %p\n",
					setp, currentQueryLevel);
#endif
				nextLevel = setp;
			}
#ifdef DEBUG
			else
			{
				setp = (setitem*) nextLevel;
				printf("... found a hash");
				printf(" %p\n", setp->pl.set);
			}
#endif
		} /* i == n - 1 */
		else if (i == (n - 1))
		{
			exact.search = wstarts[i];
			if (hshfind(currentQueryLevel, &exact) ==
				NULL)
			{
#ifdef CURRENTDEBUG
				printf("Inserting a leaf hash for");
				printf(" word '%s'...", wstarts[i]);
#endif
				/* we insert an exact item */
				exact.count = NULL;	// dupexact will allocate the memory
				hshinsert(currentQueryLevel, &exact);
			}
			nextLevel = NULL;
		}
#ifdef DEBUG
		else
		{
			printf("Reached a part of the loop that ");
			printf("should be impossible, i = %d\n", i);
			assert(0 == 1);
		}
#endif
		if (nextLevel != NULL)
		{
			setp = (setitem*) nextLevel;
			currentQueryLevel = setp->pl.set;
		}

		// don't look for wildcards, instead put their information in the appropriate slot
		if((doWords != dontCount) && strcmp(wstarts[i], WILDCARD) != 0)
		{
			exact.search = wstarts[i];
			exact.count = NULL;	// get dupexact to create the memory for us
			hshinsert(counters->wordTable, &exact);
		}
	}
#ifdef CURRENTDEBUG
	printf("\nQuery tree as of the end of query\n");
	printQueries(counters->queries, n,
			&exact, &set);
#endif

	return 0;
}

int addTokPair(xcCounters *counters, char **wstarts)
{
	exactitem exact;
	exactptr exactp;
	setitem set;
	setptr setp;
	int i, isnew;
	/* The hash table we're inserting into. Gets deeper
	 * with each iteration */
	hshtbl* secondLevel;
	long long *thecount;

	thecount = NULL;
	isnew = 0;
	for(i=0;i<2;i++)
	{
		// first insert into the table of individual words
		if (doWords != dontCount)
		{
			exact.search = wstarts[i];
			if(!(exactp = hshfind(counters->wordTable, &exact)))
			{
				exact.count = NULL; // get dupexact to create the memory for us
				hshinsert(counters->wordTable, &exact);
			}
		}

		// now into the table of query starts
		set.search = wstarts[i];
#ifdef DEBUG
		printf("Searching for word");
		printf(" '%s' in pos %d...", wstarts[i], i);
#endif
		setp = hshfind(counters->queries, &set);
		if(setp == NULL)
		{
#ifdef DEBUG
			printf("... not found\n");
#endif
			// insert a new node into the query starts then
			// terminal set, so indicate that
			set.pl.isterminal = 1;
			setp = hshinsert(counters->queries, &set);
			assert(setp != NULL);
#ifdef DEBUG
			printf("Hash inserted at %p into %p\n",
				setp, counters->queries);
#endif
		}
#ifdef DEBUG
		else
		{
			printf("... found a hash");
			printf(" %p\n", setp->pl.set);
		}
#endif
		// now we must ensure the other item is in the hash just dealt with
		secondLevel = setp->pl.set;
		exact.search = wstarts[1-i];
		if(hshfind(secondLevel, &exact) == NULL)
		{
#ifdef DEBUG
			printf("Inserting a leaf hash for");
			printf(" word '%s'...", wstarts[1-i]);
#endif
			// allocate the counter here so it will be the same for both orderings
			if(thecount == NULL)
			{
				thecount = malloc(sizeof(long long));
				assert(thecount != NULL);
				*thecount = 0;
				isnew = 1;
			}
			exact.count = thecount;
			/* we insert an exact item */
			exactp = hshinsert(secondLevel, &exact);
#ifdef DEBUG
			printf("at %p\n", exactp);
#endif
		}
#ifdef DEBUG
		else if(isnew)
		{
			assert(0);
		}
		else
		{
			printf("Duplicate query: \'%s\' & \'%s\'\n", wstarts[i], wstarts[1-i]);
		}
#endif
	}
#ifdef DEBUG
	printf("\nQuery tree as of the end of query\n");
	printQueries(counters->queries, n,
			&exact, &set);
#endif

	return 0;
}

int destroyCounters(xcCounters *counters)
{

	if(counters == NULL)
	{
		return 0;
	}

	if(counters->wordTable)
	{
		hshkill(counters->wordTable);
		counters->wordTable = NULL;
	}
	if (counters->queries)
	{
		hshkill(counters->queries);
		counters->queries = NULL;
	}
	if(counters->wildMatches)
	{
		hshkill(counters->wildMatches);
		counters->wildMatches = NULL;
	}
	free(counters);
	return 0;
}
