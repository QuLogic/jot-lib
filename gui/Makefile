include $(JOT_ROOT)/Makefile.local

LIBNAME    = gui

SOURCES = base_ui.C \
          color_ui.C \
          halftone_ui.C \
          hatching_ui.C \
          light_ui.C \
          patch_selection_ui.C \
          patch_ui.C \
          painterly_ui.C \
          presets_ui.C \
          proxy_texture_ui.C \
          tone_shader_ui.C \
          img_line_ui.C \
          ref_image_ui.C \
          detail_ctrl_ui.C \
	  basecoat_ui.C
          
ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS) $(GLUT_INCS)
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

ifneq ($(REF_IMG_32_BIT),)
LIBRARYDFLAGS := $(LIBRARYDFLAGS) -DREF_IMG_32_BIT
endif

include $(JOT_ROOT)/include/makefiles/std
