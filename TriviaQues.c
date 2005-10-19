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
	while (qfnode) 
	{
		qf = lnode_get(qfnode);
		ircsnprintf(pathbuf, MAXPATH, "%s/%s", questpath, qf->filename);
		dlog (DEBUG2, "Opening %s for reading offsets", pathbuf);
		qf->fn = os_fopen (pathbuf, "r");
		/*  if we can't open it, bail out */
		if (!qf->fn) 
		{
			nlog(LOG_WARNING, "Couldn't Open Question File %s for Reading offsets: %s", qf->filename, os_strerror());
			qfnode = list_next(qfl, qfnode);
			continue;
		}
		/* the first line should be the version number and description */
		if (os_fgets (questbuf, QUESTSIZE, qf->fn)) 
		{
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
		while (os_fgets (questbuf, QUESTSIZE, qf->fn)) 
		{
			i++;
			qe = ns_calloc (sizeof(Questions));
			qe->qn = i;
			qe->offset = os_ftell (qf->fn);
			qe->question = NULL;
			qe->answer = NULL;
			qe->lasthint = NULL;
			qe->regexp = NULL;
			qe->hints = 0;
			qe->points = 1;
			lnode_create_append(qf->QE, qe);
		}
		/* leave the filehandle open for later */
		nlog(LOG_NOTICE, "Finished Reading %s for Offsets (%ld)", qf->filename, i);
		qfnode = list_next(qfl, qfnode);
		TriviaServ.Questions = TriviaServ.Questions + i;
		i = 0;
	}		
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
void tvs_quesset(const CmdParams *cmdparams, TriviaChan *tc, char *qsn) 
{
	lnode_t *lnode;
	QuestionFiles *qf;
	TriviaChannelQuestionFiles *tcqf;

	if (!cmdparams) 
	{
		/* Adds categories to channel on module loading */
		lnode = list_find(qfl, qsn, find_cat_name);
		if (lnode) 
		{
			if (list_find(tc->qfl, qsn, find_cat_name)) 
			{
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
		if (lnode) 
		{
			if (list_find(tc->qfl, cmdparams->av[1], find_cat_name)) 
			{
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
		if (lnode) 
		{
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
		if (!list_isempty(tc->qfl)) 
		{
			irc_prefmsg (tvs_bot, cmdparams->source, "Categories for this Channel are:");
			lnode = list_first(tc->qfl);
			while (lnode != NULL) 
			{
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
		tcqf = ns_calloc(sizeof(TriviaChannelQuestionFiles));
		while (!list_isempty(tc->qfl)) 
		{
			lnode = list_first(tc->qfl);
			qf = lnode_get(lnode);
			ircsnprintf(tcqf->savename, MAXCHANLEN+QUESTSIZE+1, "%s%s", qf->name, tc->c->name);
			DBADelete("CQSets", tcqf->savename);
			list_destroy_nodes(tc->qfl);
		}
		ns_free(tcqf);
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

	if (list_isempty(tc->qfl)) 
	{
		/* if the qfl for this chan is empty, use all qfl's */
		if (list_count(qfl) > 1)
			qfn=(unsigned)(rand()%((int)(list_count(qfl))));
		else
			qfn= 0;
		/* ok, this is bad.. but sigh, not much we can do atm. */
		lnode = list_first(qfl);
		i = 0;
		while (i < qfn) 
		{
			lnode = list_next(qfl, lnode);
			i++;
		}
		qf = lnode_get(lnode);				
		if (qf) 
		{
			return qf;
		} else {
			nlog(LOG_WARNING, "Question File Selection (Random) for %s failed. Using first entry", tc->name);
			lnode = list_first(qfl);
			qf = lnode_get(lnode);
			if (qf)
				return qf;
			nlog(LOG_WARNING, "Question File Selection (Random) for %s failed. Unable to select First Question File", tc->name);
			return NULL;			
		}
	} else {
		/* select a random question file */
		qfn=(unsigned)(rand()%((int)(list_count(tc->qfl))));
		/* ok, this is bad.. but sigh, not much we can do atm. */
		lnode = list_first(tc->qfl);
		i = 0;
		while (i < qfn) 
		{
			lnode = list_next(tc->qfl, lnode);
			i++;
		}
		qf = lnode_get(lnode);				
		if (qf != NULL) 
		{
			return qf;
		} else {
			nlog(LOG_WARNING, "Question File Selection (Specific) for %s failed. Using first entry", tc->name);
			lnode = list_first(tc->qfl);
			qf = lnode_get(lnode);
			if (qf != NULL)
				return qf;
			nlog(LOG_WARNING, "Question File Selection (Specific) for %s failed. Unable to select First Question File", tc->name);
			return NULL;			
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
	int qn, qnt, gqt;
	lnode_t *qnode;
	Questions *qe, *cqe;
	QuestionFiles *qf;
	char tmpbuf[512];	

	qf = tvs_randomquestfile(tc);
	if (!qf) 
	{
		irc_chanalert (tvs_bot, "Question File Error for %s , no question file returned", tc->name);
		return;
	}
	gqt = 0;
restartquestionselection:
	qn=(unsigned)(rand()%((int)(list_count(qf->QE))));
	/* ok, this is bad.. but sigh, not much we can do atm. */
	qnode = list_first(qf->QE);
	qnt = 0;
	qe = NULL;
	while (qnt < qn && qnode) 
	{
		qnode = list_next(qf->QE, qnode);
		qnt++;
	}
	if (qnode)
		qe = lnode_get(qnode);				
	/* ok, we hopefully have the Q */
	if (!qe) {
		nlog(LOG_WARNING, "Eh? Never got a Question");
		if (gqt < 5) 
		{
			gqt++;
			irc_chanalert (tvs_bot, "Question Get Error for %s , Error Retrieving Question Pointer for %s (attempt %d)", tc->name, qf->name, gqt);
			goto restartquestionselection;
		}
		return;
	}
	cqe = ns_calloc (sizeof(Questions));
	/* keep current questions seperate per channel */
	os_memcpy (cqe, qe, sizeof(Questions));
	/* ok, now seek to the question in the file */
	if (os_fseek (qf->fn, cqe->offset, SEEK_SET)) 
	{
		nlog(LOG_WARNING, "Eh? Fseek returned a error(%s): %s", qf->filename, os_strerror());
		irc_chanalert (tvs_bot, "Question File Error in %s: %s", qf->filename, os_strerror());
		return;
	}
	if (!os_fgets (tmpbuf, 512, qf->fn)) 
	{
		nlog(LOG_WARNING, "Eh, fgets returned null(%s): %s", qf->filename, os_strerror());
		irc_chanalert (tvs_bot, "Question file Error in %s: %s", qf->filename, os_strerror());
		return;
	}
	if (tvs_doregex(cqe, tmpbuf) == NS_FAILURE) 
	{
		irc_chanalert (tvs_bot, "Question Parsing Failed. Please Check Log File");
		return;
	}	
	/*
	 * ToDo : 1 - add proportional scoring system, the quicker the answer the more points
	 *        2 - add random scoring system , random between set values
	*/
	cqe->points = tc->scorepoints;
	tc->curquest = cqe;
	irc_chanprivmsg (tvs_bot, tc->name, "\003%d%sFingers on the keyboard, Here comes the Next Question!", tc->messagecolour, (tc->boldcase %2) ? "\002" : "");
	obscure_question(tc);
	tc->lastquest = me.now;
}

/*
 * sends answer to channel when the time is up,
 * if not answered previously
*/
void tvs_ansquest(TriviaChan *tc) 
{
	Questions *qe;
	unsigned int i;
	
	qe = tc->curquest;
	/* modify answer character case if required before displaying */
	if (!qe || !qe->answer) {
		tc->curquest = NULL;
		return;
	}
	for (i = 0 ; i < strlen(qe->answer) ; i++) 
	{
		if ((tc->boldcase > 1) && ((qe->answer[i] >= 'a' && qe->answer[i] <= 'z') || (qe->answer[i] >= 'A' && qe->answer[i] <= 'Z'))) 
		{
			if (tc->boldcase > 3 && qe->answer[i] > 'Z') 
			{
				qe->answer[i] -= ('a' - 'A');
			} else if (tc->boldcase > 1 && tc->boldcase < 4 && qe->answer[i] < 'A') {
				qe->answer[i] += ('a' - 'A');
			}
		}
	}
	irc_chanprivmsg (tvs_bot, tc->name, "\003%d%sTimes Up! The Answer was:\003%d%s %s", tc->messagecolour, (tc->boldcase % 2) ? "\002" : "", tc->hintcolour, (tc->boldcase %2) ? "" : "\002", qe->answer);
	/* so we don't chew up memory too much */
	ns_free (qe->question);
	ns_free (qe->answer);
	ns_free (qe->lasthint);
	ns_free (qe->regexp);
	qe->question = NULL;
	qe->answer = NULL;
	qe->lasthint = NULL;
	qe->regexp = NULL;
	ns_free (tc->curquest);
	tc->curquest = NULL;
	return;
}

/*
 * creates regex used to test answers,
 * and fills in question structure fields
*/
int tvs_doregex(Questions *qe, char *buf) 
{
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
	while (1) 
	{	
		rc = pcre_exec(re, NULL, qe->question, strlen(qe->question), 0, 0, ovector, 9);
		if (rc <= 0) 
		{
			if ((rc == PCRE_ERROR_NOMATCH) && (gotanswer > 0)) 
			{
				/* we got something in q & a, so proceed. */
				ircsnprintf(tmpbuf1, REGSIZE, ".*(?i)(?:%s).*", tmpbuf);
				dlog (DEBUG3, "regexp will be %s\n", tmpbuf1);
				qe->regexp = pcre_compile(tmpbuf1, 0, &error, &errofset, NULL);
				if (qe->regexp == NULL) 
				{
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
			if (qe->answer[0] == 0) 
			{
				strlcpy(qe->answer, subs[2], ANSSIZE);
				strlcpy(qe->lasthint, subs[2], ANSSIZE);
			}
			/* tmpbuf will hold our eventual regular expression to find the answer in the channel */
			if (tmpbuf[0] == 0) 
			{
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
	unsigned int i;
	
	qe = tc->curquest;
	if (!qe)
		return;
	dlog (DEBUG3, "Testing Answer on regexp: %s", line);
	rc = pcre_exec(qe->regexp, NULL, line, strlen(line), 0, 0, NULL, 0);
	if (rc >= 0) 
	{
		/* we got a match! */
		/* modify answer character case if required before displaying */
		for (i = 0 ; i < strlen(qe->answer) ; i++) 
		{
			if ((tc->boldcase > 1) && ((qe->answer[i] >= 'a' && qe->answer[i] <= 'z') || (qe->answer[i] >= 'A' && qe->answer[i] <= 'Z'))) 
			{
				if (tc->boldcase > 3 && qe->answer[i] > 'Z') 
				{
					qe->answer[i] -= ('a' - 'A');
				} else if (tc->boldcase > 1 && tc->boldcase < 4 && qe->answer[i] < 'A') {
					qe->answer[i] += ('a' - 'A');
				}
			}
		}
		irc_chanprivmsg (tvs_bot, tc->name, "\003%d%sCorrect!\003%d %s\003%d got the answer:\003%d %s", tc->messagecolour, (tc->boldcase %2) ? "\002" : "", tc->hintcolour, u->name, tc->messagecolour, tc->hintcolour, qe->answer);
		tvs_addpoints(u, tc);
		ns_free (qe->question);
		ns_free (qe->answer);
		ns_free (qe->lasthint);
		ns_free (qe->regexp);
		qe->question = NULL;
		qe->answer = NULL;
		qe->lasthint = NULL;
		qe->regexp = NULL;
		ns_free (tc->curquest);
		tc->curquest = NULL;
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
void do_hint(const TriviaChan *tc) 
{
	Questions *qe;
	int i, i2, wordstart, wordend, totalwordletters, letterpos, letterpostest, pfw, num;

	if (!tc->curquest) 
	{
		nlog(LOG_WARNING, "curquest is missing for hint");
		return;
	}
	qe = tc->curquest;
	if (qe->hints == 0) 
	{
		for (i=0;i < (int)strlen(qe->lasthint);i++) 
		{
			/* only replace alphanumeric characters */
			num = (int)qe->lasthint[i];
			if ((num > 96 && num < 123) || (num > 64 && num < 91) || (num > 47 && num < 58))
				qe->lasthint[i] = tc->hintchar;
		}
	}
	/*
	 * might not be the best way, but works
	 * adds one letter per word per hint
	*/
	totalwordletters = wordstart = wordend = pfw = 0;	
	for (i=0;i < ((int)strlen(qe->lasthint) + 1);i++) 
	{
		/* Get word start and end, to ensure letters added in each word */
		if (qe->lasthint[i] != ' ' && qe->lasthint[i] != '/' && i != (int)strlen(qe->lasthint)) 
		{
			/* only count alphanumeric characters */
			num = (int)qe->answer[i];
			if ((num > 96 && num < 123) || (num > 64 && num < 91) || (num > 47 && num < 58))
				totalwordletters++;
			if (!wordstart && pfw)
				wordstart = i;
			wordend = i;
		} else if (totalwordletters > qe->hints && totalwordletters > 0) { /* checks that there are more letters than hints */
			/* pick a random number for the amount of
			 * letters left in the word, then replace
			 * the correct one
			*/
			letterpos= (rand() % (totalwordletters - qe->hints));
			letterpostest= 0;
			for (i2=wordstart ; i2 <= wordend ; i2++) 
			{
				num = (int)qe->answer[i2];
				if (qe->lasthint[i2] == tc->hintchar && ((num > 96 && num < 123) || (num > 64 && num < 91) || (num > 47 && num < 58))) 
				{
					if (letterpos == letterpostest) 
					{
						if (tc->boldcase > 1 && ((qe->answer[i2] >= 'a' && qe->answer[i2] <= 'z') || (qe->answer[i2] >= 'A' && qe->answer[i2] <= 'Z'))) 
						{
							if (tc->boldcase > 3 && qe->answer[i2] > 'Z') 
							{
								qe->lasthint[i2] = (qe->answer[i2] - ('a' - 'A'));
							} else if (tc->boldcase > 1 && tc->boldcase < 4 && qe->answer[i2] < 'A') {
								qe->lasthint[i2] = (qe->answer[i2] + ('a' - 'A'));
							} else {
								qe->lasthint[i2] = qe->answer[i2];
							}
						} else {
							qe->lasthint[i2] = qe->answer[i2];
						}
						break;
					}
					letterpostest++;
				}
			}
			/* reset word start and total words in letter */
			wordstart = totalwordletters = 0;
			pfw= 1;
		} else {
			wordstart = totalwordletters = 0;
			pfw= 1;
		}
	}
	qe->hints++;
	irc_chanprivmsg (tvs_bot, tc->name, "\003%d%sHint %d: %s", tc->hintcolour, (tc->boldcase %2) ? "\002" : "", qe->hints, qe->lasthint);
}

/*
 * Obscures question from basic trivia answer bots
*/
void obscure_question(const TriviaChan *tc) 
{
	char *out, *tmpcolour, *tmpunseen, *tmpstr;
	Questions *qe;
	int random, i, qlen, charcount=0, tucount, tccount;

	if (tc->curquest == NULL) 
	{
		nlog(LOG_WARNING, "curquest is missing for obscure_question");
		return;
	}
	qe = tc->curquest;     
	/* set question colour and hidden letters colour */
	tmpstr = ns_calloc (BUFSIZE);
	tmpcolour = ns_calloc (BUFSIZE);
	strlcpy(tmpcolour, "\003", BUFSIZE);
	if (tc->foreground < 10)
		strlcat(tmpcolour, "0", BUFSIZE);
	ircsnprintf(tmpstr, BUFSIZE, "%d,", tc->foreground);
	strlcat(tmpcolour, tmpstr, BUFSIZE);
	if (tc->background < 10)
		strlcat(tmpcolour, "0", BUFSIZE);
	ircsnprintf(tmpstr, BUFSIZE, "%d", tc->background);
	strlcat(tmpcolour, tmpstr, BUFSIZE);
	tmpunseen = ns_calloc (BUFSIZE);
	strlcpy(tmpunseen, "\003", BUFSIZE);
	if (tc->background < 10)
		strlcat(tmpunseen, "0", BUFSIZE);
	ircsnprintf(tmpstr, BUFSIZE, "%d,", tc->background);
	strlcat(tmpunseen, tmpstr, BUFSIZE);
	if (tc->background < 10)
		strlcat(tmpunseen, "0", BUFSIZE);
	ircsnprintf(tmpstr, BUFSIZE, "%d", tc->background);
	strlcat(tmpunseen, tmpstr, BUFSIZE);
	/* obscure question using defined channel colours, and send to channel */
	out = ns_calloc (BUFSIZE+1);
	tucount = (int)strlen(tmpunseen);
	tccount = (int)strlen(tmpcolour);
	strlcpy(out, tmpcolour, BUFSIZE);
	charcount += tccount;
	qlen = (int)strlen(qe->question);
	for (i=0;( i < qlen ) && ( charcount < (BUFSIZE-13) );i++) 
	{
		if (qe->question[i] == ' ') 
		{
			/* get random letter */
			random =  ((rand() % 52) + 65);
			if (random > 90)
				random += 5;
			/* insert same background/foreground color,
			 * then random character, and question colour
			 */
			strlcat(out, tmpunseen, BUFSIZE);
			charcount += tucount;
			out[charcount++] = random;
			strlcat(out, tmpcolour, BUFSIZE);
			charcount += tccount;
		} else {
			/* insert the char, changing case if required, its a word */
			if (tc->boldcase > 1 && ((qe->question[i] >= 'a' && qe->question[i] <= 'z') || (qe->question[i] >= 'A' && qe->question[i] <= 'Z'))) 
			{
				if (tc->boldcase > 3 && qe->question[i] > 'Z') 
				{
					out[charcount++] = (qe->question[i] - ('a' - 'A'));
				} else if (tc->boldcase > 1 && tc->boldcase < 4 && qe->question[i] < 'a') {
					out[charcount++] = (qe->question[i] + ('a' - 'A'));
				} else {
					out[charcount++] = qe->question[i];
				}
			} else {
				out[charcount++] = qe->question[i];
			}
		}
	}
	irc_chanprivmsg (tvs_bot, tc->name, "%s%s", (tc->boldcase % 2) ? "\002" : "", out);
	ns_free (tmpstr);
	ns_free (tmpcolour);
	ns_free (tmpunseen);
	ns_free (out);
}
