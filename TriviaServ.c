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
** $Id$
*/

/** TriviaServ.c 
 *  Trivia Service for NeoStats
 */

#include "neostats.h"	/* Neostats API */
#include "modconfig.h"
#include "TriviaServ.h"
#include <strings.h>
#include <error.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>



static int tvs_get_settings();
static void tvs_parse_questions();
static int tvs_about();
static int tvs_version();
static int tvs_chans();
static int tvs_catlist();
static TriviaChan *FindTChan(char *);
static TriviaChan *NewTChan(Chans *);
static int SaveTChan (TriviaChan *);
static int DelTChan(char *);
int PartChan(char **av, int ac);
int NewChan(char **av, int ac);
static TriviaChan *OnlineTChan(Chans *c);

static void tvs_sendhelp(char *);
static void tvs_sendscore(char *, TriviaChan *);
static void tvs_sendhint(char *, TriviaChan *);
static void tvs_starttriv(char *, TriviaChan *);
static void tvs_stoptriv(char *, TriviaChan *);
static void tvs_set(char *, TriviaChan *, char **, int );

static void tvs_newquest(TriviaChan *);
static void tvs_ansquest(TriviaChan *);
static int tvs_doregex(Questions *, char *);
static void tvs_testanswer(char *origin, TriviaChan *tc, char *line);
static void do_hint(TriviaChan *tc);
static void obscure_question(TriviaChan *tc);
static ModUser *tvs_bot;



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
	{"ABOUT",	tvs_about,	0, 	NS_ULEVEL_OPER,		tvs_help_about, 	tvs_help_about_oneline },
	{"VERSION",	tvs_version,	0, 	NS_ULEVEL_OPER,		tvs_help_version,	tvs_help_version_oneline },
	{"CHANS",	tvs_chans,	1,	NS_ULEVEL_OPER, 	tvs_help_chans,		tvs_help_chans_oneline },
	{"CATLIST", 	tvs_catlist, 	0, 	0,			tvs_help_catlist, 	tvs_help_catlist_oneline },
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
	prefmsg(u->nick, s_TriviaServ, "Loaded %ld Questions out of %ld files", TriviaServ.Questions, (long)list_count(qfl));
	return 1;
}

