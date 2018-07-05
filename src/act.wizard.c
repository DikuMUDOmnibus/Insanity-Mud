/*************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
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
#include "house.h"
#include "screen.h"
#include "olc.h"
#include "loadrooms.h"

/* extern functions */
void do_start(struct char_data *ch);
void gain_exp(struct char_data * ch, int gain);
void show_shops(struct char_data * ch, char *value);
void hcontrol_list_houses(struct char_data *ch);
void appear(struct char_data *ch);
int parse_class(struct char_data *ch, char arg);
void reset_zone(int zone);
void roll_real_abils(struct char_data *ch);
extern void olc_add_to_save_list(int zone, byte type);
extern int real_zone(int number);
void    look_at_room(struct char_data *ch, int mode);
void read_aliases(struct char_data *ch);
void dismount_char(struct char_data * ch);
void write_aliases(struct char_data *ch);
extern void redit_save_internally(struct descriptor_data *d);
extern void olc_add_to_save_list(int zone, byte type);
void list_skills(struct char_data * ch, struct char_data * vict);
void show_score_to_char(struct char_data * ch, struct char_data * recip);

/*   external vars  */
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern int top_of_objt;
extern int top_of_mobt;
extern int top_of_world;
extern const int rev_dir[];
extern FILE *player_fl;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;

extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct int_app_type int_app[];
extern struct wis_app_type wis_app[];
extern struct zone_data *zone_table;
extern time_t boot_time;
extern struct attack_hit_type attack_hit_text[];
extern char *class_abbrevs[];
extern int top_of_zone_table;
extern int restrict;
extern int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_p_table;
extern int circle_shutdown, circle_reboot;
extern int buf_switches, buf_largecount, buf_overflows;
extern struct player_index_element *player_table;
extern int reserved_word(char *argument);
extern int Valid_Name(char *newname);
extern sh_int r_immort_start_room;

/* arena stuff */
extern int in_arena;
extern int in_start_arena;
extern int game_time;
extern int time_to_start;
extern int game_length;
extern int start_tome;
extern int time_left_in_game;
extern int start_time;

/* for objects */
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *container_bits[];
extern const char *drinks[];
extern const char *shot_types[];
extern const int shot_damage[];

/* for rooms */
extern const char *dirs[];
extern const char *room_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const int rev_dir[];

/* for chars */
extern char *genders[];
extern const char *spells[];
extern const  char *equipment_types[];
extern const char *affected_bits[];
extern const  char *apply_types[];
extern const  char *temp_bits[];
extern const  char *pc_class_types[];
extern const char *npc_class_types[];
extern const char *action_bits[];
extern const char *player_bits[];
extern const  char *preference_bits[];
extern const char *position_types[];
extern const char *connected_types[];
extern sh_int r_mortal_start_room[NUM_STARTROOMS +1]; /* This is necessary for parole */
extern sh_int r_jailed_start_room; /* This is necessary for jail */
extern char *languages[];

void Read_Invalid_List(void);
void show_connections(struct char_data *ch, char *arg);
ACMD(do_wmote);

/* check for true zone number */
int get_zon_num(int vnum)
{
  int i, zn = -1, zone;

  zone = vnum / 100;

  for(i = 0; i <= top_of_zone_table && zone_table[i].number <= zone; i++);
  if(i <= top_of_zone_table || zone_table[i - 1].top >= vnum)
    zn = zone_table[i - 1].number;

  return zn;
}

/* function to check olc permissions */
int has_zone_permission(int number, sh_int level, struct char_data *ch) {
  int zone;
  zone = real_zone(number);
  if ((GET_LEVEL(ch) < level) && !PLR_FLAGGED(ch, PLR_WORLD) &&
      ((zone_table[zone].number != GET_OLC_ZONE1(ch)) &&
       (zone_table[zone].number != GET_OLC_ZONE2(ch)) &&
       (zone_table[zone].number != GET_OLC_ZONE3(ch)))) {
    return 0;
  }
  return 1;
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data * ch, char *rawroomstr)
{
  int tmp;
  sh_int location;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char("You must supply a room number or name.\r\n", ch);
    return NOWHERE;
  }
  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    tmp = atoi(roomstr);
    if ((location = real_room(tmp)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return NOWHERE;
    }
  } else if ((target_mob = get_char_vis(ch, roomstr)))
    location = target_mob->in_room;
  else if ((target_obj = get_obj_vis(ch, roomstr))) {
    if (target_obj->in_room != NOWHERE)
      location = target_obj->in_room;
    else {
      send_to_char("That object is not available.\r\n", ch);
      return NOWHERE;
    }
  } else {
    send_to_char("No such creature or object around.\r\n", ch);
    return NOWHERE;
  }

  /* a location has been found -- if you're < SUPREME, check restrictions. */

  tmp = world[location].number;
  if (GET_LEVEL(ch) < LVL_LOWIMPL) {
    if (ROOM_FLAGGED(location, ROOM_IMPL)) {
      send_to_char("You are not godly enough to use that room!\r\n", ch);
      return NOWHERE;
    }
  }

  if (GET_LEVEL(ch) < LVL_SUPREME) {
    if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
      send_to_char("You are not godly enough to use that room!\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_PRIVATE) &&
	world[location].people && world[location].people->next_in_room) {
      send_to_char("There's a private conversation going on in that room.\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_HOUSE) &&
	!House_can_enter(ch, world[location].number)) {
      send_to_char("That's private property -- no trespassing!\r\n", ch);
      return NOWHERE;
    }
  }
  return location;
}

#define ZCMD zone_table[zone].cmd[cmd_no]

ACMD(do_zload) {


  int cmd_no, zone = 0, room = -1, j;
  char buf[MAX_STRING_LENGTH];

  one_argument(argument, arg);

  if (!*arg || !isdigit(*arg)) {
    send_to_char("Usage: zload <zone number>\r\n", ch);
    return;
  }

  j = atoi(arg);
  for (zone = 0; zone_table[zone].number != j && zone <= top_of_zone_table; zone++);

  if (zone > top_of_zone_table || zone < 0) {
    send_to_char("That is not a valid zone.\r\n", ch);
    return;
  }

  sprintf(buf, "Loads for Zone %d [&r%s&n].\r\n\r\n", zone, zone_table[zone].name);

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
    
    if (ZCMD.command == 'D')
      continue;

    switch(ZCMD.command) {

    case'M':
      if (room != world[ZCMD.arg3].number) {
	room = world[ZCMD.arg3].number;
	sprintf (buf, "&yRoom %d:&n\r\n", room);
        send_to_char(buf, ch);
      }

      sprintf(buf2, "%sLoad %s [&c%d&n], Max : &r%d&n",
	      ZCMD.if_flag ? " then " : "",
	      mob_proto[ZCMD.arg1].player.short_descr,
	      mob_index[ZCMD.arg1].virtual,
	      ZCMD.arg2
	      );
      break;
    case'G':
      sprintf(buf2, "%sGive it %s [&c%d&n], Max : &r%d&n",
	      ZCMD.if_flag ? " then " : "",
	      obj_proto[ZCMD.arg1].short_description,
	      obj_index[ZCMD.arg1].virtual,
	      ZCMD.arg2
	      );
      break;
    case'O':
      if (room != world[ZCMD.arg3].number) {
	room = world[ZCMD.arg3].number;
	sprintf (buf, "&yRoom %d:&n\r\n", room);
        send_to_char(buf, ch);
      }

      sprintf(buf2, "%sLoad %s [&c%d&n], Max : &r%d&n",
	      ZCMD.if_flag ? " then " : "",
	      obj_proto[ZCMD.arg1].short_description,
	      obj_index[ZCMD.arg1].virtual,
	      ZCMD.arg2
	      );
      break;
    case'E':
      sprintf(buf2, "%sEquip with %s [&c%d&n], %s, Max : &r%d&n",
	      ZCMD.if_flag ? " then " : "",
	      obj_proto[ZCMD.arg1].short_description,
	      obj_index[ZCMD.arg1].virtual,
	      equipment_types[ZCMD.arg3],
	      ZCMD.arg2
	      );
      break;
    case'P':
      sprintf(buf2, "%sPut %s [&c%d&n] in %s [&c%d&n], Max : &r%d&n",
	      ZCMD.if_flag ? " then " : "",
	      obj_proto[ZCMD.arg1].short_description,
	      obj_index[ZCMD.arg1].virtual,
	      obj_proto[ZCMD.arg3].short_description,
	      obj_index[ZCMD.arg3].virtual,
	      ZCMD.arg2
	      );
      break;
    case'R':
      if (room != world[ZCMD.arg1].number) {
	room = world[ZCMD.arg1].number;
	sprintf (buf, "&yRoom %d:&n\r\n", room);
        send_to_char(buf, ch);
      }

      sprintf(buf2, "%sRemove %s [&c%d&n] from room.",
	      ZCMD.if_flag ? " then " : "",
	      obj_proto[ZCMD.arg2].short_description,
	      obj_index[ZCMD.arg2].virtual
	      );
      break;

    default:
      strcpy(buf2, "<Unknown Command>");
      break;
    }

    sprintf(buf, "  %s\r\n", buf2);
    send_to_char(buf, ch);
  }
}


ACMD(do_godengr)
{
  struct char_data *victim;
  struct obj_data *obj;
  char buf2[80];
  char buf3[80];

  two_arguments(argument, buf2, buf3);

  if (!*buf3 || !*buf2)
    send_to_char("Usage: godengrave <object> <character>.\r\n", ch);
  else if (!(victim = get_char_vis(ch, buf3)))
    send_to_char("No one by that name here.\r\n", ch);
  else if (!(obj = get_obj_in_list_vis(ch, buf2, ch->carrying))) {
    sprintf(buf, "You don't seem to have any %ss.\r\n", arg);
    send_to_char(buf, ch);
  } else if (GET_OBJ_ENGRAVE(obj) != 0)
    send_to_char ("Thats already been engraved to someone!\r\n", ch);
  else {
	  GET_OBJ_ENGRAVE(obj) = GET_IDNUM(victim);
	  act("$n begins to engrave $p to $N.", FALSE, ch, obj, victim, TO_NOTVICT);
	  act("$n begins to engrave $p to you.", FALSE, ch, obj, victim, TO_VICT);
	  act("You engrave $p to $N.", FALSE, ch, obj, victim, TO_CHAR);
  }
}

ACMD(do_chown)
{
  struct char_data *victim;
  struct obj_data *obj;
  char buf2[80];
  char buf3[80];
  int i, k = 0;

  two_arguments(argument, buf2, buf3);

  if (!*buf3 || !*buf2)
    send_to_char("Syntax: chown <object> <character>.\r\n", ch);
  else if (!(victim = get_char_vis(ch, buf3)))
    send_to_char("No one by that name here.\r\n", ch);
  else if (victim == ch)
    send_to_char("Are you sure you're feeling ok?\r\n", ch);
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char("That's really not such a good idea.\r\n", ch);
  else if ((IS_SET(ROOM_FLAGS(victim->in_room), ROOM_LOCKED_DOWN)) 
	   && (GET_LEVEL(ch) != LVL_LOWIMPL))
    send_to_char("An impenetrable barrier obstructs you.\r\n", ch);
  else {
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(victim, i) && CAN_SEE_OBJ(ch, GET_EQ(victim, i)) &&
	  isname(buf2, GET_EQ(victim, i)->name)) {
        obj_to_char(unequip_char(victim, i), victim);
        k = 1;
      }
    }

    if (!(obj = get_obj_in_list_vis(ch, buf2, victim->carrying))) {
      if (!k && !(obj = get_obj_in_list_vis(ch, buf2, victim->carrying))) {
	sprintf(buf, "%s does not appear to have the %s.\r\n", GET_NAME(victim), buf2);
	send_to_char(buf, ch);
	return;
      }
    }

    act("$n makes a magical gesture and $p flies from $N to $m.",FALSE,ch,obj,
	victim,TO_NOTVICT);
    act("$n makes a magical gesture and $p flies away from you to $m.",FALSE,ch,
	obj,victim,TO_VICT);
    act("You make a magical gesture and $p flies away from $N to you.",FALSE,ch,
	obj, victim,TO_CHAR);

    obj_from_char(obj);
    obj_to_char(obj, ch);
    save_char(ch, NOWHERE);
    save_char(victim, NOWHERE);
  }
}

ACMD(do_objdam)
{
  two_arguments(argument, buf1, buf2);

  if ((!isdigit(*buf1)) || (!isdigit(*buf2))) {
        send_to_char("Usage: objdam <num-dice> <damdice>\r\n", ch);
        return;
        }

   obj_max_damage(atoi(buf1), atoi(buf2), ITEM_WEAPON, ch);

}

ACMD(do_chskill) {

  struct char_data *vict;
  int number = 0, i;

  half_chop(argument, arg, buf);

  if (!*arg || !*buf) {
    send_to_char("Usage: chskill <player> <skill level>\r\n", ch);
    return;
  }

  number = atoi(buf);
  if (!(vict = get_char_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
    return;
  }

  for (i = 1; i <= MAX_SKILLS; i++)
    SET_SKILL(vict, i, number);

  sprintf(buf, "All of %s's skills have now been set to %d.\r\n", GET_NAME(vict), number);
  send_to_char(buf, ch);
}

ACMD(do_site)
{
  extern struct player_index_element *player_table;
  long count = 0;
  int i;
  char *LastTime = '\0';

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("For what site do you wish to search?\r\n", ch);
    return;
  }

  strcpy(buf, "Player:              Site:                          Time:\r\n"
	      "-------------------- ------------------------------ ------------------------\r\n");

  for (i = 0; i < top_of_p_table; i++) {
    if (strstr(player_table[i].host, arg)) {
        LastTime = (char *) malloc(strlen(ctime(&player_table[i].last_logon)));
        LastTime = ctime(&player_table[i].last_logon);
        LastTime[strlen(LastTime)-1] = '\0';
        sprintf(buf+strlen(buf), "%-20s %-30s %s\r\n", player_table[i].name,
 	      player_table[i].host, 
		((GET_LEVEL(ch) < LVL_LOWIMPL) ? "Not Available" : 
		 LastTime));
       count++;
      }
  }
  sprintf(buf2, "\r\n%ld Matches found.\r\n", count);
  strcat(buf, buf2);
  page_string(ch->desc, buf, 1);
}

ACMD(do_marry) {

  struct char_data *vict1 = NULL;
  struct char_data *vict2 = NULL;
  long id1 = 0, id2 = 0;

  half_chop(argument, arg, buf);

  if (!*arg || !*buf)
  {
    send_to_char("Usage: marry <player> <player>\r\n", ch);
    return;
   }

  if (!(vict1 = get_char_vis(ch, arg)) || !(vict2 = get_char_vis(ch, buf)))
  {
    send_to_char(NOPERSON, ch);
    return;
  }

  if ((GET_LEVEL(ch) < GET_LEVEL(vict1)) || (GET_LEVEL(ch) < GET_LEVEL(vict2))) {
    send_to_char("Hmmm.. Maybe you shouldn't!\r\n", ch);
    return;
  }

  if (GET_SEX(vict2) == GET_SEX(vict1)) {
    send_to_char("They are the same sex, I don't think so!\r\n", ch);
    return;
  }

  if (GET_MARRIED(vict1) || GET_MARRIED(vict2)) {
    send_to_char("They are already married.\r\n",ch);
    return;
  }

  if (!TMP_FLAGGED(vict1, TMP_MARRIAGE) || !TMP_FLAGGED(vict2, TMP_MARRIAGE)) {
    send_to_char ("But they are not concenting adults!\r\n", ch);
    return;
  }

  id2 = GET_IDNUM(vict2);
  id1 = GET_IDNUM(vict1);
  GET_MARRIED(vict1) = id2;
  GET_MARRIED(vict2) = id1;

  send_to_char("You pronounce them Husband and Wife.\r\n", ch);
  sprintf(buf, "You are now the %s of %s.\r\n",
	  (GET_SEX(vict1) == SEX_MALE ? "husband" : "wife"), GET_NAME(vict2));
  send_to_char(buf, vict1);
  sprintf(buf, "You are now the %s of %s.\r\n",
	  (GET_SEX(vict2) == SEX_MALE ? "husband" : "wife"), GET_NAME(vict1));
  send_to_char(buf, vict2);
}

