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

#include "neostats.h"	/* Neostats API */
#include "TriviaServ.h"
#ifdef WIN32
#define MODULE_VERSION "3.0"
#else
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <error.h>
#include <sys/dir.h>
#include <sys/param.h>
#endif

const char *questpath = "data/TSQuestions/";

static int tvs_get_settings();
static void tvs_parse_questions();
static int tvs_chans(CmdParams* cmdparams);
static int tvs_catlist(CmdParams* cmdparams);
static TriviaChan *NewTChan(Channel *);
static int SaveTChan (TriviaChan *);
static int DelTChan(char *);
int EmptyChan (CmdParams* cmdparams);
int NewChan(CmdParams* cmdparams);
static TriviaChan *OnlineTChan(Channel *c);
int tvs_processtimer (void);

static void tvs_set(CmdParams* cmdparams, TriviaChan *tc);

static void tvs_newquest(TriviaChan *);
static void tvs_ansquest(TriviaChan *);
static int tvs_doregex(Questions *, char *);
static void tvs_testanswer(Client* u, TriviaChan *tc, char *line);
static void do_hint(TriviaChan *tc);
static void obscure_question(TriviaChan *tc);
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

int tvs_cmd_score (CmdParams* cmdparams)
{
	TriviaChan *tc;

	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc) {
		return NS_FAILURE;
	}
	irc_prefmsg (tvs_bot, cmdparams->source, "Score for %s in %s", cmdparams->source->name, tc->name);
	return NS_SUCCESS;
}

int tvs_cmd_hint (CmdParams* cmdparams)
{
	TriviaChan *tc;

	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc) {
		return NS_FAILURE;
	}
	irc_prefmsg (tvs_bot, cmdparams->source, "Hint for %s in %s", cmdparams->source->name, tc->name);
	return NS_SUCCESS;
}
int tvs_cmd_start (CmdParams* cmdparams)
{
	TriviaChan *tc;

	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc) {
		return NS_FAILURE;
	}
	if ((tc->publiccontrol == 1) && (!IsChanOp(cmdparams->channel->name, cmdparams->source->name))) {
		/* nope, get lost, silently exit */
		return NS_FAILURE;
	}
	tc->active = 1;
	irc_prefmsg (tvs_bot, cmdparams->source, "Starting Trivia in %s shortly", tc->name);
	irc_chanprivmsg (tvs_bot, tc->name, "%s has activated Trivia. Get Ready for the first question!", cmdparams->source->name);
	return NS_SUCCESS;
}
int tvs_cmd_stop (CmdParams* cmdparams)
{
	TriviaChan *tc;

	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc) {
		return NS_FAILURE;
	}
	if ((tc->publiccontrol == 1) && (!IsChanOp(cmdparams->channel->name, cmdparams->source->name))) {
		/* nope, get lost, silently exit */
		return NS_FAILURE;
	}
	tc->active = 0;
	if (tc->curquest != NULL) {
		tc->curquest = NULL;
	}
	irc_prefmsg (tvs_bot, cmdparams->source, "Trivia Stoped in %s", tc->name);
	irc_chanprivmsg (tvs_bot, tc->name, "%s has stopped Trivia.", cmdparams->source->name);
	return NS_SUCCESS;
}

int tvs_cmd_sset (CmdParams* cmdparams)
{
	TriviaChan *tc;

	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc) {
		return NS_FAILURE;
	}
	/* finally, these ones are restricted always */
	if (!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) {
		/* nope, get lost */
		return NS_FAILURE;
	}
	tvs_set(cmdparams, tc);
	return NS_SUCCESS;
}

static bot_cmd tvs_commands[]=
{
	{"CHANS",	tvs_chans,		1,	NS_ULEVEL_OPER, tvs_help_chans,		tvs_help_chans_oneline },
	{"CATLIST", tvs_catlist,	0, 	0,				tvs_help_catlist,	tvs_help_catlist_oneline },
	{"SCORE",	tvs_cmd_score,	0, 	0,				NULL,	NULL},
	{"HINT",	tvs_cmd_hint,	0, 	0,				NULL,	NULL},
	{"START",	tvs_cmd_start,	0, 	0,				NULL,	NULL},
	{"STOP",	tvs_cmd_stop,	0, 	0,				NULL,	NULL},
	{"SSET",	tvs_cmd_sset,	0, 	0,				NULL,	NULL},
	{NULL,		NULL,			0, 	0,				NULL, 				NULL}
};

