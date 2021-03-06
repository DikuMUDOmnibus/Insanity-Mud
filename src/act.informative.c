/*************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
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
#include "screen.h"
#include "clan.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern struct command_info cmd_info[];
extern struct clan_control_rec clan_control[MAX_CLANS];
extern struct time_info_data time_info;
extern struct help_index_element *help_table;
extern int top_of_helpt;

extern const char *weekdays[];
extern const char *month_name[];
extern char *help;
extern const char *connected_types[];
extern char circlemud_version[];
extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *herolist;
extern char *policies;
extern char *handbook;
extern char *newbie;
extern char *olcrules;
extern const char *dirs[];
extern const char *where[];
extern const char *color_liquid[];
extern const char *fullness[];
extern const char *connected_types[];
extern const char *class_abbrevs[];
extern const char *room_bits[];
extern const char *spells[];
extern struct spell_info_type spell_info[];
extern const char *pc_class_types[];
extern const char *sector_types[];
extern const char *affected_bits[];
extern const  char *apply_types[];
extern const char *languages[];

/* extern functions */
struct time_info_data real_time_passed(time_t t2, time_t t1);
ACMD(do_action);
long find_class_bitvector(char arg);

void show_obj_to_char(struct obj_data * object, struct char_data * ch,
			int mode)
{
  bool found;
  int vnum;
  char name[MAX_NAME_LENGTH+1];
  vnum = GET_OBJ_VNUM(object);

  *buf = '\0';
  /* alterations here by Nahaz to enable seevnums */
  if ((mode == 0) && object->description)
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_VNUMS)) {
      sprintf(buf, "(#%d) ", vnum);
      strcat(buf, object->description);
    }
    else
      strcpy(buf, object->description);
  else if (object->short_description && ((mode == 1) || (mode == 2) || 
					 (mode == 3) || (mode == 4))) {
    if ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_VNUMS)) && (mode==1)) {
      sprintf(buf, "(#%d) ", vnum);
      strcat(buf, object->short_description);
    }
    else
      strcpy(buf, object->short_description);
  }
  else if (mode == 5) {
    if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
      if (object->action_description) {
	strcpy(buf, "There is something written upon it:\r\n\r\n");
	strcat(buf, object->action_description);
	page_string(ch->desc, buf, 1);
      } else
	act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    } else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
      strcpy(buf, "You see nothing special..");
    } else			/* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
      strcpy(buf, "It looks like a drink container.");
  }
  if (mode != 3) {
    found = FALSE;

    if (GET_OBJ_ENGRAVE(object)) {
      strcpy(name, (get_name_by_id((GET_OBJ_ENGRAVE(object)))));
      sprintf(buf, "%s ..belonging to %s.", buf, CAP(name));
      found = TRUE;
    }

    if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
      strcat(buf, " (invisible)");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_BLESS) && IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
      strcat(buf, " ..It glows blue!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_MAGIC) && IS_AFFECTED(ch, AFF_DETECT_MAGIC)) {
      strcat(buf, " ..It glows yellow!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_GLOW)) {
      strcat(buf, " ..It has a soft glowing aura!");
      found = TRUE;
    }
    if (IS_OBJ_STAT(object, ITEM_HUM)) {
      strcat(buf, " ..It emits a faint humming sound!");
      found = TRUE;
    }
  }
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}


void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode,
                           bool show)
{
  struct obj_data *i, *j;
/*  char buf[10]; */
  bool found;
  int num;

  found = FALSE;
  for (i = list; i; i = i->next_content) {
      num = 0;
      for (j = list; j != i; j = j->next_content)
        if (j->item_number==NOTHING) {
            if(strcmp(j->short_description,i->short_description)==0) break;
        }
        else if (j->item_number==i->item_number) break;
      if (j!=i) continue;
      for (j = i; j; j = j->next_content)
        if (j->item_number==NOTHING) {
            if(strcmp(j->short_description,i->short_description)==0) num++;
          }
        else if (j->item_number==i->item_number) num++;
     if (CAN_SEE_OBJ(ch, i)) {
          if (num!=1) {
                sprintf(buf,"(%2ix) ",num);
                send_to_char(buf,ch);
            }
          show_obj_to_char(i, ch, mode);
          found = TRUE;
      }
  }
  if (!found && show)
    send_to_char(" Nothing.\r\n", ch);
}

void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int percent;

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = -1;		/* How could MAX_HIT be < 1?? */

  strcpy(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100)
    strcat(buf, " is in excellent condition.\r\n");
  else if (percent >= 90)
    strcat(buf, " has a few scratches.\r\n");
  else if (percent >= 75)
    strcat(buf, " has some small wounds and bruises.\r\n");
  else if (percent >= 50)
    strcat(buf, " has quite a few wounds.\r\n");
  else if (percent >= 30)
    strcat(buf, " has some big nasty wounds and scratches.\r\n");
  else if (percent >= 15)
    strcat(buf, " looks pretty hurt.\r\n");
  else if (percent >= 0)
    strcat(buf, " is in awful condition.\r\n");
  else
    strcat(buf, " is bleeding awfully from big wounds.\r\n");

  send_to_char(buf, ch);
}


void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (i->player.description)
    send_to_char(i->player.description, ch);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  diag_char_to_char(i, ch);

  if (RIDING(i) && RIDING(i)->in_room == i->in_room) {
    if (RIDING(i) == ch)
      act("$e is mounted on you.", FALSE, i, 0, ch, TO_VICT);
    else {
      sprintf(buf2, "$e is mounted upon %s.", PERS(RIDING(i), ch));
      act(buf2, FALSE, i, 0, ch, TO_VICT);
    }
  } else if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room) {
    if (RIDDEN_BY(i) == ch)
      act("You are mounted upon $m.", FALSE, i, 0, ch, TO_VICT);
    else {
      sprintf(buf2, "$e is mounted by %s.", PERS(RIDDEN_BY(i), ch));
      act(buf2, FALSE, i, 0, ch, TO_VICT);
    }
  }

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  if (found) {
    act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
	send_to_char(where[j], ch);
	show_obj_to_char(GET_EQ(i, j), ch, 1);
      }
  }
  if (ch != i && (GET_LEVEL(ch) >= LVL_IMMORT)) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 20) < GET_LEVEL(ch))) {
	show_obj_to_char(tmp_obj, ch, 1);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char("You can't see anything.\r\n", ch);
  }
}


