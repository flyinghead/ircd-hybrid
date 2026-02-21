/*
 * $Id: cf_fix.h,v 1.7 2004/08/24 22:21:52 cfh7 REL_2_1_3 $
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




/* Functions that deal with fixing channels, both manually and
 * automatically. */


/* All the below times are in seconds. */
/* The time between consecutive attempts to fix an opless channel */
#define OCF_AUTOFIX_INTERVAL 600

/* The maximum time to try to fix an opless channel */
#define OCF_AUTOFIX_MAXIMUM 3600

/* The max number of clients opped by chanfix during an autofix. */
#define OCF_AUTOFIX_NUM_OPPED 5

/* The time to wait between the TS-1 join and the first attempt to
 * give ops to people. */
#define OCF_CHANFIX_DELAY 30

/* The time between consecutive attempts to manually fix a channel */
#define OCF_CHANFIX_INTERVAL 300

/* The maximum time to try to manually fix a channel */
#define OCF_CHANFIX_MAXIMUM 3600

/* The max number of clients opped by chanfix during a manual fix. */
#define OCF_CHANFIX_NUM_OPPED 5


/* Score values.
 * The first 2 values are the minimum scores required at the beginning
 * of a chanfix; the last 2 values are the minimum scores at the very
 * end of the fix. Between these times, there is a linear decrease from
 * the high to the low values. */

/* Minimum absolute score required for chanfix to op, relative to
 * the maximum score possible (default: 0.20 * 4032). 
 * Make sure this value is between 0 and 1. */
#define OCF_FIX_MIN_ABS_SCORE_BEGIN 0.20f

/* Minimum score required for chanfix to op, relative to the maximum
 * score for this channel in the database, at the beginning of the fix.
 * Make sure this value is between 0 and 1. */
#define OCF_FIX_MIN_REL_SCORE_BEGIN 0.90f

/* Minimum absolute score required for chanfix to op, relative to
 * the maximum score possible (default: 0.04 * 4032). 
 * Make sure this value is between 0 and OCF_FIX_MIN_ABS_SCORE_BEGIN. */
#define OCF_FIX_MIN_ABS_SCORE_END 0.04f

/* Minimum score required for chanfix to op, relative to the maximum
 * score for this channel in the database. So if you have less than
 * 30% of the maximum score, chanfix will never op you.
 * Make sure this value is between 0 and OCF_FIX_MIN_REL_SCORE_BEGIN. */
#define OCF_FIX_MIN_REL_SCORE_END 0.30f

/* Data about any channel that is being manually fixed will be stored 
 * in a struct like this and put into the list of channels being
 * manually fixed. 
 * A chanfix channel has 2 stages:
 * Stage 1 - join with TS-1 and get rid of all the modes;
 * Stage 2 - give ops to the right people. */
struct chanfix_channel
{
	char name[CHANNELLEN];
	time_t time_fix_started;
	time_t time_previous_attempt;
	int stage;
	int max_score;
};

struct autofix_channel
{
	char name[CHANNELLEN];
	time_t time_fix_started;
	time_t time_previous_attempt;
	int max_score;
	int modes_removed;
};

/* List of channels that are being manually fixed. */
dlink_list chanfix_channel_list;

/* List of channels that are being automatically fixed. */
dlink_list autofix_channel_list;

/* Adds this channel to the list of channels being manually fixed.
 * Will not add the channel if it's already being any fixed. */
int add_chanfix_channel(const char* channel);

/* Adds this channel to the list of channels being automatically fixed.
 * Will not add the channel if it's already being any fixed. */
int add_autofix_channel(const char* channel);

/* Removes this channel from the list of channels being manually fixed,
 * if it is on there. */
int del_chanfix_channel(const char* channel);

/* Removes this channel from the list of channels being automatically fixed,
 * if it is on there. */
int del_autofix_channel(const char* channel);

/* Returns true if the channel is being manually chanfixed. */
int is_being_chanfixed(const char* channel);

/* Returns true if the channel is being automatically chanfixed. */
int is_being_autofixed(const char* channel);

/* Returns true if the channel is being manually or automatically chanfixed. */
int is_being_fixed(const char* channel);

/* Returns a pointer to the chanfix data of this channel, if it is being
 * manually chanfixed. Returns NULL if not. */
struct chanfix_channel* get_chanfix_channel(const char* channel);

/* Returns a pointer to the autofix data of this channel, if it is being
 * automatically chanfixed. Returns NULL if not. */
struct autofix_channel* get_autofix_channel(const char* channel);


/* Makes chanfix join chptr->chname with serverops, without disturbing
 * the TS of the channel. */
void ocf_ojoin_channel(struct Channel *chptr);

void ojoin_channel_decrease_ts(struct Channel *chptr);

void give_ops(struct Channel *chptr, const char *nick);
void part_channel(const char *chan);

/* Returns true if chptr->chname is opless, and false otherwise. */
int ocf_is_channel_opless(struct Channel *chptr);

/* Returns the number of channel ops in this channel. */
int ocf_get_number_chanops(struct Channel *chptr);

/* Returns true if this channel has any of the following modes set:
 * +b, +k, +l, +i. */
int opless_needs_modes_removed(struct Channel *chptr);

/* Fixing stuff */

/* First checks whether chanfix is currently allowed to autofix
 * channels. If so, it checks all channels to find the ones that
 * are opless, and fixes them. */
void ocf_fix_opless_channels();


int ocf_fix_opless_channel(struct autofix_channel *afc);

int ocf_manual_fix_stage_one(struct chanfix_channel *cfc);
int ocf_manual_fix_stage_two(struct chanfix_channel *cfc);

int find_scored_nicks(struct Channel *chptr, char **nicks, int num_nicks, 
					  int max_scored_nicks, struct OCF_chanuser* highscores,
					  int min_score_userhost);