static bot_setting tvs_settings[]=
{
	{"USEEXCLUSIONS", 	&TriviaServ.use_exc,	SET_TYPE_BOOLEAN,	0, 0, 		NS_ULEVEL_ADMIN, "Exclusions",	NULL,	tvs_help_set_exclusions },
	{NULL,			NULL,				0,					0, 0, 	0,				 NULL,			NULL,	NULL	},
};

static int tvs_catlist(CmdParams* cmdparams) {
	lnode_t *lnode;
	QuestionFiles *qf;
	
	lnode = list_first(qfl);
	irc_prefmsg (tvs_bot, cmdparams->source, "Question Categories (%d):", (int)list_count(qfl));
	while (lnode != NULL) {
		qf = lnode_get(lnode);
		irc_prefmsg (tvs_bot, cmdparams->source, "%s) %s Questions: %d", qf->name, qf->description, (int) list_count(qf->QE));
		lnode = list_next(qfl, lnode);
	}
	irc_prefmsg (tvs_bot, cmdparams->source, "End Of List.");
	return NS_SUCCESS;
}
static int tvs_chans(CmdParams* cmdparams) {
	hscan_t hs;
	hnode_t *hnode;
	TriviaChan *tc;
	Channel *c;
	int i;
	
	if (!ircstrcasecmp(cmdparams->av[0], "ADD")) {
		if (cmdparams->ac < 3) {
			return NS_ERR_SYNTAX_ERROR;
		}
		c = FindChannel (cmdparams->av[1]);
		if (!c) {
			irc_prefmsg (tvs_bot, cmdparams->source, "Error: Channel must be online");
			return NS_FAILURE;
		}
		if ((TriviaChan *)GetChannelModValue (c)) {
			irc_prefmsg (tvs_bot, cmdparams->source, "Error: Channel already exists in the database");
			return NS_FAILURE;
		}
		tc = NewTChan(c);
		if (!tc) {
			irc_prefmsg (tvs_bot, cmdparams->source, "Error: Channel must be online");
			return NS_FAILURE;
		}
		if (!ircstrcasecmp(cmdparams->av[2], "On")) {
			tc->publiccontrol = 1;
		} else {
			tc->publiccontrol = 0;
		}
		SaveTChan(tc);
		irc_prefmsg (tvs_bot, cmdparams->source, "Added %s with public control set to %s", tc->name, tc->publiccontrol ? "On" : "Off");
		CommandReport(tvs_bot, "%s added %s with public control set to %s", cmdparams->source->name, tc->name, tc->publiccontrol ? "On" : "Off");
		irc_join (tvs_bot, tc->name, 0);
	} else if (!ircstrcasecmp(cmdparams->av[0], "DEL")) {
		if (cmdparams->ac < 2) {
			return NS_ERR_SYNTAX_ERROR;
		}
		if (DelTChan(cmdparams->av[1])) {
			irc_prefmsg (tvs_bot, cmdparams->source, "Deleted %s out of Channel List", cmdparams->av[1]);
			CommandReport(tvs_bot, "%s deleted %s out of Channel List", cmdparams->source->name, cmdparams->av[1]);
			return NS_SUCCESS;
		} else {
			irc_prefmsg (tvs_bot, cmdparams->source, "Cant find %s in channel list.", cmdparams->av[1]);
			return NS_FAILURE;
		}
	} else if (!ircstrcasecmp(cmdparams->av[0], "LIST")) {
		irc_prefmsg (tvs_bot, cmdparams->source, "Trivia Channel:");
		hash_scan_begin(&hs, tch);
		i = 0;
		while ((hnode = hash_scan_next(&hs)) != NULL) {
			tc = hnode_get(hnode);
			i++;
			irc_prefmsg (tvs_bot, cmdparams->source, "\1%d\1) %s (%s) - Public? %s", i, tc->name, (TriviaChan *)GetChannelModValue (tc->c) ? "*" : "",  tc->publiccontrol ? "Yes" : "No");
		}
		irc_prefmsg (tvs_bot, cmdparams->source, "End of list.");
	} else {
		return NS_ERR_SYNTAX_ERROR;
	}
	return NS_SUCCESS;
}



