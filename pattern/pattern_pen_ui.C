/*****************************************************************
 * This file is part of jot-lib (or "jot" for short):
 *   <http://code.google.com/p/jot-lib/>
 * 
 * jot-lib is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * jot-lib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.`
 *****************************************************************/
/***************************************************************************
    pattern_pen_ui.C
    
    PatternPenUI 
        -GLUI interface for PatternPen       
    -------------------
    Simon Breslav
    Fall 2004
 ***************************************************************************/

#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
#pragma warning(disable: 4786)
#endif

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "glui/glui.h" 
#include "base_jotapp/base_jotapp.H"
#include "stroke/base_stroke.H"

#include <map>

#include "pattern_pen.H"
#include "pattern_pen_ui.H"

using mlib::Wvec;
using mlib::Wtransf;

#define PRESET_DIRECTORY         "nprdata/pattern_stroke_presets/"
#define ID_SHIFT                 10
#define ID_MASK                  ((1<<ID_SHIFT)-1)

bool PatternPenUI::use_image_pressure = 0;
bool PatternPenUI::use_width = 0;
bool PatternPenUI::use_alpha = 0;
bool PatternPenUI::use_image_color = 0;  
bool PatternPenUI::show_boundery_box = 1;
PatternPenUI::lum_fun_id_t PatternPenUI::lum_function = LUM_SHADOW;

ARRAY<PatternPenUI*> PatternPenUI::_ui;

PatternPenUI::PatternPenUI(PatternPen* pen) :
   _glui(0),
   _pen(pen),
   _init(false),
   _current_mode(0),
   _light(0,0,1) 
{
   _ui += this;
   _id = (_ui.num()-1);
   _stroke =  new BaseStroke();
  
}
PatternPenUI::~PatternPenUI()
{
	delete _stroke;
}
void
PatternPenUI::show()
{
   if (!_glui) {   
      build();     

      if (_glui) {
         _glui->show();
         // Update the controls that don't use
         // 'live' variables
         //update_non_lives();         
         _glui->sync_live();
         
         VIEW::peek()->light_set_enable(0,true);
         VIEW::peek()->light_set_enable(1,true);
         VIEW::peek()->light_set_enable(2,true);
         VIEW::peek()->light_set_enable(3,true);
         /*
         VIEW::peek()->light_set_in_cam_space(0, false);
         VIEW::peek()->light_set_in_cam_space(1, false);
         VIEW::peek()->light_set_in_cam_space(2, false);
         VIEW::peek()->light_set_in_cam_space(3, false);
         */
      } else {
         err_mesg(ERR_LEV_ERROR, "PatternPenUI::show() - Error!");        
      }              
   } 
}

void
PatternPenUI::hide()
{  
   if (!_glui) {
      err_mesg(ERR_LEV_ERROR, "PatternPenUI::hide() - Error!");      
   } else {
      _glui->hide();
      destroy();
      assert(!_glui); 
   }
}

void
PatternPenUI::update()
{
   if(!_glui) {
      err_mesg(ERR_LEV_ERROR, "PatternPenUI::update() - Error! ");      
   } else {
      err_msg("PatternPenUI::update()");         
      
//       update_non_lives();
     
      _glui->sync_live();
   }
}



void
PatternPenUI::init() {
   _init = true; 
}



void
PatternPenUI::destroy() {
  _button.clear();
  _glui->close();
  _glui = NULL;
}


