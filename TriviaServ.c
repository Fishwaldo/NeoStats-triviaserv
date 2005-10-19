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

/** TriviaServ.c 
 *  Trivia Service for NeoStats
 */

#include "neostats.h"	/* Neostats API */
#include "TriviaServ.h"
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

static int ChanPrivmsg (const CmdParams *cmdparams);

#ifdef WIN32
static void *(*old_malloc)(size_t);
static void (*old_free) (void *);
#endif

const char *questpath = "data/TSQuestions/";

Bot *tvs_bot;
TriviaServCfg TriviaServ;
list_t *qfl;
list_t *userlist;
hash_t *tch;

/** Copyright info */
static const char *ts_copyright[] = {
	"Copyright (c) 1999-2005, NeoStats",
	"http://www.neostats.net/",
	NULL
};

/** Module info */
ModuleInfo module_info = {
	"TriviaServ",
	"Triva Service for NeoStats.",
	ts_copyright,
	tvs_about,
	NEOSTATS_VERSION,
	MODULE_VERSION,
	__DATE__,
	__TIME__,
	0,
	0,
	0,
};

static bot_cmd tvs_commands[]=
{
	{"CHANS",	tvs_chans,		1,	NS_ULEVEL_ADMIN,	tvs_help_chans, 0, NULL, NULL},
	{"CATLIST",	tvs_catlist,		0, 	0,			tvs_help_catlist, 0, NULL, NULL},
	{"SCORE",	tvs_cmd_score,		0, 	0,			NULL, 0, NULL, NULL},
	{"HINT",	tvs_cmd_hint,		0, 	0,			NULL, 0, NULL, NULL},
	{"START",	tvs_cmd_start,		0, 	0,			tvs_help_start,		CMD_FLAG_CHANONLY, NULL, NULL},
	{"STOP",	tvs_cmd_stop,		0, 	0,			tvs_help_stop,		CMD_FLAG_CHANONLY, NULL, NULL},
	{"QS",		tvs_cmd_qs,		1, 	0,			tvs_help_qs,		CMD_FLAG_CHANONLY, NULL, NULL},
	{"SETPOINTS",	tvs_cmd_sp,		1, 	0,			tvs_help_sp,		CMD_FLAG_CHANONLY, NULL, NULL},
	{"PUBLIC",	tvs_cmd_pc,		1, 	0,			tvs_help_pc,		CMD_FLAG_CHANONLY, NULL, NULL},
	{"OPCHAN",	tvs_cmd_opchan,		1, 	0,			tvs_help_opchan,	CMD_FLAG_CHANONLY, NULL, NULL},
	{"RESETSCORES",	tvs_cmd_resetscores,	1, 	0,			tvs_help_resetscores,	CMD_FLAG_CHANONLY, NULL, NULL},
	{"COLOUR",	tvs_cmd_colour,		2, 	0,			tvs_help_colour,	CMD_FLAG_CHANONLY, NULL, NULL},
	{"HINTCHAR",	tvs_cmd_hintchar,	1, 	0,			tvs_help_hintchar,	CMD_FLAG_CHANONLY, NULL, NULL},
	NS_CMD_END()
};

static bot_setting tvs_settings[]=
{
	{"VERBOSE", 		&TriviaServ.verbose,		SET_TYPE_BOOLEAN,	0,	0, 		NS_ULEVEL_ADMIN,	NULL,	tvs_help_set_verbose,	NULL,	(void *) 1},
	{"EXCLUSIONS", 		&TriviaServ.use_exc,		SET_TYPE_BOOLEAN,	0,	0, 		NS_ULEVEL_ADMIN,	NULL,	tvs_help_set_exclusions,	NULL,	(void *) 0},
	{"DEFAULTPOINTS", 	&TriviaServ.defaultpoints,	SET_TYPE_INT,		1,	25, 		NS_ULEVEL_ADMIN,	NULL,	tvs_help_set_defaultpoints,	NULL,	(void *) 1},
	{"RESETTYPE",	 	&TriviaServ.resettype,		SET_TYPE_INT,		0,	6, 		NS_ULEVEL_ADMIN,	NULL,	tvs_help_set_resettype,		NULL,	(void *) 0},
	NS_SETTING_END()
};

/** Module Events */
ModuleEvent module_events[] = {
	{EVENT_CPRIVATE, ChanPrivmsg, 0},
	{EVENT_EMPTYCHAN, EmptyChan, 0},
	{EVENT_NEWCHAN, NewChan, 0},
	{EVENT_QUIT, QuitNickUser, 0},
	{EVENT_KILL, KillUser, 0},
	{EVENT_NICK, QuitNickUser, 0},
	{EVENT_UMODE, UmodeUser, 0},
	NS_EVENT_END()
};

