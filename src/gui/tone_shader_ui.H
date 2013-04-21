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
#ifndef _TONE_SHADER_UI_H_IS_INCLUDED_
#define _TONE_SHADER_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// ToneShaderUI
////////////////////////////////////////////

#include "disp/view.H"
#include "gui/base_ui.H"

#include <vector>

class ToneShader;
/*****************************************************************
 * ToneShaderUI
 *****************************************************************/
class ToneShaderUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum operation_id_t {
      OP_ALL = 0,
      OP_ENABLE,
      OP_REMAP_NL,
      OP_REMAP,
      OP_BACKLIGHT,
      OP_REMAP_A,
      OP_REMAP_B,
      OP_BACKLIGHT_A,
      OP_BACKLIGHT_B,
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
      SLIDE_BACKLIGHT_A,
      SLIDE_BACKLIGHT_B,
      SLIDE_NUM
   };
   enum spinner_id_t{     
      SPINNER_NUM
   };
   enum panel_id_t {    
      PANEL_GROUPS = 0,
      PANEL_REMAP,
      PANEL_BACKLIGHT,
      PANEL_NUM
   };
   enum rollout_id_t {
      ROLLOUT_MAIN = 0,
      ROLLOUT_NUM,
   };
   enum radiogroup_id_t {
      RADGROUP_REMAP = 0,
      RADGROUP_BACKLIGHT,
      RADGROUP_NUM
   };
   enum radiobutton_id_t {
      RADBUT_REMAP_NONE = 0,
      RADBUT_REMAP_TOON,
      RADBUT_REMAP_SMOTHSTEP,
      RADBUT_BACKLIGHT_NONE,
      RADBUT_BACKLIGHT_DARK,
      RADBUT_BACKLIGHT_LIGHT,
      RADBUT_NUM
   };
   enum checkbox_id_t { 
      CHECK_ENABLED = 0,
      CHECK_REMAP_NL,
      CHECK_NUM
   }; 
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<ToneShaderUI*>  _ui;       
  
 public:
    //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ToneShaderUI", ToneShaderUI*, ToneShaderUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   ToneShaderUI(BaseUI* parent);
   virtual ~ToneShaderUI(){}

   /******** MEMBERS METHODS ********/
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();

   void apply_changes_to_texture_parent(int op, ToneShader* tex, int layer);
 protected:   
   /******** MEMBERS VARS ********/
   int               _id;
   operation_id_t    _last_op;
   str_list          _texture_filenames;

 protected:     
   /******** Convenience Methods ********/
   void apply_changes_to_texture(operation_id_t op, ToneShader* tex, int layer);
   
 
   /******** STATIC CALLBACK METHODS ********/
   static void  button_cb(int id);
   static void  listbox_cb(int id);
   static void  slider_cb(int id);
   static void  spinner_cb(int id);
   static void  radiogroup_cb(int id);
   static void  rotation_cb(int id);
   static void  checkbox_cb(int id);
};


#endif // _TONE_SHADER_UI_H_IS_INCLUDED_

/* end of file tone_shader_ui.H */
