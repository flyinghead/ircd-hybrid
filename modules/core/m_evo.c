
/*  @evo -> RF this is a new module specific for evo
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  m_evo.c: 4x4 Evolution eircd module
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: m_evo.c 313 2008-05-17 10:55:47Z tom $
 */
#include "stdinc.h"
#include "list.h"
#include "client.h"
#include "hash.h"
#include "ircd.h"
#include "numeric.h"
#include "log.h"
#include "send.h"
#include "irc_string.h"
#include "parse.h"
#include "modules.h"
#include "channel_mode.h"
#include "conf.h"
#include "server.h"

#define	strncpyzt(x, y, N) do{(void)strncpy(x,y,N);x[N-1]='\0';}while(0)

/* Define this if you're making an Evo2 server */
#define EVO2_SERVER
static dlink_node *prev_hook;

static time_t last_used = 0;

#ifdef STATIC_MODULES
extern void *evo_check_auth(va_list);
#else
static void *evo_check_auth(va_list);
#endif
extern void *evo_check_nick(va_list);

extern void m_evo_not_oper(struct Client *, struct Client *, int, char *[]);

//#define USE_MORE_CODE
#ifdef USE_MORE_CODE
extern void warnLikelyHackerDetails(struct Client *);
extern int checkBootHacker(struct Client *, struct Client *);
extern int checkRealName(const char *, struct Client *);
#endif

static void m_usrip(struct Client *, int, char **);
static void m_rchg(struct Client *, int, char **);
static void m_rlst(struct Client *, int, char **);
static void ms_rlst(struct Client *, int, char **);
static void m_broadcast(struct Client *, int, char **);
static void m_ban(struct Client *, struct Client *, int, char **);


#ifndef STATIC_MODULES


#endif

/* m_evo_not_oper()
 * inputs	- 
 * output	-
 * side effects	- just returns a nastyogram to given user
 */
void m_evo_not_oper(struct Client *client_p, struct Client *source_p,
               int parc, char *parv[])
{
  sendto_realops_flags(UMODE_ALL, L_ALL,
                       "HACKER ALERT: Non-Oper used %s command.", parv[0]);
#ifdef USE_MORE_CODE
  warnLikelyHackerDetails(source_p);
#endif
}
/* evo_check_auth
 *
 * inputs	- client
 * output	- inherited
 * side effects	- kicks any non-evo clients
 */
void *
evo_check_auth(va_list args)
{
 struct Client *source_p = va_arg(args, struct Client *);
 const char *username = va_arg(args, const char *);

//TODO: check connecting clients for foreign clients

#ifndef STATIC_MODULES
  //return pass_callback(prev_hook, source_p, username);
#else
  return 1;
#endif
}

/* evo_check_nick
 *
 * inputs	- client
 * output	- inherited
 * side effects	- kicks any non-evo nicks
 */
void *
evo_check_nick(va_list args)
{
  struct Client *source_p = va_arg(args, struct Client *);
  char *nick = va_arg(args, char *);
  char *temp = nick;
  int nlen, i;

  ilog(6, "evo_check_nick() - NICK: %s", nick);

  nlen = strlen(nick);
  if (nlen != 12)
  {
    exit_client(source_p, "Invalid nick length");
    return NULL;
  }

  // test if nick has correct letters in it
  for (i = 0; i < nlen; i++)
  {
    if (strcspn( temp, "ABCDEFGHIJKLMNOP" ) > 0 )
    {
      exit_client(source_p, "Invalid nick");
      //break;
      return NULL;
    }
    temp++;
  }
  
  return 0;
}


// m_usrip()

