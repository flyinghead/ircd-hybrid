/*
 * $Id: cf_base.h,v 1.14 2004/11/09 20:29:29 cfh7 REL_2_1_3 $
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



/* This is the main header file. Include this and only this in 
 * every .c file. */

#include "stdinc.h"
#include "handlers.h"
#include "client.h"
#include "channel_mode.h" /* added by garion */
#include "packet.h" /* added by garion */
#include "common.h"     /* FALSE bleah */
#include "ircd.h"
#include "irc_string.h"
#include "numeric.h"
#include "fdlist.h"
#include "s_bsd.h"
#include "s_conf.h"
#include "s_log.h"
#include "s_serv.h"
#include "send.h"
#include "msg.h"
#include "parse.h"
#include "modules.h"
#include "hash.h" /* added by garion */

#define OCF_VERSION "2.1.3"
#define OCF_VERSION_NAME "Zen"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <crypt.h>
#include "cf_tools.h"
#include "cf_init.h"
#include "cf_event.h"
#include "cf_database.h"
#include "cf_user.h"
#include "cf_ignore.h"
#include "cf_command.h"
#include "cf_message.h"
#include "cf_settings.h"
#include "cf_fix.h"
#include "cf_log.h"
#include "cf_gather.h"
#include "cf_process.h"

#include "cf_main.h"