void list_all_char(struct char_data * i, struct char_data * ch, int num)
{
  const char *positions[] = {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is lying here, stunned.",
    " is sleeping here.",
    " is resting here.",
    " is sitting here.",
    "!FIGHTING!",
    " is standing here."
  };

  if (IS_NPC(i) && i->player.long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
    /* this added by Nahaz for seevnums */
    if(PRF_FLAGGED(ch, PRF_VNUMS)) {
      int vnum=-1;
      vnum=GET_MOB_VNUM(i);
      sprintf(buf2, "(#%d) ", vnum);
      strcpy(buf, buf2);
    }
    else
      *buf = '\0';

    if (IS_AFFECTED(i, AFF_INVISIBLE))
      strcpy(buf, "*");

    if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i))
	strcat(buf, "(Red Aura) ");
      else if (IS_GOOD(i))
	strcat(buf, "(Blue Aura) ");
    }
    strcat(buf, i->player.long_descr);

    if (num > 1) {
      while ((buf[strlen(buf)-1]=='\r') ||
	     (buf[strlen(buf)-1]=='\n') ||
	     (buf[strlen(buf)-1]==' ')) {
	buf[strlen(buf)-1] = '\0';
      }
      sprintf(buf2,"(%dx) %s\r\n", num, buf);
      send_to_char(buf2, ch);
    } else {
    	send_to_char(buf, ch);
    }

    if (IS_AFFECTED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_DOOM))
      act("...$e is surrounded by a grey aura!", FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (IS_AFFECTED(i, AFF_GLOW))
      act("...$e is glowing a bright orange colour!", FALSE, i, 0, ch, TO_VICT);
    
    return;
  }
  if (IS_NPC(i)) {
    strcpy(buf, i->player.short_descr);
    CAP(buf);
  } else
    sprintf(buf, "%s", i->player.name);

  if (IS_AFFECTED(i, AFF_INVISIBLE))
    strcat(buf, " (invisible)");
  if (IS_AFFECTED(i, AFF_HIDE))
    strcat(buf, " (hidden)");
  if (!IS_NPC(i) && !i->desc)
    strcat(buf, " (linkless)");
  if (TMP_FLAGGED(i, TMP_WRITING))
    strcat(buf, " (writing)");
  if (TMP_FLAGGED(i, TMP_OLC))
    strcat(buf, " (building)");
  if (PLR_FLAGGED(i, PLR_AFK))
    strcat (buf, " (AFK)");

  if (RIDING(i) && RIDING(i)->in_room == i->in_room) {
    strcat(buf, " is here, mounted upon ");
    if (RIDING(i) == ch)
      strcat(buf, "you");
    else
      strcat(buf, PERS(RIDING(i), ch));
    strcat(buf, ".");
  } else if (GET_POS(i) != POS_FIGHTING)
    strcat(buf, positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      strcat(buf, " is here, fighting ");
      if (FIGHTING(i) == ch)
	strcat(buf, "YOU!");
      else {
	if (i->in_room == FIGHTING(i)->in_room)
	  strcat(buf, PERS(FIGHTING(i), ch));
	else
	  strcat(buf, "someone who has already left");
	strcat(buf, "!");
      }
    } else			/* NIL fighting pointer */
      strcat(buf, " is here struggling with thin air.");
  }

  if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      strcat(buf, " (Red Aura)");
    else if (IS_GOOD(i))
      strcat(buf, " (Blue Aura)");
  }
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  if (IS_AFFECTED(i, AFF_SANCTUARY))
    act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
}



void list_char_to_char(struct char_data *list, struct char_data *ch)
{
  struct char_data *i, *plr_list[100];
  int num, counter, locate_list[100], found=FALSE;
  num = 0;

  for (i=list; i; i = i->next_in_room) {
    if(i != ch)  {
      if (CAN_SEE(ch, i)) {
	if (num< 100) {

	  found = FALSE;

	  for (counter=0;(counter<num&& !found);counter++) {
	    if (i->nr == plr_list[counter]->nr &&
		(GET_POS(i) == GET_POS(plr_list[counter])) &&
		(AFF_FLAGS(i)==AFF_FLAGS(plr_list[counter])) &&
		(FIGHTING(i) == FIGHTING(plr_list[counter])) &&
		!strcmp(GET_NAME(i), GET_NAME(plr_list[counter]))) {
	      locate_list[counter] += 1;
	      found=TRUE;
	    }
	  }
	  if (!found) {
	    plr_list[num] = i;
	    locate_list[num] = 1;
	    num++;
	  }
	} else {
	  list_all_char(i,ch,0);
	}
      }
    }
  }
  
  if (num) {
    for (counter=0; counter<num; counter++) {
      if (locate_list[counter] > 1) {
	list_all_char(plr_list[counter],ch,locate_list[counter]);
      } else {
	list_all_char(plr_list[counter],ch,0);
      }
    }
  }
}

void do_auto_exits(struct char_data *ch)
{
  int door;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      if (GET_LEVEL(ch) >= LVL_IMMORT)
	sprintf(buf2, "%-5s - [%5d] %s\r\n", dirs[door],
		world[EXIT(ch, door)->to_room].number,
		world[EXIT(ch, door)->to_room].name);
      else {
	sprintf(buf2, "%-5s - ", dirs[door]);
	if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
	  strcat(buf2, "Too dark to tell\r\n");
	else {
	  strcat(buf2, world[EXIT(ch, door)->to_room].name);
	  strcat(buf2, "\r\n");
	}
      }
      strcat(buf, CAP(buf2));
    }
  send_to_char("Obvious exits:\r\n", ch);

  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);

}

ACMD(do_whois)
{
  struct char_data *victim = 0;
  struct char_file_u tmp_store;

  skip_spaces(&argument);

  if (!*argument) {
      send_to_char("Do a WhoIS on which player?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(argument, &tmp_store) > -1) {
        store_to_char(&tmp_store, victim);

	sprintf(buf, "&y%s is:&n\r\n\r\n&c%s %s&n\r\nLevel: &g%d&n, Age: &g%d&n years old.\r\n",
		GET_NAME(victim), GET_NAME(victim), GET_TITLE(victim), 
		GET_LEVEL(victim), GET_AGE(victim));
	
	sprinttype(victim->player.class, pc_class_types, buf1);

	sprintf(buf, "%sClass: &m%s&n, Hometown: &m%s&n\r\n", buf, buf1, 
		languages[GET_CLASS(victim)+1]);
	sprintf(buf, "%sP-Kills: &c%d&n, Death by mobs: &c%d&n, Death by Players: &c%d&n\r\n",
		buf, GET_PKKILLS(victim), GET_MOBDEATH(victim), GET_PKDEATH(victim));

	if ((GET_LEVEL(victim) < LVL_OVERSEER) || 
	    (GET_LEVEL(ch) >= LVL_OVERSEER && GET_LEVEL(victim) <= GET_LEVEL(ch))) {
	  
	  sprintf (buf, "%sLast Logged on: &g%s&n\r\n",
		   buf,  ctime(&tmp_store.last_logon));
	}
	
        send_to_char(buf, ch);
      } else {
        send_to_char("There is no such player.\r\n", ch);
      } 
      free(victim);
    } 
}