/** Channel message processing
 *  What do we do with messages in channels
 *  This is required if you want your module to respond to channel messages
 */
int ChanPrivmsg (CmdParams* cmdparams)
{
	TriviaChan *tc;
	char *tmpbuf;
	
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc) {
		return NS_FAILURE;
	}
	tmpbuf = joinbuf(cmdparams->av, cmdparams->ac, 1);
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
	return NS_SUCCESS;
}

#ifndef WIN32
int file_select (const struct direct *entry) {
	char *ptr;
	if ((strcasecmp(entry->d_name, ".")==0) || (strcasecmp(entry->d_name, "..")==0)) {
		return 0;
	}
	/* check filename extension */
	ptr = rindex(entry->d_name, '.');
	if ((ptr != NULL) && 
		(strcasecmp(ptr, ".qns") == 0)) {
			return NS_SUCCESS;
	}
	return 0;	
}
#else
char* filelist[] = {
"acronyms1.qns",
"ads1.qns",
"algebra1.qns",
#if 0
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
#endif
NULL
};
#endif

int LoadChannel( void *data )
{
	TriviaChan *tc;

	tc = ns_calloc (sizeof(TriviaChan));
	memcpy (tc, data, sizeof(TriviaChan));
	tc->qfl = list_create(-1);
	hnode_create_insert(tch, tc, tc->name);
	dlog (DEBUG1, "Loaded TC entry for Channel %s", tc->name);
	return NS_FALSE;
}

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

void tvs_parse_questions() {
	QuestionFiles *qf;
	Questions *qe;
	lnode_t *qfnode;
	char pathbuf[MAXPATH];
	char questbuf[QUESTSIZE];	
	long i = 0;
	
	/* run through each file, and only load the offsets into the Questions struct to save memory */	
	qfnode = list_first(qfl);
	while (qfnode != NULL) {
		qf = lnode_get(qfnode);
		ircsnprintf(pathbuf, MAXPATH, "%s/%s", questpath, qf->filename);
		dlog (DEBUG2, "Opening %s for reading offsets", pathbuf);
		qf->fn = os_fopen (pathbuf, "r");
		/*  if we can't open it, bail out */
		if (!qf->fn) {
			nlog(LOG_WARNING, "Couldn't Open Question File %s for Reading offsets: %s", qf->filename, os_strerror());
			qfnode = list_next(qfl, qfnode);
			continue;
		}
		/* the first line should be the version number and description */
		if (os_fgets (questbuf, QUESTSIZE, qf->fn) != NULL) {
			/* XXX Parse the Version Number */
			/* Do this post version 1.0 when we have download/update support */
			strlcpy(qf->description, questbuf, QUESTSIZE);
			strlcat(qf->description, "\0", QUESTSIZE);
			strlcpy(qf->name, qf->filename, strcspn(qf->filename, ".")+1);
		} else {
			/* couldn't load version, description. Bail out */
			nlog(LOG_WARNING, "Couldn't Load Question File Header for %s", qf->filename);
			qfnode = list_next(qfl, qfnode);
			continue;
		}
		i = 0;
		/* ok, now that its opened, we can start reading the offsets into the qe and ql entries. */
		/* use pathbuf as we don't actuall care about the data */
	
		/* THIS IS DAMN SLOW. ANY HINTS TO SPEED UP? */
		while (os_fgets (questbuf, QUESTSIZE, qf->fn) != NULL) {
			i++;
			qe = ns_calloc (sizeof(Questions));
			qe->qn = i;
			qe->offset = os_ftell (qf->fn);
			lnode_create_append(qf->QE, qe);
		}
			
		/* leave the filehandle open for later */
		nlog(LOG_NOTICE, "Finished Reading %s for Offsets (%ld)", qf->filename, i);
		qfnode = list_next(qfl, qfnode);
		TriviaServ.Questions = TriviaServ.Questions + i;
		i = 0;
	}		
	/* can't call this, because we are not online yet */
//	irc_chanalert (tvs_bot, "Successfully Loaded information for %ld questions", (long)list_count(ql));
}


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
	tc->questtime = 60;
	SetChannelModValue (c, tc);
	tc->qfl = list_create(-1);
	hnode_create_insert (tch, tc, tc->name);
	dlog (DEBUG1, "Created New TC entry for Channel %s", c->name);
	return tc;
}

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
		irc_join (tvs_bot, tc->name, 0);
		return tc;
	}
	return NULL;
}
	
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

