/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2005 Adam Rutter, Justin Hammond, Mark Hetherington, DeadNotBuried
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

/*
 * Loads Channel Data
*/
int LoadChannel( void *data, int size ) {
	TriviaChan *tc;

	tc = ns_calloc (sizeof(TriviaChan));
	os_memcpy (tc, data, sizeof(TriviaChan));
	tc->qfl = list_create(-1);
	hnode_create_insert(tch, tc, tc->name);
	dlog (DEBUG1, "Loaded TC entry for Channel %s", tc->name);
	return NS_FALSE;
}

/*
 * Loads Channel Question Sets Data
 * and attaches lists to channel via
 * tvs_quesset procedure
*/
int LoadCQSets( void *data, int size ) {
	TriviaChan *tc;
	hnode_t *tcn;
	TriviaChannelQuestionFiles *tcqf;

	tcqf = ns_calloc(sizeof(TriviaChannelQuestionFiles));
	os_memcpy (tcqf, data, sizeof(TriviaChannelQuestionFiles));
	tcn = hash_lookup(tch, tcqf->cname);
	if (tcn != NULL) {
		tc = hnode_get(tcn);
		tvs_quesset(NULL, tc, tcqf->qname);
	}
	ns_free(tcqf);
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
	tc->scorepoints = TriviaServ.defaultpoints;
	tc->questtime = 60;
	tc->opchan = 1;
	tc->resettype = 0;
	tc->lastreset = me.now;
	tc->foreground = 4;
	tc->background = 1;
	tc->boldcase = 0;
	tc->hintcolour = 0;
	tc->messagecolour = 0;
	tc->hintchar = '-';
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
		tc->active = 0;
		tc->curquest = NULL;
		SetChannelModValue (c, tc);
		if (tc->opchan) {
			irc_join (tvs_bot, tc->name, "+o");
		} else {
			irc_join (tvs_bot, tc->name, NULL);
		}
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
