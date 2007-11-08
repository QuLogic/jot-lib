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
#ifndef _PAINTERLY_UI_H_IS_INCLUDED_
#define _PAINTERLY_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// PainterlyUI
////////////////////////////////////////////

#include "disp/view.H"
#include "gui/base_ui.H"
#include "gtex/painterly.H"

#include "patch_selection_ui.H"

#include <vector>

class ColorUI;
class PresetsUI;
class ToneShaderUI;
/*****************************************************************
 * PainterlyUI
 *****************************************************************/
class PainterlyUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum operation_id_t {
      OP_ALL = 0,
      OP_COLOR,
      OP_ENABLE,
      OP_HIGHLIGHT,
      OP_ANGLE,
      OP_SCALE,
      OP_PAPER_TEXTURE,
      OP_PATTERN_TEXTURE,
      OP_PAPER_CONTRAST,
      OP_PAPER_SCALE,
      OP_TONE_SHADER,
      OP_CHANNEL,
      OP_TONE_PUSH,
      OP_NUM
   };

   enum button_id_t {      
      BUT_NUM 
   };
   enum rotation_id_t {      
      ROT_NUM
   };
   enum listbox_id_t {
      LIST_PATTERN = 0,
      LIST_PAPER,
      LIST_NUM
   };
   enum slider_id_t {
      SLIDER_ANGLE = 0,
      SLIDER_SCALE,
      SLIDER_PAPER_CONTRAST,
      SLIDER_PAPER_SCALE,
      SLIDER_TONE_PUSH,
      SLIDER_NUM
   };
   enum spinner_id_t{     
      SPINNER_NUM
   };
   enum panel_id_t {
      PANEL_MAIN = 0,
      PANEL_PROPERTIES,
      PANEL_COLOR_SWITCH,
      PANEL_NUM
   };
   enum rollout_id_t {
      ROLLOUT_HATCHING = 0,
      ROLLOUT_NUM,
   };
   enum radiogroup_id_t { 
      RADGROUP_COLOR_SEL = 0,     
      RADGROUP_NUM
   };
   enum radiobutton_id_t {   
      RADBUT_STROKE_COL = 0,
      RADBUT_BASE_COL,
      RADBUT_BACKGROUND,    
      RADBUT_NUM
   };
   enum checkbox_id_t {  
      CHECK_ENABLE,
      CHECK_HIGHLIGHT,
      CHECK_NUM
   }; 
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<PainterlyUI*>  _ui;       
  
 public:
    //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("PainterlyUI", PainterlyUI*, PainterlyUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   PainterlyUI(BaseUI* parent=0);
   virtual ~PainterlyUI(){}

   /******** MEMBERS METHODS ********/
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();
   virtual bool child_callback(BaseUI* sender, int event);

   int          get_current_layer() { return _texture_selection_ui->get_layer_num(); }
   int          get_current_channel() { return _texture_selection_ui->get_channel_num(); }
   ToneShader*  get_tone_shader() { return (_current_painterly) ? _current_painterly->get_tone_shader() : 0; }
   bool         load_preset();
   bool         save_preset();
 protected:   
   /******** MEMBERS VARS ********/
   int                  _id;
   
   ColorUI*             _color_ui;
   PatchSelectionUI*    _texture_selection_ui;
   operation_id_t       _last_op;
   Painterly*          _current_painterly;
   PresetsUI*           _presets_ui;
   ToneShaderUI*        _tone_shader_ui;
   str_list             _pattern_filenames;
   str_list             _paper_filenames;
   int                  _color_sel;
 protected:     
   /******** Convenience Methods ********/
   void         toggle_enable_me(); 
   void         apply_changes();
   void         apply_changes_to_texture(operation_id_t op, Painterly* tex, int layer);
  
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
