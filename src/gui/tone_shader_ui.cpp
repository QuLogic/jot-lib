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
// ToneShaderUI
////////////////////////////////////////////

#include "std/support.H"
#include <GL/glew.h>

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui_jot.H"
#include "std/config.H"

#include "gtex/tone_shader.H"

#include "tone_shader_ui.H"

using namespace mlib;

#define ID_SHIFT  10
#define ID_MASK   ((1<<ID_SHIFT)-1)

/*****************************************************************
 * ToneShaderUI
 *****************************************************************/

vector<ToneShaderUI*> ToneShaderUI::_ui;

ToneShaderUI::ToneShaderUI(BaseUI* parent) :
     BaseUI(parent,"Tone Shader UI")
{
   _ui.push_back(this);
   _id = (_ui.size()-1);  
}


void     
ToneShaderUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
   int id = _id << ID_SHIFT;

   //Lighting
   _rollout[ROLLOUT_MAIN] = (base) ? new GLUI_Rollout(base, "Tone Shader UI", open)
                                    : new GLUI_Rollout(glui, "Tone Shader UI", open);
   
   _checkbox[CHECK_ENABLED] = new GLUI_Checkbox(
      _rollout[ROLLOUT_MAIN],
      "Enabled",
      nullptr,
      id+CHECK_ENABLED,
      checkbox_cb);
   _checkbox[CHECK_REMAP_NL] = new GLUI_Checkbox(
      _rollout[ROLLOUT_MAIN],
      "Remap NL",
      nullptr,
      id+CHECK_REMAP_NL,
      checkbox_cb);

   
   _panel[PANEL_GROUPS] = new GLUI_Panel(_rollout[ROLLOUT_MAIN], "");
   // Remap Radio Group
   _panel[PANEL_REMAP] = new GLUI_Panel(_panel[PANEL_GROUPS], "Remap");
   _radgroup[RADGROUP_REMAP] = new GLUI_RadioGroup(
       _panel[PANEL_REMAP],
      nullptr,
      id+RADGROUP_REMAP, radiogroup_cb);
   _radbutton[RADBUT_REMAP_NONE] = new GLUI_RadioButton(
      _radgroup[RADGROUP_REMAP],
      "None");
   _radbutton[RADBUT_REMAP_TOON] = new GLUI_RadioButton(
      _radgroup[RADGROUP_REMAP],
      "Toon");
   _radbutton[RADBUT_REMAP_SMOTHSTEP] = new GLUI_RadioButton(
      _radgroup[RADGROUP_REMAP],
      "Smoothstep");

   new GLUI_Column(_panel[PANEL_GROUPS], false);
   
   // backlight radio group
   _panel[PANEL_BACKLIGHT] = new GLUI_Panel(_panel[PANEL_GROUPS], "Backlight");
   _radgroup[RADGROUP_BACKLIGHT] = new GLUI_RadioGroup(
       _panel[PANEL_BACKLIGHT],
      nullptr,
      id+RADGROUP_BACKLIGHT, radiogroup_cb);
   _radbutton[RADBUT_BACKLIGHT_NONE] = new GLUI_RadioButton(
      _radgroup[RADGROUP_BACKLIGHT],
      "None");
   _radbutton[RADBUT_BACKLIGHT_DARK] = new GLUI_RadioButton(
      _radgroup[RADGROUP_BACKLIGHT],
      "Dark");
   _radbutton[RADBUT_BACKLIGHT_LIGHT] = new GLUI_RadioButton(
      _radgroup[RADGROUP_BACKLIGHT],
      "Light");

   //Sliders
   _slider[SLIDE_REMAP_A] = new GLUI_Slider(
      _rollout[ROLLOUT_MAIN], 
      "Remap smoothstep A", 
      id+SLIDE_REMAP_A, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      nullptr);
   _slider[SLIDE_REMAP_A]->set_num_graduations(100);
   _slider[SLIDE_REMAP_A]->set_w(200);

   _slider[SLIDE_REMAP_B] = new GLUI_Slider(
      _rollout[ROLLOUT_MAIN], 
      "Remap smoothstep B", 
      id+SLIDE_REMAP_B, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      nullptr);
   _slider[SLIDE_REMAP_B]->set_num_graduations(100);
   _slider[SLIDE_REMAP_B]->set_w(200);

   _slider[SLIDE_BACKLIGHT_A] = new GLUI_Slider(
      _rollout[ROLLOUT_MAIN], 
      "Backligh smoothstep A", 
      id+SLIDE_BACKLIGHT_A, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      nullptr);
   _slider[SLIDE_BACKLIGHT_A]->set_num_graduations(100);
   _slider[SLIDE_BACKLIGHT_A]->set_w(200);
   _slider[SLIDE_BACKLIGHT_B] = new GLUI_Slider(
      _rollout[ROLLOUT_MAIN], 
      "Backligh smoothstep B", 
      id+SLIDE_BACKLIGHT_B, slider_cb,
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      nullptr);
   _slider[SLIDE_BACKLIGHT_B]->set_num_graduations(100);
   _slider[SLIDE_BACKLIGHT_B]->set_w(200);


   //toon texture selection list 

   _listbox[LIST_TEXTURE] = new GLUI_Listbox(
                               _rollout[ROLLOUT_MAIN],
                               "Toon Map",
                               nullptr,
                               id+LIST_TEXTURE,
                               listbox_cb);
   
   fill_directory_listbox(_listbox[LIST_TEXTURE], _texture_filenames, Config::JOT_ROOT() + "/nprdata/toon_textures_1D/", ".png", false);
   
   _listbox[LIST_TEXTURE]->curr_text = "clear-black.png" ;
  
   // Cleanup sizes
   for (int i=0; i<ROLLOUT_NUM; i++){
      if(_rollout[i])
      _rollout[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   for (int i=0; i<PANEL_NUM; i++){
      if(_panel[i])
      _panel[i]->set_alignment(GLUI_ALIGN_LEFT);
   }

   update();
}

void
ToneShaderUI::update_non_lives()
{
   if(!_parent)
      return;
   ToneShader* tex = _parent->get_tone_shader();
   if(!tex)
      return;

   int layer = _parent->get_current_channel();

   _checkbox[CHECK_ENABLED]->set_int_val(tex->_layer[layer]._is_enabled);
   _checkbox[CHECK_REMAP_NL]->set_int_val(tex->_layer[layer]._remap_nl);
   _radgroup[RADGROUP_REMAP]->set_int_val(tex->_layer[layer]._remap);
   _radgroup[RADGROUP_BACKLIGHT]->set_int_val(tex->_layer[layer]._backlight);
   _slider[SLIDE_REMAP_A]->set_float_val(tex->_layer[layer]._e0);
   _slider[SLIDE_REMAP_B]->set_float_val(tex->_layer[layer]._e1);
   _slider[SLIDE_BACKLIGHT_A]->set_float_val(tex->_layer[layer]._s0);
   _slider[SLIDE_BACKLIGHT_B]->set_float_val(tex->_layer[layer]._s1);

  
}
//toon texture map selection 

void
ToneShaderUI::listbox_cb(int id)
{
    switch(id&ID_MASK)
   {
      case LIST_TEXTURE:
          _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_TEXTURE, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());

      break;
   }
}