static int tvs_catlist(User *u, char **av, int ac) {
	lnode_t *lnode;
	QuestionFiles *qf;
	
	lnode = list_first(qfl);
	prefmsg(u->nick, s_TriviaServ, "Question Categories (%d):", (int)list_count(qfl));
	while (lnode != NULL) {
		qf = lnode_get(lnode);
		prefmsg(u->nick, s_TriviaServ, "%s) %s Questions: %d", qf->name, qf->description, (int) list_count(qf->QE));
		lnode = list_next(qfl, lnode);
	}
	prefmsg(u->nick, s_TriviaServ, "End Of List.");
	return NS_SUCCESS;
}
static int tvs_chans(User *u, char **av, int ac) {
	hscan_t hs;
	hnode_t *hnode;
	TriviaChan *tc;
	Chans *c;
	int i;
	
	if (!ircstrcasecmp(av[2], "ADD")) {
		if (ac < 5) {
			prefmsg(u->nick, s_TriviaServ, "Invalid Syntax. /msg %s help chans", s_TriviaServ);
			return NS_FAILURE;
		}
		c = findchan(av[3]);
		if (!c) {
			prefmsg(u->nick, s_TriviaServ, "Error: Channel must be online");
			return NS_FAILURE;
		}
		if (FindTChan(av[3])) {
			prefmsg(u->nick, s_TriviaServ, "Error: That Channel already exists in the database");
			return NS_FAILURE;
		}
		tc = NewTChan(c);
		if (!tc) {
			prefmsg(u->nick, s_TriviaServ, "Error: Channel must be online");
			return NS_FAILURE;
		}
		if (!ircstrcasecmp(av[4], "On")) {
			tc->publiccontrol = 1;
		} else {
			tc->publiccontrol = 0;
		}
		SaveTChan(tc);
		prefmsg(u->nick, s_TriviaServ, "Added %s with public control set to %s", tc->name, tc->publiccontrol ? "On" : "Off");
		chanalert(s_TriviaServ, "%s added %s with public control set to %s", u->nick, tc->name, tc->publiccontrol ? "On" : "Off");
		join_bot_to_chan(s_TriviaServ, tc->name, 0);
	} else if (!ircstrcasecmp(av[2], "DEL")) {
		if (ac < 4) {
			prefmsg(u->nick, s_TriviaServ, "Invalid Syntax. /msg %s help chans", s_TriviaServ);
			return NS_FAILURE;
		}
		if (DelTChan(av[3])) {
			prefmsg(u->nick, s_TriviaServ, "Deleted %s out of Channel List", av[3]);
			chanalert(s_TriviaServ, "%s deleted %s out of Channel List", u->nick, av[3]);
			return NS_SUCCESS;
		} else {
			prefmsg(u->nick, s_TriviaServ, "Cant find %s in channel list.", av[3]);
			return NS_FAILURE;
		}
	} else if (!ircstrcasecmp(av[2], "LIST")) {
		prefmsg(u->nick, s_TriviaServ, "Trivia Chans:");
		hash_scan_begin(&hs, tch);
		i = 0;
		while ((hnode = hash_scan_next(&hs)) != NULL) {
			tc = hnode_get(hnode);
			i++;
			prefmsg(u->nick, s_TriviaServ, "\1%d\1) %s (%s) - Public? %s", i, tc->name, FindTChan(tc->name) ? "*" : "",  tc->publiccontrol ? "Yes" : "No");
		}
		prefmsg(u->nick, s_TriviaServ, "End of List.");
	} else {
		prefmsg(u->nick, s_TriviaServ, "Invalid Syntax. /msg %s help Chans", s_TriviaServ);
	}
	return NS_SUCCESS;
}



/** Channel message processing
 *  What do we do with messages in channels
 *  This is required if you want your module to respond to channel messages
 */
int __ChanMessage(char *origin, char **argv, int argc)
{
	TriviaChan *tc;
	char *tmpbuf;
	
	if (argc <= 1) {
		return NS_FAILURE;
	}
	/* find if its our channel. */
	tc = FindTChan(argv[0]);
	if (!tc) {
		return NS_FAILURE;
	}
	/* if first char is a ! its a command */
	if (argv[1][0] == '!') {
		if (!ircstrcasecmp("!help", argv[1])) {
			tvs_sendhelp(origin);
		} else if (!ircstrcasecmp("!score", argv[1])) {
			tvs_sendscore(origin, tc);
		} else if (!ircstrcasecmp("!hint", argv[1])) {
			tvs_sendhint(origin, tc);
		}
		/* if we get here, then the following commands are limited if publiccontrol is enabled */
		if ((tc->publiccontrol == 1) && (!is_chanop(argv[0], origin))) {
			/* nope, get lost, silently exit */
			printf("haha, nope\n");
			return NS_FAILURE;
		}
		if (!ircstrcasecmp("!start", argv[1])) {
			tvs_starttriv(origin, tc);
		} else if (!ircstrcasecmp("!stop", argv[1])) {
			tvs_stoptriv(origin, tc);
		}
		/* finally, these ones are restricted always */
		if (!is_chanop(argv[0], origin)) {
			/* nope, get lost */
			printf("not on your life\n");
			return NS_FAILURE;
		}
		if (!ircstrcasecmp("!set", argv[1])) {
			tvs_set(origin, tc, argv, argc);
		}
		/* when we get here, just exit out */
		return NS_SUCCESS;
	}
	/* XXX if we are here, it could be a answer, process it. */	
	tmpbuf = joinbuf(argv, argc, 1);
	strip_mirc_codes(tmpbuf);
	tvs_testanswer(origin, tc, tmpbuf);
	free(tmpbuf);
	return 1;
}

