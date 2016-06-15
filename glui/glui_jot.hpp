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
 * along with jot-lib.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************/

#ifndef GLUI_JOT_H
#define GLUI_JOT_H

#include <GL/glui.h>
#include "glui_custom_controls.hpp"
#include <cassert>

/********************************* jot_check_glui_item_fit() **************/
bool inline jot_check_glui_fit(GLUI_Control *c, const char *s)
{
   GLUI_Listbox *lb;
   GLUI_ActiveText *at;
   GLUI_StaticText *st;
   if ((lb = dynamic_cast<GLUI_Listbox*>(c))) {
      if ( lb->w < lb->text_x_offset + lb->string_width(s) + 20 )
         return false;
      else
         return true;

   } else if ((st = dynamic_cast<GLUI_StaticText*>(c))) {
      if ( st->w < st->string_width(s) )
         return false;
      else
         return true;

   } else if ((at = dynamic_cast<GLUI_ActiveText*>(c))) {
      return at->check_fit(s);

   } else {
      assert(0);
   }
}

#endif // GLUI_JOT_H

