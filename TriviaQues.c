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

/** TriviaQues.c 
 *  Trivia Question Procedures
 */

#include "neostats.h"	/* Neostats API */
#include "TriviaServ.h"

/*
 * Loads question file offsets into memory
*/
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

/*
 * returns existance of a category name
*/
int find_cat_name(const void *catnode, const void *name) 
{
	QuestionFiles *qf = (void *)catnode;
	return (ircstrcasecmp(qf->name, name));
}

/*
 * Adds/Removes/Lists or resets to default
 * Category Entries/Question Sets for a channel
*/
void tvs_quesset(CmdParams* cmdparams, TriviaChan *tc, char *cn) 
{
	lnode_t *lnode;
	QuestionFiles *qf;
	TriviaChannelQuestionFiles *tcqf;

	if (!cmdparams) {
		/* Adds categories to channel on module loading */
		lnode = list_find(qfl, cn, find_cat_name);
		if (lnode) {
			if (list_find(tc->qfl, cn, find_cat_name)) {
				return;
			} else {
				qf = lnode_get(lnode);
				lnode_create_append(tc->qfl, qf);
				return;
			}
		} else {
			return;
		}
	} else if (cmdparams->ac == 2 && !ircstrcasecmp(cmdparams->av[0], "ADD")) {
		lnode = list_find(qfl, cmdparams->av[1], find_cat_name);
		if (lnode) {
			if (list_find(tc->qfl, cmdparams->av[1], find_cat_name)) {
				irc_prefmsg (tvs_bot, cmdparams->source, "Category %s is already set for this channel", cmdparams->av[1]);
				return;
			} else {
				qf = lnode_get(lnode);
				lnode_create_append(tc->qfl, qf);
				/* add to Channel Question Sets DB */
				tcqf = ns_calloc(sizeof(TriviaChannelQuestionFiles));
				strlcpy(tcqf->cname, tc->c->name, MAXCHANLEN);
				strlcpy(tcqf->qname, cmdparams->av[1], QUESTSIZE);
				ircsnprintf(tcqf->savename, MAXCHANLEN+QUESTSIZE+1, "%s%s", tcqf->qname, tcqf->cname);
				DBAStore( "CQSets", tcqf->savename, tcqf, sizeof(TriviaChannelQuestionFiles));
				ns_free(tcqf);
				irc_prefmsg (tvs_bot, cmdparams->source, "Added Category %s to this channel", cmdparams->av[1]);
				irc_chanprivmsg (tvs_bot, tc->name, "%s added Category %s to the list of questions", cmdparams->source->name, cmdparams->av[1]);
				return;
			}
		} else {
			irc_prefmsg (tvs_bot, cmdparams->source, "Can't Find Category %s. Try /msg %s catlist", cmdparams->av[1], tvs_bot->name);
			return;
		}
	} else if (cmdparams->ac == 2 && !ircstrcasecmp(cmdparams->av[0], "DEL")) {
		lnode = list_find(tc->qfl, cmdparams->av[1], find_cat_name);
		if (lnode) {
			list_delete(tc->qfl, lnode);
			lnode_destroy(lnode);
			/* remove from Channel Question Sets DB */
			tcqf = ns_calloc(sizeof(TriviaChannelQuestionFiles));
			ircsnprintf(tcqf->savename, MAXCHANLEN+QUESTSIZE+1, "%s%s", cmdparams->av[1], tc->c->name);
			DBADelete("CQSets", tcqf->savename);
			ns_free(tcqf);
			/* dun delete the QF entry. */
			irc_prefmsg (tvs_bot, cmdparams->source, "Deleted Category %s for this channel", cmdparams->av[1]);
			irc_chanprivmsg (tvs_bot, tc->name, "%s deleted category %s for this channel", cmdparams->source->name, cmdparams->av[1]);
			return;
		} else {
			irc_prefmsg (tvs_bot, cmdparams->source, "Couldn't find Category %s in the list. Try !set category list", cmdparams->av[1]);
			return;
		}
	} else if (!ircstrcasecmp(cmdparams->av[0], "LIST")) {
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
	} else if (!ircstrcasecmp(cmdparams->av[0], "RESET")) {
		/* first we remove all entries from Channel Question Sets DB */
		tcqf = ns_calloc(sizeof(TriviaChannelQuestionFiles));
		while (lnode != NULL) {
			qf = lnode_get(lnode);
			ircsnprintf(tcqf->savename, MAXCHANLEN+QUESTSIZE+1, "%s%s", qf->name, tc->c->name);
			DBADelete("CQSets", tcqf->savename);
			lnode = list_next(tc->qfl, lnode);
		}
		ns_free(tcqf);
		/* then remove the list nodes */
		while (!list_isempty(tc->qfl)) {
			list_destroy_nodes(tc->qfl);
		}
		irc_prefmsg (tvs_bot, cmdparams->source, "Reset the Question Catagories to Default in %s", tc->name);
		irc_chanprivmsg (tvs_bot, tc->name, "%s reset the Question Categories to Default", cmdparams->source->name);
		return;
	} else {
		irc_prefmsg (tvs_bot, cmdparams->source, "Syntax Error. /msg %s help QS", tvs_bot->name);
	}
	return;		
}

