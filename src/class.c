/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "olc.h"
#include "loadrooms.h"
#include "comm.h"

/* extern functions */
void advance_level(struct char_data * ch);
void perform_recall (struct char_data *victim, sh_int room);

/* extern variales */
extern struct wis_app_type wis_app[];
extern struct con_app_type con_app[];
extern struct end_app_type end_app[];
extern sh_int r_mortal_start_room[NUM_STARTROOMS+1];	/* rnum of mortal start room	 */
extern struct room_data *world;

/* Names first */

const char *class_abbrevs[] = {
  "Cl",
  "Mu",
  "Wa",
  "Th",
  "Wh",
  "Av",
  "Dr",
  "\n"
};


const char *pc_class_types[] = {
  "Cleric",
  "Mage",
  "Warrior",
  "Thief",
  "Witchunter",
  "Avatar",
  "Druid",
  "\n"
};

/* The menu for choosing a class in interpreter.c: */

const char *class_menu =
"\r\n"
"Select a Hometown:\r\n\r\n"
"  [H]umpsumore (Cleric)\r\n"
"  [S]trokewood (Mage)\r\n"
"  [B]angingham (Warrior)\r\n"
"  [E]ffem (Thief)\r\n"
"  [F]umbleton (Witchunter)\r\n"
"  [D]igby (Avatar)\r\n"
"  [P]urralot (Druid)\r\n";


/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(struct char_data *ch, char arg)
{

  int class = CLASS_UNDEFINED;

  switch (LOWER(arg)) {
  case 'h':
    class = CLASS_HUMP;
    break;
  case 's':
    class = CLASS_STROKE;
    break;
  case 'b':
    class = CLASS_BANG;
    break;
  case 'e':
    class = CLASS_EFF;
    break;
  case 'f':
    class = CLASS_FUM;
    break;
  case 'd':
    class = CLASS_DIG;
    break;
  case 'p':
    class = CLASS_PURR;
	break;
   default:
    class = CLASS_UNDEFINED;
    break;
  }

  return (class);
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */

long find_class_bitvector(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
    case 'h':
      return (1 << 0);
      break;
    case 's':
      return (1 << 1);
      break;
    case 'b':
      return (1 << 2);
      break;
    case 'e':
      return (1 << 3);
      break;
    case 'f':
      return (1 << 4);
      break;
    case 'd':
      return (1 << 5);
      break;
	case 'p':
	  return (1 << 6);
	  break;
    default:
      return 0;
      break;
  }
}

/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 *
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 *
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

