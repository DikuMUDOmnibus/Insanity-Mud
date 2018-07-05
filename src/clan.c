/* ************************************************************************
*  File: clan.c                                         Part of CircleMUD *
*  Usage: Handling of player clans                                        *
*  (c) 1996/7 Robert Woodward  (Zeus)                                     *
*                                                                         *
*  Minos Mud 1996/7                                                       *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************* */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "clan.h"
#include "screen.h"

extern char *dirs[];
extern struct room_data *world;
extern int top_of_world;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;

struct clan_control_rec clan_control[MAX_CLANS];
int num_of_clans = 0;
int castles[MAX_CASTLES];
int num_of_castles = 0;

/******************************************************************
 *  Functions for clan administration (creation, deletion, etc.)  *
 *****************************************************************/

/* is in clan checks to see if a character is in a clan 
   if they are it returns the clan subsript of the possition
   in the array. Else it returns -1 */

int is_in_clan( struct char_data *ch ) {

  int i, j;

  for (i = 0; i < num_of_clans; i++) {
    if (clan_control[i].leader == GET_IDNUM(ch))
      return i;
  
    for (j = 0; j < clan_control[i].num_of_members; j++)
      if (GET_IDNUM(ch) == clan_control[i].members[j]) {
	return i;
      }
  }
  
  return -1;

}

int find_clan(int vnum)
{
  int i;

  for (i = 0; i < num_of_clans; i++)
    if (clan_control[i].room == vnum)
      return i;

  return -1;
}

int find_clan_num(int num)
{
  int i;

  for (i = 0; i < num_of_clans; i++)
    if (clan_control[i].num == num)
      return i;

  return -1;
}

/* Save the castles information */
void save_castles(void)
{
  FILE *fl;

  if (!(fl = fopen(CASTLE_FILE, "wb"))) {
    perror("SYSERR: Unable to open castle file");
    return;
  }

  fwrite(castles, sizeof(int), num_of_castles, fl);

  fclose(fl);
}

/* Save the clan control information */
void clan_save_control(void)
{
  FILE *fl;

  if (!(fl = fopen(CCONTROL_FILE, "wb"))) {
    perror("SYSERR: Unable to open clan control file");
    return;
  }
  /* write all the clan control recs in one fell swoop.  Pretty nifty, eh? */
  fwrite(clan_control, sizeof(struct clan_control_rec), num_of_clans, fl);

  fclose(fl);
}

/* call from boot_db - will load records of the clans etc. */

void clan_boot(void)
{
  struct clan_control_rec temp_clan;
  sh_int real_clandon;
  FILE *fl;
  int temp, end, start, curr, j, k;

  memset((char *)clan_control,0,sizeof(struct clan_control_rec)*MAX_CLANS);

  if (!(fl = fopen(CCONTROL_FILE, "rb"))) {
    log("Clan control file does not exist.");
  } else {
    
    while (!feof(fl) && num_of_clans < MAX_CLANS) {
      
      fread(&temp_clan, sizeof(struct clan_control_rec), 1, fl);
      
      if (feof(fl))
	break;

      /* check if members still exist */
      for (j = 0; j < temp_clan.num_of_members; j++) {
	if(get_name_by_id(temp_clan.members[j]) == NULL) {
	  /* delete them */
	  log("    Deleting undefined members in clan %d.", num_of_clans+1);
	  for (k = j; k < temp_clan.num_of_members; k++)
            temp_clan.members[k] = temp_clan.members[k + 1];
          temp_clan.num_of_members--;
	  j = 0; /* if the one moved is still null */
	}
      }

      if (get_name_by_id(temp_clan.leader) == NULL) {
	log ("    Leader of clan %d does not exist!", num_of_clans+1);
	if (temp_clan.num_of_members > 0) {
	  log ("        Making first member the leader.");
	  temp_clan.leader = temp_clan.members[0];
	  for (j = 0; j < temp_clan.num_of_members; j++)
            temp_clan.members[j] = temp_clan.members[j + 1];
          temp_clan.num_of_members--;
	} else {
	  log ("        Clan has no members... Skipping.");
	  continue;
	}
      }
      
      clan_control[num_of_clans++] = temp_clan;
      real_clandon = real_room(temp_clan.room);
      
      SET_BIT(ROOM_FLAGS(real_clandon), ROOM_CLAN);
    }

    fclose(fl);
    clan_save_control();
  }

  /* now load the castle information */

  log ("Booting Castles...");

  if (!(fl = fopen(CASTLE_FILE, "rb"))) {
    log("       Castle file does not exist.");
    return;
  }

  while (!feof(fl) && num_of_castles < MAX_CASTLES) {

    fread(&temp, sizeof(int), 1, fl);
    if (feof(fl))
      break;

    castles[num_of_castles++] = temp;

    /* now we set the castle flag across the zone */

    start = temp * 100;
    end = start + 99;

    for (curr = 0; curr < top_of_world; curr++) {
	if (ROOM_FLAGGED(curr, ROOM_CASTLE))
		REMOVE_BIT(ROOM_FLAGS(curr), ROOM_CASTLE);
    }

    for (curr = 0; (curr <= top_of_world) && (world[curr].number <= end); curr++) 
      if (world[curr].number >= start ) 
	SET_BIT(ROOM_FLAGS(curr), ROOM_CASTLE); 
  }

  fclose(fl);

}

