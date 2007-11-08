include $(JOT_ROOT)/Makefile.local

DIRNAME = src

ifeq ($(ARCH),WIN32)
JOTRC = jot.rc
JOTICOS = jotapp.ico jotfile.ico jotcmd.ico
endif

ifeq ($(PROG),)
PROG=jot
endif
 
#
# Program 1 - jot
#
PROG_1          = jot
PROG_1SRC       = jot.C
PROG_1RC        = $(JOTRC)
PROG_1RCDEP     = $(JOTICOS)
LIB_LIST        = base_jotapp glut_winsys glui widgets tess manip ffs \
                 pattern triangle npr stroke geom disp gtex mesh net \
                 dev std gest wnpr map3d mlib dlhandler libpng zlib \
                 glew sps gui proxy_pattern
# doubling up the lib list is a hack to make linking work on linux:
PROG_1JOT_LIBS  := $(LIB_LIST) $(LIB_LIST)
PROG_1LIB       = $(OGL_LIBS) $(GLUT_LIBS) $(WIN_LIBS) $(SYS_LIBS)
PROG_1LDFLAGS   = $(OGL_FLAGS) $(GLUT_FLAGS)

#
# Program 2 - smview
#
PROG_2		= smview
PROG_2SRC	= smview.C 
PROG_2RC	= $(JOTRC)
PROG_2RCDEP	= $(JOTICOS)
LIB_LIST	= base_jotapp glut_winsys glui widgets manip gest geom disp\
		gtex mesh sps net dev std mlib dlhandler libpng zlib glew 
# doubling up the lib list is a hack to make linking work on linux:
PROG_2JOT_LIBS	:= $(LIB_LIST) $(LIB_LIST) 
PROG_2LIB	= $(OGL_LIBS) $(GLUT_LIBS) $(WIN_LIBS) $(SYS_LIBS)
PROG_2LDFLAGS	= $(OGL_FLAGS) $(GLUT_FLAGS)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS) $(GLUT_INCS)
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

include $(JOT_ROOT)/include/makefiles/std
