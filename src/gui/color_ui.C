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
// ColorUI
////////////////////////////////////////////

#include "std/support.H"
#include <GL/glew.h>
#include <fstream>

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui_jot.H"
#include "std/config.H"

#include "color_ui.H"
#include "presets_ui.H"
#include "disp/colors.H"

using namespace mlib;

#define ID_SHIFT                     10
#define ID_MASK                      ((1<<ID_SHIFT)-1)

/*****************************************************************
 * ColorUI
 *****************************************************************/
vector<ColorUI*>         ColorUI::_ui;

const static int PRESET_NUMBER=12; 

ColorUI::ColorUI(BaseUI* parent) :
     BaseUI(parent, "ColorUI"),
        _color(COLOR(1.0,1.0,1.0)),
        _rgb(false),
        _set_palette_mode(false)

{
    _presets_ui = new PresetsUI(this, "nprdata/color_presets/", ".col");
   _ui.push_back(this);
   _id = (_ui.size()-1);  
}

void     
ColorUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
   int id = _id << ID_SHIFT;
   //Lighting
   _rollout[ROLLOUT_MAIN] = (base) ? new GLUI_Rollout(base, "Color", open)
                                   : new GLUI_Rollout(glui, "ColorUI", open);
   
   //_panel[PANEL_COLORS] = new GLUI_Panel(_rollout[ROLLOUT_MAIN], "hello");
   
   for(int i=PRESET; i < PRESET+PRESET_NUMBER; ++i){
     _button[i] = new GLUI_Button(
         _rollout[ROLLOUT_MAIN],  "", 
         id+i, button_cb);
      _button[i]->set_w(20);
      _button[i]->set_h(20);
      if(i == ((PRESET+PRESET_NUMBER)/2))
         new GLUI_Column(_rollout[ROLLOUT_MAIN], false);
   }
   new GLUI_Column(_rollout[ROLLOUT_MAIN], true);
   _button[BUT_CURRENT] = new GLUI_Button(
         _rollout[ROLLOUT_MAIN],  "Now", 
         id+BUT_CURRENT, button_cb);
   _button[BUT_CURRENT]->set_w(40);
   _button[BUT_CURRENT]->set_h(40);
   _button[BUT_LAST] = new GLUI_Button(
         _rollout[ROLLOUT_MAIN],  "Last", 
         id+BUT_LAST, button_cb);
   _button[BUT_LAST]->set_w(40);
   _button[BUT_LAST]->set_h(40);
   _radgroup[RADGROUP_COLOR] = new GLUI_RadioGroup(
                                     _rollout[ROLLOUT_MAIN],
                                     nullptr,
                                     id+RADGROUP_COLOR, radiogroup_cb);
   _radbutton[RADBUT_HSV] = new GLUI_RadioButton(
                                   _radgroup[RADGROUP_COLOR],
                                   "HSV");
   _radbutton[RADBUT_RGB] = new GLUI_RadioButton(
                                   _radgroup[RADGROUP_COLOR],
                                   "RGB");
   _radgroup[RADGROUP_COLOR]->set_int_val(int(_rgb));
   new GLUI_Column(_rollout[ROLLOUT_MAIN], true);

   _slider[SLIDER_R] = new GLUI_Slider(
      _rollout[ROLLOUT_MAIN], "Hue", 
      id+SLIDER_R, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0, 
      nullptr);
   _slider[SLIDER_R]->set_w(150);
   _slider[SLIDER_R]->set_num_graduations(200);
  
   _slider[SLIDER_G] = new GLUI_Slider(
      _rollout[ROLLOUT_MAIN], "Saturation", 
      id+SLIDER_G, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0, 
      nullptr);
   _slider[SLIDER_G]->set_w(150);
   _slider[SLIDER_G]->set_num_graduations(200);

   _slider[SLIDER_B] = new GLUI_Slider(
      _rollout[ROLLOUT_MAIN], "Value", 
      id+SLIDER_B, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0, 
      nullptr);
   _slider[SLIDER_B]->set_w(150);
   _slider[SLIDER_B]->set_num_graduations(200);

   new GLUI_Column(_rollout[ROLLOUT_MAIN], true);

   _edittext[EDITTEXT_R] =  new GLUI_EditText(
      _rollout[ROLLOUT_MAIN], 
      "H", 
		GLUI_EDITTEXT_INT, 
      nullptr,
		id+EDITTEXT_R, edittext_cb);
   _edittext[EDITTEXT_R]->set_w(40);
   _edittext[EDITTEXT_R]->set_int_limits(0,100);
   _edittext[EDITTEXT_G] =  new GLUI_EditText(
      _rollout[ROLLOUT_MAIN], 
      "S", 
      GLUI_EDITTEXT_INT,
      nullptr,
      id+EDITTEXT_G, edittext_cb);
   _edittext[EDITTEXT_G]->set_w(40);
   _edittext[EDITTEXT_G]->set_int_limits( 0,100);
   _edittext[EDITTEXT_B] =  new GLUI_EditText(
      _rollout[ROLLOUT_MAIN], 
      "V", 
      GLUI_EDITTEXT_INT,
      nullptr,
      id+EDITTEXT_B, edittext_cb);
   _edittext[EDITTEXT_B]->set_w(40);
   _edittext[EDITTEXT_B]->set_int_limits( 0,100);

    _edittext[EDITTEXT_A] =  new GLUI_EditText(
      _rollout[ROLLOUT_MAIN], 
      "A", 
      GLUI_EDITTEXT_INT,
      nullptr,
      id+EDITTEXT_A, edittext_cb);
   _edittext[EDITTEXT_A]->set_w(40);
   _edittext[EDITTEXT_A]->set_int_limits( 0,100);

   new GLUI_Column(_rollout[ROLLOUT_MAIN], true);
   
   _presets_ui->build(glui,_rollout[ROLLOUT_MAIN], true); 

   for (int i=0; i<ROLLOUT_NUM; i++){
      if(_rollout[i])
      _rollout[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
  
   vector<COLOR> colors;
   colors.push_back(Color::white);
   colors.push_back(Color::white);
   colors.push_back(Color::blue1);
   colors.push_back(Color::blue2);
   colors.push_back(Color::blue3);
   colors.push_back(Color::blue4);
   colors.push_back(Color::brown);
   colors.push_back(Color::brown1);
   colors.push_back(Color::brown2);
   colors.push_back(Color::brown3);
   colors.push_back(Color::brown4);
   colors.push_back(Color::brown5);
   colors.push_back(Color::green1);
   colors.push_back(Color::green2);
   apply_color(colors);

   switch_color_mode(_rgb);
}

void 
ColorUI::update_non_lives()
{
   _presets_ui->update_non_lives();
}

bool
ColorUI::child_callback(BaseUI* sender, int event)
{
   bool s = false;
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
   return s;
}

bool
ColorUI::load_preset()
{
   const char* f = _presets_ui->get_filename().c_str();
   ifstream fin(f, ifstream::in);
   bool result = true;

   if (!fin)    {
      err_msg("ColorUI::load_preset() - Error! Could not open file: '%s'", f);
      return false;
   }

   vector<COLOR> c;
   vector<COLOR>::size_type count;
   fin >> count;

   for (vector<COLOR>::size_type i = 0; i < count; i++) {
      char open, close;
      float r, g, b;

      if (fin >> open >> r >> g >> b >> close) {
         c.push_back(COLOR(r, g, b));
      } else {
         result = false;
         break;
      }
   }

   if (result)
      apply_color(c);

   fin.close();
   return result;
}

bool         
ColorUI::save_preset()
{ 
   const char* f = _presets_ui->get_filename().c_str();
   if(!f)
      return false;
   fstream fout;
   fout.open(f,ios::out);
   
   if (!fout) {
      err_msg("ColorUI::save_preset - Error! Could not open file: '%s'", f);         
      return false;
   }

   fout << BUT_NUM << endl;

   for (int i=0; i<BUT_NUM; ++i){
      float *col = _button[i]->glui->bkgd_color_f;
      fout << "< " << col[0] << ' ' << col[1] << ' ' << col[2] << " >" << endl;
   }

   fout.close();
   return true;
}

void         
ColorUI::set_alpha(float a)
{
   _alpha = a;
   _edittext[EDITTEXT_B]->set_float_val(int(a*100));
}

void         
ColorUI::set_current_color(float a, float b, float c, bool rgb_input, bool update_last)
{
   COLOR tmp(a, b, c);
   _color = tmp;
   int mult = 255;

   if(!_rgb){
      HSVCOLOR hsv(tmp[0],tmp[1],tmp[2]);
      if(rgb_input){
         // Convert rgb -> hsv
         hsv = HSVCOLOR(tmp);
      }
      tmp[0] = hsv[0];
      tmp[1] = hsv[1];
      tmp[2] = hsv[2];

      _color = COLOR(hsv);
      mult=100;
   
   }else{
      if(!rgb_input){
         // Convert hsv -> rgb
         tmp = COLOR(HSVCOLOR(tmp[0],tmp[1],tmp[2]));
      }
   }
 
   _slider[SLIDER_R]->set_float_val(tmp[0]);
   _slider[SLIDER_G]->set_float_val(tmp[1]);
   _slider[SLIDER_B]->set_float_val(tmp[2]);

   _edittext[EDITTEXT_R]->set_float_val(int(tmp[0]*mult));
   _edittext[EDITTEXT_G]->set_float_val(int(tmp[1]*mult));
   _edittext[EDITTEXT_B]->set_float_val(int(tmp[2]*mult));

   GLUI *current = _button[BUT_CURRENT]->glui;
   if (update_last) {
      GLUI *last = _button[BUT_LAST]->glui;
      for (int i=0; i < 3; i++) {
	 last->bkgd_color[i] = current->bkgd_color[i];
	 last->bkgd_color_f[i] = current->bkgd_color_f[i];
      }
   }
   for (int i=0; i < 3; i++) {
      current->bkgd_color_f[i] = _color[i];
      current->bkgd_color[i] = (unsigned char)(_color[i] * 255.0);
   }
}


void         
ColorUI::switch_color_mode(bool rgb)
{
  _rgb = rgb;
  
  if(_rgb){
   _slider[SLIDER_R]->set_name("Red");
   _slider[SLIDER_G]->set_name("Green");
   _slider[SLIDER_B]->set_name("Blue");

   _slider[SLIDER_R]->set_float_val(_color[0]);
   _slider[SLIDER_G]->set_float_val(_color[1]);
   _slider[SLIDER_B]->set_float_val(_color[2]);

   _edittext[EDITTEXT_R]->set_name("R");
   _edittext[EDITTEXT_G]->set_name("G");
   _edittext[EDITTEXT_B]->set_name("B");
   _edittext[EDITTEXT_R]->set_int_limits(0,255);
   _edittext[EDITTEXT_G]->set_int_limits(0,255);
   _edittext[EDITTEXT_B]->set_int_limits(0,255);
   _edittext[EDITTEXT_R]->set_int_val(int(_color[0]*255));
   _edittext[EDITTEXT_G]->set_int_val(int(_color[1]*255));
   _edittext[EDITTEXT_B]->set_int_val(int(_color[2]*255));

  }else{
   HSVCOLOR hsv(_color);
   _slider[SLIDER_R]->set_name("Hue");
   _slider[SLIDER_G]->set_name("Saturation");
   _slider[SLIDER_B]->set_name("Value");
   _slider[SLIDER_R]->set_float_val(hsv[0]);
   _slider[SLIDER_G]->set_float_val(hsv[1]);
   _slider[SLIDER_B]->set_float_val(hsv[2]);

   _edittext[EDITTEXT_R]->set_name("H");
   _edittext[EDITTEXT_G]->set_name("S");
   _edittext[EDITTEXT_B]->set_name("V");
   _edittext[EDITTEXT_R]->set_int_limits(0,100);
   _edittext[EDITTEXT_G]->set_int_limits(0,100);
   _edittext[EDITTEXT_B]->set_int_limits(0,100);

   _edittext[EDITTEXT_R]->set_int_val(int(hsv[0]*100));
   _edittext[EDITTEXT_G]->set_int_val(int(hsv[1]*100));
   _edittext[EDITTEXT_B]->set_int_val(int(hsv[2]*100));
  }
}

void
ColorUI::apply_color(const vector<COLOR>& colors)
{
   //We get a list of 2 + 12 colors max and 2 min
   if (colors.size() < 2){
      cerr << "ColorUI::apply_color: Less then 2 colors given" << endl;
      return;
   }

   for (int i=0; i < BUT_NUM; ++i) {
      if (i < (int)colors.size()) {
         for (int j=0; j < 3; j++) {
            _button[i]->glui->bkgd_color_f[j] = colors[i][j];
            _button[i]->glui->bkgd_color[j] = (unsigned char)(colors[i][j] * 255.0);
         }
      } else {
         for (int j=0; j < 3; j++) {
            _button[i]->glui->bkgd_color[j] = 0;
            _button[i]->glui->bkgd_color_f[j] = 0.0;
         }
      }
   }
}


void
ColorUI::button_cb(int id)
{
   float *c;
   switch(id&ID_MASK)
   {
   case BUT_LAST:     
      c = _ui[id >> ID_SHIFT]->_button[BUT_LAST]->glui->bkgd_color_f;
      _ui[id >> ID_SHIFT]->set_current_color(c[0], c[1], c[2], true, true);
      _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT], 0);
     
      break;
   case BUT_CURRENT:
       
      if(!(_ui[id >> ID_SHIFT]->_set_palette_mode)){
          _ui[id >> ID_SHIFT]->_set_palette_mode = true;
          _ui[id >> ID_SHIFT]->_button[BUT_CURRENT]->draw_pressed();
          WORLD::message("Click PALLET to overwite with current color, To CANCEL click THIS button again");
      }else{
         _ui[id >> ID_SHIFT]->_set_palette_mode = false;
         if (_ui[id >> ID_SHIFT]->_button[BUT_CURRENT]->can_draw())
            _ui[id >> ID_SHIFT]->_button[BUT_CURRENT]->translate_and_draw_front();
      }
      
      break;
   case PRESET:
   case PRESET1:
   case PRESET2:
   case PRESET3:
   case PRESET4:
   case PRESET5:
   case PRESET6:
   case PRESET7:
   case PRESET8:
   case PRESET9:
   case PRESET10:
   case PRESET11:

      if (_ui[id >> ID_SHIFT]->_set_palette_mode) {
         GLUI *current = _ui[id >> ID_SHIFT]->_button[BUT_CURRENT]->glui;
         GLUI *mask = _ui[id >> ID_SHIFT]->_button[id&ID_MASK]->glui;
         for (int i=0; i < 3; i++) {
            mask->bkgd_color[i] = current->bkgd_color[i];
            mask->bkgd_color_f[i] = current->bkgd_color_f[i];
         }
         _ui[id >> ID_SHIFT]->_set_palette_mode = false;
      } else {
         c = _ui[id >> ID_SHIFT]->_button[id&ID_MASK]->glui->bkgd_color_f;
         _ui[id >> ID_SHIFT]->set_current_color(c[0], c[1], c[2], true, true);
         _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT], 0);
      }
      break;
   }
}

