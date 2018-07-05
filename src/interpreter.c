/*
************************************************************************
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "olc.h"
#include "loadrooms.h"

extern const char *PK;
extern const char *ANSI;
extern const char *GREETINGS;
extern const char *ANSIGREET;
extern char *motd;
extern char *imotd;
extern char *background;
extern char *policies;
extern char *MENU;
extern char *WELC_MESSG;
extern char *START_MESSG;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;
extern int top_of_p_table;
extern int restrict;
extern int no_specials;
extern sh_int r_mortal_start_room[NUM_STARTROOMS +1];
extern sh_int r_jailed_start_room;
extern int max_bad_pws;
extern sh_int r_immort_start_room;
extern sh_int r_frozen_start_room;
extern sh_int r_newbie_start_room;
extern const char *class_menu;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;




/* external functions */
void echo_on(struct descriptor_data *d);
void echo_off(struct descriptor_data *d);
void do_start(struct char_data *ch);
void init_char(struct char_data *ch);
int load_char(char *name, struct char_file_u *char_element);
int parse_class(struct char_data *ch, char arg);
int create_entry(char *name);
int special(struct char_data *ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void oedit_parse(struct descriptor_data *d, char *arg);
void redit_parse(struct descriptor_data *d, char *arg);
void zedit_parse(struct descriptor_data *d, char *arg);
void medit_parse(struct descriptor_data *d, char *arg);
void sedit_parse(struct descriptor_data *d, char *arg);
void read_aliases(struct char_data *ch);
extern char *class_display[];
void sort_commands(void);
int get_from_q(struct txt_q *queue, char *dest, int *aliased);

/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_advance);
ACMD(do_affects);
ACMD(do_alias);
ACMD(do_areas);
ACMD(do_arena);
ACMD(do_arenaload);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_auction);
ACMD(do_auctioneer);
ACMD(do_autoquest);
ACMD(do_awho);
ACMD(do_ahall);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_bet);
ACMD(do_bid);
ACMD(do_brew);
ACMD(do_buck);
ACMD(do_cast);
ACMD(do_castle);
ACMD(do_chaos);
ACMD(do_chown);
ACMD(do_chskill);
ACMD(do_clanbuild);
ACMD(do_clanmember);
ACMD(do_color);
ACMD(do_concent);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_copyto);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_divorce);
ACMD(do_diagnose);
ACMD(do_disable);
ACMD(do_dismount);
ACMD(do_display);
ACMD(do_disarm);
ACMD(do_dns);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exchange);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_fas);
ACMD(do_file);
ACMD(do_fire);
ACMD(do_flee);
ACMD(do_forage);
ACMD(do_forge);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gact);
ACMD(do_gecho);
ACMD(do_gmote);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_godengr);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_gohome);
ACMD(do_givehome);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_heal);
ACMD(do_healall);
ACMD(do_help);
ACMD(do_hero);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_house);
ACMD(do_info);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_immgoss);
ACMD(do_impnet);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_knife);
ACMD(do_land);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_liblist);
ACMD(do_linkload);
ACMD(do_listen);
ACMD(do_load);
ACMD(do_load_weapon);
ACMD(do_lockdown);
ACMD(do_look);
ACMD(do_marry);
ACMD(do_move);
ACMD(do_mount);
ACMD(do_msay);
ACMD(do_msummon);
ACMD(do_newquest);
ACMD(do_not_here);
ACMD(do_nuke);
ACMD(do_objdam);
ACMD(do_offer);
ACMD(do_olc);
ACMD(do_order);
ACMD(do_page);
ACMD(do_peace);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_private_channel);
ACMD(do_practice);
ACMD(do_prompt);
ACMD(do_purge);
ACMD(do_push);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_qpexchange);
ACMD(do_qpoints);
ACMD(do_quit);
ACMD(do_recall);
ACMD(do_rdig);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_rent);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_roomlink);
ACMD(do_rspan);
ACMD (do_sacrifice);
ACMD(do_save);
ACMD(do_say);
ACMD(do_site);
ACMD(do_system);
ACMD(do_score);
ACMD(do_scan);
ACMD(do_scribe);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skills);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_speak);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_stop_auction);
ACMD(do_strike);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_takehome);
ACMD(do_tame);
ACMD(do_tedit);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_updtwiz);
ACMD(do_upme);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_vitem);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whois);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_xname);
ACMD(do_zreset);
ACMD(do_zstat);
ACMD(do_zload);