ACMD(do_divorce) {

  struct char_data *vict1;
  struct char_data *vict2;

  half_chop(argument, arg, buf);

  if (!*arg || !*buf) {
    send_to_char("Usage: divorce <player> <player>\r\n", ch);
    return;
  }

  if (!(vict1 = get_char_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
    return;
  }

  if (!(vict2 = get_char_vis(ch, buf))) {
    send_to_char(NOPERSON, ch);
    return;
  }

  if (((GET_MARRIED(vict1) == 0) && (GET_MARRIED(vict2) == GET_IDNUM(vict1))) ||
      ((GET_MARRIED(vict2) == 0) && (GET_MARRIED(vict1) == GET_IDNUM(vict2)))) {

    send_to_char ("Seems something dodgy has been going on here.\r\nBest remove all marriage flags.\r\n", ch);
    GET_MARRIED(vict1) = 0;
    GET_MARRIED(vict2) = 0;
    return;
  }

  if ((GET_MARRIED(vict1) != GET_IDNUM(vict2)) && (GET_MARRIED(vict2) != GET_IDNUM(vict1))) {
    send_to_char ("Trying to divorce people who arn't married... I don't think so!\r\n", ch);
    return;
  }
  if (!TMP_FLAGGED(vict1, TMP_MARRIAGE) || !TMP_FLAGGED(vict2, TMP_MARRIAGE)) {
    send_to_char ("But they don't seem to want to get divorced!\r\n", ch);
    return;
  }

  GET_MARRIED(vict1) = 0;
  GET_MARRIED(vict2) = 0;

  send_to_char("You declare them divorced.\r\n", ch);
  sprintf(buf, "You are now divorced from %s.\r\n", GET_NAME(vict1));
  send_to_char(buf, vict2);
  sprintf(buf, "You are now divorced from %s.\r\n", GET_NAME(vict2));
  send_to_char(buf, vict1);
}

ACMD(do_updtwiz) {

    sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &",
            LVL_GOD, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
    mudlogf(CMP, LVL_IMPL, FALSE, "Initiating autowiz.");
    system(buf);

    sprintf(buf, "nice ../bin/autohero LVL_HERO %s %d &",
            HEROLIST_FILE, (int) getpid());
    mudlogf(CMP, LVL_IMPL, FALSE, "Initiating autohero.");
    system(buf);
}

ACMD(do_givehome)
{
  struct char_data *vict;
  int home;

  half_chop(argument, arg, buf);

  if (!*arg || !*buf || !isdigit(*buf)) {
    send_to_char("Usage: givehome <player> <location>\r\n", ch);
    return;
  }

  if (!(vict = get_char_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
    return;
  }

  if (GET_GOHOME(vict) != 0) {
    send_to_char("Character already has a gohome, remove the old one first!\r\n", ch );
    return;
  }

  home = atoi(buf);
  if (real_room(home) < 0) {
    send_to_char("Room does not exist.\r\n", ch);
    return;
  }

  GET_GOHOME(vict) = home;
  GET_LOADROOM(vict) = home;
  sprintf(buf, "You make %s a gohome!\r\n", GET_NAME(vict));
  send_to_char(buf, ch);
  sprintf(buf, "%s makes you a gohome!\r\n", GET_NAME(ch));
  send_to_char(buf, vict);

  mudlogf(NRM, MAX(LVL_ARCHWIZ, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s has given %s gohome #%d", GET_NAME(ch), GET_NAME(vict), GET_GOHOME(vict));
}

ACMD(do_takehome)
{
  struct char_data *vict;
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Usage: takehome <player>\r\n", ch);
    return;
  }

  if (!(vict = get_char_vis(ch, argument))) {
    send_to_char(NOPERSON, ch);
    return;
  }

  if (GET_LEVEL(vict) > GET_LEVEL(ch) ) {
    send_to_char("You not godly enough to do that!\r\n", ch);
    return;
  }

  if (GET_GOHOME(vict) == 0) {
    sprintf(buf, "%s does not even have a gohome!\r\n", GET_NAME(vict));
    send_to_char(buf, ch);
    return;
  }

  mudlogf(NRM, MAX(LVL_ARCHWIZ, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s has taken %s's gohome.", GET_NAME(ch), GET_NAME(vict));

  GET_GOHOME(vict) = 0;
  sprintf(buf, "%s removes your gohome!\r\n", GET_NAME(ch));
  send_to_char(buf, vict);
  sprintf(buf, "You remove %s gohome!\r\n", GET_NAME(vict));
  send_to_char(buf, ch);
}


ACMD(do_roomlink) {        /* Shows all exits to a given room */

  int door = 0;
  int i = 0;
  int room_num = 0;

  one_argument(argument, buf);

  if (!*buf) {
    room_num = ch->in_room;
  } else {
    room_num = real_room(atoi(buf));
  }

  if (room_num == NOWHERE) {
    send_to_char("There is no room with that number.\r\n", ch);
    return;
  } else {
    for (i = 0; i <= top_of_world; i++) {
      for (door = 0; door < NUM_OF_DIRS; door++) {
        if (world[i].dir_option[door] && world[i].dir_option[door]->to_room == room_num) {
	  sprintf(buf,  "Exit %s from room %d.\r\n", dirs[door], world[i].number);
	  send_to_char(buf, ch);
        }
      }
    }
  }
}

ACMD(do_copyto)
{
  int iroom = 0, rroom = 0;
  struct room_data *room;

  one_argument(argument, buf2);

  /* buf2 is room to copy to */
  CREATE (room, struct room_data, 1);
  iroom = atoi(buf2);
  rroom = real_room(atoi(buf2));
  *room = world[rroom];

  if (!*buf2) {
    send_to_char("Format: copyto <room number>\r\n", ch);
    return;
  }

  if (rroom <= 0) {
    sprintf(buf, "There is no room with the number %d.\r\n", iroom);
    send_to_char(buf, ch);
    return;
  }

  if (!has_zone_permission(iroom, LVL_CREATOR, ch)) {
    send_to_char("You don't have permission to edit that zone.\r\n", ch);
    return;
  }

  /* Main stuff */
  if (world[ch->in_room].description) {
    world[rroom].description = str_dup(world[ch->in_room].description);
    olc_add_to_save_list((iroom/100), OLC_SAVE_ROOM);
    sprintf(buf, "You copy the description to room %d.\r\n", iroom);
    send_to_char(buf, ch);
  } else
    send_to_char("This room has no description!\r\n", ch);
}

ACMD(do_rdig) {

  char buf3[10];
  char **Ptr = (char **)NULL;
  long iroom = 0;
  int rroom = 0, room = 0;
  int dir = 0, new = 0;

  two_arguments(argument, buf2, buf3);

  if (!*buf2) {
    send_to_char("Format: rdig <dir> <room number>\r\n", ch);
    return; }
  else if (!*buf3) {
    send_to_char("Format: rdig <dir> <room number>\r\n", ch);
    return;
  }

  iroom = strtol(buf3, Ptr, 10);
  
  if (iroom == 0) {
    send_to_char("Format: rdig <dir> <room number>\r\n", ch);
    return;
  }
  rroom = real_room((int) iroom);

  if (real_zone(iroom) == -1) {
    send_to_char("There is no zone for that number!\r\n",ch);
    return;
  }

  room = world[ch->in_room].number;
  if (!has_zone_permission(room, LVL_CREATOR, ch)) {
    send_to_char("You don't have permission to edit that zone.\r\n", ch);
    return;
  }
  if (!has_zone_permission((int) iroom, LVL_CREATOR, ch)) {
    send_to_char("You don't have permission to edit that zone.\r\n", ch);
    return;
  }

  CREATE(ch->desc->olc, struct olc_data, 1);
  if (rroom <= 0) {
    CREATE(OLC_ROOM(ch->desc), struct room_data, 1);
    OLC_ROOM(ch->desc)->name = strdup("An Unfinished room");
    OLC_ROOM(ch->desc)->description = strdup("An Unfinished Room\n");
    OLC_NUM(ch->desc) = ((int) iroom);
    new = 1;
  }

  /* Main stuff */
  switch (*buf2) {
  case 'n':
  case 'N':
    dir = NORTH;
    break;
  case 'e':
  case 'E':
    dir = EAST;
    break;
  case 's':
  case 'S':
    dir = SOUTH;
    break;
  case 'w':
  case 'W':
    dir = WEST;
    break;
  case 'u':
  case 'U':
    dir = UP;
    break;
  case 'd':
  case 'D':
    dir = DOWN;
    break;
  }

  if (world[ch->in_room].dir_option[dir])
    free(world[ch->in_room].dir_option[dir]);

  CREATE(world[ch->in_room].dir_option[dir], struct room_direction_data,1);
  world[ch->in_room].dir_option[dir]->general_description = NULL;
  world[ch->in_room].dir_option[dir]->keyword = NULL;
  world[ch->in_room].dir_option[dir]->to_room = rroom;

  if(!new) {
    OLC_ZNUM(ch->desc) = world[rroom].zone;
    if (world[rroom].dir_option[rev_dir[dir]])
      free(world[rroom].dir_option[rev_dir[dir]]);

    CREATE(world[rroom].dir_option[rev_dir[dir]], struct room_direction_data,1);

    world[rroom].dir_option[rev_dir[dir]]->general_description = NULL;
    world[rroom].dir_option[rev_dir[dir]]->keyword = NULL;
    world[rroom].dir_option[rev_dir[dir]]->to_room = ch->in_room;
  } else {
    OLC_ZNUM(ch->desc) = real_zone((int) iroom);
    CREATE(OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]], struct room_direction_data, 1);
    OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]]->general_description = NULL;
    OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]]->keyword = NULL;
    OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]]->to_room = ch->in_room;
  }

  if(new)
    redit_save_internally(ch->desc);

  world[ch->in_room].dir_option[dir]->to_room = real_room((int) iroom);
  olc_add_to_save_list((((int) iroom)/100), OLC_SAVE_ROOM);
  olc_add_to_save_list((room/100), OLC_SAVE_ROOM);
  free(OLC_ROOM(ch->desc));
  free(ch->desc->olc);
  sprintf(buf1, "You make an exit %s to room %d.\r\n", buf2, ((int) iroom) );
  send_to_char(buf1, ch);
}

int copy_flags(int src, int dest)
{
  if (src < 0 || dest < 0) {
    log("SYSERR:  copy_flags: source or dest. room does not exist!");
    return 1;
  }
  world[dest].room_flags = world[src].room_flags;
  world[dest].sector_type = world[src].sector_type;

  return 0;
}


ACMD(do_rspan)
{
  int rnum, vnum, rnum1, vnum1, i;

  skip_spaces(&argument);
  half_chop(argument, buf1, buf2);

  if (!*buf1 || !*buf2) {
    send_to_char("Usage: flagcopy <copy from> <copy to>\r\n\r\n", ch);
    return;
  }

  if (!is_number(buf1) || !is_number(buf2)) {
    send_to_char("Values must be numbers!\r\n", ch);
    return;
  }

  if (((vnum = atoi(buf1)) < 0) || ((vnum1 = atoi(buf2)) < 0)) {
    send_to_char("No negative vnums\r\n", ch);
    return;
  }

  if (vnum > vnum1) {
    send_to_char("The first argument must be smaller than the second.\r\n", ch);
    return;
  }

  if (real_zone(vnum) != real_zone(vnum1)) {
    send_to_char("You can't copy between different zones.\r\n",ch);
    return;
  }

  /* add check to see if the zone is correct for that builder unless he is a supreme god and above*/
  if (!has_zone_permission(vnum, LVL_CREATOR, ch)) {
    send_to_char("You don't have permission to edit that zone.\r\n", ch);
    return;
  }

  if((rnum = real_room(vnum)) < 0) {
    send_to_char("The first room does not exist!\r\n", ch);
    return;
  }

  for ((i = vnum + 1);i < (vnum1 + 1);i++)
  {
    if ((rnum1 = real_room(i)) < 0) {
      sprintf(buf, "Error, Room #%d does not exist skipping to next!\r\n", i);
      send_to_char(buf, ch);
    }
    else {
      copy_flags(rnum, real_room(i));
      sprintf(buf, "Copying Room #%d flags to Room #%d!\r\n", vnum, i);
      send_to_char(buf, ch);
    }
  }
}

/* linkload a player */
ACMD(do_linkload)
{
  int plr_index;
  struct char_file_u st;
  struct char_data *vict;

  one_argument(argument, arg);

  /* Standard checks */
  if(IS_NPC(ch)) {
    send_to_char( "Mobs Link loading players? I think not\r\n", ch );
    return;
  }
  if(!*arg ) {
    send_to_char( "Linkload which player?\r\n", ch );
    return;
  }
  if(strlen(arg) < 2 || strlen(arg) > MAX_NAME_LENGTH ||
     fill_word(strcpy(buf, arg)) || reserved_word (buf) || !Valid_Name(buf)) {
    send_to_char( "Invalid name.\r\n", ch );
    return;
  }

  /* Okay, so load him from file */
  if((plr_index = load_char(arg, &st)) > -1) {
    struct char_data *k;

    /* Dont load if already playing */
    for(k = character_list; k; k = k->next) {
      if((GET_IDNUM(k) == st.char_specials_saved.idnum)) {
	send_to_char( "You can't do that they are already playing!\r\n", ch);
	return;
      }
    }

    /* Not allowed to load someone = or > than you */
    if(st.level >= GET_LEVEL(ch)) {
      sprintf(buf, "%s is a higher being, you would be foolish to try this again!\r\n", CAP(st.name));
      send_to_char(buf, ch);
      mudlogf(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s illegally attempted to linkload %s", GET_NAME( ch ), st.name );
      return;
    }

    mudlogf(NRM,  MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
	    "(GC) %s has just linkloaded %s", GET_NAME(ch), CAP(st.name));

    /* Create new player structure, and load from store */
    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    store_to_char(&st, vict);
    vict->desc = NULL;
    GET_PFILEPOS(vict) = plr_index;

    /* Remove any unecessary player bits, and let us know 
       if this person has been deleted. */
    
    REMOVE_BIT( TMP_FLAGS( vict ), 
		TMP_WRITING | TMP_MAILING | PLR_CRYO | TMP_OLC);

    if(PLR_FLAGGED(vict, PLR_DELETED)) {
      strcpy(buf, GET_TITLE(vict));
      strcat(buf, " (DELETED)");
      free(GET_TITLE(vict));
      GET_TITLE(vict) = str_dup(buf);
    }

    reset_char(vict);
    Crash_load(vict);
    read_aliases(vict);
    vict->in_room = NOWHERE;

    /* Save & Add in to main char list */
    save_char(vict, NOWHERE);
    vict->next = character_list;
    character_list = vict;

    char_to_room(vict, ch->in_room);
    act( "With great magical powers $n rips the soul of $N from $S eternal sleep.", FALSE, ch, 0, vict, TO_ROOM );
    act( "You rip the soul of $N from $S eternal sleep.", FALSE, ch, 0, vict, TO_CHAR );
  }
  else
    send_to_char( "That person does not exist.\r\n", ch );
}

ACMD(do_file)
{
  FILE *req_file;
  int cur_line = 0,
  num_lines = 0,
  req_lines = 0,
  i,
  j;
  int l;
  char field[MAX_INPUT_LENGTH],
  value[MAX_INPUT_LENGTH];

  struct file_struct {
    char *cmd;
    char level;
    char *file;
  } fields[] = {
    { "none",           LVL_IMPL,    "Does Nothing" },
    { "bug",	        LVL_LOWIMPL,    "misc/bugs"},
    { "typo",		LVL_SUPREME, "misc/typos"},
    { "ideas",		LVL_SUPREME,    "misc/ideas"},
    { "xnames",		LVL_LOWIMPL,      "misc/xnames"},
    { "levels",         LVL_GRGOD,   "../log/levels" },
    { "rip",            LVL_GOD,   "../log/rip" },
    { "players",        LVL_GRGOD,   "../log/newplayers" },
    { "rentgone",       LVL_GOD,   "../log/rentgone" },
    { "godcmds",        LVL_SUPREME,    "../log/godcmds" },
    { "connections",    LVL_LOWIMPL,  "../log/connections"},
    { "loads",          LVL_GRGOD,   "../log/loads" },
    { "errors",         LVL_SUPREME,    "../log/errors" },
    { "dts",            LVL_GOD,     "../log/dts" },
    { "delete",         LVL_CREATOR, "../log/delete" },
    { "help",           LVL_GRGOD,    "../log/help"},
    { "badpws",         LVL_GRGOD,     "../log/badpws" },
    { "olc",            LVL_SUPREME,   "../log/olc"},
    { "equip",          LVL_CREATOR, "../log/equip"},
    { "maint",          LVL_CREATOR,    "etc/DELETED" },
    { "syslog",         LVL_LOWIMPL,    "../syslog" },
    { "crash",          LVL_LOWIMPL,    "../syslog.CRASH" },
    { "pk",             LVL_GRGOD,   "../log/pk" },   
    { "denied",         LVL_GRGOD,   "../log/denied" },
    { "\n", 0, "\n" }
  };

  skip_spaces(&argument);
  if (!*argument) {
    strcpy(buf, "USAGE: file <option> <num lines (Max 75)>\r\n\r\nFile options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch)) {
	sprintf(buf, "%-15s%s\r\n", fields[i].cmd, fields[i].file);
	send_to_char(buf, ch);
      }
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));
  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if(*(fields[l].cmd) == '\n') {
    send_to_char("That is not a valid option!\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("You are not godly enough to view that file!\r\n", ch);
    return;
  }

  if(!*value)
    req_lines = 15; /* default is the last 15 lines */
  else
    req_lines = atoi(value);

  /* open the requested file */
  if (!(req_file=fopen(fields[l].file,"r"))) {
    mudlogf(BRF, LVL_GRGOD, FALSE, "SYSERR: Error opening file %s using 'file' command.",
	    fields[l].file);
    return;
  }

  /* count lines in requested file */
  get_line(req_file,buf);
  while (!feof(req_file)) {
    num_lines++;
    get_line(req_file,buf);
  }
  fclose(req_file);

  /* Limit # of lines printed to # requested or # of lines in file or 75 lines */
  if(req_lines > num_lines) req_lines = num_lines;
  if(req_lines > 75) req_lines = 75;

  /* close and re-open */
  if (!(req_file=fopen(fields[l].file,"r"))) {
    mudlogf(BRF, LVL_IMPL, TRUE, "SYSERR: Error opening file %s using 'file' command.",
	    fields[l].file );
    return;
  }

  buf2[0] = '\0';

  /* and print the requested lines */
  get_line(req_file,buf);
  while (!feof(req_file)) {
    cur_line++;
    if(cur_line > (num_lines - req_lines)) {
      sprintf(buf2,"%s%s\r\n",buf2, buf);
    }
    get_line(req_file,buf);
  }
  page_string(ch->desc, buf2, 1);
  fclose(req_file);
}

/* New Quest Commands */
ACMD(do_qpoints) {

  struct char_data *vict;
  int value;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Give quest points to who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
    return;
  }

  one_argument (buf, buf2);
  if (!*buf2) {
    send_to_char("How many quest points do you wish to give?\r\n", ch);
    return;
  }

  if (is_number(buf2))
    value = atoi(buf2);
  else {
    send_to_char ("Sorry but you can do that!\r\n", ch);
    return;
  }

  if (value <= 0) {
    send_to_char("Tight Git, give them more than that!\r\n", ch);
    return;
  }

  GET_QUEST(vict) += value;
  sprintf(buf,"You have just been given %d quest points by the Gods!", value);
  send_to_char(buf, vict);
  sprintf(buf, "You give %s %d Quest points.\r\n", GET_NAME(vict), value);
  send_to_char(buf, ch);

  mudlogf(NRM, MAX(LVL_ARCHWIZ, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s has given %d Quest Points to %s.", GET_NAME(ch),
	  value, GET_NAME(vict) );
}

ACMD(do_newquest) {

  struct descriptor_data *pt;
  int value;

  half_chop(argument, arg, buf);

  if (!*arg || !*buf) {
    send_to_char("Usage: newquest <points> <name>\r\n", ch);
    return;
  }

  if (is_number(arg))
    value = atoi (arg);
  else {
    send_to_char("You must specity the amount of points the quest is worth.\r\n", ch);
    return;
  }

  for (pt = descriptor_list; pt; pt = pt->next)
    if (!pt->connected && pt->character && pt->character != ch) {
      sprintf(buf2, "%sQUEST:: %s (Worth %d Quest Points)%s\r\n",
	      CCYEL(pt->character, C_NRM), buf, value, CCNRM(pt->character, C_NRM));
      send_to_char(buf2, pt->character);
    }
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    sprintf(buf2, "%sQUEST:: %s (Worth %d Quest Points)%s\r\n",
	    CCYEL(ch, C_NRM), buf2, value, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
  }
}

ACMD(do_vitem)
{
  one_argument(argument, buf);

  if (!*buf) {
    send_to_char("Usage: vitem <wear position> | <Item Type>\r\n\r\n"
		 "Wear positions are:\r\n\r\n"
		 "finger  neck   body   head   legs    feet    hands\r\n"
		 "shield  arms   about  waist  wrist   wield   hold\r\n"
		 "face    ears   eyes   hair   ankle\r\n"
		 "------------------------------------------------------\r\n"
		 "Item types are:\r\n\r\n"
		 "light   scroll  wand   staff  treasure  armour  \r\n"
		 "potion  worn    other  trash  container liquid \r\n"
		 "key     food    money  pen    boat      fountain\r\n", ch);

    return;
  }
  if (is_abbrev(buf, "finger"))
    vwear_object(ITEM_WEAR_FINGER, ch);
  else if (is_abbrev(buf, "neck"))
    vwear_object(ITEM_WEAR_NECK, ch);
  else if (is_abbrev(buf, "body"))
    vwear_object(ITEM_WEAR_BODY, ch);
  else if (is_abbrev(buf, "head"))
    vwear_object(ITEM_WEAR_HEAD, ch);
  else if (is_abbrev(buf, "legs"))
    vwear_object(ITEM_WEAR_LEGS, ch);
  else if (is_abbrev(buf, "feet"))
    vwear_object(ITEM_WEAR_FEET, ch);
  else if (is_abbrev(buf, "hands"))
    vwear_object(ITEM_WEAR_HANDS, ch);
  else if (is_abbrev(buf, "arms"))
    vwear_object(ITEM_WEAR_ARMS, ch);
  else if (is_abbrev(buf, "shield"))
    vwear_object(ITEM_WEAR_SHIELD, ch);
  else if (is_abbrev(buf, "about body"))
    vwear_object(ITEM_WEAR_ABOUT, ch);
  else if (is_abbrev(buf, "waist"))
    vwear_object(ITEM_WEAR_WAIST, ch);
  else if (is_abbrev(buf, "wrist"))
    vwear_object(ITEM_WEAR_WRIST, ch);
  else if (is_abbrev(buf, "wield"))
    vwear_object(ITEM_WEAR_WIELD, ch);
  else if (is_abbrev(buf, "hold"))
    vwear_object(ITEM_WEAR_HOLD, ch);
  else if (is_abbrev(buf, "face"))
    vwear_object(ITEM_WEAR_FACE, ch);
  else if (is_abbrev(buf, "ears"))
    vwear_object(ITEM_WEAR_EAR, ch);
  else if (is_abbrev(buf, "eyes"))
    vwear_object(ITEM_WEAR_EYES, ch);
  else if (is_abbrev(buf, "hair"))
    vwear_object(ITEM_WEAR_HAIR, ch);
  else if (is_abbrev(buf, "ankle"))
    vwear_object(ITEM_WEAR_ANKLE, ch);
  else if (is_abbrev(buf, "light"))
    vwear_obj(ITEM_LIGHT, ch);
  else if (is_abbrev(buf, "scroll"))
    vwear_obj(ITEM_SCROLL, ch);
  else if (is_abbrev(buf, "wand"))
    vwear_obj(ITEM_WAND, ch);
  else if (is_abbrev(buf, "staff"))
    vwear_obj(ITEM_STAFF, ch);
  else if (is_abbrev(buf, "treasure"))
    vwear_obj(ITEM_TREASURE, ch);
  else if (is_abbrev(buf, "armor"))
    vwear_obj(ITEM_ARMOR, ch);
  else if (is_abbrev(buf, "potion"))
    vwear_obj(ITEM_POTION, ch);
  else if (is_abbrev(buf, "worn"))
    vwear_obj(ITEM_WORN, ch);
  else if (is_abbrev(buf, "other"))
    vwear_obj(ITEM_OTHER, ch);
  else if (is_abbrev(buf, "trash"))
    vwear_obj(ITEM_TRASH, ch);
  else if (is_abbrev(buf, "container"))
    vwear_obj(ITEM_CONTAINER, ch);
  else if (is_abbrev(buf, "liquid"))
    vwear_obj(ITEM_DRINKCON, ch);
  else if (is_abbrev(buf, "key"))
    vwear_obj(ITEM_KEY, ch);
  else if (is_abbrev(buf, "food"))
    vwear_obj(ITEM_FOOD, ch);
  else if (is_abbrev(buf, "money"))
    vwear_obj(ITEM_MONEY, ch);
  else if (is_abbrev(buf, "pen"))
    vwear_obj(ITEM_PEN, ch);
  else if (is_abbrev(buf, "boat"))
    vwear_obj(ITEM_BOAT, ch);
  else if (is_abbrev(buf, "fountain"))
    vwear_obj(ITEM_FOUNTAIN, ch);
  else
    send_to_char("Possible positions are:\r\n"
		 "Wear positions are:\r\n"
		 "finger  neck   body   head   legs    feet    hands\r\n"
		 "shield  arms   about  waist  wrist   wield   hold\r\n"
		 "face    ears   eyes   hair\r\n"
		 "------------------------------------------------------\r\n"
		 "Item types are:\r\n"
		 "light   scroll  wand   staff  treasure  armour  \r\n"
		 "potion  worn    other  trash  container liquid \r\n"
		 "key     food    money  pen    boat      fountain\r\n", ch);
}



ACMD(do_xname)
{
  char tempname[MAX_INPUT_LENGTH];
  int i = 0;
  FILE *fp;

  one_argument(argument, buf);

  if (!*buf)
    send_to_char("Xname which name?\r\n", ch);
  else if (!(fp = fopen(XNAME_FILE, "a"))) {
    perror("Problems opening xname file for do_xname\r\n");
    return;
  } else {
    strcpy(tempname, buf);
    for (i = 0; tempname[i]; i++)
      tempname[i] = LOWER(tempname[i]);
    fprintf(fp, "%s\n", tempname);
    fclose(fp);
    send_to_char(OK, ch);
    Read_Invalid_List();
  }
}


ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes.. but what?\r\n", ch);
  else {
    if (subcmd == SCMD_EMOTE)
      sprintf(buf, "$n %s", argument);
    else
      strcpy(buf, argument);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}

ACMD(do_liblist)
{
  int first, last, nr, found = 0;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2) {
    switch (subcmd) {
    case SCMD_RLIST:
      send_to_char("Usage: rlist <begining number> <ending number>\r\n", ch);
      break;
    case SCMD_OLIST:
      send_to_char("Usage: olist <begining number> <ending number>\r\n", ch);
      break;
    case SCMD_MLIST:
      send_to_char("Usage: mlist <begining number> <ending number>\r\n", ch);
      break;
    default:
      mudlogf(BRF, LVL_GOD, TRUE, "SYSERR:: invalid SCMD passed to ACMDdo_build_list!");
      break;
    }
    return;
  }

  first = atoi(buf);
  last = atoi(buf2);

  if (first/100 != last/100) {
    send_to_char("Stick to one zone!\r\n", ch);
    return;
  }

  if ((first < 0) || (first > 99999) || (last < 0) || (last > 99999)) {
    send_to_char("Values must be between 0 and 99999.\n\r", ch);
    return;
  }

  if (first >= last) {
    send_to_char("Second value must be greater than first.\n\r", ch);
    return;
  }

  if (!has_zone_permission(last, LVL_IMMORT, ch)) {
    send_to_char("You don't have permission for that zone.\r\n", ch);
    return;
  }

  switch (subcmd) {
  case SCMD_RLIST:
    sprintf(buf, "Room List From Vnum %d to %d\r\n", first, last);
    for (nr = 0; nr <= top_of_world && (world[nr].number <= last); nr++) {
      if (world[nr].number >= first) {
	sprintf(buf, "%s%5d. [%5d] (%3d) %s\r\n", buf, ++found,
		world[nr].number, world[nr].zone,
		world[nr].name);
      }
    }
    break;
  case SCMD_OLIST:
    sprintf(buf, "Object List From Vnum %d to %d\r\n", first, last);
    for (nr = 0; nr <= top_of_objt && (obj_index[nr].virtual <= last); nr++) {
      if (obj_index[nr].virtual >= first) {
	sprintf(buf, "%s%5d. [%5d] %s\r\n", buf, ++found,
		obj_index[nr].virtual,
		obj_proto[nr].short_description);
      }
    }
    break;
  case SCMD_MLIST:
    sprintf(buf, "Mob List From Vnum %d to %d\r\n", first, last);
    for (nr = 0; nr <= top_of_mobt && (mob_index[nr].virtual <= last); nr++) {
      if (mob_index[nr].virtual >= first) {
	sprintf(buf, "%s%5d. [%5d] %s\r\n", buf, ++found,
		mob_index[nr].virtual,
		mob_proto[nr].player.short_descr);
      }
    }
    break;
  default:
    mudlogf(BRF, LVL_GOD, TRUE, "SYSERR:: invalid SCMD passed to liblist!");
    return;
  }

  if (!found) {
    switch (subcmd) {
    case SCMD_RLIST:
      send_to_char("No rooms found within those parameters.\r\n", ch);
      break;
    case SCMD_OLIST:
      send_to_char("No objects found within those parameters.\r\n", ch);
      break;
    case SCMD_MLIST:
      send_to_char("No mobiles found within those parameters.\r\n", ch);
      break;
    default:
      mudlogf(BRF, LVL_GOD, TRUE, "SYSERR:: invalid SCMD passed to do_build_list!");
      break;
    }
    return;
  }
  page_string(ch->desc, buf, 1);
}


ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Send what to who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
    return;
  } else if ((IS_SET(ROOM_FLAGS(vict->in_room), ROOM_LOCKED_DOWN)) 
	     && (GET_LEVEL(ch) != LVL_LOWIMPL)) {
    send_to_char("You can't do that right now.\r\n", ch);
    return;
  }
  send_to_char(buf, vict);
  send_to_char("\r\n", vict);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char("Sent.\r\n", ch);
  else {
    sprintf(buf2, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
    send_to_char(buf2, ch);
  }
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH];
  int location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char("You must supply a room number or a name.\r\n", ch);
    return;
  }

  if (!*command) {
    send_to_char("What do you want to do there?\r\n", ch);
    return;
  }

  if ((location = find_target_room(ch, buf)) < 0)
    return;

  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}


