# Additional definitions for library directories.
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
# 
# This is to be included after defs.make, and after the definition of
# $(sources) and $(library).

all: $(library).a

# Do not install the libraries anywhere.
install: $(library).a
#	$(INSTALL_DATA) $(library).a $(libdir)/lib$(library).a

$(library).a: $(objects)
	rm -f $(library).a
	$(AR) qc $(library).a $(objects)
#	for o in $(objects); \
#        do \
#          $(AR) qv $(library).a $${o}; \
#        done
	$(RANLIB) $(library).a
        
mostlyclean::
	rm -f $(library).a