int SaveTChan (TriviaChan *tc) 
{
	DBAStore ("Channel", tc->name, tc, sizeof (TriviaChan));
	/* XXX Save Category List */
	return NS_SUCCESS;
}

int EmptyChan (CmdParams* cmdparams)
{
	OfflineTChan(cmdparams->channel);
	return NS_SUCCESS;
}

int NewChan(CmdParams* cmdparams) 
{
	OnlineTChan(cmdparams->channel);
	return NS_SUCCESS;
}

int find_cat_name(const void *catnode, const void *name) 
{
	QuestionFiles *qf = (void *)catnode;
	return (strcasecmp(qf->name, name));
}

void tvs_set(CmdParams* cmdparams, TriviaChan *tc) 
{
	lnode_t *lnode;
	QuestionFiles *qf;
	if (cmdparams->ac >= 3 && !strcasecmp(cmdparams->av[0], "CATEGORY")) {
		if (cmdparams->ac == 5 && !strcasecmp(cmdparams->av[1], "ADD")) {
			lnode = list_find(qfl, cmdparams->av[2], find_cat_name);
			if (lnode) {
				if (list_find(tc->qfl, cmdparams->av[2], find_cat_name)) {
					irc_prefmsg (tvs_bot, cmdparams->source, "Category %s is already set for this channel", cmdparams->av[2]);
					return;
				} else {
					qf = lnode_get(lnode);
					lnode_create_append(tc->qfl, qf);
					irc_prefmsg (tvs_bot, cmdparams->source, "Added Category %s to this channel", cmdparams->av[2]);
					irc_chanprivmsg (tvs_bot, tc->name, "%s added Category %s to the list of questions", cmdparams->source->name, cmdparams->av[2]);
					SaveTChan(tc);
					return;
				}
			} else {
				irc_prefmsg (tvs_bot, cmdparams->source, "Can't Find Category %s. Try /msg %s catlist", cmdparams->av[2], tvs_bot->name);
				return;
			}
		} else if (cmdparams->ac == 5 && !strcasecmp(cmdparams->av[1], "DEL")) {
			lnode = list_find(tc->qfl, cmdparams->av[2], find_cat_name);
			if (lnode) {
				list_delete(tc->qfl, lnode);
				lnode_destroy(lnode);
				/* dun delete the QF entry. */
				irc_prefmsg (tvs_bot, cmdparams->source, "Deleted Category %s for this channel", cmdparams->av[2]);
				irc_chanprivmsg (tvs_bot, tc->name, "%s deleted category %s for this channel", cmdparams->source->name, cmdparams->av[2]);
				SaveTChan(tc);
				return;
			} else {
				irc_prefmsg (tvs_bot, cmdparams->source, "Couldn't find Category %s in the list. Try !set category list", cmdparams->av[2]);
				return;
			}
		} else if (!strcasecmp(cmdparams->av[1], "LIST")) {
			if (!list_isempty(tc->qfl)) {
				irc_prefmsg (tvs_bot, cmdparams->source, "Categories for this Channel are:");
				lnode = list_first(tc->qfl);
				while (lnode != NULL) {
					qf = lnode_get(lnode);
					irc_prefmsg (tvs_bot, cmdparams->source, "%s) %s", qf->name, qf->description);
					lnode = list_next(tc->qfl, lnode);
				}
				irc_prefmsg (tvs_bot, cmdparams->source, "End of List.");
				return;
			} else {
				irc_prefmsg (tvs_bot, cmdparams->source, "Using Random Categories for this channel");
				return;
			}
		} else if (!strcasecmp(cmdparams->av[1], "RESET")) {
			while (!list_isempty(tc->qfl)) {
				list_destroy_nodes(tc->qfl);
			}
			irc_prefmsg (tvs_bot, cmdparams->source, "Reset the Question Catagories to Default in %s", tc->name);
			irc_chanprivmsg (tvs_bot, tc->name, "%s reset the Question Categories to Default", cmdparams->source->name);
			SaveTChan(tc);
			return;
		} else {
			irc_prefmsg (tvs_bot, cmdparams->source, "Syntax Error. /msg %s help !set", tvs_bot->name);
		}
		return;		
	} else {
		irc_prefmsg (tvs_bot, cmdparams->source, "Syntax Error. /msg %s help !set", tvs_bot->name);
	}		
	irc_prefmsg (tvs_bot, cmdparams->source, "%s used Set", cmdparams->source->name);
}

