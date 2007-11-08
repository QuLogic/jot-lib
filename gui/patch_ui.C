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
// PatchUI
////////////////////////////////////////////

#include "mesh/bmesh.H"
#include "mesh/patch.H"
#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "std/config.H"

#include "patch_selection_ui.H"
#include "patch_ui.H"
#include "ref_image_ui.H"

#include "detail_ctrl_ui.H" 

using namespace mlib;

#define ID_SHIFT  10
#define ID_MASK   ((1<<ID_SHIFT)-1)

/*****************************************************************
 * PatchUI
 *****************************************************************/

vector<PatchUI*> PatchUI::_ui;

const static int WIN_WIDTH=300; 

PatchUI::PatchUI(BaseUI* parent) :
     BaseUI(parent, "Patch UI")
{
   _ui.push_back(this);
   _id = (_ui.size()-1);  

   _patch_selection_ui = new PatchSelectionUI(this, false);
   _ref_image_ui = new RefImageUI(this);
   //_detail_ctrl_ui = new DetailCtrlUI(this); 
}


void     
PatchUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
   int id = _id << ID_SHIFT;

  
   _rollout[ROLLOUT_PATCH] = (base) ? glui->add_rollout_to_panel(base, "Patch UI",open)
                                    : glui->add_rollout("Patch UI",open);
  
    _ref_image_ui->build(glui, _rollout[ROLLOUT_PATCH],false);
   

   _patch_selection_ui->build(glui, _rollout[ROLLOUT_PATCH], true);
  

   _checkbox[CHECK_SHOW_BLEND]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Show Blend Patches",
                              NULL,
                              id+CHECK_SHOW_BLEND,
                              checkbox_cb);
   _spinner[SPINNER_N_RING] = glui->add_spinner_to_panel(
                             _rollout[ROLLOUT_PATCH],
                             "N-Rings",
                             GLUI_SPINNER_INT,
                             NULL,
                             id+SPINNER_N_RING, spinner_cb);
   _spinner[SPINNER_N_RING]->set_w(20);
   _spinner[SPINNER_N_RING]->set_int_limits(0,2);

    _slider[SLIDE_REMAP_POWER] = _glui->add_slider_to_panel(
      _rollout[ROLLOUT_PATCH], 
      "Remap Power", 
      GLUI_SLIDER_FLOAT, 
      1.0, 6.0,
      NULL,
      id+SLIDE_REMAP_POWER, slider_cb);
   _slider[SLIDE_REMAP_POWER]->set_num_graduations(100);
   _slider[SLIDE_REMAP_POWER]->set_w(200);

  


   glui->add_column_to_panel(_rollout[ROLLOUT_PATCH],true);

    _listbox[LIST_TEX_SEL] = glui->add_listbox_to_panel(
                              _rollout[ROLLOUT_PATCH], 
                              "Texture", NULL,
                              id+LIST_TEX_SEL, listbox_cb);
    
    
    _checkbox[CHECK_HALO] = glui->add_checkbox_to_panel(_rollout[ROLLOUT_PATCH],
                                                         "Draw Halo",
                                                         0,
                                                         id+CHECK_HALO,
                                                         checkbox_cb);

   _checkbox[CHECK_TRACKING]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Tracking",
                              NULL,
                              id+CHECK_TRACKING,
                              checkbox_cb);   
   _checkbox[CHECK_LOD]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "LOD",
                              NULL,
                              CHECK_LOD,
                              checkbox_cb);
   _checkbox[CHECK_ROTATION]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Rotate",
                              NULL,
                              id+CHECK_ROTATION,
                              checkbox_cb);

   _checkbox[CHECK_TIMED_LOD]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Timed LOD Transitions",
                              NULL,
                              id+CHECK_TIMED_LOD,
                              checkbox_cb);
   _checkbox[CHECK_USE_DIRECTION]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Use Direction Vector",
                              NULL,
                              id+CHECK_USE_DIRECTION,
                              checkbox_cb);
   _checkbox[CHECK_USE_WEIGHTED_LS]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Use Weighted LS",
                              NULL,
                              id+CHECK_USE_WEIGHTED_LS,
                              checkbox_cb);
   _checkbox[CHECK_USE_VIS_TEST]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Use Visibility Test",
                              NULL,
                              id+CHECK_USE_VIS_TEST,
                              checkbox_cb);
   
   
   _panel[PANEL_SPS] = glui->add_panel_to_panel(
                               _rollout[ROLLOUT_PATCH],
                               "Stratified Point Sampling");
   _spinner[SPINNER_SPS_HIGHT] = glui->add_spinner_to_panel(
                             _panel[PANEL_SPS],
                             "Hight",
                             GLUI_SPINNER_INT,
                             NULL,
                             id+SPINNER_SPS_HIGHT, spinner_cb);
   _spinner[SPINNER_SPS_HIGHT]->set_w(20);
   _spinner[SPINNER_SPS_HIGHT]->set_int_limits(0,10);

   _spinner[SPINNER_SPS_MIN_DIST] = glui->add_spinner_to_panel(
                             _panel[PANEL_SPS],
                             "Min Dist",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             id+SPINNER_SPS_MIN_DIST, spinner_cb);
   _spinner[SPINNER_SPS_MIN_DIST]->set_w(20);
   _spinner[SPINNER_SPS_MIN_DIST]->set_float_limits(0.0,1.0);

   _spinner[SPINNER_SPS_REG] = glui->add_spinner_to_panel(
                             _panel[PANEL_SPS],
                             "Regularity",
                             GLUI_SPINNER_FLOAT,
                             NULL,
                             id+SPINNER_SPS_REG, spinner_cb);
   _spinner[SPINNER_SPS_REG]->set_w(20);
   _spinner[SPINNER_SPS_REG]->set_float_limits(0.0,50.0);

   _button[BUT_SPS_APPLY] = glui->add_button_to_panel(
         _panel[PANEL_SPS],  "Re-genarate", 
         id+BUT_SPS_APPLY, button_cb);

   _checkbox[CHECK_OCCLUDER]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Occluder",
                              NULL,
                              id+CHECK_OCCLUDER,
                              checkbox_cb);

   _checkbox[CHECK_RECIEVER]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_PATCH],
                              "Reciever",
                              NULL,
                              id+CHECK_RECIEVER,
                              checkbox_cb);

   _slider[SLIDE_SHADOW_SCALE]=glui->add_slider_to_panel(_rollout[ROLLOUT_PATCH],
                                                          "Shadow Scale",GLUI_SLIDER_FLOAT,0.0,1.5,0,
                                                          id+SLIDE_SHADOW_SCALE,slider_cb);


   _slider[SLIDE_SHADOW_SOFT]=glui->add_slider_to_panel(_rollout[ROLLOUT_PATCH],
                                                          "Shadow Softness",GLUI_SLIDER_FLOAT,0.01,1.0,0,
                                                          id+SLIDE_SHADOW_SOFT,slider_cb);

   _slider[SLIDE_SHADOW_OFFSET]=glui->add_slider_to_panel(_rollout[ROLLOUT_PATCH],
                                                          "Shadow Y Offset",GLUI_SLIDER_FLOAT,-1.0,1.0,0,
                                                          id+SLIDE_SHADOW_OFFSET,slider_cb);



   //_detail_ctrl_ui->build(glui, _rollout[ROLLOUT_PATCH], false);

   // Cleanup sizes
   int w = _rollout[ROLLOUT_PATCH]->get_w();
   w = max(WIN_WIDTH, w);
   
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
PatchUI::update_non_lives()
{   
   _patch_selection_ui->update_non_lives();
   //_detail_ctrl_ui->update_non_lives(); 
   _ref_image_ui->update_non_lives();
   _patch = Patch::focus();
   fill_texture_listbox();
   update_dynamic_params();   
}

