
ADD_LIBRARY(net
	stream.cpp
	net.cpp
	net_types.cpp
	data_item.cpp
	jotappconfig.cpp
	io_manager.cpp)

TARGET_LINK_LIBRARIES(net
	disp
	dev
	mlib
	std)

ADD_EXECUTABLE(netiotest EXCLUDE_FROM_ALL
	io.cpp
	net.cpp
	stream.cpp
	jotappconfig.cpp
	data_item.cpp)

TARGET_LINK_LIBRARIES(netiotest
	std)

