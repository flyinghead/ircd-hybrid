/*
 * $Id: cf_process.c,v 1.27 2004/10/30 19:50:53 cfh7 REL_2_1_3 $
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

void process_cmd_wrong_syntax(struct message_data *md, struct command_data *cd)
{
	int parc;
	char *parv[OCF_MAXPARA];
	char help[IRCLINELEN];

	get_command_help(cd->command, help, sizeof(help));
	parc = tokenize_string_separator(help, parv, '\n');

	if (parc == 0) {
		chanfix_send_privmsg(md->nick, 
			"I do not know the syntax for this command.");
	} else {
		chanfix_send_privmsg(md->nick, "Please use the following syntax:");
		chanfix_send_privmsg(md->nick, "%s", parv[0]);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_unknown(struct message_data *md, struct command_data *cd)
{
	chanfix_send_privmsg(md->nick, 
		"Unknown command. Please use \"HELP\" for help.");
}

/* ----------------------------------------------------------------- */
/* HELP [command] 
 * cd->params - command
 */

void process_cmd_help(struct message_data *md, struct command_data *cd)
{
	int parc;
	char *parv[OCF_MAXPARA];
	char help[IRCLINELEN];
	int i, cmd, flag;
	struct ocf_loggedin_user *olu;
	olu = ocf_get_loggedin_user(md->nick);

	if (cd->params[0]) {
		cmd = get_command_int_from_word(cd->params);
		/* Don't show help for commands that require you to be logged in,
		 * unless you're actually logged in. */
		if ( cmd == CMD_UNKNOWN ||
			(command_requires_loggedin(cmd) && olu == NULL) ) {
			chanfix_send_privmsg(md->nick, "Unknown command %s.", cd->params);
		} else {
			get_command_help(cmd, help, sizeof(help));
			parc = tokenize_string_separator(help, parv, '\n');
			for (i = 0; i < parc; i++) {
				chanfix_send_privmsg(md->nick, "%s", parv[i]);
			}
			/* Add example command for LOGIN */
			if (cmd == 4) {
				chanfix_send_privmsg(md->nick, 
					"Example: /MSG %s@%s LOGIN %s mypassword",
					chanfix->username, me.name, md->nick);
			}
			flag = cmd_flags_required[cmd];
			if (flag) {
				if (ocf_user_flag_get_char(flag) != ' ') {
					chanfix_send_privmsg(md->nick, "This command requires flag %c.", 
						ocf_user_flag_get_char(flag));
				} else {
					chanfix_send_privmsg(md->nick, "This command requires one of these flags: %s.", 
						ocf_user_flags_to_string(flag));
				}
			}
		}
	} else {
		/* TODO: don't show commands you can't execute. */
		send_command_list(md->nick);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_score(struct message_data *md, struct command_data *cd)
{
	struct Channel* chptr;
	int dummy;

	/* Check if there are scores in the database for this channel. */
	DB_set_read_channel(cd->channel);
	if (!DB_get_op_scores( &dummy, 1 )) {
		chanfix_send_privmsg(md->nick, 
			"There are no scores in the database for \"%s\".", cd->channel);
		return;
	}
	
	if (cd->nick[0] != 0) {
		process_cmd_score_nick(md, cd, 0);
		return;
	}

	if (cd->user[0] != 0) {
		process_cmd_score_hostmask(md, cd, 0);
		return;
	}

	chptr = hash_find_channel(cd->channel);

	if (chptr) {
		process_cmd_score_existing_channel(md, cd, chptr, 0);
	} else {
		process_cmd_score_nonexisting_channel(md, cd, 0);
	}

}

/* ----------------------------------------------------------------- */

void process_cmd_cscore(struct message_data *md, struct command_data *cd)
{
	struct Channel* chptr;
	int dummy;

	/* Check if there are scores in the database for this channel. */
	DB_set_read_channel(cd->channel);
	if (!DB_get_op_scores( &dummy, 1 )) {
		chanfix_send_privmsg(md->nick, 
			"~! %s", cd->channel);
		return;
	}
	
	if (cd->nick[0] != 0) {
		process_cmd_score_nick(md, cd, 1);
		return;
	}

	if (cd->user[0] != 0) {
		process_cmd_score_hostmask(md, cd, 1);
		return;
	}

	chptr = hash_find_channel(cd->channel);

	if (chptr) {
		process_cmd_score_existing_channel(md, cd, chptr, 1);
	} else {
		process_cmd_score_nonexisting_channel(md, cd, 1);
	}

}

/* ----------------------------------------------------------------- */
/* TODO: this function is too long. Break it up. */

void process_cmd_score_existing_channel(struct message_data *md, 
										struct command_data *cd, 
										struct Channel* chptr,
										int compact)
{
	struct OCF_topops channelscores;
	struct Client *clptr;
	dlink_node *node_client;
	int i;
	char tmpBuf[IRCLINELEN];
	char tmpBuf2[IRCLINELEN];
	int num_opped = 0,
		num_peons = 0,
		numtopscores, temp_opped, temp_nopped;
	char **oppedusers, **oppedhosts, **noppedusers, **noppedhosts;

	ocf_log(OCF_LOG_SCORE, 
		"SCOR %s requested scores for channel \"%s\"", 
		md->nick, cd->channel);

	numtopscores = settings_get_int("num_top_scores");
	if (numtopscores < 1 || numtopscores > OPCOUNT) {
		numtopscores = OCF_NUM_TOP_SCORES;
	}

	/* Fill the (n)opped user stuff */
	//num_clients = dlink_list_length(&(chptr->members));
	temp_opped  = ocf_get_number_chanops(chptr);
	temp_nopped = dlink_list_length(&(chptr->voiced)) +
				  dlink_list_length(&(chptr->peons));
	oppedusers  = MyMalloc(temp_opped  * sizeof(char*));
	oppedhosts  = MyMalloc(temp_opped  * sizeof(char*));
	noppedusers = MyMalloc(temp_nopped * sizeof(char*));
	noppedhosts = MyMalloc(temp_nopped * sizeof(char*));

	/* Create an array with unique userhosts of the opped
	 * clients in the channel */
	DLINK_FOREACH(node_client, chptr->chanops.head) {
		clptr = (struct Client*)(node_client->data);
		num_opped = add_userhost_to_array(oppedusers, oppedhosts, num_opped, 
			clptr->username, clptr->host);
	}

#ifdef REQUIRE_OANDV
	DLINK_FOREACH(node_client, chptr->chanops_voiced.head) {
		clptr = (struct Client*)(node_client->data);
		num_opped = add_userhost_to_array(oppedusers, oppedhosts, num_opped, 
			clptr->username, clptr->host);
	}
#endif

	/* Create an array with unique userhosts of the non-opped
	 * clients in the channel */
	DLINK_FOREACH(node_client, chptr->voiced.head) {
		clptr = (struct Client*)(node_client->data);
			num_peons = add_userhost_to_array(noppedusers, noppedhosts, num_peons, 
				clptr->username, clptr->host);
	}

	/* Create an array with unique userhosts of the non-opped
	 * clients in the channel */
	DLINK_FOREACH(node_client, chptr->peons.head) {
		clptr = (struct Client*)(node_client->data);
			num_peons = add_userhost_to_array(noppedusers, noppedhosts, num_peons, 
				clptr->username, clptr->host);
	}

	memset(&channelscores, 0, sizeof(channelscores));
	DB_set_read_channel(chptr->chname);
	DB_poll_channel( &channelscores, 
		oppedusers, oppedhosts, num_opped, 
		noppedusers, noppedhosts, num_peons);

	/* (n)opped users no longer needed */
	MyFree(oppedusers);
	MyFree(oppedhosts);
	MyFree(noppedusers);
	MyFree(noppedhosts);

	if (channelscores.topscores[0] == 0) {
		if (compact) {
			chanfix_send_privmsg(md->nick, "~! %s", cd->channel);
		} else {
			chanfix_send_privmsg(md->nick, 
				"There are no scores in the database for \"%s\".",
				cd->channel);
		}
		return;
	}

	/* Top scores */
	if (!compact) {
		chanfix_send_privmsg(md->nick, 
			"Top %d scores for channel \"%s\" in the database:",
			numtopscores, cd->channel);
	}
	tmpBuf[0] = 0;
	i = 0;
	if (compact) {
		snprintf(tmpBuf, sizeof(tmpBuf), "~S %s ", cd->channel);
	}
	while (i < numtopscores && channelscores.topscores[i] > 0) {
		if (compact) {
			snprintf(tmpBuf2, sizeof(tmpBuf2), "%d ", channelscores.topscores[i]);
		} else {
			snprintf(tmpBuf2, sizeof(tmpBuf2), "%d, ", channelscores.topscores[i]);
		}
		strcat(tmpBuf, tmpBuf2);
		i++;
	}
	if (!compact) {
		tmpBuf[strlen(tmpBuf) - 2] = '.';
	}
	chanfix_send_privmsg(md->nick, "%s", tmpBuf);

	/* Top scores of opped clients */
	if (!compact) {
		chanfix_send_privmsg(md->nick, 
			"Top %d scores for ops in channel \"%s\" in the database:",
			numtopscores, chptr->chname);
	}

	tmpBuf[0] = 0;
	i = 0;
	if (compact) {
		snprintf(tmpBuf, sizeof(tmpBuf), "~O %s ", cd->channel);
	}
	while (i < numtopscores && channelscores.currentopscores[i] > 0) {
		if (compact) {
			snprintf(tmpBuf2, sizeof(tmpBuf2), "%d ", channelscores.currentopscores[i]);
		} else {
			snprintf(tmpBuf2, sizeof(tmpBuf2), "%d, ", channelscores.currentopscores[i]);
		}
		strcat(tmpBuf, tmpBuf2);
		i++;
	}
	if (strlen(tmpBuf) > 0) {
		if (!compact) {
			tmpBuf[strlen(tmpBuf) - 2] = '.';
		}
		chanfix_send_privmsg(md->nick, "%s", tmpBuf);
	} else {
		chanfix_send_privmsg(md->nick, "None.");
	}

	/* Top scores of non-opped clients */
	if (!compact) {
		chanfix_send_privmsg(md->nick, 
			"Top %d scores for non-ops in channel \"%s\" in the database:",
			numtopscores, chptr->chname);
	}

	tmpBuf[0] = 0;
	i = 0;
	if (compact) {
		snprintf(tmpBuf, sizeof(tmpBuf), "~N %s ", cd->channel);
	}
	while (i < numtopscores && channelscores.currentnopscores[i] > 0) {
		if (compact) {
			snprintf(tmpBuf2, sizeof(tmpBuf2), "%d ", channelscores.currentnopscores[i]);
		} else {
			snprintf(tmpBuf2, sizeof(tmpBuf2), "%d, ", channelscores.currentnopscores[i]);
		}
		strcat(tmpBuf, tmpBuf2);
		i++;
	}
	if (strlen(tmpBuf) > 0) {
		if (!compact) {
			tmpBuf[strlen(tmpBuf) - 2] = '.';
		}
		chanfix_send_privmsg(md->nick, "%s", tmpBuf);
	} else {
		chanfix_send_privmsg(md->nick, "None.");
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_score_nonexisting_channel(struct message_data *md, 
										   struct command_data *cd,
										   int compact)
{
	struct OCF_topops channelscores;
	char tmpBuf[IRCLINELEN];
	char tmpBuf2[IRCLINELEN];
	int i;
	int numtopscores = settings_get_int("num_top_scores");
	if (numtopscores < 1 || numtopscores > OPCOUNT) {
		numtopscores = OCF_NUM_TOP_SCORES;
	}

	ocf_log(OCF_LOG_SCORE, 
		"SCOR %s requested scores for channel \"%s\"", 
		md->nick, cd->channel);

	memset(&channelscores, 0, sizeof(channelscores));
	DB_set_read_channel(cd->channel);
	DB_poll_channel( &channelscores, (char**)1, (char**)1, 0, (char**)1, (char**)1, 0);
	
	if (channelscores.topscores[0] == 0) {
		if (compact) {
			chanfix_send_privmsg(md->nick, "~! %s", cd->channel);
		} else {
			chanfix_send_privmsg(md->nick, 
				"There are no scores in the database for \"%s\".",
				cd->channel);
		}
		return;
	}

	/* Top scores */
	if (!compact) {
		chanfix_send_privmsg(md->nick, 
			"Top %d scores for channel \"%s\" in the database:",
			numtopscores, cd->channel);
	}
	tmpBuf[0] = 0;
	i = 0;
	if (compact) {
		snprintf(tmpBuf, sizeof(tmpBuf), "~S %s ", cd->channel);
	}
	while (i < numtopscores && channelscores.topscores[i] > 0) {
		if (compact) {
			snprintf(tmpBuf2, sizeof(tmpBuf2), "%d ", channelscores.topscores[i]);
		} else {
			snprintf(tmpBuf2, sizeof(tmpBuf2), "%d, ", channelscores.topscores[i]);
		}
		strcat(tmpBuf, tmpBuf2);
		i++;
	}
	chanfix_send_privmsg(md->nick, "%s", tmpBuf);
	if (compact) {
		chanfix_send_privmsg(md->nick, "~O %s", cd->channel);
		chanfix_send_privmsg(md->nick, "~N %s", cd->channel);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_score_nick(struct message_data *md, struct command_data *cd,
							int compact)
{
	struct Client *clptr;
	struct OCF_chanuser highscores;
	char* users[1];
	char* hosts[1];

	if (!settings_get_bool("enable_cmd_score_nick")) {
		chanfix_send_privmsg(md->nick, "This command has been disabled.");
		return;
	}

	clptr = find_client(cd->nick);

	if (!clptr) {
		if (compact) {
			chanfix_send_privmsg(md->nick, "~U %s no@such.nick 0", cd->channel);
		} else {
			chanfix_send_privmsg(md->nick, "No such nick %s.", cd->nick);
		}
		return;
	}
	
	users[0] = clptr->username;
	hosts[0] = clptr->host;	

	ocf_log(OCF_LOG_SCORE, 
		"SCOR %s requested score for %s (%s@%s) in channel \"%s\"", 
		md->nick, cd->nick, clptr->username, clptr->host, cd->channel);

	highscores.score = 0;
	DB_set_read_channel(cd->channel);
	DB_get_top_user_hosts(&highscores, users, hosts, 1, 1);

	if (compact) {
		chanfix_send_privmsg(md->nick, "~U %s %s@%s %d",
			cd->channel, clptr->username, clptr->host, highscores.score);
	} else {
		chanfix_send_privmsg(md->nick, 
			"Score for \"%s@%s\" in channel \"%s\": %d.",
			clptr->username, clptr->host, cd->channel, highscores.score);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_score_hostmask(struct message_data *md, struct command_data *cd,
								int compact)
{
	struct OCF_chanuser highscores;
	char* users[1];
	char* hosts[1];

	if (!settings_get_bool("enable_cmd_score_userhost")) {
		chanfix_send_privmsg(md->nick, "This command has been disabled.");
		return;
	}

	users[0] = cd->user;
	hosts[0] = cd->host;	

	ocf_log(OCF_LOG_SCORE, 
		"SCOR %s requested score for %s@%s in channel \"%s\"", 
		md->nick, cd->user, cd->host, cd->channel);

	highscores.score = 0;
	DB_set_read_channel(cd->channel);
	DB_get_top_user_hosts(&highscores, users, hosts, 1, 1);

	if (compact) {
		chanfix_send_privmsg(md->nick, "~U %s %s@%s %d",
			cd->channel, cd->user, cd->host, highscores.score);
	} else {
		chanfix_send_privmsg(md->nick, 
			"Score for \"%s@%s\" in channel \"%s\": %d.",
			cd->user, cd->host, cd->channel, highscores.score);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_chanfix(struct message_data *md, struct command_data *cd)
{
	struct Channel* chptr;
	char tmpBuf[IRCLINELEN];
	unsigned int flags;
	struct OCF_chanuser top_score_client;

	/* Check whether manual chanfix has been disabled in the settings. */
	if (!settings_get_bool("enable_chanfix")) {
		chanfix_send_privmsg(md->nick, 
			"Sorry, manual chanfixes are currently disabled.");
		send_notice_to_loggedin_except(cd->loggedin_user_p, 
			OCF_NOTICE_FLAGS_CHANFIX,
			"[currently disabled] %s requested manual fix for %s", 
			get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
		ocf_log(OCF_LOG_CHANFIX, 
			"CHFX [currently disabled] %s requested manual fix for %s", 
			get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
		return;
	}

	/* Don't allow manual fixes if we're split. */
	ocf_check_split(NULL);
	if (currently_split) {
		chanfix_send_privmsg(md->nick, 
			"Sorry, chanfix is currently disabled because not enough servers are linked.");
		send_notice_to_loggedin_except(cd->loggedin_user_p, 
			OCF_NOTICE_FLAGS_CHANFIX,
			"[splitmode enabled] %s requested manual fix for %s", 
			get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
		ocf_log(OCF_LOG_CHANFIX, 
			"CHFX [splitmode enabled] %s requested manual fix for %s", 
			get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
		return;
	}

	/* Only allow chanfixes for channels that are in the database. */
	if (!DB_channel_exists(cd->channel)) {
		chanfix_send_privmsg(md->nick, 
			"There are no scores in the database for \"%s\".", cd->channel);
		return;
	}

	/* Don't fix a channel being chanfixed. */
	if (is_being_chanfixed(cd->channel)) {
		chanfix_send_privmsg(md->nick, 
			"Channel \"%s\" is already being manually fixed.", cd->channel);
		return;
	}

	/* Check if the highest score is high enough for a fix. */
	top_score_client.score = 0;
	DB_set_read_channel(cd->channel);
	DB_get_oplist( &top_score_client, 1 );
	if (top_score_client.score <= 
					(int)(OCF_FIX_MIN_ABS_SCORE_END * OCF_MAX_SCORE)) {
		chanfix_send_privmsg(md->nick, 
			"The highscore in channel \"%s\" is %d which is lower than the minimum score required (%.3f * %d = %d).", 
			cd->channel, top_score_client.score,
			OCF_FIX_MIN_ABS_SCORE_END, OCF_MAX_SCORE,
			(int)(OCF_FIX_MIN_ABS_SCORE_END * OCF_MAX_SCORE));
		return;
	}
	
	/* Don't fix a channel being autofixed without OVERRIDE flag. */
	if (is_being_autofixed(cd->channel)) {
		if (cd->flags == 0) {
			chanfix_send_privmsg(md->nick, 
				"Channel \"%s\" is being automatically fixed. Append the OVERRIDE flag to force a manual fix.", 
				cd->channel);
			return;
		} else {
			/* We're going to manually fix this instead of autofixing it,
			 * so remove this channel from the autofix list. */
			del_autofix_channel(cd->channel);
		}
	}

	/* Don't fix a blocked channel. */
	flags = DB_channel_get_flag();
	if (flags & OCF_CHANNEL_FLAG_BLOCK) {
		chanfix_send_privmsg(md->nick, 
			"Channel \"%s\" is BLOCKED.", cd->channel);
		return;
	}

	/* Don't fix an alerted channel without the OVERRIDE flag. */
	flags = DB_channel_get_flag();
	if (flags & OCF_CHANNEL_FLAG_ALERT && cd->flags == 0) {
		chanfix_send_privmsg(md->nick, 
			"Alert: channel \"%s\" has notes. Use \"INFO %s\" to read them. Append the OVERRIDE flag to force a manual fix.", 
			cd->channel, cd->channel);
		return;
	}

	/* Only allow chanfixes for existing channels. */
	chptr = hash_find_channel(cd->channel);
	if (!chptr) {
		chanfix_send_privmsg(md->nick, "No such channel %s.", cd->channel);
		return;
	}

	/* Fix the channel */
	add_chanfix_channel(cd->channel);

	/* Add note to the channel about this manual fix */
	snprintf(tmpBuf, sizeof(tmpBuf), "CHANFIX by %s", 
		cd->loggedin_user_p->ocf_user_p->name);
	DB_set_write_channel(cd->channel);
	DB_channel_add_note(cd->loggedin_user_p->ocf_user_p->id, 
		time(NULL), tmpBuf);

	/* Log the chanfix */
	chanfix_send_privmsg(md->nick, 
		"Manual chanfix acknowledged for %s", cd->channel);
	send_notice_to_loggedin_except(cd->loggedin_user_p, OCF_NOTICE_FLAGS_CHANFIX,
		"%s requested manual fix for %s", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
	ocf_log(OCF_LOG_CHANFIX, "CHFX %s requested manual fix for %s", 
		cd->loggedin_user_p->nick, cd->channel);
}

/* ----------------------------------------------------------------- */
/* Tries to login the user sending the login command. */

void process_cmd_login(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	struct ocf_loggedin_user *olu;
	char hostmask[USERLEN + HOSTLEN + 1];

	/* Only accept the LOGIN command if it has been sent to user@server,
	 * and not if it's been sent to the chanfix nick. */
	if (!md->safe) {
		chanfix_send_privmsg(md->nick, 
			"For security reasons, you must message your login to %s@%s instead of %s.",
			chanfix->username, me.name, chanfix->name);
		return;
	}

	/* Get the user belonging to this username. */
	ou = ocf_get_user(cd->name);
	if (!ou) {
		/* This user doesn't exist. */
		chanfix_send_privmsg(md->nick, "Incorrect username or password.");
		ocf_log(OCF_LOG_USERS, 
			"LOGI Login attempt for non-existing user %s by %s (%s@%s) [%s].", 
			cd->name, md->nick, md->user, md->host, md->server);
		return;
	} else if (ou->deleted) {
		/* This user has been deleted. */
		chanfix_send_privmsg(md->nick, "Incorrect username or password.");
		ocf_log(OCF_LOG_USERS, 
			"LOGI Login attempt for deleted user %s by %s (%s@%s) [%s].", 
			ou->name, md->nick, md->user, md->host, md->server);
		return;
	}

	/* Check if a user with this nick is already logged in.
	 * If so, log them out first. This prevents users getting the wrong
	 * rights for any reason. */
	olu = ocf_get_loggedin_user(md->nick);
	if (olu) {
		ocf_user_logout(md->nick);
	}

	/* Check hostmask of user. */
	snprintf(hostmask, sizeof(hostmask), "%s@%s", md->user, md->host);
	if (!ocf_user_matches_hostmask(ou, hostmask)) {
		chanfix_send_privmsg(md->nick, "Incorrect username or password.");
		send_notice_to_loggedin(OCF_NOTICE_FLAGS_LOGINOUT,
			"Login failed (wrong hostmask) as %s by %s (%s@%s) [%s].", 
			ou->name, md->nick, md->user, md->host, md->server);
		ocf_log(OCF_LOG_USERS, "LOGI Login failed (wrong hostmask) as %s by %s (%s@%s) [%s].", 
			ou->name, md->nick, md->user, md->host, md->server);
		return;
	}

	/* Check server of user. */
	if (!ocf_user_has_server(ou, md->server)) {
		chanfix_send_privmsg(md->nick, "Incorrect username or password.");
		send_notice_to_loggedin(OCF_NOTICE_FLAGS_LOGINOUT,
			"Login failed (wrong server) as %s by %s (%s@%s) [%s].", 
			ou->name, md->nick, md->user, md->host, md->server);
		ocf_log(OCF_LOG_USERS, "LOGI Login failed (wrong server) as %s by %s (%s@%s) [%s].", 
			ou->name, md->nick, md->user, md->host, md->server);
		return;
	}

	/* Hostmask ok, now the password check. */
	if (ocf_user_check_password(ou, cd->password)) {
		ocf_log(OCF_LOG_DEBUG, "Correct password for %s", cd->name);
		olu = ocf_user_login(md->nick, md->user, md->host, md->server, ou);
		chanfix_send_privmsg(md->nick,
			"Authentication successful. Welcome, %s.", md->nick);
		if (olu->ocf_user_p->flags) {
			chanfix_send_privmsg(md->nick, "Your flags are %s.", 
				ocf_user_flags_to_string(olu->ocf_user_p->flags));
		}
		if (olu->ocf_user_p->noticeflags) {
			chanfix_send_privmsg(md->nick, "Your umodes are %s.", 
				ocf_user_notice_flags_to_string(olu->ocf_user_p->noticeflags));
		}
		send_notice_to_loggedin_except(olu, OCF_NOTICE_FLAGS_LOGINOUT,
			"Login as %s by %s (%s@%s) [%s].", olu->ocf_user_p->name,
			md->nick, md->user, md->host, md->server);
		ocf_log(OCF_LOG_USERS, "LOGI %s has logged in as %s from %s@%s [%s].",
			md->nick, ou->name, md->user, md->host, md->server);
	} else {
		/* Password was wrong. Log failure. */
		chanfix_send_privmsg(md->nick, "Incorrect username or password.");
		send_notice_to_loggedin(OCF_NOTICE_FLAGS_LOGINOUT,
			"Login failed (wrong password) as %s by %s (%s@%s) [%s].", 
			ou->name, md->nick, md->user, md->host, md->server);
		ocf_log(OCF_LOG_USERS, "LOGI Login failed (wrong password) as %s by %s (%s@%s) [%s].", 
			ou->name, md->nick, md->user, md->host, md->server);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_logout(struct message_data *md, struct command_data *cd)
{
	ocf_user_logout(md->nick);
	chanfix_send_privmsg(md->nick, "You have logged out.");
	send_notice_to_loggedin(OCF_NOTICE_FLAGS_LOGINOUT,
		"%s has logged out.", 
		get_logged_in_user_string(cd->loggedin_user_p));
	ocf_log(OCF_LOG_USERS, "LOGO %s has logged out.", md->nick);
}

/* ----------------------------------------------------------------- */

void process_cmd_passwd(struct message_data *md, struct command_data *cd)
{
	/* Test if old password is correct. */
	if (!ocf_user_check_password(cd->loggedin_user_p->ocf_user_p, cd->params)) {
		chanfix_send_privmsg(md->nick, "Old password does not match.");
		return;
	}
	
	ocf_user_set_password(cd->loggedin_user_p->ocf_user_p, cd->password);
	ocf_users_updated();
	chanfix_send_privmsg(md->nick, "Your password has been changed.");
}

/* ----------------------------------------------------------------- */

void process_cmd_history(struct message_data *md, struct command_data *cd)
{
	int i = 0;
	struct OCF_topops result;

	if (!settings_get_bool("enable_cmd_history")) {
		chanfix_send_privmsg(md->nick, "This command has been disabled.");
		return;
	}

	DB_set_read_channel(cd->channel);
	DB_poll_channel(&result, (char**)1, (char**)1, 0, (char**)1, (char**)1, 0);

	if (result.manualfixhistory[0] == 0) {
		chanfix_send_privmsg(md->nick, 
			"Channel \"%s\" has never been manually fixed.", cd->channel);
		return;
	}

	chanfix_send_privmsg(md->nick, 
			"Channel \"%s\" has been manually fixed on:", cd->channel);
	while (result.manualfixhistory[i] > 0) {
		chanfix_send_privmsg(md->nick, "%s", 
			seconds_to_datestring((time_t)result.manualfixhistory[i]));
		i++;
	}
	chanfix_send_privmsg(md->nick, "End of list.");
}

/* ----------------------------------------------------------------- */

void process_cmd_info(struct message_data *md, struct command_data *cd)
{
	int num_notes, i;
	unsigned int flags;
	struct OCF_channote* note_p;

	if (!DB_channel_exists(cd->channel)) {
		chanfix_send_privmsg(md->nick, 
			"No information on %s in the database.", cd->channel);
		return;
	}

	chanfix_send_privmsg(md->nick, "Information on \"%s\":", cd->channel);

	DB_set_read_channel(cd->channel);
	flags = DB_channel_get_flag();
	if (flags & OCF_CHANNEL_FLAG_BLOCK) {
		chanfix_send_privmsg(md->nick, "%s is BLOCKED.", cd->channel);
	} else if (flags & OCF_CHANNEL_FLAG_ALERT) {
		chanfix_send_privmsg(md->nick, "%s is ALERTED.", cd->channel);
	}

	if (is_being_chanfixed(cd->channel)) {
		chanfix_send_privmsg(md->nick, "%s is being chanfixed.", cd->channel);
	}
	if (is_being_autofixed(cd->channel)) {
		chanfix_send_privmsg(md->nick, "%s is being autofixed.", cd->channel);
	}

	num_notes = DB_channel_note_count();
	if (num_notes > 0) {
		chanfix_send_privmsg(md->nick, "Notes (%d):", num_notes);
		for (i = 1; i <= num_notes; i++) {
			note_p = DB_channel_get_note(i);
			if (note_p) {
				if (ocf_user_get_name_from_id(note_p->id)) {
					chanfix_send_privmsg(md->nick, "[%d:%s] %s %s", 
						i, ocf_user_get_name_from_id(note_p->id), 
						seconds_to_datestring((time_t)note_p->timestamp), 
						note_p->note);
				} else {
					chanfix_send_privmsg(md->nick, "[%d:UNKNOWN] %s %s", 
						i, seconds_to_datestring((time_t)note_p->timestamp), 
						note_p->note);
				}
			}
		}
	}
	chanfix_send_privmsg(md->nick, "End of information.");

	ocf_log(OCF_LOG_CHANFIX,
		"INFO %s requested info` for \"%s\"",
		md->nick, cd->channel);
}

/* ----------------------------------------------------------------- */

void process_cmd_oplist(struct message_data *md, struct command_data *cd)
{
	struct OCF_chanuser oplist[10];
	int scorecount[1];
	int r, i;

	if (!settings_get_bool("enable_cmd_oplist")) {
		chanfix_send_privmsg(md->nick, "This command has been disabled.");
		return;
	}

	DB_set_read_channel(cd->channel);

	if ( !DB_get_op_scores( scorecount, 1 ) )
	{
		chanfix_send_privmsg(md->nick,
			"There are no scores in the database for \"%s\".",
			cd->channel);
		return;
    }

	ocf_log(OCF_LOG_CHANFIX,
		"OPLI %s requested oplist for \"%s\"",
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);

	send_notice_to_loggedin(OCF_NOTICE_FLAGS_MAIN, 
		"%s has requested OPLIST for \"%s\"", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);

	r = DB_get_oplist( oplist, 10 );

	if ( r == 10 )
	{
		chanfix_send_privmsg(md->nick,
			"Top 10 unique op hostmasks in channel \"%s\":",
			cd->channel);
	}
	else
	{
		chanfix_send_privmsg(md->nick,
			"Found %d unique op hostmasks in channel \"%s\":",
			r, cd->channel);
	}

	for( i = 0; i < r; i++ )
	{
		chanfix_send_privmsg(md->nick,
			"%2d. %4d %s@%s",
			( i + 1 ), oplist[i].score, oplist[i].user, oplist[i].host);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_addnote(struct message_data *md, struct command_data *cd)
{
	int num_notes;

	DB_set_write_channel(cd->channel);
	num_notes = DB_channel_add_note(cd->loggedin_user_p->ocf_user_p->id, time(NULL), cd->params);

	chanfix_send_privmsg(md->nick, "Added note to %s (%d notes).",
		cd->channel, num_notes);
	send_notice_to_loggedin_except(cd->loggedin_user_p, OCF_NOTICE_FLAGS_NOTES,
		"%s added note to %s (%d notes).", get_logged_in_user_string(cd->loggedin_user_p),
		cd->channel, num_notes);
	ocf_log(OCF_LOG_NOTES, "NOTE %s added note to %s (%d notes).", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel, num_notes);
}

/* ----------------------------------------------------------------- */

void process_cmd_delnote(struct message_data *md, struct command_data *cd)
{
	struct OCF_channote* note_p;
	int res;

	if (!DB_channel_exists(cd->channel)) {
		chanfix_send_privmsg(md->nick, 
			"No information on %s in the databas.", cd->channel);
		return;
	}
	
	DB_set_read_channel(cd->channel);
	note_p = DB_channel_get_note(cd->id);

	if (note_p == NULL) {
		chanfix_send_privmsg(md->nick, 
			"No such note %d for channel %s", cd->id, cd->channel);
		return;
	}

	DB_set_write_channel(cd->channel);
	res = DB_channel_del_note(cd->id);

	if (res) {
		chanfix_send_privmsg(md->nick, 
			"Note %d for channel %s deleted.", cd->id, cd->channel);
		send_notice_to_loggedin_except(cd->loggedin_user_p, OCF_NOTICE_FLAGS_NOTES,
			"%s deleted note %d for channel %s.", 
			get_logged_in_user_string(cd->loggedin_user_p), cd->id, cd->channel);
		ocf_log(OCF_LOG_NOTES, "NOTE %s deleted note %d for channel %s.", 
			get_logged_in_user_string(cd->loggedin_user_p), cd->id, cd->channel);
	} else {
		chanfix_send_privmsg(md->nick, 
			"Error when deleting note %d for channel %s", cd->id, cd->channel);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_whois(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;

	ou = ocf_get_user(cd->name);
	if (ou) {
		chanfix_send_privmsg(md->nick, "User: %s", ou->name);
		if (ou->flags == 0) {
			chanfix_send_privmsg(md->nick, "Flags: none.", ou->name);
		} else {
			chanfix_send_privmsg(md->nick, "Flags: %s", 
				ocf_user_flags_to_string(ou->flags));
		}
		chanfix_send_privmsg(md->nick, "Hosts: %s", ou->hostmasks);
		chanfix_send_privmsg(md->nick, "Main server: %s", ou->main_server);
		if (strlen(ou->servers) > 0) {
			chanfix_send_privmsg(md->nick, "Other servers: %s", ou->servers);
		}
	}else {
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
	}
}

/* ----------------------------------------------------------------- */
/* Add a flag to a user.
 * cd->name = user to be modified.
 * cd->flags = single flag to be added.
 */
void process_cmd_addflag(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	int num_f_flags, num_b_flags, max_num_f_flags, max_num_b_flags;

	ou = ocf_get_user(cd->name);

	if (ou == NULL){
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	if (cd->flags == OCF_USER_FLAGS_OWNER) {
		chanfix_send_privmsg(md->nick, "You cannot add an owner flag.");
		return;
	}

	if (cd->flags == OCF_USER_FLAGS_USERMANAGER &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_OWNER)) {
		chanfix_send_privmsg(md->nick, "Only an owner can add the user management flag.");
		return;
	}

	/* A serveradmin can only add flags to users on his own server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		if (!ocf_user_has_main_server(ou, cd->loggedin_user_p->ocf_user_p->main_server)) {
			chanfix_send_privmsg(md->nick, 
				"You cannot add a flag to a user with a different main server.");
			return;
		}
		if (cd->flags == OCF_USER_FLAGS_BLOCK) {
			chanfix_send_privmsg(md->nick, "You cannot add a block flag.");
			return;
		}
		if (cd->flags == OCF_USER_FLAGS_SERVERADMIN) {
			chanfix_send_privmsg(md->nick, "You cannot add a serveradmin flag.");
			return;
		}
	}

	if (ocf_user_has_flag(ou, cd->flags)) {
		chanfix_send_privmsg(md->nick, "User %s already has flag %c.",
			ou->name, ocf_user_flag_get_char(cd->flags));
		return;
	}

	/* Check if the maximum number of flags for this server has already.
	 * been reached, if applicable. */
	max_num_f_flags = settings_get_int("f_flags_per_server");
	max_num_b_flags = settings_get_int("b_flags_per_server");

	if (cd->flags == OCF_USER_FLAGS_BLOCK && max_num_b_flags > 0) {
		num_b_flags = get_num_flags_of_server('b', ou->main_server);
		if (num_b_flags >= max_num_b_flags) {
			chanfix_send_privmsg(md->nick, "Sorry, the server %s already has %d b flags.",
				ou->main_server, max_num_b_flags);
			return;
		}
	}

	if (cd->flags == OCF_USER_FLAGS_FIX && max_num_f_flags > 0) {
		num_f_flags = get_num_flags_of_server('f', ou->main_server);
		if (num_f_flags >= max_num_f_flags) {
			chanfix_send_privmsg(md->nick, "Sorry, the server %s already has %d f flags.",
				ou->main_server, max_num_f_flags);
			return;
		}
	}

	ocf_user_add_flags(ou, cd->flags);
	ocf_users_updated();
	chanfix_send_privmsg(md->nick, "Added flag %c to user %s.",
		ocf_user_flag_get_char(cd->flags), ou->name);
	send_notice_to_loggedin_except(cd->loggedin_user_p,
		OCF_NOTICE_FLAGS_USER,
		"%s added flag %c to user %s.", get_logged_in_user_string(cd->loggedin_user_p), 
		ocf_user_flag_get_char(cd->flags), ou->name);
	send_notice_to_loggedin_username(ou->name, "%s gave you the %c flag.",
		get_logged_in_user_string(cd->loggedin_user_p), ocf_user_flag_get_char(cd->flags));
	ocf_log(OCF_LOG_USERS, "USFL %s added flag %c to user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		ocf_user_flag_get_char(cd->flags), ou->name);
}

/* ----------------------------------------------------------------- */

void process_cmd_delflag(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	ou = ocf_get_user(cd->name);

	if (ou == NULL){
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	if (cd->flags == OCF_USER_FLAGS_OWNER) {
		chanfix_send_privmsg(md->nick, "You cannot delete an owner flag.");
		return;
	}

	if (cd->flags == OCF_USER_FLAGS_USERMANAGER &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_OWNER)) {
		chanfix_send_privmsg(md->nick, "Only an owner can delete the user management flag.");
		return;
	}

	/* A serveradmin can only delete flags from users on his own server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		if (!ocf_user_has_main_server(ou, cd->loggedin_user_p->ocf_user_p->main_server)) {
			chanfix_send_privmsg(md->nick, 
				"You cannot delete a flag from a user with a different main server.");
			return;
		}
		if (cd->flags == OCF_USER_FLAGS_BLOCK) {
			chanfix_send_privmsg(md->nick, "You cannot remove a block flag.");
			return;
		}
		if (cd->flags == OCF_USER_FLAGS_SERVERADMIN) {
			chanfix_send_privmsg(md->nick, "You cannot remove a serveradmin flag.");
			return;
		}
	}

	if (!ocf_user_has_flag(ou, cd->flags)) {
		chanfix_send_privmsg(md->nick, "User %s does not have flag %c.",
			ou->name, ocf_user_flag_get_char(cd->flags));
		return;
	}

	ocf_user_del_flags(ou, cd->flags);
	ocf_users_updated();
	chanfix_send_privmsg(md->nick, "Deleted flag %c of user %s.",
		ocf_user_flag_get_char(cd->flags), ou->name);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER,
		"%s deleted flag %c from user %s.", get_logged_in_user_string(cd->loggedin_user_p), 
		ocf_user_flag_get_char(cd->flags), ou->name);
	send_notice_to_loggedin_username(ou->name, "%s deleted your %c flag.",
		get_logged_in_user_string(cd->loggedin_user_p), ocf_user_flag_get_char(cd->flags));
	ocf_log(OCF_LOG_USERS, "USFL %s deleted flag %c from user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p),
		ocf_user_flag_get_char(cd->flags), ou->name);
}

/* ----------------------------------------------------------------- */

void process_cmd_addhost(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	int result;
	ou = ocf_get_user(cd->name);

	if (ou == NULL){
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	/* A serveradmin can only add hosts to users on his own server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		if (!ocf_user_has_main_server(ou, cd->loggedin_user_p->ocf_user_p->main_server)) {
			chanfix_send_privmsg(md->nick, 
				"You cannot add a host to a user with a different main server.");
			return;
		}
	}

	if (ocf_user_has_hostmask(ou, cd->hostmask)) {
		chanfix_send_privmsg(md->nick, "User %s already has hostmask %s.", 
			cd->name, cd->hostmask);
		return;
	}

	result = ocf_user_add_hostmask(ou, cd->hostmask);
	if (!result) {
		chanfix_send_privmsg(md->nick, "Failed adding hostmask %s to user %s.",
			cd->hostmask, ou->name);
		return;
	}

	ocf_users_updated();
	chanfix_send_privmsg(md->nick, "Added hostmask %s to user %s.",
		cd->hostmask, ou->name);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER,
		"%s added hostmask %s to user %s.", get_logged_in_user_string(cd->loggedin_user_p), 
		cd->hostmask, ou->name);
	send_notice_to_loggedin_username(ou->name, "%s added you the hostmask %s",
		get_logged_in_user_string(cd->loggedin_user_p), cd->hostmask);
	ocf_log(OCF_LOG_USERS, "USHM %s added hostmask %s to user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		cd->hostmask, ou->name);
}

/* ----------------------------------------------------------------- */

void process_cmd_delhost(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	ou = ocf_get_user(cd->name);

	if (ou == NULL){
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	/* A serveradmin can only delete hosts from users on his own server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		if (!ocf_user_has_main_server(ou, cd->loggedin_user_p->ocf_user_p->main_server)) {
			chanfix_send_privmsg(md->nick, 
				"You cannot delete a host from a user with a different main server.");
			return;
		}
	}

	if (!ocf_user_has_hostmask(ou, cd->hostmask)) {
		chanfix_send_privmsg(md->nick, "User %s doesn't have hostmask %s.", 
			cd->name, cd->hostmask);
		return;
	}

	ocf_user_del_hostmask(ou, cd->hostmask);
	ocf_users_updated();
	chanfix_send_privmsg(md->nick, "Deleted hostmask %s from user %s.",
		cd->hostmask, ou->name);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER, "%s deleted hostmask %s from user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		cd->hostmask, ou->name);
	send_notice_to_loggedin_username(ou->name, "%s deleted your hostmask %s",
		get_logged_in_user_string(cd->loggedin_user_p), cd->hostmask);
	ocf_log(OCF_LOG_USERS, "USHM %s deleted hostmask %s from user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		cd->hostmask, ou->name);
}

/* ----------------------------------------------------------------- */
/* Add a user.
 * cd->name = user to be modified.
 * cd->hostmask (optional) = user@host of user.
 * TODO: if owner && user == deleted, set deleted = 0.
 */
void process_cmd_adduser(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;

	ou = ocf_get_user(cd->name);

	/* User already exists. */
	if (ou) {
		chanfix_send_privmsg(md->nick, "User %s already exists.", ou->name);
		return;
	}

	ou = ocf_add_user(cd->name, cd->hostmask);
	if (ou) {
		strncpy(ou->password, "NOTSETYET", sizeof(ou->password));
		ocf_users_updated();
		if (cd->hostmask) {
			chanfix_send_privmsg(md->nick, "Created user %s (%s).", 
				cd->name, cd->hostmask);
			send_notice_to_loggedin_except(cd->loggedin_user_p, 
				OCF_NOTICE_FLAGS_USER, "%s added user %s (%s).", 
				get_logged_in_user_string(cd->loggedin_user_p), ou->name,
				cd->hostmask);
			ocf_log(OCF_LOG_USERS, "USER %s added user %s (%s).", 
				get_logged_in_user_string(cd->loggedin_user_p), 
				ou->name, cd->hostmask);
		} else {
			chanfix_send_privmsg(md->nick, "Created user %s.", cd->name);
			send_notice_to_loggedin(OCF_NOTICE_FLAGS_USER,
				"%s added user %s.", 
				get_logged_in_user_string(cd->loggedin_user_p), ou->name);
			ocf_log(OCF_LOG_USERS, "USER %s added user %s.", 
				get_logged_in_user_string(cd->loggedin_user_p), ou->name);
		}
	} else {
		chanfix_send_privmsg(md->nick, "Error creating user %s.", cd->name);
	}

	/* A user added by a serveradmin automatically has the same main server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		ocf_user_set_main_server(ou, cd->loggedin_user_p->ocf_user_p->main_server);
		chanfix_send_privmsg(md->nick, "Set main server to %s.", ou->main_server);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_deluser(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	ou = ocf_get_user(cd->name);

	/* Test if the user does actually exist. */
	if (!ou) {
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	/* Can't delete an owner. */
	if (ocf_user_has_flag(ou, OCF_USER_FLAGS_OWNER)) {
		chanfix_send_privmsg(md->nick, "You cannot delete an owner.");
		return;
	}

	/* Can only delete a user manager if you're an owner. */
	if (ocf_user_has_flag(ou, OCF_USER_FLAGS_USERMANAGER) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_OWNER)) {
		chanfix_send_privmsg(md->nick, "You cannot delete a user manager.");
		return;
	}

	/* A serveradmin can only delete hosts from users on his own server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		if (!ocf_user_has_main_server(ou, cd->loggedin_user_p->ocf_user_p->main_server)) {
			chanfix_send_privmsg(md->nick, 
				"You cannot delete user with a different main server.");
			return;
		}
	}

	ou->deleted = 1;
	chanfix_send_privmsg(md->nick, "Deleted user %s.", ou->name);
	ocf_users_updated();
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER, "%s deleted user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), ou->name);
	ocf_log(OCF_LOG_USERS, "USER %s deleted user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), ou->name);

	/* We'll see if the user is logged in. If so, he'll be logged out. */
	ocf_check_loggedin_users();
}

/* ----------------------------------------------------------------- */

void process_cmd_chpass(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	ou = ocf_get_user(cd->name);

	/* Test if the user does actually exist. */
	if (!ou) {
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	/* Can't change an owner's password unless you're an owner. */
	if (ocf_user_has_flag(ou, OCF_USER_FLAGS_OWNER) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_OWNER)) {
		chanfix_send_privmsg(md->nick, "You cannot change an owner's password.");
		return;
	}

	/* A serveradmin can only change the password of a user on his own server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		if (!ocf_user_has_main_server(ou, cd->loggedin_user_p->ocf_user_p->main_server)) {
			chanfix_send_privmsg(md->nick, 
				"You cannot change the password of a user with a different main server.");
			return;
		}
	}

	ocf_user_set_password(ou, cd->password);;
	chanfix_send_privmsg(md->nick, "Changed %s's password.", ou->name);
	ocf_users_updated();
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER, "%s changed user %s's password.", 
		get_logged_in_user_string(cd->loggedin_user_p), ou->name);
	ocf_log(OCF_LOG_USERS, "USER %s changed user %s's password.", 
		get_logged_in_user_string(cd->loggedin_user_p), ou->name);
}

/* ----------------------------------------------------------------- */

void process_cmd_chpasshash(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	ou = ocf_get_user(cd->name);

	/* Test if the user does actually exist. */
	if (!ou) {
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	/* Can't change an owner's password unless you're an owner. */
	if (ocf_user_has_flag(ou, OCF_USER_FLAGS_OWNER) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_OWNER)) {
		chanfix_send_privmsg(md->nick, "You cannot change an owner's password.");
		return;
	}

	/* A serveradmin can only change the password of a user on his own server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		if (!ocf_user_has_main_server(ou, cd->loggedin_user_p->ocf_user_p->main_server)) {
			chanfix_send_privmsg(md->nick, 
				"You cannot change the password of a user with a different main server.");
			return;
		}
	}

	ocf_user_set_passwordhash(ou, cd->password);;
	chanfix_send_privmsg(md->nick, "Changed %s's password hash.", ou->name);
	ocf_users_updated();
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER, "%s changed user %s's password.", 
		get_logged_in_user_string(cd->loggedin_user_p), ou->name);
	ocf_log(OCF_LOG_USERS, "USER %s changed user %s's password.", 
		get_logged_in_user_string(cd->loggedin_user_p), ou->name);
}

/* ----------------------------------------------------------------- */

void process_cmd_who(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;

	dlink_node *ptr;
	struct ocf_loggedin_user *olu;
	char names[IRCLINELEN];
	ou = ocf_get_user(cd->name);
	names[0] = 0;

	chanfix_send_privmsg(md->nick, "Logged in users [nick (username:flags)]:");

	DLINK_FOREACH(ptr, ocf_loggedin_user_list.head) {
		olu = ((struct ocf_loggedin_user*)(ptr->data));
		strcat(names, olu->nick);
		strcat(names, " (");
		strcat(names, olu->ocf_user_p->name);
		if (olu->ocf_user_p->flags) {
			strcat(names, ":");
			strcat(names, ocf_user_flags_to_string(olu->ocf_user_p->flags));
		}
		strcat(names, ") ");
		
		/* If the string is getting too long, send it now and empty it. */
		if (strlen(names) > IRCLINELEN - 50) {
			chanfix_send_privmsg(md->nick, "%s", names);
			names[0] = 0;
		}
	}

	chanfix_send_privmsg(md->nick, "%s", names);
}

/* ----------------------------------------------------------------- */
/* UMODE [<+|->flags]
 * If the argument is + or - some flags, the flags are stored in
 * cd->flags. If +, then cd->params[0] = 1; If -, then cd->params[0] = 0. 
 * Without flags, cd->flags is 0. 
 */

void process_cmd_umode(struct message_data *md, struct command_data *cd)
{
	/* Update the umodes */
	if (cd->params[0] == '+') { 
		ocf_user_add_notice_flags(cd->loggedin_user_p->ocf_user_p, cd->flags);
		ocf_users_updated();
	}

	if (cd->params[0] == '-') { 
		ocf_user_del_notice_flags(cd->loggedin_user_p->ocf_user_p, cd->flags);
		ocf_users_updated();
	}

	/* Show the umodes */
	chanfix_send_privmsg(md->nick, "Your umodes are: %s.",
		ocf_user_notice_flags_to_string(cd->loggedin_user_p->ocf_user_p->noticeflags));
}

/* ----------------------------------------------------------------- */

void process_cmd_status(struct message_data *md, struct command_data *cd)
{
	char datetimestr[OCF_TIMESTAMP_LEN];
	struct tm *mlt;
	int num_servers_linked;

	ocf_check_split(NULL);
	mlt = localtime(&module_loaded_time);
	memset(datetimestr, 0, OCF_TIMESTAMP_LEN);
	strftime(datetimestr, OCF_TIMESTAMP_LEN, "%Y-%m-%d %H:%M:%S", mlt);

	chanfix_send_privmsg(md->nick, "This is OpenChanfix version %s (the %s release).",
		OCF_VERSION, OCF_VERSION_NAME);
	chanfix_send_privmsg(md->nick, "The module was loaded on %s. Status:", 
		datetimestr);

	chanfix_send_privmsg(md->nick, "Enable autofix is %d.", 
		settings_get_bool("enable_autofix"));
	chanfix_send_privmsg(md->nick, "Enable chanfix is %d.", 
		settings_get_bool("enable_chanfix"));
	chanfix_send_privmsg(md->nick, "Enable channel blocking is %d.", 
		settings_get_bool("enable_channel_blocking"));

	if (settings_get_int("f_flags_per_server") > 0) {
		chanfix_send_privmsg(md->nick, "Maximum number of f flags per server: %d.", 
			settings_get_int("f_flags_per_server"));
	}

	if (settings_get_int("b_flags_per_server") > 0) {
		chanfix_send_privmsg(md->nick, "Maximum number of b flags per server: %d.", 
			settings_get_int("b_flags_per_server"));
	}

	chanfix_send_privmsg(md->nick, 
		"Required amount of servers linked is %d percent of %d, which is a minimum of %d servers.", 
		settings_get_int("min_servers_present"),
		settings_get_int("num_servers"),
		(settings_get_int("min_servers_present") * 
			settings_get_int("num_servers")) / 100 + 1);

	num_servers_linked = dlink_list_length(&global_serv_list);
	if (currently_split) {
		if (num_servers_linked == 1) {
			chanfix_send_privmsg(md->nick, 
				"Splitmode enabled: no servers linked.");
		} else {
			chanfix_send_privmsg(md->nick, 
				"Splitmode enabled: only %d servers linked.",
				num_servers_linked);
		}
	} else {
		chanfix_send_privmsg(md->nick, 
			"Splitmode disabled. There are %d servers linked.", num_servers_linked);
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_set(struct message_data *md, struct command_data *cd)
{
	int val;
	if (!strcasecmp(cd->name, "num_servers")) {
		val = atoi(cd->params);
		if (val > 0) {
			settings_set_int("num_servers", val);
			chanfix_send_privmsg(md->nick, "NUM_SERVERS is now %d.",
				val);
			send_notice_to_loggedin_except(cd->loggedin_user_p, 
				OCF_NOTICE_FLAGS_MAIN,
				"%s changed NUM_SERVERS to %d.", 
				get_logged_in_user_string(cd->loggedin_user_p), val);
			ocf_log(OCF_LOG_MAIN, " SET %s changed NUM_SERVERS to %d.", 
				get_logged_in_user_string(cd->loggedin_user_p), val);
		} else {
			chanfix_send_privmsg(md->nick, 
				"Please use SET NUM_SERVERS <integer number>.");
		}
	}

	else if (!strcasecmp(cd->name, "enable_autofix")) {
		if (!strcasecmp(cd->params, "1") ||
				!strcasecmp(cd->params, "on")) {
			settings_set_bool("enable_autofix", 1);
			chanfix_send_privmsg(md->nick, "Enabled autofix.");
			send_notice_to_loggedin_except(cd->loggedin_user_p, 
				OCF_NOTICE_FLAGS_MAIN, "%s enabled autofix.", 
				get_logged_in_user_string(cd->loggedin_user_p));
			ocf_log(OCF_LOG_MAIN, " SET %s enabled autofix.", 
				get_logged_in_user_string(cd->loggedin_user_p));
		} else if (!strcasecmp(cd->params, "0") ||
				!strcasecmp(cd->params, "off")) {
			settings_set_bool("enable_autofix", 0);
			chanfix_send_privmsg(md->nick, "Disabled autofix.");
			send_notice_to_loggedin_except(cd->loggedin_user_p, 
				OCF_NOTICE_FLAGS_MAIN, "%s disabled autofix.", 
				get_logged_in_user_string(cd->loggedin_user_p));
			ocf_log(OCF_LOG_MAIN, " SET %s disabled autofix.", 
				get_logged_in_user_string(cd->loggedin_user_p));
		} else {
			chanfix_send_privmsg(md->nick, 
				"Please use SET ENABLE_AUTOFIX <on|off>.");
		}

	}

	else if (!strcasecmp(cd->name, "enable_chanfix")) {
		if (!strcasecmp(cd->params, "1") ||
				!strcasecmp(cd->params, "on")) {
			settings_set_bool("enable_chanfix", 1);
			chanfix_send_privmsg(md->nick, "Enabled manual chanfix.");
			send_notice_to_loggedin_except(cd->loggedin_user_p, 
				OCF_NOTICE_FLAGS_MAIN, "%s enabled manual chanfix.", 
				get_logged_in_user_string(cd->loggedin_user_p));
			ocf_log(OCF_LOG_MAIN, " SET %s enabled manual chanfix.", 
				get_logged_in_user_string(cd->loggedin_user_p));
		} else if (!strcasecmp(cd->params, "0") ||
				!strcasecmp(cd->params, "off")) {
			settings_set_bool("enable_chanfix", 0);
			chanfix_send_privmsg(md->nick, "Disabled manual chanfix.");
			send_notice_to_loggedin_except(cd->loggedin_user_p, 
				OCF_NOTICE_FLAGS_MAIN, "%s disabled manual chanfix.", 
				get_logged_in_user_string(cd->loggedin_user_p));
			ocf_log(OCF_LOG_MAIN, " SET %s disabled manual chanfix.", 
				get_logged_in_user_string(cd->loggedin_user_p));
		} else {
			chanfix_send_privmsg(md->nick, 
				"Please use SET ENABLE_CHANFIX <on|off>.");
		}

	} 
	
	else if (!strcasecmp(cd->name, "enable_channel_blocking")) {
		if (!strcasecmp(cd->params, "1") ||
				!strcasecmp(cd->params, "on")) {
			settings_set_bool("enable_channel_blocking", 1);
			chanfix_send_privmsg(md->nick, "Enabled channel blocking.");
			send_notice_to_loggedin_except(cd->loggedin_user_p, 
				OCF_NOTICE_FLAGS_MAIN, "%s enabled channel blocking.", 
				get_logged_in_user_string(cd->loggedin_user_p));
			ocf_log(OCF_LOG_MAIN, " SET %s enabled channel blocking.", 
				get_logged_in_user_string(cd->loggedin_user_p));
		} else if (!strcasecmp(cd->params, "0") ||
				!strcasecmp(cd->params, "off")) {
			settings_set_bool("enable_channel_blocking", 0);
			chanfix_send_privmsg(md->nick, "Disabled channel blocking.");
			send_notice_to_loggedin_except(cd->loggedin_user_p, 
				OCF_NOTICE_FLAGS_MAIN, "%s disabled channel blocking.", 
				get_logged_in_user_string(cd->loggedin_user_p));
			ocf_log(OCF_LOG_MAIN, " SET %s disabled channel blocking.", 
				get_logged_in_user_string(cd->loggedin_user_p));
		} else {
			chanfix_send_privmsg(md->nick, 
				"Please use SET ENABLE_CHANFIX <on|off>.");
		}

	} 
	
	else {
		chanfix_send_privmsg(md->nick, "This setting does not exist.");
	}
}

/* ----------------------------------------------------------------- */

void process_cmd_block(struct message_data *md, struct command_data *cd)
{
	unsigned int flags;
	char blocknote[IRCLINELEN];

	if (settings_get_bool("enable_channel_blocking") == 0) {
		chanfix_send_privmsg(md->nick, "Channel blocking is disabled.");
		return;
	}

	DB_set_read_channel(cd->channel);
	flags = DB_channel_get_flag();
	if (flags & OCF_CHANNEL_FLAG_BLOCK) {
		chanfix_send_privmsg(md->nick, "Channel %s is already blocked.",
			cd->channel);
		return;
	}
	
	snprintf(blocknote, sizeof(blocknote), "BLOCK by %s: %s", 
		cd->loggedin_user_p->ocf_user_p->name, cd->params);
	DB_set_write_channel(cd->channel);
	DB_channel_set_flag(OCF_CHANNEL_FLAG_BLOCK);
	DB_channel_add_note(cd->loggedin_user_p->ocf_user_p->id, time(NULL),
		blocknote);

	chanfix_send_privmsg(md->nick, "Channel %s has been blocked.",
		cd->channel);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_BLOCK, " %s has blocked channel %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
	ocf_log(OCF_LOG_CHANFIX, "BLCK %s has blocked channel %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
}

/* ----------------------------------------------------------------- */

void process_cmd_unblock(struct message_data *md, struct command_data *cd)
{
	unsigned int flags;
	char unblocknote[IRCLINELEN];

	if (settings_get_bool("enable_channel_blocking") == 0) {
		chanfix_send_privmsg(md->nick, "Channel blocking is disabled.");
		return;
	}

	DB_set_read_channel(cd->channel);
	flags = DB_channel_get_flag();
	if (!(flags & OCF_CHANNEL_FLAG_BLOCK)) {
		chanfix_send_privmsg(md->nick, "Channel %s is not blocked.",
			cd->channel);
		return;
	}
	
	snprintf(unblocknote, sizeof(unblocknote), "UNBLOCK by %s", cd->loggedin_user_p->ocf_user_p->name);
	DB_set_write_channel(cd->channel);
	DB_channel_set_flag(flags & ~OCF_CHANNEL_FLAG_BLOCK);
	DB_channel_add_note(cd->loggedin_user_p->ocf_user_p->id, time(NULL),
		unblocknote);

	chanfix_send_privmsg(md->nick, "Channel %s has been unblocked.",
		cd->channel);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_BLOCK, " %s has unblocked channel %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
	ocf_log(OCF_LOG_CHANFIX, "BLCK %s has unblocked channel %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);

	/* TODO: add the reason here, which is in cd->params */
}

/* ----------------------------------------------------------------- */

void process_cmd_rehash(struct message_data *md, struct command_data *cd)
{
	chanfix_send_privmsg(md->nick, "Rehashing.");
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_MAIN, "%s has rehashed.", 
		get_logged_in_user_string(cd->loggedin_user_p));
	ocf_log(OCF_LOG_MAIN, "RHSH %s has rehashed.", 
		get_logged_in_user_string(cd->loggedin_user_p));

	/* Save logged in users. */
	ocf_user_save_loggedin();

	/* Delete all users and logged in users. */
	ocf_users_deinit();
	ocf_ignores_deinit();
	ocf_logfiles_close();

	/* Rehash settings. */
	settings_read_file();
	ocf_logfiles_open();
	
	/* Load user file and restore logged in users. */
	ocf_user_load_all(settings_get_str("userfile"));
	ocf_user_load_loggedin();
	ocf_ignore_load(settings_get_str("ignorefile"));
    send_notices_instead_of_privmsgs = settings_get_bool("send_notices");
}

/* ----------------------------------------------------------------- */

void process_cmd_check(struct message_data *md, struct command_data *cd)
{
	struct Channel *chptr;

	if (!settings_get_bool("enable_cmd_check")) {
		chanfix_send_privmsg(md->nick, "This command has been disabled.");
		return;
	}

	chptr = hash_find_channel(cd->channel);

	if (!chptr) {
		chanfix_send_privmsg(md->nick, "No such channel %s.", cd->channel);
		return;
	}

	/* Reports ops and total clients. */
	chanfix_send_privmsg(md->nick, 
		"I see %d opped out of %d total clients in \"%s\".",
		ocf_get_number_chanops(chptr),
		chptr->users, cd->channel);

	/* Log command. */
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_CHANFIX,
		"%s has requested a CHECK on channel \"%s\"", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
	ocf_log(OCF_LOG_CHANFIX, "CHCK %s has requested check %s", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
}

/* ----------------------------------------------------------------- */

void process_cmd_opnicks(struct message_data *md, struct command_data *cd)
{
	struct Channel *chptr;
	struct Client *clptr;
	int num_ops = 0;
	char line[IRCLINELEN + 1];
	dlink_node *node_client;

	if (!settings_get_bool("enable_cmd_opnicks")) {
		chanfix_send_privmsg(md->nick, "This command has been disabled.");
		return;
	}

	chptr = hash_find_channel(cd->channel);

	if (!chptr) {
		chanfix_send_privmsg(md->nick, "No such channel %s.", cd->channel);
		return;
	}

	/* Send list of opped clients. */
	chanfix_send_privmsg(md->nick, "Opped clients on channel \"%s\":",
		cd->channel);

	line[0] = '\0';

	DLINK_FOREACH(node_client, chptr->chanops.head) {
		clptr = (struct Client*)(node_client->data);
		num_ops++;
		strcat(line, clptr->name);
		strcat(line, " ");
		if (strlen(line) > IRCLINELEN / 2) {
			/* Send line and clear the line. */
			chanfix_send_privmsg(md->nick, "%s", line);
			line[0] = '\0';
		}
	}

#ifdef REQUIRE_OANDV   
        DLINK_FOREACH(node_client, chptr->chanops_voiced.head) {
                clptr = (struct Client*)(node_client->data);
                num_ops++;
                strcat(line, clptr->name);
                strcat(line, " ");
                if (strlen(line) > IRCLINELEN / 2) {
                        /* Send line and clear the line. */
                        chanfix_send_privmsg(md->nick, "%s", line);
                        line[0] = '\0';
                }
        }
#endif

	if (strlen(line) > 0) {
		chanfix_send_privmsg(md->nick, "%s", line);
	}

	if (num_ops == 1) {
		chanfix_send_privmsg(md->nick, "I see 1 opped client in \"%s\".", 
			cd->channel);
	} else {
		chanfix_send_privmsg(md->nick, "I see %d opped clients in \"%s\".", 
			num_ops, cd->channel);
	}

	/* Log command. */
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_CHANFIX,
		"%s has requested OPNICKS on channel \"%s\"", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
	ocf_log(OCF_LOG_CHANFIX, "OPNK %s has requested OPNICKS %s", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
}

/* ----------------------------------------------------------------- */

void process_cmd_alert(struct message_data *md, struct command_data *cd)
{
	unsigned int flags;
	char blocknote[IRCLINELEN];

	DB_set_read_channel(cd->channel);
	flags = DB_channel_get_flag();
	if (flags & OCF_CHANNEL_FLAG_ALERT) {
		chanfix_send_privmsg(md->nick, "Channel %s already has the ALERT flag.",
			cd->channel);
		return;
	}
	
	snprintf(blocknote, sizeof(blocknote), "ALERT flag added by %s", 
		cd->loggedin_user_p->ocf_user_p->name);
	DB_set_write_channel(cd->channel);
	DB_channel_set_flag(OCF_CHANNEL_FLAG_ALERT);
	DB_channel_add_note(cd->loggedin_user_p->ocf_user_p->id, time(NULL),
		blocknote);

	chanfix_send_privmsg(md->nick, "ALERT flag added to channel \"%s\".",
		cd->channel);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_CHANFIX, "%s has alerted channel %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
	ocf_log(OCF_LOG_CHANFIX, "ALRT %s has alerted channel %s", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
}

/* ----------------------------------------------------------------- */

void process_cmd_unalert(struct message_data *md, struct command_data *cd)
{
	unsigned int flags;
	char unblocknote[IRCLINELEN];

	DB_set_read_channel(cd->channel);
	flags = DB_channel_get_flag();
	if (!(flags & OCF_CHANNEL_FLAG_ALERT)) {
		chanfix_send_privmsg(md->nick, "Channel %s does not have the ALERT flag.",
			cd->channel);
		return;
	}
	
	snprintf(unblocknote, sizeof(unblocknote), "UNALERT by %s", cd->loggedin_user_p->ocf_user_p->name);
	DB_set_write_channel(cd->channel);
	DB_channel_set_flag(flags & ~OCF_CHANNEL_FLAG_ALERT);
	DB_channel_add_note(cd->loggedin_user_p->ocf_user_p->id, time(NULL),
		unblocknote);

	chanfix_send_privmsg(md->nick, "ALERT flag removed from channel \"%s\".",
		cd->channel);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_CHANFIX, "%s has unalerted channel %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
	ocf_log(OCF_LOG_CHANFIX, "ALRT %s has unalerted channel %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), cd->channel);
}

/* ----------------------------------------------------------------- */

void process_cmd_addserver(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	int result;
	ou = ocf_get_user(cd->name);

	if (ou == NULL){
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	if (ocf_user_has_server(ou, cd->server)) {
		chanfix_send_privmsg(md->nick, "User %s already has server %s.", 
			cd->name, cd->server);
		return;
	}

	result = ocf_user_add_server(ou, cd->server);
	if (!result) {
		chanfix_send_privmsg(md->nick, "Failed adding server %s to user %s.",
			cd->server, ou->name);
		return;
	}

	ocf_users_updated();
	chanfix_send_privmsg(md->nick, "Added server %s to user %s.",
		cd->server, ou->name);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER, "%s added server %s to user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		cd->server, ou->name);
	send_notice_to_loggedin_username(ou->name, "%s added you the server %s",
		get_logged_in_user_string(cd->loggedin_user_p), cd->server);
	ocf_log(OCF_LOG_USERS, "USSE %s added server %s to user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		cd->server, ou->name);
}

/* ----------------------------------------------------------------- */

void process_cmd_delserver(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	ou = ocf_get_user(cd->name);

	if (ou == NULL){
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	if (!ocf_user_has_other_server(ou, cd->server)) {
		chanfix_send_privmsg(md->nick, "User %s doesn't have server %s.", 
			cd->name, cd->server);
		return;
	}

	ocf_user_del_server(ou, cd->server);
	ocf_users_updated();
	chanfix_send_privmsg(md->nick, "Deleted server %s from user %s.",
		cd->server, ou->name);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER, "%s deleted server %s from user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		cd->server, ou->name);
	send_notice_to_loggedin_username(ou->name, "%s deleted your server %s",
		get_logged_in_user_string(cd->loggedin_user_p), cd->server);
	ocf_log(OCF_LOG_USERS, "USSE %s deleted server %s from user %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		cd->server, ou->name);
}

/* ----------------------------------------------------------------- */

void process_cmd_whoserver(struct message_data *md, struct command_data *cd)
{
	dlink_node *ptr;
	struct ocf_user *ou;
	char names[IRCLINELEN];
	int number_of_users = 0;
	names[0] = 0;

	/* A serveradmin can only add flags to users on his own server. */
	if (ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_SERVERADMIN) &&
		!ocf_user_has_flag(cd->loggedin_user_p->ocf_user_p, OCF_USER_FLAGS_USERMANAGER)
	) {
		if (!ocf_user_has_main_server(cd->loggedin_user_p->ocf_user_p, cd->server)) {
			chanfix_send_privmsg(md->nick, 
				"You can only use WHOSERVER on your main server.");
			return;
		}
	}

	chanfix_send_privmsg(md->nick, "Users with main server %s [username (flags)]:",
		cd->server);

	DLINK_FOREACH(ptr, ocf_user_list.head) {
		ou = ((struct ocf_user*)(ptr->data));

		if (!ou->deleted && ocf_user_has_main_server(ou, cd->server)) {
			number_of_users++;
			strcat(names, ou->name);
			strcat(names, " (");
			if (ou->flags) {
				strcat(names, ocf_user_flags_to_string(ou->flags));
			}
			strcat(names, ") ");
			
			/* If the string is getting too long, send it now and empty it. */
			if (strlen(names) > IRCLINELEN - 50) {
				chanfix_send_privmsg(md->nick, "%s", names);
				names[0] = 0;
			}
		}
	}
	
	if (strlen(names) > 0) {
		chanfix_send_privmsg(md->nick, "%s", names);
	}
	names[0] = 0;
	
	chanfix_send_privmsg(md->nick, "Users with other server %s [username (flags)]:",
		cd->server);

	DLINK_FOREACH(ptr, ocf_user_list.head) {
		ou = ((struct ocf_user*)(ptr->data));

		if (ocf_user_has_other_server(ou, cd->server)) {
			number_of_users++;
			strcat(names, ou->name);
			strcat(names, " (");
			if (ou->flags) {
				strcat(names, ocf_user_flags_to_string(ou->flags));
			}
			strcat(names, ") ");
			
			/* If the string is getting too long, send it now and empty it. */
			if (strlen(names) > IRCLINELEN - 50) {
				chanfix_send_privmsg(md->nick, "%s", names);
				names[0] = 0;
			}
		}
	}
	
	if (strlen(names) > 0) {
		chanfix_send_privmsg(md->nick, "%s", names);
	}
	chanfix_send_privmsg(md->nick, "Number of users: %d.", number_of_users);
}

/* ----------------------------------------------------------------- */

void process_cmd_setmainserver(struct message_data *md, struct command_data *cd)
{
	struct ocf_user *ou;
	int result;
	ou = ocf_get_user(cd->name);

	if (ou == NULL){
		chanfix_send_privmsg(md->nick, "No such user %s.", cd->name);
		return;
	}

	result = ocf_user_set_main_server(ou, cd->server);
	if (!result) {
		chanfix_send_privmsg(md->nick, "Failed setting main server of user %s to %s.",
			ou->name, cd->server);
		return;
	}

	ocf_users_updated();
	chanfix_send_privmsg(md->nick, "Set main server of user %s to %s.",
		ou->name, cd->server);
	send_notice_to_loggedin_except(cd->loggedin_user_p, 
		OCF_NOTICE_FLAGS_USER, "%s set main server of user %s to %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		ou->name, cd->server);
	send_notice_to_loggedin_username(ou->name, "%s set your main server to %s.",
		get_logged_in_user_string(cd->loggedin_user_p), cd->server);
	ocf_log(OCF_LOG_USERS, "USSE %s set main server of user %s to %s.", 
		get_logged_in_user_string(cd->loggedin_user_p), 
		ou->name, cd->server);
}

/* ----------------------------------------------------------------- */

