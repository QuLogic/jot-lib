
ADD_LIBRARY(glut_winsys
	glut_winsys.cpp
	kbd.cpp
	mouse.cpp
	glui_menu.cpp
	glui_dialogs.cpp)

TARGET_LINK_LIBRARIES(glut_winsys
	widgets
	mlib
	geom
	dev
	std
	glui
	${OPENGL_LIBRARIES}
	${GLUT_LIBRARIES}
	${GLEW_LIBRARIES})

