/**************************************************************************
*  File: quest.c                                        Part of CircleMUD *
*  Usage: Handling of the autoquests                                      *
*                                                                         *
*  Insanity Mud 1997/8 - Based on code from autoquest on Merc/Rom         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "comm.h"
#include "db.h"

extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *character_list;
extern struct object_data *object_list;
extern struct object_data *obj_proto;
extern struct char_data *mob_proto;
extern struct zone_data *zone_table;
extern int top_of_mobt;
extern int top_of_world;
extern struct room_data *world;

int real_mobile(int virtual);

#define IS_QUESTOR(ch)     (TMP_FLAGGED(ch, TMP_QUESTOR))

/* Object vnums for Quest Rewards */

#define QUEST_ITEM1 2899
#define QUEST_ITEM2 2898
#define QUEST_ITEM3 2897
#define QUEST_ITEM4 2896
#define QUEST_ITEM5 2895

/* Items for object quests, now moved into a zone so we can have lots and lots
   of them, that way its neater in the code */

#define QUEST_OBJSTART 2800
#define QUEST_OBJEND   2801

/* now mobs */

#define QUEST_MOBSTART 2800
#define QUEST_MOBEND   2804

/* Local functions */

void generate_quest(struct char_data *ch, struct char_data *questman);
void quest_update(void);
int chance(int num);

struct obj_data *create_object(int vnum, int dummy) {

  int r_num;
  struct obj_data *tobj;
  
  if((r_num = real_object(vnum)) < 0)
    tobj = NULL;
  else
    tobj = read_object(r_num, REAL);

  return(tobj);
}

/* CHANCE function. I use this everywhere in my code, very handy :> */

int chance(int num)
{
  if (number(1, 100) <= num) 
    return 1;
  else 
    return 0;
}

/* The main quest function */

