/*
 * $Id: cf_init.c,v 1.7 2004/08/24 18:55:22 cfh7 REL_2_1_3 $
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

void init_chanfix_remote()
{
    /* Create the remote chanfix client.
	 * This puts chanfix_remote on the unknown_list. */
    chanfix_remote = make_client(NULL);

	/* Remove chanfix_remote from the unknown_list to prevent it from
	 * being deleted 30 seconds from now. */
	/* Hm, removing it at this points breaks the whole thing. Need
	 * to remove it later on.
	if ((m = dlinkFindDelete(&unknown_list, chanfix_remote)) != NULL)
	{
		ocf_log(OCF_LOG_DEBUG, "Removed chanfix_remote from the unknown_list.");
		free_dlink_node(m);
	}
	*/

    /* Copy in the server, hostname */
    strlcpy(chanfix_remote->name, settings_get_str("nick"), sizeof(chanfix_remote->name));
    strlcpy(chanfix_remote->host, settings_get_str("host"), sizeof(chanfix_remote->host));
	strlcpy(chanfix_remote->localClient->sockhost, settings_get_str("own_host"), HOSTIPLEN);

	ocf_log(OCF_LOG_DEBUG, "nick %s; host %s, own_host %s.", settings_get_str("nick"),
		settings_get_str("host"),settings_get_str("own_host"));
}

/* ----------------------------------------------------------------- */
/* This code has almost literally been stolen from m_killhost.c */

void
kill_chanfix_client(struct Client* target_p, char* reason)
{
	char bufhost[IRCLINELEN];
	struct Client* source_p = &me;
	struct Client* client_p = &me;
	const char* inpath = client_p->name;

	if (MyConnect(target_p))
		sendto_one(target_p, ":%s!%s@%s KILL %s :%s",
			source_p->name, source_p->username, source_p->host,
			target_p->name, reason);

	if (!MyConnect(target_p))
	{
		ocf_relay_kill(client_p, source_p, target_p, inpath, reason);
		SetKilled(target_p);
	}

	snprintf(bufhost, sizeof(bufhost), "Killed (%s (%s))", source_p->name, reason);
	exit_client(client_p, target_p, source_p, bufhost);
}

/* ----------------------------------------------------------------- */
/* This code has been stolen from m_killhost.c */

void ocf_relay_kill(struct Client *one, struct Client *source_p,
                       struct Client *target_p,
                       const char *inpath,
		       const char *reason)
{
  dlink_node *ptr;
  struct Client *client_p;
  int introduce_killed_client;
  char* user; 
  
  /* LazyLinks:
   * Check if each lazylink knows about target_p.
   *   If it does, send the kill, introducing source_p if required.
   *   If it doesn't either:
   *     a) don't send the kill (risk ghosts)
   *     b) introduce the client (and source_p, if required)
   *        [rather redundant]
   *
   * Use a) if IsServer(source_p), but if an oper kills someone,
   * ensure we blow away any ghosts.
   *
   * -davidt
   */

  if(IsServer(source_p))
    introduce_killed_client = 0;
  else
    introduce_killed_client = 1;

  DLINK_FOREACH(ptr, serv_list.head)
  {
    client_p = ptr->data;
    
    if(client_p == one)
      continue;

    if( !introduce_killed_client )
    {
      if( ServerInfo.hub && IsCapable(client_p, CAP_LL) )
      {
        if(((client_p->localClient->serverMask &
             target_p->lazyLinkClientExists) == 0))
        {
          /* target isn't known to lazy leaf, skip it */
          continue;
        }
      }
    }
    /* force introduction of killed client but check that
     * its not on the server we're bursting too.. */
    else if(strcmp(target_p->servptr->name, client_p->name))
      client_burst_if_needed(client_p, target_p);

    /* introduce source of kill */
    client_burst_if_needed(client_p, source_p);

    user = target_p->name;

    if(MyClient(source_p))
      {
        sendto_one(client_p, ":%s KILL %s :%s!%s!%s!%s (%s)",
                   source_p->name, user,
                   me.name, source_p->host, source_p->username,
                   source_p->name, reason);
      }
    else
      {
        /*sendto_one(client_p, ":%s KILL %s :%s %s",
                   source_p->name, user,
                   inpath, reason);*/
        sendto_one(client_p, ":%s KILL %s :%s",
                   source_p->name, user,
                   reason);
      }
  }
}

/* ----------------------------------------------------------------- */

void OCF_connect_chanfix_with_delay(int delay)
{
	eventAdd("OCF_connect_chanfix", &OCF_connect_chanfix, NULL, delay);
}

/* ----------------------------------------------------------------- */