void
PatternPenUI::build() {
   int i;
   int id = _id << ID_SHIFT;

   assert(!_glui);

   int root_x, root_y, root_w, root_h;
   _pen->view()->win()->size(root_w,root_h);
   _pen->view()->win()->position(root_x,root_y);

   _glui = GLUI_Master.create_glui("Pattern Editor", 0, root_x+root_w+10, root_y);
   _glui->set_main_gfx_window(_pen->view()->win()->id());
   
   //Init the control arrays   
   for (i=0; i<BUT_NUM; i++)      _button.add(0);
   for (i=0; i<PANEL_NUM; i++)    _panel.add(0);
   for (i=0; i<LIST_NUM; i++)     _listbox.add(0);    
   for (i=0; i<CHECK_NUM; i++)    _checkbox.add(0);
   for (i=0; i<SLIDE_NUM; i++)    _slider.add(0);
   for (i=0; i<ROLLOUT_NUM; i++)  _rollout.add(0);
   for (i=0; i<ROT_NUM; i++)      _rotation.add(0);
   for (i=0; i<RADGROUP_NUM; i++) _radgroup.add(0);
   for (i=0; i<RADBUT_NUM; i++)   _radbutton.add(0);


   // Panel containing pen buttons
   _panel[PANEL_PEN] = _glui->add_panel("");
   assert(_panel[PANEL_PEN]);

   //Prev pen
   _button[BUT_PREV_PEN] = _glui->add_button_to_panel(
      _panel[PANEL_PEN], "Previous Mode", 
      id+BUT_PREV_PEN, button_cb);
   assert(_button[BUT_PREV_PEN]);

   _glui->add_column_to_panel(_panel[PANEL_PEN],false);

   //Prev pen
   _button[BUT_NEXT_PEN] = _glui->add_button_to_panel(
      _panel[PANEL_PEN], "Next Mode", 
      id+BUT_NEXT_PEN, button_cb);
   assert(_button[BUT_NEXT_PEN]);

   _panel[PANEL_MODE] = _glui->add_panel("");

    // stroke pattern modes
   _listbox[LIST_MODE]= _glui->add_listbox_to_panel(_panel[PANEL_MODE],"Mode", NULL,
					  id+LIST_MODE, listbox_cb);
   _listbox[LIST_MODE]->add_item(0, "Analysis");
   _listbox[LIST_MODE]->add_item(1, "Synthesis");
   _listbox[LIST_MODE]->add_item(2, "Path");
   _listbox[LIST_MODE]->add_item(3, "Proxy");
   _listbox[LIST_MODE]->add_item(4, "Ellipses");

    _glui->add_column_to_panel(_panel[PANEL_MODE],false);

   _listbox[LIST_STROKE_PRESET]= _glui->add_listbox_to_panel(_panel[PANEL_MODE],"Preset", NULL, id+LIST_STROKE_PRESET, listbox_cb);  
   fill_preset_listbox(_listbox[LIST_STROKE_PRESET], _preset_filenames, Config::JOT_ROOT() + PRESET_DIRECTORY);
   preset_stroke();

  

   // stroke mode
   _rollout[ROLLOUT_ANALYSIS] =  _glui->add_rollout("Analysis",true);
   
   _panel[PANEL_STROKES] = _glui->add_panel_to_panel(_rollout[ROLLOUT_ANALYSIS], "");
   
   _slider[SLIDE_EPS] = _glui->add_slider_to_panel(_panel[PANEL_STROKES], "Epsilon", 
						   GLUI_SLIDER_FLOAT, 0, 20, NULL, 
						   id+SLIDE_EPS, slider_cb);
   _slider[SLIDE_EPS]->set_num_graduations(101);
   _slider[SLIDE_EPS]->set_float_val(0.0f);
   _button[BUT_POP] = _glui->add_button_to_panel(_panel[PANEL_STROKES], "Pop Stroke", 
						 id+BUT_POP, button_cb);
   _button[BUT_CLEAR] = _glui->add_button_to_panel(_panel[PANEL_STROKES], "Clear Strokes", 
						   id+BUT_CLEAR, button_cb);

  
   _glui->add_column_to_panel(_panel[PANEL_STROKES],false);


   _listbox[LIST_TYPE]= _glui->add_listbox_to_panel(_panel[PANEL_STROKES],
						     "Type", NULL,
						     id+LIST_TYPE, listbox_cb);  
   _listbox[LIST_TYPE]->add_item(0, "Hatching");
   _listbox[LIST_TYPE]->add_item(1, "Stippling");
   _listbox[LIST_TYPE]->add_item(2, "Free");
   _listbox[LIST_TYPE]->set_int_val(2);

   _listbox[LIST_PARAM]= _glui->add_listbox_to_panel(_panel[PANEL_STROKES],
						     "Param.", NULL,
						     id+LIST_PARAM, listbox_cb);  
   _listbox[LIST_PARAM]->add_item(0, "Axis");
   _listbox[LIST_PARAM]->add_item(1, "Cartesian");
   _listbox[LIST_PARAM]->add_item(2, "Angular");

   _checkbox[CHECK_ANAL_STYLE] = _glui->add_checkbox_to_panel(_panel[PANEL_STROKES],
							      "Analyze style", NULL,
							      id+CHECK_ANAL_STYLE, checkbox_cb);  
   _checkbox[CHECK_ANAL_STYLE]->set_int_val(1);

   _slider[SLIDE_STYLE] = _glui->add_slider_to_panel(_panel[PANEL_STROKES], "Style adjust.", 
						   GLUI_SLIDER_FLOAT, 0, 1, NULL, 
						   id+SLIDE_STYLE, slider_cb);
   _slider[SLIDE_STYLE]->set_num_graduations(101);
   _slider[SLIDE_STYLE]->set_float_val(1.0f);

   _button[BUT_NEW_GROUP] = _glui->add_button_to_panel(_panel[PANEL_STROKES], "New Group", 
						       id+BUT_NEW_GROUP, button_cb);
   
   _checkbox[CHECK_STRUCT] = _glui->add_checkbox_to_panel(_panel[PANEL_STROKES],
							  "Display structure", NULL,
							  id+CHECK_STRUCT, checkbox_cb);  
   _checkbox[CHECK_FRAME] = _glui->add_checkbox_to_panel(_panel[PANEL_STROKES],
							 "Display ref frame", NULL,
							 id+CHECK_FRAME, checkbox_cb);  
   _checkbox[CHECK_FRAME]->set_int_val(1);
   

   // synthesis mode
   _rollout[ROLLOUT_SYNTH] =  _glui->add_rollout("Synthesis",false);
   _listbox[LIST_CELL]= _glui->add_listbox_to_panel(_rollout[ROLLOUT_SYNTH],
						    "Cell", NULL, id+LIST_CELL, listbox_cb);  
   _listbox[LIST_CELL]->add_item(0, "Box");
   _listbox[LIST_CELL]->add_item(1, "Rect");
   _listbox[LIST_CELL]->add_item(2, "Path");
   _listbox[LIST_CELL]->add_item(3, "Carriers");

   _panel[PANEL_IMAGE] = _glui->add_panel_to_panel(_rollout[ROLLOUT_SYNTH], "Back Image");
   assert(_panel[PANEL_IMAGE]);
   _checkbox[CHECK_IMAGE_PRESSURE] = _glui->add_checkbox_to_panel(_panel[PANEL_IMAGE],
							  "Use Image Luminasity", NULL,
							  id+CHECK_IMAGE_PRESSURE, checkbox_cb); 

   _checkbox[CHECK_IMAGE_ALPHA] = _glui->add_checkbox_to_panel(_panel[PANEL_IMAGE],
							  "Modify Alpha", NULL,
							  id+CHECK_IMAGE_ALPHA, checkbox_cb); 
   _checkbox[CHECK_IMAGE_ALPHA]->set_int_val(_pen->get_gesture_drawer_a()); 

   _checkbox[CHECK_IMAGE_WIDTH] = _glui->add_checkbox_to_panel(_panel[PANEL_IMAGE],
							  "Modify Width", NULL,
							  id+CHECK_IMAGE_WIDTH, checkbox_cb);
   _checkbox[CHECK_IMAGE_WIDTH]->set_int_val(_pen->get_gesture_drawer_w()); 
     
   _checkbox[CHECK_IMAGE_COLOR] = _glui->add_checkbox_to_panel(_panel[PANEL_IMAGE],
							  "Use Color", NULL,
							  id+CHECK_IMAGE_COLOR, checkbox_cb);  


   _checkbox[CHECK_SHOW_BBOX] = _glui->add_checkbox_to_panel(_rollout[ROLLOUT_SYNTH],
							  "Show cell", NULL,
							  id+CHECK_SHOW_BBOX, checkbox_cb); 
    _checkbox[CHECK_SHOW_BBOX]->set_int_val(1);

    _checkbox[CHECK_SHOW_ICON] = _glui->add_checkbox_to_panel(_rollout[ROLLOUT_SYNTH],
							  "Show example", NULL,
							  id+CHECK_SHOW_ICON, checkbox_cb);  
    _checkbox[CHECK_SHOW_ICON]->set_int_val(1);


    _panel[PANEL_COLOR] = _glui->add_panel_to_panel(_rollout[ROLLOUT_SYNTH], "Image Color Adjust");

   	_slider[SLIDE_COLOR_H] = _glui->add_slider_to_panel(_panel[PANEL_COLOR], "H Adjust", 
						   GLUI_SLIDER_FLOAT, -1, 1, NULL, 
						   id+SLIDE_COLOR_H, slider_cb);
    _slider[SLIDE_COLOR_H]->set_num_graduations(21);
    _slider[SLIDE_COLOR_H]->set_float_val(0.0f);
    _slider[SLIDE_COLOR_H]->set_w(120);

	_slider[SLIDE_COLOR_S] = _glui->add_slider_to_panel(_panel[PANEL_COLOR], "S Adjust", 
						   GLUI_SLIDER_FLOAT, -1, 1, NULL, 
						   id+SLIDE_COLOR_S, slider_cb);
    _slider[SLIDE_COLOR_S]->set_num_graduations(21);
    _slider[SLIDE_COLOR_S]->set_float_val(0.0f);
    _slider[SLIDE_COLOR_S]->set_w(120);

	_slider[SLIDE_COLOR_V] = _glui->add_slider_to_panel(_panel[PANEL_COLOR], "V Adjust", 
						   GLUI_SLIDER_FLOAT, -1, 1, NULL, 
						   id+SLIDE_COLOR_V, slider_cb);
    _slider[SLIDE_COLOR_V]->set_num_graduations(21);
    _slider[SLIDE_COLOR_V]->set_float_val(0.0f);
    _slider[SLIDE_COLOR_V]->set_w(120);

	_listbox[LIST_LUMIN_FUNC]= _glui->add_listbox_to_panel(_panel[PANEL_COLOR],
						   "Lum Func", NULL, id+LIST_LUMIN_FUNC, listbox_cb);  
    _listbox[LIST_LUMIN_FUNC]->add_item(0, "Shadow");
    _listbox[LIST_LUMIN_FUNC]->add_item(1, "Highlight");

   _glui->add_column_to_panel(_rollout[ROLLOUT_SYNTH],false);

   _listbox[LIST_SYNTH]= _glui->add_listbox_to_panel(_rollout[ROLLOUT_SYNTH],
						   "Mode", NULL, id+LIST_SYNTH, listbox_cb);  
   _listbox[LIST_SYNTH]->add_item(0, "synth_mimic");
   _listbox[LIST_SYNTH]->add_item(1, "synth_Efros");
   _listbox[LIST_SYNTH]->add_item(2, "synth_sample");
   _listbox[LIST_SYNTH]->add_item(3, "synth_copy");
   _listbox[LIST_SYNTH]->add_item(4, "synth_clone");

   _listbox[LIST_DIST]= _glui->add_listbox_to_panel(_rollout[ROLLOUT_SYNTH],
						    "Distribution", NULL, id+LIST_DIST, listbox_cb);  
   _listbox[LIST_DIST]->add_item(0, "Lloyd");
   _listbox[LIST_DIST]->add_item(1, "Stratified");

   _slider[SLIDE_RING] = _glui->add_slider_to_panel(_rollout[ROLLOUT_SYNTH], "#Ring", 
						    GLUI_SLIDER_INT, 1, 5, NULL, 
						    id+SLIDE_RING, slider_cb);

   _button[BUT_POP_SYNTH] = _glui->add_button_to_panel(_rollout[ROLLOUT_SYNTH], "Pop Cell", 
						 id+BUT_POP_SYNTH, button_cb);
   _button[BUT_CLEAR_SYNTH] = _glui->add_button_to_panel(_rollout[ROLLOUT_SYNTH], "Clear All", 
						   id+BUT_CLEAR_SYNTH, button_cb);

   
   _checkbox[CHECK_STRETCH] = _glui->add_checkbox_to_panel(_rollout[ROLLOUT_SYNTH],
							  "Element Stretching", NULL,
							  id+CHECK_STRETCH, checkbox_cb);  
  

   _slider[SLIDE_CORRECT] = _glui->add_slider_to_panel(_rollout[ROLLOUT_SYNTH], "Perceptual correction", 
						       GLUI_SLIDER_FLOAT, 0, 1, NULL, 
						       id+SLIDE_CORRECT, slider_cb);
   _slider[SLIDE_CORRECT]->set_num_graduations(200);
   _slider[SLIDE_CORRECT]->set_float_val(1.0f);
   _slider[SLIDE_CORRECT]->set_w(150);

   _slider[SLIDE_GLOBAL_SCALE] = _glui->add_slider_to_panel(_rollout[ROLLOUT_SYNTH], "Global Scale", 
						   GLUI_SLIDER_FLOAT, 0, 2, NULL, 
						   id+SLIDE_GLOBAL_SCALE, slider_cb);
   _slider[SLIDE_GLOBAL_SCALE]->set_num_graduations(21);
   _slider[SLIDE_GLOBAL_SCALE]->set_float_val(1.0f);
   _slider[SLIDE_GLOBAL_SCALE]->set_w(150);


   _button[BUT_RESYNTH] = _glui->add_button_to_panel(
      _rollout[ROLLOUT_SYNTH], "Resynthesize", 
      id+BUT_RESYNTH, button_cb);
   assert(_button[BUT_RESYNTH]);


   

   _rollout[ROLLOUT_SYNTH]->disable();


      
   // Path mode
   _rollout[ROLLOUT_PATH] =  _glui->add_rollout("Path",false);
   _rollout[ROLLOUT_PATH]->disable();

      
   // Proxy mode
   _rollout[ROLLOUT_PROXY] =  _glui->add_rollout("Proxy",false);
   _rollout[ROLLOUT_PROXY]->disable();

      
   // Ellipses mode
   _rollout[ROLLOUT_ELLIPSE] =  _glui->add_rollout("Ellipses",false);
   _rollout[ROLLOUT_ELLIPSE]->disable();

 

}








