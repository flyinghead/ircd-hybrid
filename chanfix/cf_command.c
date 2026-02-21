/*
 * $Id: cf_command.c,v 1.13 2004/10/30 19:50:53 cfh7 REL_2_1_3 $
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
/* The command words per command.
 * CAREFUL - If making changes to this, make sure it's still in sync with
 * the CMD_* defines in cf_command.h! 
 */
char* cmd_word[] = {
	"__UNKNOWN__",
	"HELP",
	"SCORE",
	"CHANFIX",
	"LOGIN",
	"LOGOUT",
	"PASSWD",
	"HISTORY",
	"INFO",
	"OPLIST",
	"ADDNOTE",
	"DELNOTE",
	"WHOIS",
	"ADDFLAG",
	"DELFLAG",
	"ADDHOST",
	"DELHOST",
	"ADDUSER",
	"DELUSER",
	"CHPASS",
	"WHO",
	"UMODE",
	"STATUS",
	"SET",
	"BLOCK",
	"UNBLOCK",
	"REHASH",
	"CHECK",
	"OPNICKS",
	"CSCORE",
	"ALERT",
	"UNALERT",
	"ADDSERVER",
	"DELSERVER",
	"CHPASSHASH",
	"WHOSERVER",
	"SETMAINSERVER",
};

/* ----------------------------------------------------------------- */
/* The required number of parameters per command.
 * CAREFUL - If making changes to this, make sure it's still in sync with
 * the CMD_* defines in cf_command.h! 
 */
int cmd_num_params[] = {
	0, /* __UNKNOWN__ */
	0, /* HELP */
	1, /* SCORE */
	1, /* CHANFIX */
	2, /* LOGIN */
	0, /* LOGOUT */
	2, /* PASSWD */
	1, /* HISTORY */
	1, /* INFO */
	1, /* OPLIST */
	2, /* ADDNOTE */
	2, /* DELNOTE */
	1, /* WHOIS */
	2, /* ADDFLAG */
	2, /* DELFLAG */
	2, /* ADDHOST */
	2, /* DELHOST */
	1, /* ADDUSER */
	1, /* DELUSER */
	2, /* CHPASS */
	0, /* WHO */
	0, /* UMODE */
	0, /* STATUS */
	2, /* SET */
	2, /* BLOCK */
	1, /* UNBLOCK */
	0, /* REHASH */
	1, /* CHECK */
	1, /* OPNICKS */
	1, /* CSCORE */
	1, /* ALERT */
	1, /* UNALERT */
	2, /* ADDSERVER */
	2, /* DELSERVER */
	2, /* CHPASSHASH */
	1, /* WHOSERVER */
	2, /* SETMAINSERVER */
};

/* ----------------------------------------------------------------- */
/* The required flags per command.
 * CAREFUL - If making changes to this, make sure it's still in sync with
 * the CMD_* defines in cf_command.h! 
 */
int cmd_flags_required[] = {
	0,                          /* __UNKNOWN__ */
	0,                          /* HELP */
	0,                          /* SCORE */
	OCF_USER_FLAGS_FIX,         /* CHANFIX */
	0,                          /* LOGIN */
	0,                          /* LOGOUT */
	0,                          /* PASSWD */
	0,                          /* HISTORY */
	0,                          /* INFO */
	OCF_USER_FLAGS_FIX,         /* OPLIST */
	OCF_USER_FLAGS_CHANNEL,     /* ADDNOTE */
	OCF_USER_FLAGS_CHANNEL,     /* DELNOTE */
	0,                          /* WHOIS */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* ADDFLAG */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* DELFLAG */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* ADDHOST */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* DELHOST */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* ADDUSER */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* DELUSER */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* CHPASS */
	0,                          /* WHO */
	0,                          /* UMODE */
	0,                          /* STATUS */
	OCF_USER_FLAGS_OWNER,       /* SET */
	OCF_USER_FLAGS_BLOCK,       /* BLOCK */
	OCF_USER_FLAGS_BLOCK,       /* UNBLOCK */
	OCF_USER_FLAGS_OWNER,       /* REHASH */
	0,                          /* CHECK */
	OCF_USER_FLAGS_FIX,         /* OPNICKS */
	0,                          /* CSCORE */
	OCF_USER_FLAGS_CHANNEL,     /* ALERT */
	OCF_USER_FLAGS_CHANNEL,     /* UNALERT */
	OCF_USER_FLAGS_USERMANAGER, /* ADDSERVER */
	OCF_USER_FLAGS_USERMANAGER, /* DELSERVER */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* CHPASSHASH */
	OCF_USER_FLAGS_USERMANAGER | OCF_USER_FLAGS_SERVERADMIN, /* WHOSERVER */
	OCF_USER_FLAGS_USERMANAGER, /* SETMAINSERVER */
};