//static void m_usrip(struct Client *source_p, int parc, char *parv[])
static void m_usrip(struct Client *source_p,
      int parc, char *parv[])
{
  struct Client *target_p;

  //char buf[IRCD_BUFSIZE];
  //char response[NICKLEN*2+USERLEN+HOSTLEN+30];
  //char *t = NULL, *p = NULL, *nick = NULL;
  //int i, n;          
  //int cur_len;
  //int rl;
  
  //cur_len = snprintf(buf, sizeof(buf), numeric_form(RPL_USERHOST), me.name, source_p->name, "");
 // t = buf + cur_len;

  if (IsUnknown(source_p))
  {

    //rl = snprintf(response, sizeof(buf), "unknown=+unknown@%s ",source_p->sockhost);
    //snprintf(t, sizeof(buf), "%s", response);
   // t += rl;
    //cur_len += rl;
    source_p->connection->challengeValue = rand() ^ (rand() << 8) ^ (rand() << 16) ^ (rand() << 24);
    sendto_one(source_p,"CHALLENGE %x", source_p->connection->challengeValue);
    //sendto_one(source_p,"%s", buf);
    sendto_one(source_p,":%s 302 unknown=+unknown@%s", me.name, source_p->sockhost);
   }
  return;
}

/*
** m_rchg()
**      parv[0] = sender prefix
**      parv[1] = real name info
*/
static void m_rchg(struct Client *source_p,
       int parc, char *parv[])
{
		
	const char *fromNick = NULL;
	const char *realName = NULL;
	char hold[100];

	if (source_p == NULL)
		return;

	if (parc != 2)
  	{
		return;
	}
	realName = parv[1];
	if (realName == NULL)
  	{
	        sendto_one_numeric(source_p, &me, ERR_NEEDMOREPARAMS, "RCHG");
		return;
	}

	/*if (IsServer(client_p))
  	{
		fromNick = parv[0];
	} else {
		if (!IsRegistered(client_p) || !MyClient(client_p) || (client_p != source_p)) {
			return;
		}
		fromNick = source_p->name;
	}

	if (EmptyString(fromNick)) {
		return;
	}

	*/
	
	strncpyzt(source_p->info, realName, sizeof(source_p->info));

        //sendto_realops_flags(UMODE_ALL, L_ALL, SEND_NOTICE, "%s", source_p->info);
	strcpy(hold, source_p->info);
	if (!HasOFlag(source_p, OPER_FLAG_OPME)) {
                if (hold[0] == '^') {
                        sendto_realops_flags(UMODE_ALL, L_ALL,"%s (%s@%s) [%s] IP = %s Blank Name", 
                                source_p->name, source_p->username, source_p->host, source_p->info, source_p->sockhost);
                        exit_client(source_p, "Restricted nickname");
                        return;
                }
                if (hold[0] == ' ') {
                        sendto_realops_flags(UMODE_ALL, L_ALL,"%s (%s@%s) [%s] IP = %s Blank Name", 
                                source_p->name, source_p->username, source_p->host, source_p->info, source_p->sockhost);
                        exit_client(source_p, "Restricted nickname");
                        return;
                }
                if (hold[0] == '�') {
                        sendto_realops_flags(UMODE_ALL, L_ALL,"%s (%s@%s) [%s] IP = %s Blank Name", 
                                source_p->name, source_p->username, source_p->host, source_p->info, source_p->sockhost);
                        exit_client(source_p, "Restricted nickname");
                        return;
                }
        }

	sendto_common_channels_local(source_p, 1, 0, ":%s RCHG :%s", source_p->name, realName);
	
	sendto_server(source_p, 0, 0, 
                ":%s RCHG :%s", source_p->name, realName);	
        
        //hash_del_client(source_p);
	strlcpy(source_p->info, realName, sizeof(source_p->info));
        //hash_add_client(source_p);
       // fd_note(&source_p->connection->fd, "RCHG: %s", realName);

}

/*
** m_rlst
**      parv[0] = sender prefix
**      parv[1] = nickname mask list
*/
static void m_rlst(struct Client *source_p,
       int parc, char *parv[]) {
	// For now, let's just treat this as a who message.
	// It's extra bandwidth to send since the who command has more
	// info than we need, but for now we won't worry about it

   return;
   //return m_who(client_p, source_p, parc, parv); //sponji
}

