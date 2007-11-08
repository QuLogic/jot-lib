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
// PatchSelectionUI
////////////////////////////////////////////

#include "std/support.H"
#include "glew/glew.H"

#include "geom/winsys.H"
#include "geom/world.H"
#include "glui/glui.h"
#include "std/config.H"

#include "patch_selection_ui.H"

#include <vector>
using namespace mlib;

#define ID_SHIFT                     10
#define ID_MASK                      ((1<<ID_SHIFT)-1)

/*****************************************************************
 * PatchSelectionUI
 *****************************************************************/
vector<PatchSelectionUI*>         PatchSelectionUI::_ui;
const static int MY_WIDTH = 100;
const static int MAX_LAYERS = 4;
const static int MAX_CHANNELS = 4;

PatchSelectionUI::PatchSelectionUI(BaseUI* parent, bool has_layers) :
     BaseUI(parent, "PatchSelectionUI"),
     _has_layers(has_layers),
     _layer_num(0),
     _channel_num(0),
     _whole_mesh(0),
     _all_meshes(0),
     _all_layers(0),
     _last_operation(0)
{
   _ui.push_back(this);
   _id = (_ui.size()-1);  
}

void     
PatchSelectionUI::build(GLUI* glui, GLUI_Panel* base, bool open)
{   
    _glui = glui; 
   int id = _id << ID_SHIFT;
      
   _panel[PANEL_MAIN] = (base) ? glui->add_panel_to_panel(base,"Layer Selection")
                                 : glui->add_panel("Layer Selection");

   
   _listbox[LIST_PATCH] = glui->add_listbox_to_panel(
                              _panel[PANEL_MAIN], 
                              "Patch", NULL,
                              id+LIST_PATCH, listbox_cb);
   
   if(_has_layers){
       _spinner[SPINNER_LAYER] = glui->add_spinner_to_panel(
                              _panel[PANEL_MAIN],
                             "Layer",
                             GLUI_SPINNER_INT,
                             NULL,
                             id+SPINNER_LAYER, spinner_cb);
       _spinner[SPINNER_LAYER]->set_int_limits(0,MAX_LAYERS-1);

       _spinner[SPINNER_CHANNEL] = glui->add_spinner_to_panel(
                              _panel[PANEL_MAIN],
                             "Channel",
                             GLUI_SPINNER_INT,
                             NULL,
                             id+SPINNER_CHANNEL, spinner_cb);
       _spinner[SPINNER_CHANNEL]->set_int_limits(0,MAX_CHANNELS-1);

   }
 
   glui->add_column_to_panel(_panel[PANEL_MAIN],true);
   _checkbox[CHECK_WHOLE_MESH]=glui->add_checkbox_to_panel(
                              _panel[PANEL_MAIN],
                              "Whole Mesh",
                              &_whole_mesh,
                              id+CHECK_WHOLE_MESH,
                              checkbox_cb);

   _checkbox[CHECK_ALL_MESHES]=glui->add_checkbox_to_panel(
                              _panel[PANEL_MAIN],
                              "All Meshes",
                              &_all_meshes,
                              id+CHECK_ALL_MESHES,
                              checkbox_cb);
   _checkbox[CHECK_ALL_MESHES]->disable();


   if(_has_layers){
      _checkbox[CHECK_ALL_LAYERS]=glui->add_checkbox_to_panel(
                              _panel[PANEL_MAIN],
                              "All Layers",
                              &_all_layers,
                              id+CHECK_ALL_LAYERS,
                              checkbox_cb);
   }
   _checkbox[CHECK_LAST_OPERATION]=glui->add_checkbox_to_panel(
                              _panel[PANEL_MAIN],
                              "Last Operation",
                              &_last_operation,
                              id+CHECK_LAST_OPERATION,
                              checkbox_cb);

   _button[BUT_APPLY_CHANGES] = glui->add_button_to_panel(
         _panel[PANEL_MAIN],  "Apply", 
         id+BUT_APPLY_CHANGES, button_cb);

  
   for (int i=0; i<PANEL_NUM; i++){
      if(_panel[i])
      _panel[i]->set_alignment(GLUI_ALIGN_LEFT);
   }
   
}

