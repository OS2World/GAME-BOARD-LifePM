# Makefile for life und Watcom C
# DOS 32 Bit Version 
# Dieses makefile benutzt die GNU make Syntax.
# Um das Program zu compilieren gebe in der Kommandozeile 
#    make -f dos32.mak 
# ein.

# Under no circumstances use the -oea option for Watcom since it results
# in nasty trap-errors under OS/2

CC = wcc386
PP = wpp386
LD = wcl386

# Targetdirectory
OBJD = dos32\\
BIND = dos32\\

# Default compiler flags
CFLAGS=-3r -bt=dos4g -wx -zq
LDFLAGS=-zq -l=dos4g

# Soll optimiert werden setzte OPTIMIZE = 1
ifndef OPTIMIZE
OPTIMIZE = 0
endif

OPTIMIZE:= $(strip $(OPTIMIZE))
ifeq ($(OPTIMIZE),0)
   CFLAGS+= -d3 -DDEBUG
	LDFLAGS+= -d2
else
   CFLAGS+= -ox -zp4
endif

# Erkl„rung der benutzten Compiler Flags
# -4r     --> Erstelle 486 code.
# -bt     --> target system ( DOS ).
# -d3     --> Alle debug Informationen
# -zq     --> quiet
# -wx     --> Maximal warning level
# -oex    --> Maximale Optimierung
# -zp4    --> Richte Daten an dword Grenzen aus

CSRC = doslife.c life.c events.c mouse.c ehandler.c
CXXSRC =
OBJ = $(patsubst %.c,$(OBJD)\%.obj,$(CSRC))

$(OBJD)\%.obj: %.c
	$(CC) $(CFLAGS) -fo=$@ -fh=$(OBJD)$*.pch $<

doslife.exe: $(OBJ)
	$(LD) $(LDFLAGS) $^ @dos32.lnk

$(OBJD)ehandler.obj: ehandler.c
	$(CC) $(CFLAGS) -zu -zdf -fo=$@ $<

$(OBJD)doslife.obj: doslife.c events.h life.h

$(OBJD)life.obj: life.c life.h

$(OBJD)events.obj: events.c mouse.h events.h

$(OBJD)mouse.obj: mouse.c mouse.h
