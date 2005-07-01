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

/** TriviaCmds.c 
 *  Trivia Command Processing
 */

#include "neostats.h"	/* Neostats API */
#include "TriviaServ.h"

/*
 * Display Scores
 *
 * ToDo : Everything
*/
int tvs_cmd_score (CmdParams* cmdparams)
{
	TriviaChan *tc;

	SET_SEGV_LOCATION();
	/* check command was from a channel. */
	if (!cmdparams->channel) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "SCORE Command is used in Channel Only");
		return NS_FAILURE;
	}
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	irc_prefmsg (tvs_bot, cmdparams->source, "Score for %s in %s", cmdparams->source->name, tc->name);
	return NS_SUCCESS;
}

/*
 * Display Hint
 *
 * ToDo : Everything
*/
int tvs_cmd_hint (CmdParams* cmdparams)
{
	TriviaChan *tc;

	SET_SEGV_LOCATION();
	/* check command was from a channel. */
	if (!cmdparams->channel) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "HINT Command is used in Channel Only");
		return NS_FAILURE;
	}
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc) 
		return NS_FAILURE;
	irc_prefmsg (tvs_bot, cmdparams->source, "Hint for %s in %s", cmdparams->source->name, tc->name);
	return NS_SUCCESS;
}

/*
 * Starts Trivia in channel
*/
int tvs_cmd_start (CmdParams* cmdparams)
{
	TriviaChan *tc;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((tc->publiccontrol == 0) && (!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN)) 
		return NS_FAILURE;
	if (tc->active) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "Trivia is already active in %s", tc->name);
		return NS_SUCCESS;
	}
	tc->active = 1;
	tc->curquest = NULL;
	irc_prefmsg (tvs_bot, cmdparams->source, "Starting Trivia in %s shortly", tc->name);
	irc_chanprivmsg (tvs_bot, tc->name, "\003%d%s%s \003%dhas activated Trivia. Get Ready for the first question!", tc->hintcolour, (tc->boldcase %2) ? "\002" : "", cmdparams->source->name, tc->messagecolour);
	return NS_SUCCESS;
}

/*
 * Stops Trivia in channel
*/
int tvs_cmd_stop (CmdParams* cmdparams)
{
	TriviaChan *tc;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((tc->publiccontrol == 0) && (!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN)) 
		return NS_FAILURE;
	if (!tc->active) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "Trivia is not currently running in %s", tc->name);
		return NS_SUCCESS;
	}
	tc->active = 0;
	if (tc->curquest)
		tc->curquest = NULL;
	irc_prefmsg (tvs_bot, cmdparams->source, "Trivia Stoped in %s", tc->name);
	irc_chanprivmsg (tvs_bot, tc->name, "\003%d%s%s \003%dhas stopped Trivia.", tc->hintcolour, (tc->boldcase %2) ? "\002" : "", cmdparams->source->name, tc->messagecolour);
	return NS_SUCCESS;
}

/*
 * Checks access allowed to set Question Sets for channel
 * then calls tvs_questset to perform the action
*/
int tvs_cmd_qs (CmdParams* cmdparams)
{
	TriviaChan *tc;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN))
		return NS_FAILURE;
	tvs_quesset(cmdparams, tc, NULL);
	return NS_SUCCESS;
}

/*
 * Sets amount of points per question in channel
*/
int tvs_cmd_sp (CmdParams* cmdparams)
{
	int ps=0;
	TriviaChan *tc;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN))
		return NS_FAILURE;
	ps = atoi(cmdparams->av[0]);
	if (ps < 1 || ps > 25) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "Points must be between 1 and 25 (%d)", ps);
		return NS_FAILURE;
	}
	tc->scorepoints = ps;
	irc_prefmsg (tvs_bot, cmdparams->source, "Channel Correct Answer Points now set to %d", tc->scorepoints);
	SaveTChan(tc);
	return NS_SUCCESS;
}

/*
 * Allows Chanops to turn Public Control on or off
*/
int tvs_cmd_pc (CmdParams* cmdparams)
{
	TriviaChan *tc;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN))
		return NS_FAILURE;
	if (!ircstrcasecmp(cmdparams->av[0], "ON")) 
	{
		tc->publiccontrol = 1;
		SaveTChan(tc);
	} else if (!ircstrcasecmp(cmdparams->av[0], "OFF")) {
		tc->publiccontrol = 0;
		SaveTChan(tc);
	} else {
		irc_prefmsg (tvs_bot, cmdparams->source, "Public Control can be set to ON or OFF only");
		return NS_FAILURE;
	}
	irc_prefmsg (tvs_bot, cmdparams->source, "Public Control for %s set to %s", tc->name , tc->publiccontrol ? "On" : "Off");
	return NS_SUCCESS;
}

