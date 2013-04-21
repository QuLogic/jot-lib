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
 * bedge.H
 **********************************************************************/
#ifndef BEDGE_H_HAS_BEEN_INCLUDED
#define BEDGE_H_HAS_BEEN_INCLUDED

#include "net/data_item.H"
#include "simplex_filter.H"
#include "simplex_array.H"


class Patch;

class BMESH;
typedef const BMESH CBMESH;

/**********************************************************************
 * Bedge:
 *
 *      An edge joining two vertices, bordering one or two faces.
 *      The order of vertices is arbitrary.
 **********************************************************************/
class Bedge : public Bsimplex {
 public:

   //******** FLAG BITS ********

   // Used to store boolean states (see Bsimplex class):
   enum {
      CREASE_BIT = Bsimplex::NEXT_AVAILABLE_BIT, // it's a crease
      PATCH_BOUNDARY_BIT,                        // it's a patch boundary
      CONVEX_BIT,                                // it's convex
      CONVEX_VALID_BIT,                          // previous bit is believable
      WEAK_BIT,                                  // edge is labelled "weak"
      NEXT_AVAILABLE_BIT                         // derived classes start here
   };

   //******** MANAGERS ********

   Bedge(Bvert* u, Bvert* v);
   virtual ~Bedge();

   //******** ACCESSORS ********

   Bvert* v1()                  const   { return _v1; }
   Bvert* v2()                  const   { return _v2; }
   Bvert* v(int k)              const   { return (&_v1)[k-1]; } // k in [1..2]

   // Return primary face 1 or 2:
   Bface* f1()                  const   { return _f1; } 
   Bface* f2()                  const   { return _f2; }

   // Return number of non-NULL primary faces (f1 or f2):
   // XXX - ignores multi faces at this time:
   int    nfaces()              const;

   // Number of faces, including multi faces:
   // (For more info, see comment below about NON-MANIFOLD MESHES)
   int    num_all_faces()       const;

   // return a list of all adjacent faces, including f1 and f2
   // (but not if they are null):
   Bface_list get_all_faces() const;

   // Return face k, starting from 1, including "multi" faces:
   Bface* f(int k)              const;

   Patch* patch()               const;  

   uint   sil_stamp()           const   { return _sil_stamp; }
   void   set_sil_stamp(uint s)         { _sil_stamp = s; }

   // Clear flag of this edge and adjacent faces:
   // (clearing propagates from dim 0 to dim 2)
   void clear_flag02(); // defined in bface.H
   
   //******** GEOMETRIC MEASURES ********

   Wvec    vec()          const;  // vector from vertex 1 to vertex 2
   double  length()       const   { return vec().length(); }
   Wline   line()         const;  // mlib::Wline defined by endpoints
   PIXELline pix_line() const; //returns a line in screen space
   Wpt     mid_pt()       const;  // returns midpoint of edge
   Wpt&    mid_pt(mlib::Wpt& v) const;  // same as above, but returns reference
   Wvec    norm()         const;  // unit-length average of 2 face normals
   double  dot()          const;  // dot product of 2 face normals
   double  avg_area()     const;  // average area of 2 faces

   // Angle (in radians) between two adjacent face normals
   // (or 0, if < 2 faces):
   // XXX - name problem: it is not actually the dihedral angle!
   double dihedral_angle() const { return Acos(dot()); }

   Wpt interp(double s) const;

   // returns true if the edge is convex (uses cached value):
   bool is_convex() const {
      if (!is_set(CONVEX_VALID_BIT))
         ((Bedge*)this)->set_convex();
      return is_set(CONVEX_BIT);
   }

   //******** ADJACENCY ********

   // Only meaningful for primary faces:
   // Return the other vertex of the other face.
   // E.g., if f is _f1, return the other vertex of _f2;
   // and vice versa:
   Bvert* opposite_vert(CBface* f) const; // Defined in bface.H:

