
IF(WIN32)
SET(JOTRC jot.rc)
SET(JOTICOS jotapp.ico jotfile.ico jotcmd.ico)
ENDIF(WIN32)

#
# Program 1 - jot
#
ADD_EXECUTABLE(jot jot.C ${JOTRC})
TARGET_LINK_LIBRARIES(jot
	base_jotapp ffs tess manip widgets npr pattern stroke disp
	geom wnpr gest map3d mlib sps proxy_pattern gtex triangle
	mesh net std dlhandler dev gui glui glut_winsys glew)
TARGET_LINK_LIBRARIES(jot ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${GLEW_LIBRARIES} ${PNG_LIBRARIES})

#
# Program 2 - smview
#
ADD_EXECUTABLE(smview smview.C ${JOTRC})
TARGET_LINK_LIBRARIES(smview
	base_jotapp manip widgets disp geom gest mlib sps gtex
	npr gtex mesh net std dlhandler dev glui glut_winsys glew)
TARGET_LINK_LIBRARIES(smview ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${GLEW_LIBRARIES} ${PNG_LIBRARIES})

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

