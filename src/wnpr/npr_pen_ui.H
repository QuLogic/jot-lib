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
#ifndef _NPR_PEN_UI_H_IS_INCLUDED_
#define _NPR_PEN_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// ViewUI
////////////////////////////////////////////

#include "disp/view.H"

class NPRPen;

class GLUI;
class GLUI_Listbox;
class GLUI_Button;
class GLUI_Slider;
class GLUI_Panel;
class GLUI_Rollout;
class GLUI_Rotation;
class GLUI_Translation;
class GLUI_RadioGroup;
class GLUI_RadioButton;
class GLUI_Checkbox;
class GLUI_EditText;

/*****************************************************************
 * NPRPenUI
 *****************************************************************/
class NPRPenUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum button_id_t {
      BUT_REFRESH=0,
      BUT_NEXT_PEN,
      BUT_PREV_PEN,
      BUT_ADD,
      BUT_DEL,
      BUT_SMOOTH,
      BUT_ELLIPTIC,
      BUT_SPHERIC,
      BUT_CYLINDRIC,
      BUT_NUM
   };

   enum rotation_id_t {
      ROT_ROT=0,
      ROT_X,
      ROT_Y,
      ROT_Z,
      ROT_L,
      ROT_NUM
   };

   enum scaling_id_t {
      SCALE_UNIFORM=0,
      SCALE_X,
      SCALE_Y,
      SCALE_Z,
      SCALE_NUM
   };

   enum translation_id_t {
      TRAN_X=0,
      TRAN_Y,
      TRAN_Z,
      TRAN_NUM
   };

   enum listbox_id_t {
      LIST_TEX=0,
      LIST_LAYER,
      LIST_DETAIL,
	  LIST_DETAIL2,
      LIST_NUM
   };

   enum slider_id_t {
      SLIDE_T = 0,
      SLIDE_F,
      SLIDE_L,
      SLIDE_H,
      SLIDE_S,
      SLIDE_V,
      SLIDE_A,
      SLIDE_R,
      SLIDE_NUM
   };

   enum panel_id_t {
      PANEL_GTEX=0,
      PANEL_GTEX_GLOBS,
      PANEL_GTEX_LAYER_NAME,
      PANEL_GTEX_LAYER_OPTS,
      PANEL_GTEX_LIGHT_OPTS,
      PANEL_NORMALS,
      PANEL_TEX,
      PANEL_XFORM,
      PANEL_PEN,
      PANEL_EDIT_XFORM,
      PANEL_NUM
   };

   enum rollout_id_t {
      ROLLOUT_GTEX = 0,
      ROLLOUT_XFORM,
      ROLLOUT_TOON,
      ROLLOUT_LIGHT,
      ROLLOUT_HACK,
      ROLLOUT_NUM
   };

   enum radiogroup_id_t {
      RADGROUP_GTEX = 0,
      RADGROUP_LIGHT,
      RADGROUP_NUM
   };

   enum radiobuttonL_id_t {
      RADBUT_SOLID=0,
      RADBUT_TOON,
	  RADBUT_XTOON,
      RADBUT_LIGHT0,
      RADBUT_LIGHT1,
      RADBUT_LIGHT2,
      RADBUT_LIGHT3,
      RADBUT_LIGHTC,
      RADBUT_NUM
   };

   enum checkbox_id_t {
      CHECK_PAPER=0,
      CHECK_CENTER,
      CHECK_TRANS,
      CHECK_LIGHT,
      CHECK_SPEC,
      CHECK_TRAVEL,
      CHECK_ANNOTATE,
      CHECK_DIR,
      CHECK_CAM,
      CHECK_INV,
      CHECK_NUM
   };

   enum edittext_id_t {
      EDIT_TRAN_X=0,
      EDIT_TRAN_Y,
      EDIT_TRAN_Z,
      EDIT_SCALE_X,
      EDIT_SCALE_Y,
      EDIT_SCALE_Z,
      EDIT_ROT_X,
      EDIT_ROT_Y,
      EDIT_ROT_Z,
      EDIT_NAME,
      EDIT_NUM
   };


 protected:
   /******** STATIC MEMBERS VARS ********/

   static ARRAY<NPRPenUI*>   _ui;

   /******** MEMBERS VARS ********/

   int                           _id;
   bool                          _init;

   GLUI*                         _glui;   

   NPRPen*	                     _pen;

   mlib::Wtransf                       _xform;
   mlib::Wpt                           _center;
   mlib::Wvec                          _light;

   str_list                      _other_filenames;
   str_list                      _toon_filenames;

   ARRAY<GLUI_Listbox*>          _listbox;
   ARRAY<GLUI_Button*>           _button;
   ARRAY<GLUI_Slider*>           _slider;
   ARRAY<GLUI_Panel*>            _panel;
   ARRAY<GLUI_Rollout*>          _rollout;
   ARRAY<GLUI_Rotation*>         _rotation;
   ARRAY<GLUI_Translation*>      _translation;
   ARRAY<GLUI_Translation*>      _scale;
   ARRAY<GLUI_RadioGroup*>       _radgroup;
   ARRAY<GLUI_RadioButton*>      _radbutton;
   ARRAY<GLUI_Checkbox*>         _checkbox;
   ARRAY<GLUI_EditText*>         _edittext;

  float _old_target_value [6];
  float _old_factor_value [6];


   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
 public:     
   NPRPenUI(NPRPen *);
   virtual ~NPRPenUI();

   /******** MEMBERS METHODS ********/

   void     show();
   void     hide();
   void     update();

   void     deselect();
   void     select();
 protected:      

   void     init();

   void     build();
   void     destroy();

   void     update_non_lives();
   void     update_gtex();
   void     update_xform();

   void     apply_gtex();
   void     apply_xform();

   void     add_layer();
   void     del_layer();
   void     rename_layer();

   void     smooth_normals();
   void     elliptic_normals();
   void     spheric_normals();
   void     cylindric_normals();
   void     compute_curvatures();

   void     refresh_tex();

   void     get_xform_edits();
   void     update_xform_edits();
   void     update_scale_edits();

   /******** Convenience Methods ********/

   void     fill_tex_listbox(GLUI_Listbox *lb, Cstr_list &save_files);
   void     fill_layer_listbox();
   str_ptr  get_layer_name(int);
   str_ptr  try_layer_name(int,str_ptr);
   bool     set_layer_name(int,str_ptr);

   /******** STATIC CALLBACK METHODS ********/
   static void button_cb(int id);
   static void listbox_cb(int id);
   static void edittext_cb(int id);
   static void slider_cb(int id);
   static void radiogroup_cb(int id);
   static void rotation_cb(int id);
   static void translation_cb(int id);
   static void scale_cb(int id);
   static void checkbox_cb(int id);
   static void edit_xform_cb(int id);
};


#endif // _NPR_PEN_UI_H_IS_INCLUDED_

/* end of file npr_pen_ui.H */
