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
// ProxyTextureUI
////////////////////////////////////////////

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "std/config.H"

#include "proxy_pattern/proxy_texture.H"
#include "proxy_texture_ui.H"
#include "color_ui.H"
#include "patch_selection_ui.H"

#include "gui.H"

using namespace mlib;

#define ID_SHIFT  10
#define ID_MASK   ((1<<ID_SHIFT)-1)


/*****************************************************************
 * ProxyTextureUI
 *****************************************************************/

vector<ProxyTextureUI*> ProxyTextureUI::_ui;

const static int WIN_WIDTH=300; 
const static int MAX_LAYERS = 4;

ProxyTextureUI::ProxyTextureUI(BaseUI* parent) :
     BaseUI(parent,"Haching UI")
{
   _ui.push_back(this);
   _id = (_ui.size()-1); 
   _color_ui = new ColorUI(this);

   _texture_selection_ui = new PatchSelectionUI(this, false);
   _current_tex = 0;  
}


void     
ProxyTextureUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
   _glui = glui; 
//   int id = _id << ID_SHIFT;

   _rollout[ROLLOUT_MAIN] = (base) ? glui->add_rollout_to_panel(base, "Proxy Pattern",open)
                                    : glui->add_rollout("Proxy Pattern UI",open);

   _texture_selection_ui->build(glui, _rollout[ROLLOUT_MAIN], true);
   

  
   
   glui->add_column_to_panel(_rollout[ROLLOUT_MAIN],true);
  
   _listbox[LIST_BASECOAT]= glui->add_listbox_to_panel(
                              _rollout[ROLLOUT_MAIN], 
                              "Basecoat", NULL,
                              LIST_BASECOAT, listbox_cb);                             
   fill_basecoat_listbox(_listbox[LIST_BASECOAT]);   

   _rollout[ROLLOUT_SAMPLES] =  glui->add_rollout_to_panel(_rollout[ROLLOUT_MAIN], "Misc", false);
    

   _checkbox[CHECK_SHOW_SAMPLES]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_SAMPLES],
                              "Show Samples",
                              NULL,
                              CHECK_SHOW_SAMPLES,
                              checkbox_cb);    

    _checkbox[CHECK_SHOW_SAMPLES_OLD]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_SAMPLES],
                              "Show Samples Old",
                              NULL,
                              CHECK_SHOW_SAMPLES_OLD,
                              checkbox_cb);    

   _checkbox[CHECK_SHOW_PROXY_MESH]=glui->add_checkbox_to_panel(
                              _rollout[ROLLOUT_SAMPLES],
                              "Show Proxy Mesh",
                              NULL,
                              CHECK_SHOW_PROXY_MESH,
                              checkbox_cb);
   _checkbox[CHECK_SHOW_PROXY_MESH]->disable();

  
   
   // Cleanup sizes

   for (int i=0; i<ROLLOUT_NUM; i++){
      if(_rollout[i])
      _rollout[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   for (int i=0; i<PANEL_NUM; i++){
      if(_panel[i])
      _panel[i]->set_alignment(GLUI_ALIGN_LEFT);
   }

   int w = _rollout[ROLLOUT_MAIN]->get_w();
   w = max(WIN_WIDTH, w);

   update_non_lives();
}

void
ProxyTextureUI::update_non_lives()
{  
  
   if(GUI::get_current_texture<ProxyTexture>()){
      _listbox[LIST_BASECOAT]->set_int_val(GUI::get_current_texture<ProxyTexture>()->get_base());
      _checkbox[CHECK_SHOW_SAMPLES]->set_int_val(GUI::get_current_texture<ProxyTexture>()->get_draw_samples());
      //int draw_proxy_mesh =  (int)_pen->proxy_texture()->hatching_tex()->get_draw_proxy_mesh();	   
      //_checkbox[CHECK_SHOW_PROXY_MESH]->set_int_val(draw_proxy_mesh);     
   }
   toggle_enable_me();
   _texture_selection_ui->update_non_lives();
   _texture_selection_ui->fill_my_texture_listbox<ProxyTexture>();
   _current_tex = GUI::get_current_texture<ProxyTexture>();
   _color_ui->update_non_lives();

   
}

void
ProxyTextureUI::toggle_enable_me()
{
   if(GUI::exists_in_the_world<ProxyTexture>()){
      _rollout[ROLLOUT_MAIN]->enable();
      if(GUI::get_current_texture<ProxyTexture>()){
         _rollout[ROLLOUT_MAIN]->open();
      }else {        
         _rollout[ROLLOUT_MAIN]->close();
      }
   } else {
      _rollout[ROLLOUT_MAIN]->close();
      _rollout[ROLLOUT_MAIN]->disable();        
   }  
}


void
ProxyTextureUI::listbox_cb(int id)
{
   switch(id&ID_MASK)
   {    
     case LIST_BASECOAT:	   
   	 _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_CHANGE_BASECOAT, _ui[id >> ID_SHIFT]->_current_tex);
       break;
   }
}

