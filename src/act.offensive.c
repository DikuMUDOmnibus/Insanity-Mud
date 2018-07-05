/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern const char *shot_types[];
extern const int shot_damage[];
extern const char *dirs[];
extern const char *spells[];
struct follow_type *k;

/* extern functions */
void raw_kill(struct char_data * ch);
void sportschan(char *);
void check_killer(struct char_data * ch, struct char_data * vict);
int pk_allowed(struct char_data *ch, struct char_data *vict);
ACMD(do_assist);

void improve_skill(struct char_data *ch, int skill) {

  int percent = GET_SKILL(ch, skill);
  int newpercent;
  char skillbuf[MAX_STRING_LENGTH];

  if (number(1, 200) > GET_WIS(ch) + GET_INT(ch))
    return;

  if (percent > 99 || percent <= 0)
     return;

  newpercent = number(1, 5);
  percent += newpercent;

  if (percent > 100)
    percent = 100;

  SET_SKILL(ch, skill, percent);
  if (newpercent >= 4) {
     sprintf(skillbuf, "You feel your skill in %s improving.", spells[skill]);
     send_to_char(skillbuf, ch);
  }
}


ACMD(do_load_weapon)
{
  /* arg1 = fire weapon arg2 = missiles */
  
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  int num_needed = 0;
  int num_ammo = 0;
 
  struct obj_data *missile;
  struct obj_data *weapon = GET_EQ(ch, WEAR_HOLD);	 
  
  two_arguments(argument, arg1, arg2);
  
  if (!*arg1 || !*arg2) {
    send_to_char("Usage loadgun <weapon> <ammo>\r\n\r\n",ch);
    return;
  }
  if (!weapon) {
    send_to_char("You must hold the weapon to load it.",ch);
    return;
  }
  if (GET_OBJ_TYPE(weapon) != ITEM_FIREWEAPON) {
    send_to_char("That item doesn't use ammunition!",ch);
    return;
  }
  
  missile = get_obj_in_list_vis(ch, arg2, ch->carrying);
  if (!missile) {
    send_to_char("What are you trying to use as ammunition?",ch);
    return;
  }

  if (GET_OBJ_TYPE(missile) != ITEM_CLIP) {
    send_to_char("That isn't ammunition!",ch);
    return;
  }
  if (GET_OBJ_VAL(missile,0) != GET_OBJ_VAL(weapon,0)) {
    send_to_char("The ammunition won't fit in the weapon!",ch);
    return;
  }

  num_needed = GET_OBJ_VAL(weapon,2) - GET_OBJ_VAL(weapon,3);
  if (!num_needed) {
    send_to_char("It's already fully loaded.",ch);
    return;
  }

  num_ammo = GET_OBJ_VAL(missile,3);
  if (!num_ammo) {
    /* shouldn't really get here.. this one's for Murphy :) */
    send_to_char("It's empty!",ch);
    extract_obj(missile);
    return;
  }

  if (num_ammo <= num_needed) {
    GET_OBJ_VAL(weapon,3) += num_ammo;
    extract_obj(missile);
  } else {
    GET_OBJ_VAL(weapon,3) += num_needed;
    GET_OBJ_VAL(missile,3) -= num_needed;
  }
  act("You load $p", FALSE, ch, weapon, 0, TO_CHAR);
  act("$n loads $p", FALSE, ch, weapon, 0, TO_ROOM);
}

