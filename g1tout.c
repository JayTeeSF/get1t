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

#include <string.h>	// for strcmp
#include <stdlib.h>	// for malloc

#include "get1t.h"
#include "g1thash.h"
#include "g1tdebug.h"

// globals from g1tmain
extern int printZeroCounts;
extern int doCooc;

// local types
typedef struct rationer
{
	char *firstWord;
	hshtbl *queries;
} rationer, *rationptr;

// local functions
static int writeexact(void *item, void *datum, void *xtra);
static int writeset(void *item, void *datum, void *xtra);
static int writequery(void *item, void *datum, void *xtra);

int writeTotal(FILE *outF, xcCounters *counters)
{
	fprintf(outF, "%lld\n", counters->bigcount);
	return 0;
}

int writeWordCounts(FILE *outF, xcCounters *counters)
{
	return hshwalk(counters->wordTable, writeexact, outF);
}

int writeNgramAndWildCountsR(void *item, void *datum, void* xtra)
{
	setitem* set;
	hshtbl* nexthash;
	char** query;
	outputState* osDatum = (outputState *) datum;
	set = (setitem*) item;
	nexthash = set->pl.set;
	query = osDatum->query;
	query[osDatum->currentDepth - 1] = set->search;
	printf("osDatum->currentDepth %d osDatum->queryLength %d\n", osDatum->currentDepth, osDatum->queryLength);
	if (osDatum->currentDepth == osDatum->queryLength - 1)
	{
		/* we're down to the hash table with exactitems */
#ifdef CURRENTDEBUG
		printf("Printing queries at depth %d\n",
				osDatum->queryLength - 1);
		hshstats hs = hshstatus(nexthash);
		printf("No. items in queries: %lu\n", hs.hentries);
#endif
		hshwalk(nexthash, writequery, osDatum);

	}
	else
	{
		/* we're still in the nested hashtables, we need to walk
		 * further */
		osDatum->currentDepth++;
#ifdef CURRENTDEBUG
		hshstats hs = hshstatus(nexthash);
		printf("No. items surrounding: %lu\n", hs.hentries);
#endif
		hshwalk(nexthash, writeNgramAndWildCountsR,
				(void *) osDatum);
		osDatum->currentDepth--;
	}
	return 0;
}

int doRation(void *item, void *datum, void *xtra)
{
	exactitem ei;
	exactptr exactp, othexact;
	setitem si;
	setptr setp;
	rationptr rp;
	int cmpres;

	exactp = item;
	rp = datum;

	// check if we've been visited and cleaned out beforehand
	if (exactp->count == NULL)
	{
		return 0;
	}

	// first find the other one that has the same combination
	si.search = exactp->search;
	setp = hshfind(rp->queries, &si);
	ei.search = rp->firstWord;
	othexact = hshfind(setp->pl.set, &ei);
	// see if it's been cleaned out before this one
	if (othexact->count == NULL)
	{
		return 0;
	}

	// now decide who gets to keep the counts
	cmpres = strcmp(rp->firstWord, exactp->search);
	if(cmpres > 0)
	{
		// first word lexicographically higher, transfer to other
		exactp->count = NULL;
	}
	else
	{
		// transfer other to here
		othexact->count = NULL;
	}

	return 0;	
}

// first level of rationalisation. Store the first word and delegate the checking against the second word
int rationaliseCounts(void *item, void *datum, void *xtra)
{
	setptr setp;
	rationer ration;
	int result;

	setp = item;
	ration.firstWord = setp->search;
	ration.queries = datum;

	result = hshwalk(setp->pl.set, doRation, &ration);
	
	return result;
}

#if 0
int writeCoCounts(FILE *outngramF, FILE *outwildF, xcCounters *counters)
{
	int result;
	char *query[2];
	outputState osDatum;

	// first combine queries where the words occurred in opposite orders
	result = hshwalk(counters->queries, rationaliseCounts, counters->queries);

	// then write it all out
	osDatum.outngramF = outngramF;
	osDatum.outwildF = outwildF;
	osDatum.queryLength = counters->queryLength;
	osDatum.currentDepth = 1;
	osDatum.query = query;
	result = hshwalk(counters->queries, writeSetCounts, &osDatum);
	
	return result;
}
#endif

int writeNgramAndWildCounts(FILE *outF, xcCounters *counters)
{
	int result;
	char** query;
	query = malloc(sizeof(char*) * counters->queryLength - 1);

	outputState* osDatum = malloc(sizeof(outputState));

	if (doCooc)
	{
		result = hshwalk(counters->queries, rationaliseCounts, counters->queries);
	}
	
	osDatum->outF = outF;
	osDatum->queryLength = counters->queryLength;
	osDatum->currentDepth = 1;
#ifdef CURRENTDEBUG
	printf("writeNgramAndWildCounts: currentDepth = %d\n", osDatum->currentDepth);
	printf("writeNgramAndWildCounts: queryLength = %d\n", osDatum->queryLength);
#endif
	osDatum->query = query;
	if (osDatum->queryLength > 1) {
		result = hshwalk(counters->queries, writeNgramAndWildCountsR,
				osDatum);
	} else {
		result = hshwalk(counters->queries, writequery,
				osDatum);
	}
	free(osDatum);
	free(query);
	return result;
}

int writeWildMatches(FILE *outF, xcCounters *counters)
{
	return hshwalk(counters->wildMatches, writeset, outF);
}

int writeexact(void *item, void *datum, void *xtra)
{
	exactptr itemptr;
	FILE *outF;

	itemptr = item;
	/* don't do anything if we're not interested in zero counts */
	if(!(printZeroCounts || (*(itemptr->count) != 0)))
	{
		return 0;
	}
	outF = datum;
	
	fprintf(outF, "%s\t%lld\n", itemptr->search, *itemptr->count);

	return 0;
}

int writequery(void *item, void *datum, void *xtra)
{
	exactitem* qr;
	FILE *outF;
	outputState* os;
	char** query;
	int i;
	long long *count;

	qr = (exactitem*) item;
	count = qr->count;
	/* don't do anything if we're not interested in zero counts */
//	if((count == NULL) || !(printZeroCounts || (*count != 0)))
	if(count == NULL || !(printZeroCounts || *count != 0))
	{
		return 0;
	}

	/* the query is to be written, carry on */
	os = (outputState*) datum;
	outF = os->outF;
	query = os->query;

	for (i=0; i< os->queryLength - 1; i++){
		/* with spaces after them */
		
		fprintf(outF, "%s ", query[i]);
	}
	/* no space after it */
	fprintf(outF, "%s", qr->search);

	fprintf(outF, "\t%lld\n", *count);

	return 0;
}

int writeset(void *item, void *datum, void *xtra)
{
	setptr itemptr;
	FILE *outF;
	long long sum;

	itemptr = item;
	outF = datum;
	sum = 0;

	hshwalk(itemptr->pl.set, addexact, &sum);

	fprintf(outF, "%s\t%lld\n", itemptr->search, sum);
	hshwalk(itemptr->pl.set, writeexact, outF);
	fprintf(outF, "\n");

	return 0;
}

int addexact(void *item, void *datum, void *xtra)
{
	exactptr itemptr;
	long long *sum;

	itemptr = item;
	sum = datum;
	
	*sum += *itemptr->count;

	return 0;
}
