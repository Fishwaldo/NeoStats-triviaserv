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

/** TriviaUser.c 
 *  Trivia Tracking for Users for TrivaServ
 */

#include "neostats.h"	/* Neostats API */
#include "TriviaServ.h"

/*
 * Adds points to user for channel and network
*/
void tvs_addpoints(Client *u, TriviaChan *tc) 
{
	TriviaUser *tu;
	TriviaChannelScore *ts, *tsn;
	Questions *qe;
	
	if (!u | !tc->curquest) {
		nlog(LOG_WARNING, "Can't find user for AddPoints?!");
		return;
	}
	tu = (TriviaUser *)GetUserModValue(u);
	if (!tu) {
		tu = ns_calloc(sizeof(TriviaUser));
		if (!(u->user->Umode & UMODE_REGNICK)) {
			tu->lastregnick[0] = '\0';
		} else {
			strlcpy(tu->lastregnick, u->name, MAXNICK);
		}
		tu->lastusednick[0] = '\0';
		tu->networkscore = 0;
		tu->lastused = 0;
		ts = ns_calloc(sizeof(TriviaChannelScore));
		tu->tcs = ts;
		ts->prev = NULL;
		strlcpy(ts->cname, tc->c->name, MAXNICK);
		ts->score = 0;
		ts->lastused = 0;
		ts->next = NULL;
		SetUserModValue (u, tu);
	} else {
		ts = tu->tcs;
		/* not really a perm loop, just looks like one */
		while (0 == 0) {
			if (!ircstrcasecmp(tc->c->name, ts->cname)) {
				break;
			}
			if (ts->next == NULL) {
				tsn = ns_calloc(sizeof(TriviaChannelScore));
				tsn->prev = ts;
				strlcpy(tsn->cname, tc->c->name, MAXNICK);
				tsn->score = 0;
				tsn->lastused = 0;
				tsn->next = NULL;
				ts->next = tsn;
			}
			ts = ts->next;
		}
	}
	strlcpy(tu->lastusednick, u->name, MAXNICK);
	if (tu->lastregnick[0] == '\0' && tu->lastused < (me.now - 900)) {
		irc_prefmsg (tvs_bot, u, "If you want your score to be kept between sessions, you should register your nickname");
	}
	tu->networkscore += TriviaServ.defaultpoints;
	tu->lastused = me.now;
	qe = tc->curquest;
	ts->score += qe->points;
	ts->lastused = me.now;
	irc_chanprivmsg (tvs_bot, tc->name, "%s now has %d Points in %s, and %d points network wide", u->name, ts->score, tc->c->name, tu->networkscore);
	return;
}	

/*
 * Free User Data
 *
 * ToDo : 1. Save Points between sessions if registered
 *        2. find way to link nicks so multiple entries for
 *		same user but different nicks, using duplicate scores
 *		are not created (maybe check for saved scores on mode +r
 *		and add them to the current users score, removing the
 *		old from the database if they change nicks and identify again)
*/
int QuitUser (CmdParams* cmdparams) {
	return UserLeaving(cmdparams->source);
}

int KillUser (CmdParams* cmdparams) {
	return UserLeaving(cmdparams->target);
}

int UserLeaving (Client *u) {
	TriviaUser *tu;
	TriviaChannelScore *ts, *tsp;
	
	tu = (TriviaUser *)GetUserModValue(u);
	if (tu) {
		ts = tu->tcs;
		while (ts->next != NULL) {
			ts = ts->next;
		}
		while (ts->prev != NULL) {
			tsp = ts;
			ts = tsp->prev;
			ts->next = NULL;
			ns_free(tsp);
		}
		tu->tcs = NULL;
		ns_free(ts);
		ns_free(tu);
	}
	return NS_SUCCESS;
}
