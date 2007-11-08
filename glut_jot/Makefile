include $(JOT_ROOT)/Makefile.local

LIBNAME    = glut_jot

GLUT_SOURCES = glut_8x13.C \
               glut_9x15.C \
               glut_bitmap.C \
               glut_bwidth.C \
               glut_cindex.C \
               glut_cmap.C \
               glut_cursor.C \
               glut_dials.C \
               glut_dstr.C \
               glut_event.C \
               glut_ext.C \
               glut_fullscrn.C \
               glut_gamemode.C \
               glut_get.C \
               glut_glxext.C \
               glut_hel10.C \
               glut_hel12.C \
               glut_hel18.C \
               glut_init.C \
               glut_input.C \
               glut_joy.C \
               glut_key.C \
               glut_keyctrl.C \
               glut_keyup.C \
               glut_mesa.C \
               glut_modifier.C \
               glut_mroman.C \
               glut_overlay.C \
               glut_roman.C \
               glut_shapes.C \
               glut_space.C \
               glut_stroke.C \
               glut_swap.C \
               glut_swidth.C \
               glut_tablet.C \
               glut_teapot.C \
               glut_tr10.C \
               glut_tr24.C \
               glut_util.C \
               glut_vidresize.C \
               glut_warp.C \
               glut_win.C \
               glut_winmisc.C 

GLUT_SOURCES_WIN32 = win32_glx.C \
                     win32_menu.C \
                     win32_util.C \
                     win32_winproc.C \
                     win32_x11.C

GLUT_SOURCES_UNIX = glut_menu.C \
                     glut_menu2.C \
                     layerutil.C 


#If no BUILD_GLUT, SOURCES is empty
#and no library will be built...

ifeq ($(BUILD_GLUT),)
SOURCES=
else
SOURCES = $(GLUT_SOURCES)
ifeq ($(ARCH),WIN32)
SOURCES := $(SOURCES) $(GLUT_SOURCES_WIN32)
endif
ifeq ($(ARCH),linux)
SOURCES := $(SOURCES) $(GLUT_SOURCES_UNIX)
endif
endif

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS = 
LIBRARYIFLAGS = -I$(JOT_ROOT)/glut_jot $(OGL_INCS) 
LIBRARYDFLAGS = 

#Compile as C (not C++)

ifeq ($(ARCH),WIN32)
FINALCFLAGS  := $(FINALCFLAGS) -Tc
endif
ifeq ($(ARCH),linux)
FINALCFLAGS   := $(FINALCFLAGS) -x c
endif

include $(JOT_ROOT)/include/makefiles/std
