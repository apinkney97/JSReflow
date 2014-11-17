# Makefile for the fontutils library.
# 
# Copyright (C) 1992 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

library = lib

c_only =								\
atou basename bb-outline bitmap bounding-box charcode cmdline concat	\
dir-p encoding fmod file-input file-output file-p filename fix-num font	\
getopt getopt1 hexify libfile line list logreport malloc math now numtoa\
pathsrch report safe-free scaled-num spline strstr substring varstring	\
vector xcalloc xfopen xfseek xftell xmalloc xmessage xopendir xrealloc	\
xrename xstat xstrdup

include ../data/defs.make
include ../data/defslib.make

include M.depend

malloc_stats = -DMALLOC_STATS
malloc_inline = -Dinline=
malloc.o: # Dependencies are in `M.depend'.
	$(CC) $(CFLAGS) $(malloc_stats) $(malloc_inline) -c malloc.c