ACMD(do_fire)
{ 
  /* fire <weapon> <mob> [direction] */
	  
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  char argt[MAX_INPUT_LENGTH];

  struct obj_data *weapon;
  struct char_data *vict;
  
  int target_room = -1;
  int source_room = -1;
  int far_room = 0;
  int num1 = 0;
  int num2 = 0;
  int roll = 0;
  int dmg = 0;
  
  half_chop(argument, arg1, argt);
  half_chop(argt, arg2, arg3);
  
  /* arg1 = weapon arg2= victim arg3= direction */
 
  if (!*arg1 || !*arg2) {
    send_to_char("Usage: FIRE <item> <victim> [direction]\r\n",ch);
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
    return;
  }

  if (!GET_SKILL(ch, SKILL_SHOOT)) {
    send_to_char("You aren't trained in the use of this weapon.\r\n",ch);
    return;
  }

  weapon = GET_EQ(ch, WEAR_HOLD);
  if (!weapon) {
    sprintf(buf2, "You aren't holding %s!\r\n",arg1);
    send_to_char(buf2, ch);
    return;
  }

  if ( (GET_OBJ_TYPE(weapon) != ITEM_FIREWEAPON) ) {  
    send_to_char("You can't fire that!\r\n",ch);
    return;
  }

  if ( !GET_OBJ_VAL(weapon, 3) ) {  
    send_to_char("It isn't loaded with any ammo.\r\n",ch);
    return;
  } 

  if (*arg3) { /* attempt to fire direction x */
    far_room = -1;
    switch (arg3[0]) {
    case 'n':
    case 'N':
      far_room = 0;
      break;
    case 'e':
    case 'E':
      far_room = 1;
      break;
    case 's':
    case 'S':
      far_room = 2;
      break;
    case 'w':
    case 'W':
      far_room = 3;
      break;
    case 'u':
    case 'U':
      far_room = 4;
      break;
    case 'd':
    case 'D':
      far_room = 5;
      break;
    }
    if (far_room == -1 ) {
      send_to_char("Invalid direction given. Valid values are N, E, S, W, U, D.",ch);
      return;
    }
    if (CAN_GO(ch, far_room)) {
      target_room = world[ch->in_room].dir_option[far_room]->to_room;
    }
    if (target_room == -1) {
      send_to_char("You can't find anything worth firing at in that direction.",ch);
      return;
    }
    source_room = ch->in_room;
    ch->in_room = target_room;
    vict = get_char_room_vis(ch, arg2);
    ch->in_room = source_room;
    if (!vict) {
      sprintf(buf, "There doesn't seem to be a %s that way.",arg2);
      send_to_char(buf, ch);
      return;
    }
  } else { 
    vict = get_char_room_vis(ch, arg2);
    if ((!*arg2) || (!vict)) {
      act("Who are you trying to fire $p at?", FALSE, ch, weapon, 0, TO_CHAR);
      return;
    }
  }

  /* ok.. got a loaded weapon, the victim is identified */

  GET_OBJ_VAL(weapon, 3) -= 1;
  
  num1 = GET_DEX(ch) - GET_DEX(vict);
  if (num1 > 25) num1 = 25;
  if (num1 < -25) num1 = -25;
  num2 = GET_SKILL(ch, SKILL_SHOOT);
  num2 = (int)(num2 * .75);
  if (num2 < 1) num2 = 1;
  
  roll = number(1, 101);

  strcpy(argt, shot_types[GET_OBJ_VAL(weapon, 0)]);

	   
  if ( (num1+num2) >= roll ) {

    /* we hit 
       1) print message in room.
       2) print rush message for mob in mob's room.
       3) trans mob
       4) set mob to fighting ch
       */

    improve_skill(ch, SKILL_SHOOT);

    sprintf(buf, "You hit $N with %s fired from $p!", argt);
    act(buf, FALSE, ch, weapon, vict, TO_CHAR);
    sprintf(buf, "$n hit you with %s fired from $p!", argt);
    act(buf, FALSE, ch, weapon, vict, TO_VICT);
    sprintf(buf, "$n hit $N with %s fired from $p!", argt);
    act(buf, FALSE, ch, weapon, vict, TO_NOTVICT);
    act("$n rushes at $N and attacks!",FALSE, vict, 0, ch, TO_NOTVICT);
    act("$n rushes at you and attacks!",FALSE, vict, 0, ch, TO_VICT);
    act("You rush at $N and attack!",FALSE, vict, 0, ch, TO_CHAR);
    char_from_room(vict);
    char_to_room(vict, ch->in_room);
    act("$n rushes at $N and attacks!",FALSE, vict, 0, ch, TO_NOTVICT);
    dmg = shot_damage[GET_OBJ_VAL(weapon, 0)];
    damage(ch, vict, dmg, TYPE_UNDEFINED);

  } else {
  
    sprintf(buf, "You fire %s at $N and miss!", argt);
    act(buf, FALSE, ch, weapon, vict, TO_CHAR);
    sprintf(buf, "$n fires %s at you and misses!", argt);
    act(buf, FALSE, ch, weapon, vict, TO_VICT);
    sprintf(buf, "$n fires %s at $N and misses!", argt);
    act(buf, FALSE, ch, weapon, vict, TO_NOTVICT);
  }
}


