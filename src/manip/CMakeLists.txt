
ADD_LIBRARY(manip
	cam_pz.C
	cam_fp.C
	cam_edit.C
	manip.C
	mmenu.C
	key_menu.C)

TARGET_LINK_LIBRARIES(manip
	base_jotapp
	disp
	geom
	gtex
	mesh
	widgets
	dev
	std)