void OCF_connect_chanfix(void *args)
{
    struct Client *chfx;
    int fd;

	chfx = find_person(settings_get_str("nick"));
	if (chfx && IsPerson(chfx)) {
		ocf_log(OCF_LOG_DEBUG, "Killing existing CHANFIX client (%s@%s).", chfx->username, chfx->host);
		kill_chanfix_client(chfx, "This nick is in use by a service.");
	}

	chanfix_reintroduced = 0;

    /* create a socket for the server connection */ 
    if ((fd = comm_open(AF_INET, SOCK_STREAM, 0, NULL)) < 0)
    {
		ocf_log(OCF_LOG_DEBUG, "opening stream socket for chanfix client%s: %s", errno);
      /* Eek, failure to create the socket */
      report_error(L_ALL,
		   "opening stream socket for chanfix client%s: %s", "", errno);
      return;
    }

    chanfix_remote->localClient->fd = fd;
    /*
     * Set up the initial server evilness, ripped straight from
     * connect_server(), so don't blame me for it being evil.
     *   -- adrian
     */

    if (!set_non_blocking(chanfix_remote->localClient->fd))
    {
      report_error(L_ADMIN, NONB_ERROR_MSG, get_client_name(chanfix_remote, SHOW_IP), errno);
      report_error(L_OPER, NONB_ERROR_MSG, get_client_name(chanfix_remote, MASK_IP), errno);
    }

    if (!set_sock_buffers(chanfix_remote->localClient->fd, READBUF_SIZE))
    {
      report_error(L_ADMIN, SETBUF_ERROR_MSG,
		   get_client_name(chanfix_remote, SHOW_IP), errno);
      report_error(L_OPER, SETBUF_ERROR_MSG,
		   get_client_name(chanfix_remote, MASK_IP), errno);
    }

    /*
     * at this point we have a connection in progress and C/N lines
     * attached to the client, the socket info should be saved in the
     * client and it should either be resolved or have a valid address.
     *
     * The socket has been connected or connect is in progress.
     */
    SetConnecting(chanfix_remote);
	
    /* from def_fam */
    chanfix_remote->localClient->aftype = AF_INET;
    
    /* Now, initiate the connection */
    /* XXX assume that a non 0 type means a specific bind address 
     * for this connect.
     */
    ocf_log(OCF_LOG_DEBUG, "Connecting chanfix to %s port %d",
                          settings_get_str("connect_host"), settings_get_int("connect_port"));
	ocf_log(OCF_LOG_DEBUG, "Connecting chanfix, calling comm_connect_tcp");
	comm_connect_tcp(chanfix_remote->localClient->fd, 
		settings_get_str("connect_host"), settings_get_int("connect_port"),
        NULL, 0, OCF_connect_chanfix_callback, chanfix_remote, AF_INET,
        CONNECTTIMEOUT);
	ocf_log(OCF_LOG_DEBUG, "Called comm_connect_tcp");

	/* Ugly hack that removes this function from the event table if it is
	 * on there. It might be, and it might be not. Fortunately this call
	 * fails silently if the function is not in the event table. */
	eventDelete(OCF_connect_chanfix, NULL);

	return;
}

/* ----------------------------------------------------------------- */

void
OCF_connect_chanfix_callback(int fd, int status, void *data)
{
	struct Client *client_p = data;

	ocf_log(OCF_LOG_DEBUG, "[1st callback] client_p %x, chanfix_remote %x, chanfix %x",
		client_p, chanfix_remote, chanfix);

	/* First, make sure its a real client! */
	assert(client_p != NULL);
	assert(client_p->localClient->fd == fd);

	//XXXXXXXXXXXXX set_no_delay(fd);

	/* Next, for backward purposes, record the ip of the server */
	memcpy(&client_p->localClient->ip, &fd_table[fd].connect.hostaddr,
		sizeof(struct irc_sockaddr));
	/* Check the status */
	if (status != COMM_OK)
	{
		/* We have an error, so report it and quit
		* Admins get to see any IP, mere opers don't *sigh*
		*/
		sendto_realops_flags(ALL_UMODES, L_ADMIN,
							  "Error connecting chanfix client: %s",
							  comm_errstr(status));

		/* If a fd goes bad, call dead_link() the socket is no
		 * longer valid for reading or writing.
		 */
		dead_link_on_write(client_p, 0);
		return;
	}

    /* COMM_OK, so continue the connection procedure */
	issue_chanfix_command("NICK %s", settings_get_str("nick"));
	issue_chanfix_command("USER %s %s %d :%s", 
		settings_get_str("user"), settings_get_str("host"), 0, settings_get_str("real"));

    /* If we get here, we're ok, so lets start reading some data */
    comm_setselect(fd, FDLIST_SERVER, COMM_SELECT_READ, OCF_read_packet,
                   client_p, 0);
}

/* ----------------------------------------------------------------- */

/*
 * read_packet - Read a 'packet' of data from a connection and process it.
 */