/* ----------------------------------------------------------------- */
/* The help for each command 
 * CAREFUL - If making changes to this, make sure it's still in sync with
 * the CMD_* defines in cf_command.h! 
 */

char* cmd_help_string[] = {
	"Unknown command.",
	"HELP <command>\n"
		"Shows help about <command>.",
	"SCORE <channel> [nick|hostmask]\n"
		"Without extra arguments, shows the top scores of <channel>.\n"
		"Otherwise, it shows the score of either the currently online client "
		"<nick>, or <hostmask> for <channel>.",
	"CHANFIX <channel> [override]\n"
		"Performs a manual fix on <channel>. Append OVERRIDE, YES or an "
		"exclamation mark (!) to force this manual fix.",
	"LOGIN <username> <password>  [Note: see HELP LOGIN !]\n"
		"Logs you in as user <username>, if your hostmask and password "
		"match.\nNote: for security reasons, you MUST message the login "
		"command to <username>@<server>, where <username> is OCF's username "
		"and <server> is OCF's server.",
	"LOGOUT\n"
		"Logs you out if you're logged in.",
	"PASSWD <currentpass> <newpass>\n"
		"Sets your password to <newpass>.",
	"HISTORY <channel>\n"
		"Shows the times that <channel> has been manually fixed.",
	"INFO <channel>\n"
		"Shows all notes of this channel, and whether it has been blocked.",
	"OPLIST <channel>\n"
		"Shows a list of hostmasks plus their score of the top ops of this "
		"channel.\nNOTE: the use of this command broadcasts a notice to all "
		"users logged in to chanfix.",
	"ADDNOTE <channel> <note>\n"
		"Adds a note to a channel.",
	"DELNOTE <channel> <note_id>\n"
		"Deletes the note with this note_id from the channel. You can only "
		"delete notes you added yourself.",
	"WHOIS <user>\n"
		"Shows information about this user.",
	"ADDFLAG <user> <flag>\n"
		"Adds this flag to the user. Possible flags:\n"
		"b - can block/unblock channels\n"
		"f - can perform manual chanfix\n"
		"u - can manage users\n"
		"Note: add only a single flag per command.",
	"DELFLAG <user> <flag>\n"
		"Removes this flag from the user. See ADDFLAG.",
	"ADDHOST <user> <hostmask>\n"
		"Adds this hostmask to the user's list of hostmasks.",
	"DELHOST <user> <hostmask>\n"
		"Deletes this hostmask from the user's list of hostmasks.",
	"ADDUSER <user> [hostmask]\n"
		"Adds a new user, without flags, and optionally with this hostmask.",
	"DELUSER <user> [hostmask]\n"
		"Deletes this user.",
	"CHPASS <user> <newpass>\n"
		"Changes the password of <user> into <newpass>.",
	"WHO\n"
		"Shows all logged in users.",
	"UMODE [<+|->flags]\n"
		"Without flags, this shows your current umode.\n"
		"With +flags, these flags are added to your current umode.\n"
		"With -flags, these flags are removed to your current umode.\n"
		"Possible flags: abclmnu.",
	"STATUS\n"
		"Shows current status.",
	"SET <setting> <value>\n"
		"Sets <setting> to value <value>.\n"
		"Boolean settings: ENABLE_AUTOFIX, ENABLE_CHANFIX.\n"
		"Integer settings: NUM_SERVERS.",
	"BLOCK <channel> <reason>\n"
		"Blocks a channel from being fixed, both automatically and manually.\n"
		"The reason will be shown when doing INFO <channel>.",
	"UNBLOCK <channel>\n"
		"Removes the block on a channel.",
	"REHASH\n"
		"Reloads config and users.",
	"CHECK <channel>\n"
		"Shows the number of ops and total clients in <channel>. Broadcasts "
		"a notice to all logged in users.",
	"OPNICKS <channel>\n"
		"Shows the nicks of all ops in <channel>. Broadcasts a notice to all "
		"logged in users.",
	"CSCORE <channel> [nick|hostmask]\n"
		"Shows the same as SCORE, but in a compact output. See HELP SCORE.",
	"ALERT <channel>\n"
		"Sets the ALERT flag of this channel.",
	"UNALERT <channel>\n"
		"Unsets the ALERT flag of this channel.",
	"ADDSERVER <user> <server>\n"
		"Adds this server to the user's list of servers.",
	"DELSERVER <user> <server>\n"
		"Deletes this server from the user's list of servers.",
	"CHPASSHASH <user> <hash>\n"
		"Changes the password hash of <user> to <hash>.",
	"WHOSERVER <server>\n"
		"Shows all users from server <server>.",
	"SETMAINSERVER <user> <server>\n"
		"Sets the main server of <user> to <server>.",
};


