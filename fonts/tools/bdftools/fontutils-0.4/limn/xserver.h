/* xserver.h: declarations for our X ``server'' that handles the actual
   displaying. 

Copyright (C) 1992 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef XSERVER_H
#define XSERVER_H

#include <X11/Xatom.h>
#include "types.h"

/* Tell our server to shut down.  It has no data.  */
#define FOSERVER_EXIT_ATOM "GNU_FIT_OUTLINES_SERVER_EXIT"
extern Atom foserver_exit_atom;

/* Tell our server to display a new pixmap.  The pixmap ID is given as
   the first `long' data element.  */
#define FOSERVER_UPDATE_PIXMAP_ATOM "GNU_FIT_OUTLINES_SERVER_UPDATE_PIXMAP"
extern Atom foserver_update_pixmap_atom;

/* Identifies the server.  */
#define FOSERVER_IDENTITY_ATOM "GNU_FIT_OUTLINES_SERVER_IDENTITY"
extern Atom foserver_identity_atom;

#ifndef STANDALONE_SERVER
/* Start the server with a default window size of INITIAL_SIZE pixels
   and an identification string IDENTITY.  */
extern void start_server (unsigned initial_size, string identity);
#endif /* not STANDALONE_SERVER */

#endif /* not XSERVER_H */
