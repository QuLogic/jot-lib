/*****************************************************************
 * This file is part of jot-lib (or "jot" for short):
 *   <http://code.google.com/p/jot-lib/>
 * 
 * jot-lib is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * jot-lib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.`
 *****************************************************************/
/* Copyright 1995, Brown Computer Graphics Group.  All Rights Reserved. */

/* -------------------------------------------------------------------------
 *
 *                <     File description here    >
 *
 * ------------------------------------------------------------------------- */

#ifndef NETWORK_HAS_BEEN_INCLUDED
#define NETWORK_HAS_BEEN_INCLUDED

#include "std/platform.hpp" //#include <windows.h>

#ifdef WIN32
#define ssize_t int
#include <winsock.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "std/support.hpp"

#ifdef WIN32
ssize_t write_win32(int fildes, const void *buf, size_t nbyte);
int num_bytes_to_read(int fildes);
#else
int num_bytes_to_read(int fildes);
#endif

#endif  /* NETWORK_HAS_BEEN_INCLUDED */
