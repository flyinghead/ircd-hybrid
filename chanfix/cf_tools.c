/*
 * $Id: cf_tools.c,v 1.13 2004/10/30 19:50:53 cfh7 REL_2_1_3 $
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

void issue_chanfix_command(const char *pattern, ...)
{
	va_list args;
	char tmpBuf[1024];
	if (chanfix_remote) {
		va_start(args, pattern);
		vsprintf(tmpBuf, pattern, args);
		va_end(args);
		ocf_log(OCF_LOG_DEBUG, "issuecmd: [%s]", tmpBuf);
		sendto_one(chanfix_remote, "%s", tmpBuf);
		// XXXXXX
		//send_queued_write(chanfix_remote->localClient->fd, chanfix_remote);
	}
}

/* ----------------------------------------------------------------- */

void chanfix_send_privmsg(char* target, const char *pattern, ...)
{
	va_list args;
	char tmpBuf1[1024];
	char tmpBuf2[1024];
	struct Client *clptr;

	if (!chanfix_remote || !target) {
		return;
	}

	if (target[0] == '#') {
		/* Message to a channel */
		va_start(args, pattern);
		vsprintf(tmpBuf1, pattern, args);
		va_end(args);
		/* Always send PRIVMSGs to a channel, not NOTICEs. */
		snprintf(tmpBuf2, sizeof(tmpBuf2), "PRIVMSG %s :%s", target, tmpBuf1);
		issue_chanfix_command("%s", tmpBuf2);
	} else {
		/* Message to a client */
		clptr = find_client(target);
		if (!clptr) {
			return;
		}
		va_start(args, pattern);
		vsprintf(tmpBuf1, pattern, args);
		va_end(args);
		if (send_notices_instead_of_privmsgs) {
			/* NOTICE syntax depends on the target client being local */
			if (clptr->localClient) {
				sendto_one(clptr, ":%s!%s@%s NOTICE %s :%s", chanfix->name,
					chanfix->username, chanfix->host,
					clptr->name, tmpBuf1);
			} else {
				sendto_one(clptr, ":%s NOTICE %s :%s", chanfix->name,
					clptr->name, tmpBuf1);
			}
		} else {
			sendto_one(clptr, ":%s PRIVMSG %s :%s", chanfix->name,
                clptr->name, tmpBuf1);
		}
	}
}

/* ----------------------------------------------------------------- */

void chanfix_perform_mode_group(char* channel, char plusminus, char mode, 
			char** args, int num_args)
{
	int i, j, num_modes_per_command;

	num_modes_per_command = settings_get_int("max_channel_modes");
	if (num_modes_per_command < 1) {
		num_modes_per_command = MAX_CHANNEL_MODES;
	}

	i = 0;
    /* Do as many modes as we can per line */
    while ( i < num_args ) {
		j = chanfix_perform_mode_num(channel, plusminus, mode,
                    args + i, ( num_args - i ) < num_modes_per_command ? 
		( num_args - i ) : num_modes_per_command );

		if ( !j )
		{
			/* For some reason, we could not set this mode
			 * so we'll just skip it altogether.
			 */
			i++;
		}
		else
		{
	        i += j;
		}
    }
}

/* ----------------------------------------------------------------- */

int chanfix_perform_mode_num(char* channel, char plusminus, char mode,
                        char** args, int number)
{
    int i;
    char cmd[IRCLINELEN + 1];
	int actualnumber;
	int linesize;

	/* "MODE #channel - " = 8 chars + channel length. */
	actualnumber = 0;
	linesize = 8 + strlen( channel );

	/* Here we will estimate how many commands we can fit into
	 * a single IRCLINELEN buffer.
 	 */
	while ( actualnumber < number && 
		( linesize + 2 + strlen( args[actualnumber] ) < 
		IRCLINELEN ) )
	{
		linesize += 2 + strlen( args[actualnumber] );
		actualnumber++;
	}

	if ( !actualnumber )
	{
		/* This should never happen, but we like
		 * old Mr Murphy and his laws!
		 */
		return 0;
	}

	/* Here we will fit the modes accordingly */
        snprintf(cmd, sizeof(cmd), "MODE %s %c", channel, plusminus);
	linesize = strlen( cmd );
    for (i = 0; i < actualnumber; i++) {
        cmd[linesize++] = mode;
    }
	cmd[linesize++] = ' ';
	cmd[linesize] = 0;

    for (i = 0; i < actualnumber; i++) {
        strcat(cmd, args[i]);
        strcat(cmd, " ");
    }
    issue_chanfix_command("%s", cmd);

	return actualnumber;
}


/* ----------------------------------------------------------------- */

void chanfix_remove_channel_modes(struct Channel *chptr)
{
	char** banhostmasks;
	dlink_node *node;
	int num_bans, i;
	struct Ban *ban_p;

	/* Remove invite-only. */
	if (chptr->mode.mode & MODE_INVITEONLY) {
		issue_chanfix_command("MODE %s -i", chptr->chname);
	}

	/* Remove key. */
	if (chptr->mode.key[0] != '\0') {
		issue_chanfix_command("MODE %s -k", chptr->chname);
	}

	/* Remove max user limit. */
	if (chptr->mode.limit != 0) {
		issue_chanfix_command("MODE %s -l", chptr->chname);
	}

	/* Done if there are no bans. */
	num_bans = dlink_list_length(&chptr->banlist);
	if (num_bans == 0) {
		return;
	}

	/* Allocate enough space for all bans. */
	banhostmasks = (char**)MyMalloc(num_bans * sizeof(char*));
	for (i = 0; i < num_bans; i++) {
		banhostmasks[i] = (char*)MyMalloc((NICKLEN + USERLEN + HOSTLEN + 6) * sizeof(char));
	}
	
	/* Copy all banned hostmasks into our own array of hostmasks. */
	i = 0;
	DLINK_FOREACH(node, chptr->banlist.head) {
		ban_p = (struct Ban*)(node->data);
		strncpy(banhostmasks[i], ban_p->banstr, 
			(NICKLEN + USERLEN + HOSTLEN + 6) * sizeof(char));
		i++;
	}
	
	/* Remove the bans. */
	chanfix_perform_mode_group(chptr->chname, '-', 'b', 
			banhostmasks, num_bans);

	for (i = 0; i < num_bans; i++)
		MyFree(banhostmasks[i]);

	MyFree(banhostmasks);
	return;
}

