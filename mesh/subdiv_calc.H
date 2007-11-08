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
 * subdiv_calc.H
 *****************************************************************/
#ifndef SUBDIV_CALC_H_IS_INCLUDED
#define SUBDIV_CALC_H_IS_INCLUDED

#include "bfilters.H"
#include "lvert.H"

/*****************************************************************
 * SubdivCalc:
 *
 *      Calculate subdivision values (e.g. position, color,
 *      uv-coords, etc) according to some scheme. LoopCalc,
 *      below, uses the scheme of Charles Loop. It's
 *      templated to work on different types, e.g. Wpts for
 *      computing subdivison postions, COLORs for computing
 *      subdivision colors, etc. In those examples, the
 *      calculations are identical, only the types are
 *      different. 
 *****************************************************************/
#define CSubdivCalc const SubdivCalc
template <class T>
class SubdivCalc {
 public:
   //******** MANAGERS ********
   SubdivCalc(): _boss(0) {}
   virtual ~SubdivCalc() {}

   void set_boss(SubdivCalc<T> * boss) { _boss=boss; }

   //******** VALUE ACCESSORS ********

   // This method tells how to get the "value" out of a vertex.
   // Normally subclasses override it. But they can optionally
   // not over-ride it as long as they provide a "boss"
   // SubdivCalc that does over-ride it. Used in the hybrid
   // scheme, below, which is the boss of a LoopCalc and a
   // CatmullClarkCalc, calling on each as needed:
   virtual T get_val(CBvert* v)    const {
      assert(_boss);
      return _boss->get_val(v);
   }

   // override for types that need initialization, eg double
   virtual void clear(T&) const { }

   //******** DEBUG HELPER ********

   // Used mainly for debugging, to identify the type of scheme
   // in cerr statements:
   virtual str_ptr name() const {
      // XXX - can't use static variables ...
      // The Sun compiler asks that you bear with it.
      return str_ptr("SubdivCalc");
   }

   //******** SUBDIVISION CALCULATION ********

   virtual T subdiv_val(CBvert*)        const = 0;
   virtual T subdiv_val(CBedge*)        const = 0;
   virtual T limit_val (CBvert*)        const = 0;

   //******** DUPLICATING ********

   virtual SubdivCalc<T> *dup() const { return 0; }

 protected:
   SubdivCalc<T> * _boss;
};

/*****************************************************************
 * SimpleCalc:
 *
 *      Calculates subdivision values using simple averaging at
 *      edges, interpolating at vertices.
 *****************************************************************/
template <class T>
class SimpleCalc : public SubdivCalc<T> {
 public:

   //******** SubdivCalc VIRTUAL METHODS ********
   virtual str_ptr name() const {
      // XXX - can't use static variables ...
      // The Sun compiler asks that you bear with it.
      return str_ptr("Simple subdivision");
   }

   //******** SUBDIVISION CALCULATION ********

   // Interpolate at vertices:
   virtual T subdiv_val(CBvert* v) const { return get_val(v); }

   // Take midpoint average at edges:
   virtual T subdiv_val(CBedge* e) const {
      assert(e);
      if (e->is_weak()) {

         // The edge is a quad diagonal.
         // For quad diagonals we actually average the 4 quad points:

         Bface* f1 = e->f1();
         Bface* f2 = e->f2();

         // Many assumptions should hold:
         assert(f1 && f1->is_quad() &&
                f2 && f2->is_quad() && f1->quad_partner() == f2);

         return (
            get_val(f1->v1()) +
            get_val(f1->v2()) +
            get_val(f1->v3()) +
            get_val(f1->quad_vert()))/4.0;

      } else {
         return (get_val(e->v1()) + get_val(e->v2()))/2.0;
      }
   }

   // Interpolate at vertices:
   virtual T limit_val (CBvert* v) const { return get_val(v); }

   using SubdivCalc<T>::get_val;
};

extern double loop_alpha(int n);
/*****************************************************************
 * LoopCalc:
 *
 *      Calculates subdivision values using the scheme of
 *      Charles Loop, with special rules for creases as
 *      described in:
 *
 *        Hugues Hoppe, Tony DeRose, Tom Duchamp, Mark Halstead,
 *        Hubert Jin, John McDonald, Jean Schweitzer, and Werner
 *        Stuetzle. Piecewise Smooth Surface Reconstruction,
 *        Proceedings of SIGGRAPH 94, Computer Graphics
 *        Proceedings, Annual Conference Series, pp. 295-302
 *        (July 1994, Orlando, Florida). ACM Press. Edited by
 *        Andrew Glassner. ISBN 0-89791-667-0.
 *****************************************************************/
