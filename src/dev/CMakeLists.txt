
ADD_LIBRARY(dev
	dev.cpp
	tty.cpp
	tablet.cpp)

TARGET_LINK_LIBRARIES(dev
	mlib
	std)

