/*
 * $Id: cf_init.h,v 1.6 2004/08/24 22:21:52 cfh7 REL_2_1_3 $
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




void init_chanfix_remote();

void ocf_relay_kill(struct Client *one, struct Client *source_p,
                       struct Client *target_p,
                       const char *inpath,
		       const char *reason);

void OCF_connect_chanfix_callback(int fd, int status, void *data);

void check_for_chanfix();

void OCF_connect_chanfix_with_delay(int delay);
void OCF_connect_chanfix(void *args);


int chanfix_reintroduced;

