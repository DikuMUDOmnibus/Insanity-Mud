/*************************************************************************
*   File: config.c                                      Part of CircleMUD *
*  Usage: Configuration of various aspects of CircleMUD operation         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __CONFIG_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "loadrooms.h"
#include "utils.h"

#define YES	1
#define FALSE	0
#define NO	0

/*
 * Below are several constants which you can change to alter certain aspects
 * of the way CircleMUD acts.  Since this is a .c file, all you have to do
 * to change one of the constants (assuming you keep your object files around)
 * is change the constant in this file and type 'make'.  Make will recompile
 * this file and relink; you don't have to wait for the whole thing to
 * recompile as you do if you change a header file.
 *
 * I realize that it would be slightly more efficient to have lots of
 * #defines strewn about, so that, for example, the autowiz code isn't
 * compiled at all if you don't want to use autowiz.  However, the actual
 * code for the various options is quite small, as is the computational time
 * in checking the option you've selected at run-time, so I've decided the
 * convenience of having all your options in this one file outweighs the
 * efficency of doing it the other way.
 *
 */

/****************************************************************************/
/****************************************************************************/


/* GAME PLAY OPTIONS */

/*
 * pk_allowed sets the tone of the entire game.  If pk_allowed is set to
 * NO, then players will not be allowed to kill, summon, charm, or sleep
 * other players, as well as a variety of other "asshole player" protections.
 * However, if you decide you want to have an all-out knock-down drag-out
 * PK Mud, just set pk_allowed to YES - and anything goes.
 */
int pk_allowed(struct char_data *ch, struct char_data *vict) {

  if (PLR_FLAGGED(ch, PLR_PK) && PLR_FLAGGED(vict, PLR_PK))
    return TRUE;

  if (GET_LEVEL(ch) >= LVL_KNIGHT && GET_LEVEL(vict) >= LVL_KNIGHT)
    return TRUE;
  else
    return FALSE;
}

/* is playerthieving allowed? */
int pt_allowed = YES;

/* minimum level a player must be to shout/holler/gossip/auction */
int level_can_shout = 2;

/* number of movement points it costs to holler */
int holler_move_cost = 20;

/* exp change limits */
int max_exp_gain = 310000;	/* max gainable per kill */
int max_exp_loss = 1000000;	/* max losable per death */

/* number of tics (usually 75 seconds) before PC/NPC corpses decompose */
int max_npc_corpse_time = 5;
int max_pc_corpse_time = 20;
int max_pieces_time = 30;

/* should items in death traps automatically be junked? */
int dts_are_dumps = NO;

/* "okay" etc. */
const char *OK = "Okay.\r\n";
const char *NOPERSON = "There ain't no-one here by that name.\r\n";
const char *NOEFFECT = "Looks like you messed up, nothing happened!\r\n";

/****************************************************************************/
/****************************************************************************/


/* RENT/CRASHSAVE OPTIONS */

/*
 * Should the MUD allow you to 'rent' for free?  (i.e. if you just quit,
 * your objects are saved at no cost, as in Merc-type MUDs.)
 */
int free_rent = YES;

/* maximum number of items players are allowed to rent */
int max_obj_save = 60;

/* receptionist's surcharge on top of item costs */
int min_rent_cost = 1;

/*
 * Should the game automatically save people?  (i.e., save player data
 * every 4 kills (on average), and Crash-save as defined below.
 */
int auto_save = YES;

/*
 * if auto_save (above) is yes, how often (in minutes) should the MUD
 * Crash-save people's objects?   Also, this number indicates how often
 * the MUD will Crash-save players' houses.
 */
int autosave_time = 10;

/* Lifetime of crashfiles and forced-rent (idlesave) files in days */
int crash_file_timeout = 45;

/* Lifetime of normal rent files in days */
int rent_file_timeout = 90;


/****************************************************************************/
/****************************************************************************/


/* ROOM NUMBERS */

/* virtual number of room that mortals should enter at */
sh_int mortal_start_room[NUM_STARTROOMS+1] = {
  4579,		/* the main Recall  */
  5509,      /* Hometown Recalls */
  21112,
  6601,
  5509,
  21112,
  6601,
  5509
};

/* virtual number of room that immorts should enter at by default */
sh_int immort_start_room = 1204;

/* virtual number of the newbie start room. */
sh_int newbie_start_room = 18600;

/* virtual number of room that frozen players should enter at */
sh_int frozen_start_room = 1202;

/* virtual room number of place where corpses are sent to */
sh_int corpse_goto_room = 4569;

/* virtual room number of place where dead people are sent to */
sh_int death_room = 4570;

/* virtual number of room that jailed players should enter at */
sh_int jailed_start_room = 1211;

 /* virtual numbers of donation rooms.  Changed to array by Zeus */

sh_int donation_rooms[NUM_DONROOMS+1] = {
  -1,      /* needed so that there is a chance of it not succeding */
  4580,    /* Minos */
  5510,    /* New thalos */
  6787,    /* Sundhaven */
  21113   /* New Sparta */
}; 


/****************************************************************************/
/****************************************************************************/