   // Opposite vertex on face 1 or 2, respectively:
   Bvert* opposite_vert1() const { return opposite_vert(_f2); }
   Bvert* opposite_vert2() const { return opposite_vert(_f1); }

   // If v is a vertex of this edge, return the other vertex:
   Bvert* other_vertex(CBvert* v) const {
      return (v==_v1)?_v2:(v==_v2)?_v1:0;
   }

   // Return the vertex's "index." i.e., 1 or 2
   // whether it is vertex _v1 or _v2 respectively:
   int vindex(CBvert* v) const { return (v==_v1) ? 1 : (v==_v2) ? 2 : -1; }

   Bvert* shared_vert(CBedge* e) const {
      return (
         (e == 0 || e == this) ? 0   :
         e->contains(_v1)      ? _v1 :
         e->contains(_v2)      ? _v2 : 0
         );
   }

   // tell whether this edge is adjacent to to the given vertex or face:
   bool contains(CBvert* v)   const   { return (_v1 == v || _v2 == v); }
   bool contains(CBface* f)   const;    // defined in bface.H

   // does this edge connect the given vertices?
   bool same_verts(CBvert* u, CBvert* v)  const {
      return contains(u) && contains(v);
   }

   // If f is a primary adjacent face, return the other primary adjacent face:
   Bface* other_face(CBface *f)  const   { return (f==_f1)?_f2:(f==_f2)?_f1:0;} 


   // Return a front-facing face, if any:
   // XXX - ignores secondary faces
   Bface* frontfacing_face()     const;

   // Return an adjacent face containing a given vertex or edge
   Bface* lookup_face(CBvert *v) const;
   Bface* lookup_face(CBedge *e) const;

   // Given vertex v belonging to this edge, return the adjacent
   // face (_f1 or _f2) for which this edge follows v in CCW
   // order within the face.
   Bface* ccw_face(CBvert* v) const;
   Bface*  cw_face(CBvert* v) const { return ccw_face(other_vertex(v)); }

   // Return the face (f1 or f2) for which _v2 follows _v1
   // in CCW or CW order, respectively:
   Bface* ccw_face() const { return ccw_face(_v1); }
   Bface* cw_face()  const { return ccw_face(_v2); }

   // Return the number of quads adjacent to the edge:
   int num_quads() const;

   // Count the number of adjacent faces (including secondary ones)
   // satisfied by the given filter:
   int nfaces_satisfy(CSimplexFilter& f) const;

   //******** NON-MANIFOLD MESHES ********

   // A "multi" edge has additional adjacent faces besides the 2
   // primary ones. (I.e. the surface is non-manifold at that edge).
   // A face is a "multi" face (WRT a given edge) if the face
   // is one of the additional adjacent faces of that edge.

   // Additional faces (usually null unless the edge is non-manifold):
   Bface_list*  adj() const { return _adj; }

 protected:
   // Allocate the list of secondary faces ("adjacent faces")
   void allocate_adj();

   // Add the face as a secondary (or "multi") face.
   // In Ledge, do the same for the appropriate child faces.
   virtual bool add_multi(Bface* f); 

   // Add the face as a primary face (in slot _f1 or _f2).
   // In Ledge, do the same for the appropriate child faces.
   virtual bool add_primary(Bface* f); 

 public:

   // Returns true if there is at least one available primary slot (_f1 or _f2).
   // In Ledge, all child edges must also have available slots.
   virtual bool can_promote() const;

   // Move f from the _adj list to _f1 or _f2:
   // It's up to the caller to check Bedge::can_promote().
   bool promote(Bface* f);

   // Move f from _f1 or _f2 to the _adj list:
   bool demote(Bface* f);

   // Is the edge a non-manifold ("multi") edge?
   bool is_multi() const;

   // Is the face a secondary face of this edge?
   bool is_multi(CBface* f) const;

   // If any faces in the _adj listed are labelled "primary",
   // then move them to a primary slot (_f1 or _f2). Used when
   // reading a mesh from file, when primary/secondary face
   // labels are specified only after all faces are created.
   void fix_multi(); 