/////////////////////////
// Callback functions
/////////////////////////

void
PatternPenUI::button_cb(int id) {    
  switch(id&ID_MASK) {
  case BUT_NEXT_PEN:
    BaseJOTapp::instance()->next_pen(); 
    break;
  case BUT_PREV_PEN:
    BaseJOTapp::instance()->prev_pen();
    break;
  case BUT_POP:
    _ui[id >> ID_SHIFT]->pop_stroke();
    break; 
  case BUT_CLEAR:
    _ui[id >> ID_SHIFT]->clear_strokes();
    break; 
  case BUT_NEW_GROUP:
    _ui[id >> ID_SHIFT]->create_new_group();
    break; 
  case BUT_POP_SYNTH:
    _ui[id >> ID_SHIFT]->pen()->pop_synthesized();
    break;
  case BUT_CLEAR_SYNTH:
    _ui[id >> ID_SHIFT]->pen()->clear_synthesized();
    break;
  case BUT_RESYNTH:
    _ui[id >> ID_SHIFT]->pen()->resynthesize();
    break;
 }	
}


void
PatternPenUI::listbox_cb(int id) {
 
  switch(id&ID_MASK) {
  case LIST_MODE:
    _ui[id >> ID_SHIFT]->change_mode();
    break;
  case LIST_TYPE:
    _ui[id >> ID_SHIFT]->create_new_group();
    break;
  case LIST_PARAM:
    _ui[id >> ID_SHIFT]->create_new_group();
    break;
  case LIST_CELL:
    _ui[id >> ID_SHIFT]->set_cell_type();
    break;
  case LIST_SYNTH:
    _ui[id >> ID_SHIFT]->set_synth_mode();
    break;
  case LIST_DIST:
    _ui[id >> ID_SHIFT]->set_distribution();
    break;
  case LIST_STROKE_PRESET:
	_ui[id >> ID_SHIFT]->preset_stroke();
    _ui[id >> ID_SHIFT]->pen()->set_gesture_drawer_base(_ui[id >> ID_SHIFT]->_stroke);
	_ui[id >> ID_SHIFT]->_checkbox[CHECK_IMAGE_WIDTH]->set_int_val(_ui[id >> ID_SHIFT]->_pen->get_gesture_drawer_w()); 
	_ui[id >> ID_SHIFT]->_checkbox[CHECK_IMAGE_ALPHA]->set_int_val(_ui[id >> ID_SHIFT]->_pen->get_gesture_drawer_a()); 
    break;   
  case LIST_LUMIN_FUNC:
    PatternPenUI::lum_function = (lum_fun_id_t)_ui[id >> ID_SHIFT]->_listbox[LIST_LUMIN_FUNC]->get_int_val();
	break;
  }
   
	
}

void 
PatternPenUI::checkbox_cb(int id) {	
  switch(id&ID_MASK) {
  case CHECK_STRUCT:
    _ui[id >> ID_SHIFT]->display_structure();
    break;
  case CHECK_ANAL_STYLE:
    _ui[id >> ID_SHIFT]->set_analyze_style();
    break;
  case CHECK_FRAME:
    _ui[id >> ID_SHIFT]->display_reference_frame();
    break;
  case CHECK_IMAGE_PRESSURE:
	  PatternPenUI::use_image_pressure = bool(_ui[id >> ID_SHIFT]->_checkbox[CHECK_IMAGE_PRESSURE]->get_int_val());
	  break;
  case CHECK_IMAGE_WIDTH:
	  PatternPenUI::use_width = bool(_ui[id >> ID_SHIFT]->_checkbox[CHECK_IMAGE_WIDTH]->get_int_val());
	  _ui[id >> ID_SHIFT]->_pen->set_gesture_drawer_w(_ui[id >> ID_SHIFT]->_checkbox[CHECK_IMAGE_WIDTH]->get_int_val());
	  break;
  case CHECK_IMAGE_ALPHA:
	  PatternPenUI::use_alpha = bool(_ui[id >> ID_SHIFT]->_checkbox[CHECK_IMAGE_ALPHA]->get_int_val());
	  _ui[id >> ID_SHIFT]->_pen->set_gesture_drawer_a(_ui[id >> ID_SHIFT]->_checkbox[CHECK_IMAGE_ALPHA]->get_int_val());
	  break;
  case CHECK_IMAGE_COLOR:
	  PatternPenUI::use_image_color = bool(_ui[id >> ID_SHIFT]->_checkbox[CHECK_IMAGE_COLOR]->get_int_val());
	  break;
  case CHECK_SHOW_BBOX:
	  PatternPenUI::show_boundery_box = bool(_ui[id >> ID_SHIFT]->_checkbox[CHECK_SHOW_BBOX]->get_int_val());
	  break;
  case CHECK_SHOW_ICON:
    _ui[id >> ID_SHIFT]->show_example_strokes();
	  break;
  case CHECK_STRETCH:
    _ui[id >> ID_SHIFT]->set_stretching_state();
    break;
 }
}

