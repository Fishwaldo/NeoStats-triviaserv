/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2004 Adam Rutter, Justin Hammond, Mark Hetherington
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
** $Id: TriviaServ.c 1604 2004-03-01 00:02:21Z Mark $
*/

/** TriviaServ.c 
 *  Trivia Service for NeoStats
 */

#include "neostats.h"	/* Neostats API */
#include "modconfig.h"
#include "TriviaServ.h"
#include <strings.h>
#include <error.h>

static void tvs_get_settings();
static void tvs_parse_questions();
static int tvs_about();
static int tvs_version();


static ModUser *tvs_bot;


/** 
 *  A string to hold the name of our bot
 */
char s_TriviaServ[MAXNICK];

/** Module Info definition 
 *  Information about our module
 *  This structure is required for your module to load and run on NeoStats
 */
ModuleInfo __module_info = {
	"TriviaServ",
	"Triva Service for NeoStats.",
	MODULE_VERSION,
	__DATE__,
	__TIME__
};
Functions __module_functions[] = {
	{NULL, NULL, 0}
};

static bot_cmd tvs_commands[]=
{
	{"ABOUT",	tvs_about,	0, 	NS_ULEVEL_ADMIN,	tvs_help_about, 	tvs_help_about_oneline },
	{"VERSION",	tvs_version,	0, 	NS_ULEVEL_ADMIN,	tvs_help_version,	tvs_help_version_oneline },
	{NULL,		NULL,		0, 	0,					NULL, 			NULL}
};

static bot_setting tvs_settings[]=
{
	{"NICK",		&s_TriviaServ,		SET_TYPE_NICK,		0, MAXNICK, 	NS_ULEVEL_ADMIN, "Nick",	NULL,	ns_help_set_nick },
	{"USER",		&TriviaServ.user,	SET_TYPE_USER,		0, MAXUSER, 	NS_ULEVEL_ADMIN, "User",	NULL,	ns_help_set_user },
	{"HOST",		&TriviaServ.host,	SET_TYPE_HOST,		0, MAXHOST, 	NS_ULEVEL_ADMIN, "Host",	NULL,	ns_help_set_host },
	{"REALNAME",		&TriviaServ.realname,	SET_TYPE_REALNAME,	0, MAXREALNAME, NS_ULEVEL_ADMIN, "RealName",	NULL,	ns_help_set_realname },
	{"USEEXCLUSIONS", 	&TriviaServ.use_exc,	SET_TYPE_BOOLEAN,	0, 0, 		NS_ULEVEL_ADMIN, "Exclusions",	NULL,	tvs_help_set_exclusions },
	{NULL,			NULL,				0,					0, 0, 	0,				 NULL,			NULL,	NULL	},
};

static int tvs_about(User * u, char **av, int ac)
{
	privmsg_list(u->nick, s_TriviaServ, tvs_help_about);
	return 1;
}

static int tvs_version(User * u, char **av, int ac)
{
	SET_SEGV_LOCATION();
	prefmsg(u->nick, s_TriviaServ, "\2%s Version Information\2", s_TriviaServ);
	prefmsg(u->nick, s_TriviaServ, "%s Version: %s Compiled %s at %s", __module_info.module_name,
		__module_info.module_version, __module_info.module_build_date, __module_info.module_build_time);
	prefmsg(u->nick, s_TriviaServ, "http://www.neostats.net");
	prefmsg(u->nick, s_TriviaServ, "Loaded %ld Questions out of %ld files", (long)list_count(ql), (long)list_count(qfl));
	return 1;
}




/** Channel message processing
 *  What do we do with messages in channels
 *  This is required if you want your module to respond to channel messages
 */
int __ChanMessage(char *origin, char **argv, int argc)
{
	return 1;
}

/** Online event processing
 *  What we do when we first come online
 */
static int Online(char **av, int ac)
{
	/* Introduce a bot onto the network */
	tvs_bot = init_mod_bot(s_TriviaServ, TriviaServ.user, TriviaServ.host, TriviaServ.realname,
	                        services_bot_modes, BOT_FLAG_RESTRICT_OPERS, tvs_commands, tvs_settings, __module_info.module_name);
	if (tvs_bot) {
		TriviaServ.isonline = 1;
	}
	tvs_parse_questions();
	return 1;
};

/** Module event list
 *  What events we will act on
 *  This is required if you want your module to respond to events on IRC
 *  see events.h for a list of all events available
 */
EventFnList __module_events[] = {
	{EVENT_ONLINE, Online},
	{NULL, NULL}
};

/** Init module
 *  Required if you need to do initialisation of your module when
 *  first loaded
 */
int __ModInit(int modnum, int apiver)
{
	/* Check that our compiled version if compatible with the calling version of NeoStats */
	if(	ircstrncasecmp (me.version, NEOSTATS_VERSION, VERSIONSIZE) !=0) {
		return NS_ERR_VERSION;
	}
	strlcpy(s_TriviaServ, "TriviaServ", MAXNICK);
	TriviaServ.isonline = 0;
	TriviaServ.modnum = modnum;
	
	ql = list_create(-1);
	qfl = list_create(-1);
	tch = hash_create(-1, 0, 0);
	tvs_get_settings();
	return 1;
}