   // Return true if at least one adjacent face is primary
   bool is_primary() const;

   // The edge is labelled "secondary" if it is not adjacent
   // to a primary face. This means "polyline" edges are
   // also considered secondary.
   bool is_secondary() const { return !is_primary(); }

   //******** EDGE TESTS ********
   // various tests that can be applied to an edge:
   //   border edge:   just 1 adjacent face.
   //   polyline edge: no adjacent faces
   //   crease edge:
   //      an edge can be labelled a "crease," which does 2 things:
   //      (1) normals are not blended across the edge in gouraud shading,
   //      (2) for subdivision meshes (LMESHes), separate crease rules 
   //          are used so the edge retains its sharpness.
   //   patch boundary:
   //      adjacent faces are in different patches; or they're in the same
   //      patch but the edge is considered to mark a patch boundary anyway, as
   //      when texture coordinates exhibit a seam running down a cylinder.
   //   sil edge:
   //      silhouette edge (for the current view / frame)
   //   stressed edge:
   //      has a pretty sharp dihedral angle.
   //   crossable edge:
   //      a triangle strip can pass across the edge -- i.e., the
   //      edge is not a patch boundary or a crease edge.

   bool is_interior()           const   { return nfaces() == 2; }
   bool is_border()             const   { return nfaces() == 1; }
   bool is_polyline()           const   { return nfaces() == 0; }
   bool is_crease()             const   { return is_set(CREASE_BIT); }
   bool is_weak()               const   { return is_set(WEAK_BIT); }
   bool is_strong()             const   { return !is_weak(); }
   bool is_poly_crease()        const   { return is_crease() || (nfaces()<2);}
   bool is_strong_poly_crease() const   { return is_strong() &&
                                                 is_poly_crease(); }
   bool is_patch_boundary()     const;
   bool is_texture_seam()       const;
   bool is_sil()                const;
   bool is_stressed()           const;
   bool is_crossable()          const;

   // Returns true if the 2 adjacent faces have a consistent orientation:
   bool consistent_orientation() const;
   bool oriented_ccw(Bface* =0)  const;

   // Returns true if either vertex has an odd number of adjacent
   // polyline edges (or crease edges, respectively). Used in
   // finding long edge strips of a given type:
   bool is_polyline_end()                       const;
   bool is_crease_end()                         const;
   bool is_chain_tip(CSimplexFilter& filter)      const;

   //******** EDGE LABELS ********
   void set_patch_boundary(int b=1) { set_bit(PATCH_BOUNDARY_BIT,b); }

   // labels this edge as a "crease" if passed-in value exceeds the
   // dot product of the 2 adjacent faces:
   void compute_crease(double dot); 

   // these take on more meaning in Ledge, which supports variable
   // sharpness creases for subdivision:
   virtual void set_crease(unsigned short = USHRT_MAX);
   virtual unsigned short crease_val() const { return is_crease() ? 1 : 0; }

   // The following increment and decrement the crease val,
   // respectively. 'max_val' represents a "large" value; if you try to
   // increment past max_val, it goes straight to USHRT_MAX. If you
   // try to decrement when the current value is USHRT_MAX, it goes
   // straight to max_val (not USHRT_MAX - 1). E.g. if max_val is 5,
   // then possible crease values are: { 0, 1, 2, 3, 4, 5, USHRT_MAX }.
   void inc_crease(ushort max_val = USHRT_MAX);
   void dec_crease(ushort max_val = USHRT_MAX);

   //******** SUBDIVISION ********

   // Difference between subdiv level of this edge and its control element
   // (See Ledge):
   virtual uint rel_level() const { return 0; }

   //******** BUILDING/REDEFINING ********

   bool operator +=(Bface* face);       // add adjacent face
   bool operator -=(Bface* face);       // remove adjacent face
   int  detach();                       // tell verts good-bye (pre-suicide)

   // Redefine this edge, replacing v with u:
   virtual int  redefine(Bvert *v, Bvert *u);