void 
ColorUI::slider_cb(int id)
{  
   float c[3];

   switch(id&ID_MASK)
   {
   case SLIDER_R: 
   case SLIDER_G: 
   case SLIDER_B:
       c[0] = _ui[id >> ID_SHIFT]->_slider[SLIDER_R]->get_float_val();
       c[1] = _ui[id >> ID_SHIFT]->_slider[SLIDER_G]->get_float_val();
       c[2] = _ui[id >> ID_SHIFT]->_slider[SLIDER_B]->get_float_val();
       _ui[id >> ID_SHIFT]->set_current_color(c[0], c[1], c[2], _ui[id >> ID_SHIFT]->_rgb, false);
       _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT], 0);
      break;
   }
}

void  
ColorUI::radiogroup_cb(int id)
{
   switch(id&ID_MASK)
   {
   case RADGROUP_COLOR:
      bool rgb = (_ui[id >> ID_SHIFT]->_radgroup[RADGROUP_COLOR]->get_int_val() == 1);
      _ui[id >> ID_SHIFT]->switch_color_mode(rgb);
      _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT], 0);
      break;
   }
}

void  
ColorUI::edittext_cb(int id)
{
   float c[3];
   float mult = (_ui[id >> ID_SHIFT]->_rgb) ? (1.0/255.0) : (0.01);
   switch(id&ID_MASK)
   {
   case EDITTEXT_R: 
   case EDITTEXT_G: 
   case EDITTEXT_B:
       c[0] = _ui[id >> ID_SHIFT]->_edittext[SLIDER_R]->get_int_val()*mult;
       c[1] = _ui[id >> ID_SHIFT]->_edittext[SLIDER_G]->get_int_val()*mult;
       c[2] = _ui[id >> ID_SHIFT]->_edittext[SLIDER_B]->get_int_val()*mult;

       _ui[id >> ID_SHIFT]->set_current_color(c[0], c[1], c[2], _ui[id >> ID_SHIFT]->_rgb, false);
       _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT], 0);
      break;
   case EDITTEXT_A:
       _ui[id >> ID_SHIFT]->_alpha = _ui[id >> ID_SHIFT]->_edittext[SLIDER_B]->get_int_val()*0.01;
       _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT], 0);
   }
}



// color_ui.C