/** Online event processing
 *  What we do when we first come online
 */
static int Online(char **av, int ac)
{
	hscan_t hs;
	hnode_t *hnodes;
	TriviaChan *tc;
	Chans *c;
	
	/* Introduce a bot onto the network */
	tvs_bot = init_mod_bot(s_TriviaServ, TriviaServ.user, TriviaServ.host, TriviaServ.realname,
	                        services_bot_modes, BOT_FLAG_RESTRICT_OPERS, tvs_commands, tvs_settings, __module_info.module_name);
	if (tvs_bot) {
		TriviaServ.isonline = 1;
	}

	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs)) != NULL) {
		tc = hnode_get(hnodes);
		c = findchan(tc->name);
		if (c) {
			OnlineTChan(c);
		}
	}

	/* kick of the question/answer timer */
	add_mod_timer("tvs_processtimer", "TriviaServ Process Timer",  __module_info.module_name, 10);

	return 1;
};

/** Module event list
 *  What events we will act on
 *  This is required if you want your module to respond to events on IRC
 *  see events.h for a list of all events available
 */
EventFnList __module_events[] = {
	{EVENT_ONLINE, Online},
	{EVENT_PARTCHAN, PartChan},
	{EVENT_KICK, PartChan},
	{EVENT_NEWCHAN, NewChan},
	{EVENT_SIGNOFF, DelUser},
	{EVENT_KILL, DelUser},
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
	TriviaServ.Questions = 0;
	/* XXX todo */
	TriviaServ.HintRatio = 3;
	qfl = list_create(-1);
	tch = hash_create(-1, 0, 0);
	if (tvs_get_settings() == NS_FAILURE) 
		return NS_FAILURE;
	tvs_parse_questions();

	return 1;
}

/** Init module
 *  Required if you need to do cleanup of your module when it ends
 */
void __ModFini()
{
	lnode_t *lnodes, *ln2, *ln3, *ln4;
	hnode_t *hnodes;
	QuestionFiles *qf;
	Questions *qe;
	hscan_t hs;
	TriviaChan *tc;
	Chans *c;
	
	
	hash_scan_begin(&hs, tch);
	while ((hnodes = hash_scan_next(&hs)) != NULL) {
		tc = hnode_get(hnodes);
		if (tc->c) {
			c = tc->c;
			c->moddata[TriviaServ.modnum] = NULL;
		}
		list_destroy_nodes(tc->qfl);
		hash_scan_delete(tch, hnodes);
		hnode_destroy(hnodes);
		free(tc);
	}
	lnodes = list_first(qfl);
	while (lnodes != NULL) {
		qf = lnode_get(lnodes);
		if (qf->fn) {
			fclose(qf->fn);
		}
		ln3 = list_first(qf->QE);
		while (ln3 != NULL) {
			qe = lnode_get(ln3);
			if (qe->question) {
				free(qe->question);
				free(qe->answer);
			}
			list_delete(qf->QE, ln3);
			ln4 = list_next(qf->QE, ln3);
			lnode_destroy(ln3);
			free(qe);
			ln3 = ln4;
		}

		list_delete(qfl, lnodes);
		ln2 = list_next(qfl, lnodes);
		lnode_destroy(lnodes);	
		free(qf);
		lnodes = ln2;
	}
};

int file_select (struct direct *entry) {
	char *ptr;
	if ((strcmp(entry->d_name, ".")==0) || (strcmp(entry->d_name, "..")==0)) {
		return 0;
	}
	/* check filename extension */
	ptr = rindex(entry->d_name, '.');
	if ((ptr != NULL) && 
		(strcmp(ptr, ".qns") == 0)) {
			return NS_SUCCESS;
	}
	return 0;	
}

