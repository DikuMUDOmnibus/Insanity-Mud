/* ************************************************************************
*  File: arena.c                                        Part of CircleMUD *
*  Usage: Various functions to run an arena.                              *
*                                                                         *
*                                                                         *
*  Minos Mud 1996/7                                                       *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************* */


#include "conf.h"
#include "sysdep.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"

#define ARENA_ZONE  2       /* Real zone number */
#define PREP_START  200   /* vnum of first prep room */
#define PREP_END    210   /* vnum of last prep room */
#define ARENA_START 211   /* vnum of first real arena room*/
#define ARENA_END   255   /* vnum of last real arena room*/
#define HALL_FAME_FILE "etc/hallfame" /* the arena hall of fame */


struct hall_of_fame_element {
  char	name[MAX_NAME_LENGTH+1];
  time_t date;
  int award;
  struct  hall_of_fame_element *next;
};

/*   external vars  */
extern FILE *player_fl;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern struct title_type titles[NUM_CLASSES][LVL_IMPL + 1];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct int_app_type int_app[36];
extern struct wis_app_type wis_app[36];
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int restrict;
extern int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_p_table;
extern sh_int r_mortal_start_room[];

/* for objects */
extern char *item_types[];
extern char *wear_bits[];
extern char *extra_bits[];
extern char *drinks[];

/* for rooms */
extern char *dirs[];
extern char *room_bits[];
extern char *exit_bits[];
extern char *sector_types[];

/* for chars */
extern char *spells[];
extern char *equipment_types[];
extern char *affected_bits[];
extern char *apply_types[];
extern char *pc_class_types[];
extern char *npc_class_types[];
extern char *action_bits[];
extern char *player_bits[];
extern char *preference_bits[];
extern char *position_types[];
extern char *connected_types[];
extern char *who_abbrevs[];
extern char *class_abbrevs[];

void sportschan(char *);
void start_arena();
void show_jack_pot();
void do_game();
int num_in_arena();
void find_game_winner();
void do_end_game();
void start_game();
void silent_end();
void write_fame_list(void);
void write_one_fame_node(FILE * fp, struct hall_of_fame_element * node);
void load_hall_of_fame(void);
void find_bet_winners(struct char_data *winner);
struct obj_data *read_object(int nr, int type);

int in_arena = 0;
int in_start_arena = 0;
int start_time;
int game_length;
int lo_lim;
int hi_lim;
int cost_per_lev;
int time_to_start;
int time_left_in_game;
int arena_pot;
int bet_pot;

struct hall_of_fame_element *fame_list = NULL;

ACMD(do_arenaload) {

  struct obj_data *obj;
  int r_num, i, num;
  
  num = number(50, 80);
  
  for (i = 1; i <= num; i++) {
    if ((r_num = real_object(number(200, 212))) >= 1) {
      obj = read_object(r_num, REAL);
      obj_to_room(obj, real_room(number(ARENA_START, ARENA_END)));
    }
  }
  mudlogf(CMP, MAX(LVL_ARCHWIZ, GET_INVIS_LEV(ch)), TRUE, 
	  "(GC) %d objects loaded into the arena by %s", num, GET_NAME(ch));
  
  send_to_char("You load the objects into the arena.\r\n", ch);
  return;
}