ACMD(do_goto)
{
  sh_int location;

  if ((location = find_target_room(ch, argument)) < 0)
    return;
  else if ((IS_SET(ROOM_FLAGS(location), ROOM_LOCKED_DOWN)) 
	   && (GET_LEVEL(ch) != LVL_LOWIMPL)) {
    send_to_char("You can't go there right now.\r\n", ch);
    return;
  }

  if(RIDING(ch)) {
    act("$n disappears in a puff of red smoke!", TRUE, RIDING(ch), 0, 0, TO_ROOM);
    char_from_room(RIDING(ch));
    char_to_room(RIDING(ch), location);
    act("$n appears in a puff of purple smoke!", TRUE, RIDING(ch), 0, 0, TO_ROOM);
  }

  if (POOFOUT(ch))
    sprintf(buf, "$n %s", POOFOUT(ch));
  else
    strcpy(buf, "$n explodes in a ball of green flames and is gone.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);

  if (POOFIN(ch))
    sprintf(buf, "$n %s", POOFIN(ch));
  else
    strcpy(buf, "$n suddenly appears out of a cloud of white mist.");

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}

ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to transfer?\r\n", ch);
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf)))
      send_to_char(NOPERSON, ch);
    else if ((IS_SET(ROOM_FLAGS(victim->in_room), ROOM_LOCKED_DOWN)) 
	     && (GET_LEVEL(ch) != LVL_LOWIMPL))
      send_to_char("You are thwarted by higher powers!\r\n", ch);
    else if (victim == ch)
      send_to_char("That doesn't make much sense, does it?\r\n", ch);
    else {
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char("Go transfer someone your own size.\r\n", ch);
	return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      if(RIDING(victim))
	dismount_char(victim);
      char_from_room(victim);
      char_to_room(victim, ch->in_room);
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(victim, 0);
    }
  } else {			/* Trans All */
    if (GET_LEVEL(ch) < LVL_SUPREME) {
      send_to_char("I think not.\r\n", ch);
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i->character && i->character != ch) {
	victim = i->character;
	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
	  continue;
	else if ((IS_SET(ROOM_FLAGS(victim->in_room), ROOM_LOCKED_DOWN)) 
		 && (GET_LEVEL(ch) != LVL_LOWIMPL))
	  continue;
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, ch->in_room);
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
      }
    send_to_char(OK, ch);
  }
}

ACMD(do_teleport)
{
  struct char_data *victim;
  sh_int target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char("Whom do you wish to teleport?\r\n", ch);
  else if (!(victim = get_char_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (victim == ch)
    send_to_char("Use 'goto' to teleport yourself.\r\n", ch);
  else if (GET_LEVEL(victim) >= GET_LEVEL(ch))
    send_to_char("Maybe you shouldn't do that.\r\n", ch);
  else if (!*buf2)
    send_to_char("Where do you wish to send this person?\r\n", ch);
  else if ((target = find_target_room(ch, buf2)) >= 0) {
    if (((IS_SET(ROOM_FLAGS(target), ROOM_LOCKED_DOWN)) 
	 || (IS_SET(ROOM_FLAGS(victim->in_room), ROOM_LOCKED_DOWN)))
	&& (GET_LEVEL(ch) != LVL_LOWIMPL)) {
      send_to_char("You are thwarted by higher powers!\r\n", ch);
      return;
    }
    send_to_char(OK, ch);
    if(RIDING(victim))
      dismount_char(victim);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(victim, 0);
  }
}



ACMD(do_vnum)
{
  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj"))) {
    send_to_char("Usage: vnum { obj | mob } <name>\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(buf2, ch))
      send_to_char("No mobiles by that name.\r\n", ch);

  if (is_abbrev(buf, "obj"))
    if (!vnum_object(buf2, ch))
      send_to_char("No objects by that name.\r\n", ch);
}



void do_stat_room(struct char_data * ch)
{
  struct extra_descr_data *desc;
  struct room_data *rm = &world[ch->in_room];
  int i, found = 0;
  struct obj_data *j = 0;
  struct char_data *k = 0;

  sprintf(buf, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name,
	  CCNRM(ch, C_NRM));

  sprinttype(rm->sector_type, sector_types, buf2);
  sprintf(buf, "%sZone: [%3d], VNum: [%s%5d%s], RNum: [%5d], Type: %s\r\n", buf,
	  zone_table[rm->zone].number, CCGRN(ch, C_NRM), rm->number,
	  CCNRM(ch, C_NRM), ch->in_room, buf2);

  sprintbit((long) rm->room_flags, room_bits, buf2);
  sprintf(buf, "%sSpecProc: %s, Flags: %s\r\n", buf,
	  (rm->func == NULL) ? "None" : "Exists", buf2);

  send_to_char(buf, ch);

  send_to_char("Description:\r\n", ch);
  if (rm->description)
    send_to_char(rm->description, ch);
  else
    send_to_char("  None.\r\n", ch);

  if (rm->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }

  sprintf(buf, "Chars present:%s", CCYEL(ch, C_NRM));
  for (found = 0, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;
    sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k),
	    (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (k->next_in_room)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "\r\n"), ch);
  send_to_char(CCNRM(ch, C_NRM), ch);

  if (rm->contents) {
    sprintf(buf, "Contents:%s", CCGRN(ch, C_NRM));
    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;
      sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (rm->dir_option[i]) {
      if (rm->dir_option[i]->to_room == NOWHERE)
	sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
      else
	sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM),
		world[rm->dir_option[i]->to_room].number, CCNRM(ch, C_NRM));
      sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);

      sprintf(buf, "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n ",
	      CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1, rm->dir_option[i]->key,
	      rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None",
	      buf2);
      send_to_char(buf, ch);

      if (rm->dir_option[i]->general_description)
	strcpy(buf, rm->dir_option[i]->general_description);
      else
	strcpy(buf, "  No exit description.\r\n");
      send_to_char(buf, ch);
    }
  }
}