ACMD(do_affects)
{
  struct affected_type *af;
  int type=1;
  char sname[256];
  
  send_to_char("You carry these affections: \r\n", ch);

  for (af = ch->affected; af; af = af->next) {
    strcpy(sname, spells[af->type]);
    strcat(sname, ":");
    if (GET_LEVEL(ch) >= 20) {
      sprintf(buf, "   %s%-22s%s    affects %s%s%s by %s%d%s for %s%d%s hours\r\n", 
	      CCCYN(ch, C_NRM), (type ? sname : ""), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM),
	      ((GET_LEVEL(ch) >= 50  
		&& af->location) ? apply_types[(int) af->location] : "Something"),
	      CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), af->modifier, CCNRM(ch, C_NRM),
	      CCCYN(ch, C_NRM), af->duration, CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
      if (GET_LEVEL(ch) >= 80 && 
	  (af->bitvector && 
	   (!af->next || af->next->bitvector != af->bitvector))) {
	sprintbit(af->bitvector, affected_bits, buf2);
	sprintf(buf1, "%35s adds %s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
	send_to_char(buf1, ch);
      }
    }
    else if (type){
      sprintf(buf, "   %s%-25s%s\r\n", CCCYN(ch, C_NRM), sname, CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
    }
    type = af->next ? (af->next->type != af->type) : 1;
  }
}

ACMD(do_skills) {

  char buf[MAX_STRING_LENGTH];
  int class = CLASS_UNDEFINED, i, col = 1, j = 0;

  class = GET_CLASS(ch);

  sprintf(buf, "The following %s skills/spells are availible to"
	  " mortals:\r\n\r\n", pc_class_types[class]);

  for (j = 1; j < LVL_HERO; j++) {
    for (i = 1; i < LANG_COMMON; i++) {

      if (spell_info[i].min_level[class] != j)
	continue;

      if (spell_info[i].min_level[class] >= LVL_HERO)
	continue;

      if (strlen(buf) >= MAX_STRING_LENGTH-32) {
	strcat(buf, "**OVERFLOW**\r\n");
	break;
      }

      sprintf(buf2, "%3d %-30s ", spell_info[i].min_level[class], spells[i]);
      col++;
      strcat(buf, buf2);
      if (col > 2 ) {
        col = 1;
        sprintf(buf, "%s\r\n",buf);
      }
    }
  }

  if (col % 3)
    strcat(buf, "\r\n");

  page_string(ch->desc, buf, 1);

}

ACMD(do_exits)
{
  int door;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      if (GET_LEVEL(ch) >= LVL_IMMORT)
	sprintf(buf2, "%-5s - [%5d] %s\r\n", dirs[door],
		world[EXIT(ch, door)->to_room].number,
		world[EXIT(ch, door)->to_room].name);
      else {
	sprintf(buf2, "%-5s - ", dirs[door]);
	if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch))
	  strcat(buf2, "Too dark to tell\r\n");
	else {
	  strcat(buf2, world[EXIT(ch, door)->to_room].name);
	  strcat(buf2, "\r\n");
	}
      }
      strcat(buf, CAP(buf2));
    }
  send_to_char("Obvious exits:\r\n", ch);

  if (*buf)
    send_to_char(buf, ch);
  else
    send_to_char(" None.\r\n", ch);
}



void look_at_room(struct char_data * ch, int ignore_brief)
{

  char *blood_messages[] = {
    "Should never see this.",
    "There's a little blood here.",
    "You're standing in some blood.",
    "The blood in this room is flowing.",
    "There's so much blood here it's intoxicating!",
    "How much more blood can there be in any one room?",
    "Such carnage. The God of Death is feastin' tonight!",
    "You are splashing around in the blood of the slain!",
    "Even the walls are revolted by the death and destruction!",
    "The Gods should show some mercy and cleanse this horrid room!",
    "So much blood has been shed here, you are drowning in it!",
    "\n"
  };

  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You see nothing but infinite darkness...\r\n", ch);
    return;
  }

  send_to_char(CCCYN(ch, C_NRM), ch);
  if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    sprintbit((long) ROOM_FLAGS(ch->in_room), room_bits, buf);
    sprinttype(SECT(ch->in_room), sector_types, buf1);

    sprintf(buf2, "[%5d] %s [ %s] [ %s ]", world[ch->in_room].number,
	    world[ch->in_room].name, buf, buf1);
    send_to_char(buf2, ch);
  } else
    send_to_char(world[ch->in_room].name, ch);

  send_to_char(CCNRM(ch, C_NRM), ch);
  send_to_char("\r\n",ch);

  if (!PRF_FLAGGED(ch, PRF_BRIEF) || ignore_brief ||
      ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
    send_to_char(world[ch->in_room].description, ch);

  if (RM_BLOOD(ch->in_room) > 0)
    act(blood_messages[RM_BLOOD(ch->in_room)], FALSE, ch, 0, 0, TO_CHAR);


  send_to_char("You can see:\r\n", ch);

  /* now list characters & objects */
  send_to_char(CCGRN(ch, C_NRM), ch);
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
  send_to_char(CCYEL(ch, C_NRM), ch);
  list_char_to_char(world[ch->in_room].people, ch);
  send_to_char(CCNRM(ch, C_NRM), ch);
  send_to_char("\n", ch);

  /* autoexits */
  if (PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(ch);
}

void look_in_direction(struct char_data * ch, int dir)
{
  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(EXIT(ch, dir)->general_description, ch);
    else
      send_to_char("You see nothing special.\r\n", ch);

    if (IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    } else if (IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) && EXIT(ch, dir)->keyword) {
      sprintf(buf, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
      send_to_char(buf, ch);
    }
  } else
    send_to_char("You don't see anything important...\r\n", ch);
}



void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char("Look in what?\r\n", ch);
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
				 FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)) {

    if ( (GET_OBJ_TYPE(obj) == ITEM_FIREWEAPON) ||
	(GET_OBJ_TYPE(obj) == ITEM_CLIP) ) {
      sprintf(buf, "It contains %d units of ammunition.\r\n", GET_OBJ_VAL(obj,3));
      send_to_char(buf,ch);
    } else {
      send_to_char("There's nothing inside that!\r\n", ch);
    }
  } else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER) {
      if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED))
	send_to_char("It is closed.\r\n", ch);
      else {
	send_to_char(fname(obj->name), ch);
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(" (carried): \r\n", ch);
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(" (here): \r\n", ch);
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(" (used): \r\n", ch);
	  break;
	}

	list_obj_to_char(obj->contains, ch, 2, TRUE);
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0)
	send_to_char("It is empty.\r\n", ch);
      else {
	if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0)) {
	  sprintf(buf, "Its contents seem somewhat murky.\r\n"); /* BUG */
	} else {
	  amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
	  sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
	  sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
	}
	send_to_char(buf, ch);
      }
    }
  }
}



char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return NULL;
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */

void look_at_target(struct char_data * ch, char *arg)
{
  int bits, found = 0, j;
  struct char_data *found_char = NULL;
  struct obj_data *obj = NULL, *found_obj = NULL;
  char *desc;

  if (!*arg) {
    send_to_char("Look at what?\r\n", ch);
    return;
  }
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch))
	act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }
  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL) {
    page_string(ch->desc, desc, 0);
    return;
  }
  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[ch->in_room].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
	if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
	send_to_char(desc, ch);
	found = 1;
      }
  if (bits) {			/* If an object was found back in
				 * generic_find */
    if (!found)
      show_obj_to_char(found_obj, ch, 5);	/* Show no-description */
    else
      show_obj_to_char(found_obj, ch, 6);	/* Find hum, glow etc */
  } else if (!found)
    send_to_char("You do not see that here.\r\n", ch);
}


ACMD(do_look)
{
  static char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char("You can't see anything but stars!\r\n", ch);
  else if (IS_AFFECTED(ch, AFF_BLIND))
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
  else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char("It is pitch black...\r\n", ch);
    list_char_to_char(world[ch->in_room].people, ch);	/* glowing red eyes */
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg)
	send_to_char("Read what?\r\n", ch);
      else
	look_at_target(ch, arg);
      return;
    }
    if (!*arg)			/* "look" alone, without an argument at all */
      look_at_room(ch, 1);
    else if (is_abbrev(arg, "in"))
      look_in_obj(ch, arg2);
    /* did the char type 'look <direction>?' */
    else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
      look_in_direction(ch, look_type);
    else if (is_abbrev(arg, "at"))
      look_at_target(ch, arg2);
    else
      look_at_target(ch, arg);
  }
}



