
ADD_LIBRARY(widgets
	fps.C
	kbd_nav.C
	collide.C)

TARGET_LINK_LIBRARIES(widgets
	disp
	geom
	manip
	mlib
	sps
	std)

