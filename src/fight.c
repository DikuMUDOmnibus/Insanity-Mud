/*************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "clan.h"
#include "loadrooms.h"

#define DAMAGE_LIMIT  800

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern struct index_data *mob_index;
extern struct str_app_type str_app[];
extern struct dex_app_type dex_app[];
extern int auto_save;		/* see config.c */
extern int max_exp_gain;	/* see config.c */
extern int max_exp_loss;       /* see config.c */
extern struct clan_control_rec clan_control[MAX_CLANS];
extern int max_npc_corpse_time, max_pc_corpse_time, max_pieces_time;
extern int thaco[NUM_CLASSES][LVL_IMPL+1];
extern struct index_data *obj_index;
extern struct end_app_type end_app[];
/* External procedures */

int pk_allowed (struct char_data *ch, struct char_data *vict);
int backstab_mult(int level);
int knife_mult(int level);
struct obj_data *create_money(int amount);
char *fread_action(FILE * fl, int nr);
char *fread_string(FILE * fl, char *error);
void stop_follower(struct char_data * ch);
ACMD(do_flee);
ACMD(do_get);
ACMD(do_split);
void improve_skill(struct char_data *ch, int skill);
void hit(struct char_data * ch, struct char_data * victim, int type);
void forget(struct char_data * ch, struct char_data * victim);
void remember(struct char_data * ch, struct char_data * victim);
int ok_damage_shopkeeper(struct char_data * ch, struct char_data * victim);

void sportschan(char *);
/* external vars */
extern sh_int r_corpse_goto_room;
extern sh_int corpse_goto_room;
extern sh_int r_mortal_start_room[NUM_STARTROOMS+1];
extern sh_int r_death_room;
extern sh_int death_room;

/* bodypart code */

#define NUM_OF_FUN_PARTS 21

struct fun_body_piece 
{
  int number;          /* this parts number */
  char name[40];       /* names of this part */
  int  nname;          /* some parts you couldn't trace to an owner */
  char sdesc[128];     /* short desc: that of inventory  */
  char rdesc[128];     /* room desc: when on ground */
  int  take;           /* some body parts don't transfer well */
  char actout[128];    /* what people in room see upon death, using act()*/
  char actkil[128];    /* what the killer sees upon dismemberment, using act() */
};


struct fun_body_piece parts[NUM_OF_FUN_PARTS] = {

  {0,"eyeball eye",1,"the eye of %s","The eyeball of %s is lying here.",
     1,"$n's attack knocks an eye out of $N!",
     "Your attack knocks an eye out of $N!"},
  {1,"liver",1,"the liver of %s","%s's liver is lying here.",
     1,"$n's cruel attack blows $N's liver out!",
     "Your cruel attack blows $N's liver out!"},
  {2,"arm",1,"one of %s's arms","%s's arm is lying here on the ground.",
     1,"$n removes $N's arm!",
     "You remove $N's arm!"},
  {3,"bowels",1,"%s's bowels","Ick. %s's bowels are lying here.",
     1,"$n debowels $N!",
     "You debowl $N!"},
  {4,"tush butt rear ass",1,"%s's rear end",
     "Someone cut off %s's butt and left it here.",
     1,"$n laughs as $e severs $N's rear end!",
     "You laugh as you sever $N's rear end!"},
  {5,"right leg",1,"%s's right leg","%s's right leg is here.",
     1,"$n gracefully cuts $N's leg off! $n chortles merrily!",
     "You watch in awe as $n cuts $N's leg off!"},
  {6,"left leg",1,"the left leg of %s","The left leg of %s is lying here.",
     1,"$n screams and strikes $N's leg off at the hip!",
     "With a scream of rage, you strike $N's leg off!"},
  {7,"head",1,"%s's ugly head","%s's head is lying here, staring at you.",
     1,"$n severs $N's in a move composed of speed and grace!",
     "With speed and grace, you sever $N's head!"},
  {8,"thumb",1,"%s's thumb","One of %s's thumbs is lying here.",
     1,"$n's attack severs a thumb from $N!",
     "Your attack severs a thumb from $N!"},
  {9,"finger",1,"%s's finger","One of %s fingers is lying here.",
     1,"$n's attack severs a finger from $N!",
     "Your attack severs a finger from $N!"},
  {10,"stomach",1,"%s's stomach","%s lost $s stomach here.",
     1,"With animal force, $n tears $N's stomach out!",
     "With animal force, you tear $N's stomach out!"},
  {11,"heart",1,"the once beating heart of %s",
     "%s's once beating heart lies here.",
     1,"$n uses pure strength to eviscerate $N!",
     "Your depend on your fierce strength, and eviscerate $N!"},
  {12,"spine",1,"the spine of %s","The spine of %s is lying here.",
     1,"$n's attack shatters $N's spine!",
     "Your attack shatters $N's spine!"},
  {13,"intestine",0,"An icky pile of intestines",
     "An icky pile of intestines is here - colon and all.",
     0,"$n hits so hard, that $N pukes up his intestines !",
     "You hit $N so hard that he pukes up his intestines!"},
  {14,"puke vomit",0,"chunky vomit","Some one upchucked on the floor here.",
     0,"$N throws up all over!",
     "$N throws up all over you!"},
  {15,"pool blood",0,"A pool of blood","Blood has formed a pool on the ground.",
     0,"$N bleeds horrendously!",
     "$N bleeds horrendously!"},
  {16,"riblet",1,"a meaty %s riblet","A meaty riblet from %s is lying here.",
     1,"$n's explodes $N's chest with a barrage of attacks!",
     "Your cause $N's chest to explode from a barrage of attacks!"},
  {17,"nose",1,"%s's nose","%s lost his nose here.",
     1,"$n cackles gleefuly as he removes $N's nose!",
     "You cackle as you sever $N's nose!"},
  {18,"ear",1,"%s's ear","%'s bloody severed ear is here.",
     1,"$n's grabs $N's ear and rips it off!",
     "Your rip off $N's ear!"},
  {19,"brain",1,"the jiggly brain of %s","The squishy brain of %s is here.",
     1,"$n shatters $N's skull, knocking the brain on the ground!",
     "You shatter $N's skull, knocking the brain on the ground!"},
  {20,"lung",1,"a bloody lung from %s","A blood soaked lung from %s.",
     1,"$N screams his last as $n removes a lung!",
     "$N's screams are cut short as you remove a lung!"}
};



