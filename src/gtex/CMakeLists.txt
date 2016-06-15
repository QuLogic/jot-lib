
SET(GTEX_FILES
	basic_texture.cpp
	gl_extensions.cpp
	paper_effect.cpp
	xtoon_texture.cpp
	toon_texture_1D.cpp
	color_id_texture.cpp
	fader_texture.cpp
	wireframe.cpp
	control_frame.cpp
	control_line.cpp
	solid_color.cpp
	smooth_shade.cpp
	flat_shade.cpp
	normals_texture.cpp
	sils_texture.cpp
	creases_texture.cpp
	key_line.cpp
	zxsils_texture.cpp
	ref_image.cpp
	buffer_ref_image.cpp
	aux_ref_image.cpp
	curvature_texture.cpp
	curvature_ui.cpp
	line_drawing.cpp
	perlin.cpp
	glsl_shader.cpp
	halftone_shader.cpp
	halftone_shader_ex.cpp
	hatching_tx.cpp
	painterly.cpp
	glsl_toon.cpp
	tone_shader.cpp
	glsl_halo.cpp
	glsl_marble.cpp
	glsl_toon_halo.cpp
	glsl_paper.cpp
	glsl_xtoon.cpp
	glsl_solid.cpp
	glsl_normal.cpp
	dots.cpp
	dots_ex.cpp
	haftone_tx.cpp
	glsl_hatching.cpp
	gl_sphir_tex_coord_gen.cpp
	glsl_layer_base.cpp
	halo_ref_image.cpp
	blur_shader.cpp
	halo_blur_shader.cpp
	basecoat_shader.cpp
	multi_lights_tone.cpp
	patch_id_texture.cpp)

ADD_LIBRARY(gtex ${GTEX_FILES})

TARGET_LINK_LIBRARIES(gtex
	disp
	dlhandler
	geom
	glew
	mesh
	mlib
	npr
	std
	glui
	${OPENGL_LIBRARY}
	${GLEW_LIBARY})

IF(REF_IMG_32_BIT)
SET_PROPERTY(
	SOURCE ${GTEX_FILES}
	APPEND
	PROPERTY COMPILE_DEFINITIONS
	REF_IMG_32_BIT)
ENDIF(REF_IMG_32_BIT)

