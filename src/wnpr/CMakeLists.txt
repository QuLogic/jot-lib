
ADD_LIBRARY(wnpr
	halo_sphere.C
	hatching_pen.C
	hatching_pen_ui.C
	line_pen.C
	line_pen_ui.C
	npr_pen.C
	npr_pen_ui.C
	paper_ui.C
	sil_ui.C
	sky_box.C
	stroke_ui.C
	view_ui.C)

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

