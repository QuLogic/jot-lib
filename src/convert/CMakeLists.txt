
#
# Program 1 - m2sm
#
ADD_EXECUTABLE(m2sm m2sm.cpp)
TARGET_LINK_LIBRARIES(m2sm mesh std)
INSTALL(TARGETS m2sm DESTINATION bin)

#
# Program 2 - ply2sm
#
ADD_EXECUTABLE(ply2sm ply2sm.cpp ply.cpp)
TARGET_LINK_LIBRARIES(ply2sm mesh std)
INSTALL(TARGETS ply2sm DESTINATION bin)

#
# Program 3 - sm2iv
#
ADD_EXECUTABLE(sm2iv sm2iv.cpp)
TARGET_LINK_LIBRARIES(sm2iv mesh)
INSTALL(TARGETS sm2iv DESTINATION bin)

IF(COIN3D_FOUND)

#
# Program 4 - iv2sm (not tested lately...)
#
ADD_EXECUTABLE(iv2sm iv2sm.cpp)
TARGET_LINK_LIBRARIES(iv2sm dev mesh std)
TARGET_LINK_LIBRARIES(iv2sm ${COIN3D_LIBRARIES})
INSTALL(TARGETS iv2sm DESTINATION bin)

SET_PROPERTY(TARGET iv2sm
	APPEND
	PROPERTY INCLUDE_DIRECTORIES
	${COIN3D_INCLUDE_DIRS})

ENDIF(COIN3D_FOUND)

#
# Program 5 - sm2obj
#
ADD_EXECUTABLE(sm2obj sm2obj.cpp)
TARGET_LINK_LIBRARIES(sm2obj mesh std)
INSTALL(TARGETS sm2obj DESTINATION bin)

#
# Program 6 - obj2sm
#
ADD_EXECUTABLE(obj2sm obj2sm.cpp)
TARGET_LINK_LIBRARIES(obj2sm mesh std)
INSTALL(TARGETS obj2sm DESTINATION bin)

#
# Program 7 - png2sm
#
ADD_EXECUTABLE(png2sm png2sm.cpp)
TARGET_LINK_LIBRARIES(png2sm mesh geom std)
INSTALL(TARGETS png2sm DESTINATION bin)

#
# Program 8 - obj2jot
#
ADD_EXECUTABLE(obj2jot obj2jot.cpp)
TARGET_LINK_LIBRARIES(obj2jot mesh net std)
INSTALL(TARGETS obj2jot DESTINATION bin)

