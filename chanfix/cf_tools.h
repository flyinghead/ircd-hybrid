/*
 * $Id: cf_tools.h,v 1.10 2004/10/30 19:56:02 cfh7 REL_2_1_3 $
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





/* These variables are used by the tokenize_string function. */
#define OCF_MAXPARA 255

#define IRCLINELEN 512

/* YYYY-MM-DD HH:MM:SS */
#define OCF_TIMESTAMP_LEN 20

/* Turns a string into an array of strings, separated by <separator>.
 * Example: "for great justice", tokenized with separator ' ', becomes
 * "for", "great" and "justice".
 * The return value is the number of tokens found.
 * By default, the space will be used as separator.
 *
 * Note: if there are more than OCF_MAXPARA tokens, the last token will
 * contain the rest of the string, i.e. all tokens that didn't fit.
 */
int tokenize_string_separator(char *string, char *parv[OCF_MAXPARA], char separator);
int tokenize_string(char *string, char *parv[OCF_MAXPARA]);

/* Removes all whitespace at the beginning and end of <string>. */
void strtrim(char *string);

/* Reads a line from file fp into <line>. Stops when \n is encountered
 * or <maxlen> characters has been reached, whichever occurs first. */
int readline(FILE * fp, char* line, int maxlen);

/* Writes a line to file fp, printf-style. */
int writeline(FILE * fp, char* pattern, ...);

/* If <user>@<host> is already present in array userhosts which has
 * num_userhosts userhosts, nothing happens and <num_userhosts> is
 * returned.
 * Otherwise, <user>@<host> is added to <userhosts> at the end, and
 * <num_userhosts> + 1 is returned. 
 * NOTE: this function does not allocate anything, it only adds
 * some pointers in users/hosts. */
int add_userhost_to_array(char** users, char** hosts, int num_userhosts, 
						  char* user, char* host);

/*
 * If you want to make chanfix issue a client command, you have
 * to use this function. The commands are in raw client IRC
 * format, like "PRIVMSG nick :my message".
 *
 * Using sendto_one(chanfix_remote, msg); will not work properly
 * because chanfix_remote is not in any client lists and therefore
 * its send buffer will never be emptied automatically.
 */
void issue_chanfix_command(const char *pattern, ...);


void chanfix_send_privmsg(char* target, const char *pattern, ...);

/* Groups num_args commands of the form
 * MODE <channel> <plusminus><mode> <arg>
 * into as many groups of
 * MODE <channel> <plusminus><mode><mode><mode> <arg><arg><arg>
 * as necessary to perform all num_args modes. */
void chanfix_perform_mode_group(char* channel, char plusminus, char mode, 
			char** args, int num_args);

/* Performs
 * MODE <channel> <plusminus><mode><mode><mode> <arg><arg><arg>
 * where the number of modes/args is <number>. */
int chanfix_perform_mode_num(char* channel, char plusminus, char mode, 
			char** args, int number);

/* Removes all bans, +i, +k and +l modes from the channel. */
void chanfix_remove_channel_modes(struct Channel *chptr);

/* Returns the number of servers currently linked, including this
 * one. */
int get_number_of_servers_linked();

/* Returns true if the minimum amount of servers necessary for
 * chanfix to work are present. 
 * Returns false if not. */
int check_minimum_servers_linked();

/* Returns the number of seconds in the current day. */
int get_seconds_in_current_day();

/* Returns the number of days since 1 Jan 1970, in GMT/UTC.
 * This function is used for the daily rotation of the database.
 * Using GMT means that the rotation of the database will take
 * place at midnight GMT, regardless of your own timezone.
 * The advantage of this is that you can move the database to
 * another timezone without problems.
 */
int get_days_since_epoch();

/* Returns a pointer to a buffer containing a YYYY-MM-DD HH:MM:SS
 * representation of <seconds>. This is a static buffer which is
 * overwritten each time this function is called. */
char* seconds_to_datestring(time_t seconds);


/* Returns a pointer to a null-terminated string with in it
 * the following text:
 * nickname (username@hostname) [accountname]
 * This is stored in a static buffer which is overwritten
 * each time this function is called.
 */
struct ocf_loggedin_user;
char* get_logged_in_user_string(struct ocf_loggedin_user *olu);
