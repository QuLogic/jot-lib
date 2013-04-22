
ADD_LIBRARY(tess
	meme.C
	blending_meme.C
	bbase.C
	bpoint.C
	bcurve.C
	bsurface.C
	bvert_grid.C
	uv_surface.C
	panel.C
	primitive.C
	skin.C
	skin_meme.C
	xf_meme.C
	tex_body.C
	subdiv_updater.C
	disk_map.C
	vert_mapper.C
	mesh_op.C
	tess_cmd.C
	action.C)

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

