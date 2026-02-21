/*
 * $Id: cf_database.c,v 1.8 2004/08/24 22:20:13 cfh7 REL_2_1_3 $
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



#include <stdio.h>
#include <stdlib.h>
#include "cf_base.h"


/* The database ID tag. By associating this with the database files, we
 * can make the DB_load backwards compatible if we ever need to change
 * the format in the future.
 */
#define DB_VERSION_NUMBER 0x0200001


/* One ChannelElement contains and manages N channels. For speed reasons, 
 * the database will contain 256 separate ChannelElements 
 */

struct ChannelElement
{
	int			channel_cursize;
	int			channel_maxsize;
	struct OCF_channel**	channels;

	int			subsort[256];
};


/* Key variables */
static int						current_day = 0;
static int						db_start = 0;

static struct ChannelElement	db_channels[256];
static struct OCF_channel*	db_read_channel;
static struct OCF_channel*	db_write_channel;



char* AllocateMemory( int memneeded )
{
	return( (char*)MyMalloc( memneeded ) );
}

void DeallocateMemory( char* target )
{
	MyFree( target );
}

extern int irccmp(const char *s1, const char *s2);
int LexicographicCompare( char* s1, char* s2 )
{
	return( irccmp( s1, s2 ) );
}


/* These functions can be upgraded for more balanced hashing */
int GetCEHash( char* channel )
{
	if ( channel[1] && channel[2] ) return (int)( ToUpper( channel[2] ) );
	else return 0;
}

int GetDBHash( char* channel )
{
	if ( channel[1] ) return (int)( ToUpper( channel[1] ) );
	else return 0;
}


void CE_reset( struct ChannelElement* ce )
{
	int i;

	ce->channels = NULL;
	ce->channel_cursize = 0;
	ce->channel_maxsize = 0;
	
	for ( i = 0; i < 256; i++ )
	{
		ce->subsort[i] = 0;
	}

	ce->channels = (struct OCF_channel**)AllocateMemory( sizeof( struct OCF_channel* ) * DBCACHEINITSIZE );

	for ( i = 0; i < DBCACHEINITSIZE; i++ )
	{
		ce->channels[i] = NULL;
	}

	ce->channel_maxsize = DBCACHEINITSIZE;
}

void CE_free( struct ChannelElement* ce )
{
	int i, j;

	for ( i = 0; i < ce->channel_cursize; i++ )
	{
		for ( j = 0; j < ce->channels[i]->channotecount; j++ )
		{
			DeallocateMemory( (char*)ce->channels[i]->channote[j] );
		}

		DeallocateMemory( (char*)ce->channels[i]->chanop );
		DeallocateMemory( (char*)ce->channels[i]->name );
		DeallocateMemory( (char*)ce->channels[i] );
	}
	DeallocateMemory( (char*)ce->channels );
}

struct OCF_channel* CE_new_channel( struct ChannelElement* ce, char* channel )
{
	int i, j, h, r;
	struct OCF_channel* l;
	int f;

	/* Quench a silly compiler warning */
	r = 0;

	/* If our local array is too small to fit another channel, we need to expand the buffer first */
	if ( ce->channel_cursize + 1 >= ce->channel_maxsize )
	{
		struct OCF_channel** tc;
		
		tc = (struct OCF_channel**)AllocateMemory( sizeof( struct OCF_channel* ) * ( ce->channel_maxsize + DBCACHEINCREASE ) );
		if ( !tc ) return( NULL );

		memcpy( tc, ce->channels, sizeof( struct OCF_channel* ) * ce->channel_cursize );
		DeallocateMemory( (char*)ce->channels );

		ce->channels = tc;
		ce->channel_maxsize += DBCACHEINCREASE;
	}

	/* Find the correct location (indexwise) to store the new channel */
	h = GetCEHash( channel );

	f = 0; /* false */
	for ( i = ce->subsort[h]; i < ce->channel_cursize; i++ )
	{
		if ( LexicographicCompare( ce->channels[i]->name, channel ) >= 1 )
		{
			/* This is the spot, shift the arrays */
			r = i;
			for ( j = ce->channel_cursize; j >= r; j-- )
			{
				ce->channels[j] = ce->channels[j - 1];
			}
			f = 1; /* true */
			break;
		}
	}
	if ( !f )
	{
		/* If we have not found a match at this point, place it at the end of the queue */
		r = ce->channel_cursize;
	}

	for ( j = h + 1; j < 256; j++ )
	{
		ce->subsort[j]++;
	}

	ce->channel_cursize++;

	l = ce->channels[r] = (struct OCF_channel*)AllocateMemory( sizeof( struct OCF_channel ) );
	if ( !l ) return 0;
	memset( l, 0, sizeof( struct OCF_channel ) );

	l->name = (char*)AllocateMemory( sizeof( char ) * strlen( channel ) + 1 );
	if ( !l->name ) return 0;
	strcpy( l->name, channel );

	/* Makes sense to hard-code 8 as initial value, no matter how big or small the network. */
	ce->channels[i]->chanop = (struct OCF_chanop *)AllocateMemory( sizeof( struct OCF_chanop ) * 8 );
	if ( !ce->channels[i]->chanop ) return 0;
	ce->channels[i]->chanopmax = 8;

	return( l );
}

