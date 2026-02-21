/*
 * $Id: cf_gather.c,v 1.10 2004/08/29 09:42:52 cfh7 REL_2_1_3 $
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



#include "cf_base.h"

/* ----------------------------------------------------------------- */
/* Walks through all channels. If a channel has enough clients in
 * it (at least the value of setting min_clients), the channel is
 * processed and its data is updated. */

void gather_all_channels()
{
	static int current_bucket;
	static int total_channels;
	static int qualifying_channels;
	static int total_time; /* in milliseconds */
	int num_channels_in_current_run, num_channels_to_process;
	int minclients = settings_get_int("min_clients");
	int required_status = OCF_OPPED;
	struct timeval tv_before, tv_after;
	long duration;

	/* Rotate the database if it's midnight. */
	if (get_days_since_epoch() > current_day_since_epoch) {
		current_day_since_epoch = get_days_since_epoch();
		ocf_log(OCF_LOG_MAIN, "DATA Rotating database.");
		send_notice_to_loggedin(OCF_NOTICE_FLAGS_MAIN,
			"Rotating database.");
		DB_update_day(current_day_since_epoch);
	}

	/* Check whether enough servers are currently linked. */
	ocf_check_split(NULL);

	/* If not, bail out. We don't want to gather scores if there are not
	 * enough servers linked. */
	if (currently_split) {
		eventDelete(ocf_update_database, NULL);
		add_update_database_event();
		return;
	}

	/* Determine the parameters of the gathering. */
	if (minclients < 2) {
		minclients = OCF_MINCLIENTS;
	}

	if (settings_get_bool("client_needs_ident")) {
		required_status |= OCF_IDENTED;
	}

	if (settings_get_bool("client_needs_reverse")) {
		required_status |= OCF_REVERSED;
	}

	/* If we weren't busy gathering channels, this is a new round.
	 * Initialise some vars. */
	if (!busy_gathering_channels) {
		busy_gathering_channels = 1;
		current_bucket          = 0;
		qualifying_channels     = 0;
		total_channels          = 0;
		total_time              = 0;
	}

	/* Figure out how many channels to process in this run. */
	num_channels_to_process = settings_get_int("channels_per_run");
	if (num_channels_to_process < 1) { 
		num_channels_to_process = OCF_CHANNELS_PER_RUN;
	}

	/* Process them channels. */
	num_channels_in_current_run = 0;
	gettimeofday(&tv_before, NULL);
	while (current_bucket < CH_MAX &&
			num_channels_in_current_run < num_channels_to_process) {
		num_channels_in_current_run += 
			gather_channels_in_bucket(current_bucket, minclients, 
			required_status, &qualifying_channels);

		current_bucket++;
	}
	gettimeofday(&tv_after, NULL);
	duration = (tv_after.tv_usec - tv_before.tv_usec) +
		1000000 * (tv_after.tv_sec - tv_before.tv_sec);
	total_time     += duration;
	total_channels += num_channels_in_current_run;

	/* Check whether this was the last run. */
	if (current_bucket == CH_MAX) {
		/* Done */
		ocf_log(OCF_LOG_MAIN, "DBGA Gathered total chans: %d/%d in %d us.", 
			qualifying_channels, total_channels, total_time);
		current_bucket = 0;
		busy_gathering_channels = 0;
		eventDelete(ocf_update_database, NULL);
		add_update_database_event();
	} else {
		ocf_log(OCF_LOG_DEBUG, 
			"DBGA gathered %d chans in %d ms. bucket is now %d.", 
			num_channels_in_current_run, duration, current_bucket);
		/* Continue in 1 second from now. */
		eventDelete(ocf_update_database, NULL);
		eventAdd("ocf_update_database", &ocf_update_database, NULL, 1);
	}
}

/* ----------------------------------------------------------------- */

int gather_channels_in_bucket(unsigned int current_bucket, 
	int minclients, int required_status, int *qualifying_channels)
{
	struct Channel *chptr;
	int num_clients, num_channels = 0;
	struct HashEntry bucket;

	bucket = hash_get_channel_block(current_bucket);

	if (bucket.hits == 0 && bucket.links == 0) {
		return 0;
	}

	//chptr = hash_get_chptr(current_bucket);
	chptr = (struct Channel*) hash_get_channel_block(current_bucket).list;

	while (chptr) {
		num_clients = chptr->users;
		/* Only update channels which have the minimum of clients. */
		if (num_clients > OCF_NUM_STATIC_USERHOSTS) {
			update_scores_of_large_channel(chptr, num_clients, required_status);
			(*qualifying_channels)++;
		} else if (num_clients >= minclients) {
			update_scores_of_channel(chptr, num_clients, required_status);
			(*qualifying_channels)++;
		}

		num_channels++;
		chptr = chptr->hnextch;
	}

	return num_channels;
}

