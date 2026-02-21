/*
 * $Id: cf_settings.c,v 1.10 2004/09/30 19:57:44 cfh7 REL_2_1_3 $
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

static BlockHeap *setting_heap  = NULL;

/* ----------------------------------------------------------------- */

void settings_init()
{
	setting_heap = BlockHeapCreate(sizeof(struct setting_item), SETTING_HEAP_SIZE);

	settings_add_str("nick",                       OCF_NICK);
	settings_add_str("user",                       OCF_USER);
	settings_add_str("host",                       OCF_HOST);
	settings_add_str("real",                       OCF_REAL);

	settings_add_str("own_host",                   OCF_OWN_HOST);
	settings_add_int("connect_port",               OCF_CONNECT_PORT);
	settings_add_str("connect_host",               OCF_CONNECT_HOST);

	settings_add_int("min_clients",                OCF_MINCLIENTS);
	settings_add_bool("client_needs_ident",        1);
	settings_add_bool("client_needs_reverse",      1);

	settings_add_int("max_channel_modes",          MAX_CHANNEL_MODES);
	settings_add_int("num_servers",                2);
	settings_add_int("min_servers_present",        MIN_SERVERS_PRESENT);

	settings_add_int("f_flags_per_server",         0);
	settings_add_int("b_flags_per_server",         0);

	settings_add_bool("enable_autofix",            1);
	settings_add_bool("enable_chanfix",            1);
	settings_add_bool("enable_channel_blocking",   1);

	settings_add_int("num_top_scores",             OCF_NUM_TOP_SCORES);
	settings_add_bool("send_notices",              0);
	settings_add_int("num_clients_opped",          OCF_NUM_CLIENTS_OPPED);

	settings_add_int("autofix_num_opped",          OCF_AUTOFIX_NUM_OPPED);
	settings_add_int("chanfix_num_opped",          OCF_CHANFIX_NUM_OPPED);

	settings_add_int("channels_per_run",           OCF_CHANNELS_PER_RUN);

	settings_add_str("userfile",                   OCF_USERFILE);
	settings_add_str("ignorefile",                 OCF_IGNOREFILE);
	settings_add_str("dbpath",                     OCF_DBPATH);

	settings_add_str("logfile_chanfix",            OCF_LOGFILE_CHANFIX);
	settings_add_str("logfile_error",              OCF_LOGFILE_ERROR);
	settings_add_str("logfile_debug",              OCF_LOGFILE_DEBUG);

	settings_add_bool("enable_cmd_score_nick",     1);
	settings_add_bool("enable_cmd_score_userhost", 1);
	settings_add_bool("enable_cmd_check",          1);
	settings_add_bool("enable_cmd_opnicks",        1);
	settings_add_bool("enable_cmd_oplist",         1);
	settings_add_bool("enable_cmd_history",        1);
}

/* ----------------------------------------------------------------- */

void settings_deinit()
{
	dlink_node *ptr, *next_ptr;
	DLINK_FOREACH_SAFE(ptr, next_ptr, setting_list.head)
	{
		BlockHeapFree(setting_heap, ptr->data);
		dlinkDelete(ptr, &setting_list);
	}
	/* TODO: check whether setting_heap itself needs to be Free()ed */
	/* 
	BlockHeapFree(setting_heap);
	*/
}

/* ----------------------------------------------------------------- */

void settings_add_int(const char* name, int value)
{
	_settings_add(SETTING_TYPE_INT, name, value, 0, NULL);
}

/* ----------------------------------------------------------------- */

void settings_add_bool(const char* name, int value)
{
	_settings_add(SETTING_TYPE_BOOL, name, 0, value, NULL);
}

/* ----------------------------------------------------------------- */

void settings_add_str(const char* name, char* value)
{
	_settings_add(SETTING_TYPE_STR, name, 0, 0, value);
}

/* ----------------------------------------------------------------- */
/* Don't call this directly! Use settings_add_(int|str|bool)! */

