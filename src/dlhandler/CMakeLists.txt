
ADD_LIBRARY(dlhandler
	dlhandler.C)

TARGET_LINK_LIBRARIES(dlhandler
	disp
	std
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