ACMD(do_examine)
{
  int bits;
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }
  look_at_target(ch, arg);

  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char("When you look inside, you see:\r\n", ch);
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char("You're broke!\r\n", ch);
  else if (GET_GOLD(ch) == 1)
    send_to_char("You have one miserable little gold coin.\r\n", ch);
  else {
    sprintf(buf, "You have %d gold coins.\r\n", GET_GOLD(ch));
    send_to_char(buf, ch);
  }
}

/* altered by Nahaz to allow for show score */
void show_score_to_char(struct char_data * ch, struct char_data * recip)
{
  struct time_info_data playing_time;

  const char *align[] = {
    "Daemonic",
    "Chaotic",
    "Very Evil",
    "Evil",
    "Evil Neutral",
    "Neutral",
    "Good Neutral",
    "Good",
    "Very Good",
    "Lawfull",
    "Saintly"
  };

  sprintf(buf, "\r\n%s%s %s%s\r\n", CCCYN(ch, C_CMP), GET_NAME(ch), GET_TITLE(ch), CCWHT(ch, C_CMP));
  send_to_char(buf, recip);
  send_to_char ("--------------------------------------------------------------------------\r\n", recip);

  sprintf(buf, " Hit: %s%d%s(%s%d%s)  Mana: %s%d%s(%s%d%s)  Move: %s%d%s(%s%d%s)  Stamina: %s%d%s(%s%d%s)\r\n",
	  CCGRN(ch, C_CMP), GET_HIT(ch), CCWHT(ch, C_CMP),
	  CCRED(ch, C_CMP), GET_MAX_HIT(ch), CCWHT(ch, C_CMP),
	  CCGRN(ch, C_CMP), GET_MANA(ch), CCWHT(ch, C_CMP),
	  CCRED(ch, C_CMP), GET_MAX_MANA(ch), CCWHT(ch, C_CMP),
	  CCGRN(ch, C_CMP), GET_MOVE(ch), CCWHT(ch, C_CMP),
	  CCRED(ch, C_CMP), GET_MAX_MOVE(ch), CCWHT(ch, C_CMP),
	  CCGRN(ch, C_CMP), GET_STAM(ch), CCWHT(ch, C_CMP),
	  CCRED(ch, C_CMP), GET_MAX_STAM(ch), CCWHT(ch, C_CMP));

  send_to_char(buf, recip);
  send_to_char ("--------------------------------------------------------------------------\r\n", recip);

  sprinttype(ch->player.class, pc_class_types, buf1);
  sprintf(buf, "|  %sClass:  %s%-10s%s \\ /     %sStr:%s%4d%s/%s%-7d%s\\ /   %sAlign: %s%-14s%s|\r\n",
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), buf1, CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_STR(ch), CCWHT(ch, C_CMP),
	  CCMAG(ch, C_CMP), GET_ADD(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), align[(int)(GET_ALIGNMENT(ch)/200)+5],
	  CCWHT(ch, C_CMP));

  sprintf(buf, "%s|  %sLevel:  %s%-11d%s |      %sDex:  %s%-11d%s|   %sArmour: %s%-14d%s|\r\n", buf,
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_LEVEL(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_DEX(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_AC(ch), CCWHT(ch, C_CMP));

  sprintf(buf, "%s|  %s  Age:  %s%-11d%s |      %sCon:  %s%-11d%s| %s Hitroll: %s%-14d%s|\r\n", buf,
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_AGE(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_CON(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_HITROLL(ch), CCWHT(ch, C_CMP));

  sprintf(buf, "%s|  %sQuest:  %s%-11d%s |      %sInt:  %s%-11d%s|  %sDamroll: %s%-14d%s|\r\n", buf,
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_QUEST(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_INT(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_DAMROLL(ch), CCWHT(ch, C_CMP));

  sprintf(buf, "%s| %s   Exp:  %s%-11d%s |      %sWis:  %s%-11d%s|     %sGold: %s%-14d%s|\r\n", buf,
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_EXP(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_WIS(ch), CCWHT(ch, C_CMP),
	  CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_GOLD(ch), CCWHT(ch, C_CMP));

  if (GET_LEVEL(ch) < LVL_IMMORT)
    sprintf(buf, "%s| %sTo Lev:  %s%-10d%s / \\     %sEnd:  %s%-10d%s/ \\                        |\r\n", buf,
	    CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), (exp_to_level(GET_LEVEL(ch)) - GET_EXP(ch)), CCWHT(ch, C_CMP),
	    CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_END(ch), CCWHT(ch, C_CMP));
  else
    sprintf(buf, "%s| %sTo Lev:  %sImmortal  %s / \\     %sEnd:  %s%-10d%s/ \\                        |\r\n", buf,
	    CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), CCWHT(ch, C_CMP),
	    CCYEL(ch, C_CMP), CCMAG(ch, C_CMP), GET_END(ch), CCWHT(ch, C_CMP));

  send_to_char(buf,recip);

  send_to_char("--------------------------------------------------------------------------\r\n", recip);
  playing_time = real_time_passed((time(0) - ch->player.time.logon) +
                                  ch->player.time.played, 0);
  sprintf(buf, "You have been playing for %s%d%s days and %s%d%s hours.\r\n",
	  CCRED(ch, C_CMP), playing_time.day, CCWHT(ch, C_CMP),
	  CCRED(ch, C_CMP), playing_time.hours, CCWHT(ch, C_CMP));
  send_to_char(buf, recip);

  sprintf (buf, "Killed %s%d%s times, by mobs: %s%d%s, by players: %s%d%s. PKills: %s%d%s.\r\n",
	   CCYEL(ch, C_CMP), GET_PKDEATH(ch) + GET_MOBDEATH(ch), CCWHT(ch, C_CMP),
	   CCYEL(ch, C_CMP), GET_MOBDEATH(ch),  CCWHT(ch, C_CMP),
	   CCYEL(ch, C_CMP), GET_PKDEATH(ch), CCWHT(ch, C_CMP),
	   CCYEL(ch, C_CMP), GET_PKKILLS(ch),  CCWHT(ch, C_CMP));
  send_to_char(buf, recip);

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "Poofin:  %s %s\r\n", GET_NAME(ch),
	    (POOFIN(ch) ? POOFIN(ch) : "appears in a puff of purple smoke!"));
    sprintf(buf, "%sPoofout: %s %s\r\n", buf, GET_NAME(ch),
	    (POOFOUT(ch) ? POOFOUT(ch) : "disappears in a ball of fire!"));
    send_to_char(buf, recip);
    send_to_char("--------------------------------------------------------------------------\r\n", recip);
  }

  send_to_char (CCNRM(ch, C_CMP), recip);

  switch (GET_POS(ch)) {
  case POS_DEAD:
    strcpy(buf, "You are DEAD!\r\n");
    break;
  case POS_MORTALLYW:
    strcpy(buf, "You are mortally wounded!  You should seek help!\r\n");
    break;
  case POS_INCAP:
    strcpy(buf, "You are incapacitated, slowly fading away...\r\n");
    break;
  case POS_STUNNED:
    strcpy(buf, "You are stunned!  You can't move!\r\n");
    break;
  case POS_SLEEPING:
    strcpy(buf, "You are sleeping.\r\n");
    break;
  case POS_RESTING:
    strcpy(buf, "You are resting.\r\n");
    break;
  case POS_SITTING:
    strcpy(buf, "You are sitting.\r\n");
    break;
  case POS_FIGHTING:
    if (FIGHTING(ch))
      sprintf(buf, "You are fighting %s.\r\n", PERS(FIGHTING(ch), ch));
    else
      strcpy(buf, "You are fighting thin air.\r\n");
    break;
  case POS_STANDING:
    strcpy(buf, "You are standing.\r\n");
    break;
  default:
    strcpy(buf, "You are floating.\r\n");
    break;
  }

  if (GET_COND(ch, DRUNK) > 10)
    strcat(buf, "You are intoxicated.\r\n");

  if (GET_COND(ch, FULL) == 0)
    strcat(buf, "You are hungry.\r\n");

  if (GET_COND(ch, THIRST) == 0)
    strcat(buf, "You are thirsty.\r\n");

  if (IS_AFFECTED(ch, AFF_BLIND))
    strcat(buf, "You have been blinded!\r\n");

  if (IS_AFFECTED(ch, AFF_INVISIBLE))
    strcat(buf, "You are invisible.\r\n");

  if (IS_AFFECTED(ch, AFF_HASTE))
    strcat(buf, "You seem to be moving quickly.\r\n");

  if (affected_by_spell(ch, SPELL_STONESKIN))
    strcat(buf, "You have stone skin.\r\n");

  if (IS_AFFECTED(ch, AFF_FLYING))
    strcat(buf, "You are flying\r\n");
  
  if (IS_AFFECTED(ch, AFF_WATERBREATH))
    strcat(buf, "You are able to breath underwater.\r\n");
  
  if (IS_AFFECTED(ch, AFF_DETECT_INVIS))
    strcat(buf, "You are sensitive to the presence of invisible things.\r\n");

  if (IS_AFFECTED(ch, AFF_SANCTUARY))
    strcat(buf, "You are protected by Sanctuary.\r\n");

  if (IS_AFFECTED(ch, AFF_POISON))
    strcat(buf, "You are poisoned!\r\n");

  if (IS_AFFECTED(ch, AFF_CHARM))
    strcat(buf, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_ARMOR))
    strcat(buf, "You feel protected.\r\n");

  if (affected_by_spell(ch, SPELL_SHIELD))
    strcat(buf, "You are protected by a shield.\r\n");

  if (affected_by_spell(ch, SPELL_BARKSKIN))
    strcat(buf, "You have bark skin.\r\n");

  if (IS_AFFECTED(ch, AFF_INFRAVISION))
    strcat(buf, "Your eyes are glowing red.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    strcat(buf, "You are summonable by other players.\r\n");

  send_to_char(buf, recip);
}

ACMD(do_score)
{
  show_score_to_char(ch, ch);
}

ACMD(do_inventory)
{
  send_to_char("You are carrying:\r\n", ch);
  list_obj_to_char(ch->carrying, ch, 1, TRUE);
}


ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char("You are using:\r\n", ch);
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
	send_to_char(where[i], ch);
	show_obj_to_char(GET_EQ(ch, i), ch, 1);
	found = TRUE;
      } else {
	send_to_char(where[i], ch);
	send_to_char("Something.\r\n", ch);
	found = TRUE;
      }
    }
  }
  if (!found) {
    send_to_char(" Nothing.\r\n", ch);
  }
}


ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  sprintf(buf, "It is %d o'clock %s, on ",
	  ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
	  ((time_info.hours >= 12) ? "pm" : "am"));

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  day = time_info.day + 1;	/* day in [1..35] */

  if (day == 1)
    suf = "st";
  else if (day == 2)
    suf = "nd";
  else if (day == 3)
    suf = "rd";
  else if (day < 20)
    suf = "th";
  else if ((day % 10) == 1)
    suf = "st";
  else if ((day % 10) == 2)
    suf = "nd";
  else if ((day % 10) == 3)
    suf = "rd";
  else
    suf = "th";

  sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[(int) time_info.month], time_info.year);

  send_to_char(buf, ch);
}


