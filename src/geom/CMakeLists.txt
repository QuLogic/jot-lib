
SET(GEOM_FILES
	geom.cpp
	gl_view.cpp
	fsa.cpp
	text2d.cpp
	icon2d.cpp
	line3d.cpp
	command.cpp
	world.cpp
	texture.cpp
	image.cpp
	hspline.cpp
	appear.cpp
	texturegl.cpp
	distrib.cpp
	recorder_ui.cpp
	file_listbox.cpp)

ADD_LIBRARY(geom ${GEOM_FILES})

TARGET_LINK_LIBRARIES(geom
	dev
	disp
	dlhandler
	gtex
	mlib
	net
	widgets
	std
	glui
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	${PNG_LIBRARIES})

SET_PROPERTY(
	SOURCE ${GEOM_FILES}
	APPEND
	PROPERTY INCLUDE_DIRECTORIES
	${PNG_INCLUDE_DIRS})