void do_stat_object(struct char_data * ch, struct obj_data * j)
{
  int i, virtual, found;
  struct obj_data *j2;
  struct extra_descr_data *desc;
  char name[MAX_NAME_LENGTH+1];

  virtual = GET_OBJ_VNUM(j);
  sprintf(buf, "Name: '%s%s%s', Aliases: %s\r\n", CCYEL(ch, C_NRM),
	  ((j->short_description) ? j->short_description : "<None>"),
	  CCNRM(ch, C_NRM), j->name);
  send_to_char(buf, ch);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
  if (GET_OBJ_RNUM(j) >= 0)
    strcpy(buf2, (obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None"));
  else
    strcpy(buf2, "None");

  sprintf(buf, "VNum: [%s%5d%s], RNum: [%5d], Type: %s, SpecProc: %s\r\n",
	  CCGRN(ch, C_NRM), virtual, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), buf1, buf2);
  sprintf(buf, "%sL-Des: %s\r\n", buf, ((j->description) ? j->description : "None"));
  send_to_char(buf, ch);

  if (j->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = j->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  send_to_char("Can be worn on: ", ch);
  sprintbit(j->obj_flags.wear_flags, wear_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Set char bits : ", ch);
  sprintbit(j->obj_flags.bitvector, affected_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("Extra flags   : ", ch);
  sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  if (GET_OBJ_ENGRAVE(j)) {
    strcpy(name, (get_name_by_id((GET_OBJ_ENGRAVE(j)))));
    sprintf(buf, "Engraved to   : %s.\r\n", CAP(name));
    send_to_char(buf, ch);
  }

  sprintf(buf, "Weight: %d, Value: %d, Extra Attacks: %d, Timer: %d, Level: %d\r\n",
	  GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_ATT(j), GET_OBJ_TIMER(j), GET_OBJ_LEVEL(j));
  send_to_char(buf, ch);
  strcpy(buf, "In room: ");
  if (j->in_room == NOWHERE)
    strcat(buf, "Nowhere");
  else {
    sprintf(buf2, "%d", world[j->in_room].number);
    strcat(buf, buf2);
  }
  strcat(buf, ", In object: ");
  strcat(buf, j->in_obj ? j->in_obj->short_description : "None");
  strcat(buf, ", Carried by: ");
  strcat(buf, j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
  strcat(buf, ", Worn by: ");
  strcat(buf, j->worn_by ? GET_NAME(j->worn_by) : "Nobody");
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, 2) == -1)
      strcpy(buf, "Hours left: Infinite");
    else
      sprintf(buf, "Hours left: [%d]", GET_OBJ_VAL(j, 2));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    sprintf(buf, "Spells: (Level %d) %s, %s, %s", GET_OBJ_VAL(j, 0),
	    skill_name(GET_OBJ_VAL(j, 1)), skill_name(GET_OBJ_VAL(j, 2)),
	    skill_name(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    sprintf(buf, "Spell: %s at level %d, %d (of %d) charges remaining",
	    skill_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 0),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_WEAPON:
    sprintf(buf, "Todam: %dd%d, Message type: %d",
	    GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  case ITEM_ARMOR:
    sprintf(buf, "AC-apply: [%d]", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_TRAP:
    sprintf(buf, "Spell: %d, - Hitpoints: %d",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1));
    break;
  case ITEM_CONTAINER:
    sprintbit(GET_OBJ_VAL(j, 1), container_bits, buf2);
    sprintf(buf, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s",
	    GET_OBJ_VAL(j, 0), buf2, GET_OBJ_VAL(j, 2),
	    YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
    sprintf(buf, "Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), YESNO(GET_OBJ_VAL(j, 3)),
	    buf2);
    break;
  case ITEM_NOTE:
    sprintf(buf, "Tongue: %d", GET_OBJ_VAL(j, 0));
    break;
  case ITEM_KEY:
    strcpy(buf, "");
    break;
  case ITEM_FOOD:
    sprintf(buf, "Makes full: %d, Poisoned: %s", GET_OBJ_VAL(j, 0),
	    YESNO(GET_OBJ_VAL(j, 3)));
    break;
  case ITEM_MONEY:
    sprintf(buf, "Coins: %d", GET_OBJ_VAL(j, 0));
    break;

  case ITEM_FIREWEAPON:
    sprintf(buf, "Takes %s for damage of %dhp, Current Ammo: %d, Max Ammo: %d\r\n",
	    shot_types[(GET_OBJ_VAL(j, 0))], shot_damage[(GET_OBJ_VAL(j, 0))],
	    GET_OBJ_VAL(j, 3), GET_OBJ_VAL(j, 2));
    break;

  case ITEM_CLIP:
    sprintf(buf, "Item is %s clip for damage of %dhp per round of ammo, Holds: %d\r\n",
	    shot_types[(GET_OBJ_VAL(j, 0))], shot_damage[(GET_OBJ_VAL(j, 0))],
	    GET_OBJ_VAL(j, 3));
    break;

  default:
    sprintf(buf, "Values 0-3: [%d] [%d] [%d] [%d]",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3));
    break;
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (j->contains) {
    sprintf(buf, "\r\nContents:%s", CCGRN(ch, C_NRM));
    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
	if (j2->next_content)
	  send_to_char(strcat(buf, ",\r\n"), ch);
	else
	  send_to_char(strcat(buf, "\r\n"), ch);
	*buf = found = 0;
      }
    }

    if (*buf)
      send_to_char(strcat(buf, "\r\n"), ch);
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  found = 0;
  send_to_char("Affections:", ch);
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      sprinttype(j->affected[i].location, apply_types, buf2);
      sprintf(buf, "%s %+d to %s", found++ ? "," : "",
	      j->affected[i].modifier, buf2);
      send_to_char(buf, ch);
    }
  if (!found)
    send_to_char(" None", ch);

  send_to_char("\r\n", ch);
}


void do_stat_character(struct char_data * ch, struct char_data * k)
{
  int i, i2, found = 0;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;
  char name[MAX_NAME_LENGTH+1];

  switch (GET_SEX(k)) {
  case SEX_NEUTRAL:    strcpy(buf, "NEUTRAL-SEX");   break;
  case SEX_MALE:       strcpy(buf, "MALE");          break;
  case SEX_FEMALE:     strcpy(buf, "FEMALE");        break;
  default:             strcpy(buf, "ILLEGAL-SEX!!"); break;
  }

  if (IS_MOB(k)) {    
    sprintf(buf2, "%s MOB, '%s.'\r\n", buf, GET_NAME(k));
    send_to_char(buf2, ch);
    
    sprintf(buf, "Alias: %s, VNum: [&y%5d&n], RNum: [&y%5d&n], In Room: [&y%5d&n]\r\n",
	    k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k),
	    k->in_room != NOWHERE ? world[k->in_room].number : NOWHERE);
    send_to_char(buf, ch);
    
    sprintf(buf, "L-Des: %s", (k->player.long_descr ? k->player.long_descr : "<None>\r\n"));
    send_to_char(buf, ch);
  } else { 
    sprintf(buf2, "[&g%ld&n] %s PC, '&c%s %s&n'\r\n", GET_IDNUM(k), buf, GET_NAME(k),
	    (k->player.title ? k->player.title : ""));
    send_to_char(buf2, ch);
  }
  
  if (IS_NPC(k)) {
    strcpy(buf, "Monster Class: ");
    sprinttype(k->player.class, npc_class_types, buf2);
  } else {
    strcpy(buf, "Class: &y");
    sprinttype(k->player.class, pc_class_types, buf2);
  }
  strcat(buf, buf2);
  
  sprintf(buf2, "&n, Level: [%s%3d%s], Exp: [%s%7d%s], Alignment: [&y%d&n]\r\n",
	  CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM),
	  CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM), GET_ALIGNMENT(k));
  
  strcat(buf, buf2);
  send_to_char(buf, ch);
  
  if (!IS_NPC(k)) {
    strcpy(buf1, (char *) asctime(localtime(&(k->player.time.birth))));
    strcpy(buf2, (char *) asctime(localtime(&(k->player.time.logon))));
    buf1[10] = buf2[10] = '\0';

    sprintf(buf, "Created: [&c%s&n], Last Logon: [&c%s&n], Played [&r%dh %dm&n]\r\n",
	    buf1, buf2, k->player.time.played / 3600,
	    ((k->player.time.played / 3600) % 60));
    send_to_char(buf, ch);

    if (GET_MARRIED(k))
      strcpy(name, (get_name_by_id(GET_MARRIED(k))));
    else
      strcpy(name, "Nobody");
    
    sprintf(buf, "Age [&r%d&n], Thirst: [&r%d&n], Hunger: [&r%d&n], Drunk: [&r%d&n], &yMarried to %s.&n\r\n",
	    age(k).year, GET_COND(k, THIRST), GET_COND(k, FULL), 
	    GET_COND(k, DRUNK), CAP(name));
    send_to_char(buf, ch); 

    sprintf(buf, "Hometown: [&g%d&n], PK: [&g%3s&n], QPoints: [&g%d&n], Gohome: [&g%d&n]\r\n", 
	    k->player.class, (PLR_FLAGGED(k, PLR_PK) ? "On" : "Off"),
	    GET_QUEST(k), GET_GOHOME(k));

    send_to_char(buf,ch);

    sprintf (buf, "Speaks: [%s ", CCMAG(ch, C_NRM));
    for (i = LANG_COMMON; i <= MAX_LANG; i++)
      if (GET_SKILL(k, i) > 0)
	sprintf(buf, "%s%s ", buf, languages[i - LANG_COMMON]);

    sprintf(buf, "%s%s]\r\n", buf, CCNRM(ch, C_NRM));
    send_to_char(buf, ch); 
    
  }

  /* ------ End of PC only bit ---------- */

  sprintf(buf, "Coins: [&y%9d&n], Bank: [&y%9d&n], (Total: &y%d&n)\r\n",
	  GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));
  send_to_char(buf, ch); 
  
  sprintf(buf, "Hitp: [&r%5d&n/&g%-5d&n], Mana: [&r%5d&n/&g%-5d&n], Move: [&r%5d&n/&g%-5d&n]\r\n",
	  GET_HIT(k), GET_MAX_HIT(k), GET_MANA(k), GET_MAX_MANA(k),
	  GET_MOVE(k), GET_MAX_MOVE(k) );
  send_to_char(buf, ch);
  
  sprintf(buf, "Str: [&c%d&n/&y%d&n]  Int: [&c%d&n]  Wis: [&c%d&n]  "
	  "Dex: [&c%d&n]  Con: [&c%d&n]  End: [&c%d&n]\r\n",
	  GET_STR(k), GET_ADD(k), GET_INT(k), GET_WIS(k),
	  GET_DEX(k), GET_CON(k), GET_END(k));
  send_to_char(buf, ch);
  
  sprintf(buf, "AC: [&y%d/10&n], Hitroll: [&y%2d&n], Damroll: [&y%2d&n], "
	  "Saving throws: [&y%d/%d/%d/%d/%d&n]\r\n",
	  GET_AC(k), k->points.hitroll, k->points.damroll, GET_SAVE(k, 0),
	  GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3), GET_SAVE(k, 4));
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    if (GET_LEVEL(k) >= LVL_OVERSEER) {
      sprintf (buf, "OLC:  A: [&g%d&n], B: [&g%d&n] C: [&g%d&n], Office: [&g%d&n]\r\n", 
	       GET_OLC_ZONE1(k), GET_OLC_ZONE2(k), GET_OLC_ZONE3(k),
	       GET_OFFICE(k));
      send_to_char(buf,ch);
    }
  }

  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf, "Pos: &m%s&n, Fighting: &m%s&n", buf2,
	  (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));
  
  if (k->desc) {
    sprinttype(k->desc->connected, connected_types, buf2);
    strcat(buf, ", Connected: &m");
    strcat(buf, buf2);
  }
  
  if (IS_NPC(k))
    sprintf(buf, "%s, Attack type: &m%s&n", buf, attack_hit_text[k->mob_specials.attack_type].singular);
  else 
    sprintf(buf, "%s&n, Idle: [&m%d&n]", buf, k->char_specials.timer);
  
  send_to_char(strcat(buf, "\r\n"), ch);

  sprintf(buf, "Carried: weight: &r%d&n, items: &r%d&n; ",
	  IS_CARRYING_W(k), IS_CARRYING_N(k));
  
  for (i = 0, j = k->carrying; j; j = j->next_content, i++);
  sprintf(buf, "%sItems in: inventory: &r%d&n, ", buf, i);
  
  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      i2++;
  sprintf(buf2, "eq: &r%d&n\r\n", i2);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (IS_MOB(k)) {
    sprintf(buf, "Mob Spec-Proc: &y%s&n, NPC Bare Hand Dam: &y%dd%d&n, Attacks: &y%d&n\r\n",
	    (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
	    k->mob_specials.damnodice, k->mob_specials.damsizedice, GET_MOB_ATTACKS(k));
    send_to_char(buf, ch);
  }

  sprintf(buf, "Master is: &c%s&n, Followers are:&g",
	  ((k->master) ? GET_NAME(k->master) : "<none>"));
  
  for (fol = k->followers; fol; fol = fol->next) {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (fol->next)
	send_to_char(strcat(buf, ",\r\n"), ch);
      else
	send_to_char(strcat(buf, "&n\r\n"), ch);
      *buf = found = 0;
    }
  }

  if (*buf)
    send_to_char(strcat(buf, "&n\r\n"), ch);

  
  if (IS_NPC(k)) {
    sprintbit(MOB_FLAGS(k), action_bits, buf2);
    sprintf(buf, "NPC flags: %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
  } else {
    sprintbit(PRF_FLAGS(k), preference_bits, buf2);
    sprintf(buf, "PRF: [ &y%s&n]\r\n", buf2);
    send_to_char(buf, ch);
    sprintbit(TMP_FLAGS(k), temp_bits, buf1);
    sprintbit(PLR_FLAGS(k), player_bits, buf2);
    sprintf(buf, "PLR: [ &g%s&n], TMP: [ &c%s&n]\r\n", buf2, buf1);
    send_to_char(buf, ch);
  }

  /* Showing the bitvector */
  sprintbit(AFF_FLAGS(k), affected_bits, buf2);
  sprintf(buf, "AFF: %s%s%s\r\n", CCMAG(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  /* Routine to show what spells a char is affected by */
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      *buf2 = '\0';
      sprintf(buf, "SPL: (%3dhr) %s%-21s%s ", aff->duration + 1,
	      CCCYN(ch, C_NRM), (aff->type >= 0 && aff->type <= MAX_SPELLS) ?
	      spells[aff->type] : "TYPE UNDEFINED", CCNRM(ch, C_NRM));

      if (aff->modifier) {
	sprintf(buf2, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);
	strcat(buf, buf2);
      }
      if (aff->bitvector) {
	if (*buf2)
	  strcat(buf, ", sets ");
	else
	  strcat(buf, "sets ");
	sprintbit(aff->bitvector, affected_bits, buf2);
	strcat(buf, buf2);
      }
      send_to_char(strcat(buf, "\r\n"), ch);
    }
  }

}


ACMD(do_stat)
{
  struct char_data *victim = 0;
  struct obj_data *object = 0;
  struct char_file_u tmp_store;
  int tmp;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    do_stat_character(ch, ch);
  } else if (is_abbrev(buf1, "room")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char("Stats on which mobile?\r\n", ch);
    else {
      if ((victim = get_char_vis(ch, buf2)))
	do_stat_character(ch, victim);
      else
	send_to_char("No such mobile around.\r\n", ch);
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      if ((victim = get_player_vis(ch, buf2, 0)))
	do_stat_character(ch, victim);
      else
	send_to_char("No such player around.\r\n", ch);
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char(buf2, &tmp_store) > -1) {
	store_to_char(&tmp_store, victim);
	char_to_room(victim, 0);
	if (GET_LEVEL(victim) > GET_LEVEL(ch))
	  send_to_char("Sorry, you can't do that.\r\n", ch);
	else
	  do_stat_character(ch, victim);
	extract_char(victim);
      } else {
	send_to_char("There is no such player.\r\n", ch);
	free(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char("Stats on which object?\r\n", ch);
    else {
      if ((object = get_obj_vis(ch, buf2)))
	do_stat_object(ch, object);
      else
	send_to_char("No such object around.\r\n", ch);
    }
  } else {
    if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp)))
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying)))
      do_stat_object(ch, object);
    else if ((victim = get_char_room_vis(ch, buf1)))
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room].contents)))
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, buf1)))
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, buf1)))
      do_stat_object(ch, object);
    else
      send_to_char("Nothing around by that name.\r\n", ch);
  }
}


ACMD(do_shutdown)
{
  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char("If you want to shut something down, say so!\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    touch("../.fastboot");
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "now")) {
    log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    circle_shutdown = 1;
    circle_reboot = 2;
  } else if (!str_cmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch("../.killscript");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch("../pause");
    circle_shutdown = 1;
  } else
    send_to_char("Unknown shutdown option.\r\n", ch);
}


void stop_snooping(struct char_data * ch)
{
  if (!ch->desc->snooping)
    send_to_char("You aren't snooping anyone.\r\n", ch);
  else {
    send_to_char("You stop snooping.\r\n", ch);
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}


ACMD(do_snoop)
{
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg)))
    send_to_char("No such person around.\r\n", ch);
  else if (!victim->desc)
    send_to_char("There's no link.. nothing to snoop.\r\n", ch);
  else if (victim == ch)
    stop_snooping(ch);
  else if ((IS_SET(ROOM_FLAGS(victim->in_room), ROOM_LOCKED_DOWN)) 
	   && (GET_LEVEL(ch) != LVL_LOWIMPL))
    send_to_char("That person seems to be shielded to you.\r\n", ch);
  else if (victim->desc->snoop_by)
    send_to_char("Busy already. \r\n", ch);
  else if (victim->desc->snooping >= ch->desc)
    send_to_char("Don't be stupid.\r\n", ch);
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_LEVEL(tch) >= GET_LEVEL(ch) && GET_IDNUM(ch) != 1) {
      send_to_char("You can't.\r\n", ch);
      return;
    }
    send_to_char(OK, ch);

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}



ACMD(do_switch)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (GET_LEVEL(ch) < LVL_SWITCH && !PLR_FLAGGED(ch, PLR_SWITCH)) {
    send_to_char("Switch has not been enabled for you, get an Implementor to enable it.\r\n", ch);
    return;
  }

  if (ch->desc->original)
    send_to_char("You're already switched.\r\n", ch);
  else if (!*arg)
    send_to_char("Switch with who?\r\n", ch);
  else if (!(victim = get_char_vis(ch, arg)))
    send_to_char("No such character.\r\n", ch);
  else if (ch == victim)
    send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
  else if (victim->desc)
    send_to_char("You can't do that, the body is already in use!\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_CREATOR) && !IS_NPC(victim)) 
    send_to_char("You aren't holy enough to use a mortal's body.\r\n", ch);
  /* altered by Nahaz to prevent lower levels switching higher levels. */
  else if ((GET_LEVEL(ch) >= LVL_CREATOR) && !IS_NPC(victim)
	   && (GET_LEVEL(ch) <= GET_LEVEL(victim))) 
    send_to_char("You are not godly enough for that.\r\n", ch);
  else {
    send_to_char(OK, ch);
    
    ch->desc->character = victim;
    ch->desc->original = ch;
    
    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}


ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char("You return to your original body.\r\n", ch);

    /* JE 2/22/95 */
    /* if someone switched into your original body, disconnect them */
    if (ch->desc->original->desc)
      STATE(ch->desc->original->desc) = CON_DISCONNECT;


    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}

