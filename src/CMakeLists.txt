
IF(WIN32)
SET(JOTRC jot.rc)
SET(JOTICOS jotapp.ico jotfile.ico jotcmd.ico)
ENDIF(WIN32)

#
# Program 1 - jot
#
ADD_EXECUTABLE(jot jot.C ${JOTRC})
TARGET_LINK_LIBRARIES(jot
	base_jotapp glut_winsys glui widgets tess manip ffs
	pattern triangle npr stroke geom disp gtex mesh net
	dev std gest wnpr map3d mlib dlhandler
	glew sps gui proxy_pattern)
TARGET_LINK_LIBRARIES(jot ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${PNG_LIBRARIES})

#
# Program 2 - smview
#
ADD_EXECUTABLE(smview smview.C ${JOTRC})
TARGET_LINK_LIBRARIES(smview
	base_jotapp glut_winsys glui widgets manip gest geom disp
	gtex mesh sps net npr dev std mlib dlhandler glew)
TARGET_LINK_LIBRARIES(jot ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} ${PNG_LIBRARIES})

INSTALL(TARGETS jot smview DESTINATION bin)

