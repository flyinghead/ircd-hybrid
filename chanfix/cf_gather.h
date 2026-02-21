/*
 * $Id: cf_gather.h,v 1.7 2004/08/29 09:42:52 cfh7 REL_2_1_3 $
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
 * The periodic data gathering.
 * Check all channels for scores and update the database.
 */

/* This boolean shows whether we're in the middle of a database
 * update. */
int busy_gathering_channels;

/* The number of channels to be processed per run. */
#define OCF_CHANNELS_PER_RUN 1000

/* If the channel has fewer clients than this, a static array
 * of userhosts pointers can be used instead of dynamically
 * allocating the memory for each channel. */
#define OCF_NUM_STATIC_USERHOSTS 1024

void gather_all_channels();

/* Looks in channelTable[current_bucket] and updates the scores
 * of all channels in there which have at least minclients client. */
int gather_channels_in_bucket(unsigned int current_bucket, 
	int minclients, int required_status, int *qualifying_channels);

/* Updates the scores of this channel, which may have no more than
 * OCF_NUM_STATIC_USERHOSTS clients in it. 
 * Uses a static array of userhast pointers, of size
 * OCF_NUM_STATIC_USERHOSTS. */
void update_scores_of_channel(struct Channel* chptr, 
							  int num_clients, int required_status);

/* Updates the scores of this channel, which should have more than
 * OCF_NUM_STATIC_USERHOSTS clients in it. 
 * Uses malloc/free to create arrays of userhost pointers. */
void update_scores_of_large_channel(struct Channel* chptr, 
									int num_clients, int required_status);