   // Redefine this edge, replacing both vertices w/ 2 new ones:
   virtual void set_new_vertices(Bvert *v1, Bvert *v2);

   // return true if performing an "edge swap" operation is both legal
   // and desirable:
   bool  swapable(Bface*&, Bface*&, Bvert*&, Bvert*&, Bvert*&, Bvert*&,
                         bool favor_degree_six=0);

   // NEW VERSIONS (12/2004)

   // Return true if edge swap does not violate topology requirements:
   bool swap_is_legal() const;

   // Carry out swap if it is legal:
   virtual bool do_swap();

   // 2nd version of redefine, should replace redefine() above, after testing:
   virtual bool redef2(Bvert *v, Bvert *u);

   //******** INTERSECTION ********

   bool    ndc_intersect(CNDCpt& p, mlib::CNDCpt& q, mlib::NDCpt& ret) const;
   int     which_side(CWplane& plane) const;

   //******** HANDLING CACHED VALUES ********

   virtual void notify_split(Bsimplex*);
   virtual void geometry_changed();  // Bsimplex method - notify SimplexData
   virtual void normal_changed();    // adjacent face changes its normal
   virtual void crease_changed();    // called when crease status changes
   virtual void faces_changed();    // called when faces are added or deleted

   //******** I/O ********

   friend ostream &operator <<(ostream &os, CBedge &e);

   //******** Bsimplex VIRTUAL METHODS ********

   // dimension
   virtual int    dim()              const { return 1; }

   // index in BMESH Bedge array:
   virtual int  index() const;
   
   // (Bsimplex virtual method):
   // Intersection w/ ray from given screen point -- returns the point
   // on the Bedge that is nearest to the given screen space point.
   // Note: the returned "near point" and "normal" are both
   //       transformed to world space.
   virtual bool view_intersect(
      CNDCpt&,  // Given screen point. Following are returned:
      Wpt&,     // Near point on the simplex IN WORLD SPACE (not object space)
      double&,  // Distance from camera to near point
      double&,  // Screen distance (in PIXELs) from given point
      Wvec& n   // "normal" vector at intersection point IN WORLD SPACE
      ) const;

   // faces
   virtual Bface* get_face()         const { return _f1?_f1:_f2?_f2:0;  }
   virtual bool   on_face(CBface* f) const { return contains(f); }

   // Return the first face accepted by the filter:
   Bface* screen_face(CSimplexFilter& filter) const;

   // compute barycentric coords of a point wrt this edge
   virtual void project_barycentric(CWpt &p, mlib::Wvec &bc)  const;

   // given barycentric coords, return a position
   virtual void bc2pos(CWvec& bc, mlib::Wpt& pos)             const;    
 
   // given barycentric coords, return a simplex 
   // (endpoint vertex, or this edge):
   virtual Bsimplex* bc2sim(CWvec& bc) const
         { return ((bc[0] >= 1) ? (Bsimplex*)_v1 :
                   (bc[1] >= 1) ? (Bsimplex*)_v2 :
                   (Bsimplex*)this); }

   // Return a list of adjacent Bsimplexes:
   // (returns v1, v2, f1 and f2, if not null)
   virtual Bsimplex_list neighbors() const;

   //******** XXX - OBSOLETE -- OLD TESSELLATOR STUFF
   virtual bool local_search(Bsimplex *&end, Wvec &final_bc,
         CWpt &target, mlib::Wpt &reached, 
         Bsimplex *repeater = 0, int iters = 30);
   virtual NDCpt nearest_pt_ndc(mlib::CNDCpt& p, mlib::Wvec &bc, int &is_on_simplex) const;
   virtual Wpt   nearest_pt(mlib::CWpt& p, mlib::Wvec &bc, bool &is_on_simplex) const;

 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   // order of verts/faces is not significant
   Bvert*       _v1;    // first vertex
   Bvert*       _v2;    // 2nd vertex
   Bface*       _f1;    // first adjacent face
   Bface*       _f2;    // 2nd one
   Bface_list*  _adj;   // additional faces

