
ADD_LIBRARY(gest
	gesture.cpp
	gest_int.cpp
	gesture_box_drawer.cpp
	pen.cpp
	pen_manager.cpp
	draw_widget.cpp
	mode_name.cpp
	vieweasel.cpp
	easel_manager.cpp
	patch_pen.cpp)

TARGET_LINK_LIBRARIES(gest
	disp
	geom
	gtex
	manip
	mesh
	mlib
	net
	std)

