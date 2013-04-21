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
#ifndef _PROXY_TEXTURE_UI_H_IS_INCLUDED_
#define _PROXY_TEXTURE_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// HacthingUI
////////////////////////////////////////////

#include "disp/view.H"
#include "gui/base_ui.H"

#include <vector>

class ColorUI;
class PatchSelectionUI;
class ProxyTexture;

/*****************************************************************
 * ProxyTextureUI
 *****************************************************************/
class ProxyTextureUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum operation_id_t {
      OP_ALL = 0,
      OP_CHANGE_BASECOAT,
      OP_SHOW_SAMPLES,
      OP_SHOW_SAMPLES_OLD,
      OP_NUM
   };

   enum button_id_t {      
      BUT_NUM 
   };
   enum rotation_id_t {      
      ROT_NUM
   };
   enum listbox_id_t {
      LIST_BASECOAT = 0,
      LIST_NUM
   };
   enum slider_id_t {
      SLIDE_NUM
   };
   enum spinner_id_t{     
      SPINNER_NUM
   };
   enum panel_id_t {
      PANEL_MAIN = 0, 
      PANEL_NUM
   };
   enum rollout_id_t {
      ROLLOUT_MAIN = 0,
      ROLLOUT_SAMPLES,
      ROLLOUT_NUM,
   };
   enum radiogroup_id_t {      
      RADGROUP_NUM
   };
   enum radiobutton_id_t {      
      RADBUT_NUM
   };
   enum checkbox_id_t {
      CHECK_SHOW_SAMPLES = 0,
      CHECK_SHOW_PROXY_MESH,
      CHECK_SHOW_SAMPLES_OLD,
      CHECK_NUM
   }; 
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<ProxyTextureUI*>  _ui;       
  
 public:
    //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ProxyTextureUI", ProxyTextureUI*, ProxyTextureUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   ProxyTextureUI(BaseUI* parent=0);
   virtual ~ProxyTextureUI(){}

   /******** MEMBERS METHODS ********/
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();
   virtual bool child_callback(BaseUI* sender, int event);
 protected:   
   /******** MEMBERS VARS ********/
   int                  _id;
   
   ColorUI*             _color_ui;
   PatchSelectionUI*    _texture_selection_ui;
   operation_id_t       _last_op;
   ProxyTexture*        _current_tex;


 protected:     
   /******** Convenience Methods ********/
   void        toggle_enable_me(); 
   void        apply_changes();
   void        apply_changes_to_texture(operation_id_t op, ProxyTexture* tex);

   void        fill_basecoat_listbox(GLUI_Listbox *lb);     
   /******** STATIC CALLBACK METHODS ********/
   static void  button_cb(int id);
   static void  listbox_cb(int id);
   static void  slider_cb(int id);
   static void  spinner_cb(int id);
   static void  radiogroup_cb(int id);
   static void  rotation_cb(int id);
   static void  checkbox_cb(int id);
};


#endif // _HATCHING_UI_H_IS_INCLUDED_

/* end of file hatching_ui.H */