/*
** m_rlst
**      parv[0] = sender prefix
**      parv[1] = nickname mask list
*/
static void ms_rlst(struct Client *source_p,
       int parc, char *parv[])
{
	// For now, let's just treat this as a who message.
	// It's extra bandwidth to send since the who command has more
	// info than we need, but for now we won't worry about it

  return;
//return ms_who(client_p, source_p, parc, parv); //sponji
}
/*
** m_broadcast
**      parv[0] = sender prefix
**      parv[1] = message
*/
/*
** m_broadcast
**      parv[0] = sender prefix
**      parv[1] = message
*/
static void m_broadcast(struct Client *source_p, 
	int parc, char *parv[]) 
{
  	sendto_match_butone(source_p->from, source_p, "*", MATCH_HOST, "BROADCAST :%s", parv[1]);
}

/*
** m_ban
**      parv[0] = sender prefix
**      parv[1] = channel name
**      parv[2] = nickname
**      parv[3] = duration (0 or omitted if ban does not expire)
*/
//TODO: actually check the ban expire times when some1 tries to join..
static void m_ban(struct Client *client_p, struct Client *source_p,
      int parc, char *parv[]) {
  struct Channel *chptr;
  int duration = 0;
  char banStr[NICKLEN+USERLEN+HOSTLEN + 100];
  char *userName;
  struct Client *bannedUser;
  int chasing;
  char *banParm;
  char banNick[NICKLEN+100];
  //char banPropogate[NICKLEN+USERLEN+HOSTLEN + 100];
  //char *p, *p2;
  unsigned serialNumber;

  // Find the channel
  if ((chptr = hash_find_channel(parv[1])) == NULL) {
    sendto_one(source_p, numeric_form(ERR_NOSUCHCHANNEL),
               me.name, parv[0], parv[1]);
    return;
  }

  // Fetch the ban parameter
  banParm = parv[2];

  // Figure out the ban duration
  if (parc >= 4) {
    duration = atoi(parv[3]);
  }

  // Check if this if from a server, then trust it
  if (IsServer(client_p)) {

    // The server should have already looked up the
    // nick and generated a real name mask.  The parm should
    // be in the form nick!user@host
    // We will propogate the exact string that the server gave us
    strcpy(banStr, banParm);

    // Server-to-server ban messages include the original NICK tagged on the end
    strcpy(banNick, parv[4]);
  } else {

    // Make sure they're an operator of the channel
    if (!has_member_flags(find_channel_link(source_p, chptr), CHFL_HALFOP | CHFL_CHANOP)) {
      sendto_one(source_p, numeric_form(ERR_CHANOPRIVSNEEDED),
                 me.name, parv[0], chptr->name);
      return;
    }

    // Figure out who they are trying to ban
   // bannedUser = find_chasing(client_p, source_p, banParm, &chasing);
    if (bannedUser == NULL)
      return;

    //@evo tc -> if ircop dont ban
//    if (IsOper(bannedUser))
//		return;
    //@evo tc <-	

    // Get shortcut to their username
    if (bannedUser->username == NULL)
      return;
    userName = bannedUser->username;

    // Figure out the nick that was banned, for purposes of sending a message
    strcpy(banNick, bannedUser->name);

    // Check if we have a new client that sends the serial number
    serialNumber = 0;
    sscanf(userName, "%X", &serialNumber);
    if (
       (serialNumber != 0) &&
       ((char*)strchr(userName, '.') != NULL) &&
       ((char*)strchr(userName, '-') != NULL)
    ) {

      // This guy sends his serial number and full build info.
      // Let's ban him based on this
      sprintf(banStr, "*!%s@*", userName);
    } else {

      // Older clients and mac users that do not generate a unique username.
      // Just BAN by IP (using their nick), since that's the best
      // we can do
      strcpy(banStr, banParm);
      banStr[8] = '\0';		// chop off the port portion of the nick
      strcat(banStr, "*!*@*");
    }
  }

  // OK, add the ban
  if(!add_id(client_p, chptr, banStr, CHFL_BAN))
    return;

  sendto_one(source_p, banNick);

  // Send a message to all the clients in the channel that this guy got banned
  if (banNick[0] != '\0') {
   // sendto_channel_local(ALL_MEMBERS, NO, chptr, ":%s BAN %s %s %d", parv[0], chptr->name, banNick, duration);
  }

  // And propogate the change to the rest of the servers, including the calculated ban-mask
 // sendto_match_servs(chptr, client_p, CAP_CLUSTER, ":%s BAN %s %s %d %s", parv[0], chptr->name, banStr, duration, banNick);

  // !TEST! Send a debug message
  sendto_one(source_p,":%s NOTICE %s :*** Notice -- banning nick %s from %s, banParm is %s, banid is %s, duration %d.",me.name, source_p->name, banNick, chptr->name, banParm, banStr, duration);
}
#ifdef USE_MORE_CODE
static void warnLikelyHackerDetails(struct Client *source_p)
{

	struct Link *chLink;
	int	bootDelay;

	if (source_p == NULL) {
		return; //eh?
	}

	sendto_realops("  His details: %s (%s)", source_p->name, source_p->info);
	/*if (source_p->user != NULL) {
		chLink = source_p->user->channel;
		if (chLink == NULL) {
			sendto_realops("  (Not currently on any channels)");
		} else {
			while (chLink != NULL) {
				if (chLink->value.chptr != NULL) {
					sendto_realops("  Currently in channel '%s'", chLink->value.chptr->name);
				}
				chLink = chLink->next;
			}
		}
	}*/

	// Mark to boot him, if we haven't already, it
	// will happen within a random interval, so they have
	// just a bit harder time figuring out why it happened

	if (source_p->localClient->bootHackerTime == 0) {
		bootDelay = 5 + (rand() % 10);
		source_p->localClient->bootHackerTime = CurrentTime + bootDelay;
		sendto_realops("  Jerk will be booted in approximately %d seconds", bootDelay);
		sendto_realops("  %d -> %d", CurrentTime, source_p->localClient->bootHackerTime);
	}
}