struct OCF_channel* CE_find_channel( struct ChannelElement* ce, char* channel )
{
	int i, h;

	/* Find rough bearings in our array */
	h = GetCEHash( channel );

	for ( i = ce->subsort[h]; i < ce->channel_cursize; i++ )
	{
		if ( LexicographicCompare( ce->channels[i]->name, channel ) == 0 )
		{
			return( ce->channels[i] );
		}
	}

	return( NULL );
}

int CE_del_channel( struct ChannelElement* ce, char* channel )
{
	int i, j, h, r;

	/* Find the correct location (indexwise) to the channel */
	h = GetCEHash( channel );

	for ( i = ce->subsort[h]; i < ce->channel_cursize; i++ )
	{
		if ( !LexicographicCompare( ce->channels[i]->name, channel ) )
		{
			for ( j = 0; j < ce->channels[i]->channotecount; j++ )
			{
				DeallocateMemory( (char*)ce->channels[i]->channote[j] );
			}

			DeallocateMemory( (char*)ce->channels[i]->chanop );
			DeallocateMemory( (char*)ce->channels[i]->name );
			DeallocateMemory( (char*)ce->channels[i] );

			/* This is the spot, shift the arrays */
			r = i + 1;
			for ( j = r; j < ce->channel_cursize; j++ )
			{
				ce->channels[j - 1] = ce->channels[j];
			}

			for ( j = h + 1; j < 256; j++ )
			{
				ce->subsort[j]--;
			}

			ce->channel_cursize--;
			return( 1 );
		}
	}

	return( 0 );
}



void DB_write_end();
void CE_update_day( struct ChannelElement* ce )
{
	int j, k;
	int so, s, r, tc, tj;

	/* Null new day counter on all channels */
	for ( j = 0; j < ce->channel_cursize; j++ )
	{
		for ( k = 0; k < ce->channels[j]->chanopcount; k++ )
		{
			ce->channels[j]->chanop[k].daycount[current_day] = 0;
		}
	}

	/* Make sure op records are still properly sorted (and resort as needed) 
	 * Note: This is somewhat CPU-intensive, so it makes sense to CE_update 
	 * only a few elements at a time to give ChanFix enough cycles to keep
	 * the show running during the rollover period each day.
	 */
	for ( j = 0; j < ce->channel_cursize; j++ )
	{
		db_write_channel = ce->channels[j]; 
		DB_write_end();
	}
	db_write_channel = NULL;

	/* Free up obsolete records (database cleanup) 
	 * (Note: We are exchanging j for tc and r to proactively counter against the risk that
	 *  certain compilers/optimizers might mess things up with the dynamic database changes) 
	 */
	tc = ce->channel_cursize;
	r = 0;
	for ( j = 0; j < tc; j++ )
	{
		tj = j - r;

		for ( k = 0; k < ce->channels[tj]->chanopcount; k++ )
		{
			/* Test the score - if an op has no score, we'll remove it from the channel op record */
			so = 0;
			for ( s = 0; s < DAYSAMPLES; s++ )
			{
				/* Halt check if scores found - we are only interested in non-scorers */
				so = ce->channels[tj]->chanop[k].daycount[s];
				if ( so ) break;
			}
			if ( !so )
			{
				/* Because this list should always be properly sorted in scoring order at this point, we can logically 
				 * assume that this and all the remaining entries are non-scorers (and fit for removal) 
				 */
				ce->channels[tj]->chanopcount = k;
				break;
			}
		}

		/* No scoring ops left? Then we'll remove the channel from our database structure! */
		if ( !ce->channels[tj]->chanopcount )
		{
#ifdef REMEMBER_CHANNELS_WITH_NOTES_OR_FLAGS
			int count = 0;
			for ( s = 0; s < MAXNOTECOUNT && !count; s++ )
			{
				if ( ce->channels[tj]->channote[s] ) count++;
			}
			if ( !ce->channels[tj]->flag && !count )
#else
			if ( 1 )
#endif
			{
				if ( CE_del_channel( ce, ce->channels[tj]->name ) )
				{
					/* If we successfully removed the channel record, we "must" increase r to keep tj in balance */
					r++;
				}
			}
		}
		else
		{
			/* Wasting memory on too large chanop structures? Let's reduce size if needed
			 * (Yes, the check is supposed to be / 3, not / 2, to avoid repeated increases or 
			 * decreases near boundary regions) 
			 */
			if ( ce->channels[tj]->chanopcount <= ( ce->channels[tj]->chanopmax / 3 ) && ( ce->channels[tj]->chanopmax > 8 ) ) 
			{
				struct OCF_chanop* tc;
				tc = (struct OCF_chanop*)AllocateMemory( sizeof( struct OCF_chanop ) * ce->channels[tj]->chanopmax / 2 );
				if ( !tc ) return;

				memcpy( tc, ce->channels[tj]->chanop, sizeof( struct OCF_chanop ) * ce->channels[tj]->chanopcount );
				DeallocateMemory( (char*)ce->channels[tj]->chanop );

				ce->channels[tj]->chanop = tc;
				ce->channels[tj]->chanopmax /= 2;
			}
		}
	}
}

void DB_update_day( int daystamp )
{
	int i;

	/* These must be inactivated before we do cleanups on our database */
	DB_set_read_channel( NULL );
	DB_set_write_channel( NULL );

	current_day = daystamp % DAYSAMPLES;

	for ( i = 0; i < 256; i++ )
	{
		CE_update_day( &db_channels[i] );
	}
}





