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

/** TriviaUser.c 
 *  Trivia Tracking for Users for TrivaServ
 */

#include "neostats.h"	/* Neostats API */
#include "modconfig.h"
#include "TriviaServ.h"
#include <strings.h>
#include <error.h>




void tvs_addpoints(char *who, TriviaChan *tc) {
	User *u;
	TriviaScore *ts;
	Questions *qe;
	
	u = finduser(who);
	if (!u | !tc->curquest) {
		nlog(LOG_WARNING, LOG_MOD, "Can't Find user %s for AddPoints?!", who);
		return;
	}
	
	if (u->moddata[TriviaServ.modnum] != NULL) {
		/* create/load a new user */
		ts = u->moddata[TriviaServ.modnum];
		/* XXX Load User? */		
	} else {
		if (!(u->Umode & UMODE_REGNICK)) {
			/* not a registered user */
			prefmsg(u->nick, s_TriviaServ, "If you want your score to be kept between sessions, you should register your nickname");
		}
		ts = malloc(sizeof(TriviaScore));
		bzero(ts, sizeof(TriviaScore));
		strlcpy(ts->nick, u->nick, MAXNICK);
		u->moddata[TriviaServ.modnum] = ts;
	}

	qe = tc->curquest;
	ts->score = ts->score + qe->points;			
	ts->lastused = me.now;
	privmsg(tc->name, s_TriviaServ, "%s now has %d Points", u->nick, ts->score);
}	

int DelUser(char **av, int ac) {
	User *u;
	TriviaScore *ts;
	u = finduser(av[0]);
	if (!u) {
		nlog(LOG_WARNING, LOG_MOD, "Can't find user %s for signoff", av[0]);
		return NS_FAILURE;
	}
	if (u->moddata[TriviaServ.modnum] != NULL) {
		/* XXX Save User */
		ts = u->moddata[TriviaServ.modnum];		
		free(ts);
	}
	return NS_SUCCESS;
}