/** Init module
 *  Required if you need to do cleanup of your module when it ends
 */
void __ModFini()
{

};

void tvs_get_settings() {
	QuestionFiles *qf;
	lnode_t *node;
	
	/* temp */
	ircsnprintf(TriviaServ.user, MAXUSER, "Trivia");
	ircsnprintf(TriviaServ.host, MAXHOST, "Trivia.com");
	ircsnprintf(TriviaServ.realname, MAXREALNAME, "Trivia Bot");


	qf = malloc(sizeof(QuestionFiles));
	strncpy(qf->filename, "questions.txt", MAXPATH);
	qf->fn = 0;
	node = lnode_create(qf);
	list_append(qfl, node);
	
};

void tvs_parse_questions() {
	QuestionFiles *qf;
	Questions *qe;
	lnode_t *qfnode, *qenode;
	char pathbuf[MAXPATH];
	long i = 0;
	
	/* run through each file, and only load the offsets into the Questions struct to save memory */	
	qfnode = list_first(qfl);
	while (qfnode != NULL) {
		qf = lnode_get(qfnode);
		ircsnprintf(pathbuf, MAXPATH, "data/%s", qf->filename);
		nlog(LOG_DEBUG2, LOG_MOD, "Opening %s for reading offsets", pathbuf);
		qf->fn = fopen(pathbuf, "r");
		/*  if we can't open it, bail out */
		if (qf->fn == NULL) {
			nlog(LOG_WARNING, LOG_MOD, "Couldn't Open Question File %s for Reading offsets: %s", qf->filename, strerror(errno));
			qfnode = list_next(qfl, qfnode);
			continue;
		}
		i = 0;
		/* ok, now that its opened, we can start reading the offsets into the qe and ql entries. */
		/* use pathbuf as we don't actuall care about the data */
		while (fgets(pathbuf, MAXPATH, qf->fn) != NULL) {
			i++;
			qe = malloc(sizeof(Questions));
			bzero(qe, sizeof(Questions));
			qe->qn = i;
			qe->offset = ftell(qf->fn);
			qe->QF = qf;
			qenode = lnode_create(qe);
			list_append(ql, qenode);
		}
			
		/* leave the filehandle open for later */
		nlog(LOG_NOTICE, LOG_MOD, "Finished Reading %s for Offsets (%ld)", qf->filename, (long)list_count(ql));
		qfnode = list_next(qfl, qfnode);
	}		
	chanalert(s_TriviaServ, "Successfully Loaded information for %ld questions", (long)list_count(ql));
}


TriviaChan *FindTChan(char *name) {
	Chans *c;
	
	c = findchan(name);
	
	if (!c) {
		return NULL;
	}
	if (c->moddata[TriviaServ.modnum] != NULL) {
		return (TriviaChan *)c->moddata[TriviaServ.modnum];
	}
	return NULL;
}

TriviaChan *NewTChan(Chans *c) {
	TriviaChan *tc;
	hnode_t *tcn;
	
	if (!c) {
		return NULL;
	}
	if (c->moddata[TriviaServ.modnum] != NULL) {
		nlog(LOG_WARNING, LOG_MOD, "Hrm, Chan %s already has a TriviaChanStruct with it", c->name);
		return NULL;
	}
	/* ok, first we lookup in the tch hash, to see if this is a channel that we already have a setting for */
	tcn = hash_lookup(tch, c->name);
	if (tcn == NULL) {
		/* ok, create and insert into hash */
		tc = malloc(sizeof(TriviaChan));
		bzero(tc, sizeof(TriviaChan))
		ircsnprintf(tc->name, CHANLEN, c->name);
		tc->c = c;
		c->moddata[TriviaServ.modnum] = tc;
		tcn = hnode_create(tc);
		hash_insert(tch, tcn, tc->name);
		nlog(LOG_DEBUG1, LOG_MOD, "Created New TC entry for Channel %s", c->name);
	} else {
		tc = hnode_get(tcn);
		tc->c = c;
		c->moddata[TriviaServ.modnum] = tc;
		nlog(LOG_DEBUG1, LOG_MOD, "Loaded a TC entry from hash for Channel %s", c->name);
	}
	return tc;
}

TriviaChan *OfflineTChan(Chans *c) {
	TriviaChan *tc;
	if (!c) {
		return NULL;
	}
	if (c->moddata[TriviaServ.modnum] == NULL) {
		nlog(LOG_WARNING, LOG_MOD, "TriviaChan %s already marked offline?!!?!", c->name);
		return NULL;
	}
	tc = c->moddata[TriviaServ.modnum];
	c->moddata[TriviaServ.modnum] = NULL;
	tc->c = NULL;
	return tc;
}

int DelTChan(TriviaChan *tc) {
	/* XXX todo */
	return NS_SUCCESS;
}

int SaveTChan (TriviaChan *tc) {
	/* XXX todo */
	return NS_SUCCESS;
}
