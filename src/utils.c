/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
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
#include "screen.h"
#include "spells.h"
#include "handler.h"

extern struct time_data time_info;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern int sunlight;

int exp_to_level(sh_int level)
{
  int exp;
  
  if (!level) return 1;

  switch (level) {
  case 1: case 2: case 3: case 4: case 5:
    exp = (level * 500);
    break;
  case 6: case 7: case 8: case 9: case 10: 
    exp = exp_to_level(5) + ((level-6) * 1000);
    break;
  case 11: case 12: case 13: case 14: case 15: 
    exp = exp_to_level(10) + ((level-10) * 2500);
    break;
  case 16: case 17: case 18: case 19: case 20:
    exp = exp_to_level(15) + ((level-15) * 5000);
    break;
  case 21: case 22: case 23: case 24: case 25:
    exp =  exp_to_level(20) + ((level-20) * 20000); 
    break;
  case 26: case 27: case 28: case 29: case 30:
    exp = exp_to_level(25) + ((level-25) * 50000);
    break;
  case 31: case 32: case 33: case 34: case 35:
    exp = exp_to_level(30) + ((level-30) * 100000);
    break;
  case 36: case 37: case 38: case 39: case 40:
    exp = exp_to_level(35) + ((level-35) * 250000);
    break;
  case 41: case 42: case 43: case 44: case 45:
    exp = exp_to_level(40) + ((level-40) * 500000);
    break;
  case 46: case 47: case 48: case 49: case 50: case 51: case 52: case 53:
    exp =exp_to_level(45)  + ((level-45) * 1000000);
    break;
  case 54: case 55: case 56: case 57: case 58: case 59: case 60:
    exp =  exp_to_level(53)   + ((level-53) * 5000000);
    break;
  case 61: case 62: case 63: case 64: case 65: case 66:
    exp = exp_to_level(60) + ((level-60) * 7500000);
    break;
  case 67: case 68: case 69: case 70: case 71: case 72: case 73:
    exp = exp_to_level(66) + ((level-66) * 20000000);
    break;
  case 74: case 75: case 76: case 77: case 78:
    exp = exp_to_level(73) + ((level-73) * 35000000);
    break;
  case 79: case 80: case 81: case 82: case 83: case 84: case 85:
    exp = exp_to_level(78) + ((level-78) * 50000000);
    break;
  case 86: case 87: case 88: case 89: case 90: case 91: case 92:
    exp = exp_to_level(85) + ((level-85) * 55000000);
    break;
  case 93: case 94: case 95: case 96: case 97: case 98:
    exp = exp_to_level(92) + ((level-92) * 60000000);
    break;
  case 99: case 100:
    exp =  exp_to_level(98) + ((level-98) * 65000000);
    break;
  case 101: case 102:
    exp =   exp_to_level(100) + ((level-100) * 100000000);
    break;
  case 103: case 104: case 105: case 106: case 107: case 108: 
  case 109: case 110: case 111: case 112: case 113: case 114:
    exp =  exp_to_level(102) + ((level-102) * 2000000);
    break;
  default:
    mudlogf(CMP, LVL_IMMORT, FALSE, "SYSERR: exp_to_level(int) reached Default Case!");
    exp = 1;
    break;
  }

  return exp;
}



/* creates a random number in interval [from;to] */
int number(int from, int to)
{
  /* error checking in case people call number() incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
  }

  return ((random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int number, int size)
{
  int sum = 0;

  if (size <= 0 || number <= 0)
    return 0;

  while (number-- > 0)
    sum += ((random() % size) + 1);

  return sum;
}


int MIN(int a, int b)
{
  return a < b ? a : b;
}


int MAX(int a, int b)
{
  return a > b ? a : b;
}



/* Create a duplicate of a string */
char *str_dup(const char *source)
{
  char *new;

  CREATE(new, char, strlen(source) + 1);
  return (strcpy(new, source));
}



/* str_cmp: a case-insensitive version of strcmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  for (i = 0; *(arg1 + i) || *(arg2 + i); i++)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) {
      if (chk < 0)
	return (-1);
      else
	return (1);
    }
  return (0);
}


/* strn_cmp: a case-insensitive version of strncmp */
/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different, end of both, or n reached     */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  for (i = 0; (*(arg1 + i) || *(arg2 + i)) && (n > 0); i++, n--)
    if ((chk = LOWER(*(arg1 + i)) - LOWER(*(arg2 + i)))) {
      if (chk < 0)
	return (-1);
      else
	return (1);
    }

  return (0);
}