bool
PatchUI::child_callback(BaseUI* sender, int event)
{
   bool s = false;  
   if(sender->class_name() == PatchSelectionUI::static_name()){
      switch(event)
      {
      case PatchSelectionUI::SELECT_FILL_PATCHES:
         _patch_selection_ui->fill_my_patch_listbox();
         break;
      case PatchSelectionUI::SELECT_PATCH_SELECTED:  
         update();
         break;
      case PatchSelectionUI::SELECT_APPLY_CHANGE:
         apply_changes();
         break;
      } 
   }    
   return s;
}

void 
PatchUI::apply_changes()
{
   cerr << "PatchUI::apply_changes" << endl;
   // Figure out which patches to apply changes to and call
   // apply_changes_to_patch with them  
   bool whole_mesh     = (_patch_selection_ui->get_whole_mesh());
   bool all_meshes     = (_patch_selection_ui->get_all_meshes()); 
   bool last_operation = (_patch_selection_ui->get_last_operation());

   operation_id_t op = (last_operation) ? _last_op : OP_ALL;

   if(!whole_mesh && !all_meshes){
      apply_changes_to_patch(op, _patch);
      return;
   }
   
   if(all_meshes && whole_mesh)
   {
      BMESH_list meshes = BMESH_list(DRAWN);  
      for(int i=0; i < meshes.num(); ++i){
         Patch_list patch = meshes[i]->patches();
         for(int j=0; j < patch.num(); ++j){
            apply_changes_to_patch(op, patch[j]);
         }
      }  
   } else if (whole_mesh){
      Patch_list patch = _patch->mesh()->patches();
      for(int j=0; j < patch.num(); ++j){
         apply_changes_to_patch(op, patch[j]);
      }
   }
}