template <class T>
class LoopCalc : public SubdivCalc<T> {
 public:

   //******** SubdivCalc VIRTUAL METHODS ********
   virtual str_ptr name() const {
      // XXX - can't use static variables ...
      // The Sun compiler asks that you bear with it.
      return str_ptr("Loop subdivision");
   }

   virtual T centroid(CLvert* v) const {

      // Return average of neighboring vertices for use in computing
      // subdivision values. Here "neighbors" are those determined by
      // the masks in the Hoppe paper, above.

      switch (v->subdiv_mask()) {
       case Lvert::SMOOTH_VERTEX:
       case Lvert::DART_VERTEX:
       {
          // regular centroid -- average of all neighboring verts
          T ret;
          clear(ret);
          Bvert_list nbrs;
          v->get_p_nbrs(nbrs);
          int n = nbrs.num();
          for (int k=0; k<n; k++)
             ret = (ret + get_val(nbrs[k]));
          return ret/n;
       }
       case Lvert::REGULAR_CREASE_VERTEX:
       case Lvert::NON_REGULAR_CREASE_VERTEX:
       {
          // average of neighboring crease/border/polyline verts
          //
          // XXX - there should be exactly 2 qualified neighbors.
          //       should we simplify the code to rely on that?
          //       ... naaah.
          T ret;
          clear(ret);
          int count = 0;
          Bvert_list nbrs;
          v->get_p_nbrs(nbrs);
          for (int k=0; k<nbrs.num(); k++) {
             Bedge* e = v->e(k);
             if (e->is_crease() || e->is_border() || e->is_polyline()) {
                double w = 1.0 / ++count; // weight for incoming value
                ret = interp(ret, get_val(v->nbr(k)), w);
             }
          }
          return ret;
       }
       case Lvert::CORNER_VERTEX:
       case Lvert::CUSP_VERTEX:
       default:
         // if that's really the mask, we don't need the
         // centroid. i.e. this won't get called. return the value at
         // the vertex anyway.
         return get_val(v);
      }
   }

   // helpers
   T smooth_subdiv_val(CLvert* v) const {
      // Compute subdivision value for a vertex whose mask is
      // SMOOTH_VERTEX or DART_VERTEX

      // from Figure 3 in Hoppe et al. Siggraph 94 (see above).

      int    n = v->p_degree(); // its degree
      double a = loop_alpha(n); // "alpha" from Hoppe94 above
      double w = a / (a + n);   // weight for the vertex
      return interp(centroid(v), get_val(v), w);
   }

   T crease_subdiv_val(CLvert* v) const {
      // Compute subdivision value for a crease vertex

      // From figure 4 in Hoppe et al., Siggraph 94 (see above).

      return interp(centroid(v), get_val(v), 0.75);
   }

   T smooth_limit_val (CLvert* v) const {
      // Compute the limit value using the smooth limit mask.

      // From figure 2 in Hoppe et al., Siggraph 94 (see above).

      // XXX - should cache omega in lookup table:

      int n = v->p_degree();
      double a = 5.0/8 - sqr(3 + 2*cos(TWO_PI/n))/64;
      double o = 3*n/(8*a);
      double w = o / (o + n);
//       double omega = 3 * n / (5.0 - sqr(3.0 + 2.0*cos(2*M_PI/n))/8.0);
//       double w = omega / (omega + n);
      return interp(centroid(v), get_val(v), w);
   }

   T crease_limit_val (CLvert* v) const {
      // Compute limit value for a crease vertex

      // From figure 4 in Hoppe et al., Siggraph 94 (see above).

      return interp(centroid(v), get_val(v), 2.0/3);
   }

   //******** SUBDIVISION CALCULATION ********
   virtual T subdiv_val(CBvert* bv) const {
      // XXX - don't call with a non-Lvert
      CLvert* v = (CLvert*)bv;      
      switch (v->subdiv_mask()) {
       case Lvert::SMOOTH_VERTEX:
       case Lvert::DART_VERTEX:
         return smooth_subdiv_val(v);
       case Lvert::REGULAR_CREASE_VERTEX:
       case Lvert::NON_REGULAR_CREASE_VERTEX:
         return crease_subdiv_val(v);
       default:
         return get_val(v);
      }
   }

