/*

    Get 1T: retrieve relevant counts from n-gram database.
    Copyright (C) 2007 Mary Gardiner

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

#if defined(DEBUG) || defined(CURRENTDEBUG)

#include <stdio.h>
#include <stdlib.h>
#include "hashlib.h"
#include "g1tdebug.h"

typedef struct {
	int currentDepth;
	int queryLength;
	setitem* si;
	exactitem* ei;
} printStatus;

int printQueriesR(void* item, void* datum, void *xtra)
{
	int i;
	printStatus* ps = datum;
	if (ps->currentDepth < ps->queryLength - 1)
	{
		ps->si = (setitem*) item;
		for (i=0; i<ps->currentDepth+1; i++)
		{
			printf("-");
		}
		hshstats stats = hshstatus(ps->si->pl.set);
		printf("%s (node, no. entries: %ld, hash pointer: %p)\n",
			ps->si->search, stats.hentries, ps->si->pl.set);
		ps->currentDepth++;
		hshwalk(ps->si->pl.set, printQueriesR, ps);
		ps->currentDepth--;
	}
	else
	{
		ps->ei = (exactitem*) item;
		for (i=0; i<ps->currentDepth+1; i++)
		{
			printf("-");
		}
		printf("'%s' (leaf, query count: %lld)\n", ps->ei->search,
			*(ps->ei->count));
	}	
	return 0;
}

void printQueries(hshtbl* queries, int queryLength,
		exactitem* ei, setitem* si)
{
	printStatus* ps = malloc(sizeof(printStatus));
	hshstats stats = hshstatus(queries);
	printf("Number of top level queries entries: %ld\n", stats.hentries);
	printf("Address of top level queries entries: %p\n", queries);
	ps->currentDepth = 0;
	ps->queryLength = queryLength;
	printf("debug: currentDepth = %d\n", ps->currentDepth);
	printf("debug: queryLength = %d\n", ps->queryLength);
	ps->si = si;
	ps->ei = ei;
	hshwalk(queries, printQueriesR, ps);
	free(ps);
}

#endif /* DEBUG */