/* This can be emptied IF we can be ABSOLUTELY sure that the caller sends us VALID data: */
int strboundscheck( char* data, int maxsize )
{
	int t;
	for ( t = 0; t < maxsize; t++ ) if ( !data[t] ) return 1;
	return( 0 );
}


void CE_register_op( char* user, char* host )
{
	int j, o;

	/* Find user in the channel's database */
	o = -1;
	for( j = 0; j < db_write_channel->chanopcount; j++ )
	{
		/* No need to support 7-bit Scandinavian letters in userhosts */
		if ( !strcasecmp( db_write_channel->chanop[j].user, user ) &&
			 !strcasecmp( db_write_channel->chanop[j].host, host ) )
		{
			o = j;
			break;
		}
	}

	/* If the opped user isn't in the database yet, let's add it in the list */
	if ( o < 0 )
	{
		/* Need to expand our chanop cache? */
		if ( db_write_channel->chanopcount >= db_write_channel->chanopmax ) 
		{
			struct OCF_chanop* tc;
		
			if ( db_write_channel->chanopmax >= MAXOPCOUNT )
			{
				return;	/* No room for additional new ops */
			}

			/* Let's expand our chanop array to meet the increased resource needs */
			tc = (struct OCF_chanop*)AllocateMemory( sizeof( struct OCF_chanop ) * db_write_channel->chanopmax * 2 );
			if ( !tc ) return;

			memcpy( tc, db_write_channel->chanop, sizeof( struct OCF_chanop ) * db_write_channel->chanopcount );
			DeallocateMemory( (char*)db_write_channel->chanop );

			db_write_channel->chanop = tc;
			db_write_channel->chanopmax *= 2;
		}

		o = db_write_channel->chanopcount;
		db_write_channel->chanopcount++;

		strncpy( db_write_channel->chanop[o].user, user, sizeof(db_write_channel->chanop[o].user) );
		strncpy( db_write_channel->chanop[o].host, host, sizeof(db_write_channel->chanop[o].host) );
		memset( db_write_channel->chanop[o].daycount, 0, sizeof( short ) * DAYSAMPLES );
	}

	/* o is the current index */
	db_write_channel->chanop[o].daycount[current_day]++;
}

int CE_set_read_channel( struct ChannelElement* ce, char* channel )
{
	db_read_channel = CE_find_channel( ce, channel );
	if ( db_read_channel ) 
	{
		return 1;
	}
	else 
	{
		return 0;
	}
}

int CE_set_write_channel( struct ChannelElement* ce, char* channel )
{
	struct OCF_channel* c;

	/* Test if channel exists in our database */
	c = CE_find_channel( ce, channel );

	/* Did we find the channel in our database? If not, let's add it */
	if ( !c )
	{
		c = CE_new_channel( ce, channel );
		if ( !c )
		{
			/* Out of memory? TO-DO: FLAG! */
			return( 0 );
		}
	}

	db_write_channel = c;
	return( 1 );
}

/* Test if channel exists in our database */
int CE_channel_exists( struct ChannelElement* ce, char* channel )
{
	/* Did we find the channel in our database? */
	if ( !CE_find_channel( ce, channel ) ) 
		return( 0 );
	else
		return( 1 );
}

/* Test if channel exists in our database, and has scoring ops */
int CE_channel_get_number_of_userhosts( struct ChannelElement* ce, char* channel )
{
	struct OCF_channel* oc;

	/* Did we find the channel in our database? */
	oc = CE_find_channel( ce, channel );

	if ( !oc ) return( 0 );

	return( oc->chanopcount );
}



/* Registers op with current write channel */
void DB_register_op( char* user, char* host )
{
	/* Basic sanity checks */
	if ( !user || !host || !db_write_channel ) return;
	if ( !strboundscheck( user, USERLEN ) || !strboundscheck( host, HOSTLEN ) ) return;

	CE_register_op( user, host );
}

/* registers manual fix with current write channel */
void DB_register_manualfix( unsigned int timestamp )
{
	int i;

	if ( !db_write_channel || !timestamp ) return;

	/* Not performance critical, so a lame shift will suffice */

	for ( i = MAXMANUALFIX - 1; i > 0; i-- )
	{
		db_write_channel->manualfixhistory[i] = db_write_channel->manualfixhistory[i - 1];
	}

	db_write_channel->manualfixhistory[0] = timestamp;
}

/* registers auto fix with current write channel */
void DB_register_autofix( unsigned int timestamp )
{
	int i;

	if ( !db_write_channel || !timestamp ) return;

	/* Not performance critical, so a lame shift will suffice */

	for ( i = MAXAUTOFIX - 1; i > 0; i-- )
	{
		db_write_channel->autofixhistory[i] = db_write_channel->autofixhistory[i - 1];
	}

	db_write_channel->autofixhistory[0] = timestamp;
}



