
SET(PATTERN_FILES
	stroke_pattern.cpp
	stroke_pattern_drawer.cpp
	ref_frame_drawer.cpp
	stroke_group.cpp
	stroke_path.cpp
	gesture_stroke.cpp
	gesture_cell.cpp
	gesture_cell_drawer.cpp
	pattern_stroke.cpp
	pattern_texture.cpp
	pattern_pen.cpp
	pattern_pen_ui.cpp
	bbox_cell.cpp
	rect_cell.cpp
	path_cell.cpp
	carriers_cell.cpp
	gesture_rect_drawer.cpp
	gesture_path_drawer.cpp
	gesture_carrier_drawer.cpp
	eigen_solver.cpp
	impostor.cpp)

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