/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

void appear(struct char_data * ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_IMMORT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, TO_ROOM);
}

void make_fun_body_pieces(struct char_data *ch, struct char_data *killer)
{
  struct obj_data *piece;
  int i;

  /* lets check and see if we even GET body parts eh - i mean, they're
     fun, but it wouldn't be quite as fun if they were always there! */

  if ( number(1,5) < 3)
    return;

  /* Then Horray! We's got parts! 
   * But which part? 
   */

  i = number(0,20);   /* 20 pieces should be okay */
  piece = create_obj();

  /*
   * now, everything we have should be in the structures neh?
   * name first
   */

  piece->name=str_dup(parts[i].name);

  /* then lets see about the descs */

  if (parts[i].nname) {
    sprintf(buf2, parts[i].sdesc, GET_NAME(ch));
    piece->short_description = str_dup(buf2);
    sprintf(buf2, parts[i].rdesc, GET_NAME(ch));
    piece->description = str_dup(buf2);
  } else {
    piece->short_description=str_dup(parts[i].sdesc);
    piece->description = str_dup(parts[i].rdesc);
  }

  piece->item_number = NOTHING;
  piece->in_room = NOWHERE;

  /* well, now we know how it looks, lets see if we wanna take it. */
  if (parts[i].take) {
    
    GET_OBJ_WEAR(piece) = ITEM_WEAR_TAKE;
    
    GET_OBJ_TYPE(piece) = ITEM_FOOD;
    GET_OBJ_VAL(piece, 0) = number(1, 24);  
    GET_OBJ_VAL(piece, 3) = 0;  
    
    GET_OBJ_EXTRA(piece) = ITEM_NODONATE;
    GET_OBJ_WEIGHT(piece) = number (1, 5);

    GET_OBJ_TIMER(piece) = max_pieces_time;
    
  } else {
    GET_OBJ_TYPE(piece) = ITEM_TRASH;
  }

  /* And lets see how it got here in the first place neh? */

  act( parts[i].actout, FALSE, killer, 0, ch, TO_ROOM);
  act( parts[i].actkil, FALSE, killer, 0, ch, TO_CHAR);

  /* setup the rest of the stats any object needs */

  obj_to_room(piece, ch->in_room);
}



void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    sprintf(buf2, "SYSERR: Error reading combat message file %s", MESS_FILE);
    perror(buf2);
    exit(1);
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }


  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      fprintf(stderr, "SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data * victim)
{
  if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -2)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -1)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data * ch, struct char_data * vict)
{
  if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    return;

  if (!PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF)
      && !PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(ch) && !IS_NPC(vict) &&
      (ch != vict)) {
    SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
    mudlogf(BRF, LVL_IMMORT, TRUE, "PC Killer bit set on %s for initiating attack on %s at %s.",
	GET_NAME(ch), GET_NAME(vict), world[vict->in_room].name);
    send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
  }
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data * ch, struct char_data * vict)
{
  if (ch == vict)
    return;

  assert(!FIGHTING(ch));

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (IS_AFFECTED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!pk_allowed(ch, vict) && !clan_can_kill(ch, vict))
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data * ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}