ACMD(do_autoquest)
{ 
  char arg2[MAX_STRING_LENGTH], arg1[MAX_STRING_LENGTH];
  struct char_data *questman= NULL;
  struct obj_data *obj=NULL, *obj_next = NULL;
  struct obj_data *questinfoobj = NULL;
  struct char_data *questinfo = NULL;
  
  two_arguments(argument, arg1, arg2);
  
  if (is_abbrev(arg1, "info")) {

    if (IS_QUESTOR(ch)) {
      if ((GET_QUESTMOB(ch) == -1) && (GET_QUESTGIVER(ch) != NULL)) {
	sprintf(buf, "Your quest is almost complete!\r\nGet back to %s before your time runs out!\r\n",
		GET_NAME(GET_QUESTGIVER(ch)));
	send_to_char(buf, ch);
      } 

      else if (GET_QUESTOBJ(ch) > 0) {

	questinfoobj = get_obj_num(real_object(GET_QUESTOBJ(ch)));

	if (questinfoobj != NULL) {
	  sprintf(buf, "You are on a quest to recover the fabled %s!\r\n", 
		  questinfoobj->name);
	  send_to_char(buf, ch);
	} else 
	  send_to_char("You aren't currently on a quest.\r\n",ch);
	return;
      }
      
      else if (GET_QUESTMOB(ch) > 0) {
	questinfo = get_char_num(real_mobile(GET_QUESTMOB(ch)));
	if (questinfo != NULL) {
	  sprintf(buf, "You are on a quest to slay the dreaded %s!\r\n",
		  GET_NAME(questinfo));
	  send_to_char(buf, ch);
	}
	else 
	  send_to_char("You aren't currently on a quest.\r\n",ch);
	return;
      }
      
    } else
      send_to_char("You aren't currently on a quest.\r\n",ch);
    
    return;
  }
  
  
  /* Points */
  
  if (is_abbrev(arg1, "points")) {
    sprintf(buf, "You have %d quest points.\r\n",GET_QUESTPOINTS(ch));
    send_to_char(buf, ch);
    return;
  }

  /* Time */

  else if (is_abbrev(arg1, "time")) {
    
    if (!IS_QUESTOR(ch)) {
      
      send_to_char("You aren't currently on a quest.\r\n",ch);

      if (GET_NEXTQUEST(ch) > 1) {
	sprintf(buf, "There are %d minutes remaining until you can go on another quest.\r\n",
		GET_NEXTQUEST(ch));
	send_to_char(buf, ch);
      }
      else if (GET_NEXTQUEST(ch) == 1) {
	sprintf(buf, "There is less than a minute remaining until you can go on another quest.\r\n");
	send_to_char(buf, ch);
      }
    }
    else if (GET_COUNTDOWN(ch) > 0) {
      sprintf(buf, "You have %d minutes left to complete your quest.\r\n", GET_COUNTDOWN(ch));
      send_to_char(buf, ch);
    }
    return;
  }

  /* checks for a mob flagged MOB_QUESTMASTER */
  for (questman = world[ch->in_room].people; questman; questman = questman->next_in_room ) {

    if (!IS_NPC(questman)) 
      continue;
    
    if (MOB_FLAGGED(questman, MOB_QUESTMASTER))
      break;
  }

  if (questman == NULL) {
    send_to_char("You can't do that here.\r\n",ch);
    return;
  }

  if ( FIGHTING(questman) != NULL) {
    send_to_char("Wait until the fighting stops.\r\n",ch);
    return;
  }

  GET_QUESTGIVER(ch) = questman;

  /* And, of course, you will need to change the following lines for YOUR
     quest item information. Quest items on Moongate are unbalanced, very
     very nice items, and no one has one yet, because it takes awhile to
     build up quest points :> Make the item worth their while. */
  
  if (is_abbrev(arg1, "list")) {

    act ("$n asks $N for a list of quest items.",FALSE, ch, NULL, questman, TO_ROOM); 
    act ("You ask $N for a list of quest items.",FALSE, ch, NULL, questman, TO_CHAR);

    sprintf(buf, "Current Quest Items available for Purchase:\r\n\r\n"
	    "Need to change this stuff\r\n"
	    "1000qp.........The COMFY CHAIR!!!!!!\r\n"
	    "850qp..........Sword of Vassago\r\n"
	    "750qp..........Amulet of Vassago\r\n"
	    "750qp..........Shield of Vassago\r\n"
	    "550qp..........Decanter of Endless Water\r\n"
	    "500qp..........350,000 gold pieces\r\n"
	    "500qp..........30 Practices\r\n\r\n"
	    "To buy an item, type '&gquest buy <item>&n'.\r\n");
    send_to_char(buf, ch);
    return;
  }

  else if (is_abbrev(arg1, "buy")) {
    
    if (arg2[0] == '\0') {
      send_to_char("To buy an item, type '&gquest buy <item>&n'.\r\n",ch);
      return;
    }

    if (isname(arg2, "amulet")) {

      if (GET_QUESTPOINTS(ch) >= 750) {
	GET_QUESTPOINTS(ch) -= 750;
	obj = create_object(QUEST_ITEM1, GET_LEVEL(ch));
      } else {
	sprintf(buf, "$n says, '&cSorry, %s, but you don't have enough quest points for that.&n'",GET_NAME(ch));
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	return;
      }
    }
    else if (isname(arg2, "shield")) {

      if (GET_QUESTPOINTS(ch) >= 750) {
	GET_QUESTPOINTS(ch) -= 750;
	obj = create_object(QUEST_ITEM2,GET_LEVEL(ch));
      } else {
	sprintf(buf, "$n says, '&cSorry, %s, but you don't have enough quest points for that.&n'",GET_NAME(ch));
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	return;
      }
    }
    else if (isname(arg2, "sword")) {

      if (GET_QUESTPOINTS(ch) >= 850) {
	GET_QUESTPOINTS(ch) -= 850;
	obj = create_object(QUEST_ITEM3, GET_LEVEL(ch));
      } else {
	sprintf(buf, "$n says, '&cSorry, %s, but you don't have enough quest points for that.&n'",GET_NAME(ch));
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	return;
      }
    }
    else if (isname(arg2, "decanter endless water")) {

      if (GET_QUESTPOINTS(ch) >= 550) {
	GET_QUESTPOINTS(ch) -= 550;
	obj = create_object(QUEST_ITEM4, GET_LEVEL(ch));
      } else {
	sprintf(buf, "$n says, '&cSorry, %s, but you don't have enough quest points for that.&n'",GET_NAME(ch));
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	return;
      }
    }
    
    else if (isname(arg2, "chair comfy")) {
      if (GET_QUESTPOINTS(ch) >= 1000) {
	GET_QUESTPOINTS(ch) -= 1000;
	obj = create_object(QUEST_ITEM5,GET_LEVEL(ch));
      } else {
	sprintf(buf, "$n says, '&cSorry, %s, but you don't have enough quest points for that.&n'",GET_NAME(ch));
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	return;
      }
    }
    
    else if (isname(arg2, "practices pracs prac practice")) {
	
      if (GET_QUESTPOINTS(ch) >= 500) {
	GET_QUESTPOINTS(ch) -= 500;
	GET_PRACTICES(ch) += 30;
	act( "$N gives 30 practices to $n.",FALSE, ch, NULL, questman, TO_ROOM );
	act( "$N gives you 30 practices.", FALSE, ch, NULL, questman, TO_CHAR );
	return;
      } else {
	sprintf(buf, "$n says, '&cSorry, %s, but you don't have enough quest points for that.&n'",GET_NAME(ch));
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	return;
      }
    }
    
    else if (isname(arg2, "gold gp")) {
      if (GET_QUESTPOINTS(ch) >= 500) {
	GET_QUESTPOINTS(ch) -= 500;
	GET_GOLD(ch) += 350000;
	act( "$N gives 350,000 gold pieces to $n.",FALSE, ch, NULL, questman, TO_ROOM );
	act( "$N has 350,000 in gold transfered from $s Swiss account to your balance.", 
	    FALSE,  ch, NULL, questman, TO_CHAR );
	return;
      } else {
	sprintf(buf, "$n says, '&cSorry, %s, but you don't have enough quest points for that.&n'" ,GET_NAME(ch));
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	return;
      }
    } else {
      sprintf(buf, "$n says, '&cI don't have that item, %s.&n'", GET_NAME(ch)); 
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      return;
    }
    
    if (obj != NULL) {
      act( "$N gives $p to $n.",FALSE, ch, obj, questman, TO_ROOM );
      act( "$N gives you $p.",FALSE,   ch, obj, questman, TO_CHAR );
      obj_to_char(obj, ch);
    }
    return;
  }
  else if (is_abbrev(arg1, "request")) {

    act( "$n asks $N for a quest.", FALSE, ch, NULL, questman, TO_ROOM); 
    act ("You ask $N for a quest.", FALSE, ch, NULL, questman, TO_CHAR);
    
    if (IS_QUESTOR(ch)) {
      sprintf(buf, "$n says, '&cBut you're already on a quest!&n'");
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      return;
    }
    
    if (GET_NEXTQUEST(ch) > 0) {
      sprintf(buf, "$n says, '&cYou're very brave, %s, but let someone else have a chance.&n'",
	      GET_NAME(ch));
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      sprintf(buf, "$n says, '&cCome back later.&n'");
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      return;
    }

    sprintf(buf, "$n says, '&cThank you, brave %s!&n'", GET_NAME(ch)); 
    act(buf, FALSE, questman, 0, 0, TO_ROOM);
    generate_quest(ch, questman);

    if (GET_QUESTMOB(ch) > 0 || GET_QUESTOBJ(ch) > 0) {
    
      GET_COUNTDOWN(ch) = number(10, 30);
      SET_BIT(TMP_FLAGS(ch), TMP_QUESTOR);
      sprintf(buf, "$n says, '&cYou have %d minutes to complete this quest.&n'", 
	      GET_COUNTDOWN(ch));
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      sprintf(buf, "$n says, '&cMay the gods go with you!&n'");
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
    }
    return;
  }
  
  else if (is_abbrev(arg1, "complete")) {

    act( "$n informs $N $e has completed $s quest.",FALSE, ch, NULL, questman, TO_ROOM); 
    act ("You inform $N you have completed your quest.",FALSE, ch, NULL, questman, TO_CHAR);

    if (GET_QUESTGIVER(ch) != questman) {
      sprintf(buf, "$N says, '&cI never sent you on a quest! Perhaps you're thinking of someone else.&n'");
      act(buf, FALSE, 0, 0, questman, TO_ROOM);
      return;
    }

    if (IS_QUESTOR(ch)) {
      
      if (GET_QUESTMOB(ch) != 0 && GET_COUNTDOWN(ch) > 0) {

	int reward, pointreward, pracreward;

	reward = number(25, 1000);
	pointreward = number(50, 500);
	
	sprintf(buf, "$n says, '&cCongratulations on completing your quest!&n'");
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	sprintf(buf,"$n says, '&cAs a reward, I am giving you %d quest points, and %d gold.&n'",
		pointreward, reward);
	act(buf, FALSE, questman, 0, 0, TO_ROOM);

	if (chance(15)) {
	  pracreward = number(1,6);
	  sprintf(buf, "You gain %d practices!\r\n", pracreward);
	  send_to_char(buf, ch);
	  GET_PRACTICES(ch) += pracreward;
	}

	REMOVE_BIT(TMP_FLAGS(ch), TMP_QUESTOR);
	GET_QUESTGIVER(ch) = NULL;
	GET_COUNTDOWN(ch) = 0;
	GET_QUESTMOB(ch) = 0;
	GET_QUESTOBJ(ch) = 0;
	GET_NEXTQUEST(ch) = 2;
	GET_GOLD(ch) += reward;
	GET_QUESTPOINTS(ch) += pointreward;

	return;
      }
      
      else if (GET_QUESTOBJ(ch) > 0 && GET_COUNTDOWN(ch) > 0) {
	
	bool obj_found = FALSE;
	
	for (obj = ch->carrying; obj != NULL; obj= obj_next) {
	  obj_next = obj->next_content;
        
	  if (obj != NULL && GET_OBJ_VNUM(obj) == GET_QUESTOBJ(ch)) {
	    obj_found = TRUE;
	    break;
	  }
	}
	if (obj_found == TRUE) {

	  int reward, pointreward, pracreward;
	  
	  reward = number(25, 1000);
	  pointreward = number(1000, 5000);
	  
	  act("You hand $p to $N.",FALSE, ch, obj, questman, TO_CHAR);
	  act("$n hands $p to $N.",FALSE, ch, obj, questman, TO_ROOM);

	  sprintf(buf, "$n says, '&cCongratulations on completing your quest!&n'");
	  act(buf, FALSE, questman, 0, 0, TO_ROOM);
	  sprintf(buf,"$n says, '&cAs a reward, I am giving you %d quest points, and %d gold.&n'"
		  ,pointreward, reward);
	  act(buf, FALSE, questman, 0, 0, TO_ROOM);

	  if (chance(15)) {
	    pracreward = number(1,6);
	    sprintf(buf, "You gain %d practices!\r\n", pracreward);
	    send_to_char(buf, ch);
	    GET_PRACTICES(ch) += pracreward;
	  }

	  REMOVE_BIT(TMP_FLAGS(ch), TMP_QUESTOR);
	  GET_QUESTGIVER(ch) = NULL;
	  GET_COUNTDOWN(ch) = 0;
	  GET_QUESTMOB(ch) = 0;
	  GET_QUESTOBJ(ch) = 0;
	  GET_NEXTQUEST(ch) = 2;
	  GET_GOLD(ch) += reward;
	  GET_QUESTPOINTS(ch) += pointreward;
	  extract_obj(obj);
	  return;
	} else {
	  
	  sprintf(buf, "$n says, '&cYou haven't completed the quest yet, but there is still time!&n");
	  act(buf, FALSE, questman, 0, 0, TO_ROOM);
	  return;
	}
	
	return;
      }
      
      else if ((GET_QUESTMOB(ch) > 0 || GET_QUESTOBJ(ch) > 0) && GET_COUNTDOWN(ch) > 0) {
	sprintf(buf, "$n says, '&cYou haven't completed the quest yet, but there is still time!&n");
	act(buf, FALSE, questman, 0, 0, TO_ROOM);
	return;
      }
    }
    
    sprintf(buf, "$n says, '&cYou have to request a quest first, %s.&n'", GET_NAME(ch));
    act(buf, FALSE, questman, 0, 0, TO_ROOM);
    return;
  }

  send_to_char("Usage: &mquest <command>.&n\r\n\r\n"
	       "&yThe following commands are available:&n\r\n\r\n"
	       "&rPoints,     Info,       Time&n\r\n"
	       "Request,    Complete,   Buy\r\n"
	       "&rList.&n\r\n\r\n"
	       "For more information, type '&cHELP QUEST&n'.\r\n", ch);
  return;
}

