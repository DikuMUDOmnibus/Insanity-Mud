/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
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
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "olc.h"

extern int has_boat(struct char_data *ch);
void raw_kill(struct char_data * ch, struct char_data * killer);
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct room_data *world;
extern int max_exp_gain;
extern int max_exp_loss;
extern struct index_data *obj_index;
extern int use_autowiz;
extern int min_wizlist_lev;
extern int free_rent;
void Crash_rentsave(struct char_data *ch, int cost);
void update_char_objects(struct char_data * ch);      /* handler.c */
void extract_obj(struct obj_data * obj);      /* handler.c */

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{

  if (age < 15)
    return (p0);		/* < 15   */
  else if (age <= 29)
    return (int) (p1 + (((age - 15) * (p2 - p1)) / 15));	/* 15..29 */
  else if (age <= 44)
    return (int) (p2 + (((age - 30) * (p3 - p2)) / 15));	/* 30..44 */
  else if (age <= 59)
    return (int) (p3 + (((age - 45) * (p4 - p3)) / 15));	/* 45..59 */
  else if (age <= 79)
    return (int) (p4 + (((age - 60) * (p5 - p4)) / 20));	/* 60..79 */
  else
    return (p6);		/* >= 80 */
}


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(struct char_data * ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = graf(age(ch).year, 4, 8, 12, 16, 12, 10, 8);

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain <<= 1;
      break;
    case POS_RESTING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_SITTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    }
    /*
       if ((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_CLERIC))
       gain <<= 1;
       */
  }

  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 2;

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_REGEN))
    gain += gain << 2;

  return (gain);
}


int hit_gain(struct char_data * ch)
/* Hitpoint gain pr. game hour */
{
  int gain;

  if (IS_NPC(ch)) {
    gain = GET_LEVEL(ch);
    /* Neat and fast */
  } else {

    gain = graf(age(ch).year, 8, 12, 20, 32, 16, 10, 4);

    /* Class/Level calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain >> 3);	/* Divide by 8 */
      break;
    }

    /*    if ((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_CLERIC))
	  gain >>= 1; */
  }
  
  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 2;

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_REGEN))
    gain += gain << 1;
	
  return (gain);
}


int stam_gain(struct char_data * ch)
     /* Stamina point gain pr. game hour */
{
  int gain;
  
  if (IS_NPC(ch)) {
    gain = GET_LEVEL(ch);
    /* Neat and fast */
  } else {
    
    gain = graf(age(ch).year, 8, 15, 25, 40, 10, 4, 2);

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain >> 3);	/* Divide by 8 */
      break;
    }

    /*    if ((GET_CLASS(ch) == CLASS_MAGIC_USER) || (GET_CLASS(ch) == CLASS_CLERIC))
	  gain >>= 1; */
  }
  
  if (IS_AFFECTED(ch, AFF_POISON))
    gain >>= 2;

  if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
    gain >>= 2;

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_REGEN))
    gain += gain << 4;
	
  gain += gain << 2;

  return (gain);
}



int move_gain(struct char_data * ch)
/* move gain pr. game hour */
{
  int gain;

  if (IS_NPC(ch)) {
    return (GET_LEVEL(ch));
    /* Neat and fast */
  } else {
    gain = graf(age(ch).year, 16, 20, 24, 20, 16, 12, 10);


    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain += (gain >> 1);	/* Divide by 2 */
      break;
    case POS_RESTING:
      gain += (gain >> 2);	/* Divide by 4 */
      break;
    case POS_SITTING:
      gain += (gain >> 3);	/* Divide by 8 */
      break;
    }

    if (IS_AFFECTED(ch, AFF_POISON))
      gain >>= 2;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain >>= 2;

    if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_REGEN))
      gain += gain << 3;

    return gain;
  }
}


