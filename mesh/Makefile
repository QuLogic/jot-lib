include $(JOT_ROOT)/Makefile.local

LIBNAME    = mesh

SOURCES = simplex_data.C \
          bsimplex.C \
          bvert.C \
          bedge.C \
          bface.C \
          bmesh.C \
          bmesh_curvature.C \
          patch.C \
          gtexture.C \
          base_ref_image.C \
          vert_strip.C \
          edge_strip.C \
          tri_strip.C \
          lvert.C \
          ledge.C \
          lface.C \
          lpatch.C \
          lmesh.C \
          zcross_path.C \
          lstrip.C \
          ledge_strip.C \
          lvert_strip.C \
          ioblock.C \
          mi.C \
          uv_data.C \
          uv_mapping.C \
          mesh_global.C \
          zcross_extractor.C \
          objreader.C \
          mesh_select_cmd.C \
          tex_coord_gen.C \
	  patch_blend_weight.C

ALLSRC  = $(SOURCES) $(PROG_1SRC) $(PROG_2SRC) $(PROG_3SRC) $(PROG_4SRC) \
                     $(PROG_5SRC) $(PROG_6SRC) $(PROG_7SRC) $(PROG_8SRC)

# Common flags...
PROG_JOT_LIBS = mesh sps mlib disp geom glew net std dlhandler dev libpng zlib
PROG_LIBS     = $(OGL_LIBS) $(GLUT_LIBS) $(SYS_LIBS)
PROG_LDFLAGS  = $(OGL_FLAGS) $(GLUT_FLAGS)

#
# Program 1 - subdivide 
#
PROG_1         = subdivide
PROG_1SRC      = subdivide.C
PROG_1JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_1LIB      = $(PROG_LIBS)
PROG_1LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 2 - cpmesh
#
# Copies mesh from input to output...
#   can be used to get clean version of mesh
PROG_2         = cpmesh
PROG_2SRC      = cpmesh.C
PROG_2JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_2LIB      = $(PROG_LIBS)
PROG_2LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 3 - fit
#
PROG_3         = fit
PROG_3SRC      = fit.C
PROG_3JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_3LIB      = $(PROG_LIBS)
PROG_3LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 4 - split
#
PROG_4         = split
PROG_4SRC      = split.C
PROG_4JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_4LIB      = $(PROG_LIBS)
PROG_4LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 5 - merge
#
PROG_5         = merge
PROG_5SRC      = merge.C
PROG_5JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_5LIB      = $(PROG_LIBS)
PROG_5LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 6 - reverse
#
PROG_6         = reverse
PROG_6SRC      = reverse.C
PROG_6JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_6LIB      = $(PROG_LIBS)
PROG_6LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 7 - print_mesh
#
PROG_7         = print_mesh
PROG_7SRC      = print_mesh.C
PROG_7JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_7LIB      = $(PROG_LIBS)
PROG_7LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 8 - test_mesh
#
PROG_8         = test_mesh
PROG_8SRC      = test_mesh.C
PROG_8JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_8LIB      = $(PROG_LIBS)
PROG_8LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 9 - test_OBJReader
#
PROG_9         = test_OBJReader
PROG_9SRC      = test_OBJReader.C
PROG_9JOT_LIBS = $(PROG_JOT_LIBS) draw glui
PROG_9LIB      = $(PROG_LIBS)
PROG_9LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 10 - color a mesh
#
PROG_10         = color_mesh
PROG_10SRC      = color_mesh.C
PROG_10JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_10LIB      = $(PROG_LIBS)
PROG_10LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 11 - qinv 
#
PROG_11         = qinv
PROG_11SRC      = qinv.C
PROG_11JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_11LIB      = $(PROG_LIBS)
PROG_11LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 12 - hist_eq
#
PROG_12         = hist_eq
PROG_12SRC      = hist_eq.C
PROG_12JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_12LIB      = $(PROG_LIBS)
PROG_12LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 13 - p2h
#
PROG_13         = p2h
PROG_13SRC      = p2h.C
PROG_13JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_13LIB      = $(PROG_LIBS)
PROG_13LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 14 - p2hatch
#
PROG_14         = p2hatch
PROG_14SRC      = p2hatch.C
PROG_14JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_14LIB      = $(PROG_LIBS)
PROG_14LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 15 - split_patches
#
PROG_15         = split_patches
PROG_15SRC      = split_patches.C
PROG_15JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_15LIB      = $(PROG_LIBS)
PROG_15LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 16 - align
#
PROG_16         = align
PROG_16SRC      = align.C
PROG_16JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_16LIB      = $(PROG_LIBS)
PROG_16LDFLAGS  = $(PROG_LDFLAGS)

#
# Program 17 - dt
#
PROG_17         = dt
PROG_17SRC      = dt.C
PROG_17JOT_LIBS = $(PROG_JOT_LIBS) $(PROG_JOT_LIBS)
PROG_17LIB      = $(PROG_LIBS)
PROG_17LDFLAGS  = $(PROG_LDFLAGS)


LIBRARYCFLAGS =
LIBRARYIFLAGS = $(OGL_INCS)
LIBRARYDFLAGS =

FINALCFLAGS  := $(FINALCFLAGS)

include $(JOT_ROOT)/include/makefiles/std
