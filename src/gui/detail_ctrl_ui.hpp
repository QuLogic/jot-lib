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
#ifndef _DETAIL_CTRL_UI_H_IS_INCLUDED_
#define _DETAIL_CTRL_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// DetailCtrlUI
////////////////////////////////////////////

#include "disp/view.H"
#include "gui/base_ui.H"

#include <vector>

class ToneShader;
class ImageLineShader;
/*****************************************************************
 * DetailCtrlUI
 *****************************************************************/
class DetailCtrlUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum operation_id_t {
      OP_ALL = 0,
      OP_DETAIL_FUNC,
      OP_DETAIL_TYPE,
      OP_UNIT_LEN,
      OP_EDGE_LEN_SCALE,
      OP_RATIO_SCALE,
      OP_NORMAL,
      OP_USER_DEPTH,
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
      SLIDE_UNIT_LEN = 0,
      SLIDE_EDGE_LEN_SCALE,
      SLIDE_RATIO_SCALE,
      SLIDE_USER_DEPTH,
      SLIDE_NUM
   };
   enum spinner_id_t{     
      SPINNER_BOX_SIZE = 0,
      SPINNER_NUM
   };
   enum panel_id_t {    
      PANEL_DETAIL_TYPE,
      PANEL_DETAIL_FUNC,
      PANEL_NUM
   };
   enum rollout_id_t {
      ROLLOUT_MAIN = 0,
      ROLLOUT_NUM,
   };
   enum radiogroup_id_t {
      RADGROUP_NORMAL = 0,
      RADGROUP_DETAIL_TYPE,
      RADGROUP_DETAIL_FUNC,
      RADGROUP_NUM
   };
   enum radiobutton_id_t {
      RADBUT_NORMAL_SMOOTH = 0,
      RADBUT_NORMAL_SPHERIC,
      RADBUT_NORMAL_ELLIPTIC,
      RADBUT_NORMAL_CYLINDRIC,
      RADBUT_DETAIL_ALL,
      RADBUT_DETAIL_LINE_WIDTH,
      RADBUT_DETAIL_BLUR_SIZE,
      RADBUT_DETAIL_TONE_NORMAL,
      RADBUT_DETAIL_BASE_NORMAL,
      RADBUT_DETAIL_FUNC_NONE,
      RADBUT_DETAIL_FUNC_DEPTH_UNIFORM,
      RADBUT_DETAIL_FUNC_DEPTH_GLOBAL_LENGTH,
      RADBUT_DETAIL_FUNC_DEPTH_LOCAL_LENGTH,
      RADBUT_DETAIL_FUNC_RELATIVE_LENGTH,
      RADBUT_DETAIL_FUNC_USER,
      RADBUT_NUM
   };
   enum checkbox_id_t { 
      CHECK_NUM
   }; 
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<DetailCtrlUI*>  _ui;       
  
 public:
    //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("DetailCtrlUI", DetailCtrlUI*, DetailCtrlUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   DetailCtrlUI(BaseUI* parent);
   virtual ~DetailCtrlUI(){}

   /******** MEMBERS METHODS ********/
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();

   void apply_changes_to_texture_parent(int op, ImageLineShader* tex, int layer);
 protected:   
   /******** MEMBERS VARS ********/
   int               _id;
   operation_id_t    _last_op;
   ImageLineShader*     _current_tex;

   float                _ratio_scale, _edge_len_scale, _unit_len, _user_depth;
   int                  _detail_type;
 protected:     
   /******** Convenience Methods ********/
   void apply_changes_to_texture(operation_id_t op, ImageLineShader* tex, int layer);
   
 
   /******** STATIC CALLBACK METHODS ********/
   static void  slider_cb(int id);
   static void  radiogroup_cb(int id);
   static void  checkbox_cb(int id);
   static void  spinner_cb(int id);
};


#endif // _DETAIL_CTRL_UI_H_IS_INCLUDED_

/* end of file detail_ctrl_ui.H */
