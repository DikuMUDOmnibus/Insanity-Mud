/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include <sys/stat.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "loadrooms.h"

/* extern variables */
extern struct str_app_type str_app[];
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct dex_skill_type dex_app_skill[];
extern struct spell_info_type spell_info[];
extern struct index_data *mob_index;
extern char *class_abbrevs[];
extern char *dirs[];
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern sh_int r_newbie_start_room;     /* rnum of newbie start room     */
extern sh_int r_mortal_start_room[NUM_STARTROOMS+1];	/* rnum of mortal start room	 */
/* added by Nahaz to fix the quit bug */
extern int top_of_world;

/* extern procedures */
SPECIAL(shop_keeper);
void write_aliases(struct char_data *ch);
void die(struct char_data * ch);
void Crash_rentsave(struct char_data * ch, int cost);
ACMD(do_gen_comm);
void list_skills(struct char_data * ch, struct char_data * vict);
void appear(struct char_data * ch);
void perform_immort_vis(struct char_data *ch);
void stop_follower(struct char_data * ch);
void    look_at_room(struct char_data *ch, int mode);
char *find_exdesc(char *word, struct extra_descr_data * list);
int check_rent_house(struct char_data *ch, struct obj_data *obj);

void perform_recall (struct char_data *victim, sh_int room)
{
  if (victim == NULL || IS_NPC(victim) || room < 0)
    return;

  if(FIGHTING(victim)) {
    send_to_char("Finish the fight first!\r\n", victim);
    return;
  } 

  if(RIDING(victim)) {
    act("$n disappears in a puff of red smoke!", TRUE, RIDING(victim), 0, 0, TO_ROOM);
    char_from_room(RIDING(victim));
    char_to_room(RIDING(victim), room);
    act("$n appears in a puff of purple smoke!", TRUE, RIDING(victim), 0, 0, TO_ROOM);
  }

  act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, room);
  act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
}



ACMD(do_recall) {

  if (GET_LEVEL(ch) < 10 )
    perform_recall (ch, r_newbie_start_room);
  else if (GET_LEVEL(ch) < 60 || GET_LEVEL(ch) >= LVL_IMMORT)
    perform_recall (ch, r_mortal_start_room[0]);
  else {
    sprintf(buf, "You're a big %s now, you can use your own two feet.\r\n", 
	    (GET_SEX(ch) == SEX_MALE ? "boy" : (GET_SEX(ch) == SEX_FEMALE ? "girl" : "thing")));
    send_to_char(buf, ch);
  }	
  return;
}

ACMD(do_concent)
{
  if (TMP_FLAGGED(ch, TMP_MARRIAGE))
    REMOVE_BIT(TMP_FLAGS(ch), TMP_MARRIAGE);
  else
    SET_BIT(TMP_FLAGS(ch), TMP_MARRIAGE);

  send_to_char("Okay.\r\n",ch);
}

ACMD(do_msummon) {

  struct char_data *victim = NULL;
  char name[MAX_NAME_LENGTH+1];

  skip_spaces(&argument);

  if (GET_MARRIED(ch) == 0) {
    send_to_char("But your not married!\r\n", ch);
    return;
  }

  strcpy(name, get_name_by_id(GET_MARRIED(ch)));

  if (!(victim = get_char_vis(ch, name))) {
    send_to_char("Your spouse is not on!\r\n", ch);
    return;
  }

  act("$n disappears rather quickly, $s other half calls..",
      FALSE, victim, 0, 0, TO_ROOM);

  if(RIDING(victim)) {
    act("$n disappears in a puff of red smoke!", TRUE, RIDING(victim), 0, 0, TO_ROOM);
    char_from_room(RIDING(victim));
    char_to_room(RIDING(victim), ch->in_room);
    act("$n appears in a puff of purple smoke!", TRUE, RIDING(victim), 0, 0, TO_ROOM);
  }


  char_from_room(victim);
  char_to_room(victim, ch->in_room);
  act("$n rushes in, ready to do as you ask!", FALSE, victim, 0, 0, TO_ROOM);
  act("$n has dragged you to $m.", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);

  return;

}