void
PatchUI::apply_changes_to_patch(operation_id_t op, Patch* p)
{
   if(!p)
      return;

   if((op==OP_ALL) || (op==OP_TRACKING)){
      int val = _checkbox[CHECK_TRACKING]->get_int_val();
      p->do_dynamic_stuff((val==1));   
   }
   if((op==OP_ALL) || (op==OP_LOD)){
      int val = _checkbox[CHECK_LOD]->get_int_val();
      p->set_do_lod((val==1));   
   }
   if((op==OP_ALL) || (op==OP_ROTATION)){
      int val = _checkbox[CHECK_ROTATION]->get_int_val();
      p->set_do_rotate((val==1));   
   }
   if((op==OP_ALL) || (op==OP_TIMED_LOD)){
      int val = _checkbox[CHECK_TIMED_LOD]->get_int_val();
      p->set_use_timed_lod_transitions((val==1));
   }
   if((op==OP_ALL) || (op==OP_TEX_CHANGED)){
      char* s = _listbox[LIST_TEX_SEL]->curr_text.string;
      str_ptr name(s);
      if(name != "Other..."){
         p->set_texture(str_ptr(s));
         GTexture* cur_texture = p->cur_tex();
         if(cur_texture)
            cur_texture->set_patch(p);   
         //activate_this_texture(cur_texture);
         VIEW::peek()->set_rendering(str_ptr("Textured patch"));     
      }
   }
   if((op==OP_ALL) || (op==OP_USE_DIRECTION)){
       int val = _checkbox[CHECK_USE_DIRECTION]->get_int_val();
       p->set_use_direction_vec((val==1));
   }
   if((op==OP_ALL) || (op==OP_USE_WEIGHTED_LS)){
       int val = _checkbox[CHECK_USE_WEIGHTED_LS]->get_int_val();
       p->set_use_weighted_ls((val==1));
   }
   if((op==OP_ALL) || (op==OP_USE_VIS_TEST)){
       int val = _checkbox[CHECK_USE_VIS_TEST]->get_int_val();
       p->set_use_visibility_test((val==1));
   }
   if((op==OP_ALL) || (op==OP_SHOW_BLEND)){
       int val = _checkbox[CHECK_SHOW_BLEND]->get_int_val();
       p->mesh()->set_do_patch_blend(val==1);
       p->mesh()->changed();     
   }
   if((op==OP_ALL) || (op==OP_NRING)){
      int val = _spinner[SPINNER_N_RING]->get_int_val();
      p->mesh()->set_patch_blend_smooth_passes(val);
      p->mesh()->changed();         
   }
   if((op==OP_ALL) || (op==OP_REMAP_POWER)){
      float val = _slider[SLIDE_REMAP_POWER]->get_float_val();
      p->mesh()->set_blend_remap_value(val);

   }      
   if((op==OP_ALL) || (op==OP_SPS_APPLY)){
      p->set_sps_height(_spinner[SPINNER_SPS_HIGHT]->get_int_val());
      p->set_sps_min_dist(_spinner[SPINNER_SPS_MIN_DIST]->get_float_val());
      p->set_sps_regularity(_spinner[SPINNER_SPS_REG]->get_float_val());
      p->create_dynamic_samples();
   }
   if((op==OP_ALL) || (op==OP_OCCLUDER)){
      bool val = (_checkbox[CHECK_OCCLUDER]->get_int_val()==1);
      p->mesh()->set_occluder(val==1);
   }    
   if((op==OP_ALL) || (op==OP_RECIEVER)){
      bool val = (_checkbox[CHECK_RECIEVER]->get_int_val()==1);
      p->mesh()->set_reciever(val==1);
   }
   if((op==OP_ALL) || (op==OP_SSCALE)){
      _patch->mesh()->set_shadow_scale(_slider[SLIDE_SHADOW_SCALE]->get_float_val());
   }

   if((op==OP_ALL) || (op==OP_SSOFTNESS)){
     _patch->mesh()->set_shadow_softness( _slider[SLIDE_SHADOW_SOFT]->get_float_val() * _slider[SLIDE_SHADOW_SOFT]->get_float_val());
   }
   
  if((op==OP_ALL) || (op==OP_SOFFSET)){
     _patch->mesh()->set_shadow_offset( _slider[SLIDE_SHADOW_OFFSET]->get_float_val());
   }
  
  if((op==OP_ALL) || (op==OP_HALO)){
     _patch->mesh()->geom()->set_can_do_halo( _checkbox[CHECK_HALO]->get_int_val()==1);
   }

   _last_op = op;
}

