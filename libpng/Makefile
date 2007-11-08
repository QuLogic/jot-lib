include $(JOT_ROOT)/Makefile.local

LIBNAME    = libpng

SOURCES = png.C \
         pngerror.C \
         pngget.C \
         pngmem.C  \
         pngpread.C \
         pngread.C \
		   pngrio.C \
         pngrtran.C \
         pngrutil.C \
         pngset.C \
         pngtrans.C \
         pngvcrd.C \
		   pnggccrd.C \
         pngwio.C \
         pngwrite.C \
         pngwtran.C \
         pngwutil.C

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = -I$(JOT_ROOT)/zlib
LIBRARYDFLAGS = -DPNG_NO_CONSOLE_IO

FINALCFLAGS   := $(FINALCFLAGS)



ifeq ($(ARCH),WIN32)
LIBRARYDFLAGS := $(LIBRARYDFLAGS) -DPNG_USE_PNGVCRD
endif

ifeq ($(ARCH),linux)
LIBRARYDFLAGS := $(LIBRARYDFLAGS) -DPNG_THREAD_UNSAFE_OK
endif


#Compile as C (not C++)
ifeq ($(ARCH),WIN32)
FINALCFLAGS   := $(FINALCFLAGS) -Tc
endif
ifeq ($(ARCH),linux)
FINALCFLAGS   := $(FINALCFLAGS) -x c
endif


include $(JOT_ROOT)/include/makefiles/std








