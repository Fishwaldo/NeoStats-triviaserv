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

#include "neostats.h"	/* Neostats API */

const char *tvs_about[] = {
	"Trivia Service",
	NULL
};
const char *tvs_help_set_exclusions[] = {
	"help",
	NULL
};

const char *tvs_help_chans[] = {
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
const char tvs_help_chans_oneline[] = "Add/Delete Trivia Channels";

const char *tvs_help_catlist[] = {
	"Displays a list of ALL available Question Categories",
	NULL
};
const char tvs_help_catlist_oneline[] = "Display Question Category List";

const char *tvs_help_start[] = {
	"Syntax: (in Channel only) \2!START\2",
	"",
	"Starts Asking Questions in channel",
	NULL
};
const char tvs_help_start_oneline[] = "Start Trivia";

const char *tvs_help_stop[] = {
	"Syntax: (in Channel only) \2!STOP\2",
	"",
	"Stops Asking Questions in channel",
	NULL
};
const char tvs_help_stop_oneline[] = "Stop Trivia";

const char *tvs_help_qs[] = {
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
const char tvs_help_qs_oneline[] = "Change Question Sets In Channel";
