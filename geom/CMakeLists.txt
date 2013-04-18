
SET(GEOM_FILES
	geom.C
	gl_view.C
	fsa.C
	text2d.C
	icon2d.C
	line3d.C
	command.C
	world.C
	texture.C
	image.C
	hspline.C
	appear.C
	texturegl.C
	distrib.C
	recorder_ui.C
	file_listbox.C)

ADD_LIBRARY(geom ${GEOM_FILES})

SET_PROPERTY(
	SOURCE ${GEOM_FILES}
	APPEND
	PROPERTY INCLUDE_DIRECTORIES
	${PNG_INCLUDE_DIRS})

