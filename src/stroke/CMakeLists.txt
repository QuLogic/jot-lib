
ADD_LIBRARY(stroke
	hatching_stroke.cpp
	b_stroke_pool.cpp
	decal_line_stroke.cpp
	base_stroke.cpp
	stroke_generator.cpp
	sil_stroke_pool.cpp
	edge_stroke.cpp
	edge_stroke_pool.cpp
	decal_stroke_pool.cpp
	outline_stroke.cpp
	gesture_stroke_drawer.cpp)

TARGET_LINK_LIBRARIES(stroke
	disp
	geom
	gtex
	mesh
	mlib
	net
	npr
	std
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