   // last frame number when this edge was checked to see if it was a
   // silhouette:
   uint     _sil_stamp; 

   //******** INTERNAL METHODS ********
   // methods for recomputing cached values when needed:
   void  set_convex();  
};


/*****************************************************************
 * Bedge inline methods
 *****************************************************************/
inline Bedge*
bedge(Bsimplex* sim)
{
   return is_edge(sim) ? (Bedge*)sim : 0;
}

/*****************************************************************
 * Bedge filters
 *****************************************************************/
class CreaseEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_crease();
   }
};

class SharpEdgeFilter : public SimplexFilter {
   // accept edges with sufficiently large angle between
   // adjacent face normals
 public:
   SharpEdgeFilter(double min_angle_deg=60) : _max_dot(0) {
      set_angle(min_angle_deg);
   }

   void set_angle(double min_angle_deg) {
      _max_dot = cos(deg2rad(min_angle_deg));
   }

   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && (((Bedge*)s)->dot() < _max_dot);
   }

 protected:
   double _max_dot;   // max acceptable dot() value
};

class InteriorEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_interior();
   }
};

class BorderEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_border();
   }
};

class PolylineEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_polyline();
   }
};

class MultiEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_multi();
   }
};

class CanPromoteEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->can_promote();
   }
};

class CrossableEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_crossable();
   }
};

class UncrossableEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && !((Bedge*)s)->is_crossable();
   }
};

class PrimaryEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_primary();
   }
};

class SecondaryEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_secondary();
   }
};

class ConvexEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_convex();
   }
};

class ConcaveEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && !((Bedge*)s)->is_convex();
   }
};

class WeakEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_weak();
   }
};

class StrongEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_strong();
   }
};

// Accept an edge if it has an adjacent front-facing triangle
class FrontFacingEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && (((Bedge*)s)->frontfacing_face() != 0);
   }
};

class StressedEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_stressed();
   }
};

class ConsistentEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->consistent_orientation();
   }
};

class SilEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_sil();
   }
};

class StrongPolyCreaseEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_strong_poly_crease();
   }
};

// used in defining blend weights between patches:
class PatchBlendBoundaryFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const;
};

class ManifoldEdgeFilter : public SimplexFilter {
 public:
   ManifoldEdgeFilter(CSimplexFilter& f) : _filter(f) {}

   // Accepts edges with <= 2 adjacent faces of a given type.
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->nfaces_satisfy(_filter) <= 2;
   }

 protected:
   CSimplexFilter& _filter;       // accepts faces of the given type
};

class BoundaryEdgeFilter : public SimplexFilter {
 public:
   BoundaryEdgeFilter(CSimplexFilter& f) : _filter(f) {}

   // Accepts edges along the boundary between faces of a
   // given type and faces not of that type.
   virtual bool accept(CBsimplex* s) const;

 protected:
   CSimplexFilter& _filter;       // accepts faces of the given type
};

/*****************************************************************
 * NewSilEdgeFilter:
 *
 *   Accepts silhouette edges that have not been detected already
 *   in the current frame.
 *****************************************************************/
class NewSilEdgeFilter : public SimplexFilter {
 public:
   NewSilEdgeFilter(uint frame_number, bool skip_sec=true) :
      _stamp(frame_number), _skip_secondary(skip_sec) {}

   virtual bool accept(CBsimplex* s) const {
      if (!is_edge(s))                          // reject if non-edge
         return false;
      Bedge* e = (Bedge*)s;
      if (e->sil_stamp() == _stamp)             // reject if previously checked
         return 0;
      e->set_sil_stamp(_stamp);                 // mark as checked this frame
      if (_skip_secondary && e->is_secondary()) // reject secondary edges as needed
         return false;
      return e->is_sil();                       // accept if silhouette
   }

 protected:
   uint  _stamp;                // frame number of current frame
   bool  _skip_secondary;       // if true, skip "secondary" edges
};

