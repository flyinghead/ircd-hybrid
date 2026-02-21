/*
 * $Id: cf_event.h,v 1.8 2004/08/24 22:21:52 cfh7 REL_2_1_3 $
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





/* Walks through all channels and checks the scores. Then
 * updates the database with those scores. */
void ocf_update_database(void* args);

/* Saves the database to disk. */
void ocf_save_database(void* args);

/* Returns true if at least one client in this channel has a score
 * which is high enough to be opped. */
int has_a_scored_client(struct Channel *chptr);

/* Checks whether we are currently split. Activates split
 * mode if necessary. */
void ocf_check_split(void* args);

/* Walks through all channels to see which ones are opless.
 * The opless channels are added to the autofix list. */
void ocf_check_opless_channels(void* args);

/* Walks through all channel on the chanfix list and sees
 * which ones need action. Deletes all channels that have been
 * fixed from the chanfix list. */
void ocf_fix_chanfix_channels(void* args);

/* Walks through all channel on the autofix list and sees
 * which ones need action. Deletes all channels that have been
 * fixed from the autofix list. */
void ocf_fix_autofix_channels(void* args);

/* Checks the current time and makes sure that the update 
 * database event is added at an amount of seconds from now
 * so that it takes place at an exact integer amount times
 * OCF_DATABASE_UPDATE_TIME from midnight. */
void add_update_database_event();

/* Adds the save database event at the right time from now. */
void add_save_database_event();
