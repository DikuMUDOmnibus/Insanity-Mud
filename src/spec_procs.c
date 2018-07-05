/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "clan.h"
#include "screen.h"

/*   external vars  */
extern const char *spells[];
extern int guild_info[][3];
extern struct spell_info_type spell_info[];
extern struct int_app_type int_app[];
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct command_info cmd_info[];
extern struct clan_control_rec clan_control[MAX_CLANS];
extern int castles[MAX_CASTLES];
extern int num_of_clans;

/* extern functions */
void add_follower(struct char_data * ch, struct char_data * leader);
char *fname(char *namelist);
ACMD(do_say);
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_flee);
void perform_remove(struct char_data * ch, int pos);

struct social_type {
  char *cmd;
  int next_line;
};


/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

int spell_sort_info[LANG_COMMON+1];

extern const char *spells[];

void sort_spells(void)
{
  int a, b, tmp;

  /* initialize arrays */
  for (a = 1; a < LANG_COMMON; a++)
    spell_sort_info[a] = a;


  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < LANG_COMMON - 1; a++)
    for (b = a + 1; b < LANG_COMMON; b++)
      if (strcmp(spells[spell_sort_info[a]], spells[spell_sort_info[b]]) > 0) {
	tmp = spell_sort_info[a];
	spell_sort_info[a] = spell_sort_info[b];
	spell_sort_info[b] = tmp;
      }
}

const char *prac_types[] = {
  "spell",
  "skill"
};

#define LEARNED_LEVEL	0	/* % known which is considered "learned" */
#define MAX_PER_PRAC	1	/* max percent gain in skill per practice */
#define MIN_PER_PRAC	2	/* min percent gain in skill per practice */
#define PRAC_TYPE	3	/* should it say 'spell' or 'skill'?	 */

/* actual prac_params are in class.c */
extern int prac_params[4][NUM_CLASSES];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

