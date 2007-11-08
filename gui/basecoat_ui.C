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
// BasecoatUI
////////////////////////////////////////////

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "std/config.H"

#include "gtex/tone_shader.H"

#include "basecoat_ui.H"
#include "npr/img_line_shader.H"
#include "img_line_ui.H"

using namespace mlib;

#define ID_SHIFT  10
#define ID_MASK   ((1<<ID_SHIFT)-1)

/*****************************************************************
 * BasecoatUI
 *****************************************************************/

vector<BasecoatUI*> BasecoatUI::_ui;

const static int WIN_WIDTH=300; 

BasecoatUI::BasecoatUI(BaseUI* parent) :
     BaseUI(parent,"Basecoat UI") 
{
   _ui.push_back(this);
   _id = (_ui.size()-1);  
}


void     
BasecoatUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
   int id = _id << ID_SHIFT;

   _rollout[ROLLOUT_MAIN] 
      = (base) ? glui->add_rollout_to_panel(base, "Basecoat UI",open)
	 : glui->add_rollout("Basecoat UI",open);

   _radgroup[RADGROUP_BASECOAT] = glui->add_radiogroup_to_panel(
				       _rollout[ROLLOUT_MAIN],
				       NULL,
				       id+RADGROUP_BASECOAT, radiogroup_cb);

   _radbutton[RADBUT_BASECOAT_NONE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_BASECOAT],
					  "None");

   _radbutton[RADBUT_BASECOAT_CONSTANT] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_BASECOAT],
					  "Constant");

   _radbutton[RADBUT_BASECOAT_TOON] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_BASECOAT],
					  "Toon");

   _radbutton[RADBUT_BASECOAT_SMOOTHSTEP] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_BASECOAT],
					  "Smoothstep");

   _checkbox[CHECK_LIGHT_SEPARATE] = glui->add_checkbox_to_panel(
                                _rollout[ROLLOUT_MAIN],
                                "Light Seperation",
                                NULL,
                                CHECK_LIGHT_SEPARATE,
                                checkbox_cb);

   glui->add_column_to_panel(_rollout[ROLLOUT_MAIN],true);

   //Sliders
   _slider[SLIDE_REMAP_A] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_MAIN], 
      "Remap smoothstep A", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_REMAP_A, slider_cb);
   _slider[SLIDE_REMAP_A]->set_num_graduations(100);
   _slider[SLIDE_REMAP_A]->set_w(200);

   _slider[SLIDE_REMAP_B] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_MAIN], 
      "Remap smoothstep B", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_REMAP_B, slider_cb);
   _slider[SLIDE_REMAP_B]->set_num_graduations(100);
   _slider[SLIDE_REMAP_B]->set_w(200);

   _slider[SLIDE_COLOR_OFFSET] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_MAIN], 
      "Offset", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_COLOR_OFFSET, slider_cb);
   _slider[SLIDE_COLOR_OFFSET]->set_num_graduations(100);
   _slider[SLIDE_COLOR_OFFSET]->set_w(200);

   _slider[SLIDE_COLOR_STEEPNESS] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_MAIN], 
      "Steepness", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_COLOR_STEEPNESS, slider_cb);
   _slider[SLIDE_COLOR_STEEPNESS]->set_num_graduations(100);
   _slider[SLIDE_COLOR_STEEPNESS]->set_w(200);

   //toon texture selection list 

   _listbox[LIST_TEXTURE] = _glui->add_listbox_to_panel(
                               _rollout[ROLLOUT_MAIN],
                               "Toon Map",
                               0,
                               id+LIST_TEXTURE,
                               listbox_cb);
   
   fill_directory_listbox(_listbox[LIST_TEXTURE],_texture_filenames, Config::JOT_ROOT() + "/nprdata/toon_textures_1D/", ".png", false);
   
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

   //int w = _rollout[ROLLOUT_MAIN]->get_w();
   //w = max(WIN_WIDTH, w);
}

void
BasecoatUI::update_non_lives()
{
   if(!_parent)
      return;

   _current_tex = ((ImageLineUI*)_parent)->get_image_line_shader();

   if(!_current_tex)
      return;

      _radgroup[RADGROUP_BASECOAT]->set_int_val(_current_tex->get_basecoat_shader()->get_basecoat_mode());

      _slider[SLIDE_REMAP_A]->set_float_val(_current_tex->get_basecoat_shader()->_layer[0]._e0);
      _slider[SLIDE_REMAP_B]->set_float_val(_current_tex->get_basecoat_shader()->_layer[0]._e1);
      _slider[SLIDE_COLOR_OFFSET]->set_float_val(_current_tex->get_basecoat_shader()->get_color_offset());
      _slider[SLIDE_COLOR_STEEPNESS]->set_float_val(_current_tex->get_basecoat_shader()->get_color_steepness());

      _checkbox[CHECK_LIGHT_SEPARATE]->set_int_val(_current_tex->get_basecoat_shader()->get_light_separation());
}

