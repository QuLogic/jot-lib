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
// DetailCtrlUI
////////////////////////////////////////////

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "std/config.H"

#include "gtex/tone_shader.H"

#include "detail_ctrl_ui.H"
#include "npr/img_line_shader.H"
#include "img_line_ui.H"

using namespace mlib;

#define ID_SHIFT  10
#define ID_MASK   ((1<<ID_SHIFT)-1)

/*****************************************************************
 * DetailCtrlUI
 *****************************************************************/

vector<DetailCtrlUI*> DetailCtrlUI::_ui;

const static int WIN_WIDTH=300; 

DetailCtrlUI::DetailCtrlUI(BaseUI* parent) :
     BaseUI(parent,"Detail Control UI"), _detail_type(0)
{
   _ui.push_back(this);
   _id = (_ui.size()-1);  
}


void     
DetailCtrlUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
   int id = _id << ID_SHIFT;

   _rollout[ROLLOUT_MAIN] 
      = (base) ? glui->add_rollout_to_panel(base, "Detail Control UI",open)
	 : glui->add_rollout("Detail Control UI",open);

    _panel[PANEL_DETAIL_TYPE] = glui->add_panel_to_panel( _rollout[ROLLOUT_MAIN], "Detail Types");

   _radgroup[RADGROUP_DETAIL_TYPE] = glui->add_radiogroup_to_panel(
				       _panel[PANEL_DETAIL_TYPE],
				       NULL,
				       id+RADGROUP_DETAIL_TYPE, radiogroup_cb);

   _radbutton[RADBUT_DETAIL_ALL] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_TYPE],
					  "All");

   _radbutton[RADBUT_DETAIL_LINE_WIDTH] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_TYPE],
					  "Line Width");

   _radbutton[RADBUT_DETAIL_BLUR_SIZE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_TYPE],
					  "Blur Size");

   _radbutton[RADBUT_DETAIL_TONE_NORMAL] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_TYPE],
					  "Normal Blending (Tone)");
   _radbutton[RADBUT_DETAIL_BASE_NORMAL] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_TYPE],
					  "Normal Blending (Basecoat)");

    _panel[PANEL_DETAIL_FUNC] = glui->add_panel_to_panel( _rollout[ROLLOUT_MAIN], "Detail Factors");

   _radgroup[RADGROUP_DETAIL_FUNC] = glui->add_radiogroup_to_panel(
				       _panel[PANEL_DETAIL_FUNC],
				       NULL,
				       id+RADGROUP_DETAIL_FUNC, radiogroup_cb);

   _radbutton[RADBUT_DETAIL_FUNC_NONE] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_FUNC],
					  "None");

   _radbutton[RADBUT_DETAIL_FUNC_DEPTH_UNIFORM] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_FUNC],
					  "Depth");

   _radbutton[RADBUT_DETAIL_FUNC_DEPTH_GLOBAL_LENGTH] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_FUNC],
					  "Depth (Global Length)");

   _radbutton[RADBUT_DETAIL_FUNC_DEPTH_LOCAL_LENGTH] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_FUNC],
					  "Depth (Local Length)");

   _radbutton[RADBUT_DETAIL_FUNC_RELATIVE_LENGTH] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_FUNC],
					  "Relative Length");

   _radbutton[RADBUT_DETAIL_FUNC_USER] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_DETAIL_FUNC],
					  "User");

   glui->add_column_to_panel(_rollout[ROLLOUT_MAIN],true);

   _spinner[SPINNER_BOX_SIZE] = glui->add_spinner_to_panel(
                              _rollout[ROLLOUT_MAIN],
                             "Box size",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             SPINNER_BOX_SIZE, spinner_cb);

   _slider[SLIDE_USER_DEPTH] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_MAIN], 
      "User Depth", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_USER_DEPTH, slider_cb);
   _slider[SLIDE_USER_DEPTH]->set_num_graduations(100);
   _slider[SLIDE_USER_DEPTH]->set_w(200);


   _slider[SLIDE_UNIT_LEN] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_MAIN], 
      "Unit Length", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_UNIT_LEN, slider_cb);
   _slider[SLIDE_UNIT_LEN]->set_num_graduations(100);
   _slider[SLIDE_UNIT_LEN]->set_w(200);

   _slider[SLIDE_EDGE_LEN_SCALE] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_MAIN], 
      "Edge Length Scale", 
      GLUI_SLIDER_FLOAT, 
      0.0, 1.0,
      NULL,
      id+SLIDE_EDGE_LEN_SCALE, slider_cb);
   _slider[SLIDE_EDGE_LEN_SCALE]->set_num_graduations(100);
   _slider[SLIDE_EDGE_LEN_SCALE]->set_w(200);

   _slider[SLIDE_RATIO_SCALE] = glui->add_slider_to_panel(
      _rollout[ROLLOUT_MAIN], 
      "Ratio Scale", 
      GLUI_SLIDER_FLOAT, 
      0.0, 10.0,
      NULL,
      id+SLIDE_RATIO_SCALE, slider_cb);
   _slider[SLIDE_RATIO_SCALE]->set_num_graduations(100);
   _slider[SLIDE_RATIO_SCALE]->set_w(200);

   _radgroup[RADGROUP_NORMAL] = glui->add_radiogroup_to_panel(
				       _rollout[ROLLOUT_MAIN],
				       NULL,
				       id+RADGROUP_NORMAL, radiogroup_cb);

   _radbutton[RADBUT_NORMAL_SMOOTH] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_NORMAL],
					  "Smooth");

   _radbutton[RADBUT_NORMAL_SPHERIC] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_NORMAL],
					  "Spheric");
   _radbutton[RADBUT_NORMAL_ELLIPTIC] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_NORMAL],
					  "Elliptic");

   _radbutton[RADBUT_NORMAL_CYLINDRIC] = glui->add_radiobutton_to_group(
					  _radgroup[RADGROUP_NORMAL],
					  "Cylindric");

  
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
DetailCtrlUI::update_non_lives()
{
   if(!_parent)
      return;

   _current_tex = ((ImageLineUI*)_parent)->get_image_line_shader();

   if(!_current_tex)
      return;

   _radgroup[RADGROUP_DETAIL_TYPE]->set_int_val(_detail_type);
   _spinner[SPINNER_BOX_SIZE]->set_float_val(_current_tex->get_ndcz_bounding_box_size());

   if(_detail_type == 0 || _detail_type == 1){
      _radgroup[RADGROUP_DETAIL_FUNC]->set_int_val(_current_tex->get_detail_func());
      _edge_len_scale = _current_tex->get_edge_len_scale();
      _slider[SLIDE_EDGE_LEN_SCALE]->set_float_val(_edge_len_scale);

      _ratio_scale = _current_tex->get_ratio_scale();
      _slider[SLIDE_RATIO_SCALE]->set_float_val(_ratio_scale);

      _unit_len = _current_tex->get_unit_len();
      _slider[SLIDE_UNIT_LEN]->set_float_val(_unit_len);

      _slider[SLIDE_USER_DEPTH]->set_float_val(_current_tex->get_user_depth());
   }
   if(_detail_type == 2){
      _radgroup[RADGROUP_DETAIL_FUNC]->set_int_val(_current_tex->get_blur_shader()->get_detail_func());
      _edge_len_scale = _current_tex->get_blur_shader()->get_edge_len_scale();
      _slider[SLIDE_EDGE_LEN_SCALE]->set_float_val(_edge_len_scale);

      _ratio_scale = _current_tex->get_blur_shader()->get_ratio_scale();
      _slider[SLIDE_RATIO_SCALE]->set_float_val(_ratio_scale);

      _unit_len = _current_tex->get_blur_shader()->get_unit_len();
      _slider[SLIDE_UNIT_LEN]->set_float_val(_unit_len);

      _slider[SLIDE_USER_DEPTH]->set_float_val(_current_tex->get_blur_shader()->get_user_depth());
   }
   if(_detail_type == 3 || _detail_type == 4){
      ToneShader *shader;

      if(_detail_type == 3)
	 shader = _current_tex->get_tone_shader();
      else
	 shader = _current_tex->get_basecoat_shader();

      _radgroup[RADGROUP_DETAIL_FUNC]->set_int_val(shader->get_blend_normal());
      _edge_len_scale = shader->get_edge_len_scale();
      _slider[SLIDE_EDGE_LEN_SCALE]->set_float_val(_edge_len_scale);

      _ratio_scale = shader->get_ratio_scale();
      _slider[SLIDE_RATIO_SCALE]->set_float_val(_ratio_scale);

      _unit_len = shader->get_unit_len();
      _slider[SLIDE_UNIT_LEN]->set_float_val(_unit_len);

      _slider[SLIDE_USER_DEPTH]->set_float_val(shader->get_user_depth());

      if(shader->normals_smoothed())
	_radgroup[RADGROUP_NORMAL]->set_int_val(0);
      else if(shader->normals_spheric())
	_radgroup[RADGROUP_NORMAL]->set_int_val(1);
      else if(shader->normals_elliptic())
	_radgroup[RADGROUP_NORMAL]->set_int_val(2);
      else
	_radgroup[RADGROUP_NORMAL]->set_int_val(3);
   }

  
}