/*
 * Allows Chanops to set if Pseudo Client set Op on joining Channel or not
*/
int tvs_cmd_opchan (CmdParams* cmdparams)
{
	TriviaChan *tc;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN))
		return NS_FAILURE;
	if (!ircstrcasecmp(cmdparams->av[0], "ON")) 
	{
		tc->opchan = 1;
	} else if (!ircstrcasecmp(cmdparams->av[0], "OFF")) {
		tc->opchan = 0;
	} else {
		irc_prefmsg (tvs_bot, cmdparams->source, "OPCHAN can be set to ON or OFF only");
		return NS_FAILURE;
	}
	SaveTChan(tc);
	/* check if setting changes and op/deop bot */
	if (tc->opchan && !IsChanOp(cmdparams->channel->name, tvs_bot->u->name)) 
	{
		irc_chanusermode( tvs_bot, cmdparams->channel->name, "+o", tvs_bot->u->name);
	} else if (!tc->opchan && IsChanOp(cmdparams->channel->name, tvs_bot->u->name)) {
		irc_chanusermode( tvs_bot, cmdparams->channel->name, "-o", tvs_bot->u->name);
	}
	irc_prefmsg (tvs_bot, cmdparams->source, "OPCHAN for %s set to %s", tc->name , tc->opchan ? "On" : "Off");
	return NS_SUCCESS;
}

/*
 * Allows Chanops to configure Automatic Score Resetting
*/
int tvs_cmd_resetscores (CmdParams* cmdparams) 
{
	TriviaChan *tc;
	int resettype;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN))
		return NS_FAILURE;
	resettype = atoi(cmdparams->av[0]);
	if ( resettype < 0 || resettype > 6) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "RESETSCORES Value out of range (0 to 6 allowed only).");
		return NS_FAILURE;
	}
	tc->resettype = resettype;
	SaveTChan(tc);
	switch (tc->resettype) 
	{
		case 0:
			irc_chanprivmsg (tvs_bot, cmdparams->channel->name, "Channel Trivia Scores will no longer reset automatically.");
			break;
		case 1:
			irc_chanprivmsg (tvs_bot, cmdparams->channel->name, "Channel Trivia Scores will now be reset Daily.");
			break;
		case 2:
			irc_chanprivmsg (tvs_bot, cmdparams->channel->name, "Channel Trivia Scores will now be reset Weekly.");
			break;
		case 3:
			irc_chanprivmsg (tvs_bot, cmdparams->channel->name, "Channel Trivia Scores will now be reset Monthly.");
			break;
		case 4:
			irc_chanprivmsg (tvs_bot, cmdparams->channel->name, "Channel Trivia Scores will now be reset Quarterly.");
			break;
		case 5:
			irc_chanprivmsg (tvs_bot, cmdparams->channel->name, "Channel Trivia Scores will now be reset Bi-Anually.");
			break;
		case 6:
			irc_chanprivmsg (tvs_bot, cmdparams->channel->name, "Channel Trivia Scores will now be reset Anually.");
			break;
	}
	return NS_SUCCESS;
}

/*
 * Allows Chanops to set colour/bold/underline for questions/hints/etc
*/
int tvs_cmd_colour (CmdParams* cmdparams) 
{
	TriviaChan *tc;
	int fg, bg, lc, i;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN))
		return NS_FAILURE;
	lc = 0;
	fg = atoi(cmdparams->av[0]);
	bg = atoi(cmdparams->av[1]);
	if ( fg < 0 || fg > 15 ) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "Foreground Colour out of range, Colour codes are from 0 through 15 only.");
		return NS_FAILURE;
	}
	if ( bg < 0 || bg > 15 ) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "Background Colour out of range, Colour codes are from 0 through 15 only.");
		return NS_FAILURE;
	}
	if ( fg == bg ) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "Foreground and Background colours MUST be different.");
		return NS_FAILURE;
	}
	tc->foreground = fg;
	tc->background = bg;
	if (cmdparams->ac > 2) 
	{
		for (i = 2 ; i < cmdparams->ac ; i++) 
		{
			if (!ircstrcasecmp(cmdparams->av[i], "B")) 
			{
				if (!lc)
					tc->boldcase = 0;
				tc->boldcase += 1;
				lc = 1;
			}
			if (!ircstrcasecmp(cmdparams->av[i], "L")) 
			{
				if (!lc)
					tc->boldcase = 0;
				tc->boldcase += 2;
				lc = 1;
			}
			if (!ircstrcasecmp(cmdparams->av[i], "U")) 
			{
				if (!lc)
					tc->boldcase = 0;
				tc->boldcase += 4;
				lc = 1;
			}
			if (!lc) 
			{
				if (i == 2) 
				{
					tc->hintcolour = atoi(cmdparams->av[i]);
				} else if (i == 3) {
					tc->messagecolour = atoi(cmdparams->av[i]);
				}
			}
		}
	}
	SaveTChan(tc);
	irc_chanprivmsg (tvs_bot, tc->name, "Question colour updated");
	return NS_SUCCESS;
}

