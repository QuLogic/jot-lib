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
#include "mi.H"

Bface_list
leading_faces(CBvert* v, CSimplexFilter& filter)
{
   // return a list of faces, one face per sector in the
   // "star" around v, where sectors separated by edges
   //  accpted by the filter (or by border edges):

   Bface_list ret;
   if (!v)
      return ret;

   Bface* f = 0;
   for (int i=0; i<v->degree(); i++) {
      Bedge* e = v->e(i);
      if ((e->is_border() || filter.accept(e)) &&
          (f = e->ccw_face((Bvert*)v))) // XXX - fix Bedge::ccw_face()
         ret += f;
         
   }
   // if no edges are accepted by the filter and there are no border
   // edges, add in any old face:
   if (ret.empty() && v->get_face())
      ret += v->get_face();

   return ret;
}

Wvec
vert_normal(CBvert* v, CBface* f, CSimplexFilter& filter)
{
   assert(v && f && f->contains(v));

   if (v->degree(filter) < 2)
      return v->norm();

   Wvec ret;
   Bedge* e=0;
   Bface* cur = rewind_cw(v, f, filter);
   do {
      ret += weighted_vnorm(cur, v);
   } while (
      (e = cur->edge_before_vert(v)) &&
      (cur = e->other_face(cur)) &&
      !filter.accept(e)
      );
   return ret.normalized();
}

// end of file mi.C
