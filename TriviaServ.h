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
	int defaultpoints;
} TriviaServ;

typedef struct QuestionFiles {
	FILE *fn;
	char filename[MAXPATH];
	list_t *QE;
	int version;
	char description[QUESTSIZE];
	/* actually just the striped filename */
	char name[QUESTSIZE];
}QuestionFiles;

typedef struct Questions {
	long qn;
	long offset;
	char *question;
	char *answer;
	char *lasthint;
	pcre *regexp;
	int hints;
	int points;
}Questions;

typedef struct TriviaChan {
	char name[MAXCHANLEN];
	Channel *c;
	int publiccontrol;
	int active;
	int scorepoints;
	time_t lastquest;
	long questtime;
	Questions *curquest;
	list_t *qfl;
}TriviaChan;

typedef struct TriviaScore {
	char nick[MAXNICK];
	int score;
	time_t lastused;
}TriviaScore;

list_t *qfl;
hash_t *tch;

extern const char *questpath;

/* TriviaServ_help.c */
extern const char *tvs_about[];
extern const char *tvs_help_set_exclusions[];
extern const char *tvs_help_set_defaultpoints[];
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
extern const char *tvs_help_sp[];
extern const char tvs_help_sp_oneline[];
extern const char *tvs_help_pc[];
extern const char tvs_help_pc_oneline[];

/* TriviaUser.c */
void tvs_addpoints(Client *u, TriviaChan *tc);
int QuitUser(CmdParams* cmdparams);
int KillUser(CmdParams* cmdparams);

/* TriviaCmds.c */
int tvs_cmd_score (CmdParams* cmdparams);
int tvs_cmd_hint (CmdParams* cmdparams);
int tvs_cmd_start (CmdParams* cmdparams);
int tvs_cmd_stop (CmdParams* cmdparams);
int tvs_cmd_qs (CmdParams* cmdparams);
int tvs_cmd_sp (CmdParams* cmdparams);
int tvs_cmd_pc (CmdParams* cmdparams);
int tvs_catlist(CmdParams* cmdparams);
int tvs_chans(CmdParams* cmdparams);

/* TriviaChan.c */
int LoadChannel( void *data );
TriviaChan *NewTChan(Channel *c);
TriviaChan *OfflineTChan(Channel *c);
TriviaChan *OnlineTChan(Channel *c);
int DelTChan(char *chan);
int SaveTChan (TriviaChan *tc);
int EmptyChan (CmdParams* cmdparams);
int NewChan(CmdParams* cmdparams);

/* TriviaQues.c */
void tvs_parse_questions();
int find_cat_name(const void *catnode, const void *name);
void tvs_quesset(CmdParams* cmdparams, TriviaChan *tc);
QuestionFiles *tvs_randomquestfile(TriviaChan *tc);
void tvs_newquest(TriviaChan *tc);
void tvs_ansquest(TriviaChan *tc);
int tvs_doregex(Questions *qe, char *buf);
void tvs_testanswer(Client* u, TriviaChan *tc, char *line);
void do_hint(TriviaChan *tc);
void obscure_question(TriviaChan *tc);

#endif /* TRIVIASERV_H */
