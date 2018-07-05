/* *********************************************************************** *
 *   File:  mob_fix.c                                    Part of CircleMUD *
 *  Usage:  equalizing the mob files                                       *
 *  Writer: Angus Mezick, parts taken from db.c                            *
 * *********************************************************************** */

/*
 * here is some code for a mob file equalizer.   Since we spend a lot of
 * time porting in zones, we noticed that some zones were really powerful
 * and some were weak.  this mainly had to do with mobs.  This program
 * takes in a .mob file, and then equalizes some of the stats based on
 * level, writing out to another file.  A lot of this code is taken from
 * db.c, so you should recognize it :)  I just don't want anyone making
 * money with it :).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int stats[][9]=
{
/*lev hp        exp thaco  ac   #d#+dam     gold*/
  {0,  10,        50,  20,  10,  1,3,0,       100}, 
  {1,  15,       100,  19,  10,  1,4,0,       200}, 
  {2,  25,       400,  18,  10,  1,5,0,       300}, 
  {3,  40,       500,  17,   9,  1,6,0,       400},
  {4,  65,       600,  16,   9,  1,8,0,       500}, 
  {5,  75,       700,  15,   9,  2,8,0,       600}, 
  {6,  85,       800,  14,   8,  2,8,1,       700},
  {7,  95,       900,  13,   8,  2,8,2,       800},
  {8,  130,     1000,  12,   8,  2,8,3,       900},
  {9,  140,     2000,  11,   7,  2,8,4,       1000},
  {10, 150,     3000,  10,   7,  2,8,5,       1100},
  {11, 170,     4000,   9,   7,  3,8,6,       1200},
  {12, 200,     5000,   8,   6,  3,8,7,       1300},
  {13, 250,     6000,   7,   6,  3,8,8,       1400},
  {14, 300,     7000,   6,   6,  3,8,9,       1500},
  {15, 400,     8000,   5,   5,  3,8,10,      1600},
  {16, 450,     9000,   4,   5,  4,8,11,      1700},
  {17, 460,    10000,   3,   5,  4,8,12,      1800},
  {18, 470,    20000,   2,   4,  4,8,13,      1900},
  {19, 480,    30000,   1,   4,  4,8,14,      2000},
  {20, 500,    40000,   0,   4,  4,8,15,      5000},
  {21, 530,    50000,  -1,   3,  5,8,16,      10000},
  {22, 560,    60000,  -2,   3,  5,8,17,      15000},
  {23, 590,    70000,  -3,   3,  5,8,18,      20000},
  {24, 600,    80000,  -4,   2,  5,8,19,      25000},
  {25, 630,    90000,  -5,   2,  5,8,20,      30000},
  {26, 660,   100000,  -6,   2,  6,8,21,      35000},
  {27, 690,   110000,  -7,   1,  6,8,22,      40000},
  {28, 700,   120000,  -8,   1,  6,8,23,      45000},
  {29, 800,   150000,  -9,   1,  6,8,24,      50000},
  {30, 900,   200000, -10,   0,  6,8,25,      55000},
  {31, 1000,  250000, -11,  -1,  7,8,26,      60000},
  {32, 1200,  300000, -12,  -1,  7,8,27,      70000},
  {33, 1400,  350000, -13,  -2,  7,8,28,      75000},
  {34, 1600,  400000, -14,  -2,  7,8,29,      80000},
  {35, 1800,  450000, -15,  -3,  7,8,30,      85000},
  {36, 2000,  500000, -16,  -3,  8,8,30,      90000},
  {37, 2200,  550000, -17,  -4,  8,8,30,      100000},
  {38, 2400,  600000, -18,  -4,  8,8,30,      120000},
  {39, 2600,  650000, -19,  -5,  8,8,30,      130000},
  {40, 2800,  700000, -20,  -5,  8,8,30,      140000},
  {41, 3000,  710000, -20,  -6,  9,8,30,      150000},
  {42, 3200,  720000, -20,  -6,  9,8,30,      160000},
  {43, 3400,  730000, -20,  -7,  9,8,30,      170000},
  {44, 3600,  740000, -20,  -7,  9,8,30,      180000},
  {45, 3800,  750000, -20,  -8,  9,8,35,      190000},
  {46, 4000,  760000, -20,  -8,  10,8,36,     200000},
  {47, 4500,  770000, -20,  -9,  10,8,37,     220000},
  {48, 5000,  780000, -20,  -9,  10,8,38,     230000},
  {49, 5500,  790000, -20, -10,  10,8,39,     240000},
  {50, 6000,  800000, -20, -10,  10,8,40,     250000},
  {51, 12000, 810000, -20, -10,  11,8,41,     260000},
  {52, 14000, 820000, -20, -10,  11,8,42,     270000},
  {53, 16000, 830000, -20, -10,  11,8,43,     280000},
  {54, 18000, 840000, -20, -10,  11,8,44,     290000},
  {55, 20000, 850000, -20, -10,  11,8,45,     300000},
  {56, 22000, 860000, -20, -10,  12,12,46,    310000},
  {57, 24000, 870000, -20, -10,  12,12,47,    320000},
  {58, 26000, 880000, -20, -10,  12,12,48,    330000},
  {59, 28000, 890000, -20, -10,  12,12,49,    340000},
  {60, 30000, 900000, -20, -10,  12,12,50,    350000}
};


