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
#include "std/file.H"
#include <GL/glew.h>

#include "geom/winsys.H"
#include "glui/glui_jot.H"
#include "base_jotapp/base_jotapp.H"
#include "stroke/base_stroke.H"

#include <fstream>
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

vector<PatternPenUI*> PatternPenUI::_ui;

PatternPenUI::PatternPenUI(PatternPen* pen) :
   _glui(nullptr),
   _pen(pen),
   _init(false),
   _current_mode(0),
   _light(0,0,1) 
{
   _ui.push_back(this);
   _id = (_ui.size()-1);
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
  _glui = nullptr;
}


void
PatternPenUI::build()
{
   int id = _id << ID_SHIFT;

   assert(!_glui);

   int root_x, root_y, root_w, root_h;
   _pen->view()->win()->size(root_w,root_h);
   _pen->view()->win()->position(root_x,root_y);

   _glui = GLUI_Master.create_glui("Pattern Editor", 0, root_x+root_w+10, root_y);
   _glui->set_main_gfx_window(_pen->view()->win()->id());
   
   //Init the control arrays
   _button.resize(BUT_NUM, nullptr);
   _panel.resize(PANEL_NUM, nullptr);
   _listbox.resize(LIST_NUM, nullptr);
   _checkbox.resize(CHECK_NUM, nullptr);
   _slider.resize(SLIDE_NUM, nullptr);
   _rollout.resize(ROLLOUT_NUM, nullptr);
   _rotation.resize(ROT_NUM, nullptr);
   _radgroup.resize(RADGROUP_NUM, nullptr);
   _radbutton.resize(RADBUT_NUM, nullptr);

   // Panel containing pen buttons
   _panel[PANEL_PEN] = new GLUI_Panel(_glui, "");
   assert(_panel[PANEL_PEN]);

   //Prev pen
   _button[BUT_PREV_PEN] = new GLUI_Button(
      _panel[PANEL_PEN], "Previous Mode", 
      id+BUT_PREV_PEN, button_cb);
   assert(_button[BUT_PREV_PEN]);

   new GLUI_Column(_panel[PANEL_PEN], false);

   //Prev pen
   _button[BUT_NEXT_PEN] = new GLUI_Button(
      _panel[PANEL_PEN], "Next Mode", 
      id+BUT_NEXT_PEN, button_cb);
   assert(_button[BUT_NEXT_PEN]);

   _panel[PANEL_MODE] = new GLUI_Panel(_glui, "");

    // stroke pattern modes
   _listbox[LIST_MODE]= new GLUI_Listbox(_panel[PANEL_MODE], "Mode", nullptr,
					  id+LIST_MODE, listbox_cb);
   _listbox[LIST_MODE]->add_item(0, "Analysis");
   _listbox[LIST_MODE]->add_item(1, "Synthesis");
   _listbox[LIST_MODE]->add_item(2, "Path");
   _listbox[LIST_MODE]->add_item(3, "Proxy");
   _listbox[LIST_MODE]->add_item(4, "Ellipses");

   new GLUI_Column(_panel[PANEL_MODE], false);

   _listbox[LIST_STROKE_PRESET] = new GLUI_Listbox(_panel[PANEL_MODE], "Preset", nullptr, id+LIST_STROKE_PRESET, listbox_cb);
   fill_preset_listbox(_listbox[LIST_STROKE_PRESET], _preset_filenames, Config::JOT_ROOT() + PRESET_DIRECTORY);
   preset_stroke();

  

   // stroke mode
   _rollout[ROLLOUT_ANALYSIS] = new GLUI_Rollout(_glui, "Analysis", true);
   
   _panel[PANEL_STROKES] = new GLUI_Panel(_rollout[ROLLOUT_ANALYSIS], "");
   
   _slider[SLIDE_EPS] = new GLUI_Slider(_panel[PANEL_STROKES], "Epsilon",
                                        id+SLIDE_EPS, slider_cb,
                                        GLUI_SLIDER_FLOAT, 0, 20, nullptr);
   _slider[SLIDE_EPS]->set_num_graduations(101);
   _slider[SLIDE_EPS]->set_float_val(0.0f);
   _button[BUT_POP] = new GLUI_Button(_panel[PANEL_STROKES], "Pop Stroke",
                                      id+BUT_POP, button_cb);
   _button[BUT_CLEAR] = new GLUI_Button(_panel[PANEL_STROKES], "Clear Strokes",
                                        id+BUT_CLEAR, button_cb);

  
   new GLUI_Column(_panel[PANEL_STROKES], false);


   _listbox[LIST_TYPE]= new GLUI_Listbox(_panel[PANEL_STROKES],
                                         "Type", nullptr,
                                         id+LIST_TYPE, listbox_cb);
   _listbox[LIST_TYPE]->add_item(0, "Hatching");
   _listbox[LIST_TYPE]->add_item(1, "Stippling");
   _listbox[LIST_TYPE]->add_item(2, "Free");
   _listbox[LIST_TYPE]->set_int_val(2);

   _listbox[LIST_PARAM]= new GLUI_Listbox(_panel[PANEL_STROKES],
                                          "Param.", nullptr,
                                          id+LIST_PARAM, listbox_cb);
   _listbox[LIST_PARAM]->add_item(0, "Axis");
   _listbox[LIST_PARAM]->add_item(1, "Cartesian");
   _listbox[LIST_PARAM]->add_item(2, "Angular");

   _checkbox[CHECK_ANAL_STYLE] = new GLUI_Checkbox(_panel[PANEL_STROKES],
                                                   "Analyze style", nullptr,
                                                   id+CHECK_ANAL_STYLE, checkbox_cb);
   _checkbox[CHECK_ANAL_STYLE]->set_int_val(1);

   _slider[SLIDE_STYLE] = new GLUI_Slider(_panel[PANEL_STROKES], "Style adjust.",
                                          id+SLIDE_STYLE, slider_cb,
                                          GLUI_SLIDER_FLOAT, 0, 1, nullptr);
   _slider[SLIDE_STYLE]->set_num_graduations(101);
   _slider[SLIDE_STYLE]->set_float_val(1.0f);

   _button[BUT_NEW_GROUP] = new GLUI_Button(_panel[PANEL_STROKES], "New Group",
                                            id+BUT_NEW_GROUP, button_cb);
   
   _checkbox[CHECK_STRUCT] = new GLUI_Checkbox(_panel[PANEL_STROKES],
                                               "Display structure", nullptr,
                                               id+CHECK_STRUCT, checkbox_cb);
   _checkbox[CHECK_FRAME] = new GLUI_Checkbox(_panel[PANEL_STROKES],
                                              "Display ref frame", nullptr,
                                              id+CHECK_FRAME, checkbox_cb);
   _checkbox[CHECK_FRAME]->set_int_val(1);
   

   // synthesis mode
   _rollout[ROLLOUT_SYNTH] =  new GLUI_Rollout(_glui, "Synthesis", false);
   _listbox[LIST_CELL]= new GLUI_Listbox(_rollout[ROLLOUT_SYNTH],
                                         "Cell", nullptr, id+LIST_CELL, listbox_cb);
   _listbox[LIST_CELL]->add_item(0, "Box");
   _listbox[LIST_CELL]->add_item(1, "Rect");
   _listbox[LIST_CELL]->add_item(2, "Path");
   _listbox[LIST_CELL]->add_item(3, "Carriers");

   _panel[PANEL_IMAGE] = new GLUI_Panel(_rollout[ROLLOUT_SYNTH], "Back Image");
   assert(_panel[PANEL_IMAGE]);
   _checkbox[CHECK_IMAGE_PRESSURE] = new GLUI_Checkbox(_panel[PANEL_IMAGE],
                                                       "Use Image Luminasity", nullptr,
                                                       id+CHECK_IMAGE_PRESSURE, checkbox_cb);

   _checkbox[CHECK_IMAGE_ALPHA] = new GLUI_Checkbox(_panel[PANEL_IMAGE],
                                                    "Modify Alpha", nullptr,
                                                    id+CHECK_IMAGE_ALPHA, checkbox_cb);
   _checkbox[CHECK_IMAGE_ALPHA]->set_int_val(_pen->get_gesture_drawer_a()); 

   _checkbox[CHECK_IMAGE_WIDTH] = new GLUI_Checkbox(_panel[PANEL_IMAGE],
                                                    "Modify Width", nullptr,
                                                    id+CHECK_IMAGE_WIDTH, checkbox_cb);
   _checkbox[CHECK_IMAGE_WIDTH]->set_int_val(_pen->get_gesture_drawer_w()); 
     
   _checkbox[CHECK_IMAGE_COLOR] = new GLUI_Checkbox(_panel[PANEL_IMAGE],
                                                    "Use Color", nullptr,
                                                    id+CHECK_IMAGE_COLOR, checkbox_cb);


   _checkbox[CHECK_SHOW_BBOX] = new GLUI_Checkbox(_rollout[ROLLOUT_SYNTH],
                                                  "Show cell", nullptr,
                                                  id+CHECK_SHOW_BBOX, checkbox_cb);
    _checkbox[CHECK_SHOW_BBOX]->set_int_val(1);

    _checkbox[CHECK_SHOW_ICON] = new GLUI_Checkbox(_rollout[ROLLOUT_SYNTH],
                                                   "Show example", nullptr,
                                                   id+CHECK_SHOW_ICON, checkbox_cb);
    _checkbox[CHECK_SHOW_ICON]->set_int_val(1);


    _panel[PANEL_COLOR] = new GLUI_Panel(_rollout[ROLLOUT_SYNTH], "Image Color Adjust");

    _slider[SLIDE_COLOR_H] = new GLUI_Slider(_panel[PANEL_COLOR], "H Adjust",
                                             id+SLIDE_COLOR_H, slider_cb,
                                             GLUI_SLIDER_FLOAT, -1, 1, nullptr);
    _slider[SLIDE_COLOR_H]->set_num_graduations(21);
    _slider[SLIDE_COLOR_H]->set_float_val(0.0f);
    _slider[SLIDE_COLOR_H]->set_w(120);

    _slider[SLIDE_COLOR_S] = new GLUI_Slider(_panel[PANEL_COLOR], "S Adjust",
                                             id+SLIDE_COLOR_S, slider_cb,
                                             GLUI_SLIDER_FLOAT, -1, 1, nullptr);
    _slider[SLIDE_COLOR_S]->set_num_graduations(21);
    _slider[SLIDE_COLOR_S]->set_float_val(0.0f);
    _slider[SLIDE_COLOR_S]->set_w(120);

    _slider[SLIDE_COLOR_V] = new GLUI_Slider(_panel[PANEL_COLOR], "V Adjust",
                                             id+SLIDE_COLOR_V, slider_cb,
                                             GLUI_SLIDER_FLOAT, -1, 1, nullptr);
    _slider[SLIDE_COLOR_V]->set_num_graduations(21);
    _slider[SLIDE_COLOR_V]->set_float_val(0.0f);
    _slider[SLIDE_COLOR_V]->set_w(120);

    _listbox[LIST_LUMIN_FUNC] = new GLUI_Listbox(_panel[PANEL_COLOR],
                                                 "Lum Func", nullptr, id+LIST_LUMIN_FUNC, listbox_cb);
    _listbox[LIST_LUMIN_FUNC]->add_item(0, "Shadow");
    _listbox[LIST_LUMIN_FUNC]->add_item(1, "Highlight");

   new GLUI_Column(_rollout[ROLLOUT_SYNTH], false);

   _listbox[LIST_SYNTH] = new GLUI_Listbox(_rollout[ROLLOUT_SYNTH],
                                           "Mode", nullptr, id+LIST_SYNTH, listbox_cb);
   _listbox[LIST_SYNTH]->add_item(0, "synth_mimic");
   _listbox[LIST_SYNTH]->add_item(1, "synth_Efros");
   _listbox[LIST_SYNTH]->add_item(2, "synth_sample");
   _listbox[LIST_SYNTH]->add_item(3, "synth_copy");
   _listbox[LIST_SYNTH]->add_item(4, "synth_clone");

   _listbox[LIST_DIST] = new GLUI_Listbox(_rollout[ROLLOUT_SYNTH],
                                          "Distribution", nullptr, id+LIST_DIST, listbox_cb);
   _listbox[LIST_DIST]->add_item(0, "Lloyd");
   _listbox[LIST_DIST]->add_item(1, "Stratified");

   _slider[SLIDE_RING] = new GLUI_Slider(_rollout[ROLLOUT_SYNTH], "#Ring",
                                         id+SLIDE_RING, slider_cb,
                                         GLUI_SLIDER_INT, 1, 5, nullptr);

   _button[BUT_POP_SYNTH] = new GLUI_Button(_rollout[ROLLOUT_SYNTH], "Pop Cell",
                                            id+BUT_POP_SYNTH, button_cb);
   _button[BUT_CLEAR_SYNTH] = new GLUI_Button(_rollout[ROLLOUT_SYNTH], "Clear All",
                                              id+BUT_CLEAR_SYNTH, button_cb);

   
   _checkbox[CHECK_STRETCH] = new GLUI_Checkbox(_rollout[ROLLOUT_SYNTH],
                                                "Element Stretching", nullptr,
                                                id+CHECK_STRETCH, checkbox_cb);
  

   _slider[SLIDE_CORRECT] = new GLUI_Slider(_rollout[ROLLOUT_SYNTH], "Perceptual correction",
                                            id+SLIDE_CORRECT, slider_cb,
                                            GLUI_SLIDER_FLOAT, 0, 1, nullptr);
   _slider[SLIDE_CORRECT]->set_num_graduations(200);
   _slider[SLIDE_CORRECT]->set_float_val(1.0f);
   _slider[SLIDE_CORRECT]->set_w(150);

   _slider[SLIDE_GLOBAL_SCALE] = new GLUI_Slider(_rollout[ROLLOUT_SYNTH], "Global Scale",
                                                 id+SLIDE_GLOBAL_SCALE, slider_cb,
                                                 GLUI_SLIDER_FLOAT, 0, 2, nullptr);
   _slider[SLIDE_GLOBAL_SCALE]->set_num_graduations(21);
   _slider[SLIDE_GLOBAL_SCALE]->set_float_val(1.0f);
   _slider[SLIDE_GLOBAL_SCALE]->set_w(150);


   _button[BUT_RESYNTH] = new GLUI_Button(
      _rollout[ROLLOUT_SYNTH], "Resynthesize", 
      id+BUT_RESYNTH, button_cb);
   assert(_button[BUT_RESYNTH]);

   _rollout[ROLLOUT_SYNTH]->disable();

   // Path mode
   _rollout[ROLLOUT_PATH] = new GLUI_Rollout(_glui, "Path", false);
   _rollout[ROLLOUT_PATH]->disable();

   // Proxy mode
   _rollout[ROLLOUT_PROXY] = new GLUI_Rollout(_glui, "Proxy", false);
   _rollout[ROLLOUT_PROXY]->disable();

   // Ellipses mode
   _rollout[ROLLOUT_ELLIPSE] =  new GLUI_Rollout(_glui, "Ellipses", false);
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
  assert((id >> ID_SHIFT) < (int)_ui.size());

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
  assert((id >> ID_SHIFT) < (int)_ui.size());

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
   if (!load_preset(_preset_filenames[val-1].c_str()))
        return;
   _glui->sync_live();
} 

