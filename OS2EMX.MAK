# Makefile for life.exe with emx-gcc
#
# To compile all necessary files simply type `make gcc'at the command line 
# prompt.
# This makefile runs with GNU make (version >= 3.??) only

# Commands
CC = gcc
CXX = gcc
LD = gcc
CP = cp
RM = rm
MV = mv
ECHO = echo

# Targetdirectories
OBJD = os2emx\\
BIND = os2emx\\

# Default compiler flags:
CFLAGS = -Wall -mprobe -ZC++-comments
LDFLAGS=
BFLAGS =
IPFCFLAGS = 

# To optimize set OPTIMIZE to 1
ifndef OPTIMIZE
OPTIMIZE = 0
endif

# To produce debug information and assert code set DEBUG to 1
ifndef DEBUG
DEBUG = 0
endif

# To use method (E2) for compiling as described in emxdev.doc/emxdev.inf
# set USEOMFLIBS to 1
ifndef USEOMFLIBS
USEOMFLIBS = 0
endif

# If you don't want to use emxlibc.dll set STATICLIBS to 1
ifndef STATICLIBS
STATICLIBS = 0
endif

ifeq ($(DEBUG),0)
   CFLAGS+= -DNDEBUG
else
   CFLAGS+= -g -DDEBUG
	LDFLAGS+= -g
endif

ifneq ($(OPTIMIZE),0)
   CFLAGS+= -O2
	LDFLAGS+= -s
	IPFCFLAGS+= /S
endif
# The options -s doesn't optimize but strips the symbol table which
# is added to each file. So the code produced with -s isn't faster but the 
# executable is smaller. One the other hand, stripping symbols makes 
# debugging impossible.

ifdef WARP
# Use the -x2 resource compiler option with OS/2 Warp or above only.
	BFLAGS+= -x2
endif

# extension of object and library files
ifeq ($(USEOMFLIBS),0)
   O = .o
	L = .a
else
   O = .obj
	L = .lib
	CFLAGS+= -Zomf
	LDFLAGS+= -Zomf
endif

ifeq ($(STATICLIBS),0)
	CFLAGS+= -Zmtd
	LDFLAGS+= -Zmtd
else
	CFLAGS+= -Zmts
	LDFLAGS+= -Zmts
endif

CSRC = lifepm.c life.c errmsgs.c threads.c profile.c bitmaps.c
CXXSRC =
OBJ = $(patsubst %.c,$(OBJD)\%$O,$(CSRC))

# Resource file to be bound to the executable.
RESFILE = $(OBJD)lifepm.res
# Online help file:
HELPFILE = $(BIND)lifepm.hlp

# Implicite rules:
$(OBJD)\%$O: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
$(OBJD)\%.res: %.rc
	rc -r $< $@


.PHONY: all
all: $(BIND)lifepm.exe $(HELPFILE)

$(BIND)lifepm.exe: $(OBJD)lifepm $(RESFILE)
ifeq ($(USEOMFLIBS),0)
	emxbind -bp -r$(OBJD)lifepm.res $<
else
	$(CP) $< $@ >nul && rc $(BFLAGS) $(RESFILE) $@ >nul
endif

$(OBJD)lifepm : $(OBJ)
	$(LD) $(LDFLAGS) -o $@. $(OBJ) lifepm.def

$(RESFILE): lifepm.rc dialogs.dlg lifepm.ico resdefs.h

$(HELPFILE): lifepm.scr license.ipf resdefs.h
	ipfcprep $< $(BIND)lifepm.ipf &&	ipfc $(IPFCFLAGS) $(BIND)lifepm.ipf && \
	$(RM) $(BIND)lifepm.ipf

.PHONY: dep
dep:
	gcc -MM $(CFLAGS) $(CSRC) >depend

-include depend


