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

#include "disp/view.hpp"
#include "geom/winsys.hpp"
#include <GL/glew.h>
#include "glui/glui_jot.hpp"
#include "std/support.hpp"
#include "std/file.hpp"
#include "base_ui.hpp"


BaseUI::BaseUI(string n)
   : _name(n),
     _glui(nullptr)
{}

BaseUI::BaseUI(BaseUI* parent, string n)
   : _name(n),
     _glui(nullptr),
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

   _glui = GLUI_Master.create_glui(_name.c_str(), 0, root_x + root_w + 10, root_y);
   _glui->set_main_gfx_window(VIEW::peek()->win()->id());
      
   build(_glui, nullptr, true);

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
   _glui = nullptr;
}

void 
BaseUI::fill_listbox(GLUI_Listbox* listbox, const vector<string>& list)
{
   if(!listbox)
      return;

   //clear the listbox
   int i=0;
   while (listbox->delete_item(i++));

   for (vector<string>::size_type i=0; i < list.size(); ++i) {
      listbox->add_item(i, list[i].c_str());
   }
}

void 
BaseUI::fill_directory_listbox(GLUI_Listbox* listbox,
                               vector<string> &save_files,
                               const string &full_path,
                               const string &extension,
                               bool         hide_extension,
                               bool         put_default,
                               const string default_text)
{
   //cerr << "Does this BaseUI::fill_directory_listbox work? not for me....I'm using the one without save_files..." << endl;

   int current_count = 0;
   //First clear out any previous presets
   vector<string>::size_type i=0;
   for (i=1; i<=save_files.size(); i++) {
      listbox->delete_item(i);
   }  
   save_files.clear();

   
   //inserts a -default- choice in the listbox
   //it will have the int value zero
   //this is usefull for setting the procedural mode in halftone shader
   if (put_default) {
      string default_name = default_text;
      
      save_files.push_back(default_name);
      listbox->add_item(current_count++, default_name.c_str());
   }

   string::size_type extension_length = extension.length();
   vector<string> in_files = dir_list(full_path);
   for (i = 0; i < in_files.size(); i++) {
      string::size_type len = in_files[i].length();

      if ((len>3) && (in_files[i].substr(len-extension_length) == extension))
      {
         string basename;
         if (hide_extension)
            basename = in_files[i].substr(0, len-extension_length);
         else
            basename = in_files[i];

         save_files.push_back(in_files[i]);
         listbox->add_item(current_count++, basename.c_str());
      }
      else if (in_files[i] != "CVS")
      {
         err_msg("BaseUI::fill_directory_listbox - Discarding preset file (bad name): %s", in_files[i].c_str());
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
