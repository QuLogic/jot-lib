
ADD_LIBRARY(net
	stream.C
	net.C
	net_types.C
	data_item.C
	jotappconfig.C
	io_manager.C)

TARGET_LINK_LIBRARIES(net
	disp
	dev
	mlib
	std)

