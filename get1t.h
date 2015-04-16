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

#include <stdio.h>
#include "hashlib.h"

#ifndef GET1T_H
#define GET1T_H

// constants
#define WILDCARD "<*>"
#define TENK 10240

// datatypes

typedef struct
{
	// actual counters
	hshtbl *wordTable;
	hshtbl *queries;
	hshtbl *wildMatches;
	long long bigcount;
	int queryLength;
} xcCounters;

typedef struct
{
	FILE *outF;
	int queryLength;
	int currentDepth;
	char** query;
} outputState;

typedef struct
{
	long long count;
	char* originalQuery;
} queryResult;

typedef enum wordBhv
{
	dontCount,
	countInst,
	countOcc
} wordBhv;

// functions
xcCounters *makeCounters(const char *searchfilename);
int findHits(const char *datadir, xcCounters *counters, FILE *wildF);
int destroyCounters(xcCounters *counters);
int writeTotal(FILE *outF, xcCounters *counters);
int writeWordCounts(FILE *outF, xcCounters *counters);
int writeNgramCounts(FILE *outF, xcCounters *counters);
int writeWildCounts(FILE *outF, xcCounters *counters);
int writeNgramAndWildCounts(FILE *outF, xcCounters *counters);
int writeWildMatches(FILE *outF, xcCounters *counters);

#endif
