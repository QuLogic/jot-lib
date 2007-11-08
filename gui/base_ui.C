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
/**********************************************************************
 * base_ui.C:
 **********************************************************************/

#include "disp/view.H"
#include "geom/winsys.H"
#include "glew/glew.H"
#include "glui/glui.h" 
#include "std/support.H"
#include "base_ui.H"


BaseUI::BaseUI(str_ptr n)
   : _name(n),
     _glui(0)
{}

BaseUI::BaseUI(BaseUI* parent, str_ptr n)
   : _name(n),
     _glui(0),
     _parent(parent)
{}

void     
BaseUI::show()
{
   if (!_glui) {   
      build();     

      if (_glui) {
         _glui->show();        
         update();             
      } else {
         err_msg("BaseUI::show() - Error building the menu!");        
      }              
   } 
}

void
BaseUI::hide()
{
   if (!_glui) {
      err_msg("BaseUI::hide() - Error! BaseUI is already hidden!");
   } else {
      _glui->hide();
      destroy();
      assert(!_glui);
   }
}

void
BaseUI::update()
{
   if(!_glui) {
      err_msg("BaseUI::update() - Error! ");      
   } else {      
      update_non_lives();     
      _glui->sync_live();
      _glui->refresh();
   }
}

void
BaseUI::build()
{
   assert(!_glui);

   int root_x, root_y, root_w, root_h;
   VIEW::peek()->win()->size(root_w,root_h);
   VIEW::peek()->win()->position(root_x,root_y);

   _glui = GLUI_Master.create_glui(**_name, 0, root_x + root_w + 10, root_y);
   _glui->set_main_gfx_window(VIEW::peek()->win()->id());
      
   build(_glui, NULL, true);

   //Show will actually show it...
   _glui->hide();

}
 
void
BaseUI::destroy()
{
   _listbox.clear();
   _button.clear();
   _slider.clear();
   _spinner.clear();
   _statictext.clear();
   _edittext.clear();
   _panel.clear(); 
   _rollout.clear();
   _rotation.clear();
   _radgroup.clear();
   _radbutton.clear();
   _checkbox.clear();
   
   //Recursively kills off all controls, and itself
   _glui->close();
   _glui = NULL;
}

void 
BaseUI::fill_listbox(GLUI_Listbox* listbox, Cstr_list& list)
{
   if(!listbox)
      return;

   //clear the listbox
   int i=0;
   while (listbox->delete_item(i++));
        
   for(i=0; i < list.num(); ++i)
   {
      listbox->add_item(i, **(list[i]));  
   }
}

void 
BaseUI::fill_directory_listbox(GLUI_Listbox* listbox,
                               str_list     &save_files,
                               Cstr_ptr     &full_path,
                               Cstr_ptr     &extension,
                               bool         hide_extension,
                               bool         put_default,
                               Cstr_ptr     default_text)
{
   //cerr << "Does this BaseUI::fill_directory_listbox work? not for me....I'm using the one without save_files..." << endl;

   int current_count = 0;
   //First clear out any previous presets
   int i=0;
   for (i=1; i<=save_files.num();i++)
   {
      listbox->delete_item(i);
   }  
   save_files.clear();

   
   //inserts a -default- choice in the listbox
   //it will have the int value zero
   //this is usefull for setting the procedural mode in halftone shader
   if(put_default)
   {
      str_ptr default_name = default_text;
      
      save_files+=default_name;
      listbox->add_item(current_count++, **default_name);

   }

   
   int extention_lenght = extension.len();
   str_list in_files = dir_list(full_path);
   for (i = 0; i < in_files.num(); i++) {
      int len = in_files[i].len();

      if ((len>3)  && (strcmp(&(**in_files[i])[len-extention_lenght],**extension) == 0))
      {

         char *basename = new char[len+1];       
         assert(basename);
         strcpy(basename,**in_files[i]);
         basename[(hide_extension) ? len-extention_lenght : len] = 0;

         save_files += in_files[i];
         listbox->add_item(current_count++, basename);
        
         delete [] basename;
      }
      else if (in_files[i] != "CVS")
      {
         err_msg("BaseUI::fill_directory_listbox - Discarding preset file (bad name): %s", **in_files[i]);         
      }
   }
}


/////////////////////////////////////
// is_vis()
/////////////////////////////////////

bool
BaseUI::is_vis()
{
   if (_glui) {
      return true;
   } else {
      return false;
   }
}