/* "clan Control" functions */

char *CCONTROL_FORMAT =
"Usage: cbuild make <clan room> <leader name> <clan name>\r\n"
"       cbuild destroy <clan number>\r\n"
"       cbuild reset <clan number>\r\n"
"       cbuild leader <clan number> <leader name>\r\n"
"       cbuild show\r\n"
"       cbuild listmembers <clan number>\r\n"
"       cbuild delmem <clan number> <member name>\r\n"
"       clbuild castle <clan number> <castle zone number>\r\n";

#define NAME(x) ((temp = get_name_by_id(x)) == NULL ? "<UNDEF>" : temp)


/* List the current clans */

void ccontrol_list_clans(struct char_data * ch)
{
  int i;
  char *temp;
  char leader_name[128], clan_name[128];

  if (!num_of_clans) {
    send_to_char("No Clans have been defined.\r\n", ch);
    return;
  }

  strcpy(buf, "Number   Name                Leader       Members  Room     Points  Castle\r\n");
  strcat(buf, "-------  ------------------  -----------  -------  -------  ------  ------\r\n");

  for (i = 0; i < num_of_clans; i++) {
    
    strcpy(leader_name, NAME(clan_control[i].leader));
    strcpy(clan_name, clan_control[i].clanname);

    sprintf(buf, "%s %-7d %-19s %-14s %-6d %-8d %-6d", buf,
	    clan_control[i].num, CAP(clan_name), CAP(leader_name),
	    clan_control[i].num_of_members, clan_control[i].room, clan_control[i].clan_points);

    if ( clan_control[i].castlezone < 1 ) 
      sprintf (buf, "%s None\r\n",buf);
    else
      sprintf (buf, "%s %d\r\n",buf, clan_control[i].castlezone);

  }
  send_to_char(buf, ch);
}


/* list the members of a clan */
void ccontrol_list_members( struct char_data *ch, char *arg ) {

  int i, j;
  char *temp;
  
  skip_spaces (&arg);
  
  if (!*arg) {
    send_to_char ("You must supply a clan number.\r\n", ch);
    return;
  }
  
  if ((i = find_clan_num(atoi(arg))) < 0) {
    send_to_char ("Clan does not exist!\r\n", ch);
    return;
  }

  if (clan_control[i].num_of_members > 0) {
    sprintf(buf, "\r\nClan Name: %s.\r\n---------------------------------------\r\nClan Leader: %s.\r\nMembers:\r\n\r\n", 
	    CAP(clan_control[i].clanname), CAP(NAME(clan_control[i].leader)));
    for (j = 0; j < clan_control[i].num_of_members; j++) {
      sprintf(buf2, "%s.\r\n", NAME(clan_control[i].members[j]));
      strcat(buf, CAP(buf2));
    }
    strcat(buf, "\r\n");
  }
  else {
    sprintf(buf, "Clan: %s.\r\nLeader: %s.\r\nMembers:\r\n\r\n", CAP(clan_control[i].clanname), CAP(NAME(clan_control[i].leader)));
    sprintf(buf, "%s   No members currently in this Clan.\r\n\r\n", buf);
  }

  send_to_char(buf, ch);
}