void make_corpse(struct char_data * ch)
{
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  corpse->in_room = NOWHERE;
  corpse->name = str_dup("corpse");

  sprintf(buf2, "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = str_dup(buf2);

  sprintf(buf2, "the corpse of %s", GET_NAME(ch));
  corpse->short_description = str_dup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_obj(unequip_char(ch, i), corpse);

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  if ((r_corpse_goto_room = real_room(corpse_goto_room)) < 0) {
    r_corpse_goto_room = r_mortal_start_room[1];
  }

  if ((r_death_room = real_room(death_room)) < 0) {
    r_death_room = r_mortal_start_room[1];
  }

  if (!IS_NPC(ch) && GET_LEVEL(ch) <= 80) {
    obj_to_room(corpse, r_corpse_goto_room);
  } else {
    obj_to_room(corpse, ch->in_room);
  }

}


/* When ch kills victim */
void change_alignment(struct char_data * ch, struct char_data * victim)
{
  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) >> 4;
}



void death_cry(struct char_data * ch)
{
  int door, was_in;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (CAN_GO(ch, door)) {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      act("Your blood freezes as you hear someone's death cry.", FALSE, ch, 0, 0, TO_ROOM);
      ch->in_room = was_in;
    }
  }
}

void raw_kill(struct char_data * ch)
{

  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  death_cry(ch);

  if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) {
    if(IS_NPC(ch)){
      act("$n's body glows brightly for an instant and then slowly fades away"
	  , FALSE, ch, 0, 0, TO_ROOM);
    } else
      act("$n is magically whisked away.", FALSE, ch, 0, 0, TO_ROOM);

    char_from_room(ch);
    char_to_room(ch, r_mortal_start_room[0]);
    look_at_room(ch, 0);
    act("$n falls from the sky.", FALSE, ch, 0, 0, TO_ROOM);
    act("$n looks like he had a bad day", FALSE, ch, 0, 0, TO_ROOM);
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    GET_STAM(ch) = GET_MAX_STAM(ch);
    if (ch->affected)
      while (ch->affected)
	affect_remove(ch, ch->affected);
    GET_POS(ch) = POS_STANDING;
  } else {
    make_corpse(ch);
    if (!IS_NPC(ch) ) {
      send_to_char(
		   "&1               ...  \r\n"
		   "             ;::::; \r\n"
		   "           ;::::; :; \r\n"
		   "         ;:::::'   :; \r\n"
		   "        ;:::::;     ;. \r\n"
		   "       ,:::::'       ;           OOO\\ \r\n"
		   "       ::::::;       ;          OOOOO\\ \r\n"
		   "       ;:::::;       ;         OOOOOOOO \r\n"
		   "      ,;::::::;     ;'         / OOOOOOO \r\n"
		   "    ;:::::::::`. ,,,;.        /  / DOOOOOO \r\n"
		   "  .';:::::::::::::::::;,     /  /     DOOOO \r\n"
		   " ,::::::;::::::;;;;::::;,   /  /        DOOO \r\n"
		   ";`::::::`'::::::;;;::::: ,#/  /          DOOO \r\n"
		   ":`:::::::`;::::::;;::: ;::#  /            DOOO \r\n"
		   "::`:::::::`;:::::::: ;::::# /              DOO \r\n"
		   "`:`:::::::`;:::::: ;::::::#/               DOO \r\n"
		   " :::`:::::::`;; ;:::::::::##                OO \r\n"
		   " ::::`:::::::`;::::::::;:::#                OO \r\n"
		   " `:::::`::::::::::::;'`:;::#                O \r\n"
		   "  `:::::`::::::::;' /  / `:# \r\n"
		   "   ::::::`:::::;'  /  /   `# \r\n"
		   "\r\n&0"
		   "     You feel your soul being ripped from your body,\r\n"
		   "     and you realise that death is upon you!!\r\n\r\n"
		   "     Then Suddenly you are pulled back into the land of\r\n"
		   "     the living... The Gods must like you today!!\r\n"
		   "\r\n", ch);

      char_from_room(ch);
      char_to_room(ch, r_death_room);
      GET_HIT(ch) = GET_MAX_HIT(ch);
      GET_MOVE(ch) = GET_MAX_MOVE(ch);
      GET_MANA(ch) = GET_MAX_MANA(ch);
      GET_STAM(ch) = GET_MAX_STAM(ch);
      GET_POS(ch) = POS_STANDING;

    } else {
      extract_char(ch);
    }
  }
}


