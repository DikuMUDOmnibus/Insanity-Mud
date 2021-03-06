/* ************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
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

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern struct command_info cmd_info[];

/* extern functions */
char *fread_action(FILE * fl, int nr);

/* local globals */
static int list_top = -1;

struct social_messg {
  int act_nr;
  int hide;
  int min_victim_position;	/* Position of victim */

  /* No argument was supplied */
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;		/* if NULL, read no further, ignore args */
  char *others_found;
  char *vict_found;

  /* An argument was there, but no victim was found */
  char *not_found;

  /* The victim turned out to be the character */
  char *char_auto;
  char *others_auto;
}           *soc_mess_list = NULL;


int find_action(int cmd)
{
  int bot, top, mid;

  bot = 0;
  top = list_top;

  if (top < 0)
    return (-1);

  for (;;) {
    mid = (bot + top) >> 1;

    if (soc_mess_list[mid].act_nr == cmd)
      return (mid);
    if (bot >= top)
      return (-1);

    if (soc_mess_list[mid].act_nr > cmd)
      top = --mid;
    else
      bot = ++mid;
  }
}



ACMD(do_action)
{
  int act_nr;
  struct social_messg *action;
  struct char_data *vict;

  if ((act_nr = find_action(cmd)) < 0) {
    send_to_char("That action is not supported.\r\n", ch);
    return;
  }
  action = &soc_mess_list[act_nr];

  if (action->char_found)
    one_argument(argument, buf);
  else
    *buf = '\0';

  if (!*buf) {
    send_to_char(action->char_no_arg, ch);
    send_to_char("\r\n", ch);
    act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);
    return;
  }
  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char(action->not_found, ch);
    send_to_char("\r\n", ch);
  } else if (vict == ch) {
    send_to_char(action->char_auto, ch);
    send_to_char("\r\n", ch);
    act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
  } else {
    if (GET_POS(vict) < action->min_victim_position)
      act("$N is not in a proper position for that.",
	  FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    else {
      act(action->char_found, 0, ch, 0, vict, TO_CHAR | TO_SLEEP);
      act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
      act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
    }
  }
}



ACMD(do_insult)
{
  struct char_data *victim;

  one_argument(argument, arg);

  if (*arg) {
    if (!(victim = get_char_room_vis(ch, arg)))
      send_to_char("Can't hear you!\r\n", ch);
    else {
      if (victim != ch) {
	sprintf(buf, "You insult %s.\r\n", GET_NAME(victim));
	send_to_char(buf, ch);

	switch (number(0, 2)) {
	case 0:
	  if (GET_SEX(ch) == SEX_MALE) {
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, TO_VICT);
	    else
	      act("$n says that women can't fight.", FALSE, ch, 0, victim, TO_VICT);
	  } else {		/* Ch == Woman */
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of having the smallest... (brain?)",
		  FALSE, ch, 0, victim, TO_VICT);
	    else
	      act("$n tells you that you'd lose a beauty contest against a troll.",
		  FALSE, ch, 0, victim, TO_VICT);
	  }
	  break;
	case 1:
	  act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT);
	  break;
	default:
	  act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT);
	  break;
	}			/* end switch */

	act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT);
      } else {			/* ch == victim */
	send_to_char("You feel insulted.\r\n", ch);
      }
    }
  } else
    send_to_char("I'm sure you don't want to insult *everybody*...\r\n", ch);
}


char *fread_action(FILE * fl, int nr)
{
  char buf[MAX_STRING_LENGTH], *rslt;

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl)) {
    fprintf(stderr, "SYSERR: fread_action - unexpected EOF near action #%d", nr);
    exit(1);
  }
  if (*buf == '#')
    return (NULL);
  else {
    *(buf + strlen(buf) - 1) = '\0';
    CREATE(rslt, char, strlen(buf) + 1);
    strcpy(rslt, buf);
    return (rslt);
  }
}


void boot_social_messages(void)
{
  FILE *fl;
  int nr, i, hide, min_pos, curr_soc = -1;
  char next_soc[100];
  struct social_messg temp;

  /* open social file */
  if (!(fl = fopen(SOCMESS_FILE, "r"))) {
    sprintf(buf, "SYSERR: Can't open socials file '%s'", SOCMESS_FILE);
    perror(buf);
    exit(1);
  }
  /* count socials & allocate space */
  for (nr = 0; *cmd_info[nr].command != '\n'; nr++)
    if (cmd_info[nr].command_pointer == do_action)
      list_top++;

  CREATE(soc_mess_list, struct social_messg, list_top + 1);

  /* now read 'em */
  for (;;) {
    fscanf(fl, " %s ", next_soc);
    if (*next_soc == '$')
      break;
    if ((nr = find_command(next_soc)) < 0)
      log("Unknown social '%s' in social file", next_soc);
    if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2) {
      fprintf(stderr, "SYSERR: Format error in social file near social '%s'\n",
	      next_soc);
      exit(1);
    }
    /* read the stuff */
    curr_soc++;
    soc_mess_list[curr_soc].act_nr = nr;
    soc_mess_list[curr_soc].hide = hide;
    soc_mess_list[curr_soc].min_victim_position = min_pos;

    soc_mess_list[curr_soc].char_no_arg = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_no_arg = fread_action(fl, nr);
    soc_mess_list[curr_soc].char_found = fread_action(fl, nr);

    /* if no char_found, the rest is to be ignored */
    if (!soc_mess_list[curr_soc].char_found)
      continue;

    soc_mess_list[curr_soc].others_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].vict_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].not_found = fread_action(fl, nr);
    soc_mess_list[curr_soc].char_auto = fread_action(fl, nr);
    soc_mess_list[curr_soc].others_auto = fread_action(fl, nr);
  }

  /* close file & set top */
  fclose(fl);
  list_top = curr_soc;

  /* now, sort 'em */
  for (curr_soc = 0; curr_soc < list_top; curr_soc++) {
    min_pos = curr_soc;
    for (i = curr_soc + 1; i <= list_top; i++)
      if (soc_mess_list[i].act_nr < soc_mess_list[min_pos].act_nr)
	min_pos = i;
    if (curr_soc != min_pos) {
      temp = soc_mess_list[curr_soc];
      soc_mess_list[curr_soc] = soc_mess_list[min_pos];
      soc_mess_list[min_pos] = temp;
    }
  }
}


