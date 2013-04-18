
SET(GLUT_JOT_FILES
	glut_8x13.C
	glut_9x15.C
	glut_bitmap.C
	glut_bwidth.C
	glut_cindex.C
	glut_cmap.C
	glut_cursor.C
	glut_dials.C
	glut_dstr.C
	glut_event.C
	glut_ext.C
	glut_fullscrn.C
	glut_gamemode.C
	glut_get.C
	glut_glxext.C
	glut_hel10.C
	glut_hel12.C
	glut_hel18.C
	glut_init.C
	glut_input.C
	glut_joy.C
	glut_key.C
	glut_keyctrl.C
	glut_keyup.C
	glut_mesa.C
	glut_modifier.C
	glut_mroman.C
	glut_overlay.C
	glut_roman.C
	glut_shapes.C
	glut_space.C
	glut_stroke.C
	glut_swap.C
	glut_swidth.C
	glut_tablet.C
	glut_teapot.C
	glut_tr10.C
	glut_tr24.C
	glut_util.C
	glut_vidresize.C
	glut_warp.C
	glut_win.C
	glut_winmisc.C)

IF(WIN32)
SET(GLUT_JOT_FILES ${GLUT_JOT_FILES}
	win32_glx.C
	win32_menu.C
	win32_util.C
	win32_winproc.C
	win32_x11.C)
ENDIF(WIN32)

IF(UNIX)
SET(GLUT_JOT_FILES ${GLUT_JOT_FILES}
	glut_menu.C
	glut_menu2.C
	layerutil.C)
ENDIF(UNIX)

ADD_LIBRARY(glut_jot ${GLUT_JOT_FILES})

SET_PROPERTY(
	SOURCE ${GLUT_JOT_FILES}
	APPEND
	PROPERTY INCLUDE_DIRECTORIES
	${PROJECT_SOURCE_DIR}/glut_jot)

#Compile as C (not C++)
SET_SOURCE_FILES_PROPERTIES(${GLUT_JOT_FILES} PROPERTIES LANGUAGE "C" COMPILE_FLAGS "-x c")