/* Build A New Clan */

void ccontrol_build_clan(struct char_data * ch, char *arg) {

  char arg1[MAX_INPUT_LENGTH];
  struct clan_control_rec temp_clan;
  sh_int virt_clandon, real_clandon;
  long leaderid;
  struct char_data *leader = NULL;

  if (num_of_clans >= MAX_CLANS) {
    send_to_char("Max clans already defined.\r\n", ch);
    return;
  }

  /* first arg: clans donation room vnum */

  arg = one_argument(arg, arg1);

  if (!*arg1) {
    send_to_char(CCONTROL_FORMAT, ch);
    return;
  }
  virt_clandon = atoi(arg1);
  if ((real_clandon = real_room(virt_clandon)) < 0) {
    send_to_char("No such room exists.\r\n", ch);
    return;
  }
  if ((find_clan(virt_clandon)) >= 0) {
    send_to_char("Clan room already exists for another clan.\r\n", ch);
    return;
  }

  /* second arg: name of the clan leader */

  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(CCONTROL_FORMAT, ch);
    return;
  }

  if (!(leader = get_char_vis(ch,arg1))) {
    send_to_char("Player must be present to do this.\r\n", ch);
    return;
  }
  
  leaderid = get_id_by_name(arg1);

  if (GET_LEVEL(leader) >= LVL_IMMORT) {
    send_to_char("Sorry, Immortals Can not be Clan Leaders.\r\n", ch);
    return;
  }
 
  if (GET_GOLD(leader) < CLAN_COST) {
    send_to_char("Sorry, but the leader does not have enough gold to form a clan.\r\n", ch);
    return;
  } 

  if (GET_LEVEL(leader) < CLAN_LEVEL) {
    send_to_char("Sorry, but the leader must be level 20 and above.\r\n", ch);
    return;
  }  

  if ( is_in_clan(leader) >= 0 ) {
    send_to_char("They are already in a clan!\r\n", ch);
    return;
  }

  /* third arg: clan name */

  if (!*arg1) {
    send_to_char(CCONTROL_FORMAT, ch);
    return;
  } 
  else { 
    if ( strlen(arg1) > 32 ) {
      send_to_char("Clan name must be 32 characters or less.\r\n", ch);
      return;
    }
    skip_spaces(&arg);
    strcpy (temp_clan.clanname, arg);
  }

  temp_clan.room = virt_clandon;
  temp_clan.leader = leaderid;
  temp_clan.num_of_members = 0;
  temp_clan.num = num_of_clans + 1;
  temp_clan.castlezone = -1;
  temp_clan.clan_points = 0;

  GET_GOLD(leader) -= CLAN_COST;

  clan_control[num_of_clans++] = temp_clan;

  SET_BIT(ROOM_FLAGS(real_clandon), ROOM_CLAN);

  mudlogf(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s has just built clan #%d", GET_NAME(ch), temp_clan.num);

  send_to_char("Clan Built!\r\n", ch);
  send_to_char("You are now a clan leader!", leader);

  clan_save_control();
}

void ccontrol_change_castle(struct char_data *ch, char *arg )
{
  int i;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  two_arguments(arg, arg1, arg2);

  if (!*arg2) {
    send_to_char(CCONTROL_FORMAT, ch);
    return;
  }

  if ((i = find_clan_num(atoi(arg1))) < 0) {
    send_to_char("Unknown clan.\r\n", ch);
    return;
  }

  clan_control[i].castlezone = atoi(arg2);

  mudlogf(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	 "(GC) %s has just changed the clan castle of clan #%d to castle %d", 
	 GET_NAME(ch), i+1, clan_control[i].castlezone );


  send_to_char("Clan Castle changed.\r\n", ch);
  clan_save_control();

}