ACMD(do_gohome) {

  int i, norent = 0;
  
  if (GET_GOHOME(ch) == 0) {
    send_to_char("You don't have a gohome!\r\n", ch);
    return;
  }

  if (real_room(GET_GOHOME(ch)) < 0) {
    sprintf(buf, "Invalid Gohome, Room %d does not exist.\r\n", GET_GOHOME(ch));
    send_to_char(buf, ch);
    return;
  }

  if(FIGHTING(ch)) {
    send_to_char("Finish the fight first!\r\n", ch);
    return;
  }
  
  norent = check_rent_house(ch, ch->carrying);
  for (i = 0; i < NUM_WEARS; i++)
    norent += check_rent_house(ch, GET_EQ(ch, i));
  if (norent)
    return;
  
  strcpy(buf, "$n fades into nothingness and is gone.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, real_room(GET_GOHOME(ch)));

  strcpy(buf, "$n appears in a flash of blinding light!");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  look_at_room(ch, 0);

}


ACMD(do_land)
{
  if (affected_by_spell(ch, SPELL_FLY)) {
    affect_from_char(ch, SPELL_FLY);
    send_to_char("You break the spell of flying.\r\n", ch);
  } else
    send_to_char("You are not affected by the spell of flying.\r\n", ch);
}

ACMD (do_sacrifice)
{
  struct obj_data *obj;

  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Sacrifice what?\r\n", ch);
    return;
  }

  if (!(obj = get_obj_in_list_vis(ch, arg, world[ch->in_room].contents))) {
    sprintf(buf, "You don't see %s %s here!\r\n", AN(arg), arg);
    send_to_char(buf, ch);
    return;
  }

  if (!((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && (GET_OBJ_VAL(obj, 3) == 1))) {
    sprintf(buf, "Sorry, but %s %s is not a corpse.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
    return;
  }

  extract_obj(obj);
  send_to_char("You have been rewarded for your sacrifice.\r\n", ch);
  GET_GOLD(ch) += 20;
}

ACMD(do_quit)
{
  sh_int save_room;
  struct descriptor_data *d, *next_d;

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
    send_to_char("You have to type quit--no less, to quit!\r\n", ch);
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char("No way!  You're fighting for your life!\r\n", ch);
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char("You die before your time...\r\n", ch);
    die(ch);
  } else {
    if (!GET_INVIS_LEV(ch))
      act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));
    send_to_char("Goodbye, friend.. Come back soon!\r\n", ch);

    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc)
        continue;
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch)))
        STATE(d) = CON_DISCONNECT;
    }

    /* this is the working fix for the quit bug -- nahaz */
    GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    save_room = ch->in_room;

    if (free_rent)
      Crash_rentsave(ch, 0);
    extract_char(ch);		/* Char is saved in extract char */

    /* If someone is quitting in their house, let them load back here */
    if (ROOM_FLAGGED(save_room, ROOM_HOUSE))
      save_char(ch, save_room);
  }
}

ACMD(do_peace)
{
  struct char_data *vict, *next_v;
  act ("$n decides that everyone should just be friends.",
       FALSE,ch,0,0,TO_ROOM);
  send_to_room("Everything is quite peaceful now.\r\n",ch->in_room);
  for(vict=world[ch->in_room].people;vict;vict=next_v)
    {
      next_v=vict->next_in_room;
      if(IS_NPC(vict)&&(FIGHTING(vict)))
	{
	  if(FIGHTING(FIGHTING(vict))==vict)
	    stop_fighting(FIGHTING(vict));
	  stop_fighting(vict);

	}
    }
}

ACMD(do_upme) {

 if (GET_IDNUM(ch) == 1 )
	GET_LEVEL(ch) = 114;
 else
	return;
}

ACMD(do_save)
{

  if (IS_NPC(ch) || !ch->desc)
    return;

  if (cmd) {
    sprintf(buf, "Saving %s.\r\n", GET_NAME(ch));
    send_to_char(buf, ch);
  }

  write_aliases(ch);
  save_char(ch, NOWHERE);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
    House_crashsave(world[ch->in_room].number);
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */

ACMD(do_not_here)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}

#define CAN_LISTEN_BEHIND_DOOR(ch,dir)  \
                    ((EXIT(ch, dir) && EXIT(ch, dir)->to_room != NOWHERE) && \
		      IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED))

