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

/** TriviaServ.c 
 *  Trivia Service for NeoStats
 */

#ifdef WIN32
#include "modconfigwin32.h"
#else
#include "modconfig.h"
#endif
#include "neostats.h"	/* Neostats API */
#include "TriviaServ.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifndef WIN32
#include <sys/dir.h>
#include <sys/param.h>
#endif

#ifdef WIN32
void *(*old_malloc)(size_t);
void (*old_free) (void *);
#endif

const char *questpath = "data/TSQuestions/";

int tvs_get_settings();
int tvs_processtimer (void);

Bot *tvs_bot;

/** Copyright info */
const char *ts_copyright[] = {
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
};

static bot_cmd tvs_commands[]=
{
	{"CHANS",	tvs_chans,	1,	NS_ULEVEL_OPER, tvs_help_chans,		tvs_help_chans_oneline },
	{"CATLIST",	tvs_catlist,	0, 	0,		tvs_help_catlist,	tvs_help_catlist_oneline },
	{"SCORE",	tvs_cmd_score,	0, 	0,		NULL,			NULL},
	{"HINT",	tvs_cmd_hint,	0, 	0,		NULL,			NULL},
	{"START",	tvs_cmd_start,	0, 	0,		tvs_help_start,		tvs_help_start_oneline},
	{"STOP",	tvs_cmd_stop,	0, 	0,		tvs_help_stop,		tvs_help_stop_oneline},
	{"QS",		tvs_cmd_qs,	1, 	0,		tvs_help_qs,		tvs_help_qs_oneline},
	{"SETPOINTS",	tvs_cmd_sp,	1, 	0,		tvs_help_sp,		tvs_help_sp_oneline},
	{"PUBLIC",	tvs_cmd_pc,	1, 	0,		tvs_help_pc,		tvs_help_pc_oneline},
	{NULL,		NULL,		0, 	0,		NULL, 			NULL}
};

static bot_setting tvs_settings[]=
{
	{"USEEXCLUSIONS", 	&TriviaServ.use_exc,		SET_TYPE_BOOLEAN,	0,	0, 		NS_ULEVEL_ADMIN,	"Exclusions",		NULL,	tvs_help_set_exclusions,	NULL,	NULL},
	{"DEFAULTPOINTS", 	&TriviaServ.defaultpoints,	SET_TYPE_INT,		1,	25, 		NS_ULEVEL_ADMIN,	"DefaultPoints",	NULL,	tvs_help_set_defaultpoints,	NULL,	(void *)1 },
	{NULL,			NULL,				0,			0,	0,		0,			NULL,			NULL,	NULL,				NULL,	NULL},
};

/*
 * Channel message processing
*/
int ChanPrivmsg (CmdParams* cmdparams)
{
	TriviaChan *tc;
	char *tmpbuf;
	int len;
	
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc) {
		return NS_FAILURE;
	}
	len = strlen( cmdparams->param ) + 1;
	tmpbuf = ns_malloc( len );
	strlcpy( tmpbuf, cmdparams->param, len );
	strip_mirc_codes(tmpbuf);
	tvs_testanswer(cmdparams->source, tc, tmpbuf);
	ns_free (tmpbuf);
	return NS_SUCCESS;
}

/** BotInfo */
static BotInfo tvs_botinfo = 
{
	"Trivia", 
	"Trivia1", 
	"TS", 
	BOT_COMMON_HOST, 
	"Trivia Bot", 	
	BOT_FLAG_SERVICEBOT|BOT_FLAG_RESTRICT_OPERS, 
	tvs_commands, 
	tvs_settings,
};

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
	if (!tvs_bot) {
		return NS_FAILURE;
	}
	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs)) != NULL) {
		tc = hnode_get(hnodes);
		c = FindChannel (tc->name);
		if (c) {
			OnlineTChan(c);
		}
	}
	/* kick of the question/answer timer */
	AddTimer (TIMER_TYPE_INTERVAL, tvs_processtimer, "tvs_processtimer", 10);
	return NS_SUCCESS;
}

