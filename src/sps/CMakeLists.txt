
ADD_LIBRARY(sps
	sps.C)

TARGET_LINK_LIBRARIES(sps
	base_jotapp
	gtex
	mesh)

ADD_EXECUTABLE(sps_test test_app.C)
TARGET_LINK_LIBRARIES(sps_test sps)

