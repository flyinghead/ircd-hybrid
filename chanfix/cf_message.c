/*
 * $Id: cf_message.c,v 1.10 2004/08/31 17:33:46 cfh7 REL_2_1_3 $
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

process_cmd_t process_cmd_functions[] = {
	process_cmd_unknown,
	process_cmd_help,
	process_cmd_score,
	process_cmd_chanfix,
	process_cmd_login,
	process_cmd_logout,
	process_cmd_passwd,
	process_cmd_history,
	process_cmd_info,
	process_cmd_oplist,
	process_cmd_addnote,
	process_cmd_delnote,
	process_cmd_whois,
	process_cmd_addflag,
	process_cmd_delflag,
	process_cmd_addhost,
	process_cmd_delhost,
	process_cmd_adduser,
	process_cmd_deluser,
	process_cmd_chpass,
	process_cmd_who,
	process_cmd_umode,
	process_cmd_status,
	process_cmd_set,
	process_cmd_block,
	process_cmd_unblock,
	process_cmd_rehash,
	process_cmd_check,
	process_cmd_opnicks,
	process_cmd_cscore,
	process_cmd_alert,
	process_cmd_unalert,
	process_cmd_addserver,
	process_cmd_delserver,
	process_cmd_chpasshash,
	process_cmd_whoserver,
	process_cmd_setmainserver,
};


/* ----------------------------------------------------------------- */

void send_notice_to_loggedin(int flag_required, char *pattern, ...)
{
	va_list args;
	char tmpBuf[IRCLINELEN];
	dlink_node *ptr, *next_ptr;
	struct ocf_loggedin_user *olu;
	int ver_user;

	va_start(args, pattern);
	vsprintf(tmpBuf, pattern, args);
	va_end(args);
	tmpBuf[IRCLINELEN - 1] = 0;

	DLINK_FOREACH_SAFE(ptr, next_ptr, ocf_loggedin_user_list.head) {
		olu = ((struct ocf_loggedin_user*)(ptr->data));
		if (flag_required == OCF_NOTICE_FLAGS_ALL ||
				ocf_user_has_notice_flag(olu->ocf_user_p, flag_required)) {
			/* Make sure the client that currently has nickname olu->nick
			 * is the same as the one that logged in. */
			ver_user = verify_loggedin_user(olu);
			if (ver_user == 1) {
				chanfix_send_privmsg(olu->nick, "%s", tmpBuf);
			} else if (ver_user == 0 ) {
				/* Suppress the message, but don't logout the user. */	
			} else {
				ocf_user_logout(olu->nick);
			}
		}
	}
}

/* ----------------------------------------------------------------- */

void send_notice_to_loggedin_except(struct ocf_loggedin_user *user_p, 
							 int flag_required, char *pattern, ...)
{
	va_list args;
	char tmpBuf[IRCLINELEN];
	dlink_node *ptr, *next_ptr;
	struct ocf_loggedin_user *olu;
	int ver_user;

	va_start(args, pattern);
	vsprintf(tmpBuf, pattern, args);
	va_end(args);
	tmpBuf[IRCLINELEN - 1] = 0;

	DLINK_FOREACH_SAFE(ptr, next_ptr, ocf_loggedin_user_list.head) {
		olu = ((struct ocf_loggedin_user*)(ptr->data));
		if ( (flag_required == OCF_NOTICE_FLAGS_ALL || 
				ocf_user_has_notice_flag(olu->ocf_user_p, flag_required) ) &&
				user_p != olu) {
			/* Make sure the client that currently has nickname olu->nick
			 * is the same as the one that logged in. */
			ver_user = verify_loggedin_user(olu);
			if (ver_user == 1) {
				chanfix_send_privmsg(olu->nick, "%s", tmpBuf);
			} else if (ver_user == 0 ) {
				/* Suppress the message, but don't logout the user. */	
			} else {
				ocf_user_logout(olu->nick);
			}
		}
	}
}