ACMD(do_weather)
{

  extern struct zone_data *zone_table;

  static const char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"};

  if (OUTSIDE(ch)) {
    sprintf(buf, "The sky is %s and %s.\r\n",
            sky_look[zone_table[world[ch->in_room].zone].sky],
	    (zone_table[world[ch->in_room].zone].change >= 0 ?
             "you feel a warm wind from south" :
	     "your foot tells you bad weather is due"));
    send_to_char(buf, ch);
  } else
    send_to_char("You have no feeling about the weather at all.\r\n", ch);
}


ACMD(do_help)
{
  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table) {
    send_to_char("No help available.\r\n", ch);
    return;
  }

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      send_to_char("There is no help on that word.\r\n", ch);
      mudlogf(NRM, MAX(GET_INVIS_LEV(ch), LVL_IMMORT), TRUE, "HELP: %s attempted to get help on %s", GET_NAME(ch), argument);
      return;
    } else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen))) {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
	mid--;
      page_string(ch->desc, help_table[mid].entry, 0);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}

struct char_data *theWhoList[100];

int compareChars(const void *l, const void *r) {

  struct char_data **left;
  struct char_data **right;

  left = (struct char_data **)l;
  right = (struct char_data **)r;

  if(GET_LEVEL(*left) < GET_LEVEL(*right))
    return 1;
  else if(GET_LEVEL(*left) == GET_LEVEL(*right))
    return 0;
  else
    return -1;
}

#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-c classlist] [-i] [-m] [-o] [-q] [-r] [-z]\r\n"
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *wch;
  char name_search[MAX_INPUT_LENGTH];
  char mode;
  size_t i;
  int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0;
  int showclass = 0, outlaws = 0, num_can_see = 0;
  int who_room = 0;
  int clan;
  int noElements = 0;
  int curEl;
  char name[MAX_NAME_LENGTH+1];


