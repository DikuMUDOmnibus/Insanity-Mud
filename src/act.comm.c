/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
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
#include "screen.h"
#include "spells.h"

/* extern variables */
extern int level_can_shout;
extern int holler_move_cost;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern char *languages[];
ACMD(do_gmote);

void send_to_private (struct char_data *ch, const char *mesg, int issay, int echoback)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i->character &&
	!TMP_FLAGGED(i->character, TMP_WRITING | TMP_OLC | TMP_MAILING) &&
	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      if (!echoback && ch->desc == i)
	continue;

      if (GET_PRIVATE(i->character) == GET_PRIVATE(ch)) {
	if (!issay)
	  if (GET_INVIS_LEV(ch) > GET_LEVEL(i->character))
	    sprintf (buf, "[Private] Someone %s.\r\n", mesg);
	  else
	    sprintf(buf, "[Private] %s %s.\r\n", GET_NAME(ch), mesg);
	else
	  if (GET_INVIS_LEV(ch) > GET_LEVEL(i->character))
	    sprintf (buf, "[Private] Someone :: %s.\r\n", mesg);
	  else
	    sprintf(buf, "[Private] %s :: %s.\r\n", GET_NAME(ch), mesg);
	send_to_char(buf, i->character);
      }
    }
  }
}


