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

#include <unistd.h>	// for NULL
#include <stdlib.h>	// for malloc
#include <assert.h>	// for assert

#include "get1t.h"
#include "g1tproc.h"

// globals from g1tmain
extern wordBhv doWords;
extern int n;
extern int doCooc;

// heap variables for efficiency
static hshtbl **hitstarts = NULL;
static long long **counthits = NULL;

int incrementQueries(hshtbl* queries, long long curcount, char** tstarts, int
		currentdepth, int querylength, exactitem* exact, setitem* set)
{
	void* next;
	exactitem* item;
	setitem* si;
	int found;
	int exactfound;
	int wildfound;

	found = 0;
	exactfound = 0;
	wildfound = 0;

#ifdef DEBUG
	printf("Looking for query with word %d, '%s' or '%s'\n", currentdepth,
		tstarts[currentdepth], WILDCARD);
#endif
	if (currentdepth < (querylength - 1))
	/* search for another hash table down */
	{
		/* search for the current word */
		set->search = tstarts[currentdepth];
		next = hshfind(queries, set);
		if (next != NULL)
		{
#ifdef DEBUG
			printf("Found word match '%s'\n", tstarts[currentdepth]);
#endif
			si = (setitem*) next;
			exactfound = incrementQueries(si->pl.set, curcount,
				tstarts, currentdepth + 1, querylength,
				exact, set);
		}
		/* search for a wildcard in the current position */
		set->search = WILDCARD;
		next = hshfind(queries, set);
		if (next != NULL)
		{
#ifdef DEBUG
			printf("Found wild match '<*>'\n");
#endif
			si = (setitem*) next;
			wildfound = incrementQueries(si->pl.set, curcount,
					tstarts, currentdepth + 1,
					querylength, exact, set);
		}
		found = (exactfound || wildfound);
	}
	else
	/* search for exact items */
	{
		/* search for the current word */
		exact->search = tstarts[currentdepth];
		next = hshfind(queries, exact);
		if (next != NULL)
		{
			item = (exactitem*) next;
			long long* count = item->count;
			*count = *count + curcount;
			found = 1;
		}
		/* search for a wildcard */
		exact->search = WILDCARD;
		next = hshfind(queries, exact);
		if (next != NULL)
		{
			item = (exactitem*) next;
			long long* count = item->count;
			*count = *count + curcount;
			found = 1;
		}
	}
	return found;
}

int incrementCooc(hshtbl *queries, long long curcount, char **tstarts)
{
	exactitem exact;
	exactptr item;
	setitem set;
	setptr si;
	int numStart, numExact, i, j, k, useCount;
	
	numStart = 0;
	numExact = 0;
	for (i=0;i<n;i++)
	{
#ifdef DEBUG
		printf("Looking for query with word %d, ('%s')\n", i,
			tstarts[i]);
#endif
		// can't possibly get a full match if there are no partial matches by the final token
		if ((i == (n - 1)) && (numStart == 0))
		{
			break;
		}
		// now check to see if there are any full matches, from the starts previously discovered
		for(j=0;j<numStart;j++)
		{
			exact.search = tstarts[i];
			item = hshfind(hitstarts[j], &exact);
			if (item != NULL)
			{
				useCount = 1;
				for(k=0;k<numExact;k++)
				{
					if(counthits[k] == item->count)
					{
						useCount = 0;
						break;
					}
				}
				if(useCount)
				{
					*item->count += curcount;
					counthits[numExact] = item->count;
					numExact++;
				}
			}
		}
		// final token can't start matches, so we're done if we're on that
		if (i == (n - 1))
		{
			break;
		}
		// then see if we have any starts of matches
		set.search = tstarts[i];
		si = hshfind(queries, &set);
		if(si != NULL)
		{
			hitstarts[numStart] = si->pl.set;
			numStart++;
		}
	}

	return (numExact > 0);
}

int incrementWords(hshtbl *wordTable, long long curcount, char **tstarts)
{
	exactitem exact;
	exactptr exactmatch;
	int i, j, numhits, isnew;

	numhits = 0;
		
	for(i=0;i<n;i++)
	{
		// is the word interesting?
#ifdef DEBUG
		printf("Checking for match of word \'%s\': ", tstarts[i]);
#endif
		exact.search = tstarts[i];
		exactmatch = hshfind(wordTable, &exact);
		if(exactmatch != NULL)
		{
			if (doWords == countOcc)
			{
				isnew = 1;
				for(j=0;j<numhits;j++)
				{
					if (exactmatch->count == counthits[j])
					{
						isnew = 0;
						break;
					}
				}
				if (isnew)
				{
					*exactmatch->count += curcount;
					counthits[numhits] = exactmatch->count;
					numhits++;
				}
			}
			else
			{
				*exactmatch->count += curcount;
			}
#ifdef DEBUG
			printf("hit\n");
		}
		else
		{
			printf("miss\n");
#endif
		}
	}
	return 0;
}

int procPrep()
{
	// ensure required memory buffers are allocated
	if (doCooc && (hitstarts == NULL))
	{
		hitstarts = malloc((n - 1) * sizeof(hshtbl *));
		assert(hitstarts != NULL);
	}
	if ((doCooc || (doWords == countOcc)) && (counthits == NULL))
	{
		counthits = malloc((n - 1) * sizeof(long long *));
		assert(counthits != NULL);
	}

	return 0;
}

int procCleanup()
{
	if (doCooc)
	{
		free(hitstarts);
		hitstarts = NULL;
	}
	if(doCooc || (doWords == countOcc))
	{
		free(counthits);
		counthits = NULL;
	}

	return 0;
}
