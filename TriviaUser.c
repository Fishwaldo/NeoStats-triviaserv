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
	TriviaChannelScore *ts, *tst;
	Questions *qe;
	lnode_t *ln;
	
	if (!u | !tc->curquest) {
		nlog(LOG_WARNING, "Can't find user for AddPoints?!");
		return;
	}
	tu = (TriviaUser *)GetUserModValue(u);
	if (!tu) {
		tu = ns_calloc(sizeof(TriviaUser));
		if (!(u->user->Umode & UMODE_REGNICK)) {
			tu->lastusedreg = 0;
			tst = NULL;
		} else {
			tu->lastusedreg = 1;
			tst = GetUsersChannelScore(u->name, "Network");
		}
		strlcpy(tu->lastusednick, u->name, MAXNICK);
		if (!tst) {
			tu->networkscore = 0;
			tu->lastused = 0;
		} else {
			tu->networkscore = tst->score;
			tu->lastused = tst->lastused;
		}
		tu->tcsl = list_create(-1);
		SetUserModValue (u, tu);
	}
	ln = list_first(tu->tcsl);
	while (ln != NULL) {
		ts = lnode_get(ln);
		if (!ircstrcasecmp(tc->c->name, ts->cname)) {
			break;
		}
		ln = list_next(tu->tcsl, ln);
	}
	if (ln == NULL) {
		ts = ns_calloc(sizeof(TriviaChannelScore));
		if (!tu->lastusedreg) {
			tst = NULL;
		} else {
			tst = GetUsersChannelScore(u->name, tc->c->name);
		}
		if (!tst) {
			ts->score = 0;
			ts->lastused = 0;
		} else {
			ts->score = tst->score;
			ts->lastused = tst->lastused;
		}
		strlcpy(ts->cname, tc->c->name, MAXCHANLEN);
		ts->savename[0] = '\0';
		strlcpy(ts->uname, u->name, MAXNICK);
		lnode_create_append(tu->tcsl, ts);
	}
	if (!tu->lastusedreg && tu->lastused < (me.now - 900)) {
		irc_prefmsg (tvs_bot, u, "If you want your score to be kept between sessions, you should register and identify for your nickname");
	}
	tu->networkscore += TriviaServ.defaultpoints;
	tu->lastused = me.now;
	qe = tc->curquest;
	if (ts->lastused < tc->lastreset) {
		ts->score = 0;
	}
	ts->score += qe->points;
	ts->lastused = me.now;
	/* showing network wide here for testing, will be removed once !score command created */
	irc_chanprivmsg (tvs_bot, tc->name, "%s now has %d Points in %s, and %d points network wide", u->name, ts->score, tc->c->name, tu->networkscore);
	return;
}	
/*
 * Load Users Score For Channel If Exists
*/
TriviaChannelScore *GetUsersChannelScore (char *uname, char *cname) {
	TriviaChannelScore *ts;
	
	ircsnprintf(ts->savename, sizeof(ts->savename), "%s%s", uname, cname);
	DBAFetch( "Scores", ts->savename, ts, sizeof(TriviaChannelScore));
	return ts;
}

/*
 * Check if nick Registered or Identified and
 * find channel scores attached to user, and add
 * saved scores for the registered nick if any.
*/
int UmodeUser (CmdParams* cmdparams) {
	TriviaUser *tu;
	TriviaChannelScore *ts, *tst;
	lnode_t *ln;

	tu = (TriviaUser *)GetUserModValue(cmdparams->source);
	if (tu) {
		if (!tu->lastusedreg && (cmdparams->source->user->Umode & UMODE_REGNICK)) {
			ln = list_first(tu->tcsl);
			while (ln != NULL) {
				ts = lnode_get(ln);
				tst = GetUsersChannelScore(cmdparams->source->name, ts->cname);
				if (tst) {
					ts->score += tst->score;
					if (tst->lastused > ts->lastused) {
						ts->lastused = tst->lastused;
					}
				}
				ln = list_next(tu->tcsl, ln);
			}
			tst = GetUsersChannelScore(cmdparams->source->name, "Network");
			if (tst) {
				tu->networkscore += tst->score;
				if (tst->lastused > tu->lastused) {
					tu->lastused = tst->lastused;
				}
			}
			tu->lastusedreg = 1;
		}
	}
}

/*
 * Quit/Kill/Nick Events
 *
 * all call UserLeaving(Client *u)
 * to save users scores and free
 * module data attached to the user.
*/
int QuitNickUser (CmdParams* cmdparams) {
	return UserLeaving(cmdparams->source);
}

int KillUser (CmdParams* cmdparams) {
	return UserLeaving(cmdparams->target);
}

/*
 * Free User Data
 *
 * ToDo : 1. possibly find way to link nicks so multiple entries for
 *		same user but different nicks, using duplicate scores
 *		are not created (maybe check for saved scores on mode +r
 *		and add them to the current users score, removing the
 *		old from the database if they change nicks and identify again)
*/
int UserLeaving (Client *u) {
	TriviaUser *tu;
	TriviaChannelScore *ts;
	lnode_t *ln;
	
	tu = (TriviaUser *)GetUserModValue(u);
	if (tu) {
		while (list_count(tu->tcsl) > 0) {
			ln = list_first(tu->tcsl);
			ts = lnode_get(ln);
			if (tu->lastusedreg) {
				ircsnprintf(ts->savename, sizeof(ts->savename), "%s%s", ts->uname, ts->cname);
				DBADelete("Scores", ts->savename);
				DBAStore( "Scores", ts->savename, ts, sizeof(TriviaChannelScore));
			}
			ns_free(ts);
			list_delete(tu->tcsl, ln);
			lnode_destroy(ln);
		}
		list_destroy_auto(tu->tcsl);
		tu->tcsl = NULL;
		if (tu->lastusedreg) {
			ts = ns_calloc(sizeof(TriviaChannelScore));
			strlcpy(ts->cname, "Network", MAXCHANLEN);
			strlcpy(ts->uname, tu->lastusednick, MAXNICK);
			ts->score = tu->networkscore;
			ts->lastused = tu->lastused;
			ircsnprintf(ts->savename, sizeof(ts->savename), "%s%s", ts->uname, ts->cname);
			DBADelete("Scores", ts->savename);
			DBAStore( "Scores", ts->savename, ts, sizeof(TriviaChannelScore));
			ns_free(ts);
		}
		ns_free(tu);
		ClearUserModValue(u);
	}
	return NS_SUCCESS;
}
