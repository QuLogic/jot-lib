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
#ifndef GLUI_MOVE_MENU_DEF
#define GLUI_MOVE_MENU_DEF

#include "widgets/menu.H"
#include "std/support.H"

class GLUI;
class GLUIMoveMenu : public MoveMenu {
  public:
   GLUIMoveMenu(Cstr_ptr &name, int main_window_id);

   // XXX -- move local should reposition the glui window.
   virtual void move_local(const mlib::XYpt &loc) {_loc = loc;}
   virtual void hide();
   virtual void menu(int recreate = false);
   virtual void show();

   static void btn_callback    (int id);

   void create_menu();

 protected:

   // Global list of menu items contained by any currently instantiated
   // menu.  Used for making callbacks to menu items.
   static ARRAY<MenuItem *> _menu_items;

   int map_menu_item(MenuItem *item);
   void unmap_menu_item(int item_index);

   GLUI *      _glui;
   bool        _menu_created;
   int         _main_window_id;    // id of parent window
   int         _id;                // id of menu window
   ARRAY<int>  _item_ids;    // XXX -- Hack:  keep track of this menu's mapped
                             // item id's (used for unmapping items when the
                             // menu is recreated).       
};
#endif

/* end of file glui_menu.H */