ACMD(do_gact)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);
  
  if (PRF_FLAGGED(ch, PRF_NOGOSS)) {
    send_to_char("You're not even on that channel.\r\n", ch);
  } else {
    sprintf(buf, "[ACTION] $n %s", argument);
    act(buf, FALSE, ch, 0, 0, TO_GACT);
  }
  return;
}

ACMD(do_gmote)
{
  int i, act_nr, length, is_disabled = 0;
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct social_messg *action;
  struct char_data *vict = NULL;

  half_chop(argument, buf, arg);

  if(subcmd)
    for (length = strlen(buf), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(cmd_info[cmd].command, buf, length))
        break;

   /* if is disabled */
  for (i = 0; i < 10; i++) {
    if (!strcmp(((ch)->player_specials->saved.disabled[i]), cmd_info[cmd].command))
      is_disabled = 1;
  }
  
  if ((act_nr = find_action(cmd)) < 0) {
    send_to_char("That's not a social!\r\n", ch);
    return;
  }
  else if (is_disabled) {
    sprintf(buf, "You don't seem to remember how to %s.\r\n", cmd_info[cmd].command); 
    send_to_char(buf, ch);
    return;
  }
  
  action = &soc_mess_list[act_nr];

  if (!action->char_found)
    *arg = '\0';

  if (!*arg) {
    if(!action->others_no_arg || !*action->others_no_arg) {
      send_to_char("Who are you going to do that to?\r\n", ch);
      return;
    }
    sprintf(buf, "[GOSSIP] %s", action->others_no_arg);
  } else if (!(vict = get_char_vis(ch, arg))) {
    send_to_char(action->not_found, ch);
    send_to_char("\r\n", ch);
    return;
  } else if (vict == ch) {
    if(!action->others_auto || !*action->others_auto) {
      send_to_char(action->char_auto, ch);
      send_to_char("\r\n", ch);
      return;
    }
    sprintf(buf, "[GOSSIP] %s", action->others_auto);
  } else {
    if (GET_POS(vict) < action->min_victim_position) {
      act("$N is not in a proper position for that.",
          FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
      return;
    } else {
      sprintf(buf, "[GOSSIP] %s", action->others_found);
    }
  }
  act(buf, FALSE, ch, 0, vict, TO_GMOTE);
}

/* added by nahaz for wmote */
ACMD(do_wmote)
{
  int i, act_nr, length, is_disabled = 0;
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct social_messg *action;
  struct char_data *vict = NULL;

  half_chop(argument, buf, arg);

  if(subcmd)
    for (length = strlen(buf), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
      if (!strncmp(cmd_info[cmd].command, buf, length))
        break;

  /* if is disabled */
  for (i = 0; i < 10; i++) {
    if (!strcmp(((ch)->player_specials->saved.disabled[i]), cmd_info[cmd].command))
      is_disabled = 1;
  }
  
  if ((act_nr = find_action(cmd)) < 0) {
    send_to_char("That's not a social!\r\n", ch);
    return;
  }
  else if (is_disabled) {
    sprintf(buf, "You don't seem to remember how to %s.\r\n", cmd_info[cmd].command); 
    send_to_char(buf, ch);
    return;
  }

  action = &soc_mess_list[act_nr];

  if (!action->char_found)
    *arg = '\0';

  if (!*arg) {
    if(!action->others_no_arg || !*action->others_no_arg) {
      send_to_char("Who are you going to do that to?\r\n", ch);
      return;
    }
    sprintf(buf, "[WIZNET] %s", action->others_no_arg);
  } else if (!(vict = get_char_vis(ch, arg))) {
    send_to_char(action->not_found, ch);
    send_to_char("\r\n", ch);
    return;
  } else if (vict == ch) {
    if(!action->others_auto || !*action->others_auto) {
      send_to_char(action->char_auto, ch);
      send_to_char("\r\n", ch);
      return;
    }
    sprintf(buf, "[WIZNET] %s", action->others_auto);
  } else {
    if (GET_POS(vict) < action->min_victim_position)
      act("$N is not in a proper position for that.",
          FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    else {
      sprintf(buf, "[WIZNET] %s", action->others_found);
    }
  }
  act(buf, FALSE, ch, 0, vict, TO_WMOTE);
}
