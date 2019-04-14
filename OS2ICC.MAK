# Makefile for life.exe and IBM C
# OS/2 PM 32 Bit Version 
# Dieses makefile benutzt die GNU make Syntax.
# Um das Program zu compilieren gebe in der Kommandozeile 
#    make -f OS2icc.mak 
# ein.

CC = icc
CXX = icc
LD = icc
MV = mv
CP = cp
RM = rm
ECHO = echo

# Targetdirectory
OBJD = os2icc\\
BIND = os2icc\\

# Default compiler flags
CFLAGS = -q+ -g3 -gm -gs- -W3 -ss -tl+
LDFLAGS = -q+ -tl+
BFLAGS =
IPFCFLAGS = 

# Soll optimiert werden setzte OPTIMIZE = 1
ifndef OPTIMIZE
OPTIMIZE = 0
endif

ifndef DEBUG
DEBUG = 0
endif

ifndef STATICLIBS
STATICLIBS = 0
endif

OPTIMIZE:= $(strip $(OPTIMIZE))
DEBUG:= $(strip $(DEBUG))

ifneq ($(DEBUG),0)
   CFLAGS+= -DDEBUG -ti
	LDFLAGS+= 
endif
ifneq ($(OPTIMIZE),0)
   CFLAGS+= -o+
	IPFCFLAGS+= /S
endif

ifeq ($(STATICLIBS),0)
   CFLAGS+= -gd+
	LDFLAGS+= -gd+
endif

ifdef WARP
# Use the -x2 resource compiler option with OS/2 Warp or above only.
	BFLAGS+= -x2
endif

# Descripiton of the compiler flags used:
# -g4     --> produce code optimized for 486
# -gm     --> multi threaded
# -gs-    --> create stack probes
# -gd+    --> use dll runtime library
# -ss+    --> allow C++ comments
# -ti+    --> generate debug information
# -tl+    --> preload the compiler
# -o+     --> optimize
# -q+     --> quiet
# -W3     --> produce all warnings

CSRC = lifepm.c life.c errmsgs.c threads.c profile.c bitmaps.c
CXXSRC =
OBJ = $(patsubst %.c,$(OBJD)\%.obj,$(CSRC))

# Resource file to be bound to the executable.
RESFILE = $(OBJD)lifepm.res
# Online help file:
HELPFILE = $(BIND)lifepm.hlp

# Implicite rules:
$(OBJD)\%.obj: %.c
	$(CC) -c $(CFLAGS) -fo$@ $<
$(OBJD)\%.res: %.rc
	rc -r $< $@

.PHONY: all
all: $(BIND)lifepm.exe $(HELPFILE)

$(BIND)lifepm.exe: $(OBJD)lifepm $(RESFILE)
	$(CP) $< $@ >nul && rc $(BFLAGS) $(RESFILE) $@ >nul

$(OBJD)lifepm: $(OBJ) lifepm.def
	$(LD) $(LDFLAGS) $(OBJ) lifepm.def -fe$@.

$(RESFILE): lifepm.rc dialogs.dlg lifepm.ico resdefs.h

$(HELPFILE): lifepm.scr license.ipf resdefs.h
	ipfcprep $< $(BIND)lifepm.ipf &&	ipfc $(IPFCFLAGS) $(BIND)lifepm.ipf && \
	$(RM) $(BIND)lifepm.ipf

$(OBJD)lifepm.obj: lifepm.c lifepm.inc resdefs.h messages.h bitmaps.h \
	errmsgs.h  threads.h profile.h life.h
$(OBJD)life.obj: life.c life.h
$(OBJD)errmsgs.obj: errmsgs.c errmsgs.h
$(OBJD)threads.obj: threads.c messages.h threads.h
$(OBJD)profile.obj: profile.c profile.h
$(OBJD)bitmaps.obj: bitmaps.c bitmaps.h