ACMD(do_assist)
{
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Whom do you wish to assist?\r\n", ch);
  else if (!(helpee = get_char_room_vis(ch, arg)))
    send_to_char(NOPERSON, ch);
  else if (helpee == ch)
    send_to_char("You can't help yourself any more than this!\r\n", ch);
  else {
    for (opponent = world[ch->in_room].people;
	 opponent && (FIGHTING(opponent) != helpee);
	 opponent = opponent->next_in_room)
		;

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!pk_allowed(ch, opponent) && !IS_NPC(opponent) && 
	     !clan_can_kill(ch, opponent))	/* prevent accidental pkill */
      act("Use 'murder' if you really want to attack $N.", FALSE,
	  ch, 0, opponent, TO_CHAR);
    else {
      send_to_char("You join the fight!\r\n", ch);
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}

ACMD(do_push)
{
   struct char_data *vict;
   static char arg2[MAX_INPUT_LENGTH];
   int mydir;

   half_chop(argument, arg, arg2);
   
   if(GET_SKILL(ch, SKILL_PUSH) <= 0) {
     send_to_char("You have no idea how to push!\r\n", ch);
     return; 
   }

   if (!(vict = get_char_room_vis(ch, arg))) {
     send_to_char("Push who?\r\n", ch);
     return;
   }
   if (vict == ch) {
     send_to_char("Just walk there, idiot.\r\n", ch);
     return;
   }
   if ((mydir = search_block(arg2, dirs, FALSE)) >= 0) {
     if (CAN_GO(vict, mydir)) {
       if ((number(1, 100) + GET_DEX(ch) > number(1, 100) + GET_DEX(vict)) &&
	   (number(1,101) < GET_SKILL(ch, SKILL_PUSH))) {
	 act("You throw your weight against $N and push $M out of the room.", FALSE, ch, 0, vict, TO_CHAR);
	 act("$n throws $s weight against $N and pushes $M out of the room.", FALSE, ch, 0, vict, TO_NOTVICT);
	 act("$n throws $s weight against you and pushes you out of the room.", FALSE, ch, 0, vict, TO_VICT);
	 perform_move(vict, mydir, 1);
	 improve_skill(ch, SKILL_PUSH);
	 return;
       } else {
	 act("$N skillfully sidesteps and you go flying to the ground!", FALSE, ch, 0, vict, TO_CHAR);
	 act("$n tries to push $N, but goes flying to the ground instead.", FALSE, ch, 0, vict, TO_ROOM);
	 act("You skillfully sidestep $n's push, and $e goes tumbling to the ground!", FALSE, ch, 0, vict, TO_VICT);
	 if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
	   hit(vict, ch, TYPE_UNDEFINED);
       }
     }
   }
   send_to_char("Push them which way?\r\n", ch);
   return;
}
/* End push */


ACMD(do_hit)
{
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Hit who?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if (vict == ch) {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (IS_AFFECTED(ch, AFF_CHARM))
    send_to_char("You can not attack, you must assist.\r\n", ch);
  else if (PLR_FLAGGED (ch, PLR_AFK) ) {
    send_to_char("Try removing your AFK flag first", ch);
    SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
  } else {
    if (!pk_allowed(ch, vict) && !clan_can_kill(ch, vict) && 
	!ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) {
      if (!IS_NPC(vict) && !IS_NPC(ch)) {
	if (subcmd != SCMD_MURDER) {
	  send_to_char("Use 'murder' to hit another player.\r\n", ch);
	  return;
	} else {
	  check_killer(ch, vict);
	}
      }
      if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			/* you can't order a charmed pet to attack a
				 * player */
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA)){
      sprintf(buf2, "%s and %s engage in mortal combat", GET_NAME(ch),
	      GET_NAME(vict));
      sportschan(buf2);
    }

    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
      hit(ch, vict, TYPE_UNDEFINED);
      for (k = ch->followers; k; k = k->next) {
	if (PRF_FLAGGED(k->follower, PRF_AUTOASSIST) && 
	    (k->follower->in_room == ch->in_room))
	  do_assist(k->follower, GET_NAME(ch), 0, 0);
      }
      if (ch->master) {
	if (PRF_FLAGGED(ch->master, PRF_AUTOASSIST) && 
	    (ch->master->in_room == ch->in_room))
	  do_assist(ch->master, GET_NAME(ch), 0, 0);
      }
      WAIT_STATE(ch, PULSE_VIOLENCE + 2);
    } else
      send_to_char("You do the best you can!\r\n", ch);
  }
}