/* Reset the Clan Points to 0 */

void ccontrol_reset(struct char_data *ch, char *arg )
{
  int i;

  if (!*arg) {
    send_to_char(CCONTROL_FORMAT, ch);
    return;
  }
  
  if ((i = find_clan_num(atoi(arg))) < 0) {
    send_to_char("Unknown clan.\r\n", ch);
    return;
  }

  clan_control[i].clan_points = 0;
  mudlogf(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s has just reset  clan #%d", GET_NAME(ch), i+1);

  send_to_char("Clan Reset.\r\n", ch);
  clan_save_control();
  
}

/* Change the leader of a clan */

void ccontrol_change_leader(struct char_data *ch, char *arg) {

  int i, leaderid;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct char_data *leader = NULL;
  
  two_arguments(arg, arg1, arg2);
  
  if (!*arg2) {
    send_to_char(CCONTROL_FORMAT, ch);
    return;
  }

  if ((i = find_clan_num(atoi(arg1))) < 0) {
    send_to_char("Unknown clan.\r\n", ch);
    return;
  }
  
  if (!(leader = get_char_vis(ch, arg2))) {
    send_to_char("Player must be present to do this.\r\n", ch);
    return;
  }
  
  leaderid = get_id_by_name(arg2);

  if (GET_LEVEL(leader) >= LVL_IMMORT) {
    send_to_char("Sorry, Immortals Can not be Clan Leaders.\r\n", ch);
    return;
  }
 
  if (GET_LEVEL(leader) < CLAN_LEVEL) {
    send_to_char("Sorry, but the leader must be level 20 and above.\r\n", ch);
    return;
  }  

  if ( is_in_clan(leader) >= 0 ) {
    send_to_char("They are already in a clan!\r\n", ch);
    return;
  }
  clan_control[i].leader = leaderid;

  mudlogf(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s has just changed the leader of clan #%d to %s", 
	  GET_NAME(ch), i+1, GET_NAME(leader));

  send_to_char("Clan Leader changed.\r\n", ch);
  clan_save_control();
  
}

void ccontrol_delmem(struct char_data *ch, char *arg) {

  int j, i, memberid;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct char_data *member = NULL;
  
  two_arguments(arg, arg1, arg2);
  
  if (!*arg2) {
    send_to_char(CCONTROL_FORMAT, ch);
    return;
  }

  if ((i = find_clan_num(atoi(arg1))) < 0) {
    send_to_char("Unknown clan.\r\n", ch);
    return;
  }
  
  if (!(member = get_char_vis(ch, arg2))) {
    send_to_char("Player must be present to do this.\r\n", ch);
    return;
  }
  
  memberid = get_id_by_name(arg2);

  for (j = 0; j < clan_control[i].num_of_members; j++)
    if (clan_control[i].members[j] == memberid) {
      for (; j < clan_control[i].num_of_members; j++)
	clan_control[i].members[j] = clan_control[i].members[j + 1];
      clan_control[i].num_of_members--;

      send_to_char("Member deleted from clan.\r\n", ch);
      send_to_char("The gods have removed you from your clan.\r\n", member);
      clan_save_control();
      return;
    }

  send_to_char("No such member.\r\n", ch);
}
  
/* Destroy A Clan */

void ccontrol_destroy_clan(struct char_data * ch, char *arg)
{
  int i, j;
  sh_int real_clandon;

  if (!*arg) {
    send_to_char(CCONTROL_FORMAT, ch);
    return;
  }
  if ((i = find_clan_num(atoi(arg))) < 0) {
    send_to_char("Unknown clan.\r\n", ch);
    return;
  }

  if ((real_clandon = real_room(clan_control[i].room)) < 0)
    log("SYSERR: Clan Donation Room has invalid vnum!");
  else
    REMOVE_BIT(ROOM_FLAGS(real_clandon), ROOM_CLAN);

  for (j = i; j < num_of_clans - 1; j++) {
    clan_control[j] = clan_control[j + 1];
    clan_control[j].num--;
  }
  num_of_clans--;

  mudlogf(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	  "(GC) %s has just destroyed clan #%d", GET_NAME(ch), i+1);

  send_to_char("Clan deleted.\r\n", ch);
  clan_save_control();
}


/* The clanbuild command itself, used by imms to create/destroy clans */
ACMD(do_clanbuild)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg1, arg2);

  if (is_abbrev(arg1, "make"))
    ccontrol_build_clan(ch, arg2);
  else if (is_abbrev(arg1, "destroy"))
    ccontrol_destroy_clan(ch, arg2);
  else if (is_abbrev(arg1, "show"))
    ccontrol_list_clans(ch);
  else if (is_abbrev(arg1, "listmembers"))
    ccontrol_list_members(ch, arg2);
  else if (is_abbrev(arg1, "castle"))
    ccontrol_change_castle(ch, arg2);  
  else if (is_abbrev(arg1, "leader"))
    ccontrol_change_leader(ch, arg2);
  else if (is_abbrev(arg1, "reset"))
    ccontrol_reset(ch, arg2);
  else if(is_abbrev(arg1, "delmem"))
    ccontrol_delmem(ch, arg2);
  else
    send_to_char(CCONTROL_FORMAT, ch);
}