#define MAX_STRING_LENGTH 8192
#define READ_LINE  get_line(input_file,output_file,line)
#define HP(lev)  stats[(lev)][1]
#define EXP(lev) stats[(lev)][2]
#define THACO(lev) stats[(lev)][3]
#define AC(lev) stats[(lev)][4]
#define NUM_DICE(lev) stats[(lev)][5]
#define TYPE_DICE(lev) stats[(lev)][6]
#define DAMAGE(lev) stats[(lev)][7]
#define GOLD(lev) stats[(lev)][8]
#define CREATE(result, type, number)  do {\
        if (!((result) = (type *) calloc ((number), sizeof(type))))\
	 { perror("malloc failure");abort();}}while(0)
#define LOWER(c) (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))


int get_line(FILE *input_file, FILE *output_file, char *buf)
{
  char temp[256];
  int lines = 0;
  do
  {
    lines++;
    fgets(temp, 256, input_file);
    if(*temp=='*')
      fprintf(output_file,temp);
    else if (*temp)
      temp[strlen(temp) - 1] = '\0';
    else if(!*temp)
      fprintf(output_file,temp);
  }
  while (!feof(input_file)&&((*temp=='*')||(!*temp)));
  
  if (feof(input_file))
    return 0;
  else
  {
    strcpy(buf, temp);
    return lines;
  }
}


char *asciiflag_conv(char *flag)
{
  unsigned long flags = 0;
  int is_number = 1;
  char *p='\0';
  char flag_list[70];
  int i;
  char temp[40];
  char hold='\0';

  bzero(flag_list,70);
  bzero(temp,40);
  
  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
    {
      flags |= 1 << (26 + (*p - 'A'));
      }
    if (!isdigit(*p))
      is_number = 0;
  }
  
  if (is_number)
    flags = atol(flag);
  
  if(flags>0)
  {
    for(i=31;i>=0;i--)
    {
      if(i>25)
      {
	if(flags>=(1<<i))
	{
	  hold = 'A'+(i-26);
	  sprintf(temp,"%c",hold);
	  strcat(flag_list,temp);
	  flags = flags-(1<<i);
	}
      }
      else if(flags>=(1<<i))
      {
	hold = 'a'+(i);
	sprintf(temp,"%c",hold);
	strcat(flag_list,temp);
	flags = flags-(1<<i);
      }
    }
    flag = flag_list;
  }
  return flag;
}

char fread_letter(FILE *input_file)
{
  char c;
  do
  {
    c=getc(input_file);
  }
  while(isspace(c));
  return c;
}


/* read and allocate space for a '~'-terminated string from a given file */
void  fread_string(FILE *fl,FILE *output_file, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[500];
  register char        *point;
  int  flag;
  
  bzero(buf, MAX_STRING_LENGTH);
  
  do {
    if (!fgets(tmp, MAX_STRING_LENGTH, fl)) {
      fprintf(stderr, "fread_string: format error at or near %s\n", error);
      exit(0);
    }
    
    fprintf(output_file,tmp);
    
    for (point = tmp + strlen(tmp) - 2; point >= tmp && isspace(*point);
	 point--)
      ;
    flag = (*point == '~');
  } while (!flag);
  
}

char    *fname(char *namelist)
{
  static char  holder[30];
  register char        *point;
  
  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;
  
  *point = '\0';
  
  return(holder);
}