/* this altered by nahaz to allow show score */
void list_skills(struct char_data * ch, struct char_data * vict)
{
  int i, sortpos;
  int odd = 1;

  if (!GET_PRACTICES(vict))
    strcpy(buf, "You have no practice sessions remaining.\r\n");
  else
    sprintf(buf, "You have %d practice session%s remaining.\r\n",
            GET_PRACTICES(vict), (GET_PRACTICES(vict) == 1 ? "" : "s"));
  
  sprintf(buf, "%sYou know of the following %ss:\r\n", buf, SPLSKL(vict));
  
  strcpy(buf2, buf);
  
  for (sortpos = 1; sortpos < LANG_COMMON; sortpos++) {
    i = spell_sort_info[sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if (GET_LEVEL(vict) >= spell_info[i].min_level[(int) GET_CLASS(vict)]) {
      if (odd) {
	sprintf(buf, "%-30s %s%3d%%%s    ", 
		spells[i], CCYEL(ch, C_NRM), GET_SKILL(vict, i), CCNRM(ch, C_NRM));
	strcat(buf2, buf);
	odd--;
      } else {
	sprintf(buf, "%-30s %s%d%%%s\r\n", 
		spells[i], CCYEL(ch, C_NRM), GET_SKILL(vict, i), CCNRM(ch, C_NRM));
	strcat(buf2, buf);
	odd++;
      }
      
    }
  }
  
  page_string(ch->desc, buf2, 1);
}

SPECIAL(you) {

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
    call_magic(ch, FIGHTING(ch), 0, SPELL_ENERGY_DRAIN, GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
  
}

SPECIAL(eris) {

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
    call_magic(ch, FIGHTING(ch), 0, SPELL_MINUTE_METEOR, GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
}

SPECIAL(icewizard){

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room)) {
    act("$n whispers something to $N", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n whispers, 'I'm going to rip out your soul $N!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    act("$n's frayed cloak blows as he points his staff at $N.", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n's frayed cloak blows as he aims his staff at you.", 1, ch, 0, FIGHTING(ch), TO_VICT);
    act("A Pale light seems to surround $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("A Pale light seems to surround your body!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, SPELL_CONE_OF_COLD, GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
  
}

SPECIAL(guild)
{
  int skill_num, percent;

  if (IS_NPC(ch) || !CMD_IS("practice"))
    return 0;

  skip_spaces(&argument);

  if (!*argument) {
    list_skills(ch, ch);
    return 1;
  }
  if (GET_PRACTICES(ch) <= 0) {
    send_to_char("You do not seem to be able to practice now.\r\n", ch);
    return 1;
  }

  skill_num = find_skill_num(argument);

  if (skill_num < 1 || skill_num >= LANG_COMMON ||
      GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
    sprintf(buf, "You do not know of that %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return 1;
  }
  if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char("You are already learned in that area.\r\n", ch);
    return 1;
  }
  send_to_char("You practice for a while...\r\n", ch);
  GET_PRACTICES(ch)--;

  percent = GET_SKILL(ch, skill_num);
  percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));

  SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\r\n", ch);

  return 1;


}

SPECIAL(linguist)
{
  int skill_num, percent;
  extern void list_lang(struct char_data *ch);

  if (IS_NPC(ch) || !CMD_IS("practice"))
    return 0;

  skip_spaces(&argument);

  if (!*argument) {
    list_lang(ch);
    return 1;
  }

  if (GET_PRACTICES(ch) <= 1) {
    send_to_char("You don't have enough practice sessions left to learn the language.\r\n", ch);
    return 1;
  }

  skill_num = find_skill_num(argument);

  if (skill_num < LANG_COMMON || skill_num > MAX_LANG ||
      GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)]) {
    sprintf(buf, "There is no such language as %s.\r\n", argument);
    send_to_char(buf, ch);
    return 1;
  }

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char("You feel you can not learn anymore about that language.\r\n", ch);
    return 1;
  }

  send_to_char("You begin to understand the words a little better.\r\n", ch);
  GET_PRACTICES(ch) = GET_PRACTICES(ch) - 2;

  percent = GET_SKILL(ch, skill_num);
  percent += number (1, 5);

  SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You feel you can not learn anymore about that language.\r\n", ch);

  return 1;
}



SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p gets taken away and put into the furnace!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return 0;

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p gets taken away and put into the furnace.", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    act("You are awarded for outstanding performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 10)
      gain_exp(ch, value);
    else
      GET_GOLD(ch) += value;
  }
  return 1;
}