ACMD(do_load)
{

  struct char_data *mob;
  struct obj_data *obj;
  int number = 0, r_num;
  int type = 0;
  int tmp;

  /* type 0 = unspecified, 1 = mob, 2 = obj */

  half_chop(argument, buf, buf2);

  if( !*buf || isdigit(*buf) || *buf == '-' ) {
    send_to_char("Usage: load [ obj | mob ] name | < obj | mob > number\r\n", ch);
    return;
  }

  if(is_abbrev(buf, "mob"))
    type = 1;
  else if(is_abbrev(buf, "obj"))
    type = 2;

  if(type) {
    if( !*buf2 ) {
      send_to_char("Usage: load [ obj | mob ] name | <obj | mob > number\r\n", ch);
      return;
    }

    if(!isdigit(*buf2)) {
      if(type == 1) {
	if(!(mob = get_char_vis(ch, buf2))) {
	  send_to_char("There is no mobile with that name.\r\n", ch);
	  return;
	}
	number = GET_MOB_VNUM( mob );
      }
      else{
        if(!(obj = get_obj_vis(ch, buf2))) {
          send_to_char("There is no object with that name.\r\n", ch);
          return;
        }
        number = GET_OBJ_VNUM( obj );
      }
    }
    else if((number = atoi(buf2)) < 0) {
      send_to_char("A NEGATIVE number??\r\n", ch);
      return;
    }
  }
  /* No type specified */
  else {
    /* First check eq */
    if((obj = get_object_in_equip_vis(ch, buf, ch->equipment, &tmp))) {
      number = GET_OBJ_VNUM(obj);
      type = 2;
    }
    /* Then inventory */
    else if((obj = get_obj_in_list_vis( ch, buf, ch->carrying))) {
      number = GET_OBJ_VNUM(obj);
      type = 2;
    }
    /* Then mobs in your room */
    else if((mob = get_char_room_vis(ch, buf))) {
      number = GET_MOB_VNUM(mob);
      type = 1;
    }
    /* Then objs in your room */
    else if((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      number = GET_OBJ_VNUM(obj);
      type = 2;
    }
    /* Then mobs anywhere in mud */
    else if(( mob = get_char_vis(ch, buf))) {
      number = GET_MOB_VNUM( mob );
      type = 1;
    }
    /* Then objs anywhere in mud */
    else if((obj = get_obj_vis(ch, buf))) {
      number = GET_OBJ_VNUM(obj);
      type = 2;
    }
    else {
      send_to_char("There is nothing with that name currently in the game.\r\n", ch);
      return;
    }
  }

  if (!has_zone_permission(number, LVL_CREATOR, ch)) {
    send_to_char("You don't have permission for that zone.\r\n", ch);
    return;
  }

  if( type == 1 ) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no mobile with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, ch->in_room);
    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);

    if (GET_LEVEL(ch) < LVL_SUPREME )
      mudlogf(CMP, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
	      "(Load) Mobile: %s #%d, loaded by %s.",
	      CAP(mob->player.short_descr), number, GET_NAME(ch) );
  } else {
    if((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }

    obj = read_object(r_num, REAL);

    /* Otherwise we load it in here */
    if (GET_LEVEL(ch) < LVL_SUPREME )
      mudlogf(CMP, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
	      "(Load) Object: %s #%d, loaded by %s.",
	      CAP(obj->short_description), number, GET_NAME(ch));

	if (subcmd == SCMD_LOADHAND) {
		obj_to_char(obj, ch);
	} else if (subcmd == SCMD_LOADGROUND) {
		obj_to_room(obj, ch->in_room);
	}

    act("With a single hand gesture $n calls upon great powers.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
  }
}

ACMD(do_vstat)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number, r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: vstat { obj | mob } <number>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }

  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
  } else
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  one_argument(argument, buf);

  if (*buf) {			/* argument supplied. destroy single object
				 * or char */
    if ((vict = get_char_room_vis(ch, buf))) {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) <= GET_LEVEL(vict))) {
	send_to_char("Oh Dear... He's not going to be happy!!!\r\n", ch);
	sprintf(buf, "%s has just tried to purge you! Isn't he stupid!?\r\n\r\n",
		GET_NAME(ch));
	send_to_char(buf, vict);
	return;
      }

      /* don't let the imms purge players */
      if (!IS_NPC(vict) && (GET_LEVEL(ch) < LVL_GOD)) {
	send_to_char("Trying to purge a player at your level! Don't think so!!\r\n", ch);
	mudlogf(BRF, LVL_GOD, TRUE, "(GC) %s tried to purge %s.",
		GET_NAME(ch), GET_NAME(vict));
	act("$n stares at $N and tries to use his Immortal Powers....",
	    FALSE, ch, 0, vict, TO_NOTVICT);
	act("$n stares at you and tries to use his Immortal Powers....",
	    FALSE, ch, 0, vict, TO_VICT);
	act("You stare at $N and try to purge them....",
	    FALSE, ch, 0, vict, TO_CHAR);
	send_to_room("... but nothing seems to happen!\r\n", ch->in_room);
	return;
      }

      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
	mudlogf(BRF, LVL_GRGOD, TRUE, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	if (vict->desc) {
	  STATE(vict->desc) = CON_DISCONNECT;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char("Nothing here by that name.\r\n", ch);
      return;
    }

    send_to_char(OK, ch);
  } else {			/* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("The world seems a little cleaner.\r\n", ch->in_room);

    for (vict = world[ch->in_room].people; vict; vict = next_v) {
      next_v = vict->next_in_room;
      if (IS_NPC(vict))
	extract_char(vict);
    }

    for (obj = world[ch->in_room].contents; obj; obj = next_o) {
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
}

ACMD(do_nuke)
{
  struct char_data *victim;
  int i;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Nuke who?\r\n", ch);
    return;
  }

  if (!(victim = get_char_vis(ch, arg))) {
    send_to_char("That player is not here.\r\n", ch);
    return;
  }

  if (GET_IDNUM(victim) == 1 || GET_IDNUM(victim) == 298) {
    send_to_char("I don't think so do you?\r\n", ch);
    return;
  }

  /* ok we clear up some shit here which isn't handled below */
  GET_MAX_MANA(victim) = 50;
  GET_MAX_MOVE(victim) = 50 ;
  GET_PRACTICES(victim) = 0;
  GET_INVIS_LEV(victim) = 0;
  GET_GOLD(victim) = 0;
  GET_BANK_GOLD(victim) = 0;
  GET_QUEST(victim) = 0;

  GET_OLC_ZONE1(victim) = -1;
  GET_OLC_ZONE2(victim) = -1;
  GET_OLC_ZONE3(victim) = -1;
  GET_OLC_ZONE4(victim) = -1;
  GET_OLC_ZONE5(victim) = -1;

  for (i = 1; i <= MAX_SKILLS; i++)
    SET_SKILL(victim, i, 0);

  REMOVE_BIT(PLR_FLAGS(victim), -1);
  REMOVE_BIT(PRF_FLAGS(victim), -1);

  /* ok lets set thier stats up as if they are a new char */
  do_start(victim);

  mudlogf(CMP, MAX(LVL_WIZARD, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s has just NUKED %s.", GET_NAME(ch), GET_NAME(victim));

  send_to_char("You feel the very molecules of you body tearing apart,\r\n"
	       "A swirling mass enters your head and you feel youself\r\n"
	       "tumbling helplessly through space and time.\r\n\r\n"
	       "When the mists start to clear you find that\r\n"
	       "you feel cold and vunerable,\r\n"
	       "Your strength seems to have left you........\r\n\r\n"
	       "You feel slightly different.", victim);
}

ACMD(do_advance)
{
  struct char_data *victim;
  char *name = arg, *level = buf2;
  int newlevel, oldlevel;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name))) {
      send_to_char("That player is not here.\r\n", ch);
      return;
    }
  } else {
    send_to_char("Advance who?\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("Maybe that's not such a great idea.\r\n", ch);
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char("NO!  Not on NPC's.\r\n", ch);
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char("That's not a level!\r\n", ch);
    return;
  }

  if (newlevel > LVL_IMPL) {
    sprintf(buf, "%d is the highest possible level.\r\n", LVL_IMPL);
    send_to_char(buf, ch);
    return;
  }

  if (newlevel > GET_LEVEL(ch)) {
    send_to_char("Yeah, right.\r\n", ch);
    return;
  }

  if (newlevel == GET_LEVEL(victim)) {
    send_to_char("They are already at that level.\r\n", ch);
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    do_start(victim);
    GET_LEVEL(victim) = newlevel;
    send_to_char("You are momentarily enveloped by darkness!\r\n"
		 "You feel somewhat diminished.\r\n", victim);
  } else {
    act("$n makes some strange gestures.\r\n"
	"A strange feeling comes upon you,\r\n"
	"Like a giant hand, light comes down\r\n"
	"from above, grabbing your body, that\r\n"
	"begins to pulse with colored lights\r\n"
	"from inside.\r\n\r\n"
	"Your head seems to be filled with demons\r\n"
	"from another plane as your body dissolves\r\n"
	"to the elements of time and space itself.\r\n"
	"Suddenly a silent explosion of light\r\n"
	"snaps you back to reality.\r\n\r\n"
	"You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(OK, ch);

  log("(GC) %s has advanced %s to level %d (from %d)",
	  GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);
  gain_exp_regardless(victim, (exp_to_level(newlevel-1) - GET_EXP(victim)));
  save_char(victim, NOWHERE);
}



ACMD(do_restore)
{
  struct char_data *vict;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char("Whom do you wish to restore?\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);
    GET_STAM(vict) = GET_MAX_STAM(vict);

    if ((GET_LEVEL(ch) >= LVL_SUPREME) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
      for (i = 1; i <= MAX_SKILLS; i++)
	SET_SKILL(vict, i, 100);

      if (GET_LEVEL(vict) >= LVL_GOD) {
	vict->real_abils.str_add = 100;
	vict->real_abils.intel = 25;
	vict->real_abils.wis = 25;
	vict->real_abils.dex = 25;
	vict->real_abils.str = 25;
	vict->real_abils.con = 25;
	vict->real_abils.end = 25;
      }
      vict->aff_abils = vict->real_abils;
    }
    update_pos(vict);
    send_to_char(OK, ch);
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  }
}


void perform_immort_vis(struct char_data *ch)
{
  if (GET_INVIS_LEV(ch) == 0 && !IS_AFFECTED(ch, AFF_HIDE | AFF_INVISIBLE)) {
    send_to_char("You are already fully visible.\r\n", ch);
    return;
  }

  GET_INVIS_LEV(ch) = 0;
  appear(ch);
  send_to_char("You are now fully visible.\r\n", ch);
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  if (IS_NPC(ch))
    return;

  for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (GET_LEVEL(tch) >= GET_INVIS_LEV(ch) && GET_LEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
	  tch, TO_VICT);
    if (GET_LEVEL(tch) < GET_INVIS_LEV(ch) && GET_LEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
	  tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  sprintf(buf, "Your invisibility level is %d.\r\n", level);
  send_to_char(buf, ch);
}


ACMD(do_invis)
{
  int level;

  if (IS_NPC(ch)) {
    send_to_char("You can't do that!\r\n", ch);
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_LEVEL(ch));
  } else {
    level = atoi(arg);
	if (level > GET_LEVEL(ch))
      send_to_char("You can't go invisible above your own level.\r\n", ch);
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}


ACMD(do_healall) {

  struct descriptor_data *pt;

  for (pt = descriptor_list; pt; pt = pt->next)
    if (!pt->connected && pt->character && pt->character != ch) {
      GET_HIT(pt->character) = GET_MAX_HIT(pt->character);
      GET_STAM(pt->character) = GET_MAX_STAM(pt->character);
      GET_MOVE(pt->character) = GET_MAX_MOVE(pt->character);
      GET_MANA(pt->character) = GET_MAX_MANA(pt->character);
      update_pos(pt->character);
      act("You have been fully healed by $N!", FALSE, pt->character, 0, ch, TO_CHAR);
    }
  send_to_char("You heal everybody in Insanity.\r\n", ch);
}


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char("That must be a mistake...\r\n", ch);
  else {
    sprintf(buf, "%s\r\n", argument);
    for (pt = descriptor_list; pt; pt = pt->next)
      if (!pt->connected && pt->character && pt->character != ch)
	send_to_char(buf, pt->character);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else
      send_to_char(buf, ch);
  }
}

ACMD(do_immgoss)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char("That must be a mistake...\r\n", ch);
  else {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (!pt->connected && pt->character ) {
	sprintf(buf, "\r\n%s[GOSSIP] %s :: %s%s\r\n",
		CCBLU(pt->character, C_CMP),
		((GET_INVIS_LEV(ch) > GET_LEVEL(pt->character)) ? "Someone" : GET_NAME(ch)),
		argument, CCNRM(pt->character, C_CMP));
	send_to_char(buf, pt->character);
      }
  }
}

ACMD(do_system)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char("That must be a mistake...\r\n", ch);
  else {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (!pt->connected && pt->character && pt->character != ch) {
	sprintf(buf, "%s** SYSTEM (%s) :: %s\r\n\a%s",
		CCRED(pt->character, C_CMP),
		(GET_INVIS_LEV(ch) > GET_LEVEL(pt->character) ? "Someone" : GET_NAME(ch)),
		argument, CCNRM(pt->character, C_CMP));
	send_to_char(buf, pt->character);
      }
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "%s** SYSTEM (%s) :: %s\r\n\a%s",
	      CCRED(ch, C_CMP), GET_NAME(ch), argument, CCNRM(ch, C_CMP));
      send_to_char(buf, ch);
    }
  }
}


ACMD(do_poofset)
{
  char **msg;

  if (!*argument) {
    send_to_char("You must set your poofin and poofout to something!\r\n", ch);
    return;
  }

  switch (subcmd) {
  case SCMD_POOFIN:    msg = &(POOFIN(ch));    break;
  case SCMD_POOFOUT:   msg = &(POOFOUT(ch));   break;
  default:    return;    break;
  }

  skip_spaces(&argument);

  if (*msg)
    free(*msg);

  *msg = str_dup(argument);
  send_to_char(OK, ch);
}


ACMD(do_dc)
{
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char("Usage: DC <user number> (type USERS for a list)\r\n", ch);
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char("No such connection.\r\n", ch);
    return;
  }
  if (d->character && GET_LEVEL(d->character) >= GET_LEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char("No such connection.\r\n", ch);
    else
      send_to_char("Umm.. maybe that's not such a good idea...\r\n", ch);
    return;
  }
  STATE(d) = CON_DISCONNECT;
  sprintf(buf, "Connection #%d closed.\r\n", num_to_dc);
  send_to_char(buf, ch);
  log("(GC) Connection closed by %s.", GET_NAME(ch));
}


ACMD(do_wizlock)
{
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char("Invalid wizlock value.\r\n", ch);
      return;
    }
    restrict = value;
    when = "now";
  } else
    when = "currently";

  switch (restrict) {
  case 0:
    sprintf(buf, "The game is %s completely open.\r\n", when);
    send_to_char(buf, ch);
    if (*arg)
      mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s has re-opened the game for all players.", GET_NAME(ch));
    break;
  case 1:
    sprintf(buf, "The game is %s closed to new players.\r\n", when);
    send_to_char(buf, ch);
    if (*arg)
      mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s has wizlocked the game for new players.", GET_NAME(ch));
    break;
  default:
    sprintf(buf, "Only level %d and above may enter the game %s.\r\n", restrict, when);
    send_to_char(buf, ch);
    if (*arg)
      mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s has wizlocked the game at level %d", GET_NAME(ch), restrict);
    break;
  }
}


ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE) {
    sprintf(buf, "Current machine time: %s\r\n", tmstr);
    send_to_char(buf, ch);
  } else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;
    sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d,
	    ((d == 1) ? "" : "s"), h, m);
    send_to_char(buf, ch);
  }
}



ACMD(do_last)
{
  struct char_file_u chdata;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("For whom do you wish to search?\r\n", ch);
    return;
  }
  if (load_char(arg, &chdata) < 0) {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if ((chdata.level > GET_LEVEL(ch)) && (GET_LEVEL(ch) < LVL_LOWIMPL)) {
    send_to_char("You are not sufficiently godly for that!\r\n", ch);
    return;
  }
  sprintf(buf, "[%5ld] [%2d %s] %-12s : %-18s : %-20s\r\n",
	  chdata.char_specials_saved.idnum, (int) chdata.level,
	  class_abbrevs[(int) chdata.class], chdata.name, chdata.host,
	  ctime(&chdata.last_logon));
  send_to_char(buf, ch);
}

ACMD(do_fas) {

  struct descriptor_data *i, *next_desc;
  struct char_data *vict;
  char to_force[MAX_INPUT_LENGTH + 2];

  strcpy(to_force, "save");

  skip_spaces(&argument);

  if (*argument)
    sprintf(buf1, "$n has forced you to save because %s.", argument);
  else
    strcpy(buf1, "$n has forced you to save.");

  send_to_char(OK, ch);
  mudlogf(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s forced all to Save.", GET_NAME(ch));

  for (i = descriptor_list; i; i = next_desc) {
    next_desc = i->next;

    if (i->connected || !(vict = i->character) || GET_LEVEL(vict) >= GET_LEVEL(ch))
      continue;

    act(buf1, TRUE, ch, NULL, vict, TO_VICT);
    write_aliases(vict);
    save_char(vict, NOWHERE);
    Crash_crashsave(vict);
    if (ROOM_FLAGGED(vict->in_room, ROOM_HOUSE_CRASH))
      House_crashsave(world[vict->in_room].number);

  }
}

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char("Whom do you wish to force do what?\r\n", ch);
  else if ((GET_LEVEL(ch) < LVL_SUPREME) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg)))
      send_to_char(NOPERSON, ch);
    else if (GET_LEVEL(ch) <= GET_LEVEL(vict))
      send_to_char("No, no, no!\r\n", ch);
    else {
      send_to_char(OK, ch);
      if (GET_INVIS_LEV(ch) <= GET_LEVEL(vict))
	act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      mudlogf(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(OK, ch);
    mudlogf(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	    "(GC) %s forced room %d to %s", GET_NAME(ch), world[ch->in_room].number,to_force);
    for (vict = world[ch->in_room].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      if (GET_INVIS_LEV(ch) <= GET_LEVEL(vict))
	act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(OK, ch);
    mudlogf(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
		"(GC) %s forced all to %s", GET_NAME(ch), to_force);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (i->connected || !(vict = i->character) || GET_LEVEL(vict) >= GET_LEVEL(ch))
	continue;
      if (GET_INVIS_LEV(ch) <= GET_LEVEL(vict))
	act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}


/* modifications for wemote here by nahaz */
ACMD(do_wiznet)
{
  struct descriptor_data *d;
  char emote = FALSE;
  byte wmote = FALSE;
  char any = FALSE;
  int level = LVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (subcmd == SCMD_WEMOTE) {
    subcmd = SCMD_WIZNET;
    wmote = TRUE;
  }

  if (!*argument) {
    send_to_char("Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n "
		 "       wiznet @<level> *<emotetext> | wiz @ | wiz ?<min lev>\r\n", ch);
    return;
  }

  if (wmote) {
    do_wmote(ch, argument, 0, 1);
    return;
  }

  level = MAX(LVL_IMMORT, GET_MINWIZ(ch));

  switch (*argument) {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), LVL_IMMORT);
      if (level > GET_LEVEL(ch) || GET_LEVEL(ch) < LVL_WIZARD || level < GET_MINWIZ(ch)) {
	send_to_char("You can't do that!\r\n", ch);
	return;
      }
    } else if (emote)
      argument++;
    break;
  case '?':
    if (GET_LEVEL(ch) < LVL_GOD) {
      send_to_char("You're not godly enough to do that!", ch);
      return;
    }
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      GET_MINWIZ(ch) = MIN(atoi(buf1), GET_LEVEL(ch));
      sprintf(buf, "You set your min wiznet level to %d.\r\n", GET_MINWIZ(ch));
      send_to_char(buf, ch);
      return;
    } else
      return;
    break;
  case '@':
    for (d = descriptor_list; d; d = d->next) {
      if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT &&
	  !PRF_FLAGGED(d->character, PRF_NOWIZ) &&
	  (CAN_SEE(ch, d->character) || GET_LEVEL(ch) == LVL_LOWIMPL)) {
	if (!any) {
	  sprintf(buf1, "Gods online:\r\n");
	  any = TRUE;
	}
	sprintf(buf1, "%s  %s", buf1, GET_NAME(d->character));
	if (TMP_FLAGGED(d->character, TMP_WRITING))
	  sprintf(buf1, "%s (Writing)\r\n", buf1);
	else if (TMP_FLAGGED(d->character, TMP_MAILING))
	  sprintf(buf1, "%s (Writing mail)\r\n", buf1);
	else if (TMP_FLAGGED(d->character, TMP_OLC))
	  sprintf(buf1, "%s (Using OLC)\r\n", buf1);
	else
	  sprintf(buf1, "%s\r\n", buf1);
	
      }
    }
    any = FALSE;
    for (d = descriptor_list; d; d = d->next) {
      if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT &&
	  PRF_FLAGGED(d->character, PRF_NOWIZ) &&
	  CAN_SEE(ch, d->character)) {
	if (!any) {
	  sprintf(buf1, "%sGods offline:\r\n", buf1);
	  any = TRUE;
	}
	sprintf(buf1, "%s  %s\r\n", buf1, GET_NAME(d->character));
      }
    }
    send_to_char(buf1, ch);
    return;
    break;
  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char("You are offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Don't bother the gods like that!\r\n", ch);
    return;
  }
  if (level > LVL_IMMORT) {
    sprintf(buf1, "Wiz:: (%s) <%d> %s%s\r\n", GET_NAME(ch), level,
	    emote ? "<--- " : "", argument);
    sprintf(buf2, "Wiz:: (Someone) <%d> %s%s\r\n", level, emote ? "<--- " : "",
	    argument);
  } else {
    sprintf(buf1, "Wiz:: (%s) %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
	    argument);
    sprintf(buf2, "Wiz:: (Someone) %s%s\r\n", emote ? "<--- " : "", argument);
  }

  for (d = descriptor_list; d; d = d->next) {
    if ((!d->connected) && (GET_LEVEL(d->character) >= level) &&
	(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
	(GET_MINWIZ(d->character) <= GET_LEVEL(ch)) &&
	(!TMP_FLAGGED(d->character, TMP_WRITING | TMP_MAILING | TMP_OLC))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(CCCYN(d->character, C_NRM), d->character);
      if (CAN_SEE(d->character, ch))
	send_to_char(buf1, d->character);
      else
	send_to_char(buf2, d->character);
      send_to_char(CCNRM(d->character, C_NRM), d->character);
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
}

ACMD(do_impnet)
{
  struct descriptor_data *d;
  char emote = FALSE;
  int level = LVL_LOWIMPL;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("Usage: impnet [#level] <text> | *<emotetext>\r\n ", ch);
    return;
  }

  switch (*argument) {

  case '*':
    emote = TRUE;

  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), LVL_IMMORT);
    } else if (emote)
      argument++;
    break;

  case '\\':
    ++argument;
    break;

  default:
    break;

  } /* end of switch argument */

  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char("You are offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Don't bother the gods like that!\r\n", ch);
    return;
  }

  if (level < LVL_LOWIMPL) {
      sprintf(buf1, "Imp:: (%s) <%d> %s%s\r\n", GET_NAME(ch), level,
  	    emote ? "<--- " : "", argument);
      sprintf(buf2, "Imp:: (Someone) <%d> %s%s\r\n", level, emote ? "<--- " : "",
  	    argument);
    } else {
      sprintf(buf1, "Imp:: (%s) %s%s\r\n", GET_NAME(ch), emote ? "<--- " : "",
	      argument);
      sprintf(buf2, "Imp:: (Someone) %s%s\r\n", emote ? "<--- " : "", argument);
    }
  
  
  
  for (d = descriptor_list; d; d = d->next) {
    if ((!d->connected) && (GET_LEVEL(d->character) >= level) &&
	(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
	(!TMP_FLAGGED(d->character, TMP_WRITING | TMP_MAILING | TMP_OLC))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(CCCYN(d->character, C_NRM), d->character);
  if (CAN_SEE(d->character, ch))
    send_to_char(buf1, d->character);
  else
    send_to_char(buf2, d->character);
  send_to_char(CCNRM(d->character, C_NRM), d->character);
    }
  }
  
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
}

