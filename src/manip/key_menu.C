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
/*!
 *  \file key_menu.C
 *  \brief Contains the implementation of the KeyMenu class.
 *
 *  \sa key_menu.H
 *
 */

#include <iostream>
#include <vector>
#include <string>

using namespace std;

#include "geom/fsa.H"

#include "key_menu.H"

/*!
 *  \brief Holds all information needed about an entry in a KeyMenu.
 *
 */
struct KeyMenuItem {
   
   KeyMenuItem(char k, string d, KeyMenu::key_callback_t cb)
      : key(k), desc(d), callback(cb) { }
   
   char key;   
   string desc;   
   KeyMenu::key_callback_t callback;
   
};

/*!
 *  \param[in] start_in The FSA State where keyboard events will originate from.
 *
 */
KeyMenu::KeyMenu(State *start_in)
   : start(start_in)
{
   if (start)
      start->set_name("KeyMenu::start");
}
      
/*!
 *  \param[in] key The character representing the key on the keyboard that will
 *  be mapped to the given operation.
 *  \param[in] desc The description of the operation that will be mapped to the
 *  supplied key.
 *  \param[in] cb A callback function that will be called when the supplied key
 *  is pressed.
 *
 *  \note If this function is called with the same key multiple times, only the
 *  most recent calling will have any effect.
 *
 */
void
KeyMenu::add_menu_item(char key, const string &desc, key_callback_t cb)
{
   
   // Remove the key if it is already in the menu:
   for(unsigned i = 0; i < menu_items.size(); ++i){
      
      if(menu_items[i].key == key){
         
         remove_menu_item(key);
         break;
         
      }
      
   }
   
   // Add the key to the menu:
   
   // Add the FSA arc:
   *start += Arc(Event(NULL, Evd(key,KEYD)), new CallFunc_t<Event>(cb, start));
   
   // Add the KeyMenuItem to the list:
   menu_items.push_back(KeyMenuItem(key, desc, cb));
   
}

/*!
 *  \param[in] keys A string of characters representing the keys on the keyboard
 *  that will be mapped to the given operation.
 *  \param[in] desc The description of the operation that will be mapped to the
 *  supplied keys.
 *  \param[in] cb A callback function that will be called when any of the
 *  supplied keys is pressed.
 *
 *  \note If the same character occurs multiple times in \p keys, it will still
 *  only be added to the menu once.
 *
 */
void
KeyMenu::add_menu_item(const char *keys, const string &desc, key_callback_t cb)
{
   
   // Loop over all characters in the string and add each one the menu:
   for(; *keys; ++keys){
      
      add_menu_item(*keys, desc, cb);
      
   }
   
}

/*!
 *  \param[in] key Character representing the key to remove from the menu.
 *
 *  \note This function does nothing if \p key is not already in the menu.
 *
 */
void
KeyMenu::remove_menu_item(char key)
{
   
   unsigned key_index = (uint)-1;
   
   // Find the key:
   for(unsigned i = 0; i < menu_items.size(); ++i){
      
      if(menu_items[i].key == key){
         
         key_index = i;
         break;
         
      }
      
   }
   
   // Do nothing if key is not found:
   if (key_index == (uint)-1)
      return;
   
   // Remove the FSA arc:
   *start -= Arc(Event(NULL, Evd(menu_items[key_index].key,KEYD)),
                 new CallFunc_t<Event>(menu_items[key_index].callback, start));
   
   // Remove the KeyMenuItem from the list:
   menu_items.erase(menu_items.begin() + key_index);
   
}

/*!
 *  \param[in] keys String of characters representing the keys to remove from
 *  the menu.
 *
 */
void
KeyMenu::remove_menu_item(const char *keys)
{
   
   // Loop over all characters in the string and remove each one from the menu:
   for(; *keys; ++keys){
      
      remove_menu_item(*keys);
      
   }
   
}

/*!
 *  \param[in] key Character representing the key to get the description for.
 *
 *  \return The description of the supplied key if it is in the menu and the
 *  empty string otherwise.
 *
 */
std::string
KeyMenu::get_item_desc(char key)
{
   
   string desc;
   
   // Find the key:
   for(unsigned i = 0; i < menu_items.size(); ++i){
      
      if(menu_items[i].key == key){
         
         desc = menu_items[i].desc;
         break;
         
      }
      
   }
   
   return desc;
   
}

/*!
 *  \param[inout] The stream to output menu listing to.
 *
 */
void
KeyMenu::display_menu(ostream &out)
{
   
   for (unsigned i = 0; i < menu_items.size(); ++i){
      
      out << menu_items[i].key << "\t" << menu_items[i].desc << endl;
      
   }
   
}
