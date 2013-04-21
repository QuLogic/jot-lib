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
    base_ui.H
 ***************************************************************************/
#ifndef _BASE_UI_H_IS_INCLUDED_
#define _BASE_UI_H_IS_INCLUDED_

#include "net/rtti.H"

#include <map>

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
 * BaseUI - pure virtual base class for UI's
 * All the GUIs should be able to live on their own or inserted  
 * inside other UI's, this is done by separating build process into 
 * 2 function build() witch builds the window and calls build(GLUI*, GLUI_Panel*)
 * which creates the heart of each UI, 
 * build() is implemented in base class so that any child can be it's own 
 * window if it wants. 
 * If a UI wants to include another UI, all it has to do is call 
 * build(GLUI*, GLUI_Panel*) inside of it's build(GLUI*, GLUI_Panel*) function,
 * and update_non_lives() inside update_non_lives() function...
 * BaseUI does not take care of managing instances of UI's, meaning, 
 * if you want to have multiple instences of the UI, make sure to 
 * have a static array with the list of your ui's...
 * If you KNOW that there will only ever be one copy of 
 * the UI, then you don't have to worry about that...
 *****************************************************************/

class ToneShader;

class BaseUI
{
 public:   
   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS_BASE("BaseUI", BaseUI*);

   /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   BaseUI(str_ptr n=0);
   BaseUI(BaseUI* parent, str_ptr n=0);
   virtual ~BaseUI() { }
   
   // In most cases these don't need to be over-writen my children
   void           show();        // Display the UI window
   void           hide();        // Hide the UI window 
   void           update();      // call update_non_lives and glui_sync 
   bool           is_vis();

 protected:  
   void           build();       // Creates the window, and then calls build(GLUI*, GLUI_Panel*)
                                 // which is implemented by the  
 public:    
   // You want to overwrite if you child class needs to clean up memory
   // If overwriting make sure to call BaseUI::destroy();
   virtual  void  destroy();    
   // You NEED to implement these   
   virtual  void  build(GLUI*, GLUI_Panel*, bool open) = 0;   // All the building code in the children should go here   
   virtual  void  update_non_lives()            {};           // If the child has some functions that update UI elements 
   virtual  bool  child_callback(BaseUI* sender, int event)        { return false; };
   
   // Really should not be in the base class, only used by hatching and halftone ui's, but whatever...
   virtual int          get_current_layer() { return 0; }
   virtual int          get_current_channel() { return 0; }
   virtual ToneShader*  get_tone_shader() { return 0; }
 protected:
   
   void fill_listbox(GLUI_Listbox* listbox, Cstr_list& list);  
   void fill_directory_listbox(GLUI_Listbox* listbox,
                               str_list     &save_files,
                               Cstr_ptr     &full_path,
                               Cstr_ptr     &extension,
                               bool         hide_extention=false,
                               bool         put_default=false,
                               str_ptr      default_text="-default-");   
   /******** MEMBERS VARS ********/    
   str_ptr                             _name;     // The name of the window
   GLUI*                               _glui;
   BaseUI*                             _parent;


   std::map<int,GLUI_Listbox*>         _listbox;
   std::map<int,GLUI_Button*>          _button;
   std::map<int,GLUI_Slider*>          _slider;
   std::map<int,GLUI_Spinner*>         _spinner;
   std::map<int,GLUI_EditText*>        _edittext;
   std::map<int,GLUI_StaticText*>      _statictext;
   std::map<int,GLUI_Panel*>           _panel;
   std::map<int,GLUI_Rollout*>         _rollout;
   std::map<int,GLUI_Rotation*>        _rotation;
   std::map<int,GLUI_RadioGroup*>      _radgroup;
   std::map<int,GLUI_RadioButton*>     _radbutton;
   std::map<int,GLUI_Checkbox*>        _checkbox;
};

#endif // _BASE_UI_H_IS_INCLUDED_

/* end of file base_ui.H */
