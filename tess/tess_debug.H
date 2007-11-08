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
#ifndef INCLUDE_TESS_DEBUG_H
#define INCLUDE_TESS_DEBUG_H

#include "geom/world.H"
#include "mesh/bvert.H"

inline void
show_vert(CBvert* v, int size, CCOLOR& c)
{
   if (!v) {
      err_msg("show_vert: error: vert is null");
      return;
   }
   WORLD::show(v->loc(), size, c);
}

inline void
show_verts(CBvert_list& verts, int size, CCOLOR& c0, CCOLOR& c1)
{
   if (verts.empty())
      return;
   if (verts.num() == 1)
      show_vert(verts[0], size, c0);

   // ramp the colors
   double di = 1.0/(verts.num()-1);
   for (int i=0; i<verts.num(); i++)
      show_vert(verts[i], size, interp(c0, c1, i*di));
}

#endif // INCLUDE_TESS_DEBUG_H
