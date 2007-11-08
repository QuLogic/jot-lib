include $(JOT_ROOT)/Makefile.local

LIBNAME    = triangle

SOURCES = triangle.C

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS = 
LIBRARYIFLAGS = -I$(JOT_ROOT)/zlib
LIBRARYDFLAGS = -DTRILIBRARY -DANSI_DECLARATORS 

FINALCFLAGS   := $(FINALCFLAGS)



ifeq ($(ARCH),WIN32)
LIBRARYDFLAGS := $(LIBRARYDFLAGS) -DNO_TIMER 
endif

ifeq ($(ARCH),linux)
LIBRARYDFLAGS := $(LIBRARYDFLAGS) 
endif


#Compile as C (not C++)
ifeq ($(ARCH),WIN32)
FINALCFLAGS   := $(FINALCFLAGS) -Tc
endif
ifeq ($(ARCH),linux)
FINALCFLAGS   := $(FINALCFLAGS) -x c
endif
ifeq ($(ARCH),macosx)
FINALCFLAGS   := $(FINALCFLAGS) -x c
endif

include $(JOT_ROOT)/include/makefiles/std