void
ProxyTextureUI::button_cb(int id)
{
   
}

void
ProxyTextureUI::slider_cb(int id)
{
   
}

void 
ProxyTextureUI::spinner_cb(int id)
{  
   
}

void
ProxyTextureUI::radiogroup_cb(int id)
{   
   
}

void
ProxyTextureUI::checkbox_cb(int id)
{
  switch(id&ID_MASK)
   {     
     case CHECK_SHOW_SAMPLES:       
       _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_SHOW_SAMPLES, _ui[id >> ID_SHIFT]->_current_tex);
	    break;    
     case CHECK_SHOW_PROXY_MESH:{
       // int val = _ui[id >> ID_SHIFT]->_checkbox[CHECK_SHOW_PROXY_MESH]->get_int_val();
       //GUI::get_current_texture<ProxyTexture>()->hatching_tex()->set_draw_proxy_mesh((val==1));	   
       break;                   }            
     case CHECK_SHOW_SAMPLES_OLD:
       _ui[id >> ID_SHIFT]->apply_changes_to_texture(OP_SHOW_SAMPLES_OLD, _ui[id >> ID_SHIFT]->_current_tex);
	    break;    	
   }
}

void
ProxyTextureUI::rotation_cb(int id)
{    
  
}

bool
ProxyTextureUI::child_callback(BaseUI* sender, int event)
{
   bool s = false;  
   if(sender->class_name() == PatchSelectionUI::static_name()){
      switch(event)
      {
      case PatchSelectionUI::SELECT_FILL_PATCHES:
         _texture_selection_ui->fill_my_texture_listbox<ProxyTexture>();
         break;
      case PatchSelectionUI::SELECT_PATCH_SELECTED:
         if(_parent)
            _parent->update();
         else
            update();
         break;
      case PatchSelectionUI::SELECT_LAYER_CHENGED:
         break;
      case PatchSelectionUI::SELECT_APPLY_CHANGE:
         apply_changes();
         break;
      } 
   }    
   return s;
}

void 
ProxyTextureUI::apply_changes()
{
   // Figure out which patches to apply changes to and call
   // apply_changes_to_texture with them
   bool whole_mesh     = (_texture_selection_ui->get_whole_mesh());
   bool all_meshes     = (_texture_selection_ui->get_all_meshes());  
   bool last_operation = (_texture_selection_ui->get_last_operation());
   

   Patch_list patches = _texture_selection_ui->get_all_patches();
//   Patch*     patch   = _texture_selection_ui->get_current_patch();  //XXX why not Patch::focus(); ?
   _current_tex = GUI::get_current_texture<ProxyTexture>();          //XXX don't really need to do this...done in update_non_live..

   operation_id_t my_op = (last_operation) ? _last_op : OP_ALL;

   if(!whole_mesh && !all_meshes){
      apply_changes_to_texture(my_op, _current_tex);
      return;
   }
      
   
      if(all_meshes && whole_mesh){
            for(int j=0; j < patches.num(); ++j){
               apply_changes_to_texture(my_op, 
                                        GUI::get_current_texture<ProxyTexture>(patches[j]));
            }
      }else if (whole_mesh){
            // Apply to patches only within the selected model
            for(int j=0; j < patches.num(); ++j){
               if(BMESH::is_focus(patches[j]->mesh()))
                  apply_changes_to_texture(my_op, 
                                       GUI::get_current_texture<ProxyTexture>(patches[j]));
            }

      }
   
   
}

void
ProxyTextureUI::apply_changes_to_texture(operation_id_t op, ProxyTexture* tex)
{ 
   cerr << "ProxyTextureUI::apply_changes_to_texture " << op << endl;
   if((op==OP_ALL) || (op==OP_CHANGE_BASECOAT)){
      int val = _listbox[LIST_BASECOAT]->get_int_val();
      tex->set_base(val); //this pointer seems to be null 
      update_non_lives();
   }
   if((op==OP_ALL) || (op==OP_SHOW_SAMPLES)){
      int val = _checkbox[CHECK_SHOW_SAMPLES]->get_int_val();
      tex->set_draw_samples((val==1));
   }
   if((op==OP_ALL) || (op==OP_SHOW_SAMPLES_OLD)){
       int val = _checkbox[CHECK_SHOW_SAMPLES_OLD]->get_int_val();
      
       if(val==1)
          tex->make_old_samples();
       else
          tex->clear_old_sampels();
   }
   
   _last_op = op;
}


//******** Convenience Methods ********

void        
ProxyTextureUI::fill_basecoat_listbox(GLUI_Listbox *lb)
{
  
   const vector<string>& names = ProxyTexture::base_names();
   for(uint i=0; i < names.size(); ++i)
   {
      lb->add_item(i, (char*)((names[i]).c_str()));  
   }
   
}

// patch_ui.C