/* ----------------------------------------------------------------- */

void strtrim(char *string)
{
	int len = strlen(string);
	int count;

	if (len == 0) {
		return;
	}

	/* Remove whitespace at the end */
	count = len - 1;
	while (isspace(string[count]) && count > 0) {
		string[count] = 0;
		count--;
	}

	/* All-whitespace string */
	if (count == 0) {
		return;
	}

	/* Remove whitespace at the beginning */
	count = 0;
	while (isspace(string[count])) {
		count++;
	}

	if (count > 0) {
		memmove(string, string + count, len - count);
		string[len - count] = 0;
	}
}

/* ----------------------------------------------------------------- */

int readline(FILE * fp, char* line, int maxlen)
{
	int count = 0;
	char c;

	while ((c = fgetc(fp)) != EOF) {
		if (c == '\n')
		{
			line[count] = 0;
			return 1;
		}

		line[count] = c;
		count++;

		if (count >= maxlen - 1) 
		{
			line[maxlen - 1] = 0;
			return 1;
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int writeline(FILE * fp, char* pattern, ...)
{
	va_list args;
	char tmpBuf[1024];

	va_start(args, pattern);
	vsprintf(tmpBuf, pattern, args);
	va_end(args);
	tmpBuf[1023] = 0;

	fputs(tmpBuf, fp);
	fputc('\n', fp);
	return 1;
}

/* ----------------------------------------------------------------- */
/* Walks through array userhosts, which currently has num_userhosts
 * userhosts in it, and checks whether <user>@<host> is already
 * in there. 
 * If so, nothing is changed, and num_userhosts is returned.
 * If not, <user>@<host> is added at the end, and num_userhosts + 1
 * is returned.
 * NOTE: the userhost is copied and tolower()ed.
 */
int add_userhost_to_array(char** users, char **hosts, int num_userhosts, 
						  char* user, char* host)
{
	int i;

	/* Check if the userhost is already present. 
	 * This check is case insensitive since user@hosts are case
	 * insensitive. */
	for (i = 0; i < num_userhosts; i++) {
		if (!strcasecmp(users[i], user) && !strcasecmp(hosts[i], host)) {
			/* userhost already present. Abort and return. */
			return num_userhosts;
		}
	}

	/* Userhost is not yet in the array. Add. */
	users[num_userhosts] = user;
	hosts[num_userhosts] = host;

	return num_userhosts + 1;
}

/* ----------------------------------------------------------------- */

int get_number_of_servers_linked() 
{	
	return dlink_list_length(&global_serv_list);
}

/* ----------------------------------------------------------------- */

int check_minimum_servers_linked()
{
	int num_linked = get_number_of_servers_linked();
	
	if (100 * num_linked >= 
			settings_get_int("min_servers_present") * 
			settings_get_int("num_servers")) {
		return 1;
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int tokenize_string(char *string, char *parv[OCF_MAXPARA])
{
	return tokenize_string_separator(string, parv, ' ');
}

/* ----------------------------------------------------------------- */

int tokenize_string_separator(char *string, char *parv[OCF_MAXPARA], char separator)
{
	char *p;
	char *buf = string;
	int x = 0;

	parv[x] = NULL;

	while (*buf == separator) /* skip leading spaces */
		buf++;

	if (*buf == '\0') /* ignore all-separator args */
		return x;

	do
	{
		parv[x++] = buf;
		parv[x]   = NULL;

		if ((p = strchr(buf, separator)) != NULL)
		{
			*p++ = '\0';
			buf  = p;
		}
		else
		{
			return(x);
		}

		/* Treat multiple separators as one */
		while (*buf == separator)
			buf++;

		if (*buf == '\0')
			return x;
	} while (x < OCF_MAXPARA - 1);

	parv[x++] = p;
	parv[x]   = NULL;
	return x;
}

/* ----------------------------------------------------------------- */

int get_seconds_in_current_day()
{
	time_t tt;
	struct tm *t;
	tt = time(NULL);
	t = localtime(&tt);
	return t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec;
}

/* ----------------------------------------------------------------- */

int get_days_since_epoch()
{
	return time(NULL) / 86400;
}

/* ----------------------------------------------------------------- */

char* seconds_to_datestring(time_t seconds)
{
	static char datetimestring[OCF_TIMESTAMP_LEN];
	struct tm *stm;

	stm = localtime(&seconds);
	memset(datetimestring, 0, OCF_TIMESTAMP_LEN);
	strftime(datetimestring, OCF_TIMESTAMP_LEN, "%Y-%m-%d %H:%M:%S", stm);

	return datetimestring;
}

/* ----------------------------------------------------------------- */

char* get_logged_in_user_string(struct ocf_loggedin_user *olu)
{
#define USER_STRING_LENGTH 80
	static char user_string[USER_STRING_LENGTH];
	memset(user_string, 0, USER_STRING_LENGTH);

	snprintf(user_string, USER_STRING_LENGTH, "%s (%s@%s) [%s]", 
		olu->nick, olu->user, olu->host,
		olu->ocf_user_p->name);

	return user_string;
}

/* ----------------------------------------------------------------- */