int tvs_processtimer(void) 
{
	TriviaChan *tc;
	hscan_t hs;
	hnode_t *hnodes;
	time_t timediff;	
	static int newrand = 0;

	/* occasionally reseed the random generator just for fun */
	newrand++;
	if (newrand > 30 || newrand == 1)
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

QuestionFiles *tvs_randomquestfile(TriviaChan *tc) 
{
	lnode_t *lnode;
	QuestionFiles *qf;
	int qfn, i;

	if (list_isempty(tc->qfl)) {
		/* if the qfl for this chan is empty, use all qfl's */
		if(list_count(qfl) > 1)
			qfn=(unsigned)(rand()%((int)(list_count(qfl))));
		else
			qfn= 0;
		/* ok, this is bad.. but sigh, not much we can do atm. */
		lnode = list_first(qfl);
		qf = lnode_get(lnode);
		i = 0;
		while (i != qfn) {
			lnode = list_next(qfl, lnode);
			i++;
		}
		qf = lnode_get(lnode);				
		if (qf != NULL) {
			return qf;
		} else {
			nlog(LOG_WARNING, "Question File Selection (Random) for %s failed. Using first entry", tc->name);
			lnode = list_first(qfl);
			qf = lnode_get(lnode);
			return qf;			
		}
	} else {
		/* select a random question file */
		qfn=(unsigned)(rand()%((int)(list_count(tc->qfl))));
		/* ok, this is bad.. but sigh, not much we can do atm. */
		lnode = list_first(tc->qfl);
		qf = lnode_get(lnode);
		i = 0;
		while (i != qfn) {
			lnode = list_next(tc->qfl, lnode);
			i++;
		}
		qf = lnode_get(lnode);				
		if (qf != NULL) {
			return qf;
		} else {
			nlog(LOG_WARNING, "Question File Selection (Specific) for %s failed. Using first entry", tc->name);
			lnode = list_first(tc->qfl);
			qf = lnode_get(lnode);
			return qf;			
		}
	}
	/* to shut up the compilers */
	return NULL;
}	

void tvs_newquest(TriviaChan *tc) {
	int qn;
	lnode_t *qnode;
	Questions *qe;
	QuestionFiles *qf;
	char tmpbuf[512];	

	qf = tvs_randomquestfile(tc);
restartquestionselection:
	qn=(unsigned)(rand()%((int)(list_count(qf->QE)-1)));
	/* ok, this is bad.. but sigh, not much we can do atm. */
	qnode = list_first(qf->QE);
	qe = NULL;
	while (qnode != NULL) {
		qe = lnode_get(qnode);				
		if (qe->qn == qn) {
			break;
		}
		qnode = list_next(qf->QE, qnode);
	}
	/* ok, we hopefully have the Q */
	if (qe == NULL) {
		nlog(LOG_WARNING, "Eh? Never got a Question");
		goto restartquestionselection;
	}
	/* ok, now seek to the question in the file */
	if (os_fseek (qf->fn, qe->offset, SEEK_SET)) {
		nlog(LOG_WARNING, "Eh? Fseek returned a error(%s): %s", qf->filename, os_strerror());
		irc_chanalert (tvs_bot, "Question File Error in %s: %s", qf->filename, os_strerror());
		return;
	}
	if (!os_fgets (tmpbuf, 512, qf->fn)) {
		nlog(LOG_WARNING, "Eh, fgets returned null(%s): %s", qf->filename, os_strerror());
		irc_chanalert (tvs_bot, "Question file Error in %s: %s", qf->filename, os_strerror());
		return;
	}
	if (tvs_doregex(qe, tmpbuf) == NS_FAILURE) {
		irc_chanalert (tvs_bot, "Question Parsing Failed. Please Check Log File");
		return;
	}	
	tc->curquest = qe;
	irc_chanprivmsg (tvs_bot, tc->name, "Fingers on the keyboard, Here comes the Next Question!");
	obscure_question(tc);
	tc->lastquest = me.now;
}

void tvs_ansquest(TriviaChan *tc) {
	Questions *qe;
	qe = tc->curquest;
	irc_chanprivmsg (tvs_bot, tc->name, "Times Up! The Answer was: \2%s\2", qe->answer);
	/* so we don't chew up memory too much */
	ns_free (qe->regexp);
	ns_free (qe->question);
	ns_free (qe->answer);
	qe->question = NULL;
	tc->curquest = NULL;
}

int tvs_doregex(Questions *qe, char *buf) {
	pcre *re;
	const char *error;
	int errofset;
	int rc;
	int ovector[9];
	const char **subs;
	int gotanswer;
	char tmpbuf[REGSIZE], tmpbuf1[REGSIZE];	
	
	re = pcre_compile(TVSREXEXP, 0, &error, &errofset, NULL);
	if (!re) {
		nlog(LOG_WARNING, "Warning, PCRE compile failed: %s at %d", error, errofset);
		return NS_FAILURE;
	}
	gotanswer = 0;
	/* strip any newlines out */
	strip(buf);
	/* we copy the entire thing into the question struct, but it will end up as only the question after pcre does its thing */
	qe->question = ns_calloc (QUESTSIZE);
	qe->answer = ns_calloc (ANSSIZE);
	strlcpy(qe->question, buf, QUESTSIZE);
	memset (tmpbuf, 0, ANSSIZE);
	/* no, its not a infinate loop */
	while (1) {	
		rc = pcre_exec(re, NULL, qe->question, strlen(qe->question), 0, 0, ovector, 9);
		if (rc <= 0) {
			if ((rc == PCRE_ERROR_NOMATCH) && (gotanswer > 0)) {
				/* we got something in q & a, so proceed. */
				ircsnprintf(tmpbuf1, REGSIZE, ".*(?i)(?:%s).*", tmpbuf);
				printf("regexp will be %s\n", tmpbuf1);
				qe->regexp = pcre_compile(tmpbuf1, 0, &error, &errofset, NULL);
				if (qe->regexp == NULL) {
					/* damn damn damn, our constructed regular expression failed */
					nlog(LOG_WARNING, "pcre_compile_answer failed: %s at %d", error, errofset);
					ns_free (re);
					return NS_FAILURE;
				}
				ns_free (re);
				/* XXXX Random Scores? */
				qe->points = 10;
				qe->hints = 0;
				return NS_SUCCESS;
			} 
			/* some other error occured. Damn. */
			nlog(LOG_WARNING, "pcre_exec failed. %s - %d", qe->question, rc);
			ns_free (re);
			return NS_FAILURE;
		} else if (rc == 3) {
			gotanswer++;
			/* split out the regexp */
			pcre_get_substring_list(buf, ovector, rc, &subs);
			/* we pull one answer off at a time, so we place the question (and maybe another answer) into question again for further processing later */
			strlcpy(qe->question, subs[1], QUESTSIZE);
			/* if this is the first answer, this is the one we display in the channel */
			if (qe->answer[0] == 0) {
				strlcpy(qe->answer, subs[2], ANSSIZE);
			}
			/* tmpbuf will hold our eventual regular expression to find the answer in the channel */
			if (tmpbuf[0] == 0) {
				ircsnprintf(tmpbuf, ANSSIZE, "%s", subs[2]);
			} else {
				ircsnprintf(tmpbuf1, ANSSIZE, "%s|%s", tmpbuf, subs[2]);
				strlcpy(tmpbuf, tmpbuf1, ANSSIZE);
			}
			/* free our subs */
			pcre_free_substring_list(subs);
		}
	}		
	/* XXXX Random Scores? */
	qe->points = 10;
	return NS_SUCCESS;		
	
}

void tvs_testanswer(Client* u, TriviaChan *tc, char *line) 
{
	Questions *qe;
	int rc;
	
	qe = tc->curquest;
	if (qe == NULL) 
		return;
	
	dlog (DEBUG3, "Testing Answer on regexp: %s", line);
	rc = pcre_exec(qe->regexp, NULL, line, strlen(line), 0, 0, NULL, 0);
	if (rc >= 0) {
		/* we got a match! */
		irc_chanprivmsg (tvs_bot, tc->name, "Correct! %s got the answer: %s", u->name, qe->answer);
		tvs_addpoints(u, tc);
		tc->curquest = NULL;
		ns_free (qe->regexp);
		return;
	} else if (rc == -1) {
		/* no match, return silently */
		return;
	} else if (rc < 0) {
		nlog(LOG_WARNING, "pcre_exec in tvs_testanswer failed: %s - %d", line, rc);
		return;
	}
}

/* The following came from the blitzed TriviaBot. 
 * Credit goes to Andy for this function
*/
void do_hint(TriviaChan *tc) 
{
	char *out;
	Questions *qe;
	int random, num, i;

	if (tc->curquest == NULL) {
		nlog(LOG_WARNING, "curquest is missing for hint");
		return;
	}
	qe = tc->curquest;
	out = strdup(qe->answer);
	num = strlen(qe->answer) / TriviaServ.HintRatio;
	if (qe->hints > 0) {
		num = num * qe->hints;
	}
	for(i=0;i < (int)strlen(out);i++) {
		if(out[i] != ' ') {
			out[i] = '-';
		}
	}
	for(i=0;i < (num-1);i++) {
		do {
			random =  (int) ((double)rand() * (strlen(qe->answer) - 1 + 1.0) / (RAND_MAX+1.0));
		} while(out[random] == ' ' || out[random] != '-');
		out[random] = qe->answer[random];   
	} 
	qe->hints++;
	irc_chanprivmsg (tvs_bot, tc->name, "Hint %d: %s", qe->hints, out);
	ns_free (out);
}

void obscure_question(TriviaChan *tc) 
{
   char *out;
   Questions *qe;
   int random, i;
   
   if (tc->curquest == NULL) {
		nlog(LOG_WARNING, "curquest is missing for obscure_answer");
		return;
   }
   qe = tc->curquest;     
   out = ns_calloc (BUFSIZE+1);
   strlcpy(out, "\00304,01", BUFSIZE);
   for(i=0;i < (int)strlen(qe->question);i++) {
      if(qe->question[i] == ' ') {
	 random =  34 + (int) ((double)rand() * (91) / (RAND_MAX+1.0));
	 if (random == 92) random = 65;
	 /* no numbers, they screw with our colors */
	 if (random >= 48 && random <= 57) {
	 	random = random + 15;
	 }
	 /* insert same background/foreground color here */
	 strncat(out, "\00301,01", BUFSIZE);
	 /* insert random char here */
	 out[strlen(out)] = random;
	 /* reset color back to standard for next word. */
	 strncat(out, "\00304,01", BUFSIZE);
      } else {
         /* just insert the char, its a word */
         out[strlen(out)] = qe->question[i];
      }
      
   }
   irc_chanprivmsg (tvs_bot, tc->name, "%s", out);
   ns_free (out);
}

#ifdef WIN32 /* temp */

int main (int argc, char **argv)
{
	return 0;
}
#endif