/* Polling functions */
int DB_get_top_user_hosts( struct OCF_chanuser* op_array, char** users, char** hosts, int num_userhosts, int max )
{
	int i, j, c;

	if ( !op_array || !db_read_channel || !users || !hosts ) return 0;

	/* Lame, but should do the job (not that performance-critical) */
	c = 0;
	for ( j = 0; j < db_read_channel->chanopcount; j++ )
	{
		for( i = 0; i < num_userhosts; i++ )
		{
			if ( !strcasecmp( db_read_channel->chanop[j].user, users[i] ) &&
				 !strcasecmp( db_read_channel->chanop[j].host, hosts[i] ) )
			{
				int score, k;

				strncpy( op_array[c].user, db_read_channel->chanop[j].user, sizeof(op_array[c].user) );
				strncpy( op_array[c].host, db_read_channel->chanop[j].host, sizeof(op_array[c].host) );

				score = 0;
				for ( k = 0; k < DAYSAMPLES; k++ )
				{
					score += (int)db_read_channel->chanop[j].daycount[k];
				}

				op_array[c].score = score;
				c++;
				break;
			}
		}

		if ( c >= max ) break;
	}
	return( c );
}

int DB_poll_channel( struct OCF_topops* result, char** opusers, char** ophosts, int num_ophosts, 
					 char** nopusers, char** nophosts, int num_nophosts )
{
	int i, j, tc, to, tn;

	if ( !result ) return 0;
	memset( result, 0, sizeof( struct OCF_topops ) );

	if ( !opusers || !ophosts || !nopusers || !nophosts || !db_read_channel ) return 0;

	/* Lame, but should do the job (not that performance-critical) */
	tc = 0;
	for ( j = 0; j < db_read_channel->chanopcount; j++ )
	{
		int score, k;

		score = 0;
		for ( k = 0; k < DAYSAMPLES; k++ )
		{
			score += (int)db_read_channel->chanop[j].daycount[k];
		}

		result->topscores[tc] = score;
		tc++;

		if ( tc >= OPCOUNT ) break;
	}

	to = 0;
	tn = 0;
	for ( j = 0; j < db_read_channel->chanopcount; j++ )
	{
		for( i = 0; i < num_ophosts && to < OPCOUNT; i++ )
		{
			if ( !strcasecmp( db_read_channel->chanop[j].user, opusers[i] ) &&
				 !strcasecmp( db_read_channel->chanop[j].host, ophosts[i] ) )
			{
				int score, k;

				score = 0;
				for ( k = 0; k < DAYSAMPLES; k++ )
				{
					score += (int)db_read_channel->chanop[j].daycount[k];
				}

				result->currentopscores[to] = score;
				to++;
				break;
			}
		}

		for( i = 0; i < num_nophosts && tn < OPCOUNT; i++ )
		{
			if ( !strcasecmp( db_read_channel->chanop[j].user, nopusers[i] ) &&
				 !strcasecmp( db_read_channel->chanop[j].host, nophosts[i] ) )
			{
				int score, k;

				score = 0;
				for ( k = 0; k < DAYSAMPLES; k++ )
				{
					score += (int)db_read_channel->chanop[j].daycount[k];
				}

				result->currentnopscores[tn] = score;
				tn++;
				break;
			}
		}

		if ( to >= OPCOUNT && tn >= OPCOUNT ) break;
	}

	memcpy( result->autofixhistory,   db_read_channel->autofixhistory,   sizeof( unsigned int ) * MAXAUTOFIX );
	memcpy( result->manualfixhistory, db_read_channel->manualfixhistory, sizeof( unsigned int ) * MAXMANUALFIX );

	return 1;
}

/* Simple score checker for some of the opping logic - fills a caller-created structure with N top op scores */
int DB_get_op_scores( int* score, int scorecount )
{
	int j, tc;

	if ( !db_read_channel || !score || !scorecount ) return 0;

	/* First, we'll clean the caller's array */
	for ( j = 0; j < scorecount; j++ )
	{
		score[j] = 0;
	}

	tc = 0;
	for ( j = 0; j < db_read_channel->chanopcount; j++ )
	{
		int tscore, k;

		tscore = 0;
		for ( k = 0; k < DAYSAMPLES; k++ )
		{
			tscore += (int)db_read_channel->chanop[j].daycount[k];
		}

		score[tc] = tscore;
		tc++;

		if ( tc >= scorecount ) break;
	}

	return( tc );
}

/* For seeding OPLIST */
int DB_get_oplist( struct OCF_chanuser* chanusers, int num_users )
{
	int c, i, k, score;

	if ( !db_read_channel || !chanusers || !num_users ) return 0;

	for ( c = 0, i = 0; i < db_read_channel->chanopcount && i < num_users; i++ )
	{
		strncpy( chanusers[i].user, db_read_channel->chanop[i].user, sizeof(chanusers[i].user) );
		strncpy( chanusers[i].host, db_read_channel->chanop[i].host, sizeof(chanusers[i].host) );

		score = 0;
		for ( k = 0; k < DAYSAMPLES; k++ )
		{
			score += (int)db_read_channel->chanop[i].daycount[k];
		}

		chanusers[i].score = score;
		c++;
	}
	return( c );
}




void DB_start()
{
	int i;

	if ( !db_start )
	{
		current_day		= 0;

		for ( i = 0; i < 256; i++ )
		{
			CE_reset( &db_channels[i] );
		}

		db_start = 1;
	}
	else
	{
		/* Restarting (possibly as a result of a reload), free all memory and resources */
		for ( i = 0; i < 256; i++ )
		{
			CE_free( &db_channels[i] );
		}
		for ( i = 0; i < 256; i++ )
		{
			CE_reset( &db_channels[i] );
		}
	}

	db_read_channel		= NULL;
	db_write_channel	= NULL;
}

