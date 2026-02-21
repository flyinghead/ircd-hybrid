/*
 * $Id: cf_command.h,v 1.8 2004/08/28 08:01:42 cfh7 REL_2_1_3 $
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




/* This file contains all functions necessary to describe commands
 * and to convert a string to a command. 
 * CAREFUL - If making changes to this, make sure it's still in sync with
 * the command data in cf_command.c! 
 */

#define CMD_UNKNOWN			0
#define CMD_HELP			1
#define CMD_SCORE			2
#define CMD_CHANFIX			3
#define CMD_LOGIN			4
#define CMD_LOGOUT			5
#define CMD_PASSWD			6
#define CMD_HISTORY			7
#define CMD_INFO			8
#define CMD_OPLIST			9
#define CMD_ADDNOTE			10
#define CMD_DELNOTE			11
#define CMD_WHOIS			12
#define CMD_ADDFLAG			13
#define CMD_DELFLAG			14
#define CMD_ADDHOST			15
#define CMD_DELHOST			16
#define CMD_ADDUSER			17
#define CMD_DELUSER			18
#define CMD_CHPASS          19
#define CMD_WHO             20
#define CMD_UMODE			21
#define CMD_STATUS			22
#define CMD_SET				23
#define CMD_BLOCK			24
#define CMD_UNBLOCK			25
#define CMD_REHASH			26
#define CMD_CHECK			27
#define CMD_OPNICKS			28
#define CMD_CSCORE			29
#define CMD_ALERT			30
#define CMD_UNALERT			31
#define CMD_ADDSERVER		32
#define CMD_DELSERVER		33
#define CMD_CHPASSHASH		34
#define CMD_WHOSERVER		35
#define CMD_SETMAINSERVER	36

#define NUM_COMMANDS		36

struct ocf_loggedin_user;

/* This is a command.
 * user_p - pointer to the logged in user struct of the user performing the command.
 */
struct command_data {
	int command;
	int wrong_syntax;
	char channel[CHANNELLEN + 1];
	char nick[NICKLEN + 1];
	char user[USERLEN + 1];
	char host[HOSTLEN + 1];
	char server[HOSTLEN + 1];
	char hostmask[USERLEN + HOSTLEN + 1];
	char name[64];
	char password[64];
	char params[IRCLINELEN];
	int flags;
	int id;
	struct ocf_loggedin_user *loggedin_user_p;
};


typedef void (*get_command_data_t)(struct command_data*, int, char**);

/* The following vars are defined in cf_command.c and are all directly
 * related to the #define CMD_* directly above. If you edit any of these,
 * make sure everything else is edited correctly too! */

/* Array of command words. */
extern char* cmd_word[];

/* Array of required number of parameters per command. */
extern int cmd_num_params[];

/* Array of required flag(s) to execute this command. */
extern int cmd_flags_required[];

/* Array of command help strings. */
extern char* cmd_help_string[];

/* Array of functions to get the data of a command (argument parsing). */
extern get_command_data_t cmd_get_data_functions[];

/* Returns 0 if this is an unknown command, or the associated #define
 * value (which is larger than 0) for a known command. */
int get_command_int_from_word(char* word);

void get_command_help(int cmd, char* string, int length);

/* Returns true if <cmd> requires you to be logged in. */
int command_requires_loggedin(int cmd);

/* Retrieves the command and parameters from a command string 
 * and stores them in <cmddata>. */
void get_command_data(char *command, struct command_data *cd);

void get_command_unknown_data(struct command_data *cd, int parc, char** parv);
void get_command_help_data(struct command_data *cd, int parc, char** parv);
void get_command_score_data(struct command_data *cd, int parc, char** parv);
void get_command_chanfix_data(struct command_data *cd, int parc, char** parv);
void get_command_login_data(struct command_data *cd, int parc, char** parv);
void get_command_logout_data(struct command_data *cd, int parc, char** parv);
void get_command_passwd_data(struct command_data *cd, int parc, char** parv);
void get_command_history_data(struct command_data *cd, int parc, char** parv);
void get_command_info_data(struct command_data *cd, int parc, char** parv);
void get_command_oplist_data(struct command_data *cd, int parc, char** parv);
void get_command_addnote_data(struct command_data *cd, int parc, char** parv);
void get_command_delnote_data(struct command_data *cd, int parc, char** parv);
void get_command_whois_data(struct command_data *cd, int parc, char** parv);
void get_command_addflag_data(struct command_data *cd, int parc, char** parv);
void get_command_delflag_data(struct command_data *cd, int parc, char** parv);
void get_command_addhost_data(struct command_data *cd, int parc, char** parv);
void get_command_delhost_data(struct command_data *cd, int parc, char** parv);
void get_command_adduser_data(struct command_data *cd, int parc, char** parv);
void get_command_deluser_data(struct command_data *cd, int parc, char** parv);
void get_command_chpass_data(struct command_data *cd, int parc, char** parv);
void get_command_chpasshash_data(struct command_data *cd, int parc, char** parv);
void get_command_who_data(struct command_data *cd, int parc, char** parv);
void get_command_umode_data(struct command_data *cd, int parc, char** parv);
void get_command_status_data(struct command_data *cd, int parc, char** parv);
void get_command_set_data(struct command_data *cd, int parc, char** parv);
void get_command_block_data(struct command_data *cd, int parc, char** parv);
void get_command_unblock_data(struct command_data *cd, int parc, char** parv);
void get_command_rehash_data(struct command_data *cd, int parc, char** parv);
void get_command_check_data(struct command_data *cd, int parc, char** parv);
void get_command_opnicks_data(struct command_data *cd, int parc, char** parv);
void get_command_cscore_data(struct command_data *cd, int parc, char** parv);
void get_command_alert_data(struct command_data *cd, int parc, char** parv);
void get_command_unalert_data(struct command_data *cd, int parc, char** parv);
void get_command_addserver_data(struct command_data *cd, int parc, char** parv);
void get_command_delserver_data(struct command_data *cd, int parc, char** parv);
void get_command_whoserver_data(struct command_data *cd, int parc, char** parv);
void get_command_setmainserver_data(struct command_data *cd, int parc, char** parv);


