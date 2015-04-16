Get 1T: retrieve relevant counts from n-gram database.
Copyright (C) 2007 Tobias Hawker, Mary Gardiner and Andrew Bennetts
Incorporates hashlib, Copyright (C) 2002, 2006 C. B. Falconer

This is free software, licensed under the GNU GPL. Please see the file
LICENSE.txt for details.

version 0.2.2

GENERAL
=======
This program is designed to retrieve only those counts of interest from a very
large collection of n-gram frequencies. The n-grams in which the user is
interested must be collected before a single pass is made over the corpus, and
the frequencies for the desired patterns is retrieved. Wildcard queries are
also supported.

Currently it is targeted at the Web 1T corpus (Brantz and Franz, 2006;
LDC2006T13) but if comparable corpora or other data sources where the
approaches used are suitable are released (or pointed out...) modification to
be useful for these corpora would not be particularly difficult.

Suggestions, bug reports and other comments are welcome. Please email us at:
thawker AT users DOT sourceforge DOT net
hypatia AT users DOT sourceforge DOT net

INSTALLATION
============
To build, simply cd to the trunk/get1t directory where you untarred or checked
out from SVN and type "make".

The get1t executable should compile on most UNIX-like systems (Linux, BSD,
Solaris, Mac OS X, Cygwin, etc). Please let us know if you have any
difficulties.

USING GET1T
===========

Usage
-----
If you run get1t with invalid arguments (including no arguments at all) it
will print a help message to stderr.

The software expects the path to the database directory containing the
compressed counts for n-grams of the desired length as a compulsory argument.
Currently, operation over uncompressed data is not supported.

Query Format
------------
What is needed is a set of queries, in a single file - each number of tokens
requires a separate query file and invocation of the program. Tokens in the
queries should separated by a single space. The software will find n-grams in
the databse that match any of those in the supplied queries, and write the
results to the output files. Wildcards in any position may be specified by
using the special token <*>. The reported counts may be the sum of many
n-grams when using wildcards or operating in case-insensitive mode.

For example, the queries:
one two three four five
one two <*> four five
<*> two <*> four five

will all match the database entry:
one two three four five	100

however, only the latter two will match
one two 3 four five	100

and only the final query will match:
1 two 3 four five	100

If the program runs in case-insensitive mode (the default) then the count for:
ONE TWO THREE FOUR FIVE
will also be added to the count for each of the queries above.

Output
------
Extracted frequencies are by default written to the file
<n>-ngram.txt. This  contains the totals for each query, reported in
the same way the query was specified. The results are reported in an arbitrary
order, and thus will not correspond to the order of patterns in the query file.

CITATION (OPTIONAL)
===================
Get 1T is free software, and all requirements regarding licensing and
redistribution are set forth in the accompanying GPL.

However, if you use this software for published research, it would be very
much appreciated if you acknowledged the software, and the methods used.

The following article describes the techniques used in the software:
Tobias Hawker, Mary Gardiner and Andrew Bennetts, 2007. Practical queries of a
massive n-gram database. In Proceedings of the Australasian Language
Technology Workshop 2007 (ALTW 2007), Melbourne, Australia. To appear.

If a full citation is not practical, a reference to the website in a footnote
or similar (http://get1t.sf.net) would be most appreciated.

FUTURE FEATURES
===============
The ability to use database files in uncompressed format (which on most
systems will be faster than decompressing on the fly).

The capability to operate in co-occurrence counting mode, where the number of
n-grams in which two tokens co-occur is determined, will also be incorporated
shortly.

Additional software has been written that can make on-the-fly queries of the
database, but at the cost of approximate counts, and the possibility of false
positive counts. This software will be added in a separate module in the
not-too-distant future.

RELEASE NOTES
=============
v0.1	2007-9-3	Initial version
v0.2	2007-10-24	Incorporated multiple wildcards; printing wild-card
			matches not working
v0.2.1	2007-10-25	Printing wild-card matches working (all matching lines
			from the corpus printed separately, including case
			differences)
v0.2.2  2007-10-26      Removed the dependency on non-standard GNU C libraries