ACMD(do_forage)
{
  struct obj_data *item_found = '\0';
  int item_no = 10053; /* Initialize with first item poss. */
  *buf = '\0';

  if(GET_MOVE(ch) < 66) {
    send_to_char("You do not have enough energy right now.\r\n", ch); 
    return; 
  }

  if(SECT(ch->in_room) != SECT_FIELD &&
     SECT(ch->in_room) != SECT_FOREST && 
     SECT(ch->in_room) != SECT_HILLS && 
     SECT(ch->in_room) != SECT_MOUNTAIN) {
    send_to_char("You cannot forage on this type of terrain!\r\n", ch);
    return; 
  }

  if(GET_SKILL(ch, SKILL_FORAGE) <= 0) {
    send_to_char("You have no idea how to forage!\r\n", ch);
    return; 
  }

  send_to_char("You start searching the area for signs of food.\r\n", ch); 
  act("$n starts foraging the area for food.\r\n", FALSE, ch, 0, 0, TO_ROOM);

  if(number(1,101) > GET_SKILL(ch, SKILL_FORAGE)) {
    WAIT_STATE(ch, PULSE_VIOLENCE * 2);
    GET_MOVE(ch) -= (66 - GET_LEVEL(ch)); 
    send_to_char("\r\nYou have no luck finding anything to eat.\r\n", ch);
    return;
  }
  else {
    switch ( number(1,7) ) {
    case 1:
      item_no = 6010; break;  /*<--- Here are the objects you need to code */
    case 2:                   /* Add more or remove some, just change the */
      item_no = 6023; break;  /* switch(number(1, X) */ 
    case 3:
      item_no = 6675; break;
    case 4:
      item_no = 6024; break;
    case 5:
      item_no = 6011; break;
    case 6:
      item_no = 6719; break;
    case 7:
      item_no = 5457; break;
    }
    WAIT_STATE( ch, PULSE_VIOLENCE * 2);  /* Not really necessary */
    improve_skill(ch, SKILL_FORAGE);

    GET_MOVE(ch) -= (76 - GET_LEVEL(ch));
    item_found = read_object( item_no, VIRTUAL);
    obj_to_char(item_found, ch);
    sprintf(buf, "%sYou have found %s!\r\n", buf, item_found->short_description);
    send_to_char(buf, ch);
    act("$n has found something in his forage attempt.\r\n", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }
}

ACMD(do_disarm)
{
  struct obj_data *obj;
  struct char_data *vict;

  one_argument(argument, buf);

  if (!*buf) {
        send_to_char("Whom do you want to disarm?\r\n", ch);
	return;
  }
  else if (!(vict = get_char_room_vis(ch, buf))) {
        send_to_char(NOPERSON, ch);
	return;
  }
  else if (vict == ch) {
        send_to_char("Try removing your weapon instead.\r\n", ch);
	return;
  }
  else if (!pk_allowed(ch, vict) && !IS_NPC(vict) && !IS_NPC(ch) 
	   && !clan_can_kill(ch, vict)) {
    send_to_char("That would be seen as an act of aggression!\r\n", ch);
    return;
  }
  else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict)) {
    send_to_char("The thought of disarming your master seems revolting to you.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("You just don't feel like it, must be this room... very peacefull.\r\n", ch);
    return;
  }
  else if (!(obj = GET_EQ(vict, WEAR_WIELD)))
    act("$N is unarmed!", FALSE, ch, 0, vict, TO_CHAR);
  else if ((GET_OBJ_VAL(obj, 0) == 2) ||
           (number(1, 101) > GET_SKILL(ch, SKILL_DISARM)))
    act("You failed to disarm $N!", FALSE, ch, 0, vict, TO_CHAR);
  else if (dice(2, GET_STR(ch)) + GET_LEVEL(ch) <= dice(2, GET_STR(vict)) + GET_LEVEL(vict)) {
    act("You almost succeed in disarming $N", FALSE, ch, 0, vict, TO_CHAR);
    act("You were almost disarmed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  } else {
    obj_to_room(unequip_char(vict, WEAR_WIELD), vict->in_room);
    act("You succeed in disarming your enemy!", FALSE, ch, 0, 0, TO_CHAR);
    act("Your $p flies from your hands!", FALSE, vict, obj, 0, TO_CHAR);
    act("$n disarms $N, $p drops to the ground.", FALSE, ch, obj, vict, TO_ROOM);
    improve_skill(ch, SKILL_DISARM);
  }
  hit(vict , ch, TYPE_UNDEFINED);
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}


ACMD(do_kill)
{
  struct char_data *vict;

  /* Changed by Nahaz so dear old Arnie can Slay ppl again */
  if ((GET_LEVEL(ch) < LVL_LOWIMPL) || IS_NPC(ch)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Kill who?\r\n", ch);
  } else {
    if (!(vict = get_char_room_vis(ch, arg)))
      send_to_char("They aren't here.\r\n", ch);
    else if (ch == vict)
      send_to_char("Your mother would be so sad.. :(\r\n", ch);
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict);
    }
  }
}



