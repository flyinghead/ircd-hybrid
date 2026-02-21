/*
 * $Id: cf_user.c,v 1.15 2004/10/30 19:50:53 cfh7 REL_2_1_3 $
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

static BlockHeap *ocf_user_heap  = NULL;
static BlockHeap *ocf_loggedin_user_heap  = NULL;
static char saltchars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ./\0";

/* ----------------------------------------------------------------- */

void ocf_users_init()
{
	ocf_user_heap = BlockHeapCreate(sizeof(struct ocf_user), OCF_USER_HEAP_SIZE);
	ocf_loggedin_user_heap = BlockHeapCreate(sizeof(struct ocf_loggedin_user), OCF_LOGGEDIN_USER_HEAP_SIZE);
	
}

/* ----------------------------------------------------------------- */

void ocf_users_deinit()
{
	dlink_node *ptr, *next_ptr;
	DLINK_FOREACH_SAFE(ptr, next_ptr, ocf_user_list.head)
	{
		BlockHeapFree(ocf_user_heap, ptr->data);
		dlinkDelete(ptr, &ocf_user_list);
	}
	DLINK_FOREACH_SAFE(ptr, next_ptr, ocf_loggedin_user_list.head)
	{
		BlockHeapFree(ocf_loggedin_user_heap, ptr->data);
		dlinkDelete(ptr, &ocf_loggedin_user_list);
	}
}

/* ----------------------------------------------------------------- */

int ocf_users_get_unique_id()
{
	dlink_node *ptr;
	struct ocf_user *ou;
	int max = 0;

	DLINK_FOREACH(ptr, ocf_user_list.head) {
		ou = ((struct ocf_user*)(ptr->data));
		if (ou->id > max) {
			max = ou->id;
		}
	}

	return (max + 1);
}

/* ----------------------------------------------------------------- */

void ocf_users_updated()
{
	ocf_user_save_all(settings_get_str("userfile"));
}

/* ----------------------------------------------------------------- */

struct ocf_user* ocf_add_user(const char* name, const char* hostmask)
{
	struct ocf_user *user_p;

	if (ocf_get_user(name)) {
		/* User already exists */
		return NULL;
	}


	user_p = BlockHeapAlloc(ocf_user_heap);
	memset(user_p, 0, sizeof(struct ocf_user));
	strncpy(user_p->name, name, sizeof(user_p->name) - 1);
	strncpy(user_p->hostmasks, hostmask, sizeof(user_p->hostmasks) - 1);
	user_p->id = ocf_users_get_unique_id();
	dlinkAdd(user_p, make_dlink_node(), &ocf_user_list);

	return user_p;
}

/* ----------------------------------------------------------------- */

struct ocf_user* ocf_add_user_from_obj(struct ocf_user* other)
{
	struct ocf_user *user_p;
	int i = 0;
	char temp_servers[OCF_USER_SERVERS_LENGTH + 1];

	if (ocf_get_user(other->name)) {
		/* User already exists */
		return NULL;
	}

	
	user_p = BlockHeapAlloc(ocf_user_heap);
	memcpy(user_p, other, sizeof(struct ocf_user));
	if (other->id < 1) {
		user_p->id = ocf_users_get_unique_id();
	}
	dlinkAdd(user_p, make_dlink_node(), &ocf_user_list);

	/* We need to make sure that each user has a main server. If the
	 * main server has not been set, we'll take the first server in
	 * their server list. */

	if (!user_p->main_server[0]) {
		if (user_p->servers){
			/* Copy the first server into main_server */
			while (user_p->servers[i] && user_p->servers[i] != ',') {
				user_p->main_server[i] = user_p->servers[i];
				i++;
			}
			user_p->main_server[i] = 0;
			
			/* Now remove the host from ->servers */
			if (strlen(user_p->servers) == strlen(user_p->main_server)) {
				user_p->servers[0] = 0;
			} else {
				strncpy(temp_servers, user_p->servers + i + 1, sizeof(temp_servers));
				strncpy(user_p->servers, temp_servers, sizeof(user_p->servers));
			}
			ocf_log(OCF_LOG_USERS, "USFL fixing main server of %s to %s.", 
				user_p->name, user_p->main_server);
		} else {
			ocf_log(OCF_LOG_USERS, "USFL Warning: no servers for user %s.", 
			user_p->name);
		}
	}

	return user_p;
}

/* ----------------------------------------------------------------- */