ACMD(do_bet)
{
  int newbet;
  struct char_data *bet_on;

  two_arguments(argument, arg, buf1);

  if (IS_NPC(ch)) {
    send_to_char("Mobs cant bet on the arena.\r\n",ch);
    return;
  }

  if(!*arg) {
    if(in_start_arena) {
      send_to_char("Usage: bet <player> <amt>\r\n",ch);
      return;
    } else if(!in_start_arena){
      send_to_char("Sorry no arena is in going on.\r\n", ch);
      return;
    } else if(in_arena) {
      send_to_char("Sorry Arena has already started, no more bets.\r\n", ch);
      return;
    }
  }

  if (!in_start_arena)
    send_to_char("Sorry the arena is closed, wait until it opens up to bet.\r\n",ch);
  else if (in_arena)
    send_to_char("Sorry arena is in session, no more bets.\r\n", ch);
  else if (!(bet_on = get_char_vis(ch, arg)))
    send_to_char(NOPERSON, ch);
  else if (bet_on == ch)
    send_to_char("That doesn't make much sense, does it?\r\n", ch);
  else if (!(world[bet_on->in_room].zone == ARENA_ZONE &&
	     ROOM_FLAGGED(IN_ROOM(bet_on), ROOM_ARENA)))
    send_to_char("Sorry that person is not in the arena.\r\n", ch);
  else {

    if(GET_BET_AMT(ch) > 0) {
      send_to_char("You have already made a bet this time!\r\n", ch);
      return;
    }

    GET_BETTED_ON(ch) = bet_on;

    if (!is_number(buf1)) {
      send_to_char("That should be a number idiot!\r\n", ch);
      return;
    }

    newbet = atoi(buf1);

    if (newbet == 0 ) {
      send_to_char("Bet some gold why dont you!\r\n", ch);
      return;
    }

    if (newbet > GET_GOLD(ch) ) {
      send_to_char ("You don't have that much money!\n\r",ch);
      return;
    }

    if ( newbet > 100000000 ) {
      send_to_char("Sorry the house will not accept that much.\r\n", ch);
      return;
    }

    GET_BET_AMT(ch) = newbet;

    *buf2 = '\0';
    GET_GOLD(ch) -= newbet; /* substract the gold - important :) */
    arena_pot += (newbet << 1);
    bet_pot += (newbet << 1);
    sprintf(buf2, "You place %d coins on %s.\r\n", newbet, GET_NAME(bet_on));
    send_to_char(buf2, ch);
    *buf = '\0';
    sprintf(buf,"%s has placed %d coins on %s.", GET_NAME(ch),
	    newbet, GET_NAME(bet_on));
    sportschan(buf);
  }
}

ACMD(do_arena)
{

 if (IS_NPC(ch)){
   send_to_char("Mobs cant play in the arena.\r\n",ch);
   return;
 }
 if(!in_start_arena)
   send_to_char("The arena is closed at the moment.\r\n", ch);
 else if(GET_LEVEL(ch) < lo_lim){
   sprintf(buf, "Sorry but you must be at least level %d to enter this arena.\r\n",
         lo_lim);
   send_to_char(buf, ch);
 }else if( GET_LEVEL(ch) > hi_lim)
     send_to_char("Sorry the arena is not open to your level.\r\n",ch);
 else if( GET_GOLD(ch) < (cost_per_lev * GET_LEVEL(ch))){
    sprintf(buf, "Sorry but you need %d coins to enter the arena\r\n",
           (cost_per_lev * GET_LEVEL(ch)) );
    send_to_char(buf, ch);
 }
 else if(ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA))
   send_to_char("You are in the arena already\r\n",ch);
 else {
   act("$n has been whisked away to join the madness.", FALSE, ch, 0, 0, TO_ROOM);

   char_from_room (ch);
   char_to_room (ch, real_room( number (PREP_START, PREP_END)));
   act("$n is droped from the sky.", FALSE, ch, 0, 0, TO_ROOM);
   send_to_char("You have been taken to the Blood Bath Arena!\r\n",ch);
   look_at_room(ch, 0);

   sprintf(buf, "%s has joined the Blood Bath Arena!", GET_NAME(ch));
   sportschan(buf);

   GET_GOLD(ch) -= ( cost_per_lev * GET_LEVEL(ch));

   arena_pot += ( cost_per_lev * GET_LEVEL(ch));

   sprintf(buf, "You pay %d coins to enter the arena\r\n",
	   (cost_per_lev * GET_LEVEL(ch)));
   send_to_char(buf, ch);

   /* ok lets give them there free restore and take away all there */
   /* effects so they have to recast them spells onthemselfs       */

   GET_HIT(ch) = GET_MAX_HIT(ch);
   GET_MANA(ch) = GET_MAX_MANA(ch);
   GET_MOVE(ch) = GET_MAX_MOVE(ch);

   if (ch->affected)
     while (ch->affected)
       affect_remove(ch, ch->affected);
 }
}


