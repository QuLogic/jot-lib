
ADD_LIBRARY(mesh
	simplex_data.C
	bsimplex.C
	bvert.C
	bedge.C
	bface.C
	bmesh.C
	bmesh_curvature.C
	patch.C
	gtexture.C
	base_ref_image.C
	vert_strip.C
	edge_strip.C
	tri_strip.C
	lvert.C
	ledge.C
	lface.C
	lpatch.C
	lmesh.C
	zcross_path.C
	lstrip.C
	ledge_strip.C
	lvert_strip.C
	ioblock.C
	mi.C
	uv_data.C
	uv_mapping.C
	mesh_global.C
	zcross_extractor.C
	objreader.C
	mesh_select_cmd.C
	tex_coord_gen.C
	patch_blend_weight.C)

TARGET_LINK_LIBRARIES(mesh
	disp
	dlhandler
	geom
	gtex
	map3d
	mlib
	net
	sps
	std
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

#
# Program 1 - subdivide 
#
ADD_EXECUTABLE(subdivide EXCLUDE_FROM_ALL subdivide.C)
TARGET_LINK_LIBRARIES(subdivide mesh std)

#
# Program 2 - cpmesh
#
# Copies mesh from input to output...
#   can be used to get clean version of mesh
ADD_EXECUTABLE(cpmesh EXCLUDE_FROM_ALL cpmesh.C)
TARGET_LINK_LIBRARIES(cpmesh mesh std)

#
# Program 3 - fit
#
ADD_EXECUTABLE(fit EXCLUDE_FROM_ALL fit.C)
TARGET_LINK_LIBRARIES(fit mesh std)

#
# Program 4 - split
#
ADD_EXECUTABLE(split EXCLUDE_FROM_ALL split.C)
TARGET_LINK_LIBRARIES(split mesh std)

#
# Program 5 - merge
#
ADD_EXECUTABLE(merge EXCLUDE_FROM_ALL merge.C)
TARGET_LINK_LIBRARIES(merge mesh std)

#
# Program 6 - reverse
#
ADD_EXECUTABLE(reverse EXCLUDE_FROM_ALL reverse.C)
TARGET_LINK_LIBRARIES(reverse mesh std)

#
# Program 7 - print_mesh
#
ADD_EXECUTABLE(print_mesh EXCLUDE_FROM_ALL print_mesh.C)
TARGET_LINK_LIBRARIES(print_mesh mesh std)

#
# Program 8 - test_mesh
#
ADD_EXECUTABLE(test_mesh EXCLUDE_FROM_ALL test_mesh.C)
TARGET_LINK_LIBRARIES(test_mesh mesh std)

#
# Program 9 - test_OBJReader
#
#ADD_EXECUTABLE(test_OBJReader test_OBJReader.C)
#TARGET_LINK_LIBRARIES(test_OBJReader mesh std)

#
# Program 10 - color a mesh
#
ADD_EXECUTABLE(color_mesh EXCLUDE_FROM_ALL color_mesh.C)
TARGET_LINK_LIBRARIES(color_mesh mesh disp std)

#
# Program 11 - qinv 
#
ADD_EXECUTABLE(qinv EXCLUDE_FROM_ALL qinv.C)
TARGET_LINK_LIBRARIES(qinv mesh std)

#
# Program 12 - hist_eq
#
ADD_EXECUTABLE(hist_eq EXCLUDE_FROM_ALL hist_eq.C)
TARGET_LINK_LIBRARIES(hist_eq mesh geom std)

#
# Program 13 - p2h
#
ADD_EXECUTABLE(p2h EXCLUDE_FROM_ALL p2h.C)
TARGET_LINK_LIBRARIES(p2h mesh geom std)

#
# Program 14 - p2hatch
#
ADD_EXECUTABLE(p2hatch EXCLUDE_FROM_ALL p2hatch.C)
TARGET_LINK_LIBRARIES(p2hatch mesh geom std)

#
# Program 15 - split_patches
#
ADD_EXECUTABLE(split_patches EXCLUDE_FROM_ALL split_patches.C)
TARGET_LINK_LIBRARIES(split_patches mesh std)

#
# Program 16 - align
#
ADD_EXECUTABLE(align EXCLUDE_FROM_ALL align.C)
TARGET_LINK_LIBRARIES(align mesh std)

#
# Program 17 - dt
#
ADD_EXECUTABLE(dt EXCLUDE_FROM_ALL dt.C)
TARGET_LINK_LIBRARIES(dt mesh geom std)