/* log a death trap hit */
void log_death_trap(struct char_data * ch)
{
  mudlogf(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch),
          world[ch->in_room].number, world[ch->in_room].name);
}

/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    perror(path);
    return -1;
  } else {
    fclose(fl);
    return 0;
  }
}

void sprintbit(long bitvector, const char *names[], char *result)
{
  long nr;

  *result = '\0';

  if (bitvector < 0) {
    strcpy(result, "<INVALID BITVECTOR>");
    return;
  }
  for (nr = 0; bitvector; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      if (*names[nr] != '\n') {
	strcat(result, names[nr]);
	strcat(result, " ");
      } else
	strcat(result, "UNDEFINED ");
    }
    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    strcpy(result, "NOBITS ");
}



void sprinttype(int type, const char *names[], char *result)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  if (*names[nr] != '\n')
    strcpy(result, names[nr]);
  else
    strcpy(result, "UNDEFINED");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data real_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  secs -= SECS_PER_REAL_DAY * now.day;

  now.month = -1;
  now.year = -1;

  return now;
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;	/* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17;	/* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return now;
}



struct time_info_data age(struct char_data * ch)
{
  struct time_info_data player_age;

  player_age = mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17;	/* All players start at 17 */

  return player_age;
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data * ch, struct char_data * victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return TRUE;
  }

  return FALSE;
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  assert(ch->master);

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
    act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
    if (affected_by_spell(ch, SPELL_CHARM))
      affect_from_char(ch, SPELL_CHARM);
  } else {
    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
  }

  if (ch->master->followers->follower == ch) {	/* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else {			/* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next);

    j = k->next;
    k->next = j->next;
    free(j);
  }

  ch->master = NULL;
  REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}



/* Called when a character that follows/is followed dies */
void die_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data * ch, struct char_data * leader)
{
  struct follow_type *k;

  assert(!ch->master);

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (CAN_SEE(leader, ch))
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf)
{
  char temp[256];
  int lines = 0;

  do {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl)) {
    *buf = '\0';	/* Make it NULL so we crash if wrongly used. */
    return 0;
  } else {
    strcpy(buf, temp);
    return lines;
  }
}


int get_filename(char *orig_name, char *filename, int mode)
{
  const char *prefix, *suffix, *middle;
  char *ptr, name[64];

  switch (mode) {

/* allows alias saving */

  case ALIAS_FILE :
    prefix = "plralias";
    suffix = "alias"; 
    break;

  case CRASH_FILE:
    prefix = "plrobjs";
    suffix = "objs";
    break;
  case ETEXT_FILE:
    prefix = "plrtext";
    suffix = "text";
    break;
  default:
    return 0;
    break;
  }

  if (!*orig_name)
    return 0;

  strcpy(name, orig_name);
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  sprintf(filename, "%s/%s/%s.%s", prefix, middle, name, suffix);
  return 1;
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return i;
}


