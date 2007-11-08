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
 * hybrid.H
 *****************************************************************/
#ifndef HYBRID_H_IS_INCLUDED
#define HYBRID_H_IS_INCLUDED

#include "subdiv_calc.H"

//
// A Bface is a triangle if it is non-null and
// isn't part of a quad
//
inline bool
is_tri(CBface* f)
{
   return f && !f->is_quad();
}

//
// A Bedge is adjacent to a triangle if it belongs
// to a face that is a triangle
//
inline bool
is_adjacent_tri(CBedge* e)
{
   return e && (is_tri(e->f1()) || is_tri(e->f2()));
}

//
// A Bvert is adjacent to a triangle if it belongs
// to an edge that is adjacent to a triangle
//
inline bool
is_adjacent_tri(CBvert* v)
{
   if (!v) return 0;
   for (int i=0; i<v->degree(); i++)
      if (is_adjacent_tri(v->e(i)))
         return 1;
   return 0;
}


//
// A Bvert is near a triangle if any of its
// neighboring vertices is adjacent to a triangle
//
inline bool
is_near_tri(CBvert* v)
{
   if (!v) return 0;
   if (is_adjacent_tri(v))
      return 1;
   for (int i=0; i<v->degree(); i++)
      if (is_adjacent_tri(v->nbr(i)))
         return 1;
   return 0;
}

/*
//
//                  v2                              
//                /  | \                            
//              /    |   \                          
//        vf2 /      |     \ vf1                    
//            \  f2  | f1  /                        
//              \    |   /                          
//                \  | /                            
//                  v1                              
//
// A Bedge is near a triangle if any of the 4
// vertices v1, v2, vf1, vf2 (see above) are
// adjacent to a triangle. I.e., the edge is in
// the 1-ring (maybe on the boundary) of a vertex
// that is adjacent to a triangle.
//
*/
inline bool
is_near_tri(CBedge* e)
{
   return (
      e && (is_adjacent_tri(e->v1()) ||
            is_adjacent_tri(e->v2()) ||
            is_adjacent_tri(e->opposite_vert1()) ||
            is_adjacent_tri(e->opposite_vert2()))
      );
}

/*****************************************************************
 * HybridCalc2:
 *
 *   Scheme that is a hybrid of the Loop scheme and the
 *   Catmull-Clark scheme. In regions near triangles it uses the
 *   Loop masks. In regions of quads it uses the Catmull-clark
 *   masks. The "near-triangle" region of the quads keeps shrinking
 *   with each subdivision.
 *****************************************************************/
template <class T>
class HybridCalc2 : public SubdivCalc<T> {
 public:
   //******** MANAGERS ********
   HybridCalc2<T>() {
      _loop_calc.set_boss(this);
      _cc_calc.set_boss(this);
   }

   //******** SubdivCalc VIRTUAL METHODS ********
   virtual str_ptr name() const {
      static str_ptr ret("Nouveau Hybrid Subdivision");
      return ret;
   }

   //******** SUBDIVISION CALCULATION ********
   virtual T subdiv_val(CBvert* v) const {
      if (is_near_tri(v))
         return(_loop_calc.subdiv_val(v));
      return(_cc_calc.subdiv_val(v));
   }

   virtual T subdiv_val(CBedge* e) const {
      if (is_near_tri(e))
         return _loop_calc.subdiv_val(e);
      return (_cc_calc.subdiv_val(e));  // All Catmull-Clark
   }

   virtual T limit_val (CBvert* v) const {
      // actually we don't know the limit value in a region
      // near the boundary of quads and triangles.
      if (is_near_tri(v))
         return _loop_calc.limit_val(v);
      return _cc_calc.limit_val(v);
   }

 protected:
   LoopCalc<T>          _loop_calc;  // Computes Loop scheme
   CatmullClarkCalc<T>  _cc_calc;    // Computes Catmull-clark scheme
};

class Hybrid2Loc : public HybridCalc2<Wpt> {
 public:
   //******** VALUE ACCESSORS ********
   virtual Wpt get_val(CBvert* v)   const { return v->loc(); }

   //******** DUPLICATING ********
   virtual SubdivCalc<Wpt> *dup() const { return new Hybrid2Loc(); }
};

typedef VolPreserve<Hybrid2Loc> Hybrid2VolPreserve;

#endif // HYBRID_H_IS_INCLUDED

/* end of file hybrid.H */
