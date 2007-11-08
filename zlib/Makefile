include $(JOT_ROOT)/Makefile.local

LIBNAME    = zlib

SOURCES = adler32.C \
         compress.C \
         crc32.C \
         gzio.C \
         uncompr.C \
	      deflate.C \
         trees.C \
         zutil.C \
         inflate.C \
         infblock.C \
         inftrees.C \
         infcodes.C \
         infutil.C  \
         inffast.C

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = 
LIBRARYDFLAGS =

FINALCFLAGS   := $(FINALCFLAGS)

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








