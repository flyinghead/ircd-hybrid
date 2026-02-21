/*
 * $Id: cf_event.c,v 1.12 2004/11/09 20:22:17 cfh7 REL_2_1_3 $
 *
 *   OpenChanfix v2.0 
 *
 *   Channel Re-op Service Module for Hybrid 7.0
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




#include "cf_base.h"

/* ----------------------------------------------------------------- */

void ocf_update_database(void* args)
{
	gather_all_channels();
}

/* ----------------------------------------------------------------- */

void ocf_save_database(void* args)
{
	eventDelete(ocf_save_database, NULL);
	DB_save();
	add_save_database_event();
	ocf_log(OCF_LOG_MAIN, "DBSV Saved database.");
}

/* ----------------------------------------------------------------- */

void ocf_check_split(void* args)
{
	if (currently_split) {
		if (check_minimum_servers_linked()) {
			/* Yay, no longer split. */
			currently_split = 0;
			ocf_log(OCF_LOG_MAIN, 
				"SPLT Splitmode disabled.");
			send_notice_to_loggedin(OCF_NOTICE_FLAGS_MAIN,
				"Splitmode disabled: %d server%s linked.", 
				get_number_of_servers_linked(),
				get_number_of_servers_linked() == 1 ? "" : "s");
		}
	} else {
		if (!check_minimum_servers_linked()) {
			/* We are split. */
			currently_split = 1;
			ocf_log(OCF_LOG_MAIN, 
				"SPLT Splitmode enabled..");
			send_notice_to_loggedin(OCF_NOTICE_FLAGS_MAIN,
				"Splitmode enabled: %d server%s linked.", 
				get_number_of_servers_linked(),
				get_number_of_servers_linked() == 1 ? "" : "s");
		}
	}
}

/* ----------------------------------------------------------------- */
/* NOTE: This function assumes that DB_set_read_channel(chptr->chname)
 * has already been called! 
 * It looks at all the clients in the channel, gets the user@hosts, 
 * and finds the highest score currently present. If that's more than
 * the absolute minimum, true is returned. Otherwise, false is returned.
 */

int has_a_scored_client(struct Channel *chptr)
{
	struct Client *clptr;
	dlink_node *node_client;
	struct OCF_chanuser top_score_client, top_score_channel;
	char **users, **hosts;
	int arraypos, num_clients;
	
	num_clients = chptr->users;
	users = MyMalloc(num_clients * sizeof(char*));
	hosts = MyMalloc(num_clients * sizeof(char*));

	/* We need both the highest score of the channel and the highest
	 * score of the current clients in the channel. */

	/* First get the maximum score of the channel in the database. */
    top_score_channel.score = 0;
	DB_get_oplist( &top_score_channel, 1 );

	/* If this score is lower than the absolute minimum, forget it.
	 * We will not fix this channel. */
	if (top_score_channel.score <= 
					(int)(OCF_FIX_MIN_ABS_SCORE_END * OCF_MAX_SCORE)) {
		return 0;
	}

	/* Create an array with unique userhosts of the non-opped clients
	 * in the channel */
	arraypos = 0;
	DLINK_FOREACH(node_client, chptr->voiced.head) {
		clptr = (struct Client*)(node_client->data);
		/* The below line may increase arraypos. */
		arraypos = add_userhost_to_array(users, hosts, arraypos, 
				clptr->username, clptr->host);
	}

	DLINK_FOREACH(node_client, chptr->peons.head) {
		clptr = (struct Client*)(node_client->data);
		/* The below line may increase arraypos. */
		arraypos = add_userhost_to_array(users, hosts, arraypos, 
				clptr->username, clptr->host);
	}

	/* Get the top score of the userhosts of the non-opped clients. */
	top_score_client.score = 0;
	DB_get_top_user_hosts(&top_score_client, users, hosts, arraypos, 1);

	MyFree(users);
	MyFree(hosts);

	/*top_score_client.score = 0;
	DB_get_oplist( &top_score_client, 1 );*/
	
	ocf_log(OCF_LOG_DEBUG, 
		"AUFX Found top score %d of current clients in %s with max score %d.", 
			top_score_client.score, chptr->chname, top_score_channel.score);
	/* The top score of the present clients needs be higher than both the
	 * absolute minimum and the relavite minimum. */
	if (top_score_client.score > 
		    (int)(OCF_FIX_MIN_ABS_SCORE_END * OCF_MAX_SCORE) &&
		top_score_client.score > 
			(int)(OCF_FIX_MIN_REL_SCORE_END * top_score_channel.score)	) 
	{
		return 1;
	}


	
	return 0;
}

