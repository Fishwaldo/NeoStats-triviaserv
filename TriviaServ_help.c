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
	"\2LIST\2 will list the current Trivia",
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
	"catlist help",
	NULL
};
const char tvs_help_catlist_oneline[] = "Catlist";

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