ACMD(do_backstab)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, buf);

  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char("Backstab who?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("How can you sneak up on yourself?\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
    return;
  }
  if (FIGHTING(vict)) {
    send_to_char("You can't backstab a fighting person -- they're too alert!\r\n", ch);
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BACKSTAB);

  if (AWAKE(vict) && (percent > prob)) {
    damage(ch, vict, 0, SKILL_BACKSTAB);
    improve_skill(ch, SKILL_BACKSTAB);
  }
  else
    hit(ch, vict, SKILL_BACKSTAB);
}


ACMD(do_knife)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, buf);

  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char("Knife who?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("Erm you want to knife yourself?\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char("Only piercing weapons can be used for knifing someone.\r\n", ch);
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }

  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_KNIFE);

  if (AWAKE(vict) && (percent > prob)) {
    damage(ch, vict, 0, SKILL_KNIFE);
    improve_skill(ch, SKILL_KNIFE);
  }
  else
    hit(ch, vict, SKILL_KNIFE);
}


ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  int org_room;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char("Order who to do what?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, name)) && !is_abbrev(name, "followers"))
    send_to_char("That person isn't here.\r\n", ch);
  else if (ch == vict)
    send_to_char("You obviously suffer from skitzofrenia.\r\n", ch);

  else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {

      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !IS_AFFECTED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(OK, ch);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      sprintf(buf, "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, vict, TO_ROOM);

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
	if (org_room == k->follower->in_room)
	  if (IS_AFFECTED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(OK, ch);
      else
	send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
    }
  }
}



ACMD(do_flee)
{
  int i, attempt, loss;
  struct char_data *was_fighting;

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char("You are in pretty bad shape, unable to flee!\r\n", ch);
    return;
  }

  if ((IS_NPC(ch) && GET_MOB_WAIT(ch)) || (!IS_NPC(ch) && CHECK_WAIT(ch))) {
    send_to_char("You cannot flee yet!\r\n", ch);
    return;
  }

  was_fighting = FIGHTING(ch);
  for (i = 0; i < 6; i++) {
    attempt = number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      if (do_simple_move(ch, attempt, TRUE)) {
	send_to_char("You flee head over heels.\r\n", ch);
	if (was_fighting && !IS_NPC(ch)) {
	  loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
	  loss *= GET_LEVEL(was_fighting);
	  gain_exp(ch, -loss);
	  sprintf(buf, "You lose %d experience points!\r\n", loss);
	  send_to_char(buf, ch);
	}
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}


ACMD(do_bash)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Bash who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  percent = number(1, 101);	/* 101% is a complete failure */
  prob = GET_SKILL(ch, SKILL_BASH);

  if (MOB_FLAGGED(vict, MOB_NOBASH))
    percent = 101;

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_BASH);
    GET_POS(ch) = POS_SITTING;
  } else {
    damage(ch, vict, 1, SKILL_BASH);
    /*
     * There's a bug in the two lines below.  We could fail bash and they sit.
     */
    GET_POS(vict) = POS_SITTING;
    WAIT_STATE(vict, PULSE_VIOLENCE);
    improve_skill(ch, SKILL_BASH);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}