/*
 * Returns a random question file from those
 * available for the channel.
*/
QuestionFiles *tvs_randomquestfile(TriviaChan *tc) 
{
	lnode_t *lnode;
	QuestionFiles *qf;
	int qfn, i;

	if (list_isempty(tc->qfl)) {
		/* if the qfl for this chan is empty, use all qfl's */
		if (list_count(qfl) > 1)
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

/*
 * Selects random question from the question
 * file, then calls obscure which sends the
 * obscured question to the channel
*/
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
	/*
	 * ToDo : 1 - add proportional scoring system, the quicker the answer the more points
	 *        2 - add random scoring system , random between set values
	*/
	qe->points = tc->scorepoints;
	tc->curquest = qe;
	irc_chanprivmsg (tvs_bot, tc->name, "Fingers on the keyboard, Here comes the Next Question!");
	obscure_question(tc);
	tc->lastquest = me.now;
}

/*
 * sends answer to channel when the time is up,
 * if not answered previously
*/
void tvs_ansquest(TriviaChan *tc) {
	Questions *qe;
	qe = tc->curquest;
	irc_chanprivmsg (tvs_bot, tc->name, "Times Up! The Answer was: \2%s\2", qe->answer);
	/* so we don't chew up memory too much */
	ns_free (qe->regexp);
	ns_free (qe->question);
	ns_free (qe->answer);
	ns_free (qe->lasthint);
	qe->question = NULL;
	tc->curquest = NULL;
}

/*
 * creates regex used to test answers,
 * and fills in question structure fields
*/
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
	qe->lasthint = ns_calloc (ANSSIZE);
	strlcpy(qe->question, buf, QUESTSIZE);
	os_memset (tmpbuf, 0, ANSSIZE);
	/* no, its not a infinate loop */
	while (1) {	
		rc = pcre_exec(re, NULL, qe->question, strlen(qe->question), 0, 0, ovector, 9);
		if (rc <= 0) {
			if ((rc == PCRE_ERROR_NOMATCH) && (gotanswer > 0)) {
				/* we got something in q & a, so proceed. */
				ircsnprintf(tmpbuf1, REGSIZE, ".*(?i)(?:%s).*", tmpbuf);
				dlog (DEBUG3, "regexp will be %s\n", tmpbuf1);
				qe->regexp = pcre_compile(tmpbuf1, 0, &error, &errofset, NULL);
				if (qe->regexp == NULL) {
					/* damn damn damn, our constructed regular expression failed */
					nlog(LOG_WARNING, "pcre_compile_answer failed: %s at %d", error, errofset);
					ns_free (re);
					return NS_FAILURE;
				}
				ns_free (re);
				qe->points = 1;
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
				strlcpy(qe->lasthint, subs[2], ANSSIZE);
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
	qe->points = 1;
	return NS_SUCCESS;		
	
}

/*
 * Checks for correct answer
*/
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

/* 
 * Add letters to and display hint
*/
void do_hint(TriviaChan *tc) 
{
	Questions *qe;
	int i, i2, ws, we, twl, swl, swlt, pfw, num;

	if (tc->curquest == NULL) {
		nlog(LOG_WARNING, "curquest is missing for hint");
		return;
	}
	qe = tc->curquest;
	if (qe->hints == 0) {
		for (i=0;i < (int)strlen(qe->lasthint);i++) {
			/* only replace alphanumeric characters */
			num = (int)qe->lasthint[i];
			if ((num > 96 && num < 123) || (num > 64 && num < 91) || (num > 47 && num < 58)) {
				qe->lasthint[i] = '-';
			}
		}
	}
	/*
	 * might not be the best way, but works
	 * adds one letter per word per hint
	*/
	twl = ws = we = pfw = 0;	
	for (i=0;i < ((int)strlen(qe->lasthint) + 1);i++) {
		/* Get word start and end, to ensure letters added in each word */
		if (qe->lasthint[i] != ' ' && qe->lasthint[i] != '/' && i != (int)strlen(qe->lasthint)) {
			/* only count alphanumeric characters */
			num = (int)qe->answer[i];
			if ((num > 96 && num < 123) || (num > 64 && num < 91) || (num > 47 && num < 58)) {
				twl++;
			}
			if (!ws && pfw) {
				ws = i;
			}
			we = i;
		} else if (twl > qe->hints && twl > 0) { /* checks that there are more letters than hints */
			/* pick a random number for the amount of
			 * letters left in the word, then replace
			 * the correct one
			*/
			swl= (rand() % (twl - qe->hints));
			swlt= 0;
			for (i2=ws ; i2 <= we ; i2++) {
				num = (int)qe->answer[i2];
				if (qe->lasthint[i2] == '-' && ((num > 96 && num < 123) || (num > 64 && num < 91) || (num > 47 && num < 58))) {
					if (swl == swlt) {
						qe->lasthint[i2] = qe->answer[i2];
						break;
					}
					swlt++;
				}
			}
			/* reset word start and total words in letter */
			ws = twl = 0;
			pfw= 1;
		} else {
			ws = twl = 0;
			pfw= 1;
		}
	}
	qe->hints++;
	irc_chanprivmsg (tvs_bot, tc->name, "Hint %d: %s", qe->hints, qe->lasthint);
}

/*
 * Obscures question from basic trivia answer bots
*/
void obscure_question(TriviaChan *tc) 
{
	char *out;
	Questions *qe;
	int random, i;

	if (tc->curquest == NULL) {
		nlog(LOG_WARNING, "curquest is missing for obscure_question");
		return;
	}
	qe = tc->curquest;     
	out = ns_calloc (BUFSIZE+1);
	strlcpy(out, "\00304,01", BUFSIZE);
	for (i=0;i < (int)strlen(qe->question);i++) {
		if (qe->question[i] == ' ') {
			/* get random letter */
			random =  ((rand() % 52) + 65);
			if (random > 90) {
				random += 5;
			}
			/* insert same background/foreground color here */
			strlcat(out, "\00301,01", BUFSIZE);
			/* insert random char here */
			out[strlen(out)] = random;
			/* reset color back to standard for next word. */
			strlcat(out, "\00304,01", BUFSIZE);
		} else {
			/* just insert the char, its a word */
			out[strlen(out)] = qe->question[i];
		}
	}
	irc_chanprivmsg (tvs_bot, tc->name, "%s", out);
	ns_free (out);
}
