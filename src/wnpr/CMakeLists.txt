
ADD_LIBRARY(wnpr
	halo_sphere.cpp
	hatching_pen.cpp
	hatching_pen_ui.cpp
	line_pen.cpp
	line_pen_ui.cpp
	npr_pen.cpp
	npr_pen_ui.cpp
	paper_ui.cpp
	sil_ui.cpp
	sky_box.cpp
	stroke_ui.cpp
	view_ui.cpp)

TARGET_LINK_LIBRARIES(wnpr
	base_jotapp
	disp
	geom
	gest
	gtex
	mesh
	npr
	stroke
	tess
	std
	glui
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