/* ----------------------------------------------------------------- */
/* Function pointers to data obtaining functions for each command 
 * CAREFUL - If making changes to this, make sure it's still in sync with
 * the CMD_* defines in cf_command.h! 
 */

get_command_data_t cmd_get_data_functions[] = {
	get_command_unknown_data,
	get_command_help_data,
	get_command_score_data,
	get_command_chanfix_data,
	get_command_login_data,
	get_command_logout_data,
	get_command_passwd_data,
	get_command_history_data,
	get_command_info_data,
	get_command_oplist_data,
	get_command_addnote_data,
	get_command_delnote_data,
	get_command_whois_data,
	get_command_addflag_data,
	get_command_delflag_data,
	get_command_addhost_data,
	get_command_delhost_data,
	get_command_adduser_data,
	get_command_deluser_data,
	get_command_chpass_data,
	get_command_who_data,
	get_command_umode_data,
	get_command_status_data,
	get_command_set_data,
	get_command_block_data,
	get_command_unblock_data,
	get_command_rehash_data,
	get_command_check_data,
	get_command_opnicks_data,
	get_command_cscore_data,
	get_command_alert_data,
	get_command_unalert_data,
	get_command_addserver_data,
	get_command_delserver_data,
	get_command_chpasshash_data,
	get_command_whoserver_data,
	get_command_setmainserver_data,
};

/* ----------------------------------------------------------------- */

int get_command_int_from_word(char* word)
{
	int i;

	for (i = 1; i <= NUM_COMMANDS; i++) {
		if (strcasecmp(cmd_word[i], word) == 0) {
			return i;
		}
	}

	return CMD_UNKNOWN;
}

/* ----------------------------------------------------------------- */

void get_command_help(int cmd, char* string, int length)
{
	if (cmd < 0 || cmd > NUM_COMMANDS) {
		snprintf(string, length, "get_command_help: unknown command id %d.", cmd);
		return;
	}
	
	strncpy(string, cmd_help_string[cmd], length);
}

/* ----------------------------------------------------------------- */

int command_requires_loggedin(int cmd)
{
	switch (cmd){
	case CMD_HELP:
	case CMD_LOGIN:
	case CMD_SCORE:
	case CMD_CSCORE:
	case CMD_HISTORY:
	case CMD_INFO:
	case CMD_UNKNOWN:
		return 0;
	default:
		return 1;
	}

	return 1;
}

/* ----------------------------------------------------------------- */

void get_command_data(char *command, struct command_data *cd)
{
	char *parv[OCF_MAXPARA];
	int parc;
	int cmd;
	get_command_data_t funcp;
	cd->wrong_syntax = 0;

	/* parc will always be 1 or more for a normal command
	 * parv[0] contains the command, parv[1] and onwards the args */
	parc = tokenize_string(command, parv);

	if (parc == 0) {
		return;
	}

	cmd = get_command_int_from_word(parv[0]);
	cd->command = cmd;

	/* Check whether the minimum amount of parameters has been given */
	if (parc < cmd_num_params[cmd] + 1) {
		ocf_log(OCF_LOG_DEBUG, "not enough params (%d) for command %s [%d] (min %d params)",
			parc - 1, parv[0], cmd, cmd_num_params[cmd]);
		cd->wrong_syntax = 1;
		return;
	}

	ocf_log(OCF_LOG_DEBUG, "%d tokens, cmd is %d, command: %s", parc, cmd, command);

	funcp = cmd_get_data_functions[cmd];
	if (funcp == NULL) {
		ocf_log(OCF_LOG_DEBUG, "Eeks! NULL function pointer in get_command_data!");
		ocf_log(OCF_LOG_ERROR, 
			"NULL function pointer in get_command_data (cmd is %d).",
			cmd);
		cd->command = CMD_UNKNOWN;
		return;
	}
	funcp(cd, parc, parv);
}

/* ----------------------------------------------------------------- */

void get_command_unknown_data(struct command_data *cd, int parc, char** parv)
{
}

/* ----------------------------------------------------------------- */
/* HELP [command] */

void get_command_help_data(struct command_data *cd, int parc, char** parv)
{
	if (parc > 1) {
		strncpy(cd->params, parv[1], sizeof(cd->params));
		cd->params[sizeof(cd->params) - 1] = 0;
	} else {
		cd->params[0] = 0;
	}
}

