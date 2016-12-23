
ADD_LIBRARY(ffs
	circle_widget.cpp
	crease_widget.cpp
	crv_sketch.cpp
	cursor3d.cpp
	draw_manip.cpp
	draw_pen.cpp
	extender.cpp
	roof.cpp
	ffs_util.cpp
	floor.cpp
	inflate.cpp
	oversketch.cpp
	profile.cpp
	paper_doll.cpp
	select_widget.cpp
	sweep.cpp
	trace.cpp
	xform_pen.cpp)

TARGET_LINK_LIBRARIES(ffs
	disp
	geom
	gest
	gtex
	manip
	map3d
	mesh
	mlib
	npr
	tess
	widgets
	std)

