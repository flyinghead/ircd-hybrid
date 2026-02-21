/*
 * $Id: cf_log.c,v 1.6 2004/08/24 18:55:22 cfh7 REL_2_1_3 $
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

char* log_setting_name[] = {
	"log_error",
	"log_debug",
	"log_main",
	"log_privmsg",
	"log_login",
	"log_chanfix",
	"log_notes",
	"log_users",
	"log_autofix",
	"log_score"
};

/* ----------------------------------------------------------------- */

void ocf_log_init()
{
	settings_add_str("logfile_chanfix", "chanfix/chanfix.log");
	settings_add_str("logfile_error", "chanfix/error.log");
	settings_add_str("logfile_debug", "chanfix/debug.log");
	settings_add_bool("log_debug", 0);
	settings_add_bool("log_privmsg", 0);
	settings_add_bool("log_login", 1);
	settings_add_bool("log_chanfix", 1);
	settings_add_bool("log_notes", 1);
	settings_add_bool("log_users", 1);
	settings_add_bool("log_autofix", 1);
	settings_add_bool("log_score", 1);
}

/* ----------------------------------------------------------------- */

void ocf_logfiles_open()
{
	fp_log_chanfix = fopen(settings_get_str("logfile_chanfix"), "a");
    if(!fp_log_chanfix) {
		sendto_realops_flags(ALL_UMODES, L_ALL, 
			"Error opening chanfix logfile %s",
			settings_get_str("logfile_chanfix"));
	}

	fp_log_error = fopen(settings_get_str("logfile_error"), "a");
    if(!fp_log_error) {
		sendto_realops_flags(ALL_UMODES, L_ALL, 
			"Error opening chanfix error logfile %s",
			settings_get_str("logfile_error"));
	}

	if (settings_get_bool("log_debug")) {
		fp_log_debug = fopen(settings_get_str("logfile_debug"), "a");
		if(!fp_log_debug) {
			sendto_realops_flags(ALL_UMODES, L_ALL, 
				"Error opening chanfix debug logfile %s",
				settings_get_str("logfile_debug"));
		}
	}
}

/* ----------------------------------------------------------------- */

void ocf_logfiles_close()
{
	if (fp_log_chanfix)
		fclose(fp_log_chanfix);

	if (fp_log_error)
		fclose(fp_log_error);
	
	if (settings_get_bool("log_debug")) {
		if (fp_log_debug)
			fclose(fp_log_debug);
	}
}

/* ----------------------------------------------------------------- */

void ocf_log(int msgtype, char* pattern, ...)
{
#define OCF_LOG_TIMESTAMP_LEN 24
	va_list args;
	char tmpBuf[1024];
	char tmpBuf2[1024];
	time_t ltime;
	struct tm *now;
	char datetimestr[OCF_LOG_TIMESTAMP_LEN];

	/* Don't process any log message if it's not required */
	if (msgtype != OCF_LOG_ERROR && msgtype != OCF_LOG_MAIN && 
			!settings_get_bool(log_setting_name[msgtype])) {
		return;
	}

	/* Get timestamp string. */
	time(&ltime);
	now = localtime(&ltime);
	memset(datetimestr, 0, OCF_LOG_TIMESTAMP_LEN);
	strftime(datetimestr, OCF_LOG_TIMESTAMP_LEN, "%Y-%m-%d %H:%M:%S", now);

	va_start(args, pattern);
	vsprintf(tmpBuf, pattern, args);
	va_end(args);
	tmpBuf[1023] = 0;

	/* Add timestamp to log line. */
	snprintf(tmpBuf2, sizeof(tmpBuf2), "%s %s", datetimestr, tmpBuf);
	tmpBuf2[1023] = 0;

	if (msgtype == OCF_LOG_ERROR) {
		if (fp_log_error) {
			fputs(tmpBuf2, fp_log_error);
			fputc('\n', fp_log_error);
			fflush(fp_log_error);
		}
	} else if (msgtype == OCF_LOG_DEBUG) {
		if (fp_log_debug) {
			fputs(tmpBuf2, fp_log_debug);
			fputc('\n', fp_log_debug);
			fflush(fp_log_debug);
		}
	} else {
		if (fp_log_chanfix) {
			fputs(tmpBuf2, fp_log_chanfix);
			fputc('\n', fp_log_chanfix);
			fflush(fp_log_chanfix);
		}
	}
}

/* ----------------------------------------------------------------- */

