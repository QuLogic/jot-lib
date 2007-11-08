include $(JOT_ROOT)/Makefile.local

LIBNAME    = tess

SOURCES = meme.C \
         blending_meme.C \
	 bbase.C \
         bpoint.C \
         bcurve.C \
         bsurface.C \
	 bvert_grid.C \
         uv_surface.C \
         panel.C \
         primitive.C \
         skin.C \
         skin_meme.C \
	 xf_meme.C \
         tex_body.C \
	 subdiv_updater.C \
	 disk_map.C \
	 vert_mapper.C \
	 mesh_op.C \
	 tess_cmd.C \
         action.C

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS)
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

include $(JOT_ROOT)/include/makefiles/std
