# Common definitions for Makefiles in this directory hierarchy.
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

 
# A GNUmakefile in this hierarchy should first include this file, and
# then include either `defsprog.make' or `defslib.make', as appropriate.
# Dependencies are created by `make depend', which writes onto the file
# `M.depend'.  A Makefile should include that, also.


# $(srcdir) is the top-level directory, relative to the directory this file
# is in.
srcdir = ..

# The following variables are used only when the top-level Makefile is
# not used.
CC = gcc
INCLUDES = -I$(srcdir)/include $(xincludedir)
CFLAGS = $(INCLUDES) -g $(XCFLAGS)

# Options to be given to the loader.
LDFLAGS = $(XLDFLAGS)
LIBS = -lm # System libraries.

# This program is invoked on the libraries after `ar' builds them.
RANLIB = ranlib



# Nothing below here should need to be changed.

# Libraries to support bitmap font formats.  Defined just so the main
# Makefiles can use $(bitmap_libs), in case we ever add more formats.
bitmap_libs = gf pk

# If the caller wants our widgets, they also want the other X libraries.
# On SunOS 4.1, dynamic linking with -lXaw fails: formWidgetClass and
# the other X widgets in ../widgets are multiply defined.  I don't
# understand why the entire library file is pulled in with dynamic
# linking.  If someone explains it to me, maybe I can change things so
# they work.
# 
ifneq "$(findstring widgets, $(libraries))" ""
ifeq "$(xlibdir)" ""
# We have to guess something for the default.
xlibdirslash = /usr/lib/
else
xlibdirslash = $(subst -L,,$(xlibdir)/)
endif
X_libraries = $(xlibdirslash)libXaw.a -lXmu -lXext -lXt -lX11 $(wlibs)
endif

# Compose the entire list of libraries from the defined `libraries'.  We
# always add `lib' at the beginning and the end, since everyone uses that.
ourlibs := $(foreach lib,lib $(libraries) lib,$(srcdir)/$(lib)/$(lib).a)

# This is what we will link with.
LOADLIBES = $(ourlibs) $(X_libraries) $(LIBS)

# Make the list of object files, headers, and sources.  The headers only
# matter for tags.
sources = $(addsuffix .c, $(c_and_h) $(basename $(c_only)))
headers = $(addsuffix .h, $(c_and_h) $(basename $(h_only)))
objects = $(addsuffix .o, $(basename $(sources)))


# These set things correctly for my development environment.
ifeq ($(HOSTNAME),hayley.fsf.org)
override RANLIB = true
override CC = gcc -posix
override XCFLAGS := $(XCFLAGS) -Wall
override wlibs = -shlib -lnsl_s -linet
endif
ifeq ($(HOSTNAME),fosse.fsf.org)
override XCFLAGS := $(XCFLAGS) -Wno-parentheses 
override xincludedir = -I/usr/local/include
endif


# The real default target is in either defslib.make or defsprog.make.
default: all
.PHONY: default

# Make a TAGS file for Emacs.
librarytags := $(patsubst %,../%/*.[hc], $(libraries) lib)
TAGS:
	etags -t $(headers) $(sources)
	etags -ta ../include/*.h
	etags -ta $(librarytags)

# Make the dependency file.
M.depend depend:
	$(CC) -MM $(INCLUDES) $(sources) > M.depend

# Remove most files.
mostlyclean::
	rm -f $(objects)
.PHONY: mostlyclean

# Remove all files that would not be in a distribution.
clean:: mostlyclean
	rm -f core *.dvi *.log
.PHONY: clean

# Remove all files that can be reconstructed with make.
realclean: clean
	rm -f TAGS M.depend