void
PatchSelectionUI::update_non_lives()
{
   // We should keep our list up to date
   fill_my_patch_listbox();
   if(_checkbox[CHECK_WHOLE_MESH]->get_int_val()==1){
         _checkbox[CHECK_ALL_MESHES]->enable();         
   } else {
         _checkbox[CHECK_ALL_MESHES]->disable();
         _checkbox[CHECK_ALL_MESHES]->set_int_val(0);        
   }
   if(_has_layers){
        _spinner[SPINNER_LAYER]->set_int_val(_layer_num);   
        _spinner[SPINNER_CHANNEL]->set_int_val(_channel_num);   
   }
   //_parent->child_callback(this, SELECT_FILL_PATCHES);   
}

void 
PatchSelectionUI::pick_patch(int i)
{
   Patch* p = _patches[i];
   if(!p){
      err_msg("PatchSelectionUI::pick_patch failed!!!");
      return;
   }   
   _patch = p;
   Patch::set_focus(_patch);
   BMESH::set_focus(_patch->mesh());
}

void
PatchSelectionUI::button_cb(int id)
{
   switch(id&ID_MASK)
   {
       case BUT_APPLY_CHANGES:
         _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT],SELECT_APPLY_CHANGE);  
       break;
   }
}



void  
PatchSelectionUI::listbox_cb(int id)
{
   switch (id&ID_MASK)
   {       
       case LIST_PATCH:
         _ui[id >> ID_SHIFT]->pick_patch(_ui[id >> ID_SHIFT]->_listbox[LIST_PATCH]->get_int_val());
         _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT],SELECT_PATCH_SELECTED);  
         break;
   }
}

void 
PatchSelectionUI::spinner_cb(int id)
{  
   switch (id&ID_MASK)
   {       
       case SPINNER_LAYER:
         _ui[id >> ID_SHIFT]->_layer_num = _ui[id >> ID_SHIFT]->_spinner[SPINNER_LAYER]->get_int_val();
         _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT],SELECT_LAYER_CHENGED);  
         break;
       case SPINNER_CHANNEL:
         _ui[id >> ID_SHIFT]->_channel_num = _ui[id >> ID_SHIFT]->_spinner[SPINNER_CHANNEL]->get_int_val();
         _ui[id >> ID_SHIFT]->_parent->child_callback(_ui[id >> ID_SHIFT],SELECT_CHANNEL_CHENGED);  
         break;        
   }
}
void
PatchSelectionUI::checkbox_cb(int id)
{
   switch(id&ID_MASK)
   {
   case CHECK_ALL_MESHES:
      break;
   case CHECK_WHOLE_MESH:
      _ui[id >> ID_SHIFT]->update_non_lives();
      break;
   }
   
  
}

// Fill out values with list all the patches in the scene
inline void 
fill_all_patches_listbox(str_list& list, int& index, Patch*& my_p, Patch_list& my_patches)
{   
   my_patches.clear();
   BMESH_list meshes = BMESH_list(DRAWN);  
   for(int i=0; i < meshes.num(); ++i){
         Patch_list patch = meshes[i]->patches();
         for(int j=0; j < patch.num(); ++j){
            my_patches.add(patch[j]);
            list.add(str_ptr(" m_")+str_ptr(i)+str_ptr("_p_")+str_ptr(j));
            if(Patch::is_focus(patch[j])){
               index=my_patches.num()-1;              
               my_p=patch[j];
            }
         }
   } 
}

void 
PatchSelectionUI::fill_my_patch_listbox()
   {
      str_list list;
      int index;      
      fill_all_patches_listbox(list, index, _patch, _patches);
      fill_listbox(_listbox[LIST_PATCH], list);      
      _listbox[LIST_PATCH]->set_int_val(index);

   }

// patch_selection_ui.C