   virtual T subdiv_val(CBedge* e) const {
      // XXX - don't call with a non-Ledge
      switch (((CLedge*)e)->subdiv_mask()) {
       case Ledge::REGULAR_SMOOTH_EDGE:
         return (
            (get_val(e->v1()) + get_val(e->v2()))*3.0 +
            get_val(e->opposite_vert1()) +
            get_val(e->opposite_vert2())
            )/8.0;
       case Ledge::REGULAR_CREASE_EDGE:
       default:
         // we're skipping the whole regular / non-regular thing
         // because this seems to work fine:
         return (get_val(e->v1()) + get_val(e->v2()))/2.0;
      }
   }

   virtual T limit_val (CBvert* bv) const {
      // XXX - don't call with a non-Lvert
      CLvert* v = (CLvert*)bv;
      switch (v->subdiv_mask()) {
       case Lvert::SMOOTH_VERTEX:
       case Lvert::DART_VERTEX:
         return smooth_limit_val(v);
       case Lvert::REGULAR_CREASE_VERTEX:
       case Lvert::NON_REGULAR_CREASE_VERTEX:
         return crease_limit_val(v);
       default:
         return get_val(v);
      }
   }

   using SubdivCalc<T>::get_val;
};

/*****************************************************************
 * LoopLoc: Calculates subdivision positions using Loop scheme.
 *****************************************************************/
class LoopLoc : public LoopCalc<Wpt> {
 public:
   //******** VALUE ACCESSORS ********
   virtual Wpt get_val(CBvert* v)   const { return v->loc(); }

   //******** SUBDIVISION CALCULATION ********
   Wvec limit_normal(CBvert* v) const {
      // Compute limit normal following Hoppe et al., SIGGRAPH 94.
      //
      // XXX - doesn't handle creases, just borders.
      //       for creases, the input arguments would have to specify
      //       which side to take the normal from.

      assert(v);

      // need adjacent faces to proceed:
      if (v->face_degree(BfaceFilter()) == 0)
         return Wvec::null();

      // until ordering CCW is fixed for non-manifold surfaces,
      // we'll assert the surface is manifold here:
      assert(v->is_manifold());

      // get neighbors in CCW order:
      Bvert_list nbrs = v->get_ccw_nbrs();
      assert(nbrs.num() == v->degree());
      
      Wpt t1, t2; // tangent "vectors" 
      int n = nbrs.num();
      int bd = v->border_degree();
      if (bd == 0) {
         // no borders: simple case
         for (int k=0; k<n; k++) {
            double theta = TWO_PI * k / n;
            t1 += nbrs[k]->loc() * cos(theta);
            t2 += nbrs[k]->loc() * sin(theta);
         }
      } else if (bd == 2) {
         // t1: along the border:
         t1 = nbrs.first()->loc() + (-1 * nbrs.last()->loc());
         // t2 goes across the border:
         if (n == 4) {
            // regular case
            t2 = (      v->loc()*(-2) +
                  nbrs[0]->loc()*(-1) +
                  nbrs[1]->loc()*( 2) +
                  nbrs[2]->loc()*( 2) +
                  nbrs[3]->loc()*(-1));
         } else if (n == 2) {
            t2 = nbrs.first()->loc() + nbrs.last()->loc() + (-2 * v->loc());
         } else if (n == 3) {
            t2 = nbrs[1]->loc() + (-1 * v->loc());
         } else {
            double theta = M_PI/(n - 1);
            double c = 2*cos(theta) - 2;
            t2 = (nbrs.first()->loc() + nbrs.last()->loc())*sin(theta);
            for (int i=1; i<n-1; i++) {
               t2 += c*sin(i*theta)*nbrs[i]->loc();
            }   
         }
      } else {
         // number of border edges not 0 or 2: error
         assert(0);
      }
      return (cross(t1 - Wpt::Origin(), t2 - Wpt::Origin())).normalized();
   }

   //******** DUPLICATING ********
   virtual SubdivCalc<Wpt> *dup() const { return new LoopLoc(); }
};

/*****************************************************************
 * LoopColor: Calculates subdivision colors using Loop scheme.
 *****************************************************************/
class LoopColor : public LoopCalc<COLOR> {
 public:
   //******** VALUE ACCESSORS ********
   virtual COLOR get_val(CBvert* v) const { return v->color(); }

   //******** DUPLICATING ********
   virtual SubdivCalc<COLOR> *dup() const { return new LoopColor(); }
};

