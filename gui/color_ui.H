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
#ifndef _COLOR_UI_H_IS_INCLUDED_
#define _COLOR_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// ColorUI
////////////////////////////////////////////

#include "disp/colors.H"
#include "gui/base_ui.H"
#include <vector>

class PresetsUI;

/*****************************************************************
 * ColorUI
 *****************************************************************/
class ColorUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum button_id_t {
      BUT_CURRENT =0,
      BUT_LAST, 
      PRESET,
      PRESET1,
      PRESET2,
      PRESET3,
      PRESET4,
      PRESET5,
      PRESET6,
      PRESET7,
      PRESET8,
      PRESET9,
      PRESET10,
      PRESET11,
      BUT_NUM 
   };   
   
   enum slider_id_t{
      SLIDER_R=0,
      SLIDER_G,
      SLIDER_B,    
      SLIDER_NUM
   };
   
   enum rollout_id_t {
      ROLLOUT_MAIN = 0,    
      ROLLOUT_NUM,
   };  
   enum radiogroup_id_t {
      RADGROUP_COLOR = 0,
      RADGROUP_NUM
   };
   enum radiobutton_id_t {
      RADBUT_HSV = 0,
      RADBUT_RGB,      
      RADBUT_NUM
   };
   enum edittext_id_t{
      EDITTEXT_R=0,
      EDITTEXT_G,
      EDITTEXT_B,  
      EDITTEXT_A,  
      EDITTEXT_NUM
   };
  
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<ColorUI*>  _ui;       
  
 public:
   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ColorUI", ColorUI*, ColorUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   ColorUI(BaseUI* parent);
   virtual ~ColorUI(){}

   /******** MEMBERS METHODS ********/  
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();
   virtual bool child_callback(BaseUI* sender, int event);
   CCOLOR&      get_color() const { return _color; }

   // set RGB or HSV color:
   void set_current_color(float a, float b, float c,
                          bool rgb_input, bool update_last=true);
   // set RGB color from a COLOR:
   void set_current_color(CCOLOR& c, bool update_last=true) {
      set_current_color(c[0], c[1], c[2], true, update_last);
   }

   void         set_alpha(float a);
   float        get_alpha()         { return _alpha; }
 protected:
   
   /******** MEMBERS VARS ********/
   int                 _id;
   COLOR               _color;  
   float               _alpha;
   PresetsUI*          _presets_ui;   
   bool                _rgb;   
   bool                _set_palette_mode;

 protected:     
   /******** Convenience Methods ********/
    
   void         update_color();
   void         apply_color(CARRAY<COLOR>& colors);
       
   bool         load_preset();
   bool         save_preset();
   
   void         try_set_active_preset();
   void         switch_color_mode(bool rgb);
   /******** STATIC CALLBACK METHODS ********/
   static void  button_cb(int id);   
   static void  slider_cb(int id);
   static void  radiogroup_cb(int id);
   static void  edittext_cb(int id);

};


#endif // _COLOR_UI_H_IS_INCLUDED_

/* end of file color_ui.H */