/* ----------------------------------------------------------------- */

void ocf_check_opless_channels(void* args)
{
	struct Channel *chptr;
	int flags, channel_exists, min_clients;
	struct timeval tv_before, tv_after;
	long duration;
	int num_opless = 0;

	/* If we're split, forget it. */
	if (currently_split) {
		return;
	}

	/* If autofixing has been disabled, well, forget it. */
	if (!settings_get_bool("enable_autofix")) {
		return;
	}

	/* Get the minimum number of clients that must be present in a channel
	 * before it will be fixed. */
	min_clients = settings_get_int("min_clients");

	/* Now walk through all channels to find the opless ones. */
	gettimeofday(&tv_before, NULL);
	for (chptr = GlobalChannelList; chptr; chptr = chptr->nextch) {
		//chptr = (struct Channel*)(node_chan->data);
		if (chptr->users >= min_clients &&
			ocf_is_channel_opless(chptr)) {

			/* Only add this to the autofix list if :
			 * - there are scores on this channel in the database,
			 * - and there is currently a client that has a higher score
			 *   than the minimum absolute score.
			 * - and the channel is not already being fixed,
			 * - and the channel is not blocked. */
			channel_exists = DB_set_read_channel(chptr->chname);
			if (channel_exists && !is_being_fixed(chptr->chname)) {
				flags = DB_channel_get_flag();
				if (!(flags & OCF_CHANNEL_FLAG_BLOCK) &&
					has_a_scored_client(chptr))
				{
					ocf_log(OCF_LOG_AUTOFIX, "AUFX %s is opless, fixing.",
						chptr->chname);
					send_notice_to_loggedin(OCF_NOTICE_FLAGS_AUTOFIX,
						"AUTOFIX %s is opless, fixing.", chptr->chname);
					add_autofix_channel(chptr->chname);
					num_opless++;
				}
			}
		}
	}
	gettimeofday(&tv_after, NULL);
	duration = (tv_after.tv_usec - tv_before.tv_usec) +
		1000000 * (tv_after.tv_sec - tv_before.tv_sec);
	
	/* The following line is rather annoying, because it writes to
	 * the debug log every 15 seconds. TODO: this should be inside some
	 * #define block, and en/disabled in some config.h like file. For
	 * now, we'll leave it up to you to uncomment this line if you want
	 * to measure the speed of opless channel checking. */
	/*
	ocf_log(OCF_LOG_DEBUG, "FOPL Found %d opless of %d channels in %d us.", 
		num_opless, dlink_list_length(&global_channel_list), duration);
	 */
}

/* ----------------------------------------------------------------- */