/* This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

const struct command_info cmd_info[] = {
  { "RESERVED", 0, 0, 0, 0 },	/* this must be first -- for specprocs */

  /* directions must come before other commands but after RESERVED */
  { "north"    , POS_STANDING, do_move     , 0, SCMD_NORTH },
  { "east"     , POS_STANDING, do_move     , 0, SCMD_EAST },
  { "south"    , POS_STANDING, do_move     , 0, SCMD_SOUTH },
  { "west"     , POS_STANDING, do_move     , 0, SCMD_WEST },
  { "up"       , POS_STANDING, do_move     , 0, SCMD_UP },
  { "down"     , POS_STANDING, do_move     , 0, SCMD_DOWN },

  /* now, the main list */
  { "ack"      , POS_RESTING , do_action    , 0, 0 },
  { "affects"  , POS_DEAD    , do_affects  , 0, 0 },
  { "at"       , POS_DEAD    , do_at        , LVL_GOD, 0 },
  { "advance"  , POS_DEAD    , do_advance   , LVL_SUPREME, 0},
  { "alias"    , POS_DEAD    , do_alias     , 0, 0 },
  { "accuse"   , POS_SITTING , do_action    , 0, 0 },
  { "ache"     , POS_SITTING , do_action    , 0, 0 },
  { "agree"    , POS_SITTING , do_action    , 0, 0 },
  { "areas"    , POS_DEAD    , do_areas    , 0, 0 },
  { "assist"   , POS_FIGHTING, do_assist    , 1, 0 },
  { "ask"      , POS_RESTING , do_spec_comm , 0, SCMD_ASK },
  { "auction"  , POS_SLEEPING, do_auction   , 0, 0 },
  { "auctioneer", POS_DEAD   , do_auctioneer, LVL_GOD, 0 },
  { "afk"      , POS_DEAD    , do_gen_tog   , 1, SCMD_AFK },
  { "autoexit" , POS_DEAD    , do_gen_tog   , 0, SCMD_AUTOEXIT },
  { "autoloot" , POS_DEAD    , do_gen_tog   , 0, SCMD_AUTOLOOT},
  { "autogold" , POS_DEAD    , do_gen_tog   , 0, SCMD_AUTOGOLD},
  { "autosplit", POS_DEAD    , do_gen_tog   , 0, SCMD_AUTOSPLIT},
  { "autoassist", POS_DEAD   , do_gen_tog   , 0, SCMD_AUTOASSIST},
  { "arena",     POS_STANDING, do_arena     , 0, 0},
  { "arse"     , POS_SITTING , do_action    , 0, 0 },
  { "aload"    , POS_STANDING, do_arenaload , LVL_GOD, 0},
  { "awho",      POS_STANDING, do_awho      , 0, 0},
  { "ahall",     POS_STANDING, do_ahall     , 0, 0},
  { "astart"   , POS_RESTING , do_chaos     , LVL_GOD, 0 },
  { "aww"      , POS_RESTING , do_action    , 0, 0 },

  { "bathe"    , POS_STANDING, do_action   , 0, 0 },
  { "baton"    , POS_STANDING, do_action   , 0, 0 },
  { "bounce"   , POS_STANDING, do_action   , 0, 0 },
  { "backstab" , POS_STANDING, do_backstab , 1, 0 },
  { "ban"      , POS_DEAD    , do_ban      , LVL_BAN, 0},
  { "balance"  , POS_STANDING, do_not_here , 1, 0 },
  { "bash"     , POS_FIGHTING, do_bash     , 1, 0 },
  { "beg"      , POS_RESTING , do_action   , 0, 0 },
  { "bet"      , POS_RESTING , do_bet      , 0, 0 },
  { "bid"      , POS_RESTING , do_bid      , 0, 0 },
  { "bleed"    , POS_RESTING , do_action   , 0, 0 },
  { "blow"     , POS_RESTING , do_action   , 0, 0 },
  { "blush"    , POS_RESTING , do_action   , 0, 0 },
  { "bow"      , POS_STANDING, do_action   , 0, 0 },
  { "boink"    , POS_STANDING, do_action   , 0, 0 },
  { "brew"     , POS_STANDING, do_brew     , 0, 0 },
  { "brb"      , POS_RESTING , do_action   , 0, 0 },
  { "brief"    , POS_DEAD    , do_gen_tog  , 0, SCMD_BRIEF },
  { "buck"     , POS_STANDING, do_buck	   , 0, 0 },
  { "burp"     , POS_RESTING , do_action   , 0, 0 },
  { "buy"      , POS_STANDING, do_not_here , 0, 0 },
  { "bug"      , POS_DEAD    , do_gen_write, 0, SCMD_BUG },

  { "cast"     , POS_SITTING , do_cast     , 1, 0 },
  { "cackle"   , POS_RESTING , do_action   , 0, 0 },
  { "cards"    , POS_SITTING , do_action   , 0, 0 },
  { "caress"   , POS_RESTING , do_action   , 0, 0 },
  { "cheer"    , POS_RESTING , do_action   , 0, 0 },
  { "check"    , POS_STANDING, do_not_here , 1, 0 },
  { "chuckle"  , POS_RESTING , do_action   , 0, 0 },
  { "cigarette", POS_RESTING , do_action   , 0, 0 },
  { "clap"     , POS_RESTING , do_action   , 0, 0 },
  { "cbuild"   , POS_STANDING, do_clanbuild, LVL_GRGOD, 0 },
  { "chown"    , POS_STANDING, do_chown    , LVL_SUPREME, 0 },
  { "chskill"   , POS_STANDING, do_chskill , LVL_CREATOR, 0 },
  { "clanmem"  , POS_STANDING, do_clanmember , 0, 0 },
  { "close"    , POS_SITTING , do_gen_door , 0, SCMD_CLOSE },
  { "cls"      , POS_DEAD    , do_gen_ps   , 0, SCMD_CLEAR },
  { "clip"     , POS_RESTING , do_action   , 0, 0 },
  { "comfort"  , POS_RESTING , do_action   , 0, 0 },
  { "commis"   , POS_SLEEPING, do_gen_comm , 0, SCMD_COMMIS },
  { "court"    , POS_RESTING , do_action   , 0, 0 },
  { "consider" , POS_RESTING , do_consider , 0, 0 },
  { "colour"   , POS_DEAD    , do_color    , 0, 0 },
  { "consent"  , POS_DEAD    , do_concent  , 0, 0 },
  { "comfort"  , POS_RESTING , do_action   , 0, 0 },
  { "comb"     , POS_RESTING , do_action   , 0, 0 },
  { "commands" , POS_DEAD    , do_commands , 0, SCMD_COMMANDS },
  { "comprehend", POS_RESTING, do_action   , 0,  0},
  { "compact"  , POS_DEAD    , do_gen_tog  , 0, SCMD_COMPACT },
  { "copy"     , POS_STANDING, do_copyto   , LVL_OLC, 0 },
  { "cough"    , POS_RESTING , do_action   , 0, 0 },
  { "credits"  , POS_DEAD    , do_gen_ps   , 0, SCMD_CREDITS },
  { "cringe"   , POS_RESTING , do_action   , 0, 0 },
  { "cry"      , POS_RESTING , do_action   , 0, 0 },
  { "cuddle"   , POS_RESTING , do_action   , 0, 0 },
  { "curse"    , POS_RESTING , do_action   , 0, 0 },

  { "dance"    , POS_STANDING, do_action   , 0, 0 },
  { "damage"   , POS_DEAD    , do_gen_tog  , 1, SCMD_DAMAGE },
  { "date"     , POS_DEAD    , do_date     , LVL_OVERSEER, SCMD_DATE },
  { "dc"       , POS_DEAD    , do_dc       , LVL_SUPREME, 0 },
  { "deposit"  , POS_STANDING, do_not_here , 1, 0 },
  { "despair"  , POS_SLEEPING, do_action   , 0, 0 },
  { "diagnose" , POS_RESTING , do_diagnose , 0, 0 },
  { "disable"  , POS_DEAD    , do_disable  , LVL_LOWIMPL, 0 },
  { "disarm"   , POS_FIGHTING, do_disarm   , 0, 0 },
  { "dismount" , POS_STANDING, do_dismount , 0, 0 },
  { "display"  , POS_DEAD    , do_display  , 0, 0 },
  { "disturb" , POS_SLEEPING, do_action   , 0, 0 },
  { "divorce"  , POS_RESTING , do_divorce  , LVL_CREATOR, 0 },
  { "dns"      , POS_DEAD    , do_dns      , LVL_LOWIMPL, 0 },
  { "doh"      , POS_RESTING , do_action   , 0, 0 },
  { "donate"   , POS_RESTING , do_drop     , 0, SCMD_DONATE },
  { "drink"    , POS_RESTING , do_drink    , 0, SCMD_DRINK },
  { "drop"     , POS_RESTING , do_drop     , 0, SCMD_DROP },
  { "drool"    , POS_RESTING , do_action   , 0, 0 },
  { "duck"    , POS_RESTING , do_action   , 0, 0 },

  { "eat"      , POS_RESTING , do_eat      , 0, SCMD_EAT },
  { "echo"     , POS_SLEEPING, do_echo     , LVL_GOD, SCMD_ECHO },
  { "emote"    , POS_RESTING , do_echo     , 1, SCMD_EMOTE },
  { ":"        , POS_RESTING, do_echo      , 1, SCMD_EMOTE },
  { "embrace"  , POS_STANDING, do_action   , 0, 0 },
  { "enter"    , POS_STANDING, do_enter    , 0, 0 },
  { "engrave"  , POS_STANDING, do_not_here , 1, 0 },
  { "equipment", POS_SLEEPING, do_equipment, 0, 0 },
  { "eyebrow"  , POS_STANDING, do_action   , 0, 0 },
  { "exits"    , POS_RESTING , do_exits    , 0, 0 },
  { "examine"  , POS_SITTING , do_examine  , 0, 0 },
  { "excite"    , POS_RESTING , do_action    , 0, 0 },
  { "exchange"  , POS_DEAD    , do_exchange  , 1, 0 },

  { "fas"      , POS_SLEEPING, do_fas      , LVL_ARCHWIZ, 0 },
  { "force"    , POS_SLEEPING, do_force    , LVL_GRGOD, 0 },
  { "fart"     , POS_RESTING , do_action   , 0, 0 },
  { "flex"     , POS_RESTING , do_action   , 0, 0 },
  { "fwap"     , POS_RESTING , do_action   , 0, 0 },
  { "faint"    , POS_RESTING , do_action   , 0, 0 },
  { "file"     , POS_SLEEPING, do_file     , LVL_GOD, 0},
  { "fill"     , POS_STANDING, do_pour     , 0, SCMD_FILL },
  { "fire"     , POS_FIGHTING , do_fire    , 1, 0 },
  { "fireball" , POS_STANDING, do_action   , LVL_OVERSEER, 0 },
  { "flagcopy" , POS_DEAD    , do_rspan    , LVL_OLC, 0 },
  { "flee"     , POS_FIGHTING, do_flee     , 1, 0 },
  { "flip"     , POS_STANDING, do_action   , 0, 0 },
  { "flirt"    , POS_RESTING , do_action   , 0, 0 },
  { "flutter"  , POS_RESTING , do_action   , 0, 0 },
  { "follow"   , POS_RESTING , do_follow   , 0, 0 },
  { "forage"   , POS_FIGHTING, do_forage   , 1, 0 },
  { "forge"    , POS_STANDING, do_forge    , 0, 0 },
  { "forehead" , POS_STANDING, do_action   , 0, 0 },
  { "french"   , POS_RESTING , do_action   , 0, 0 },
  { "freeze"   , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_FREEZE },
  { "frown"    , POS_RESTING , do_action   , 0, 0 },
  { "fuck"     , POS_RESTING , do_action   , 0, 0 },
  { "fume"     , POS_RESTING , do_action   , 0, 0 },

  { "get"      , POS_RESTING , do_get      , 0, 0 },
  { "gasp"     , POS_RESTING , do_action   , 0, 0 },
  { "gecho"    , POS_DEAD    , do_gecho    , LVL_GRGOD, 0 },
  { "gact"     , POS_STANDING, do_gact   , 0, 0 },
  { "gemote"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GMOTE },
  { "give"     , POS_RESTING , do_give     , 0, 0 },
  { "giggle"   , POS_RESTING , do_action   , 0, 0 },
  { "givehome" , POS_DEAD    , do_givehome , LVL_SUPREME, 0 },
  { "glare"    , POS_RESTING , do_action   , 0, 0 },
  { "goto"     , POS_SLEEPING, do_goto     , LVL_IMMORT, 0 },
  { "godengr"  , POS_SLEEPING, do_godengr  , LVL_GRGOD, 0 },
  { "gold"     , POS_RESTING , do_gold     , 0, 0 },
  { "gossip"   , POS_SLEEPING, do_gen_comm , 0, SCMD_GOSSIP },
  { "gohome"   , POS_STANDING, do_gohome   , 1, 0 },
  { "group"    , POS_RESTING , do_group    , 1, 0 },
  { "grab"     , POS_RESTING , do_grab     , 0, 0 },
  { "grats"    , POS_SLEEPING, do_gen_comm , 0, SCMD_GRATZ },
  { "grin"     , POS_RESTING , do_action   , 0, 0 },
  { "grimace"  , POS_RESTING , do_action   , 0, 0 },
  { "groan"    , POS_RESTING , do_action   , 0, 0 },
  { "grovel"   , POS_RESTING , do_action   , 0, 0 },
  { "growl"    , POS_RESTING , do_action   , 0, 0 },
  { "grumble"  , POS_RESTING , do_action   , 0, 0 },
  { "grunt"    , POS_RESTING , do_action   , 0, 0 },
  { "gyrate"   , POS_RESTING , do_action   , 0, 0 },
  { "gsay"     , POS_SLEEPING, do_gsay     , 0, 0 },
  { "gtell"    , POS_SLEEPING, do_gsay     , 0, 0 },

  { "help"     , POS_DEAD    , do_help     , 0, 0 },
  { "handbook" , POS_DEAD    , do_gen_ps   , LVL_OVERSEER, SCMD_HANDBOOK },
  { "hcontrol" , POS_STANDING, do_hcontrol , LVL_SUPREME, 0 },
  { "heal"     , POS_DEAD    , do_not_here , 0, 0 },
  { "healall"  , POS_DEAD    , do_healall  , LVL_GRGOD, 0 },
  { "hero"     , POS_DEAD    , do_hero     , LVL_HERO, 0 },
  { "herolist" , POS_DEAD    , do_gen_ps   , 0, SCMD_HEROLIST },
  { "handbag"  , POS_RESTING , do_action   , 0, 0 },
  { "headbutt" , POS_RESTING , do_action   , 0, 0 },
  { "hiccup"   , POS_RESTING , do_action   , 0, 0 },
  { "hickey"   , POS_RESTING , do_action   , 0, 0 },
  { "highfive" , POS_RESTING , do_action   , 0, 0 },
  { "hop"      , POS_RESTING , do_action   , 0, 0 },
  { "hide"     , POS_RESTING , do_hide     , 1, 0 },
  { "hit"      , POS_FIGHTING, do_hit      , 0, SCMD_HIT },
  { "hold"     , POS_RESTING , do_grab     , 1, 0 },
  { "holler"   , POS_RESTING , do_gen_comm , 1, SCMD_HOLLER },
  { "holylight", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_HOLYLIGHT },
  { "hop"      , POS_RESTING , do_action   , 0, 0 },
  { "howl"      , POS_RESTING , do_action   , 0, 0 },
  { "house"    , POS_RESTING , do_house    , 0, 0 },
  { "hug"      , POS_RESTING , do_action   , 0, 0 },

  { "inventory", POS_DEAD    , do_inventory, 0, 0 },
  { "idea"     , POS_DEAD    , do_gen_write, 0, SCMD_IDEA },
  { "imotd"    , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_IMOTD },
  { "immlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_IMMLIST },
  { "info"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_INFO },
  { "insult"   , POS_RESTING , do_insult   , 0, 0 },
  { "innuendo" , POS_RESTING , do_action   , 0, 0 },
  { "invis"    , POS_DEAD    , do_invis    , LVL_GUARDIAN, 0 },
  { "immgoss"  , POS_DEAD    , do_immgoss  , LVL_WIZARD, 0 },
  { "impnet"   , POS_DEAD    , do_impnet   , LVL_LOWIMPL, 0 },
  { ","        , POS_DEAD    , do_impnet   , LVL_LOWIMPL, 0 },

  { "jail"     , POS_DEAD    , do_wizutil  , LVL_JAIL, SCMD_JAIL },
  { "junk"     , POS_RESTING , do_drop     , 0, SCMD_JUNK },
  { "jeer"     , POS_RESTING , do_action   , 0, 0 },

  { "kill"     , POS_FIGHTING, do_kill     , 0, 0 },
  { "kick"     , POS_FIGHTING, do_kick     , 1, 0 },
  { "knife"    , POS_FIGHTING, do_knife     , 1, 0 },
  { "kiss"     , POS_RESTING , do_action   , 0, 0 },
  { "kneel"    , POS_RESTING , do_action   , 0, 0 },

  { "look"     , POS_RESTING , do_look     , 0, SCMD_LOOK },
  { "land"     , POS_STANDING, do_land     , 0, 0 },
  { "laugh"    , POS_RESTING , do_action   , 0, 0 },
  { "last"     , POS_DEAD    , do_last     , LVL_RULER, 0 },
  { "leave"    , POS_STANDING, do_leave    , 0, 0 },
  { "leer"     , POS_RESTING , do_action   , 0, 0 },
  { "levels"   , POS_DEAD    , do_levels   , 0, 0 },
  { "list"     , POS_STANDING, do_not_here , 0, 0 },
  { "linkload" , POS_DEAD    , do_linkload , LVL_LOWIMPL, 0 },
  { "lick"     , POS_RESTING , do_action   , 0, 0 },
  { "listen"   , POS_FIGHTING, do_listen   , 1, 0 },
  { "lock"     , POS_SITTING , do_gen_door , 0, SCMD_LOCK },
  { "lockdown" , POS_DEAD    , do_lockdown , LVL_LOWIMPL, 0 },
  { "load"     , POS_DEAD    , do_load     , LVL_OLC, SCMD_LOADHAND },
  { "love"     , POS_RESTING , do_action   , 0, 0 },
  { "lust"     , POS_RESTING , do_action   , 0, 0 },

  { "marry"    , POS_RESTING , do_marry    , LVL_OVERSEER, 0 },
  { "mental"   , POS_STANDING, do_castle   , LVL_LOWIMPL, 0 },
  { "moan"     , POS_RESTING , do_action   , 0, 0 },
  { "medit"    , POS_DEAD    , do_olc      , LVL_OLC, SCMD_OLC_MEDIT},
  { "motd"     , POS_DEAD    , do_gen_ps   , 0, SCMD_MOTD },
  { "moof"     , POS_RESTING , do_action   , 0, 0 },
  { "moon"     , POS_RESTING , do_action   , 0, 0 },
  { "mount"    , POS_STANDING, do_mount    , 0, 0 },
  { "mail"     , POS_STANDING, do_not_here , 1, 0 },
  { "massage"  , POS_RESTING , do_action   , 0, 0 },
  { "mlist"    , POS_DEAD    , do_liblist  , LVL_OLC, SCMD_MLIST},
  { "msay"     , POS_RESTING , do_msay     , 1, 0 },
  { "muhaha"   , POS_RESTING , do_action   , 0, 0 },
  { "mumble"   , POS_RESTING , do_action   , 0, 0 },
  { "music"    , POS_SLEEPING, do_gen_comm , 0, SCMD_MUSIC },
  { "mute"     , POS_DEAD    , do_wizutil  , LVL_GRGOD, SCMD_SQUELCH },
  { "murder"   , POS_FIGHTING, do_hit      , 0, SCMD_MURDER },
  { "msummon"  , POS_DEAD    , do_msummon  , 0, 0 },

  { "news"     , POS_SLEEPING, do_gen_ps   , 0, SCMD_NEWS },
  { "newquest" , POS_RESTING , do_newquest , LVL_GUARDIAN, 0 },
  { "neck"     , POS_RESTING , do_action   , 0, 0 },
  { "nhandbook" , POS_DEAD    , do_gen_ps   , LVL_IMMORT, SCMD_NEWBIE },
  { "nibble"   , POS_RESTING , do_action   , 0, 0 },
  { "nick"    , POS_STANDING, do_not_here , 1, 0 },
  { "nod"      , POS_RESTING , do_action   , 0, 0 },
  { "nose"     , POS_RESTING , do_action   , 0, 0 },
  { "noauction", POS_DEAD    , do_gen_tog  , 0, SCMD_NOAUCTION },
  { "nogossip" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGOSSIP },
  { "nomusic"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOMUSIC },
  { "nocommis" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOCOMMIS },
  { "nuke"     , POS_RESTING , do_nuke     , LVL_LOWIMPL, 0 },
  { "noarena"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOSPORTS },
  { "nograts"  , POS_DEAD    , do_gen_tog  , 0, SCMD_NOGRATZ },
  { "nohassle" , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOHASSLE },
  { "nopage"   , POS_DEAD    , do_gen_tog  , LVL_GRGOD, SCMD_NOPAGE },
  { "norepeat" , POS_DEAD    , do_gen_tog  , 0, SCMD_NOREPEAT },
  { "noshout"  , POS_SLEEPING, do_gen_tog  , 1, SCMD_DEAF },
  { "nosummon" , POS_DEAD    , do_gen_tog  , 1, SCMD_NOSUMMON },
  { "notell"   , POS_DEAD    , do_gen_tog  , 1, SCMD_NOTELL },
  { "notitle"  , POS_DEAD    , do_wizutil  , LVL_GOD, SCMD_NOTITLE },
  { "nowiz"    , POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_NOWIZ },
  { "nudge"    , POS_RESTING , do_action   , 0, 0 },
  { "nuzzle"   , POS_RESTING , do_action   , 0, 0 },


  { "objdam"   , POS_RESTING , do_objdam   , LVL_SUPREME, 0 },
  { "order"    , POS_RESTING , do_order    , 1, 0 },
  { "offer"    , POS_STANDING, do_not_here , 1, 0 },
  { "office"   , POS_STANDING, do_wizutil  , LVL_SUPREME, SCMD_OFFICE },
  { "open"     , POS_SITTING , do_gen_door , 0, SCMD_OPEN },
  { "olc"      , POS_DEAD    , do_olc      , LVL_OLC, SCMD_OLC_SAVEINFO },
  { "olcrules" , POS_DEAD    , do_gen_ps   , LVL_OLC, SCMD_OLC_RULES },
  { "olist"    , POS_DEAD    , do_liblist  , LVL_OLC, SCMD_OLIST},
  { "oload"    , POS_DEAD    , do_load     , LVL_ARCHWIZ, SCMD_LOADGROUND },
  { "oedit"    , POS_DEAD    , do_olc      , LVL_OLC, SCMD_OLC_OEDIT},
  { "orgasm"   , POS_RESTING , do_action   , 0, 0 },

  { "put"      , POS_RESTING , do_put      , 0, 0 },
  { "pat"      , POS_RESTING , do_action   , 0, 0 },
  { "page"     , POS_DEAD    , do_page     , LVL_GOD, 0 },
  { "pardon"   , POS_DEAD    , do_wizutil  , LVL_CREATOR, SCMD_PARDON },
  { "parole"   , POS_DEAD    , do_wizutil  , LVL_JAIL, SCMD_PAROLE },
  { "peck"     , POS_RESTING , do_action   , 0, 0 },
  { "peer"     , POS_RESTING , do_action   , 0, 0 },
  { "pick"     , POS_STANDING, do_gen_door , 1, SCMD_PICK },
  { "pimpslap" , POS_STANDING, do_action   , 0, 0 },
  { "pin"      , POS_STANDING, do_action   , 0, 0 },
  { "peace"    , POS_DEAD    , do_peace    , LVL_CREATOR, 0 },
  { "point"    , POS_RESTING , do_action   , 0, 0 },
  { "poke"     , POS_RESTING , do_action   , 0, 0 },
  { "pose"     , POS_RESTING , do_action   , 0, 0 },
  { "policy"   , POS_DEAD    , do_gen_ps   , 0, SCMD_POLICIES },
  { "ponder"   , POS_RESTING , do_action   , 0, 0 },
  { "poofin"   , POS_DEAD    , do_poofset  , LVL_IMMORT, SCMD_POOFIN },
  { "poofout"  , POS_DEAD    , do_poofset  , LVL_IMMORT, SCMD_POOFOUT },
  { "pour"     , POS_STANDING, do_pour     , 0, SCMD_POUR },
  { "pout"     , POS_RESTING , do_action   , 0, 0 },
  { "prompt"   , POS_DEAD    , do_prompt   , 0, 0 },
  { "practice" , POS_RESTING , do_practice , 1, 0 },
  { "psay"     , POS_DEAD    , do_private_channel , 0, PRIVATE_SAY },
  { "padd"     , POS_DEAD    , do_private_channel , 0, PRIVATE_ADD },
  { "pclose"   , POS_DEAD    , do_private_channel , 0, PRIVATE_CLOSE },
  { "pleave"     , POS_DEAD    , do_private_channel , 0, PRIVATE_OFF },
  { "popen"    , POS_DEAD    , do_private_channel , 0, PRIVATE_OPEN },
  { "premove"  , POS_DEAD    , do_private_channel , 0, PRIVATE_REMOVE },
  { "pwho"     , POS_DEAD    , do_private_channel , 0, PRIVATE_WHO },
  { "pcheck"   , POS_DEAD    , do_private_channel , LVL_CREATOR, PRIVATE_CHECK },
  { "pray"     , POS_SITTING , do_action   , 0, 0 },
  { "puke"     , POS_RESTING , do_action   , 0, 0 },
  { "punch"    , POS_RESTING , do_action   , 0, 0 },
  { "purr"     , POS_RESTING , do_action   , 0, 0 },
  { "purge"    , POS_DEAD    , do_purge    , LVL_OLC, 0 },
  { "push"     , POS_FIGHTING, do_push     , 1, 0 },
  { "puzzle"   , POS_RESTING , do_action   , 0,  0},


  { "quaff"    , POS_RESTING , do_use      , 0, SCMD_QUAFF },
  { "qecho"    , POS_DEAD    , do_qcomm    , LVL_GUARDIAN, SCMD_QECHO },
  { "quest"    , POS_DEAD    , do_autoquest, 0, 0 },
  { "questflag", POS_DEAD    , do_gen_tog  , 0, SCMD_QUEST },
  { "qui"      , POS_DEAD    , do_quit     , 0, 0 },
  { "quit"     , POS_DEAD    , do_quit     , 0, SCMD_QUIT },
  { "quiver"   , POS_RESTING , do_action   , 0, 0 },
  { "qsay"     , POS_RESTING , do_qcomm    , 0, SCMD_QSAY },
  { "qpoints"  , POS_DEAD    , do_qpoints  , LVL_GUARDIAN, 0 },

  { "reply"    , POS_SLEEPING, do_reply    , 0, 0 },
  { "recall"   , POS_STANDING, do_recall   , 0, 0 },
  { "rdig"     , POS_STANDING, do_rdig     , LVL_OLC, 0 },
  { "rest"     , POS_RESTING , do_rest     , 0, 0 },
  { "read"     , POS_RESTING , do_look     , 0, SCMD_READ },
  { "remove"   , POS_RESTING , do_remove   , 0, 0 },
  { "recite"   , POS_RESTING , do_use      , 0, SCMD_RECITE },
  { "ren"      , POS_RESTING , do_action   , 0, 0 },
  { "raspberry", POS_RESTING , do_action   , 0, 0 },
  { "reload"   , POS_DEAD    , do_reboot   , LVL_LOWIMPL, 0 },
  { "receive"  , POS_STANDING, do_not_here , 1, 0 },
  { "rent"     , POS_STANDING, do_not_here , 1, 0 },
  { "report"   , POS_RESTING , do_report   , 0, 0 },
  { "reroll"   , POS_DEAD    , do_wizutil  , LVL_CREATOR, SCMD_REROLL },
  { "rescue"   , POS_FIGHTING, do_rescue   , 1, 0 },
  { "restore"  , POS_DEAD    , do_restore  , LVL_CREATOR, 0 },
  { "respond", POS_RESTING, do_not_here, 0, 0 },
  { "return"   , POS_DEAD    , do_return   , 0, 0 },
  { "redit"    , POS_DEAD    , do_olc      , LVL_OLC, SCMD_OLC_REDIT},
  { "reloadgun"   , POS_FIGHTING, do_load_weapon , 1, 0 },
  { "rlist"    , POS_DEAD    , do_liblist  , LVL_OLC, SCMD_RLIST},
  { "kiss"     , POS_RESTING , do_action   , 0, 0 },
  { "rock"     , POS_RESTING , do_action   , 0, 0 },
  { "roll"     , POS_RESTING , do_action   , 0, 0 },
  { "rofl"     , POS_RESTING , do_action   , 0, 0 },
  { "romance"  , POS_STANDING, do_action   , 0, 0 },
  { "roomflags", POS_DEAD    , do_gen_tog  , LVL_IMMORT, SCMD_ROOMFLAGS },
  { "roomlink" , POS_DEAD    , do_roomlink , LVL_GRGOD, 0},
  { "ruffle"   , POS_STANDING, do_action   , 0, 0 },
  { "rub"      , POS_STANDING, do_action   , 0, 0 },

  { "sacrifice", POS_RESTING , do_sacrifice, 0, 0 },
  { "say"      , POS_RESTING , do_say      , 0, 0 },
  { "'"        , POS_RESTING , do_say      , 0, 0 },
  { "save"     , POS_SLEEPING, do_save     , 0, 0 },
  { "score"    , POS_DEAD    , do_score    , 0, 0 },
  { "scan"     , POS_DEAD    , do_scan     , 0, 0 },
  { "scribe"   , POS_STANDING, do_scribe   , 0, 0 },
  { "scream"   , POS_RESTING , do_action   , 0, 0 },
  { "sell"     , POS_STANDING, do_not_here , 0, 0 },
  { "send"     , POS_SLEEPING, do_send     , LVL_SUPREME, 0 },
  { "set"      , POS_DEAD    , do_set      , LVL_GOD, 0 },
  { "sedit"    , POS_DEAD    , do_olc      , LVL_LOWIMPL, SCMD_OLC_SEDIT},
  { "sex"    , POS_RESTING , do_action   , 0, 0 },
  { "shout"    , POS_RESTING , do_gen_comm , 0, SCMD_SHOUT },
  { "shame"    , POS_RESTING , do_action , 0, SCMD_SHOUT },
  { "shake"    , POS_RESTING , do_action   , 0, 0 },
  { "shiver"   , POS_RESTING , do_action   , 0, 0 },
  { "shoot"    , POS_FIGHTING , do_fire    , 1, 0 },
  { "show"     , POS_DEAD    , do_show     , LVL_IMMORT, 0 },
  { "shoulder" , POS_RESTING , do_action   , 0, 0 },
  { "squirm"   , POS_RESTING , do_action   , 0, 0 },
  { "shrug"    , POS_RESTING , do_action   , 0, 0 },
  { "shutdow"  , POS_DEAD    , do_shutdown , LVL_LOWIMPL, 0 },
  { "shutdown" , POS_DEAD    , do_shutdown , LVL_LOWIMPL, SCMD_SHUTDOWN },
  { "sigh"     , POS_RESTING , do_action   , 0, 0 },
  { "sing"     , POS_RESTING , do_action   , 0, 0 },
  { "sip"      , POS_RESTING , do_drink    , 0, SCMD_SIP },
  { "sit"      , POS_RESTING , do_sit      , 0, 0 },
  { "site"     , POS_RESTING , do_site     , LVL_LOWIMPL, 0 },
  { "skills"   , POS_RESTING , do_skills   , 0, 0 },
  { "skillset" , POS_SLEEPING, do_skillset , LVL_CREATOR, 0 },
  { "sleep"    , POS_SLEEPING, do_sleep    , 0, 0 },
  { "slap"     , POS_RESTING , do_action   , 0, 0 },
  { "slowns"   , POS_DEAD    , do_gen_tog  , LVL_LOWIMPL, SCMD_SLOWNS },
  { "speak"    , POS_DEAD    , do_speak  , 1, 0 },
  { "smile"    , POS_RESTING , do_action   , 0, 0 },
  { "shishkabab" , POS_RESTING , do_action   , 0, 0 },
  { "spew"     , POS_RESTING , do_action   , 0, 0 },
  { "shudder"  , POS_RESTING , do_action   , 0, 0 },
  { "smirk"    , POS_RESTING , do_action   , 0, 0 },
  { "smug"     , POS_RESTING , do_action   , 0, 0 },
  { "seduce"   , POS_RESTING , do_action   , 0, 0 },
  { "seevnums" , POS_DEAD    , do_gen_tog  , LVL_RULER, SCMD_VNUMS },
  { "snicker"  , POS_RESTING , do_action   , 0, 0 },
  { "snap"     , POS_RESTING , do_action   , 0, 0 },
  { "snarl"    , POS_RESTING , do_action   , 0, 0 },
  { "sneeze"   , POS_RESTING , do_action   , 0, 0 },
  { "sneak"    , POS_STANDING, do_sneak    , 1, 0 },
  { "sniff"    , POS_RESTING , do_action   , 0, 0 },
  { "snore"    , POS_SLEEPING, do_action   , 0, 0 },
  { "snowball" , POS_STANDING, do_action   , LVL_OVERSEER, 0 },
  { "snoop"    , POS_DEAD    , do_snoop    , LVL_CREATOR, 0 },
  { "snuggle"  , POS_RESTING , do_action   , 0, 0 },
  { "socials"  , POS_DEAD    , do_commands , 0, SCMD_SOCIALS },
  { "split"    , POS_SITTING , do_split    , 1, 0 },
  { "spells"   , POS_DEAD    , do_skills   , 0, 0 },
  { "spank"    , POS_RESTING , do_action   , 0, 0 },
  { "spit"     , POS_STANDING, do_action   , 0, 0 },
  { "squeel"   , POS_STANDING, do_action   , 0, 0 },
  { "stand"    , POS_RESTING , do_stand    , 0, 0 },
  { "stare"    , POS_RESTING , do_action   , 0, 0 },
  { "stat"     , POS_DEAD    , do_stat     , LVL_OLC, 0 },
  { "steal"    , POS_STANDING, do_steal    , 1, 0 },
  { "steam"    , POS_RESTING , do_action   , 0, 0 },
  { "stretch"   , POS_RESTING , do_action   , 0, 0 },
  { "strike"   , POS_FIGHTING, do_strike   , 1, 0 },
  { "stroke"   , POS_RESTING , do_action   , 0, 0 },
  { "strut"    , POS_STANDING, do_action   , 0, 0 },
  { "stimpy"   , POS_RESTING , do_action   , 0, 0 },
  { "sulk"     , POS_RESTING , do_action   , 0, 0 },
  { "stopauc"  , POS_DEAD    , do_stop_auction , LVL_GOD, 0 },
  { "switch"   , POS_DEAD    , do_switch   , LVL_GUARDIAN, 0 }, /* set access with switch flag */
  { "system"   , POS_DEAD    , do_system   , LVL_SUPREME, 0 },
  { "syslog"   , POS_DEAD    , do_syslog   , LVL_IMMORT, 0 },

  { "tell"     , POS_DEAD    , do_tell     , 0, 0 },
  { "tap"      , POS_RESTING , do_action   , 0, 0 },
  { "tackle"   , POS_RESTING , do_action   , 0, 0 },
  { "take"     , POS_RESTING , do_get      , 0, 0 },
  { "tame"     , POS_STANDING, do_tame     , 0, 0 },
  { "takehome" , POS_DEAD    , do_takehome , LVL_SUPREME, 0 },
  { "tango"    , POS_STANDING, do_action   , 0, 0 },
  { "taunt"    , POS_RESTING , do_action   , 0, 0 },
  { "taste"    , POS_RESTING , do_eat      , 0, SCMD_TASTE },
  { "tedit"    , POS_DEAD    , do_tedit    , LVL_CREATOR, 0 },
  { "teleport" , POS_DEAD    , do_teleport , LVL_CREATOR, 0 },
  { "thank"    , POS_RESTING , do_action   , 0, 0 },
  { "think"    , POS_RESTING , do_action   , 0, 0 },
  { "thaw"     , POS_DEAD    , do_wizutil  , LVL_FREEZE, SCMD_THAW },
  { "thumbsup" , POS_RESTING , do_action   , 0, 0 },
  { "throw"    , POS_RESTING , do_action   , 0, 0 },
  { "title"    , POS_DEAD    , do_title    , 0, 0 },
  { "tick"     , POS_DEAD    , do_gen_tog  , 0, SCMD_TICK },
  { "time"     , POS_DEAD    , do_time     , 0, 0 },
  { "tip"      , POS_RESTING , do_action   , 0, 0 },
  { "toggle"   , POS_DEAD    , do_toggle   , 0, 0 },
  { "track"    , POS_STANDING, do_track    , 0, 0 },
  { "transfer" , POS_SLEEPING, do_trans    , LVL_GOD, 0 },
  { "tutt"     , POS_RESTING , do_action   , 0, 0 },
  { "twiddle"  , POS_RESTING , do_action   , 0, 0 },
  { "typo"     , POS_DEAD    , do_gen_write, 0, SCMD_TYPO },

  { "unengrave", POS_STANDING, do_not_here , 1, 0 },
  { "unlock"   , POS_SITTING , do_gen_door , 0, SCMD_UNLOCK },
  { "ungroup"  , POS_DEAD    , do_ungroup  , 0, 0 },
  { "unban"    , POS_DEAD    , do_unban    , LVL_BAN, 0 },
  { "unaffect" , POS_DEAD    , do_wizutil  , LVL_CREATOR, SCMD_UNAFFECT },
  { "upme"     , POS_DEAD    , do_upme     , 113, 0 },
  { "uptime"   , POS_DEAD    , do_date     , LVL_RULER, SCMD_UPTIME },
  { "update"   , POS_DEAD    , do_updtwiz  , LVL_LOWIMPL, 0},
  { "urinate"  , POS_RESTING , do_action   , 0, 0 },
  { "use"      , POS_SITTING , do_use      , 1, SCMD_USE },
  { "users"    , POS_DEAD    , do_users    , LVL_GOD, 0 },

  { "value"    , POS_STANDING, do_not_here , 0, 0 },
  { "version"  , POS_DEAD    , do_gen_ps   , 0, SCMD_VERSION },
  { "violin"   , POS_STANDING, do_action   , LVL_SUPREME, 0 },
  { "visible"  , POS_RESTING , do_visible  , 1, 0 },
  { "vnum"     , POS_DEAD    , do_vnum     , LVL_WIZARD, 0 },
  { "vstat"    , POS_DEAD    , do_vstat    , LVL_WIZARD, 0 },
  { "vitem"    , POS_DEAD    , do_vitem    , LVL_WIZARD, 0 },

  { "wake"     , POS_SLEEPING, do_wake     , 0, 0 },
  { "wave"     , POS_RESTING , do_action   , 0, 0 },
  { "wear"     , POS_RESTING , do_wear     , 0, 0 },
  { "waterpistol" , POS_RESTING , do_action   , 0, 0 },
  { "weather"  , POS_RESTING , do_weather  , 0, 0 },
  { "wee"      , POS_RESTING , do_action   , 0, 0 },
  { "wed"      , POS_RESTING , do_action   , 0, 0 },
  { "who"      , POS_DEAD    , do_who      , 0, 0 },
  { "whois"    , POS_DEAD    , do_whois    , 0, 0 },
  { "whoami"   , POS_DEAD    , do_gen_ps   , 0, SCMD_WHOAMI },
  { "whip"     , POS_RESTING , do_action   , 0, 0 },
  { "whack"    , POS_RESTING , do_action   , 0, 0 },
  { "where"    , POS_RESTING , do_where    , 1, 0 },
  { "whimper"  , POS_RESTING , do_action   , 0, 0 },
  { "whisper"  , POS_RESTING , do_spec_comm, 0, SCMD_WHISPER },
  { "whine"    , POS_RESTING , do_action   , 0, 0 },
  { "white"    , POS_RESTING , do_action   , 0, 0 },
  { "wedgie"   , POS_RESTING , do_action   , 0, 0 },
  { "wemote"   , POS_DEAD    , do_wiznet   , LVL_IMMORT, SCMD_WEMOTE },
  { "whoosh"    , POS_RESTING , do_action   , 0, 0 },
  { "whistle"  , POS_RESTING , do_action   , 0, 0 },
  { "wield"    , POS_RESTING , do_wield    , 0, 0 },
  { "wiggle"   , POS_STANDING, do_action   , 0, 0 },
  { "wimpy"    , POS_DEAD    , do_wimpy    , 0, 0 },
  { "wink"     , POS_RESTING , do_action   , 0, 0 },
  { "withdraw" , POS_STANDING, do_not_here , 1, 0 },
  { "wiznet"   , POS_DEAD    , do_wiznet   , LVL_IMMORT, 0 },
  { ";"        , POS_DEAD    , do_wiznet   , LVL_IMMORT, SCMD_WIZNET },
  { "wizhelp"  , POS_SLEEPING, do_commands , LVL_IMMORT, SCMD_WIZHELP },
  { "wizlist"  , POS_DEAD    , do_gen_ps   , 0, SCMD_WIZLIST },
  { "wizlock"  , POS_DEAD    , do_wizlock  , LVL_LOWIMPL, 0 },
  { "worship"  , POS_RESTING , do_action   , 0, 0 },
  { "wrinkle"  , POS_RESTING , do_action   , 0, 0 },
  { "write"    , POS_STANDING, do_write    , 1, 0 },

  { "xname"    , POS_DEAD    , do_xname    , LVL_LOWIMPL, 0 },

  { "yawn"     , POS_RESTING , do_action   , 0, 0 },

  { "zedit"    , POS_DEAD    , do_olc      , LVL_OLC, SCMD_OLC_ZEDIT},
  { "zreset"   , POS_DEAD    , do_zreset   , LVL_OLC, 0 },
  { "zload"    , POS_DEAD    , do_zload    , LVL_GRGOD, 0 },
  { "zstat"    , POS_DEAD    , do_zstat    , LVL_WIZARD, 0 },

  { "\n", 0, 0, 0, 0 } };	/* this must be last */


