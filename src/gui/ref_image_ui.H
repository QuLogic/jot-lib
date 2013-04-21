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
#ifndef _REF_IMAGE_UI_H_IS_INCLUDED_
#define _REF_IMAGE_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// RefImageUI
////////////////////////////////////////////

#include "gui/base_ui.H"
#include <vector>


class RefImageUI : public BaseUI {

 public:
   
    
    /******** WIDGET ID NUMBERS ********/
    
    
    enum rollout_id_t {
      ROLLOUT_MAIN = 0,    
      ROLLOUT_NUM,
     };
    
    enum radiogroup_id_t {
      RADGROUP_REF_IMAGES = 0,
      RADGROUP_NUM
   };
  
    enum radiobutton_id_t {
      RADBUT_MAIN = 0,
      RADBUT_TONE0,
      RADBUT_TONE1,
      RADBUT_HALO,
      RADBUT_NUM
   };

    enum slider_id_t {
       SLIDER_HALO = 0,
       SLIDER_NUM
    };

    enum listbox_id_t {
      LISTBOX_SKY = 0,
      LISTBOX_NUM 
    };

    enum checkbox_id_t {
      CHECKBOX_SKY = 0,
      CHECKBOX_NUM
    };

   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<RefImageUI*>  _ui;      


public:
   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("RefImageUI", RefImageUI*, RefImageUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   RefImageUI(BaseUI* parent);
   virtual ~RefImageUI(){}

   /******** MEMBERS METHODS ********/  
   virtual void build(GLUI*, GLUI_Panel*, bool open);  


protected:
   
   /******** MEMBERS VARS ********/
   int                 _id;
   str_list            _sky_filenames;
   
   /******** STATIC CALLBACK METHODS ********/
   static void  radiogroup_cb(int id);
   static void  slider_cb(int id);
   static void  listbox_cb(int id);
   static void  checkbox_cb(int id);

};


#endif // _REF_IMAGE_UI_H_IS_INCLUDED_

// end of ref_image_ui.H