/* ----------------------------------------------------------------- */

void send_notice_to_loggedin_except_two(struct ocf_loggedin_user *user_p, 
							 struct ocf_loggedin_user *user2_p, 
							 int flag_required, char *pattern, ...)
{
	va_list args;
	char tmpBuf[IRCLINELEN];
	dlink_node *ptr, *next_ptr;
	struct ocf_loggedin_user *olu;
	int ver_user;

	va_start(args, pattern);
	vsprintf(tmpBuf, pattern, args);
	va_end(args);
	tmpBuf[IRCLINELEN - 1] = 0;

	DLINK_FOREACH_SAFE(ptr, next_ptr, ocf_loggedin_user_list.head) {
		olu = ((struct ocf_loggedin_user*)(ptr->data));
		if ( (flag_required == OCF_NOTICE_FLAGS_ALL || 
				ocf_user_has_notice_flag(olu->ocf_user_p, flag_required) ) &&
				user_p != olu && user2_p != olu) {
			/* Make sure the client that currently has nickname olu->nick
			 * is the same as the one that logged in. */
			ver_user = verify_loggedin_user(olu);
			if (ver_user == 1) {
				chanfix_send_privmsg(olu->nick, "%s", tmpBuf);
			} else if (ver_user == 0 ) {
				/* Suppress the message, but don't logout the user. */	
			} else {
				ocf_user_logout(olu->nick);
			}
		}
	}
}

/* ----------------------------------------------------------------- */

void send_notice_to_loggedin_username(const char* username, char *pattern, ...)
{
	va_list args;
	char tmpBuf[IRCLINELEN];
	dlink_node *ptr;
	struct ocf_loggedin_user *olu;
	int ver_user;

	va_start(args, pattern);
	vsprintf(tmpBuf, pattern, args);
	va_end(args);
	tmpBuf[IRCLINELEN - 1] = 0;

	DLINK_FOREACH(ptr, ocf_loggedin_user_list.head) {
		olu = ((struct ocf_loggedin_user*)(ptr->data));
		if (!strcmp(olu->ocf_user_p->name, username)) {
			/* Make sure the client that currently has nickname olu->nick
			 * is the same as the one that logged in. */
			ver_user = verify_loggedin_user(olu);
			if (ver_user == 1) {
				chanfix_send_privmsg(olu->nick, "%s", tmpBuf);
			} else if (ver_user == 0 ) {
				/* Suppress the message, but don't logout the user. */	
			} else {
				ocf_user_logout(olu->nick);
			}
		}
	}
}

/* ----------------------------------------------------------------- */

int process_received_messages(const char* readBuf, int length)
{
	char tmpBuf[READBUF_SIZE];
	struct message_data md;
	int begin = 0;
	int end = 0;
	int num_msgs = 0;

	/* Redundant safety measure: */
//	memset( &md, 0, sizeof( struct message_data ) );

	/* Split the incoming buffer into lines and process each line
	 * separately */
	while (end < length){
		if (readBuf[end] == '\n'){
			memcpy(tmpBuf, &readBuf[begin], end - begin);
			tmpBuf[end - begin - 1] = 0; /* there is \r\n at the end, which is 2 chars*/
			get_message_data(tmpBuf, &md);
			ocf_log(OCF_LOG_DEBUG, "prm: [%d] %s", end-begin-1, tmpBuf);
			process_single_message(&md);
			num_msgs++;
			begin = end + 1;
		}
		end++;
	}

	return num_msgs;
}


/* ----------------------------------------------------------------- */

