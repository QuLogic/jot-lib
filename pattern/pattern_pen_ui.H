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
/***************************************************************************
    pattern_pen_ui.H
    
    PatternPenUI 
        -GLUI interface for PatternPen       
    -------------------
    Simon Breslav
    Fall 2004
***************************************************************************/
#ifndef _PATTERN_PEN_UI_H_IS_INCLUDED_
#define _PETTERN_PEN_UI_H_IS_INCLUDED_

#include <map>
#include <string>

class GLUI;
class GLUI_Listbox;
class GLUI_Button;
class GLUI_Slider;
class GLUI_Panel;
class GLUI_Rollout;
class GLUI_Rotation;
class GLUI_Checkbox;
class GLUI_RadioGroup;
class GLUI_RadioButton;

class PatternPen;
class BaseStroke;

class PatternPenUI {
public:
  //******** MANAGERS ********
  PatternPenUI(PatternPen* p);
  ~PatternPenUI();
  static bool   use_image_pressure;
  static bool   use_width;
  static bool   use_alpha;
  static bool   use_image_color;   
  static bool   show_boundery_box;   
  static double luminance_function(double l);
  static enum lum_fun_id_t {
    LUM_SHADOW = 0,
    LUM_HIGHLIGHT
  } lum_function;

  //******** ENUMS ********
  enum button_id_t {
    BUT_NEXT_PEN = 0,
    BUT_PREV_PEN,
    BUT_POP,
    BUT_CLEAR,
    BUT_NEW_GROUP,
    BUT_POP_SYNTH,
    BUT_CLEAR_SYNTH,
    BUT_RESYNTH,
    BUT_NUM
  };
  enum panel_id_t {
    PANEL_PEN = 0,  
    PANEL_MODE,  
    PANEL_REF,     
    PANEL_STROKES,
    PANEL_IMAGE,
    PANEL_COLOR,
    PANEL_NUM
  };
  enum rollout_id_t {
    ROLLOUT_ANALYSIS = 0,
    ROLLOUT_SYNTH,
    ROLLOUT_PATH,
    ROLLOUT_PROXY,
    ROLLOUT_ELLIPSE,
    ROLLOUT_NUM
  };
  enum listbox_id_t {
    LIST_MODE = 0,
    LIST_TYPE,
    LIST_PARAM,
    LIST_CELL,
    LIST_SYNTH,
    LIST_DIST,
    LIST_STROKE_PRESET,	
    LIST_LUMIN_FUNC,
    LIST_NUM
  };
  enum checkbox_id_t {
    CHECK_EXTEND = 0,
    CHECK_ANAL_STYLE,
    CHECK_STRETCH,
    CHECK_STRUCT,
    CHECK_FRAME,
    CHECK_IMAGE_PRESSURE,
    CHECK_IMAGE_ALPHA,
    CHECK_IMAGE_WIDTH,
    CHECK_IMAGE_COLOR,
    CHECK_SHOW_BBOX,
    CHECK_SHOW_ICON,
    CHECK_NUM
  };
  enum slider_id_t {    
    SLIDE_EPS = 0,
    SLIDE_STYLE,
    SLIDE_GLOBAL_SCALE,
    SLIDE_RING,
    SLIDE_CORRECT,
    SLIDE_COLOR_H,
    SLIDE_COLOR_S,
    SLIDE_COLOR_V,
    SLIDE_NUM
  };
  enum rotation_id_t {
    ROT_NUM = 0
  };
  enum radiogroup_id_t {     
    RADGROUP_NUM = 0
  };
  
  enum radiobutton_id_t {      
    RADBUT_NUM = 0
  };

      
  //******** MEMBERS METHODS ********
  void show();
  void hide();
  void update();
  PatternPen* pen()   const { return _pen; }

protected:
  static ARRAY<PatternPenUI*>   _ui;
  
  void         init();   
  void         build();
  void         destroy();  
   
  //******** MEMBERS ********    
  GLUI*                  _glui;
  PatternPen*            _pen;
  int                    _id;
  bool                   _init;
  str_list               _preset_filenames;
  int                    _current_mode;  
  mlib::Wvec             _light;
  BaseStroke*            _stroke;
  
  ARRAY<GLUI_Button*>    _button;
  ARRAY<GLUI_Panel*>     _panel;
  ARRAY<GLUI_Listbox*>   _listbox;
  ARRAY<GLUI_Checkbox*>  _checkbox;
  ARRAY<GLUI_Slider*>    _slider;   
  ARRAY<GLUI_Rollout*>   _rollout;
  ARRAY<GLUI_Rotation*>  _rotation;
  ARRAY<GLUI_RadioGroup*>  _radgroup;
  ARRAY<GLUI_RadioButton*> _radbutton;   
   
  //void     update_non_lives();
  //void     update_from_layer_vals();
  //void     update_light();
  //void     update_limits();
  //void     apply_vals_to_layer();
  //void     apply_light();
  //void     add_layer();
  //void     add_grid_layer();
  //void     del_layer();
  void     fill_preset_listbox( GLUI_Listbox *listbox,
								str_list     &save_files,
								Cstr_ptr     &full_path);
  bool     load_preset(const char *f);
  void     preset_stroke();


  //******** CALLBACK METHODS ********
  static void button_cb(int id);     
  static void listbox_cb(int id);
  static void checkbox_cb(int id);    
  static void slider_cb(int id);
  static void rotation_cb(int id);
  static void radiogroup_cb(int id);   
     
  //******** I/O METHODS ********
  // modes
  void change_mode();
  // strokes
  void set_epsilon();
  void set_style_adjust();
  void pop_stroke();
  void clear_strokes();
  void set_reference_frame();
  void set_analyze_style();
  void create_new_group();
  void display_structure();
  void display_reference_frame();
  // synthesis
  void set_cell_type();
  void show_example_strokes();
  void set_global_scale();
  void set_synth_mode();
  void set_distribution();
  void set_stretching_state();
  void set_ring_nb();
  void set_correction_amount();
  void apply_color_adjust();

};
#endif