void
DetailCtrlUI::spinner_cb(int id)
{
}

void
DetailCtrlUI::slider_cb(int id)
{
   int i = id >> ID_SHIFT;
   int layer = _ui[i]->_parent->get_current_layer();
   ImageLineShader *tex = _ui[i]->_current_tex;

   switch(id&ID_MASK)
   {
      case SLIDE_USER_DEPTH:
         _ui[i]->apply_changes_to_texture(OP_USER_DEPTH, tex, layer);
         break;
      case SLIDE_EDGE_LEN_SCALE:
	 _ui[i]->apply_changes_to_texture(OP_EDGE_LEN_SCALE, tex, layer);
	 break;
      case SLIDE_RATIO_SCALE:
	 _ui[i]->apply_changes_to_texture(OP_RATIO_SCALE, tex, layer);
	 break;
      case SLIDE_UNIT_LEN:
	 _ui[i]->apply_changes_to_texture(OP_UNIT_LEN, tex, layer);
	 break;
   }
}

void
DetailCtrlUI::radiogroup_cb(int id)
{   
//   int val = _ui[id >> ID_SHIFT]->_radgroup[id & ID_MASK]->get_int_val();
   int layer = _ui[id >> ID_SHIFT]->_parent->get_current_layer();
   int i = id >> ID_SHIFT;

   switch (id & ID_MASK) {
      case RADGROUP_NORMAL: 
	 _ui[i]->apply_changes_to_texture(OP_NORMAL, _ui[i]->_current_tex, layer);
	 break;
      case RADGROUP_DETAIL_TYPE: 
	 _ui[i]->apply_changes_to_texture(OP_DETAIL_TYPE, _ui[i]->_current_tex, layer);
	 break;
      case RADGROUP_DETAIL_FUNC: 
	 _ui[i]->apply_changes_to_texture(OP_DETAIL_FUNC, _ui[i]->_current_tex, layer);
	 break;
   }
}