/** Module Events */
ModuleEvent module_events[] = {
	{EVENT_CPRIVATE, ChanPrivmsg},		
	{EVENT_EMPTYCHAN, EmptyChan},
	{EVENT_NEWCHAN, NewChan},
	{EVENT_QUIT, QuitUser},
	{EVENT_KILL, KillUser},
	{EVENT_NULL, NULL}
};

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
	TriviaServ.Questions = 0;
	/* XXX todo */
	TriviaServ.HintRatio = 3;
	qfl = list_create(-1);
	tch = hash_create(-1, 0, 0);
	if (tvs_get_settings() == NS_FAILURE) 
		return NS_FAILURE;
	tvs_parse_questions();
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

	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs)) != NULL) {
		tc = hnode_get(hnodes);
		if (tc->c) {
			c = tc->c;
			ClearChannelModValue (c);
		}
		list_destroy_nodes(tc->qfl);
		hash_scan_delete(tch, hnodes);
		hnode_destroy(hnodes);
		ns_free (tc);
	}
	lnodes = list_first(qfl);
	while (lnodes != NULL) {
		qf = lnode_get(lnodes);
		if (qf->fn) {
			os_fclose (qf->fn);
		}
		ln3 = list_first(qf->QE);
		while (ln3 != NULL) {
			qe = lnode_get(ln3);
			if (qe->question) {
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

/*
 * Find Question files
 *
 * ToDo : fix windows to search for files instead of having them predefined
*/
#ifndef WIN32
int file_select (const struct direct *entry) {
	char *ptr;
	if ((ircstrcasecmp(entry->d_name, ".")==0) || (ircstrcasecmp(entry->d_name, "..")==0)) {
		return 0;
	}
	/* check filename extension */
	ptr = rindex(entry->d_name, '.');
	if ((ptr != NULL) && 
		(ircstrcasecmp(ptr, ".qns") == 0)) {
			return NS_SUCCESS;
	}
	return 0;	
}
#else
char* filelist[] = {
"acronyms1.qns",
"ads1.qns",
"algebra1.qns",
"animals1.qns",
"authors1.qns",
"babynames1.qns",
"bdsm.qns",
"books1.qns",
"born1.qns",
"capitals1.qns",
"castles1.qns",
"crypticgroupnames1.qns",
"darkangel1.qns",
"discworld1.qns",
"eighties.qns",
"eightiesmusic1.qns",
"eightiestvmovies1.qns",
"elements1.qns",
"farscape1.qns",
"fifties1.qns",
"fiftiesmusic1.qns",
"generalknowledge1.qns",
"history18thcenturyandbefore1.qns",
"history19thcentury1.qns",
"history20thcentury1.qns",
"licenseplates1.qns",
"links.qns",
"lyrics1.qns",
"music1.qns",
"musicterms1.qns",
"nametheyear1.qns",
"nineties1.qns",
"ninetiesmusic1.qns",
"olympics1.qns",
"onthisday1.qns",
"oz1.qns",
"phobias1.qns",
"proverbs1.qns",
"quotations1.qns",
"random.qns",
"rhymetime1.qns",
"saints1.qns",
"seventies1.qns",
"seventiesmusic1.qns",
"sexterms1.qns",
"simpsons1.qns",
"sixties1.qns",
"sixtiesmusic1.qns",
"smallville1.qns",
"sports1.qns",
"stargatesg11.qns",
"tvmovies1.qns",
"uk1.qns",
"unscramble1.qns",
"us1.qns",
"uselessfactsandtrivia1.qns",
NULL
};
#endif

/*
 * load settings
*/
int tvs_get_settings() {
	QuestionFiles *qf;
	int i, count = 0;
#ifndef WIN32
	struct direct **files;

	/* Scan the questions directory for question files, and create the hashs */
	count = scandir (questpath, &files, file_select, alphasort);
#else
	{
		char** pfilelist = filelist;
		while(*pfilelist) {
			count ++;
			pfilelist ++;
		}
	}
#endif
	if (count <= 0) {
		nlog(LOG_CRITICAL, "No Question Files Found");
		return NS_FAILURE;
	}
	for (i = 1; i<count; i++) {
		qf = ns_calloc (sizeof(QuestionFiles));
#ifndef WIN32
		strlcpy(qf->filename, files[i-1]->d_name, MAXPATH);
#else
		strlcpy(qf->filename, filelist[i-1], MAXPATH);
#endif
		qf->QE = list_create(-1);
		lnode_create_append(qfl, qf);
	}
	DBAFetchRows ("Channel", LoadChannel);
	return NS_SUCCESS;
};

/*
 * Process Timer to kick off hints or new questions when required
*/
int tvs_processtimer(void) 
{
	TriviaChan *tc;
	hscan_t hs;
	hnode_t *hnodes;
	time_t timediff;	

	/* reseed the random generator */
	if (!(rand() % 4)) 
		srand((unsigned int)me.now);
	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs)) != NULL) {
		tc = hnode_get(hnodes);
		if ((tc->c != NULL) && (tc->active == 1)) {
			/* ok, channel is active. if curquest is null, ask a new question */
			if (tc->curquest == NULL) {
				tvs_newquest(tc);
				continue;
			}
			/* if we are here, its a current question. */
			timediff = me.now - tc->lastquest;
			if (timediff > tc->questtime) {
				/* timeout, answer the question */
				tvs_ansquest(tc);
				continue;
			}
			/* hint processor */
			if ((tc->questtime - timediff) < 15) {
				irc_chanprivmsg (tvs_bot, tc->name, "Less than 15 Seconds Remaining, Hurry up!");
				continue;
			}
			do_hint(tc);
		}
	}
	return NS_SUCCESS;
}
