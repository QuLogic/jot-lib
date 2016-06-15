
ADD_LIBRARY(tess
	meme.cpp
	blending_meme.cpp
	bbase.cpp
	bpoint.cpp
	bcurve.cpp
	bsurface.cpp
	bvert_grid.cpp
	uv_surface.cpp
	panel.cpp
	primitive.cpp
	skin.cpp
	skin_meme.cpp
	xf_meme.cpp
	tex_body.cpp
	subdiv_updater.cpp
	disk_map.cpp
	vert_mapper.cpp
	mesh_op.cpp
	tess_cmd.cpp
	action.cpp)

TARGET_LINK_LIBRARIES(tess
	disp
	geom
	gtex
	map3d
	mesh
	mlib
	net
	npr
	std
	${OPENGL_LIBRARIES})