/** BotInfo */
static BotInfo tvs_botinfo = 
{
	"TriviaServ", 
	"TriviaServ1", 
	"TS", 
	BOT_COMMON_HOST, 
	"Trivia Bot", 	
	BOT_FLAG_SERVICEBOT,
	tvs_commands, 
	tvs_settings,
};

/*
 * Channel message processing
*/
static int ChanPrivmsg (const CmdParams *cmdparams)
{
	TriviaChan *tc;
	char *tmpbuf;
	unsigned int len;
	
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	len = strlen( cmdparams->param ) + 1;
	tmpbuf = ns_malloc( len );
	strlcpy( tmpbuf, cmdparams->param, len );
	strip_mirc_codes(tmpbuf);
	tvs_testanswer(cmdparams->source, tc, tmpbuf);
	ns_free (tmpbuf);
	return NS_SUCCESS;
}

/*
 * Process Timer to kick off hints or new questions when required
*/
static int tvs_processtimer(void *userptr) 
{
	TriviaChan *tc;
	hscan_t hs;
	hnode_t *hnodes;
	time_t timediff;

	SET_SEGV_LOCATION();
	/* reseed the random generator */
	if (!(rand() % 4)) 
		srand((unsigned int)me.now);
	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs))) 
	{
		tc = hnode_get(hnodes);
		if ((tc->c) && (tc->active == 1)) 
		{
			/* ok, channel is active. if curquest is null, ask a new question */
			if (!tc->curquest) 
			{
				tvs_newquest(tc);
				continue;
			}
			/* if we are here, its a current question. */
			timediff = me.now - tc->lastquest;
			if (timediff > tc->questtime) 
			{
				/* timeout, answer the question */
				tvs_ansquest(tc);
				continue;
			}
			/* hint processor */
			if ((tc->questtime - timediff) < 15) 
			{
				irc_chanprivmsg (tvs_bot, tc->name, "\003%d%sLess than %ld Seconds Remaining, Hurry up!", tc->messagecolour, (tc->boldcase %2) ? "\002" : "", (long)( ( tc->questtime - timediff ) + 10));
				continue;
			}
			do_hint(tc);
		}
	}
	return NS_SUCCESS;
}

/*
 * Process Timers to clear channel scores
*/
static void tvs_clearscoretimers(int cleartype)
{
	TriviaChan *tc;
	hscan_t hs;
	hnode_t *hnodes;

	/* clear types :
	 * 1 = Daily
	 * 2 = Weekly
	 * 3 = Monthly
	 * 4 = Quarterly
	 * 5 = Bi-Annually
	 * 6 = Annually
	 */ 
	/* clear channel scores if needed */
	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs))) 
	{
		tc = hnode_get(hnodes);
		if (tc) 
		{
			/* for each channel check if to be cleared */
			if (tc->resettype == cleartype) 
			{
				switch (cleartype) 
				{
					case 1:
					case 2:
					case 3:
						/* Daily Weekly and Monthly do the same */
						tc->lastreset = me.now;
						break;
					case 4:
						if (tc->lastreset < (me.now - (86 * TS_ONE_DAY))) 
							tc->lastreset = me.now;
						break;
					case 5:
						if (tc->lastreset < (me.now - (177 * TS_ONE_DAY))) 
							tc->lastreset = me.now;
						break;
					case 6:
						if (tc->lastreset < (me.now - (363 * TS_ONE_DAY))) 
							tc->lastreset = me.now;
						break;
					default:
						break;
				}
				SaveTChan(tc);
			}
		}
	}
	/* clear Network scores if needed */
	if (TriviaServ.resettype == cleartype) 
	{
		switch (cleartype) 
		{
			case 1:
			case 2:
			case 3:
				/* Daily Weekly and Monthly do the same */
				TriviaServ.lastreset = me.now;
				break;
			case 4:
				if (TriviaServ.lastreset < (me.now - (86 * TS_ONE_DAY)))
					TriviaServ.lastreset = me.now;
				break;
			case 5:
				if (TriviaServ.lastreset < (me.now - (177 * TS_ONE_DAY)))
					TriviaServ.lastreset = me.now;
				break;
			case 6:
				if (TriviaServ.lastreset < (me.now - (363 * TS_ONE_DAY)))
					TriviaServ.lastreset = me.now;
				break;
			default:
				break;
		}
		DBAStoreConfigInt("LastReset", TriviaServ.lastreset);
	}
}

static int tvs_dailytimer(void *userptr) 
{
	SET_SEGV_LOCATION();
	tvs_clearscoretimers(1);
	return NS_SUCCESS;
}

