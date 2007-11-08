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
 * bsimplex.C
 **********************************************************************/
#include "mesh/bsimplex.H"
#include "mesh/simplex_filter.H"
#include "mesh/simplex_array.H"

Bsimplex::IDtable Bsimplex::_table(1<<14);

Bsimplex::~Bsimplex()
{
   if (_data_list) {
      _data_list->notify_simplex_deleted();
      delete _data_list;
      _data_list = 0;
   }
}

uint 
Bsimplex::generate_key() 
{
   // called once to generate the "key" for this simplex 
   // (first time _key is accessed)

   // XXX - change back to 1<<24 after verifying highest bit is no
   // longer being used in some hacky code somewhere like it was at
   // one point
   if (_table.num() >= ((1<<23) - 1)) {
      // can't allocate > 8 million IDs
      err_msg("Bsimplex::generate_key: error: key table is full");
   } else {
      _key = _table.num();
      _table += this;
   }
   return _key;
}

void 
Bsimplex::notify_split(Bsimplex *new_simp)
{
   // the simplex has split, introducing one or more new
   // simplices. this method, called once for each newly created
   // simplex, notifies simplex data of the original simplex
   // (i.e. this one) about the new simplex. that way the data or its
   // owner can decide to put some relevant data onto the new simplex
   // if that's appropriate.
   if (_data_list)
      _data_list->notify_split(new_simp);
}

void
Bsimplex::notify_xform(CWtransf& xf)
{
   // a transform was applied to the vertices ... pass it on.
   if (_data_list)
      _data_list->notify_simplex_xformed(xf);
}

void
Bsimplex::geometry_changed()
{
   // This is called for the following reasons.
   //
   //   For a vertex: It moved.
   //
   //   For an edge or face:
   //      Its shape changed. I.e. one of its vertices moved.

   // Notify associated data in case any of them care:
   if (_data_list)
      _data_list->notify_simplex_changed();
}

void
Bsimplex::normal_changed()
{
   // This is called for the following reasons.
   //
   //   For a face: One of its vertices moved.
   //
   //   For an edge or vertex:
   //      An adjacent face changed shape, or was added or removed.

   // Notify associated data in case any of them care:
   if (_data_list)
      _data_list->notify_normal_changed();
}

void 
Bsimplex::add_simplex_data(SimplexData* sd) 
{
   // Quietly ignore NULL pointers:
   if (!sd)
      return;

   // React badly if an item with the same key already exists.
   // But cut a little slack if it's actually the same item:
   SimplexData* cur = find_data(sd->id());
   if (cur) {
      if (cur == sd) {
         cerr << "Bsimplex::add_simplex_data: Warning: "
              << "attempt to add data twice -- ignored"
              << endl;
         return;
      } else assert(0);
   }

   // Create the data list if needed:
   if (!_data_list) _data_list = new SimplexDataList(); assert(_data_list);

   // Now go ahead:
   _data_list->add(sd);
}

Bsimplex* 
Bsimplex::walk_to_target(CWpt& target, const SimplexFilter& f) const
{
   assert(f.accept(this));

   // overview: do a local search over the mesh to get as
   // close as possible to the given target point. return
   // the simplex that is locally closest to the target.
   // however the mesh walk is restricted to traversing
   // simplices accepted by the given filter.

   // first find the closest point on this simplex, and find the
   // corresponding "closest" simplex contained in this one.
   // e.g. if this is a face and the closest point is on the
   // boundary of the face, then it is on an edge or vertex; if
   // this is an edge, "closest" could be one of the vertices at
   // its endpoints. or it could be this simplex itself if the
   // closest point is not on the boundary.
   Wvec bc;
   double    min_dist = dist(target,bc);
   Bsimplex*  closest = bc2sim(bc);

   // note: we can switch to a lower-dimension simplex
   // without getting closer to the target, but if we go to
   // a higher dimesion (e.g. edge to face) we require that
   // the distance to target decreases by some nonzero amount.

   if (closest != this && f.accept(closest))
      return closest->walk_to_target(target, f); // switch to lower-dim simplex
   closest = (Bsimplex*)this;

   // can any neighbors get closer?
   Bsimplex_list nbrs = neighbors().filter(f);
   for (int i=0; i<nbrs.num(); i++) {
      double d = nbrs[i]->dist(target);
      if (d < min_dist - epsAbsMath()) {         // get closer by nonzero amount
         min_dist = d;
         closest = nbrs[i];
      }
   }

   // return this if it is closest;
   // otherwise recurse to the neighbor that was closest:
   return (closest == this) ? closest : closest->walk_to_target(target, f);
}

// end of file bsimplex.C
