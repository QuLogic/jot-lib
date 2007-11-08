include $(JOT_ROOT)/Makefile.local

LIBNAME    = dev

SOURCES = dev.C \
         devpoll.C \
         tty.C \
         tablet.C 

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS)
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

# Not under development...
ifeq ($(ARCH),linux)
WARNINGCFLAGS = -w
endif

include $(JOT_ROOT)/include/makefiles/std
