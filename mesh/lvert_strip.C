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
 * lvert_strip.C:
 **********************************************************************/


#include "lvert_strip.H"
#include "lmesh.H"

int
LvertStrip::cur_level() const
{
  if ( _verts.empty() )
    return 0;

   return  lv(0)->lmesh()->rel_cur_level();
}

void
LvertStrip::draw(int level, StripCB* cb)
{
   if (level < 1)
      VertStrip::draw(cb);      // draw this one
   else {
      generate_substrip();      // draw the substrip (or lower)
      _substrip->draw(level-1, cb);
   }
}

void
LvertStrip::generate_substrip()
{
   // the convention is: if the substrip is allocated,
   // it's also filled in w/ data
   if (_substrip)
      return;   // all set

   // allocate it ...
   _substrip  = new LvertStrip;

   // ... and fill it in
   for (int i=0; i<_verts.num(); i++) {
     if (((Lvert*)_verts[i])->subdiv_vertex()) {
      _substrip->add(((Lvert*)_verts[i])->subdiv_vertex());
     }
   }
}

void 
LvertStrip::clear_subdivision(int level) 
{
   // Subdivision elements at the given level have been deleted.
   // The strip one level above should delete its substrip.
   // If level == 0 the strip is invalidated.

   if (level <= 0) {
      delete_substrip();
      reset();
   } else if (level == 1) {
      delete_substrip();
   } else if (_substrip) {
      _substrip->clear_subdivision(level - 1);
   }
}

/* end of file lvert_strip.C */
