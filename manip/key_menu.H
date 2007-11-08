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
#ifndef KEY_MENU_H_IS_INCLUDED
#define KEY_MENU_H_IS_INCLUDED

/*!
 *  \file key_menu.H
 *  \brief Contains the definition of the KeyMenu class.
 *
 *  \sa key_menu.C
 *
 */

#include <iostream>
#include <vector>
#include <string>

#include "geom/fsa.H"
#include "geom/geom.H"

struct KeyMenuItem;

/*!
 *  \brief An Interactor that provides a menu of operations that can be
 *  triggered by pressing a single key on the keyboard.
 *
 *  Each key/operation pair also has a corresponding description so that the
 *  menu can be displayed to a standard output stream.
 *
 */
class KeyMenu : public Interactor<KeyMenu, Event, State> {
   
   public:
   
      typedef int (*key_callback_t)(const Event&, State *&);
   
      KeyMenu(State *start_in);
      
      //! \brief Add a single key/operation pair to the menu.
      void add_menu_item(char key, const std::string &desc, key_callback_t cb);
      //! \brief Add multiple keys all with the same operation and description.
      void add_menu_item(const char *keys, const std::string &desc, key_callback_t cb);
      
      //! \brief Remove a single key/operation pair from the menu.
      void remove_menu_item(char key);
      //! \brief Remove multiple key/operation pairs from the menu.
      void remove_menu_item(const char *keys);
      
      //! \brief Get the description for the supplied key.
      std::string get_item_desc(char key);
      
      //! \brief Display the menu to the supplied output stream.
      void display_menu(std::ostream &out);
   
   private:
   
      State *start;
      State button_down;
      
      std::vector<KeyMenuItem> menu_items;

};

#endif // KEY_MENU_H_IS_INCLUDED