void
BasecoatUI::spinner_cb(int id)
{
}

void
BasecoatUI::slider_cb(int id)
{
   int i = id >> ID_SHIFT;
   int layer = _ui[i]->_parent->get_current_layer();
   ImageLineShader *tex = _ui[i]->_current_tex;

   switch(id&ID_MASK)
   {
      case SLIDE_REMAP_A: 
         _ui[i]->apply_changes_to_texture(OP_REMAP_A, tex, layer);
         break;
      case SLIDE_REMAP_B:
         _ui[i]->apply_changes_to_texture(OP_REMAP_B, tex, layer);
      case SLIDE_COLOR_OFFSET:
         _ui[i]->apply_changes_to_texture(OP_COLOR_OFFSET, tex, layer);
         break;
      case SLIDE_COLOR_STEEPNESS:
         _ui[i]->apply_changes_to_texture(OP_COLOR_STEEPNESS, tex, layer);
         break;
   }
}

void
BasecoatUI::radiogroup_cb(int id)
{   
//   int val = _ui[id >> ID_SHIFT]->_radgroup[id & ID_MASK]->get_int_val();
   int layer = _ui[id >> ID_SHIFT]->_parent->get_current_layer();
   int i = id >> ID_SHIFT;

   switch (id & ID_MASK) {
      case RADGROUP_BASECOAT:
	 _ui[i]->apply_changes_to_texture(OP_BASECOAT, _ui[i]->_current_tex, layer);
	 break;
   }
}


void
BasecoatUI::apply_changes_to_texture_parent(int op, ImageLineShader* tex, int layer)
{
   operation_id_t o = (op == OP_ALL) ? OP_ALL : _last_op;
   apply_changes_to_texture(o, tex, layer);
}

void
BasecoatUI::apply_changes_to_texture(operation_id_t op, ImageLineShader* tex, int layer)
{   
   if((op==OP_ALL) || (op==OP_BASECOAT)){
      int val = _radgroup[RADGROUP_BASECOAT]->get_int_val();
      tex->get_basecoat_shader()->set_basecoat_mode(val);

      if(_radgroup[RADGROUP_BASECOAT]->get_int_val()==2)
         _listbox[LIST_TEXTURE]->enable();
      else
         _listbox[LIST_TEXTURE]->disable();
   }
   if((op==OP_ALL) || (op==OP_LIGHT_SEPARATE)){
      int val = _checkbox[CHECK_LIGHT_SEPARATE]->get_int_val();
      tex->get_basecoat_shader()->set_light_separation(val);
   }
   if((op==OP_ALL) || (op==OP_REMAP_A)){
      tex->get_basecoat_shader()->_layer[0]._e0 = _slider[SLIDE_REMAP_A]->get_float_val();  
   }
   if((op==OP_ALL) || (op==OP_REMAP_B)){
      tex->get_basecoat_shader()->_layer[0]._e1 = _slider[SLIDE_REMAP_B]->get_float_val();  
   }
   if((op==OP_ALL) || (op==OP_COLOR_OFFSET)){
      tex->get_basecoat_shader()->set_color_offset(_slider[SLIDE_COLOR_OFFSET]->get_float_val());  
   }
   if((op==OP_ALL) || (op==OP_COLOR_STEEPNESS)){
      tex->get_basecoat_shader()->set_color_steepness(_slider[SLIDE_COLOR_STEEPNESS]->get_float_val());  
   }
   if((op==OP_ALL) || (op==OP_TEXTURE)){
      tex->get_basecoat_shader()->set_tex(GtexUtil::toon_1D_name( _texture_filenames[_listbox[LIST_TEXTURE]->get_int_val()]));  
   } 
   
   if(_parent)
      _parent->child_callback(this, op);

   _last_op = op;
}

void
BasecoatUI::checkbox_cb(int id)
{
   int i = id >> ID_SHIFT;
   int layer = _ui[i]->_parent->get_current_layer();
   ImageLineShader *tex = _ui[i]->_current_tex;

   switch(id&ID_MASK)
   {
      case CHECK_LIGHT_SEPARATE: 
         _ui[i]->apply_changes_to_texture(OP_LIGHT_SEPARATE, tex, layer);
         break;
   }
}


void
BasecoatUI::listbox_cb(int id)
{
   int i = id >> ID_SHIFT;
   int layer = _ui[i]->_parent->get_current_layer();
   ImageLineShader *tex = _ui[i]->_current_tex;

    switch(id&ID_MASK)
   {
      case LIST_TEXTURE:
          _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_TEXTURE, tex, layer);

      break;
   }
}


// basecoat_ui.C