ACMD(do_strike)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("They are not here to strike!\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Nope better not do that it might hurt!\r\n", ch);
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }


  percent = number(1, 101);	/* 101% is a complete failure */

  if (!(prob = GET_SKILL(ch, SKILL_STRIKE))) {
	send_to_char("You can't, you don't know how!\r\n", ch);
	return;
  }

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_STRIKE);
    GET_POS(ch) = POS_SITTING;
  } else {
    damage(ch, vict, (GET_LEVEL(ch) * 1.5), SKILL_STRIKE);
    GET_POS(vict) = POS_SITTING;
    WAIT_STATE(vict, PULSE_VIOLENCE);
    improve_skill(ch, SKILL_STRIKE);
  }
  WAIT_STATE(ch, PULSE_VIOLENCE * 2);
}


ACMD(do_rescue)
{
  struct char_data *vict, *tmp_ch;
  int percent, prob;

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("Whom do you want to rescue?\r\n", ch);
  else if (vict == ch)
    send_to_char("What about fleeing instead?\r\n", ch);
  else if (FIGHTING(ch) == vict)
    send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
  else {
    for (tmp_ch = world[ch->in_room].people; tmp_ch; tmp_ch = tmp_ch->next_in_room)
      if (FIGHTING(tmp_ch) == vict)
	break;
    
    if (!tmp_ch)
      act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      percent = number(1, 101);       /* 101% is a complete failure */
      prob = GET_SKILL(ch, SKILL_RESCUE);
      
      if (percent > prob) {
	send_to_char("You fail the rescue!\r\n", ch);
	return;
      }
      send_to_char("Banzai!  To the rescue...\r\n", ch);
      act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      
      if (FIGHTING(vict) == tmp_ch)
	stop_fighting(vict);
      if (FIGHTING(tmp_ch))
	stop_fighting(tmp_ch);
      if (FIGHTING(ch))
	stop_fighting(ch);
  
      set_fighting(ch, tmp_ch);
      set_fighting(tmp_ch, ch);
      
      WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
    }
  } 
}

ACMD(do_kick)
{
  struct char_data *vict;
  int percent, prob;

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Kick who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101);	/* 101% is a complete
								 * failure */
  prob = GET_SKILL(ch, SKILL_KICK);

  if (percent > prob) {
    damage(ch, vict, 0, SKILL_KICK);
  } else
    damage(ch, vict, GET_LEVEL(ch) >> 1, SKILL_KICK);
  WAIT_STATE(ch, PULSE_VIOLENCE * 3);
}
