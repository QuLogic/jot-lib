include $(JOT_ROOT)/Makefile.local

# There is no library here, but need the following line
# for the main makefile (include/makefiles/std) to
# recognize the targets defined below.
LIBNAME = convert

SOURCES = 
ALLSRC  = $(SOURCES) $(PROG_1SRC) $(PROG_2SRC) $(PROG_3SRC)

# Common flags...
PROG_JOT_LIBS = mesh mlib disp geom net std dlhandler dev libpng zlib sps glew
PROG_LIBS     = $(OGL_LIBS) $(GLUT_LIBS) $(SYS_LIBS)
PROG_LDFLAGS  = $(OGL_FLAGS) $(GLUT_FLAGS)

#
# Program 1 - m2sm
#
PROG_1         = m2sm
PROG_1SRC      = m2sm.C
PROG_1JOT_LIBS = $(PROG_JOT_LIBS)
PROG_1LIB      = $(PROG_LIBS)
PROG_1LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 2 - ply2sm
#
PROG_2         = ply2sm
PROG_2SRC      = ply2sm.C ply.C
PROG_2JOT_LIBS = $(PROG_JOT_LIBS)
PROG_2LIB      = $(PROG_LIBS)
PROG_2LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 3 - sm2iv
#
PROG_3         = sm2iv
PROG_3SRC      = sm2iv.C
PROG_3JOT_LIBS = $(PROG_JOT_LIBS)
PROG_3LIB      = $(PROG_LIBS)
PROG_3LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 4 - iv2sm (not tested lately...)
#
PROG_4         = iv2sm
PROG_4SRC      = iv2sm.C
PROG_4JOT_LIBS = $(PROG_JOT_LIBS)
PROG_4LIB      = $(PROG_LIBS) $(OIV_LIBS)
PROG_4LDFLAGS  = $(PROG_LDFLAGS) $(OIV_FLAGS)

#
# Program 5 - sm2obj
#
PROG_5         = sm2obj
PROG_5SRC      = sm2obj.C
PROG_5JOT_LIBS = $(PROG_JOT_LIBS)
PROG_5LIB      = $(PROG_LIBS)
PROG_5LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 6 - obj2sm
#
PROG_6         = obj2sm
PROG_6SRC      = obj2sm.C
PROG_6JOT_LIBS = $(PROG_JOT_LIBS)
PROG_6LIB      = $(PROG_LIBS)
PROG_6LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 7 - png2sm
#
PROG_7         = png2sm
PROG_7SRC      = png2sm.C
PROG_7JOT_LIBS = $(PROG_JOT_LIBS)
PROG_7LIB      = $(PROG_LIBS)
PROG_7LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 8 - obj2jot
#
PROG_8         = obj2jot
PROG_8SRC      = obj2jot.C
PROG_8JOT_LIBS = $(PROG_JOT_LIBS)
PROG_8LIB      = $(PROG_LIBS)
PROG_8LDFLAGS  = $(PROG_LDFLAGS)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS)
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

include $(JOT_ROOT)/include/makefiles/std
