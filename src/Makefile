# Generated automatically from Makefile.in by configure.
# CircleMUD 3.0 Makefile.in - Makefile template used by 'configure'
#

# C compiler to use
CC = gcc

# Any special flags you want to pass to the compiler
MYFLAGS = -Wall

#flags for profiling (see hacker.doc for more information)
PROFILE = 

##############################################################################
# Do Not Modify Anything Below This Line (unless you know what you're doing) #
##############################################################################

CFLAGS = -g -O2 $(MYFLAGS) $(PROFILE)

OBJFILES = act.comm.o act.create.o act.informative.o act.movement.o \
	act.item.o act.offensive.o act.other.o act.social.o act.wizard.o \
	alias.o arena.o auction.o ban.o boards.o clan.o class.o config.o color.o \
	constants.o comm.o db.o fight.o graph.o handler.o house.o interpreter.o \
	limits.o log.o magic.o mail.o medit.o mobact.o modify.o objsave.o oedit.o \
	olc.o quest.o redit.o sedit.o shop.o spec_assign.o spec_procs.o spell_parser.o \
	spells.o utils.o weather.o zedit.o

default: .accepted
	$(MAKE) ../bin/circle

.accepted:
	@./licheck more

utils: .accepted
	$(MAKE) ../bin/autowiz
	$(MAKE) ../bin/autohero
	$(MAKE) ../bin/delobjs
	$(MAKE) ../bin/listrent
	$(MAKE) ../bin/mudpasswd
	$(MAKE) ../bin/purgeplay

admin: .accepted
	$(MAKE) ../bin/sign
	$(MAKE) ../bin/showplay
	$(MAKE) ../bin/showadd

all: .accepted
	$(MAKE) ../bin/circle
	$(MAKE) utils

circle:
	$(MAKE) ../bin/circle
autowiz:
	$(MAKE) ../bin/autowiz
autohero:
	$(MAKE) ../bin/autohero
delobjs:
	$(MAKE) ../bin/delobjs
listrent:
	$(MAKE) ../bin/listrent
mudpasswd:
	$(MAKE) ../bin/mudpasswd
purgeplay:
	$(MAKE) ../bin/purgeplay
showplay:
	$(MAKE) ../bin/showplay
sign:
	$(MAKE) ../bin/sign
mobfix:
	$(MAKE) ../bin/mobfix
showadd:
	$(MAKE) ../bin/showadd

../bin/autowiz: util/autowiz.c conf.h sysdep.h structs.h utils.h db.h
	$(CC) $(CFLAGS) -o ../bin/autowiz util/autowiz.c
../bin/autohero: util/autohero.c conf.h sysdep.h structs.h utils.h db.h
	$(CC) $(CFLAGS) -o ../bin/autohero util/autohero.c
../bin/delobjs: util/delobjs.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o ../bin/delobjs util/delobjs.c
../bin/listrent: util/listrent.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o ../bin/listrent util/listrent.c
../bin/mudpasswd: util/mudpasswd.c conf.h sysdep.h structs.h utils.h
	$(CC) $(CFLAGS) -o ../bin/mudpasswd util/mudpasswd.c
../bin/purgeplay: util/purgeplay.c conf.h sysdep.h structs.h utils.h
	$(CC) $(CFLAGS) -o ../bin/purgeplay util/purgeplay.c
../bin/showplay: util/showplay.c conf.h sysdep.h structs.h
	$(CC) $(CFLAGS) -o ../bin/showplay util/showplay.c
../bin/sign: util/sign.c conf.h sysdep.h
	$(CC) $(CFLAGS) -o ../bin/sign util/sign.c
../bin/mobfix: util/mob_fix.c
	$(CC) $(CFLAGS) -o ../bin/mobfix util/mob_fix.c
../bin/showadd: util/showadd.c
	$(CC) $(CFLAGS) -o ../bin/showadd util/showadd.c


../bin/circle : $(OBJFILES)
	$(CC) -o ../bin/circle $(PROFILE) $(OBJFILES)
clean:
	rm -f *.o

# Dependencies for the object files (automagically generated with
# gcc -MM)

