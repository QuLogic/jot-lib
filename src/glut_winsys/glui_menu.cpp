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
#include "glui_menu.H" 
#include "glui/glui_jot.H"

vector<MenuItem *> GLUIMoveMenu::_menu_items(10);

GLUIMoveMenu::GLUIMoveMenu(const string &name, int main_window_id) :
   MoveMenu(name),
   _glui(nullptr),
   _menu_created(false),
   _main_window_id(main_window_id),
   _id(-1),
   _item_ids()
{
}


void
GLUIMoveMenu::show()
{
   if (!_glui) return;

   int old_id = glutGetWindow();

   glutSetWindow(_main_window_id);
   int root_x = glutGet(GLUT_WINDOW_X);
   int root_y = glutGet(GLUT_WINDOW_Y);
   int root_w = glutGet(GLUT_WINDOW_WIDTH);
   //  int root_h = glutGet(GLUT_WINDOW_HEIGHT);

   glutSetWindow(_id);
   glutPositionWindow(root_x + root_w + 10, root_y);

    glutSetWindow(old_id);

   if(!_is_shown) 
   {
      _glui->show();
      _is_shown = 1;
   }
}

//
// Show menu - if recreate is true, recreate the menu
//
void
GLUIMoveMenu::menu(int recreate)
{
   if (recreate || !_menu_created) create_menu();
}

void
GLUIMoveMenu::hide()
{
   if (!_glui)
      return;
   
   if (_is_shown) {
      _glui->hide();
      _is_shown = 0;
   }
}


void
GLUIMoveMenu::create_menu()
{       

   // XXX -- Need to call _glui->close() to destroy the menu before
   // recreating it. I don't know of any other way to recreate the menu
   // without creating a new glui -- this is awkward!!

   if(_glui) {
      _glui->close();
      _glui = nullptr;
   }

   // If we're recreating the menu, must unmap previously mapped item ids
   for (auto & item : _item_ids) {
      unmap_menu_item(item);
   }
   
   _item_ids.clear();

   // get position of main window for relative placement of menu
   glutSetWindow(_main_window_id);
   int root_x = glutGet(GLUT_WINDOW_X);
   int root_y = glutGet(GLUT_WINDOW_Y);
   int root_w = glutGet(GLUT_WINDOW_WIDTH);
//   int root_h = glutGet(GLUT_WINDOW_HEIGHT);
   
   // create the glui window
   _glui = GLUI_Master.create_glui(_name.c_str(), 0, root_x + root_w + 10, root_y);
   assert(_glui);
   
   // tell glui which is the main window
   _glui->set_main_gfx_window(_main_window_id);

   _id = _glui->get_glut_window_id();

   // create a button for every menu item
   if (!_item_list.empty()) {
      for (auto & item : _item_list) {
         // Tell the menu item which menu it belongs to
         item->menu(this);
         const char *label = nullptr;
         // Make default label if one doesn't exist
         if (item->label().empty()) {
            label = "----";
         } else label = item->label().c_str();

         // set the menu item in the global list used for callbacks
         int item_id = map_menu_item(item);
         // record this menu's item id's
         _item_ids.push_back(item_id);

         // create the button
         new GLUI_Button(_glui, label, item_id, GLUIMoveMenu::btn_callback);
      }
   }
   _menu_created = true;
   _is_shown = true;
}


void
GLUIMoveMenu::btn_callback(int id)
{
   using mlib::XYpt;
   assert(0 <= id && id < (int)_menu_items.size());
   assert(_menu_items[id]);
   XYpt dummy;
   _menu_items[id]->exec(dummy);
}


// Assigns a unique id to a menu item, which can be used to access the
// item from a static array (_menu_item_map) in a static callback
// functions.  Finds an empty slot in the _menu_item_map, growing the
// array, if necessary, assigns the 'item' pointer to that slot, and
// returns the slot's index, to be used as the item's unique id.

int
GLUIMoveMenu::map_menu_item(MenuItem *item)
{
   assert(item);

   // See if there's an empty slot in the menu item array.
   for (vector<MenuItem *>::size_type i=0; i<_menu_items.size(); i++) {
      if (_menu_items[i] == nullptr) {    // found an empty slot
         cerr << "map_menu_item(), empty slot " << i << endl;
         _menu_items[i] = item; // store item 
         return i;              // return the index as the item's id
      }
   }

   // we found no empty slot, so append the item to the array
   _menu_items.push_back(item);

   // return its index
   int ret_id = _menu_items.size()-1;

   return ret_id;
}



// Clears the entry for the menu item a the give index.
void
GLUIMoveMenu::unmap_menu_item(int item_index)
{
   assert(0 <= item_index && item_index < (int)_menu_items.size());
   _menu_items[item_index] = nullptr;
}

