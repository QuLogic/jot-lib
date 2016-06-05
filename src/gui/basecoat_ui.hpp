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
#ifndef _BASECOAT_UI_H_IS_INCLUDED_
#define _BASECOAT_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// BasecoatUI
////////////////////////////////////////////

#include "disp/view.H"
#include "gui/base_ui.H"

#include <vector>

class ToneShader;
class ImageLineShader;
/*****************************************************************
 * BasecoatUI
 *****************************************************************/
class BasecoatUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum operation_id_t {
      OP_ALL = 0,
      OP_LIGHT_SEPARATE,
      OP_BASECOAT,
      OP_REMAP_A,
      OP_REMAP_B,
      OP_COLOR_OFFSET,
      OP_COLOR_STEEPNESS,
      OP_TEXTURE,
      OP_NUM
   };

   enum button_id_t {
      BUT_NUM 
   };
   enum rotation_id_t {      
      ROT_NUM
   };
   enum listbox_id_t {
      LIST_TEXTURE = 0,
      LIST_NUM
   };
   enum slider_id_t {
      SLIDE_REMAP_A = 0,
      SLIDE_REMAP_B,
      SLIDE_COLOR_OFFSET,
      SLIDE_COLOR_STEEPNESS,
      SLIDE_NUM
   };
   enum spinner_id_t{     
      SPINNER_NUM
   };
   enum panel_id_t {    
      PANEL_MAIN,
      PANEL_NUM
   };
   enum rollout_id_t {
      ROLLOUT_MAIN = 0,
      ROLLOUT_NUM,
   };
   enum radiogroup_id_t {
      RADGROUP_BASECOAT = 0,
      RADGROUP_NUM
   };
   enum radiobutton_id_t {
      RADBUT_BASECOAT_NONE,
      RADBUT_BASECOAT_CONSTANT,
      RADBUT_BASECOAT_TOON,
      RADBUT_BASECOAT_SMOOTHSTEP,
      RADBUT_NUM
   };
   enum checkbox_id_t { 
      CHECK_LIGHT_SEPARATE = 0,
      CHECK_NUM
   }; 
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<BasecoatUI*>  _ui;       
  
 public:
    //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("BasecoatUI", BasecoatUI*, BasecoatUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   BasecoatUI(BaseUI* parent);
   virtual ~BasecoatUI(){}

   /******** MEMBERS METHODS ********/
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();

   void apply_changes_to_texture_parent(int op, ImageLineShader* tex, int layer);
 protected:   
   /******** MEMBERS VARS ********/
   int               _id;
   operation_id_t    _last_op;
   ImageLineShader*     _current_tex;

   vector<string>       _texture_filenames;

 protected:     
   /******** Convenience Methods ********/
   void apply_changes_to_texture(operation_id_t op, ImageLineShader* tex, int layer);
   
 
   /******** STATIC CALLBACK METHODS ********/
   static void  slider_cb(int id);
   static void  radiogroup_cb(int id);
   static void  checkbox_cb(int id);
   static void  spinner_cb(int id);
   static void  listbox_cb(int id);
};


#endif // _BASECOAT_UI_H_IS_INCLUDED_

/* end of file basecoat_ui.H */
