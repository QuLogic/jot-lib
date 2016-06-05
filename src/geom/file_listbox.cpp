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

#include "std/support.H"
#include "std/file.H"
#include "disp/view.H"
#include "geom/winsys.H"
#include "glui/glui_jot.H"

#include "file_listbox.H"

vector<FileListbox*> FileListbox::_file_list_boxes;

FileListbox::FileListbox(
   const string &label,
   const string &listbox_name,
   const string &button_name
   ) : _glui(nullptr), _listbox(nullptr), _selected(0), _shown(false)
{
  _file_list_boxes.push_back(this);
  init(label, listbox_name, button_name);
}

void
FileListbox::init(
   const string &label,
   const string &listbox_name,
   const string &button_name
   )
{
   // Get the glut main window ID from the winsys
   int main_win_id = VIEW::peek()->win()->id();

   // Create the glui widget that will contain the pen controls
   _glui = GLUI_Master.create_glui(listbox_name.c_str(), 0);
   _glui->set_main_gfx_window(main_win_id);

   // Create label
   new GLUI_StaticText(_glui, label.c_str());
   // Followed by a separator
   new GLUI_Separator(_glui);

   // Create the texture list box
   _listbox = new GLUI_Listbox(_glui, listbox_name.c_str(),
                               nullptr, // not using live var
                               _file_list_boxes.size()-1,
                               FileListbox::listbox_cb
                               // registering the callback func
                               );
   assert(_listbox);

   // Another separator
   new GLUI_Separator(_glui);

   // Add a button for setting the attributes to the stroke
   new GLUI_Button(_glui, button_name.c_str(), _file_list_boxes.size()-1,
                   FileListbox::set_cb);

   // Add a button for setting the attributes to the stroke
   new GLUI_Button(_glui, "Hide", _file_list_boxes.size()-1,
                   FileListbox::hide_cb);

   _glui->hide();
}

void
FileListbox::fill_listbox(
   GLUI_Listbox   *listbox,
   vector<string> &save_files,
   const string   &full_path,
   const string   &save_path,
   const char     *extension
   )
{
   int j = 0;
   vector<string> in_files = dir_list(full_path);
   for (auto & in_file : in_files)
   {
      string::size_type len = in_file.length();
      if ( (extension) && (len>3) && 
            (in_file.substr(len-4) == extension))
      {
         save_files.push_back(save_path + in_file);
         listbox->add_item(j++, in_file.c_str());
      }
   }
}

void
FileListbox::show()
{
   if (_glui) {
      _glui->show();
      _shown = true;
   }
}


void
FileListbox::hide()
{
   if (_glui)  {
      _glui->hide();
      _shown = false;
   }
}

void
FileListbox::toggle()
{
   if (_shown) _glui->hide();
   else        _glui->show();
}

void
FileListbox::listbox_cb(int id)
{
   int item_id = _file_list_boxes[id]->_listbox->get_int_val();
   _file_list_boxes[id]->_selected = item_id;
}

void
FileListbox::set_cb(int id)
{
   _file_list_boxes[id]->button_press_cb();
}


void
FileListbox::hide_cb(int id)
{
   _file_list_boxes[id]->hide();
}
