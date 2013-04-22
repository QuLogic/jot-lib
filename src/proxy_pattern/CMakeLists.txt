
ADD_LIBRARY(proxy_pattern
	proxy_surface.C
	proxy_stroke.C
	proxy_texture.C
	hatching_texture.C
	proxy_pen.C
	proxy_pen_ui.C)

TARGET_LINK_LIBRARIES(proxy_pattern
	disp
	geom
	gest
	gtex
	gui
	mesh
	npr
	stroke
	std
	glui
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