void set_title(struct char_data * ch, char *title) 
{
  if (title == NULL)
    title = '\0';
  else if (strlen(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  GET_TITLE(ch) = str_dup(title);
}


void check_autowiz(struct char_data * ch)
{
  char buf[100];

  if (use_autowiz && GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
    mudlogf(CMP, LVL_IMMORT, FALSE, "Initiating autowiz.");
    system(buf);
  }
}

void check_autohero(struct char_data * ch)
{
  char buf[100];

  if (GET_LEVEL(ch) >= LVL_HERO && GET_LEVEL(ch) <= LVL_PRINCE) {
    sprintf(buf, "nice ../bin/autohero LVL_HERO %s %d &",
	    HEROLIST_FILE, (int) getpid());
    mudlogf(CMP, LVL_IMMORT, FALSE, "Initiating autohero.");
    system(buf);
  }
}

void check_immort(struct char_data * ch) {

  char buf[100];
  int i;

  if (GET_LEVEL(ch) == LVL_IMMORT) {

    SET_BIT(PRF_FLAGS(ch), PRF_ROOMFLAGS);
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    SET_BIT(PRF_FLAGS(ch), PRF_NOHASSLE);
    SET_BIT(PRF_FLAGS(ch), PRF_LOG1);
    SET_BIT(PRF_FLAGS(ch), PRF_LOG2);


    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;

    mudlogf(BRF, LVL_IMMORT, FALSE, 
	    "%s has become an Immortal - They may need assistance.", GET_NAME(ch));

    send_to_char("Well Done......\r\n"
		 "\r\n"
		 "You are now an Immoral - Welcome to the Heavens!!\r\n"
		 "-- You can not die, or be hurt, you can go anywhere you like with\r\n"
		 "   just a simple thought.\r\n"
		 "-- You no longer play as you did before but now you are part of a team\r\n"
		 "   who makes this MUD run smoothly, you may also design new areas and worlds.\r\n"
		 "-- You can no longer gain levels by killing things this would be pointless,\r\n"
		 "   now you must get promotion from the Creators, building new areas or being\r\n"
		 "   nice to them usually gets you promoted much faster!\r\n"
		 "-- Type Wizhelp to see the immortal commands available to you.\r\n"
		 "-- You may want to ask a God for further help use wiznet to talk to them,\r\n"
		 "   its like gossip except only Immortals and Gods can hear it.\r\n\r\n"
		 "Well Done! Hopefully you will help make this mud a better and bigger place!\r\n", ch);

    sprintf(buf, "----====== Congratulations, %s has become Immortal! =====----\r\n",
	    GET_NAME(ch));

    send_to_most(buf);

  } /* end if they are immortal */
}

void gain_exp(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;
  char buf[128];

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }

  if (gain > 0) {
    gain = MIN(max_exp_gain, gain);	/* put a cap on the max gain per kill */

    if (GET_LEVEL(ch) == LVL_PRINCE) 
      GET_EXP(ch) += MIN(gain, (exp_to_level(GET_LEVEL(ch)) - GET_EXP(ch)));
    else
      GET_EXP(ch) += gain;

    while (GET_LEVEL(ch) < LVL_PRINCE && GET_EXP(ch) >= exp_to_level(GET_LEVEL(ch))) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      if (num_levels == 1) {
        send_to_char("----***** You rise a level! *****----\r\n", ch);
	mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
		"%s has advanced 1 level to level %d", GET_NAME(ch), GET_LEVEL(ch));
	sprintf(buf, "----====== Congratulations %s has gained a level ======----\r\n", GET_NAME(ch));
	send_to_most(buf);
      }
      else {
	sprintf(buf, "----***** You rise %d levels! *****----\r\n", num_levels);
	send_to_char(buf, ch);
	mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
		"%s has advanced %d levels to level %d", GET_NAME(ch), num_levels, GET_LEVEL(ch));
	sprintf(buf, "----====== Congratulations %s has gained %d levels ======----\r\n",
		GET_NAME(ch), num_levels);
	send_to_most(buf);
      }
      check_autohero(ch);
    }
  } else if (gain < 0) {
    gain = MAX(-max_exp_loss, gain);	/* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }
}


void gain_exp_regardless(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  GET_EXP(ch) += MAX(0, gain);

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMPL && GET_EXP(ch) >= exp_to_level(GET_LEVEL(ch))) {
      GET_LEVEL(ch) += 1;
      num_levels++;
      advance_level(ch);
      is_altered = TRUE;
    }

    if (is_altered) {
      if (num_levels == 1) {
	mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
		"%s has advanced 1 level to level %d", GET_NAME(ch), GET_LEVEL(ch));
        send_to_char("You rise a level!\r\n", ch);
      }
      else {
	mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
		"%s has advanced %d levels to level %d", GET_NAME(ch), 
		num_levels, GET_LEVEL(ch));
	sprintf(buf, "You rise %d levels!\r\n", num_levels);
	send_to_char(buf, ch);
      }
      check_immort(ch);
      check_autowiz(ch);
      check_autohero(ch);
    }
  }
}


void gain_condition(struct char_data * ch, int condition, int value)
{
  bool intoxicated;

  if (GET_COND(ch, condition) == -1)	/* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || 
      TMP_FLAGGED(ch, TMP_WRITING) || 
      TMP_FLAGGED(ch, TMP_OLC))
    return;
  
  switch (condition) {
  case FULL:
    send_to_char("You are hungry.\r\n", ch);
    return;
  case THIRST:
    send_to_char("You are thirsty.\r\n", ch);
    return;
  case DRUNK:
    if (intoxicated)
      send_to_char("You are now sober.\r\n", ch);
    return;
  default:
    break;
  }

}


void check_idling(struct char_data * ch)
{
  if (++(ch->char_specials.timer) > 6 && GET_LEVEL(ch) < LVL_GRGOD) {
    if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE) {
      GET_WAS_IN(ch) = ch->in_room;
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
      save_char(ch, NOWHERE);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->char_specials.timer > 16 && GET_LEVEL(ch) < LVL_GRGOD) {
      if (ch->in_room != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc) {
	STATE(ch->desc) = CON_DISCONNECT;
	ch->desc->character = NULL;
      }
      ch->desc = NULL;
      if (free_rent)
	Crash_rentsave(ch, 0);
      else
	Crash_idlesave(ch);

      mudlogf(CMP, LVL_GRGOD, TRUE, "%s force-rented and extracted (idle).", GET_NAME(ch));
      extract_char(ch);
    }
  }
}

/* Update PCs, NPCs, and objects */
void point_update(void)
{
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;

    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);

    if (GET_POS(i) >= POS_STUNNED) {
      GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
      GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
      GET_STAM(i) = MIN(GET_STAM(i) + stam_gain(i), GET_MAX_STAM(i));

      if (IS_AFFECTED(i, AFF_POISON))
	damage(i, i, 2, SPELL_POISON);
      if (GET_POS(i) <= POS_STUNNED)
	update_pos(i);
    } else if (GET_POS(i) == POS_INCAP)
      damage(i, i, 1, TYPE_SUFFERING);
    else if (GET_POS(i) == POS_MORTALLYW)
      damage(i, i, 2, TYPE_SUFFERING);
    if (!IS_NPC(i)) {
      update_char_objects(i);

      if (GET_LEVEL(i) < LVL_IMMORT)
	if (SECT(i->in_room) == SECT_WATER_NOSWIM && !has_boat(i)) {
	  act("$n thrashes about in the water straining to stay afloat.", FALSE,
	      i, 0, 0, TO_ROOM);
	  send_to_char("You are drowning!\r\n", i);
	  GET_MOVE(i) -= MIN(number(6, 30), GET_MOVE(i));
	  GET_HIT(i) -= MIN(number(1, 60), GET_HIT(i));
	  update_pos(i);
	  if (GET_POS(i) == POS_DEAD)
	    raw_kill(i,i);
	}

      if (SECT(i->in_room) == SECT_DESERT && GET_COND(i, THIRST) <= 10) {
	act("$n falls to the floor with exhaustion!!!", FALSE, i, 0, 0, TO_ROOM);
	send_to_char("You fall to the floor with exhaustion!\r\n", i);
	GET_POS(i) = POS_SITTING;
	GET_MOVE(i) -= MIN(number(20, 80), GET_MOVE(i));
	GET_MANA(i) -= MIN(number(5, 40), GET_MANA(i));
	GET_HIT(i) -= MIN(number(1, 60), GET_HIT(i));
	update_pos(i);
	if (GET_POS(i) == POS_DEAD)
	  raw_kill(i,i);
      }

      if (GET_LEVEL(i) < LVL_CREATOR)
	check_idling(i);
    }
  }

  /* objects */
  for (j = object_list; j; j = next_thing) {
    next_thing = j->next;	/* Next in object list */


    /* If this object is in water. */
    if (j->in_room != NOWHERE && (SECT(j->in_room) == SECT_WATER_NOSWIM ||
				  SECT(j->in_room) == SECT_WATER_SWIM)) {
      /* Give everything a random chance of sinking, some may never. */
      if (GET_OBJ_TYPE(j) != ITEM_BOAT && number(0, GET_OBJ_WEIGHT(j)) > 0) {
	act("$p sinks into the murky depths.", FALSE, 0, j, 0, TO_ROOM);
	extract_obj(j);
	continue;
      } else
	act("$p floats unsteadily in the water.", FALSE, 0, j, 0, TO_ROOM);
    }

    /* If this is a corpse */
    if ((GET_OBJ_TYPE(j) == ITEM_CONTAINER) && GET_OBJ_VAL(j, 3)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
        GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {

        if (j->carried_by)
          act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
        else if ((j->in_room != NOWHERE) && (world[j->in_room].people)) {
          act("A quivering horde of maggots consumes $p.",
              TRUE, world[j->in_room].people, j, 0, TO_ROOM);
          act("A quivering horde of maggots consumes $p.",
              TRUE, world[j->in_room].people, j, 0, TO_CHAR);
        }
        for (jj = j->contains; jj; jj = next_thing2) {
          next_thing2 = jj->next_content;       /* Next in inventory */
          obj_from_obj(jj);

          if (j->in_obj)
            obj_to_obj(jj, j->in_obj);
          else if (j->carried_by)
            obj_to_room(jj, j->carried_by->in_room);
          else if (j->in_room != NOWHERE)
            obj_to_room(jj, j->in_room);
          else
            assert(FALSE);
        }
	extract_obj(j);
      }
    }

    /* make portals dissapear */
    if (GET_OBJ_VNUM(j) == 4562)
      {
	if (GET_OBJ_VAL(j,0) > 0)
	  GET_OBJ_VAL(j,0)--;
	if (GET_OBJ_VAL(j,0) == 1)
	  {
	    if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
	      act("$p starts to fade!",
		  FALSE, world[j->in_room].people, j, 0, TO_ROOM);
	      act("$p starts to fade!",
		  FALSE, world[j->in_room].people, j, 0, TO_CHAR);
	    }
	  }
	if (GET_OBJ_VAL(j,0) == 0)
	  {
	    if ((j->in_room != NOWHERE) &&(world[j->in_room].people)) {
	      act("$p vanishes in a cloud of smoke!",
		  FALSE, world[j->in_room].people, j, 0, TO_ROOM);
	      act("$p vanishes in a cloud of smoke!",
		  FALSE, world[j->in_room].people, j, 0, TO_CHAR);
	    }
	    extract_obj(j);
	  }
      }
  }
}