/*****************************************************************
 * CatmullClarkCalc:
 *
 *      Calculates subdivision values using the Catmull-Clark
 *      scheme. Based on:
 *
 *        Tony DeRose and Michael Kass and Tien Truong.
 *        Subdivision Surfaces in Character Animation,
 *        Proceedings of SIGGRAPH 98, Computer Graphics
 *        Proceedings, Annual Conference Series, pp. 85-94 
 *        (July 1998, Orlando, Florida). Addison Wesley. 
 *        Edited by Michael Cohen.  ISBN 0-89791-999-8.
 *
 *      The BMESH class is based on triangles (Bfaces) and
 *      doesn't have an explicit notion of quads or polygons
 *      with more than 4 sides. To work around that, an edge
 *      (Bedge) can be labelled "weak," which means it is
 *      considered to be the internal diagonal edge of a
 *      quad, defined by the 2 adjacent Bfaces.
 *
 *      XXX - These rules should work for meshes consisting of
 *      quads and triangles (possible bugs aside). However,
 *      meshes at this time do refinement by splitting all
 *      triangles 1-to-4, which means that for now Catmull-Clark
 *      rules should only be applied to meshes consisting
 *      entirely of "quads."
 *****************************************************************/
template <class T>
class CatmullClarkCalc : public SubdivCalc<T> {
 public:

   //******** SubdivCalc VIRTUAL METHODS ********
   virtual str_ptr name() const {
      // XXX - can't use static variables ...
      // The Sun compiler asks that you bear with it.
      return str_ptr("Catmull-Clark subdivision");
   }

   T vcentroid(CBvert* v) const {
      // "vert centroid" - average of neighboring vertices,
      // not counting those connected by weak edges.
      // At non-manifold parts of the mesh, take neighbors
      // just from the primary level.
      T ret;
      clear(ret);
      Bedge_list adj;    // adjacent edges from primary level
      v->get_manifold_edges(adj);
      int count = 0;
      for (int k=0; k<adj.num(); k++) {
         if (adj[k]->is_strong()) {
            // weight for incoming value:
            double w = 1.0 / ++count; 
            ret = interp(ret, get_val(adj[k]->other_vertex(v)), w);
         }
      }
      return ret;
   }
   T fcentroid(CBface* f) const {
      // "face centroid" - if the face is half of a quad,
      // returns the average of the 4 vertex values at the
      // corners of the quad. otherwise returns the average
      // of the 3 vertex values of the face.
      T ret = (get_val(f->v1()) + get_val(f->v2()) + get_val(f->v3()))/3.0;
      if (f->is_quad() && f->quad_vert())
         ret = (ret*0.75) + (get_val(f->quad_vert())*0.25);
      return ret;
   }
   T fcentroids(CARRAY<Bface*>& faces) const {
      // returns average of the face centroids in the given list
      // (usually from the 1-ring of a given vertex)
      T ret;
      clear(ret);
      if (faces.empty()) {
         err_msg("CatmullClarkCalc::fcentroids: error: empty face list");
         return ret;
      }
      for (int k=0; k<faces.num(); k++)
         ret = ret + fcentroid(faces[k]);
      return ret / faces.num();
   }
   T smooth_centroid(CBvert* v) const {
      // value used in catmull-clark computation for
      // "smooth" case - average of:
      //   (1) vertex centroid and
      //   (2) average of the neighboring face centroids

      Bface_list faces;
      v->get_quad_faces(faces);

      return (vcentroid(v) + fcentroids(faces))*0.5;
   }
   T crease_centroid(CBvert* v) const {
      // value used in catmull-clark computation for a
      // "crease" vertex. returns average of neighboring
      // crease/border/polyline verts.
      //
      // should only have 2 qualifying neighbors
      T ret;
      int count = 0;
      Bedge_list adj;    // adjacent edges from primary level
      v->get_manifold_edges(adj);
      for (int k=0; k<adj.num(); k++) {
         if (v->e(k)->is_strong_poly_crease()) {
            // weight for incoming value:
            double w = 1.0 / ++count; 
            ret = interp(ret, get_val(v->nbr(k)), w);
         }
      }
      if (count != 2) {
         cerr << "CatmullClarkCalc::crease_centroid: "
              << "bad number of neighbors ("
              << count
              << ")" << endl;
      }
      return ret;
   }