SPECIAL(newbie_guide)
{
 
 static char tour_path[] =
 "WAAAA500E33U123K11101T330O0010P21110N23333330I232F032D0111103V103G1011R332222Q2222110L23332M032J0114H4BC5Z."; 

static char *path; 
static int index; 
static bool move = FALSE;

 if (!move) {
   if (((time_info.hours == 12) || 
	(time_info.hours == 0)) && 
       (index == 0)) { /* Tour starts at 12 pm and 1pm unless she's not back*/
     move = TRUE;
     path = tour_path;
     index = 0;
   }
 }
 if (cmd || !move || (GET_POS(ch) < POS_RESTING) ||
     (GET_POS(ch) == POS_FIGHTING))
   return FALSE;

 switch (path[index]) {
 case '0': 
 case '1': 
 case '2': 
 case '3': 
 case '4':
 case '5':
   perform_move(ch, path[index] - '0', 1); 
   break;
  case 'W':
    GET_POS(ch)=POS_STANDING;
    act("$n stands up and announces 'The tour is going to start soon!'", FALSE, ch, 0, 0, TO_ROOM);
    break;
  case 'Z':
    act("$n sits and rests for $s next journey.", FALSE, ch, 0, 0, TO_ROOM);
    GET_POS(ch)=POS_RESTING;
    break;
  case 'M': 
    act("$n says 'This is the enterence to the WARRIORS guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
   case 'L': 
    act("$n says 'This is the enterence to the THIEVES guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'K': 
    act("$n says 'This is the enterence to the MAGES guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'J': 
    act("$n says 'This is the enterence to the CLERICS guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'U': 
    act("$n says 'This is the enterence to the DRUIDS guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'N': 
    act("$n says 'This is the enterence to the WITCHUNTERS guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'T': 
    act("$n says 'This is the enterence to the AVATARS guild.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'V': 
    act("$n says 'This is the Minos Doctors Surgery, Come here to get healed.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'H': 
    act("$n says 'Right now, you may find it usefull to type 'WEAR ALL''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
    case 'A': 
    act("$n says 'Newbies!! Type 'FOLLOW GUIDE' for a guided tour'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'B': 
    act("$n says 'This here is the enterence to the newbie area. Please, type 'FOLLOW SELF''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'C': 
    act("$n says 'Now have fun out there, and be careful!'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'Q': 
    act("$n says 'Type DISPLAY 1 to see your HITPOINTS, MANA and MOVEPOINTS'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'P': 
    act("$n says 'This is the Bank, deposit money here to keep it safe.'", FALSE, ch, 0, 0, TO_ROOM); 
   act("$n says 'Type 'DEPOSIT' to put money it, 'WITHDRAW' to take money out.'", FALSE, ch, 0, 0, TO_ROOM); 
   act("$n says 'And 'BALANCE' to see how much money you have in.'", FALSE, ch, 0, 0, TO_ROOM);    
   break;
  case 'O': 
    act("$n says 'To help you find the exits type 'AUTOEXIT''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'D': 
    act("$n says 'This is our dear friend the baker, to buy bread from him, type 'BUY BREAD''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'E': 
    act("$n says 'This is the Fountain, to drink from it, type 'DRINK FOUNTAIN''", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'F': 
    act("$n says 'This is our dear friend Wally, he will sell you water, type 'LIST' to see a list of what he has.'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case 'G': 
    act("$n says 'This is the Armorer, he makes armor, type LIST to see what he has to sell'", FALSE, ch, 0, 0, TO_ROOM); 
    break; 
  case 'I': 
    act("$n says 'This is the Weaponsmith, he makes weapons, type LIST to see what he has to sell'", FALSE, ch, 0, 0, TO_ROOM); 
    break;
  case '.':
    move = FALSE;
    break;
  case 'R':
    act("$n says 'This is the RECEPTION, in this MUD, you must RENT.'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'To see how much your rent will cost, type 'OFFER'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n says 'To rent, type RENT.'", FALSE, ch, 0, 0, TO_ROOM);
    break;
  }
 index++;
 return FALSE;
}

/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */


void npc_steal(struct char_data * ch, struct char_data * victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_OVERSEER)
    return;

  if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
    if (gold > 0) {
      GET_GOLD(ch) += gold;
      GET_GOLD(victim) -= gold;
    }
  }
}


SPECIAL(snake)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) &&
      (number(0, 42 - GET_LEVEL(ch)) == 0)) {
    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
    return TRUE;
  }
  return FALSE;
}


SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_STANDING)
    return FALSE;

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_OVERSEER) && (!number(0, 4))) {
      npc_steal(ch, cons);
      return TRUE;
    }
  return FALSE;
}


SPECIAL(magic_user)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
    cast_spell(ch, vict, NULL, SPELL_SLEEP);

  if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
    cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

  if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
  }
  if (number(0, 4))
    return TRUE;

  switch (GET_LEVEL(ch)) {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL);
    break;
  }
  return TRUE;

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard)
{
  int i;
  struct char_data *guard = (struct char_data *) me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || IS_AFFECTED(guard, AFF_BLIND))
    return FALSE;

  if (GET_LEVEL(ch) >= LVL_OVERSEER)
    return FALSE;

  for (i = 0; guild_info[i][0] != -1; i++) {
    if ((IS_NPC(ch) || GET_CLASS(ch) != guild_info[i][0]) &&
	world[ch->in_room].number == guild_info[i][1] &&
	cmd == guild_info[i][2]) {
      send_to_char(buf, ch);
      act(buf2, FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  }

  return FALSE;
}

SPECIAL(syrup)
{
  struct obj_data *tobj = me;

  if (tobj->in_room == NOWHERE)
    return 0;

  if ((CMD_IS("north") || CMD_IS("south") || CMD_IS("east") || CMD_IS("west") ||
      CMD_IS("up") || CMD_IS("down") || CMD_IS("recall") || CMD_IS("cast") || 
      CMD_IS("recite") || CMD_IS("quaff") || CMD_IS("quit")) && GET_LEVEL(ch) < LVL_LOWIMPL) {
      act("You are held fast by $p.", FALSE, ch, tobj, 0, TO_CHAR);
      act("$n tries to move, but $e is stuck in $p.", FALSE, ch, tobj, 0, TO_ROOM);
      return 1;
  } 
  return 0;
}

SPECIAL(marbles)
{
  struct obj_data *tobj = me;

  if (tobj->in_room == NOWHERE)
    return 0;

  if (CMD_IS("north") || CMD_IS("south") || CMD_IS("east") || CMD_IS("west") ||
      CMD_IS("up") || CMD_IS("down")) {
    if (number(1, 100) + GET_DEX(ch) > 50) {
      act("You slip on $p and fall.", FALSE, ch, tobj, 0, TO_CHAR);
      act("$n slips on $p and falls.", FALSE, ch, tobj, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      return 1;
    }
    else {
      act("You slip on $p, but manage to retain your balance.", FALSE, ch, tobj, 0, TO_CHAR);
      act("$n slips on $p, but manages to retain $s balance.", FALSE, ch, tobj, 0, TO_ROOM);
    }
  }
  return 0;
}

/* end procedure sundhaven */

SPECIAL(angel)
{
  if (cmd)
    return (0);

  switch (number(0, 20)) {
  case 0:
    do_say(ch, "You must travel upwards to reach you true destiny.", 0, 0);
    return (1);
  case 1:
    do_say(ch, "The gods have decided to give you another life.", 0, 0);
    return (1);
  case 2:
    do_say(ch, "Quick get out of here before the Devil comes to get you!", 0, 0);
    return (1);
  case 3:
    do_say(ch, "Do you like floating about here", 0, 0);
    return (1);
  case 4:
    do_say (ch, "Go and Get your corpse up from here.", 0, 0);
    return (1);
  case 5:
    do_say (ch, "If only you had used consider!", 0, 0);
    return (1);
  case 6:
    do_say (ch, "Don't loot anyones else corpse, the Gods will punish you!", 0, 0);
  default:
    return (0);
  }
}



SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3)) {
      act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
      for (temp = i->contains; temp; temp = next_obj) {
	next_obj = temp->next_content;
	obj_from_obj(temp);
	obj_to_room(temp, ch->in_room);
      }
      extract_obj(i);
      return (TRUE);
    }
  }
  return (FALSE);
}

/* This special procedure makes a mob into a 'rent-a-cleric', who sells spells
   by the sea shore... uuh, maybe not.  Anyway, the mob will also cast certain
   spells on low-level characters in the room for free.  
   By:  Wyatt Bode      Date:  April, 1996
*/

SPECIAL(cleric)
{
  int i;
  struct char_data *vict;
  struct price_info {
    short int number;
    char name[25];
    short int price;
  } prices[] = {
    /* Spell Num (defined)      Name shown        Price  */
    { SPELL_ARMOR,              "armor          ", 100 },
    { SPELL_BLESS,              "bless          ", 200 },
    { SPELL_REMOVE_POISON,      "remove poison  ", 500 },
    { SPELL_CURE_BLIND,         "cure blindness ", 500 },
    { SPELL_CURE_CRITIC,        "critic         ", 700 },
    { SPELL_SANCTUARY,          "sanctuary      ", 1500 },
    { SPELL_HEAL,               "heal           ", 2000 },

    /* The next line must be last, add new spells above. */ 
    { -1, "\r\n", -1 }
  };

  if (CMD_IS("heal")) {

    argument = one_argument(argument, buf);
 
    if (GET_POS(ch) == POS_FIGHTING) return TRUE;
    
    for (i=0; prices[i].number > SPELL_RESERVED_DBC; i++) {
      if (is_abbrev(buf, prices[i].name)) {
	if (GET_GOLD(ch) < ((int)(prices[i].price*(GET_LEVEL(ch)/2)))) {
	  act("$n tells you, 'You don't have enough gold for that spell!'",
	      FALSE, (struct char_data *) me, 0, ch, TO_VICT);
	  return TRUE;
	} else {
            
	  act("$N gives $n some money.",
	      FALSE, (struct char_data *) me, 0, ch, TO_NOTVICT);
	  sprintf(buf, "You give %s %d coins.\r\n", 
		  GET_NAME((struct char_data *) me), (int)(prices[i].price*(GET_LEVEL(ch)/2)));
	  send_to_char(buf, ch);

	  GET_GOLD(ch) -= prices[i].price * (GET_LEVEL(ch)/2);
	  
	  GET_GOLD((struct char_data *) me) += (int)(prices[i].price*(GET_LEVEL(ch)/2)); 
	  
	  cast_spell((struct char_data *) me, ch, NULL, prices[i].number);
	  return TRUE;
          
	}
      }
    }
    act("$n tells you, 'I do not know of that spell!"
	"  Type 'LIST' for a list.'", FALSE, (struct char_data *) me, 
	0, ch, TO_VICT);
    return TRUE;
  }
 
  if (CMD_IS("list")) {
    
    act("$n tells you, 'Here is a listing of the prices for my services.'",
	FALSE, (struct char_data *) me, 0, ch, TO_VICT);
    for (i=0; prices[i].number > SPELL_RESERVED_DBC; i++) {
      sprintf(buf, "%s%d\r\n", prices[i].name, (int)(prices[i].price * (GET_LEVEL(ch)/2)));
      send_to_char(buf, ch);
    }
    return TRUE;
  }
  
  if (cmd) return FALSE;
  
  /* pseudo-randomly choose someone in the room */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (!number(0, 3))
      break;
  
  /* change the level at the end of the next line to control free spells */
  if (vict == NULL || IS_NPC(vict) || (GET_LEVEL(vict) > 10))
    return FALSE;
  
  switch (number(1, GET_LEVEL(vict))) { 
      case 1: cast_spell(ch, vict, NULL, SPELL_CURE_LIGHT); break; 
      case 2: cast_spell(ch, vict, NULL, SPELL_BLESS); break; 
      case 3: cast_spell(ch, vict, NULL, SPELL_ARMOR); break;
      case 4: cast_spell(ch, vict, NULL, SPELL_CURE_LIGHT); break; 
      case 5: cast_spell(ch, vict, NULL, SPELL_BLESS); break; 
      case 6: cast_spell(ch, vict, NULL, SPELL_CURE_CRITIC); break; 
      case 7: cast_spell(ch, vict, NULL, SPELL_ARMOR); break;
      case 8: cast_spell(ch, vict, NULL, SPELL_CURE_CRITIC); break; 
      case 9: cast_spell(ch, vict, NULL, SPELL_ARMOR); break; 
      case 10: 
        /* special wacky thing, your mileage may vary */ 
        act("$n utters the words, 'energizer'.", TRUE, ch, 0, vict, TO_ROOM);
        act("You feel invigorated!", FALSE, ch, 0, vict, TO_VICT);
        GET_MANA(vict) = 
           MIN(GET_MAX_MANA(vict), MAX((GET_MANA(vict) + 10), number(50, 200)));
        break; 
  }
  return TRUE; 
}   

#define ENG_PRICE    GET_LEVEL(ch) * 6000
#define UNENG_PRICE  GET_LEVEL(ch) * 8500
          
SPECIAL(engraver)
{
  struct obj_data *obj;

  if (CMD_IS("engrave")) {

    argument = one_argument(argument, arg);
    
    if (!*arg) {
      send_to_char("Engrave what?\r\n", ch);
      return TRUE;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
      sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
      send_to_char(buf, ch);
      return TRUE; 
    }

    if (GET_OBJ_ENGRAVE(obj) != 0) {
      send_to_char ("Thats already been engraved to someone! Unengrave it first.\r\n", ch);
      return TRUE;
    }

    if (GET_POS(ch) == POS_FIGHTING) return TRUE;
    
    if (GET_GOLD(ch) < ENG_PRICE) {
      act("$n tells you, 'You don't have enough gold to engrave that!'",
	  FALSE, (struct char_data *)me, 0, ch, TO_VICT);
      return TRUE;
    } else {
      
      act("$N gives $n some money.",FALSE, (struct char_data *)me, 0, ch, TO_NOTVICT);
      sprintf(buf, "You give %s %d coins.\r\n", GET_NAME((struct char_data *) me), ENG_PRICE);
      send_to_char(buf, ch);
      
      sprintf(buf, "%s engraves %s.\r\n", 
	      GET_NAME((struct char_data *) me), obj->short_description);
      send_to_char(buf, ch);
      
      GET_GOLD(ch) -= ENG_PRICE;
      GET_GOLD((struct char_data *) me) += ENG_PRICE; 
      GET_OBJ_ENGRAVE(obj) = GET_IDNUM(ch);
      return TRUE;
    }
  }

  if (CMD_IS("list")) {
    
    act("$n tells you, 'Here is a listing of the prices for my services.'",
	FALSE, (struct char_data *) me, 0, ch, TO_VICT);
    sprintf(buf, "\r\n\r\nEngraving     - %d gold coins.\r\nUnengraving   - %d gold coins.\r\n",
	    ENG_PRICE, UNENG_PRICE);
    send_to_char(buf, ch);
    return TRUE;
  }

  if (CMD_IS("unengrave")) {

    argument = one_argument(argument, arg);
     
    if (!*arg) {
      send_to_char("Unengrave what?\r\n", ch);
      return TRUE;
    }

    if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
      sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
      send_to_char(buf, ch);
      return TRUE; 
    }

    if (GET_OBJ_ENGRAVE(obj) == 0) {
      send_to_char ("It doesn't seem to be engraved.\r\n", ch);
      return TRUE;
    }

    if (GET_POS(ch) == POS_FIGHTING) return TRUE;
    
    if (GET_GOLD(ch) < UNENG_PRICE) {
      act("$n tells you, 'You don't have enough gold to unengrave that!'",
	  FALSE, (struct char_data *)me, 0, ch, TO_VICT);
      return TRUE;
    } else {
      
      act("$N gives $n some money.",FALSE, (struct char_data *)me, 0, ch, TO_NOTVICT);
      sprintf(buf, "You give %s %d coins.\r\n", GET_NAME((struct char_data *) me), UNENG_PRICE);
      send_to_char(buf, ch);
      
      sprintf(buf, "%s unengraves %s.\r\n", 
	      GET_NAME((struct char_data *) me), obj->short_description);
      send_to_char(buf, ch);
      
      GET_GOLD(ch) -= UNENG_PRICE;
      GET_GOLD((struct char_data *) me) += UNENG_PRICE; 
      GET_OBJ_ENGRAVE(obj) = 0;
      return TRUE;
    }
  }
  
  if (cmd) return FALSE;
  
  return FALSE;

}             

        
SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return TRUE;
  }

  return FALSE;
}

