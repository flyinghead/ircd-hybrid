/*
 * $Id: cf_settings.h,v 1.6 2004/08/24 22:21:52 cfh7 REL_2_1_3 $
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




/*
 * Flexible settings functions. Each setting is stored by its
 * unique name. There are 3 types of settings:
 * - integer
 * - boolean
 * - string
 * Settings can be added, retrieved and set.
 * A function to read a config file with lines in the syntax 
 * "setting = value" is provided as well.
 */

#define SETTING_NAME_LEN 32
#define SETTING_VALUE_LEN 256
#define SETTING_FILE_LEN 256

#define SETTING_TYPE_INT 1
#define SETTING_TYPE_BOOL 2
#define SETTING_TYPE_STR 3

#define SETTING_HEAP_SIZE 1024

/* A single setting item, either a string, an int or a bool. */
struct setting_item
{
	int type;
	char name[SETTING_NAME_LEN];
	char strvalue[SETTING_VALUE_LEN];
	int intvalue;
	int boolvalue;
	char file[SETTING_FILE_LEN];
	int linenum;
};

dlink_list setting_list;

/* Inits the settings heap, and a load of default chanfix settings. 
 * You MUST call this function before using the other settings_*
 * functions. */
void settings_init();

/* Deletes all settings and deinits the settings heap.
 * You MUST call this function for proper cleanup. */
void settings_deinit();

/* Reads the chanfix config file and sets those settings */
void settings_read_file();

/* Gets the setting out of a "key = value" line. */
int get_setting_info(char *line, char* name, int* intvalue, int* boolvalue, char* strvalue);

/* Adds a new setting with name <name> and value <value>. If a setting
 * with this name already exists, nothing happens.
 * TODO: Proper error logging. 
 */
void settings_add_int(const char* name, int value);
void settings_add_bool(const char* name, int value);
void settings_add_str(const char* name, char* value);

/* Retrieves the value of <name>. Returns 0 or an empty string
 * if the gives setting does not exist, or is of a different type.
 * TODO: Proper error logging. 
 */
char* settings_get_str(const char* name);
int settings_get_int(const char* name);
int settings_get_bool(const char* name);

/* Sets setting <name> to value <value>. If the setting does not exist,
 * it will be created. If the setting is of a different type, nothing
 * happens.
 * TODO: Proper error logging. 
 */
void settings_set_int(const char* name, int value);
void settings_set_bool(const char* name, int value);
void settings_set_str(const char* name, char* value);

/* Deletes a setting. */
void settings_del(const char* name);

/* Returns true if there is a setting with this name */
int settings_exists(const char* name);

/* Don't call these directly. */
void _settings_add(int type, const char* name, int intvalue, int boolvalue, char* strvalue);
void _settings_set(int type, const char* name, int intvalue, int boolvalue, char* strvalue);