/* The clanmember command, used by mortal clan leaders to assign memebers */
ACMD(do_clanmember)
{
  int i = 0, j, id;
  char *temp = NULL;
  struct char_data *victim = NULL;

  one_argument(argument, arg);

  if ((i = is_in_clan(ch)) < 0 ) {
    send_to_char("Only Clan Leaders clan do this.\r\n",ch);
    return;
  }

  if (GET_IDNUM(ch) != clan_control[i].leader) {
    send_to_char("Only clan leaders can do this.\r\n", ch);
    return;
  }
  
  if (!*arg) {
    sprintf(buf, "Clan Information for %s.\r\n\r\nClan Points: %d\r\nCastle    : ", 
	    CAP(clan_control[i].clanname), clan_control[i].clan_points);
    if ( clan_control[i].castlezone < 1 ) 
      strcat(buf, "None\r\n\r\n");
    else
      sprintf (buf, "%s%d\r\n\r\n",buf, clan_control[i].castlezone);
    send_to_char(buf, ch);
    
    send_to_char("Members of your clan:\r\n", ch);
    if (clan_control[i].num_of_members == 0) {
      send_to_char("  None.\r\n", ch);
    } else {
      for (j = 0; j < clan_control[i].num_of_members; j++) {
	strcpy(buf, NAME(clan_control[i].members[j]));
	send_to_char(strcat(CAP(buf), "\r\n"), ch);
      }
    }
  } else { 
    if ((victim = get_char_vis(ch, arg)) == NULL) {
      send_to_char("They Must be here to do that.\r\n", ch);
      return;
    } else {
      id = get_id_by_name(arg);
      for (j = 0; j < clan_control[i].num_of_members; j++)
	if (clan_control[i].members[j] == id) {
	  for (; j < clan_control[i].num_of_members; j++)
	    clan_control[i].members[j] = clan_control[i].members[j + 1];
	  clan_control[i].num_of_members--;
	  clan_save_control();
	  send_to_char("Memeber deleted.\r\n", ch);
	  return;
	}
      if (is_in_clan(victim) < 0) {
	if (GET_LEVEL(victim) < LVL_IMMORT) {
	  j = clan_control[i].num_of_members++;
	  clan_control[i].members[j] = id;
	  clan_save_control();
	  send_to_char("Member added.\r\n", ch);
	} else {
	  send_to_char("Immortals can't be in clans!!!\r\n", ch);
	}
      } else {
	send_to_char("They are in a clan already!\r\n",ch);
      }
    }
  }
}

/* Misc. administrative functions */

/* Build A New Clan */