SPECIAL(cityguard)
{
  struct char_data *tch, *evil;
  int max_evil;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return FALSE;

  max_evil = 1000;
  evil = 0;
    
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_KILLER)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }
    
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (!IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_THIEF)){
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }
    
  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
      if ((GET_ALIGNMENT(tch) < max_evil) &&
	  (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
	max_evil = GET_ALIGNMENT(tch);
	evil = tch;
      }
    }
  }
  
  if (evil && (GET_ALIGNMENT(FIGHTING(evil)) >= 0)) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED);
    return (TRUE);
  }

  return (FALSE);
  
}

#define PET_PRICE(pet) (GET_LEVEL(pet) * 500)

SPECIAL(pet_shops)
{
  char pet_name[256];
  int pet_room;
  struct char_data *pet;

  pet_room = ch->in_room + 1;

  if (CMD_IS("list")) {
    send_to_char("Available pets are:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    argument = one_argument(argument, buf);
    argument = one_argument(argument, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("There is no such pet!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= PET_PRICE(pet);

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      sprintf(buf, "%s %s", pet->player.name, pet_name);
      /* free(pet->player.name); don't free the prototype! */
      pet->player.name = str_dup(buf);

      sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->player.description, pet_name);
      /* free(pet->player.description); don't free the prototype! */
      pet->player.description = str_dup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("May you enjoy your pet.\r\n", ch);
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return 1;
  }
  /* All commands except list and buy */
  return 0;
}





/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */

SPECIAL(coke_machine)
{
 struct obj_data *obj = me, *drink;
 int give_coke = 2; /* Vnum of the drink */

 if (CMD_IS("list"))
   {
     send_to_char("To buy a drink, type 'buy drink'.\r\n", ch);
     return (TRUE);
   }
 
 else if (CMD_IS("buy")) {
   if (GET_GOLD(ch) < 25) {
     send_to_char("You don't have enough gold!\r\n", ch);
     return (TRUE);
   } else {
     drink = read_object(give_coke, VIRTUAL); 
     obj_to_char(drink, ch);
     send_to_char("You insert your money into the machine\r\n",ch);
     GET_GOLD(ch) -= 25; /* coke costs 25 gold */
     act("$n gets a bottle of mineral water from $p.", FALSE, ch, obj, 0, TO_ROOM);
     send_to_char("You get a bottle of mineral water from the machine.\r\n",ch);
   }
   return 1;
 }
 return 0;
}

SPECIAL (portal)
{
  struct obj_data *obj = (struct obj_data *) me;
  struct obj_data *port;
  char obj_name[MAX_STRING_LENGTH];

  if (!CMD_IS("enter")) return FALSE;

  argument = one_argument(argument,obj_name);
  if (!(port = get_obj_in_list_vis(ch, obj_name, world[ch->in_room].contents))) {
    return(FALSE);
  }
    
  if (port != obj)
    return(FALSE);
  if (port->obj_flags.value[1] <= 0 ||
      port->obj_flags.value[1] > 32000) {
    send_to_char("The portal leads nowhere\n\r", ch);
    return TRUE;
  }
  
  act("$n enters $p, and vanishes!", FALSE, ch, port, 0, TO_ROOM);
  act("You enter $p, and you are transported elsewhere", FALSE, ch, port, 0, TO_CHAR);
  char_from_room(ch);  
  char_to_room(ch, port->obj_flags.value[1]);
  look_at_room(ch,0);
  act("$n appears from thin air!", FALSE, ch, 0, 0, TO_ROOM);
  return TRUE;
}

SPECIAL (castleflag)
{
  struct obj_data *flag = NULL;
  char obj_name[MAX_STRING_LENGTH];
  int clan = -1, zone, i;
  struct descriptor_data *pt;

  if (!CMD_IS("nick")) return FALSE;

  if ((clan = is_in_clan(ch)) < 0 ) {
    send_to_char("You aren't even in a clan!\r\n", ch);
    return TRUE;
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    send_to_char("You are immortal, you can't do that!\r\n", ch);
    return TRUE;
  }

  zone = (int)world[ch->in_room].number/100;

  if (!ROOM_FLAGGED(ch->in_room, ROOM_CASTLE)) {
    send_to_char("You aren't in a clan castle.\r\n", ch);
    return TRUE;
  }

  if (zone == clan_control[clan].castlezone) {
    send_to_char("Your clan already controls this castle!\r\n", ch);
    return TRUE;
  }

  argument = one_argument(argument, obj_name);
  if (!(flag = get_obj_in_list_vis(ch, obj_name, world[ch->in_room].contents))) {
    send_to_char("Nick what?\r\n", ch);
    return TRUE;
  }

  if (zone != GET_OBJ_VNUM(flag)/100)
    return FALSE;

  for (i = 0; i < num_of_clans; i++)
    if(clan_control[i].castlezone == zone) {
      clan_control[i].castlezone = -1;
    }

  clan_control[clan].castlezone = zone;

  act("$n grabs $p, a pale aura briefly surrounds $m!", FALSE, ch, flag, 0, TO_ROOM);
  act("You grab $p, you feel a strange feeling rush through your body!", FALSE, ch, flag, 0, TO_CHAR);
     

  for (pt = descriptor_list; pt; pt = pt->next) {
    if (!pt->connected && pt->character)
      if (!TMP_FLAGGED(pt->character, TMP_WRITING | TMP_MAILING | TMP_OLC)) {
	sprintf(buf, "%s[CLAN] Castle %d has been taken over by %s of the clan %s%s\r\n",
		CCYEL(pt->character, C_CMP), zone, GET_NAME(ch), 
		CAP(clan_control[clan].clanname), CCNRM(pt->character, C_CMP));
	send_to_char(buf, pt->character);
      }
  }
  clan_save_control();
  return TRUE;
}


SPECIAL(bank)
{
  int amount;

  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      sprintf(buf, "Your current balance is %d coins.\r\n",
	      GET_BANK_GOLD(ch));
    else
      sprintf(buf, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return 1;
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return 1;
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) -= amount;
    GET_BANK_GOLD(ch) += amount;
    sprintf(buf, "You deposit %d coins.\r\n", amount);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return 1;
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char("You don't have that many coins deposited!\r\n", ch);
      return 1;
    }
    GET_GOLD(ch) += amount;
    GET_BANK_GOLD(ch) -= amount;
    sprintf(buf, "You withdraw %d coins.\r\n", amount);
    send_to_char(buf, ch);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return 1;
  } else
    return 0;
}