ACMD(do_chaos)
{
  char cost[MAX_INPUT_LENGTH], lolimit[MAX_INPUT_LENGTH];
  char hilimit[MAX_INPUT_LENGTH], start_delay[MAX_INPUT_LENGTH];
  char length[MAX_INPUT_LENGTH];

  half_chop(argument, lolimit, buf);
  lo_lim = atoi(lolimit);
  half_chop(buf, hilimit, buf);
  hi_lim = atoi(hilimit);
  half_chop(buf, start_delay, buf);
  start_time = atoi(start_delay);
  half_chop(buf, cost, buf);
  cost_per_lev = atoi(cost);
  strcpy(length, buf);
  game_length = atoi(length);

  if ( hi_lim >= LVL_IMPL + 1) {
    send_to_char("Please choose a High Level Under the Imps level\r\n", ch);
    return;
  }

  if(!*lolimit || !*hilimit || !*start_delay || !*cost || !*length){
    send_to_char("Usage: astart <Min Level> <Max Level> <Start Delay> <Cost Per Level> <Length>", ch);
    return;
  }

  if (lo_lim >= hi_lim) {
    send_to_char("Ya that just might be smart.\r\n", ch);
    return;
  }

  if ((lo_lim || hi_lim || cost_per_lev || game_length) < 0 ){
    send_to_char("I like positive numbers thank you.\r\n", ch);
    return;
  }

  if ( start_time <= 0) {
    send_to_char("Lets at least give them a chance to enter!\r\n", ch);
    return;
  }

  if ((GET_LEVEL(ch) < LVL_CREATOR) && ( cost_per_lev < 50 )) {
    send_to_char("The Gods have put a minimum of 50 entry fee.\r\n",ch);
    return;
  }

  in_arena = 0;
  in_start_arena = 1;
  time_to_start = start_time;
  time_left_in_game =0;
  arena_pot =0;
  bet_pot = 0;
  start_arena();
}

void start_arena()
{
  if (time_to_start == 0) {
    in_start_arena = 0;
    show_jack_pot();
    in_arena = 1;    /* start the blood shed */
    time_left_in_game = game_length;
    start_game();
  } else {
    if ( time_to_start >1 ) {
      sprintf(buf1, "The Blood Bath Arena is open to levels %d to %d.\r\n",
	      lo_lim, hi_lim);
      sprintf(buf1, "%s%d coins per level to enter. %d hours to start.\r\n", buf1,
	      cost_per_lev, time_to_start);
      sprintf(buf1, "%s\r\nType arena to enter.\r\n", buf1);
    }else{
       sprintf(buf1, "The Blood Bath Arena is open to levels %d to %d.\r\n",
	       lo_lim, hi_lim);
       sprintf(buf1, "%s%d coins per level to enter. 1 hour to start.\r\n", buf1,
	       cost_per_lev);
       sprintf(buf1, "%s\r\nType arena to enter.\r\n", buf1);
     }
    send_to_all(buf1);
    time_to_start--;
  }
}

void show_jack_pot()
{

  sprintf(buf1, "\r\n\r\n\007\007Lets get ready to RUMBLE!!!!!!!!\r\n");
  sprintf(buf1, "%sThe jack pot for this arena is %d coins.\r\n",
          buf1, arena_pot);
  sprintf(buf1, "%s%d coins have been bet on this arena.\r\n\r\n",buf1, bet_pot);
  send_to_all(buf1);

}