int sortscores[MAXOPCOUNT];
int sortscore;
struct OCF_chanop sorttemp;
void DB_write_end()
{
	int i, j, k;

	/* Here we will ensure that the channel client order stays intact */

	for ( i = 0; i < db_write_channel->chanopcount; i++ )
	{
		sortscores[i] = 0;
		
		for ( k = 0; k < DAYSAMPLES; k++ )
		{
			sortscores[i] += (int)db_write_channel->chanop[i].daycount[k];
		}
	}

	for ( i = 1; i < db_write_channel->chanopcount; i++ )
	{
		if ( sortscores[i - 1] < sortscores[i] )
		{
			/* Muzt.. shift.. pozitions.. */ 
			for ( k = i - 2; k >= 0; k-- )
			{
				if ( sortscores[k] >= sortscores[i] )
				{
					break;
				}
			}
			k++;

			strncpy( sorttemp.user, db_write_channel->chanop[i].user, sizeof(sorttemp.user) );
			strncpy( sorttemp.host, db_write_channel->chanop[i].host, sizeof(sorttemp.host) );
			memcpy( sorttemp.daycount, db_write_channel->chanop[i].daycount, sizeof( short ) * DAYSAMPLES );
			sortscore = sortscores[i];

			for ( j = i; j > k; j-- )
			{
				strncpy( db_write_channel->chanop[j].user, db_write_channel->chanop[j - 1].user, sizeof(db_write_channel->chanop[j].user) );
				strncpy( db_write_channel->chanop[j].host, db_write_channel->chanop[j - 1].host, sizeof(db_write_channel->chanop[j].host) );
				memcpy( db_write_channel->chanop[j].daycount, db_write_channel->chanop[j - 1].daycount, sizeof( short ) * DAYSAMPLES );
				sortscores[j] = sortscores[j - 1];
			}

			strncpy( db_write_channel->chanop[k].user, sorttemp.user, sizeof(db_write_channel->chanop[k].user) );
			strncpy( db_write_channel->chanop[k].host, sorttemp.host, sizeof(db_write_channel->chanop[k].host) );
			memcpy( db_write_channel->chanop[k].daycount, sorttemp.daycount, sizeof( short ) * DAYSAMPLES );
			sortscores[k] = sortscore;
		}
	}
}



/* This is a very basic function which tries to calculate a
 * simple CRC against which basic congruency checking can be
 * done on load and save. We don't care if the CRC integer
 * overflows.
 */
int GetTestCRC( struct OCF_channel* c )
{
	int i, crc;
	unsigned long* buf;
	int buflen;

	buf = (unsigned long*)c->name;
	buflen = strlen( c->name ) / sizeof( unsigned long );

	crc = 0;
	for ( i = 0; i < buflen; i++ )
	{
		crc += *buf;
		buf++;
	}

	crc += c->timestamp;
	crc += c->channotecount;
	crc += c->chanopcount;
	crc += c->chanopmax;
	crc += c->flag;
	
	return( crc );
}



int DB_load_iteration( int iteration, FILE* f )
{
	int j, k;
	int crc, l;
	char *c;
	int dbver;

	/* Ok, first check database version tag. This lets us make DB_load
	 * backwards compatible, if we ever decide to change the format again.
	 */
	if ( !fread( &dbver, sizeof( int ), 1, f ) ) 
	{
		return 0;
	}
	if ( dbver != DB_VERSION_NUMBER )
	{
		/* ..but since we only have one version number for now, we'll call 
		 * the whole thing off instead, if the version number does not 
		 * match. 
		 */
		return 0;
	}

	/* ..then main header data */
	DeallocateMemory( (char*)db_channels[iteration].channels );
	if ( !fread( &db_channels[iteration], sizeof( struct ChannelElement ), 1, f ) )
	{
		return 0;
	}
	db_channels[iteration].channels = (struct OCF_channel**)AllocateMemory( sizeof( struct OCF_channel* ) * db_channels[iteration].channel_maxsize );

	if ( !db_channels[iteration].channels )
	{
		return 0;
	}

	/* Then channel-specific data */
	for ( j = 0; j < db_channels[iteration].channel_cursize; j++ )
	{
		db_channels[iteration].channels[j] = (struct OCF_channel*)AllocateMemory( sizeof( struct OCF_channel ) );

		if ( !db_channels[iteration].channels[j] )
		{
			return 0;
		}

		/* Let's read the CRC, channel name length and channel name */
		if ( !fread( &crc, sizeof( int ), 1, f ) )
		{
			return 0;
		}
		if ( !fread( &l, sizeof( int ), 1, f ) )
		{
			return 0;
		}
		if ( l > MAXCHANNELNAME || l < 0 )
		{
			return 0;
		}
		c = (char*)AllocateMemory( sizeof( char ) * l + 1 );
		if ( !c ) return 0;

		if ( !fread( c, sizeof( char ), l, f ) )
		{
			return 0;
		}
		c[l] = 0;

		/* Then, let's read the channel structure itself */
		if ( !fread( db_channels[iteration].channels[j], sizeof( struct OCF_channel ), 1, f ) )
		{
			return 0;
		}
		/* ..and fix the name pointer */
		db_channels[iteration].channels[j]->name = c;

		/* A few sanity checks never hurt anyone.. */
		if ( db_channels[iteration].channels[j]->chanopmax < 0 || db_channels[iteration].channels[j]->chanopmax > MAXOPCOUNT )
		{
			return 0;
		}
		if ( db_channels[iteration].channels[j]->chanopcount < 0 || db_channels[iteration].channels[j]->chanopcount > MAXOPCOUNT )
		{
			return 0;
		}

		/* We resize the memory buffers for the chanop records as needed */
		db_channels[iteration].channels[j]->chanop = (struct OCF_chanop*)AllocateMemory( sizeof( struct OCF_chanop ) * db_channels[iteration].channels[j]->chanopmax );

		if ( !db_channels[iteration].channels[j]->chanop )
		{
			return 0;
		}

		/* Then, chanop record-specific data */
		if ( db_channels[iteration].channels[j]->chanopcount && !fread( db_channels[iteration].channels[j]->chanop, sizeof( struct OCF_chanop ), db_channels[iteration].channels[j]->chanopcount, f ) )
		{
			return 0;
		}

		/* Finally, we read the channel notes */
		for ( k = 0; k < db_channels[iteration].channels[j]->channotecount; k++ )
		{
			db_channels[iteration].channels[j]->channote[k] = (struct OCF_channote*)AllocateMemory( sizeof( struct OCF_channote ) );
			if ( !fread( db_channels[iteration].channels[j]->channote[k], sizeof( struct OCF_channote ), 1, f ) )
			{
				return 0;
			}
		}
		for ( ; k < MAXNOTECOUNT; k++ )
		{
			db_channels[iteration].channels[j]->channote[k] = NULL;
		}

		if ( GetTestCRC( db_channels[iteration].channels[j] ) != crc )
		{
			/* Our current channel information does not agree with the previously 
			 * calculated crc - we must assume a corrupted database.
			 */
			return 0;
		}
	}

	return( 1 );
}