act.comm.o: act.comm.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h \
  handler.h db.h screen.h spells.h
	$(CC) -c $(CFLAGS) act.comm.c
act.informative.o: act.informative.c conf.h sysdep.h structs.h utils.h comm.h \
  interpreter.h handler.h db.h spells.h screen.h
	$(CC) -c $(CFLAGS) act.informative.c
act.item.o: act.item.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h \
  handler.h db.h spells.h loadrooms.h
	$(CC) -c $(CFLAGS) act.item.c
act.movement.o: act.movement.c conf.h sysdep.h structs.h utils.h comm.h \
  interpreter.h handler.h db.h spells.h house.h
	$(CC) -c $(CFLAGS) act.movement.c
act.offensive.o: act.offensive.c conf.h sysdep.h structs.h utils.h comm.h \
  interpreter.h handler.h db.h spells.h
	$(CC) -c $(CFLAGS) act.offensive.c
act.other.o: act.other.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h \
  handler.h db.h spells.h screen.h house.h loadrooms.h
	$(CC) -c $(CFLAGS) act.other.c
act.social.o: act.social.c conf.h sysdep.h structs.h utils.h comm.h \
  interpreter.h handler.h db.h spells.h
	$(CC) -c $(CFLAGS) act.social.c
act.wizard.o: act.wizard.c conf.h sysdep.h structs.h utils.h comm.h \
  interpreter.h handler.h db.h spells.h house.h screen.h loadrooms.h
	$(CC) -c $(CFLAGS) act.wizard.c
ban.o: ban.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h handler.h db.h
	$(CC) -c $(CFLAGS) ban.c
boards.o: boards.c conf.h sysdep.h structs.h utils.h comm.h db.h boards.h \
  interpreter.h handler.h
	$(CC) -c $(CFLAGS) boards.c
class.o: class.c conf.h sysdep.h structs.h db.h utils.h spells.h interpreter.h
	$(CC) -c $(CFLAGS) class.c
comm.o: comm.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h handler.h \
  db.h house.h olc.h
	$(CC) -c $(CFLAGS) comm.c
config.o: config.c conf.h sysdep.h structs.h loadrooms.h
	$(CC) -c $(CFLAGS) config.c
color.o: color.c conf.h
	$(CC) -c $(CFLAGS) color.c
constants.o: constants.c conf.h sysdep.h structs.h
	$(CC) -c $(CFLAGS) constants.c
db.o: db.c conf.h sysdep.h structs.h utils.h db.h comm.h handler.h spells.h mail.h \
  interpreter.h house.h loadrooms.h auction.h
	$(CC) -c $(CFLAGS) db.c
fight.o: fight.c conf.h sysdep.h structs.h utils.h comm.h handler.h interpreter.h \
  db.h spells.h screen.h loadrooms.h
	$(CC) -c $(CFLAGS) fight.c
graph.o: graph.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h handler.h \
  db.h spells.h
	$(CC) -c $(CFLAGS) graph.c
handler.o: handler.c conf.h sysdep.h structs.h utils.h comm.h db.h handler.h \
  interpreter.h spells.h
	$(CC) -c $(CFLAGS) handler.c
house.o: house.c conf.h sysdep.h structs.h comm.h handler.h db.h interpreter.h \
  utils.h house.h
	$(CC) -c $(CFLAGS) house.c
interpreter.o: interpreter.c conf.h sysdep.h structs.h comm.h interpreter.h db.h \
  utils.h spells.h handler.h mail.h screen.h olc.h loadrooms.h
	$(CC) -c $(CFLAGS) interpreter.c
limits.o: limits.c conf.h sysdep.h structs.h utils.h spells.h comm.h db.h \
  handler.h
	$(CC) -c $(CFLAGS) limits.c
magic.o: magic.c conf.h sysdep.h structs.h utils.h comm.h spells.h handler.h db.h
	$(CC) -c $(CFLAGS) magic.c
mail.o: mail.c conf.h sysdep.h structs.h utils.h comm.h db.h interpreter.h \
  handler.h mail.h
	$(CC) -c $(CFLAGS) mail.c
