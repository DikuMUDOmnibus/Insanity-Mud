/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern struct room_data *world;
extern int top_of_world;
extern int mini_mud;
extern int dts_are_dumps;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern int dts_are_dumps;

/* special procedure list */
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(postmaster);
SPECIAL(guild_guard);
SPECIAL(guild);
SPECIAL(puff);
SPECIAL(angel);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(cleric);
SPECIAL(newbie_guide);
SPECIAL(icewizard);
SPECIAL(marbles);
SPECIAL(eris);
SPECIAL(you);
SPECIAL(linguist);
SPECIAL(engraver);
void assign_kings_castle(void);

/* objs */
SPECIAL(bank);
SPECIAL(gen_board);
SPECIAL(portal);
SPECIAL(coke_machine);
SPECIAL(castleflag);
SPECIAL(syrup);

/* rooms */
SPECIAL(dump);
SPECIAL(pet_shops);


/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname))
{
  int rnum;
 
  if ((rnum = real_mobile(mob)) >= 0)
    mob_index[rnum].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant mob #%d",
	    mob);
    log(buf);
  }
}

void ASSIGNOBJ(int obj, SPECIAL(fname))
{
  if (real_object(obj) >= 0)
    obj_index[real_object(obj)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant obj #%d",
	    obj);
    log(buf);
  }
}

void ASSIGNROOM(int room, SPECIAL(fname))
{
  if (real_room(room) >= 0)
    world[real_room(room)].func = fname;
  else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant rm. #%d",
	    room);
    log(buf);
  }
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */

void assign_mobiles(void)
{
  
  /* Immortal Zone */
  ASSIGNMOB(1201, postmaster);
  ASSIGNMOB(1202, janitor);

  /* Wastelands */
  ASSIGNMOB(9002, snake);
  ASSIGNMOB(9012, magic_user);
  ASSIGNMOB(9011, icewizard);

  /* Assigns for the Town of Sundhaven  */
  ASSIGNMOB(6601, cityguard);
  ASSIGNMOB(6664, postmaster);
  ASSIGNMOB(6656, guild_guard);
  ASSIGNMOB(6655, guild_guard); 
  ASSIGNMOB(6658, guild_guard);
  ASSIGNMOB(6657, guild_guard);
  ASSIGNMOB(6667, receptionist);
  ASSIGNMOB(6606, fido);             /* Smoke rat */
  ASSIGNMOB(6616, guild);
  ASSIGNMOB(6619, guild);
  ASSIGNMOB(6617, guild);  
  ASSIGNMOB(6618, guild);
  ASSIGNMOB(6659, cityguard);
  ASSIGNMOB(6660, cityguard);    
  ASSIGNMOB(6607, thief);
  ASSIGNOBJ(6647, marbles);
  ASSIGNOBJ(6709, marbles);

  /* Kerofk */

  ASSIGNMOB(702, guild);
  ASSIGNMOB(704, guild);
  ASSIGNMOB(727, guild);
  ASSIGNMOB(729, guild);
  ASSIGNMOB(714, receptionist);
  ASSIGNMOB(753, receptionist);
  ASSIGNMOB(730, thief);
  ASSIGNMOB(733, cityguard);
  ASSIGNMOB(735, cityguard);



  /* Elven Zone */
  
  ASSIGNMOB(19009, receptionist);

  /* New Sparta */

  ASSIGNMOB(21003, cityguard);
  ASSIGNMOB(21004, cityguard);
  ASSIGNMOB(21005, thief);
  ASSIGNMOB(21042, cityguard);
  ASSIGNMOB(21054, cityguard);
  ASSIGNMOB(21059, cityguard);
  ASSIGNMOB(21060, cityguard);
  ASSIGNMOB(21062, thief);
  ASSIGNMOB(21084, janitor);
  ASSIGNMOB(21085, fido);
  ASSIGNMOB(21072, guild); 
  ASSIGNMOB(21073, guild); 
  ASSIGNMOB(21074, guild); 
  ASSIGNMOB(21075, guild);
  ASSIGNMOB(21068, guild_guard);
  ASSIGNMOB(21069, guild_guard);
  ASSIGNMOB(21070, guild_guard);
  ASSIGNMOB(21071, guild_guard);
  ASSIGNMOB(21078, receptionist);


  /* Minos */
  ASSIGNMOB(4512, linguist);
  ASSIGNMOB(4505, receptionist);
  ASSIGNMOB(4510, postmaster);
  ASSIGNMOB(4520, guild);
  ASSIGNMOB(4521, guild);
  ASSIGNMOB(4522, guild);
  ASSIGNMOB(4523, guild);
  ASSIGNMOB(4530, guild);
  ASSIGNMOB(4547, guild);
  ASSIGNMOB(4588, guild);
  ASSIGNMOB(4524, guild_guard);
  ASSIGNMOB(4548, guild_guard);
  ASSIGNMOB(4525, guild_guard);
  ASSIGNMOB(4526, guild_guard);
  ASSIGNMOB(4527, guild_guard);
  ASSIGNMOB(4597, guild_guard);
  ASSIGNMOB(4529, guild_guard);
  ASSIGNMOB(4528, cleric);
  ASSIGNMOB(4559, cityguard);
  ASSIGNMOB(4560, cityguard);
  ASSIGNMOB(4561, janitor);
  ASSIGNMOB(4562, fido);
  ASSIGNMOB(4566, fido);
  ASSIGNMOB(4567, cityguard);
  ASSIGNMOB(4568, janitor);
  ASSIGNMOB(4595, cryogenicist);
  ASSIGNMOB(4596, newbie_guide);
  ASSIGNMOB(4511, angel);
  ASSIGNMOB(4513, engraver);
  
  /* MORIA */
  ASSIGNMOB(4000, snake);
  ASSIGNMOB(4001, snake);
  ASSIGNMOB(4053, snake);
  ASSIGNMOB(4100, magic_user);
  ASSIGNMOB(4102, snake);
  ASSIGNMOB(4103, thief);

  /* Redferne's */
  ASSIGNMOB(7900, cityguard);

  /* PYRAMID */
  ASSIGNMOB(5300, snake);
  ASSIGNMOB(5301, snake);
  ASSIGNMOB(5304, thief);
  ASSIGNMOB(5305, thief);
  ASSIGNMOB(5309, magic_user); /* should breath fire */
  ASSIGNMOB(5311, magic_user);
  ASSIGNMOB(5313, magic_user); /* should be a cleric */
  ASSIGNMOB(5314, magic_user); /* should be a cleric */
  ASSIGNMOB(5315, magic_user); /* should be a cleric */
  ASSIGNMOB(5316, magic_user); /* should be a cleric */
  ASSIGNMOB(5317, magic_user);

  /* High Tower Of Sorcery */
  ASSIGNMOB(2501, magic_user); /* should likely be cleric */
  ASSIGNMOB(2504, magic_user);
  ASSIGNMOB(2507, magic_user);
  ASSIGNMOB(2508, magic_user);
  ASSIGNMOB(2510, magic_user);
  ASSIGNMOB(2511, thief);
  ASSIGNMOB(2514, magic_user);
  ASSIGNMOB(2515, magic_user);
  ASSIGNMOB(2516, magic_user);
  ASSIGNMOB(2517, magic_user);
  ASSIGNMOB(2518, magic_user);
  ASSIGNMOB(2520, magic_user);
  ASSIGNMOB(2521, magic_user);
  ASSIGNMOB(2522, magic_user);
  ASSIGNMOB(2523, magic_user);
  ASSIGNMOB(2524, magic_user);
  ASSIGNMOB(2525, magic_user);
  ASSIGNMOB(2526, magic_user);
  ASSIGNMOB(2527, magic_user);
  ASSIGNMOB(2528, magic_user);
  ASSIGNMOB(2529, magic_user);
  ASSIGNMOB(2530, magic_user);
  ASSIGNMOB(2531, magic_user);
  ASSIGNMOB(2532, magic_user);
  ASSIGNMOB(2533, magic_user);
  ASSIGNMOB(2534, magic_user);
  ASSIGNMOB(2536, magic_user);
  ASSIGNMOB(2537, magic_user);
  ASSIGNMOB(2538, magic_user);
  ASSIGNMOB(2540, magic_user);
  ASSIGNMOB(2541, magic_user);
  ASSIGNMOB(2548, magic_user);
  ASSIGNMOB(2549, magic_user);
  ASSIGNMOB(2552, magic_user);
  ASSIGNMOB(2553, magic_user);
  ASSIGNMOB(2554, magic_user);
  ASSIGNMOB(2556, magic_user);
  ASSIGNMOB(2557, magic_user);
  ASSIGNMOB(2559, magic_user);
  ASSIGNMOB(2560, magic_user);
  ASSIGNMOB(2562, magic_user);
  ASSIGNMOB(2564, magic_user);

  /* SEWERS */
  ASSIGNMOB(7006, snake);
  ASSIGNMOB(7009, magic_user);
  ASSIGNMOB(7200, magic_user);
  ASSIGNMOB(7201, magic_user);
  ASSIGNMOB(7202, magic_user);

  /* FOREST */
  ASSIGNMOB(6113, snake);
  ASSIGNMOB(6114, magic_user);
  ASSIGNMOB(6115, magic_user);
  ASSIGNMOB(6116, magic_user); /* should be a cleric */
  ASSIGNMOB(6117, magic_user);

  /* ARACHNOS */
  ASSIGNMOB(6309, magic_user);
  ASSIGNMOB(6312, magic_user);
  ASSIGNMOB(6314, magic_user);
  ASSIGNMOB(6315, magic_user);

  /* Desert */
  ASSIGNMOB(5004, magic_user);
  ASSIGNMOB(5010, magic_user);
  ASSIGNMOB(5014, magic_user);

  /* Drow City */
  ASSIGNMOB(5103, magic_user);
  ASSIGNMOB(5104, magic_user);
  ASSIGNMOB(5107, magic_user);
  ASSIGNMOB(5108, magic_user);

  /* Old Thalos */
  ASSIGNMOB(5200, magic_user);
  ASSIGNMOB(5201, magic_user);
  ASSIGNMOB(5209, magic_user);

  /* Ofingia and Goblin towm */

  ASSIGNMOB(8122, fido);
  ASSIGNMOB(8117, cityguard);
  ASSIGNMOB(8115, cityguard);
  ASSIGNMOB(8172, magic_user);
  ASSIGNMOB(8116, cityguard);
  ASSIGNMOB(8112, thief);
  ASSIGNMOB(8136, cityguard);
  ASSIGNMOB(8133, snake);
  ASSIGNMOB(8144, snake);
  ASSIGNMOB(8123, fido);
  ASSIGNMOB(8128, janitor);
  ASSIGNMOB(8113, cityguard);
  ASSIGNMOB(8110, cityguard);
  ASSIGNMOB(8111, magic_user);


  /* New Thalos */
  ASSIGNMOB(5481, magic_user);
  ASSIGNMOB(5404, receptionist);
  ASSIGNMOB(5421, magic_user);
  ASSIGNMOB(5422, magic_user);
  ASSIGNMOB(5423, magic_user);
  ASSIGNMOB(5424, magic_user);
  ASSIGNMOB(5425, magic_user);
  ASSIGNMOB(5426, magic_user);
  ASSIGNMOB(5427, magic_user);
  ASSIGNMOB(5428, magic_user);
  ASSIGNMOB(5434, cityguard);
  ASSIGNMOB(5440, magic_user);
  ASSIGNMOB(5455, magic_user);
  ASSIGNMOB(5461, cityguard);
  ASSIGNMOB(5462, cityguard);
  ASSIGNMOB(5463, cityguard);
  ASSIGNMOB(5482, cityguard);
  ASSIGNMOB(5400, guild);
  ASSIGNMOB(5401, guild);
  ASSIGNMOB(5402, guild);
  ASSIGNMOB(5403, guild);
  ASSIGNMOB(5456, guild_guard);
  ASSIGNMOB(5457, guild_guard);
  ASSIGNMOB(5458, guild_guard);
  ASSIGNMOB(5459, guild_guard);

  /* ROME */
  ASSIGNMOB(12009, magic_user);
  ASSIGNMOB(12018, cityguard);
  ASSIGNMOB(12020, magic_user);
  ASSIGNMOB(12021, cityguard);
  ASSIGNMOB(12025, magic_user);
  ASSIGNMOB(12030, magic_user);
  ASSIGNMOB(12031, magic_user);
  ASSIGNMOB(12032, magic_user);

  /* King Welmar's Castle (not covered in castle.c) */
  ASSIGNMOB(15015, thief);      /* Ergan... have a better idea? */
  ASSIGNMOB(15032, magic_user); /* Pit Fiend, have something better?  Use it */

  /* DWARVEN KINGDOM */
  ASSIGNMOB(6500, cityguard);
  ASSIGNMOB(6502, magic_user);
  ASSIGNMOB(6509, magic_user);
  ASSIGNMOB(6516, magic_user);

  /* Vorpalbunnies Zone */
  ASSIGNMOB(12300, eris);
  ASSIGNMOB(12307, you);

}