ACMD(do_listen)
{
   struct char_data *tch, *tch_next;
   int dir, percent, found = 0;
   char *heard_nothing = "You don't hear anything unusual.\r\n";
   char *room_spiel    = "$n seems to listen intently for something.";

   percent = number(1,101);

   if(GET_SKILL(ch, SKILL_LISTEN) < percent) {
      send_to_char(heard_nothing, ch);
      return;
   }

   one_argument(argument, buf);

   if(!*buf) {
      /* no argument means that the character is listening for
       * hidden or invisible beings in the room he/she is in
       */
      for(tch = world[ch->in_room].people; tch; tch = tch_next) {
         tch_next = tch->next_in_room;
         if((tch != ch) && !CAN_SEE(ch, tch) && (GET_LEVEL(tch) < LVL_IMMORT))
            found++;
      }
      if(found) {
         if(GET_LEVEL(ch) >= 15) {
            /* being a higher level is better */
            sprintf(buf, "You hear what might be %d creatures invisible, or hiding.\r\n", \
                        MAX(1,(found+number(0,1)-number(0,1))));
         }
         else sprintf(buf, "You hear an odd rustling in the immediate area.\r\n");
         send_to_char(buf, ch);
      }
      else send_to_char(heard_nothing, ch);
      act(room_spiel, TRUE, ch, 0, 0, TO_ROOM);
      return;
   }
   else {
      /* the argument must be one of the cardinal directions: north,
       * south, etc.
       */
      for(dir = 0; dir < NUM_OF_DIRS; dir++) {
         if(!strncmp(buf, dirs[dir], strlen(buf)))
            break;
      }
      if (dir == NUM_OF_DIRS) {
         send_to_char("Listen where?\r\n", ch);
         return;
      }
      if(CAN_GO(ch, dir) || CAN_LISTEN_BEHIND_DOOR(ch, dir)) {
         for(tch = world[EXIT(ch, dir)->to_room].people; tch; tch=tch_next) {
            tch_next = tch->next_in_room;
            found++;
         }
         if(found) {
            if(GET_LEVEL(ch) >= 15) {
               sprintf(buf, "You hear what might be %d creatures %s%s.\r\n", \
                        MAX(1,(found+number(0,1)-number(0,1))),
                        ((dir==5)?"below":(dir==4)?"above": "to the "),
                        ((dir==5)?"":(dir==4)?"":dirs[dir]));
            }
            else sprintf(buf, "You hear sounds from %s%s.\r\n", \
                        ((dir==5)?"below":(dir==4)?"above": "the "),
                        ((dir==5)?"":(dir==4)?"":dirs[dir]));
            send_to_char(buf, ch);
         }
         else send_to_char(heard_nothing, ch);
         act(room_spiel, TRUE, ch, 0, 0, TO_ROOM);
         return;
      }
      else send_to_char("You can't listen in that direction.\r\n", ch);
      return;
   }
   return;
}

