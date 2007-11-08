include $(JOT_ROOT)/Makefile.local

LIBNAME = pattern

SOURCES = stroke_pattern.C \
          stroke_pattern_drawer.C \
          ref_frame_drawer.C \
          stroke_group.C \
          stroke_path.C \
          gesture_stroke.C \
          gesture_cell.C \
          gesture_cell_drawer.C \
          pattern_stroke.C \
          pattern_texture.C \
          pattern_pen.C \
          pattern_pen_ui.C \
     	  bbox_cell.C \
	  rect_cell.C \
	  path_cell.C \
	  carriers_cell.C \
	  gesture_rect_drawer.C \
	  gesture_path_drawer.C \
	  gesture_carrier_drawer.C \
	  eigen_solver.C \
	  impostor.C

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS) $(GLUT_INCS)
LIBRARYDFLAGS = -DANSI_DECLARATORS

FINALCFLAGS   := $(FINALCFLAGS)

include $(JOT_ROOT)/include/makefiles/std