#define MAX_WHOTITLES  8

  const char *whotitles[MAX_WHOTITLES] =
    {
      "These people are Insane.\r\n~~~~~~~~~~~~~~~~~~~~~~~\r\n",
      "These are the people with no lives.\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n",
      "The following people have lost their minds!\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n",
      "Here are some people with nothing better to do.\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n",
      "Only the Insane are visible!\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n",
      "The Insane who are hiding from Reality.\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n",
      "These people are completely bonkers!!\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n",
      "The Few, The Chosen, The Insane.\r\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n"
    };


  const char *godlevs[LVL_IMPL - (LVL_IMMORT-1)] = {
    " HELPER ",
    "OVERSEER",
    "GUARDIAN",
    "OVERLORD",
    " WIZARD ",
    "ARCH-WIZ",
    "LESR GOD",
    "GRTR GOD",
    "SUPR GOD",
    "SUB-IMPL",
    "  IMPL  ",
    "SNR IMPL"
  };

  const char *herolevs[] = {
    "  Hero  ",
    " Knight ",
    " Prince "
  };

  skip_spaces(&argument);
  strcpy(buf, argument);
  name_search[0] = '\0';

  while (*buf) {
    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);
    } else if (*arg == '-') {
      mode = *(arg + 1);        /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
        outlaws = 1;
        strcpy(buf, buf1);
        break;
      case 'z':
        localwho = 1;
        strcpy(buf, buf1);
        break;
      case 'q':
        questwho = 1;
        strcpy(buf, buf1);
        break;
      case 'l':
        half_chop(buf1, arg, buf);
        sscanf(arg, "%d-%d", &low, &high);
        break;
      case 'n':
        half_chop(buf1, name_search, buf);
        break;
      case 'r':
        who_room = 1;
        strcpy(buf, buf1);
        break;
      case 'c':
        half_chop(buf1, arg, buf);
        for (i = 0; i < strlen(arg); i++)
          showclass |= find_class_bitvector(arg[i]);
        break;
      default:
        send_to_char(WHO_FORMAT, ch);
        return;
        break;
      }                         /* end of swiwch */

    } else {                    /* endif */
      send_to_char(WHO_FORMAT, ch);
      return;
    }
  }                             /* end while (parser) */

  for (noElements = 0, d = descriptor_list; d; d = d->next) {
    if (d->connected != CON_REDIT &&
        d->connected != CON_MEDIT &&
        d->connected != CON_ZEDIT &&
        d->connected != CON_OEDIT &&
        d->connected != CON_SEDIT &&
        d->connected != CON_PLAYING &&
        d->connected != CON_TEXTED)
      continue;

    if (d->original)
      wch = d->original;
    else if (!(wch = d->character))
      continue;

    theWhoList[noElements++] = wch;
  }

  /* Sort it using the built in libc-quicksort routine */
  qsort(theWhoList, noElements, sizeof(struct char_data  *), compareChars);

  sprintf(buf, "%s%s%s", CCRED(ch, C_NRM), whotitles[number(0, MAX_WHOTITLES-1)],
	  CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  for(curEl = 0; curEl < noElements; curEl++) {
    wch = theWhoList[curEl];

    if (*name_search && str_cmp(GET_NAME(wch), name_search) &&
	!strstr(GET_TITLE(wch), name_search))
      continue;
    if (!CAN_SEE(ch, wch) || GET_LEVEL(wch) < low || GET_LEVEL(wch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(wch, PLR_KILLER) && !PLR_FLAGGED(wch, PLR_THIEF)
	&& !PLR_FLAGGED(wch, PLR_CRIMINAL))
      continue;
    if (questwho && !PRF_FLAGGED(wch, PRF_QUEST))
      continue;
    if (localwho && world[ch->in_room].zone != world[wch->in_room].zone)
      continue;
    if (who_room && (wch->in_room != ch->in_room))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(wch))))
      continue;

    num_can_see++;

    if (GET_LEVEL(wch) >= LVL_IMMORT) {
      sprintf(buf, "%s[%s] %s %s%s",
	      CCGRN(ch, C_SPR), godlevs[GET_LEVEL(wch)-LVL_IMMORT],
	      GET_NAME(wch), GET_TITLE(wch), CCNRM(ch, C_SPR));
    } else if (GET_LEVEL(wch) >= LVL_HERO) {
      sprintf(buf, "%s[%s] %s%s%s%s %s%s",
	      CCMAG(ch, C_SPR), herolevs[GET_LEVEL(wch)-LVL_HERO],
	      CCRED(ch, C_SPR), (PLR_FLAGGED(wch, PLR_PK) ? "[PK] " :""), 
	      CCMAG(ch, C_SPR), GET_NAME(wch), GET_TITLE(wch), CCNRM(ch, C_SPR));
    } else if(GET_LEVEL(wch) < 3) {
      sprintf(buf, "%s[ Newbie ] %s %s%s",
	      CCYEL(ch, C_SPR), GET_NAME(wch),
				GET_TITLE(wch), CCNRM(ch, C_SPR));
    } else {
      sprintf(buf, "[%3d  %s ] %s%s%s%s %s",
	      GET_LEVEL(wch), CLASS_ABBR(wch), CCRED(ch, C_SPR),
	      (PLR_FLAGGED(wch, PLR_PK) ? "[PK] " : ""), CCNRM(ch, C_SPR),
	      GET_NAME(wch), GET_TITLE(wch));
    }
    
    if (GET_INVIS_LEV(wch))
      sprintf(buf, "%s (i%d)", buf, GET_INVIS_LEV(wch));
    else if (IS_AFFECTED(wch, AFF_INVISIBLE))
      strcat(buf, " (invis)");
    
    if (TMP_FLAGGED(wch, TMP_MAILING))
      strcat(buf, " (mailing)");
    else if (TMP_FLAGGED(wch, TMP_WRITING))
      strcat(buf, " (writing)");
    else if (TMP_FLAGGED(wch, TMP_OLC))
      strcat(buf, " (building)");
    
    if (PLR_FLAGGED(wch, PLR_TESTER))
      strcat(buf, " (Tester)");
    if (PRF_FLAGGED(wch, PRF_NOGOSS))
      strcat(buf, " (nogos)");
    if (PRF_FLAGGED(wch, PRF_DEAF))
      strcat(buf, " (deaf)");
    if (PRF_FLAGGED(wch, PRF_NOTELL))
      strcat(buf, " (notell)");
    if (PRF_FLAGGED(wch, PRF_QUEST))
      strcat(buf, " (quest)");
    if (PLR_FLAGGED(wch, PLR_THIEF))
      strcat(buf, " (THIEF)");
    if (PLR_FLAGGED(wch, PLR_KILLER))
      strcat(buf, " (KILLER)");
    if (PLR_FLAGGED(wch, PLR_CRIMINAL))
      strcat(buf, " (JAILED)");
    if (PLR_FLAGGED(wch, PLR_AFK))
      strcat(buf, " (AFK)");
    if (ROOM_FLAGGED(IN_ROOM(wch), ROOM_ARENA))
      strcat(buf, " (arena)");
    
    if(GET_MARRIED(wch)) {
      strcpy(name, get_name_by_id(GET_MARRIED(wch)));
      sprintf(buf, "%s %s(%s of %s)%s", buf, CCYEL(ch, C_SPR),
	      (GET_SEX(wch) == SEX_MALE ? "Husband" : "Wife"), CAP(name), CCNRM(ch, C_SPR));
    }
    
    if ((clan = is_in_clan(wch)) >= 0) {
      if (GET_IDNUM(wch) == clan_control[clan].leader)
        sprintf(buf, "%s %s([Leader] %s)%s", buf, CCRED(ch, C_SPR),
		CAP(clan_control[clan].clanname), CCNRM(ch, C_SPR));
      else
        sprintf(buf, "%s %s(%s)%s", buf, CCRED(ch, C_SPR),
		CAP(clan_control[clan].clanname), CCNRM(ch, C_SPR));
    } /* end if in clan */
    

    strcat(buf, "\r\n");
    send_to_char(buf, ch);
  }
  
  if (num_can_see == 0)
    sprintf(buf, "\r\nNo-one at all!\r\n");
  else if (num_can_see == 1)
    sprintf(buf, "\r\nOne lonely character displayed.\r\n");
  else
    sprintf(buf, "\r\n%d characters displayed.\r\n", num_can_see);
  send_to_char(buf, ch);
}

