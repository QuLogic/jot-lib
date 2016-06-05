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
#ifndef _FILE_LISTBOX_H_IS_INCLUDED_
#define _FILE_LISTBOX_H_IS_INCLUDED_

#include "std/support.H"

#include <vector>

/*****************************************************************
 * FileListbox
 *****************************************************************/
class GLUI;
class GLUI_Listbox;
class FileListbox {
 public:
   // Callbacks to application provided to GLUI widgets
   static void listbox_cb(int id);
   static void set_cb    (int id);
   static void hide_cb   (int id);

 protected:
   GLUI*              _glui;   

   GLUI_Listbox*      _listbox;
   int                _selected;
   bool               _shown;
   vector<string>     _files;

   static vector<FileListbox *> _file_list_boxes;

   //******** MEMBERS ********
   virtual void button_press_cb() = 0;

 public:
   //******** MANAGERS ********
   FileListbox(const string &label, const string &listbox_name, const string &button_name);

   virtual ~FileListbox() {}

   void toggle();
   void show();
   void hide();

   // If extention is provided, it should be like ".png".  It must
   // be four characters long, and include the dot!  Files lacking
   // this extension are ignored.
   static void fill_listbox(GLUI_Listbox *listbox, vector<string> &files,
                            const string &full_path, const string &save_path,
                            const char *extension=nullptr);

 protected:

   void init(const string &label, const string &listbox_name, const string &button_name);
};

#endif // _FILE_LISTBOX_H_IS_INCLUDED_

/* end of file paper_ui.H */