void ocf_user_set_password(struct ocf_user* ocf_user_p, const char* password)
{
	char salt[3];
	salt[0] = saltchars[ (int) (random() % strlen(saltchars)) ];
	salt[1] = saltchars[ (int) (random() % strlen(saltchars)) ];
	salt[2] = 0;

	strncpy(ocf_user_p->password, crypt(password, salt), sizeof(ocf_user_p->password));

	/* Just to be safe */
 	ocf_user_p->password[sizeof(ocf_user_p->password)-1] = 0;
}

/* ----------------------------------------------------------------- */

void ocf_user_set_passwordhash(struct ocf_user* ocf_user_p, const char* hash)
{
	strncpy(ocf_user_p->password, hash, sizeof(ocf_user_p->password));

	/* Just to be safe */
 	ocf_user_p->password[sizeof(ocf_user_p->password)-1] = 0;
}

/* ----------------------------------------------------------------- */

int ocf_user_check_password(struct ocf_user* ocf_user_p, const char* password)
{
	char salt[3];
	char* passwordhash;

	salt[0] = ocf_user_p->password[0];
	salt[1] = ocf_user_p->password[1];
	salt[2] = 0;

	passwordhash = crypt(password, salt);
	if (!strncmp(passwordhash, ocf_user_p->password, sizeof(ocf_user_p->password)))
	{
		return 1;
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int ocf_user_add_hostmask(struct ocf_user* ocf_user_p, const char* hostmask)
{
	/* Don't add the hostmask if that will exceed the maximum length for
	 * hostmasks. */
	if (strlen(ocf_user_p->hostmasks) + strlen(hostmask) + 1 > OCF_USER_HOSTS_LENGTH) {
		return 0;
	}

	/* Don't add the hostmask if it's already present */
	if (ocf_user_has_hostmask(ocf_user_p, hostmask)) {
		return 0;
	}

	if (strlen(ocf_user_p->hostmasks) == 0) {
		strncpy(ocf_user_p->hostmasks, hostmask, sizeof(ocf_user_p->hostmasks));
		ocf_user_p->hostmasks[sizeof(ocf_user_p->hostmasks)-1] = 0;
		return 1;
	}

	strcat(ocf_user_p->hostmasks, ",");
	strcat(ocf_user_p->hostmasks, hostmask);
	return 1;
}

/* ----------------------------------------------------------------- */

int ocf_user_del_hostmask(struct ocf_user* ocf_user_p, const char* hostmask)
{
	char *parv[OCF_MAXPARA];
	int i, parc;
	char masks[OCF_USER_HOSTS_LENGTH];

	if (!ocf_user_has_hostmask(ocf_user_p, hostmask)){
		return 0;
	}

	/* tokenize destroys the original string, so tokenize a copy */
	strncpy(masks, ocf_user_p->hostmasks, sizeof(masks));
	masks[sizeof(masks)-1] = 0;
	parc = tokenize_string_separator(masks, parv, ',');

	/* Re-add all hostmasks except the one that needs to be removed */
	ocf_user_p->hostmasks[0] = 0;
	for (i = 0; i < parc; i++) {
		if (strlen(hostmask) == strlen(parv[i])){
			if ( !strncmp(hostmask, parv[i], strlen(hostmask)) ) {
				/* don't re-add this host */
			} else {
				ocf_user_add_hostmask(ocf_user_p, parv[i]);
			}
		} else {
			ocf_user_add_hostmask(ocf_user_p, parv[i]);
		}
	}

	return 1;
}

/* ----------------------------------------------------------------- */

int ocf_user_has_hostmask(struct ocf_user* ocf_user_p, const char* hostmask)
{
	char *parv[OCF_MAXPARA];
	int i, parc;
	char masks[OCF_USER_HOSTS_LENGTH];
	
	if (strlen(ocf_user_p->hostmasks) == 0) {
		return 0;
	}

	/* tokenize destroys the original string, so tokenize a copy */
	strncpy(masks, ocf_user_p->hostmasks, sizeof(masks));
	masks[sizeof(masks)-1] = 0;
	parc = tokenize_string_separator(masks, parv, ',');

	for (i = 0; i < parc; i++) {
		/* TODO: wildcard host matching? */
		if (strlen(hostmask) == strlen(parv[i])){
			if ( !strncmp(hostmask, parv[i], strlen(hostmask)) ) {
				return 1;
			}
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int ocf_user_matches_hostmask(struct ocf_user* ocf_user_p, const char* hostmask)
{
	char *parv[OCF_MAXPARA];
	int i, parc;
	char masks[OCF_USER_HOSTS_LENGTH];
	
	if (strlen(ocf_user_p->hostmasks) == 0) {
		return 0;
	}

	/* tokenize destroys the original string, so tokenize a copy */
	strncpy(masks, ocf_user_p->hostmasks, sizeof(masks));
	masks[sizeof(masks)-1] = 0;
	parc = tokenize_string_separator(masks, parv, ',');

	for (i = 0; i < parc; i++) {
		if (match(parv[i], hostmask)){
			return 1;
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int ocf_user_set_main_server(struct ocf_user* ocf_user_p, const char* server)
{
	strncpy(ocf_user_p->main_server, server, sizeof(ocf_user_p->main_server));

	return 1;
}

/* ----------------------------------------------------------------- */

int ocf_user_has_main_server(struct ocf_user* ocf_user_p, const char* server)
{
	if (!server)
		return 0;

	if (!ocf_user_p->main_server)
		return 0;

	if (strlen(server) == strlen(ocf_user_p->main_server)){
		if ( !strcasecmp(ocf_user_p->main_server, server) ) {
			return 1;
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int ocf_user_add_server(struct ocf_user* ocf_user_p, const char* server)
{
	/* Don't add the server if that will exceed the maximum length for
	 * servers. */
	if (strlen(ocf_user_p->servers) + strlen(server) + 1 > OCF_USER_SERVERS_LENGTH) {
		return 0;
	}

	/* Don't add the server if it's already present in the other servers */
	if (ocf_user_has_other_server(ocf_user_p, server)) {
		return 0;
	}

	if (strlen(ocf_user_p->servers) == 0) {
		strncpy(ocf_user_p->servers, server, sizeof(ocf_user_p->servers));
		ocf_user_p->servers[sizeof(ocf_user_p->servers)-1] = 0;
		return 1;
	}

	strcat(ocf_user_p->servers, ",");
	strcat(ocf_user_p->servers, server);
	return 1;
}

/* ----------------------------------------------------------------- */

int ocf_user_del_server(struct ocf_user* ocf_user_p, const char* server)
{
	char *parv[OCF_MAXPARA];
	int i, parc;
	char masks[OCF_USER_SERVERS_LENGTH];

	if (!ocf_user_has_other_server(ocf_user_p, server)){
		return 0;
	}

	/* tokenize destroys the original string, so tokenize a copy */
	strncpy(masks, ocf_user_p->servers, sizeof(masks));
	masks[sizeof(masks)-1] = 0;
	parc = tokenize_string_separator(masks, parv, ',');

	/* Re-add all servers except the one that needs to be removed */
	ocf_user_p->servers[0] = 0;
	for (i = 0; i < parc; i++) {
		if (strlen(server) == strlen(parv[i])){
			if ( !strncmp(server, parv[i], strlen(server)) ) {
				/* don't re-add this server */
			} else {
				ocf_user_add_server(ocf_user_p, parv[i]);
			}
		} else {
			ocf_user_add_server(ocf_user_p, parv[i]);
		}
	}

	return 1;
}

/* ----------------------------------------------------------------- */

int ocf_user_has_server(struct ocf_user* ocf_user_p, const char* server) 
{
	return (ocf_user_has_main_server(ocf_user_p, server) ||
		ocf_user_has_other_server(ocf_user_p, server));
}

/* ----------------------------------------------------------------- */

int ocf_user_has_other_server(struct ocf_user* ocf_user_p, const char* server)
{
	char *parv[OCF_MAXPARA];
	int i, parc;
	char servers[OCF_USER_SERVERS_LENGTH];
	
	if (strlen(ocf_user_p->servers) == 0) {
		return 0;
	}

	/* tokenize destroys the original string, so tokenize a copy */
	strncpy(servers, ocf_user_p->servers, sizeof(servers));
	servers[sizeof(servers)-1] = 0;
	parc = tokenize_string_separator(servers, parv, ',');

	for (i = 0; i < parc; i++) {
		/* TODO: wildcard server matching? */
		if (strlen(server) == strlen(parv[i])){
			if ( !strncmp(server, parv[i], strlen(server)) ) {
				return 1;
			}
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

int get_num_flags_of_server(const char flag, const char* server)
{
	dlink_node *ptr;
	struct ocf_user *ou;
	int flag_val, owner_flag_val, num_users;

	num_users      = 0;
	flag_val       = ocf_user_flag_get_int(flag);
	owner_flag_val = ocf_user_flag_get_int('o');

	/* Count users with this flag. Ignore owners. */
	DLINK_FOREACH(ptr, ocf_user_list.head) {
		ou = ((struct ocf_user*)(ptr->data));
		if (!ou->deleted &&
			ocf_user_has_flag(ou, flag_val) && 
			!ocf_user_has_flag(ou, owner_flag_val) &&
			ocf_user_has_main_server(ou, server)
		) {
			num_users++;
		}
	}

	return num_users;
}

/* ----------------------------------------------------------------- */

void ocf_user_add_flags(struct ocf_user* ocf_user_p, int flags)
{
	ocf_user_p->flags |= flags;
}

/* ----------------------------------------------------------------- */

void ocf_user_del_flags(struct ocf_user* ocf_user_p, int flags)
{
	ocf_user_p->flags &= ~flags;
}

/* ----------------------------------------------------------------- */

int ocf_user_has_flag(struct ocf_user* ocf_user_p, int flag)
{
	/* Owner flag overrules all other flags. */
	if (ocf_user_p->flags & OCF_USER_FLAGS_OWNER) {
		return 1;
	}

	if (ocf_user_p->flags & flag) {
		return 1;
	}

	return 0;
}

/* ----------------------------------------------------------------- */

void ocf_user_add_notice_flags(struct ocf_user* ocf_user_p, int flags)
{
	ocf_user_p->noticeflags |= flags;
}

/* ----------------------------------------------------------------- */

void ocf_user_del_notice_flags(struct ocf_user* ocf_user_p, int flags)
{
	ocf_user_p->noticeflags &= ~flags;
}

/* ----------------------------------------------------------------- */

int ocf_user_has_notice_flag(struct ocf_user* ocf_user_p, int flag)
{
	if (ocf_user_p->noticeflags & flag) {
		return 1;
	}

	return 0;
}

/* ----------------------------------------------------------------- */

void ocf_user_disable(struct ocf_user* ocf_user_p)
{
	ocf_user_p->disabled = 1;
	/* Find out if the user is logged in; if so, log him out */
}

/* ----------------------------------------------------------------- */

void ocf_user_enable(struct ocf_user* ocf_user_p)
{
	ocf_user_p->disabled = 0;
}

/* ----------------------------------------------------------------- */

void ocf_user_delete(struct ocf_user* ocf_user_p)
{
	ocf_user_p->deleted = 1;
	/* Find out if the user is logged in; if so, log him out */
}

/* ----------------------------------------------------------------- */

char* ocf_user_get_name_from_id(int id)
{
	dlink_node *ptr;
	struct ocf_user *ou;

	DLINK_FOREACH(ptr, ocf_user_list.head) {
		ou = ((struct ocf_user*)(ptr->data));
		if (ou->id == id) {
			return ou->name;
		}
	}

	return NULL;
}

/* ----------------------------------------------------------------- */

void ocf_user_load_all(const char* filename)
{
	char line[MAX_LINE_LENGTH + 1]; /* Yeah, lazy. Should work with realloc perhaps. */
	FILE* fp;
	int linenum = 0;
	int numusers = 0;
	char *parv[OCF_MAXPARA];
	int parc;
	int i;
	struct ocf_user tmp_user;
	struct ocf_user *user_p;
	
    fp = fopen(filename, "r");

    if(!fp) {
		ocf_log(OCF_LOG_MAIN, "USER Error opening users file %s for reading.", filename);
		ocf_log(OCF_LOG_ERROR, "USER Error opening users file %s for reading.", filename);
		sendto_realops_flags(ALL_UMODES, L_ALL, 
			"Error opening users file %s for reading.", filename);

        return;
    }

	memset(&tmp_user, 0, sizeof(tmp_user));

	/* Read lines until end of file.
	 * If a line contains 'id = <num>', add the previously gathered
	 * data as a new user. 
	 */
	while (readline(fp, line, MAX_LINE_LENGTH))
	{
        linenum++;
		strtrim(line);
        if (!*line || *line == '#')
            continue;

		parc = tokenize_string_separator(line, parv, '=');
		if (parc != 2) {
			/* log parse error on this line */
			continue;
		}

		strtrim(parv[0]);
		strtrim(parv[1]);
		if (strcmp(parv[0], "id") == 0) {
			if (numusers > 0) {
				user_p = ocf_add_user_from_obj(&tmp_user);
				if (!user_p) {
					ocf_log(OCF_LOG_MAIN, "USER Error adding user %s while loading users.", tmp_user.name);
					ocf_log(OCF_LOG_ERROR, "USER Error adding user %s while loading users.", tmp_user.name);
				} else {
					ocf_log(OCF_LOG_DEBUG, "Loaduser: %s", tmp_user.name);
				}
				/* add previous, check for double name/id */
			}
			numusers++;
			memset(&tmp_user, 0, sizeof(tmp_user));
			tmp_user.id = atoi(parv[1]);
			/* add previous, start new user */
		}

		else if (strcmp(parv[0], "name") == 0) {
			strncpy(tmp_user.name, parv[1], sizeof(tmp_user.name));
		}

		else if (strcmp(parv[0], "pass") == 0) {
			strncpy(tmp_user.password, parv[1], sizeof(tmp_user.password));
		}

		else if (strcmp(parv[0], "flags") == 0) {
			for(i = 0; i < strlen(parv[1]); i++) {
				/* Ignore any minus characters */
				if (parv[1][i] != '-') {
					ocf_user_add_flags(&tmp_user, 
						ocf_user_flag_get_int(parv[1][i]));
				}
			}
		}

		else if (strcmp(parv[0], "umode") == 0) {
			for(i = 0; i < strlen(parv[1]); i++) {
				/* Ignore any minus characters */
				if (parv[1][i] != '-') {
					ocf_user_add_notice_flags(&tmp_user, 
						ocf_user_notice_flag_get_int(parv[1][i]));
				}
			}
		}

		else if (strcmp(parv[0], "host") == 0) {
			ocf_user_add_hostmask(&tmp_user, parv[1]);
		}

		else if (strcmp(parv[0], "server") == 0) {
			ocf_user_add_server(&tmp_user, parv[1]);
		}

		else if (strcmp(parv[0], "main_server") == 0) {
			ocf_user_set_main_server(&tmp_user, parv[1]);
		}

		else if (strcmp(parv[0], "deleted") == 0) {
			tmp_user.deleted = atoi(parv[1]);
		}

		else if (strcmp(parv[0], "disabled") == 0) {
			tmp_user.disabled = atoi(parv[1]);
		}
	}

	/* Add the final user */
	user_p = ocf_add_user_from_obj(&tmp_user);
	if (!user_p) {
		ocf_log(OCF_LOG_MAIN, "USER Error adding user %s while loading users.", tmp_user.name);
		ocf_log(OCF_LOG_ERROR, "USER Error adding user %s while loading users.", tmp_user.name);
	}

	fclose(fp);
}

/* ----------------------------------------------------------------- */

void ocf_user_save_all(const char* filename)
{
	FILE* fp;
	int numusers = 0;
	char hostmasks[OCF_USER_HOSTS_LENGTH + 1];
	char servers[OCF_USER_SERVERS_LENGTH + 1];
	char *hostmasks_parv[OCF_MAXPARA], *servers_parv[OCF_MAXPARA];
	int hostmasks_parc, servers_parc;
	int i;
	struct ocf_user *ou;
	dlink_node *ptr;

    fp = fopen(filename, "w");
	ocf_log(OCF_LOG_MAIN, "USER Writing users to %s.", filename);

    if(!fp) {
		ocf_log(OCF_LOG_MAIN, "USER Error opening users file %s for writing.", filename);
		ocf_log(OCF_LOG_ERROR, "USER Error opening users file %s for writing.", filename);
		sendto_realops_flags(ALL_UMODES, L_ALL, 
			"Error opening users file %s for writing.", filename);
        return;
    }

	DLINK_FOREACH(ptr, ocf_user_list.head)
	{
		numusers++;
		ou = ((struct ocf_user*)(ptr->data));

		/* Tokenize all this user's hostmasks. */
		strncpy(hostmasks, ou->hostmasks, sizeof(hostmasks));
		hostmasks[sizeof(hostmasks)-1] = 0;
		hostmasks_parc = tokenize_string_separator(hostmasks, hostmasks_parv, ',');

		/* Tokenize all this user's servers. */
		strncpy(servers, ou->servers, sizeof(servers));
		servers[sizeof(servers)-1] = 0;
		servers_parc = tokenize_string_separator(servers, servers_parv, ',');

		/* write user */
		writeline(fp, "id = %d", ou->id);
		writeline(fp, "name = %s", ou->name);
		writeline(fp, "pass = %s", ou->password);
		if (strlen(ocf_user_flags_to_string(ou->flags)) > 0) {
			writeline(fp, "flags = %s", ocf_user_flags_to_string(ou->flags));
		} else {
			writeline(fp, "flags = -");
		}
		if (strlen(ocf_user_notice_flags_to_string(ou->noticeflags)) > 0) {
			writeline(fp, "umode = %s", ocf_user_notice_flags_to_string(ou->noticeflags));
		} else {
			writeline(fp, "umode = -");
		}
		for (i = 0; i < hostmasks_parc; i++) {
			writeline(fp, "host = %s", hostmasks_parv[i]);
		}
		writeline(fp, "main_server = %s", ou->main_server);
		for (i = 0; i < servers_parc; i++) {
			writeline(fp, "server = %s", servers_parv[i]);
		}
		writeline(fp, "disabled = %d", ou->disabled);
		writeline(fp, "deleted = %d", ou->deleted);
		writeline(fp, "");
	}

	fclose(fp);
}

/* ----------------------------------------------------------------- */

void ocf_user_load_loggedin()
{
	char line[MAX_LINE_LENGTH + 1]; /* Yeah, lazy. Should work with realloc perhaps. */
	FILE* fp;
	int linenum = 0;
	char *parv[OCF_MAXPARA];
	int parc;
	time_t time_saved;
	struct ocf_user *ou;

	fp = fopen(OCF_USER_LOGGEDIN_FILE, "r");
	if (fp == NULL) {
		return;
	}

	/* Get the time that this file was created. */
	if (readline(fp, line, MAX_LINE_LENGTH)) {
		time_saved = atol(line);
		if (time(NULL) - time_saved > OCF_USER_LOGGEDIN_MAXTIME) {
			/* Data is too old. Discard it. */
			fclose(fp);
			unlink(OCF_USER_LOGGEDIN_FILE);
			return;
		}
	} else {
		/* Error reading the time. Bail out. */
		fclose(fp);
		unlink(OCF_USER_LOGGEDIN_FILE);
		return;
	}

	/* Read all the logged in users from the file now. */
	while (readline(fp, line, MAX_LINE_LENGTH))
	{
        linenum++;
		parc = tokenize_string_separator(line, parv, '|');
		if (parc != 5) {
			/* log parse error on this line */
			continue;
		}
		/* Each line is "name|nick|user|host|server" */
		ou = ocf_get_user(parv[0]);
		if (ou == NULL) {
			ocf_log(OCF_LOG_ERROR, 
				"Loading loggedin users: found non-existing username %s.",
				parv[0]);
			continue;
		}
		ocf_user_login(parv[1], parv[2], parv[3], parv[4], ou);
	}

	fclose(fp);
}

/* ----------------------------------------------------------------- */

void ocf_user_save_loggedin()
{
	char line[MAX_LINE_LENGTH + 1]; /* Yeah, lazy. Should work with realloc perhaps. */
	dlink_node *ptr;
	struct ocf_loggedin_user *olu;
	FILE* fp;

	if (dlink_list_length(&ocf_loggedin_user_list) == 0) {
		/* No logged in users. Delete the logged in users file. */
		fp = fopen(OCF_USER_LOGGEDIN_FILE, "w");
		if (fp) {
			fclose(fp);
			unlink(OCF_USER_LOGGEDIN_FILE);
		}
		return;
	}

	fp = fopen(OCF_USER_LOGGEDIN_FILE, "w");

	if (fp == NULL) {
		ocf_log(OCF_LOG_MAIN, 
			"USER Error opening logged in users file %s for writing.", 
			OCF_USER_LOGGEDIN_FILE);
		ocf_log(OCF_LOG_ERROR, 
			"USER Error opening logged in users file %s for writing.", 
			OCF_USER_LOGGEDIN_FILE);
		sendto_realops_flags(ALL_UMODES, L_ALL, 
			"Error opening logged in users file %s for writing.", OCF_USER_LOGGEDIN_FILE);
        return;
	}

	/* Write the current time. */
	snprintf(line, sizeof(line), "%ld", (long)time(NULL));
	writeline(fp, line);

	/* Write all users. */
	DLINK_FOREACH(ptr, ocf_loggedin_user_list.head) {
		olu = ((struct ocf_loggedin_user*)(ptr->data));
		/* Write line "name|nick|user|host|server" */
		snprintf(line, sizeof(line), "%s|%s|%s|%s|%s", olu->ocf_user_p->name,
			olu->nick, olu->user, olu->host, olu->server);
		writeline(fp, line);
	}

	fclose(fp);
}

/* ----------------------------------------------------------------- */

struct ocf_loggedin_user* ocf_get_loggedin_user(const char* nick)
{
	dlink_node *ptr;
	struct ocf_loggedin_user *olu;

	DLINK_FOREACH(ptr, ocf_loggedin_user_list.head) {
		olu = ((struct ocf_loggedin_user*)(ptr->data));
		if (irccmp(olu->nick, nick) == 0) {
			return olu;
		}
	}
	
	return NULL;
}

/* ----------------------------------------------------------------- */

struct ocf_user* ocf_get_user(const char* name)
{
	dlink_node *ptr;
	struct ocf_user *ou;

	DLINK_FOREACH(ptr, ocf_user_list.head) {
		ou = ((struct ocf_user*)(ptr->data));
		if (strcasecmp(ou->name, name) == 0 && !ou->deleted)
			return ou;
	}
	
	return NULL;
}

/* ----------------------------------------------------------------- */

struct ocf_loggedin_user* ocf_user_login(const char* nick, const char *user, 
										 const char *host, const char *server,
										 struct ocf_user *u)
{
	struct ocf_loggedin_user *user_p;

	if (ocf_get_loggedin_user(nick)) {
		ocf_log(OCF_LOG_DEBUG, "oul %s already logged in", nick);
		/* User already exists */
		return NULL;
	}

	user_p = BlockHeapAlloc(ocf_loggedin_user_heap);
	memset(user_p, 0, sizeof(struct ocf_loggedin_user));
	strncpy(user_p->nick, nick, sizeof(user_p->nick) - 1);
	strncpy(user_p->user, user, sizeof(user_p->user) - 1);
	strncpy(user_p->host, host, sizeof(user_p->host) - 1);
	strncpy(user_p->server, server, sizeof(user_p->server) - 1);
	user_p->ocf_user_p = u;
	user_p->lasttime = time(NULL);
	dlinkAdd(user_p, make_dlink_node(), &ocf_loggedin_user_list);

	return user_p;
}

/* ----------------------------------------------------------------- */

void ocf_user_logout(const char* nick)
{
	dlink_node *ptr;
	struct ocf_loggedin_user *olu;

	DLINK_FOREACH(ptr, ocf_loggedin_user_list.head)
	{
		olu = ((struct ocf_loggedin_user*)(ptr->data));
		if (irccmp(olu->nick, nick) == 0)
		{
			BlockHeapFree(ocf_loggedin_user_heap, ptr->data);
			dlinkDelete(ptr, &ocf_loggedin_user_list);
			return;
		}
	}
}

/* ----------------------------------------------------------------- */

void ocf_check_loggedin_users()
{
	dlink_node *ptr, *next_ptr;
	struct ocf_loggedin_user *olu;
	time_t now;

	if (OCF_USER_LOGGEDIN_TIMEOUT == 0) {
		return;
	}

	now = time(NULL);

	DLINK_FOREACH_SAFE(ptr, next_ptr, ocf_loggedin_user_list.head)
	{
		olu = (struct ocf_loggedin_user*)(ptr->data);
		if (verify_loggedin_user(olu)) {
			olu->lasttime = now;
		} else {
			if (now - olu->lasttime > 60 * OCF_USER_LOGGEDIN_TIMEOUT) {
				ocf_log(OCF_LOG_LOGIN, 
					"GONE Logged out gone user %s (%s@%s) [%s].",
					olu->nick, olu->user, olu->host, olu->server);
				send_notice_to_loggedin_except(olu, OCF_NOTICE_FLAGS_LOGINOUT,
					"Logged out gone user %s (%s@%s) [%s].",
					olu->nick, olu->user, olu->host, olu->server);
				ocf_user_logout(olu->nick);
			} else if (olu->ocf_user_p->deleted) {
				ocf_log(OCF_LOG_LOGIN, 
					"GONE Logged out deleted user %s (%s@%s) [%s].",
					olu->nick, olu->user, olu->host, olu->server);
				send_notice_to_loggedin_except(olu, OCF_NOTICE_FLAGS_LOGINOUT,
					"Logged out deleted user %s (%s@%s) [%s].",
					olu->nick, olu->user, olu->host, olu->server);
				ocf_user_logout(olu->nick);
			}
		}
	}
}

/* ----------------------------------------------------------------- */

int verify_loggedin_user(struct ocf_loggedin_user *olu)
{
	struct Client *clptr;

	/* If the user has been deleted, the user can't be logged in anymore. */
	if (olu->ocf_user_p->deleted) {
		return 0;
	}

	clptr = find_client(olu->nick);

	/* Return 0 if there is no client with nick olu->nick. */
	if (!clptr) {
		return 0;
	}

	/* Return 1 if this is the correct client. */
	if (!strcasecmp(clptr->username, olu->user) &&
		!strcasecmp(clptr->host, olu->host) &&
		!strcasecmp(clptr->servptr->name, olu->server)) {
		return 1;
	}

	/* Different client, return -1. */
	return -1;
}

/* ----------------------------------------------------------------- */

const char* ocf_user_flags_to_string(int flags)
{
	static char flagchars[32];
	int pos = 0;
	int i;

	for (i = 1; i <= OCF_USER_FLAGS_LAST; i *= 2)
	{
		if (flags & i) {
			flagchars[pos] = ocf_user_flag_get_char(i);
			pos++;
		}
	}
	flagchars[pos] = 0;

	return flagchars;
}

/* ----------------------------------------------------------------- */

char ocf_user_flag_get_char(int flag)
{
	switch (flag) {
	case OCF_USER_FLAGS_OWNER      : return 'o';
	case OCF_USER_FLAGS_BLOCK      : return 'b';
	case OCF_USER_FLAGS_USERMANAGER: return 'u';
	case OCF_USER_FLAGS_FIX        : return 'f';
	case OCF_USER_FLAGS_CHANNEL    : return 'c';
	case OCF_USER_FLAGS_SERVERADMIN: return 'a';
	}
	return ' ';
}

/* ----------------------------------------------------------------- */

char ocf_user_flag_get_int(char flag)
{
	switch (flag) {
	case 'o': return OCF_USER_FLAGS_OWNER;
	case 'b': return OCF_USER_FLAGS_BLOCK;
	case 'u': return OCF_USER_FLAGS_USERMANAGER;
	case 'f': return OCF_USER_FLAGS_FIX;
	case 'c': return OCF_USER_FLAGS_CHANNEL;
	case 'a': return OCF_USER_FLAGS_SERVERADMIN;
	}
	return 0;
}

/* ----------------------------------------------------------------- */

const char* ocf_user_notice_flags_to_string(int flags)
{
	static char flagchars[32];
	int pos = 0;
	int i;

	for (i = 1; i <= OCF_NOTICE_FLAGS_LAST; i *= 2)
	{
		if (flags & i) {
			flagchars[pos] = ocf_user_notice_flag_get_char(i);
			pos++;
		}
	}
	flagchars[pos] = 0;

	return flagchars;
}

/* ----------------------------------------------------------------- */

char ocf_user_notice_flag_get_char(int flag)
{
	switch (flag) {
	case OCF_NOTICE_FLAGS_BLOCK   : return 'b';
	case OCF_NOTICE_FLAGS_CHANFIX : return 'c';
	case OCF_NOTICE_FLAGS_LOGINOUT: return 'l';
	case OCF_NOTICE_FLAGS_NOTES   : return 'n';
	case OCF_NOTICE_FLAGS_USER    : return 'u';
	case OCF_NOTICE_FLAGS_AUTOFIX : return 'a';
	case OCF_NOTICE_FLAGS_MAIN    : return 'm';
	}
	return ' ';
}

/* ----------------------------------------------------------------- */

char ocf_user_notice_flag_get_int(char flag)
{
	switch (flag) {
	case 'b': return OCF_NOTICE_FLAGS_BLOCK;
	case 'c': return OCF_NOTICE_FLAGS_CHANFIX;
	case 'l': return OCF_NOTICE_FLAGS_LOGINOUT;
	case 'n': return OCF_NOTICE_FLAGS_NOTES;
	case 'u': return OCF_NOTICE_FLAGS_USER;
	case 'a': return OCF_NOTICE_FLAGS_AUTOFIX;
	case 'm': return OCF_NOTICE_FLAGS_MAIN;
	}
	return 0;
}

/* ----------------------------------------------------------------- */

