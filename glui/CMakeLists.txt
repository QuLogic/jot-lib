
ADD_LIBRARY(glui
	glui.cpp
	glui_add_controls.cpp
	glui_bitmap_img_data.cpp
	glui_bitmaps.cpp
	glui_button.cpp
	glui_checkbox.cpp
	glui_column.cpp
	glui_commandline.cpp
	glui_control.cpp
	glui_edittext.cpp
	glui_filebrowser.cpp
	glui_listbox.cpp
	glui_list.cpp
	glui_mouse_iaction.cpp
	glui_node.cpp
	glui_panel.cpp
	glui_radio.cpp
	glui_rollout.cpp
	glui_rotation.cpp
	glui_scrollbar.cpp
	glui_separator.cpp
	glui_spinner.cpp
	glui_statictext.cpp
	glui_string.cpp
	glui_textbox.cpp
	glui_translation.cpp
	glui_tree.cpp
	glui_treepanel.cpp
	glui_window.cpp
	arcball.cpp
	algebra3.cpp
	quaternion.cpp
	viewmodel.cpp
	# The following four files appear to be custom items.
	glui_activetext.cpp
	glui_bitmapbox.cpp
	glui_graph.cpp
	glui_slider.cpp
)

TARGET_LINK_LIBRARIES(glui
	${GLUT_LIBRARIES}
	${OPENGL_LIBRARIES})

