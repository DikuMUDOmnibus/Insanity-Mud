/* ************************************************************************
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "loadrooms.h"

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct cha_app_type cha_app[];
extern struct int_app_type int_app[];
extern struct index_data *obj_index;

ACMD(do_flee);

extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern const char *spells[];
extern const char *item_types[];
extern const char *extra_bits[];
extern const char *apply_types[];
extern const char *affected_bits[];
extern int top_of_world;
extern sh_int r_mortal_start_room[NUM_STARTROOMS+1];
extern sh_int r_death_room;

extern int mini_mud;

extern struct default_mobile_stats *mob_defaults;
extern const char weapon_verbs[];
extern int *max_ac_applys;
extern struct apply_mod_defaults *apmd;

struct time_info_data age(struct char_data * ch);
void name_to_drinkcon(struct obj_data * obj, int type);
void name_from_drinkcon(struct obj_data * obj);
int pk_allowed(struct char_data *ch, struct char_data *vict);
void perform_recall (struct char_data *victim, sh_int room);

/* fire weapons stuff */
extern const char *shot_types[];
extern const int shot_damage[];


/* external functions */
void clearMemory(struct char_data * ch);
void act(const char *str, int i, struct char_data * c, struct obj_data * o,
	 const void *vict_obj, int j);

void damage(struct char_data * ch, struct char_data * victim,
	         int damage, int weapontype);

void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int mag_savingthrow(struct char_data * ch, int type);
int is_zone_closed(room_vnum number);
void death_cry(struct char_data * ch);


/*
 * Special spells appear below.
 */

