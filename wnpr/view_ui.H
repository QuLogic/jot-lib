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
#ifndef _VIEW_UI_H_IS_INCLUDED_
#define _VIEW_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// ViewUI
////////////////////////////////////////////

#include "disp/view.H"

class GLUI;
class GLUI_Listbox;
class GLUI_EditText;
class GLUI_Button;
class GLUI_Slider;
class GLUI_StaticText;
class GLUI_Spinner;
class GLUI_Panel;
class GLUI_Rollout;
class GLUI_Rotation;
class GLUI_RadioButton;
class GLUI_RadioGroup;
class GLUI_Checkbox;


/*****************************************************************
 * ViewUI
 *****************************************************************/
class ViewUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum button_id_t {
      BUT_DEBUG,
      BUT_NUM
   };

   enum rotation_id_t {
      ROT_LIGHT=0,
      ROT_SPOT,
      ROT_NUM
   };

   enum listbox_id_t {
      LIST_BKGTEX=0,
      LIST_PAPER,
      LIST_ANTI,
      LIST_NUM
   };

   enum slider_id_t {
      SLIDE_LH = 0,
      SLIDE_LS,
      SLIDE_LV,
      SLIDE_SPOT_EXPONENT,
      SLIDE_SPOT_CUTOFF,
      SLIDE_SPOT_K0,
      SLIDE_SPOT_K1,
      SLIDE_SPOT_K2,
      SLIDE_BH,
      SLIDE_BS,
      SLIDE_BV,
      SLIDE_BA,
      SLIDE_BRIG,
      SLIDE_CONT,
      SLIDE_SUGR,
      SLIDE_SUGT,
      SLIDE_SUGMIN,
      SLIDE_SUGMAX,
      SLIDE_SUGLWID,
      SLIDE_SUGOBJT,
      SLIDE_SUGOBJT2,
      SLIDE_SUGOBJT3,
      SLIDE_SUGOBJT5,
      SLIDE_SUGOBJT2a,
      SLIDE_SUGOBJT3a,
      SLIDE_SUGOBJWHICH,
      SLIDE_SUGOBJDREG,
      SLIDE_NUM,
   };

   enum spinner_id_t{
      SPINNER_LIGHT_DIR_X=0,
      SPINNER_LIGHT_DIR_Y,
      SPINNER_LIGHT_DIR_Z,
      SPINNER_LIGHT_SPOT_X,
      SPINNER_LIGHT_SPOT_Y,
      SPINNER_LIGHT_SPOT_Z,
      SPINNER_NUM
   };
   enum panel_id_t {
      PANEL_LIGHTTOP=0,
      PANEL_LIGHT_SPOTDIR,
      PANEL_BKGTEX,
      PANEL_PAPER,
      PANEL_ANTI,
      PANEL_NUM
   };

   enum rollout_id_t {
      ROLLOUT_LIGHT = 0,
      ROLLOUT_SPOT,
      ROLLOUT_BKG,
      ROLLOUT_PAPER,
      ROLLOUT_ANTI,
      ROLLOUT_LINE,
      ROLLOUT_LINE_OBJ,
      ROLLOUT_NUM,
   };

   enum radiogroup_id_t {
      RADGROUP_LIGHTNUM = 0,
      RADGROUP_LIGHTCOL,
      RADGROUP_NUM
   };

   enum radiobuttonL_id_t {
      RADBUT_LIGHT0 = 0,
      RADBUT_LIGHT1,
      RADBUT_LIGHT2,
      RADBUT_LIGHT3,
      RADBUT_LIGHT4,
      RADBUT_LIGHT5,
      RADBUT_LIGHT6,
      RADBUT_LIGHT7,
      RADBUT_LIGHTG,
      RADBUT_DIFFUSE,
      RADBUT_AMBIENT,
      RADBUT_NUM
   };

   enum checkbox_id_t {
      CHECK_POS = 0,
      CHECK_CAM,
      CHECK_ENABLE,
      CHECK_SHOW_WIDGET,
      CHECK_PAPER,
      CHECK_ACTIVE,
      CHECK_ANTI,
      CHECK_SUGKEY,
      CHECK_SUGLCK,
      CHECK_SUGMED,
      CHECK_SUGOBJ_ENABLE,
      CHECK_SUGOBJ_C1,
      CHECK_SUGOBJ_C2,
      CHECK_SUGOBJ_C3,
      CHECK_SUGOBJ_C4,
      CHECK_SUGOBJ_C5,
      CHECK_SUGOBJ_C6,
      CHECK_NUM
   };


 protected:
   /******** STATIC MEMBERS VARS ********/
   static HASH             _hash; 
   static ARRAY<ViewUI*>   _ui;
 
 public:
   static bool             is_vis(CVIEWptr& v);
   static bool             show(CVIEWptr& v);
   static bool             hide(CVIEWptr& v);
   static bool             update(CVIEWptr& v);

 protected:

   static ViewUI*          fetch(CVIEWptr& v);

 protected:
   /******** MEMBERS VARS ********/

   int                           _id;
   bool                          _init;
   VIEWptr                       _view;
   mlib::Wvec                    _light_dir;
   mlib::Wvec                    _spot_dir;

   GLUI*                         _glui;   
   str_list                      _bkgtex_filenames;
   str_list                      _paper_filenames;
   ARRAY<GLUI_Listbox*>          _listbox;
   ARRAY<GLUI_Button*>           _button;
   ARRAY<GLUI_Slider*>           _slider;
   ARRAY<GLUI_Spinner*>          _spinner;
   ARRAY<GLUI_Panel*>            _panel;
   ARRAY<GLUI_Rollout*>          _rollout;
   ARRAY<GLUI_Rotation*>         _rotation;
   ARRAY<GLUI_RadioGroup*>       _radgroup;
   ARRAY<GLUI_RadioButton*>      _radbutton;
   ARRAY<GLUI_Checkbox*>         _checkbox;

   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
 protected:     
   ViewUI(VIEWptr v);
   virtual ~ViewUI();

   /******** MEMBERS METHODS ********/

   bool     internal_is_vis();
   bool     internal_show();
   bool     internal_hide();
   bool     internal_update();
      
   void     init();

   void     build();
   void     destroy();

   void     update_non_lives();
   void     update_light();
   void     update_bkg();
   void     update_paper();
   void     update_anti();
   void     update_lines();
   void     update_lines_obj();

   void     apply_light();
   void     apply_bkg();
   void     apply_paper();
   void     apply_anti();
   void     apply_lines();
   void     apply_lines_obj();

   /******** Convenience Methods ********/

   void     fill_bkgtex_listbox(GLUI_Listbox *lb, str_list &save_files, Cstr_ptr &full_path);
   void     fill_paper_listbox(GLUI_Listbox *lb, str_list &save_files, Cstr_ptr &full_path);
   void     fill_anti_listbox(GLUI_Listbox *lb);

   /******** STATIC CALLBACK METHODS ********/
   static void button_cb(int id);
   static void listbox_cb(int id);
   static void slider_cb(int id);
   static void spinner_cb(int id);
   static void radiogroup_cb(int id);
   static void rotation_cb(int id);
   static void checkbox_cb(int id);
};


#endif // _VIEW_UI_H_IS_INCLUDED_

/* end of file view_ui.H */
