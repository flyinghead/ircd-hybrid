/*
 * $Id: cf_user.h,v 1.10 2004/08/28 10:09:03 cfh7 REL_2_1_3 $
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






#define OCF_USER_NAME_LENGTH 9
#define OCF_USER_PASSWORD_LENGTH 14
#define OCF_USER_HOSTS_LENGTH 256 
#define OCF_USER_SERVERS_LENGTH 256 


#define OCF_USER_FLAGS_OWNER		0x0001
#define OCF_USER_FLAGS_BLOCK		0x0002
#define OCF_USER_FLAGS_USERMANAGER	0x0004
#define OCF_USER_FLAGS_FIX			0x0008
#define OCF_USER_FLAGS_CHANNEL      0x0010
#define OCF_USER_FLAGS_SERVERADMIN  0x0020

#define OCF_USER_FLAGS_LAST			0x0020

#define OCF_NOTICE_FLAGS_AUTOFIX	0x0001
#define OCF_NOTICE_FLAGS_BLOCK		0x0002
#define OCF_NOTICE_FLAGS_CHANFIX	0x0004
#define OCF_NOTICE_FLAGS_LOGINOUT	0x0008
#define OCF_NOTICE_FLAGS_MAIN		0x0010
#define OCF_NOTICE_FLAGS_NOTES		0x0020
#define OCF_NOTICE_FLAGS_USER		0x0040

#define OCF_NOTICE_FLAGS_LAST		0x0040
#define OCF_NOTICE_FLAGS_ALL		0x0000


/* Automatic login timeout, in minutes. Set to 0 to disable. */
#define OCF_USER_LOGGEDIN_TIMEOUT		60

/* Interval between logged in user checking, in minutes. */
#define OCF_USER_LOGGEDIN_INTERVAL		10

#define OCF_USER_HEAP_SIZE 1024
#define OCF_LOGGEDIN_USER_HEAP_SIZE 1024

/* File to which the logged in users are written during a
 * rehash and when the module is unloaded. */
#define OCF_USER_LOGGEDIN_FILE		"ocf_loggedin_users.txt"

/* If reloading the loggedin users from file, what's the maximum
 * allowed time difference between now and the time the file was
 * written? */
#define OCF_USER_LOGGEDIN_MAXTIME	600

/*
 * Holds all the data for a single user. This is the data that is
 * stored in the users.conf file. One entry per user id, which means
 * one struct per user.
 *
 * id       - the unique user id.
 * name     - the unique username, used to login with.
 * password - the DES encrypted password.
 * flags    - bfou flags.
 * hosts    - space separated list of u@h of this user.
 * disabled - whether the user account has been disabled
 * deleted  - if 1, this account has been deleted. This is necessary
 *            to make sure that each id is only ever used once.
 * noticeflags - whether the user wants to see notices of certain 
 *               events happening if they are logged in.
 *
 * Note: yes, there is a hard limit in the number of hosts that a
 * single user can use. Don't use too many different ones :)
 *
 * Note: the id must be unique.
 * 
 */
struct ocf_user {
	int id;
	char name[OCF_USER_NAME_LENGTH + 1];
	char password[OCF_USER_PASSWORD_LENGTH + 1];
	int flags;
	char hostmasks[OCF_USER_HOSTS_LENGTH + 1];
	char main_server[HOSTLEN + 1];
	char servers[OCF_USER_SERVERS_LENGTH + 1];
	int disabled;
	int deleted;
	int noticeflags;
};


/* This struct is created once a user has logged in successfully.
 * In it, the nick!user@host of the user that has logged in is stored.
 * If a command is sent to chanfix that requires the user to be logged
 * in, the user's nick and user and hostname must be identical to
 * when the user logged in.
 * This means that if you change your nick, you have to re-login.
 * If you use a different client, you have to login that client.
 * You can be logged in multiple times from multiple clients. 
 */

struct ocf_loggedin_user {
	char nick[NICKLEN + 1];
	char user[USERLEN + 1];
	char host[HOSTLEN + 1];
	char server[HOSTLEN + 1];
	time_t lasttime;
	struct ocf_user* ocf_user_p;
};

dlink_list ocf_user_list;
dlink_list ocf_loggedin_user_list;

/* Inits the user heaps. MUST be called BEFORE any other ocf_user_*
 * functions. */
void ocf_users_init();

/* Deletes all (logged in) users and deallocates the user heaps. */
void ocf_users_deinit();

/* Call this function whenever any user data is changed. This function
 * will save the users to the userfile. */
void ocf_users_updated();

int ocf_users_get_unique_id();

/* Allocates a new user and returns a pointer to the new user. */
struct ocf_user* ocf_add_user(const char* name, const char* hostmask);

/* Allocates a new user, copies data from <other> into it,
 * and returns a pointer to the new user. 
 * If other->id is 0 or negative, a new user id will be generated.
 * If other->id is 1 or larger, this id is not modified. 
 * TODO: check whether id already exists; if so, error.
 */
struct ocf_user* ocf_add_user_from_obj(struct ocf_user* other);