int DB_has_database()
{
	int i;
	char filename[512];
	char path[256];
	FILE* f;

	if ( !strboundscheck( settings_get_str( "dbpath" ), 256 ) ) return 0;
	strncpy( path, settings_get_str( "dbpath" ), sizeof(path) );

	/* Let's look for subdatabase files.. As all subdatabases are saved,
	 * even record 0 should always exist in one form or another (so this
	 * could be optimized that way).
	 */
	for ( i = 0; i < 256; i++ )
	{
		/* One file for each subsection */
		snprintf( filename, sizeof(filename), "%sdatabase%d.ocf", path, i );
		f = fopen( filename, "rb" );
		if ( f )
		{
			fclose( f );
			return 1;
		}

		snprintf( filename, sizeof(filename), "%sdatabase%d.ocf.bak", path, i );
		f = fopen( filename, "rb" );
		if ( f )
		{
			fclose( f );
			return 1;
		}

		snprintf( filename, sizeof(filename), "%sdatabase%d.ocf.old", path, i );
		f = fopen( filename, "rb" );
		if ( f )
		{
			fclose( f );
			return 1;
		}
	}
	return 0;
}

void DB_load()
{
	int i;
	FILE* f;
	char filename[512];
	char path[256];
	int sentalert;

	sentalert = 0;

	if ( !strboundscheck( settings_get_str( "dbpath" ), 256 ) ) return;
	strncpy( path, settings_get_str( "dbpath" ), sizeof(path) );

	DB_start();

	/* If we are starting this database for the first time, we won't try loading an old one.
	 * Let's test if a database exists in the database directory..
	 */
	if ( !DB_has_database() ) 
	{
		send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, "Alert: Can't find database, creating.." );
		return;
	}

	for ( i = 0; i < 256; i++ )
	{
		/* One file for each subsection */
		snprintf( filename, sizeof(filename), "%sdatabase%d.ocf", path, i );
		f = fopen( filename, "rb" );

		if ( !f || !DB_load_iteration( i, f ) )
		{
			if ( f ) fclose( f );

			if ( ++sentalert == 1 )
			{
				send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, 
					"Alert: Unable to load primary database file %s.", filename );
			}

			/* If this failed, try loading a backup */
			snprintf( filename, sizeof(filename), "%sdatabase%d.ocf.bak", path, i );
			f = fopen( filename, "rb" );

			if ( !f || !DB_load_iteration( i, f ) )
			{
				if ( f ) fclose( f );

				if ( ++sentalert == 1 )
				{
					send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, 
						"Alert: Unable to load backup database file %s.", filename );
				}

				/* Still no luck? Try loading a backup of a backup */

				snprintf( filename, sizeof(filename), "%sdatabase%d.ocf.old", path, i );
				f = fopen( filename, "rb" );

				if ( !f || !DB_load_iteration( i, f ) )
				{
					if ( f ) fclose( f );

					/* TO-DO: Should be flagged, maybe even halted? This is clearly a problem condition, if database exists */
					if ( ++sentalert == 1 )
					{
						send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, 
							"Alert: Unable to fall back to secondary backup database file %s.", filename );
					}

					/* This might leak some memory, but we'll do this as a failsafe operation for now, to deal with a worst-case 
					 * situation (if we ever show up here, we already have a serious problem, a lot more serious than a little bit
					 * of potentially wasted RAM).
					 */
					CE_reset( &db_channels[i] );

					continue;
				}
			}
		}

		fclose( f );
	}

	if ( sentalert > 1 )
	{
		send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, "   (%d errors in total)", sentalert );
	}
}

