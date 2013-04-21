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
#ifndef _IMG_LINE_UI_H_IS_INCLUDED_
#define _IMG_LINE_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// ImageLineUI
////////////////////////////////////////////

#include "disp/view.H"
#include "base_ui.H"

#include "patch_selection_ui.H"
#include "gtex/glsl_toon.H"

#include <vector>

class ToneShaderUI;
class LightUI;
class ColorUI;
class ImageLineShader;
class DetailCtrlUI;
class BasecoatUI;
/*****************************************************************
 * ImageLineUI
 *****************************************************************/
class ImageLineUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum operation_id_t {
      OP_ALL = 0,
      OP_TONE_SHADER,
      OP_COLOR,
      OP_LINE,
      OP_CURVATURE0,
      OP_CURVATURE1,
      OP_HIGHLIGHT_CONTROL,
      OP_LIGHT_CONTROL,
      OP_WIDTH,
      OP_DEBUG_SHADER,
      OP_REF_IMG,
      OP_CONFIDENCE,
      OP_SILHOUETTE_MODE,
      OP_TAPERING_MODE,
      OP_DRAW_SILHOUETTE,
      OP_ALPHA_OFFSET,
      OP_BLUR_SIZE,
      OP_MOVING_FACTOR,
      OP_TONE_EFFECT,
      OP_SHININESS,
      OP_SHOW_CHANNEL,
      OP_HT_WIDTH_CONTROL,
      OP_NUM
   };

   enum spinner_id_t{     
      SPINNER_NUM
   };
   enum panel_id_t {
      PANEL_MAIN = 0,
      PANEL_CONFIDENCE,
      PANEL_SILHOUETTE,
      PANEL_NUM
   };
   enum rollout_id_t {
      ROLLOUT_MAIN = 0,
      ROLLOUT_BASECOAT,
      ROLLOUT_COLOR_SELECTION,
      ROLLOUT_LINE,
      ROLLOUT_LINE2,
      ROLLOUT_REF_IMG,
      ROLLOUT_DEBUG_SHADER,
      ROLLOUT_NUM,
   };
   enum radiogroup_id_t { 
      RADGROUP_COLOR = 0,
      RADGROUP_REF_IMG,
      RADGROUP_DEBUG_SHADER,
      RADGROUP_LINE,
      RADGROUP_CONFIDENCE,
      RADGROUP_SILHOUETTE,
      RADGROUP_SHOW_CHANNEL,
      RADGROUP_NUM
   };
   enum radiobutton_id_t {   
      RADBUT_COLOR_BASE0,
      RADBUT_COLOR_BASE1,
      RADBUT_COLOR_DARK,
      RADBUT_COLOR_LIGHT,
      RADBUT_COLOR_HIGHLIGHT,
      RADBUT_COLOR_BACKGROUND,
      RADBUT_REF_IMG_NONE,
      RADBUT_REF_IMG_TONE,
      RADBUT_REF_IMG_BLUR,
      RADBUT_DEBUG_NONE,
      RADBUT_DEBUG_SIGN,
      RADBUT_DEBUG_MOVE,
      RADBUT_DEBUG_CURV,
      RADBUT_DEBUG_DIST,
      RADBUT_DEBUG_ANGLE,
      RADBUT_LINE_NONE,
      RADBUT_LINE_ALL,
      RADBUT_LINE_DK,
      RADBUT_LINE_LT,
      RADBUT_LINE_HT,
      RADBUT_LINE_DK_LT,
      RADBUT_LINE_DK_HT,
      RADBUT_LINE_LT_HT,
      RADBUT_CONFIDENCE_NONE,
      RADBUT_CONFIDENCE_NORMAL,
      RADBUT_CONFIDENCE_DEBUG,
      RADBUT_SILHOUETTE_NONE,
      RADBUT_SILHOUETTE_TAPERING,
      RADBUT_CHANNEL_ALL,
      RADBUT_CHANNEL_RED,
      RADBUT_CHANNEL_GREEN,
      RADBUT_CHANNEL_LUMI,
      RADBUT_NUM
   };

   enum checkbox_id_t { 
      CHECK_DRAW_SILHOUETTE,
      CHECK_TAPERING_MODE,
      CHECK_TONE_EFFECT,
      CHECK_NUM
   }; 

   enum slider_id_t {
      SLIDE_CURV_THRESHOLD0 = 0,
      SLIDE_CURV_THRESHOLD1,
      SLIDE_HIGHLIGHT_CONTROL,
      SLIDE_LIGHT_CONTROL,
      SLIDE_LINE_WIDTH,
      SLIDE_ALPHA_OFFSET,
      SLIDE_BLUR_SIZE,
      SLIDE_MOVING_FACTOR,
      SLIDE_SHININESS,
      SLIDE_HT_WIDTH_CONTROL,
      SLIDE_NUM
   };
   enum listbox_id_t {
      LIST_TEXTURE = 0,
      LIST_NUM
   };
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<ImageLineUI*>  _ui;       
   static HASH             _hash; 
  
 public:
    //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ImageLineUI", ImageLineUI*, ImageLineUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   ImageLineUI(BaseUI* parent=0);
   virtual ~ImageLineUI(){}

   /******** MEMBERS METHODS ********/
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();
   virtual bool child_callback(BaseUI* sender, int event);
   virtual ToneShader*  get_tone_shader();
   virtual ImageLineShader*  get_image_line_shader();
   virtual int          get_current_layer(); 

   //GTexture*  get_texture() { return _current_tex; }

   static bool             is_vis_external(CVIEWptr& v);
   static bool             show_external(CVIEWptr& v);
   static bool             hide_external(CVIEWptr& v);
   static bool             update_external(CVIEWptr& v);

protected:
   static ImageLineUI*          fetch(CVIEWptr& v);
 protected:   
   /******** MEMBERS VARS ********/
   int                  _id;
   
   float                _curv_threshold[2];
   float                _highlight_control;
   float                _light_control;
   float                _line_width;
   COLOR                _base_color[2], _dk_color, _ht_color;
   int                  _color_sel;

   PatchSelectionUI*    _texture_selection_ui;
   operation_id_t       _last_op;
   ImageLineShader*     _current_tex;
   ToneShaderUI*        _tone_shader_ui;
   LightUI*             _light_ui;
   ColorUI*             _color_ui;
   DetailCtrlUI*        _detail_ctrl_ui;
   BasecoatUI*          _basecoat_ui;

 protected:     
   /******** Convenience Methods ********/
   void         toggle_enable_me(); 
   void         apply_changes();
   void         apply_changes_to_texture(operation_id_t op, ImageLineShader* tex, int layer);

   void          set_colors(ImageLineShader* tex, const int i);
   void          get_colors(ImageLineShader* tex);
   ImageLineShader* get_current_image_line_shader(CVIEWptr& v);

   /******** STATIC CALLBACK METHODS ********/
   static void  spinner_cb(int id);
   static void  radiogroup_cb(int id);
   static void  button_cb(int id);     
   static void  checkbox_cb(int id);
   static void  slider_cb(int id);
   static void  listbox_cb(int id);
};

#endif // _IMG_LINE_UI_H_IS_INCLUDED_

/* end of file im_line_ui.H */
