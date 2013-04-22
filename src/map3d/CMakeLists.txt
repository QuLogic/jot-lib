
ADD_LIBRARY(map3d
	map0d3d.C
	map1d3d.C
	map2d3d.C
	bnode.C)

TARGET_LINK_LIBRARIES(map3d
	geom
	mlib
	net
	std)

