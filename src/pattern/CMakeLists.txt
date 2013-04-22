
SET(PATTERN_FILES
	stroke_pattern.C
	stroke_pattern_drawer.C
	ref_frame_drawer.C
	stroke_group.C
	stroke_path.C
	gesture_stroke.C
	gesture_cell.C
	gesture_cell_drawer.C
	pattern_stroke.C
	pattern_texture.C
	pattern_pen.C
	pattern_pen_ui.C
	bbox_cell.C
	rect_cell.C
	path_cell.C
	carriers_cell.C
	gesture_rect_drawer.C
	gesture_path_drawer.C
	gesture_carrier_drawer.C
	eigen_solver.C
	impostor.C)

ADD_LIBRARY(pattern ${PATTERN_FILES})

TARGET_LINK_LIBRARIES(pattern
	base_jotapp
	disp
	geom
	gest
	gtex
	manip
	mesh
	mlib
	npr
	tess
	triangle
	std
	glui
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

SET_PROPERTY(
	SOURCE ${PATTERN_FILES}
	APPEND
	PROPERTY COMPILE_DEFINITIONS
	ANSI_DECLARATORS)

