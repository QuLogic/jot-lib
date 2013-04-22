
ADD_LIBRARY(gest
	gesture.C
	gest_int.C
	gesture_box_drawer.C
	pen.C
	pen_manager.C
	draw_widget.C
	mode_name.C
	vieweasel.C
	easel_manager.C
	patch_pen.C)

TARGET_LINK_LIBRARIES(gest
	disp
	geom
	gtex
	manip
	mesh
	mlib
	net
	std)

