
ADD_LIBRARY(ffs
	circle_widget.C
	crease_widget.C
	crv_sketch.C
	cursor3d.C
	draw_manip.C
	draw_pen.C
	extender.C
	roof.C
	ffs_util.C
	floor.C
	inflate.C
	oversketch.C
	profile.C
	paper_doll.C
	select_widget.C
	sweep.C
	trace.C
	xform_pen.C)

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