#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
  char line[200], line2[220], idletime[10], classname[20];
  char state[30], *timeptr, mode;
  const char *format;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  size_t i;
  int low = 0, high = LVL_IMPL, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      case 'c':
	playing = 1;
	half_chop(buf1, arg, buf);
	for (i = 0; i < strlen(arg); i++)
	  showclass |= find_class_bitvector(arg[i]);
	break;
      default:
	send_to_char(USERS_FORMAT, ch);
	return;
	break;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  }				/* end while (parser) */
  strcpy(line,
	 "Num Class    Name         State  Idl Login@   Site\r\n");
  strcat(line,
	 "&c--- -------- ------------ ------ --- -------- --------------------------------&n\r\n");
  send_to_char(line, ch);

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && playing)
      continue;
    if (!d->connected && deadweight)
      continue;
    if (!d->connected) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;

      if (d->original)
	sprintf(classname, "[%3d %s]", GET_LEVEL(d->original),
		CLASS_ABBR(d->original));
      else
	sprintf(classname, "[%3d %s]", GET_LEVEL(d->character),
		CLASS_ABBR(d->character));
    } else
      strcpy(classname, "   --   ");

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (!d->connected && d->original)
      strcpy(state, "Switch");
    else
      strcpy(state, connected_types[d->connected]);

    if (d->character && !d->connected && GET_LEVEL(d->character) < LVL_CREATOR)
      sprintf(idletime, "%3d", d->character->char_specials.timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    format = "&g%3d&n %-7s &r%-12s&n %-6s %-3s %-8s ";

    if (d->character && d->character->player.name) {
      if (d->original)
	sprintf(line, format, d->desc_num, classname,
		d->original->player.name, state, idletime, timeptr);
      else
	sprintf(line, format, d->desc_num, classname,
		d->character->player.name, state, idletime, timeptr);
    } else
      sprintf(line, format, d->desc_num, "   --   ", "UNDEFINED",
	      state, idletime, timeptr);

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[&y%s&n]\r\n", d->host);
    else
      strcat(line, "[&rHostname unknown&n]\r\n");

    if (d->connected) {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strcpy(line, line2);
    }
    if (d->connected || (!d->connected && CAN_SEE(ch, d->character))) {
      send_to_char(line, ch);
      num_can_see++;
    }
    

  }

  sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}

/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HEROLIST:
    page_string(ch->desc, herolist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_NEWBIE:
    page_string(ch->desc, newbie, 0);
    break;
  case SCMD_OLC_RULES:
    page_string(ch->desc, olcrules, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char("\033[H\033[J", ch);
    break;
  case SCMD_VERSION:
    send_to_char(circlemud_version, ch);
    break;
  case SCMD_WHOAMI:
    send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
    break;
  default:
    return;
    break;
  }
}


void perform_mortal_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct descriptor_data *d;

  if (!*arg) {
    send_to_char("Players in your Zone\r\n--------------------\r\n", ch);
    for (d = descriptor_list; d; d = d->next) {
      
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (i->in_room == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[ch->in_room].zone != world[i->in_room].zone)
	continue;
      sprintf(buf, "%-20s - %s\r\n", GET_NAME(i), world[i->in_room].name);
      send_to_char(buf, ch);
    }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (i->in_room == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[i->in_room].zone != world[ch->in_room].zone)       continue;
      if (!isname(arg, i->player.name))
	continue;
      sprintf(buf, "%-25s - %s\r\n", GET_NAME(i), world[i->in_room].name);
      send_to_char(buf, ch);
      return;
    }
    send_to_char("No-one around by that name.\r\n", ch);
  }
}


void print_object_location(int num, struct obj_data * obj, struct char_data * ch,
			        int recur)
{
  if (num > 0)
    sprintf(buf, "O%3d. %-25s - ", num, obj->short_description);
  else
    sprintf(buf, "%33s", " - ");

  if (obj->in_room > NOWHERE) {
    sprintf(buf + strlen(buf), "[%5d] %s\n\r",
	    world[obj->in_room].number, world[obj->in_room].name);
    send_to_char(buf, ch);
  } else if (obj->carried_by) {
    sprintf(buf + strlen(buf), "carried by %s\n\r", PERS(obj->carried_by, ch));
    send_to_char(buf, ch);
  } else if (obj->worn_by) {
    sprintf(buf + strlen(buf), "worn by %s\n\r", PERS(obj->worn_by, ch));
    send_to_char(buf, ch);
  } else if (obj->in_obj) {
    sprintf(buf + strlen(buf), "inside %s%s\n\r",
	    obj->in_obj->short_description, (recur ? ", which is" : " "));
    send_to_char(buf, ch);
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else {
    sprintf(buf + strlen(buf), "in an unknown location\n\r");
    send_to_char(buf, ch);
  }
}



void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {
    send_to_char("Players\r\n-------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
	  if (d->original)
	    sprintf(buf, "%-20s - [%5d] %s (in %s)\r\n",
		    GET_NAME(i), world[d->character->in_room].number,
		 world[d->character->in_room].name, GET_NAME(d->character));
	  else
	    sprintf(buf, "%-20s - [%5d] %s\r\n", GET_NAME(i),
		    world[i->in_room].number, world[i->in_room].name);
	  send_to_char(buf, ch);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, i->player.name)) {
	found = 1;
	sprintf(buf, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
		world[i->in_room].number, world[i->in_room].name);
	send_to_char(buf, ch);
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name) &&
	  (!k->carried_by || CAN_SEE(ch, k->carried_by)) &&
	  (!k->worn_by || CAN_SEE(ch, k->worn_by))) {
	found = 1;

      if (num > 80) {
	send_to_char("*** OVERFLOW ***", ch);
	return;
      } else
		print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char("Couldn't find any such thing.\r\n", ch);
  }
}



ACMD(do_where)
{
  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}


ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("Easy!  Very easy indeed!\r\n", ch);
    return;
  }
  if (!IS_NPC(victim)) {
    send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char("Now where did that chicken go?\r\n", ch);
  else if (diff <= -5)
    send_to_char("You could do it with a needle!\r\n", ch);
  else if (diff <= -2)
    send_to_char("Easy.\r\n", ch);
  else if (diff <= -1)
    send_to_char("Fairly easy.\r\n", ch);
  else if (diff == 0)
    send_to_char("The perfect match!\r\n", ch);
  else if (diff <= 1)
    send_to_char("You would need some luck!\r\n", ch);
  else if (diff <= 2)
    send_to_char("You would need a lot of luck!\r\n", ch);
  else if (diff <= 3)
    send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
  else if (diff <= 5)
    send_to_char("Do you feel lucky, punk?\r\n", ch);
  else if (diff <= 10)
    send_to_char("Are you mad!?\r\n", ch);
  else if (diff <= 100)
    send_to_char("You ARE mad!\r\n", ch);

}



ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    } else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char("Diagnose who?\r\n", ch);
  }
}


static const char *ctypes[] = {
"off", "sparse", "normal", "complete", "\n"};

ACMD(do_color)
{
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(TMP_FLAGS(ch), TMP_COLOR_1 | TMP_COLOR_2);
  SET_BIT(TMP_FLAGS(ch), (TMP_COLOR_1 * (tp & 1)) | (TMP_COLOR_2 * (tp & 2) >> 1));

  sprintf(buf, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR),
	  CCNRM(ch, C_OFF), ctypes[tp]);
  send_to_char(buf, ch);
}