ACMD(do_zreset)
{
  int i, j;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("You must specify a zone.\r\n", ch);
    return;
  }
  if (*arg == '*') {

    if (GET_LEVEL(ch) < LVL_CREATOR) {
      send_to_char("You are not godly enough to do that!\r\n", ch);
      return;
    }

    for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i);
    send_to_char("Reset world.\r\n", ch);
    mudlogf(NRM, MAX(LVL_SUPREME, GET_INVIS_LEV(ch)), TRUE,
	    "(GC) %s reset entire world.", GET_NAME(ch));

    return;
  } else if (*arg == '.')
    i = world[ch->in_room].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }

  /* add check to see if the zone is correct for that builder */
  if (!has_zone_permission(zone_table[i].number*100, LVL_CREATOR, ch)) {
    send_to_char("You don't have permission to reset that zone.\r\n", ch);
    return;
  }

  if (i >= 0 && i <= top_of_zone_table) {
    reset_zone(i);
    sprintf(buf, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number,
	    zone_table[i].name);
    send_to_char(buf, ch);
    mudlogf(NRM, MAX(LVL_SUPREME, GET_INVIS_LEV(ch)), TRUE,
	    "(GC) %s reset zone %d (%s)", GET_NAME(ch),zone_table[i].number, zone_table[i].name);

  } else
    send_to_char("Invalid zone number.\r\n", ch);
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
  struct char_data *vict;
  long result;
  sh_int target;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char("Yes, but for whom?!?\r\n", ch);
  else if (!(vict = get_char_vis(ch, arg)))
    send_to_char("There is no such player.\r\n", ch);
  else if (IS_NPC(vict))
    send_to_char("You can't do that to a mob!\r\n", ch);
  else if (GET_LEVEL(vict) > GET_LEVEL(ch))
    send_to_char("Hmmm...you'd better not.\r\n", ch);
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      send_to_char("Rerolled...\r\n", ch);
      roll_real_abils(vict);
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      sprintf(buf, "New stats: Str %d/%d, Int %d, Wis %d, Dex %d, Con %d, Bio %d\r\n",
	      GET_STR(vict), GET_ADD(vict), GET_INT(vict), GET_WIS(vict),
	      GET_DEX(vict), GET_CON(vict), GET_END(vict));
      send_to_char(buf, ch);
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
	send_to_char("Your victim is not flagged.\r\n", ch);
	return;
      }
      REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
      send_to_char("Pardoned.\r\n", ch);
      send_to_char("You have been pardoned by the Gods!\r\n", vict);
      mudlogf(BRF, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      mudlogf(NRM, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) Notitle %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char("Notitled\r\n", ch);
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      mudlogf(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) Squelch %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char("Squelched.\r\n", ch);
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
	return;
      }

      if (GET_LEVEL(ch) == GET_LEVEL(vict)) {
	send_to_char("Better not!!\r\n", ch);
	return;
      }

      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Your victim is already pretty cold.\r\n", ch);
	return;
      }
      SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
      REMOVE_BIT(PRF_FLAGS(vict), PRF_SUMMONABLE); /* so they can't summon out of ice box */
      send_to_char("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n", vict);
      send_to_char("Frozen.\r\n", ch);
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      mudlogf(BRF, MAX(LVL_SUPREME, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\r\n", ch);
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
	sprintf(buf, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
	   GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	send_to_char(buf, ch);
	return;
      }
      mudlogf(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n", vict);
      send_to_char("Thawed.\r\n", ch);
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected) {
	while (vict->affected)
	  affect_remove(vict, vict->affected);
	send_to_char("There is a brief flash of light!\r\n"
		     "You feel slightly different.\r\n", vict);
	send_to_char("All spells removed.\r\n", ch);
      } else {
	send_to_char("Your victim does not have any affections!\r\n", ch);
	return;
      }
      break;
    case SCMD_JAIL:
      if (ch == vict) {
        send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
        return;
      }
      if (PLR_FLAGGED(vict, PLR_CRIMINAL)) {
        send_to_char("Your victim is already jailed.\r\n", ch);
        return;
      }
      SET_BIT(PLR_FLAGS(vict), PLR_CRIMINAL);
      GET_JAIL_LEV(vict) = GET_LEVEL(ch);
      send_to_char("You've been caught doing something bad!  You feel as you've been moved!\r\n", vict);
      send_to_char("Jailed!\r\n", ch);
      char_from_room(vict);
      char_to_room(vict, r_jailed_start_room);
      act("The wardens come in and haul $n's ass to jail!", FALSE, vict, 0, 0, TO_ROOM);
      mudlogf(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	     "(GC) %s jailed by %s.", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_PAROLE:
      if (!PLR_FLAGGED(vict, PLR_CRIMINAL)) {
        send_to_char("Sorry, your victim is not a criminal.\r\n", ch);
        return;
      }
      if ((byte)GET_JAIL_LEV(vict) > GET_LEVEL(ch)) {
        sprintf(buf, "Sorry, a level %d God jailed %s... you can't parole %s.\r\n",
           GET_JAIL_LEV(vict), GET_NAME(vict), HMHR(vict));
        send_to_char(buf, ch);
        return;
      }
      mudlogf(BRF, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	      "(GC) %s paroled by %s.", GET_NAME(vict), GET_NAME(ch));
      REMOVE_BIT(PLR_FLAGS(vict), PLR_CRIMINAL);
      send_to_char("Paroled.\r\n", ch);
      char_from_room(vict);
      char_to_room(vict, r_mortal_start_room[(int)GET_CLASS(ch)]);
      break;

    case SCMD_OFFICE:
      if ((target = real_room(GET_OFFICE(ch))) > 0) {
	if (FIGHTING(vict))
	  stop_fighting(vict);
	send_to_char(OK, ch);
	act("$n disappears in a flash of bright light.", FALSE, vict, 0, 0, TO_ROOM);
	char_from_room(vict);
	char_to_room(vict, target);
	act("$n arrives from a flash of bright light.", FALSE, vict, 0, 0, TO_ROOM);
	act("You have been summoned to $n's office!", FALSE, ch, 0, vict, TO_VICT);
	look_at_room(vict, 0);
      }
      else
	send_to_char("Invalid office room number, use set me <vnum> to change it.\r\n", ch);
      break;

    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      break;
    }
    save_char(vict, NOWHERE);
  }
}


ACMD(do_zstat) {

  int zone = -1, j = -1, done = 0;
  FILE *fl;
  struct char_file_u player;
  struct char_data *cbuf;

  one_argument(argument, arg);

  if (!(fl = fopen(PLAYER_FILE, "r+"))) {
    send_to_char("Can't open player file.", ch);
    return;
  }

  if (!*arg)
    zone = world[ch->in_room].zone;
  else {
    j = atoi(arg);
    for (zone = 0; zone_table[zone].number != j && zone <= top_of_zone_table; zone++);
  }

  if (zone > top_of_zone_table || zone < 0) {
    send_to_char("That is not a valid zone.\r\n", ch);
    return;
  }

  send_to_char("Zone Stats:\r\n-----------\r\n\r\n", ch);
  sprintf(buf, "Zone Number: [%s%3d%s], Name: [%s%s%s]\r\nReal Zone Number: [%s%3d%s], Top of Zone: [%s%6d%s]\r\n",
	  CCRED(ch, C_CMP), zone_table[zone].number, CCNRM(ch, C_CMP),
	  CCBLU(ch, C_CMP), zone_table[zone].name, CCNRM(ch, C_CMP),
	  CCRED(ch, C_CMP), zone, CCNRM(ch, C_CMP),
	  CCRED(ch, C_CMP), zone_table[zone].top, CCNRM(ch, C_CMP));

  sprintf(buf, "%sAge: [%s%3d%s], Reset Time: [%s%3d%s], Reset Type: [%s%s%s]\r\n", buf,
	  CCGRN(ch, C_CMP), zone_table[zone].age, CCNRM(ch, C_CMP),
	  CCGRN(ch, C_CMP), zone_table[zone].lifespan, CCNRM(ch, C_CMP),
	  CCGRN(ch, C_CMP), zone_table[zone].reset_mode ? ((zone_table[zone].reset_mode == 1) ?
							   "Reset when no players in zone" : "Normal reset") :
	  "Never reset", CCNRM(ch, C_CMP));

  sprintf(buf, "%sMinimum Level: [%s%3d%s], Maximum Level: [%s%3d%s], Approved: [%s%3s%s]\r\n", buf,
	  CCCYN(ch, C_CMP), zone_table[zone].lvl1, CCNRM(ch, C_CMP),
	  CCCYN(ch, C_CMP), zone_table[zone].lvl2, CCNRM(ch, C_CMP),
	  CCCYN(ch, C_CMP), (zone_table[zone].status_mode == 0 ? "No" : "Yes"), CCNRM(ch, C_CMP));
  send_to_char(buf, ch);

  strcpy(buf, "\r\nBuilders:\r\n\r\n"
	 "   Name:                Level:\r\n"
	 "   ~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n");

  while (!done) {
      fread(&player, sizeof(struct char_file_u), 1, fl);
      if (feof(fl)) {
	fclose(fl);
	done=TRUE;
      }
      if (!done) {
	CREATE(cbuf, struct char_data, 1);
	clear_char(cbuf);
	store_to_char(&player, cbuf);

        if (player.level >= LVL_IMMORT &&
	   	    ((zone_table[zone].number == GET_OLC_ZONE1(cbuf)) ||
		     (zone_table[zone].number == GET_OLC_ZONE2(cbuf)) ||
		     (zone_table[zone].number == GET_OLC_ZONE3(cbuf)) ||
		     (zone_table[zone].number == GET_OLC_ZONE4(cbuf)) ||
		     (zone_table[zone].number == GET_OLC_ZONE5(cbuf)))) {
	  sprintf(buf, "%s   %-20s %d\r\n", buf, player.name, player.level);
	}
	free_char(cbuf);
      }
    }
  strcat(buf, "\r\n");
  send_to_char(buf, ch);
}

/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, int zone)
{
  sprintf(bufptr, "%s%3d %-30.30s Age: %3d; Reset: %3d (%1d); Top: %5d\r\n",
	  bufptr, zone_table[zone].number, zone_table[zone].name,
	  zone_table[zone].age, zone_table[zone].lifespan,
	  zone_table[zone].reset_mode, zone_table[zone].top);
}