int DB_save_iteration( int iteration, FILE* f )
{
	int j, k;
	int crc, l;
	int dbver;

	/* Ok, first DB version number and main header data */
	dbver = DB_VERSION_NUMBER;
	if ( !fwrite( &dbver, sizeof( int ), 1, f ) ) 
	{
		return 0;
	}

	if ( !fwrite( &db_channels[iteration], sizeof( struct ChannelElement ), 1, f ) ) 
	{
		return 0;
	}

	/* Then channel-specific data */
	for ( j = 0; j < db_channels[iteration].channel_cursize; j++ )
	{
		/* We write the channel CRC for basic congruency testing */
		crc = GetTestCRC( db_channels[iteration].channels[j] );
		if ( !fwrite( &crc, sizeof( int ), 1, f ) )
		{
			return 0;
		}

		l = strlen( db_channels[iteration].channels[j]->name );
		if ( !fwrite( &l, sizeof( int ), 1, f ) )
		{
			return 0;
		}

		if ( !fwrite( db_channels[iteration].channels[j]->name, sizeof( char ) * l, 1, f ) )
		{
			return 0;
		}

		if ( !fwrite( db_channels[iteration].channels[j], sizeof( struct OCF_channel ), 1, f ) )
		{
			return 0;
		}

		/* We write the chanop record-specific data */
		if ( db_channels[iteration].channels[j]->chanopcount && !fwrite( db_channels[iteration].channels[j]->chanop, sizeof( struct OCF_chanop ), db_channels[iteration].channels[j]->chanopcount, f ) )
		{
			return 0;
		}

		/* Finally, we write the channel notes */
		for ( k = 0; k < MAXNOTECOUNT; k++ )
		{
			if ( db_channels[iteration].channels[j]->channote[k] )
				if ( !fwrite( db_channels[iteration].channels[j]->channote[k], sizeof( struct OCF_channote ), 1, f ) )
				{
					return 0;
				}
		}
	}
	return 1;
}

void DB_save()
{
	int i;
	FILE* f;
	char filename[512];
	char bakname[512];
	char path[256];
	int sentalert;

	sentalert = 0;

	if ( !strboundscheck( settings_get_str( "dbpath" ), 256 ) ) return;
	strncpy( path, settings_get_str( "dbpath" ), sizeof(path) );

	for ( i = 0; i < 256; i++ )
	{
		/* One file for each subsection */
		snprintf( filename, sizeof(filename), "%sdatabase%d.ocf", path, i );
		f = fopen( filename, "wb" );

		if ( f )
		{
			if ( DB_save_iteration( i, f ) )
			{
				/* Successfully written? If so, let's make backups */
				snprintf( filename, sizeof(filename), "%sdatabase%d.ocf.old", path, i );
				snprintf( bakname, sizeof(bakname), "%sdatabase%d.ocf.bak", path, i );

				/* Move current bak to old */

				/* TO-DO: MAKE THIS BETTER. WOULD MAKE SENSE TO WRITE INTO .TMP FILES FIRST, FOR EXAMPLE */
				remove( filename );
				if ( rename( bakname, filename ) )
				{
					FILE* t;
					t = fopen( bakname, "rb" );
					
					if ( t )
					{
						/* We only alert here if the .bak exists (and specifically, should have been possible to access) */
						if ( ++sentalert == 1 )
						{
							send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, "Alert: Unable to rotate old backups! (Low on diskspace? Wrong file/dir permissions?)" );
						}
						fclose( t );
					}
				}

				fclose( f );

				f = fopen( bakname, "wb" );

				if ( f )
				{
					if ( DB_save_iteration( i, f ) )
					{
						/* All is lovely */
					}
					else
					{
						if ( ++sentalert == 1 )
						{
							send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, "Alert: Unable to write database backup! (Low on diskspace?)" );
						}
					}
					fclose( f );
				}
				else
				{
					if ( ++sentalert == 1 )
					{
						send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, "Alert: Unable to open backup database file for writing! (Gremlins?)" );
					}
				}
			}
			else
			{
				fclose( f );
				if ( ++sentalert == 1 )
				{
					send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, 
						"Alert: Unable to write database file! (Low on diskspace?)");
				}
			}
		}
		else
		{
			if ( ++sentalert == 1 )
			{
				send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, "Alert: Unable to open database for writing! (Directory permission problem?)" );
			}
		}
	}

	if ( sentalert > 1 )
	{
		send_notice_to_loggedin( OCF_NOTICE_FLAGS_MAIN, "   (%d errors in total)", sentalert );
	}
}

void DB_set_day(int daystamp)
{
	current_day = daystamp % DAYSAMPLES;
}

/* channel name currently read from (optimizes multiple read queries to same channel) */
int DB_set_read_channel( char* channel )
{
	if ( !channel ) 
	{
		db_read_channel = NULL;
		return 0;
	}
	return( CE_set_read_channel( &db_channels[GetDBHash( channel )], channel ) );
}

