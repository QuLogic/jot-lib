
#
# Program 1 - m2sm
#
ADD_EXECUTABLE(m2sm m2sm.C)
TARGET_LINK_LIBRARIES(m2sm mesh std)
INSTALL(TARGETS m2sm DESTINATION bin)

#
# Program 2 - ply2sm
#
ADD_EXECUTABLE(ply2sm ply2sm.C ply.C)
TARGET_LINK_LIBRARIES(ply2sm mesh std)
INSTALL(TARGETS ply2sm DESTINATION bin)

# TODO: Check for OpenInventor somehow
IF(FALSE)
SET_PROPERTY(TARGET sm2iv iv2sm
	PROPERTY INCLUDE_DIRECTORIES
	${OPENINVENTOR_INCLUDE_DIR})

#
# Program 3 - sm2iv
#
ADD_EXECUTABLE(sm2iv sm2iv.C)
TARGET_LINK_LIBRARIES(sm2iv mesh)
TARGET_LINK_LIBRARIES(sm2iv ${OPENINVENTOR_LIBRARY})
INSTALL(TARGETS sm2iv DESTINATION bin)

#
# Program 4 - iv2sm (not tested lately...)
#
ADD_EXECUTABLE(iv2sm iv2sm.C)
TARGET_LINK_LIBRARIES(iv2sm dev mesh std)
TARGET_LINK_LIBRARIES(iv2sm ${OPENINVENTOR_LIBRARY})
INSTALL(TARGETS iv2sm DESTINATION bin)
ENDIF(FALSE)

#
# Program 5 - sm2obj
#
ADD_EXECUTABLE(sm2obj sm2obj.C)
TARGET_LINK_LIBRARIES(sm2obj mesh std)
INSTALL(TARGETS sm2obj DESTINATION bin)

#
# Program 6 - obj2sm
#
ADD_EXECUTABLE(obj2sm obj2sm.C)
TARGET_LINK_LIBRARIES(obj2sm mesh std)
INSTALL(TARGETS obj2sm DESTINATION bin)

#
# Program 7 - png2sm
#
ADD_EXECUTABLE(png2sm png2sm.C)
TARGET_LINK_LIBRARIES(png2sm mesh geom std)
INSTALL(TARGETS png2sm DESTINATION bin)

#
# Program 8 - obj2jot
#
ADD_EXECUTABLE(obj2jot obj2jot.C)
TARGET_LINK_LIBRARIES(obj2jot mesh net std)
INSTALL(TARGETS obj2jot DESTINATION bin)