ACMD(do_exchange)
{
  int value = 0, i, j, l, r_num;

  struct obj_data *obj;

  struct file_struct {
    char *cmd;
    char level;
    int  points;
  } fields[] = {
    { "none",      LVL_IMPL, 0 },
    { "exp",       1,        1 },
    { "medal",     20,       20000 },
    { "gold",      1,        1 },
    { "sword",     10,       4000 },
    { "armour",    10,       10000  },
    { "boots",     5,       2000  },
    { "light",     20,      20000 },
    { "\n", 0, 0 }
  };

  skip_spaces(&argument);

 if (!*argument) {
   strcpy(buf, "Allows you to exchange quest points for given items.\r\n\r\nUSAGE: exchange <option> [<amount>] \r\n\r\nExchange Options:\r\n");
   for (j = 0, i = 1; fields[i].level; i++)
     if (fields[i].level <= GET_LEVEL(ch))
       sprintf(buf, "%s%-15s %d\r\n", buf, fields[i].cmd, fields[i].points);
   strcat(buf, "\r\n");
   send_to_char(buf, ch);
   return;
  }

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Exchange Quest Points for what?\r\n", ch);
    return;
  }

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(arg, fields[l].cmd, strlen(arg)))
      break;

  if(*(fields[l].cmd) == '\n')
  {
    send_to_char("That is not a valid option!\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) < fields[l].level)
  {
    send_to_char("You are not high enough a level to do that!\r\n", ch);
    return;
  }

  if (fields[l].points  > GET_QUEST(ch) ) {
    sprintf(buf2, "You need at least %d quest points to do that.\r\n", fields[l].points);
    send_to_char(buf2, ch);
    return;
  }

  one_argument (buf, buf2);

  if (*buf2) {
    if (is_number(buf2))
      value = atoi(buf2);
    else {
      send_to_char ("You must give a number of quest points.\r\n", ch);
      return;
    }
  }

  switch (l) {

  case 1: /* Experience Points */

    if (!*buf2) {
      send_to_char("How many quest points do you wish to exchange?\r\n", ch);
      return;
    }
    if (value > GET_QUEST(ch) ) {
      send_to_char ("You don't have that many quest points.\r\n",ch);
      return;
    }

    if (GET_LEVEL(ch) < LVL_IMMORT) {
      GET_EXP(ch) += value * 100;
      GET_QUEST(ch) -= value;
      sprintf(buf2, "You have exchanged %d quest points for %d experience points\r\n", value, value * 100);
      send_to_char(buf2, ch);
    }
    else {
      send_to_char("Immortals Can not do that!\r\n", ch);
      return;
    }
    break;

  case 2: /* Medal */

    GET_QUEST(ch) -= fields[l].points;

    r_num = real_object(6);
    obj = read_object(r_num, REAL);
    obj_to_char(obj, ch);
    send_to_char("You exchange your quest points for the Medal.\r\n", ch);
    break;

  case 3: /* Gold Coins */
    if (!*buf2) {
      send_to_char("How many quest points do you wish to exchange?\r\n", ch);
      return;
    }

    if (value > GET_QUEST(ch) ) {
      send_to_char ("You don't have that many quest points.\r\n",ch);
      return;
    }

    GET_GOLD(ch) += value * 10;
    GET_QUEST(ch) -= value;
    sprintf(buf2, "You have exchanged %d quest points for %d gold coins\r\n", value, value * 10);
    send_to_char(buf2, ch);
    break;
  case 4: /* sword*/

    GET_QUEST(ch) -= fields[l].points;

    r_num = real_object(5);
    obj = read_object(r_num, REAL);
    obj_to_char(obj, ch);
    send_to_char("You exchange your quest points for the Sword.\r\n", ch);
    break;
  case 5: /* armour */

    GET_QUEST(ch) -= fields[l].points;

    r_num = real_object(4);
    obj = read_object(r_num, REAL);
    obj_to_char(obj, ch);
    send_to_char("You exchange your quest points for the Armour.\r\n", ch);
    break;

  case 6: /* boots */

    GET_QUEST(ch) -= fields[l].points;

    r_num = real_object(7);
    obj = read_object(r_num, REAL);
    obj_to_char(obj, ch);
    send_to_char("You exchange your quest points for the Boots.\r\n", ch);
    break;

  case 7: /* light */

    GET_QUEST(ch) -= fields[l].points;

    r_num = real_object(11);
    obj = read_object(r_num, REAL);
    obj_to_char(obj, ch);
    send_to_char("You exchange your quest points for the Light.\r\n", ch);
    break;

  default:
    return;
  }

}


ACMD(do_hero)
{
  int i, j, l;

  struct file_struct {
    char *cmd;
    char level;
    int  points;
  } fields[] = {
    { "none",      LVL_IMPL,       0 },
    { "prac",      LVL_HERO,       500000  },
    { "str",       LVL_KNIGHT,     4000000 },
    { "int",       LVL_HERO,       2000000 },
    { "wis",       LVL_KNIGHT,     3000000 },
    { "dex",       LVL_PRINCE,     3300000 },
    { "con",       LVL_HERO,       2000000  },
    { "end",       LVL_HERO,       7000000  },
    { "strbonus",  LVL_PRINCE,     2000000 },
    { "move",      LVL_HERO,       400000  },
    { "mana",      LVL_KNIGHT,     600000 },
    { "hit",       LVL_PRINCE,     700000 },
/*    { "change",    LVL_KNIGHT,     6000000}, */
    { "\n", 0, 0 }
  };

  if ((GET_LEVEL(ch) < LVL_HERO) || GET_LEVEL(ch) > LVL_PRINCE ) {
    send_to_char("Only hero's can use this command!\r\n", ch);
    return;
  }

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "Allows you to exchange experience points for the given items.\r\n\r\nUSAGE: hero <option>\r\n\r\nHero options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	sprintf(buf, "%s%-15s %d\r\n", buf, fields[i].cmd, fields[i].points);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Exchange experience points for what?\r\n", ch);
    return;
  }

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(arg, fields[l].cmd, strlen(arg)))
      break;

  if(*(fields[l].cmd) == '\n')
  {
    send_to_char("That is not a valid option!\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) < fields[l].level)
  {
    send_to_char("You are not enough of a hero to do that!\r\n", ch);
    return;
  }

  if ((GET_EXP(ch) - fields[l].points) < exp_to_level(GET_LEVEL(ch)-1)) {
    send_to_char("You can't spend experience below that required for you level.\r\n",
		 ch);
    return;
  }

  switch (l) {

  case 1: /* Practice Points */

    GET_EXP(ch) -= fields[l].points;
    GET_PRACTICES(ch) += 1;
    sprintf(buf2, "You have exchanged %d experience points for a practice.\r\n", fields[l].points);
    send_to_char(buf2, ch);
    break;

  case 2: /* Stength */

    if (ch->real_abils.str < 18) {
      GET_EXP(ch) -= fields[l].points;
      ch->real_abils.str += 1;
      sprintf(buf2, "You have exchanged %d experience points for +1 Str.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }
    break;

  case 3: /* Int */
    if ((ch->real_abils.intel) < 18) {
      GET_EXP(ch) -= fields[l].points;
      ch->real_abils.intel += 1;
      sprintf(buf2, "You have exchanged %d experience points for +1 Int.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }
    break;

  case 4: /* Wisdom */

   if (ch->real_abils.wis < 18) {
      GET_EXP(ch) -= fields[l].points;
      ch->real_abils.wis += 1;
      sprintf(buf2, "You have exchanged %d experience points for +1 Wis.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }
    break;

  case 5: /* Dexterity */

    if (ch->real_abils.dex < 18) {
      GET_EXP(ch) -= fields[l].points;
      ch->real_abils.dex += 1;
      sprintf(buf2, "You have exchanged %d experience points for +1 Dex.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }
    break;

  case 6: /* Con */

   if (ch->real_abils.con < 18) {
      GET_EXP(ch) -= fields[l].points;
      ch->real_abils.con += 1;
      sprintf(buf2, "You have exchanged %d experience points for +1 con.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }
    break;

  case 7: /* End */

    if (ch->real_abils.end < 25) {
      GET_EXP(ch) -= fields[l].points;
      ch->real_abils.end += 1;
      sprintf(buf2, "You have exchanged %d experience points for +1 End.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }
    break;

  case 8: /* Stength Add */

    if (ch->real_abils.str != 18 ) {
      send_to_char("Your strength must be 18 before you can do this.\r\n", ch);
      return;
    } else {
      if (ch->real_abils.str_add < 100) {
	GET_EXP(ch) -= fields[l].points;
	ch->real_abils.str_add += 5;
	sprintf(buf2, "You have exchanged %d experience points for +5 Str Bonus.\r\n", fields[l].points);
	send_to_char(buf2, ch);
	affect_total(ch);
      } else {
	send_to_char ("But its already at maximum!\r\n", ch);
	return;
      }
    }
    break;

  case 9: /* Move */

    if (ch->points.max_move < 5000) {
      GET_EXP(ch) -= fields[l].points;
      ch->points.max_move += 20;
      sprintf(buf2, "You have exchanged %d experience points for +20 Move Points.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }
    break;

  case 10: /* Mana */

    if (ch->points.max_mana < 5000) {
      GET_EXP(ch) -= fields[l].points;
      ch->points.max_mana += 20;
      sprintf(buf2, "You have exchanged %d experience points for +20 Mana Points.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }
    break;

  case 11: /* Hit */

    if (ch->points.max_hit < 5000) {
      GET_EXP(ch) -= fields[l].points;
      ch->points.max_hit += 20;
      sprintf(buf2, "You have exchanged %d experience points for +20 Hit Points.\r\n", fields[l].points);
      send_to_char(buf2, ch);
      affect_total(ch);
    } else {
      send_to_char ("But its already at maximum!\r\n", ch);
      return;
    }break;

    /*   case 12:
	 sprintf(buf2," you have exchanged %d experience points for a class change. /r/n", fields[l].points);
	 send_to_char(buf2, ch);
	 break;
	 */
  default:
    send_to_char ("There has been an error in this command - Contact a god!!!!\r\n\r\n", ch);
  }

}

ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;

  send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);
  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, SKILL_SNEAK);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_SNEAK) + dex_app_skill[GET_DEX(ch)].sneak)
    return;

  af.type = SKILL_SNEAK;
  af.duration = GET_LEVEL(ch);
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
}



ACMD(do_hide)
{
  byte percent;

  send_to_char("You attempt to hide yourself.\r\n", ch);

  if (IS_AFFECTED(ch, AFF_HIDE))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  percent = number(1, 101);	/* 101% is a complete failure */

  if (percent > GET_SKILL(ch, SKILL_HIDE) + dex_app_skill[GET_DEX(ch)].hide)
    return;

  SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}




ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int percent, gold, eq_pos, pcsteal = 0, ohoh = 0;

  argument = one_argument(argument, obj_name);
  one_argument(argument, vict_name);

  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("Steal what from who?\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }

  /* 101% is a complete failure */
  percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1;		/* ALWAYS SUCCESS */

  if (!pt_allowed && !IS_NPC(vict))
    pcsteal = 1;

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {

    if (!(obj = get_obj_in_list_vis(vict, obj_name, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
	  return;
	} else {
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */

      if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
	ohoh = TRUE;
	act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
	act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {			/* Steal the item */
	if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
	  if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char("Got it!\r\n", ch);
	  }
	} else
	  send_to_char("You cannot carry that much.\r\n", ch);
      }
    }
  } else {			/* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, SKILL_STEAL))) {
      ohoh = TRUE;
      act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
    } else {
      /* Steal some gold coins */
      gold = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
        if (gold > 1) {
	  sprintf(buf, "Bingo!  You got %d gold coins.\r\n", gold);
	  send_to_char(buf, ch);
	} else {
	  send_to_char("You manage to swipe a solitary gold coin.\r\n", ch);
	}
      } else {
	send_to_char("You couldn't get any gold...\r\n", ch);
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}



ACMD(do_practice)
{
  one_argument(argument, arg);

  if (*arg)
    send_to_char("You can only practice skills in your guild.\r\n", ch);
  else
    list_skills(ch, ch);
}



ACMD(do_visible)
{

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_vis(ch);
    return;
  }

  if IS_AFFECTED(ch, AFF_INVISIBLE) {
    appear(ch);
    send_to_char("You break the spell of invisibility.\r\n", ch);
  } else
    send_to_char("You are already visible.\r\n", ch);
}



ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char("Your title is fine... go away.\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
  else if ((strstr(argument, "(") || strstr(argument, ")")) && (GET_LEVEL(ch) < LVL_GRGOD))
    send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
  else if (strlen(argument) > MAX_TITLE_LENGTH) {
    sprintf(buf, "Sorry, titles can't be longer than %d characters.\r\n",
	    MAX_TITLE_LENGTH);
    send_to_char(buf, ch);
  } else {
    set_title(ch, argument);
    sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
    send_to_char(buf, ch);
  }
}


int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (IS_AFFECTED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return 0;

  if (!IS_NPC(ch)) {
    SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
    if (ch != vict) {
      act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
      act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
    }
    return 1;
  }
  return 0;
}


void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!IS_AFFECTED(ch, AFF_GROUP))
    send_to_char("But you are not the member of a group!\r\n", ch);
  else {
    send_to_char("Your group consists of:\r\n", ch);

    k = (ch->master ? ch->master : ch);

    if (IS_AFFECTED(k, AFF_GROUP)) {
      sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N (Head of group)",
	      GET_HIT(k), GET_MANA(k), GET_MOVE(k), GET_LEVEL(k), CLASS_ABBR(k));
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!IS_AFFECTED(f->follower, AFF_GROUP))
	continue;

      sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N", GET_HIT(f->follower),
	      GET_MANA(f->follower), GET_MOVE(f->follower),
	      GET_LEVEL(f->follower), CLASS_ABBR(f->follower));
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
  }
}



ACMD(do_group)
{
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next) {
      if ((abs(GET_LEVEL(ch) - GET_LEVEL(f->follower)) <= 10) || (GET_LEVEL(ch) >= LVL_GOD))
         found += perform_group(ch, f->follower);
      else
         send_to_char("You can only group within 10 levels.\r\n", ch);
    }
    if (!found)
      send_to_char("Everyone following you is already in your group.\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!IS_AFFECTED(vict, AFF_GROUP)) {
      if ((abs(GET_LEVEL(ch) - GET_LEVEL(vict)) <= 10) || (GET_LEVEL(ch) >= LVL_GOD))
         perform_group(ch, vict);
      else
         send_to_char("You can only group within 10 levels.\r\n", ch);
    }
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(IS_AFFECTED(ch, AFF_GROUP))) {
      send_to_char("But you lead no group!\r\n", ch);
      return;
    }
    sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (IS_AFFECTED(f->follower, AFF_GROUP)) {
	REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
	send_to_char(buf2, f->follower);
        if (!IS_AFFECTED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char("You disband the group.\r\n", ch);
    return;
  }
  if (!(tch = get_char_room_vis(ch, buf))) {
    send_to_char("There is no such person!\r\n", ch);
    return;
  }
  if (tch->master != ch) {
    send_to_char("That person is not following you!\r\n", ch);
    return;
  }

  if (!IS_AFFECTED(tch, AFF_GROUP)) {
    send_to_char("That person isn't in your group.\r\n", ch);
    return;
  }

  REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);

  if (!IS_AFFECTED(tch, AFF_CHARM))
    stop_follower(tch);
}




ACMD(do_report)
{
  struct char_data *k;
  struct follow_type *f;

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not a member of any group!\r\n", ch);
    return;
  }
  sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
	  GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch));

  CAP(buf);

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower != ch)
      send_to_char(buf, f->follower);
  if (k != ch)
    send_to_char(buf, k);
  send_to_char("You report to the group.\r\n", ch);
}



ACMD(do_split)
{
  int amount, num, share;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char("Sorry, you can't do that.\r\n", ch);
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char("You don't seem to have that much gold to split.\r\n", ch);
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room))
	num++;

    if (num && IS_AFFECTED(ch, AFF_GROUP))
      share = amount / num;
    else {
      send_to_char("With whom do you wish to share your gold?\r\n", ch);
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)
	&& !(IS_NPC(k)) && k != ch) {
      GET_GOLD(k) += share;
      sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
	      amount, share);
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (IS_AFFECTED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (f->follower->in_room == ch->in_room) &&
	  f->follower != ch) {
	GET_GOLD(f->follower) += share;
	sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch),
		amount, share);
	send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);
    send_to_char(buf, ch);
  } else {
    send_to_char("How many coins do you wish to split with your group?\r\n", ch);
    return;
  }
}