char* DB_get_read_channel()
{
	if ( db_read_channel )
	{
		return( db_read_channel->name );
	}
	else
	{
		return( NULL );
	}
}

/* channel name currently written to (updated, op registration) */
int DB_set_write_channel( char* channel )
{
	/* If we have a previously open write channel, wrap it up */
	if ( db_write_channel )
	{
		DB_write_end();
		db_write_channel = NULL;
	}

	/* If NULL, nothing to do beyond this part */
	if ( !channel )
	{
		return 1;
	}

	if ( !strboundscheck( channel, MAXCHANNELNAME ) ) return 0;

	return( CE_set_write_channel( &db_channels[GetDBHash( channel )], channel ) );
}

char* DB_get_write_channel()
{
	if ( db_write_channel )
	{
		return( db_write_channel->name );
	}
	else
	{
		return( NULL );
	}
}

/* Test if channel exists in our database */
int DB_channel_exists( char* channel )
{
	/* If NULL, nothing to do beyond this part */
	if ( !channel )
	{
		return 0;
	}

	if ( !strboundscheck( channel, MAXCHANNELNAME ) ) return 0;

	return( CE_channel_exists( &db_channels[GetDBHash( channel )], channel ) );
}

/* Test if channel exists in our database, and has chanops */
int DB_channel_get_number_of_userhosts( char* channel )
{
	/* If NULL, nothing to do beyond this part */
	if ( !channel )
	{
		return 0;
	}

	if ( !strboundscheck( channel, MAXCHANNELNAME ) ) return 0;

	return( CE_channel_get_number_of_userhosts( &db_channels[GetDBHash( channel )], channel ) );
}




int CN_channel_note_count( struct OCF_channote** cn )
{
	int i, count;

	count = 0;
	for ( i = 0; i < MAXNOTECOUNT; i++ )
	{
		if ( cn[i] )
		{
			count++;
		}
	}

	return( count );
}

int DB_channel_add_note( unsigned int id, unsigned int timestamp, char* note )
{
	int s, i;

	if ( !db_write_channel || !note ) return 0;

	s = MAXNOTECOUNT - 1;

	/* Let's backshift no further than to the first open/empty slot */
	for ( i = 0; i < MAXNOTECOUNT; i++ )
	{
		if ( !db_write_channel->channote[i] )
		{
			s = i;
			break;
		}
	}
	/* Full house? If so, delete the oldest record before it is overwritten by the shift */
	if ( s == MAXNOTECOUNT - 1 )
	{
		if ( db_write_channel->channote[MAXNOTECOUNT - 1] )
		{
			DeallocateMemory( (char*) db_write_channel->channote[MAXNOTECOUNT - 1] );
		}
	}

	for ( i = s; i > 0; i-- )
	{
		db_write_channel->channote[i] = db_write_channel->channote[i - 1];
	}


	db_write_channel->channote[0] = (struct OCF_channote*)AllocateMemory( sizeof( struct OCF_channote ) );

	db_write_channel->channote[0]->id = id;
	db_write_channel->channote[0]->timestamp = timestamp;
	strncpy( db_write_channel->channote[0]->note, note, sizeof(db_write_channel->channote[0]->note) );

	/* This might seem a bit resource unfriendly, but this is not a performance-critical function */
	return( db_write_channel->channotecount = CN_channel_note_count( db_write_channel->channote ) );
}

/* Clean notes, ids: 1 - MAXNOTECOUNT */
int DB_channel_del_note( int count )
{
	int i;

	if ( !db_write_channel || count <= 0 || count > MAXNOTECOUNT ) return( 0 );

	for ( i = 0; i < MAXNOTECOUNT; i++ )
	{
		if ( db_write_channel->channote[i] )
		{
			count--;

			if ( !count )
			{
				/* This is the one */

				DeallocateMemory( (char*) db_write_channel->channote[i] );
				db_write_channel->channote[i] = NULL;

				/* Might seem a bit resource unfriendly, but this is not a performance-critical function */
				db_write_channel->channotecount = CN_channel_note_count( db_write_channel->channote );

				return( 1 );
			}
		}
	}

	/* Might seem a bit resource unfriendly, but this is not a performance-critical function */
	db_write_channel->channotecount = CN_channel_note_count( db_write_channel->channote );

	return( 0 );
}

int DB_channel_note_count()
{
	int i, count;

	if ( !db_read_channel ) return( 0 );

	count = 0;
	for ( i = 0; i < MAXNOTECOUNT; i++ )
	{
		if ( db_read_channel->channote[i] )
		{
			count++;
		}
	}

	return( count );
}

struct OCF_channote* DB_channel_get_note( int count )
{
	int i;

	if ( !db_read_channel || count <= 0 || count > MAXNOTECOUNT ) return( NULL );

	for ( i = 0; i < MAXNOTECOUNT; i++ )
	{
		if ( db_read_channel->channote[i] )
		{
			count--;

			if ( !count )
			{
				/* This is the one */
				return( db_read_channel->channote[i] );
			}
		}
	}

	return( NULL );
}

void DB_channel_set_flag( unsigned int flag )
{
	if ( !db_write_channel ) return;

	db_write_channel->flag = flag;
}

unsigned int DB_channel_get_flag()
{
	if ( !db_read_channel ) return 0;

	return( db_read_channel->flag );
}