ACMD(do_private_channel)
{
  struct char_data *vict = NULL;
  struct descriptor_data *i = NULL;

  if (subcmd == PRIVATE_SAY) {
    skip_spaces(&argument);

    if (!*argument) {
      send_to_char("Yes but what do you wish to say?\r\n", ch);
      return;
    }

    if (GET_PRIVATE(ch) == 0) {
      send_to_char("You are not on a private channel!\r\n", ch);
      return;
    }

    send_to_private(ch, CAP(argument), TRUE, TRUE);

  } else {

    half_chop(argument, buf, buf2);

    switch (subcmd) {

    case PRIVATE_OPEN:

      if(GET_PRIVATE(ch) == GET_IDNUM(ch)) {
	send_to_char("You already have one!\r\n", ch);
	return;
      }

      if (GET_PRIVATE(ch) != 0) {
	send_to_char("Your already on a channel, use pleave to leave it.\r\n", ch);
	return;
      }

      GET_PRIVATE(ch) = GET_IDNUM(ch);
      send_to_char("You have just opened your own Private Channel.\r\n", ch);
      mudlogf (CMP, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
	       "%s has just opened a private channel #%ld.", GET_NAME(ch), GET_IDNUM(ch));
      break;

    case PRIVATE_OFF:

      if (GET_PRIVATE(ch) == GET_IDNUM(ch)) {
	send_to_char("You can't leave your own private channel, close it instead.\r\n", ch);
	return;
      }

      send_to_private(ch, "has just left the channel", FALSE, FALSE);
      GET_PRIVATE(ch) = 0;
      send_to_char("You have just quit the Private Channel.\r\n", ch);
      break;

    case PRIVATE_CLOSE:

      /* lets supreme god close the channel */
      if (*buf && GET_LEVEL(ch) >= LVL_SUPREME) {
	if (!(vict = get_char_vis(ch, buf)))
	  send_to_char(NOPERSON, ch);
	else if (IS_NPC(vict))
	  send_to_char("What? Sod off!\r\n", ch);
	else if (GET_LEVEL(ch) <= GET_LEVEL(vict))
	  send_to_char("You think so do you?!?!\r\n", ch);
	else {
	  mudlogf (CMP, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
		   "%s has just closed %s's private channel.", GET_NAME(ch), GET_NAME(vict));

	  GET_PRIVATE(ch) = GET_IDNUM(vict);
	  send_to_private(ch, "has closed this private channel", FALSE, FALSE);
	  send_to_char("You have just closed the Private Channel.\r\n", ch);
	  GET_PRIVATE(ch) = 0;
	}
      } else {

	if (GET_PRIVATE(ch) == GET_IDNUM(ch)) {

	  mudlogf (CMP, MAX(LVL_GRGOD, GET_INVIS_LEV(ch)), TRUE,
		   "%s has just closed a private channel #%ld.", GET_NAME(ch), GET_IDNUM(ch));

	  if (GET_SEX(ch) == SEX_MALE)
	    send_to_private(ch, "has closed his private channel", FALSE, FALSE);
	  else if (GET_SEX(ch) == SEX_FEMALE)
	    send_to_private(ch, "has closed her private channel", FALSE, FALSE);
	  else if (GET_SEX(ch) == SEX_NEUTRAL)
	    send_to_private(ch, "has closed its private channel", FALSE, FALSE);

	  send_to_char("You have just closed your Private Channel.\r\n", ch);

	  for (i = descriptor_list; i; i = i->next)
	    if (!i->connected)
	      if ((GET_PRIVATE(i->character) == GET_IDNUM(ch)) && (ch != i->character))
		GET_PRIVATE(i->character) = 0;

	  GET_PRIVATE(ch) = 0;

	} else
	  send_to_char("You do not own a channel, to leave use pleave.\r\n", ch);
      }

      break;

    case PRIVATE_WHO:

      if (GET_PRIVATE(ch) == 0)
	send_to_char("You are not on a private channel\r\n",ch);
      else {
	send_to_char("Private Channel Members\r\n", ch);
	send_to_char("-----------------------\r\n", ch);
	for (i = descriptor_list; i; i = i->next)
	  if (!i->connected)
	    if (GET_PRIVATE(i->character) == GET_PRIVATE(ch))
	      if (GET_INVIS_LEV(i->character) <= GET_LEVEL(ch)) {
		sprintf(buf, "%s\r\n", GET_NAME(i->character));
		send_to_char(buf, ch);
	      }
      }
      break;

    case PRIVATE_CHECK:

      if (!*buf) {

	send_to_char("Private Channels Currently on Insanity.\r\n", ch);
	send_to_char("------------------------------------\r\n", ch);
	for (i = descriptor_list; i; i = i->next)
	  if (!i->connected && GET_PRIVATE(i->character))
	    if (GET_LEVEL(ch) >= GET_INVIS_LEV(i->character))
	      if (GET_PRIVATE(i->character) == GET_IDNUM(i->character)) {
		sprintf(buf, "[%-5d]  %s\r\n", GET_PRIVATE(i->character), GET_NAME(i->character));
		send_to_char(buf, ch);
	      }
      } else {

	if (!(vict = get_char_vis(ch, buf)))
	  send_to_char(NOPERSON, ch);
	else if (IS_NPC(vict))
	  send_to_char("What? Sod off!\r\n", ch);
	else if (GET_PRIVATE(vict) != GET_IDNUM(vict))
	  send_to_char("They are not the owner of a private channel.\r\n", ch);
	else {
	  sprintf(buf, "Visible Memebers of %s's Private Channel:\r\n\r\n", GET_NAME(vict));
	  send_to_char(buf, ch);
	  for (i = descriptor_list; i; i = i->next)
	    if (!i->connected)
	      if (GET_PRIVATE(i->character) == GET_PRIVATE(vict))
		if (GET_INVIS_LEV(i->character) <= GET_LEVEL(ch)) {
		  sprintf(buf, "%s\r\n", GET_NAME(i->character));
		  send_to_char(buf, ch);
		}
	}
      }
      break;

    case PRIVATE_REMOVE:

      if (!*buf)
	send_to_char("Who do you wish to remove from your private channel?\r\n", ch);
      else if (!(vict = get_char_vis(ch, buf)))
	send_to_char(NOPERSON, ch);
      else if (IS_NPC(vict))
	send_to_char("NPC's cannot be on private channels\r\n", ch);
      else if (GET_PRIVATE(ch) != GET_IDNUM(ch))
	send_to_char("This is not your channel!\r\n", ch);
      else if (GET_PRIVATE(ch) == GET_IDNUM(vict))
	send_to_char("You can't remove yourself!\r\n", ch);
      else if (GET_PRIVATE(vict) != GET_IDNUM(ch)) {
	sprintf(buf,"%s is not on your Private Channel!\r\n", GET_NAME(vict));send_to_char(buf, ch);
      } else {

	sprintf(buf,"You have been removed from %s's Private Channel!\r\n",
		(GET_INVIS_LEV(ch) > GET_LEVEL(vict) ? "Someone" : GET_NAME(ch)));
	send_to_char("You have removed them from your Private Channel!\r\n", ch);

	send_to_char(buf, vict);
	send_to_private(vict, "has been removed from the channel", FALSE, FALSE);
	GET_PRIVATE(vict) = 0;
      }
      break;

    case PRIVATE_ADD:

      if (GET_PRIVATE(ch) != GET_IDNUM(ch))
	send_to_char("You must open your own private channel first.\r\n",ch);
      else if (!*buf)
	send_to_char("Who do you wish to add to you private channel?\r\n", ch);
      else if (!(vict = get_char_vis(ch, buf)))
	send_to_char(NOPERSON, ch);
      else if (ch == vict)
	GET_PRIVATE(ch) = GET_IDNUM(ch);
      else if (IS_NPC(vict))
	send_to_char("NPC's cannot be added to private channels.\r\n", ch);
      else if (GET_PRIVATE(vict) != 0) {
	sprintf(buf,"%s is already on another private channel!\r\n", GET_NAME(vict));
	send_to_char(buf, ch);
      } else {
	GET_PRIVATE(vict) = GET_IDNUM(ch);
	sprintf(buf, "You have been added to %s's Private Channel!\r\n",
		(GET_INVIS_LEV(ch) > GET_LEVEL(vict) ? "Someone" : GET_NAME(ch)));
	send_to_char(buf, vict);
	send_to_private(vict, "has joined the channel", FALSE, FALSE);
      }
      break;

    default:
      mudlogf (BRF, LVL_IMMORT, TRUE, "SYSERR: Reached default case in private channel.");
      break;
    }
  }
}