void
OCF_read_packet(int fd, void *data)
{
	char readBuf[READBUF_SIZE];
	int i;
	int num_msgs;
	struct Client *client_p = data;
	int length = 0;
	int fd_r = 0;
	SetClient(client_p);
	if (IsDefunct(client_p))
		return;

	fd_r = client_p->localClient->fd;
#ifndef HAVE_SOCKETPAIR
	if (HasServlink(client_p))
	{
		assert(client_p->localClient->fd_r > -1);
		fd_r = client_p->localClient->fd_r;
	}
#endif

  /*
   * Read some data. We *used to* do anti-flood protection here, but
   * I personally think it makes the code too hairy to make sane.
   *     -- adrian
   */
  length = recv(fd_r, readBuf, READBUF_SIZE, 0);

  if (length <= 0)
  {
    if ((length == -1) && ignoreErrno(errno))
    {
      comm_setselect(fd_r, FDLIST_IDLECLIENT, COMM_SELECT_READ,
                     OCF_read_packet, client_p, 0);
      return;
    }  	

    dead_link_on_read(client_p, length);
    return;
  }


  if (client_p->lasttime < CurrentTime)
    client_p->lasttime = CurrentTime;
  if (client_p->lasttime > client_p->since)
    client_p->since = CurrentTime;

  num_msgs = process_received_messages(readBuf, length);

  for (i = 0; i < length; i++){
    readBuf[i] = 0;
  }
  
  /* Attempt to parse what we have 
  parse_client_queued(client_p); */

  /* server fd may have changed */
  fd_r = client_p->localClient->fd;

  if (!IsDefunct(client_p))
  {
    /* If we get here, we need to register for another COMM_SELECT_READ */
    if (PARSE_AS_SERVER(client_p))
    {
      comm_setselect(fd_r, FDLIST_SERVER, COMM_SELECT_READ,
		     OCF_read_packet, client_p, 0);
    }
    else
    {
      comm_setselect(fd_r, FDLIST_IDLECLIENT, COMM_SELECT_READ,
		     OCF_read_packet, client_p, 0);
    }
  }

	if (!chanfix_reintroduced) {
		check_for_chanfix();
	}
}


/* ----------------------------------------------------------------- */

void check_for_chanfix()
{
 	dlink_node *m;
	
	if (!chanfix) {
		chanfix = find_person(settings_get_str("nick"));
	}

	if (!chanfix) {
		ocf_log(OCF_LOG_DEBUG, "Couldn't find chanfix locally.");
		return;
	}

	ocf_log(OCF_LOG_DEBUG, "Found chanfix locally (%s@%s) [%d hops].",
				   chanfix->username, chanfix->host, chanfix->hopcount);

	/* Now test if this client is on our server or not. If it's on
	 * a different server, it's some joker who took our nick, so we'll
	 * kill them. */
	if (chanfix->hopcount > 0) {
		kill_chanfix_client(chanfix, "This nick is in use by a service.");
		chanfix = NULL;
		return;
	}

	/* If the hopcount is zero, we assume that this client is the
	 * real chanfix client. This means that any oper on the OpenChanfix
	 * server could confuse it by /kill'ing the OpenChanfix client and
	 * quickly taking its nick. This needs improvement but it's not
	 * critical. */
	strlcpy(chanfix->username, settings_get_str("user"), sizeof(chanfix->username));
	strlcpy(chanfix->host, settings_get_str("host"), sizeof(chanfix->host));
	strlcpy(chanfix->info, settings_get_str("real"), sizeof(chanfix->info));
	chanfix->hopcount = 0;
	chanfix->tsinfo = 1;
	chanfix->firsttime = 1;
	oper_up(chanfix);

	/* Remove chanfix from the unknown_list if it's there */
	if ((m = dlinkFindDelete(&unknown_list, chanfix)) != NULL)
	{
		ocf_log(OCF_LOG_DEBUG, "Removed chanfix from the unknown_list.");
		free_dlink_node(m);
	}
	
	if ((m = dlinkFindDelete(&unknown_list, chanfix_remote)) != NULL)
	{
		ocf_log(OCF_LOG_DEBUG, "Removed chanfix_remote from the unknown_list.");
		free_dlink_node(m);
	}

	/* Now to reintroduce the chanfix client to the rest of the network,
	 * because the user, host and realname data has changed.
	 * To accomplish that, we send a QUIT and a NICK to all servers. 
	 * NOTE THAT THIS BREAKS IF WE HAVE CAP_UID AND OUR UPLINK TOO !
	 */

	sendto_server(NULL, NULL, NULL, NOCAPS, NOCAPS, NOFLAGS,
		":%s QUIT :brb", chanfix->name);

    sendto_server(NULL, NULL, NULL, NOCAPS, CAP_LL,
                  NOFLAGS, "NICK %s %d %lu +io %s %s %s :%s",
                  chanfix->name,
                  chanfix->hopcount+1,
                  (unsigned long) chanfix->firsttime,
                  chanfix->username, chanfix->host,
                  me.name, chanfix->info);

	if (!sent_init_message) {
		sent_init_message = 1;
		send_notice_to_loggedin(OCF_NOTICE_FLAGS_ALL, 
			"Loaded OpenChanfix module %s (the %s release). You are still logged in.",
			OCF_VERSION, OCF_VERSION_NAME);
	}

	chanfix_reintroduced = 1;
}

/* ----------------------------------------------------------------- */