int tvs_get_settings() {
	QuestionFiles *qf;
	lnode_t *node;
	char **row;
	TriviaChan *tc;
	hnode_t *tcn;
	int i, count;
	struct direct **files;
	
	/* temp */
	ircsnprintf(TriviaServ.user, MAXUSER, "Trivia");
	ircsnprintf(TriviaServ.host, MAXHOST, "Trivia.com");
	ircsnprintf(TriviaServ.realname, MAXREALNAME, "Trivia Bot");


	/* Scan the questions directory for question files, and create the hashs */
	count = scandir("data/TSQuestions/", &files, file_select, alphasort);
	if (count <= 0) {
		nlog(LOG_CRITICAL, LOG_MOD, "No Question Files Found");
		return NS_FAILURE;
	}
	for (i = 1; i<count; i++) {
		qf = malloc(sizeof(QuestionFiles));
		strncpy(qf->filename, files[i-1]->d_name, MAXPATH);
		qf->fn = 0;
		qf->QE = list_create(-1);
		node = lnode_create(qf);
		list_append(qfl, node);
	}
	/* load the channel list */
	if (GetTableData("Chans", &row) > 0) {
		for (i = 0; row[i] != NULL; i++) {
			tc = malloc(sizeof(TriviaChan));
			bzero(tc, sizeof(TriviaChan))
			ircsnprintf(tc->name, CHANLEN, row[i]);
			GetData((void *)&tc->publiccontrol, CFGINT, "Chans", row[i], "Public");
			if (GetData((void *)&tc->questtime, CFGINT, "Chans", row[i], "Timeout") < 0) {
				tc->questtime = 60;
			}
			tc->qfl = list_create(-1);
			tcn = hnode_create(tc);
			hash_insert(tch, tcn, tc->name);
			nlog(LOG_DEBUG1, LOG_MOD, "Loaded TC entry for Channel %s", tc->name);
		}
	}
	return NS_SUCCESS;
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
		ircsnprintf(pathbuf, MAXPATH, "data/TSQuestions/%s", qf->filename);
		nlog(LOG_DEBUG2, LOG_MOD, "Opening %s for reading offsets", pathbuf);
		qf->fn = fopen(pathbuf, "r");
		/*  if we can't open it, bail out */
		if (qf->fn == NULL) {
			nlog(LOG_WARNING, LOG_MOD, "Couldn't Open Question File %s for Reading offsets: %s", qf->filename, strerror(errno));
			qfnode = list_next(qfl, qfnode);
			continue;
		}

		/* the first line should be the version number and description */
		if (fgets(pathbuf, QUESTSIZE, qf->fn) != NULL) {
			/* XXX Parse the Version Number */
			/* Do this post version 1.0 when we have download/update support */
			strlcpy(qf->description, pathbuf, strlen(pathbuf)-2);
			strcat(qf->description, "\0");
			strlcpy(qf->name, qf->filename, strcspn(qf->filename, ".")+1);
		} else {
			/* couldn't load version, description. Bail out */
			nlog(LOG_WARNING, LOG_MOD, "Couldn't Load Question File Header for %s", qf->filename);
			qfnode = list_next(qfl, qfnode);
			continue;
		}
		i = 0;
		/* ok, now that its opened, we can start reading the offsets into the qe and ql entries. */
		/* use pathbuf as we don't actuall care about the data */
	
		/* THIS IS DAMN SLOW. ANY HINTS TO SPEED UP? */
		while (fgets(pathbuf, MAXPATH, qf->fn) != NULL) {
			i++;
			qe = malloc(sizeof(Questions));
//			bzero(qe, sizeof(Questions));
			qe->qn = i;
			qe->offset = ftell(qf->fn);
			qenode = lnode_create(qe);
			list_append(qf->QE, qenode);
		}
			
		/* leave the filehandle open for later */
		nlog(LOG_NOTICE, LOG_MOD, "Finished Reading %s for Offsets (%ld)", qf->filename, i);
		qfnode = list_next(qfl, qfnode);
		TriviaServ.Questions = TriviaServ.Questions + i;
		i = 0;
	}		
	/* can't call this, because we are not online yet */