void increase_blood(int rm)
{
  RM_BLOOD(rm) = MIN(RM_BLOOD(rm) + 1, 10);
}


void die(struct char_data * ch)
{
  increase_blood(ch->in_room);
  if (!ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    gain_exp(ch, -(GET_EXP(ch) >> 1));
  if (!IS_NPC(ch))
    REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  raw_kill(ch);
}



void perform_group_gain(struct char_data * ch, int base,
			     struct char_data * victim)
{
  int share;

  if (ROOM_FLAGGED(ch->in_room, ROOM_ARENA))
    return;

  if (!IS_NPC(victim))
    return;

  share = MIN(max_exp_gain, MAX(1, base));

  /* added by Nahaz so immortals see no exp gain, and for noexp players. */
  if(GET_LEVEL(ch) >= LVL_IMMORT)
    send_to_char("You receive your share of the experience -- absolutely nothing.\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOEXP)) {
    send_to_char("You cannot gain any experience at the moment.\r\n", ch);
    return;
  } else {
    if (share > 1) {
      sprintf(buf2, "You receive your share of experience -- %d points.\r\n", share);
      send_to_char(buf2, ch);
    } else
      send_to_char("You receive your share of experience -- one measly little point!\r\n", ch);
  }

  gain_exp(ch, share);
  change_alignment(ch, victim);
}


void group_gain(struct char_data * ch, struct char_data * victim)
{
  int tot_members, base, tot_gain;
  struct char_data *k;
  struct follow_type *f;

  if(!IS_NPC(victim))
	return;

  if (!(k = ch->master))
    k = ch;

  if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      tot_members++;

  /* round up to the next highest tot_members */
  tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

  if (tot_members >= 1)
    base = MAX(1, tot_gain / tot_members);
  else
    base = 0;

  if (IS_AFFECTED(k, AFF_GROUP) && k->in_room == ch->in_room)
    perform_group_gain(k, base, victim);

  for (f = k->followers; f; f = f->next)
    if (IS_AFFECTED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
      perform_group_gain(f->follower, base, victim);
}

void solo_gain(struct char_data * ch, struct char_data * victim)
{
  int exp;

  if (!IS_NPC(victim))
	return;

  exp = MIN(max_exp_gain, GET_EXP(victim) / 3);

  /* Calculate level-difference bonus */
  if (IS_NPC(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
  else
    exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);

  exp = MAX(exp, 1);

  /* Added by Nahaz for imms and noexp */
  if(GET_LEVEL(ch) >= LVL_IMMORT)
    send_to_char("You receive absolutely no experience whatsoever.\r\n", ch);
  else if (PLR_FLAGGED(ch, PLR_NOEXP)) {
    send_to_char("You cannot gain any experience at the moment.\r\n", ch);
    return;
  } else {
    if (exp > 1) {
      sprintf(buf2, "You receive %d experience points.\r\n", exp);
      send_to_char(buf2, ch);
    } else {
      send_to_char("You receive one lousy experience point.\r\n", ch);
    }
  }
  gain_exp(ch, exp);
  change_alignment(ch, victim);
}


char *replace_string(const char *str, const char *weapon_singular,
	const char *weapon_plural)
{
  static char buf[256];
  char *cp;

  cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data * ch, struct char_data * victim,
		      int w_type)
{
  char *buf;
  int msgnum;

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N, but misses.",	/* 0: 0     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
    },

    {
      "$n tickles $N as $e #W $M.",	/* 1: 1..2  */
      "You tickle $N as you #w $M.",
      "$n tickles you as $e #W you."
    },

    {
      "$n barely #W $N.",		/* 2: 3..4  */
      "You barely #w $N.",
      "$n barely #W you."
    },

    {
      "$n #W $N.",			/* 3: 5..6  */
      "You #w $N.",
      "$n #W you."
    },

    {
      "$n #W $N hard.",			/* 4: 7..10  */
      "You #w $N hard.",
      "$n #W you hard."
    },

    {
      "$n #W $N very hard.",		/* 5: 11..14  */
      "You #w $N very hard.",
      "$n #W you very hard."
    },

    {
      "$n #W $N extremely hard.",	/* 6: 15..19  */
      "You #w $N extremely hard.",
      "$n #W you extremely hard."
    },

    {
      "$n mauls $N to smithereens, with a #w.",	/* 7: 19..23 */
      "You maul $N to smithereens, with a #w.",
      "$n mauls you to smithereens, with #w."
    },

    {
      "$n decimates $N with a brutal #w.",	/* 8: 23..28 */
      "You decimate $N with a brutal #w.",
      "$n decimates you with a brutal #w."
    },

    {
      "With a mighty #w, $n devastates $N.",	/* 9: 28..32 */
      "You totally devastate $N.",
      "$n devastates you fully."
    },

    {
      "$n #W $N and maims $M.",	/* 10: 32..36 */
      "You #w $N and maim $M.",
      "$n #W you and maims you."
    },

    {
      "$n #W and mutilates $N.",	/* 11: 36..44 */
      "You #w and mutilate $N.",
      "$n #W and mutilates you."
    },

    {
      "$n massacres $N to small fragments, with a deadly #w.",	/* 12:44..50 */
      "You massacre $N to small fragments, with your deadly #w.",
      "$n massacres you to small fragments, with a deadly #w."
    },

    {
      "$n's #w critically DEMOLISHES $N.",	/* 13: 50..100 */
      "Your #w critically DEMOLISHES $N.",
      "$n's #w critically DEMOLISHES you."
    },

    {
      "$n's #w ANNIHILATES $N's vitals.",	/* 14: 100..150 */
      "Your #w ANNIHILATES $N's vitals.",
      "$n's #w ANNIHILATES your vitals."
    },

    {
      "$n OBLITERATES $N with a deadly #w!!",	/* 15: 150..250   */
      "You OBLITERATE $N with a deadly #w!!",
      "$n OBLITERATES you with a deadly #w!!"
    },

    {
      "$n DISINTERATES $N with a LETHAL #w!!",	/* 16: > 250   */
      "You DISINTEGRATE $N with a LETHAL #w!!",
      "$n DISINTEGRATES you with a LETHAL #w!!"
    }

  };

  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 4)    msgnum = 2;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 23)   msgnum = 7;
  else if (dam <= 28)   msgnum = 8;
  else if (dam <= 32)   msgnum = 9;
  else if (dam <= 36)   msgnum = 10;
  else if (dam <= 44)   msgnum = 11;
  else if (dam <= 54)   msgnum = 12;
  else if (dam <= 99)   msgnum = 13;
  else if (dam <= 150)  msgnum = 14;
  else if (dam <= 250) 	msgnum = 15;
  else 			msgnum = 16;


  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  send_to_char(CCYEL(ch, C_CMP), ch);
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_DAMAGE)) {
    sprintf(buf, "%s--< %s%s%s >-< %s%d%s >-----< %s%s%s >-< %s%d%s >--\r\n",
	    CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_NAME(ch), CCNRM(ch, C_NRM),
	    CCGRN(ch, C_NRM), GET_HIT(ch), CCNRM(ch, C_NRM),
	    CCRED(ch, C_NRM), GET_NAME(victim), CCNRM(ch, C_NRM),
	    CCRED(ch, C_NRM), GET_HIT(victim), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
  }
  /* damage message to damagee */
  send_to_char(CCRED(victim, C_CMP), victim);
  buf = replace_string(dam_weapons[msgnum].to_victim,
		       attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(victim, C_CMP), victim);

  if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_DAMAGE)) {
    sprintf(buf, "%s--< %s%s%s >-< %s%d%s >-----< %s%s%s >-< %s%d%s >--\r\n",
	    CCNRM(victim, C_NRM), CCGRN(victim, C_NRM), GET_NAME(victim), CCNRM(victim, C_NRM),
	    CCGRN(victim, C_NRM), GET_HIT(victim), CCNRM(victim, C_NRM),
	    CCRED(victim, C_NRM), GET_NAME(ch), CCNRM(victim, C_NRM),
	    CCRED(victim, C_NRM), GET_HIT(ch), CCNRM(victim, C_NRM));
    send_to_char(buf, victim);
  }
}