/*****************************************************************
 * ChainTipEdgeFilter:
 *
 *   Given a kind of filter, screens edges for those that match the
 *   filter and also do not lie in the middle of a chain of edges
 *   that match the filter. I.e. finds edges at the tips of chains.
 *****************************************************************/
class ChainTipEdgeFilter : public SimplexFilter {
 public:
   ChainTipEdgeFilter(CSimplexFilter& f) : _filter(f) {}

   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && ((Bedge*)s)->is_chain_tip(_filter);
   }
 protected:
   CSimplexFilter& _filter;
};

/************************************************************
 * Bedge_list
 ************************************************************/
typedef const Bedge_list CBedge_list;
class Bedge_list : public SimplexArray<Bedge_list,Bedge*> {
 public:

   //******** MANAGERS ********

   Bedge_list(CARRAY<Bedge*>& list) : SimplexArray<Bedge_list,Bedge*>(list) {}
   explicit Bedge_list(int n=0)     : SimplexArray<Bedge_list,Bedge*>(n)    {}
   explicit Bedge_list(Bedge* e)    : SimplexArray<Bedge_list,Bedge*>(e)    {}

   //******** EDGE FLAGS ********

   void clear_flag02() const {
      for (int i=0; i<_num; i++)
         _array[i]->clear_flag02();
   }

   //******** EDGE LENGTHS ********

   // Returns the sum of the edge lengths:
   double total_length() const {
      double ret = 0;
      for (int i=0; i<_num; i++)
         ret += _array[i]->length();
      return ret;
   }

   // Returns average length of the edges:
   double avg_len() const { return empty() ? 0 : total_length()/_num; }

   // minimum edge length
   double min_edge_len() const {
      if (empty())
         return 0;
      double ret = first()->length();
      for(int i=1; i<_num; i++)
         ret = min(ret,_array[i]->length());
      return ret;
   }

   // Return true if all edges have the same number of adjacent
   // primary faces:
   bool nfaces_is_equal(int num_faces) const {
      if (empty())
         return false;
      for (int i=0; i<_num; i++)
         if (_array[i]->nfaces() != num_faces)
            return false;
      return true;
   }

   void inc_crease_vals(ushort max_val = USHRT_MAX) const {
      for (int i=0; i<num(); i++)
         _array[i]->inc_crease(max_val);
   }

   void dec_crease_vals(ushort max_val = USHRT_MAX) const {
      for (int i=0; i<num(); i++)
         _array[i]->dec_crease(max_val);
   }

   // Returns list of verts contained in these edges:
   Bvert_list get_verts() const;

   // Returns list of faces adjacent to these edges:
   Bface_list get_faces() const;

   // Returns list of primary faces adjacent to these edges:
   // (defined inline in Bface.H)
   Bface_list get_primary_faces() const;        

   // Returns Bverts of this edge list that are "fold"
   // vertices WRT these edges (see bedge.C):
   Bvert_list fold_verts() const;

   // Check to see if a set of edges is simple,
   // i.e. it doesn't contain any self-intersections
   bool is_simple() const;

   Bedge_list strong_edges() const { return filter(StrongEdgeFilter()); }
   Bedge_list weak_edges()   const { return filter(WeakEdgeFilter()); }

   //******** PRIMARY/SECONDARY EDGES ********

   Bedge_list primary_edges()   const { return filter(PrimaryEdgeFilter()); }
   Bedge_list secondary_edges() const { return filter(SecondaryEdgeFilter()); }

   bool is_primary()    const { return all_satisfy(PrimaryEdgeFilter()); }
   bool is_secondary()  const { return all_satisfy(SecondaryEdgeFilter());}

 protected:
   //******** PROTECTED METHODS ********
   void clear_vert_flags() const;

   // Set the flag of each edge to 1, and clear the flags of
   // each edge around the boundary of this set of edges:
   void mark_edges() const;
};

#endif  // BEDGE_H_HAS_BEEN_INCLUDED

// end of file bedge.H
