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
#ifndef _LIGHT_UI_H_IS_INCLUDED_
#define _LIGHT_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// LightUI
////////////////////////////////////////////

#include "disp/view.H"
#include "gui/base_ui.H"

class ColorUI;
class PresetsUI;
/*****************************************************************
 * LightUI
 *****************************************************************/
class LightUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum button_id_t {
      BUT_NUM = 0
   };
   enum rotation_id_t {
      ROT_LIGHT=0,
      ROT_SPOT,
      ROT_NUM
   };
   enum listbox_id_t {
      LIST_NUM=0
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
      PANEL_NUM
   };

   enum rollout_id_t {
      ROLLOUT_LIGHT = 0,
      ROLLOUT_SPOT,
      ROLLOUT_NUM,
   };
   enum radiogroup_id_t {
      RADGROUP_LIGHTNUM = 0,
      RADGROUP_LIGHTCOL,
      RADGROUP_NUM
   };
   enum radiobutton_id_t {
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
      CHECK_NUM
   }; 
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static LightUI*  _ui;       
  
 public:
   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("LightUI", LightUI*, LightUI, BaseUI*);

   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   LightUI(VIEWptr v);
   virtual ~LightUI(){}

   /******** MEMBERS METHODS ********/
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();
   virtual bool child_callback(BaseUI* sender, int event);
   void   set_current_light(int i) { _radgroup[RADGROUP_LIGHTNUM]->set_int_val(i); update(); }
 protected:
   
   /******** MEMBERS VARS ********/
   VIEWptr                       _view;
   mlib::Wvec                    _light_dir;
   mlib::Wvec                    _spot_dir;  
   ColorUI*                      _color_ui;
   PresetsUI*                    _presets_ui;
   COLOR                         _color;
 protected:     
   /******** Convenience Methods ********/
   void         update_light();
   void         apply_light();  
   bool         load_preset();
   bool         save_preset();

   /******** STATIC CALLBACK METHODS ********/
   static void  button_cb(int id);
   static void  listbox_cb(int id);
   static void  slider_cb(int id);
   static void  spinner_cb(int id);
   static void  radiogroup_cb(int id);
   static void  rotation_cb(int id);
   static void  checkbox_cb(int id);
};


#endif // _LIGHT_UI_H_IS_INCLUDED_

/* end of file light_ui.H */
