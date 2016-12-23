
ADD_LIBRARY(map3d
	map0d3d.cpp
	map1d3d.cpp
	map2d3d.cpp
	bnode.cpp)

TARGET_LINK_LIBRARIES(map3d
	geom
	mlib
	net
	std)

