
ADD_LIBRARY(base_jotapp
	base_jotapp.cpp)

TARGET_LINK_LIBRARIES(base_jotapp
	disp
	geom
	gest
	glut_winsys
	gtex
	manip
	mesh
	mlib
	net
	npr
	widgets
	std)