ACMD(do_show)
{
  struct descriptor_data *d;
  struct char_file_u vbuf;
  struct char_data *vict;
  int i, j = 0, k, l, con, cmdno, no=0;
  char self = 0;
  struct obj_data *obj;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], birth[80];

  FILE *fl;
  struct char_file_u player;
  int done = 0;

  struct show_struct {
    const char *cmd;
    char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		LVL_GUARDIAN },			/* 1 */
    { "player",		LVL_GOD },
    { "rent",		LVL_WIZARD },
    { "stats",		LVL_SUPREME },
    { "errors",		LVL_CREATOR },			/* 5 */
    { "death",		LVL_WIZARD },
    { "godrooms",	LVL_SUPREME },
    { "shops",		LVL_ARCHWIZ   },
    { "houses",		LVL_SUPREME },
    { "connections",    LVL_GRGOD },                    /* 10 */
    { "immortals",      LVL_CREATOR },
    { "heros",          LVL_GUARDIAN },
    { "arena",          LVL_RULER },
    { "linkless",       LVL_GRGOD },
    { "plist",          LVL_LOWIMPL },          /* 15 */
    { "pk",             LVL_GOD },
    { "world",          LVL_LOWIMPL },
    { "frozen",         LVL_FREEZE },
    { "muted",          LVL_GRGOD },
    { "olc",            LVL_GOD },          /* 20 */
    { "score",          LVL_IMMORT },
    { "skills",         LVL_IMMORT },
    { "wizcmds",        LVL_LOWIMPL },
    { "noexp",          LVL_SUPREME },
    { "lockdown",       LVL_LOWIMPL },      /* 25 */
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_LEVEL(ch))
	sprintf(buf, "%s%-15s%s", buf, fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_LEVEL(ch) < fields[l].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }
  if (!strcmp(value, "."))
    self = 1;
  buf[0] = '\0';
  switch (l) {
  case 1:			/* zone */
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, world[ch->in_room].zone);
    else if (*value && is_number(value)) {
      for (j = atoi(value), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++);
      if (i <= top_of_zone_table)
	print_zone_to_buf(buf, i);
      else {
	send_to_char("That is not a valid zone.\r\n", ch);
	return;
      }
    } else
      for (i = 0; i <= top_of_zone_table; i++)
	print_zone_to_buf(buf, i);
    page_string(ch->desc, buf, 1);

    break;
  case 2:			/* player */
    if (!*value) {
      send_to_char("A name would help.\r\n", ch);
      return;
    }

    if (load_char(value, &vbuf) < 0) {
      send_to_char("There is no such player.\r\n", ch);
      return;
    }

    if (vbuf.level > GET_LEVEL(ch)) {
      send_to_char("Hmmmmm I've fixed it... thank Arnachak! *grin*\r\n", ch);
      return;
    }

    sprintf(buf, "Player: %-12s (%s) [%2d %s]\r\n", vbuf.name,
      genders[(int) vbuf.sex], vbuf.level, class_abbrevs[(int) vbuf.class]);
    sprintf(buf,
	 "%sAu: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n",
	    buf, vbuf.points.gold, vbuf.points.bank_gold, vbuf.points.exp,
	    vbuf.char_specials_saved.alignment,
	    vbuf.player_specials_saved.spells_to_learn);
    strcpy(birth, ctime(&vbuf.birth));
    sprintf(buf,
	    "%sStarted: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n",
	    buf, birth, ctime(&vbuf.last_logon), (int) (vbuf.played / 3600),
	    (int) (vbuf.played / 60 % 60));
    send_to_char(buf, ch);
    break;
  case 3:
    /* the following added by Nahaz to fix the show rent bug. */
    if (!*value) {
      send_to_char("A name would help.\r\n", ch);
      return;
    }
    
    if (load_char(value, &vbuf) < 0) {
      send_to_char("There is no such player.\r\n", ch);
      return;
    }

    if (vbuf.level > GET_LEVEL(ch)) {
      send_to_char("You are not godly enough for that.\r\n", ch);
      return;
    }
    /* end of Nahaz fix */
    
    Crash_listrent(ch, value);
    break;
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    sprintf(buf, "Current stats:\r\n");
    sprintf(buf, "%s  %5d players in game  %5d connected\r\n", buf, i, con);
    sprintf(buf, "%s  %5d registered\r\n", buf, top_of_p_table + 1);
    sprintf(buf, "%s  %5d mobiles          %5d prototypes\r\n",
	    buf, j, top_of_mobt + 1);
    sprintf(buf, "%s  %5d objects          %5d prototypes\r\n",
	    buf, k, top_of_objt + 1);
    sprintf(buf, "%s  %5d rooms            %5d zones\r\n",
	    buf, top_of_world + 1, top_of_zone_table + 1);
    sprintf(buf, "%s  %5d large bufs\r\n", buf, buf_largecount);
    sprintf(buf, "%s  %5d buf switches     %5d overflows\r\n", buf,
	    buf_switches, buf_overflows);
    send_to_char(buf, ch);
    break;
  case 5:
    strcpy(buf, "Errant Rooms\r\n------------\r\n");
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++)
	if (world[i].dir_option[j] && (world[i].dir_option[j]->to_room == 0 || world[i].dir_option[j]->to_room == 1))
	  sprintf(buf, "%s%2d: [%5d] %s\r\n", buf, ++k, world[i].number,
		  world[i].name);
    send_to_char(buf, ch);
    break;
  case 6:
    strcpy(buf, "Death Traps\r\n-----------\r\n");
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (IS_SET(ROOM_FLAGS(i), ROOM_DEATH))
	sprintf(buf, "%s%2d: [%5d] %s\r\n", buf, ++j,
		world[i].number, world[i].name);
    send_to_char(buf, ch);
    break;
  case 7:
    strcpy(buf, "Godrooms\r\n--------------------------\r\n");
    for (i = 0, j = 0; i < top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_GODROOM))
	sprintf(buf,"%s%2d: [%5d] %s\r\n",buf,++j,world[i].number,world[i].name);
    send_to_char(buf, ch);
    break;
  case 8:
    show_shops(ch, value);
    break;
  case 9:
    hcontrol_list_houses(ch);
    break;

  case 10:
    show_connections(ch, value);
    break;
    /*    if (self) {
      j = world[ch->in_room].zone;
      i = (int)world[ch->in_room].number/100;
    } else {
      i = atoi(value);
      for (j = 0; zone_table[j].number != i && i <= top_of_zone_table; j++);
    }

    if (j > top_of_zone_table || j < 0)
      send_to_char("That is not a valid zone.\r\n", ch);
    else {
      sprintf(buf, "Connections in this zone %d\r\n", i);
      strcat(buf,  "============================\r\n");
      for (i = 0; i <= top_of_world; i++)
	if (world[i].zone == j)
	  for(k = 0; k <NUM_OF_DIRS; k++)
	    if(world[i].dir_option[k] && world[world[i].dir_option[k]->to_room].zone != j)
	      sprintf(buf, "%s%5d leads to %-5d.\r\n", buf, world[i].number,
		      world[world[i].dir_option[k]->to_room].number);
    }
    send_to_char(buf, ch);
    break;*/

  case 11 :

    if (!(fl = fopen(PLAYER_FILE, "r+"))) {
      send_to_char("Can't open player file.", ch);
      return;
    }
    sprintf(buf, "Player Name          Player Level\r\n");
    while (!done) {
      fread(&player, sizeof(struct char_file_u), 1, fl);
      if (feof(fl)) {
	fclose(fl);
	done=TRUE;
      }
      if (!done)
	if (player.level >= LVL_IMMORT && player.level < LVL_IMPL)
	  sprintf(buf, "%s%-20s %-2i\r\n", buf, player.name, player.level);
    }
    send_to_char(buf, ch);
    break;

  case 12:
    if (!(fl = fopen(PLAYER_FILE, "r+"))) {
      send_to_char("Can't open player file.", ch);
      return;
    }
    sprintf(buf, "Current Hero's: \r\n\r\nPlayer Name          Player Level\r\n");
    while (!done) {
      fread(&player, sizeof(struct char_file_u), 1, fl);
      if (feof(fl)) {
	fclose(fl);
	done=TRUE;
      }
      if (!done)
	if ((player.level >= LVL_HERO) && (player.level <= LVL_PRINCE))
	  sprintf(buf, "%s%-20s %-2i\r\n", buf, player.name, player.level);
    }
    send_to_char(buf, ch);
    break;
  case 13:
    send_to_char("Arena Stats\r\n-----------\r\n", ch);
    sprintf(buf, "In Arena = %-3s            In start arena = %-3s\r\n",
	    YESNO(in_arena), YESNO(in_start_arena));
    sprintf(buf, "%sStart time = %-5d        Game length = %d\r\n", buf,
	    start_time, game_length);
    sprintf(buf, "%sTime to Start = %-5d     Time left = %d\r\n", buf,
	    time_to_start, time_left_in_game);
    send_to_char(buf, ch);
    break;

  case 14:

    send_to_char("The following players are linkless at the moment:\r\n"
	       "-------------------------------------------------\r\n\r\n", ch);

    for (vict = character_list; vict; vict = vict->next) {
      if (CAN_SEE(ch, vict) && !IS_NPC(vict))
	if (vict->desc == NULL) {
	  sprintf(buf, "%s, Level %d. In Room %d.\r\n",
		  GET_NAME(vict), GET_LEVEL(vict), world[vict->in_room].number);
	  send_to_char(buf, ch);
	}
    }
    break;

  case 15:

    for (i = 0; i <= top_of_p_table; i++) {
      sprintf(buf, "%s  %-20.20s", buf, (player_table + i)->name);
      j++;
      if (j == 3) {
	j = 0;
	sprintf(buf, "%s\r\n", buf);
    }
    }
    page_string(ch->desc, buf, 1);
    break;

 case 16:
    
    if (!(fl = fopen(PLAYER_FILE, "r+"))) {
      send_to_char("Can't open player file.", ch);
      return;
    }
    sprintf(buf, "Player Killers: \r\n\r\n&rPlayer Name          Player Level&n\r\n");
    while (!done) {
      fread(&player, sizeof(struct char_file_u), 1, fl);
      if (feof(fl)) {
	fclose(fl);
	done=TRUE;
      }
      if (!done) {
	CREATE(vict, struct char_data, 1);
	clear_char(vict);
	store_to_char(&player, vict);
	
	if (PLR_FLAGGED(vict, PLR_PK))
	  sprintf(buf, "%s&y%-20s&n &c%-2i&n\r\n", buf, player.name,
		  player.level);
	free_char(vict);
      }
    }
    send_to_char(buf, ch);
    break;
    
  case 17:
    
    if (!(fl = fopen(PLAYER_FILE, "r+"))) {
      send_to_char("Can't open player file.", ch);
      return;
    }
    sprintf(buf, "Players with World access: \r\n\r\n&rPlayer Name          Player Level&n\r\n");
    while (!done) {
      fread(&player, sizeof(struct char_file_u), 1, fl);
      if (feof(fl)) {
	fclose(fl);
	done=TRUE;
      }
      if (!done) {
	CREATE(vict, struct char_data, 1);
	clear_char(vict);
	store_to_char(&player, vict);
	
	if (PLR_FLAGGED(vict, PLR_WORLD))
	  sprintf(buf, "%s&y%-20s&n &c%-2i&n\r\n", buf, player.name,
		  player.level);
	free_char(vict);
      }
    }
    send_to_char(buf, ch);
    break;

    /* Added by Nahaz - show frozen, show muted, show olc, show score
       show skills, show wizcmds */
  case 18: /* frozen */
    strcpy(buf, "Frozen Characters\r\n"
	        "------------------\r\n");
    for (i = 0; i <= top_of_p_table; i++) {
      if (load_char(((player_table + i)->name), &vbuf) < 0) {
	log("SYSERR: Invalid player in show_frozen command.");
	return;
      }
      if (IS_SET((vbuf.char_specials_saved.act), PLR_FROZEN)) {
	sprintf(buf + strlen(buf), "[%3d %s] %s\r\n", vbuf.level, 
		class_abbrevs[(int) vbuf.class], vbuf.name);
      }
    }
    page_string(ch->desc, buf, 1);
    break;

  case 19: /* muted */
    strcpy(buf, "Muted Characters\r\n"
	        "-----------------\r\n");
    for (i = 0; i <= top_of_p_table; i++) {
      if (load_char(((player_table + i)->name), &vbuf) < 0) {
	log("SYSERR: Invalid player in show_muted command.");
	return;
      }
      if (IS_SET((vbuf.char_specials_saved.act), PLR_NOSHOUT)) {
	sprintf(buf + strlen(buf), "[%3d %s] %s\r\n", vbuf.level, 
		class_abbrevs[(int) vbuf.class], vbuf.name);
      }
    }
    page_string(ch->desc, buf, 1);
    break;

 case 20: /* olc */
    strcpy(buf, "Players using OLC\r\n"
	         "-----------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if(CAN_SEE(ch, d->character)) {
	if ((STATE(d) == CON_MEDIT) ||
	    (STATE(d) == CON_OEDIT) ||
	    (STATE(d) == CON_REDIT) ||
	    (STATE(d) == CON_SEDIT) ||
	    (STATE(d) == CON_TEXTED) ||
	    (STATE(d) == CON_ZEDIT)) {
	  
	  sprintf(buf + strlen(buf), "%s :: ", GET_NAME(d->character));
	  switch(STATE(d)) {
	  case CON_MEDIT:
	    sprintf(buf + strlen(buf), "MEDIT: %d\r\n", OLC_NUM(d));
	    break;
	  case CON_OEDIT:
	    sprintf(buf + strlen(buf), "OEDIT: %d\r\n", OLC_NUM(d));
	    break;
	  case CON_REDIT:
	    sprintf(buf + strlen(buf), "REDIT: %d\r\n", OLC_NUM(d));
	    break;
	  case CON_SEDIT:
	    sprintf(buf + strlen(buf), "SEDIT: %d\r\n", OLC_NUM(d));
	    break;
	  case CON_TEXTED:
	    sprintf(buf + strlen(buf), "TEDIT: '%s'\r\n", 
		    (d->character->desc->storage));
	    break;
	  case CON_ZEDIT:
	    sprintf(buf + strlen(buf), "ZEDIT: %d\r\n", OLC_NUM(d));
	    break;
	  default:
	    log("SYSERR: unknown OLC state in show olc.");
	    break;
	  }
	}
      }
    }
    page_string(ch->desc, buf, 1);
    break;

  case 21: /* score */
    if (!*value) {
      send_to_char("For which player?\r\n", ch);
      return;
    }
    if(!(vict = get_char_vis(ch, value))) {
      send_to_char(NOPERSON, ch);
      return;
    }
    if(IS_NPC(vict)) {
      send_to_char("You can only do that to players.\r\n", ch);
      return;
    }
    show_score_to_char(vict, ch);
    break;

  case 22: /* skills */
    if (!*value) {
      send_to_char("A name would help.\r\n", ch);
      return;
    }
    if(!(vict = get_char_vis(ch, value))) {
      send_to_char(NOPERSON, ch);
      return;
    }
    if(IS_NPC(vict)) {
      send_to_char("You can only do that to players.\r\n", ch);
      return;
    }
    if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
      send_to_char("Sorry you cannot do that.\r\n", ch);
      return;
    }
    sprintf(buf, "Skills for %s [%3d %s]\r\n"
	         "------------------------------------\r\n", GET_NAME(vict),
	    GET_LEVEL(vict), class_abbrevs[(int) GET_CLASS(vict)]);
    send_to_char(buf, ch);
    list_skills(ch, vict);
    break;
    
  case 23: /* wizcmds */
    sprintf(buf, "Wiz Command levels\r\n");
    sprintf(buf+strlen(buf), "------------------\r\n");
    for (no =0, cmdno = 1; *cmd_info[cmdno].command != '\n'; cmdno++)
      if (cmd_info[cmdno].minimum_level >= LVL_IMMORT) {
	no++;
	sprintf(buf+strlen(buf), "%-11s [%3d]", cmd_info[cmdno].command, 
		cmd_info[cmdno].minimum_level);
	if (!(no % 3)) {
	  strcat(buf, "\r\n");
	  no = 0;
	} else {
	  strcat(buf, "   ");
	}
      }
    page_string(ch->desc, buf, 1);    
    break;

  case 24: /* noexp */
    strcpy(buf, "Noexp Characters\r\n"
	        "-----------------\r\n");
    for (i = 0; i <= top_of_p_table; i++) {
      if (load_char(((player_table + i)->name), &vbuf) < 0) {
	log("SYSERR: Invalid player in show_frozen command.");
	return;
      }
      if (IS_SET((vbuf.char_specials_saved.act), PLR_NOEXP)) {
	sprintf(buf + strlen(buf), "[%3d %s] %s\r\n", vbuf.level, 
		class_abbrevs[(int) vbuf.class], vbuf.name);
      }
    }
    page_string(ch->desc, buf, 1);
    break;

  case 25: /* lockdown */
    strcpy(buf, "Locked Down Rooms\r\n"
	        "------------------\r\n");
    for (i = 0, j = 0; i < top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_LOCKED_DOWN))
	sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n",
		++j, GET_ROOM_VNUM(i), world[i].name);
    page_string(ch->desc, buf, 1);
    break;

  default:
    send_to_char("Sorry, I don't understand that.\r\n", ch);
    break;
  }
}

void display_zone_to_buf(char *bufptr, int zone, struct char_data *ch)
{

  if((zone_table[zone].status_mode == 0) && (GET_LEVEL(ch) < LVL_IMMORT))
    return;

  if ((zone_table[zone].lvl1 >= 61) && (GET_LEVEL(ch) < LVL_IMMORT))
    return;

  sprintf(bufptr, "%s%-30.30s  %3d to %-10d %s\r\n",
	  bufptr, zone_table[zone].name,
	  zone_table[zone].lvl1, zone_table[zone].lvl2,
	  zone_table[zone].status_mode ? "Open." : "Closed.");
}

ACMD(do_areas)
{
  int i;

  skip_spaces(&argument);

  *buf = '\0';

  strcpy (buf,
	  "                                  Level\r\n"
	  "Name of Area.                  (Min to Max)       Status.\r\n"
	  "-----------------------------------------------------------\r\n" );

  if (!*argument) {
    for (i = 0; i <= top_of_zone_table; i++)
      display_zone_to_buf(buf, i, ch);
    page_string(ch->desc, buf, 1);
    return;
  }
}


#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT(flagset, flags); \
	else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))

struct set_struct {
  const char *cmd;
  char level;
  char pcnpc;
  char type;
} set_fields[] = {
  { "brief",		LVL_GOD, 	PC, 	BINARY },  /* 0 */
  { "invstart", 	LVL_GOD, 	PC, 	BINARY },  /* 1 */
  { "title",		LVL_GOD, 	PC, 	MISC },
  { "nosummon", 	LVL_GRGOD, 	PC, 	BINARY },
  { "maxhit",		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "maxmana", 	        LVL_CREATOR, 	BOTH, 	NUMBER },  /* 5 */
  { "maxmove", 	        LVL_CREATOR, 	BOTH, 	NUMBER },
  { "hit", 		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "mana",		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "move",		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "align",		LVL_CREATOR, 	BOTH, 	NUMBER },  /* 10 */
  { "str",		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "stradd",		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "int", 		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "wis", 		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "dex", 		LVL_CREATOR, 	BOTH, 	NUMBER },  /* 15 */
  { "con", 		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "sex", 		LVL_CREATOR, 	BOTH, 	MISC },
  { "ac", 		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "gold",		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "bank",		LVL_CREATOR, 	PC, 	NUMBER },  /* 20 */
  { "exp", 		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "hitroll", 	        LVL_CREATOR, 	BOTH, 	NUMBER },
  { "damroll",  	LVL_CREATOR, 	BOTH, 	NUMBER },
  { "invis",		LVL_LOWIMPL, 	PC, 	NUMBER },
  { "nohassle", 	LVL_GRGOD, 	PC, 	BINARY },  /* 25 */
  { "frozen",		LVL_FREEZE, 	PC, 	BINARY },
  { "speaking",         LVL_LOWIMPL,    PC,     NUMBER},
  { "practices", 	LVL_CREATOR, 	PC, 	NUMBER },
  { "drunk",		LVL_SUPREME, 	BOTH, 	MISC },
  { "hunger",		LVL_SUPREME, 	BOTH, 	MISC },    /* 30 */
  { "thirst",		LVL_SUPREME, 	BOTH, 	MISC },
  { "killer",		LVL_LOWIMPL, 	PC, 	BINARY },
  { "thief",		LVL_LOWIMPL, 	PC, 	BINARY },
  { "level",		LVL_LOWIMPL, 	BOTH, 	NUMBER },
  { "room",		LVL_LOWIMPL, 	BOTH, 	NUMBER },  /* 35 */
  { "roomflag", 	LVL_LOWIMPL, 	PC, 	BINARY },
  { "siteok",		LVL_CREATOR, 	PC, 	BINARY },
  { "deleted", 	        LVL_LOWIMPL, 	PC, 	BINARY },
  { "class",		LVL_LOWIMPL, 	BOTH, 	MISC },
  { "nowizlist", 	LVL_LOWIMPL, 	PC, 	BINARY },  /* 40 */
  { "quest",		LVL_GRGOD, 	PC, 	BINARY },
  { "loadroom", 	LVL_LOWIMPL, 	PC, 	MISC },
  { "color",		LVL_GOD, 	PC, 	BINARY },
  { "idnum",		LVL_IMPL, 	PC, 	NUMBER },
  { "passwd",		LVL_LOWIMPL, 	PC, 	MISC },    /* 45 */
  { "nodelete", 	LVL_CREATOR, 	PC, 	BINARY },
  { "end",		LVL_CREATOR, 	BOTH, 	NUMBER },
  { "olca",		LVL_CREATOR, 	PC, 	NUMBER },
  { "olcb",		LVL_CREATOR, 	PC, 	NUMBER },
  { "olcc",		LVL_CREATOR, 	PC, 	NUMBER }, /* 50 */
  { "switch",		LVL_LOWIMPL, 	PC, 	BINARY },
  { "pk",		LVL_LOWIMPL, 	PC, 	BINARY },
  { "qpoints",          LVL_CREATOR,    BOTH,   NUMBER},
  { "married",          LVL_LOWIMPL,    PC,     NUMBER},
  { "office",           LVL_LOWIMPL,    PC,     NUMBER},  /* 55 */
  { "world",            LVL_LOWIMPL,    PC,     BINARY},
  { "private",          LVL_SUPREME,    PC,     NUMBER},
  { "tester",           LVL_LOWIMPL,    PC,     BINARY},
  { "maxstam", 	        LVL_CREATOR, 	BOTH, 	NUMBER },
  { "stamina", 		LVL_CREATOR, 	BOTH, 	NUMBER }, /* 60 */
  { "noexp",            LVL_SUPREME,    PC,     BINARY },

  { "\n", 0, BOTH, MISC }
};


