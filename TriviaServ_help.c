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

#include "neostats.h"	/* Neostats API */

const char *tvs_about[] = {
	"NeoStats Trivia Game Service",
	NULL
};

const char *tvs_help_set_verbose[] = {
	"\2VERBOSE <ON|OFF>\2",
	"Control message reporting",
	NULL
};
const char *tvs_help_set_exclusions[] = {
	"\2EXCLUSIONS <ON|OFF>\2",
	"use global exclusion list",
	NULL
};

const char *tvs_help_set_defaultpoints[] = {
	"\2DEFAULTPOINTS <#>\2",
	"Sets Default Channel and Network Points for a correct Answer (1 to 25)",
	NULL
};

const char *tvs_help_set_resettype[] = {
	"\2!RESETTYPE <#>\2",
	"Valid options are :",
	"0 -> Never Reset Network Scores",
	"1 -> Reset Network Scores Daily",
	"2 -> Reset Network Scores Weekly",
	"3 -> Reset Network Scores Monthly",
	"4 -> Reset Network Scores Quarterly",
	"5 -> Reset Network Scores Bi-Anually",
	"6 -> Reset Network Scores Anually",
	NULL
};

const char *tvs_help_chans[] = {
	"Add/Delete Trivia Channels",
	"Syntax: \2CHANS LIST\2",
	"        \2CHANS ADD <channel> <PublicControl>\2",
	"        \2CHANS DEL <channel> <PublicControl>\2",
	"",
	"This command lets you assign Triva Channels",
	"",
	"\2LIST\2 will list the current Trivia Channels",
	"",
	"\2ADD\2 will add <channel> to the Trivia Channels,",
	"If Public Control is set to OFF, Only Channel Ops may",
	"use !start and !stop, if set to ON, anyone May !Start and !Stop",
	"",
	"\2DEL\2 will delete <channel> from the Trivia Channels",
	NULL
};

const char *tvs_help_catlist[] = {
	"Display Question Category List",
	"Displays a list of ALL available Question Categories",
	NULL
};
const char *tvs_help_start[] = {
	"Start Trivia",
	"Syntax: (in Channel only) \2!START\2",
	"",
	"Starts Asking Questions in channel",
	NULL
};

const char *tvs_help_stop[] = {
	"Stop Trivia",
	"Syntax: (in Channel only) \2!STOP\2",
	"",
	"Stops Asking Questions in channel",
	NULL
};

const char *tvs_help_qs[] = {
	"Change Question Sets In Channel",
	"Syntax: (in Channel only)",
	"        \2!QS LIST\2",
	"        \2!QS ADD <category>\2",
	"        \2!QS DEL <category>\2",
	"        \2!QS RESET\2",
	"",
	"This command lets you assign Triva Question Sets",
	"",
	"\2LIST\2 will list the current Question Sets",
	"",
	"\2ADD\2 will add <category> to the Question Sets used.",
	"",
	"\2DEL\2 will remove <category> from the Question Sets used.",
	"",
	"\2RESET\2 will Set the Questions asked back to the default.",
	NULL
};

const char *tvs_help_sp[] = {
	"Set points for correct answer",
	"Syntax: (in Channel only)",
	"        \2!SETPOINTS <#>\2",
	"",
	"This command Sets the amount of points to score",
	"for each correctly answered question in the channel.",
	"",
	"NOTE: This does not affect the network wide scores.",
	NULL
};

const char *tvs_help_pc[] = {
	"Set public control",
	"Syntax: (in Channel only)",
	"        \2!PUBLIC <ON/OFF>\2",
	"",
	"This command Sets public access to start and stop",
	"Trivia, If OFF , only Chanops can Start/Stop.",
	NULL
};

const char *tvs_help_opchan[] = {
	"Set CHANOP level",
	"Syntax: (in Channel only)",
	"        \2!OPCHAN <ON/OFF>\2",
	"",
	"This command Sets the Channel Op status of the Pseudo Client",
	"If ON it will gain CHANOP status on joining the Channel.",
	"If OFF it will sit as a normal user in the Channel.",
	NULL
};

const char *tvs_help_resetscores[] = {
	"Set time duration to reset Channel scores",
	"Syntax: (in Channel only)",
	"        \2!RESETSCORES <#>\2",
	"",
	"This command Sets the period that the channel scores",
	"will be automatically reset to 0.",
	"",
	"Valid options are :",
	"0 -> Never Reset Scores",
	"1 -> Reset Scores Daily",
	"2 -> Reset Scores Weekly",
	"3 -> Reset Scores Monthly",
	"4 -> Reset Scores Quarterly",
	"5 -> Reset Scores Bi-Anually",
	"6 -> Reset Scores Anually",
	NULL
};

const char *tvs_help_colour[] = {
	"Set question colour/bold/case",
	"Syntax: (in Channel only)",
	"        \2!COLOUR <foreground> <background> [<hintcolour>] [<messagecolour>] [B] [U|L]\2",
	"",
	"This command Sets the colour of the asked questions, and text",
	"",
	"foreground, background, hintcolour and messagecolour",
	" are standard colour numbers, with no control codes.",
	"E.G. (White on Black)",
	"!COLOUR 0 1",
	"",
	"B (Bold) Bolds all game text",
	"U (Uppercase) converts questions and hints to Uppercase.",
	"L (Lowercase) converts questions and hints to lowercase.",
	"",
	"Note: Uppercase Overrides Lowercase processing.",
	NULL
};

const char *tvs_help_hintchar[] = {
	"Set question hint character",
	"Syntax: (in Channel only)",
	"        \2!HINTCHAR <hintcharacter>\2",
	"",
	"Sets the hint character replacing letters in the answer",
	NULL
};