/* string manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
int replace_str(char **string, char *pattern, char *replacement, int rep_all,
		int max_size) {
   char *replace_buffer = NULL;
   char *flow, *jetsam, temp;
   int len, i;
   
   if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
     return -1;
   
   CREATE(replace_buffer, char, max_size);
   i = 0;
   jetsam = *string;
   flow = *string;
   *replace_buffer = '\0';
   if (rep_all) {
      while ((flow = (char *)strstr(flow, pattern)) != NULL) {
	 i++;
	 temp = *flow;
	 *flow = '\0';
	 if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size) {
	    i = -1;
	    break;
	 }
	 strcat(replace_buffer, jetsam);
	 strcat(replace_buffer, replacement);
	 *flow = temp;
	 flow += strlen(pattern);
	 jetsam = flow;
      }
      strcat(replace_buffer, jetsam);
   }
   else {
      if ((flow = (char *)strstr(*string, pattern)) != NULL) {
	 i++;
	 flow += strlen(pattern);  
	 len = ((char *)flow - (char *)*string) - strlen(pattern);
   
	 strncpy(replace_buffer, *string, len);
	 strcat(replace_buffer, replacement);
	 strcat(replace_buffer, flow);
      }
   }
   if (i == 0) return 0;
   if (i > 0) {
      RECREATE(*string, char, strlen(replace_buffer) + 3);
      strcpy(*string, replace_buffer);
   }
   free(replace_buffer);
   return i;
}


/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */

void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen) {

  int total_chars, cap_next = TRUE, cap_next_next = FALSE;
  char *flow, *start = NULL, temp;
  /* warning: do not edit messages with max_str's of over this value */
  char formated[MAX_STRING_LENGTH];
   
  flow   = *ptr_string;
  if (!flow) return;
  
  if (IS_SET(mode, FORMAT_INDENT)) {
    strcpy(formated, "   ");
    total_chars = 3;
  }
  else {
    *formated = '\0';
    total_chars = 0;
  } 

  while (*flow != '\0') {
    while ((*flow == '\n') ||
	   (*flow == '\r') ||
	   (*flow == '\f') ||
	   (*flow == '\t') ||
	   (*flow == '\v') ||
	   (*flow == ' ')) flow++;
    
    if (*flow != '\0') {

      start = flow++;
      while ((*flow != '\0') &&
	     (*flow != '\n') &&
	     (*flow != '\r') &&
	     (*flow != '\f') &&
	     (*flow != '\t') &&
	     (*flow != '\v') &&
	     (*flow != ' ') &&
	     (*flow != '.') &&
	     (*flow != '?') &&
	     (*flow != '!')) flow++;
      
      if (cap_next_next) {
	cap_next_next = FALSE;
	cap_next = TRUE;
      }

      /* this is so that if we stopped on a sentance .. we move off the sentance delim. */
	 while ((*flow == '.') || (*flow == '!') || (*flow == '?')) {
	   cap_next_next = TRUE;
	   flow++;
	 }
	 
      temp = *flow;
      *flow = '\0';

      if ((total_chars + strlen(start) + 1) > 79) {
	strcat(formated, "\r\n");
	total_chars = 0;
      }
      
      if (!cap_next) {
	if (total_chars > 0) {
	  strcat(formated, " ");
	  total_chars++;
	}
      }
      else {
	cap_next = FALSE;
	*start = UPPER(*start);
      }

      total_chars += strlen(start);
      strcat(formated, start);

      *flow = temp;
    }

    if (cap_next_next) {
      if ((total_chars + 3) > 79) {
	strcat(formated, "\r\n");
	total_chars = 0;
      }
      else {
	strcat(formated, " ");
	total_chars += 2;
      }
    }
  }
  strcat(formated, "\r\n");
  
  if (strlen(formated) > maxlen) formated[maxlen] = '\0';
  RECREATE(*ptr_string, char, MIN(maxlen, strlen(formated)+3));
  strcpy(*ptr_string, formated);
}
  
/* strips \r's from line */
char *stripcr(char *dest, const char *src) {
   int i, length;
   char *temp;

   if (!dest || !src) return NULL;
   temp = &dest[0];
   length = strlen(src);
   for (i = 0; *src && (i < length); i++, src++)
     if (*src != '\r') *(temp++) = *src;
   *temp = '\0';
   return dest;
}
