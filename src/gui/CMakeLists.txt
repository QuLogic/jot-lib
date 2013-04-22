
SET(GUI_FILES
	base_ui.C
	color_ui.C
	halftone_ui.C
	hatching_ui.C
	light_ui.C
	patch_selection_ui.C
	patch_ui.C
	painterly_ui.C
	presets_ui.C
	proxy_texture_ui.C
	tone_shader_ui.C
	img_line_ui.C
	ref_image_ui.C
	detail_ctrl_ui.C
	basecoat_ui.C)

ADD_LIBRARY(gui ${GUI_FILES})

TARGET_LINK_LIBRARIES(gui
	disp
	geom
	gtex
	mesh
	net
	npr
	std
	glui
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

IF(REF_IMG_32_BIT)
SET_PROPERTY(
	SOURCE ${GUI_FILES}
	APPEND
	PROPERTY COMPILE_DEFINITIONS
	REF_IMG_32_BIT)
ENDIF(REF_IMG_32_BIT)