static int checkBootHacker(struct Client *client_p, struct Client *source_p) {
	// Safety first!

	if (client_p == NULL || source_p == NULL) {
		return 0;
	}

	// Don't ever boot an operator

	if (IsOper(source_p)) {
		return 0;
	}

	// Pending to be booted?

	if (source_p->localClient->bootHackerTime != 0) {

		// Is it time yet?
	
		if (source_p->localClient->bootHackerTime <= CurrentTime) {
			sendto_realops("  Kicked %s.", source_p->name);
			exit_client(source_p, "Illegal Use Of This Server Is Prohibited.");
			return 1;
		}
	}

	// Don't kick him

	return 0;
}

// Check if a "real name" is OK for 4x4 purposes.  If so,
// return true.  Otherwise, return false, and send a
// message to the given connection

static int checkRealName(const char *realName, struct Client *source_p) {

	int		i;
	char		nick[64];
	int		n;
	unsigned	gameSpyId;
	int		a1, a2, a3, a4, port;
	int		curPlayers, maxPlayers;
	int		lapsCompleted, lapCount;

	// Make sure and always let opers do whatever they want

	if (IsOper(source_p)) {
		return 1;
	}

	// Protect against bogus pointer

	if (realName == NULL) {
		return 0;
	}

	// Check for bogus string

	if (*realName == '\0') {
		return 0;
	}

	// Check for empty nick

	if ((realName[0] == '^') || (isspace(realName[0]))) {
		sendto_one(source_p, ":%s RAUNCHYNICK %s :Empty nickname not allowed", me.name, source_p->name);
		return 0;
	}

	// Extract the nickname

	i = 0;
	while ((realName[i] != '\0') && (realName[i] != '^')) {

		// Check if it's too long

		if (i >= sizeof(nick)) {
			sendto_one(source_p, ":%s RAUNCHYNICK %s :Nickname is too long", me.name, source_p->name);
			return 0;
		}

		// Shove it in

		nick[i] = realName[i];
		++i;
	}
	nick[i] = '\0';

	// Check for trailing whitespace

	if (isspace(nick[i-1])) {
		sendto_one(source_p, ":%s RAUNCHYNICK %s :Trailing whitespace in nickname not allouwed", me.name, source_p->name);
		return 0;
	}

	// Check if it's raunchy

	if (isTextRaunchy(nick)) {
		sendto_one(source_p, ":%s RAUNCHYNICK %s :Your nickname is innappropriate, please change it", me.name, source_p->name);
		return 0;
	}

	// OK, the next part had better be a valid gamespy ID in hex

	n = -1;
	sscanf(realName + i, "^%08X%n", &gameSpyId, &n);
	if (n < 1) {
bastard:

		++source_p->localClient->triedHackCount;
		//sendto_one(source_p, ":%s RAUNCHYNICK %s :Nice try, punk.", me.name, source_p->name);
		sendto_realops("Invalid realname.  (Attempt # %d)", source_p->localClient->triedHackCount);
		sendto_realops("  Tried to use nick: '%s'", realName);
		warnLikelyHackerDetails(source_p);
		return 0;
	}
	i += n;

	// Check if that's all, then we're OK

	if (realName[i] == '\0') {
		return 1;
	}

	// Next char must be a ^

	if (realName[i] != '^') {
		goto bastard;
	}
	++i;

	// Next 12 chars better be a valid encoded IP

	a1 = parseAlphaHex(realName+i, 2); i += 2;
	a2 = parseAlphaHex(realName+i, 2); i += 2;
	a3 = parseAlphaHex(realName+i, 2); i += 2;
	a4 = parseAlphaHex(realName+i, 2); i += 2;
	port = parseAlphaHex(realName+i, 4); i += 4;
	if (
		(a1 < 0) || (a1 > 255) ||
		(a2 < 0) || (a2 > 255) ||
		(a3 < 0) || (a3 > 255) ||
		(a4 < 0) || (a4 > 255) ||
		(port < 1) || (port > 65535)
	) {
		goto bastard;
	}

	// Is he hosting?

	if (realName[i] == '\0') {

		// He's just in a race.  OK

		return 1;
	}
	if (realName[i] != '/') {
		sendto_realops("  checkRealName failed #1: %s", realName+i);
		goto bastard;
	}
	++i;

	// Eat track.sit and /

	while (realName[i] != '/') {
		if (realName[i] == '\0') {
			sendto_realops("  checkRealName failed #2: %s", realName+i);
			goto bastard;
		}
		++i;
	}
	++i;

	// Eat race mode name and /

	while (realName[i] != '/') {
		if (realName[i] == '\0') {
			sendto_realops("  checkRealName failed #3: %s", realName+i);
			goto bastard;
		}
		++i;
	}
	++i;

	// Eat +[parms] and /

	if (realName[i] != '+') {
		sendto_realops("  checkRealName failed #4: %s", realName+i);
		goto bastard;
	}
	++i;
	while (realName[i] != '/') {
		if (realName[i] == '\0') {
			sendto_realops("  checkRealName failed #5: %s", realName+i);
			goto bastard;
		}
		++i;
	}
	++i;

	// Read number of laps and max laps

	n = -1;
	sscanf(realName+i,"%d,%d%n", &lapsCompleted, &lapCount, &n);
	if (n >= 0) {
		if (
			(lapsCompleted < 0) ||
			(lapsCompleted > lapCount + 10)
		) {
			sendto_realops("  checkRealName failed #6: %s", realName+i);
			goto bastard;
		}
	} else {
		sscanf(realName+i,"%d%n", &lapCount, &n);
		if (n < 0) {
			sendto_realops("  checkRealName failed #7: %s", realName+i);
			goto bastard;
		}
	}
	if (
		(lapCount <= 0) ||
		(lapCount > 9999)
	) {
		sendto_realops("  checkRealName failed #8: %s", realName+i);
		goto bastard;
	}
	i += n;
	if (realName[i] != '/') {
		sendto_realops("  checkRealName failed #9: %s", realName+i);
		goto bastard;
	}
	++i;

	// Read number of players and max players

	n = -1;
	sscanf(realName+i,"%d,%d%n", &curPlayers, &maxPlayers, &n);
	if (
		(n < 3) ||
		(curPlayers < 0) ||
		(maxPlayers <= 0) ||
		(maxPlayers > 8) ||
		(curPlayers > maxPlayers+8)
	) {
		sendto_realops("  checkRealName failed #10: %s", realName+i);
		goto bastard;
	}
	i += n;
	if (realName[i] != '\0') {
		sendto_realops("  checkRealName failed #11: %s", realName+i);
		goto bastard;
	}

	// OK, real name is legit

	return 1;
}
static void removeExpiredBans(aChannel *chptr) {

	// Search all bans on the channel for expired bans

	Reg	Link	*ban = chptr->banlist;
	while (ban != NULL) {

		// Fetch pointer to next ban, in case we delete it,
		// the pointer will be whacked

		Link	*next = ban->next;

		// Check if it's expired

		if (
			(ban->value.banptr->expireTime > 0) &&
			(ban->value.banptr->expireTime < CurrentTime)
		) {

			// Whack it, using their original
			// function.  This is kind of a kludge,
			// but this is not my code and I'm
			// trying to be safe.  It shouldn't
			// be a speed concern, since this code
			// won't get called very frequently

			del_banid(chptr, ban->value.banptr->banstr);
		}

		// OK, move on to the next ban

		ban = next;
	}
}
#endif /* USE_MORE_CODE */
//parse_handle_command(message, from, parc, para);
static struct Message usrip_msgtab = {
  "USRIP", NULL, 0, 0, 1, MAXPARA, MFLG_SLOW, 0,
  { m_usrip, m_usrip, m_ignore, m_ignore, m_usrip, m_ignore }
};
static struct Message rchg_msgtab = {
  "RCHG", 0, 0, 0, 2, MAXPARA, MFLG_SLOW, 0,
  { m_unregistered, m_rchg, m_rchg, m_ignore, m_rchg, m_ignore }
};
static struct Message rlst_msgtab = {
  "RLST", NULL, 0, 1, 0, MAXPARA, MFLG_SLOW, 0,
  { m_unregistered, m_ignore, m_rlst, m_ignore, m_ignore, m_ignore }
};
static struct Message broadcast_msgtab = {
  "BROADCAST", 0, 0, 2, 0, MAXPARA, MFLG_SLOW, 0,
  { m_unregistered, m_not_oper, m_broadcast, m_ignore, m_broadcast, m_ignore }
};
static struct Message ban_msgtab = {
  "BAN", NULL, 0, 3, 0, MAXPARA, MFLG_SLOW, 0,
  { m_unregistered, m_ignore, m_ban, m_ignore, m_ignore, m_ignore }
};

void module_init(void)
{
//  if ((client_check_cb = find_callback("check_client")))
//     prev_hook = install_hook(client_check_cb, evo_check_auth);
  mod_add_cmd(&usrip_msgtab);
  mod_add_cmd(&rchg_msgtab);
  mod_add_cmd(&rlst_msgtab);
  mod_add_cmd(&broadcast_msgtab);
  mod_add_cmd(&ban_msgtab);
}

static void
module_exit(void)
{
  mod_del_cmd(&usrip_msgtab);
  mod_del_cmd(&rchg_msgtab);
  mod_del_cmd(&rlst_msgtab);
  mod_del_cmd(&broadcast_msgtab);
  mod_del_cmd(&ban_msgtab);
}

struct module module_entry =
{
  .node    = { NULL, NULL, NULL },
  .name    = NULL,
  .version = "$Revision: 424 $",
  .handle  = NULL,
  .modinit = module_init,
  .modexit = module_exit,
  .flags   = 0
};