/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data * ch, struct char_data * vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && ((GET_LEVEL(vict) >= LVL_IMMORT))) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
	if (GET_POS(vict) == POS_DEAD) {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
	  send_to_char(CCYEL(ch, C_CMP), ch);
	  act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	  send_to_char(CCNRM(ch, C_CMP), ch);

	  send_to_char(CCRED(vict, C_CMP), vict);
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	  send_to_char(CCNRM(vict, C_CMP), vict);

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
	send_to_char(CCYEL(ch, C_CMP), ch);
	act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	send_to_char(CCNRM(ch, C_CMP), ch);

	send_to_char(CCRED(vict, C_CMP), vict);
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(vict, C_CMP), vict);

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return 1;
    }
  }
  return 0;
}


void damage(struct char_data * ch, struct char_data * victim, int dam,
	    int attacktype)
{
  int clan;
  long local_gold = 0;
  long player_old_gold = 0;

  if (GET_POS(victim) <= POS_DEAD) {
    log("SYSERR: Attempt to damage a corpse. (Room #%d)", world[ch->in_room].number);
    die(victim);
    return;			/* -je, 7/7/92 */
  }

  /* peaceful rooms - but let imps attack */
  if (ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL) && GET_LEVEL(ch) < LVL_LOWIMPL) {
    send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
    return;
  }

  /* no pkilling AFK ppl */
  if (PLR_FLAGGED (victim, PLR_AFK) && !IS_NPC(ch)) { /* This Varies */
    send_to_char("How cheap!  Can't you see the AFK flag?!?  You make me sick!\r\n", ch); 
    SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
    return;
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return;

  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT))
    dam = 0;

  if (victim != ch) {

    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, victim);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

  /* If the attacker is invisible, he becomes visible */
  if (IS_AFFECTED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (IS_AFFECTED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;

  /* check for pk and clan stuff */
  if (!pk_allowed(ch, victim) && !clan_can_kill(ch, victim)) {
    check_killer(ch, victim);

    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  /* Set the maximum damage per round and subtract the hit points */

  if (IS_NPC(ch) || GET_LEVEL(ch) >= LVL_SUPREME)
    dam = MAX(dam, 0);
  else
    dam = MAX(MIN(dam, 260), 0);

  GET_HIT(victim) -= dam;
  /* added by Nahaz for noexp */
  if (!ROOM_FLAGGED(ch->in_room, ROOM_ARENA) && ch != victim && 
      (!PLR_FLAGGED(ch, PLR_NOEXP)))
    gain_exp(ch, GET_LEVEL(victim) * dam);

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   *
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (attacktype != -1) {
    if (!IS_WEAPON(attacktype))
      skill_message(dam, ch, victim, attacktype);
    else {
      if (GET_POS(victim) <= POS_DEAD || dam == 0) {
	if (!skill_message(dam, ch, victim, attacktype))
	  dam_message(dam, ch, victim, attacktype);

      } else {
	dam_message(dam, ch, victim, attacktype);
      }
    }
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are mortally wounded, and will die soon, if not aided.\r\n", victim);
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", victim);
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You're stunned, but will probably regain consciousness again.\r\n", victim);
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are dead!  Sorry...\r\n", victim);
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      act("That really did HURT!", FALSE, victim, 0, 0, TO_CHAR);

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
      sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
	      CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
      send_to_char(buf2, victim);
      if (IS_NPC(victim) && (ch != victim) && MOB_FLAGGED(victim, MOB_WIMPY))
	do_flee(victim, '\0', 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
      send_to_char("You wimp out, and attempt to flee!\r\n", victim);
      do_flee(victim, '\0', 0, 0);
    }
    break;
  }

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc)) {
    do_flee(victim, '\0', 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = victim->in_room;
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  /* stop someone from fighting if they're stunned or worse */
  if ((GET_POS(victim) <= POS_STUNNED) && (FIGHTING(victim) != NULL))
    stop_fighting(victim);

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD) {
    if ((ch != victim) && (IS_NPC(victim) || victim->desc) && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) {
      if (IS_AFFECTED(ch, AFF_GROUP))
	group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }

    /* check for clan points awarded */
    if (!IS_NPC(ch) && MOB_FLAGGED(victim, MOB_CLAN)) {
      clan = is_in_clan(ch);
      if (clan >= 0) {
	sprintf(buf2, "You gain %d clan points for your clan!\r\n", GET_LEVEL(victim) * 1000);
	send_to_char(buf2, ch);
	clan_control[clan].clan_points += GET_LEVEL(victim) * 1000;
      }
    }

    if (!IS_NPC(victim) && !IS_NPC(ch) && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) {

      mudlogf(BRF, LVL_OVERSEER, TRUE, "%s [%d] PK'd by %s [%d] at Room #%d", 
	      GET_NAME(victim), GET_LEVEL(victim), GET_NAME(ch), GET_LEVEL(ch), 
	      world[victim->in_room].number);
      sprintf (buf, "-=-=-=-= %s has just died at the hands of a fellow mortal! =-=-=-=-\r\n",
	       GET_NAME(victim));
      send_to_most (buf);

    } else if (!IS_NPC(victim) && !ROOM_FLAGGED(ch->in_room, ROOM_ARENA)) {
      
      mudlogf(BRF, LVL_OVERSEER, TRUE, "%s killed by %s at Room #%d", GET_NAME(victim),
	      GET_NAME(ch), world[victim->in_room].number);
      sprintf (buf, "-=-=-=-=-=-=-= %s has just died painfully! =-=-=-=-=-=-=-\r\n",
	       GET_NAME(victim));
      send_to_most (buf);


      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }

    if (ROOM_FLAGGED(victim->in_room, ROOM_ARENA)) {
      sprintf(buf2, "%s has been defeated by %s", GET_NAME(victim),
	      GET_NAME(ch));
      sportschan(buf2);
    }

    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
    }

    if (IS_NPC(ch)) {
      GET_MOBDEATH(victim)++;
    } else if (!IS_NPC(victim)) {
      GET_PKDEATH(victim)++;
      GET_PKKILLS(ch)++;
    }

    make_fun_body_pieces(victim, ch);
    die(victim);
    
    /* Altered by Nahaz */

    /* this is required to make sure that we got the gold before
       we autosplit */
    player_old_gold = GET_GOLD(ch);

    /* If Autoloot enabled, get all corpse */
    if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {  
      strcpy(buf2, "get all corpse");
      command_interpreter(ch, buf2);
    }
    /* no autoloot but autogold is on. */
    if (local_gold > 0) {
      if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOGOLD) 
	  && !(PRF_FLAGGED(ch, PRF_AUTOLOOT))) {
	strcpy(buf2, "get all.gold corpse");
	command_interpreter(ch, buf2);
      }

      /* If Autoloot AND AutoSplit AND we got money, split with group */
      if (IS_AFFECTED(ch, AFF_GROUP) && PRF_FLAGGED(ch, PRF_AUTOSPLIT) && 
	  ((PRF_FLAGGED(ch,PRF_AUTOLOOT)) ||
	   (PRF_FLAGGED(ch,PRF_AUTOGOLD))) && 
	  ((GET_GOLD(ch) - local_gold) == player_old_gold)) {
	sprintf(buf2, "split %ld", local_gold);
	command_interpreter(ch, buf2);
      }
    }
    /* End of Alteration */
    
    /* commented out by Nahaz */
    /*    if (local_gold > 0)
      if (IS_AFFECTED(ch, AFF_GROUP)) {
	if (PRF_FLAGGED(ch, PRF_AUTOSPLIT)) {
	  strcpy(buf2, "get gold corpse");
	  command_interpreter(ch, buf2);
	  sprintf(buf2, "%d", (int)local_gold);
	  do_split(ch, buf2, 0, 0);
	}
      } else if (PRF_FLAGGED(ch, PRF_AUTOGOLD)) {
	strcpy(buf2, "get gold corpse");
	command_interpreter(ch, buf2);
      }*/

    /* autoloot - Fixed thank god by Zeus after about 5 hours debuging */
    /*    strcpy(buf2, "get all corpse");
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
      command_interpreter(ch, buf2);
    }*/
  }
}