#define PORTAL 4562
#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_portal)
{
  /* create a magic portal */
  struct obj_data *tmp_obj, *tmp_obj2;
  struct extra_descr_data *ed;
  struct room_data *rp, *nrp;
  struct char_data *tmp_ch = (struct char_data *) victim;
  char buf[512];

  assert(ch);
  assert((level >= 0) && (level <= LVL_IMPL));

   if(FIGHTING(ch)) {
    send_to_char("Finish the fight first!\r\n", ch);
    return;
  } 

  /*
   * check target room for legality.
   */

  rp = &world[ch->in_room];
  tmp_obj = read_object(PORTAL, VIRTUAL);
  if (!rp || !tmp_obj) {
    send_to_char("The magic fails\n\r", ch);
    extract_obj(tmp_obj);
    return;
  }

  if ((IS_SET(rp->room_flags, ROOM_NOMAGIC)) && (GET_LEVEL(ch) < LVL_GOD)){
    send_to_char("Eldritch wizardry obstructs thee.\n\r", ch);
    extract_obj(tmp_obj);
    return;
  }

  if ((SECT(ch->in_room) == SECT_UNDERWATER) ||
      (SECT(ch->in_room) == SECT_WATER_SWIM) ||
      (SECT(ch->in_room) == SECT_WATER_NOSWIM)) {
    send_to_char("You foolishly try to create a portal over water... You Fail!!\r\n", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (IS_SET(rp->room_flags, ROOM_TUNNEL)) {
    send_to_char("There is no room in here to create a portal!\n\r", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (!(nrp = &world[tmp_ch->in_room])) {
    char str[180];
    sprintf(str, "%s not in any room", GET_NAME(tmp_ch));
    log(str);
    send_to_char("The magic cannot locate the target\n", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (GET_LEVEL(tmp_ch) > GET_LEVEL(ch)) {
    send_to_char ("Your magic Fails!\r\n", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
      !PLR_FLAGGED(victim, PLR_KILLER)) {
    sprintf(buf, "%s just tried to portal to you.\r\n"
	    "%s failed because you have summon protection on.\r\n"
	    "Type NOSUMMON to allow other players to portal to you.\r\n",
	    GET_NAME(ch),
	    (ch->player.sex == SEX_MALE) ? "He" : "She");
    send_to_char(buf, victim);

    sprintf(buf, "You failed because %s has summon protection on.\r\n",
	    GET_NAME(victim));
    send_to_char(buf, ch);

    mudlogf(BRF, LVL_WIZARD, TRUE, "%s failed portaling to %s at %s.",
	   GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
    return;
  }


  if (is_zone_closed(world[tmp_ch->in_room].number)) {
    send_to_char ("The gods have decided not to let you do that!\r\n", ch);
    extract_obj(tmp_obj);
    return;
  }

  if (ROOM_FLAGGED(tmp_ch->in_room, ROOM_NOMAGIC)) {
    send_to_char("Your target is protected against your magic.\n\r", ch);
    extract_obj(tmp_obj);
    return;
  }

  /* added by Nahaz to prevent portalling to other types of rooms. */
  if (ROOM_FLAGGED(tmp_ch->in_room, ROOM_PRIVATE) ||
      ROOM_FLAGGED(tmp_ch->in_room, ROOM_DEATH) ||
      ROOM_FLAGGED(tmp_ch->in_room, ROOM_HOUSE)) {
    send_to_char("You cannot cast a portal there.\r\n", ch);
    extract_obj(tmp_obj);
    return;
  }
  
  sprintf(buf, "Through the mists of the portal, you can faintly see %s", nrp->name);

  CREATE(ed , struct extra_descr_data, 1);
  ed->next = tmp_obj->ex_description;
  tmp_obj->ex_description = ed;
  CREATE(ed->keyword, char, strlen(tmp_obj->name) + 1);
  strcpy(ed->keyword, tmp_obj->name);
  ed->description = str_dup(buf);

  tmp_obj->obj_flags.value[0] = level/10;
  tmp_obj->obj_flags.value[1] = tmp_ch->in_room;

  obj_to_room(tmp_obj,ch->in_room);

  act("$p suddenly appears.",TRUE,ch,tmp_obj,0,TO_ROOM);
  act("$p suddenly appears.",TRUE,ch,tmp_obj,0,TO_CHAR);

  /* Portal at other side */

  rp = &world[ch->in_room];
  tmp_obj2 = read_object(PORTAL, VIRTUAL);
  if (!rp || !tmp_obj2) {
    send_to_char("The magic fails\n\r", ch);
    extract_obj(tmp_obj2);
    return;
  }
  sprintf(buf, "Through the mists of the portal, you can faintly see %s", rp->name);

  CREATE(ed , struct extra_descr_data, 1);
  ed->next = tmp_obj2->ex_description;
  tmp_obj2->ex_description = ed;
  CREATE(ed->keyword, char, strlen(tmp_obj2->name) + 1);
  strcpy(ed->keyword, tmp_obj2->name);
  ed->description = str_dup(buf);

  tmp_obj2->obj_flags.value[0] = level/10;
  tmp_obj2->obj_flags.value[1] = ch->in_room;

  obj_to_room(tmp_obj2,tmp_ch->in_room);

  act("$p suddenly appears.",TRUE,tmp_ch,tmp_obj2,0,TO_ROOM);
  act("$p suddenly appears.",TRUE,tmp_ch,tmp_obj2,0,TO_CHAR);

}


ASPELL(spell_relocate)
{
  if (ch == NULL || victim == NULL)
    return;

  if (RIDING(ch)) {
    send_to_char("While mounted I don't think so!\r\n", ch);
    return;
  }

  if(FIGHTING(ch)) {
    send_to_char("Finish the fight first!\r\n", ch);
    return;
  }

  if(GET_LEVEL(victim) >= LVL_IMMORT) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  if (is_zone_closed(world[victim->in_room].number)) {
    send_to_char ("The gods have decided not to let you do that!\r\n", ch);
    return;
  }

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  if (MOB_FLAGGED(victim, MOB_NOSUMMON)) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }
  
  /* added by Nahaz to prevent relocating to other types of rooms. */
  if (ROOM_FLAGGED(victim->in_room, ROOM_PRIVATE) ||
      ROOM_FLAGGED(victim->in_room, ROOM_DEATH) ||
      ROOM_FLAGGED(victim->in_room, ROOM_HOUSE)) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }



  if (!pk_allowed(ch, victim)) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and a rift travels\r\n"
	  "through time and space toward $N, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely close it.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      sprintf(buf, "%s just tried to relocate to you.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to relocate to you.\r\n",
	      GET_NAME(ch),
	      (ch->player.sex == SEX_MALE) ? "He" : "She");
      send_to_char(buf, victim);

      sprintf(buf, "You failed because %s has summon protection on.\r\n",
	      GET_NAME(victim));
      send_to_char(buf, ch);

      mudlogf(BRF, LVL_WIZARD, TRUE, "%s failed relocating to %s at %s.",
	     GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
      return;
    }
  }

  act("$n opens a portal and steps through it.", TRUE, ch, 0, 0, TO_ROOM);
  act("You open a portal and step through.", FALSE, ch, 0, 0, TO_CHAR);
  char_from_room(ch);
  char_to_room(ch, victim->in_room);
  act("A portal opens and $n steps out.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}

ASPELL(spell_fear)
{
  struct char_data *target = (struct char_data *) victim;
  struct char_data *next_target;
  int rooms_to_flee = 0;

  if (ch == NULL)
        return;

  send_to_char("You radiate an aura of fear into the room!\r\n", ch);
  act("$n is surrounded by an aura of fear!", TRUE, ch, 0, 0, TO_ROOM);

  for (target = world[ch->in_room].people; target; target = next_target) {
        next_target = target->next_in_room;

        if (target == NULL)
	       return;

	if (target == ch)
	       continue;

	if (GET_LEVEL(target) >= LVL_IMMORT)
	       continue;

	if (mag_savingthrow(target, 1)) {
	       sprintf(buf, "%s is unaffected by the fear!\r\n", GET_NAME(target));
	       act(buf, TRUE, ch, 0, 0, TO_ROOM);
	       send_to_char("Your victim is not afraid of the likes of you!\r\n", ch);
	       if (IS_NPC(target))
		 hit(target, ch, TYPE_UNDEFINED);
	       }
               else {
		 for(rooms_to_flee = level / 10; rooms_to_flee > 0; rooms_to_flee--) {
		    send_to_char("You flee in terror!\r\n", target);
		    do_flee(target, "", 0, 0);
		 }
	       }
	}
}


ASPELL(spell_disintegrate)
{
   struct obj_data *foolz_objs;
   int save, i;

   if (ch == NULL)
	return;
   if (obj) {
    /* Used on my mud
    if (GET_OBJ_EXTRA(obj) == ITEM_IMMORT && GET_LEVEL(ch) < LVL_IMMORT) {
	send_to_char("Your mortal magic fails.\r\n", ch);
	return;
    }
    */
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_LIGHT:
	save = 19;
	break;
    case ITEM_SCROLL:
	save = 20;
	break;
    case ITEM_STAFF:
    /*
    case ITEM_ROD:
    */
    case ITEM_WAND:
	save = 19;
	break;
    case ITEM_WEAPON:
	save = 18;
	break;
    case ITEM_CLIP:
	save = 20;
	break;
    case ITEM_ARMOR:
	save = 16;
	break;
    case ITEM_WORN:
	save = 18;
	break;
    /*
    case ITEM_SPELLBOOK:
	save = 15;
	break;
    case ITEM_PORTAL:
	save = 13;
	break;
    */
    default:
	save = 19;
	break;
    }
    /*  Save modified by affect on weapons..this is kinda based
     *  on +5 or so being high
     */
    if (GET_OBJ_EXTRA(obj) == ITEM_MAGIC && GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
      for (i = 0; i < MAX_OBJ_AFFECT; i++) {
	  if (obj->affected[i].location == APPLY_DAMROLL)
		save -= obj->affected[i].modifier;
	}
    }
    /* A bonus for ac affecting items also */
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
	if (obj->affected[i].location == APPLY_AC)
   	   save -= obj->affected[i].modifier / 10;
    }
    if (number(1, 20) < save) {
	act("$n disintegrates $p.", FALSE, ch, obj, 0, TO_NOTVICT);
	act("You disintegrate $p.", FALSE, ch, obj, 0, TO_CHAR);
	extract_obj(obj);
	return;
    }
    else {
	act("You fail to disintegrate $p.", FALSE, ch, obj, 0, TO_CHAR);
	return;
    }
   }
   if (victim) {

    if (GET_LEVEL(victim) >= LVL_IMMORT) {
	send_to_char("Nice try..\r\n", ch);
        return;
    }

    /* Note this is extremely powerful, therefore im giving it 2 saves */
    if (mag_savingthrow(victim, 1)) {
	act("You resist $n's attempt to disintegrate you.", FALSE, ch, 0, 0, TO_VICT);
	if (IS_NPC(victim))
	    hit(victim, ch, TYPE_UNDEFINED);
	return;
    }
    if (mag_savingthrow(victim, 1)) {
       	act("You resist $n's attempt to disintegrate you.", FALSE, ch, 0, 0, TO_VICT);
	if (IS_NPC(victim))
            hit(victim, ch, TYPE_UNDEFINED);
	return;
    }
    if (victim->desc) {
	close_socket(victim->desc);
	victim->desc = NULL;
    }

    /* Note this is for disintegrating all items the vict carries also..
       may wish to comment this off if you feel it too powerful */
    for (i = 0; i < NUM_WEARS; i++)
	if (GET_EQ(victim, i))
	  unequip_char(victim, i);
    while (victim->carrying) {
	foolz_objs = victim->carrying;
	extract_obj(foolz_objs);
    }

    extract_char(victim);
    act("$n disintegrates $N!", FALSE, ch, 0, victim, TO_NOTVICT);
    act("You disintegrate $N!", FALSE, ch, 0, victim, TO_CHAR);

    return;
  }
  else
    return;
}

/* end disintigrate */


ASPELL(spell_create_water)
{
  int water;

  if (ch == NULL || obj == NULL)
    return;
  level = MAX(MIN(level, LVL_IMPL), 1);

  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
	if (GET_OBJ_VAL(obj, 1) >= 0)
	  name_from_drinkcon(obj);
	GET_OBJ_VAL(obj, 2) = LIQ_WATER;
	GET_OBJ_VAL(obj, 1) += water;
	name_to_drinkcon(obj, LIQ_WATER);
	weight_change_object(obj, water);
	act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}

ASPELL(spell_recall)
{
  perform_recall (victim, r_mortal_start_room[0]);
}


ASPELL(spell_teleport)
{
  int to_room;

  if(FIGHTING(ch)) {
    send_to_char("Finish the fight first!\r\n", ch);
    return;
  }

  if (RIDING(ch)) {
    send_to_char("While your mounted I don't think so!\r\n", ch);
    return;
  }

  /* this is nasty, players get to die in a teleport accident! heh. */
  
  if(number(1, 30) > 29) {
    send_to_char("Oh Dear....\r\n\r\n"
		 "This doesn't seem right!!!!\r\n"
		 "You feel yourself being torn appart!\r\n"
		 "The Teleport must have gone wrong!\r\n"
		 "Your skin is being torn from your bones!\r\n"
		 "Your flesh decays, you feel the atoms in your body\r\n"
		 "pulling apart!!\r\n\r\n"
		 "Sorry but you seem to have Died!!!!\r\n", ch);
    
    GET_HIT(ch) = 1;
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_STAM(ch) = GET_MAX_STAM(ch);
    GET_POS(ch) = POS_STANDING;
    
    gain_exp(ch, -(GET_EXP(ch) >> 1));
    
    if (FIGHTING(ch))
      stop_fighting(ch);

    while (ch->affected)
      affect_remove(ch, ch->affected);

    death_cry(ch);
    
    mudlogf(CMP, MAX(LVL_OVERSEER, GET_INVIS_LEV(ch)), TRUE, 
	    "%s has just died in a teleport accident!", GET_NAME(ch));

    char_from_room(ch);
    char_to_room(ch, r_death_room);
    return;
  }


  do {
    to_room = number(0, top_of_world);
  } while (ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM | ROOM_IMPL
			| ROOM_HOUSE | ROOM_CLAN | ROOM_CASTLE | ROOM_ARENA) ||
	   (world[to_room].number/100 == 12) ||
	   (world[to_room].number/100 == 4) ||
	   (world[to_room].number/100 == 6) ||
	   (world[to_room].number/100 == 14) ||
	   is_zone_closed(world[to_room].number));

  act("$n slowly fades out of existence and is gone.",
      FALSE, victim, 0, 0, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, to_room);
  act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
  look_at_room(victim, 0);
}

ASPELL(spell_summon)
{
  if (ch == NULL || victim == NULL)
    return;

  if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3)) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  if (!pk_allowed(ch, victim)) {
    if (MOB_FLAGGED(victim, MOB_AGGRESSIVE)) {
      act("As the words escape your lips and $N travels\r\n"
	  "through time and space towards you, you realize that $E is\r\n"
	  "aggressive and might harm you, so you wisely send $M back.",
	  FALSE, ch, 0, victim, TO_CHAR);
      return;
    }
    if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
	!PLR_FLAGGED(victim, PLR_KILLER)) {
      sprintf(buf, "%s just tried to summon you to: %s.\r\n"
	      "%s failed because you have summon protection on.\r\n"
	      "Type NOSUMMON to allow other players to summon you.\r\n",
	      GET_NAME(ch), world[ch->in_room].name,
	      (ch->player.sex == SEX_MALE) ? "He" : "She");
      send_to_char(buf, victim);

      sprintf(buf, "You failed because %s has summon protection on.\r\n",
	      GET_NAME(victim));
      send_to_char(buf, ch);

      mudlogf(BRF, LVL_IMMORT, TRUE, "%s failed summoning %s to %s.",
		GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
      return;
    }
  }

  if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
      (IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL))) {
    send_to_char(SUMMON_FAIL, ch);
    return;
  }

  act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);

  char_from_room(victim);
  char_to_room(victim, ch->in_room);

  act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
  act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
  look_at_room(victim, 0);
}



