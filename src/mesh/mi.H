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
 * mi.H
 *
 *      (mesh inline)
 **********************************************************************/
#ifndef MI_H_HAS_BEEN_INCLUDED
#define MI_H_HAS_BEEN_INCLUDED

#include "std/run_avg.H"

#include "mesh/bfilters.H"
#include "mesh/lmesh.H"

/*****************************************************************
 * Bsimplex:
 *****************************************************************/
inline Wvec
get_norm(CBsimplex* s, bool debug=false)
{
   if (is_edge(s)) return ((Bedge*)s)->norm();
   if (is_vert(s)) return ((Bvert*)s)->norm();
   if (is_face(s)) return ((Bface*)s)->norm();
   return Wvec::null();
}

/*****************************************************************
 * Bedge:
 *****************************************************************/
inline double
avg_edge_len(Bface* f)
{
   if (!f) return 0;
   return (f->e1()->length() + f->e2()->length() + f->e3()->length())/3;
}

inline double 
avg_face_edge_len(CBedge* e)
{
   if (!e) return 0;
   if (e->is_polyline())
      return e->length();
   RunningAvg<double> avg(0);
   Bface* f=0;
   if ((f = e->f1()))
      avg.add(f->is_quad() ? f->quad_avg_dim() : avg_edge_len(f));
   if ((f = e->f2()))
      avg.add(f->is_quad() ? f->quad_avg_dim() : avg_edge_len(f));
   return avg.val();
}

/*****************************************************************
 * Bvert:
 *****************************************************************/
inline double 
avg_strong_len(CBvert* v)
{
   // average length of strong edges adjacent to v

   if (!v)
      return 0;

   RunningAvg<double> avg(0);
   Bedge* e=0;
   for (int i=0; i<v->degree(); i++)
      if ((e = v->e(i))->is_strong())
         avg.add(e->length());
   return avg.val(); 
}

/*****************************************************************
 * Bface:
 *****************************************************************/
inline Wvec
weighted_vnorm(CBface* f, CBvert* v)
{
   if (!f->contains(v))
      return Wvec(0.0, 0.0, 0.0);
   
   Bvert* a = next_vert_ccw(f,v);
   Bvert* b = next_vert_ccw(f,a);
   
//    Wvec e1 = a->loc() - v->loc();
//    Wvec e2 = b->loc() - v->loc();
//    
//    return cross(e1, e2) * (1.0 /(e1.length_sqrd() * e2.length_sqrd()));
   
   return (f->norm()*fabs(f->area())*2.0)
          / ((a->loc() - v->loc()).length_sqrd()
          * (b->loc() - v->loc()).length_sqrd());
//    return f->norm() * f->angle(v);
}

inline double
norm_angle(CBedge* e)
{
   return (e && e->nfaces() == 2) ? e->f1()->norm().angle(e->f2()->norm()) : 0;
}

// get an average normal, not crossing edges accepted by the given
// filter:
Wvec
vert_normal(CBvert* v, CBface* f, CSimplexFilter& filter);

// given a vertex v and a face f, step clockwise around v
// from one face to the next as long as no edge we step
// across is accepted by the given filter.
inline Bface* 
rewind_cw(CBvert* v, CBface* start, CSimplexFilter& filter)
{
   Bface* ret = (Bface*)start;
   if (!(start && v && start->contains(v)))
      return 0;
   if (v->degree(filter) == 0)
      return ret;

   Bface* f = ret;
   Bedge* e = 0;
   while ((e = f->edge_from_vert(v)) &&
          (f = e->other_face(ret)) &&
          !filter.accept(e))
      ret = f;

   return ret;
}

// return a list of faces, one face per sector in the
// "star" around v, where sectors separated by edges
//  accpted by the filter (or by border edges):
Bface_list
leading_faces(CBvert* v, CSimplexFilter& filter);

// if the 3 verts form a Bface that is part of a quad,
// return the face:
inline Bface*
lookup_quad(Bvert* v1, Bvert* v2, Bvert* v3)
{
   Bface* f = lookup_face(v1, v2, v3);
   return (f && f->is_quad()) ? f : 0;
}

// if the 4 vertices make up a quad, this returns the quad
// face that contains v1 (otherwise null):
inline Bface*
lookup_quad(Bvert* v1, Bvert* v2, Bvert* v3, Bvert* v4)
{
   if (!(v1 && v2 && v3 && v4))
      return 0;
   Bface* f = 0;
   if ((f = lookup_quad(v1, v2, v3)) && f->quad_vert() == v4)
      return f;
   if ((f = lookup_quad(v1, v2, v4)) && f->quad_vert() == v3)
      return f;
   if ((f = lookup_quad(v1, v3, v4)) && f->quad_vert() == v2)
      return f;
   return 0;
}

// if the 4 vertices make up a quad, this returns the weak edge
// of the quad (otherwise null):
inline Bedge*
get_quad_weak_edge(Bvert* v1, Bvert* v2, Bvert* v3, Bvert* v4)
{
   Bface* f = lookup_quad(v1, v2, v3, v4);
   return f ? f->weak_edge() : 0;
}

inline double
avg_strong_edge_len(CBface* f)
{
   // returns average length of strong edges in f

   if (!f) return 0;

   RunningAvg<double> avg(0);
   if (f->e1()->is_strong()) avg.add(f->e1()->length());
   if (f->e2()->is_strong()) avg.add(f->e2()->length());
   if (f->e3()->is_strong()) avg.add(f->e3()->length());
   return avg.val();
}

/*****************************************************************
 * Bvert_list:
 *****************************************************************/
inline void
set_adjacent_edges(CBvert_list& verts, uchar b)
{
   for (int i=0; i<verts.num(); i++)
      verts[i]->get_adj().set_flags(b);
}

/*****************************************************************
 * Bedge_list:
 *****************************************************************/
inline double
avg_strong_edge_len(CBedge_list& edges)
{
   return edges.filter(StrongEdgeFilter()).avg_len();
}

inline bool
is_maximal(CBedge_list& edges)
{
   // returns true if there are no edges connected to any in
   // the given set (other than those in the set):

   Bvert_list verts = edges.get_verts();
   verts.clear_flag02();        // clear flags of all adjacent edges
   edges.set_flags(1);          // set flags = 1 for edges in set

   // No vertex can have an adjacent edge whose flag is 0:
   return verts.all_satisfy(VertDegreeFilter(0, SimplexFlagFilter(0)));
}

/*****************************************************************
 * from inflate.C
 *****************************************************************/
#ifndef log2
#ifndef macosx
inline double
log2(double x)
{
   // return log base 2 of x

   // XXX - should remove (return NaN):
   assert(x > 0);

   return log(x) / M_LN2;
}
#endif // macosx
#endif // log2

#endif  // MI_H_HAS_BEEN_INCLUDED

/* end of file mi.H */
