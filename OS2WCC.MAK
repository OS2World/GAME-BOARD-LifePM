# Makefile for life and Watcom C
# OS/2 PM 32 Bit Version 
# Dieses makefile benutzt die GNU make Syntax.
# Um das Program zu compilieren gebe in der Kommandozeile 
#    make -f OS2wcc.mak 
# ein.

CC = wcc386
CXX = wpp386
LD = wlink
MV = mv
CP = cp
ECHO = echo

# Targetdirectory
OBJD = os2wcc\\
BIND = os2wcc\\

# Default compiler flags:
CFLAGS = -3r -bm -bt=os2v2_pm -wx
LDFLAGS = option quiet
BFLAGS =
IPFCFLAGS = 

# Soll optimiert werden setzte OPTIMIZE = 1
ifndef OPTIMIZE
OPTIMIZE = 0
endif

ifndef DEBUG
DEBUG = 0
endif

OPTIMIZE:= $(strip $(OPTIMIZE))
DEBUG:= $(strip $(DEBUG))

ifeq ($(DEBUG),0)
   CFLAGS+= -DNDEBUG
else
   CFLAGS+= -d3 -DDEBUG
	LDFLAGS+= debug all option symfile=$@
endif

ifneq ($(OPTIMIZE),0)
   CFLAGS+= -oneatx -zp4
	IPFCFLAGS+= /S
endif

# Erkl„rung der benutzten Compiler Flags
# -4r     --> Erstelle 486 code.
# -bt     --> target system ( OS/2 2.x PM ).
# -bm     --> multi threaded
# -d3     --> Alle debug Informationen
# -zq     --> quiet
# -wx     --> Maximal warning level
# -oneatx --> Maximale Optimierung
# -zp4    --> Richte Daten an dword Grenzen aus

ifdef WARP
# Use the -x2 resource compiler option with OS/2 Warp or above only.
	BFLAGS+= -x2
endif

CSRC = lifepm.c life.c errmsgs.c threads.c profile.c bitmaps.c
CXXSRC =
OBJ = $(patsubst %.c,$(OBJD)\%.obj,$(CSRC))

# We need the following makros to create lists seperated with commas
comma = ,
emtpy =
blank:= $(empty) $(empty)
OBJ:= $(strip $(OBJ))
FILES:= FILE $(subst $(blank),$(comma),$(OBJ))

# Resource file to be bound to the executable.
RESFILE = $(OBJD)lifepm.res
# Online help file:
HELPFILE = $(BIND)lifepm.hlp

# Implicite rules:
$(OBJD)\%.obj: %.c
	$(CC) $(CFLAGS) -fo=$@ -fh=$(OBJD)$*.pch $<
$(OBJD)\%.res: %.rc
	rc -r $< $@

.PHONY: all
all: $(BIND)lifepm.exe $(HELPFILE)

$(BIND)lifepm.exe: $(OBJD)lifepm $(RESFILE)
	$(CP) $< $@ >nul && rc $(BFLAGS) $(RESFILE) $@ >nul

$(OBJD)lifepm: $(OBJ) $(OBJD)lifepm.lnk
	$(LD) $(LDFLAGS) @$(OBJD)lifepm.lnk
	$(MV) lifepm.exe $@

$(OBJD)lifepm.lnk: lifepm.lnk os2wcc.mak
	$(CP) $< $@
	$(ECHO) $(FILES) >>$@

$(RESFILE): lifepm.rc help.rc dialogs.dlg lifepm.ico resdefs.h

$(HELPFILE): lifepm.scr license.ipf resdefs.h
	ipfcprep $< $(BIND)lifepm.ipf && ipfc $(IPFCFLAGS) $(BIND)lifepm.ipf && \
	rm $(BIND)lifepm.ipf


$(OBJD)lifepm.obj: lifepm.c lifepm.inc resdefs.h messages.h bitmaps.h \
	errmsgs.h threads.h profile.h life.h

$(OBJD)life.obj: life.c life.h

$(OBJD)errmsgs.obj: errmsgs.c errmsgs.h

$(OBJD)threads.obj: threads.c messages.h threads.h

$(OBJD)profile.obj: profile.c profile.h

$(OBJD)bitmaps.obj: bitmaps.c bitmaps.h


