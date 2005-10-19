/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2005 Adam Rutter, Justin Hammond, Mark Hetherington, DeadNotBuried
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

#include MODULECONFIG

#define TVSREXEXP "^(.*)\\*(.*)$"
#define QUESTSIZE 500
#define ANSSIZE 200
#define REGSIZE ( ANSSIZE + 20 )

extern Bot *tvs_bot;

struct TriviaServ {
	char user[MAXUSER]; 
	char host[MAXHOST]; 
	char realname[MAXREALNAME]; 
	int verbose;
	int use_exc;
	long Questions;
	int defaultpoints;
	int resettype;
	time_t lastreset;
} TriviaServ;

typedef struct QuestionFiles {
	FILE *fn;
	char filename[MAXPATH];
	list_t *QE;
	int version;
	char description[QUESTSIZE];
	/* actually just the striped filename */
	char name[QUESTSIZE];
} QuestionFiles;

typedef struct Questions {
	long qn;
	long offset;
	char *question;
	char *answer;
	char *lasthint;
	pcre *regexp;
	int hints;
	int points;
} Questions;

typedef struct TriviaChan {
	char name[MAXCHANLEN];
	Channel *c;
	int publiccontrol;
	int active;
	int scorepoints;
	long questtime;
	int opchan;
	int resettype;
	time_t lastreset;
	int foreground;
	int background;
	int boldcase;
	int hintcolour;
	int messagecolour;
	int hintchar;
	time_t lastquest;
	list_t *qfl;
	Questions *curquest;
} TriviaChan;

typedef struct TriviaChannelQuestionFiles {
	char savename[MAXCHANLEN+QUESTSIZE+1];
	char cname[MAXCHANLEN];
	char qname[QUESTSIZE];
} TriviaChannelQuestionFiles;

typedef struct TriviaChannelScore {
	char savename[MAXCHANLEN+MAXNICK+1];
	char cname[MAXCHANLEN];
	char uname[MAXNICK];
	int score;
	time_t lastused;
} TriviaChannelScore;

typedef struct TriviaUser {
	char lastusednick[MAXNICK];
	int lastusedreg;
	int networkscore;
	time_t lastused;
	list_t *tcsl;
} TriviaUser;

list_t *qfl;
list_t *userlist;
hash_t *tch;

extern const char *questpath;

/* TriviaServ_help.c */
extern const char *tvs_about[];
extern const char *tvs_help_set_verbose[];
extern const char *tvs_help_set_exclusions[];
extern const char *tvs_help_set_defaultpoints[];
extern const char *tvs_help_set_resettype[];
extern const char *tvs_help_chans[];
extern const char *tvs_help_catlist[];
extern const char *tvs_help_start[];
extern const char *tvs_help_stop[];
extern const char *tvs_help_qs[];
extern const char *tvs_help_sp[];
extern const char *tvs_help_pc[];
extern const char *tvs_help_opchan[];
extern const char *tvs_help_resetscores[];
extern const char *tvs_help_colour[];
extern const char *tvs_help_hintchar[];

/* TriviaUser.c */
void tvs_addpoints(Client *u, TriviaChan *tc);
int GetUsersChannelScore (Client *u, TriviaChan *tc);
int UmodeUser(const CmdParams *cmdparams);
int QuitNickUser(const CmdParams *cmdparams);
int KillUser(const CmdParams *cmdparams);
void UserLeaving(Client *u);
void SaveAllUserScores(void);

/* TriviaCmds.c */
int tvs_cmd_score (const CmdParams *cmdparams);
int tvs_cmd_hint (const CmdParams *cmdparams);
int tvs_cmd_start (const CmdParams *cmdparams);
int tvs_cmd_stop (const CmdParams *cmdparams);
int tvs_cmd_qs (const CmdParams *cmdparams);
int tvs_cmd_sp (const CmdParams *cmdparams);
int tvs_cmd_pc (const CmdParams *cmdparams);
int tvs_cmd_opchan (const CmdParams *cmdparams);
int tvs_cmd_resetscores (const CmdParams *cmdparams);
int tvs_cmd_colour (const CmdParams *cmdparams);
int tvs_cmd_hintchar (const CmdParams *cmdparams);
int tvs_catlist(const CmdParams *cmdparams);
int tvs_chans(const CmdParams *cmdparams);

/* TriviaChan.c */
int LoadChannel( void *data, int size );
int LoadCQSets( void *data, int size );
TriviaChan *NewTChan(Channel *c);
TriviaChan *OfflineTChan(Channel *c);
TriviaChan *OnlineTChan(Channel *c);
int DelTChan(char *chan);
int SaveTChan (TriviaChan *tc);
int EmptyChan (const CmdParams *cmdparams);
int NewChan(const CmdParams *cmdparams);

/* TriviaQues.c */
void tvs_parse_questions();
int find_cat_name(const void *catnode, const void *name);
void tvs_quesset(const CmdParams *cmdparams, TriviaChan *tc, char *qsn);
QuestionFiles *tvs_randomquestfile(TriviaChan *tc);
void tvs_newquest(TriviaChan *tc);
void tvs_ansquest(TriviaChan *tc);
int tvs_doregex(Questions *qe, char *buf);
void tvs_testanswer(Client* u, TriviaChan *tc, char *line);
void do_hint(TriviaChan *tc);
void obscure_question(TriviaChan *tc);

#endif /* TRIVIASERV_H */
