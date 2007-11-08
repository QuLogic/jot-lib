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
/**********************************************************************
 * lstrip.C:
 **********************************************************************/

#include "mesh/lstrip.H"
#include "mesh/lmesh.H"

void
Lstrip::delete_substrips()
{
   delete _right_substrip;
   delete _left_substrip;
   _right_substrip = _left_substrip = 0;
}

void
Lstrip::reset()
{
   delete_substrips();
   TriStrip::reset();
}

int
Lstrip::cur_level() const
{
   return _verts.empty() ? 0 : ((Lvert*)_verts[0])->lmesh()->cur_level();
}

void
Lstrip::draw(int level, StripCB* cb)
{
   if (level < 1)
      TriStrip::draw(cb);
   else {
      generate_substrips();
      _right_substrip->draw(level-1, cb);
      _left_substrip-> draw(level-1, cb);
   }
}

void
Lstrip::add(Bvert* v)
{
   // this is a protected method of Lstrip.
   // it exists as a convenience, used in
   // Lstrip::build_substrip1() and
   // Lstrip::build_substrip2() (see below).
   // assumes the whole strip is built from
   // the sequence of verts -- corresponding
   // faces need to be looked up.

   if (v == 0) {
      err_msg("Lstrip::add: error: vertex is null");
      return;
   }

   _verts += v;

   int n = _verts.num();

   if (n < 3)
      return;   // just starting -- no face lookup yet

   // add face 3 times if this is the 3rd vert.
   // once otherwise
   Bface* f = lookup_face(_verts[n-3], _verts[n-2], v);
   if (n == 3) {
      _faces += f;
      _faces += f;
   }
   _faces += f;
}


void
Lstrip::build_substrip1(Lstrip* substrip)
{
   substrip->add(subvert(0));
   substrip->add(subvert(0,1));

   for (int k=2; k<_verts.num(); k++) {
      if (k%2) {
         substrip->add(subvert(k,k-1));
      } else {
         substrip->add(subvert(k,k-2));
         substrip->add(subvert(k,k-1));
         substrip->add(subvert(k));
      }
   }
}

void
Lstrip::build_substrip2(Lstrip* substrip)
{
   substrip->add(subvert(1,0));
   substrip->add(subvert(1));

   for (int k=2; k<_verts.num(); k++) {
      if (k%2) {
         substrip->add(subvert(k,k-2));
         substrip->add(subvert(k,k-1));
         substrip->add(subvert(k));
      } else {
         substrip->add(subvert(k,k-1));
      }
   }
}

void
Lstrip::generate_substrips()
{
   if (!_left_substrip) {
      _left_substrip  = new Lstrip(_orientation);
      _right_substrip = new Lstrip(_orientation);

      if (_orientation == 0) {
         build_substrip1(_left_substrip);
         build_substrip2(_right_substrip);
      } else {
         build_substrip2(_left_substrip);
         build_substrip1(_right_substrip);
      } 
   }
}

/* end of file lstrip.C */