void 
PatternPenUI::slider_cb(int id) {  
  assert((id >> ID_SHIFT) < _ui.num());

  switch(id&ID_MASK){
  case SLIDE_EPS:
    _ui[id >> ID_SHIFT]->set_epsilon();
    break;
  case SLIDE_STYLE:
    _ui[id >> ID_SHIFT]->set_style_adjust();
    break;
  case SLIDE_RING:
    _ui[id >> ID_SHIFT]->set_ring_nb();
    break;
  case SLIDE_CORRECT:
    _ui[id >> ID_SHIFT]->set_correction_amount();
    break;
  case SLIDE_GLOBAL_SCALE:
    _ui[id >> ID_SHIFT]->set_global_scale();
    break;
  case SLIDE_COLOR_H:  
  case SLIDE_COLOR_S:  
  case SLIDE_COLOR_V:  
	_ui[id >> ID_SHIFT]->apply_color_adjust();
	_ui[id >> ID_SHIFT]->pen()->rerender_target_cell();

	break;
  }
}

void 
PatternPenUI::rotation_cb(int id) {  
}

void
PatternPenUI::radiogroup_cb(int id) {
  assert((id >> ID_SHIFT) < _ui.num());

//  switch(id&ID_MASK){
//  }

}   


///////////
// Modes
///////////

void 
PatternPenUI::change_mode(){
  // Then update UI
  for (int i=0 ; i<ROLLOUT_NUM ; i++){
    _rollout[i]->close();
    _rollout[i]->disable();
  }
  _current_mode = _listbox[LIST_MODE]->get_int_val();
  _rollout[_current_mode]->open();
  _rollout[_current_mode]->enable();
  
  // Then change pen behavior
  _pen->change_mode(_current_mode);

}







////////////////////////
// Analysis access
////////////////////////


void
PatternPenUI::set_epsilon(){
  double eps = (double) _slider[SLIDE_EPS]->get_float_val();
  _pen->set_epsilon(eps);
}

void
PatternPenUI::set_style_adjust(){
  double sa = (double) _slider[SLIDE_STYLE]->get_float_val();
  _pen->set_style_adjust(sa);
}

void
PatternPenUI::pop_stroke(){
  _pen->pop_stroke();
}

void
PatternPenUI::clear_strokes(){
  _pen->clear_strokes();
}

void 
PatternPenUI::set_reference_frame(){
  int ref_frame = _listbox[LIST_PARAM]->get_int_val();
  _pen->set_ref_frame(ref_frame);
}


void 
PatternPenUI::set_analyze_style(){
  bool b = bool(_checkbox[CHECK_ANAL_STYLE]->get_int_val());
  _pen->set_analyze_style(b);
}

void 
PatternPenUI::create_new_group(){
  int type = _listbox[LIST_TYPE]->get_int_val();
  int ref_param = _listbox[LIST_PARAM]->get_int_val();
  _pen->create_new_group(type, ref_param);
}


void 
PatternPenUI::display_structure(){
  bool disp = bool(_checkbox[CHECK_STRUCT]->get_int_val());
  _pen->display_structure(disp);
}


void 
PatternPenUI::display_reference_frame(){
  bool disp = bool(_checkbox[CHECK_FRAME]->get_int_val());
  _pen->display_reference_frame(disp);
}


void 
PatternPenUI::show_example_strokes(){
  bool show = (bool) _checkbox[CHECK_SHOW_ICON]->get_int_val();
  _pen->show_example_strokes(show);
}









////////////////////////
// Synthesis access
////////////////////////


void 
PatternPenUI::set_cell_type(){
  int current_type = _listbox[LIST_CELL]->get_int_val();
  _pen->set_current_cell_type(current_type);
}

void 
PatternPenUI::set_global_scale(){
  double current_scale = (double)_slider[SLIDE_GLOBAL_SCALE]->get_float_val();
  _pen->set_current_global_scale(current_scale);
}


void 
PatternPenUI::set_synth_mode(){
  int current_mode = _listbox[LIST_SYNTH]->get_int_val();
  _pen->set_current_synth_mode(current_mode);
}


void 
PatternPenUI::set_distribution(){
  int current_distrib = _listbox[LIST_DIST]->get_int_val();
  _pen->set_current_distribution(current_distrib);
}


void 
PatternPenUI::set_stretching_state(){
  bool current_state = bool(_checkbox[CHECK_STRETCH]->get_int_val());
  _pen->set_current_stretching_state(current_state);
}


void 
PatternPenUI::set_ring_nb(){
  int current_nb = _slider[SLIDE_RING]->get_int_val();
  _pen->set_current_ring_nb(current_nb);
}

void 
PatternPenUI::set_correction_amount(){
  double current_amount = _slider[SLIDE_CORRECT]->get_float_val();
  _pen->set_current_correction(current_amount);
}


void     
PatternPenUI::apply_color_adjust()
{
	HSVCOLOR tmp(_slider[SLIDE_COLOR_H]->get_float_val(), 
		         _slider[SLIDE_COLOR_S]->get_float_val(), 
				 _slider[SLIDE_COLOR_V]->get_float_val());
	_pen->set_color_adjust(tmp);
}


void
PatternPenUI::preset_stroke()
{
   int val = _listbox[LIST_STROKE_PRESET]->get_int_val();
   if (!load_preset(**_preset_filenames[val-1]))
        return;
   _glui->sync_live();
} 

void
PatternPenUI::fill_preset_listbox(
   GLUI_Listbox *listbox,
   str_list     &save_files,
   Cstr_ptr     &full_path
   )
{
   int i;

   //First clear out any previous presets
   for (i=1; i<=save_files.num();i++)
   {
      if(listbox)
        listbox->delete_item(i);    
   }
   save_files.clear();

   str_list in_files = dir_list(full_path);
   for (i = 0; i < in_files.num(); i++) {
      int len = in_files[i].len();

      if ( (len>3) && (strcmp(&(**in_files[i])[len-4],".pre") == 0))
      {
         char *basename = new char[len+1];
         assert(basename);
         strcpy(basename,**in_files[i]);
         basename[len-4] = 0;

         if ( listbox->check_item_fit(basename) == 1)
         {
            save_files += full_path + in_files[i];
            listbox->add_item(save_files.num(), basename);
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "PatternPenUI::fill_preset_listbox - Discarding preset file (name too long for listbox): %s", basename);
         }
         delete basename;
      }
      else if (in_files[i] != "CVS")
      {
         err_mesg(ERR_LEV_WARN, "PatternPenUI::fill_preset_listbox - Discarding preset file (bad name): %s", **in_files[i]);
      }
   }
}

