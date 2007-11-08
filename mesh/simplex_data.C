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
/*****************************************************************
 * simplex_data.C
 *****************************************************************/
#include "bsimplex.H"

void
SimplexData::set(uint id, Bsimplex* s)
{
   // If we're switching simplices, get out of the old one:
   if (_simplex)
      _simplex->rem_simplex_data(this);

   _id      = id;
   _simplex = s;

   if (_simplex)
      _simplex->add_simplex_data(this);
}

SimplexData::~SimplexData()
{
   // Get out of the data list on the simplex:
   set(0,0);
}

SimplexDataList::~SimplexDataList() 
{
   // do nothing at this time
}

/* end of file simplex_data.C */
