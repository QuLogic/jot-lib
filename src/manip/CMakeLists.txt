
ADD_LIBRARY(manip
	cam_pz.cpp
	cam_fp.cpp
	cam_edit.cpp
	manip.cpp
	mmenu.cpp
	key_menu.cpp)

TARGET_LINK_LIBRARIES(manip
	base_jotapp
	disp
	geom
	gtex
	mesh
	widgets
	dev
	std)