ACMD(do_toggle)
{
  if (IS_NPC(ch))
    return;
  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");
  else
    sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));

  sprintf(buf,
	  "      Autosplit: %-3s    "
	  "     Brief Mode: %-3s    "
	  " Summon Protect: %-3s\r\n"

	  "   Tick Display: %-3s    "
	  "   Compact Mode: %-3s    "
	  "       On Quest: %-3s\r\n"

	  " Commis Channel: %-3s    "
	  "         NoTell: %-3s    "
	  "   Repeat Comm.: %-3s\r\n"

	  " Auto Show Exit: %-3s    "
	  "           Deaf: %-3s    "
	  "     Wimp Level: %-3s\r\n"

	  " Gossip Channel: %-3s    "
	  "Auction Channel: %-3s    "
	  "  Grats Channel: %-3s\r\n"

	  "       Autoloot: %-3s    "
	  "       Autogold: %-3s    "
	  " Sports Channel: %-3s\r\n"

	  "  Music Channel: %-3s    "
	  "     AutoAssist: %-3s    "
	  "    Color Level: %s\r\n",

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
	  ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

	  ONOFF(PRF_FLAGGED(ch, PRF_TICK)),
	  ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
	  YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOCOMMIS)),
	  ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
	  YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
	  YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
	  buf2,

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),

	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOSPORTS)),

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOMUSIC)),
	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
	  ctypes[COLOR_LEV(ch)]);


  send_to_char(buf, ch);
}


struct sort_struct {
  int sort_pos;
  byte is_social;
} *cmd_sort_info = NULL;

int num_of_cmds;


void sort_commands(void)
{
  int a, b, tmp;

  num_of_cmds = 0;

  /*
   * first, count commands (num_of_commands is actually one greater than the
   * number of commands; it inclues the '\n'.
   */
  while (*cmd_info[num_of_cmds].command != '\n')
    num_of_cmds++;

  /* create data array */
  CREATE(cmd_sort_info, struct sort_struct, num_of_cmds);

  /* initialize it */
  for (a = 1; a < num_of_cmds; a++) {
    cmd_sort_info[a].sort_pos = a;
    cmd_sort_info[a].is_social = (cmd_info[a].command_pointer == do_action);
  }

  /* the infernal special case */
  cmd_sort_info[find_command("insult")].is_social = TRUE;

  /* Sort.  'a' starts at 1, not 0, to remove 'RESERVED' */
  for (a = 1; a < num_of_cmds - 1; a++)
    for (b = a + 1; b < num_of_cmds; b++)
      if (strcmp(cmd_info[cmd_sort_info[a].sort_pos].command,
		 cmd_info[cmd_sort_info[b].sort_pos].command) > 0) {
	tmp = cmd_sort_info[a].sort_pos;
	cmd_sort_info[a].sort_pos = cmd_sort_info[b].sort_pos;
	cmd_sort_info[b].sort_pos = tmp;
      }
}



ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char("You can't see the commands of people above your level.\r\n", ch);
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  sprintf(buf, "The following %s%s are available to %s:\r\n",
	  wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; cmd_num < num_of_cmds; cmd_num++) {
    i = cmd_sort_info[cmd_num].sort_pos;
    if (cmd_info[i].minimum_level >= 0 &&
	GET_LEVEL(vict) >= cmd_info[i].minimum_level &&
	(cmd_info[i].minimum_level >= LVL_IMMORT) == wizhelp &&
	(wizhelp || socials == cmd_sort_info[i].is_social)) {
      sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
      if (!(no % 7))
	strcat(buf, "\r\n");
      no++;
    }
  }

  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}

   /* functions and macros for 'scan' command */

  /*
    just add this file to the end of act.informative.c (MAKE SURE YOU PRESERVE
    THE SPACING ON THE MACRO DEFINES), and add two things to interpreter.c:
     a) a prototype for do_scan - put the line
        ACMD(do_scan);
        somewhere near the top, with all the other ACMD prototypes.
     b) put an entry in the command list for 'scan', I suggest after 'score'.

    you can contact me sjmaster@mit.edu if you have problems, which you should
    not.
   */

void list_scanned_chars(struct char_data * list, struct char_data * ch, int distance, int door)
{
  const char *how_far[] = {
    "close by",
    "a ways off",
    "far off to the"
  };

  struct char_data *i;
  int count = 0;
  *buf = '\0';

/* this loop is a quick, easy way to help make a grammatical sentence
   (i.e., "You see x, x, y, and z." with commas, "and", etc.) */

  for (i = list; i; i = i->next_in_room)

/* put any other conditions for scanning someone in this if statement -
   i.e., if (CAN_SEE(ch, i) && condition2 && condition3) or whatever */

    if (CAN_SEE(ch, i))
     count++;

  if (!count)
    return;

  for (i = list; i; i = i->next_in_room) {

/* make sure to add changes to the if statement above to this one also, using
   or's to join them.. i.e.,
   if (!CAN_SEE(ch, i) || !condition2 || !condition3) */

    if (!CAN_SEE(ch, i))
      continue;
    if (!*buf)
      sprintf(buf, "You see %s", GET_NAME(i));
    else
      sprintf(buf, "%s%s", buf, GET_NAME(i));
    if (--count > 1)
      strcat(buf, ", ");
    else if (count == 1)
      strcat(buf, " and ");
    else {
      sprintf(buf2, " %s %s.\r\n", how_far[distance], dirs[door]);
      strcat(buf, buf2);
    }

  }
  send_to_char(buf, ch);
}

/* utils.h: #define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door]) */

#define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])


ACMD(do_scan)
{
  /* >scan
     You quickly scan the area.
     You see John, a large horse and Frank close by north.
     You see a small rabbit a ways off south.
     You see a huge dragon and a griffon far off to the west.

  */
  int door;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  /* may want to add more restrictions here, too */
  send_to_char("You quickly scan the area.\r\n", ch);
  for (door = 0; door < NUM_OF_DIRS - 2; door++) /* don't scan up/down */
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE &&
	!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      if (world[EXIT(ch, door)->to_room].people) {
	list_scanned_chars(world[EXIT(ch, door)->to_room].people, ch, 0, door);
      } else if (_2ND_EXIT(ch, door) && _2ND_EXIT(ch, door)->to_room !=
		 NOWHERE && !IS_SET(_2ND_EXIT(ch, door)->exit_info, EX_CLOSED)) {
   /* check the second room away */
	if (world[_2ND_EXIT(ch, door)->to_room].people) {
	  list_scanned_chars(world[_2ND_EXIT(ch, door)->to_room].people, ch, 1, door);
	} else if (_3RD_EXIT(ch, door) && _3RD_EXIT(ch, door)->to_room !=
		   NOWHERE && !IS_SET(_3RD_EXIT(ch, door)->exit_info, EX_CLOSED)) {
	  /* check the third room */
	  if (world[_3RD_EXIT(ch, door)->to_room].people) {
	    list_scanned_chars(world[_3RD_EXIT(ch, door)->to_room].people, ch, 2, door);
	  }

	}
      }
    }
}

ACMD(do_levels) {

  int i;

  strcpy(buf, "The Following Experience is needed for each level:\r\n\r\n");

  for (i = 1; i < LVL_IMMORT; i++)
    sprintf(buf, "%s%d.  %d\r\n", buf, i, exp_to_level(i));

  page_string(ch->desc, buf, 1);

}
