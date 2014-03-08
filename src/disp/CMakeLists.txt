
ADD_LIBRARY(disp
	bbox.C
	cam.C
	cam_focus.C
	color.C
	gel.C
	gel_filt.C
	ray.C
	view.C
	recorder.C
	animator.C
	gel_set.C
	paper_effect_base.C)

TARGET_LINK_LIBRARIES(disp
	geom
	mlib
	net
	std)

