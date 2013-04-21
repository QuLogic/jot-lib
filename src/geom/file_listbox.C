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
#include "disp/view.H"
#include "geom/winsys.H"
#include "glui/glui.h"

#include "file_listbox.H"

ARRAY<FileListbox*> FileListbox::_file_list_boxes;

FileListbox::FileListbox(
   Cstr_ptr &label,
   Cstr_ptr &listbox_name,
   Cstr_ptr &button_name
   ) : _glui(0), _listbox(0), _selected(0), _shown(false)
{
  _file_list_boxes += this;
  init(label, listbox_name, button_name);
}

void
FileListbox::init(
   Cstr_ptr &label,
   Cstr_ptr &listbox_name,
   Cstr_ptr &button_name
   )
{
   // Get the glut main window ID from the winsys
   int main_win_id = VIEW::peek()->win()->id();

   // Create the glui widget that will contain the pen controls
   _glui = GLUI_Master.create_glui(**listbox_name, 0);
   _glui->set_main_gfx_window(main_win_id);

   // Create label
   _glui->add_statictext(**label); 
   // Followed by a separator
   _glui->add_separator();

   // Create the texture list box
   _listbox = _glui->add_listbox(**listbox_name,
                                 NULL, // not using live var
                                 _file_list_boxes.num()-1,
                                 FileListbox::listbox_cb
                                 // registering the callback func
                                 );
   assert(_listbox);

   // Another separator
   _glui->add_separator();

   // Add a button for setting the attributes to the stroke
   _glui->add_button(**button_name, _file_list_boxes.num()-1,
                     FileListbox::set_cb);

   // Add a button for setting the attributes to the stroke
   _glui->add_button("Hide", _file_list_boxes.num()-1,
                     FileListbox::hide_cb);

   _glui->hide();
}

void
FileListbox::fill_listbox(
   GLUI_Listbox *listbox,
   str_list     &save_files,
   Cstr_ptr     &full_path,
   Cstr_ptr     &save_path,
   const char   *extension
   )
{
   int j = 0;
   str_list in_files = dir_list(full_path);
   for (int i = 0; i < in_files.num(); i++) 
   {
      int len = in_files[i].len();
      if ( (extension) && (len>3) && 
            (strcmp(&(**in_files[i])[len-4],extension) == 0))
      {
         save_files += save_path + in_files[i];
         listbox->add_item(j++, **in_files[i]);
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
