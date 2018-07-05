/* Convert player files (char_file_u)
 * dkoepke@california.com
 */
#include "../conf.h"
#include "../sysdep.h"
#include "../structs.h"
#include "../utils.h"

typedef struct old_char_file_u {
 
    /* char_player_data */
   char	name[MAX_NAME_LENGTH+1];
   char	description[EXDSCR_LENGTH];
   char	title[MAX_TITLE_LENGTH+1];
   byte sex;
   byte class;
   byte race;
   byte level;
   sh_int hometown;
   time_t birth;   /* Time of birth of character     */
   int	played;    /* Number of secs played in total */
   ubyte weight;
   ubyte height;

   char	pwd[MAX_PWD_LENGTH+1];    /* character's password */

   struct char_special_data_saved char_specials_saved;
   struct player_special_data_saved player_specials_saved;
   struct char_ability_data abilities;
   struct char_point_data points;
   struct affected_type affected[MAX_AFFECT];

   time_t last_logon;		/* Time (in secs) of last logon */
   char host[HOST_LENGTH+1];	/* host of last logon */
   char prompt[MAX_INPUT_LENGTH+1];

} OldData;

typedef struct char_file_u NewData;

NewData ConvertData(OldData old) {
  NewData new;
  int i;
  
  memmove((void *) &new, (const void *) &old, sizeof(OldData));
  return (new);
}

void main(int argc, char **argv) {
  int recs, size = 0;
  FILE *fp, *nfp;
  char fn[128];
  OldData old;
  NewData new;
  
  if (argc != 2) {
    printf("convplr <playerfile>\n");
    exit(1);
  }
  
  sprintf(fn, "%s.new", argv[1]);
  
  if (!(fp = fopen(argv[1], "rb"))) {
    perror(argv[1]);
    exit(1);
  } else if (!(nfp = fopen(fn, "wb"))) {
    perror(fn);
    exit(1);
  }
  
  /* get number of records */
  fseek(fp, 0L, SEEK_END); /* go to the end of the file */
  size = ftell(fp); /* tell us where we are (at the end, so file size) */
  recs = size / sizeof(OldData);
  rewind(fp);
  
  if (size % sizeof(OldData)) {
    printf("Bad record count for old player file.\n");
    exit(1);
  }
  
  for (size=0; size<recs; size++) {
    fread(&old, sizeof(OldData), 1, fp);
    new = ConvertData(old);
    fwrite(&new, sizeof(NewData), 1, nfp);
  }
  
  fclose(fp);
  fclose(nfp);
  printf("Done.\n");
}       