   T smooth_subdiv_val(CBvert* v) const {
      // catmull-clark rule for smooth vertex
      int n = 0;
      if (v->is_manifold())
         n = v->degree(StrongEdgeFilter());
      else
         n = v->get_manifold_edges().filter(StrongEdgeFilter()).num();
      double w = (n-2)/double(n); // vertex weight
      return interp(smooth_centroid(v), get_val(v), w);
   }
   T crease_subdiv_val(CBvert* v) const {
      // catmull-clark rule for crease vertex
      return get_val(v)*0.75 + crease_centroid(v)*0.25;
   }
   T smooth_subdiv_val(CBedge* e) const {
      if (e->is_weak()) {
         // this is really the face rule
         return fcentroid(e->f1());
      } else {
         return (
            get_val(e->v1())  +
            get_val(e->v2())  +
            fcentroid(e->f1()) +
            fcentroid(e->f2())
            )/4.0;
      }
   }
   T crease_subdiv_val(CBedge* e) const {
      return (get_val(e->v1()) + get_val(e->v2()))*0.5;
   }

   //******** SUBDIVISION CALCULATION ********
   virtual T subdiv_val(CBvert* v) const {
      // count number of creases/borders
      int n = 0;
      if (v->is_manifold())
         n = v->degree(StrongPolyCreaseEdgeFilter());
      else
         n = v->get_manifold_edges().filter(StrongPolyCreaseEdgeFilter()).num();
      switch (n) {
       case 0:
       case 1:
         return smooth_subdiv_val(v);
       case 2:
         return crease_subdiv_val(v);
       default:
         return get_val(v);
      }
   }

   virtual T subdiv_val(CBedge* e) const {
      return e->is_poly_crease() ? crease_subdiv_val(e) : smooth_subdiv_val(e);
   }

   virtual T limit_val (CBvert*) const {
      // XXX - not implemented yet
      assert(0);
      return T();
   }

   using SubdivCalc<T>::get_val;
};

class CatmullClarkLoc : public CatmullClarkCalc<Wpt> {
 public:
   //******** VALUE ACCESSORS ********
   virtual Wpt get_val(CBvert* v)   const { return v->loc(); }

   //******** DUPLICATING ********
   virtual SubdivCalc<Wpt> *dup() const { return new CatmullClarkLoc(); }
};

/*****************************************************************
 * HybridCalc:
 *
 *   Scheme that is a hybrid of the Loop scheme and the
 *   Catmull-Clark scheme. In regions of triangles it uses the
 *   Loop masks. In regions of quads it uses the Catmull-clark
 *   masks. Along the border between the two types of regions it
 *   uses specialized "hybrid" masks.
 *****************************************************************/
template <class T>
class HybridCalc : public SubdivCalc<T> {
 protected:
   LoopCalc<T>          _loop_calc;  // Computes Loop scheme
   CatmullClarkCalc<T>  _cc_calc;    // Computes Catmull-clark scheme

 public:

   //******** MANAGERS ********
   HybridCalc<T>() {
      _loop_calc.set_boss(this);
      _cc_calc.set_boss(this);
   }

   //******** HELPER FUNCTIONS ********
   T sum(CBvert_list & p) const {
      T ret;
      clear(ret);
      for (int i=0; i<p.num(); i++)
         ret = ret + get_val(p[i]);
      return ret;
   }

   virtual T hybrid_centroid(CBvert* v) const {
      Bvert_list p; // from edges whose "num quads" is 2
      Bvert_list q; // vertices that lie opposite this one in a quad
      Bvert_list r; // from edges whose "num quads" is 1
      Bvert_list t; // from edges whose "num quads" is 0

      // Load up p and r lists
      Bedge_list e = v->get_manifold_edges();
      for (int i=0; i<e.num(); i++) {
         if (e[i]->is_strong()) {
            switch (e[i]->num_quads()) {
             case 1:
               r += e[i]->other_vertex(v);
             brcase 2:
               p += e[i]->other_vertex(v);
             brdefault:
               t += e[i]->other_vertex(v);
            }
         }
      }

      // Get the q list
      v->get_q_nbrs(q);

      // Define weighting to use for each vertex type:
      double alpha = 6;
      double beta  = 1;
      double gamma = 5;
      double delta = 4;

      double net_weight =
         alpha*p.num() + beta*q.num() + gamma*r.num() + delta*t.num();

      return (sum(p)*alpha + sum(q)*beta + sum(r)*gamma + sum(t)*delta) /
         net_weight;
   }

   //******** SubdivCalc VIRTUAL METHODS ********
   virtual str_ptr name() const {
      // XXX - can't use static variables ...
      // The Sun compiler asks that you bear with it.
      return str_ptr("Hybrid subdivision");
   }

