# Makefile for bitmap-to-outlines conversion program.
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

program = limn

c_and_h = curve display fit main output-bzr pxl-outline xserver
c_only = version

libraries = $(bitmap_libs) bzr tfm widgets

include ../data/defs.make
include ../data/defsprog.make

install::
	$(INSTALL_DATA) Limn.ad $(app_defaults_dir)/Limn

# To debug the server stuff, uncomment this and also comment out xserver above.
#CFLAGS := $(CFLAGS) -DSTANDALONE_SERVER

include M.depend
