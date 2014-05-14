#
# jot-lib - C++ libraries for 3D graphics, especially NPR
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(jot C CXX)

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Wno-unused-parameter -Wno-overloaded-virtual")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wno-unused-parameter -Wno-overloaded-virtual")

INCLUDE(${PROJECT_SOURCE_DIR}/cmake/FindCXXFeatures.cmake)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_COMPILER_FLAGS}")
IF(NOT(CXXFeatures_nullptr_FOUND))
	MESSAGE(FATAL_ERROR "jot-lib requires a compiler that supports nullptr.")
ENDIF(NOT(CXXFeatures_nullptr_FOUND))
IF(NOT(CXXFeatures_auto_FOUND))
	MESSAGE(FATAL_ERROR "jot-lib requires a compiler that supports auto.")
ENDIF(NOT(CXXFeatures_auto_FOUND))

## Check for things we need

# gettimeofday include files
INCLUDE(CheckSymbolExists)
CHECK_SYMBOL_EXISTS(gettimeofday sys/time.h HAVE_GETTIMEOFDAY)
IF(HAVE_GETTIMEOFDAY)
ADD_DEFINITIONS("-DHAVE_GETTIMEOFDAY")
ENDIF(HAVE_GETTIMEOFDAY)

# OpenGL, GLUT, and GLEW libraries
INCLUDE(${CMAKE_ROOT}/Modules/FindOpenGL.cmake)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLUT REQUIRED)
INCLUDE(${CMAKE_ROOT}/Modules/FindGLEW.cmake)
FIND_PACKAGE(GLEW REQUIRED)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/glui ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})

# libpng library
FIND_PACKAGE(PNG REQUIRED)

# Coin3D - An Open Inventor implementation
INCLUDE(${CMAKE_ROOT}/Modules/FindCoin3D.cmake)
FIND_PACKAGE(Coin3D)

IF(UNIX)
SET(REF_IMG_32_BIT BOOL True)
ADD_DEFINITIONS("-DGL_GLEXT_PROTOTYPES")
ENDIF(UNIX)

# XXX - Uncomment the following if not using Nvidia hardware
#ADD_DEFINITIONS("-DNON_NVIDIA_GFX")

# Uncomment the following if not using ATI hardware
#ADD_DEFINITIONS("-DNON_ATI_GFX")

# XXX - In order to compile the command-line apps in mesh lib,
#       this hack prevents disp lib from referencing geom lib...
#ADD_DEFINITIONS("-DDONT_LINK_GEOM_IN_DISP")


## Subdirectories

ADD_SUBDIRECTORY(glui)
ADD_SUBDIRECTORY(triangle)
ADD_SUBDIRECTORY(std)
ADD_SUBDIRECTORY(src)

