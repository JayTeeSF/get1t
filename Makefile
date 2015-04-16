#    Get 1T: retrieve relevant counts from n-gram database.
#    Copyright (C) 2007 Tobias Hawker
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

cc:=gcc
CC:=g++

CFLAGS:=-O3 -Wall -Wstrict-prototypes
LDFLAGS:=-lz

# can pass these on the commandline too: make CFLAGS="-g -Wall -Wstrict-prototypes -DDEBUG"
#CFLAGS:=-g -Werror -Wall -Wstrict-prototypes -DCURRENTDEBUG
#LDFLAGS:=-g -lz

# or these
#CFLAGS:=-O3 -Wall -Wstrict-prototypes -DNOZLIB
#LDFLAGS:=

VERSION:=0.2.3

TARGET:=get1t

OBJFILES:=g1tmain.o g1thash.o hashlib.o g1tdebug.o g1ttok.o g1tquery.o g1tinp.o g1tproc.o g1tout.o
SRCFILES:=get1t.h g1tmain.c g1thash.h g1thash.c hashlib.h hashlib.c g1tdebug.c g1tdebug.h g1ttok.c g1ttok.h g1tquery.c g1tinp.c g1tout.c g1tproc.h g1tproc.c

all: ${TARGET}

${TARGET}: ${OBJFILES}
	${cc} -o $@ ${LDFLAGS} ${OBJFILES}

dist: Makefile ${SRCFILES}
	mkdir -p get1t-${VERSION}
	cp Makefile ${SRCFILES} get1t-${VERSION}
	tar czf get1t-${VERSION}.tgz get1t-${VERSION}
	rm -r get1t-${VERSION}

%.o: %.c
	${cc} ${CFLAGS} -c $< -o $@

clean:
	rm -rf ${TARGET} ${OBJFILES}

# dependencies
hashlib.o: hashlib.h Makefile
g1thash.o: g1thash.h hashlib.h Makefile
g1tmain.o: get1t.h hashlib.h Makefile
g1tdebug.o: g1tdebug.h hashlib.h Makefile
g1ttok.o: get1t.h hashlib.h g1ttok.h Makefile
g1tquery.o: get1t.h hashlib.h g1thash.h g1ttok.h Makefile
g1tinp.o: get1t.h hashlib.h g1ttok.h g1tproc.h Makefile
g1tproc.o: g1tproc.h g1thash.h Makefile
g1tout.o: get1t.h hashlib.h g1thash.h Makefile