ACMD(do_castle) {

  char arg1[MAX_INPUT_LENGTH];
  int i, num, j;

  if (num_of_castles >= MAX_CASTLES) {
    send_to_char("Max castles are already defined.\r\n", ch);
    return;
  }

  one_argument(argument, arg1);

  if (!*arg1) {
    
    send_to_char("Usage: castle <Zone number>\r\n\r\nWill add or delete the castle in the Zone Number.\r\n\r\n", ch);

    sprintf(buf, "Current Castles are: ");

    for (i = 0; i < num_of_castles; i++)
      sprintf (buf, "%s%d, ", buf, castles[i]);
   
    sprintf(buf, "%s\r\n", buf);
    send_to_char(buf,ch);
    return;
  }

  num = atoi(arg1);

  /* check castle does not already exist */

  for (i = 0; i < num_of_castles; i++) {
    if (castles[i] == num ) {
      mudlogf(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	     "(GC) %s has just deleted castle #%d", GET_NAME(ch), castles[i]);
      
      /* Remove the castle flags */
      for (j = 0; (j <= top_of_world) && (world[j].number <= ((castles[i]*100)+99)); i++) 
	if (world[j].number >= (castles[i]*100)) 
	  REMOVE_BIT(ROOM_FLAGS(j), ROOM_CASTLE);

      castles[i] = castles[num_of_castles-1];
      num_of_castles--;
      
      send_to_char("Castle deleted.\r\n", ch);
      save_castles();  
      return;
    }
  }
    
  /* ok it didn't exist so we add it */
  castles[num_of_castles++] = num;

  mudlogf(CMP, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE,
	 "(GC) %s has just added castle #%d", GET_NAME(ch), castles[num_of_castles-1]);

  for (i = 0; (i <= top_of_world) && (world[i].number <= ((castles[num_of_castles-1]*100)+99)); i++) {
    if (world[i].number >= (castles[num_of_castles-1]*100) ) {
      SET_BIT(ROOM_FLAGS(i), ROOM_CASTLE);
    }
  }
  send_to_char("Castle added.\r\n", ch);
  save_castles();
}


int clan_can_enter_castle (struct char_data * ch, int room)
{
  int zone, clan;
  struct descriptor_data *pt;

  clan = is_in_clan(ch);

  if (IS_NPC(ch))
    return 1;

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    return 1;
  }

  if ( clan < 0 ) 
    return 0;

  zone = (int)room / 100;
  if ( zone == clan_control[clan].castlezone )
    return 1;
  else {
    if (ROOM_FLAGGED(ch->in_room, ROOM_CASTLE)) {
      return 1;
    } else {
      for (pt = descriptor_list; pt; pt = pt->next) {
	if (!pt->connected && pt->character)
	  if (!TMP_FLAGGED(pt->character, TMP_WRITING | TMP_MAILING | TMP_OLC)) {
	    sprintf(buf, "%s[CLAN] %s of the clan '%s' has attacked castle %d.%s\r\n",
		    CCYEL(pt->character, C_CMP), GET_NAME(ch), 
		    CAP(clan_control[clan].clanname), 
		    zone, CCNRM(pt->character, C_CMP));
	    send_to_char(buf, pt->character);
	  }
      }
      return 1;
    }
  }
  return 0;
}

/* note: arg passed must be clan room vnum, so there. */
int clan_can_enter(struct char_data * ch, sh_int room)
{
  int i, j;

  i = find_clan(world[room].number);

  if ( i < 0 ) 
    return 1;

  if (GET_LEVEL(ch) >= LVL_SUPREME) {
    return 1;
  }

  if (GET_IDNUM(ch) == clan_control[i].leader) {
    return 1;
  }

  for (j = 0; j < clan_control[i].num_of_members; j++)
    if (GET_IDNUM(ch) == clan_control[i].members[j]) {
      return 1;
    }

  return 0;
}

int clan_can_kill (struct char_data * ch, struct char_data * vict) {

  int ch_clan, vict_clan;
  
  if (IS_NPC(ch) || IS_NPC(vict))
    return 1;

  ch_clan = is_in_clan(ch);
  vict_clan = is_in_clan(vict);

  /* Check if both players are in a clan */
  if ((ch_clan < 0) || (vict_clan < 0) )
    return 0;

  /* if they are in the same clan don't let them attack each other */
  if (ch_clan == vict_clan) 
    return 0;

  /* ok let them kick the crap out of each other */

  return 1;
}
