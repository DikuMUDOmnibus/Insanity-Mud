#include "conf.h"
#include "sysdep.h"

#include <stdarg.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "screen.h"

extern struct descriptor_data *descriptor_list;

void mudlogf(char type, int level, byte file, const char *format, ...)
{
  va_list args;
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));
  struct descriptor_data *i;
  char *mudlog_buf = malloc(MAX_STRING_LENGTH);
  char tp;

  *(time_s + strlen(time_s) - 1) = '\0';

  if (file)
    fprintf(stderr, "%-15.15s :: ", time_s + 4);

  va_start(args, format);
  if (file)
    vfprintf(stderr, format, args);
  if (level >= 0)
    vsprintf(mudlog_buf, format, args);
  va_end(args);

  if (file)
    fprintf(stderr, "\r\n");

  if (level < 0)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && 
	!TMP_FLAGGED(i->character, TMP_WRITING) && 
	!TMP_FLAGGED(i->character, TMP_OLC)) {

      tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
            (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));

      if ((GET_LEVEL(i->character) >= level) && (tp >= type)) {
        send_to_char(CCRED(i->character, C_NRM), i->character);
	send_to_char("[ ", i->character);
        send_to_char(mudlog_buf, i->character);
	send_to_char(" ]\r\n", i->character);
        send_to_char(CCNRM(i->character, C_NRM), i->character);
      }
    }
  free(mudlog_buf);
}

void log(const char *format, ...)
{
  va_list args;
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  *(time_s + strlen(time_s) - 1) = '\0';

  fprintf(stderr, "%-15.15s :: ", time_s + 4);

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);

  fprintf(stderr, "\r\n");
}