void
PatternPenUI::fill_preset_listbox(
   GLUI_Listbox   *listbox,
   vector<string> &save_files,
   const string   &full_path
   )
{
   int i;

   //First clear out any previous presets
   for (i = 1; i <= (int)save_files.size(); i++) {
      if (listbox)
        listbox->delete_item(i);    
   }
   save_files.clear();

   vector<string> in_files = dir_list(full_path);
   for (auto & file : in_files) {
      string::size_type len = file.length();

      if ( (len>3) && (file.substr(len-4) == ".pre"))
      {
         string basename = file.substr(0, len-4);

         if ( jot_check_glui_fit(listbox, basename.c_str()) )
         {
            save_files.push_back(full_path + file);
            listbox->add_item(save_files.size(), basename.c_str());
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "PatternPenUI::fill_preset_listbox - Discarding preset file (name too long for listbox): %s", basename.c_str());
         }
      }
      else if (file != "CVS")
      {
         err_mesg(ERR_LEV_WARN, "PatternPenUI::fill_preset_listbox - Discarding preset file (bad name): %s", file.c_str());
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
   string str;
   d >> str;
   
   if (str == BaseStroke::static_name()) {
      //In case we're testing offsets, let
      //trash the old ones
      _stroke->set_offsets(nullptr);
      _stroke->decode(d);
	  cerr << "Stroke Preset Loaded" << endl;
   } else {
      err_mesg(ERR_LEV_ERROR,"PatternPenUI::load_preset() - Error! Not BaseStroke: '%s'", str.c_str());
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

