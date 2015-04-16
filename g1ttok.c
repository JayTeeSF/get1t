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

#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "g1ttok.h"

// global from g1tmain
extern int ignoreCase;

// local functions
int cleanIgnore(char *inbuf, int maxbuf, char **wstarts);
int cleanPlain(char *inbuf, int maxbuf, char **wstarts);
int cleanIgnoreCopy(char *inbuf, int maxbuf, char **wstarts, char **orig, int *maxorig);
int cleanCopy(char *inbuf, int maxbuf, char **wstarts, char **orig, int *maxorig);

/* New-style cleanline. Does more to avoid revisiting the same parts of the
   input string. Perfoms:
	- conversion to lower case (if necessary)
	- copying to storage buffer (if necessary)
	- tokenisation and storage of token starts
	- removal of leading newlines (should be universal)
   Return value is the number of tokens found. There will be one more
   token start recorded, for the start of the next line.  As far as possible,
   all checks are outside the inner loops, as profiling indicated the old
   cleanline was taking a large fraction of the time. If a complete line has
   not been found before the end of the input is reached, the tokenisation
   is undone. */
int cleanline(char *inbuf, int maxbuf, char **wstarts, char **orig, int *maxorig)
{
	char *curpos;
	char *badpos;
	int prevspace;
	int i;
	int bufleft;

	// skip any initial return characters
	curpos = inbuf;
	badpos = inbuf + maxbuf;
	while((curpos < badpos) && (*curpos == '\r' || *curpos == '\n'))
	{
		curpos++;
	}
	// decide where we will have to bug out
	bufleft = maxbuf - (curpos - inbuf);
	prevspace = 1;
	i = -1;

	// check we're still inside the permitted region
	if (curpos >= badpos)
	{
		return 0;
	}

	// can we skip copying?
	if (orig == NULL)
	{
		if (ignoreCase)
		{
			return cleanIgnore(curpos, bufleft, wstarts);
		}
		else
		{
			return cleanPlain(curpos, bufleft, wstarts);
		}
	}
	else
	{
		if (ignoreCase)
		{
			return cleanIgnoreCopy(curpos, bufleft, wstarts, orig, maxorig);
		}
		else
		{
			return cleanCopy(curpos, bufleft, wstarts, orig, maxorig);
		}
	}
}

int cleanIgnore(char *inbuf, int maxbuf, char **wstarts)
{
	char *curpos, *badpos;
	int i, j, prevspace, tab_ndx;

	// initialise	
	i = -1;
	prevspace = 1;
	tab_ndx = -1;
	curpos = inbuf;
	badpos = curpos + maxbuf;

	// now do the loop
	while (curpos < badpos)
	{
		// ensure cases are evaluated in order of frequency
		if (!isspace(*curpos))
		{
			if (prevspace)
			{
				i++;
				wstarts[i] = curpos;
				prevspace = 0;
			}
			if (isupper(*curpos))
			{
				*curpos = tolower(*curpos);
			}
		}
		else if (*curpos == ' ')
		{
			*curpos = '\0';
			prevspace = 1;
		}
		else if (*curpos == '\t')
		{
			*curpos = '\0';
			prevspace = 1;
			tab_ndx = i + 1;
		}
		else if(*curpos == '\n' || *curpos == '\r')
		{
			*curpos = '\0';
			i++;
			wstarts[i] = curpos + 1;
			return i;
		}
		curpos++;
	}
	// if we get to here, we've run out of input data
	// restore the spaces:
	// first, did we just nul one out?
	curpos--;
	if(*curpos == '\0')
	{
		if (tab_ndx != (i + 1))
		{
			*curpos = ' ';
		}
		else
		{
			*curpos = '\t';
		}
	}
	// now handle the ones we have seen previously
	for(j=1;j<=i;j++)
	{
		curpos = wstarts[j];
		curpos--;
		if(j != tab_ndx)
		{
			*curpos = ' ';
		}
		else
		{
			*curpos = '\t';
		}
	}
	return -(i + 1);
}


