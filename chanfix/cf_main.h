/*
 * $Id: cf_main.h,v 1.10 2004/08/24 22:21:52 cfh7 REL_2_1_3 $
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



/* Path to directory where the database will be stored/retrieved from */
#define OCF_DBPATH	"cfdb/"

/* Path to the chanfix config file */
#define SETTING_FILENAME "chanfix/chanfix.conf"


/*
 * Main defines, functions, and global variables.
 * Note that many of the main defines are used in a setting and therefore
 * don't need to be changed before compilation; they can be changed either
 * in the config file or via the SET command. 
 */

#define MAX_LINE_LENGTH 255

#define USERHOSTLEN (USERLEN + HOSTLEN + 1)


#define OCF_NICK "C"
#define OCF_USER "chanfix"
#define OCF_HOST "open.chanfix"
#define OCF_REAL "channel fixing service"

#define OCF_OPPED		0x0001
#define OCF_IDENTED		0x0002
#define OCF_REVERSED	0x0004
#define OCF_MINCLIENTS	4

#define OCF_NEEDED (OCF_OPPED | OCF_IDENTED | OCF_REVERSED)

#define OCF_OWN_HOST		"127.0.0.1"
#define OCF_CONNECT_HOST	"127.0.0.1"
#define OCF_CONNECT_PORT	6667
/* The (re)connect delay in case C gets disconnected, in seconds. */
#define OCF_CONNECT_DELAY	3

#define OCF_NUM_TOP_SCORES		10
#define OCF_NUM_CLIENTS_OPPED	5

/* Interval between two consecutive database updates. */
#define OCF_DATABASE_UPDATE_TIME	300

/* Interval between two consecutive database saves to disk. */
#define OCF_DATABASE_SAVE_TIME		21600
/* Database save time offset to make sure the rotation, update
 * and save commands aren't executed immediately following
 * eachother. */
#define OCF_DATABASE_SAVE_OFFSET	100


#define OCF_MAX_SCORE ((int)(DAYSAMPLES * (86400 / OCF_DATABASE_UPDATE_TIME)))


/* The maximum number of channel modes that can be issued in
 * a single command, e.g. +oooo a b c d */
#define MAX_CHANNEL_MODES	4

/* The minimum percentage of servers that need to be linked;
 * if there are fewer servers linked, chanfix will not fix
 * any channels, be it automatic or manual. */
#define MIN_SERVERS_PRESENT	75


#define OCF_USERFILE   "chanfix/users.conf"
#define OCF_IGNOREFILE "chanfix/ignores.conf"

#define OCF_LOGFILE_CHANFIX "chanfix/chanfix.log"
#define OCF_LOGFILE_ERROR   "chanfix/error.log"
#define OCF_LOGFILE_DEBUG   "chanfix/debug.log"


void init_prefs();
void OCF_read_packet(int fd, void *data);

void create_chanfix_client();

void OCF_execute_command(struct Client *source_p, int parc, char *parv[]);


void chanfix_update_database(void* args);


void kill_chanfix_client(struct Client* target_p, char* reason);


/* Global vars */

/* The core of chanfix are these two Client pointers.
 *
 * chanfix is a pointer to the actual client in the internal data of the
 * ircd, and is not created or deleted by this module. It's simply a
 * pointer.
 *
 * chanfix_remote, on the other hand, is a unique Client, which is not
 * present in any of the ircd's lists or hashes. It is used to make a
 * tcp/ip connection to localhost, and thus connect the Chanfix client
 * to the local ircd.
 * All messages sent by the ircd to chanfix will end up in chanfix_remote's
 * receive buffer, from where they will be processed.
 */
struct Client* chanfix;
struct Client* chanfix_remote;

/* Indicates whether we are currently split. */
int currently_split;

int send_notices_instead_of_privmsgs;

/* Indicates whether the welcome message has been sent to the logged
 * in users. */
int sent_init_message;

/* This is the current day, counting from the Epoch.
 * Necessary for database rotation at midnight. */
long current_day_since_epoch;

/* The time that the module was loaded at. */
time_t module_loaded_time;

/* ----------------------------------------------------------------- */
/* Fixing all "warning: implicit declaration"s.
 * These should actually be obtained by including the proper core
 * header file... 
 * TODO: Get the proper headers of hybrid included without errors.
 */
extern dlink_node* make_dlink_node();
void free_dlink_node (dlink_node *);
struct Channel* hash_find_channel (const char *name);
//struct Channel* hash_get_chptr(unsigned int hashv);
struct Client* find_client (const char *name);
extern void free_client(struct Client *client_p);
void oper_up (struct Client *);

typedef void EVH (void *);
void eventAdd (const char *name, EVH *func, void *arg, time_t when);
void eventDelete (EVH *func, void *);

/* ----------------------------------------------------------------- */
