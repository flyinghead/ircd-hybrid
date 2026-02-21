/*
 * $Id: cf_ignore.c,v 1.8 2004/10/30 19:50:53 cfh7 REL_2_1_3 $
 *
 *   OpenChanfix v2.0 
 *
 *   Channel Re-op Service Module for Hybrid 7.0
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

static BlockHeap *ocf_ignore_heap  = NULL;

/* ----------------------------------------------------------------- */

void ocf_ignores_init()
{
	ocf_ignore_heap = BlockHeapCreate(sizeof(struct ocf_ignore), OCF_IGNORE_HEAP_SIZE);
}

/* ----------------------------------------------------------------- */

void ocf_ignores_deinit()
{
	dlink_node *ptr, *next_ptr;
	DLINK_FOREACH_SAFE(ptr, next_ptr, ocf_ignore_list.head)
	{
		BlockHeapFree(ocf_ignore_heap, ptr->data);
		dlinkDelete(ptr, &ocf_ignore_list);
	}
}

/* ----------------------------------------------------------------- */

struct ocf_ignore* ocf_add_ignore(const char* hostmask, const char* server)
{
	struct ocf_ignore *ignore_p;

	ignore_p = BlockHeapAlloc(ocf_ignore_heap);
	memset(ignore_p, 0, sizeof(struct ocf_ignore));
	strncpy(ignore_p->hostmask, hostmask, sizeof(ignore_p->hostmask) - 1);
	strncpy(ignore_p->server, server, sizeof(ignore_p->server) - 1);
	dlinkAdd(ignore_p, make_dlink_node(), &ocf_ignore_list);

	return ignore_p;
}

/* ----------------------------------------------------------------- */

int ocf_is_ignored(const char* hostmask, const char* server)
{
	dlink_node *ptr;
	struct ocf_ignore *oi;

	DLINK_FOREACH(ptr, ocf_ignore_list.head) {
		oi = ((struct ocf_ignore*)(ptr->data));
		if (match(oi->hostmask, hostmask) && 
			match(oi->server, server))
		{
			return 1;
		}
	}

	return 0;
}

/* ----------------------------------------------------------------- */

void ocf_ignore_load(const char* filename)
{
	char line[MAX_LINE_LENGTH + 1]; /* Yeah, lazy. Should work with realloc perhaps. */
	FILE* fp;
	int numignores = 0;
	char *parv[OCF_MAXPARA];
	int parc;
	
    fp = fopen(filename, "r");

    if(!fp) {
		ocf_log(OCF_LOG_MAIN, "USER Error opening ignores file %s for reading.", filename);
		ocf_log(OCF_LOG_ERROR, "USER Error opening ignores file %s for reading.", filename);
		sendto_realops_flags(ALL_UMODES, L_ALL, 
			"Error opening ignores file %s for reading.", filename);

        return;
    }

	while (readline(fp, line, MAX_LINE_LENGTH))
	{
		strtrim(line);
        if (!*line || *line == '#')
            continue;

		parc = tokenize_string_separator(line, parv, ' ');
		if (parc != 2) {
			/* log parse error on this line */
			continue;
		}

		strtrim(parv[0]);
		strtrim(parv[1]);
		if (strlen(parv[0]) > USERLEN + 1 + HOSTLEN) {
			ocf_log(OCF_LOG_MAIN, "IGNO ERROR: Too long hostmask %s",
				parv[0]);
		} else if (strlen(parv[1]) > HOSTLEN) {
			ocf_log(OCF_LOG_MAIN, "IGNO ERROR: Too long servermask %s",
				parv[1]);
		} else {
			ocf_add_ignore(parv[0], parv[1]);
			ocf_log(OCF_LOG_MAIN, "IGNO Added ignore %s [server %s]", 
				parv[0], parv[1]);
			numignores++;
		}
	}

	ocf_log(OCF_LOG_MAIN, "IGNO Added %d ignore%s.", numignores, numignores == 1 ? "" : "s");

	fclose(fp);
}

/* ----------------------------------------------------------------- */