void process_single_message(struct message_data *md)
{
	switch (md->type) {
	case MSG_UNKNOWN: break;
	case MSG_PING: 
		issue_chanfix_command("PONG :%s", md->server);
		break;
	case MSG_PRIVMSG: 
		process_privmsg(md);
		break;
	case MSG_CHANMSG: 
		process_chanmsg(md);
		break;
	case MSG_CTCP: 
		ocf_log(OCF_LOG_DEBUG, "Got a ctcp %s", md->data);
		break;
	case MSG_KILLED:
		process_killed(md);
		break;
	case MSG_RECON_TOO_FAST:
		ocf_log(OCF_LOG_DEBUG, "Trying to reconnect too fast.");
		chanfix = NULL;
		OCF_connect_chanfix_with_delay(OCF_CONNECT_DELAY * 10);
		break;
	case MSG_NUMERIC:
		ocf_log(OCF_LOG_DEBUG, "Got numeric %d.", md->type);
		break;
	}
}

/* ----------------------------------------------------------------- */

void get_message_data(char *msg, struct message_data *md)
{
	md->type = MSG_UNKNOWN;

	if (!strncmp( msg, "PING :", 6 )) {
		md->type = MSG_PING;
		strncpy( md->server, msg + 6, sizeof(md->server) );
		return;
	}

	/* If the first space is followed by 'PRIVMSG #', this is a
	 * PRIVMSG sent to a channel i'm on.
	 * :sabre2th!sabre2th@talking.yoda.is PRIVMSG #carnique :hi C
	 */
	if (strstr(msg, " ") == strstr(msg, " PRIVMSG #")) {
		get_chanmsg_data(msg, md);
		return;
	}
	
	/* If the first space is followed by 'PRIVMSG', this is a
	 * PRIVMSG sent to me. Syntax:
	 * :Garion!garion@chan.fix PRIVMSG C :chanfix #test
	 */
	if (strstr(msg, " ") == strstr(msg, " PRIVMSG ")) {
		get_privmsg_data(msg, md);
		return;
	}

	if (!strncmp(msg, "ERROR :Closing Link:", 20)) {
		md->type = MSG_KILLED;
	}

	if (!strncmp(msg, "ERROR :Trying to reconnect too fast", 35)) {
		md->type = MSG_RECON_TOO_FAST;
	}
	
	if (strstr(msg, " ") == strstr(msg, " 001 ")) {
		md->type = MSG_NUMERIC;
		md->type = 1;
	}
}

/* ----------------------------------------------------------------- */
/*
 *  A safer parser, returns 0 if incomplete/unreliable details were
 *  encountered, 1 if nick only, 3 if nick, user and host were
 *  successfully found.
 */
int parse_userhost_details( char *msg, struct message_data *md )
{
	int c;
	char* m[3] = { md->nick, md->user, md->host };
	int ml[3] = { NICKLEN, USERLEN, HOSTLEN };
	int mc[3] = { 0, 0, 0 };
	char* f;

	/* This check may seem a bit anal, but Mr Murphy loves it! */
	if ( !msg || !md ) return 0;

	for ( c = 0, f = msg; 1; f++ )
	{
		switch ( *f )
		{
			case ':':
			{
				/* Ignore colons */
				break;
			}
			case 0:
			case '\r':
			case '\n':
			{
				/* Something fishy with this string, let's disqualify */
				return 0;
			}
			case '!':
			{
				/* If we ran into the ! while filling the nick element,
				 * we'll move to user
				 */
				if ( c == 0 )
				{
					*m[0] = 0;
					c = 1;
				}
				else
				{
					/* This is unexpected, let's disqualify the whole string */
					return 0;
				}
				break;
			}
			case '@':
			{
				/* If we ran into the @ while filling the user element,
				 * we'll move to host.
				 */
				if ( c == 1 )
				{
					*m[1] = 0;
					c = 2;
				}
				else
				{
					/* This is unexpected, let's disqualify the whole string */
					return 0;
				}
				break;
			}
//			case 0:	/* Uncomment this to make parser more generic */
			case ' ':
			{
				/* Space at the end terminates parse */
				if ( mc[0] )
				{
 					m[c] = 0;
					return c + 1;
				}
				break;
			}
			default:
			{
				/* Let's make sure we're not about to overflow any structures,
				 * in case a compromised ircd sends us bad data.
				 */
				if ( mc[c] >= ml[c] )
				{
					/* Uh oh, going to overflow.. Let's disqualify the whole
					 * parse.
					 */
					*m[c] = 0;
					return 0;
				}

				/* By default, we just fill the active buffer */
				*m[c] = *f;
				m[c]++;
				mc[c]++;
			}
		}
	}
	/* Should never reach this point, but this will shut up compiler warnings */
	return 0;
}

