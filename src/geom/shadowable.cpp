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
#include "world.H"
#include "shadowable.H"
#include "shadow.H"

using mlib::Wplane;

void 
Shadowable::show_shadow()
{ 
   WORLD::display(Shadowptr(_shadow), false); 
}

void 
Shadowable::hide_shadow() 
{ 
   WORLD::undisplay(Shadowptr(_shadow), false); 
}

void 
Shadowable::init_shadow(const Wplane &pln)
{
   _shadow = create_shadow(pln);
   get_geom()->add_xf_input(_shadow);
   cast_shadow();
   show_shadow();
}