//	chanalert(s_TriviaServ, "Successfully Loaded information for %ld questions", (long)list_count(ql));
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
		return (TriviaChan *)c->moddata[TriviaServ.modnum];
	}
	/* ok, first we lookup in the tch hash, to see if this is a channel that we already have a setting for */
	tcn = hash_lookup(tch, c->name);
	if (tcn == NULL) {
		/* ok, create and insert into hash */
		tc = malloc(sizeof(TriviaChan));
		bzero(tc, sizeof(TriviaChan))
		ircsnprintf(tc->name, CHANLEN, c->name);
		tc->c = c;
		/* XXX */
		tc->questtime = 60;
		c->moddata[TriviaServ.modnum] = tc;
		tc->qfl = list_create(-1);
		tcn = hnode_create(tc);
		hash_insert(tch, tcn, tc->name);
		nlog(LOG_DEBUG1, LOG_MOD, "Created New TC entry for Channel %s", c->name);
	} else {
		return NULL;
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
	tc->active = 0;
	tc->curquest = NULL;
	spart_cmd(s_TriviaServ, c->name);
	return tc;
}

TriviaChan *OnlineTChan(Chans *c) {
	TriviaChan *tc;
	hnode_t *tcn;
	if (!c) {
		return NULL;
	}
	if (c->moddata[TriviaServ.modnum] != NULL) {
		nlog(LOG_WARNING, LOG_MOD, "TriviaChan %s already marked online?!?!", c->name);
		return (TriviaChan *)c->moddata[TriviaServ.modnum];
	}
	tcn = hash_lookup(tch, c->name);
	if (tcn != NULL) {
		tc = hnode_get(tcn);
		tc->c = c;
		c->moddata[TriviaServ.modnum] = tc;
		join_bot_to_chan(s_TriviaServ, tc->name, 0);
		return tc;
	} else {
		return NULL;
	}	
}
	
int DelTChan(char *chan) {
	hnode_t *hnode;
	TriviaChan *tc;
	
	hnode = hash_lookup(tch, chan);
	if (hnode) {
		tc = hnode_get(hnode);
		/* part the channel if its online */
		if (FindTChan(tc->name)) OfflineTChan(findchan(tc->name));
		hash_delete(tch, hnode);
		list_destroy_nodes(tc->qfl);
		free(tc);
		hnode_destroy(hnode);
		/* XXX del out of database */
		return NS_SUCCESS;
	} else {
		return NS_FAILURE;
	}
}

int SaveTChan (TriviaChan *tc) {

	SetData((void *)tc->publiccontrol, CFGINT, "Chans", tc->name, "Public");
	/* XXX Save Category List */
	return NS_SUCCESS;
}


int PartChan(char **av, int ac) {
	Chans *c;
	c = findchan(av[0]);
	if (FindTChan(av[0]) && (c->cur_users == 2)) {
		/* last user leaving, so we go offline on this channel */
		OfflineTChan(c);
	}
	return NS_SUCCESS;
}

int NewChan(char **av, int ac) {
	Chans *c;
	if (TriviaServ.isonline != 1) 
		return NS_FAILURE;
	c = findchan(av[0]);
	OnlineTChan(c);
	return NS_SUCCESS;
}

void tvs_sendhelp(char *who) {
	prefmsg(who, s_TriviaServ, "This is help");
}

void tvs_sendscore(char *who, TriviaChan *tc) {
	prefmsg(who, s_TriviaServ, "Score for %s in %s", who, tc->name);
}

void tvs_sendhint(char *who, TriviaChan *tc) {
	prefmsg(who, s_TriviaServ, "Hint for %s in %s", who, tc->name);
}