void
PatchUI::update_dynamic_params()
{
   if (!_patch){
      // this may happen when loading from file
      // not really an error i think? so no cerr.
//      cerr << "PatchUI::update_dynamic_params() no patch" << endl;
      return;
   }
   char* name = get_current_tex_name();
   if(name){
      int val = (_listbox[LIST_TEX_SEL]->get_item_ptr(name)) 
         ? _listbox[LIST_TEX_SEL]->get_item_ptr(name)->id
         : _listbox[LIST_TEX_SEL]->get_item_ptr("Other...")->id;

      _listbox[LIST_TEX_SEL]->set_int_val(val);
   }
   _checkbox[CHECK_TRACKING]->set_int_val((int)_patch->get_do_dynamic_stuff());
   _checkbox[CHECK_LOD]->set_int_val((int)_patch->get_do_lod());
   _checkbox[CHECK_ROTATION]->set_int_val((int)_patch->get_do_rotate());
   _checkbox[CHECK_TIMED_LOD]->set_int_val((int)_patch->get_use_timed_lod_transitions());
   _checkbox[CHECK_USE_DIRECTION]->set_int_val((int)_patch->get_use_direction_vec());
  
   _checkbox[CHECK_SHOW_BLEND]->set_int_val((int)_patch->mesh()->get_do_patch_blend());
   _slider[SLIDE_REMAP_POWER]->set_float_val(_patch->mesh()->get_blend_remap_value());
   _spinner[SPINNER_N_RING]->set_int_val(_patch->mesh()->patch_blend_smooth_passes());

   _spinner[SPINNER_SPS_HIGHT]->set_int_val(_patch->get_sps_height());
   _spinner[SPINNER_SPS_MIN_DIST]->set_float_val((float)_patch->get_sps_min_dist());
   _spinner[SPINNER_SPS_REG]->set_float_val((float)_patch->get_sps_regularity());
   _checkbox[CHECK_USE_WEIGHTED_LS]->set_int_val(_patch->get_use_weighted_ls());
   _checkbox[CHECK_USE_VIS_TEST]->set_int_val(_patch->get_use_visibility_test());

   _checkbox[CHECK_OCCLUDER]->set_int_val(_patch->mesh()->occluder() ? 1 : 0);
   _checkbox[CHECK_RECIEVER]->set_int_val(_patch->mesh()->reciever() ? 1 : 0);

   _slider[SLIDE_SHADOW_SCALE]->set_float_val(_patch->mesh()->shadow_scale());
   _slider[SLIDE_SHADOW_SOFT]->set_float_val(sqrt(_patch->mesh()->shadow_softness()));
   _slider[SLIDE_SHADOW_OFFSET]->set_float_val(_patch->mesh()->shadow_offset());
   _checkbox[CHECK_HALO]->set_int_val(_patch->mesh()->geom()->can_do_halo() ? 1 : 0);
 
}


