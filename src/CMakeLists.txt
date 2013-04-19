
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

