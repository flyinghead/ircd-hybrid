/*
 * $Id: cf_fix.c,v 1.10 2004/08/24 18:55:22 cfh7 REL_2_1_3 $
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




/* ----------------------------------------------------------------- */

#include "cf_base.h"

/* ----------------------------------------------------------------- */

int is_being_chanfixed(const char* channel)
{
	dlink_node *node;
	struct chanfix_channel* cfc;

	DLINK_FOREACH(node, chanfix_channel_list.head) {
		cfc = (struct chanfix_channel*)(node->data);
		if (!strcmp(cfc->name, channel)) {
			return 1;
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int is_being_autofixed(const char* channel)
{
	dlink_node *node;
	struct autofix_channel* afc;

	DLINK_FOREACH(node, autofix_channel_list.head) {
		afc = (struct autofix_channel*)(node->data);
		if (!strcmp(afc->name, channel)) {
			return 1;
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int is_being_fixed(const char* channel)
{
	return(is_being_chanfixed(channel) || is_being_autofixed(channel));
}

/* ----------------------------------------------------------------- */

int add_chanfix_channel(const char* channel)
{
	struct chanfix_channel *cfc;
	struct OCF_topops channelscores;

	/* Don't add a channel twice */
	if (is_being_fixed(channel)) {
		return 0;
	}

	cfc = MyMalloc(sizeof(struct chanfix_channel));
	strncpy(cfc->name, channel, sizeof(cfc->name));
	cfc->time_fix_started = time(NULL);
	cfc->time_previous_attempt = 0;
	cfc->stage = 1;

	DB_set_read_channel((char*)channel);
	DB_poll_channel( &channelscores, (char**)1, (char**)1, 0, (char**)1, (char**)1, 0);
	cfc->max_score = channelscores.topscores[0];
	ocf_log(OCF_LOG_DEBUG, "Added manual fix for %s with max score %d",
		channel, channelscores.topscores[0]);
	DB_set_write_channel((char*)channel);
	DB_register_manualfix(time(NULL));

	dlinkAdd(cfc, make_dlink_node(), &chanfix_channel_list);
	return 1;
}

/* ----------------------------------------------------------------- */

int add_autofix_channel(const char* channel)
{
	struct autofix_channel *afc;
	struct OCF_topops channelscores;

	/* Don't add a channel twice */
	if (is_being_fixed(channel)) {
		return 0;
	}

	afc = MyMalloc(sizeof(struct autofix_channel));
	strncpy(afc->name, channel, sizeof(afc->name));
	afc->time_fix_started = time(NULL);
	afc->time_previous_attempt = 0;

	DB_set_read_channel((char*)channel);
	DB_poll_channel( &channelscores, (char**)1, (char**)1, 0, (char**)1, (char**)1, 0);
	afc->max_score = channelscores.topscores[0];
	DB_set_write_channel((char*)channel);
	DB_register_autofix(time(NULL));

	dlinkAdd(afc, make_dlink_node(), &autofix_channel_list);
	return 1;
}

/* ----------------------------------------------------------------- */

int del_chanfix_channel(const char* channel)
{
	dlink_node *node;
	struct chanfix_channel* cfc;

	DLINK_FOREACH(node, chanfix_channel_list.head) {
		cfc = (struct chanfix_channel*)(node->data);
		if (!strcmp(cfc->name, channel)) {
			dlinkDelete(node, &chanfix_channel_list);
			MyFree(cfc);
			return 1;
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int del_autofix_channel(const char* channel)
{
	dlink_node *node;
	struct autofix_channel* afc;

	DLINK_FOREACH(node, autofix_channel_list.head) {
		afc = (struct autofix_channel*)(node->data);
		if (!strcmp(afc->name, channel)) {
			dlinkDelete(node, &autofix_channel_list);
			MyFree(afc);
			return 1;
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

struct chanfix_channel* get_chanfix_channel(const char* channel)
{
	dlink_node *node;
	struct chanfix_channel* cfc;

	DLINK_FOREACH(node, chanfix_channel_list.head) {
		cfc = (struct chanfix_channel*)(node->data);
		if (!strcmp(cfc->name, channel)) {
			return cfc;
		}
	}

	return NULL;
}

/* ----------------------------------------------------------------- */

struct autofix_channel* get_autofix_channel(const char* channel)
{
	dlink_node *node;
	struct autofix_channel* afc;

	DLINK_FOREACH(node, autofix_channel_list.head) {
		afc = (struct autofix_channel*)(node->data);
		if (!strcmp(afc->name, channel)) {
			return afc;
		}
	}

	return NULL;
}

/* ----------------------------------------------------------------- */

void ocf_ojoin_channel(struct Channel *chptr)
{
	if (IsMember(chanfix, chptr))
    {
		/* TODO: proper error resolving. But this should never
		 * happen. Hi Murphy. */
		return;
    }
  
	add_user_to_channel(chptr, chanfix, CHFL_CHANOP);

	sendto_server(&me, chanfix, chptr, NOCAPS, NOCAPS, LL_ICLIENT, 
			 ":%s SJOIN %lu %s + :@%s", me.name,
			 (unsigned long)chptr->channelts,
			 chptr->chname, chanfix->name);

	sendto_channel_local(ALL_MEMBERS, chptr, ":%s!%s@%s JOIN %s",
				  chanfix->name,
				  chanfix->username,
				  chanfix->host,
				  chptr->chname);
      
	sendto_channel_local(ALL_MEMBERS, chptr, ":%s MODE %s +o %s",
					me.name, chptr->chname, chanfix->name);
}

/* ----------------------------------------------------------------- */

void ojoin_channel_decrease_ts(struct Channel *chptr)
{
	dlink_node *ptr, *next_ptr;
	struct Client *clptr;

	if (!chptr) {
		return;
	}

	if (IsMember(chanfix, chptr)) {
		return;
    }

	/* Don't TS-1 join a channel if the TS is 0 or 1. */
	if ((unsigned long)chptr->channelts <= 1) {
		return;
	}
  
	add_user_to_channel(chptr, chanfix, CHFL_CHANOP);

	/* Send the SJOIN with TS-1 to all other servers */
	sendto_server(&me, chanfix, chptr, NOCAPS, NOCAPS, LL_ICLIENT, 
				 ":%s SJOIN %lu %s + :@%s", me.name,
				 (unsigned long)chptr->channelts - 1,
				 chptr->chname, chanfix->name);
  
	/* Local server: let chanfix join */
	sendto_channel_local(ALL_MEMBERS, chptr, ":%s!%s@%s JOIN %s",
				  chanfix->name,
				  chanfix->username,
				  chanfix->host,
				  chptr->chname);

	/* On the local server, DEOP all others */
	DLINK_FOREACH_SAFE(ptr, next_ptr, chptr->chanops.head) {
		clptr = (struct Client*)(ptr->data);
		/* Don't deop chanfix! */
		if ( clptr != chanfix ) {
			/* Send deop to the channel */
			sendto_channel_local(ALL_MEMBERS, chptr, ":%s MODE %s -o %s",
							me.name, chptr->chname, clptr->name);
			/* Change the status of this client in the local server */
			remove_user_from_channel(chptr, clptr);
			add_user_to_channel(chptr, clptr, MODE_PEON);
		}
	}

#ifdef REQUIRE_OANDV
	DLINK_FOREACH_SAFE(ptr, next_ptr, chptr->chanops_voiced.head) {
		clptr = (struct Client*)(ptr->data);
		/* Don't deop chanfix! */
		if ( clptr != chanfix ) {
			/* Send deop to the channel */
			sendto_channel_local(ALL_MEMBERS, chptr, ":%s MODE %s -o %s",
							me.name, chptr->chname, clptr->name);
			/* Change the status of this client in the local server */
			remove_user_from_channel(chptr, clptr);
			add_user_to_channel(chptr, clptr, MODE_VOICE);
		}
	}
#endif

	/* Local server: OP chanfix */
	sendto_channel_local(ALL_MEMBERS, chptr, ":%s MODE %s +o %s",
					me.name, chptr->chname, chanfix->name);

	/* Local server: reduces TS */
	chptr->channelts--;
}

/* ----------------------------------------------------------------- */

void give_ops(struct Channel *chptr, const char *nick)
{
	issue_chanfix_command("MODE %s +o %s", chptr->chname, nick);
	ocf_log(OCF_LOG_DEBUG, "MODE %s +o %s", chptr->chname, nick);
}

/* ----------------------------------------------------------------- */

void part_channel(const char *chan)
{
	issue_chanfix_command("PART %s", chan);
}

/* ----------------------------------------------------------------- */

int ocf_is_channel_opless(struct Channel *chptr)
{
#ifdef REQUIRE_OANDV
	if (dlink_list_length(&chptr->chanops) == 0 &&
	    dlink_list_length(&chptr->chanops_voiced) == 0) {
		return 1;
	}
#else
	if (dlink_list_length(&chptr->chanops) == 0) {
		return 1;
	}
#endif

	return 0;
}

/* ----------------------------------------------------------------- */

int ocf_get_number_chanops(struct Channel *chptr)
{
#ifdef REQUIRE_OANDV
	return dlink_list_length(&chptr->chanops) +
		dlink_list_length(&chptr->chanops_voiced);
#else
	return dlink_list_length(&chptr->chanops);
#endif
}

/* ----------------------------------------------------------------- */

int opless_needs_modes_removed(struct Channel *chptr)
{
	/* Modes need to be removed if +i/k/l/b is set. 
	 * This check should actually be more specific (checking each ban
	 * to see if it matches a high scored hostmask) but this will do
	 * for now. */
	if (chptr->mode.mode & MODE_INVITEONLY ||
		chptr->mode.key[0] != '\0' ||
		chptr->mode.limit != 0 ||
		dlink_list_length(&chptr->banlist) > 0)
    {
		return 1;
	}

	return 0;
}

/* ----------------------------------------------------------------- */
/* Long shitty function. Bleh. */

int ocf_fix_opless_channel(struct autofix_channel *afc)
{
	struct Channel* chptr;
	int arraypos;
	int num_clients, num_clients_to_be_opped, num_ops_now,
		num_userhosts_with_score;
	struct Client *clptr;
	dlink_node *node_client;
	struct OCF_chanuser* highscores;
	char **users, **hosts, **nicks_to_be_opped;
	int num_nicks_to_be_opped;
	time_t now;
	int min_score_abs, min_score_rel, min_score;
	int time_since_start;
	int i;

	/* First update the time of the previous attempt to now. */
	now = time(NULL);
	afc->time_previous_attempt = now;

	chptr = hash_find_channel(afc->name);

	/* If the channel doesn't exist (anymore), the fix is successful. */
	if (!chptr) {
		return 1;
	}

	/* If the max score of the channel is lower than the absolute minimum
	 * score required, don't even bother trying. */
	if (afc->max_score <= OCF_FIX_MIN_ABS_SCORE_END * OCF_MAX_SCORE) {
		return 0;
	}

	/* Get the number of clients that should have ops */
	num_clients_to_be_opped = settings_get_int("num_clients_opped");
	if (num_clients_to_be_opped < 1) {
		num_clients_to_be_opped = OCF_NUM_CLIENTS_OPPED;
	}

	/* If the channel has enough ops, abort & return. */
	num_ops_now = ocf_get_number_chanops(chptr);
	if (num_ops_now >= num_clients_to_be_opped) {
		return 1;
	}

	time_since_start = now - (afc->time_fix_started);

	num_clients = chptr->users;
	highscores = MyMalloc(num_clients_to_be_opped * sizeof(struct OCF_chanuser));
	users = MyMalloc(num_clients * sizeof(char*));
	hosts = MyMalloc(num_clients * sizeof(char*));

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

	/* Determine minimum score required for this time. */

	/* Linear interpolation of (0, fraction_abs_max * max_score) -> 
	 * (max_time, fraction_abs_min * max_score)
	 * at time t between 0 and max_time. */
	min_score_abs = (OCF_MAX_SCORE * OCF_FIX_MIN_ABS_SCORE_BEGIN) -
		(float)time_since_start / OCF_AUTOFIX_MAXIMUM * 
		(OCF_MAX_SCORE * OCF_FIX_MIN_ABS_SCORE_BEGIN - OCF_FIX_MIN_ABS_SCORE_END * OCF_MAX_SCORE);

	ocf_log(OCF_LOG_DEBUG, "AFC max %d begin %f end %f time %d maxtime %d",
		OCF_MAX_SCORE, OCF_FIX_MIN_ABS_SCORE_BEGIN, OCF_FIX_MIN_ABS_SCORE_END,
		time_since_start, OCF_AUTOFIX_MAXIMUM);

	/* Linear interpolation of (0, fraction_rel_max * max_score_channel) -> 
	 * (max_time, fraction_rel_min * max_score_channel)
	 * at time t between 0 and max_time. */
	min_score_rel = (afc->max_score * OCF_FIX_MIN_REL_SCORE_BEGIN) -
		(float)time_since_start / OCF_AUTOFIX_MAXIMUM * 
		(afc->max_score * OCF_FIX_MIN_REL_SCORE_BEGIN - OCF_FIX_MIN_REL_SCORE_END * afc->max_score);
	
	/* The minimum score needed for ops is the HIGHER of these two
	 * scores. */
	min_score = min_score_abs;
	if (min_score_rel > min_score)
		min_score = min_score_rel;

	ocf_log(OCF_LOG_DEBUG, "autofix. start %d, delta %d, max %d, minabs %d, minrel %d.",
		afc->time_fix_started, time_since_start, afc->max_score, min_score_abs, min_score_rel);

	/* Get the scores of the userhosts of the non-opped clients. */
	DB_set_read_channel(chptr->chname);
	num_userhosts_with_score = DB_get_top_user_hosts(highscores, users, hosts, 
		arraypos, num_clients_to_be_opped);

	MyFree(users);
	MyFree(hosts);

	/* If no scores are high enough, return. */
	if ( (num_userhosts_with_score == 0 ||
			highscores[0].score < min_score) && 
			ocf_get_number_chanops(chptr) == 0) {
		MyFree(highscores);
		/* Now remove all channel modes just to make sure the regular ops
		 * can join the channel. */
		if (!afc->modes_removed && opless_needs_modes_removed(chptr)) {
			afc->modes_removed = 1;
			ocf_ojoin_channel(chptr);
			chanfix_remove_channel_modes(chptr);
			chanfix_send_privmsg(chptr->chname, "I only joined to remove modes.");
			part_channel(chptr->chname);
		}
		return 0;
	}

	/* Find out which nicks need to be opped. */
	nicks_to_be_opped = (char**)MyMalloc(num_clients_to_be_opped * sizeof(char*));
	for (i = 0; i < num_clients_to_be_opped; i++)
		nicks_to_be_opped[i] = (char*)MyMalloc((NICKLEN + 1) * sizeof(char));

	num_nicks_to_be_opped = 
		find_scored_nicks(chptr, nicks_to_be_opped, num_clients_to_be_opped,
		num_clients_to_be_opped - num_ops_now, highscores, min_score);
	MyFree(highscores);

	/* If we need to op at least one client, go into the channel and give out
	 * some ops. */
	if (num_nicks_to_be_opped > 0) {
		ocf_ojoin_channel(chptr);
		chanfix_perform_mode_group(chptr->chname, '+', 'o', 
			nicks_to_be_opped, num_clients_to_be_opped);
		if (num_nicks_to_be_opped == 1) {
			chanfix_send_privmsg(chptr->chname, "1 client should have been opped.");
		} else {
			chanfix_send_privmsg(chptr->chname, "%d clients should have been opped.",
				num_nicks_to_be_opped);
		}
		part_channel(chptr->chname);
	}

	for (i = 0; i < num_clients_to_be_opped; i++)
		MyFree(nicks_to_be_opped[i]);

	MyFree(nicks_to_be_opped);

	/* Now see if there are enough ops; if so, the fix is complete. */
	if (num_nicks_to_be_opped + num_ops_now >= num_clients ||
			num_nicks_to_be_opped + num_ops_now >= num_clients_to_be_opped) {
		return 1;
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int ocf_manual_fix_stage_one(struct chanfix_channel *cfc)
{
	struct Channel* chptr;

	chptr = hash_find_channel(cfc->name);

	/* If the channel doesn't exist (anymore), the fix is successful. */
	if (!chptr) {
		return 1;
	}

	cfc->stage = 2;

	/* If the TS is larger than 1, SJOIN the channel with TS - 1 to get
	 * rid of all modes. If the TS is 0 or 1, SJOIN with the same TS and
	 * manually remove all modes. */
	if ((unsigned long)chptr->channelts > 1) {
		ojoin_channel_decrease_ts(chptr);
	} else {
		ocf_ojoin_channel(chptr);
	}

	chanfix_send_privmsg(chptr->chname, "Channel fix in progress, please stand by.");
	chanfix_remove_channel_modes(chptr);
	part_channel(chptr->chname);

	/* Now make sure that OCF_CHANFIX_DELAY seconds from now, the first
	 * fixing attempt will take place. */
	cfc->time_previous_attempt = 
		time(NULL) + OCF_CHANFIX_DELAY - OCF_CHANFIX_INTERVAL;

	cfc->time_fix_started = time(NULL);

	return 0;
}

/* ----------------------------------------------------------------- */
/* Long shitty function. Bleh. */

int ocf_manual_fix_stage_two(struct chanfix_channel *cfc)
{
	struct Channel* chptr;
	int arraypos;
	int num_clients, num_clients_to_be_opped, num_ops_now,
		num_userhosts_with_score;
	struct Client *clptr;
	dlink_node *node_client;
	struct OCF_chanuser* highscores;
	char **users, **hosts, **nicks_to_be_opped;
	int num_nicks_to_be_opped;
	time_t now;
	int min_score_abs, min_score_rel, min_score;
	int time_since_start;
	int i;

	/* First update the time of the previous attempt to now. */
	now = time(NULL);
	cfc->time_previous_attempt = now;

	chptr = hash_find_channel(cfc->name);

	/* If the channel doesn't exist (anymore), the fix is successful. */
	if (!chptr) {
		return 1;
	}

	/* Get the number of clients that should have ops */
	num_clients_to_be_opped = settings_get_int("num_clients_opped");
	if (num_clients_to_be_opped < 1) {
		num_clients_to_be_opped = OCF_NUM_CLIENTS_OPPED;
	}

	/* If the channel has enough ops, abort & return. */
	num_ops_now = ocf_get_number_chanops(chptr);
	if (num_ops_now >= num_clients_to_be_opped) {
		return 1;
	}

	time_since_start = now - (cfc->time_fix_started + OCF_CHANFIX_DELAY);

	num_clients = chptr->users;
	highscores = MyMalloc(num_clients_to_be_opped * sizeof(struct OCF_chanuser));
	users = MyMalloc(num_clients * sizeof(char*));
	hosts = MyMalloc(num_clients * sizeof(char*));

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

	/* Determine minimum score required for this time. */

	/* Linear interpolation of (0, fraction_abs_max * max_score) -> 
	 * (max_time, fraction_abs_min * max_score)
	 * at time t between 0 and max_time. */
	min_score_abs = (OCF_MAX_SCORE * OCF_FIX_MIN_ABS_SCORE_BEGIN) -
		(float)time_since_start / OCF_CHANFIX_MAXIMUM * 
		(OCF_MAX_SCORE * OCF_FIX_MIN_ABS_SCORE_BEGIN - OCF_FIX_MIN_ABS_SCORE_END * OCF_MAX_SCORE);

	ocf_log(OCF_LOG_DEBUG, "max %d begin %f end %f time %d maxtime %d",
		OCF_MAX_SCORE, OCF_FIX_MIN_ABS_SCORE_BEGIN, OCF_FIX_MIN_ABS_SCORE_END,
		time_since_start, OCF_CHANFIX_MAXIMUM);

	/* Linear interpolation of (0, fraction_rel_max * max_score_channel) -> 
	 * (max_time, fraction_rel_min * max_score_channel)
	 * at time t between 0 and max_time. */
	min_score_rel = (cfc->max_score * OCF_FIX_MIN_REL_SCORE_BEGIN) -
		(float)time_since_start / OCF_CHANFIX_MAXIMUM * 
		(cfc->max_score * OCF_FIX_MIN_REL_SCORE_BEGIN - OCF_FIX_MIN_REL_SCORE_END * cfc->max_score);
	
	/* The minimum score needed for ops is the HIGHER of these two
	 * scores. */
	min_score = min_score_abs;
	if (min_score_rel > min_score)
		min_score = min_score_rel;

	ocf_log(OCF_LOG_DEBUG, "chanfix. start %d, delta %d, max %d, minabs %d, minrel %d.",
		cfc->time_fix_started, time_since_start, cfc->max_score, min_score_abs, min_score_rel);

	/* Get the scores of the userhosts of the non-opped clients. */
	DB_set_read_channel(chptr->chname);
	num_userhosts_with_score = DB_get_top_user_hosts(highscores, users, hosts, 
		arraypos, num_clients_to_be_opped);

	MyFree(users);
	MyFree(hosts);

	/* If no scores are high enough, return. */
	if (num_userhosts_with_score == 0 ||
			highscores[0].score < min_score) {
		MyFree(highscores);
		return 0;
	}

	/* Find out which nicks need to be opped. */
	nicks_to_be_opped = (char**)MyMalloc(num_clients_to_be_opped * sizeof(char*));
	for (i = 0; i < num_clients_to_be_opped; i++)
		nicks_to_be_opped[i] = (char*)MyMalloc((NICKLEN + 1) * sizeof(char));

	num_nicks_to_be_opped = 
		find_scored_nicks(chptr, nicks_to_be_opped, num_clients_to_be_opped,
		num_clients_to_be_opped - num_ops_now, highscores, min_score);
	MyFree(highscores);

	/* If we need to op at least one client, go into the channel and give out
	 * some ops. */
	if (num_nicks_to_be_opped > 0) {
		ocf_ojoin_channel(chptr);
		chanfix_perform_mode_group(chptr->chname, '+', 'o', 
			nicks_to_be_opped, num_clients_to_be_opped);
		if (num_nicks_to_be_opped == 1) {
			chanfix_send_privmsg(chptr->chname, "1 client should have been opped.");
		} else {
			chanfix_send_privmsg(chptr->chname, "%d clients should have been opped.",
				num_nicks_to_be_opped);
		}
		part_channel(chptr->chname);
	}

	for (i = 0; i < num_clients_to_be_opped; i++)
		MyFree(nicks_to_be_opped[i]);

	MyFree(nicks_to_be_opped);

	/* Now see if there are enough ops; if so, the fix is complete. */
	if (num_nicks_to_be_opped + num_ops_now >= num_clients ||
			num_nicks_to_be_opped + num_ops_now >= num_clients_to_be_opped) {
		return 1;
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int find_scored_nicks(struct Channel *chptr, char **nicks, int num_nicks, 
					  int max_scored_nicks, struct OCF_chanuser* highscores,
					  int min_score_userhost)
{
	int arraypos = 0;
	int scored_nicks = 0;
	struct Client *clptr;
	dlink_node *node_client;

	while (scored_nicks < max_scored_nicks &&
			arraypos < num_nicks &&
			highscores[arraypos].score >= min_score_userhost) {
		/* Walk through all non-opped clients in the channel and see if they have
		 * the user and host in highscores[arraypos]. If so, add the nick
		 * to the nicks array. */
		DLINK_FOREACH(node_client, chptr->voiced.head) {
			clptr = (struct Client*)(node_client->data);
			if (!strcasecmp(clptr->username, highscores[arraypos].user) &&
					!strcasecmp(clptr->host, highscores[arraypos].host)) {
				/* Client deserves ops. Add it. Return if this means
				 * we have enough nicks in the array. */
				strncpy(nicks[scored_nicks], clptr->name, NICKLEN + 1);
				scored_nicks++;
				if (scored_nicks >= max_scored_nicks) {
					return scored_nicks;
				}
			}
		}

		DLINK_FOREACH(node_client, chptr->peons.head) {
			clptr = (struct Client*)(node_client->data);
			if (!strcasecmp(clptr->username, highscores[arraypos].user) &&
					!strcasecmp(clptr->host, highscores[arraypos].host)) {
				/* Client deserves ops. Add it. Return if this means
				 * we have enough nicks in the array. */
				strncpy(nicks[scored_nicks], clptr->name, NICKLEN + 1);
				scored_nicks++;
				if (scored_nicks >= max_scored_nicks) {
					return scored_nicks;
				}
			}
		}

		arraypos++;
	}

	return scored_nicks;
}

/* ----------------------------------------------------------------- */