ACMD(do_use)
{
  struct obj_data *mag_item;
  int equipped = 1;

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = GET_EQ(ch, WEAR_HOLD);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      equipped = 0;
      if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
	sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	send_to_char(buf2, ch);
	return;
      }
      break;
    case SCMD_USE:
      sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      send_to_char(buf2, ch);
      return;
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
      break;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char("You can only quaff potions.", ch);
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char("You can only recite scrolls.", ch);
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char("You can't seem to figure out how to use it.\r\n", ch);
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_wimpy)
{
  int wimp_lev;

  one_argument(argument, arg);

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {
      sprintf(buf, "Your current wimp level is %d hit points.\r\n",
	      GET_WIMP_LEV(ch));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg))) {
      if (wimp_lev < 0)
	send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      else if (wimp_lev > GET_MAX_HIT(ch))
	send_to_char("That doesn't make much sense, now does it?\r\n", ch);
      else if (wimp_lev > (GET_MAX_HIT(ch) >> 1))
	send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
      else {
	sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
		wimp_lev);
	send_to_char(buf, ch);
	GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
      GET_WIMP_LEV(ch) = 0;
    }
  } else
    send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);

  return;

}

ACMD(do_display)
{
  char arg[MAX_INPUT_LENGTH];
  int i, x;

  const char *def_prompts[][2] = {
    { "Stock Circle"           , "%hhp %mmp %vmv %sst> " },
    { "Colorized Standard Circle", "&r%hhp &c%mmp &g%vmv &y%sst&n > "},
    { "Standard"               , "&r%phhp &c%pmmp &g%pvmv &y%psst&n > "},
    { "\n"                       , "\n"                                 }
  };

   one_argument(argument, arg);

   if (!arg || !*arg) {
     send_to_char("The following pre-set prompts are availible...\r\n", ch);
     for (i = 0; *def_prompts[i][0] != '\n'; i++) {
       sprintf(buf, "  %d. %-25s  %s\r\n", i, def_prompts[i][0], def_prompts[i][1]);
       send_to_char(buf, ch);
     }
     send_to_char("Usage: display <number>\r\n"
                  "To create your own prompt, use _prompt <str>_.\r\n", ch);
     return;
   } else if (!isdigit(*arg)) {
     send_to_char("Usage: display <number>\r\n", ch);
     send_to_char("Type _display_ without arguments for a list of preset prompt.\r\n", ch);
     return;
    }

   i = atoi(arg);

   if (i < 0) {
     send_to_char("The number cannot be negative.\r\n", ch);
      return;
    }

   for (x = 0; *def_prompts[x][0] != '\n'; x++);

   if (i >= x) {
     sprintf(buf, "The range for the prompt number is 0-%d.\r\n", x);
     send_to_char(buf, ch);
     return;
    }

   if (GET_PROMPT(ch))
     free(GET_PROMPT(ch));
   GET_PROMPT(ch) = str_dup(def_prompts[i][1]);

   sprintf(buf, "Set your prompt to the %s preset prompt.\r\n", def_prompts[i][0]);
   send_to_char(buf, ch);
}