/* ----------------------------------------------------------------- */
/* SCORE <channel> [nick|hostmask] */

void get_command_score_data(struct command_data *cd, int parc, char** parv)
{
	char* cptr;
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel) - 1] = 0;	/* Just in case */

	cd->nick[0] = 0;
	cd->user[0] = 0;
	cd->host[0] = 0;

	if (parc == 3) {
		if ((cptr = strchr(parv[2], '@')) != NULL) {
			memset(cd->user, 0, sizeof(cd->user));
			memset(cd->host, 0, sizeof(cd->host));
			memcpy(cd->user, parv[2], cptr - parv[2]);
			strncpy(cd->host, cptr + 1, sizeof(cd->host));
		} else {
			strncpy(cd->nick, parv[2], sizeof(cd->nick));
			cd->nick[sizeof(cd->nick) - 1] = 0;	/* Just in case */
		}
	}
}

/* ----------------------------------------------------------------- */
/* CHANFIX <channel> [override] */

void get_command_chanfix_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel) - 1] = 0;

	cd->flags = 0;

	/* Check whether the OVERRIDE flag has been given */
	if (parc > 2) {
		if (!strcasecmp(parv[2], "override") ||
				!strcasecmp(parv[2], "now") ||
				parv[2][0] == '!') {
			cd->flags = 1;
		}
	}
}

/* ----------------------------------------------------------------- */
/* LOGIN <username> <password> */

void get_command_login_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->password, parv[2], sizeof(cd->password));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->password[sizeof(cd->password)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_logout_data(struct command_data *cd, int parc, char** parv)
{
}

/* ----------------------------------------------------------------- */
/* PASSWD <currentpass> <newpass> */

void get_command_passwd_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->params, parv[1], sizeof(cd->params));
	strncpy(cd->password, parv[2], sizeof(cd->password));
	cd->params[sizeof(cd->params)-1] = 0;
	cd->password[sizeof(cd->password)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_history_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_info_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_oplist_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_addnote_data(struct command_data *cd, int parc, char** parv)
{
	int i;
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;

	cd->params[0] = 0;
	for (i = 2; i < parc; i++) {
		strcat(cd->params, parv[i]);
		strcat(cd->params, " ");
	}
	/* TODO: reassemble the note :) */
}

/* ----------------------------------------------------------------- */

void get_command_delnote_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
	cd->id = atoi(parv[2]);
	/* TODO: reassemble the note :) */
}

/* ----------------------------------------------------------------- */

void get_command_whois_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	cd->name[sizeof(cd->name)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_addflag_data(struct command_data *cd, int parc, char** parv)
{
	/* For now, only 1 flag at a time. */
	if (strlen(parv[2]) != 1) {
		cd->wrong_syntax = 1;
		return;
	}

	cd->flags = ocf_user_flag_get_int(parv[2][0]);

	/* And it must be a known flag. */
	if (cd->flags == 0){
		cd->wrong_syntax = 1;
		return;
	}
	
	strncpy(cd->name, parv[1], sizeof(cd->name));
	cd->name[sizeof(cd->name)-1] = 0;
}

void get_command_delflag_data(struct command_data *cd, int parc, char** parv)
{
	/* For now, only 1 flag at a time. */
	if (strlen(parv[2]) != 1) {
		cd->wrong_syntax = 1;
		return;
	}

	cd->flags = ocf_user_flag_get_int(parv[2][0]);

	/* And it must be a known flag. */
	if (cd->flags == 0){
		cd->wrong_syntax = 1;
		return;
	}
	
	strncpy(cd->name, parv[1], sizeof(cd->name));
	cd->name[sizeof(cd->name)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_addhost_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->hostmask, parv[2], sizeof(cd->hostmask));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->hostmask[sizeof(cd->hostmask)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_delhost_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->hostmask, parv[2], sizeof(cd->hostmask));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->hostmask[sizeof(cd->hostmask)-1] = 0;
}

/* ----------------------------------------------------------------- */
/* ADDUSER <username> [user@host] */

void get_command_adduser_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	cd->name[sizeof(cd->name)-1] = 0;

	if (parc == 3) {
		strncpy(cd->hostmask, parv[2], sizeof(cd->hostmask));
		cd->hostmask[sizeof(cd->hostmask)-1] = 0;
	} else {
		cd->hostmask[0] = 0;
	}
}

/* ----------------------------------------------------------------- */
/* DELUSER <username> */

void get_command_deluser_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	cd->name[sizeof(cd->name)-1] = 0;
}