bool
PatternPenUI::load_preset(const char *f)
{
   fstream fin;
   fin.open(f, ios::in);

   if (!fin) {
      err_mesg(ERR_LEV_ERROR,"PatternPenUI::load_preset() - Error! Could not open file: '%s'", f);
      return false;
   }  
   STDdstream d(&fin);
   str_ptr str;
   d >> str;
   
   if ((str == BaseStroke::static_name())) {
      //In case we're testing offsets, let
      //trash the old ones
      _stroke->set_offsets(NULL);
      _stroke->decode(d);
	  cerr << "Stroke Preset Loaded" << endl;
   } else {
      err_mesg(ERR_LEV_ERROR,"PatternPenUI::load_preset() - Error! Not BaseStroke: '%s'", **str);
   }
   fin.close();
   return true;
}

double 
PatternPenUI::luminance_function(double l)
{
	switch (PatternPenUI::lum_function)
	{
		case LUM_SHADOW:
			return l;
		case LUM_HIGHLIGHT:
			return (1.0 - l);
		default:
			return l;
	}		
}


//////////////////////////////////////////////////
///////////////////////////
//////////////
////////
///


//////////////////////////////////////
// OLD VERSION
//////////////////////////////////////

/*
void
PatternPenUI::build()
{
   int i;
   int id = _id << ID_SHIFT;

   assert(!_glui);

   int root_x, root_y, root_w, root_h;
   _pen->view()->win()->size(root_w,root_h);
   _pen->view()->win()->position(root_x,root_y);

   _glui = GLUI_Master.create_glui("Pattern Editor", 0, root_x+root_w+10, root_y);
   _glui->set_main_gfx_window(_pen->view()->win()->id());
   
   //Init the control arrays   
   for (i=0; i<BUT_NUM; i++)      _button.add(0);
   for (i=0; i<PANEL_NUM; i++)    _panel.add(0);
   for (i=0; i<LIST_NUM; i++)     _listbox.add(0);    
   for (i=0; i<CHECK_NUM; i++)    _checkbox.add(0);
   for (i=0; i<SLIDE_NUM; i++)    _slider.add(0);
   for (i=0; i<ROLLOUT_NUM; i++)  _rollout.add(0);
   for (i=0; i<ROT_NUM; i++)      _rotation.add(0);
   for (i=0; i<RADGROUP_NUM; i++) _radgroup.add(0);
   for (i=0; i<RADBUT_NUM; i++)   _radbutton.add(0);
      
      _button[BUT_VIEW_MODE] =   _glui->add_button("View Mode", id+BUT_VIEW_MODE, button_cb);
   
   
      _listbox[LIST_BASE_COLOR]= _glui->add_listbox(
                                 "Base Color", NULL,
                                 id+LIST_BASE_COLOR, listbox_cb);                             
      fill_color_listbox(_listbox[LIST_BASE_COLOR]);   
   
   // Layer Panel
   _panel[PANEL_LAYERS] =     _glui->add_panel("Layers");
   _listbox[LIST_LAYER] =    _glui->add_listbox_to_panel(_panel[PANEL_LAYERS],
                              "", NULL,
                              id+LIST_LAYER, listbox_cb);
   
  
   _button[BUT_ADD_GRID_LAYER] =  _glui->add_button_to_panel(
                              _panel[PANEL_LAYERS], 
                              "Add with Grid", 
                              id+BUT_ADD_GRID_LAYER, 
                              button_cb);
   _glui->add_column_to_panel(_panel[PANEL_LAYERS],false);
   
   _checkbox[CHECK_LAYER_VIS]=_glui->add_checkbox_to_panel(
                              _panel[PANEL_LAYERS],
                              "Visible",
                              NULL,
                              id+CHECK_LAYER_VIS,
                              checkbox_cb);                             
   _button[BUT_ADD_BLANK_LAYER] = _glui->add_button_to_panel(
                              _panel[PANEL_LAYERS], 
                              "Add New Empty", 
                              id+BUT_ADD_BLANK_LAYER, 
                              button_cb);                   
   _button[BUT_DELETE_LAYER]=_glui->add_button_to_panel(
                             _panel[PANEL_LAYERS], 
                             "Delete", 
                             id+BUT_DELETE_LAYER, 
                             button_cb);
   
                                                 

    // LOD Panal
   _rollout[ROLLOUT_PAINT_LOD] =  _glui->add_rollout("LOD",false);
   //_rollout[ROLLOUT_PAINT_LOD]->set_w(_panel[PANEL_PAINT]->get_w());
   _checkbox[CHECK_LOD_ON]=_glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PAINT_LOD],
                              "On",
                              NULL,
                              id+CHECK_LOD_ON,
                              checkbox_cb);  
   _glui->add_column_to_panel(_rollout[ROLLOUT_PAINT_LOD],false);                                
   _checkbox[CHECK_LOD_WIDTH]=_glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PAINT_LOD],
                              "Width",
                              NULL,
                              id+CHECK_LOD_WIDTH,
                              checkbox_cb); 
   _checkbox[CHECK_LOD_WIDTH]->disable();

   _checkbox[CHECK_LOD_ALPHA]=_glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PAINT_LOD],
                              "Alpha",
                              NULL,
                              id+CHECK_LOD_ALPHA,
                              checkbox_cb);   
   _slider[SLIDE_LOD_HI] = _glui->add_slider_to_panel(
                           _rollout[ROLLOUT_PAINT_LOD], 
                           "LOD Hi", 
                           GLUI_SLIDER_FLOAT, 
                           0, 3, NULL, id+SLIDE_LOD_HI, slider_cb);   
   _slider[SLIDE_LOD_HI]->set_num_graduations(20);
   _slider[SLIDE_LOD_HI]->set_w(150);
   _slider[SLIDE_LOD_LO] = _glui->add_slider_to_panel(
                           _rollout[ROLLOUT_PAINT_LOD], 
                           "LOD Lo", 
                           GLUI_SLIDER_FLOAT, 
                           0, 3, NULL, id+SLIDE_LOD_LO, slider_cb);   
   _slider[SLIDE_LOD_LO]->set_num_graduations(20);
   _slider[SLIDE_LOD_LO]->set_w(150);
   
   // Lighting Panel
   _rollout[ROLLOUT_PAINT_LIGHT] =  _glui->add_rollout("Light",false);
   //_rollout[ROLLOUT_PAINT_LIGHT]->set_w(_panel[PANEL_PAINT]->get_w());
   _checkbox[CHECK_LIGHT_ON]=_glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PAINT_LIGHT],
                              "On",
                              NULL,
                              id+CHECK_LIGHT_ON,
                              checkbox_cb);
   _glui->add_separator_to_panel( _rollout[ROLLOUT_PAINT_LIGHT]);    
  
                              
   _radgroup[RADGROUP_LIGHT] = _glui->add_radiogroup_to_panel(
                                _rollout[ROLLOUT_PAINT_LIGHT],
                                NULL,
                                id+RADGROUP_LIGHT, radiogroup_cb);
                            
   
   _radbutton[RADBUT_LIGHT0] = _glui->add_radiobutton_to_group(
                               _radgroup[RADGROUP_LIGHT],
                               "L0");
   _radbutton[RADBUT_LIGHT0]->set_w(5);
   _radbutton[RADBUT_LIGHT1] = _glui->add_radiobutton_to_group(
                               _radgroup[RADGROUP_LIGHT],
                               "L1");
   _radbutton[RADBUT_LIGHT1]->set_w(5);
   _radbutton[RADBUT_LIGHT2] = _glui->add_radiobutton_to_group(
                               _radgroup[RADGROUP_LIGHT],
                               "L2");
   _radbutton[RADBUT_LIGHT2]->set_w(5);
   _radbutton[RADBUT_LIGHT3] = _glui->add_radiobutton_to_group(
                               _radgroup[RADGROUP_LIGHT],
                               "L3");
   _glui->add_separator_to_panel( _rollout[ROLLOUT_PAINT_LIGHT]);                             
   
   
   _radgroup[RADGROUP_LIGHT_TYPE] = _glui->add_radiogroup_to_panel(
                                _rollout[ROLLOUT_PAINT_LIGHT],
                                NULL,
                                id+RADGROUP_LIGHT, radiogroup_cb);
   _radbutton[RADBUT_DIFF] = _glui->add_radiobutton_to_group(
                               _radgroup[RADGROUP_LIGHT_TYPE],
                               "Diff");
   _radbutton[RADBUT_DIFF]->set_w(5);
   _radbutton[RADBUT_DIFF] = _glui->add_radiobutton_to_group(
                               _radgroup[RADGROUP_LIGHT_TYPE],
                               "Spec");
   _radbutton[RADBUT_DIFF]->set_w(5);
   
    _glui->add_separator_to_panel( _rollout[ROLLOUT_PAINT_LIGHT]);                             
   
  
   _checkbox[CHECK_LIGHT_WIDTH]=_glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PAINT_LIGHT],
                              "Width",
                              NULL,
                              id+CHECK_LIGHT_WIDTH,
                              checkbox_cb);   
   _checkbox[CHECK_LIGHT_ALPHA]=_glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PAINT_LIGHT],
                              "Alpha",
                              NULL,
                              id+CHECK_LIGHT_ALPHA,
                              checkbox_cb);                  
   
   _glui->add_column_to_panel(_rollout[ROLLOUT_PAINT_LIGHT],true);                                 
                
   _checkbox[CHECK_LIGHT_CAM_FRAME]=_glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PAINT_LIGHT],
                              "Cam Frame",
                              NULL,
                              id+CHECK_LIGHT_CAM_FRAME,
                              checkbox_cb);                                 
   _rotation[ROT_LIGHT] = _glui->add_rotation_to_panel(
                        _rollout[ROLLOUT_PAINT_LIGHT],
                       "Light",
                       NULL,
                       id+ROT_LIGHT, rotation_cb);                
                       
   _slider[SLIDE_LIGHT_A] = _glui->add_slider_to_panel(
                           _rollout[ROLLOUT_PAINT_LIGHT], 
                           "A", 
                           GLUI_SLIDER_FLOAT, 
                           -1.0, 1.0, NULL, id+SLIDE_LIGHT_A, slider_cb);
   _slider[SLIDE_LIGHT_B] = _glui->add_slider_to_panel(
                           _rollout[ROLLOUT_PAINT_LIGHT], 
                           "B", 
                           GLUI_SLIDER_FLOAT, 
                           0.0, 2.0, NULL, id+SLIDE_LIGHT_B, slider_cb);  
                           
   _slider[SLIDE_LIGHT_A]->set_num_graduations(40);
   _slider[SLIDE_LIGHT_A]->set_w(160);
   _slider[SLIDE_LIGHT_B]->set_num_graduations(40);
   _slider[SLIDE_LIGHT_B]->set_w(160);
   _slider[SLIDE_LIGHT_A]->set_float_val(0.3);
   _slider[SLIDE_LIGHT_B]->set_float_val(0.7);
   // Grid Panal
   _rollout[ROLLOUT_GRID] =  _glui->add_rollout("Grid",true);
     
   _panel[PANEL_GRID]     =  _glui->add_panel_to_panel(_rollout[ROLLOUT_GRID],"");
   
   _button[BUT_SELECT_REF_MODE] = _glui->add_button_to_panel(
                            _panel[PANEL_GRID], 
                            "Select Ref. Cells", 
                            id+BUT_SELECT_REF_MODE, 
                            button_cb);   
   _button[BUT_GRID_MODE] =  _glui->add_button_to_panel(
                             _panel[PANEL_GRID],
                             "Grid Mode", id+BUT_GRID_MODE, button_cb);
   _button[BUT_TAGGLE_UNMARKED] = _glui->add_button_to_panel(
                             _panel[PANEL_GRID], 
                             "Show Unmarked", id+BUT_TAGGLE_UNMARKED, button_cb); 
   _glui->add_column_to_panel(_panel[PANEL_GRID],false);                          
   _button[BUT_GRID_SYNTH] = _glui->add_button_to_panel(
                             _panel[PANEL_GRID], 
                             "Strict Synth Grid", id+BUT_GRID_SYNTH, button_cb);
   _button[BUT_GRID_SYNTH2] = _glui->add_button_to_panel(
                             _panel[PANEL_GRID], 
                             "Stupid Synth Grid", id+BUT_GRID_SYNTH2, button_cb);
                                
   _checkbox[CHECK_GRID_SYNTH]=_glui->add_checkbox_to_panel(
                              _panel[PANEL_GRID],
                              "Automatic",
                              NULL,
                              id+CHECK_GRID_SYNTH,
                              checkbox_cb);     
   //Stroke Panel   
   _rollout[ROLLOUT_PAINT] =  _glui->add_rollout("Paint",true);
   //_rollout[ROLLOUT_PAINT]->set_w(_rollout[ROLLOUT_GRID]->get_w());
   
   _button[BUT_PAINT_MODE] = _glui->add_button_to_panel(_rollout[ROLLOUT_PAINT],"Paint Mode", id+BUT_PAINT_MODE, button_cb);
   _panel[PANEL_PAINT] = _glui->add_panel_to_panel(_rollout[ROLLOUT_PAINT],"");
   
   
   _listbox[LIST_STROKE_PRESET] = _glui->add_listbox_to_panel(
                                  _panel[PANEL_PAINT],
                                  "Type ", NULL,
                                  id+LIST_STROKE_PRESET, listbox_cb);
   fill_preset_listbox(_listbox[LIST_STROKE_PRESET], _preset_filenames, Config::JOT_ROOT() + PRESET_DIRECTORY);
   preset_stroke();
   
   _listbox[LIST_STROKE_COLOR] = _glui->add_listbox_to_panel(
                            _panel[PANEL_PAINT],
                            "Color ", NULL,
                            id+LIST_STROKE_COLOR, listbox_cb);
     
   fill_color_listbox(_listbox[LIST_STROKE_COLOR]);  
   _glui->add_column_to_panel(_panel[PANEL_PAINT],false);   
   _checkbox[CHECK_PAINT_WIDTH]=_glui->add_checkbox_to_panel(
                              _panel[PANEL_PAINT],
                              "Width",
                              NULL,
                              id+CHECK_PAINT_WIDTH,
                              checkbox_cb);   
   _checkbox[CHECK_PAINT_ALPHA]=_glui->add_checkbox_to_panel(
                              _panel[PANEL_PAINT],
                              "Alpha",
                              NULL,
                              id+CHECK_PAINT_ALPHA,
                              checkbox_cb);                                   
  
   // Synthesis Panel
   _rollout[ROLLOUT_SYNTHESIZE] =  _glui->add_rollout("Pattern Synthesis",true);  
   _button[BUT_SYNTHESIS_MODE] = _glui->add_button_to_panel(
                                 _rollout[ROLLOUT_SYNTHESIZE], 
                                 "Randomized", 
                                 id+BUT_SYNTHESIS_MODE, button_cb);
   _checkbox[CHECK_VARIATION]=_glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_SYNTHESIZE],
                              "Variation",
                              NULL,
                              id+CHECK_VARIATION,
                              checkbox_cb);     
   _glui->add_column_to_panel(_rollout[ROLLOUT_SYNTHESIZE],false); 
   _button[BUT_SYNTHESIS2_MODE] = _glui->add_button_to_panel(
                                  _rollout[ROLLOUT_SYNTHESIZE], 
                                  "Regular", 
                                  id+BUT_SYNTHESIS2_MODE, button_cb);

                              
   for (i=0; i<BUT_NUM; i++){
      if(_button[i])
      _button[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   for (i=0; i<PANEL_NUM; i++){
      if(_panel[i])
      _panel[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   for (i=0; i<ROLLOUT_NUM; i++){
      if(_rollout[i])
      _rollout[i]->set_alignment(GLUI_ALIGN_LEFT);
   }   
   
}
*/

