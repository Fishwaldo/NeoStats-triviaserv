/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2005 Adam Rutter, Justin Hammond, Mark Hetherington
** http://www.neostats.net/
**
**  Portions Copyright (c) 2000-2001 ^Enigma^
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
**  USA
**
** NeoStats CVS Identification
** $Id$
*/

/** TriviaChan.c 
 *  Trivia Channel Procedures
 */

#include "neostats.h"	/* Neostats API */
#include "TriviaServ.h"

int LoadChannel( void *data )
{
	TriviaChan *tc;

	tc = ns_calloc (sizeof(TriviaChan));
	os_memcpy (tc, data, sizeof(TriviaChan));
	tc->qfl = list_create(-1);
	hnode_create_insert(tch, tc, tc->name);
	dlog (DEBUG1, "Loaded TC entry for Channel %s", tc->name);
	return NS_FALSE;
}

/*
 * Adds Trivia Channel Entry
*/
TriviaChan *NewTChan(Channel *c) 
{
	TriviaChan *tc;
	hnode_t *tcn;
	
	tc = (TriviaChan *) GetChannelModValue (c);
	if (tc) {
		nlog(LOG_WARNING, "Hrm, Chan %s already has a TriviaChanStruct with it", c->name);
		return tc;
	}
	/* ok, first we lookup in the tch hash, to see if this is a channel that we already have a setting for */
	tcn = hash_lookup(tch, c->name);
	if (tcn) {
		return NULL;
	}
	/* ok, create and insert into hash */
	tc = ns_calloc (sizeof(TriviaChan));
	strlcpy (tc->name, c->name, MAXCHANLEN);
	tc->c = c;
	/* XXX */
	tc->scorepoints = TriviaServ.defaultpoints;
	tc->questtime = 60;
	SetChannelModValue (c, tc);
	tc->qfl = list_create(-1);
	hnode_create_insert (tch, tc, tc->name);
	dlog (DEBUG1, "Created New TC entry for Channel %s", c->name);
	return tc;
}

/*
 * Last User left Channel
*/
TriviaChan *OfflineTChan(Channel *c) {
	TriviaChan *tc;
	if (!c) {
		return NULL;
	}
	tc = (TriviaChan *) GetChannelModValue (c);
	if (tc == NULL) {
		nlog(LOG_WARNING, "TriviaChan %s already marked offline?!!?!", c->name);
		return NULL;
	}
	ClearChannelModValue (c);
	tc->c = NULL;
	tc->active = 0;
	tc->curquest = NULL;
	irc_part( tvs_bot, c->name, NULL );
	return tc;
}

/*
 * First User Joined Channel
*/
TriviaChan *OnlineTChan(Channel *c) {
	TriviaChan *tc;
	hnode_t *tcn;
	if (!c) {
		return NULL;
	}
	tc = (TriviaChan *) GetChannelModValue (c);
	if (tc) {
		nlog(LOG_WARNING, "TriviaChan %s already marked online?!?!", c->name);
		return tc;
	}
	tcn = hash_lookup(tch, c->name);
	if (tcn != NULL) {
		tc = hnode_get(tcn);
		tc->c = c;
		SetChannelModValue (c, tc);
		irc_join (tvs_bot, tc->name, "+o");
		return tc;
	}
	return NULL;
}
	
/*
 * Remove Trivia Channel Entry
*/
int DelTChan(char *chan) {
	hnode_t *hnode;
	TriviaChan *tc;
	
	hnode = hash_lookup(tch, chan);
	if (hnode) {
		tc = hnode_get(hnode);
		/* part the channel if its online */
		if ((TriviaChan *)GetChannelModValue (tc->c)) OfflineTChan(FindChannel (tc->name));
		hash_delete(tch, hnode);
		list_destroy_nodes(tc->qfl);
		ns_free (tc);
		hnode_destroy(hnode);
		DBADelete("Channel", chan);
		return NS_SUCCESS;
	}
	return NS_FAILURE;
}

/*
 * Save Trivia Channel Entry to DB
*/
int SaveTChan (TriviaChan *tc) 
{
	DBAStore ("Channel", tc->name, tc, sizeof (TriviaChan));
	/* XXX Save Category List */
	return NS_SUCCESS;
}

/*
 * Channel Emptied
*/
int EmptyChan (CmdParams* cmdparams)
{
	OfflineTChan(cmdparams->channel);
	return NS_SUCCESS;
}

/*
 * Channel Created
*/
int NewChan(CmdParams* cmdparams) 
{
	OnlineTChan(cmdparams->channel);
	return NS_SUCCESS;
}