SPECIAL(invis_item)
{
  int i;
  struct obj_data *invis_obj = (struct obj_data *)me;

  /* cast the "me" pointer and assign it to invis_obj */
  if (invis_obj->worn_by == ch) {
    /* check to see if the person carrying the invis_obj is the character */
    if(CMD_IS("disappear")) {
      send_to_char("You slowly fade out of view.\r\n", ch);
      act("$n slowly fades out of view.\r\n", FALSE, ch, 0, 0,TO_ROOM);
      SET_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
      return (TRUE);
    }
    
    if (CMD_IS("appear")) {
      REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
      send_to_char("You slowly fade into view.\r\n", ch);
      
      act("$n slowly fades into view.\r\n", FALSE, ch, 0, 0,TO_ROOM);
      return(TRUE);
    }

    one_argument(argument, arg);

    if (is_abbrev(arg, "magical")) {
      for (i=0; i< NUM_WEARS; i++)
	if(GET_EQ(ch,i)){
	  if (IS_SET(AFF_FLAGS(ch), AFF_INVISIBLE)) {
	    
	    REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE);
	    perform_remove(ch,i);
	    send_to_char("You slowly fade into view.\r\n", ch);
	    act("$n slowly fades into view.\r\n", FALSE, ch, 0, 0, TO_ROOM);
	    return (TRUE);
          } else {
	    perform_remove(ch, i);
	    return (TRUE);
          }
	  return (FALSE);
	}
      return (FALSE);
    }
    return (FALSE);
  }
  return (FALSE);
}
