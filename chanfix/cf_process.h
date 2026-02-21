/*
 * $Id: cf_process.h,v 1.8 2004/08/28 08:01:42 cfh7 REL_2_1_3 $
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


/* These are the functions that process all incoming messages after
 * they've been parsed. */

void process_cmd_unknown(struct message_data *md, struct command_data *cd);
void process_cmd_help(struct message_data *md, struct command_data *cd);
void process_cmd_score(struct message_data *md, struct command_data *cd);
void process_cmd_chanfix(struct message_data *md, struct command_data *cd);
void process_cmd_login(struct message_data *md, struct command_data *cd);
void process_cmd_logout(struct message_data *md, struct command_data *cd);
void process_cmd_passwd(struct message_data *md, struct command_data *cd);
void process_cmd_history(struct message_data *md, struct command_data *cd);
void process_cmd_info(struct message_data *md, struct command_data *cd);
void process_cmd_oplist(struct message_data *md, struct command_data *cd);
void process_cmd_addnote(struct message_data *md, struct command_data *cd);
void process_cmd_delnote(struct message_data *md, struct command_data *cd);
void process_cmd_whois(struct message_data *md, struct command_data *cd);
void process_cmd_addflag(struct message_data *md, struct command_data *cd);
void process_cmd_delflag(struct message_data *md, struct command_data *cd);
void process_cmd_addhost(struct message_data *md, struct command_data *cd);
void process_cmd_delhost(struct message_data *md, struct command_data *cd);
void process_cmd_adduser(struct message_data *md, struct command_data *cd);
void process_cmd_deluser(struct message_data *md, struct command_data *cd);
void process_cmd_chpass(struct message_data *md, struct command_data *cd);
void process_cmd_chpasshash(struct message_data *md, struct command_data *cd);
void process_cmd_who(struct message_data *md, struct command_data *cd);
void process_cmd_umode(struct message_data *md, struct command_data *cd);
void process_cmd_status(struct message_data *md, struct command_data *cd);
void process_cmd_set(struct message_data *md, struct command_data *cd);
void process_cmd_block(struct message_data *md, struct command_data *cd);
void process_cmd_unblock(struct message_data *md, struct command_data *cd);
void process_cmd_rehash(struct message_data *md, struct command_data *cd);
void process_cmd_check(struct message_data *md, struct command_data *cd);
void process_cmd_opnicks(struct message_data *md, struct command_data *cd);
void process_cmd_cscore(struct message_data *md, struct command_data *cd);
void process_cmd_alert(struct message_data *md, struct command_data *cd);
void process_cmd_unalert(struct message_data *md, struct command_data *cd);
void process_cmd_addserver(struct message_data *md, struct command_data *cd);
void process_cmd_delserver(struct message_data *md, struct command_data *cd);
void process_cmd_whoserver(struct message_data *md, struct command_data *cd);
void process_cmd_setmainserver(struct message_data *md, struct command_data *cd);



void process_cmd_wrong_syntax(struct message_data *md, struct command_data *cd);

void process_cmd_score_existing_channel(struct message_data *md, struct command_data *cd, struct Channel* chptr, int compact);
void process_cmd_score_nonexisting_channel(struct message_data *md, struct command_data *cd, int compact);
void process_cmd_score_nick(struct message_data *md, struct command_data *cd, int compact);
void process_cmd_score_hostmask(struct message_data *md, struct command_data *cd, int compact);
