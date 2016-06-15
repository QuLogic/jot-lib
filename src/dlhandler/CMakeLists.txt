
ADD_LIBRARY(dlhandler
	dlhandler.cpp)

TARGET_LINK_LIBRARIES(dlhandler
	disp
	std
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