void start_game()
{
  register struct char_data *i;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
    if (!d->connected) {
      i = d->character;
      if (world[i->in_room].zone == ARENA_ZONE &&
	  ROOM_FLAGGED(IN_ROOM(i), ROOM_ARENA)
	  && (i->in_room != NOWHERE)) {
	send_to_char("\r\nThe floor falls out from below, droping you in the arena\r\n", i);
	char_from_room(i);
	char_to_room(i, real_room(number(ARENA_START,ARENA_END)));
	look_at_room(i, 0);
      }
    }
  do_game();
}


void do_game()
{
  if ( num_in_arena() == 1) {
    in_arena = 0;
    find_game_winner();
  } else if (time_left_in_game == 0) {
    do_end_game();
  } else if (num_in_arena() == 0) {
    in_arena = 0;
    silent_end();
  } else if (time_left_in_game % 5) {
    sprintf(buf, "With %d hours left in the game there are %d players left.",
            time_left_in_game, num_in_arena());
    sportschan(buf);
  } else if (time_left_in_game == 1) {
    sprintf(buf, "With 1 hour left in the game there are %d players left.",
            num_in_arena());
    sportschan(buf);
  } else
  if (time_left_in_game <= 4) {
    sprintf(buf, "With %d hours left in the game there are %d players left.",
            time_left_in_game, num_in_arena());
    sportschan(buf);
  }
  time_left_in_game--;
}


void find_game_winner()
{
  register struct char_data *i;
  struct descriptor_data *d;
  struct hall_of_fame_element *fame_node;

  for (d = descriptor_list; d; d = d->next)
    if (!d->connected) {
      i = d->character;
      if (world[i->in_room].zone == ARENA_ZONE &&
	  ROOM_FLAGGED(IN_ROOM(i), ROOM_ARENA) && (!IS_NPC(i))
	  && (i->in_room != NOWHERE) && GET_LEVEL(i) < LVL_IMMORT) {

	GET_HIT(i) = GET_MAX_HIT(i);
	GET_MANA(i) = GET_MAX_MANA(i);
	GET_MOVE(i) = GET_MAX_MOVE(i);

	if (i->affected)
	  while (i->affected)
	    affect_remove(i, i->affected);

	char_from_room(i);
	char_to_room(i, r_mortal_start_room[1]);
	look_at_room(i, 0);
	act("$n falls from the sky.", FALSE, i, 0, 0, TO_ROOM);

	if(time_left_in_game == 1){
	  sprintf(buf, "After 1 hour of battle %s is declared the winner",
		  GET_NAME(i));
	  sportschan(buf);
	} else {
	  sprintf(buf, "After %d hours of battle %s is declared the winner",
		  game_length - time_left_in_game, GET_NAME(i));
	  sportschan(buf);
	}

	GET_GOLD(i) += arena_pot/2;
	sprintf(buf, "You have been awarded %d coins for winning the arena\r\n",
		(arena_pot/2));
	send_to_char(buf, i);
	sprintf(buf2, "%s awarded %d coins for winning arena", GET_NAME(i),
		(arena_pot/2));
	sportschan(buf2);

	CREATE(fame_node, struct hall_of_fame_element, 1);
	strncpy(fame_node->name, GET_NAME(i), MAX_NAME_LENGTH);
	fame_node->name[MAX_NAME_LENGTH] = '\0';
	fame_node->date = time(0);
	fame_node->award = (arena_pot/2);
	fame_node->next = fame_list;
	fame_list = fame_node;
	write_fame_list();
	find_bet_winners(i);
      }
    }
}


void silent_end()
{
  in_arena = 0;
  in_start_arena = 0;
  start_time = 0;
  game_length = 0;
  time_to_start = 0;
  time_left_in_game = 0;
  arena_pot = 0;
  bet_pot = 0;
  sprintf(buf, "It looks like no one was brave enough to enter the Arena.");
  sportschan(buf);

}

