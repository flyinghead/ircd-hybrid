/*
 * $Id: cf_main.c,v 1.13 2004/08/28 08:01:42 cfh7 REL_2_1_3 $
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

static void mo_chanfix(struct Client *client_p, struct Client *source_p,
                    int parc, char *parv[]);

struct Message Cmsgtab = {
  "CHANFIX", 0, 0, 1, 0, MFLG_SLOW, 0,
  {m_ignore, m_ignore, m_ignore, mo_chanfix}
};

/* ----------------------------------------------------------------- */

/* From packet.h */
#define MAX_FLOOD 5
#define MAX_FLOOD_BURST MAX_FLOOD * 8


/* ----------------------------------------------------------------- */
#undef STATIC_MODULES
#ifndef STATIC_MODULES

/* ----------------------------------------------------------------- */

void _modinit(void)
{
	time(&module_loaded_time);
	current_day_since_epoch = get_days_since_epoch();

	currently_split = 1;
	send_notices_instead_of_privmsgs = 0;
	sent_init_message = 0;

	/* This will add the commands in test_msgtab (which is above) */
	mod_add_cmd(&Cmsgtab);

	/* Init and load the settings */
	settings_init();
	ocf_log_init();
	settings_read_file();
	send_notices_instead_of_privmsgs = settings_get_bool("send_notices");
	ocf_logfiles_open();
	ocf_log(OCF_LOG_MAIN, "MAIN Loading OpenChanfix module.");
	ocf_users_init();
	ocf_ignores_init();

	/* Loads the database; will create a new one if no database
	 * can be found. */
	DB_load();
	DB_set_day(current_day_since_epoch);

	init_chanfix_remote();
	OCF_connect_chanfix_with_delay(OCF_CONNECT_DELAY);

	add_update_database_event();
	add_save_database_event();
	eventAdd("ocf_check_split",           &ocf_check_split, NULL, 10);
	eventAdd("ocf_check_opless_channels", &ocf_check_opless_channels, NULL, 10);
	eventAdd("ocf_fix_chanfix_channels",  &ocf_fix_chanfix_channels, NULL, 5);
	eventAdd("ocf_fix_autofix_channels",  &ocf_fix_autofix_channels, NULL, 5);
	eventAdd("ocf_check_loggedin_users", 
		&ocf_check_loggedin_users, NULL, OCF_USER_LOGGEDIN_INTERVAL * 60);

	ocf_user_load_all(settings_get_str("userfile"));
	ocf_user_load_loggedin();
	ocf_ignore_load(settings_get_str("ignorefile"));
	ocf_log(OCF_LOG_MAIN, "MAIN Initialisation complete.");
}

/* ----------------------------------------------------------------- */

void _moddeinit(void)
{
	ocf_log(OCF_LOG_MAIN, "MAIN Unloading OpenChanfix module.");
	
	send_notice_to_loggedin(OCF_NOTICE_FLAGS_ALL, "Saving Database to disk.");
	DB_save();
	send_notice_to_loggedin(OCF_NOTICE_FLAGS_ALL, "Done. Unloaded OpenChanfix module.");

	ocf_user_save_loggedin();
	ocf_user_save_all(settings_get_str("userfile"));
	mod_del_cmd(&Cmsgtab);
	eventDelete(OCF_connect_chanfix, NULL);
	eventDelete(ocf_update_database, NULL);
	eventDelete(ocf_save_database, NULL);
	eventDelete(ocf_check_split, NULL);
	eventDelete(ocf_check_opless_channels, NULL);
	eventDelete(ocf_fix_chanfix_channels, NULL);
	eventDelete(ocf_fix_autofix_channels, NULL);
	eventDelete(ocf_check_loggedin_users, NULL);
	settings_deinit();
	ocf_users_deinit();
	ocf_logfiles_close();

	if (chanfix) {
		exit_client(chanfix, chanfix, &me, "Unloading OpenChanfix module.");
	}
	
	if (chanfix_remote)
	{
		if (chanfix_remote->localClient->ctrlfd > -1)
		{
			fd_close(chanfix_remote->localClient->ctrlfd);
			chanfix_remote->localClient->ctrlfd   = -1;
			#ifndef HAVE_SOCKETPAIR
				fd_close(chanfix_remote->localClient->ctrlfd_r);
				fd_close(chanfix_remote->localClient->fd_r);
				chanfix_remote->localClient->ctrlfd_r = -1;
				chanfix_remote->localClient->fd_r     = -1;
			#endif
		}
		free_client(chanfix_remote);
	}
}

const char *_version = OCF_VERSION;
#endif

/* ----------------------------------------------------------------- */

/*
 * mo_chanfix
 *      parv[0] = sender prefix
 *      parv[1] = parameter
 */
static void mo_chanfix(struct Client *client_p, struct Client *source_p,
                   int parc, char *parv[])
{
	/* /CHANFIX connect */
	if(parc == 2) {
		if (strcmp(parv[1], "connect") == 0) {
			OCF_connect_chanfix(NULL);
		} else {
    		sendto_one(source_p, 
				":%s NOTICE %s :Use /CHANFIX CONNECT to connect the chanfix client.",
				me.name, source_p->name);
		}
	} else {
    	sendto_one(source_p, 
			":%s NOTICE %s :Use /CHANFIX CONNECT to connect the chanfix client.",
			me.name, source_p->name);
	}
}

/* ----------------------------------------------------------------- */


/* END OF CHANFIX initialisation module */