mobact.o: mobact.c conf.h sysdep.h structs.h utils.h db.h comm.h interpreter.h \
  handler.h spells.h
	$(CC) -c $(CFLAGS) mobact.c
modify.o: modify.c conf.h sysdep.h structs.h utils.h interpreter.h handler.h db.h \
  comm.h spells.h mail.h boards.h olc.h
	$(CC) -c $(CFLAGS) modify.c
objsave.o: objsave.c conf.h sysdep.h structs.h comm.h handler.h db.h \
  interpreter.h utils.h spells.h
	$(CC) -c $(CFLAGS) objsave.c
olc.o: olc.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h handler.h db.h \
  olc.h screen.h loadrooms.h
	$(CC) -c $(CFLAGS) olc.c
random.o: random.c
	$(CC) -c $(CFLAGS) random.c
shop.o: shop.c conf.h sysdep.h structs.h comm.h handler.h db.h interpreter.h \
  utils.h shop.h
	$(CC) -c $(CFLAGS) shop.c
spec_assign.o: spec_assign.c conf.h sysdep.h structs.h db.h interpreter.h \
  utils.h
	$(CC) -c $(CFLAGS) spec_assign.c
spec_procs.o: spec_procs.c conf.h sysdep.h structs.h utils.h comm.h \
  interpreter.h handler.h db.h spells.h
	$(CC) -c $(CFLAGS) spec_procs.c
spell_parser.o: spell_parser.c conf.h sysdep.h structs.h utils.h interpreter.h \
  spells.h handler.h comm.h db.h
	$(CC) -c $(CFLAGS) spell_parser.c
spells.o: spells.c conf.h sysdep.h structs.h utils.h comm.h spells.h handler.h \
  db.h loadrooms.h
	$(CC) -c $(CFLAGS) spells.c
utils.o: utils.c conf.h sysdep.h structs.h utils.h comm.h screen.h spells.h \
  handler.h
	$(CC) -c $(CFLAGS) utils.c
weather.o: weather.c conf.h sysdep.h structs.h utils.h comm.h handler.h \
  interpreter.h db.h
	$(CC) -c $(CFLAGS) weather.c
log.o: log.c conf.h sysdep.h structs.h utils.h comm.h screen.h
	$(CC) -c $(CFLAGS) log.c
redit.o: redit.c conf.h sysdep.h structs.h utils.h comm.h boards.h \
  db.h olc.h loadrooms.h
	$(CC) -c $(CFLAGS) redit.c
oedit.o: oedit.c conf.h sysdep.h structs.h utils.h comm.h boards.h spells.h \
  db.h olc.h shop.h
	$(CC) -c $(CFLAGS) oedit.c
zedit.o: zedit.c conf.h sysdep.h structs.h utils.h comm.h db.h olc.h
	$(CC) -c $(CFLAGS) zedit.c 
medit.o: medit.c conf.h sysdep.h structs.h utils.h comm.h spells.h shop.h \
  db.h olc.h handler.h
	$(CC) -c $(CFLAGS) medit.c 
sedit.o: sedit.c conf.h sysdep.h structs.h utils.h comm.h db.h olc.h shop.h
	$(CC) -c $(CFLAGS) sedit.c 
alias.o: alias.c structs.h utils.h interpreter.h
	$(CC) -c $(CFLAGS) alias.c
clan.o: clan.c conf.h sysdep.h structs.h comm.h handler.h db.h \
 interpreter.h utils.h clan.h
	$(CC) -c $(CFLAGS) clan.c
auction.o: auction.c conf.h sysdep.h structs.h utils.h comm.h interpreter.h \
  handler.h db.h screen.h auction.h
	$(CC) -c $(CFLAGS) auction.c
act.create.o: act.create.c conf.h sysdep.h structs.h utils.h comm.h \
 interpreter.h handler.h db.h spells.h
	$(CC) -c $(CFLAGS) act.create.c
arena.o: arena.c structs.h utils.h comm.h screen.h spells.h handler.h \
  interpreter.h db.h
	$(CC) -c $(CFLAGS) arena.c
quest.o: quest.c conf.h sysdep.h structs.h utils.h handler.h \
 interpreter.h comm.h db.h
	$(CC) -c $(CFLAGS) quest.c
