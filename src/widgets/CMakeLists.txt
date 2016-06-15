
ADD_LIBRARY(widgets
	fps.cpp
	kbd_nav.cpp
	collide.cpp)

TARGET_LINK_LIBRARIES(widgets
	disp
	geom
	manip
	mlib
	sps
	std)