int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
  int i, on = 0, off = 0, value = 0;
  char output[MAX_STRING_LENGTH];

  /* Check to make sure all the levels are correct */
  if (GET_LEVEL(ch) != LVL_IMPL) {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
      send_to_char("Maybe that's not such a great idea...\r\n", ch);
      return 0;
    }
  }

  if (GET_LEVEL(ch) < set_fields[mode].level) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return 0;
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
    send_to_char("You can't do that to a beast!\r\n", ch);
    return 0;
  } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char("That can only be done to a beast!\r\n", ch);
    return 0;
  }
  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char("Value must be 'on' or 'off'.\r\n", ch);
      return 0;
    }
    sprintf(output, "%s %s for %s.", set_fields[mode].cmd, ONOFF(on),
	    GET_NAME(vict));
  } else if (set_fields[mode].type == NUMBER) {
    value = atoi(val_arg);
    sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
	    set_fields[mode].cmd, value);
  } else {
    strcpy(output, "Okay.");  /* can't use OK macro here 'cause of \r\n */
  }

  switch (mode) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 2:
    if ((strstr(val_arg, "(") || strstr(val_arg, ")")) 
	&& (GET_LEVEL(ch) < LVL_LOWIMPL)) {
      send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
      return 0;
    }
    set_title(vict, val_arg);
    sprintf(output, "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    sprintf(output, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 4:
    vict->points.max_hit = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 5:
    vict->points.max_mana = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 6:
    vict->points.max_move = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 7:
    vict->points.hit = RANGE(-9, vict->points.max_hit);
    affect_total(vict);
    break;
  case 8:
    vict->points.mana = RANGE(0, vict->points.max_mana);
    affect_total(vict);
    break;
  case 9:
    vict->points.move = RANGE(0, vict->points.max_move);
    affect_total(vict);
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;
  case 11:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_ARCHWIZ)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.str = value;
    vict->real_abils.str_add = 0;
    affect_total(vict);
    break;
  case 12:
    vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
    break;
  case 13:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_ARCHWIZ)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.intel = value;
    affect_total(vict);
    break;
  case 14:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_ARCHWIZ)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.wis = value;
    affect_total(vict);
    break;
  case 15:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_ARCHWIZ)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.dex = value;
    affect_total(vict);
    break;
  case 16:
    if (IS_NPC(vict) || GET_LEVEL(vict) >= LVL_ARCHWIZ)
      RANGE(3, 25);
    else
      RANGE(3, 18);
    vict->real_abils.con = value;
    affect_total(vict);
    break;
  case 17:
    if (!str_cmp(val_arg, "male"))
      vict->player.sex = SEX_MALE;
    else if (!str_cmp(val_arg, "female"))
      vict->player.sex = SEX_FEMALE;
    else if (!str_cmp(val_arg, "neutral"))
      vict->player.sex = SEX_NEUTRAL;
    else {
      send_to_char("Must be 'male', 'female', or 'neutral'.\r\n", ch);
      return 0;
    }
    break;
  case 18:
    vict->points.armor = RANGE(-100, 100);
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 200000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 200000000);
    break;
  case 21:
    /* changed by Nahaz so Da Boss can set exp to EXPMAX */
    if (GET_LEVEL(ch) == LVL_IMPL)
      vict->points.exp = RANGE(0, 2100000000);
    /* imps can set up to imp level. */
    else if (GET_LEVEL(ch) == LVL_LOWIMPL)
      vict->points.exp = RANGE(0, 1852644000);
    /* everyone else can set up to level 80 */
    else
      vict->points.exp = RANGE(0, 507644000);
    break;
  case 22:
    vict->points.hitroll = RANGE(-127, 127);
    affect_total(vict);
    break;
  case 23:
    vict->points.damroll = RANGE(-127, 127);
    affect_total(vict);
    break;
  case 24:
    GET_INVIS_LEV(vict) = RANGE(0, LVL_LOWIMPL);
    break;
  case 25:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict) {
      send_to_char("Better not -- could be a long winter!\r\n", ch);
      return 0;
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 27:
    SPEAKING(vict) = value;
    break;
  case 28:
    GET_PRACTICES(vict) = RANGE(0, 100);
    break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "off")) {
      GET_COND(vict, (mode - 29)) = (char) -1; /* warning: magic number here */
      sprintf(output, "%s's %s now off.", GET_NAME(vict), set_fields[mode].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (mode - 29)) = (char) value; /* and here too */
      sprintf(output, "%s's %s set to %d.", GET_NAME(vict),
	      set_fields[mode].cmd, value);
    } else {
      send_to_char("Must be 'off' or a value from 0 to 24.\r\n", ch);
      return 0;
    }
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 34:
    if (value > GET_LEVEL(ch) || value > LVL_IMPL || 
	GET_IDNUM(vict) == 298 || GET_IDNUM(vict) == 1) {
      send_to_char("You can't do that.\r\n", ch);
      return 0;
    }
    
    RANGE(0, LVL_IMPL);
    vict->player.level = (byte) value;
    break;
  case 35:
    if ((i = real_room(value)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return 0;
    }
    if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
      char_from_room(vict);
    char_to_room(vict, i);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 38:

    if (GET_IDNUM(vict) == 298 || GET_IDNUM(vict) == 1) {
      send_to_char("You can not do that.\r\n", ch);
      return 0;
    }

    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 39:
    if ((i = parse_class(vict, *val_arg)) == CLASS_UNDEFINED) {
      send_to_char("That is not a class.\r\n", ch);
      return 0;
    }
    GET_CLASS(vict) = i;
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (!str_cmp(val_arg, "off")) {
      REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      if (real_room(value) != NOWHERE) {
        SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
	GET_LOADROOM(vict) = value;
	sprintf(output, "%s will enter at room #%d.", GET_NAME(vict),
		GET_LOADROOM(vict));
      } else {
	send_to_char("That room does not exist!\r\n", ch);
	return 0;
      }
    } else {
      send_to_char("Must be 'off' or a room's virtual number.\r\n", ch);
      return 0;
    }
    break;
  case 43:
    SET_OR_REMOVE(TMP_FLAGS(vict), (TMP_COLOR_1 | TMP_COLOR_2));
    break;
  case 44:
    if (IS_NPC(vict) || GET_IDNUM(ch) != 1)
      return 0;
    GET_IDNUM(vict) = value;
    break;
  case 45:
    if (get_char_vis(ch, GET_NAME(vict)))
      return 0;
    
    strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);
    *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
    sprintf(output, "Password changed to '%s'.", val_arg);
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 47:
    RANGE(3, 25);
    vict->real_abils.end = value;
    affect_total(vict);
    break;
  case 48:
    GET_OLC_ZONE1(vict) = value;
    break;
  case 49:
    GET_OLC_ZONE2(vict) = value;
    break;
  case 50:
    GET_OLC_ZONE3(vict) = value;
    break;
  case 51:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SWITCH);
    break;
  case 52:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_PK);
    break;
  case 53:
    /* Allows the setting of Quest points */
    if (value < 0 )
      value = 0;
    
    GET_QUEST(vict) = value;
    break;
  case 54:
    GET_MARRIED(vict) = value;
    break;
  case 55:
    GET_OFFICE(vict) = value;
    break;
  case 56:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_WORLD);
    break;
  case 57:
    GET_PRIVATE(vict) = RANGE(0, 32000);
    break;
  case 58:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_TESTER);
    break;
  case 59:
    vict->points.max_stamina = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 60:
    vict->points.stamina = RANGE(1, vict->points.max_stamina);
    affect_total(vict);
    break;
  case 61:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOEXP);
    break;
  default:
    send_to_char("Can't set that!\r\n", ch);
    return 0;
    break;
  }
  strcat(output, "\r\n");
  send_to_char(CAP(output), ch);
  return 1;
}

ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  struct char_file_u tmp_store;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH],
  val_arg[MAX_INPUT_LENGTH];
  int mode = -1, len = 0, player_i = 0, retval, i, j;
  char is_file = 0, is_mob = 0, is_player = 0;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob")) {
    is_mob = 1;
    half_chop(buf, name, buf);
  }

  half_chop(buf, field, buf);
  strcpy(val_arg, buf);

  if (!*name || !*field) {
    send_to_char("Usage: set [file | player | mob] <victim> <field> <value>.\r\n\r\nFields:\r\n\r\n", ch);
    for (j = 0, i = 1; set_fields[i].level; i++)
      if (set_fields[i].level <= GET_LEVEL(ch))
	sprintf(buf, "%s%-15s%s", buf, set_fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, 0))) {
	send_to_char("There is no such player.\r\n", ch);
	return;
      }
    } else {
      if (!(vict = get_char_vis(ch, name))) {
	send_to_char("There is no such creature.\r\n", ch);
	return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    if ((player_i = load_char(name, &tmp_store)) > -1) {
      store_to_char(&tmp_store, cbuf);
      if (GET_LEVEL(cbuf) > GET_LEVEL(ch) && GET_IDNUM(ch) != 1) {
	free_char(cbuf);
	send_to_char("Sorry, you can't do that.\r\n", ch);
	return;
      }
      vict = cbuf;
    } else {
      free(cbuf);
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strncmp(field, set_fields[mode].cmd, len))
      break;
  /* perform the set */
  retval = perform_set(ch, vict, mode, val_arg);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict, NOWHERE);
    if (is_file) {
      char_to_store(vict, &tmp_store);
      fseek(player_fl, (player_i) * sizeof(struct char_file_u), SEEK_SET);
      fwrite(&tmp_store, sizeof(struct char_file_u), 1, player_fl);
      send_to_char("Saved in file.\r\n", ch);
    }
  }
  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

static const char *logtypes[] = {
  "off", "brief", "normal", "complete", "\n"};

ACMD(do_syslog)
{
  int tp;

  one_argument(argument, arg);

  if (!*arg) {
    tp = ((PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) +
	  (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0));
    sprintf(buf, "Your syslog is currently %s.\r\n", logtypes[tp]);
    send_to_char(buf, ch);
    return;
  }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
    send_to_char("Usage: syslog { Off | Brief | Normal | Complete }\r\n", ch);
    return;
  }
  REMOVE_BIT(PRF_FLAGS(ch), PRF_LOG1 | PRF_LOG2);
  SET_BIT(PRF_FLAGS(ch), (PRF_LOG1 * (tp & 1)) | (PRF_LOG2 * (tp & 2) >> 1));

  sprintf(buf, "Your syslog is now %s.\r\n", logtypes[tp]);
  send_to_char(buf, ch);
}


/* dnsmod */
ACMD(do_dns)
{
  int i;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char ip[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char buf[16384];
  struct dns_entry *dns, *tdns;

  extern struct dns_entry *dns_cache[DNS_HASH_NUM];

  void save_dns_cache(void);

  half_chop(argument, arg1, arg2);

  if(!*arg1) {
    send_to_char("You shouldn't be using this if you don't know what it does!\r\n", ch);
    return;
  }

  if(is_abbrev(arg1, "delete")) {
    if(!*arg2) {
      send_to_char("Delete what?\r\n", ch);
      return;
    }
    CREATE(dns, struct dns_entry, 1);
    if(sscanf(arg2, "%d.%d.%d", dns->ip, dns->ip + 1,
      dns->ip + 2) != 3) {
      send_to_char("Delete what?\r\n", ch);
      return;
    }
    for(i = 0; i < DNS_HASH_NUM; i++) {
      if(dns_cache[i]) {
	for(tdns = dns_cache[i]; tdns; tdns = tdns->next) {
	  if(dns->ip[0] == tdns->ip[0] && dns->ip[1] == tdns->ip[1] &&
	    dns->ip[2] == tdns->ip[2]) {
	    sprintf(arg1, "Deleting %s.\r\n", tdns->name);
	    send_to_char(arg1, ch);
	    tdns->ip[0] = -1;
	  }
	}
      }
    }
    save_dns_cache();
    return;
  } else if(is_abbrev(arg1, "add")) {
    two_arguments(arg2, ip, name);
    if(!*ip || !*name) {
      send_to_char("Add what?\r\n", ch);
      return;
    }
    CREATE(dns, struct dns_entry, 1);
    dns->ip[3] = -1;
    if(sscanf(ip, "%d.%d.%d.%d", dns->ip, dns->ip + 1,
      dns->ip + 2, dns->ip + 3) < 3) {
      send_to_char("Add what?\r\n", ch);
      return;
    }
    i = (dns->ip[0] + dns->ip[1] + dns->ip[2]) % DNS_HASH_NUM;
    dns->name = str_dup(name);
    dns->next = dns_cache[i];
    dns_cache[i] = dns;
    save_dns_cache();
    send_to_char("OK!\r\n", ch);
    return;
  } else if(is_abbrev(arg1, "list")) {
    *buf = '\0';
    for(i = 0; i < DNS_HASH_NUM; i++) {
      if(dns_cache[i]) {
	for(tdns = dns_cache[i]; tdns; tdns = tdns->next) {
	  sprintf(buf, "%s%d.%d.%d.%d %s\r\n", buf, tdns->ip[0],
	    tdns->ip[1], tdns->ip[2], tdns->ip[3], tdns->name);
	}
      }
    }
    page_string(ch->desc, buf, 1);
    return;
  }
}


ACMD(do_tedit) {
   int l, i;
   char field[MAX_INPUT_LENGTH];
   extern char *credits;
   extern char *news;
   extern char *motd;
   extern char *imotd;
   extern char *help;
   extern char *info;
   extern char *background;
   extern char *handbook;
   extern char *newbie;
   extern char *policies;
   extern char *olcrules;

   struct editor_struct {
      char *cmd;
      char level;
      char *buffer;
      int  size;
      char *filename;
   } fields[] = {
      /* edit the lvls to your own needs */
	{ "credits",	LVL_LOWIMPL,	credits,	2400, CREDITS_FILE},
	{ "news",	LVL_GRGOD,	news,		8192,	NEWS_FILE},
	{ "motd",	LVL_SUPREME,	motd,		2400,	MOTD_FILE},
	{ "imotd",	LVL_LOWIMPL,	imotd,		2400, IMOTD_FILE},
	{ "help",       LVL_SUPREME,	help,		2400,	HELP_PAGE_FILE},
	{ "info",	LVL_GRGOD,	info,		8192,	INFO_FILE},
	{ "background",	LVL_CREATOR,	background,	8192,	BACKGROUND_FILE},
	{ "handbook",   LVL_LOWIMPL,	handbook,	8192,HANDBOOK_FILE},
	{ "policies",	LVL_LOWIMPL,	policies,	8192,POLICIES_FILE},
	{ "olcrules",   LVL_LOWIMPL,       olcrules,       8129,OLC_RULES_FILE},
	{ "nhandbook",  LVL_LOWIMPL,       newbie,         8129,NEWBIE_FILE},
	{ "\n",		0,		NULL,		0,	NULL }
   };

   if (ch->desc == NULL) {
      send_to_char("Get outta here you linkdead head!\r\n", ch);
      return;
   }

   half_chop(argument, field, buf);

   if (!*field) {
      strcpy(buf, "Files available to be edited:\r\n");
      i = 1;
      for (l = 0; *fields[l].cmd != '\n'; l++) {
	 if (GET_LEVEL(ch) >= fields[l].level) {
	    sprintf(buf, "%s%-11.11s", buf, fields[l].cmd);
	    if (!(i % 7)) strcat(buf, "\r\n");
	    i++;
	 }
      }
      if (--i % 7) strcat(buf, "\r\n");
      if (i == 0) strcat(buf, "None.\r\n");
      send_to_char(buf, ch);
      return;
   }

   for (l = 0; *(fields[l].cmd) != '\n'; l++)
     if (!strncmp(field, fields[l].cmd, strlen(field)))
       break;

   if (*(fields[l].cmd) == '\n') {
      send_to_char("Invalid text editor option.\r\n", ch);
      return;
   }

   if (GET_LEVEL(ch) < fields[l].level) {
      send_to_char("You are not godly enough for that!\r\n", ch);
      return;
   }

   switch (l) {
    case 0: ch->desc->str = &credits; break;
    case 1: ch->desc->str = &news; break;
    case 2: ch->desc->str = &motd; break;
    case 3: ch->desc->str = &imotd; break;
    case 4: ch->desc->str = &help; break;
    case 5: ch->desc->str = &info; break;
    case 6: ch->desc->str = &background; break;
    case 7: ch->desc->str = &handbook; break;
    case 8: ch->desc->str = &policies; break;
    case 9: ch->desc->str = &olcrules; break;
    case 10: ch->desc->str = &newbie; break;

    default:
      send_to_char("Invalid text editor option.\r\n", ch);
      return;
   }

   /* set up editor stats */
   send_to_char("\x1B[H\x1B[J", ch);
   send_to_char("Edit file below: (/s saves /h for help)\r\n", ch);
   ch->desc->backstr = NULL;
   if (fields[l].buffer) {
      send_to_char(fields[l].buffer, ch);
      ch->desc->backstr = str_dup(fields[l].buffer);
   }
   ch->desc->max_str = fields[l].size;
   ch->desc->mail_to = 0;
   ch->desc->storage = str_dup(fields[l].filename);
   act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
   SET_BIT(PLR_FLAGS(ch), TMP_WRITING);
   STATE(ch->desc) = CON_TEXTED;
}

void show_connections(struct char_data *ch, char *arg)
{
   int zone_num;
   int j,i,k;
   int start_room;

   if (!*arg) {
      send_to_char("USAGE: show connections .\r\n",ch);
      send_to_char("USAGE: show connections <zone_num>\r\n",ch);
      return;
   }
   else if (*arg=='.')
     zone_num=world[ch->in_room].zone;
   else {
     j=atoi(arg);
     for(zone_num=0;zone_num<=top_of_zone_table;zone_num++)
       if(zone_table[zone_num].number==j)
	 break;
   }
   if(zone_num>=0 && zone_num <=top_of_zone_table)
      {
      start_room=zone_table[zone_num].number*100;

      sprintf(buf,"Connections from %-30.30s\r\n"

	      "----------------------------------------------------------------------\r\n",
         zone_table[zone_num].name);
      
      for(i=0,k=0;i<=top_of_world;i++)
	{
	  for(j=0;j<NUM_OF_DIRS;j++)
	    {
	      if(world[i].zone==zone_num    &&
		 world[i].dir_option[j]     &&
		 world[i].dir_option[j]->to_room!=-1 &&
		 world[world[i].dir_option[j]->to_room].zone!=zone_num)
		{
		  sprintf(buf+strlen(buf),"%3d: [%5d] %-23.23s -(%-5.5s)-> [%5d] %-23.23s\r\n",++k,
			  world[i].number, world[i].name,dirs[j],
			  world[world[i].dir_option[j]->to_room].number,
			  world[world[i].dir_option[j]->to_room].name);
		}
	    }	  
	}
      sprintf(buf+strlen(buf),"\r\nConnections to %-30.30s\r\n"
	      
	      "----------------------------------------------------------------------\r\n",
	      zone_table[zone_num].name);
      for(i=0,k=0;i<=top_of_world;i++)
	{
	  for(j=0;j<NUM_OF_DIRS;j++)
	    {
	      if(world[i].zone!=zone_num    &&
		 world[i].dir_option[j]     &&
		 world[i].dir_option[j]->to_room!=-1 &&
		 world[world[i].dir_option[j]->to_room].zone==zone_num)
		{
		  sprintf(buf+strlen(buf),"%3d: [%5d] %-23.23s -(%-5.5s)-> [%5d] %-23.23s\r\n",++k,
			  world[i].number, world[i].name,dirs[j],
			  world[world[i].dir_option[j]->to_room].number,
			  world[world[i].dir_option[j]->to_room].name);
		}
	    }
	}
      if(ch->desc)
	page_string(ch->desc,buf,1);
      return;
      }
}

/* added for disable by Nahaz. */
ACMD(do_disable)
{
  struct char_data *victim = 0;
  int i, j, length, enabled = 0;

  two_arguments(argument, buf1, buf2);
  if (!*buf1) {
    send_to_char("USAGE: disable <player> <command>\r\n", ch);
    return;
  }
  if (!(victim = get_char_vis(ch, buf1))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if (GET_LEVEL(victim) >= GET_LEVEL(ch)) {
    send_to_char("That's REALLY not such a great idea!\r\n", ch);
    return;
  }
  if (!*buf2) {
    sprintf(buf, "Disabled commands for %s:\r\n"
	         "-------------------------------\r\n", GET_NAME(victim));
    for (j = 0; j < 10; j++) {
      if (((victim)->player_specials->saved.disabled[j][0]) != ('\0'))
	sprintf(buf + strlen(buf), "%s\r\n", ((victim)->player_specials->saved.disabled[j]));      
    }
  } else if (!strcmp(buf2, "off")) {
    for (j = 0; j < 10; j++)
      ((victim)->player_specials->saved.disabled[j][0]) = '\0';
    sprintf(buf, "All commands enabled for %s.\r\n", GET_NAME(victim));
  } else {
    for (length = strlen(buf2), i = 0; *cmd_info[i].command != '\n'; i++)
      if (!strncmp(cmd_info[i].command, buf2, length))
	if (GET_LEVEL(victim) >= cmd_info[i].minimum_level)
	  break;
    if (*cmd_info[i].command == '\n') {
      send_to_char("Disable what?\r\n", ch);
      return;
    }
    for (j = 0; j < 10; j++) {
      if (!strcmp(((victim)->player_specials->saved.disabled[j]), cmd_info[i].command)) {
	sprintf(buf, "Command '%s' enabled for %s.\r\n", ((victim)->player_specials->saved.disabled[j]),
		GET_NAME(victim));
	((victim)->player_specials->saved.disabled[j][0]) = '\0';
	enabled = 1;
	break;
      } 
    }
    if (enabled == 0) {
      for (j = 0; j < 10; j++) {
	if (((victim)->player_specials->saved.disabled[j][0]) == ('\0')) {
	  strcpy(((victim)->player_specials->saved.disabled[j]), cmd_info[i].command);
	  sprintf(buf, "Command '%s' disabled for %s.\r\n", ((victim)->player_specials->saved.disabled[j]),
		  GET_NAME(victim));
	  enabled = 0;
	  break;
	} else {
	  sprintf(buf, "No available disable slots for %s.\r\n", GET_NAME(victim));
	}
      }
    }
  }
  send_to_char(buf, ch);
}

ACMD(do_lockdown)
{
    one_argument(argument, arg);
  
  if(!*arg) {
    send_to_char("Usage: lockdown <lock/unlock>\r\n", ch);
    return;
  }
  if(!strcmp("lock", arg)) {
    if(IS_SET(ROOM_FLAGS(ch->in_room), ROOM_LOCKED_DOWN))
      send_to_char("The room is already locked down.\r\n", ch);
    else {
      SET_BIT(ROOM_FLAGS(ch->in_room), ROOM_LOCKED_DOWN);
      act("This room and its occupants are now frozen in place by greater powers!", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("Room locked down.\r\n", ch);
    }
  } else if (!strcmp("unlock", arg)) {
    if(!IS_SET(ROOM_FLAGS(ch->in_room), ROOM_LOCKED_DOWN))
      send_to_char("The room is already unlocked.\r\n", ch);
    else {
      REMOVE_BIT(ROOM_FLAGS(ch->in_room), ROOM_LOCKED_DOWN);
      act("This restraints on this room have been removed!", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("Room lock off.\r\n", ch);
    }
  } else
    send_to_char("Usage: lockdown <lock/unlock>\r\n", ch);
  return;
}
