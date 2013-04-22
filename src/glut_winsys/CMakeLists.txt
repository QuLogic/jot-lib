
ADD_LIBRARY(glut_winsys
	glut_winsys.C
	kbd.C
	mouse.C
	glui_menu.C
	glui_dialogs.C)

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

