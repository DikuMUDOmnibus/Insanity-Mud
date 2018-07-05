/* ************************************************************************
*  file:  showadd.c                                   Part of CircleMud   *
*  Usage: list a diku playerfile and prints out hosts of your users       *
*  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
*  All Rights Reserved                                                    *
************************************************************************* */

#include "../conf.h"
#include "../sysdep.h"

#include "../structs.h"

void show(char *filename)
{
  FILE *fl;
  struct char_file_u player;
/*  struct descriptor_data d; */
  int num = 0;

  if (!(fl = fopen(filename, "r+"))) {
    perror("error opening playerfile");
    exit(1);
  }
  for (;;) {
    fread(&player, sizeof(struct char_file_u), 1, fl);
    if (feof(fl)) {
      fclose(fl);
      exit(1);
    }
    printf("%5d. ID: %5ld %-20s %-40s\n", ++num,
	   player.char_specials_saved.idnum, player.name,
	   player.host);
  }
}


int main(int argc, char **argv)
{
  if (argc != 2)
    printf("Usage: %s playerfile-name\n", argv[0]);
  else
    show(argv[1]);

  return 0;
}