/* ----------------------------------------------------------------- */
/*
 * :Garion!garion@chan.fix PRIVMSG C :chanfix #test
 * or a CTCP which has \001 as first char.
 */
void get_privmsg_data(char *msg, struct message_data *md)
{
	char *cptr;
	struct Client *clptr;
	int uh_parse_results;
	int data_parse_state;
	int i, ctcp_valid;

	int id_name_idx;
	int id_server_idx;
	int id_isnick;
	char id_name[USERLEN + NICKLEN + 1];
	char id_server[HOSTLEN + 1];


	memset( md, 0, sizeof( struct message_data ) );
	md->type = MSG_PRIVMSG;

	uh_parse_results = parse_userhost_details( msg, md );

	if ( uh_parse_results == 1 )
	{
		/* We only got a nickname, let's poll for the user and host */
		clptr = find_client( md->nick );
		if ( clptr )
		{
			/* Note: strncpy does not automatically append a zero at the end
			 * if the length of the src is bigger than the available buffer.
			 * However, since we zeroed this memory structure beforehand, this
			 * SHOULD be safe.
			 */
			strncpy( md->user, clptr->username, USERLEN );
			strncpy( md->host, clptr->host, HOSTLEN );
			strncpy( md->server, clptr->servptr->name, HOSTLEN );
			ocf_log(OCF_LOG_DEBUG, "Got a nick-only message. U@H is %s@%s, server is %s", md->user, md->host, md->server);
		}
		else
		{
			/* We received a message from a nickname which does not exist
			 * in our records.. Fishy indeed. Let's ignore the message.
			 */
			ocf_log(OCF_LOG_DEBUG, "I received a message from non-existant nickname: '%s' (ignored)", msg);
			memset( md, 0, sizeof( struct message_data ) );
			md->type = MSG_UNKNOWN;
			return;
		}
	}
	else
	if ( uh_parse_results == 3 )
	{
		/* We got the nick, user and host, let's validate them to be sure */
		clptr = find_client( md->nick );
		if ( clptr )
		{
			if ( strcasecmp( md->user, clptr->username ) ||
				 strcasecmp( md->host, clptr->host ) )
			{
				/* The username or hostname passed by the PRIVMSG differs from
				 * the local record. Should this ever happen? Let's ignore for now.
				 */
				ocf_log(OCF_LOG_DEBUG, "PRIVMSG user@host mismatches my own record (%s@%s) for nickname: '%s' (ignored)", clptr->username, clptr->host, msg);
				memset( md, 0, sizeof( struct message_data ) );
				md->type = MSG_UNKNOWN;
				return;
			}
			strncpy( md->server, clptr->servptr->name, HOSTLEN );
		}
		else
		{
			/* We received a message from a nickname which does not exist
			 * in our records.. Fishy indeed. Let's ignore the message.
			 */
			ocf_log(OCF_LOG_DEBUG, "I received a message from non-existant nickname: '%s' (ignored)", msg);
			memset( md, 0, sizeof( struct message_data ) );
			md->type = MSG_UNKNOWN;
			return;
		}
	}
	else
	{
		/* Malformed message encountered, let's abort */
		ocf_log(OCF_LOG_DEBUG, "I received a message with malformed nick!user@host: '%s' (aborted parse)", msg);
		memset( md, 0, sizeof( struct message_data ) );
		md->type = MSG_UNKNOWN;
		return;
	}

	/* Now to find what was messaged to me */
	cptr = strstr(msg, " PRIVMSG ");

	if ( !cptr )
	{
		/* Not expected, we'll abort */
		memset( md, 0, sizeof( struct message_data ) );
		md->type = MSG_UNKNOWN;
		return;
	}

	/* Walk 1 + 7 + 1 chars */
	cptr += 9;

	/* Sweep past the recipient field.
	 * Here we will verify that the recipient field is what can reasonably
 	 * be expected. We will also check if this message was sent to 
	 * user@server (safe) rather than just nickname (unsafe). Any masks
	 * will be ignored, and server-wide message recipient processing
	 * aborted altogether.
	 */
	data_parse_state = 0;
	id_name_idx = 0;
	id_server_idx = 0;
	id_isnick = 1;

	memset( id_name, 0, USERLEN + NICKLEN + 1 );
	memset( id_server, 0, HOSTLEN + 1 );

	while ( 1 )
	{
		if ( !*cptr )
		{
			/* Should not null-terminate here, abort */
			memset( md, 0, sizeof( struct message_data ) );
			md->type = MSG_UNKNOWN;
			return;
		}
		else
		if ( *cptr == ' ' )
		{
			switch ( data_parse_state )
			{
				default:
				case 0:
				case 4:
				{
					/* Whitespace before or after recipient, ignore */
					break;
				}
				case 1:
				case 2:
				case 3:
				{
					/* First whitespace after recipient, shift state */
					data_parse_state = 4;

					id_name[id_name_idx] = 0;
					id_server[id_server_idx] = 0;
					break;
				}
			}
		}
		else
		{
			/* Auto-invalidate the message if we encounter unexpected chars */
			if ( *cptr == '$' || *cptr == '!' || *cptr == '\n' )
			{
				memset( md, 0, sizeof( struct message_data ) );
				md->type = MSG_UNKNOWN;
				return;
			}

			if ( data_parse_state == 0 )
			{
				/* Receipient data begins */
				data_parse_state = 1;
			}

			if ( data_parse_state == 1 )
			{
				/* Recipient data continues */
				if ( *cptr == '@' )
				{
					id_isnick = 0;
					id_name[id_name_idx] = 0;
					data_parse_state = 3;
				}
				else
				if ( *cptr == '%' )
				{
					/* There's a mask coming up */
					id_isnick = 0;
					id_name[id_name_idx] = 0;
					data_parse_state = 2;
				}
				else
				{
					id_name[id_name_idx] = *cptr;
					id_name_idx++;
					if ( id_name_idx > ( USERLEN + NICKLEN ) )
					{
						/* Unexpected, let's ignore (we don't care if it goes past
						 * NICKLEN, as we'll only use this string for a string compare)
						 */
						memset( md, 0, sizeof( struct message_data ) );
						md->type = MSG_UNKNOWN;
						return;
					}
				}
			}
			else
			if ( data_parse_state == 2 )
			{
				/* Recipient mask continues (ignored) */
				if ( *cptr == '@' )
				{
					data_parse_state = 3;
				}
			}
			else
			if ( data_parse_state == 3 )
			{
				/* Recipient server continues */
				id_server[id_server_idx] = *cptr;
				id_server_idx++;
				if ( id_server_idx > HOSTLEN )
				{
					/* Unexpected, let's ignore */
					memset( md, 0, sizeof( struct message_data ) );
					md->type = MSG_UNKNOWN;
					return;
				}
			}
			else
			{
				/* Next data field, let's stop processing */
				break;
			}
		}
		cptr++;
	}

	if ( !id_isnick )
	{
		if ( strcasecmp( id_name, chanfix->username ) || strcasecmp( id_server, me.name ) )
		{
			/* Unexpected data in recipient field, disregard.. */
			memset( md, 0, sizeof( struct message_data ) );
			md->type = MSG_UNKNOWN;
			return;
		}
		md->safe = 1;
	}
	else
	{
		if ( irccmp( id_name, chanfix->name ) )
		{
			/* Unexpected data in recipient field, disregard.. */
			memset( md, 0, sizeof( struct message_data ) );
			md->type = MSG_UNKNOWN;
			return;
		}
		md->safe = 0;
	}



	/* Ignore possible colon character */
	if ( *cptr == ':' )
	{
		cptr++;
	}

	/* A CTCP message */
	if ( *cptr == 1 )
	{
		md->type = MSG_CTCP;
		cptr++;
	}

	/* Copy everything from cptr to the end of msg to md->data.
	 * msg is null-terminated, so strncpy should suffice. */
	strncpy( md->data, cptr, IRCLINELEN );

	if ( md->type == MSG_CTCP )
	{
		ctcp_valid = 0;

		/* If CTCP, force termination at next char 001 */
		for ( i = 1; i < strlen( md->data ); i++ )
		{
			if ( md->data[i] == 1 )
			{
				ctcp_valid = 1;
				md->data[i] = 0;
				break;
			}
		}

		if ( !ctcp_valid )
		{
			/* This is a malformed CTCP. Let's just ignore it */
			memset( md, 0, sizeof( struct message_data ) );
			md->type = MSG_UNKNOWN;
			return;
		}
	}
}