void _settings_add(int type, const char* name, int intvalue, int boolvalue, char* strvalue)
{
	struct setting_item *setting_p;

	/* Don't add anything if the setting already exists. */
	if (settings_exists(name)) {
		/* log error */
		return;
	}

	if (type != SETTING_TYPE_INT &&
			type != SETTING_TYPE_BOOL &&
			type != SETTING_TYPE_STR) 
	{
		/* log error of adding invalid type */
		return;
	}

	setting_p = BlockHeapAlloc(setting_heap);
	memset(setting_p, 0, sizeof(struct setting_item));

	setting_p->type = type;
	strncpy(setting_p->name, name, sizeof(setting_p->name) - 1);

	switch(type){
	case SETTING_TYPE_INT: 
		setting_p->intvalue = intvalue;
		break;
	case SETTING_TYPE_BOOL: 
		setting_p->boolvalue = boolvalue;
		break;
	case SETTING_TYPE_STR: 
		strncpy(setting_p->strvalue, strvalue, sizeof(setting_p->strvalue) - 1);
		break;
	default:
		setting_p->type = SETTING_TYPE_STR;
		strncpy(setting_p->strvalue, "INVALID SETTING TYPE", sizeof(setting_p->strvalue) - 1);
	}
	
	dlinkAdd(setting_p, make_dlink_node(), &setting_list);
}

/* ----------------------------------------------------------------- */

void settings_set_int(const char* name, int value)
{
	_settings_set(SETTING_TYPE_INT, name, value, 0, NULL);
}

/* ----------------------------------------------------------------- */

void settings_set_bool(const char* name, int value)
{
	_settings_set(SETTING_TYPE_BOOL, name, 0, value, NULL);
}

/* ----------------------------------------------------------------- */

void settings_set_str(const char* name, char* value)
{
	_settings_set(SETTING_TYPE_STR, name, 0, 0, value);
}

/* ----------------------------------------------------------------- */
/* Don't call this directly! Use settings_set_(int|str|bool)! */

void _settings_set(int type, const char* name, int intvalue, int boolvalue, char* strvalue)
{
	dlink_node *ptr;
	struct setting_item *si;

	if (type != SETTING_TYPE_INT &&
			type != SETTING_TYPE_BOOL &&
			type != SETTING_TYPE_STR) 
	{
		/* log error of setting invalid type */
		return;
	}

	/* Walking through the whole linked list to find the item.
	 * Sure, this is slow, but with under 100 settings and only
	 * asking for them once every few seconds, it doesn't really
	 * matter.
	 * TODO: For speed increase, change the list into a tree. 
	 */
	DLINK_FOREACH(ptr, setting_list.head)
	{
		si = ((struct setting_item*)(ptr->data));
		if (strcmp(si->name, name) == 0)
		{
			if (si->type == type) {
				switch (type) {
				case SETTING_TYPE_INT: 
					si->intvalue = intvalue; 
					break;
				case SETTING_TYPE_BOOL: 
					si->boolvalue = boolvalue; 
					break;
				case SETTING_TYPE_STR: 
					strncpy(si->strvalue, strvalue, sizeof(si->strvalue)); 
					si->strvalue[sizeof(si->strvalue)-1] = 0; /* Just in case */
					break;
				}
			} else {
				ocf_log(OCF_LOG_ERROR, 
					"Setting: trying to set type %d, while setting %s is of type %d.",
					type, name, si->type);
			}
			return;
		}
	}
	
	/* log warning of nonexistant setting */
	/* The setting does not exist, so perform the implicit add. */
	_settings_add(type, name, intvalue, boolvalue, strvalue);
}

/* ----------------------------------------------------------------- */