/*
void     
PatternPenUI::update_non_lives()
{  
   update_light();
   update_from_layer_vals();  
   fill_layer_listbox();   
   _checkbox[CHECK_VARIATION]->set_int_val(PatternPen::VARIATION);
   _checkbox[CHECK_GRID_SYNTH]->set_int_val(PatternPen::AUTO_GRID_SYNTH);
   
}
*/

/*
void
PatternPenUI::apply_vals_to_layer()
{   
   PatternTexture *pt = _pen->pattern_texture();
   if(!pt)
      return;
   //update the layer  
   
   pt->get_current_layer()->set_visible(_checkbox[CHECK_LAYER_VIS]->get_int_val());
   pt->get_current_layer()->set_width_press(_checkbox[CHECK_PAINT_WIDTH]->get_int_val());
   _pen->set_gesture_drawer_w(_checkbox[CHECK_PAINT_WIDTH]->get_int_val()==1);   
   //cerr << "width is " << _checkbox[CHECK_PAINT_WIDTH]->get_int_val() << endl;
   pt->get_current_layer()->set_alpha_press(_checkbox[CHECK_PAINT_ALPHA]->get_int_val());
   _pen->set_gesture_drawer_a(_checkbox[CHECK_PAINT_ALPHA]->get_int_val()==1);   
   
   pt->update_strokes();
   
  
}
*/

