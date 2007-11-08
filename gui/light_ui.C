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
////////////////////////////////////////////
// LightUI
////////////////////////////////////////////

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "std/config.H"

#include "light_ui.H"
#include "color_ui.H"
#include "presets_ui.H"
using namespace mlib;

/*****************************************************************
 * LightUI
 *****************************************************************/
LightUI*         LightUI::_ui;

const static int WIN_WIDTH=300; 

LightUI::LightUI(VIEWptr v) :
     BaseUI("Lighting"),
     _view(v)     
{
   _ui = this;
   _color =  COLOR(1.0,1.0,1.0);
   _color_ui = new ColorUI(this);
   _presets_ui = new PresetsUI(this, str_ptr("nprdata/lights_presets/"), str_ptr(".view"));

}


void     
LightUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui;
   //Lighting
   _rollout[ROLLOUT_LIGHT] = (base) ? glui->add_rollout_to_panel(base, "Lighting",open)
                                    : glui->add_rollout("Lighting",open);
   

   assert(_rollout[ROLLOUT_LIGHT]);

   //Top panel
   _panel[PANEL_LIGHTTOP] = glui->add_panel_to_panel(
                               _rollout[ROLLOUT_LIGHT],
                               "");
   assert(_panel[PANEL_LIGHTTOP]);

   //Number
   _radgroup[RADGROUP_LIGHTNUM] = glui->add_radiogroup_to_panel(
                                     _panel[PANEL_LIGHTTOP],
                                     NULL,
                                     RADGROUP_LIGHTNUM, radiogroup_cb);
   assert(_radgroup[RADGROUP_LIGHTNUM]);

   _radbutton[RADBUT_LIGHT0] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "0");
   assert(_radbutton[RADBUT_LIGHT0]);
   _radbutton[RADBUT_LIGHT1] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "1");
   assert(_radbutton[RADBUT_LIGHT1]);
   _radbutton[RADBUT_LIGHT2] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "2");
   assert(_radbutton[RADBUT_LIGHT2]);
   _radbutton[RADBUT_LIGHT3] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "3");
   _radbutton[RADBUT_LIGHT4] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "4");
   _radbutton[RADBUT_LIGHT5] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "5");
   _radbutton[RADBUT_LIGHT6] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "6");
   _radbutton[RADBUT_LIGHT7] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "7");
   assert(_radbutton[RADBUT_LIGHT2]);
   _radbutton[RADBUT_LIGHTG] = glui->add_radiobutton_to_group(
                                  _radgroup[RADGROUP_LIGHTNUM],
                                  "G");
   assert(_radbutton[RADBUT_LIGHTG]);

   glui->add_column_to_panel(_panel[PANEL_LIGHTTOP],true);

   //Enable, Position/Direction, Amb/Diff
   _checkbox[CHECK_ENABLE] = glui->add_checkbox_to_panel(
                                _panel[PANEL_LIGHTTOP],
                                "On",
                                NULL,
                                CHECK_ENABLE,
                                checkbox_cb);
   assert(_checkbox[CHECK_ENABLE]);

    _checkbox[CHECK_SHOW_WIDGET] = glui->add_checkbox_to_panel(
                                _panel[PANEL_LIGHTTOP],
                                "Show Widget",
                                NULL,
                                CHECK_SHOW_WIDGET,
                                checkbox_cb);

   _checkbox[CHECK_POS] = glui->add_checkbox_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "Dir",
                             NULL,
                             CHECK_POS,
                             checkbox_cb);
   assert(_checkbox[CHECK_POS]);

   _checkbox[CHECK_CAM] = glui->add_checkbox_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "Cam",
                             NULL,
                             CHECK_CAM,
                             checkbox_cb);
   assert(_checkbox[CHECK_CAM]);

   _radgroup[RADGROUP_LIGHTCOL] = glui->add_radiogroup_to_panel(
                                     _panel[PANEL_LIGHTTOP],
                                     NULL,
                                     RADGROUP_LIGHTCOL, radiogroup_cb);
   assert(_radgroup[RADGROUP_LIGHTCOL]);

   _radbutton[RADBUT_DIFFUSE] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_LIGHTCOL],
                                   "Dif");
   assert(_radbutton[RADBUT_DIFFUSE]);
   _radbutton[RADBUT_AMBIENT] = glui->add_radiobutton_to_group(
                                   _radgroup[RADGROUP_LIGHTCOL],
                                   "Amb");
   assert(_radbutton[RADBUT_AMBIENT]);

   glui->add_column_to_panel(_panel[PANEL_LIGHTTOP],true);

   //Rot
   _rotation[ROT_LIGHT] = glui->add_rotation_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "Pos/Dir",
                             NULL,
                             ROT_LIGHT, rotation_cb);
   assert(_rotation[ROT_LIGHT]);
  

   _spinner[SPINNER_LIGHT_DIR_X] = glui->add_spinner_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "x",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             SPINNER_LIGHT_DIR_X, spinner_cb);
   _spinner[SPINNER_LIGHT_DIR_X]->set_w(20);
   _spinner[SPINNER_LIGHT_DIR_Y] = glui->add_spinner_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "y",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             SPINNER_LIGHT_DIR_Y, spinner_cb);
   _spinner[SPINNER_LIGHT_DIR_Y]->set_w(20);
    _spinner[SPINNER_LIGHT_DIR_Z] = glui->add_spinner_to_panel(
                             _panel[PANEL_LIGHTTOP],
                             "z",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             SPINNER_LIGHT_DIR_Z, spinner_cb);
    _spinner[SPINNER_LIGHT_DIR_Z]->set_w(20);
    glui->add_column_to_panel(_panel[PANEL_LIGHTTOP],false);
    
    _presets_ui->build(glui,_panel[PANEL_LIGHTTOP], true); 
   
    // Spotlight Menu
    _rollout[ROLLOUT_SPOT] = glui->add_rollout_to_panel( _rollout[ROLLOUT_LIGHT], "Point Light",false);
   

    _panel[PANEL_LIGHT_SPOTDIR] = glui->add_panel_to_panel( _rollout[ROLLOUT_SPOT], "");
    
    _rotation[ROT_SPOT] = glui->add_rotation_to_panel(
                             _panel[PANEL_LIGHT_SPOTDIR],
                             "Spot Dir",
                             NULL,
                             ROT_SPOT, rotation_cb);
     glui->add_column_to_panel(_panel[PANEL_LIGHT_SPOTDIR],false);
    _spinner[SPINNER_LIGHT_SPOT_X] = glui->add_spinner_to_panel(
                              _panel[PANEL_LIGHT_SPOTDIR],
                             "x",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             SPINNER_LIGHT_SPOT_X, spinner_cb);
    _spinner[SPINNER_LIGHT_SPOT_Y] = glui->add_spinner_to_panel(
                              _panel[PANEL_LIGHT_SPOTDIR],
                             "y",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             SPINNER_LIGHT_SPOT_Y, spinner_cb);
    _spinner[SPINNER_LIGHT_SPOT_Z] = glui->add_spinner_to_panel(
                              _panel[PANEL_LIGHT_SPOTDIR],
                             "z",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             SPINNER_LIGHT_SPOT_Z, spinner_cb);
    //_glui->add_column_to_panel(_rollout[ROLLOUT_SPOT],true);
    _slider[SLIDE_SPOT_EXPONENT] = glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Spot Exponent",
                          GLUI_SLIDER_FLOAT,
                          0.0, 5.0,
                          NULL,
                          SLIDE_SPOT_EXPONENT, slider_cb);
    
    _slider[SLIDE_SPOT_CUTOFF] = glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Spot Cutoff",
                          GLUI_SLIDER_INT,
                          0, 90,
                          NULL,
                          SLIDE_SPOT_CUTOFF, slider_cb);
    _slider[SLIDE_SPOT_CUTOFF]->set_int_val(90);
    _slider[SLIDE_SPOT_K0] = glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Constatnt Attenuation",
                          GLUI_SLIDER_FLOAT,
                          0.0, 5.0,
                          NULL,
                          SLIDE_SPOT_K0, slider_cb);
    _slider[SLIDE_SPOT_K0]->set_float_val(1.0);
    _slider[SLIDE_SPOT_K1] = glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Linear Attenuation",
                          GLUI_SLIDER_FLOAT,
                          0.0, 1.0,
                          NULL,
                          SLIDE_SPOT_K1, slider_cb);
   
    _slider[SLIDE_SPOT_K2] = glui->add_slider_to_panel(
                          _rollout[ROLLOUT_SPOT],
                          "Quadratic Attenuation",
                          GLUI_SLIDER_FLOAT,
                          0.0, 0.5,
                          NULL,
                          SLIDE_SPOT_K2, slider_cb);
   

 
   _color_ui->build(glui, _rollout[ROLLOUT_LIGHT],false);

   // Cleanup sizes
   for (int i=0; i<ROLLOUT_NUM; i++){
      if(_rollout[i])
      _rollout[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   for (int i=0; i<PANEL_NUM; i++){
      if(_panel[i])
      _panel[i]->set_alignment(GLUI_ALIGN_LEFT);
   }

   int w = _panel[PANEL_LIGHTTOP]->get_w();
   w = max(WIN_WIDTH, w);

   for (int i=0; i<SLIDE_NUM; i++){
      if(_slider[i])   
         _slider[i]->set_w(w);
   }
}

void
LightUI::update_non_lives()
{
   _color_ui->update_non_lives();
   _presets_ui->update_non_lives();
   update_light();
}

bool
LightUI::child_callback(BaseUI* sender, int event)
{
   bool s = false;  
   if(sender->class_name() == PresetsUI::static_name()){
      switch(event)
      {
      case PresetsUI::PRESET_SELECTED:
         s = load_preset();
         break;
      case PresetsUI::PRESET_SAVE:      
      case PresetsUI::PRESET_SAVE_NEW:      
         s = save_preset();
         break;   
      } 
   }     
   apply_light();
   return s;
}

bool
LightUI::load_preset()
{
    cerr << "LightUI::load_preset()" << endl;
   char* f = **(_presets_ui->get_filename());
   if(!f){
      err_msg("LightUI::save_preset - file not specified");
      return false;
   }

   ifstream fin(f, ifstream::in);

   if (!fin)    {
      err_msg("ColorUI::load_preset() - Error! Could not open file: '%s'", f);
      return false;
   }
   str_ptr str;

   STDdstream s(&fin);
   s >> str;
   if (str != VIEW::static_name()) 
   {
      err_msg("VIEW::get_view_data_file() - Not 'VIEW': '%s'!!", **str);
   } else {
      //_in_data_file = true;
      _view->decode(s);
      //_in_data_file = false;
   }
   fin.close();
   return true;
}

bool         
LightUI::save_preset()
{ 
   cerr << "LightUI::save_preset()" << endl;
   char* f = **(_presets_ui->get_filename());
   if(!f){
      err_msg("LightUI::save_preset - file not specified");
      return false;
   }
   fstream fout;
   fout.open(f,ios::out);
   //If this fails, then dump the null string
   if (!fout) 
   {
      err_msg("LightUI::save_preset -  Could not open: '%s', so changing to using no external npr file...!", f);   
      return false;
   }
   //Otherwise, do the right thing
   else
   {
      
      //Set the flag so tags will know we're in a data file
      
      // the ((VIEW*)this)-> stuff is to "cast away" the 
      // const of this function so that the _in_data_file
      // member can be modified
      //((VIEW*)this)->_in_data_file = true;
      STDdstream stream(&fout);
      _view->format(stream);
      //((VIEW*)this)->_in_data_file = false;

   }
   cerr << " light saved " << endl;
   return true;
}



void
LightUI::update_light()
{

   int i = _radgroup[RADGROUP_LIGHTNUM]->get_int_val();

   if (i == RADBUT_LIGHTG) {
      _radgroup[RADGROUP_LIGHTCOL]->disable();
      _checkbox[CHECK_POS]->disable();
      _checkbox[CHECK_CAM]->disable();
      _checkbox[CHECK_ENABLE]->disable();
      _rotation[ROT_LIGHT]->disable();

      COLOR c = _view->light_get_global_ambient();
        
      _color_ui->set_current_color(c[0],c[1],c[2], true, true); 

      //HSVCOLOR hsv(c);

      //_slider[SLIDE_LH]->set_float_val(hsv[0]);
      //_slider[SLIDE_LS]->set_float_val(hsv[1]);
      //_slider[SLIDE_LV]->set_float_val(hsv[2]);

   } else {
      _radgroup[RADGROUP_LIGHTCOL]->enable();
      _checkbox[CHECK_POS]->enable();
      _checkbox[CHECK_CAM]->enable();
      _checkbox[CHECK_ENABLE]->enable();     
      _rotation[ROT_LIGHT]->enable();

      _light_dir = _view->light_get_coordinates_v(i);

//      float rad = _light_dir.length();

      _light_dir = _light_dir.normalized();

      _rotation[ROT_LIGHT]->reset();
      _rotation[ROT_SPOT]->reset();

      _spinner[SPINNER_LIGHT_DIR_X]->set_float_val(_light_dir[0]);
      _spinner[SPINNER_LIGHT_DIR_Y]->set_float_val(_light_dir[1]);
      _spinner[SPINNER_LIGHT_DIR_Z]->set_float_val(_light_dir[2]);

      COLOR c = (_radgroup[RADGROUP_LIGHTCOL]->get_int_val()==0)?
                (_view->light_get_diffuse(i)):
                (_view->light_get_ambient(i));
      _color_ui->set_current_color(c[0],c[1],c[2], true, true); 
     
       _spot_dir =  _view->light_get_spot_direction(i);
         _spinner[SPINNER_LIGHT_SPOT_X]->set_float_val(_spot_dir[0]);
         _spinner[SPINNER_LIGHT_SPOT_Y]->set_float_val(_spot_dir[1]);
         _spinner[SPINNER_LIGHT_SPOT_Z]->set_float_val(_spot_dir[2]);
         _slider[SLIDE_SPOT_EXPONENT]->set_float_val(_view->light_get_spot_exponent(i));
         int cutoff = (int)_view->light_get_spot_cutoff(i);
         cutoff = (cutoff > 90) ? 90 : cutoff;
         _slider[SLIDE_SPOT_CUTOFF]->set_int_val(cutoff);
         _slider[SLIDE_SPOT_K0]->set_float_val(_view->light_get_constant_attenuation(i));
         _slider[SLIDE_SPOT_K1]->set_float_val(_view->light_get_linear_attenuation(i));
         _slider[SLIDE_SPOT_K2]->set_float_val(_view->light_get_quadratic_attenuation(i));

      _checkbox[CHECK_ENABLE]->set_int_val((_view->light_get_enable(i))?(1):(0));
      _checkbox[CHECK_POS]->set_int_val((_view->light_get_positional(i))?(0):(1));

      if(_view->light_get_positional(i)){
         _rollout[ROLLOUT_SPOT]->enable();
         _spinner[SPINNER_LIGHT_SPOT_X]->enable();
         _spinner[SPINNER_LIGHT_SPOT_Y]->enable();
         _spinner[SPINNER_LIGHT_SPOT_Z]->enable();
         _slider[SLIDE_SPOT_EXPONENT]->enable();
         _slider[SLIDE_SPOT_CUTOFF]->enable();
         _slider[SLIDE_SPOT_K0]->enable();
         _slider[SLIDE_SPOT_K1]->enable();
         _slider[SLIDE_SPOT_K2]->enable();
         _rollout[ROLLOUT_SPOT]->open();
        
      }else{
         _rollout[ROLLOUT_SPOT]->disable();
         _rollout[ROLLOUT_SPOT]->close();
      }
      _checkbox[CHECK_CAM]->set_int_val((_view->light_get_in_cam_space(i))?(1):(0));
   }
}

void
LightUI::apply_light()
{
   int i = _radgroup[RADGROUP_LIGHTNUM]->get_int_val();

   if (i == RADBUT_LIGHTG) {     
      COLOR c = _color_ui->get_color();
      _view->light_set_global_ambient(c);
   } else {
      float mat[4][4];
      _rotation[ROT_LIGHT]->get_float_array_val((float *)mat);

      Wvec c0(mat[0][0],mat[0][1],mat[0][2]);
      Wvec c1(mat[1][0],mat[1][1],mat[1][2]);
      Wvec c2(mat[2][0],mat[2][1],mat[2][2]);
      Wtransf t(c0,c1,c2);

      Wvec new_dir = (t*_light_dir).normalized();
      
      _spinner[SPINNER_LIGHT_DIR_X]->set_float_val(new_dir[0]);
      _spinner[SPINNER_LIGHT_DIR_Y]->set_float_val(new_dir[1]);
      _spinner[SPINNER_LIGHT_DIR_Z]->set_float_val(new_dir[2]);

      _view->light_set_coordinates_v(i, new_dir);
      
      COLOR c = _color_ui->get_color();

      if (_radgroup[RADGROUP_LIGHTCOL]->get_int_val()==0)
         _view->light_set_diffuse(i,c);
      else
         _view->light_set_ambient(i,c);

      _view->light_set_enable(i,       (_checkbox[CHECK_ENABLE]->get_int_val()) == 1);
      _view->light_set_positional(i,   (_checkbox[CHECK_POS]->get_int_val()) == 0);
      _view->light_set_in_cam_space(i, (_checkbox[CHECK_CAM]->get_int_val()) == 1);

      
      if(_view->light_get_positional(i)){
         _rollout[ROLLOUT_SPOT]->enable();
         _spinner[SPINNER_LIGHT_SPOT_X]->enable();
         _spinner[SPINNER_LIGHT_SPOT_Y]->enable();
         _spinner[SPINNER_LIGHT_SPOT_Z]->enable();
         _slider[SLIDE_SPOT_EXPONENT]->enable();
         _slider[SLIDE_SPOT_CUTOFF]->enable();
         _slider[SLIDE_SPOT_K0]->enable();
         _slider[SLIDE_SPOT_K1]->enable();
         _slider[SLIDE_SPOT_K2]->enable();
         _rollout[ROLLOUT_SPOT]->open();
         
         float mat2[4][4];
         _rotation[ROT_SPOT]->get_float_array_val((float *)mat2);
          Wvec c02(mat2[0][0],mat2[0][1],mat2[0][2]);
          Wvec c12(mat2[1][0],mat2[1][1],mat2[1][2]);
          Wvec c22(mat2[2][0],mat2[2][1],mat2[2][2]);
          Wtransf t2(c02,c12,c22);
          Wvec spot_dir = (t2*_spot_dir).normalized();

          _spinner[SPINNER_LIGHT_SPOT_X]->set_float_val(spot_dir[0]);
          _spinner[SPINNER_LIGHT_SPOT_Y]->set_float_val(spot_dir[1]);
          _spinner[SPINNER_LIGHT_SPOT_Z]->set_float_val(spot_dir[2]);

         _view->light_set_spot_direction(i,spot_dir);
         _view->light_set_spot_exponent(i,_slider[SLIDE_SPOT_EXPONENT]->get_float_val());
         _view->light_set_spot_cutoff(i,_slider[SLIDE_SPOT_CUTOFF]->get_int_val());
         _view->light_set_constant_attenuation(i,_slider[SLIDE_SPOT_K0]->get_float_val());
         _view->light_set_linear_attenuation(i,_slider[SLIDE_SPOT_K1]->get_float_val());
         _view->light_set_quadratic_attenuation(i,_slider[SLIDE_SPOT_K2]->get_float_val());
      }else{
         _view->light_set_spot_direction(i,Wvec(0,0,-1));
         _view->light_set_spot_exponent(i,0);
         _view->light_set_spot_cutoff(i, 180);  
         _view->light_set_constant_attenuation(i,1);
         _view->light_set_linear_attenuation(i,0);
         _view->light_set_quadratic_attenuation(i,0);

         _rollout[ROLLOUT_SPOT]->disable();
         _rollout[ROLLOUT_SPOT]->close();
      }
   }
}

void
LightUI::listbox_cb(int id)
{
   
}

void
LightUI::button_cb(int id)
{
   
}

void
LightUI::slider_cb(int id)
{
//   static bool HACK_EYE_POS = Config::get_var_bool("HACK_EYE_POS",false);
  
   switch (id) {
   case SLIDE_LH:
   case SLIDE_LS:
   case SLIDE_LV:
   case SLIDE_SPOT_EXPONENT:
   case SLIDE_SPOT_CUTOFF:
   case SLIDE_SPOT_K0:
   case SLIDE_SPOT_K1:
   case SLIDE_SPOT_K2:
      _ui->apply_light();
      break;  
   }
}

void 
LightUI::spinner_cb(int id)
{  
   int i = _ui->_radgroup[RADGROUP_LIGHTNUM]->get_int_val();
   Wvec new_dir;
   switch (id) {
      case SPINNER_LIGHT_DIR_X:
         _ui->_light_dir[0] = _ui->_spinner[SPINNER_LIGHT_DIR_X]->get_float_val();
         new_dir = (_ui->_light_dir).normalized();
         _ui->_view->light_set_coordinates_v(i, new_dir);
         break;
      case SPINNER_LIGHT_DIR_Y:
         _ui->_light_dir[1] = _ui->_spinner[SPINNER_LIGHT_DIR_Y]->get_float_val();
         new_dir = (_ui->_light_dir).normalized();
         _ui->_view->light_set_coordinates_v(i, new_dir);
         break;
      case SPINNER_LIGHT_DIR_Z:
         _ui->_light_dir[2] = _ui->_spinner[SPINNER_LIGHT_DIR_Z]->get_float_val();
         new_dir = (_ui->_light_dir).normalized();
         _ui->_view->light_set_coordinates_v(i, new_dir);
         break;
      case SPINNER_LIGHT_SPOT_X:
         _ui->_spot_dir[0] = _ui->_spinner[SPINNER_LIGHT_SPOT_X]->get_float_val();
         new_dir = (_ui->_spot_dir).normalized();
         _ui->_view->light_set_spot_direction(i, new_dir);
         break;
      case SPINNER_LIGHT_SPOT_Y:
          _ui->_spot_dir[1] = _ui->_spinner[SPINNER_LIGHT_SPOT_Y]->get_float_val();
         new_dir = (_ui->_spot_dir).normalized();
         _ui->_view->light_set_spot_direction(i, new_dir);
         break;
      case SPINNER_LIGHT_SPOT_Z:
          _ui->_spot_dir[2] = _ui->_spinner[SPINNER_LIGHT_SPOT_Z]->get_float_val();
         new_dir = (_ui->_spot_dir).normalized();
         _ui->_view->light_set_spot_direction(i, new_dir);
         break;
      }
}

void
LightUI::radiogroup_cb(int id)
{   
   switch (id) {
   case RADGROUP_LIGHTNUM:
   case RADGROUP_LIGHTCOL:
      _ui->update_light();
      break;
   }
}

void
LightUI::checkbox_cb(int id)
{
   switch (id) {
   case CHECK_POS:
      _ui->_rotation[ROT_LIGHT]->reset();
   case CHECK_ENABLE:
   case CHECK_CAM:
      _ui->apply_light();
      break;   
   }
}

void
LightUI::rotation_cb(int id)
{    
   switch (id) {
   case ROT_LIGHT:
      _ui->apply_light();     
      break;
   case ROT_SPOT:
      _ui->apply_light();     
      break;   
   }
}

// view_ui.C