void hit(struct char_data * ch, struct char_data * victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int w_type, victim_ac, calc_thaco, dam, diceroll;

  if (ch->in_room != victim->in_room) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && (ch->mob_specials.attack_type != 0))
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  /* Calculate the THAC0 of the attacker */
  if (!IS_NPC(ch))
    calc_thaco = thaco[(int) GET_CLASS(ch)][(int) GET_LEVEL(ch)];
  else		/* THAC0 for monsters is set in the HitRoll */
    calc_thaco = 20;

  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
  calc_thaco -= GET_HITROLL(ch);
  calc_thaco -= (int) ((GET_INT(ch) - 13) / 1.5);	/* Intelligence helps! */
  calc_thaco -= (int) ((GET_WIS(ch) - 13) / 1.5);	/* So does wisdom */

  /* Calculate the raw armor including magic armor.  Lower AC is better. */
  victim_ac = GET_AC(victim) / 10;

  if (AWAKE(victim))
    victim_ac += dex_app[GET_DEX(victim)].defensive;

  victim_ac = MAX(-10, victim_ac);	/* -10 is lowest */

  /* roll the die and take your chances... */
  diceroll = number(1, 20);

  /* we take some stamina off you here */

  /* decide whether this is a hit or a miss */
  if ((((diceroll < 20) && AWAKE(victim)) &&
       ((diceroll == 1) || ((calc_thaco - diceroll) > victim_ac)))) {
    if (type == SKILL_BACKSTAB)
      damage(ch, victim, 0, SKILL_BACKSTAB);
    else if (type == SKILL_KNIFE)
      damage(ch, victim, 0, SKILL_KNIFE);
    else
      damage(ch, victim, 0, w_type);
  } else {
    if (number(1, 121) <= GET_SKILL(victim, SKILL_PARRY)) {
      act("$N parries your attack.", FALSE, ch, 0, victim, TO_CHAR);
      act("You parry $n's attack.", FALSE, ch, 0, victim, TO_VICT);
      improve_skill(ch, SKILL_PARRY);
      if (!(FIGHTING(victim)))
		set_fighting(victim, ch);
      if (!(FIGHTING(ch)))
		set_fighting(ch, victim);
    } else if (GET_STAM(ch) <= 0 && !IS_NPC(ch)) {
			act ("You try to hit $N but you are to exhuasted!", FALSE, ch, 0, victim, TO_CHAR);
			act ("$n tries to hit you but looks too exhausted!", FALSE, ch, 0, victim, TO_VICT);
			if (!(FIGHTING(victim)))
				set_fighting(victim, ch);
		    if (!(FIGHTING(ch)))
				set_fighting(ch, victim);
	} else {

      /* okay, we know the guy has been hit.  now calculate damage. */
      dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
      dam += GET_DAMROLL(ch);

      if (wielded) {
	/* Add weapon-based damage if a weapon is being wielded */
	dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
      } else {
	if (IS_NPC(ch)) {
	  /* If no weapon, add bare hand damage instead */
	  dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
	} else {
	  dam += number(0, 2);	/* Max. 2 dam with bare hands */
	}
      }

      if (GET_POS(victim) < POS_FIGHTING)
	dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;
      /* Position  sitting  x 1.33 */
      /* Position  resting  x 1.66 */
      /* Position  sleeping x 2.00 */
      /* Position  stunned  x 2.33 */
      /* Position  incap    x 2.66 */
      /* Position  mortally x 3.00 */

      dam = MAX(1, dam);		/* at least 1 hp damage min per hit */

      if (type == SKILL_BACKSTAB) {
	dam *= backstab_mult(GET_LEVEL(ch));
	damage(ch, victim, dam, SKILL_BACKSTAB);
      } else if (type == SKILL_KNIFE) {
	dam *= knife_mult(GET_LEVEL(ch));
	damage(ch, victim, dam, SKILL_KNIFE);
      } else
	damage(ch, victim, dam, w_type);
    }
  }
}