void
PatchUI::slider_cb(int id)
{   
   switch(id&ID_MASK)
   {  
      case SLIDE_REMAP_POWER:
         _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_REMAP_POWER, _ui[id >> ID_SHIFT]->_patch);
         break;
      case SLIDE_SHADOW_SCALE:
         _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_SSCALE, _ui[id >> ID_SHIFT]->_patch);
         break;
      case SLIDE_SHADOW_SOFT:
         _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_SSOFTNESS, _ui[id >> ID_SHIFT]->_patch);
         break;
      case SLIDE_SHADOW_OFFSET:
         _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_SOFFSET, _ui[id >> ID_SHIFT]->_patch);
         break;
   }
}

void
PatchUI::listbox_cb(int id)
{   
   switch(id&ID_MASK)
   {  
      case LIST_TEX_SEL:
         _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_TEX_CHANGED, _ui[id >> ID_SHIFT]->_patch);
         if(_ui[id >> ID_SHIFT]->_parent)
            _ui[id >> ID_SHIFT]->_parent->update();
         else
            _ui[id >> ID_SHIFT]->update();
         break;
   }
}

void
PatchUI::button_cb(int id)
{
  switch(id&ID_MASK)
   {  
      case BUT_SPS_APPLY:
         _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_SPS_APPLY, _ui[id >> ID_SHIFT]->_patch);
         break;
   }
}

void  
PatchUI::spinner_cb(int id)
{
   switch(id&ID_MASK)
   {  
   case SPINNER_N_RING:
      _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_NRING, _ui[id >> ID_SHIFT]->_patch);
      break;
   }
   

}


void
PatchUI::checkbox_cb(int id)
{
   switch(id&ID_MASK)
   {
  
   case CHECK_TRACKING:
      _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_TRACKING, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_LOD:
      _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_LOD, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_ROTATION:
      _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_ROTATION, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_TIMED_LOD:
      _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_TIMED_LOD, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_USE_DIRECTION:
       _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_USE_DIRECTION, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_SHOW_BLEND:
       _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_SHOW_BLEND, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_USE_WEIGHTED_LS:
      _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_USE_WEIGHTED_LS, _ui[id >> ID_SHIFT]->_patch);
   case CHECK_USE_VIS_TEST:
      _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_USE_VIS_TEST, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_OCCLUDER:
       _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_OCCLUDER, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_RECIEVER:
       _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_RECIEVER, _ui[id >> ID_SHIFT]->_patch);
      break;
   case CHECK_HALO:
       _ui[id >> ID_SHIFT]->apply_changes_to_patch(OP_HALO, _ui[id >> ID_SHIFT]->_patch);
      break;
   }
   
  
}

void
PatchUI::fill_texture_listbox()
{
    str_list rend_modes; 
    
    rend_modes.add("Smooth Shading");
    rend_modes.add("HatchingTX");
    rend_modes.add("Halftone_TX");
    rend_modes.add("Painterly");
    rend_modes.add("ProxyTexture");
    rend_modes.add("Other...");
  
    fill_listbox(_listbox[LIST_TEX_SEL], rend_modes);      
}

char* 
PatchUI::get_current_tex_name()
{
   if(!_patch)
      return 0;
   GTexture* tex = _patch->cur_tex();
   if(!tex)
      return "Other...";
  
   char* r;
   
   if(tex->is_of_type("Smooth Shading"))
      r = "Smooth Shading";
   else if(tex->is_of_type("HatchingTX"))
      r = "HatchingTX";
   else if(tex->is_of_type("Halftone_TX"))
      r = "Halftone_TX";
   else if(tex->is_of_type("Painterly"))
      r = "Painterly";
   else if(tex->is_of_type("ProxyTexture"))
      r = "ProxyTexture";
   else
      r = "Other...";
   return r;
}

// patch_ui.C
