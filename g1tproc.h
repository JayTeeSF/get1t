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

#include "g1thash.h"

#ifndef G1TPROC_H
#define G1TPROC_H

int incrementQueries(hshtbl* queries, long long curcount, char** tstarts, int
		currentdepth, int querylength, exactitem* exact, setitem* set);
int incrementCooc(hshtbl *queries, long long curcount, char **tstarts);
int incrementWords(hshtbl *words, long long count, char **tstarts);

int procPrep(void);
int procCleanup(void);

#endif /* G1TPROC_H */
