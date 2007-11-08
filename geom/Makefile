include $(JOT_ROOT)/Makefile.local

LIBNAME    = geom

SOURCES = geom.C \
          gl_view.C \
          fsa.C \
          text2d.C \
          icon2d.C \
          line3d.C \
          command.C \
          world.C \
          texture.C \
          image.C \
          hspline.C \
          appear.C \
          texturegl.C \
          distrib.C \
          recorder_ui.C \
          file_listbox.C

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS) $(GLUT_INCS) -I$(JOT_ROOT)/zlib
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

include $(JOT_ROOT)/include/makefiles/std