void ocf_fix_chanfix_channels(void* args)
{
	dlink_node *ptr, *next_ptr;
	struct chanfix_channel *cfc;
	int channel_fixed;
	time_t now;

	/* If we're split, forget it. We don't want to perform any modes if
	 * we're split. */
	if (currently_split) {
		return;
	}

	now = time(NULL);

	DLINK_FOREACH_SAFE(ptr, next_ptr, chanfix_channel_list.head) {
		cfc = (struct chanfix_channel*)(ptr->data);
		channel_fixed = 0;

		/* Process stage 1 channels. */
		if (cfc->stage == 1) {
			channel_fixed = ocf_manual_fix_stage_one(cfc);

		/* Process stage 2 channels. */
		} else {
			if (now - cfc->time_previous_attempt < OCF_CHANFIX_INTERVAL) {
				/* do nothing */
			} else {
				channel_fixed = ocf_manual_fix_stage_two(cfc);
			}
		}

		/* If the channel has been fixed, or the fixing time window
		 * has passed, remove it from the list */
		if (channel_fixed ||
			now - cfc->time_fix_started > OCF_CHANFIX_MAXIMUM + OCF_CHANFIX_DELAY) {
			ocf_log(OCF_LOG_CHANFIX, "CHFX Manual fix of \"%s\" complete.",
				cfc->name);
			send_notice_to_loggedin(OCF_NOTICE_FLAGS_CHANFIX,
				"Manual chanfix of \"%s\" complete.", cfc->name);
			del_chanfix_channel(cfc->name);
		}
	}
}

/* ----------------------------------------------------------------- */

void ocf_fix_autofix_channels(void* args)
{
	dlink_node *ptr, *next_ptr;
	struct autofix_channel *afc;
	int channel_fixed;
	time_t now;

	/* If we're split, forget it. We don't want to perform any modes if
	 * we're split. */
	if (currently_split) {
		return;
	}

	now = time(NULL);

	DLINK_FOREACH_SAFE(ptr, next_ptr, autofix_channel_list.head) {
		afc = (struct autofix_channel*)(ptr->data);
		channel_fixed = 0;

		if (now - afc->time_previous_attempt < OCF_AUTOFIX_INTERVAL) {
			/* do nothing */
		} else {
			channel_fixed = ocf_fix_opless_channel(afc);
		}

		/* If the channel has been fixed, or the fixing time window
		 * has passed, remove it from the list */
		if (channel_fixed ||
			now - afc->time_fix_started > OCF_AUTOFIX_MAXIMUM) {
			ocf_log(OCF_LOG_AUTOFIX, "AUFX Autofix for \"%s\" complete.",
				afc->name);
			send_notice_to_loggedin(OCF_NOTICE_FLAGS_AUTOFIX,
				"Autofix for \"%s\" complete.", afc->name);
			del_autofix_channel(afc->name);
		}
	}
}

/* ----------------------------------------------------------------- */

void add_update_database_event() 
{
	int seconds_since_midnight, next_time_for_event;

	seconds_since_midnight = get_seconds_in_current_day();
	next_time_for_event = 
		seconds_since_midnight + OCF_DATABASE_UPDATE_TIME -
		(seconds_since_midnight % OCF_DATABASE_UPDATE_TIME);

	if (next_time_for_event >= 86400) {
		next_time_for_event = 86400;
	}

	eventAdd("ocf_update_database", &ocf_update_database, NULL, 
		next_time_for_event - seconds_since_midnight);

	ocf_log(OCF_LOG_DEBUG, "database update event, now %d, in %d secs",
			seconds_since_midnight, next_time_for_event - seconds_since_midnight);
}

/* ----------------------------------------------------------------- */

void add_save_database_event() 
{
	int seconds_since_midnight, next_time_for_event;

	seconds_since_midnight = get_seconds_in_current_day();
	next_time_for_event = 
		seconds_since_midnight + OCF_DATABASE_SAVE_TIME -
		(seconds_since_midnight % OCF_DATABASE_SAVE_TIME) +
		OCF_DATABASE_SAVE_OFFSET;

	if (next_time_for_event >= 86400) {
		next_time_for_event = 86400 + OCF_DATABASE_SAVE_OFFSET;
	}

	eventAdd("ocf_save_database", &ocf_save_database, NULL, 
		next_time_for_event - seconds_since_midnight);

	ocf_log(OCF_LOG_DEBUG, "database save event, now %d, in %d secs",
			seconds_since_midnight, next_time_for_event - seconds_since_midnight);
}

/* ----------------------------------------------------------------- */