void
DetailCtrlUI::apply_changes_to_texture_parent(int op, ImageLineShader* tex, int layer)
{
   operation_id_t o = (op == OP_ALL) ? OP_ALL : _last_op;
   apply_changes_to_texture(o, tex, layer);
}

void
DetailCtrlUI::apply_changes_to_texture(operation_id_t op, ImageLineShader* tex, int layer)
{   
   if((op==OP_ALL) || (op==OP_DETAIL_TYPE)){
      _detail_type = _radgroup[RADGROUP_DETAIL_TYPE]->get_int_val();
      update_non_lives();
   }
   if((op==OP_ALL) || (op==OP_DETAIL_FUNC)){
      int val = _radgroup[RADGROUP_DETAIL_FUNC]->get_int_val();

      if(_detail_type == 0){
	 tex->set_detail_func(val);
	 tex->get_basecoat_shader()->set_blend_normal(val);
	 tex->get_tone_shader()->set_blend_normal(val);
	 tex->get_blur_shader()->set_detail_func(val);
      }
      if(_detail_type == 1){
	 tex->set_detail_func(val);
      }
      if(_detail_type == 2){
	 tex->get_blur_shader()->set_detail_func(val);
      }
      if(_detail_type == 3){
	 tex->get_tone_shader()->set_blend_normal(val);
      }
      if(_detail_type == 4){
	 tex->get_basecoat_shader()->set_blend_normal(val);
      }
   }
   if((op==OP_ALL) || (op==OP_UNIT_LEN)){
      _unit_len = _slider[SLIDE_UNIT_LEN]->get_float_val();

      if(_detail_type == 0){
	 tex->set_unit_len(_unit_len);
	 tex->get_blur_shader()->set_unit_len(_unit_len);
	 tex->get_tone_shader()->set_unit_len(_unit_len);
	 tex->get_basecoat_shader()->set_unit_len(_unit_len);
      }
      if(_detail_type == 1){
	 tex->set_unit_len(_unit_len);
      }
      if(_detail_type == 2){
	 tex->get_blur_shader()->set_unit_len(_unit_len);
      }
      if(_detail_type == 3){
	 tex->get_tone_shader()->set_unit_len(_unit_len);
      }
      if(_detail_type == 4){
	 tex->get_basecoat_shader()->set_unit_len(_unit_len);
      }
   }
   if((op==OP_ALL) || (op==OP_EDGE_LEN_SCALE)){
      _edge_len_scale = _slider[SLIDE_EDGE_LEN_SCALE]->get_float_val();
      if(_detail_type == 0){
	 tex->set_edge_len_scale(_edge_len_scale);
	 tex->get_blur_shader()->set_edge_len_scale(_edge_len_scale);
	 tex->get_tone_shader()->set_edge_len_scale(_edge_len_scale);
	 tex->get_basecoat_shader()->set_edge_len_scale(_edge_len_scale);
      }
      if(_detail_type == 1){
	 tex->set_edge_len_scale(_edge_len_scale);
      }
      if(_detail_type == 2){
	 tex->get_blur_shader()->set_edge_len_scale(_edge_len_scale);
      }
      if(_detail_type == 3){
	 tex->get_tone_shader()->set_edge_len_scale(_edge_len_scale);
      }
      if(_detail_type == 4){
	 tex->get_basecoat_shader()->set_edge_len_scale(_edge_len_scale);
      }
   }
   if((op==OP_ALL) || (op==OP_RATIO_SCALE)){
      _ratio_scale = _slider[SLIDE_RATIO_SCALE]->get_float_val();
      if(_detail_type == 0){
	 tex->set_ratio_scale(_ratio_scale);
	 tex->get_blur_shader()->set_ratio_scale(_ratio_scale);
	 tex->get_tone_shader()->set_ratio_scale(_ratio_scale);
	 tex->get_basecoat_shader()->set_ratio_scale(_ratio_scale);
      }
      if(_detail_type == 1){
	 tex->set_ratio_scale(_ratio_scale);
      }
      if(_detail_type == 2){
	 tex->get_blur_shader()->set_ratio_scale(_ratio_scale);
      }
      if(_detail_type == 3){
	 tex->get_tone_shader()->set_ratio_scale(_ratio_scale);
      }
      if(_detail_type == 4){
	 tex->get_basecoat_shader()->set_ratio_scale(_ratio_scale);
      }
   }
   if((op==OP_ALL) || (op==OP_USER_DEPTH)){
      _user_depth = _slider[SLIDE_USER_DEPTH]->get_float_val();

      if(_detail_type == 0){
	 tex->set_user_depth(_user_depth);
	 tex->get_blur_shader()->set_user_depth(_user_depth);
	 tex->get_tone_shader()->set_user_depth(_user_depth);
	 tex->get_basecoat_shader()->set_user_depth(_user_depth);
      }
      if(_detail_type == 1){
	 tex->set_user_depth(_user_depth);
      }
      if(_detail_type == 2){
	 tex->get_blur_shader()->set_user_depth(_user_depth);
      }
      if(_detail_type == 3){
	 tex->get_tone_shader()->set_user_depth(_user_depth);
      }
      if(_detail_type == 4){
	 tex->get_basecoat_shader()->set_user_depth(_user_depth);
      }
   }
   if((op==OP_ALL) || (op==OP_NORMAL)){
      int val = _radgroup[RADGROUP_NORMAL]->get_int_val();

      if(_detail_type == 0){
	 tex->get_basecoat_shader()->set_normals(val);
	 tex->get_tone_shader()->set_normals(val);
      }
      if(_detail_type == 3){
	 tex->get_tone_shader()->set_normals(val);
      }
      if(_detail_type == 4){
	 tex->get_basecoat_shader()->set_normals(val);
      }
   }
   
   if(_parent)
      _parent->child_callback(this, op);

   _last_op = op;
}


// patch_ui.C
