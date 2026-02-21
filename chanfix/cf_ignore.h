/*
 * $Id: cf_ignore.h,v 1.6 2004/08/24 22:21:52 cfh7 REL_2_1_3 $
 *
 *   OpenChanfix v2.0 
 *
 *   Channel Re-op Service Module for Hybrid 7.0.
 *   Copyright (C) 2003-2004 Thomas Carlsson and Joost Vunderink.
 *   See http://www.garion.org/ocf/ for more information.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define OCF_IGNORE_HEAP_SIZE 1024


/*
 * One client mask that our service must ignore. All three properties
 * can be wildcarded using the ? and * wildcards.
 */
struct ocf_ignore {
	char hostmask[USERLEN + 1 + HOSTLEN + 1];
	char server[HOSTLEN + 1];
};

dlink_list ocf_ignore_list;

/* Inits the ignore heaps. MUST be called BEFORE any other ocf_ignore_*
 * functions. */
void ocf_ignores_init();

/* Deletes all ignores and deallocates the ignore heaps. */
void ocf_ignores_deinit();

/* Allocates a new user and returns a pointer to the new user. */
struct ocf_ignore* ocf_add_ignore(const char* hostmask, const char* server);

/* Checks whether this user@nick on server should be ignore. Returns
 * true if so and false if not. */
int ocf_is_ignored(const char* hostmask, const char* server);

/* Loads all ignores from the filename. 
 * Each line must be of the format "user@host server" where all three
 * can use ? and * wildcards, */
void ocf_ignore_load(const char* filename);