/* assign special procedures to objects */
void assign_objects(void)
{
  ASSIGNOBJ(4589, gen_board);	/* Witch  board */
  ASSIGNOBJ(4590, gen_board);	/* Avatar board */
  ASSIGNOBJ(4591, gen_board);	/* Sr.God board */
  ASSIGNOBJ(4592, gen_board);	/* magic  board */
  ASSIGNOBJ(4593, gen_board);	/* thief  board */  
  ASSIGNOBJ(4594, gen_board);	/* warrio board */
  ASSIGNOBJ(4595, gen_board);	/* druid  board */
  ASSIGNOBJ(4596, gen_board);	/* cleric board */
  ASSIGNOBJ(4597, gen_board);	/* freeze board */
  ASSIGNOBJ(4598, gen_board);	/* immortal board */
  ASSIGNOBJ(4599, gen_board);	/* mortal board */
  ASSIGNOBJ(4584, gen_board);	/* quest board */
  ASSIGNOBJ(4583, gen_board);	/* newbie board */
  ASSIGNOBJ(4582, gen_board);	/* clan board */
  ASSIGNOBJ(4579, gen_board);	/* news board */
  ASSIGNOBJ(4565, gen_board);  /* olies board */
  ASSIGNOBJ(4547, gen_board);   /* immortal code board */
  ASSIGNOBJ(402, gen_board);   /* pagan cult board */
  ASSIGNOBJ(4572, gen_board);   /* temple info board */
  ASSIGNOBJ(10298, gen_board); /* miri's first DS9 board */
  ASSIGNOBJ(10396, gen_board); /* miri's second DS9 board */

  ASSIGNOBJ(4534, bank);	/* atm */
  ASSIGNOBJ(6612, bank);        /* Sundhaven Pile of Treasure */
  ASSIGNOBJ(4536, bank);	/* cashcard */
  ASSIGNOBJ(4562, portal);     /* portal spell obj */
  ASSIGNOBJ(3, coke_machine);  /* the drinks machine */

  /* Syrup */
  ASSIGNOBJ(1220, syrup);
  
  /* castle Flags */

  ASSIGNOBJ(699, castleflag);      /* Castle 6 */
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  int i;

  ASSIGNROOM(4583, dump);
  ASSIGNROOM(4571, pet_shops);
  ASSIGNROOM(6655, pet_shops);
  ASSIGNROOM(4589, pet_shops);
  ASSIGNROOM(5547, dump);
  ASSIGNROOM(6625, dump);
  ASSIGNROOM(6626, dump);
  ASSIGNROOM(21108, dump);
  ASSIGNROOM(21116 , pet_shops);

  if (dts_are_dumps)
    for (i = 0; i < top_of_world; i++)
      if (IS_SET(ROOM_FLAGS(i), ROOM_DEATH))
	world[i].func = dump;
}