/* ----------------------------------------------------------------- */
/* CHPASS <username> <newpass> */

void get_command_chpass_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->password, parv[2], sizeof(cd->password));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->password[sizeof(cd->password)-1] = 0;
}

/* ----------------------------------------------------------------- */
/* CHPASSHASH <username> <hash> */

void get_command_chpasshash_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->password, parv[2], sizeof(cd->password));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->password[sizeof(cd->password)-1] = 0;
}

/* ----------------------------------------------------------------- */
/* WHO */

void get_command_who_data(struct command_data *cd, int parc, char** parv)
{
}

/* ----------------------------------------------------------------- */
/* UMODE 
 * If the argument is + or - some flags, the flags are stored in
 * cd->flags. If +, then cd->params[0] = '+'; 
 * If -, then cd->params[0] = '-'. 
 * Without flags, cd->flags is 0. 
 **/

void get_command_umode_data(struct command_data *cd, int parc, char** parv)
{
	int i;

	cd->flags = 0;
	cd->params[0] = 0;
	if (parc > 1) {
		if (parv[1][0] == '-') {
			cd->params[0] = '-';
			cd->params[1] = 0;
			for (i = 1; i < strlen(parv[1]); i++) {
				cd->flags |= ocf_user_notice_flag_get_int(parv[1][i]);
			}
		} else if (parv[1][0] == '+') {
			cd->params[0] = '+';
			cd->params[1] = 0;
			for (i = 1; i < strlen(parv[1]); i++) {
				cd->flags |= ocf_user_notice_flag_get_int(parv[1][i]);
			}
		} 
	}
}

/* ----------------------------------------------------------------- */
/* STATUS */

void get_command_status_data(struct command_data *cd, int parc, char** parv)
{
}

/* ----------------------------------------------------------------- */
/* SET <setting> <value> */

void get_command_set_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->params, parv[2], sizeof(cd->params));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->params[sizeof(cd->params)-1] = 0;
}

/* ----------------------------------------------------------------- */
/* BLOCK <channel> <reason> */

void get_command_block_data(struct command_data *cd, int parc, char** parv)
{
	int i;
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;

	cd->params[0] = 0;
	for (i = 2; i < parc; i++) {
		strcat(cd->params, parv[i]);
		strcat(cd->params, " ");
	}
}

/* ----------------------------------------------------------------- */
/* UNBLOCK <channel> */

void get_command_unblock_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
}

/* ----------------------------------------------------------------- */
/* REHASH */

void get_command_rehash_data(struct command_data *cd, int parc, char** parv)
{
}

/* ----------------------------------------------------------------- */
/* CHECK <channel> */

void get_command_check_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
}

/* ----------------------------------------------------------------- */
/* OPNICKS <channel> */

void get_command_opnicks_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
}

/* ----------------------------------------------------------------- */
/* CSCORE <channel> [nick|hostmask] */

void get_command_cscore_data(struct command_data *cd, int parc, char** parv)
{
	char* cptr;
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;

	cd->nick[0] = 0;
	cd->user[0] = 0;
	cd->host[0] = 0;

	if (parc == 3) {
		if ((cptr = strchr(parv[2], '@')) != NULL) {
			memset(cd->user, 0, sizeof(cd->user));
			memset(cd->host, 0, sizeof(cd->host));
			memcpy(cd->user, parv[2], cptr - parv[2]);
			strncpy(cd->host, cptr + 1, sizeof(cd->host));
		} else {
			strncpy(cd->nick, parv[2], sizeof(cd->nick));
			cd->nick[sizeof(cd->nick)-1] = 0;
		}
	}
}

/* ----------------------------------------------------------------- */

void get_command_alert_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_unalert_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->channel, parv[1], sizeof(cd->channel));
	cd->channel[sizeof(cd->channel)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_addserver_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->server, parv[2], sizeof(cd->server));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->server[sizeof(cd->server)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_delserver_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->server, parv[2], sizeof(cd->server));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->server[sizeof(cd->server)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_whoserver_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->server, parv[1], sizeof(cd->server));
	cd->server[sizeof(cd->server)-1] = 0;
}

/* ----------------------------------------------------------------- */

void get_command_setmainserver_data(struct command_data *cd, int parc, char** parv)
{
	strncpy(cd->name, parv[1], sizeof(cd->name));
	strncpy(cd->server, parv[2], sizeof(cd->server));
	cd->name[sizeof(cd->name)-1] = 0;
	cd->server[sizeof(cd->server)-1] = 0;
}

/* ----------------------------------------------------------------- */