void parse_simple_mob(FILE *input_file, FILE *output_file, int nr)
{
  int t[10],level;
  char line[256];

  READ_LINE;
  if(sscanf(line,"%d %d %d %dd%d+%d %dd%d+%d\n",
	    t,t+1,t+2,t+3,t+4,t+5,t+6,t+7,t+8)!=9)
   {
     printf("Format error in mob #%d, first line after S flag\n ... expecting line of form '# # # #d#+# #d#+#'\n",nr);
     exit(1);
   }
  level=t[0];
  fprintf(output_file,"%d %d %d %dd%d+%d %dd%d+%d\n",
	  t[0],stats[level][3], stats[level][4],stats[level][1],t[4],t[5],
	  stats[level][5],stats[level][6],stats[level][7]);
  READ_LINE;
  sscanf(line," %d %d ",t,t+1);
  fprintf(output_file,"%d %d\n",stats[level][8],stats[level][2]);
  READ_LINE;
  if (sscanf(line," %d %d %d ", t, t+1,t+2)!=3) 
  {
    printf("Format error in mob #%d, second line after S flag\n"
	   "...expecting line of form '# # #'\n",nr);
    exit(1);
  }
  fprintf(output_file,"%s\n",line);
}


void parse_enhanced_mob(FILE *input_file, FILE *output_file, int nr)
{
  char line[256];
  parse_simple_mob(input_file, output_file, nr);
  while(READ_LINE)
  {
    if(!strcmp(line,"E"))/*end of enhanced section*/
    {
      fprintf(output_file,"E\n");
      return;
    }
    else if (*line =='#')
    {
      printf("Unterminated E section in mob #%d\n",nr);
      exit(1);
    }
    else
      fprintf(output_file,line);
  }
  printf("Unexpected end of file reached after mob #%d\n",nr);
  exit(1);
}

void parse_mob(FILE *input_file, FILE *output_file, int nr)
{
  char line[256];
  char letter;
  int t[10];
  static int i = 0;
  char f1[128],f2[128];
  char buf2[MAX_STRING_LENGTH];
  char *f1p, *f2p;
  
  /** string data **/
  /*get the name*/
  fread_string(input_file,output_file,buf2);
  
  /*do the short desc*/
  fread_string(input_file,output_file,buf2);
    
  /*long_desc*/
  fread_string(input_file,output_file,buf2);
    
  /*description*/
  fread_string(input_file,output_file,buf2);
  
  /** numeric data **/
  READ_LINE;
  sscanf(line,"%s %s %d %c",f1,f2,t+2,&letter);
  f1p=asciiflag_conv(f1);
  strcpy(f1,f1p);
  f2p=asciiflag_conv(f2);
  strcpy(f2,f2p);
  sprintf(line,"%s %s %d %c\n",f1,f2,t[2],letter);
  fprintf(output_file,line);
  
  switch(letter)
  {
   case 'S':
     parse_simple_mob(input_file, output_file,nr);
     break;
   case 'E':
     parse_enhanced_mob(input_file,output_file,nr);
     break;
   default:
     printf("Unsupported mob type '%c' in mob #%d\n",letter,nr);
     exit(1);
     break;
  }
  
  letter=fread_letter(input_file);
  if(letter=='>')
  {
    ungetc(letter,input_file);
    do
    {
      READ_LINE;
      fprintf(output_file,line);
    }
    while(*line!='|');
    fprintf(output_file,"|\n");
  }
  else
    ungetc(letter,input_file);
  
  i++;
}




void main(int arg_count, char **arg_vector)
{
  FILE *input_file=0;/*input file*/
  FILE *output_file=0;/*the changed file*/
  int nr=-1;
  int last=0;
  char line[256];
  int end=1;
  char file_name[256];
  
  if(arg_count!=3)
  {
    printf("USAGE:  %s file_to_be_changed.mob new_file.mob\n",arg_vector[0]);
    exit(0);
  }
  
  sprintf(file_name,"./%s",arg_vector[1]);
  if(!(input_file=fopen(file_name,"r")))
  {
    printf("%s is not an existing file.\n",arg_vector[1]);
    exit(0);
  }
  
  sprintf(file_name,"./%s",arg_vector[2]);
  if(!(output_file=fopen(file_name,"w")))
  {
    printf("Could not open the output file: %s\n",arg_vector[2]);
    exit(0);
  }
  
  while(end)
  {
    if(!READ_LINE)
    {
      printf("Format error after mob #%d.(get_line)\n",nr);
      exit(1);
    }
    
    if(*line=='$')
    {
      fprintf(output_file,"$~\n");
      end=0;
      continue;
    }
    
    if(*line=='#')
    {
      last=nr;
      if(sscanf(line,"#%d",&nr)!=1)
      {
	printf("Format error after mob #%d (no number after #)\n",last);
	exit(1);
      }
      fprintf(output_file,"#%d\n",nr);
      if(nr>=99999)
      {
	fprintf(output_file,"$~\n");
	end=0;
	continue;
      }
      else
	parse_mob(input_file,output_file,nr);
    }
    else
    {
      printf("Format error in mob near #%d. \n",nr);
      printf("Offending line: '%s'\n",line);
    }
  }
  fclose(input_file);
  fclose(output_file);
}




