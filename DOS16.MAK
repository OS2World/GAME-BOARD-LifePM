# Makefile for life und Watcom C
# DOS 16 Bit Version 
# Dieses makefile benutzt die GNU make Syntax.
# Um das Program zu compilieren gebe in der Kommandozeile 
#    make -f dos16.mak 
# ein.

CC = wcc
PP = wpp
LD = wcl

# Targetdirectory
OBJD = dos16\\
BIND = dos16\\

# Default compiler flags:
CFLAGS=-2 -bt=dos -wx -ms -zq
LDFLAGS=-zq -l=dos -k48k


# Soll optimiert werden setzte OPTIMIZE = 1
ifndef OPTIMIZE
OPTIMIZE = 0
endif

ifndef DEBUG
DEBUG = 0
endif

OPTIMIZE:= $(strip $(OPTIMIZE))
ifneq ($(OPTIMIZE),0)
   CFLAGS+= -oneatx -zp2
endif
ifeq ($(DEBUG),0)
	CFLAGS+= -DNDEBUG
else
   CFLAGS+= -d3 -DDEBUG
	LDFLAGS+= -d2 
endif

# Erkl„rung der benutzten Compiler Flags
# -2      --> Erstelle 286 code.
# -bt     --> target system ( DOS ).
# -d3     --> Alle debug Informationen
# -zq     --> quiet
# -wx     --> Maximal warning level
# -oneatx --> Maximale Optimierung
# -zp2    --> Richte Daten an word Grenzen aus
# -knk    --> Stack size (n=kByte)

CSRC = doslife.c life.c events.c mouse.c ehandler.c
CXXSRC =
OBJ = $(patsubst %.c,$(OBJD)\%.obj,$(CSRC))

$(OBJD)\%.obj: %.c
	$(CC) $(CFLAGS) -fo=$@ -fh=$(OBJD)$*.pch $<

$(BIND)doslife.exe: $(OBJ)
	$(LD) $(LDFLAGS) $^

$(OBJD)ehandler.obj: ehandler.c
	$(CC) $(CFLAGS) -zu -fo=$@ $<

$(OBJD)doslife.obj: doslife.c events.h life.h

$(OBJD)life.obj: life.c life.h

$(OBJD)events.obj: events.c mouse.h events.h

$(OBJD)mouse.obj: mouse.c mouse.h
