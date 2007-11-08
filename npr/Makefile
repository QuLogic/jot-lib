include $(JOT_ROOT)/Makefile.local

LIBNAME    = npr

SOURCES = npr_view.C \
         sil_and_crease_texture.C \
         zxedge_stroke_texture.C \
         zxkey_line.C \
	 feature_stroke_texture.C \
         hatching_collection.C \
         hatching_group_base_.C \
         hatching_group_fixed.C \
         hatching_group_free.C \
	 hatching_group.C \
         npr_texture.C \
         ffstexture.C \
         ffs_control_frame.C \
         npr_solid_texture.C\
 	 npr_bkg_texture.C \
         npr_control_frame.C \
         wpath_stroke.C	\
         stylized_line3d.C \
	 image_proc.C \
         skybox_texture.C \
	 img_line.C \
	 binary_image.C \
	 ridge.C \
	 img_line_shader.C \
	 simple_img_line_shader.C \
	 image_plate.C \

ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS)
LIBRARYDFLAGS =

FINALCFLAGS   := $(FINALCFLAGS)

include $(JOT_ROOT)/include/makefiles/std
