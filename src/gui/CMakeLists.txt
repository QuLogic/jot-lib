
SET(GUI_FILES
	base_ui.cpp
	color_ui.cpp
	halftone_ui.cpp
	hatching_ui.cpp
	light_ui.cpp
	patch_selection_ui.cpp
	patch_ui.cpp
	painterly_ui.cpp
	presets_ui.cpp
	proxy_texture_ui.cpp
	tone_shader_ui.cpp
	img_line_ui.cpp
	ref_image_ui.cpp
	detail_ctrl_ui.cpp
	basecoat_ui.cpp)

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