int settings_get_int(const char* name)
{
	dlink_node *ptr;
	struct setting_item *si;

	DLINK_FOREACH(ptr, setting_list.head)
	{
		si = ((struct setting_item*)(ptr->data));
		if (strcmp(si->name, name) == 0) {
			if (si->type == SETTING_TYPE_INT) {
				return(si->intvalue);
			} else {
				ocf_log(OCF_LOG_ERROR, 
					"Setting: trying to get type int, while setting %s is of type %d.",
					si->type, name);
				return 0;
			}
		}
	}
	
	return 0;
}

/* ----------------------------------------------------------------- */

int settings_get_bool(const char* name)
{
	dlink_node *ptr;
	struct setting_item *si;

	DLINK_FOREACH(ptr, setting_list.head)
	{
		si = ((struct setting_item*)(ptr->data));
		if (strcmp(si->name, name) == 0) {
			if (si->type == SETTING_TYPE_BOOL) {
				return(si->boolvalue);
			} else {
				ocf_log(OCF_LOG_ERROR, 
					"Setting: trying to get type bool, while setting %s is of type %d.",
					si->type, name);
				return 0;
			}
		}
	}
	
	return 0;
}

/* ----------------------------------------------------------------- */

char* settings_get_str(const char* name)
{
	dlink_node *ptr;
	struct setting_item *si;

	DLINK_FOREACH(ptr, setting_list.head)
	{
		si = ((struct setting_item*)(ptr->data));
		if (strcmp(si->name, name) == 0) {
			if (si->type == SETTING_TYPE_STR) {
				return(si->strvalue);
			} else {
				ocf_log(OCF_LOG_ERROR, 
					"Setting: trying to get type str, while setting %s is of type %d.",
					si->type, name);
				return 0;
			}
		}
	}
	
	return 0;
}

/* ----------------------------------------------------------------- */

void settings_del(const char* name)
{
	dlink_node *ptr;
	struct setting_item *si;

	DLINK_FOREACH(ptr, setting_list.head)
	{
		si = ((struct setting_item*)(ptr->data));
		if (strcmp( si->name, name) == 0)
		{
			BlockHeapFree(setting_heap, ptr->data);
			dlinkDelete(ptr, &setting_list);
			return;
		}
	}
}

/* ----------------------------------------------------------------- */

int settings_exists(const char* name)
{
	dlink_node *ptr;
	struct setting_item *si;

	DLINK_FOREACH(ptr, setting_list.head)
	{
		si = ((struct setting_item*)(ptr->data));
		if (strcmp(si->name, name) == 0)
			return 1;
	}
	
	return 0;
}

/* ----------------------------------------------------------------- */

void settings_read_file()
{
	char line[MAX_LINE_LENGTH + 1]; /* Yeah, lazy. Should work with realloc perhaps. */
	FILE *fp;
	int ignore = 0;
	int linenum = 0;
	char name[SETTING_NAME_LEN];
	int intvalue;
	int boolvalue;
	char strvalue[SETTING_VALUE_LEN];
	int settingtype;
	
    fp = fopen(SETTING_FILENAME, "r");

    if(!fp) {
		ocf_log(OCF_LOG_DEBUG, "Error opening config file %s", SETTING_FILENAME);
		sendto_realops_flags(ALL_UMODES, L_ALL, 
			"Error opening config file %s for reading.", SETTING_FILENAME);
        return;
    }

	while (readline(fp, line, MAX_LINE_LENGTH))
	{
        linenum++;

        if (!*line || *line == '#')
            continue;

        if (ignore) {
            if(!strncmp(line, "#*/", 3))
                ignore = 0;
            continue;
        }

        if (!strncmp(line, "#/*", 3)) {
            ignore = 1;
            continue;
        }

		settingtype = get_setting_info(line, name, &intvalue, &boolvalue, strvalue);
		
		if (settingtype == -1){
			/* log invalid setting */
			continue;
		}

		switch (settingtype) {
		case SETTING_TYPE_INT: 
			ocf_log(OCF_LOG_DEBUG, "int : %s = %d", name, intvalue);
			settings_set_int(name, intvalue);
			break;
		case SETTING_TYPE_BOOL: 
			ocf_log(OCF_LOG_DEBUG, "bool: %s = %d", name, boolvalue);
			settings_set_bool(name, boolvalue);
			break;
		case SETTING_TYPE_STR: 
			ocf_log(OCF_LOG_DEBUG, "str : %s = %s", name, strvalue);
			settings_set_str(name, strvalue);
			break;
		}
	}

	fclose(fp);
	ocf_log(OCF_LOG_DEBUG, "Done reading settings. Read %d lines.", linenum);
}