void
ToneShaderUI::button_cb(int id)
{
   
}

void
ToneShaderUI::slider_cb(int id)
{
   switch(id&ID_MASK)
   {
      case SLIDE_REMAP_A: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_REMAP_A, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());
         break;
      case SLIDE_REMAP_B:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_REMAP_B, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());
         break;
      case SLIDE_BACKLIGHT_A: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_BACKLIGHT_A, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());         
         break;
      case SLIDE_BACKLIGHT_B:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_BACKLIGHT_B, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());
         break;
   } 
}

void 
ToneShaderUI::spinner_cb(int id)
{  
   
}

void
ToneShaderUI::radiogroup_cb(int id)
{   
   switch(id&ID_MASK)
   {
      case RADGROUP_REMAP: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_REMAP, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());         
         break;
      case RADGROUP_BACKLIGHT:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_BACKLIGHT, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());
         break;
   } 
}

void
ToneShaderUI::checkbox_cb(int id)
{
  switch(id&ID_MASK)
   {
      case CHECK_ENABLED: 
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_ENABLE, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());         
         break;
      case CHECK_REMAP_NL:
         _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_REMAP_NL, 
                                                      _ui[id >> ID_SHIFT]->_parent->get_tone_shader(),
                                                      _ui[id >> ID_SHIFT]->_parent->get_current_layer());
         break;
   }
}

void
ToneShaderUI::rotation_cb(int id)
{    
  
}

void
ToneShaderUI::apply_changes_to_texture_parent(int op, ToneShader* tex, int layer)
{
   operation_id_t o = (op == OP_ALL) ? OP_ALL : _last_op;
   apply_changes_to_texture(o, tex, layer);
}

void
ToneShaderUI::apply_changes_to_texture(operation_id_t op, ToneShader* tex, int layer)
{   
   if((op==OP_ALL) || (op==OP_ENABLE)){
      tex->_layer[layer]._is_enabled = _checkbox[CHECK_ENABLED]->get_int_val();      
   }
   if((op==OP_ALL) || (op==OP_REMAP_NL)){
      tex->_layer[layer]._remap_nl = _checkbox[CHECK_REMAP_NL]->get_int_val();      
   }
   if((op==OP_ALL) || (op==OP_REMAP)){
      tex->_layer[layer]._remap = _radgroup[RADGROUP_REMAP]->get_int_val();

      if(_radgroup[RADGROUP_REMAP]->get_int_val()==1)
         _listbox[LIST_TEXTURE]->enable();
      else
         _listbox[LIST_TEXTURE]->disable();
   }
   if((op==OP_ALL) || (op==OP_BACKLIGHT)){
      tex->_layer[layer]._backlight = _radgroup[RADGROUP_BACKLIGHT]->get_int_val();  
   }
   if((op==OP_ALL) || (op==OP_REMAP_A)){
      tex->_layer[layer]._e0 = _slider[SLIDE_REMAP_A]->get_float_val();  
   }
   if((op==OP_ALL) || (op==OP_REMAP_B)){
      tex->_layer[layer]._e1 = _slider[SLIDE_REMAP_B]->get_float_val();  
   }   
   if((op==OP_ALL) || (op==OP_BACKLIGHT_A)){
      tex->_layer[layer]._s0 = _slider[SLIDE_BACKLIGHT_A]->get_float_val();  
   }
   if((op==OP_ALL) || (op==OP_BACKLIGHT_B)){
      tex->_layer[layer]._s1 = _slider[SLIDE_BACKLIGHT_B]->get_float_val();  
   } 
   if((op==OP_ALL) || (op==OP_TEXTURE)){
      string name = _listbox[LIST_TEXTURE]->curr_text;
      tex->set_tex(GtexUtil::toon_1D_name(name));
   } 
   
   if(_parent)
      _parent->child_callback(this, op);

   _last_op = op;
}


// patch_ui.C