ACMD(do_msay) {

  struct char_data *vict = NULL;
  char name[MAX_NAME_LENGTH+1];

  skip_spaces(&argument);

  if (GET_MARRIED(ch) == 0) {
    send_to_char("But your not married!\r\n", ch);
    return;
  }

  strcpy(name, get_name_by_id(GET_MARRIED(ch)));

  if (!(vict = get_char_vis(ch, name))) {
    send_to_char("Your spouse is not here!\r\n", ch);
    return;
  }

  if (!vict->desc)	/* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (TMP_FLAGGED(vict, TMP_WRITING | TMP_MAILING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (TMP_FLAGGED(vict, TMP_OLC))
    act("$E's building at the moment, try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "%sYou tell your spouse, '&m%s&c'%s", CCCYN(ch,C_NRM), argument, CCNRM(ch, C_NRM));
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    sprintf(buf, "%sYour spouse, '$n' tells you '&m%s&c'%s", 
	    CCCYN(ch, C_NRM), argument, CCNRM(ch, C_NRM));
    act(buf, FALSE, ch, 0, vict, TO_VICT);
  }
}

void list_lang(struct char_data *ch) 
{
  int i;
  
  send_to_char("You can speak the following languages: \r\n\r\n", ch);
  for (i = LANG_COMMON; i <= MAX_LANG; i++) {
    if (GET_SKILL(ch, i) > 0) {
      sprintf(buf, "  %-20s %d\r\n",languages[i - LANG_COMMON], 
	      GET_SKILL(ch, i));
      send_to_char(buf,ch);
    }
  }
  return;
}


ACMD(do_speak) {

  if (!*argument) {
    list_lang(ch);
    return;
  }

  skip_spaces(&argument);
  
  if (is_abbrev(argument, "common") && GET_SKILL(ch, LANG_COMMON) > 0) 
    SPEAKING(ch) = LANG_COMMON;
  else if (is_abbrev(argument, "humpin") && GET_SKILL(ch, LANG_HUMP) > 0) 
    SPEAKING(ch) = LANG_HUMP;
  else if (is_abbrev(argument, "strokin") && GET_SKILL(ch, LANG_STROKE) > 0) 
    SPEAKING(ch) = LANG_STROKE;
  else if (is_abbrev(argument, "bangin") && GET_SKILL(ch, LANG_BANG) > 0) 
    SPEAKING(ch) = LANG_BANG;
  else if (is_abbrev(argument, "effin") && GET_SKILL(ch, LANG_EFF) > 0) 
    SPEAKING(ch) = LANG_EFF;
  else if (is_abbrev(argument, "fumblin") && GET_SKILL(ch, LANG_FUM) > 0) 
    SPEAKING(ch) = LANG_FUM;
  else if (is_abbrev(argument, "diggin") && GET_SKILL(ch, LANG_DIG) > 0) 
    SPEAKING(ch) = LANG_DIG;
  else if (is_abbrev(argument, "purrin") && GET_SKILL(ch, LANG_PURR) > 0)
    SPEAKING(ch) = LANG_PURR;
  else {
    send_to_char("You don't know how to speak in that tongue.\r\n", ch);
    return;
  }

  sprintf(buf, "You will now speak %s.\r\n", languages[(SPEAKING(ch) - LANG_COMMON)]);
  send_to_char(buf, ch);
  return;
}

#define ofs 190
ACMD(do_say)
{
  struct char_data *tch;
  int percent;

  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
  else {
    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
      if (tch != ch && AWAKE(tch) && tch->desc && !IS_NPC(ch) && !IS_NPC(tch)) {
	percent = number(1, 99);
	if (GET_SKILL(tch, SPEAKING(ch)) < percent)
	  strcpy(buf, "$n says something in an unfamiliar tongue.");
	else
	  sprintf(buf, "$n says in %s, '\\c06%s\\c00'", 
		  languages[(SPEAKING(ch) - ofs)],
		  argument);
	act(buf, TRUE, ch, 0, tch, TO_VICT);
      } else if (IS_NPC(ch) || IS_NPC(tch)) {
	sprintf(buf, "$n says, '\\c06%s\\c00'", argument);
	act(buf, TRUE, ch, 0, tch, TO_VICT);
      }
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      if (!IS_NPC(ch))
	sprintf(buf, "You say in %s, '\\c06%s\\c00'", languages[(SPEAKING(ch)-ofs)],
		argument);
      else
	sprintf(buf, "You say, '\\c06%s\\c00'", argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }
  }
}


ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  skip_spaces(&argument);

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not the member of a group!\r\n", ch);
    return;
  }
  if (!*argument)
    send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
  else {
    if (ch->master)
      k = ch->master;
    else
      k = ch;

    sprintf(buf, "$n tells the group, '%s'", argument);

    if (IS_AFFECTED(k, AFF_GROUP) && (k != ch))
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
    for (f = k->followers; f; f = f->next)
      if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower != ch))
	act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "You tell the group, '%s'", argument);
      act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    }
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  send_to_char(CCRED(vict, C_NRM), vict);
  sprintf(buf, "$n tells you, '%s'", arg);
  act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(vict, C_NRM), vict);

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    send_to_char(CCRED(ch, C_CMP), ch);
    sprintf(buf, "You tell $N, '%s'", arg);
    act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    send_to_char(CCNRM(ch, C_CMP), ch);
  }

  GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict;

  half_chop(argument, buf, buf2);

  /* Added by Nahaz to prevent people being able to order their pets
     to spam tell people things. */
  if (IS_NPC(ch) && (ch->master))
    return;

  if (!*buf || !*buf2)
    send_to_char("Who do you wish to tell what??\r\n", ch);
  else if (!(vict = get_char_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (ch == vict)
    send_to_char("You try to tell yourself something.\r\n", ch);
  else if (PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char("You can't tell other people while you have notell on.\r\n", ch);
  else if (PLR_FLAGGED(vict, PLR_AFK))
    act("$E is AFK.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
    send_to_char("The walls seem to absorb your words.\r\n", ch);
  else if (!IS_NPC(vict) && !vict->desc)	/* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (TMP_FLAGGED(vict, TMP_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, 
	ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (TMP_FLAGGED(vict, TMP_OLC))
    act("$E's building at the moment, try again later.", FALSE, ch, 0, 
	vict, TO_CHAR | TO_SLEEP);
  else if (PRF_FLAGGED(vict, PRF_NOTELL) || 
	   ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else {
    perform_tell(ch, vict, buf2);
  }
}


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char("You have no-one to reply to!\r\n", ch);
  else if (!*argument)
    send_to_char("What is your reply?\r\n", ch);
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     */

    while (tch != NULL && GET_IDNUM(tch) != GET_LAST_TELL(ch))
      tch = tch->next;

    if (tch == NULL)
      send_to_char("They are no longer playing.\r\n", ch);
    else {
      perform_tell(ch, tch, argument);
    }
  }
}


ACMD(do_spec_comm)
{
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  if (subcmd == SCMD_WHISPER) {
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
  } else {
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    sprintf(buf, "Whom do you want to %s.. and what??\r\n", action_sing);
    send_to_char(buf, ch);
  } else if (!(vict = get_char_room_vis(ch, buf)))
    send_to_char(NOPERSON, ch);
  else if (vict == ch)
    send_to_char("You can't get your mouth close enough to your ear...\r\n", ch);
  else {
    sprintf(buf, "$n %s you, '%s'", action_plur, buf2);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      sprintf(buf, "You %s %s, '%s'", action_sing, GET_NAME(vict), buf2);
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
  }
}



#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
  struct obj_data *paper = 0, *pen = 0;
  char *papername, *penname;

  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\r\n", ch);
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", penname);
      send_to_char(buf, ch);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "There is no %s in your inventory.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = 0;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char("That thing has nothing to do with writing.\r\n", ch);
      return;
    }
    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_HOLD)) {
      sprintf(buf, "You can't write with %s %s alone.\r\n", AN(papername),
	      papername);
      send_to_char(buf, ch);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
      send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n", ch);
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_HOLD);
    else
      pen = GET_EQ(ch, WEAR_HOLD);
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else if (paper->action_description)
    send_to_char("There's something written on it already.\r\n", ch);
  else {
    /* we can write - hooray! */
     /* this is the PERFECT code example of how to set up:
      * a) the text editor with a message already loaed
      * b) the abort buffer if the player aborts the message
      */
     ch->desc->backstr = NULL;
     send_to_char("Write your note.  (/s saves /h for help)\r\n", ch);
     /* ok, here we check for a message ALREADY on the paper */
     if (paper->action_description) {
	/* we str_dup the original text to the descriptors->backstr */
	ch->desc->backstr = str_dup(paper->action_description);
	/* send to the player what was on the paper (cause this is already */
	/* loaded into the editor) */
	send_to_char(paper->action_description, ch);
     }
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
     /* assign the descriptor's->str the value of the pointer to the text */
     /* pointer so that we can reallocate as needed (hopefully that made */
     /* sense :>) */
    ch->desc->str = &paper->action_description;
    ch->desc->max_str = MAX_NOTE_LENGTH;
  }
}



ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char("Monsters can't page.. go away.\r\n", ch);
  else if (!*arg)
    send_to_char("Whom do you wish to page?\r\n", ch);
  else {
    sprintf(buf, "\007\007*%s* %s\r\n", GET_NAME(ch), buf2);
    if (!str_cmp(arg, "all")) {
      if (GET_LEVEL(ch) > LVL_GRGOD) {
	for (d = descriptor_list; d; d = d->next)
	  if (!d->connected && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char("You will never be godly enough to do that!\r\n", ch);
      return;
    }
    if ((vict = get_char_vis(ch, arg)) != NULL) {

      if (PRF_FLAGGED(vict, PRF_NOPAGE) && GET_LEVEL(ch) < LVL_IMPL) {
	send_to_char("They have NOPAGE on, you can't page them.\r\n", ch);
	return;
      }

      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(OK, ch);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
      return;
    } else
      send_to_char("There is no such person in the game!\r\n", ch);
  }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  byte gmote = FALSE;

  /* Array of flags which must _not_ be set in order for comm to be heard */
  static const int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    PRF_NOMUSIC,
    PRF_NOCOMMIS,
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */

  static const char *com_msgs[][4] = {
    {"You cannot holler!!\r\n",
       "holler",
       "",
       KYEL},

    {"You cannot shout!!\r\n",
       "shout",
       "Turn off your noshout flag first!\r\n",
       KYEL},

    {"You cannot gossip!!\r\n",
       "gossip",
       "You aren't even on the channel!\r\n",
       KYEL},

    {"You cannot auction!!\r\n",
       "auction",
       "You aren't even on the channel!\r\n",
       KMAG},

    {"You cannot congratulate!\r\n",
       "congrat",
       "You aren't even on the channel!\r\n",
       KGRN},

    {"You cannot use the music channel!\r\n",
       "sing",
       "You aren't even on the channel!\r\n",
       KCYN},

    {"You cannot commiserate!\r\n",
       "commiserate",
       "You aren't even on the channel!\r\n",
       KBLU}
  };

  /* moved to here by Nahaz -- prevents the milky white message */
  if(subcmd == SCMD_GMOTE || (subcmd == SCMD_GOSSIP && *argument == '@')) {
    subcmd = SCMD_GOSSIP;
    gmote = TRUE;
  }

  /* to keep pets, etc from being ordered to shout */
  if (ch->master && AFF_FLAGGED(ch, AFF_CHARM))
    return;

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(com_msgs[subcmd][0], ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < level_can_shout) {
    sprintf(buf1, "You must be at least level %d before you can %s.\r\n",
	    level_can_shout, com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  /* make sure the char is on the channel */
  if (PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(com_msgs[subcmd][2], ch);
    return;
  }
  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument) {
    sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n",
	    com_msgs[subcmd][1], com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }

  if (gmote) {
    if (*argument == '@')
      do_gmote(ch, argument + 1, 0, 1);
    else
      do_gmote(ch, argument, 0, 1);
    return;
  }


  if (subcmd == SCMD_HOLLER) {
    if (GET_MOVE(ch) < holler_move_cost) {
      send_to_char("You're too exhausted to holler.\r\n", ch);
      return;
    } else
      GET_MOVE(ch) -= holler_move_cost;
  }

  /* set up the color on code */
  strcpy(color_on, com_msgs[subcmd][3]);

  /* first, set up strings to be given to the communicator */
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(OK, ch);
  else {
    if (COLOR_LEV(ch) >= C_CMP)
      sprintf(buf1, "%sYou %s, '%s'%s", color_on, com_msgs[subcmd][1],
	      argument, KNRM);
    else
      sprintf(buf1, "You %s, '%s'", com_msgs[subcmd][1], argument);
    act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
  }

  sprintf(buf, "$n %ss, '%s'", com_msgs[subcmd][1], argument);

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i != ch->desc && i->character &&
	!PRF_FLAGGED(i->character, channels[subcmd]) &&
	!TMP_FLAGGED(i->character, TMP_WRITING | TMP_OLC | TMP_MAILING) &&
	!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
	  ((world[ch->in_room].zone != world[i->character->in_room].zone) ||
	   GET_POS(i->character) < POS_RESTING))
	continue;

      if (COLOR_LEV(i->character) >= C_NRM)
	send_to_char(color_on, i->character);
      act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
      if (COLOR_LEV(i->character) >= C_NRM)
	send_to_char(KNRM, i->character);
    }
  }
}


ACMD(do_qcomm)
{
  struct descriptor_data *i;

  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char("You aren't even part of the quest!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    sprintf(buf, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
	    CMD_NAME);
    CAP(buf);
    send_to_char(buf, ch);
  } else {
    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(OK, ch);
    else {
      if (subcmd == SCMD_QSAY)
	sprintf(buf, "You quest-say, '%s'", argument);
      else
	strcpy(buf, argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }

    if (subcmd == SCMD_QSAY)
      sprintf(buf, "$n quest-says, '%s'", argument);
    else
      strcpy(buf, argument);

    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i != ch->desc &&
	  PRF_FLAGGED(i->character, PRF_QUEST))
	act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}
