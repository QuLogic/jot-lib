
IF(WIN32)
SET(JOTRC jot.rc)
SET(JOTICOS jotapp.ico jotfile.ico jotcmd.ico)
ENDIF(WIN32)

#
# Program 1 - jot
#
ADD_EXECUTABLE(jot jot.C ${JOTRC})
TARGET_LINK_LIBRARIES(jot
	base_jotapp
	dev
	disp
	ffs
	geom
	gest
	gtex
	gui
	manip
	mesh
	mlib
	npr
	pattern
	proxy_pattern
	tess
	widgets
	wnpr
	std)

#
# Program 2 - smview
#
ADD_EXECUTABLE(smview smview.C ${JOTRC})
TARGET_LINK_LIBRARIES(smview
	base_jotapp
	disp
	geom
	gtex
	manip
	mesh
	mlib
	widgets
	std
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

INSTALL(TARGETS jot smview DESTINATION bin)

ADD_SUBDIRECTORY(base_jotapp)
ADD_SUBDIRECTORY(convert)
ADD_SUBDIRECTORY(dev)
ADD_SUBDIRECTORY(disp)
ADD_SUBDIRECTORY(dlhandler)
ADD_SUBDIRECTORY(ffs)
ADD_SUBDIRECTORY(geom)
ADD_SUBDIRECTORY(gest)
ADD_SUBDIRECTORY(glew)
ADD_SUBDIRECTORY(glut_winsys)
ADD_SUBDIRECTORY(gtex)
ADD_SUBDIRECTORY(gui)
ADD_SUBDIRECTORY(manip)
ADD_SUBDIRECTORY(map3d)
ADD_SUBDIRECTORY(mesh)
ADD_SUBDIRECTORY(mlib)
ADD_SUBDIRECTORY(net)
ADD_SUBDIRECTORY(npr)
ADD_SUBDIRECTORY(pattern)
ADD_SUBDIRECTORY(proxy_pattern)
ADD_SUBDIRECTORY(sps)
ADD_SUBDIRECTORY(stroke)
ADD_SUBDIRECTORY(tess)
ADD_SUBDIRECTORY(widgets)
ADD_SUBDIRECTORY(wnpr)

