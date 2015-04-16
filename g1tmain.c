/*

    Get 1T: retrieve relevant counts from n-gram database.
    Copyright (C) 2007 Tobias Hawker

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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include "get1t.h"

static int parse_args(int argc, char **argv);
static void usage(const char *progname);
static void cleanup(void);

int doCooc = 0;
int n;
int ignoreCase = 1;
int printIndividualCounts = 0;
int printZeroCounts = 0;
wordBhv doWords = dontCount;
int checkWords;
static char *findfilename = NULL;
static char *datapath = NULL;
static char *totalout = NULL;
static char *wordout = NULL;
static char *ngramout = NULL;
static char *matchout = NULL;

int main(int argc, char **argv)
{
	int result;
	xcCounters *counters;
	FILE *outF;
	FILE *wildF;
	
	result = parse_args(argc, argv);
	if(result)
	{
		return result; 
	}

	counters = makeCounters(findfilename);
	if(counters==NULL)
	{
		fprintf(stderr, "Problems creating counters\n");
		return -1;
	}
	if(printIndividualCounts)
	{
		wildF = fopen(matchout, "w");
	}
	else
	{
		wildF = NULL;
	}

	result = findHits(datapath, counters, wildF);
	if (wildF != NULL)
	{
		fclose(wildF);
	}

	if(result)
	{
		fprintf(stderr, "Problem reading counts\n");
		return result;
	}

	outF = fopen(totalout, "w");
	result = writeTotal(outF, counters);
	if(result)
	{
		fprintf(stderr, "Problem writing total\n");
		return result;
	}
	fclose(outF);

	if(doWords != dontCount)
	{
		outF = fopen(wordout, "w");
		result = writeWordCounts(outF, counters);
		if(result)
		{
			fprintf(stderr, "Problem writing word counts\n");
			return result;
		}
		fclose(outF);

/*
		if(printIndividualCounts)
		{
			outF = fopen(matchout, "w");
			result = writeWildMatches(outF, counters);
			if(result)
			{
				fprintf(stderr, "Problem writing wildcard matches\n");
				return result;
			}
			fclose(outF);
		}
*/
	}

	outF = fopen(ngramout, "w");
	result = writeNgramAndWildCounts(outF, counters);
	if(result)
	{
		fprintf(stderr, "Problem writing ngram counts or wild cards\n");
		return result;
	}
	fclose(outF);

	// don't really care what happens here
	destroyCounters(counters);
	cleanup();
	return 0;
}

void cleanup()
{
	free(findfilename);
	free(datapath);
	free(totalout);
	free(wordout);
	free(ngramout);
	free(matchout);
}

void usage(const char *progname)
{
	fprintf(stderr, "Usage: %s [OPTIONS] <n-gram data directory>\n", progname);
	fprintf(stderr, "AVAILABLE OPTIONS:\n");
	fprintf(stderr, "\t-h:          display this message\n");
	fprintf(stderr, "\t-o:          count co-occurences, rather than n-grams\n");
	fprintf(stderr, "\t-c:          case sensitive search (default is case-insensitive)\n");
	fprintf(stderr, "\t-i:          produce wild match output file containing individual n-grams that matched (default is to only print total count for wildcard queries)\n");
	fprintf(stderr, "\t-z:          write zero count for queries not seen in database (omitted by default)\n");
	fprintf(stderr, "\t-s:          find word frequencies (multiplied by the number of times the word occurs)\n");
	fprintf(stderr, "\t-r:          find word frequencies (each word is counted once per query it occurs in)\n");
	fprintf(stderr, "\t-n <number>: size of n-grams (default 2)\n");
	fprintf(stderr, "\t-f <file>:   file of n-grams to find (default \'<n>-search.txt\')\n");
	fprintf(stderr, "\t-t <file>:   total count output file (default \'<n>-total.txt\'\n");
	fprintf(stderr, "\t-w <file>:   word  count output file (default \'<n>-word.txt\'\n");
	fprintf(stderr, "\t-g <file>:   ngram count output file (default \'<n>-ngram.txt\'\n");
	fprintf(stderr, "\t-m <file>:   wild  match output file (default \'<n>-match.txt\'\n");
}

static char optstring[] = {"hocizsrn:f:t:w:g:d:m:"};

int parse_args(int argc, char **argv)
{
	int c;
	int this_option_optind;

	while(1)
	{
		if(optind)
		{
			this_option_optind = optind;
		}
		else
		{
			optind = 1;
		}
		c = getopt(argc, argv, optstring);
		if(c == -1)
		{
			break;
		}
		switch(c)
		{
		case 'o':
			doCooc = 1;
			break;
		case 'c':
			ignoreCase = 0;
			break;
		case 'i':
			printIndividualCounts = 1;
			break;
		case ':':
			fprintf(stderr, "Missing parameter\n");
			usage(argv[0]);
			return -1;
		case '?':
			fprintf(stderr, "Unknown option\n");
			usage(argv[0]);
			return -1;
		case 'z':
			printZeroCounts = 1;
			break;
		case 's':
			doWords = countInst;
			break;
		case 'r':
			doWords = countOcc;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'f':
			findfilename = strdup(optarg);
			break;
		case 't':
			totalout = strdup(optarg);
			break;
		case 'w':
			wordout = strdup(optarg);
			break;
		case 'g':
			ngramout = strdup(optarg);
			break;
		case 'm':
			matchout = strdup(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return 1;
		default:
			fprintf(stderr, "Error processing arguments\n");
			usage(argv[0]);
			return -1;
		}
	}
	/* having processed all the options, there should be one thing left, the data path */
	if(optind != (argc - 1))
	{
		fprintf(stderr, "ERROR: Require exactly one data path\n");
		usage(argv[0]);
		return -1;
	}
	datapath = strdup(argv[optind]);
	if(n==0)
	{
		n = 2;
	}
	if(findfilename == NULL)
	{
		findfilename = strdup("n-search.txt");
		findfilename[0] = '0' + n;
	}
	if(totalout == NULL)
	{
		totalout = strdup("n-total.txt");
		totalout[0] = '0' + n;
	}
	if(wordout == NULL)
	{
		wordout = strdup("n-word.txt");
		wordout[0] = '0' + n;
	}
	if(ngramout == NULL)
	{
		ngramout = strdup("n-ngram.txt");
		ngramout[0] = '0' + n;
	}
	if(matchout == NULL)
	{
		matchout = strdup("n-match.txt");
		matchout[0] = '0' + n;
	}

	/* if here, everything is OK */
	return 0;
}