/* GAME OPERATION OPTIONS */

/*
 * This is the default port the game should run on if no port is given on
 * the command-line.  NOTE WELL: If you're using the 'autorun' script, the
 * port number there will override this setting.  Change the PORT= line in
 * instead of (or in addition to) changing this.
 */
int DFLT_PORT = 6666;

/* default directory to use as data directory */
const char *DFLT_DIR = "lib";

/* maximum number of players allowed before game starts to turn people away */
int MAX_PLAYERS = 25;

/* maximum size of bug, typo and idea files in bytes (to prevent bombing) */
int max_filesize = 6000;

/* maximum number of password attempts before disconnection */
int max_bad_pws = 2;

/*
 * Some nameservers are very slow and cause the game to lag terribly every
 * time someone logs in.  The lag is caused by the gethostbyaddr() function
 * which is responsible for resolving numeric IP addresses to alphabetic names.
 * Sometimes, nameservers can be so slow that the incredible lag caused by
 * gethostbyaddr() isn't worth the luxury of having names instead of numbers
 * for players' sitenames.
 *
 * If your nameserver is fast, set the variable below to NO.  If your
 * nameserver is slow, of it you would simply prefer to have numbers
 * instead of names for some other reason, set the variable to YES.
 *
 * You can experiment with the setting of nameserver_is_slow on-line using
 * the SLOWNS command from within the MUD.
 */

int nameserver_is_slow = NO;

const char *ANSI = "Do you want ANSI colour (Y/N)? ";

const char *PK = "Do you want to take part in player killing (Y/N)? ";

const char *MENU =
"\r\n"

"                        Insanity Main Menu.\r\n\r\n"
"   0) Exit from Insanity.       1) Enter the the World of Insanity.\r\n"
"   2) Enter your description.   3) Read the background story.\r\n"
"   4) Change your password.     5) Delete this character.\r\n"
"   6) Who's Online.             7) Read the Policy.\r\n"
"   \r\n"
"                        Pick something then: ";



const char *GREETINGS =

"\r\n\r\n"
" Welcome to....\r\n"
"                    _____                       _ _         \r\n"
"                    \\_   \\_ __  ___  __ _ _ __ (_) |_ _   _ \r\n"
"                     / /\\/ '_ \\/ __|/ _` | '_ \\| | __| | | |\r\n"
"                  /\\/ /_ | | | \\__ \\ (_| | | | | | |_| |_| |\r\n"
"                  \\____/ |_| |_|___/\\__,_|_| |_|_|\\__|\\__, |\r\n"
"                                                      |___/ \r\n"
"\r\n"
"                                    A derivative of DikuMUD\r\n"
"    Based on CircleMud 3.0,         Created by Hans Staerfeldt, Sebastian\r\n"
"    created by Jeremy Elson         Hammer, Katja Nyboe, Tom Madsen,\r\n"
"                                    and Michael Seifert\r\n"
"\r\n"
"    Senior Implementor - Zeus       Coding - Zeus and Nahaz.\r\n"
"\r\n\r\n"
"Hello, Who are you? ";



const char *ANSIGREET =

"\r\n\r\n"
" Welcome to....\r\n"
"&y                    _____                       _ _         \r\n"
"                    \\_   \\_ __  ___  __ _ _ __ (_) |_ _   _ \r\n"
"                     / /\\/ '_ \\/ __|/ _` | '_ \\| | __| | | |\r\n"
"                  /\\/ /_ | | | \\__ \\ (_| | | | | | |_| |_| |\r\n"
"                  \\____/ |_| |_|___/\\__,_|_| |_|_|\\__|\\__, |\r\n"
"                                                      |___/ \r\n"
"&n\r\n"
"                                    &rA derivative of DikuMUD\r\n"
"    &gBased on CircleMud 3.0,         &rCreated by Hans Staerfeldt, Sebastian\r\n"
"    &gcreated by Jeremy Elson         &rHammer, Katja Nyboe, Tom Madsen,\r\n"
"                                    &rand Michael Seifert\r\n"
"\r\n"
"    &wSenior Implementor - Zeus       &mCoding - Zeus and Nahaz\r\n"
"\r\n\r\n"
"&cHello, Who are you?&n ";


char const *WELC_MESSG =
"\r\n"
"Welcome to the land of Insanity!  May your visit here be... Very Insane!"
"\r\n\r\n";

char const *START_MESSG =
"Welcome.  This is your new character in Insanity!  You can now earn gold,\r\n"
"gain experience, find weapons and equipment, and much more -- while\r\n"
"meeting people from around the world! This is not stock circle, read the\r\n"
"Help file for information on new things.";

/****************************************************************************/
/****************************************************************************/


/* AUTOWIZ OPTIONS */

/* Should the game automatically create a new wizlist/immlist every time
   someone immorts, or is promoted to a higher (or lower) god level? */
int use_autowiz = YES;

/* If yes, what is the lowest level which should be on the wizlist?  (All
   immort levels below the level you specify will go on the immlist instead.) */
int min_wizlist_lev = LVL_GOD;














