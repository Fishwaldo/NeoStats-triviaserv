/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2005 Adam Rutter, Justin Hammond, Mark Hetherington
** http://www.neostats.net/
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

#ifndef TRIVIASERV_H
#define TRIVIASERV_H

#ifndef WIN32
#include "modconfig.h"
#endif

#define TVSREXEXP "^(.*)\\*(.*)$"
#define QUESTSIZE 500
#define ANSSIZE 200
#define REGSIZE ANSSIZE + 20

extern Bot *tvs_bot;

struct TriviaServ {
	char user[MAXUSER]; 
	char host[MAXHOST]; 
	char realname[MAXREALNAME]; 
	int use_exc;
	int HintRatio;
	long Questions;
} TriviaServ;

struct QuestFile {
	FILE *fn;
	char filename[MAXPATH];
	list_t *QE;
	int version;
	char description[QUESTSIZE];
	/* actually just the striped filename */
	char name[QUESTSIZE];
};

typedef struct QuestFile QuestionFiles;

struct Quests {
	long qn;
	long offset;
	char *question;
	char *answer;
	pcre *regexp;
	int hints;
	int points;
};

typedef struct Quests Questions;

struct TChans {
	char name[MAXCHANLEN];
	Channel *c;
	int publiccontrol;
	int active;
	time_t lastquest;
	long questtime;
	Questions *curquest;
	list_t *qfl;
};

typedef struct TChans TriviaChan;

struct TScore {
	char nick[MAXNICK];
	int score;
	time_t lastused;
};

typedef struct TScore TriviaScore;

list_t *qfl;
hash_t *tch;

/* TriviaServ_help.c */
extern const char *tvs_about[];
extern const char *tvs_help_set_exclusions[];
extern const char *tvs_help_chans[];
extern const char tvs_help_chans_oneline[];
extern const char *tvs_help_catlist[];
extern const char tvs_help_catlist_oneline[];
extern const char *tvs_help_start[];
extern const char tvs_help_start_oneline[];
extern const char *tvs_help_stop[];
extern const char tvs_help_stop_oneline[];
extern const char *tvs_help_qs[];
extern const char tvs_help_qs_oneline[];
/* TriviaUser.c */
void tvs_addpoints(Client *u, TriviaChan *tc);
int QuitUser(CmdParams* cmdparams);
int KillUser(CmdParams* cmdparams);
#endif /* TRIVIASERV_H */