/*
 * Allows Chanops to set hint character used for answers
*/
int tvs_cmd_hintchar (CmdParams* cmdparams) 
{
	TriviaChan *tc;
	Questions *qe;
	int i;

	SET_SEGV_LOCATION();
	/* find if its our channel. */
	tc = (TriviaChan *)GetChannelModValue (cmdparams->channel);
	if (!tc)
		return NS_FAILURE;
	if ((!IsChanOp(cmdparams->channel->name, cmdparams->source->name)) && (cmdparams->source->user->ulevel < NS_ULEVEL_ADMIN))
		return NS_FAILURE;
	if (strlen(cmdparams->av[0]) > 1) 
	{
		irc_prefmsg (tvs_bot, cmdparams->source, "Hint Character may only be a single character");
		return NS_FAILURE;
	}
	/* if currently asking question change hint characters */
	if (tc->curquest) 
	{
		qe = tc->curquest;
		for (i=0;i < (int)strlen(qe->lasthint);i++) 
		{
			if (qe->lasthint[i] == tc->hintchar && qe->answer[i] != tc->hintchar)
				qe->lasthint[i] = cmdparams->av[0][0];
		}
	}
	/* set and save new hint char */
	tc->hintchar = cmdparams->av[0][0];
	SaveTChan(tc);
	irc_chanprivmsg (tvs_bot, tc->name, "Hint Character changed to %s", cmdparams->av[0]);
	return NS_SUCCESS;
}

/*
 * Lists all Question Sets / Categories available on the network
*/
int tvs_catlist(CmdParams* cmdparams) 
{
	lnode_t *lnode;
	QuestionFiles *qf;
	
	SET_SEGV_LOCATION();
	lnode = list_first(qfl);
	irc_prefmsg (tvs_bot, cmdparams->source, "Question Categories (%d):", (int)list_count(qfl));
	while (lnode) 
	{
		qf = lnode_get(lnode);
		irc_prefmsg (tvs_bot, cmdparams->source, "\2%s\2) %d Questions , %s", qf->name, (int) list_count(qf->QE), qf->description);
		lnode = list_next(qfl, lnode);
	}
	irc_prefmsg (tvs_bot, cmdparams->source, "End Of List.");
	return NS_SUCCESS;
}

/*
 * Add/Remove/List Trivia Chans
*/
int tvs_chans(CmdParams* cmdparams) 
{
	hscan_t hs;
	hnode_t *hnode;
	TriviaChan *tc;
	Channel *c;
	int i;
	
	SET_SEGV_LOCATION();
	if (!ircstrcasecmp(cmdparams->av[0], "ADD")) 
	{
		if (cmdparams->ac < 3)
			return NS_ERR_SYNTAX_ERROR;
		c = FindChannel (cmdparams->av[1]);
		if (!c) 
		{
			irc_prefmsg (tvs_bot, cmdparams->source, "Error: Channel must be online");
			return NS_FAILURE;
		}
		if ((TriviaChan *)GetChannelModValue (c)) 
		{
			irc_prefmsg (tvs_bot, cmdparams->source, "Error: Channel already exists in the database");
			return NS_FAILURE;
		}
		tc = NewTChan(c);
		if (!tc) 
		{
			irc_prefmsg (tvs_bot, cmdparams->source, "Error: Channel must be online");
			return NS_FAILURE;
		}
		if (!ircstrcasecmp(cmdparams->av[2], "On")) 
		{
			tc->publiccontrol = 1;
		} else {
			tc->publiccontrol = 0;
		}
		SaveTChan(tc);
		irc_prefmsg (tvs_bot, cmdparams->source, "Added %s with public control set to %s", tc->name, tc->publiccontrol ? "On" : "Off");
		CommandReport(tvs_bot, "%s added %s with public control set to %s", cmdparams->source->name, tc->name, tc->publiccontrol ? "On" : "Off");
		if (tc->opchan)
			irc_join (tvs_bot, tc->name, "+o");
		else
			irc_join (tvs_bot, tc->name, NULL);
	} else if (!ircstrcasecmp(cmdparams->av[0], "DEL")) {
		if (cmdparams->ac < 2)
			return NS_ERR_SYNTAX_ERROR;
		if (DelTChan(cmdparams->av[1])) 
		{
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
		while ((hnode = hash_scan_next(&hs))) 
		{
			tc = hnode_get(hnode);
			i++;
			irc_prefmsg (tvs_bot, cmdparams->source, "\2%d\2) %s (%s) - Public? %s - OpChan? %s - Reset? %s - Points %d", i, tc->name, (TriviaChan *)GetChannelModValue (tc->c) ? "Open" : "Closed", tc->publiccontrol ? "Yes" : "No", tc->opchan ? "Yes" : "No",
				(tc->resettype == 0) ? "None" : (tc->resettype == 1) ? "Daily" : (tc->resettype == 2) ? "Weekly" : (tc->resettype == 3) ? "Monthly" : (tc->resettype == 4) ? "Quarterly" : (tc->resettype == 5) ? "Bi-Anually" : "Anually", tc->scorepoints);
		}
		irc_prefmsg (tvs_bot, cmdparams->source, "End of list.");
	} else {
		return NS_ERR_SYNTAX_ERROR;
	}
	return NS_SUCCESS;
}
