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
#ifndef _PATCH_SELECTION_UI_H_IS_INCLUDED_
#define _PATCH_SELECTION_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// PatchSelectionUI
////////////////////////////////////////////

#include "glui/glui.h"
#include "gui/base_ui.H"
#include "gui/gui.H"

#include <vector>

/*****************************************************************
 * PatchSelectionUI
 *****************************************************************/
class PatchSelectionUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum selection_stages_t{
      SELECT_FILL_PATCHES=0,
      SELECT_PATCH_SELECTED,
      SELECT_LAYER_CHENGED,
      SELECT_APPLY_CHANGE,
      SELECT_CHANNEL_CHENGED
   };
    
   enum button_id_t {
      BUT_APPLY_CHANGES = 0,   
      BUT_NUM 
   };   
  
   enum listbox_id_t {
      LIST_PATCH = 0,
      RADBUT_NUM
   };
   enum spinner_id_t{
      SPINNER_LAYER=0,
      SPINNER_CHANNEL,
      SPINNER_NUM
   };
   enum edittext_id_t{
      EDITTEXT_NUM
   };
   enum panel_id_t {
      PANEL_MAIN = 0, 
      PANEL_NUM
   };
  
   enum checkbox_id_t { 
      CHECK_WHOLE_MESH = 0,
      CHECK_ALL_MESHES,
      CHECK_ALL_LAYERS,
      CHECK_LAST_OPERATION,    
      CHECK_NUM
   }; 
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<PatchSelectionUI*>  _ui;       
  
 public:
   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("PatchSelectionUI", PatchSelectionUI*, PatchSelectionUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   PatchSelectionUI(BaseUI* parent, bool has_layers);
   virtual ~PatchSelectionUI(){}

   /******** MEMBERS METHODS ********/  
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();
   
   // For the textures only...(if there is filter, i.e. patch, this is not called...)
   template <class T> void fill_my_texture_listbox()
   {
      str_list list;
      int index;
      GUI::get_all_my_textures<T>(list, index, _patch, _patches);
      fill_listbox(_listbox[LIST_PATCH], list);   
      _listbox[LIST_PATCH]->set_int_val(index);
   }
   // For the patch
   void fill_my_patch_listbox();  

   Patch* get_current_patch()     const { return _patch;   }
   CPatch_list& get_all_patches() const { return _patches; }
   int    get_layer_num()         const { return _layer_num; }
   void   set_layer_num(int v)          { _layer_num=v; }
   int    get_channel_num()       const { return _channel_num; }
   void   set_channel_num(int i)        { _channel_num = i; update(); }
   bool   get_whole_mesh()        const { return (_whole_mesh==1); }
   bool   get_all_meshes()        const { return (_all_meshes==1); }
   bool   get_all_layer()         const { return (_all_layers==1); }
   bool   get_last_operation()    const { return (_last_operation==1); }
 protected:
   
   /******** MEMBERS VARS ********/
   int                 _id;
   bool                _has_layers;
   Patch*              _patch;
   Patch_list          _patches;
   
   int                 _layer_num;
   int                 _channel_num;
   int                 _whole_mesh;
   int                 _all_meshes;
   int                 _all_layers;
   int                 _last_operation;
  
 protected:     
   /******** Convenience Methods ********/
    void pick_patch(int i);

   /******** STATIC CALLBACK METHODS ********/
   static void  button_cb(int id);     
   static void  listbox_cb(int id);
   static void  checkbox_cb(int id);
   static void  spinner_cb(int id);


};


#endif // _PATCH_SELECTION_UI_H_IS_INCLUDED_

/* end of file patch_selection_ui.H */