const char *fill[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};


/* function to newbie equip characters */

void do_newbie(struct char_data *vict) {

  struct obj_data *obj;
  int give_obj[] = {18601, 18602, 18603, 18604, 18609, 18612, 18613, 18614,
		      4532, 4509, 4509, 3103, 6635, -1};
/* replace the 4 digit numbers on the line above with your basic eq -1 HAS
 * to  be the end field
 */

  int i;

  for (i = 0; give_obj[i] != -1; i++) {
    obj = read_object(give_obj[i], VIRTUAL);
    if (obj == NULL)
      continue;
    obj_to_char(obj, vict);
  }
}


/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data *ch, char *argument)
{

#define MAX_HUHMESG  12

  const char *huhmesg[MAX_HUHMESG] =
    {
      "Huh!?\r\n",
      "What?? You want us to invent a new command!?\r\n",
      "Don't you think that would be kinda silly?\r\n",
      "What?\r\n",
      "Sorry what was that!?\r\n",
      "You're not too good at typing are you!?\r\n",
      "Don't talk daft!\r\n",
      "No chance, you can't do that!\r\n",
      "Hahahahha! Maybe not!\r\n",
      "What would your mother say?\r\n",
      "If you wish hard enough it might happen!\r\n",
      "And you thought we were Insane!\r\n"
    };

  int cmd, length, i;
  int is_disabled = 0;
  char *line;

  REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;


  if (!isalpha(*argument) && !isdigit(*argument)) {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  } else
    line = any_one_arg(argument, arg);

  /*
   * If someone just typed a number, then put that number in the argument
   * and put 'goto' in the command string. -gg
   */
  if (GET_LEVEL(ch) >= LVL_IMMORT && isdigit(*arg)) {
    strcpy(line, arg);
    strcpy(arg, "goto");
  }

  /* otherwise, find the command */
  for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strncmp(cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level)
	break;

  /* check for disabled commands */
  for (i = 0; i < 10; i++) {
    if (!strcmp(((ch)->player_specials->saved.disabled[i]), cmd_info[cmd].command))
      is_disabled = 1;
  }

  if (*cmd_info[cmd].command == '\n')
    send_to_char(huhmesg[number(0, MAX_HUHMESG-1)], ch);
  else if (is_disabled) {
    sprintf(buf, "You don't seem to remember how to %s.\r\n", cmd_info[cmd].command); 
    send_to_char(buf, ch);
  }
  else if (PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
  else if((ROOM_FLAGGED(ch->in_room, ROOM_LOCKED_DOWN)) 
	  && (GET_LEVEL(ch) < LVL_LOWIMPL) && 
	  ((strcmp("say", cmd_info[cmd].command)) && 
	   (strcmp("'", cmd_info[cmd].command))))
    send_to_char("You try, but a will greater than yours prevents you...\r\n", ch);
  else if (cmd_info[cmd].command_pointer == NULL)
    send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
  else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
    send_to_char("You can't use immortal commands while switched.\r\n", ch);
  else if (GET_POS(ch) < cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
    case POS_DEAD:
      send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
      break;
    case POS_STUNNED:
      send_to_char("All you can do right now is think about the stars!\r\n", ch);
      break;
    case POS_SLEEPING:
      send_to_char("In your dreams, or what?\r\n", ch);
      break;
    case POS_RESTING:
      send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
      break;
    case POS_SITTING:
      send_to_char("Maybe you should get on your feet first?\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("No way!  You're fighting for your life!\r\n", ch);
      break;
  } else if (no_specials || !special(ch, cmd, line))
    ((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));

}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


struct alias *find_alias(struct alias *alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
	return alias_list;

    alias_list = alias_list->next;
  }

  return NULL;
}


void free_alias(struct alias *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
  char *repl;
  struct alias *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {			/* no argument specified -- list currently defined aliases */
    strcpy(buf, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL) {
      strcat(buf, " None.\r\n");
      send_to_char(buf, ch);
    } else {
      while (a != NULL) {
	sprintf(buf, "%s%-15s %s\r\n", buf, a->alias, a->replacement);
	a = a->next;
      }
      page_string(ch->desc, buf, 1);
    }
  } else {			/* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl) {
      if (a == NULL)
	send_to_char("No such alias.\r\n", ch);
      else
	send_to_char("Alias deleted.\r\n", ch);
    } else {			/* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias")) {
	send_to_char("You can't alias 'alias'.\r\n", ch);
	return;
      }
      CREATE(a, struct alias, 1);
      a->alias = str_dup(arg);
      delete_doubledollar(repl);
      a->replacement = str_dup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
	a->type = ALIAS_COMPLEX;
      else
	a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char("Alias added.\r\n", ch);
    }
  }
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
	strcpy(write_point, tokens[num]);
	write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
	strcpy(write_point, orig);
	write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
	*(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data *d, char *orig)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias *a, *tmp;

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return 0;

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return 0;

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return 0;

  if (a->type == ALIAS_SIMPLE) {
    strcpy(orig, a->replacement);
    return 0;
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return 1;
  }
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
  register int i, l;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
	return (i);
  } else {
    if (!l)
      l = 1;			/* Avoid "" to match the first available
				 * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
	return (i);
  }

  return -1;
}


int is_number(const char *str)
{
  if (!str || !*str)   /* Test for NULL pointer or string. */
    return FALSE;
  if (*str == '-')     /* -'s in front are valid. */
    str++;

  while (*str)
    if (!isdigit(*(str++)))
      return FALSE;

  return TRUE;
}


void skip_spaces(char **string)
{
  for (; **string && isspace(**string); (*string)++);
}


char *delete_doubledollar(char *string)
{
  char *read, *write;

  if ((write = strchr(string, '$')) == NULL)
    return string;

  read = write;

  while (*read)
    if ((*(write++) = *(read++)) == '$')
      if (*read == '$')
	read++;

  *write = '\0';

  return string;
}


int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    *first_arg = '\0';
    return NULL;
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (*argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return argument;
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
  char *begin = first_arg;

  if (!argument) {
    *first_arg = '\0';
    return NULL;
  }

  do {
    skip_spaces(&argument);

    first_arg = begin;

    if (*argument == '\"') {
      argument++;
      while (*argument && *argument != '\"') {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
      argument++;
    } else {
      while (*argument && !isspace(*argument)) {
        *(first_arg++) = LOWER(*argument);
        argument++;
      }
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return argument;
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (*argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return argument;
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return one_argument(one_argument(argument, first_arg), second_arg); /* :-) */
}



/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 *
 * returnss 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return 0;

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return 0;

  if (!*arg1)
    return 1;
  else
    return 0;
}

/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(cmd_info[cmd].command, command))
      return cmd;

  return -1;
}


int special(struct char_data *ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(ch->in_room) != NULL)
    if (GET_ROOM_SPEC(ch->in_room) (ch, world + ch->in_room, cmd, arg))
      return 1;

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
	return 1;

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return 1;

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
    if (GET_MOB_SPEC(k) != NULL)
      if (GET_MOB_SPEC(k) (ch, k, cmd, arg))
	return 1;

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
	return 1;

  return 0;
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */


/* locate entry in p_table with entry->name == name. -1 mrks failed search */
int find_name(char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++) {
    if (!str_cmp((player_table + i)->name, name))
      return i;
  }

  return -1;
}


int _parse_name(char *arg, char *name)
{
  int i;

  /* skip whitespaces */
  for (; isspace(*arg); arg++);

  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return 1;

  if (!i)
    return 1;

  return 0;
}


#define RECON		1
#define USURP		2
#define UNSWITCH	3

int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;

  int id = GET_IDNUM(d->character);

  /*
   * Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number.
   */

  for (k = descriptor_list; k; k = next_k) {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id)) {    /* switched char */
      SEND_TO_Q("\r\nMultiple login detected -- disconnecting.\r\n", k);
      STATE(k) = CON_CLOSE;
      if (!target) {
	target = k->original;
	mode = UNSWITCH;
      }
      if (k->character)
	k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    } else if (k->character && (GET_IDNUM(k->character) == id)) {
      if (!target && STATE(k) == CON_PLAYING) {
	SEND_TO_Q("\r\nThis body has been usurped!\r\n", k);
	target = k->character;
	mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      SEND_TO_Q("\r\nMultiple login detected -- disconnecting.\r\n", k);
      STATE(k) = CON_CLOSE;
    }
  }

 /*
  * now, go through the character list, deleting all characters that
  * are not already marked for deletion from the above step (i.e., in the
  * CON_HANGUP state), and have not already been selected as a target for
  * switching into.  In addition, if we haven't already found a target,
  * choose one if one is available (while still deleting the other
  * duplicates, though theoretically none should be able to exist).
  */

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target) {
      target = ch;
      mode = RECON;
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (ch->in_room != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for swicthing into was found - allow login to continue */
  if (!target)
    return 0;

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT(TMP_FLAGS(d->character), TMP_MAILING | TMP_WRITING | TMP_OLC);
  STATE(d) = CON_PLAYING;

  switch (mode) {
  case RECON:
    SEND_TO_Q("Reconnecting.\r\n", d);
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    mudlogf(NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE,
	"%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  case USURP:
    SEND_TO_Q("You take over your own body, already in use!\r\n", d);
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
	"$n's body has been taken over by a new spirit!",
	TRUE, d->character, 0, 0, TO_ROOM);
    mudlogf(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
	"%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
    break;
  case UNSWITCH:
    SEND_TO_Q("Reconnecting to unswitched char.", d);
    mudlogf(NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE,
	"%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  }

  return 1;
}



/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  char buf[256];
  int i; /* integer for converting names to lower case */
  int player_i, load_result;
  char tmp_name[MAX_INPUT_LENGTH];
  struct char_file_u tmp_store;

  sh_int load_room;

  skip_spaces(&arg);

  if (d->character == NULL) {
    CREATE(d->character, struct char_data, 1);
    clear_char(d->character);
    CREATE(d->character->player_specials, struct player_special_data, 1);
    d->character->desc = d;
  }

  switch (STATE(d)) {

  /*. OLC states .*/
  case CON_OEDIT:
    oedit_parse(d, arg);
    break;
  case CON_REDIT:
    redit_parse(d, arg);
    break;
  case CON_ZEDIT:
    zedit_parse(d, arg);
    break;
  case CON_MEDIT:
    medit_parse(d, arg);
    break;
  case CON_SEDIT:
    sedit_parse(d, arg);
    break;
  /*. End of OLC states .*/

 case CON_QANSI:
    if (!*arg || LOWER(*arg) == 'y') {
      SET_BIT(TMP_FLAGS(d->character), TMP_COLOR_1 | TMP_COLOR_2);
      SEND_TO_Q(ANSIGREET, d);
    } else if (LOWER(*arg) == 'n') {
      REMOVE_BIT(TMP_FLAGS(d->character), TMP_COLOR_1 | TMP_COLOR_2);
      SEND_TO_Q(GREETINGS, d);
    } else {
      SEND_TO_Q("That is not a proper response.\r\n", d);
      SEND_TO_Q(ANSI, d);
      return;
    }


    STATE(d) = CON_GET_NAME;
    break;

  case CON_GET_NAME:          /* wait for input of name */
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
	  strlen(tmp_name) > MAX_NAME_LENGTH ||
	  fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
	SEND_TO_Q("Invalid name, please try another.\r\n"
		  "Name: ", d);
	return;
      }
      if ((player_i = load_char(tmp_name, &tmp_store)) > -1) {

	store_to_char(&tmp_store, d->character);
	GET_PFILEPOS(d->character) = player_i;

	if (PLR_FLAGGED(d->character, PLR_DELETED)) {
	  free_char(d->character);
	  CREATE(d->character, struct char_data, 1);
	  clear_char(d->character);
	  CREATE(d->character->player_specials, struct player_special_data, 1);
	  d->character->desc = d;
	  CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	  strcpy(d->character->player.name, CAP(tmp_name));
	  GET_PFILEPOS(d->character) = player_i;

	  sprintf(buf, "Did I get that right, %s (Y/N)? ", tmp_name);
	  SEND_TO_Q(buf, d);
	  STATE(d) = CON_NAME_CNFRM;
	} else {
	  /* undo it just in case they are set */
	  REMOVE_BIT(TMP_FLAGS(d->character),
		     TMP_WRITING | TMP_OLC | TMP_MAILING | PLR_CRYO);
	  
	  
	  SEND_TO_Q("Password: ", d);
	  echo_off(d);
	  d->idle_tics = 0;
	  STATE(d) = CON_PASSWORD;
	}
      } else {
	/* player unknown -- make new character */

	/* Check for multiple creations of a character. */
	if (!Valid_Name(tmp_name)) {
	  SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
	  return;
	}
	/* fix as requested by Tlion to convert names to lower case except
	   first letter -- Nahaz */
	for (i=0; i<strlen(tmp_name); i++)
	  tmp_name[i] = tolower(tmp_name[i]);
	CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
	strcpy(d->character->player.name, CAP(tmp_name));

	sprintf(buf, "Did I get that right, %s (Y/N)? ", tmp_name);
	SEND_TO_Q(buf, d);
	STATE(d) = CON_NAME_CNFRM;
      }
    }
    break;
  case CON_NAME_CNFRM:		/* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y') {
      if (isbanned(d->host) >= BAN_NEW) {
	mudlogf(NRM, LVL_GRGOD, TRUE, "Request for new char %s denied from [%s] (siteban)",
                GET_NAME(d->character), d->host);
	SEND_TO_Q("Sorry, new characters are not allowed from your site!\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (restrict) {
	SEND_TO_Q("Sorry, new players can't be created at the moment.\r\n", d);
	mudlogf(NRM, LVL_GRGOD, TRUE, "Request for new char %s denied from [%s] (wizlock)",
		GET_NAME(d->character), d->host);
	STATE(d) = CON_CLOSE;
	return;
      }
      SEND_TO_Q("New character.\r\n", d);
      sprintf(buf, "Give me a password for %s: ", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      echo_off(d);
      STATE(d) = CON_NEWPASSWD;
    } else if (*arg == 'n' || *arg == 'N') {
      SEND_TO_Q("Okay, what IS it, then? ", d);
      free(d->character->player.name);
      d->character->player.name = NULL;
      STATE(d) = CON_GET_NAME;
    } else {
      SEND_TO_Q("Please type Yes or No: ", d);
    }
    break;
  case CON_PASSWORD:		/* get pwd for known player      */
    /*
     * To really prevent duping correctly, the player's record should
     * be reloaded from disk at this point (after the password has been
     * typed).  However I'm afraid that trying to load a character over
     * an already loaded character is going to cause some problem down the
     * road that I can't see at the moment.  So to compensate, I'm going to
     * (1) add a 15 or 20-second time limit for entering a password, and (2)
     * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
     */

    echo_on(d);    /* turn echo back on */

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else {
      if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
	mudlogf(BRF, MAX(LVL_GOD, GET_LEVEL(d->character)), TRUE, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
	GET_BAD_PWS(d->character)++;
	save_char(d->character, NOWHERE);
	if (++(d->bad_pws) >= max_bad_pws) {	/* N strikes and you're out. */
	  SEND_TO_Q("Wrong password... disconnecting.\r\n", d);
	  STATE(d) = CON_CLOSE;
	} else {
	  SEND_TO_Q("Wrong password.\r\nPassword: ", d);
	  echo_off(d);
	}
	return;
      }
      load_result = GET_BAD_PWS(d->character);
      GET_BAD_PWS(d->character) = 0;
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
	  !PLR_FLAGGED(d->character, PLR_SITEOK)) {
	SEND_TO_Q("Sorry, this char has not been cleared for login from your site!\r\n", d);
	STATE(d) = CON_CLOSE;
	mudlogf(NRM, LVL_GRGOD, TRUE, "Connection attempt for %s denied from %s",
                GET_NAME(d->character), d->host);
	return;
      }
      if (GET_LEVEL(d->character) < restrict) {
	SEND_TO_Q("The game is temporarily restricted.. try again later.\r\n", d);
	STATE(d) = CON_CLOSE;
	mudlogf(NRM, LVL_GOD, TRUE, "Request for login denied for %s [%s] (wizlock)",
		GET_NAME(d->character), d->host);
	return;
      }
      /* check and make sure no other copies of this player are logged in */
      if (perform_dupe_check(d))
	return;

      if (GET_LEVEL(d->character) >= LVL_IMMORT)
	SEND_TO_Q(imotd, d);
      else
	SEND_TO_Q(motd, d);

      /*
       * we now check to see if the player is married
       * if we don't and the person they are married to has deleted
       * then we are fucked!! - fixed 7/11/97 by Zeus
       */

      if (GET_MARRIED(d->character) != 0)
	if (get_name_by_id(GET_MARRIED(d->character)) == NULL)
	  GET_MARRIED(d->character) = 0;

      if (PLR_FLAGGED(d->character, PLR_INVSTART))
	GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);
      else
	GET_INVIS_LEV(d->character) = 0;

      mudlogf(BRF, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE,
	      "%s [%s] has connected.", GET_NAME(d->character), d->host);
      if (load_result) {
	sprintf(buf, "\r\n\r\n\007\007\007"
		"%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
		CCRED(d->character, C_SPR), load_result,
		(load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
	SEND_TO_Q(buf, d);
	GET_BAD_PWS(d->character) = 0;
      }
      SEND_TO_Q("\r\n\n[PRESS RETURN] ", d);
      STATE(d) = CON_RMOTD;
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 5 ||
	!str_cmp(arg, GET_NAME(d->character))) {
      SEND_TO_Q("\r\nIllegal password.\r\n", d);
      SEND_TO_Q("Password: ", d);
      return;
    }

    strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_NAME(d->character)), MAX_PWD_LENGTH);
    *(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';

    SEND_TO_Q("\r\nPlease retype password: ", d);
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;

    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character),
		MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nPasswords don't match... start over.\r\n", d);
      SEND_TO_Q("Password: ", d);
      if (STATE(d) == CON_CNFPASSWD)
	STATE(d) = CON_NEWPASSWD;
      else
	STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    echo_on(d);

    if (STATE(d) == CON_CNFPASSWD) {
      SEND_TO_Q("Gimme a Sex ([M]ale/[F]emale/[N]o Sex Please)? ", d);
      STATE(d) = CON_QSEX;
    } else {
      save_char(d->character, NOWHERE);
      echo_on(d);
      SEND_TO_Q("\r\nDone.\n\r", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }

    break;

  case CON_QSEX:		/* query sex of new user         */
    switch (*arg) {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    case 'n':
    case 'N':
      d->character->player.sex = SEX_NEUTRAL;
      break;
    default:
      SEND_TO_Q("That is not a sex..\r\n"
		"What IS your sex? ", d);
      return;
      break;
    }

    SEND_TO_Q(PK, d);
    STATE(d) = CON_QPK;
    break;

 case CON_QPK:
    if (!*arg || LOWER(*arg) == 'y') {
      SET_BIT(PLR_FLAGS(d->character), PLR_PK);
      SEND_TO_Q("You can take part in PK with this char.\r\n", d);
    } else if (LOWER(*arg) == 'n') {
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_PK);
      SEND_TO_Q("You will not be allowed to take part in pk.\r\n", d);
    } else {
      SEND_TO_Q("Answer Y or N!\r\n", d);
      SEND_TO_Q(PK, d);
      return;
    }

    SEND_TO_Q(class_menu, d);
    SEND_TO_Q("\r\nClass: ", d);
    STATE(d) = CON_QCLASS;
    break;

  case CON_QCLASS:

    if ((GET_CLASS(d->character) = parse_class(d->character, *arg)) == CLASS_UNDEFINED) {
      SEND_TO_Q("\r\nThat's not a class.\r\nClass: ", d);
      return;
    }

    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_NAME(d->character));
    init_char(d->character);
    save_char(d->character, NOWHERE);
    SEND_TO_Q(motd, d);
    SEND_TO_Q("\r\n\n[PRESS RETURN] ", d);

    mudlogf(NRM, LVL_IMMORT, TRUE, "%s [%s] new player.",
	    GET_NAME(d->character), d->host);

    STATE(d) = CON_RMOTD;
    break;

  case CON_RMOTD:		/* read CR after printing motd   */
    SEND_TO_Q(MENU, d);
    STATE(d) = CON_MENU;
    break;

  case CON_MENU:		/* get selection from main menu  */
    switch (*arg) {
    case '0':
      mudlogf(NRM, MAX(LVL_GOD, GET_INVIS_LEV(d->character)), TRUE,
	      "Losing player %s.", GET_NAME(d->character));
      close_socket(d);
      break;

    case '1':
      reset_char(d->character);

      if ((load_result = Crash_load(d->character)))
   	     d->character->in_room = NOWHERE;
      save_char(d->character, NOWHERE);
      send_to_char(WELC_MESSG, d->character);

      d->character->next = character_list;
      character_list = d->character;

      if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
	load_room = real_room(load_room);

      /* If char was saved with NOWHERE, or real_room above failed... */
      if (load_room == NOWHERE) {
	if (GET_LEVEL(d->character) >= LVL_IMMORT) {
	  load_room = r_immort_start_room;
	} else if (GET_LEVEL(d->character) < 10) {
	  load_room = r_newbie_start_room;
	} else {
	  load_room = r_mortal_start_room[0];
	}
      }

      if (PLR_FLAGGED(d->character, PLR_FROZEN))
	load_room = r_frozen_start_room;

      if (PLR_FLAGGED(d->character, PLR_CRIMINAL))
        load_room = r_jailed_start_room;

      char_to_room(d->character, load_room);

      if (GET_LEVEL(d->character) < LVL_IMMORT) {
	sprintf(buf, "%s has decided to enter this insane place!\r\n",
		GET_NAME(d->character));
	send_to_most(buf);
      }

      STATE(d) = CON_PLAYING;
      if (!GET_LEVEL(d->character)) {
	do_start(d->character);
	send_to_char(START_MESSG, d->character);
	do_newbie(d->character);
      }
      look_at_room(d->character, 0);
      if (has_mail(GET_IDNUM(d->character)))
	send_to_char("You have mail waiting.\r\n", d->character);
      if (load_result == 2) {	/* rented items lost */
	send_to_char("\r\n\007You could not afford your rent!\r\n"
		     "Your possesions have been donated to the Salvation Army!\r\n",
		     d->character);
      }

      d->has_prompt = 0;
      read_aliases(d->character);
      break;

    case '2':
      if (d->character->player.description) {
	SEND_TO_Q("Current description:\r\n", d);
	SEND_TO_Q(d->character->player.description, d);
	/* don't free this now... so that the old description gets loaded */
	/* as the current buffer in the editor */
	/* free(d->character->player.description); */
	/* d->character->player.description = NULL; */
	/* BUT, do setup the ABORT buffer here */
	d->backstr = str_dup(d->character->player.description);
      }
      SEND_TO_Q("Enter the text you'd like others to see when they look at you.\r\n", d);
      SEND_TO_Q("(/s saves /h for help)\r\n", d);
      d->str = &d->character->player.description;
      d->max_str = EXDSCR_LENGTH;
      STATE(d) = CON_EXDESC;
      break;

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      SEND_TO_Q("\r\nEnter your old password: ", d);
      echo_off(d);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      SEND_TO_Q("\r\nEnter your password for verification: ", d);
      echo_off(d);
      STATE(d) = CON_DELCNF1;
      break;

      /* we may need more with this ... I'll sort it later too */

    case '6':
      mudlogf(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
	      "%s [%s] is checking who is on-line.", GET_NAME(d->character), d->host);

      SEND_TO_Q("\r\nThe Following People are Playing Minos Right Now:\r\n",d );
      SEND_TO_Q(     "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\r\n\r\n",d);

      do_who(d->character, "", 0, 0);
      SEND_TO_Q("\r\n\n[PRESS RETURN] ", d);
      STATE(d) = CON_RMOTD;
      break;

    case '7':
      page_string(d, policies, 0);
      SEND_TO_Q("\r\n\n[PRESS RETURN] ", d);
      STATE(d) = CON_RMOTD;
      break;

    default:
      SEND_TO_Q("\r\nThat's not a menu choice!\r\n", d);
      SEND_TO_Q(MENU, d);
      break;
    }

    break;

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      echo_on(d);
      SEND_TO_Q("\r\nIncorrect password.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
      return;
    } else {
      SEND_TO_Q("\r\nEnter a new password: ", d);
      STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    break;

  case CON_DELCNF1:
    echo_on(d);
    if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH)) {
      SEND_TO_Q("\r\nIncorrect password.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    } else {
      SEND_TO_Q("\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
		"ARE YOU ABSOLUTELY SURE?\r\n\r\n"
		"Please type \"yes\" to confirm: ", d);
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES")) {
      if (PLR_FLAGGED(d->character, PLR_FROZEN)) {
	SEND_TO_Q("You try to kill yourself, but the ice stops you.\r\n", d);
	SEND_TO_Q("Character not deleted.\r\n\r\n", d);
	STATE(d) = CON_CLOSE;
	return;
      }
      if (GET_LEVEL(d->character) < LVL_SUPREME)
	SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character, NOWHERE);
      Crash_delete_file(GET_NAME(d->character));
      sprintf(buf, "Character '%s' deleted!\r\n"
	      "Goodbye.\r\n", GET_NAME(d->character));
      SEND_TO_Q(buf, d);
      mudlogf(NRM, LVL_WIZARD, TRUE, "%s (lev %d) has self-deleted.",
	      GET_NAME(d->character), GET_LEVEL(d->character));
      STATE(d) = CON_CLOSE;
      return;
    } else {
      SEND_TO_Q("\r\nCharacter not deleted.\r\n", d);
      SEND_TO_Q(MENU, d);
      STATE(d) = CON_MENU;
    }
    break;

/*  case CON_CLOSE:
    close_socket(d);
    break;
*/
  default:
    log("SYSERR: Nanny: illegal state of con'ness; closing connection.");
    STATE(d) = CON_DISCONNECT; /* Safest to do. */
    break;
  }
}