void do_end_game()
{
  register struct char_data *i;
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
    if (!d->connected) {
      i = d->character;
      if (ROOM_FLAGGED(IN_ROOM(i), ROOM_ARENA)
	  && (i->in_room != NOWHERE) && (!IS_NPC(i))){
        GET_HIT(i) = GET_MAX_HIT(i);
        GET_MANA(i) = GET_MAX_MANA(i);
        GET_MOVE(i) = GET_MAX_MOVE(i);
        if (i->affected)
          while (i->affected)
	    affect_remove(i, i->affected);
        char_from_room(i);
        char_to_room(i, r_mortal_start_room[1]);
        look_at_room(i, 0);
        act("$n falls from the sky.", FALSE, i, 0, 0, TO_ROOM);

      }
    }
  sprintf(buf, "After %d hours of battle the Match is a draw",game_length);
  sportschan(buf);
  time_left_in_game = 0;
}



int num_in_arena()
{
  register struct char_data *i;
  struct descriptor_data *d;
  int num = 0;

  for (d = descriptor_list; d; d = d->next)
    if (!d->connected) {
      i = d->character;
      if (world[i->in_room].zone == ARENA_ZONE &&
	  ROOM_FLAGGED(IN_ROOM(i), ROOM_ARENA)
	  && (i->in_room != NOWHERE)) {
	if(GET_LEVEL(i) < LVL_IMMORT)
	  num++;
      }
    }

  return num;
}


void sportschan(char *argument)
{
  struct descriptor_data *i;
  char color_on[24];

  /* set up the color on code */
  strcpy(color_on, KRED);

  sprintf(buf1, "[ARENA INFO] :: %s\r\n", argument);

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i->character &&
	!PRF_FLAGGED(i->character, PRF_NOSPORTS) &&
	!TMP_FLAGGED(i->character, TMP_WRITING | TMP_OLC | TMP_MAILING) &&
	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      if (COLOR_LEV(i->character) >= C_NRM)
	send_to_char(color_on, i->character);
      act(buf1, FALSE, 0, 0, i->character, TO_VICT | TO_SLEEP);
      if (COLOR_LEV(i->character) >= C_NRM)
	send_to_char(KNRM, i->character);
    }
  }
}


ACMD(do_awho)
{
  struct descriptor_data *d;
  struct char_data *tch;
  int num =0;
  *buf2 = '\0';

  if(!in_arena && !in_start_arena){
    send_to_char("There is no Arena going on right now.\r\n", ch);
	return;
  }

  sprintf(buf,"     Players in the Blood Bath Arena\r\n");
  sprintf(buf,"%s---------------------------------------\r\n", buf);
  sprintf(buf,"%sGame Length = %-3d   Time To Start %-3d\r\n", buf,
	  game_length, time_to_start);
  sprintf(buf,"%sLevel Limits %d to %d\r\n", buf, lo_lim, hi_lim);
  sprintf(buf,"%s         Jackpot = %d\r\n", buf, arena_pot);
  sprintf(buf,"%s---------------------------------------\r\n", buf);
  send_to_char(buf, ch);


  for (d = descriptor_list; d; d = d->next)
    if (!d->connected) {
      tch = d->character;
      if (world[tch->in_room].zone == ARENA_ZONE &&
	  ROOM_FLAGGED(IN_ROOM(tch), ROOM_ARENA)
	  && (tch->in_room != NOWHERE) && GET_LEVEL(tch)<LVL_IMMORT){
	sprintf(buf2, "%s%-11.11s%s", buf2,
		GET_NAME(tch),(!(++num % 5) ? "\r\n" : ""));
      }
    }
  strcat(buf2, "\r\n\r\n");
  send_to_char(buf2, ch);
}