void tvs_starttriv(char *who, TriviaChan *tc) {
	tc->active = 1;
	prefmsg(who, s_TriviaServ, "Starting Trivia in %s shortly", tc->name);
	/* use privmsg to send to a channel instead */
	privmsg(tc->name, s_TriviaServ, "%s has activated Trivia. Get Ready for the first question!", who);
}
void tvs_stoptriv(char *who, TriviaChan *tc) {
	tc->active = 0;
	if (tc->curquest != NULL) {
		tc->curquest = NULL;
	}
	prefmsg(who, s_TriviaServ, "Trivia Stoped in %s", tc->name);
	privmsg(tc->name, s_TriviaServ, "%s has stopped Trivia.", who);
}

int find_cat_name(const void *catnode, const void *name) {
	QuestionFiles *qf = (void *)catnode;
	return (strcasecmp(qf->name, name));
}
	


void tvs_set(char *who, TriviaChan *tc, char **av, int ac) {
	lnode_t *lnode;
	QuestionFiles *qf;
	if (ac >= 3 && !strcasecmp(av[2], "CATEGORY")) {
		if (ac == 5 && !strcasecmp(av[3], "ADD")) {
			lnode = list_find(qfl, av[4], find_cat_name);
			if (lnode) {
				if (list_find(tc->qfl, av[4], find_cat_name)) {
					prefmsg(who, s_TriviaServ, "Category %s is already set for this channel", av[4]);
					return;
				} else {
					qf = lnode_get(lnode);
					list_append(tc->qfl, lnode_create(qf));
					prefmsg(who, s_TriviaServ, "Added Category %s to this channel", av[4]);
					privmsg(tc->name, s_TriviaServ, "%s added Category %s to the list of questions", who, av[4]);
					SaveTChan(tc);
					return;
				}
			} else {
				prefmsg(who, s_TriviaServ, "Can't Find Category %s. Try /msg %s catlist", av[4], s_TriviaServ);
				return;
			}
		} else if (ac == 5 && !strcasecmp(av[3], "DEL")) {
			lnode = list_find(tc->qfl, av[4], find_cat_name);
			if (lnode) {
				list_delete(tc->qfl, lnode);
				lnode_destroy(lnode);
				/* dun delete the QF entry. */
				prefmsg(who, s_TriviaServ, "Deleted Category %s for this channel", av[4]);
				privmsg(tc->name, s_TriviaServ, "%s deleted category %s for this channel", who, av[4]);
				SaveTChan(tc);
				return;
			} else {
				prefmsg(who, s_TriviaServ, "Couldn't find Category %s in the list. Try !set category list", av[4]);
				return;
			}
		} else if (!strcasecmp(av[3], "LIST")) {
			if (!list_isempty(tc->qfl)) {
				prefmsg(who, s_TriviaServ, "Categories for this Channel are:");
				lnode = list_first(tc->qfl);
				while (lnode != NULL) {
					qf = lnode_get(lnode);
					prefmsg(who, s_TriviaServ, "%s) %s", qf->name, qf->description);
					lnode = list_next(tc->qfl, lnode);
				}
				prefmsg(who, s_TriviaServ, "End of List.");
				return;
			} else {
				prefmsg(who, s_TriviaServ, "Using Random Categories for this channel");
				return;
			}
		} else if (!strcasecmp(av[3], "RESET")) {
			while (!list_isempty(tc->qfl)) {
				list_destroy_nodes(tc->qfl);
			}
			prefmsg(who, s_TriviaServ, "Reset the Question Catagories to Default in %s", tc->name);
			privmsg(tc->name, s_TriviaServ, "%s reset the Question Categories to Default", who);
			SaveTChan(tc);
			return;
		} else {
			prefmsg(who, s_TriviaServ, "Syntax Error. /msg %s help !set", s_TriviaServ);
		}
		return;		
	} else {
		prefmsg(who, s_TriviaServ, "Syntax Error. /msg %s help !set", s_TriviaServ);
	}
			
	prefmsg(who, s_TriviaServ, "%s used Set", who);
}