ASPELL(spell_locate_object)
{
  struct obj_data *i;
  char name[MAX_INPUT_LENGTH];
  int j;

  strcpy(name, fname(obj->name));
  j = level >> 1;

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname(name, i->name))
      continue;

    if (i->carried_by && CAN_SEE(ch, i->carried_by))
      sprintf(buf, "%s is being carried by %s.\n\r",
	      i->short_description, PERS(i->carried_by, ch));
    else if (i->in_room != NOWHERE)
      sprintf(buf, "%s is in %s.\n\r", i->short_description,
	      world[i->in_room].name);
    else if (i->in_obj)
      sprintf(buf, "%s is in %s.\n\r", i->short_description,
	      i->in_obj->short_description);
    else if (i->worn_by && CAN_SEE(ch, i->worn_by))
      sprintf(buf, "%s is being worn by %s.\n\r",
	      i->short_description, PERS(i->worn_by, ch));
    else
      sprintf(buf, "%s's location is uncertain.\n\r",
	      i->short_description);

    CAP(buf);
    send_to_char(buf, ch);
    j--;
  }

  if (j == level >> 1)
    send_to_char("You sense nothing.\n\r", ch);
}



ASPELL(spell_charm)
{
  struct affected_type af;

  if (victim == NULL || ch == NULL)
    return;

  if (victim == ch)
    send_to_char("You like yourself even better!\r\n", ch);
  else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))
    send_to_char("You fail because SUMMON protection is on!\r\n", ch);
  else if (IS_AFFECTED(victim, AFF_SANCTUARY))
    send_to_char("Your victim is protected by sanctuary!\r\n", ch);
  else if (MOB_FLAGGED(victim, MOB_NOCHARM))
    send_to_char("Your victim resists!\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_CHARM))
    send_to_char("You can't have any followers of your own!\r\n", ch);
  else if (IS_AFFECTED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
    send_to_char("You fail.\r\n", ch);
  /* player charming another player - no legal reason for this */
  else if (!IS_NPC(victim))
    send_to_char("You fail - shouldn't be doing it anyway.\r\n", ch);
  else if (circle_follow(victim, ch))
    send_to_char("Sorry, following in circles can not be allowed.\r\n", ch);
  else if (mag_savingthrow(victim, SAVING_PARA))
    send_to_char("Your victim resists!\r\n", ch);
  else {
    if (victim->master)
      stop_follower(victim);

    add_follower(victim, ch);

    af.type = SPELL_CHARM;

    if (GET_INT(victim))
      af.duration = 24 * 18 / GET_INT(victim);
    else
      af.duration = 24 * 18;

    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
    if (IS_NPC(victim)) {
      REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
      REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
    }
  }
}



ASPELL(spell_identify)
{
  int i;
  int found;

  if (obj) {
    send_to_char("You feel informed:\r\n", ch);
    sprintf(buf, "Object '%s', Item type: ", obj->short_description);
    sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
    strcat(buf, buf2);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    if (obj->obj_flags.bitvector) {
      send_to_char("Item will give you following abilities:  ", ch);
      sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
    send_to_char("Item is: ", ch);
    sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
    strcat(buf, "\r\n");
    send_to_char(buf, ch);

    sprintf(buf, "Weight: %d, Value: %d, Extra Attacks: %d, Level: %d\r\n",
	    GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_ATT(obj), GET_OBJ_LEVEL(obj));
    send_to_char(buf, ch);

    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_LIGHT:
    if (GET_OBJ_VAL(obj, 2) == -1)
      strcpy(buf, "Hours left: Infinite");
    else
      sprintf(buf, "Hours left: [%d]", GET_OBJ_VAL(obj, 2));
    break;
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);

      if (GET_OBJ_VAL(obj, 1) >= 1)
	sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(obj, 1)]);
      if (GET_OBJ_VAL(obj, 2) >= 1)
	sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(obj, 2)]);
      if (GET_OBJ_VAL(obj, 3) >= 1)
	sprintf(buf, "%s %s", buf, spells[GET_OBJ_VAL(obj, 3)]);
      sprintf(buf, "%s\r\n", buf);
      send_to_char(buf, ch);
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
      sprintf(buf, "%s %s\r\n", buf, spells[GET_OBJ_VAL(obj, 3)]);
      sprintf(buf, "%sIt has %d maximum charge%s and %d remaining.\r\n", buf,
	      GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
	      GET_OBJ_VAL(obj, 2));
      send_to_char(buf, ch);
      break;
    case ITEM_WEAPON:
      sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
	      GET_OBJ_VAL(obj, 2));
      sprintf(buf, "%s for an average per-round damage of %.1f.\r\n", buf,
	      (((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
      send_to_char(buf, ch);
      break;
    case ITEM_FIREWEAPON:
      sprintf(buf, "Takes %s for damage of %dhp, Current Ammo: %d, Max Ammo: %d\r\n",
	      shot_types[(GET_OBJ_VAL(obj, 0))], shot_damage[(GET_OBJ_VAL(obj, 0))],
	      GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 2));
      break;

    case ITEM_CLIP:
      sprintf(buf, "Item is %s clip for damage of %dhp per round of ammo, Holds: %d\r\n",
	      shot_types[(GET_OBJ_VAL(obj, 0))], shot_damage[(GET_OBJ_VAL(obj, 0))],
	      GET_OBJ_VAL(obj, 3));
      break;

    case ITEM_ARMOR:
      sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
      send_to_char(buf, ch);
      break;
    }
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) &&
	  (obj->affected[i].modifier != 0)) {
	if (!found) {
	  send_to_char("Can affect you as :\r\n", ch);
	  found = TRUE;
	}
	sprinttype(obj->affected[i].location, apply_types, buf2);
	sprintf(buf, "   Affects: %s By %d\r\n", buf2, obj->affected[i].modifier);
	send_to_char(buf, ch);
      }
    }
  } else if (victim) {		/* victim */
    sprintf(buf, "Name: %s\r\n", GET_NAME(victim));
    send_to_char(buf, ch);
    if (!IS_NPC(victim)) {
      sprintf(buf, "%s is %d years, %d months, %d days and %d hours old.\r\n",
	      GET_NAME(victim), age(victim).year, age(victim).month,
	      age(victim).day, age(victim).hours);
      send_to_char(buf, ch);
    }
    sprintf(buf, "Height %d cm, Weight %d pounds\r\n",
	    GET_HEIGHT(victim), GET_WEIGHT(victim));
    sprintf(buf, "%sLevel: %d, Hits: %d, Mana: %d\r\n", buf,
	    GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
    sprintf(buf, "%sAC: %d, Hitroll: %d, Damroll: %d\r\n", buf,
	    GET_AC(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
    sprintf(buf, "%sStr: %d/%d, Int: %d, Wis: %d, Dex: %d, Con: %d, Bio: %d\r\n",
	    buf, GET_STR(victim), GET_ADD(victim), GET_INT(victim),
	GET_WIS(victim), GET_DEX(victim), GET_CON(victim), GET_END(victim));
    send_to_char(buf, ch);

  }
}



ASPELL(spell_enchant_weapon)
{
  int i;

  if (ch == NULL || obj == NULL)
    return;

  if ((GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
      !IS_SET(GET_OBJ_EXTRA(obj), ITEM_MAGIC)) {

    for (i = 0; i < MAX_OBJ_AFFECT; i++)
      if (obj->affected[i].location != APPLY_NONE)
	return;

    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

    obj->affected[0].location = APPLY_HITROLL;
    obj->affected[0].modifier = 1 + (level >= 18);

    obj->affected[1].location = APPLY_DAMROLL;
    obj->affected[1].modifier = 1 + (level >= 20);

    if (IS_GOOD(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_EVIL);
      act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
    } else if (IS_EVIL(ch)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ANTI_GOOD);
      act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}


ASPELL(spell_detect_poison)
{
  if (victim) {
    if (victim == ch) {
      if (IS_AFFECTED(victim, AFF_POISON))
        send_to_char("You can sense poison in your blood.\r\n", ch);
      else
        send_to_char("You feel healthy.\r\n", ch);
    } else {
      if (IS_AFFECTED(victim, AFF_POISON))
        act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
      else
        act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
    }
  }

  if (obj) {
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
    case ITEM_FOOD:
      if (GET_OBJ_VAL(obj, 3))
	act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
      else
	act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
	    TO_CHAR);
      break;
    default:
      send_to_char("You sense that it should not be consumed.\r\n", ch);
    }
  }
}

ASPELL(spell_peace)
{
   struct char_data *temp;

   act("$n tries to stop the fight.", TRUE, ch, 0, 0, TO_ROOM);
   act("You try to stop the fight.", FALSE, ch, 0, 0, TO_CHAR);

   if (IS_EVIL(ch)) {
        return;
        }
   for (temp = world[ch->in_room].people; temp; temp = temp->next_in_room)
       if (FIGHTING(temp)) {
          stop_fighting(temp);
          if (IS_NPC(temp)) {
                clearMemory(temp);
                }
          if (ch != temp) {
                act("$n stops fighting.", TRUE, temp, 0, 0, TO_ROOM);
                act("You stop fighting.", TRUE, temp, 0, 0, TO_CHAR);
                }
          }
   return;
}

