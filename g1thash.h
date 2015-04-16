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

#include "hashlib.h"

#ifndef G1THASH_H
#define G1THASH_H

typedef struct exactitem
{
	char *search;
	long long *count;
} exactitem, *exactptr;

typedef union setpayload
{
	hshtbl *set;
	int isterminal;
} setpayload;

typedef struct setitem
{
	char *search;
	setpayload pl;
} setitem, *setptr;

// functions for hshlib
// the comparison and hashes could be shared, as the first field is the key in
// both cases, but I don't want to make it break if the definitions change
int exactcmp(void *litem, void *ritem);
int cmpstr(void *litem, void *ritem);
int setcmp(void *litem, void *ritem);

void *dupexact(void *item);
void *dupset(void *item);

void freeexact(void *item);
void freeset(void *item);

unsigned long hashexact(void *item);
unsigned long rehashexact(void *item);
unsigned long hashset(void *item);
unsigned long rehashset(void *item);

int addexact(void *item, void *datum, void *xtra);

#endif