ACMD(do_ahall)
{
  char site[MAX_INPUT_LENGTH], format[MAX_INPUT_LENGTH], *timestr;
  char format2[MAX_INPUT_LENGTH];
  struct hall_of_fame_element *fame_node;

  *buf = '\0';
  *buf2 = '\0';

  if (!fame_list) {
    send_to_char("No-one is in the Hall of Fame.\r\n", ch);
      return;
  }

  sprintf(buf2, "%s|---------------------------------------|%s\r\n",
          CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));
  sprintf(buf2, "%s%s|%sPast Winners of The Blood Bath Arena%s|%s\r\n",
	  buf2, CCBLU(ch, C_NRM), CCNRM(ch, C_NRM),
    	  CCBLU(ch, C_NRM),CCNRM(ch, C_NRM));
  sprintf(buf2, "%s%s|---------------------------------------|%s\r\n\r\n",
          buf2,CCBLU(ch, C_NRM), CCNRM(ch, C_NRM));

  send_to_char(buf2, ch);

  strcpy(format, "%-25.25s  %-10.10s  %-16.16s\r\n");
  sprintf(buf, format,
	  "Name",
	  "Date",
	  "Award Amt");
  send_to_char(buf, ch);
  sprintf(buf, format,
	  "---------------------------------",
	  "---------------------------------",
	  "---------------------------------");

  send_to_char(buf, ch);

  strcpy(format2, "%-25.25s  %-10.10s  %-16d\r\n");

  for (fame_node = fame_list; fame_node; fame_node = fame_node->next) {
    if (fame_node->date) {
      timestr = asctime(localtime(&(fame_node->date)));
      *(timestr + 10) = 0;
      strcpy(site, timestr);
      } else
	strcpy(site, "Unknown");
    sprintf(buf, format2, CAP(fame_node->name), site, fame_node->award);
    send_to_char(buf, ch);
  }

  return;
}



void load_hall_of_fame(void)
{
  FILE *fl;
  int date, award;
  char name[MAX_NAME_LENGTH + 1];
  struct hall_of_fame_element *next_node;

  fame_list = 0;

  if (!(fl = fopen(HALL_FAME_FILE, "r"))) {
    perror("Unable to open hall of fame file");
    return;
  }
  while (fscanf(fl, "%s %d %d", name, &date, &award) == 3) {
    CREATE(next_node, struct hall_of_fame_element, 1);
    strncpy(next_node->name, name, MAX_NAME_LENGTH);
    next_node->name[MAX_NAME_LENGTH] = '\0';
    next_node->date = date;
    next_node->award = award;
    next_node->next = fame_list;
    fame_list = next_node;
  }

  fclose(fl);
}


void write_fame_list(void)
{
  FILE *fl;

  if (!(fl = fopen(HALL_FAME_FILE, "w"))) {
    log("Error writing _hall_of_fame_list");
    return;
  }
  write_one_fame_node(fl, fame_list);/* recursively write from end to start */
  fclose(fl);

  return;
}

void write_one_fame_node(FILE * fp, struct hall_of_fame_element * node)
{
  if (node) {
    write_one_fame_node(fp, node->next);
    fprintf(fp, "%s %ld %d\n",node->name,
	    (long) node->date,
	    node->award);
  }
}

void find_bet_winners(struct char_data *winner)
{
  register struct char_data *i;
  struct descriptor_data *d;

  *buf1 = '\0';

  for (d = descriptor_list; d; d = d->next)
    if (!d->connected) {
      i = d->character;
   if ((!IS_NPC(i)) && (i->in_room != NOWHERE) &&
       (GET_BETTED_ON(i) == winner) && GET_BET_AMT(i) > 0){
     sprintf(buf1, "You have won %d coins on your bet.\r\n",
	     GET_BET_AMT(i)*2);
     send_to_char(buf1, i);
     GET_GOLD(i) += GET_BET_AMT(i)*2;
     GET_BETTED_ON(i) = NULL;
     GET_BET_AMT(i) = 0;
   }
  }
}