int prac_params[4][NUM_CLASSES] = {
  /* HUM	STR	 BAN    EFF      FUM    DIG		PUR*/
  {85,		85,	 85,     85,     85,     85,		85    }, /* learned level */
  {15,		18,	 12,     15,     15,     12,		12    }, /* max per prac */
  {7,		8,	 5,      10,     8,      9,		9    },	/* min per pac */
  {SKILL,	SKILL,	SKILL,  SKILL,  SKILL,  SKILL, 	        SKILL } /* prac name */
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 */
int guild_info[][3] = {

  /* Minos */
  {CLASS_STROKE,	4547,	SCMD_WEST},
  {CLASS_HUMP,	4562,	SCMD_WEST},
  {CLASS_EFF,		4551,	SCMD_EAST},
  {CLASS_BANG,	4563,	SCMD_EAST},
  {CLASS_PURR,         4541,   SCMD_NORTH},
  {CLASS_FUM,    4514,   SCMD_EAST},
  {CLASS_DIG,        4545,   SCMD_NORTH},

  /* New Thalos */

  {CLASS_STROKE,    5525,   SCMD_NORTH},
  {CLASS_HUMP,        5512,   SCMD_NORTH},
  {CLASS_BANG,       5526,   SCMD_SOUTH},
  {CLASS_EFF,         5532,   SCMD_SOUTH},


  /* Brass Dragon */
  {-999 /* all */ ,	5065,	SCMD_WEST},

  /* New Sparta */
  {CLASS_STROKE,	21075,	SCMD_NORTH},
  {CLASS_HUMP,	21019,	SCMD_WEST},
  {CLASS_EFF,		21014,	SCMD_SOUTH},
  {CLASS_BANG,	21023,	SCMD_SOUTH},

  /* Sundhaven */
  {CLASS_STROKE,    6616,   SCMD_UP},
  {CLASS_BANG,       6757,   SCMD_WEST},
  {CLASS_HUMP,        6616,   SCMD_UP},
  {CLASS_EFF,         6661,   SCMD_WEST},
  
  /* this must go last -- add new guards above! */
  {-1, -1, -1}
};


/* THAC0 for classes and levels.  (To Hit Armor Class 0) */

/* [class], [level] (all) */
const int thaco[NUM_CLASSES][LVL_IMPL + 1] = {

  /* HUMP */
  {100, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 18, 18, 18, 17, 17,
     17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
     13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10,
     9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6,
     5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

  /* STROKE */
  {100, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 18, 18, 18, 17, 17,
     17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
     13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10,
     9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6,
     5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

  /* BANG */
  {100, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 18, 18, 18, 17, 17,
     17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
     13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10,
     9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6,
     5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

  /* EFF */
  {100, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 18, 18, 18, 17, 17,
     17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
     13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10,
     9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6,
     5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

  /* FUM */
  {100, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 18, 18, 18, 17, 17,
     17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
     13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10,
     9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6,
     5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},

  /* DIG */
  {100, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 18, 18, 18, 17, 17,
     17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
     13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10,
     9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6,
     5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 1,
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
   /* PURR */
   {100, 20, 20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 18, 18, 18, 17, 17,
    17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
    13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2, 2, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.  Race also
 * affects stats.
 */

void roll_real_abils(struct char_data * ch)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  ch->real_abils.str_add = 0;

  switch (GET_CLASS(ch)) {
  case CLASS_HUMP:
    ch->real_abils.dex = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.str = table[4];
    ch->real_abils.end = table[5];
    break;
  case CLASS_STROKE:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.con = table[3];
    ch->real_abils.end = table[4];
    ch->real_abils.str = table[5];
    break;
  case CLASS_BANG:
    ch->real_abils.str = table[0];
    ch->real_abils.con = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.end = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.intel = table[5];
    break;
  case CLASS_EFF:
    ch->real_abils.dex = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.str = table[4];
    ch->real_abils.end = table[5];
    break;
  case CLASS_FUM:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.end = table[4];
    ch->real_abils.wis = table[5];
  case CLASS_DIG:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.end = table[4];
    ch->real_abils.dex = table[5];
  case CLASS_PURR:
    ch->real_abils.dex = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.end = table[4];
    ch->real_abils.con = table[5];
   }

  ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data * ch)
{
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;
  GET_GOLD(ch) = number(4000, 10000);

  SET_SKILL(ch, LANG_COMMON, 100);
  SPEAKING(ch) = LANG_COMMON;

  GET_OLC_ZONE1(ch) = -1;
  GET_OLC_ZONE2(ch) = -1;
  GET_OLC_ZONE3(ch) = -1;

  set_title(ch, "the citizen of Insanity.");
  POOFIN(ch) = str_dup("appears from a cloud of yellow mist!");
  POOFOUT(ch) = str_dup("suddenly explodes in a ball of green flames and is gone!");

  roll_real_abils(ch);
  ch->points.max_hit = 10;
  ch->points.max_stamina = 20;
  ch->points.max_move = 10;
  ch->points.max_mana = 10;

  /* set up initial languages and practices */
  switch (GET_CLASS(ch)) {

  case CLASS_STROKE:
    SET_SKILL(ch, LANG_STROKE, 100);
    break;
  case CLASS_HUMP:
    SET_SKILL(ch, LANG_HUMP, 100);
    break;
  case CLASS_BANG:
    SET_SKILL(ch, LANG_BANG, 100);
    break;
  case CLASS_EFF:
    SET_SKILL(ch, SKILL_SNEAK, 10);
    SET_SKILL(ch, SKILL_HIDE, 5);
    SET_SKILL(ch, SKILL_STEAL, 15);
    SET_SKILL(ch, SKILL_BACKSTAB, 10);
    SET_SKILL(ch, SKILL_PICK_LOCK, 10);
    SET_SKILL(ch, SKILL_TRACK, 10);
    SET_SKILL(ch, LANG_EFF, 100);
    break;
  case CLASS_FUM:  
    SET_SKILL(ch, SKILL_TRACK, 75);
    SET_SKILL(ch, LANG_FUM, 100);
    break;
  case CLASS_DIG:
    SET_SKILL(ch, SPELL_MAGIC_MISSILE, 100);
    SET_SKILL(ch, LANG_DIG, 100);
    break;
 case CLASS_PURR:
    SET_SKILL(ch, LANG_PURR, 100);
    break;
  }

  advance_level(ch);

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);
  GET_STAM(ch) = GET_MAX_STAM(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  /* put some flags on to start with */
  SET_BIT(PRF_FLAGS(ch), PRF_AUTOEXIT);
  SET_BIT(PRF_FLAGS(ch), PRF_AUTOLOOT);
  SET_BIT(PRF_FLAGS(ch), PRF_AUTOSPLIT);
  SET_BIT(PRF_FLAGS(ch), PRF_AUTOASSIST);
  SET_BIT(PRF_FLAGS(ch), PRF_AUTOGOLD);
  SET_BIT(PRF_FLAGS(ch), PRF_DAMAGE);

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data * ch)
{
  int add_hp = 0, add_mana = 0, add_move = 0, add_stam = 0, i;

  add_hp = con_app[GET_CON(ch)].hitp;

  add_stam = end_app[GET_END(ch)].bonus;

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    REMOVE_BIT(PLR_FLAGS(ch), PLR_PK);
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
  }

  if ((GET_LEVEL(ch) >= 10) && (((int)world[ch->in_room].number/100) == 186)) {
    send_to_char("Now you have reached level 10 you can no longer go into the newbie zone.\r\n"
		 "You Have been teleported to your hometown. Have fun!\r\n", ch);
    perform_recall(ch, r_mortal_start_room[(int)GET_CLASS(ch) + 1]);
  }

  /*
   * Now we modify the players stats depending on the
   * characteristics of the hometown.
   */
  switch (GET_CLASS(ch)) {

  case CLASS_STROKE:
    add_hp += number(8, 12);
    add_stam += number(5, 10);
    add_mana = number(GET_LEVEL(ch), (int) (4 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 55);
    add_move = number(3, 5);
    break;

  case CLASS_HUMP:
    add_hp += number(12, 15);
    add_stam += number(7, 15);
    add_mana = number(GET_LEVEL(ch), (int) (2 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 40);
    add_move = number(4, 10);
    break;

  case CLASS_EFF:
    add_hp += number(10, 20);
    add_stam += number(10, 15);
    add_mana = number(2, 8);
    add_move = number(5, 12);
    break;

  case CLASS_BANG:
    add_hp += number(10, 50);
    add_stam += number(10, 20);
    add_mana = number (1, 3);
    add_move = number(8, 15);
    break;

  case CLASS_FUM:
    add_hp += number(9, 40);
    add_stam += number(8, 15);
    add_mana = number (1, 6);
    add_move = number(15, 25);
    break;

  case CLASS_DIG:
    add_hp += number(8, 15);
    add_stam += number(4, 8);
    add_mana = number(GET_LEVEL(ch), (int) (3 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 45);
    add_move = number(3, 4);
    break;
  case CLASS_PURR:
    add_hp += number(20, 35);
    add_stam += number(6, 15);
    add_mana = number(8, 20);
    add_move = number(13, 22);
    break;
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);
  ch->points.max_stamina += MAX(1, add_stam);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  if (GET_CLASS(ch) == CLASS_STROKE || GET_CLASS(ch) == CLASS_HUMP || 
      GET_CLASS(ch) == CLASS_PURR || GET_CLASS(ch) == CLASS_DIG)

    GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);
  else
    GET_PRACTICES(ch) += MIN(2, MAX(1, wis_app[GET_WIS(ch)].bonus));


  save_char(ch, NOWHERE);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
  if (level <= 0)
    return 1;	  /* level 0 */
  else if (level <= 20)
    return 2;	  /* level 1 - 20 */
  else if (level <= 40)
    return 3;	  /* level 40 - 60 */
  else if (level <= 60)
    return 4;	  /* level 60 - 80 */
  else if (level <= 80)
    return 5;	  /* level 100 - 104 */
  else if (level < LVL_IMMORT)
    return 6;	  /* all remaining mortal levels */
  else
    return 20;	  /* immortals */
}

int knife_mult(int level)
{
  if (level <= 0)
    return 1;	  /* level 0 */
  else if (level <= 50)
    return 2;
  else if (level <= 90)
    return 3;	  /* level 50 - 90 */
  else if (level < LVL_IMMORT)
    return 4;	  /* level 90 - 105 */
  else
    return 20;	  /* immortals */
}
/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */

int invalid_class(struct char_data *ch, struct obj_data *obj) {
  if ((IS_OBJ_STAT(obj, ITEM_ANTI_STROKE) && IS_STROKE(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_HUMP) && IS_HUMP(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_EFF) && IS_EFF(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_FUM) && IS_FUM(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_DIG) && IS_DIG(ch)) ||
	  (IS_OBJ_STAT(obj, ITEM_ANTI_PURR) && IS_PURR(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_BANG) && IS_BANG(ch)))
    return 1;
  else
    return 0;
}



/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */

void init_spell_levels(void) {
  int i, j;
  int cls_stroke = (1 << CLASS_STROKE);
  int cls_hump = (1 << CLASS_HUMP);
  int cls_bang = (1 << CLASS_BANG);
  int cls_eff = (1 << CLASS_EFF);
  int cls_fum = (1 << CLASS_FUM);
  int cls_dig = (1 << CLASS_DIG);
  int cls_purr = (1 << CLASS_PURR);

  /* Assign a spell/skill to a a whole group of classes (0 is all)
   * For instance, { SKILL_SECOND_ATTACK, cls_mage | cls_cleric, 14 },
   * will give mages and clerics the SECOND_ATTACK skill at level 14.
   * More convenient than individual spell_level()s.  Use 0 to give
   * a skill to all the classes.
   */

  int base_skl[][3] = {
    { SKILL_MOUNT , 0, 1 },
    { SKILL_RIDING, 0, 1 },
    { LANG_COMMON,  0, 1 },
    { LANG_STROKE,  0, 1 },
    { LANG_HUMP,    0, 1 },
    { LANG_BANG,   0, 1 },
    { LANG_EFF,    0, 1 },
    { LANG_FUM,    0, 1 },
    { LANG_DIG,    0, 1 },
    { -1, -1 }
  };

  /* give all the base_skill's */
  for (j = 0; base_skl[j][0] != -1; j++)
    for (i = 0; i < NUM_CLASSES; i++)
      if (!base_skl[j][1] || IS_SET(base_skl[j][1], (1 << i)))
        spell_level(base_skl[j][0], i, base_skl[j][2]);
  
  /* in my base patch, cls_mage, etc. are unused and that leads to
   * annyoing warnings, so here I'll use them...
   */

  j = (cls_stroke-cls_stroke)+(cls_hump-cls_hump)+(cls_bang-cls_bang)
    +(cls_eff-cls_eff)+(cls_fum-cls_fum)+(cls_dig-cls_dig)+(cls_purr-cls_purr);

  /* MAGES */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_STROKE, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_STROKE, 2);
  spell_level(SPELL_DETECT_MAGIC, CLASS_STROKE, 3);
  spell_level(SPELL_CHILL_TOUCH, CLASS_STROKE, 4);
  spell_level(SPELL_INFRAVISION, CLASS_STROKE, 5);
  spell_level(SPELL_SHIELD, CLASS_STROKE, 6);
  spell_level(SPELL_INVISIBLE, CLASS_STROKE, 6);
  spell_level(SPELL_BURNING_HANDS, CLASS_STROKE, 7);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_STROKE, 8);
  spell_level(SPELL_STRENGTH, CLASS_STROKE, 10);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_STROKE, 11);
  spell_level(SPELL_SLEEP, CLASS_STROKE, 12);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_STROKE, 13);
  spell_level(SPELL_BLINDNESS, CLASS_STROKE, 14);
  spell_level(SPELL_ARMOR, CLASS_STROKE, 15);
  spell_level(SPELL_DETECT_POISON, CLASS_STROKE, 16);
  spell_level(SPELL_COLOR_SPRAY, CLASS_STROKE, 17);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_STROKE, 18);
  spell_level(SPELL_CURSE, CLASS_STROKE, 19);
  spell_level(SPELL_POISON, CLASS_STROKE, 20);
  spell_level(SKILL_TAME, CLASS_STROKE, 21);
  spell_level(SPELL_GROUP_INVIS, CLASS_STROKE, 22);
  spell_level(SPELL_FIREBALL, CLASS_STROKE, 23);
  spell_level(SKILL_SHOOT, CLASS_STROKE, 23);
  spell_level(SPELL_CHARM, CLASS_STROKE, 24);
  spell_level(SPELL_SENSE_LIFE, CLASS_STROKE, 25);
  spell_level(SPELL_FIRE_SPIT, CLASS_STROKE, 26);
  spell_level(SPELL_HASTE, CLASS_STROKE, 28);
  spell_level(SPELL_CONE_OF_COLD, CLASS_STROKE, 29);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_STROKE, 30);
  spell_level(SPELL_GROUP_FLY, CLASS_STROKE, 31);
  spell_level(SPELL_FROST_SPIT, CLASS_STROKE, 35);
  spell_level(SPELL_ACID_ARROW, CLASS_STROKE, 37);
  spell_level(SPELL_FLAME_ARROW, CLASS_STROKE, 39);
  spell_level(SPELL_LIGHTNING_SPIT, CLASS_STROKE, 42);
  spell_level(SPELL_AREA_LIGHTNING, CLASS_STROKE, 53);
  spell_level(SPELL_CONJ_ELEMENTAL, CLASS_STROKE, 60);
  spell_level(SPELL_TELEPORT, CLASS_STROKE, 65);
  spell_level(SKILL_SCRIBE, CLASS_STROKE, 68);
  spell_level(SPELL_DISINTEGRATE, CLASS_STROKE, 70);
  spell_level(SKILL_BREW, CLASS_STROKE, 80);
  spell_level(SPELL_PORTAL, CLASS_STROKE, 85);


  /* CLERICS */
  spell_level(SPELL_CURE_LIGHT, CLASS_HUMP, 1);
  spell_level(SPELL_DETECT_POISON, CLASS_HUMP, 2);
  spell_level(SPELL_DETECT_ALIGN, CLASS_HUMP, 3);
  spell_level(SPELL_CURE_BLIND, CLASS_HUMP, 4);
  spell_level(SPELL_DETECT_INVIS, CLASS_HUMP, 5);
  spell_level(SPELL_BLINDNESS, CLASS_HUMP, 6);
  spell_level(SPELL_INFRAVISION, CLASS_HUMP, 7);
  spell_level(SPELL_CREATE_WATER, CLASS_HUMP, 8);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_HUMP, 8);
  spell_level(SPELL_CURE_CRITIC, CLASS_HUMP, 9);
  spell_level(SPELL_REMOVE_POISON, CLASS_HUMP, 10);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_HUMP, 11);
  spell_level(SPELL_BLESS, CLASS_HUMP, 12);
  spell_level(SPELL_EARTHQUAKE, CLASS_HUMP, 13);
  spell_level(SPELL_DISPEL_GOOD, CLASS_HUMP, 14);
  spell_level(SPELL_ARMOR, CLASS_HUMP, 15);
  spell_level(SPELL_SUMMON, CLASS_HUMP, 16);
  spell_level(SPELL_DISPEL_EVIL, CLASS_HUMP, 17);
  spell_level(SPELL_MONSUM_I, CLASS_HUMP, 18);
  spell_level(SPELL_WATERBREATH, CLASS_HUMP, 19);
  spell_level(SKILL_TAME, CLASS_HUMP, 20);
  spell_level(SPELL_GROUP_ARMOR, CLASS_HUMP, 20);
  spell_level(SPELL_FLY, CLASS_HUMP, 21);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_HUMP, 22);
  spell_level(SPELL_HARM, CLASS_HUMP, 23);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_HUMP, 24);
  spell_level(SPELL_GROUP_HEAL, CLASS_HUMP, 26);
  spell_level(SPELL_TELEPORT, CLASS_HUMP, 27);
  spell_level(SPELL_GROUP_PROT_EVIL, CLASS_HUMP, 28);
  spell_level(SKILL_SHOOT, CLASS_HUMP, 29);
  spell_level(SPELL_REMOVE_CURSE, CLASS_HUMP, 30);
  spell_level(SPELL_HEAL, CLASS_HUMP, 31);
  spell_level(SPELL_SANCTUARY, CLASS_HUMP, 39);
  spell_level(SPELL_CREATE_FOOD, CLASS_HUMP, 45);
  spell_level(SPELL_LEVITATE, CLASS_HUMP, 50);
  spell_level(SKILL_BREW, CLASS_HUMP, 60);
  spell_level(SPELL_MONSUM_II, CLASS_HUMP, 65);
  spell_level(SPELL_RELOCATE, CLASS_HUMP, 70);
  spell_level(SKILL_SCRIBE, CLASS_HUMP, 76);
  spell_level(SPELL_MONSUM_III, CLASS_HUMP, 80);
  spell_level(SPELL_DOOM, CLASS_HUMP, 82);
 
  /* THIEVES */
  spell_level(SKILL_SNEAK, CLASS_EFF, 1);
  spell_level(SKILL_PICK_LOCK, CLASS_EFF, 5);
  spell_level(SKILL_BACKSTAB, CLASS_EFF, 6);
  spell_level(SKILL_STEAL, CLASS_EFF, 10);
  spell_level(SKILL_HIDE, CLASS_EFF, 14);
  spell_level(SKILL_SHOOT, CLASS_EFF, 20);
  spell_level(SKILL_TAME, CLASS_EFF, 25);
  spell_level(SKILL_TRACK, CLASS_EFF, 30);
  spell_level(SKILL_KNIFE, CLASS_EFF, 35);
  spell_level(SKILL_SECOND_ATTACK, CLASS_EFF, 50);
  spell_level(SKILL_FORGE, CLASS_EFF, 65);


  /* WARRIORS */
  spell_level(SKILL_KICK, CLASS_BANG, 1);  
  spell_level(SKILL_TRACK, CLASS_BANG, 8);
  spell_level(SKILL_TAME, CLASS_BANG, 12);
  spell_level(SKILL_RESCUE, CLASS_BANG, 15);
  spell_level(SKILL_SHOOT, CLASS_BANG, 17);
  spell_level(SKILL_PUSH, CLASS_BANG, 20);
  spell_level(SKILL_BASH, CLASS_BANG, 25);
  spell_level(SKILL_SECOND_ATTACK, CLASS_BANG, 28);
  spell_level(SKILL_DISARM, CLASS_BANG, 30);
  spell_level(SKILL_PARRY, CLASS_BANG, 35);
  spell_level(SKILL_THIRD_ATTACK, CLASS_BANG, 40);
  spell_level(SKILL_STRIKE, CLASS_BANG, 50);
  spell_level(SKILL_FORGE, CLASS_BANG, 72);

 
  
  /* DRUIDS */
  spell_level(SPELL_CURE_LIGHT, CLASS_PURR, 1);
  spell_level(SPELL_CREATE_FOOD, CLASS_PURR, 2);
  spell_level(SPELL_CREATE_WATER, CLASS_PURR, 3);
  spell_level(SPELL_DETECT_ALIGN, CLASS_PURR, 5);
  spell_level(SPELL_CURE_BLIND, CLASS_PURR, 6);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_PURR, 8);
  spell_level(SPELL_CURE_CRITIC, CLASS_PURR, 10);
  spell_level(SPELL_SUMMON, CLASS_PURR, 14);
  spell_level(SPELL_REMOVE_POISON, CLASS_PURR, 15);
  spell_level(SPELL_BLESS, CLASS_PURR, 16);
  spell_level(SPELL_REMOVE_CURSE, CLASS_PURR, 17);
  spell_level(SPELL_BARKSKIN, CLASS_PURR, 18);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_PURR, 20);
  spell_level(SPELL_LEVITATE, CLASS_PURR, 21);
  spell_level(SKILL_TAME, CLASS_PURR, 22);
  spell_level(SPELL_DISPEL_EVIL, CLASS_PURR, 23);
  spell_level(SPELL_MONSUM_IV, CLASS_PURR, 25);
  spell_level(SPELL_HARM, CLASS_PURR, 26);
  spell_level(SPELL_DISPEL_GOOD, CLASS_PURR, 28);
  spell_level(SPELL_SANCTUARY, CLASS_PURR, 31);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_PURR, 32);
  spell_level(SKILL_SHOOT, CLASS_PURR, 33);
  spell_level(SPELL_FEAR, CLASS_PURR, 35);
  spell_level(SPELL_HEAL, CLASS_PURR, 36);
  spell_level(SPELL_STONESKIN, CLASS_PURR, 39);
  spell_level(SPELL_TELEPORT, CLASS_PURR, 50);  
  spell_level(SPELL_GROUP_STONESKIN, CLASS_PURR, 60);
  spell_level(SPELL_GODLY_HEAL, CLASS_PURR, 65);
  spell_level(SPELL_GODLY_ASSIST, CLASS_PURR, 70);
  spell_level(SPELL_DOOM, CLASS_PURR, 75);
  spell_level(SPELL_MONSUM_V, CLASS_PURR, 81);
  spell_level(SPELL_DISINTEGRATE, CLASS_PURR, 95);  

  /* WITCHUNTER */
  spell_level(SKILL_TRACK, CLASS_FUM, 1);
  spell_level(SKILL_KICK, CLASS_FUM, 6);
  spell_level(SKILL_TAME, CLASS_FUM, 12);
  spell_level(SKILL_SNEAK, CLASS_FUM, 18);
  spell_level(SKILL_RESCUE, CLASS_FUM, 21);
  spell_level(SKILL_LISTEN, CLASS_FUM, 22);
  spell_level(SKILL_SHOOT, CLASS_FUM, 28);
  spell_level(SKILL_BASH, CLASS_FUM, 31);
  spell_level(SKILL_SECOND_ATTACK, CLASS_FUM, 33);
  spell_level(SKILL_PARRY, CLASS_FUM, 50);
  spell_level(SKILL_FORAGE, CLASS_FUM, 60);
  spell_level(SKILL_THIRD_ATTACK, CLASS_FUM, 70);


  /* AVATAR */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_DIG, 1);
  spell_level(SPELL_CHILL_TOUCH, CLASS_DIG, 1);
  spell_level(SPELL_BURNING_HANDS, CLASS_DIG, 3);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_DIG, 5);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_DIG, 9);
  spell_level(SPELL_BLINDNESS, CLASS_DIG, 11);
  spell_level(SPELL_COLOR_SPRAY, CLASS_DIG, 12);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_DIG, 14);
  spell_level(SPELL_CURSE, CLASS_DIG, 15);
  spell_level(SPELL_FIREBALL, CLASS_DIG, 16);
  spell_level(SPELL_MINUTE_METEOR, CLASS_DIG, 17);
  spell_level(SPELL_FIRE_SPIT, CLASS_DIG, 18);
  spell_level(SPELL_CONE_OF_COLD, CLASS_DIG, 22);
  spell_level(SKILL_TAME, CLASS_DIG, 23);
  spell_level(SPELL_ACID_ARROW, CLASS_DIG, 25);
  spell_level(SKILL_SHOOT, CLASS_DIG, 26);
  spell_level(SPELL_ANIMATE_DEAD, CLASS_DIG, 27);
  spell_level(SPELL_FROST_SPIT, CLASS_DIG, 28);
  spell_level(SPELL_FLAME_ARROW, CLASS_DIG, 30);
  spell_level(SPELL_LIGHTNING_SPIT, CLASS_DIG, 33);
  spell_level(SPELL_DOOM, CLASS_DIG, 36);
  spell_level(SPELL_GAS_SPIT, CLASS_DIG, 38);
  spell_level(SPELL_BLADEBARRIER, CLASS_DIG, 42);
  spell_level(SPELL_ACID_SPIT, CLASS_DIG, 50);
  spell_level(SPELL_TELEPORT, CLASS_DIG, 60);
  spell_level(SPELL_DISINTEGRATE, CLASS_DIG, 82);
}