static int tvs_weeklytimer(void *userptr) 
{
	SET_SEGV_LOCATION();
	tvs_clearscoretimers(2);
	return NS_SUCCESS;
}

static int tvs_monthlytimer(void *userptr) 
{
	int i;

	SET_SEGV_LOCATION();
	for (i = 4 ; i < 7 ; i++)
		tvs_clearscoretimers(i);
	tvs_clearscoretimers(3);
	return NS_SUCCESS;
}

/** Init module
 *  Required if you need to do initialisation of your module when
 *  first loaded
 */
int ModInit( void )
{
#ifdef WIN32
	old_malloc = pcre_malloc;
	old_free = pcre_free;
	pcre_malloc = os_malloc;
	pcre_free = os_free;
#endif
	ModuleConfig (tvs_settings);
	if (DBAFetchConfigInt("LastReset", TriviaServ.lastreset) != NS_SUCCESS)
	{
		TriviaServ.lastreset = 0;
		DBAStoreConfigInt("LastReset", TriviaServ.lastreset);
	}
	userlist = list_create(LISTCOUNT_T_MAX);
	if (LoadChannels() == NS_FAILURE) 
		return NS_FAILURE;
	if (LoadQuestionFiles() == NS_FAILURE) 
		return NS_FAILURE;
	/*
	 * read the Channel Question Sets AFTER
	 * parsing the question files, so the name
	 * exists when adding to TriviaChan
	*/
	DBAFetchRows ("CQSets", LoadCQSets);
	return NS_SUCCESS;
}

/** @brief ModSynch
 *
 *  Startup handler
 *
 *  @param none
 *
 *  @return NS_SUCCESS if suceeds else NS_FAILURE
 */

int ModSynch (void)
{
	hscan_t hs;
	hnode_t *hnodes;
	TriviaChan *tc;
	Channel *c;
	
	/* Introduce a bot onto the network */
	tvs_bot = AddBot (&tvs_botinfo);
	if (!tvs_bot)
		return NS_FAILURE;
	if( TriviaServ.verbose )
		irc_chanalert (tvs_bot, "Successfully Started, %ld questions loaded", TriviaServ.Questions);
	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs)) != NULL) 
	{
		tc = hnode_get(hnodes);
		c = FindChannel (tc->name);
		if (c)
			OnlineTChan(c);
	}
	/* kick of the question/answer timer */
	AddTimer (TIMER_TYPE_INTERVAL, tvs_processtimer, "tvs_processtimer", 10, NULL);
	/* kick of the reset scores timers */
	AddTimer (TIMER_TYPE_DAILY, tvs_dailytimer, "tvs_dailytimer", 0, NULL);
	AddTimer (TIMER_TYPE_WEEKLY, tvs_weeklytimer, "tvs_weeklytimer", 0, NULL);
	AddTimer (TIMER_TYPE_MONTHLY, tvs_monthlytimer, "tvs_monthlytimer", 0, NULL);
	return NS_SUCCESS;
}

/** Init module
 *  Required if you need to do cleanup of your module when it ends
 */
int ModFini( void )
{
	lnode_t *lnodes, *ln2, *ln3, *ln4;
	hnode_t *hnodes;
	QuestionFiles *qf;
	Questions *qe;
	hscan_t hs;
	TriviaChan *tc;
	Channel *c;

	SaveAllUserScores();
	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs))) 
	{
		tc = hnode_get(hnodes);
		if (tc->c) 
		{
			c = tc->c;
			ClearChannelModValue (c);
		}
		list_destroy_nodes(tc->qfl);
		hash_scan_delete(tch, hnodes);
		hnode_destroy(hnodes);
		ns_free (tc);
	}
	lnodes = list_first(qfl);
	while (lnodes) {
		qf = lnode_get(lnodes);
		if (qf->fn)
			os_fclose (qf->fn);
		ln3 = list_first(qf->QE);
		while (ln3) 
		{
			qe = lnode_get(ln3);
			if (qe->question) 
			{
				ns_free (qe->question);
				ns_free (qe->answer);
				ns_free (qe->lasthint);
			}
			list_delete(qf->QE, ln3);
			ln4 = list_next(qf->QE, ln3);
			lnode_destroy(ln3);
			ns_free (qe);
			ln3 = ln4;
		}
		list_delete(qfl, lnodes);
		ln2 = list_next(qfl, lnodes);
		lnode_destroy(lnodes);	
		ns_free (qf);
		lnodes = ln2;
	}
#ifdef WIN32
	pcre_malloc = old_malloc;
	pcre_free = old_free;
#endif
	return NS_SUCCESS;
}
