/*
 * $Id: cf_log.h,v 1.6 2004/08/24 22:21:52 cfh7 REL_2_1_3 $
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




/* Logging
 * There are 3 logfiles:
 * chanfix.log  - common logfile for everything
 * error.log    - error log
 * debug.log    - log for debug messages
 *
 * Bools describing which events should be logged in chanfix.log:
 * log_privmsg - all privmsgs sent to chanfix [shows passwords!]
 * log_login   - login/logout commands
 * log_users   - commands changing user records
 * log_chanfix - chanfix requests
 * log_notes   - adding/deletion of notes
 * log_users   - adding/deletion users/flags/hostmasks
 * log_autofix - automatic fixing of channels
 *
 * log_debug   - logs debug messages
 */

#define OCF_LOG_ERROR		0
#define OCF_LOG_DEBUG		1
#define OCF_LOG_MAIN		2
#define OCF_LOG_PRIVMSG		3
#define OCF_LOG_LOGIN		4
#define OCF_LOG_CHANFIX		5
#define OCF_LOG_NOTES		6
#define OCF_LOG_USERS		7
#define OCF_LOG_AUTOFIX		8
#define OCF_LOG_SCORE		9


/* Array of log setting names. */
extern char* log_setting_name[];

FILE *fp_log_chanfix;
FILE *fp_log_error;
FILE *fp_log_debug;


/* Inits logfiles and default loglevels.
 * Call this function after init_settings(), but before loading
 * the settings. */
void ocf_log_init();

/* Opens the 3 logfiles. Call this function after ocf_init_log() and
 * after reading the settings from file. After all, the names of the
 * logfiles are in the settings. */
void ocf_logfiles_open();

void ocf_logfiles_close();

void ocf_log(int type, char* pattern, ...);

