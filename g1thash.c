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

#include <string.h>
#include <stdlib.h>

#include "hashlib.h"
#include "g1thash.h"

int exactcmp(void *litem, void *ritem)
{
	return strcmp(((exactptr)litem)->search, ((exactptr)ritem)->search);
}

int setcmp(void *litem, void *ritem)
{
	return strcmp(((setptr)litem)->search, ((setptr)ritem)->search);
}

void *dupexact(void *item)
{
	exactptr newitem;
	exactptr olditem;

	olditem = item;

	// allocate the new item
	if((newitem = malloc(sizeof(exactitem))))
	{
		// duplicate the string
		if((newitem->search = strdup(olditem->search)))
		{
			// if the input item's count is NULL, allocate a new one
			if(olditem->count == NULL)
			{
				if(!(newitem->count = malloc(sizeof(long long))))
				{
					free(newitem->search);
					free(newitem);
					newitem = NULL;
				}
				else
				{
					*newitem->count = 0;
				}
			}
			// otherwise, shallow copy it
			else
			{
				newitem->count = olditem->count;
			}
		}
		else
		{
			free(newitem);
			newitem = NULL;
		}
	}
	return newitem;
}

void *dupset(void *item)
{
	setptr newitem;
	setptr olditem;

	olditem = item;

	// allocate the new item
	if((newitem = malloc(sizeof(setitem))))
	{
		// duplicate the string
		if((newitem->search = strdup(olditem->search)))
		{
			// does the input item indicate a terminal set?
			// if so, we want a hashtable for exact items
			if(olditem->pl.isterminal)
			{
				newitem->pl.set = hshinit(hashexact, rehashexact, exactcmp, dupexact, freeexact, 0);
			}
			// otherwise, we want a hashtable for sets
			else
			{
				newitem->pl.set = hshinit(hashset, rehashset, setcmp, dupset, freeset, 0);
			}

			// make sure the hashtable allocation was successful
			if (newitem->pl.set == NULL)
			{
				free(newitem->search);
				free(newitem);
				newitem = NULL;
			}
		}
		else
		{
			free(newitem);
			newitem = NULL;
		}
	}
	return newitem;
}

void freeexact(void *item)
{
	exactptr itemptr = item;

	free(itemptr->search);
	free(itemptr->count);
	free(itemptr);
}

void freeset(void *item)
{
	setptr itemptr = item;

	free(itemptr->search);
	hshkill(itemptr->pl.set);
	free(itemptr);
}

unsigned long hashexact(void *item)
{
	return hshstrhash(((exactptr)item)->search);
}

unsigned long rehashexact(void *item)
{
	return hshstrehash(((exactptr)item)->search);
}

unsigned long hashset(void *item)
{
	return hshstrhash(((setptr)item)->search);
}

unsigned long rehashset(void *item)
{
	return hshstrehash(((setptr)item)->search);
}