/* ----------------------------------------------------------------- */
/* Reads <line> and checks whether it consists of a name-value settings
 * pair. If so, it sets the name and the appropriate of the three value
 * settings, and returns the type of the setting that it has chosen.
 * If an error occurs, -1 is returned. 
 * Note that only the value corresponding to the type contains useful
 * data. The other 2 may or may not contain utter crap.
 */

int get_setting_info(char *line, char* name, int* intvalue, int* boolvalue, char* strvalue)
{
	char *cptr, *walk;
	int i;
	int force_type = 0;
	/* Line must contain at least one space. */

	if ((cptr = strchr(line, ' ')) == NULL) {
		return -1;
	}

	walk = line;

	/* Check for forcing a setting type by s:setting = value */
	if (*(walk + 1) == ':') {
		switch (*walk) {
		case 'i': force_type = SETTING_TYPE_INT; break;
		case 'b': force_type = SETTING_TYPE_BOOL; break;
		case 's': force_type = SETTING_TYPE_STR; break;
		default: return -1;
		}
		walk += 2;
	}

	/* Ignore spaces left of the setting name */
	while (*walk == ' ') {
		walk++;
	}

	/* Get the setting name */
	i = 0;
	while (walk != cptr) {
		name[i] = *walk;
		walk++;
		i++;
	}
	name[i] = 0;
	
	/* Ignore spaces right of the setting name */
	while (*walk == ' ') {
		walk++;
	}

	/* Ignore equal sign if present */
	if (*walk == '=') {
		walk++;
	}

	/* Ignore spaces left of the setting value */
	while (*walk == ' ') {
		walk++;
	}

	/* Get the setting value */
	i = 0;
	while (*walk != 0) {
		strvalue[i] = *walk;
		walk++;
		i++;
	}
	strvalue[i] = 0;

	/* If type forced, set and return. */
	if (force_type) {
		switch(force_type) {
		case SETTING_TYPE_INT: 
			*intvalue = atoi(strvalue);
			break;
		case SETTING_TYPE_BOOL: 
			*boolvalue = atoi(strvalue);
			if (*boolvalue != 0) {
				*boolvalue = 1;
			}
			break;
		case SETTING_TYPE_STR: 
			break;
		}

		return force_type;
	}

	/* Guess the best type. */
	/* "on" or "1" becomes a bool true. */
	if ( ( (strncasecmp(strvalue, "on", 2) == 0) && strlen(strvalue) == 2) ||
		 ( (*strvalue == '1') && (strlen(strvalue) == 1)) )
	{
		*boolvalue = 1;
		return SETTING_TYPE_BOOL;
	}

	/* "off" or "0" becomes a bool false. */
	if ( ( (strncasecmp(strvalue, "off", 3) == 0) && strlen(strvalue) == 3) ||
		 ( (*strvalue == '0') && (strlen(strvalue) == 1)) )
	{
		*boolvalue = 0;
		return SETTING_TYPE_BOOL;
	}

	/* As soon as there is a non-digit, the value is a string. */
	i = 0;
	while (strvalue[i] != 0){
		if (!isdigit(strvalue[i])) {
			return SETTING_TYPE_STR;
		}
		i++;
	}

	/* Only digits, so it's an integer. */
	*intvalue = atoi(strvalue);
	return SETTING_TYPE_INT;
}

/* ----------------------------------------------------------------- */

