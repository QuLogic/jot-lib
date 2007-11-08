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
#ifndef MOVE_MENU_DEF
#define MOVE_MENU_DEF

#include "std/support.H"
#include "mlib/points.H"

class MoveMenu;
class MenuItem {
  public :
     MenuItem() : _menu(0) {}
     MenuItem(Cstr_ptr &label) : _label(label) { }
     virtual ~MenuItem() {}

     virtual void exec(mlib::CXYpt &) {}
     Cstr_ptr &label() const { return _label; }

     void            menu(MoveMenu *menu) {_menu = menu;}
     const MoveMenu *menu()        const  {return _menu;}

  protected:
     str_ptr   _label;
     MoveMenu *_menu;
};

typedef ARRAY<MenuItem *> MenuList;

class MoveMenu
{
  public  :
     MoveMenu(Cstr_ptr &name) : _is_shown(0), _name(name) {}
     virtual ~MoveMenu() {}

     virtual void move(const mlib::XYpt &loc) {_loc = loc;}
     virtual void move_local(const mlib::XYpt &loc) = 0;
     virtual void hide() = 0;
     virtual void menu(int recreate = false) = 0;
     virtual void show() = 0;
     MenuList &items() {return _item_list;}
     int is_shown()    {return _is_shown;}

  protected:
     int      _is_shown;
     MenuList _item_list;
     mlib::XYpt  _loc;
     str_ptr  _name;
};
#endif
