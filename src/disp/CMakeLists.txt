
ADD_LIBRARY(disp
	bbox.cpp
	cam.cpp
	cam_focus.cpp
	color.cpp
	gel.cpp
	gel_filt.cpp
	ray.cpp
	view.cpp
	recorder.cpp
	animator.cpp
	gel_set.cpp
	paper_effect_base.cpp)

TARGET_LINK_LIBRARIES(disp
	geom
	mlib
	net
	std)