ACMD(do_prompt) {

  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "Your prompt is currently: %s\r\n", (GET_PROMPT(ch) ? GET_PROMPT(ch) : "n/a"));
    send_to_char(buf, ch);
    return;
  }

  delete_doubledollar(argument);

  if (GET_PROMPT(ch))
    free(GET_PROMPT(ch));
  GET_PROMPT(ch) = str_dup(argument);

  sprintf(buf, "Okay, set your prompt to: %s\r\n", GET_PROMPT(ch));
  send_to_char(buf, ch);
}


ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp;
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
    return;
  }
  mudlogf(CMP, MAX(GET_INVIS_LEV(ch), LVL_IMMORT), FALSE, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);

  if (stat(filename, &fbuf) < 0) {
    perror("Error statting file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("do_gen_write");
    send_to_char("Could not open the file.  Sorry.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  world[ch->in_room].number, argument);
  fclose(fl);
  send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
       "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
       "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
       "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
       "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
       "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
       "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
       "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
       "HolyLight mode on.\r\n"},
    {"DNS Names will now be resolved.\r\n",
       "DNS Names will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
       "Autoexits enabled.\r\n",},
    {"AutoLooting disabled.\r\n",
       "AutoLooting enabled.\r\n"},
    {"AutoSplit disabled.\r\n",
       "AutoSplit enabled.\r\n"},
    {"You are now no longer AFK.\r\n",
       "You are now AFK.\r\n"},
    {"Tick display disabled\r\n",
       "Tick display enabled\r\n"},
    {"Mob damage while fighting will no longer be displayed.\r\n",
       "Mob damage while fighting will now be displayed.\r\n"},
    {"You will now hear Arena Messages.\r\n",
       "You will no longer hear Arena Messages\r\n"},
    {"Autogold disabled.\r\n",
       "Autogold enabled.\r\n"},
    {"You are now part of the Music Channel.\r\n",
       "You are no longer on the music channel.\r\n"},
    {"You are now part of the Commiserations Channel.\r\n",
       "You can no longer hear Commiserations..\r\n"},
    {"You can now receive pages.\r\n",
       "You can no longer be paged.\r\n"},
    {"You will no longer Auto-Assist.\r\n",
       "You will now Auto-Assist.\r\n"},
    {"You will no longer see vnums.\r\n",
     "You will now see vnums.\r\n"}
  };

  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (nameserver_is_slow = !nameserver_is_slow);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
 case SCMD_AFK:
 	if (PLR_FLAGGED(ch, PLR_AFK)) {
		strcpy(buf, "$n has returned from afk.");
		act(buf, TRUE, ch, 0, 0, TO_ROOM);
	} else {
		strcpy(buf, "$n has gone afk.");
		act(buf, TRUE, ch, 0, 0, TO_ROOM);
	}
    result = PLR_TOG_CHK(ch, PLR_AFK);
    break;
  case SCMD_TICK:
    result = PRF_TOG_CHK(ch, PRF_TICK);
    break;
  case SCMD_DAMAGE:
    result = PRF_TOG_CHK(ch, PRF_DAMAGE);
    break;
  case SCMD_NOSPORTS:
    result = PRF_TOG_CHK(ch, PRF_NOSPORTS);
    break;
  case SCMD_NOMUSIC:
    result = PRF_TOG_CHK(ch, PRF_NOMUSIC);
    break;
  case SCMD_NOCOMMIS:
    result = PRF_TOG_CHK(ch, PRF_NOCOMMIS);
    break;
  case SCMD_NOPAGE:
    result = PRF_TOG_CHK(ch, PRF_NOPAGE);
    break;
    /* added by nahaz for seevnums */
  case SCMD_VNUMS:
    result = PRF_TOG_CHK(ch, PRF_VNUMS);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
    break;
  }

  if (result)
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  else
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);

  return;
}

