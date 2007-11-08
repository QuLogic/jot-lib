include $(JOT_ROOT)/Makefile.local

LIBNAME    = mlib

NORMAL_SOURCES = global.C \
                  points.C \
                  vec4.C

TEMPLATED_SOURCES = plane.C \
                     pointlist.C \
                     point2.C \
                     point3.C \
                     mat4.C \
                     quat.C \
                     random.C

ifeq ($(ARCH),WIN32)
JOT_NEEDS_TEMPLATES_IN_H_FILE = 1
endif

ifeq ($(ARCH),linux)
JOT_NEEDS_TEMPLATES_IN_H_FILE = 1
endif

ifeq ($(ARCH),macosx)
JOT_NEEDS_TEMPLATES_IN_H_FILE = 1
endif

ifeq ($(JOT_NEEDS_TEMPLATES_IN_H_FILE),)
SOURCES = $(NORMAL_SOURCES) $(TEMPLATED_SOURCES)
else
SOURCES = $(NORMAL_SOURCES) 
endif

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS)
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

include $(JOT_ROOT)/include/makefiles/std