int cleanPlain(char *inbuf, int maxbuf, char **wstarts)
{
	char *curpos, *badpos;
	int i, j, prevspace, tab_ndx;

	// initialise	
	i = -1;
	prevspace = 1;
	tab_ndx = -1;
	curpos = inbuf;
	badpos = curpos + maxbuf;

	// now do the loop
	while (curpos < badpos)
	{
		if (!isspace(*curpos))
		{
			if (prevspace)
			{
				i++;
				wstarts[i] = curpos;
				prevspace = 0;
			}
		}
		else if (*curpos == ' ') 
		{
			*curpos = '\0';
			prevspace = 1;
		}
		else if (*curpos == '\t')
		{
			*curpos = '\0';
			prevspace = 1;
			tab_ndx = i + 1;
		}
		else if(*curpos == '\n' || *curpos == '\r')
		{
			*curpos = '\0';
			i++;
			wstarts[i] = curpos + 1;
			return i;
		}
		curpos++;
	}
	// if we get to here, we've run out of input data
	// restore the spaces:
	// first, did we just nul one out?
	curpos--;
	if(*curpos == '\0')
	{
		if (tab_ndx != (i + 1))
		{
			*curpos = ' ';
		}
		else
		{
			*curpos = '\t';
		}
	}
	// now handle the ones we have seen previously
	for(j=1;j<=i;j++)
	{
		curpos = wstarts[j];
		curpos--;
		if(j != tab_ndx)
		{
			*curpos = ' ';
		}
		else
		{
			*curpos = '\t';
		}
	}
	return -(i + 1);
}

int cleanIgnoreCopy(char *inbuf, int maxbuf, char **wstarts, char **orig, int *maxorig)
{
	char *curpos, *badpos, *origpos;
	int i, prevspace;

	//initialise
	curpos = inbuf;
	origpos = *orig;
	i = -1;
	prevspace = 1;
	// decide whether it is the input or storage that limits us
	if (maxbuf < *maxorig)
	{
		badpos = curpos + maxbuf;
	}
	else
	{
		badpos = curpos + *maxorig;
	}
	// now do the loop
	while (curpos < badpos)
	{
		if (!isspace(*curpos))
		{
			if (prevspace)
			{
				i++;
				wstarts[i] = curpos;
				prevspace = 0;
			}
			*origpos = *curpos;
			if (isupper(*curpos))
			{
				*curpos = tolower(*curpos);
			}
		}
		else if (*curpos == ' ' || *curpos == '\t')
		{
			*origpos = *curpos;
			*curpos = '\0';
			prevspace = 1;
		}
		else if (*curpos == '\n' || *curpos == '\r')
		{
			*origpos = '\0';
			*curpos = '\0';
			i++;
			wstarts[i] = curpos + 1;
			return i;
		}
		curpos++;
		origpos++;
	}
	// if we get to here, we could have run out of input data
	if (curpos >= (inbuf + maxbuf))
	{
		// restore the previous contents of the input buffer
		badpos = curpos;
		curpos = inbuf;
		origpos = *orig;
		while (curpos < badpos)
		{
			*curpos = *origpos;
			curpos++;
			origpos++;
		}
		// indicate that the line hasn't been processed
		return -(i + 1);
	}
	else
	{
		// the copy buffer could be too small
		// let's give ourselves more room
		*maxorig *= 2;
		*orig = realloc(*orig, *maxorig);
		assert(*orig != NULL);
		// and recursively try again
		return cleanIgnoreCopy(inbuf, maxbuf, wstarts, orig, maxorig);
	}
}

int cleanCopy(char *inbuf, int maxbuf, char **wstarts, char **orig, int *maxorig)
{
	char *curpos, *badpos, *origpos;
	int i, prevspace;

	//initialise
	curpos = inbuf;
	origpos = *orig;
	i = -1;
	prevspace = 1;
	// decide whether it is the input or storage that limits us
	if (maxbuf < *maxorig)
	{
		badpos = curpos + maxbuf;
	}
	else
	{
		badpos = curpos + *maxorig;
	}
	// now do the loop
	while (curpos < badpos)
	{
		if (!isspace(*curpos))
		{
			if (prevspace)
			{
				i++;
				wstarts[i] = curpos;
				prevspace = 0;
			}
			*origpos = *curpos;
		}
		else if (*curpos == ' ' || *curpos == '\t')
		{
			*origpos = *curpos;
			*curpos = '\0';
			prevspace = 1;
		}
		else if (*curpos == '\n' || *curpos == '\r')
		{
			*origpos = '\0';
			*curpos = '\0';
			i++;
			wstarts[i] = curpos + 1;
			return i;
		}
		curpos++;
		origpos++;
	}
	// if we get to here, we could have run out of input data
	if (curpos >= (inbuf + maxbuf))
	{
		// restore the previous contents of the input buffer
		badpos = curpos;
		curpos = inbuf;
		origpos = *orig;
		while (curpos < badpos)
		{
			*curpos = *origpos;
			curpos++;
			origpos++;
		}
		// indicate that the line hasn't been processed
		return -(i + 1);
	}
	else
	{
		// the copy buffer could be too small
		// let's give ourselves more room
		*maxorig *= 2;
		*orig = realloc(*orig, *maxorig);
		assert(*orig != NULL);
		// and recursively try again
		return cleanCopy(inbuf, maxbuf, wstarts, orig, maxorig);
	}
}

