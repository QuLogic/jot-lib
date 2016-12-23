
ADD_LIBRARY(proxy_pattern
	proxy_surface.cpp
	proxy_stroke.cpp
	proxy_texture.cpp
	hatching_texture.cpp
	proxy_pen.cpp
	proxy_pen_ui.cpp)

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

