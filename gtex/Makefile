include $(JOT_ROOT)/Makefile.local

LIBNAME    = gtex

SOURCES = basic_texture.C \
          gl_extensions.C \
          paper_effect.C \
          xtoon_texture.C \
          toon_texture_1D.C \
          color_id_texture.C \
          fader_texture.C \
          wireframe.C \
          control_frame.C \
          control_line.C \
          solid_color.C \
          smooth_shade.C \
          flat_shade.C \
          normals_texture.C \
          sils_texture.C \
          creases_texture.C \
          key_line.C \
          zxsils_texture.C \
          ref_image.C \
          buffer_ref_image.C \
          aux_ref_image.C \
          curvature_texture.C \
          curvature_ui.C \
          line_drawing.C \
          perlin.C \
	  glsl_shader.C \
	  halftone_shader.C \
	  halftone_shader_ex.C\
	  hatching_tx.C \
	  painterly.C \
          glsl_toon.C \
          tone_shader.C \
          glsl_halo.C \
          glsl_marble.C\
          glsl_toon_halo.C \
          glsl_paper.C \
	  glsl_xtoon.C \
          glsl_solid.C \
	  glsl_normal.C \
	  dots.C \
	  dots_ex.C \
	  haftone_tx.C \
	  glsl_hatching.C \
	  gl_sphir_tex_coord_gen.C \
	  glsl_layer_base.C \
	  halo_ref_image.C \
	  blur_shader.C \
	  halo_blur_shader.C \
	  basecoat_shader.C \
	  multi_lights_tone.C \
	  patch_id_texture.C \


ALLSRC  = $(SOURCES)

LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS) $(GLUT_INCS)
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

ifneq ($(REF_IMG_32_BIT),)
LIBRARYDFLAGS := $(LIBRARYDFLAGS) -DREF_IMG_32_BIT
endif

include $(JOT_ROOT)/include/makefiles/std
