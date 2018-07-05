#define MAX_CLANS	100
#define MAX_MEMBERS     100
#define MAX_CASTLES     10
#define CLAN_COST	500000
#define CLAN_LEVEL	20

struct clan_control_rec {
   sh_int num;			/* number of this clan		*/
   long leader;			/* idnum of clan leader  	*/
   int num_of_members;		/* how many members for clan	*/
   int room;                    /* clan donation room vnum      */
   long members[MAX_MEMBERS];	/* idnums of clan memebers 	*/
   char clanname[128];          /* clan name                   */
   int  castlezone;              /* does the clan have a castle  */
   int  clan_points;		/* Clan points of the clan */
   int  spare0;	
   int spare1;
};



   
#define TOROOM(room, dir) (world[room].dir_option[dir] ? \
			    world[room].dir_option[dir]->to_room : NOWHERE)

void	clan_boot(void);
int	clan_can_enter(struct char_data *ch, sh_int room);
int     is_in_clan( struct char_data *ch );
int     clan_can_enter_castle(struct char_data *ch, int room);
int     clan_can_kill(struct char_data *ch, struct char_data *vict);
void clan_save_control(void);
