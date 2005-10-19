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

/** TriviaUser.c 
 *  Trivia Tracking for Users for TrivaServ
 */

#include "neostats.h"	/* Neostats API */
#include "TriviaServ.h"

static int GetUsersChannelScore (Client *u, const TriviaChan *tc);
static void UserLeaving(Client *u);
/*
 * Adds points to user for channel and network
*/
void tvs_addpoints(Client *u, TriviaChan *tc) 
{
	TriviaUser *tu;
	TriviaChannelScore *ts;
	Questions *qe;
	lnode_t *ln;
	
	if (!u || !tc->curquest) 
	{
		nlog(LOG_WARNING, "Can't find user for AddPoints?!");
		return;
	}
	tu = (TriviaUser *)GetUserModValue(u);
	if (!tu) 
	{
		tu = ns_calloc(sizeof(TriviaUser));
		if (!(u->user->Umode & UMODE_REGNICK)) 
		{
			tu->lastusedreg = 0;
			tu->networkscore = 0;
		} else {
			tu->lastusedreg = 1;
			tu->networkscore = GetUsersChannelScore(u, NULL);
		}
		strlcpy(tu->lastusednick, u->name, MAXNICK);
		tu->lastused = 0;
		tu->tcsl = list_create(LISTCOUNT_T_MAX);
		SetUserModValue (u, tu);
		lnode_create_append(userlist, u);
	}
	ln = list_first(tu->tcsl);
	while (ln) 
	{
		ts = lnode_get(ln);
		if (!ircstrcasecmp(tc->c->name, ts->cname))
			break;
		ln = list_next(tu->tcsl, ln);
	}
	if (!ln) 
	{
		ts = ns_calloc(sizeof(TriviaChannelScore));
		if (!tu->lastusedreg)
			ts->score = 0;
		else
			ts->score = GetUsersChannelScore(u, tc);
		ts->lastused = 0;
		strlcpy(ts->cname, tc->c->name, MAXCHANLEN);
		ts->savename[0] = '\0';
		strlcpy(ts->uname, u->name, MAXNICK);
		lnode_create_append(tu->tcsl, ts);
	}
	if (!tu->lastusedreg && tu->lastused < (me.now - 900)) 
		irc_prefmsg (tvs_bot, u, "If you want your score to be kept between sessions, you should register and identify for your nickname");
	if (TriviaServ.resettype &&  tu->lastused > 0 && tu->lastused < TriviaServ.lastreset)
		tu->networkscore = 0;
	tu->networkscore += TriviaServ.defaultpoints;
	tu->lastused = me.now;
	qe = tc->curquest;
	if (ts->lastused > 0 && ts->lastused < tc->lastreset)
		ts->score = 0;
	ts->score += qe->points;
	ts->lastused = me.now;
	/* showing network wide here for testing, will be removed once !score command created */
	irc_chanprivmsg (tvs_bot, tc->name, "\003%d%s%s\003%d now has\003%d %d\003%d Points in\003%d %s\003%d , and\003%d %d\003%d points network wide", tc->hintcolour, (tc->boldcase % 2) ? "\002" : "", u->name, tc->foreground, tc->hintcolour, ts->score, tc->foreground, tc->hintcolour, tc->c->name, tc->foreground, tc->hintcolour, tu->networkscore, tc->foreground);
	return;
}	

/*
 * Return Users Score For Channel If Exists
*/
static int GetUsersChannelScore (Client *u, const TriviaChan *tc) {
	TriviaChannelScore *ts;
	int channelscore = 0;
	
	ts = ns_calloc(sizeof(TriviaChannelScore));
	if (tc)
		ircsnprintf(ts->savename, sizeof(ts->savename), "%s%s", u->name, tc->c->name);
	else
		ircsnprintf(ts->savename, sizeof(ts->savename), "%sNetwork", u->name);
	/* return score only if not reset since the last correct answer, else return 0 */
	if (DBAFetch( "Scores", ts->savename, ts, sizeof(TriviaChannelScore)))
		if (((!tc) && TriviaServ.lastreset < ts->lastused) || ((tc) && tc->lastreset < ts->lastused))
			channelscore = ts->score;
	ns_free(ts)
	return channelscore;
}

