/* NeoStats - IRC Statistical Services Copyright 
** Copyright (c) 1999-2004 Justin Hammond
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
** $Id: SecureServ.h 235 2004-02-19 22:17:18Z Mark $
*/

#ifndef TRIVIASERV_H
#define TRIVIASERV_H

#include "neostats.h"
#include "modconfig.h"

#define TVSREXEXP "^(.*)\\*(.*)$"
#define QUESTSIZE 500
#define ANSSIZE 200
#define REGSIZE ANSSIZE + 20

/** 
 *  A string to hold the name of our bot
 */
char s_TriviaServ[MAXNICK];


struct TriviaServ {
	char user[MAXUSER]; 
	char host[MAXHOST]; 
	char realname[MAXREALNAME]; 
	int isonline;
	int use_exc;
	int modnum;
} TriviaServ;

struct QuestFile {
	FILE *fn;
	char filename[MAXPATH];
};

typedef struct QuestFile QuestionFiles;

struct Quests {
	long qn;
	long offset;
	QuestionFiles *QF;
	char question[QUESTSIZE];
	char answer[ANSSIZE];
	pcre *regexp;
	int hints;
	int points;
};

typedef struct Quests Questions;

struct TChans {
	char name[CHANLEN];
	Chans *c;
	int publiccontrol;
	int active;
	long lastquest;
	long questtime;
	Questions *curquest;
};

typedef struct TChans TriviaChan;

struct TScore {
	char nick[MAXNICK];
	int score;
	time_t lastused;
};

typedef struct TScore TriviaScore;

list_t *ql;
list_t *qfl;
hash_t *tch;




/* TriviaServ_help.c */
extern const char *tvs_help_about[];
extern const char tvs_help_about_oneline[];
extern const char *tvs_help_version[];
extern const char tvs_help_version_oneline[];
extern const char *tvs_help_set_exclusions[];
extern const char *tvs_help_chans[];
extern const char tvs_help_chans_oneline[];

/* TriviaUser.c */
void tvs_addpoints(char *, TriviaChan *);
int DelUser(char **av, int ac);
#endif /* TRIVIASERV_H */