   //******** SUBDIVISION CALCULATION ********
   virtual T subdiv_val(CBvert* v) const {
      if (v->p_degree() < 2)
         return get_val(v);

      // count number of creases/borders
      int n = 0;
      if (v->is_manifold())
         n = v->degree(StrongPolyCreaseEdgeFilter());
      else
         n = v->get_manifold_edges().filter(StrongPolyCreaseEdgeFilter()).num();
      switch (n) {
       case 0:
       case 1: {
             
          // no quads
          if (v->num_quads()==0)
             return(_loop_calc.subdiv_val(v));

          // all quads
          else if (v->num_tris()==0)
             return(_cc_calc.subdiv_val(v));

          // mix
          else {

             // "degree" of vertex
             double n = v->num_tris()+(3/2.0)*v->num_quads();

             // k used in cc calculation
             double k = (2/3.0)*n;

             // used for loop calculation
             double b = 5.0/8.0 - sqr(3.0 + 2.0*cos(2*M_PI/n))/64.0;

             double loop_weight = 1-b;
             double cc_weight= 1 - ( 7 / (4*k) );
             double scaling=v->num_tris()/n;
             double w = loop_weight*scaling + cc_weight*(1-scaling);

             return interp(hybrid_centroid(v), get_val(v), w);
          }
       }
       brcase 2:
          return _loop_calc.crease_subdiv_val((Lvert*)v);
       brdefault:
          return get_val(v);
      }
   }

   virtual T subdiv_val(CBedge* e) const {
      if (e->is_poly_crease())
         return _loop_calc.subdiv_val(e);
      switch (e->num_quads()) {
       case 0:  return _loop_calc.subdiv_val(e);  // All Loop
       case 2:  return (_cc_calc.subdiv_val(e));  // All Catmull-Clark
       default: {
          //
          //       Hybrid case
          //
          //            3_________ 0.5       
          //           /|         |          
          //         /  |         |          
          //       /    |         |          
          //   1 /      | e       | e2       
          //     \  tri |         |         
          //       \    |   quad  |          
          //         \  |         |          
          //           \|_________|
          //            3          0.5
          //
          CBface* quad = e->f1()->is_quad() ? e->f1() : e->f2();
          CBface*  tri = e->other_face(quad);
          CBedge*   e2 = quad->opposite_quad_edge(e); 
          return (
             0.5*(get_val(e2->v1()) + get_val(e2->v2())) +
             3.0*(get_val(e->v1()) + get_val(e->v2())) +
             get_val(tri->other_vertex(e))
             )/8.0;
       }
      }
   }

   virtual T limit_val (CBvert* v) const {
      // XXX - not implemented yet -- go with Loop calculation
      return _loop_calc.limit_val(v);
   }

   using SubdivCalc<T>::get_val;
};

class HybridLoc : public HybridCalc<Wpt> {
 public:
   //******** VALUE ACCESSORS ********
   virtual Wpt get_val(CBvert* v)   const { return v->loc(); }

   //******** DUPLICATING ********
   virtual SubdivCalc<Wpt> *dup() const { return new HybridLoc(); }
};

typedef SubdivCalc<Wpt>   SubdivLocCalc;
typedef SubdivCalc<COLOR> SubdivColorCalc;

/*****************************************************************
 * VolPreserve:
 *
 *      Does subdivision via any given scheme (its base class),
 *      with a correction to control the change in volume at each
 *      refinement.
 *****************************************************************/
template <class C>
class VolPreserve : public C {
 public:
   //******** MANAGERS ********
   VolPreserve() : _base_calc(new C) {}

   //******** VALUE ACCESSORS ********
   virtual Wpt get_val(CBvert* v)   const {
      return ((Lvert*)v)->displaced_loc(_base_calc);
   }

   //******** DEBUG HELPER ********

   // Used mainly for debugging, to identify the type of scheme
   // in cerr statements:
   virtual str_ptr name() const {
      return C::name() + str_ptr(", with volume preservation");
   }

   //******** DUPLICATING ********
   virtual SubdivCalc<Wpt> *dup() const { return new VolPreserve<C>(); }

 protected:
   C*   _base_calc; // instance of plain base class
};

typedef VolPreserve<LoopLoc>            LoopVolPreserve;
typedef VolPreserve<CatmullClarkLoc>    CCVolPreserve;
typedef VolPreserve<HybridLoc>          HybridVolPreserve;

#endif // SUBDIV_CALC_H_IS_INCLUDED

// end of file subdiv_calc.H