/*
void
PatternPenUI::apply_light()
{
   VIEWptr view = VIEW::peek();
   PatternTexture *pt = _pen->pattern_texture();
   if(!pt)
      return;
   
   int i = _radgroup[RADGROUP_LIGHT]->get_int_val();
   
  
   float mat[4][4];
   _rotation[ROT_LIGHT]->get_float_array_val((float *)mat);

   Wvec c0(mat[0][0],mat[0][1],mat[0][2]);
   Wvec c1(mat[1][0],mat[1][1],mat[1][2]);
   Wvec c2(mat[2][0],mat[2][1],mat[2][2]);
   Wtransf t(c0,c1,c2);

   Wvec new_dir = (t*_light).normalized();
   
   view->light_set_coordinates_v(i,new_dir);  
   view->light_set_in_cam_space(i, (_checkbox[CHECK_LIGHT_CAM_FRAME]->get_int_val()) == 1);
  
 
   pt->get_current_layer()->set_light_num(i);
   
   
   pt->get_current_layer()->set_light_on(_checkbox[CHECK_LIGHT_ON]->get_int_val());
   pt->get_current_layer()->set_light_type(_radgroup[RADGROUP_LIGHT_TYPE]->get_int_val());
   
   pt->get_current_layer()->set_light_alpha(_checkbox[CHECK_LIGHT_ALPHA]->get_int_val());
   pt->get_current_layer()->set_light_width(_checkbox[CHECK_LIGHT_WIDTH]->get_int_val());
   //pt->get_current_layer()->set_light_radius(_slider[SLIDE_LIGHT_RAD]->get_float_val());
   pt->get_current_layer()->set_light_a(_slider[SLIDE_LIGHT_A]->get_float_val());
   pt->get_current_layer()->set_light_b(_slider[SLIDE_LIGHT_B]->get_float_val());
   
  
   
   pt->get_current_layer()->set_lod_on(_checkbox[CHECK_LOD_ON]->get_int_val()==1);    
   pt->get_current_layer()->set_lod_alpha(_checkbox[CHECK_LOD_ALPHA]->get_int_val()==1);    
   pt->get_current_layer()->set_lod_high((double)_slider[SLIDE_LOD_HI]->get_float_val());
   pt->get_current_layer()->set_lod_low((double)_slider[SLIDE_LOD_LO]->get_float_val());
   
}
*/

/*
void
PatternPenUI::update_from_layer_vals()
{
   PatternTexture *pt = _pen->pattern_texture();
   if(!pt)
      return;
   //set current grid for the pen and the texture
   
  
   pt->set_layer_number(_listbox[LIST_LAYER]->get_int_val());   
   _pen->set_grid(pt->get_current_layer());
  
   _checkbox[CHECK_LAYER_VIS]->set_int_val(pt->get_current_layer()->get_visible());
   _checkbox[CHECK_PAINT_WIDTH]->set_int_val(pt->get_current_layer()->get_width_press());
   _checkbox[CHECK_PAINT_ALPHA]->set_int_val(pt->get_current_layer()->get_alpha_press());    
   
   _checkbox[CHECK_LOD_ON]->set_int_val(pt->get_current_layer()->get_lod_on());    
   _checkbox[CHECK_LOD_ALPHA]->set_int_val(pt->get_current_layer()->get_lod_alpha());    
   _slider[SLIDE_LOD_HI]->set_float_val((float)pt->get_current_layer()->get_lod_high());
   _slider[SLIDE_LOD_LO]->set_float_val((float)pt->get_current_layer()->get_lod_low());
   
}
*/

/*
void
PatternPenUI::update_light()
{
   PatternTexture *pt = _pen->pattern_texture();
   if(!pt)
      return;
   _radgroup[RADGROUP_LIGHT]->set_int_val((int)pt->get_current_layer()->get_light_num());
   //_rotation[ROT_LIGHT]->reset();
   _radgroup[RADGROUP_LIGHT_TYPE]->set_int_val((int)pt->get_current_layer()->get_light_type());
   
   _checkbox[CHECK_LIGHT_ALPHA]->set_int_val((int)pt->get_current_layer()->get_light_alpha());
   _checkbox[CHECK_LIGHT_WIDTH]->set_int_val(pt->get_current_layer()->get_light_width());
  
   _slider[SLIDE_LIGHT_A]->set_float_val(pt->get_current_layer()->get_light_a());
   _slider[SLIDE_LIGHT_B]->set_float_val(pt->get_current_layer()->get_light_b());
   
   update_limits();
   // cerr << "Im getting " << pt->get_current_layer()->get_light_radius() << endl;
   //_slider[SLIDE_LIGHT_RAD]->set_float_val((float)pt->get_current_layer()->get_light_radius());
   
   _checkbox[CHECK_LIGHT_CAM_FRAME]->set_int_val((VIEW::peek()->light_get_in_cam_space(pt->get_current_layer()->get_light_num()))?(1):(0));
   _checkbox[CHECK_LIGHT_ON]->set_int_val((pt->get_current_layer()->get_light_on())?(1):(0));     
   _checkbox[CHECK_LIGHT_WIDTH]->set_int_val((int)pt->get_current_layer()->get_light_width());     
   _checkbox[CHECK_LIGHT_ALPHA]->set_int_val((int)pt->get_current_layer()->get_light_alpha());
   
}
*/

/*
void     
PatternPenUI::update_limits()
{
   PatternTexture *pt = _pen->pattern_texture();
   if(!pt)
      return;
   _slider[SLIDE_LIGHT_A]->set_float_limits(-1.0,pt->get_current_layer()->get_light_b());
   _slider[SLIDE_LIGHT_B]->set_float_limits(pt->get_current_layer()->get_light_a(), 2.0);
} 
*/

