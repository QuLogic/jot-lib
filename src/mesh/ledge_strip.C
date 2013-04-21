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
 * ledge_strip.C:
 **********************************************************************/
#include "mesh/ledge_strip.H"
#include "mesh/lmesh.H"

int
LedgeStrip::cur_level() const
{
   LMESH* m = lmesh();
   return m ? m->cur_level() : 0;
}

int
LedgeStrip::rel_cur_level() const
{
   LMESH* m = lmesh();
   return m ? m->rel_cur_level() : 0;
}

bool
LedgeStrip::need_rebuild() const
{
   if (!_substrip)
      return false;
   return num()*2 != _substrip->num();
}

void
LedgeStrip::draw(int level, StripCB* cb)
{
   if (level < 0)
      return;
   else if (level == 0)
      EdgeStrip::draw(cb);      // draw this one
   else {
      generate_substrip();      // draw the substrip (or lower)
      _substrip->draw(level-1, cb);
   }
}

// Filter for checking which edges can be added to a substrip:
class EdgeStripFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      if (!is_edge(s))
         return false;
      Bedge* e = (Bedge*)s;
      return (BMESH::show_secondary_faces() ? true :
              e->is_polyline() ? !e->is_multi() :
              e->is_primary());
   }
};

void
LedgeStrip::generate_substrip()
{
   // if the substrip is out of date, delete it and rebuild it
   if (need_rebuild())
      delete_substrip();

   // the convention is: if the substrip is allocated,
   // it's also filled in w/ data
   if (_substrip)
      return;   // all set

   // allocate it ...
   _substrip  = new LedgeStrip;

   // ... and fill it in
   EdgeStripFilter f;
   for (int i=0; i<_verts.num(); i++) {
      Bvert *first  = ((Lvert*)_verts[i])->subdiv_vertex();
      Bvert *middle = ((Ledge*)_edges[i])->subdiv_vertex();
      Bvert *last   = ((Lvert*)next_vert(i))->subdiv_vertex();

      Bedge* e = lookup_edge(first,middle);
      if (f.accept(e))
        _substrip->add(first, e );
      e = lookup_edge(middle, last);
      if (f.accept(e))
        _substrip->add(middle, e);
   }
}

void 
LedgeStrip::clear_subdivision(int level) 
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

/* end of file ledge_strip.C */
