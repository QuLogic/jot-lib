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
#ifndef _PRESETS_UI_H_IS_INCLUDED_
#define _PRESETS_UI_H_IS_INCLUDED_

////////////////////////////////////////////
// PresetsUI
////////////////////////////////////////////

#include "gui/base_ui.H"

#include <vector>

/*****************************************************************
 * PatchUI
 *****************************************************************/
class PresetsUI : public BaseUI {

 public:
   /******** WIDGET ID NUMBERS ********/
   enum preset_stages_t{
      PRESET_SELECTED=0,      
      PRESET_SAVE,
      PRESET_SAVE_NEW
   };
    
   enum button_id_t {
      BUT_SAVE = 0,    
      BUT_NUM 
   };   
  
   enum listbox_id_t {
      LIST_PRESET = 0,        
      RADBUT_NUM
   };
   enum edittext_id_t{
      EDITTEXT_SAVE=0,   
      EDITTEXT_NUM
   };
   enum panel_id_t {
      PANEL_PRESET=0,      
      PANEL_NUM
   };
  
   /******** STATIC MEMBERS VARS ********/
   // This is so that we can support multiple instances of this ui
   static std::vector<PresetsUI*>  _ui;       
  
 public:
   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("PresetsUI", PresetsUI*, PresetsUI, BaseUI*);

    /******** CONSTRUCTOR/DECONSTRUCTOR ********/
   PresetsUI(BaseUI* parent, str_ptr dir=0, str_ptr ext=0);
   virtual ~PresetsUI(){}

   /******** MEMBERS METHODS ********/  
   virtual void build(GLUI*, GLUI_Panel*, bool open);  
   virtual void update_non_lives();
   str_ptr get_filename() { return _filename; }    
 protected:
   
   /******** MEMBERS VARS ********/
   int                 _id;
   str_ptr             _directory;
   str_ptr             _extension;
   str_list            _preset_filenames;
   str_ptr             _filename;
 protected:     
   /******** Convenience Methods ********/
   //void          fill_preset_listbox(GLUI_Listbox *listbox,
   //                         str_list     &save_files,
   //                         Cstr_ptr     &full_path);
   void          preset_selected();
   void          preset_save_button();
   void          preset_save_text();

   //bool          save_preset(const char *f);
   //bool          load_preset(const char *f);

   /******** STATIC CALLBACK METHODS ********/
   static void  button_cb(int id);     
   static void  listbox_cb(int id);
   static void  edittext_cb(int id);

};


#endif // _PRESETS_UI_H_IS_INCLUDED_

/* end of file presets_ui.H */