void ocf_user_set_password(struct ocf_user* ocf_user_p, const char* password);
void ocf_user_set_passwordhash(struct ocf_user* ocf_user_p, const char* hash);

/* Returns 1 if <password> matches with the user's password. */
int ocf_user_check_password(struct ocf_user* ocf_user_p, const char* password);

/* Adds <hostmask> to the user's hostmasks. Returns true on success and false
 * on failure. */
int ocf_user_add_hostmask(struct ocf_user* ocf_user_p, const char* hostmask);

/* Removes <hostmask> from the user's hostmasks. Returns true on success and 
 * false on failure. */
int ocf_user_del_hostmask(struct ocf_user* ocf_user_p, const char* hostmask);
int ocf_user_has_hostmask(struct ocf_user* ocf_user_p, const char* hostmask);

/* Returns true if the given hostmask matches any of the wildcarded hostmasks
 * that are stored in this user. */
int ocf_user_matches_hostmask(struct ocf_user* ocf_user_p, const char* hostmask);

/* Sets the main server of the user. Returns true on success and false
 * on failure. */
int ocf_user_set_main_server (struct ocf_user* ocf_user_p, const char* server);

/* Returns true if the user's main server is <server>, and false otherwise */
int ocf_user_has_main_server (struct ocf_user* ocf_user_p, const char* server);

/* Adds <server> to the user's servers. Returns true on success and false
 * on failure. */
int ocf_user_add_server(struct ocf_user* ocf_user_p, const char* server);

/* Removes <server> from the user's servers. Returns true on success and 
 * false on failure. */
int ocf_user_del_server(struct ocf_user* ocf_user_p, const char* server);

/* This function looks in both user->main_server and user->servers
 * to see if the user has the given server. Necessary for logins and
 * such */
int ocf_user_has_server(struct ocf_user* ocf_user_p, const char* server);

/* Looks only in user->servers, not in the main server. Useful for
 * manipulation of user->servers without involving user->main_server. */
int ocf_user_has_other_server(struct ocf_user* ocf_user_p, const char* server);

/* Returns the total number of <flag> flags possessed by all the non-owner
 * users on <server>. */
int get_num_flags_of_server(const char flag, const char* server);

void ocf_user_add_flags(struct ocf_user* ocf_user_p, int flags);
void ocf_user_del_flags(struct ocf_user* ocf_user_p, int flags);
int ocf_user_has_flag(struct ocf_user* ocf_user_p, int flag);

/* Returns a pointer to a string containing these flags converted to
 * string. This data will be overwritten every next call. */
const char* ocf_user_flags_to_string(int flags);
char ocf_user_flag_get_char(int flag);
char ocf_user_flag_get_int(char flag);

void ocf_user_add_notice_flags(struct ocf_user* ocf_user_p, int flags);
void ocf_user_del_notice_flags(struct ocf_user* ocf_user_p, int flags);
int ocf_user_has_notice_flag(struct ocf_user* ocf_user_p, int flag);

/* Returns a pointer to a string containing these flags converted to
 * string. This data will be overwritten every next call. */
const char* ocf_user_notice_flags_to_string(int flags);
char ocf_user_notice_flag_get_char(int flag);
char ocf_user_notice_flag_get_int(char flag);

void ocf_user_disable(struct ocf_user* ocf_user_p);
void ocf_user_enable(struct ocf_user* ocf_user_p);
void ocf_user_delete(struct ocf_user* ocf_user_p);

char* ocf_user_get_name_from_id(int id);

void ocf_user_load_all(const char* filename);
void ocf_user_save_all(const char* filename);

/* Loads the logged in users from file, but only if the data in that
 * file is not older than OCF_USER_LOGGEDIN_MAXTIME seconds. */
void ocf_user_load_loggedin();
void ocf_user_save_loggedin();

/* Returns a pointer to the logged in user struct if nick is logged in.
 * Returns NULL otherwise. */
struct ocf_loggedin_user* ocf_get_loggedin_user(const char* nick);

/* Returns a pointer to the user struct if a user <name> exists.
 * Returns NULL otherwise. */
struct ocf_user* ocf_get_user(const char* name);

/* Logs in the given user. Returns a pointer to the loggedin user
 * struct that has been created. */
struct ocf_loggedin_user* ocf_user_login(const char* nick, const char *user, 
										 const char *host, const char *server,
										 struct ocf_user *u);

/* Logs out this nickname. */
void ocf_user_logout(const char* nick);

/* Checks all users that are logged in.
 * If a user has been idle for too long ("user_loggedin_timeout" setting, 
 * in minutes), they are logged out.
 */
void ocf_check_loggedin_users();

/* Returns 1 if the client that currently has nickname olu->nick
 * is the same as the one that logged in. 
 * Returns 0 if there is no client with nickname olu->nick.
 * Returns -1 if the client that currently has nickname olu->nick
 * is NOT the same as the one that logged in. 
 */
int verify_loggedin_user(struct ocf_loggedin_user *olu);
