/*
 * $Id: cf_message.h,v 1.6 2004/08/24 22:21:52 cfh7 REL_2_1_3 $
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





/*
Handling of incoming messages to the chanfix client.

All incoming messages to chanfix are converted from the single-line string
to the message_data struct. The members of this struct are:

type     - the type of message. One of the MSG_* defines.
numeric  - if the type == MSG_NUMERIC, this int tells us what the numeric is.
nick     - the nick of the sender, if applicable
user     - the username of the sender, if applicable
host     - the host of the sender, if applicable
server   - the servername, if applicable
data     - the data sent

Possible messages:
PING :chanfix.carnique.nl
:chanfix.carnique.nl 401 C Beige :No such nick/channel
:cfgaar!garion@chan.fix PRIVMSG C :chanfix #test
:chanfix.carnique.nl 001 C :Welcome to the CNQNet Internet Relay Chat Network C
:chanfix.carnique.nl 254 C 2 :channels formed
:chanfix.carnique.nl NOTICE C :*** Spoofing your IP. congrats.

*/

#define MSG_UNKNOWN			0
#define MSG_NUMERIC			1
#define MSG_PING			2
#define MSG_PRIVMSG			3
#define MSG_CHANMSG			4
#define MSG_CTCP			5
#define MSG_KILLED			6
#define MSG_RECON_TOO_FAST	7
#define MSG_NUM_001			8




/* This struct holds data about a message sent by the ircd
 * to the chanfix client. */
struct message_data {
	int type;
	int numeric;
	int safe;
	char nick[NICKLEN + 1];
	char user[USERLEN + 1];
	char host[HOSTLEN + 1];
	char server[HOSTLEN + 1];
	char channel[CHANNELLEN + 1];
	char data[IRCLINELEN + 1];
};

/* Array of function pointers to the functions that process the
 * different commands. */

typedef void (*process_cmd_t)(struct message_data *, struct command_data *);
extern process_cmd_t process_cmd_functions[];

/* Separates readBuf into 1 message per line and processes
 * each message separately */		
int process_received_messages(const char* readBuf, int length);

/* Converts a single message into a struct message_data */
void get_message_data(char *msg, struct message_data *msgdata);

/* Converts a single message, which is a PRIVMSG to me,
 * into a struct message_data */
void get_privmsg_data(char *msg, struct message_data *msgdata);

/* Converts a single message, which is a PRIVMSG to a channel I'm on,
 * into a struct message_data */
void get_chanmsg_data(char *msg, struct message_data *msgdata);

/* Takes action on this message */
void process_single_message(struct message_data *md);

/* Takes action on this PRIVMSG */
void process_privmsg(struct message_data *md);

/* Takes action on this CHANMSG */
void process_chanmsg(struct message_data *md);

/* Takes action when getting killed */
void process_killed(struct message_data *md);

void send_command_list(char* nick);


/* Sends a notice to all logged in users with notice flas
 * flag_required. */
void send_notice_to_loggedin(int flag_required, char *pattern, ...);

/* Sends a notice to all logged in users with notice flas
 * flag_required, except to user_p (who is probably the origin). */
void send_notice_to_loggedin_except(struct ocf_loggedin_user *user_p, 
									int flag_required, char *pattern, ...);

/* Sends a notice to all logged in users with notice flas
 * flag_required, except to user_p and user2_p. */
void send_notice_to_loggedin_except_two(struct ocf_loggedin_user *user_p, 
									struct ocf_loggedin_user *user2_p, 
									int flag_required, char *pattern, ...);

/* Checks all logged in users, and sends the message to each logged
 * in user with username <username>. This is usually just 1 nick, but
 * it can happen that a user has logged in with two clients. */
void send_notice_to_loggedin_username(const char* username, char *pattern, ...);