/*
void     
PatternPenUI::add_layer()
{
   PatternTexture *pt = _pen->pattern_texture();
   if(!pt)
      return;
   pt->add_grid_empty();
   
   fill_layer_listbox();   
   _listbox[LIST_LAYER]->set_int_val(pt->pattern_grid_num()-1);
   update_from_layer_vals();   
}
*/

/*
void     
PatternPenUI::add_grid_layer()
{
   PatternTexture *pt = _pen->pattern_texture();
   if(!pt)
      return;
   pt->add_grid();
   
   fill_layer_listbox();   
   _listbox[LIST_LAYER]->set_int_val(pt->pattern_grid_num()-1);
   update_from_layer_vals();   
}
*/
  
/* 
void     
PatternPenUI::del_layer()
{
   PatternTexture *pt = _pen->pattern_texture();
   if(!pt)
      return;
   if(pt->pattern_grid_num() > 1){ 
      pt->remove_grid(pt->get_layer_number());
      fill_layer_listbox();   
      _listbox[LIST_LAYER]->set_int_val(pt->get_layer_number()-1);
      update_from_layer_vals();      
   }
}
*/



/*
void
PatternPenUI::preset_stroke()
{
   int val = _listbox[LIST_STROKE_PRESET]->get_int_val();
   if (!load_preset(**_preset_filenames[val-1]))
        return;
   _glui->sync_live();
} 
*/

/*
void
PatternPenUI::set_color()
{
   int val = _listbox[LIST_BASE_COLOR]->get_int_val();
  
   COLOR col = _my_colors[_my_color_names[val]];   
   _pen->set_base_color(col);    

   _glui->sync_live();        
}
*/

/*
void
PatternPenUI::set_stroke_color()
{
   int val = _listbox[LIST_STROKE_COLOR]->get_int_val();

   COLOR col = _my_colors[_my_color_names[val]];
   _stroke->set_color(col);

   _glui->sync_live();
}
*/




//******** CALLBACK FUNCTIONS ********

/*
void
PatternPenUI::button_cb(int id) {    
  switch(id&ID_MASK) {
     case BUT_VIEW_MODE:
	     _ui[id >> ID_SHIFT]->_pen->taggle_grid_line();
	     break;
     case BUT_ADD_BLANK_LAYER:
        _ui[id >> ID_SHIFT]->add_layer();
        break;  
     case BUT_ADD_GRID_LAYER:
        _ui[id >> ID_SHIFT]->add_grid_layer();
        break;          
     case BUT_DELETE_LAYER:
        _ui[id >> ID_SHIFT]->del_layer();
        break;    
     case BUT_TAGGLE_UNMARKED:
        _ui[id >> ID_SHIFT]->_pen->taggle_select_unmarked_faces();
        break; 
  default:	  
    _ui[id >> ID_SHIFT]->_pen->change_mode(id);
    break;

  }	
  _ui[id >> ID_SHIFT]->_pen->update_grid();
}
*/

/*
void
PatternPenUI::listbox_cb(int id) {
    
  switch(id&ID_MASK) {
  case LIST_LAYER:
    _ui[id >> ID_SHIFT]->update_from_layer_vals();
    _ui[id >> ID_SHIFT]->update_light();     
    break;     
  case LIST_BASE_COLOR:	   
    _ui[id >> ID_SHIFT]->set_color();	   
    break;
  case LIST_STROKE_PRESET:	  
    _ui[id >> ID_SHIFT]->preset_stroke();
    break;
  case LIST_STROKE_COLOR:
    _ui[id >> ID_SHIFT]->set_stroke_color();
    break;
  }
   
	
}
*/

/*
void 
PatternPenUI::checkbox_cb(int id) {	
  switch(id&ID_MASK) {
     case CHECK_VARIATION:
	     PatternPen::VARIATION = _ui[id >> ID_SHIFT]->_checkbox[CHECK_VARIATION]->get_int_val();
        break;
     case CHECK_GRID_SYNTH:
        PatternPen::AUTO_GRID_SYNTH = _ui[id >> ID_SHIFT]->_checkbox[CHECK_GRID_SYNTH]->get_int_val();
        break;
     default:
        _ui[id >> ID_SHIFT]->apply_vals_to_layer();
        _ui[id >> ID_SHIFT]->apply_light();
  }
}
*/

/*
void 
PatternPenUI::slider_cb(int id) {  
   _ui[id >> ID_SHIFT]->apply_light();
   _ui[id >> ID_SHIFT]->update_limits();
}
*/

/*
void 
PatternPenUI::rotation_cb(int id) {  
   _ui[id >> ID_SHIFT]->apply_light();      
}
*/

/*
void
PatternPenUI::radiogroup_cb(int id) {
   _ui[id >> ID_SHIFT]->apply_light();   
} 
*/ 







//******** Convenience Methods ********

/*
void
PatternPenUI::fill_preset_listbox(
   GLUI_Listbox *listbox,
   str_list     &save_files,
   Cstr_ptr     &full_path
   )
{
   int i;

   //First clear out any previous presets
   for (i=1; i<=save_files.num();i++)
   {
      if(listbox)
        listbox->delete_item(i);
      //assert(foo);
   }
   save_files.clear();

   str_list in_files = dir_list(full_path);
   for (i = 0; i < in_files.num(); i++) {
      int len = in_files[i].len();

      if ( (len>3) && (strcmp(&(**in_files[i])[len-4],".pre") == 0))
      {
         char *basename = new char[len+1];
         assert(basename);
         strcpy(basename,**in_files[i]);
         basename[len-4] = 0;

         if ( listbox->check_item_fit(basename) == 1)
         {
            save_files += full_path + in_files[i];
            listbox->add_item(save_files.num(), basename);
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "PatternPenUI::fill_preset_listbox - Discarding preset file (name too long for listbox): %s", basename);
         }
         delete basename;
      }
      else if (in_files[i] != "CVS")
      {
         err_mesg(ERR_LEV_WARN, "PatternPenUI::fill_preset_listbox - Discarding preset file (bad name): %s", **in_files[i]);
      }
   }
}
*/

/*
void
PatternPenUI::fill_color_listbox(GLUI_Listbox *listbox)
{  
   for(color_name_t::iterator it=_my_color_names.begin(); it !=_my_color_names.end(); ++it){
        listbox->add_item(it->first, (char*)(it->second).c_str());
   }

}
*/

/*
void        
PatternPenUI::fill_layer_listbox()
{
   
   
   PatternTexture *pt = _pen->pattern_texture();   
   if(!pt)
      return;
   int i = 0;
   while (_listbox[LIST_LAYER]->delete_item(i)) i++;

   for (i = 0; i < pt->pattern_grid_num(); i++) {
      str_ptr number(i);
      str_ptr str ("Layer ");
      str_ptr bla = str + number;      
      _listbox[LIST_LAYER]->add_item(i, **bla);
   }
}
*/

/*
bool
PatternPenUI::load_preset(const char *f)
{
   fstream fin;
   fin.open(f, ios::in);

   if (!fin) {
      err_mesg(ERR_LEV_ERROR,"PatternPenUI::load_preset() - Error! Could not open file: '%s'", f);
      return false;
   }  
   STDdstream d(&fin);
   str_ptr str;
   d >> str;

   if ((str == BaseStroke::static_name())) {
      //In case we're testing offsets, let
      //trash the old ones
      _stroke->set_offsets(NULL);
      _stroke->decode(d);
   } else {
      err_mesg(ERR_LEV_ERROR,"StrokeUI::load_preset() - Error! Not BaseStroke: '%s'", **str);
   }
   fin.close();
   return true;
}
*/