int get_attacks(struct char_data *ch) {

  int j, attacks = 1, obj_attacks = 0;
  struct obj_data *obj = NULL;

  for (j = 0; j < NUM_WEARS; j++) {
    if((obj = GET_EQ(ch, j)))
      if (GET_OBJ_ATT(obj) >= 1)
	obj_attacks += GET_OBJ_ATT(obj);
  }

  attacks += MIN(obj_attacks, 5);

  if (number(1, 110) <= GET_SKILL(ch, SPELL_HASTE)) {
    improve_skill(ch, SPELL_HASTE);
    attacks++;
  }

  if (number(1, 110) <= GET_SKILL(ch, SKILL_SECOND_ATTACK)) {
    improve_skill(ch, SKILL_SECOND_ATTACK);
    attacks++;
  }
  if (number(1, 120) <= GET_SKILL(ch, SKILL_THIRD_ATTACK)) {
    improve_skill(ch, SKILL_THIRD_ATTACK);
    attacks++;
  }

  /* Multiple Attacks on Mobs */
  if (IS_NPC(ch))
    attacks += number (0, GET_MOB_ATTACKS(ch)-1);
  attacks = MIN(8, attacks);

  return attacks;
}

/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
  unsigned int attacks;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

      if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
        stop_fighting(ch);
        continue;
      }

      if (IS_NPC(ch)) {
        if (GET_MOB_WAIT(ch) > 0) {
  	  GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
  	  continue;
        }
      
       GET_MOB_WAIT(ch) = 0;
        if (GET_POS(ch) < POS_FIGHTING) {
   	  GET_POS(ch) = POS_FIGHTING;
 	  act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
        }
      }
      if (GET_POS(ch) < POS_FIGHTING) {
        send_to_char("You can't fight while sitting!!\r\n", ch);
        continue;
      }

     if (number(0, 25) >= GET_END(ch) && !IS_NPC(ch))
          GET_STAM(ch) -= number(0, end_app[GET_END(ch)].stam_loss);


    attacks = get_attacks(ch);

    while(attacks-- > 0) {

      if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
        stop_fighting(ch);
        continue;
      }

      hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    }
    if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL)
      (mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, '\0');
  }
}