void generate_quest(struct char_data *ch, struct char_data *questman)
{
  struct char_data *questmob = NULL;
  struct obj_data *questitem = NULL;
  int level_diff, r_num, mcounter, room;

  if(chance(50) && GET_LEVEL(ch) < LVL_OVERSEER) { 
    sprintf(buf, "$n says, '&cI'm sorry, but I don't have any quests for you at this time.&n'");
    act(buf, FALSE, questman, 0, 0, TO_ROOM);
    sprintf(buf, "$n says, '&cTry again later.&n'");
    act(buf, FALSE, questman, 0, 0, TO_ROOM);
    return;
  }
    
  if (chance(50))  { /* prefer item quests myself */
   
    if ((r_num = real_object(number(QUEST_OBJSTART, QUEST_OBJEND))) >= 1)
      questitem = read_object(r_num, REAL);
    
    if(questitem == NULL) {
      log("questitem does not EXIST!!");
      send_to_char("Error: questitem does not exist! please notify the imms\r\n",ch);
      return;
    }
    
    GET_OBJ_TIMER(questitem) = 30;

    /* ok now we find a room to put it in */
    room = number( 1, top_of_world - 1 );

    obj_to_room(questitem, room);
    
    GET_QUESTOBJ(ch) = GET_OBJ_VNUM(questitem);
    sprintf(buf, "$n says, '&c%s has been stolen from the old man who gives you the post!&n'",
	    CAP(questitem->short_description));
    act(buf, FALSE, questman, 0, 0, TO_ROOM);
    
    sprintf(buf, "$n says, '&cLook in the general area of %s for %s!&n'", 
	    zone_table[world[room].zone].name ,
	    world[room].name);
    act(buf, FALSE, questman, 0, 0, TO_ROOM);
    return;
  
    /* Quest to kill a mob */
    

  } else {    

    for (mcounter = 0; mcounter < 99; mcounter++) { 
 	    

      if ((r_num = real_mobile(number(QUEST_MOBSTART, QUEST_MOBEND))) >= 1)
	questmob = read_mobile(r_num, REAL);
      
      if(questmob == NULL) {
	log("questmob does not EXIST!!");
	send_to_char("Error: questitem does not exist! please notify the imms\r\n",ch);
	return;
      }

      level_diff = GET_LEVEL(questmob) - GET_LEVEL(ch);
      
      if (((level_diff > 20 || level_diff < -20 || !MOB_FLAGGED (questmob, MOB_QUEST)) && 
	   GET_LEVEL(ch) < LVL_OVERSEER) || 
	  (GET_LEVEL(ch) >= LVL_OVERSEER && !MOB_FLAGGED (questmob, MOB_QUEST ))) {
	
	char_to_room(questmob, 1);
	extract_char(questmob);
	questmob = NULL;
      }
      
      if(questmob) 
	break;
    }

    if (!questmob) {
      sprintf(buf, "$n says, '&cI'm sorry, but I don't have any quests for you at this time.&n'");
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      sprintf(buf, "$n says, '&cTry again later.&n'");
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      return;   
    }
    
    /* ok now we find a room to put it in */
    room = number( 1, top_of_world - 1 );
    char_to_room(questmob, room);
 
    switch(number(0,1)) {
      
    case 0:
      sprintf(buf, "$n says, '&cAn enemy of mine, %s, is making vile threats against the trees.&n'",
	      GET_NAME(questmob));
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      sprintf(buf, "$n says, '&cThis threat must be eliminated or the trees will get mad!&n'");
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      break;
    
    case 1:
      sprintf(buf, "$n says, '&cInsanity's most heinous lollypop thief, %s, has escaped from the dungeon!&n'",
	      GET_NAME(questmob));
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
      sprintf(buf, "$n says, '&cSince $S escape, %s has stolen %d lollypops from innocent children!&n'",
	      GET_NAME(questmob), number(2,20));
      act(buf, FALSE, questman, 0, questmob, TO_ROOM);
      break;
    }

    if (world[questmob->in_room].name != NULL) {
      sprintf(buf, "$n says, '&cSeek %s out somewhere in the vicinity of %s!&n'",
	      GET_NAME(questmob), world[questmob->in_room].name);
      act(buf, FALSE, questman, 0, 0, TO_ROOM);

      sprintf(buf, "$n says, '&cThat location is in the general area of %s.&n'",
	      zone_table[world[questmob->in_room].zone].name);
      act(buf, FALSE, questman, 0, 0, TO_ROOM);
    }
    GET_QUESTMOB(ch) = GET_MOB_VNUM(questmob);
  }
  return;
}