/* ----------------------------------------------------------------- */
/* Checks all clients in the channel to see whether they match the
 * required criteria to score a point. All unique userhosts that do
 * match gain a point.
 * NOTE: two or more opped clients with the same userhost still score only
 * 1 point.
 */
void update_scores_of_channel(struct Channel* chptr, 
							  int num_clients, int required_status) 
{
	int num_opped;
	dlink_node *node_client;
	struct Client *clptr;
	int status;
	char *users[OCF_NUM_STATIC_USERHOSTS],
		 *hosts[OCF_NUM_STATIC_USERHOSTS];
	int i;

	num_opped = 0;

	/* Walk through all opped clients on the channel, adding their userhost
	 * to the array of userhosts if they are opped. Make sure that
	 * each userhost only appears in the array once. */
	DLINK_FOREACH(node_client, chptr->chanops.head) {
		clptr = (struct Client*)(node_client->data);
		status = OCF_OPPED;
		if (clptr->username[0] != '~') {
			status |= OCF_IDENTED;
		}

		if ((status & required_status) == required_status) {
			num_opped = add_userhost_to_array(users, hosts, num_opped, 
				clptr->username, clptr->host);
		}
	}

#ifdef REQUIRE_OANDV   
	DLINK_FOREACH(node_client, chptr->chanops_voiced.head) {
		clptr = (struct Client*)(node_client->data);
		status = OCF_OPPED;
		if (clptr->username[0] != '~') {
			status |= OCF_IDENTED;
		}

		if ((status & required_status) == required_status) {
			num_opped = add_userhost_to_array(users, hosts, num_opped, 
				clptr->username, clptr->host);
		}
	}
#endif

	/* Now add these userhosts to the database. */
	DB_set_write_channel(chptr->chname);
	for (i = 0; i < num_opped; i++) {
		DB_register_op(users[i], hosts[i]);
	}
}

/* ----------------------------------------------------------------- */
/* Checks all clients in the channel to see whether they match the
 * required criteria to score a point. All unique userhosts that do
 * match gain a point.
 * NOTE: two or more opped clients with the same userhost still score only
 * 1 point.
 */
void update_scores_of_large_channel(struct Channel* chptr, 
									int num_clients, int required_status) 
{
	int num_opped;
	dlink_node *node_client;
	struct Client *clptr;
	int status;
	char **users, **hosts;
	int i;

	num_opped = 0;

	/* If the channel has more than OCF_NUM_STATIC_USERHOSTS clients,
	 * allocate enough room for 1 userhost per client on the channel.
	 * Otherwise we'll just use the static users/hosts arrays. */
	users = (char**) MyMalloc(num_clients * sizeof(char*));
	hosts = (char**) MyMalloc(num_clients * sizeof(char*));

	/* Walk through all clients on the channel, adding their userhost
	 * to the array of userhosts if they are opped. Make sure that
	 * each userhost only appears in the array once. */
	DLINK_FOREACH(node_client, chptr->chanops.head) {
		clptr = (struct Client*)(node_client->data);
		status = OCF_OPPED;
		if (clptr->username[0] != '~') {
			status |= OCF_IDENTED;
		}

		if ((status & required_status) == required_status) {
			num_opped = add_userhost_to_array(users, hosts, num_opped, 
				clptr->username, clptr->host);
		}
	}

#ifdef REQUIRE_OANDV   
	DLINK_FOREACH(node_client, chptr->chanops_voiced.head) {
		clptr = (struct Client*)(node_client->data);
		status = OCF_OPPED;
		if (clptr->username[0] != '~') {
			status |= OCF_IDENTED;
		}

		if ((status & required_status) == required_status) {
			num_opped = add_userhost_to_array(users, hosts, num_opped, 
				clptr->username, clptr->host);
		}
	}
#endif

	/* Now add these userhosts to the database. */
	DB_set_write_channel(chptr->chname);
	for (i = 0; i < num_opped; i++) {
		DB_register_op(users[i], hosts[i]);
	}

	MyFree(users);
	MyFree(hosts);
}

/* ----------------------------------------------------------------- */