void tvs_processtimer() {
	TriviaChan *tc;
	hscan_t hs;
	hnode_t *hnodes;
	long timediff;	
	static int newrand = 0;

	/* occasionally reseed the random generator just for fun */
	newrand++;
	if (newrand > 30 || newrand == 1)
		srand(me.now);

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
				privmsg(tc->name, s_TriviaServ, "Less than 15 Seconds Remaining, Hurry up!");
				continue;
			}
			do_hint(tc);
//			privmsg(tc->name, s_TriviaServ, "Check");
		}
	}
}

QuestionFiles *tvs_randomquestfile(TriviaChan *tc) {
	lnode_t *lnode;
	QuestionFiles *qf;
	int qfn, i;
	if (list_isempty(tc->qfl)) {
		/* if the qfl for this chan is empty, use all qfl's */
		qfn=(unsigned)(rand()%((int)(list_count(qfl))));
		/* ok, this is bad.. but sigh, not much we can do atm. */
		lnode = list_first(qfl);
		qf = lnode_get(lnode);
		i = 0;
		while (i != qfn) {
			qf = lnode_get(lnode);				
			lnode = list_next(qfl, lnode);
			i++;
		}
		if (qf != NULL) {
			return qf;
		} else {
			nlog(LOG_WARNING, LOG_MOD, "Question File Selection (Random) for %s failed. Using first entry", tc->name);
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
			qf = lnode_get(lnode);				
			lnode = list_next(tc->qfl, lnode);
			i++;
		}
		if (qf != NULL) {
			return qf;
		} else {
			nlog(LOG_WARNING, LOG_MOD, "Question File Selection (Specific) for %s failed. Using first entry", tc->name);
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
		nlog(LOG_WARNING, LOG_MOD, "Eh? Never got a Question");
		goto restartquestionselection;
	}

	/* ok, now seek to the question in the file */

	if (fseek(qf->fn, qe->offset, SEEK_SET)) {
		nlog(LOG_WARNING, LOG_MOD, "Eh? Fseek returned a error(%s): %s", qf->filename, strerror(errno));
		chanalert(s_TriviaServ, "Question File Error in %s: %s", qf->filename, strerror(errno));
		return;
	}
	if (!fgets(tmpbuf, 512, qf->fn)) {
		nlog(LOG_WARNING, LOG_MOD, "Eh, fgets returned null(%s): %s", qf->filename, strerror(errno));
		chanalert(s_TriviaServ, "Question file Error in %s: %s", qf->filename, strerror(errno));
		return;
	}
	if (tvs_doregex(qe, tmpbuf) == NS_FAILURE) {
		chanalert(s_TriviaServ, "Question Parsing Failed. Please Check Log File");
		return;
	}	
	tc->curquest = qe;
	privmsg(tc->name, s_TriviaServ, "Fingers on the keyboard, Here comes the Next Question!");
//	privmsg(tc->name, s_TriviaServ, "%s", qe->question);
	obscure_question(tc);
	tc->lastquest = me.now;

}

void tvs_ansquest(TriviaChan *tc) {
	Questions *qe;
	qe = tc->curquest;
	privmsg(tc->name, s_TriviaServ, "Times Up! The Answer was: \2%s\2", qe->answer);
	/* so we don't chew up memory too much */
	free(qe->regexp);
	free(qe->question);
	free(qe->answer);
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
		nlog(LOG_WARNING, LOG_MOD, "Warning, PCRE compile failed: %s at %d", error, errofset);
		return NS_FAILURE;
	}
	gotanswer = 0;
	/* strip any newlines out */
	strip(buf);
	/* we copy the entire thing into the question struct, but it will end up as only the question after pcre does its thing */
	qe->question = malloc(QUESTSIZE);
	qe->answer = malloc(ANSSIZE);
	strlcpy(qe->question, buf, QUESTSIZE);
	bzero(tmpbuf, ANSSIZE);
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
					nlog(LOG_WARNING, LOG_MOD, "pcre_compile_answer failed: %s at %d", error, errofset);
					free(re);
					return NS_FAILURE;
				}
				free(re);
				/* XXXX Random Scores? */
				qe->points = 10;
				qe->hints = 0;
				return NS_SUCCESS;
			} 
			/* some other error occured. Damn. */
			nlog(LOG_WARNING, LOG_MOD, "pcre_exec failed. %s - %d", qe->question, rc);
			free(re);
			return NS_FAILURE;
		} else if (rc == 3) {
			gotanswer++;
			/* split out the regexp */
			pcre_get_substring_list(buf, ovector, rc, &subs);
			/* we pull one answer off at a time, so we place the question (and maybe another answer) into question again for further processing later */
			strlcpy(qe->question, subs[1], QUESTSIZE);
			/* if this is the first answer, this is the one we display in the channel */
			if (strlen(qe->answer) == 0) {
				strlcpy(qe->answer, subs[2], ANSSIZE);
			}
			/* tmpbuf will hold our eventual regular expression to find the answer in the channel */
			if (strlen(tmpbuf) == 0) {
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

void tvs_testanswer(char *origin, TriviaChan *tc, char *line) {
	Questions *qe;
	int rc;
	
	qe = tc->curquest;
	if (qe == NULL) 
		return;
	
	nlog(LOG_DEBUG3, LOG_MOD, "Testing Answer on regexp: %s", line);
	rc = pcre_exec(qe->regexp, NULL, line, strlen(line), 0, 0, NULL, 0);
	if (rc >= 0) {
		/* we got a match! */
		privmsg(tc->name, s_TriviaServ, "Correct! %s got the answer: %s", origin, qe->answer);
		tvs_addpoints(origin, tc);
		tc->curquest = NULL;
		free(qe->regexp);
		return;
	} else if (rc == -1) {
		/* no match, return silently */
		return;
	} else if (rc < 0) {
		nlog(LOG_WARNING, LOG_MOD, "pcre_exec in tvs_testanswer failed: %s - %d", line, rc);
		return;
	}
}

/* The following came from the blitzed TriviaBot. 
 * Credit goes to Andy for this function
*/

void do_hint(TriviaChan *tc) {
   char *out;
   Questions *qe;
   int random, num, i;
 
 
   if (tc->curquest == NULL) {
	nlog(LOG_WARNING, LOG_MOD, "curquest is missing for hint");
	return;
   }
#if 0
   srand(( unsigned)time(NULL));

	/* answer is too small */
   if((int) strlen(hintanswer) < config->GAME_MinHint) {
      client->privmsg(config->IRC_Channel, config->TEXT_GAME_Toosmall);
      return;  
    }
#endif
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

//   out[0] = qe->answer[0];   

   for(i=0;i < (num-1);i++) {
       do {
          random =  (int) ((double)rand() * (strlen(qe->answer) - 1 + 1.0) / (RAND_MAX+1.0));
       } while(out[random] == ' ' || out[random] != '-');

        out[random] = qe->answer[random];   
    }
   
   qe->hints++;
   privmsg(tc->name, s_TriviaServ, "Hint %d: %s", qe->hints, out);
   free(out);
}

void obscure_question(TriviaChan *tc) {
   char *out;
   Questions *qe;
   int random, i;
   int donecolor;
   
   if (tc->curquest == NULL) {
	nlog(LOG_WARNING, LOG_MOD, "curquest is missing for obscure_answer");
	return;
   }

   qe = tc->curquest;
      
   out = malloc(BUFSIZE+1);
   bzero(out, BUFSIZE+1);
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
   privmsg(tc->name, s_TriviaServ, "%s", out);
   free(out);


}
