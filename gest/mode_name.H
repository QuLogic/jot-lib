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
#ifndef MODE_NAME_IS_INCLUDED
#define MODE_NAME_IS_INCLUDED

/*!
 *  \file mode_name.H
 *  \brief Contains the definition of the ModeName class.
 *
 *  \sa mode_name.C
 *
 */

#include "std/support.H"
#include "geom/text2d.H"

/*!
 *  \brief Simple interface for setting the name of the current "mode"
 *  which is displayed in the top left corner of the jot window.
 *
 *  \todo Rewrite as singleton class.
 *
 */
class ModeName {
   
   public:
   
      //! \brief Name currently displayed.
      static str_ptr get_name() { return name()->get_string(); }
      
      //! \brief set a new name.
      static void push_name(Cstr_ptr& n);
      
      //! \brief Remove the last name set, restore the one before it.
      static void pop_name();

   protected:
   
      static TEXT2Dptr     _mode_name;
      static LIST<str_ptr> _names;
      
      static void init();
      static TEXT2Dptr name()              { init(); return _mode_name; }
      static void set_name(Cstr_ptr& n)    { name()->set_string(n); }

};

#endif // MODE_NAME_IS_INCLUDED

// end of file mode_name.H