/* Called from update_handler() by pulse_area */

void quest_update(void)
{
  struct char_data *ch, *ch_next;

  for ( ch = character_list; ch; ch = ch_next ) {
    ch_next = ch->next;

    if (IS_NPC(ch))
      continue;

    if (GET_NEXTQUEST(ch) > 0) {
      GET_NEXTQUEST(ch)--;

      if (GET_NEXTQUEST(ch) == 0) {
	send_to_char("You may now quest again.\r\n",ch);
	return;
      }
    }
    else if (IS_QUESTOR(ch)) {
      if (--GET_COUNTDOWN(ch) <= 0) {
	GET_NEXTQUEST(ch) = 1;
	sprintf(buf, "You have run out of time for your quest!\r\nYou may quest again in %d minutes.\r\n",
		GET_NEXTQUEST(ch));
	send_to_char(buf, ch);
	REMOVE_BIT(TMP_FLAGS(ch), TMP_QUESTOR);
	GET_QUESTGIVER(ch) = NULL;
	GET_COUNTDOWN(ch) = 0;
	GET_QUESTMOB(ch) = 0;
      }
 
      if (GET_COUNTDOWN(ch) > 0 && GET_COUNTDOWN(ch) < 6) {
	send_to_char("Better hurry, you're almost out of time for your quest!\r\n",ch);
	return;
      }
    }
  }
  return;
}