/*
 * Check if nick Registered or Identified and
 * find channel scores attached to user, and add
 * saved scores for the registered nick if any.
*/
int UmodeUser (const CmdParams *cmdparams) {
	TriviaUser *tu;
	TriviaChannelScore *ts;
	lnode_t *ln;
	TriviaChan *tc;
	hnode_t *tcn;

	tu = (TriviaUser *)GetUserModValue(cmdparams->source);
	if (!tu) 
		return NS_SUCCESS;
	if (!tu->lastusedreg && (cmdparams->source->user->Umode & UMODE_REGNICK)) 
	{
		ln = list_first(tu->tcsl);
		while (ln) 
		{
			ts = lnode_get(ln);
			tcn = hash_lookup(tch, ts->cname);
			tc = hnode_get(tcn);
			ts->score += GetUsersChannelScore(cmdparams->source, tc);
			ln = list_next(tu->tcsl, ln);
		}
		tu->networkscore += GetUsersChannelScore(cmdparams->source, NULL);
		tu->lastusedreg = 1;
	}
	return NS_SUCCESS;
}

/*
 * Quit/Kill/Nick Events
 *
 * all call UserLeaving(Client *u)
 * to save users scores and free
 * module data attached to the user.
*/
int QuitNickUser (const CmdParams *cmdparams) 
{
	TriviaUser *tu;

	SET_SEGV_LOCATION();
	tu = (TriviaUser *)GetUserModValue(cmdparams->source);
	if (tu) 
		UserLeaving(cmdparams->source);
	return NS_SUCCESS;
}

int KillUser (const CmdParams *cmdparams) 
{
	TriviaUser *tu;

	SET_SEGV_LOCATION();
	tu = (TriviaUser *)GetUserModValue(cmdparams->target);
	if (tu) 
		UserLeaving(cmdparams->target);
	return NS_SUCCESS;
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
static void UserLeaving (Client *u) 
{
	TriviaUser *tu;
	TriviaChannelScore *ts;
	lnode_t *ln, *ln2;
	Client *ul;
	
	tu = (TriviaUser *)GetUserModValue(u);
	if (tu) 
	{
		ln = list_first(tu->tcsl);
		while (ln) 
		{
			ts = lnode_get(ln);
			if (tu->lastusedreg && ts->lastused > 0) 
			{
				ircsnprintf(ts->savename, sizeof(ts->savename), "%s%s", ts->uname, ts->cname);
				DBAStore( "Scores", ts->savename, ts, sizeof(TriviaChannelScore));
			}
			ns_free(ts);
			ln2 = list_next(tu->tcsl, ln);
			list_delete(tu->tcsl, ln);
			lnode_destroy(ln);
			ln = ln2;
		}
		list_destroy_auto(tu->tcsl);
		tu->tcsl = NULL;
		if (tu->lastusedreg && tu->lastused > 0) 
		{
			ts = ns_calloc(sizeof(TriviaChannelScore));
			strlcpy(ts->cname, "Network", MAXCHANLEN);
			strlcpy(ts->uname, tu->lastusednick, MAXNICK);
			ts->score = tu->networkscore;
			ts->lastused = tu->lastused;
			ircsnprintf(ts->savename, sizeof(ts->savename), "%s%s", ts->uname, ts->cname);
			DBAStore( "Scores", ts->savename, ts, sizeof(TriviaChannelScore));
			ns_free(ts);
		}
		ClearUserModValue(u);
		ns_free(tu);
	}
	ln = list_first(userlist);
	while (ln) 
	{
		ul = lnode_get(ln);
		if (ul == u) 
		{
			list_delete(userlist, ln);
			lnode_destroy(ln);
			break;
		}
		ln = list_next(userlist, ln);
	}
	return;
}

/*
 * Save all User scores when module unloads etc
*/
void SaveAllUserScores(void) 
{
	lnode_t *ln;
	Client *u;

	/*
	 * looks like a perm loop, but UserLeaving(u)
	 * removes entries from the list
	*/
	while (list_count(userlist) > 0) 
	{
		ln = list_first(userlist);
		u = lnode_get(ln);
		UserLeaving(u);
	}
}