/* ----------------------------------------------------------------- */
/*
 * :sabre2th!sabre2th@talking.yoda.is PRIVMSG #carnique :hi C
 */
void get_chanmsg_data(char *msg, struct message_data *md)
{
	/* A channel message? Uninteresting. Let's not bother parsing it */
	memset( md, 0, sizeof( struct message_data ) );
	md->type = MSG_CHANMSG;
	return;
}



/* ----------------------------------------------------------------- */

void process_chanmsg(struct message_data *md)
{
	/* Don't do anything with channel messages. */
}

/* ----------------------------------------------------------------- */

void process_killed(struct message_data *md)
{
	ocf_log(OCF_LOG_DEBUG, "Disconnected. Reconnecting.");
	ocf_log(OCF_LOG_MAIN, "DISC Disconnected. Reconnecting.");
	chanfix = NULL;
	OCF_connect_chanfix(NULL);
	/* We should actually be reconnecting with a delay to prevent
	 * throttling. But calling the delay function, as below, somehow
	 * doesn't work properly.
	 * TODO: fix delay reconnection.
	OCF_connect_chanfix_with_delay(OCF_CONNECT_DELAY);
	*/
}

/* ----------------------------------------------------------------- */

void process_privmsg(struct message_data *md)
{
	struct Client *sender = find_person(md->nick);
	struct ocf_loggedin_user *olu;
	struct command_data cd;
	process_cmd_t funcp;
	int required_flags;
	char hostmask[USERLEN + 1 + HOSTLEN + 1];

	memset(&cd, 0, sizeof(cd));

	/* Check if the sender is still on the network. */
	if (!sender) {
		/* Send error to owner(s) about non-existing nick */
		ocf_log(OCF_LOG_DEBUG, "pr_pr: nick %s does not exist.", md->nick);
		return;
	}

	/* Only accept commands from opers. */
	if (!IsOper(sender)) {
		/* Send error to owner(s) about non-oper command */
		ocf_log(OCF_LOG_DEBUG, "pr_pr: non-oper %s sent me %s", md->nick, md->data);
		ocf_log(OCF_LOG_PRIVMSG, "PRIV non-oper %s (%s@%s) [%s]: %s",
			md->nick, md->user, md->host, md->server, md->data);
		return;
	}

	snprintf(hostmask, sizeof(hostmask), "%s@%s", md->user, md->host);
    if (ocf_is_ignored(hostmask, md->server)) {
		ocf_log(OCF_LOG_PRIVMSG, "IGNO Ignoring message from oper %s (%s@%s) [%s]: %s",
			md->nick, md->user, md->host, md->server, md->data);
		return;
    }

	ocf_log(OCF_LOG_DEBUG, "Got privmsg from oper %s: %s", md->nick, md->data);
	ocf_log(OCF_LOG_PRIVMSG, "PRIV oper %s (%s@%s) [%s]: %s",
		md->nick, md->user, md->host, md->server, md->data);
	olu = ocf_get_loggedin_user(md->nick);

	/* Check if the user@host of the nickname that sent us this message is the
	 * same as the user@host of the logged in user. If not, log out that nickname`
	 * to avoid hijacking of logged in sessions. */
	if (olu != NULL) {
		if (strcasecmp(md->user, olu->user) != 0 || 
			strcasecmp(md->host, olu->host) != 0 ||
			strcasecmp(md->server, olu->server) != 0) {
			ocf_log(OCF_LOG_MAIN, 
				"WARN Possible session hijack for %s: logged in with %s@%s [%s], now %s@%s [%s].",
				md->nick, olu->user, olu->host, olu->server, md->user, md->host, md->server);
			send_notice_to_loggedin_except(olu, OCF_NOTICE_FLAGS_MAIN, 
				"Possible session hijack for %s: logged in with %s@%s [%s], now %s@%s [%s].",
				md->nick, olu->user, olu->host, olu->server, md->user, md->host, md->server);
			ocf_user_logout(md->nick);
			olu = NULL;
		}
	}

	cd.loggedin_user_p = olu;
	get_command_data(md->data, &cd);

	/* Check whether the user is logged in. */
	if (olu == NULL && command_requires_loggedin(cd.command)) {
		chanfix_send_privmsg(md->nick, "You need to login to use this command.");
		return;
	}

	/* Check whether the user has the necessary flags. */
	required_flags = cmd_flags_required[cd.command];
	if (olu && required_flags && !ocf_user_has_flag(olu->ocf_user_p, required_flags)) {
		if (ocf_user_flag_get_char(required_flags) != ' ') {
			chanfix_send_privmsg(md->nick, "This command requires flag %c.", 
				ocf_user_flag_get_char(required_flags));
		} else {
			chanfix_send_privmsg(md->nick, "This command requires one of these flags: %s.", 
				ocf_user_flags_to_string(required_flags));
		}
		return;
	}

	/* Check whether the syntax of the command is correct. */
	if (cd.wrong_syntax) {
		ocf_log(OCF_LOG_DEBUG, "Wrong syntax.");
		process_cmd_wrong_syntax(md, &cd);
		return;
	}

	/* Get the function pointer to the function to process this command. */
	funcp = process_cmd_functions[cd.command];
	if (funcp == NULL) {
		ocf_log(OCF_LOG_DEBUG, "Eeks! NULL function pointer in process_privmsg!");
		chanfix_send_privmsg(md->nick,
			"ERROR - NULL function pointer in process_privmsg for cmd %d. Please report to ocf@garion.org", cd.command);
		return;
	}
	funcp(md, &cd);
}

/* ----------------------------------------------------------------- */

void send_command_list(char* nick) {
	char cmds[IRCLINELEN];
	int i;
	struct ocf_loggedin_user *olu;
	cmds[0] = 0;
	olu = ocf_get_loggedin_user(nick);

	for (i = 1; i <= NUM_COMMANDS; i++) {
		/* Only send help for commands that require loggedin if the 
		 * user is actually logged in. */
		if (command_requires_loggedin(i)) {
			if (olu) {
				strcat(cmds, cmd_word[i]);
				if (i != NUM_COMMANDS) {
					strcat(cmds, " ");
				}
			}
		} else {
			strcat(cmds, cmd_word[i]);
			if (i != NUM_COMMANDS) {
				strcat(cmds, " ");
			}
		}
	}
	chanfix_send_privmsg(nick, "List of commands:");
	chanfix_send_privmsg(nick, "%s", cmds);
}

