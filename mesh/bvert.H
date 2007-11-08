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
 * bvert.H
 **********************************************************************/
#ifndef BVERT_H_HAS_BEEN_INCLUDED
#define BVERT_H_HAS_BEEN_INCLUDED

#include "disp/color.H"
#include "disp/bbox.H"
#include "mesh/bedge.H"
#include "mesh/bmesh_curvature.H"

/**********************************************************************
 * Bvert:
 *
 *      A vertex of a mesh. Has position and knows its adjacent edges.
 *      Derived from Bsimplex.
 **********************************************************************/
class Bvert : public Bsimplex {
 public:
   //******** BOOLEAN BIT FLAGS ********
   // used to store boolean states (see Bsimplex class):
   enum {
      VALID_COLOR_BIT = Bsimplex::NEXT_AVAILABLE_BIT, // color has been set
      VALID_NORMAL_BIT,       // normal is up-to-date
      VALID_CCW_BIT,          // edges are in CCW order
      NON_MANIFOLD_BIT,       // an adjacent edge is a multi-edge (cached value)
      VALID_NON_MANIFOLD_BIT, // previous bit is believable
      CREASE_BIT,             // some adjacent edge is a crease (cached value)
      VALID_CREASE_BIT,       // previous bit is believable
      STRESSED_BIT,           // some adjacent edge is stressed (cached value)
      VALID_STRESSED_BIT,     // previous bit is believable
      NEXT_AVAILABLE_BIT      // derived classes start here
   };
   
   //******** MANAGERS ********
   Bvert(CWpt& p=mlib::Wpt::Origin()) : _loc(p), _adj(6), _alpha(1) { }
   virtual ~Bvert();

   //******** ACCESSORS ********

   CWpt&   loc() const   { return _loc; }         ///< position (location)
   Wpt     wloc()const;                           ///< mesh->xf * loc()
   NDCZpt  ndc() const;                           ///< NDCZ position
   PIXEL   pix() const   { return mlib::PIXEL(wloc());} ///< position in mlib::PIXEL coords

   int     degree()    const   { return _adj.num(); }   ///< number of edges
   CCOLOR& color()     const   { return _color; }       ///< color
   double  alpha()     const   { return _alpha; }       ///< transparency
   bool    has_color() const   { return is_set(VALID_COLOR_BIT); }

   // list of adjacent edges:
   CBedge_list& get_adj() const { return _adj; }

   // Clear flag of this vertex and edges and faces containing it:
   // (clearing propagates from dim 0 to dim 2)
   void clear_flag02() {
      clear_flag();
      _adj.clear_flag02(); // edges propagate it to faces
   }

   //******** NORMALS ********

   // Computes an (angle-weighted) average normal from the given
   // set of faces, WRT this vertex. Any face that does not contain
   // this vertex makes no contribution to the final result.
   Wvec compute_normal(const ARRAY<Bface*>& faces) const;

   /// normal -- i.e. unit length average of adjacent faces:
   ///   (uses angle-weighted face normals)
   CWvec& norm() const {
      if (!is_set(VALID_NORMAL_BIT))
         ((Bvert*)this)->set_normal();
      return _norm;
   }

   // Assign a normal explicitly. Note that if vertices are moved,
   // nearby normals will be recomputed by averaging face normals.
   // XXX - does not apply to vertices on creases
   void set_norm(Wvec n) {
      _norm = n.normalized();
      set_bit(VALID_NORMAL_BIT); 
   }

   /// return a normal WRT faces accepted by the given filter:
   Wvec norm(CSimplexFilter& f) const;

   /// normal transformed to world space
   /// xf.inverse().transpose() * norm
   Wvec wnorm() const;
   
   /// for a vertex with adjacent crease edges, returns 1 normal
   /// for each set of adjacent faces bounded by crease edges:
   void get_normals(ARRAY<Wvec>& norms) const;

   //******** LOCATION, COLOR, CURVATURE, etc. ********

   // setting location:
   void set_loc(CWpt& p)        { _loc = p; geometry_changed(); }
   void offset_loc(CWvec& v)    { set_loc(_loc + v); }
   void transform(CWtransf& xf) { set_loc(xf*_loc); notify_xform(xf); }

   // setting color:
   void set_color(CCOLOR &c, double a=1) {
      _color = c;
      _alpha = a;
      set_bit(VALID_COLOR_BIT,1);
      color_changed();
   }

   /// Distance to another vert (ignoring modeling transform):
   double dist(CBvert* v)  { return v ? loc().dist(v->loc()) : 0.0; }

   /// Distance to another vert (computed in world space):
   double wdist(CBvert* v) { return v ? wloc().dist(v->wloc()) : 0.0; }

   //******** ADJACENCY ********

   // Adjacent edge number k:
   Bedge* e  (int k) const   { return _adj[k]; }

   // Neighboring vertex number k:
   Bvert* nbr(int k) const {
      return _adj.valid_index(k) ? _adj[k]->other_vertex(this) : 0;
   }
   // Neighboring vertex along given edge (which must be adjacent to this):
   Bvert* nbr(CBedge* e) const { return e ? e->other_vertex(this) : 0; }

   // Return a set of neighboring vertices from the
   // given set of edges, all of which must contain
   // this vertex (or an assertion failure will occur):
   Bvert_list get_nbrs(CBedge_list& edges) const;

   // Like previous 2 methods, but return neighbor location.
   // Must be valid or assertion fails:
   Wpt nbr_loc(int k)     const { Bvert* n = nbr(k); assert(n); return n->loc(); }
   Wpt nbr_loc(CBedge* e) const { Bvert* n = nbr(e); assert(n); return n->loc(); }

   // NB: The following methods for getting adjacent faces 
   //     select faces just from the primary layer of the mesh.
   //     The exception is get_all_faces().

   // Adjacent faces in star of this vertex:
   void get_faces(ARRAY<Bface*>& ret) const;

   // like above, but returns the list by copying
   // defined in bface.H
   Bface_list get_faces() const;

   // Return a list of quads in the star of this vertex.
   // (Each quad is represented by a single Bface -- the "quad rep").
   void get_quad_faces(ARRAY<Bface*>& ret) const;

   // Return a list of faces in the star of this vertex,
   // including both faces of every quad adjacent to this vertex.
   void get_q_faces(ARRAY<Bface*>& ret) const;

   // Return ALL faces incident to the vertex,
   // including faces adjacent to non-manifold ("multi") edges:
   void  get_all_faces(ARRAY<Bface*>& ret) const;

   Bface_list get_all_faces() const; // defined in bface.H

   int num_tris() const;
   int num_quads() const;

   // Return the edge connecting this to a given vertex:
   Bedge* lookup_edge(CBvert* v) const {
      if (v == this)
         return 0;  
      for(int k=0; k<_adj.num(); k++)
         if(_adj[k]->contains(v))
            return _adj[k];
      return 0;
   }

   // tell if this is adjacent to the given vertex:
   bool is_adjacent(CBvert* v) const {
      for(int k=0; k<_adj.num(); k++)
         if(v == _adj[k]->other_vertex(this))
            return 1;
      return 0;
   }

   // Return neighboring vertices in an array
   void get_nbrs(ARRAY<Bvert*>& nbrs) const;

   // The following assume edges can be put in CCW order,
   // meaning the vertex is either surrounded by faces, or
   // has exactly 2 border edges.
   //
   // XXX - not handling non-manifold surfaces yet
   //
   // Return list of edges in CCW order:
   // returns an empty list if the operation is invalid.
   Bedge_list get_ccw_edges() const;

   // Returns list of neighbor verts in CCW order:
   Bvert_list get_ccw_nbrs() const;

   // Return adjacent edges the normal way, unless it's a
   // "multi" vertex; then return primary edges. Used in
   // computing centroids, e.g. for subdivision.
   void get_manifold_edges(ARRAY<Bedge*>& ret) const;

   // Same as above but returns list by copying
   Bedge_list get_manifold_edges() const {
      Bedge_list ret;
      get_manifold_edges(ret);
      return ret;
   }

   Bedge_list strong_edges() const { return get_manifold_edges().strong_edges(); }
   double avg_strong_len()   const { return strong_edges().avg_len(); }

   // Return neighboring vertices from the "primary" layer.
   // I.e., neighboring vertices connected to this one by
   // an edge that has one or more adjacent faces with
   // Bface::SECONDARY_BIT not set in its flag.
   void get_primary_nbrs(ARRAY<Bvert*>& nbrs) const;

   // Get neighbors the normal way, unless it's a "multi"
   // (non-manifold) vertex; then get primary neighbors. Used in
   // computing centroids, e.g. for subdivision.
   void  get_p_nbrs(ARRAY<Bvert*>& nbrs) const;

   // Same as above but returns list by copying
   ARRAY<Bvert*> get_p_nbrs() const {
      ARRAY<Bvert*> ret;
      get_p_nbrs(ret);
      return ret;
   }

   int p_degree() const {
      return is_manifold() ? degree() : get_p_nbrs().num();
   }

   // Return "quad opposite vertices" in an array
   void get_q_nbrs(ARRAY<Bvert*>& q) const;

   // Given that this vertex lies on a chain of border edges,
   // (with exactly 2 adjacent border edges to this vertex),
   // return the next border edge when traversing the border
   // chain in CW order around the adjacent surface.
   Bedge* next_border_edge_cw();

   // Like above, but returns the next border vertex:
   Bvert* next_border_vert_cw() {
      Bedge* border = next_border_edge_cw();
      return border ? border->other_vertex(this) : 0;
   }               

   //******** BUILDING/REDEFINING ********
   void operator +=(Bedge* e);
   int  operator -=(Bedge* e);

   // The following assumes edges can be put in CCW order,
   // meaning the vertex is either surrounded by faces, or
   // has exactly 2 border edges.
   //
   // XXX - not handling non-manifold surfaces yet
   //
   // put edges of this vertex in CCW order:
   // returns true on success
   bool order_edges_ccw();

   //******** GEOMETRIC MEASURES ********
   // returns sum of "interior" angles of faces around vertex:
   double star_sum_angles() const;

   // returns sum of face areas around vertex:
   double star_area() const;

   double min_dot() const {
      double ret = 1;
      for (int i=0; i < degree(); i++)
         ret = min(ret, e(i)->dot());
      return ret;
   }   

   // minimum edge length
   double min_edge_len() const { return _adj.min_edge_len(); }

   // average edge length
   double avg_edge_len() const { return _adj.avg_len(); }

   //******** CENTROIDS ********

   // Return the average of neighboring vertices
   Wpt centroid() const;

   // Area weighted centroid:
   Wpt area_centroid() const;

   // Special kind of centroid that weights regular ("r")
   // neighbors differently from quad ("q") neighbors:
   Wpt  qr_centroid()   const;
   Wvec qr_delt()       const   { return qr_centroid() - loc(); }
   
   //******** CURVATURE ********
   
   BMESHcurvature_data::curv_tensor_t curv_tensor() const;
   BMESHcurvature_data::diag_curv_t diag_curv() const;
   double k1() const;
   double k2() const;
   Wvec pdir1() const;
   Wvec pdir2() const;
   BMESHcurvature_data::dcurv_tensor_t dcurv_tensor() const;

   //******** DEGREE FOR VARIOUS EDGE TESTS ********

   // Return "degree" of this vertex with respect to edges of a
   // given type
   int degree(CSimplexFilter& f) const { return _adj.num_satisfy(f); }

   int  crease_degree()   const { return degree(CreaseEdgeFilter()); }
   int  border_degree()   const { return degree(BorderEdgeFilter()); }
   int  polyline_degree() const { return degree(PolylineEdgeFilter()); }
   int  stressed_degree() const { return degree(StressedEdgeFilter()); }
   int  multi_degree()    const { return degree(MultiEdgeFilter()); }

   //******** VERTEX TESTS ********

   bool is_border()       const { return (border_degree() > 0); }
   bool is_polyline_end() const { return (polyline_degree() % 2) == 1; }
   bool is_crease_end()   const { return (crease_degree()   % 2) == 1; }

   bool is_stressed()     const { 
      if (!is_set(VALID_STRESSED_BIT)) {
         ((Bvert*)this)->set_bit(VALID_STRESSED_BIT);
         ((Bvert*)this)->set_bit(STRESSED_BIT, stressed_degree() > 0);
      }
      return is_set(STRESSED_BIT);
   }
   bool is_crease() const {
      if (!is_set(VALID_CREASE_BIT)) {
         ((Bvert*)this)->set_bit(VALID_CREASE_BIT,1);
         ((Bvert*)this)->set_bit(CREASE_BIT, crease_degree() > 0);
      }
      return is_set(CREASE_BIT);
   }

   /// camera (eye) location in local coordinates:
   Wpt eye_local() const;

   /// unit length vector to camera in local coordinates
   Wvec eye_vec() const { return (eye_local() - loc()).normalized(); }

   /// dot product of unit length vector toward camera
   /// and surface normal (ignoring creases):
   double eye_vec_dot_norm() const { return eye_vec() * norm(); }

   /// returns true if surface normal (ignoring creases) points toward camera:
   bool is_front_facing() const { return (eye_local() - loc()) * norm() > 0; }

   bool is_sharp_point(double sharpness=M_PI/2);

   bool is_non_manifold()       const { return multi_degree() > 0; }
   bool is_manifold()           const { return !is_non_manifold(); }

   // within the star of this vertex, return the number of
   // faces that satisify the given filter:
   int face_degree(CSimplexFilter& f) const;

   //******** MANAGING CACHED VALUES ********
   virtual void geometry_changed();
   virtual void normal_changed();
   virtual void degree_changed();
   virtual void star_changed()    { clear_bit(VALID_CCW_BIT); }
   virtual void crease_changed()  { clear_bit(VALID_CREASE_BIT); }
   virtual void color_changed()   { set_bit(VALID_COLOR_BIT); }
    
   //******** I/O ********
   friend ostream &operator <<(ostream &os, CBvert &v) {
      return os << v.loc()[0] << " "
                << v.loc()[1] << " "
                << v.loc()[2] << endl;
   }
   friend istream &operator >>(istream &is,  Bvert &v) {
      double x,y,z;
      is >> x >> y >> z;
      v.set_loc(Wpt(x,y,z));
      return is;
   }
   // shift operators for STDdstream
   friend STDdstream &operator <<(STDdstream &d, CBvert &v) {
      return d << v.loc()[0] << v.loc()[1] << v.loc()[2];
   }
   friend STDdstream &operator >>(STDdstream &d,  Bvert &v) {
      double x,y,z;
      d >> x >> y >> z;
      v.set_loc(Wpt(x,y,z));
      return d;
   }

   //******** Bsimplex VIRTUAL METHODS ********

   virtual int dim() const { return 0; }

   // index in BMESH Bedge array:
   virtual int  index() const;
   
   // (Bsimplex virtual method):
   // Intersection w/ ray from given screen point -- returns the point
   // on the Bvert that is nearest to the given screen space point.
   // Note: the returned "near point" and "normal" are both
   //       transformed to world space.
   virtual bool view_intersect(
      CNDCpt&,  // Given screen point. Following are returned:
      Wpt&,     // Near point on the simplex IN WORLD SPACE (not object space)
      double&,  // Distance from camera to near point
      double&,  // Screen distance (in PIXELs) from given point
      Wvec& n   // "normal" vector at intersection point IN WORLD SPACE
      ) const;

   virtual Bface* get_face()            const;
   virtual bool   on_face(CBface* f)    const;

   virtual void project_barycentric(CWpt&, mlib::Wvec& bc)  const {
      bc.set(1,0,0);
   }
   virtual void bc2pos(CWvec&, mlib::Wpt& pos) const { pos = _loc; }

   virtual Bsimplex* bc2sim(CWvec&) const { return (Bsimplex*)this; }

   // Return a list of adjacent Bsimplexes:
   // (returns edges and faces):
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
   Wpt          _loc;       // location in 3-space
   Bedge_list   _adj;       // list of adjacent edges
   COLOR        _color;     // color of vertex
   double       _alpha;     // alpha value for _color
   Wvec         _norm;      // unit-length average of face normals
  

   //******** INTERNAL METHODS ********
   // methods for recomputing cached values when needed:
   void set_normal();
};

inline Bedge*
lookup_edge(CBvert* a, CBvert* b)
{
   return (a && b) ? a->lookup_edge(b) : 0;
}

inline Bvert*
bvert(Bsimplex* sim)
{
   return is_vert(sim) ? (Bvert*)sim : 0;
}

inline Wpt 
Bedge::interp(double s) const  
{
   // s is the parameter varying from 0 to 1 along the edge from
   // _v1 to _v2. return corresponding point.

   return _v1->loc() + vec()*s; 
}

/************************************************************
 * Bvert_list
 ************************************************************/
class Bvert_list : public SimplexArray<Bvert_list,Bvert*> {
 public:

   //******** MANAGERS ********

   explicit Bvert_list(int n=0)     : SimplexArray<Bvert_list,Bvert*>(n)    {}
   explicit Bvert_list(Bvert* v)    : SimplexArray<Bvert_list,Bvert*>(v)    {}
   Bvert_list(CARRAY<Bvert*>& list) : SimplexArray<Bvert_list,Bvert*>(list) {}

   //******** STATICS ********

   // return a list of 1, 2, or 3 vertices depending
   // on whether s is a vertex, edge, or face:
   static Bvert_list get_verts(Bsimplex* s);

   // return the neighbors of v:
   // XXX - would be better to make this a method of Bvert,
   //       but Bvert_list is defined after Bvert
   static Bvert_list get_nbrs(Bvert* v) {
      if (!v) return Bvert_list();
      Bvert_list ret(v->degree());
      v->get_nbrs(ret);
      return ret;
   }

   // starting from the given simplex, return
   // the list of vertices reachable from s:
   static Bvert_list connected_verts(Bsimplex* s);

   //******** CONVENIENCE ********

   // Clear flags of vertices and adjacent edges and faces
   // (from dimension 0 to 2)
   void clear_flag02() const {
      for (int i=0; i<_num; i++)
         _array[i]->clear_flag02();
   }

   // Return corresponding list of Wpts (object space)
   Wpt_list pts() const {
      Wpt_list ret(_num);
      for (int i=0; i<_num; i++) 
         ret += _array[i]->loc();
      ret.update_length();
      return ret;
   }

   // Return corresponding list of Wpts (world space)
   Wpt_list wpts() const {
      Wpt_list ret(_num);
      for (int i=0; i<_num; i++) 
         ret += _array[i]->wloc();
      ret.update_length();
      return ret;
   }

   // Return average of point locations in object space
   Wpt center() const { return pts().average(); }

   // Return sum of point locations in object space
   Wpt sum() const { return pts().sum(); }

   // apply a transform to each vertex:
   void transform(CWtransf &xf) {
      for (int i=0; i<num(); i++)
         (*this)[i]->transform(xf);
   }

   // return the bounding box of the vertices in object space:
   BBOX get_bbox_obj();

   // return the bounding box of the vertices in world space:
   BBOX get_bbox_world();

   //******** TOPOLOGY ********

   // Returns the 1-ring faces of a set of vertices:
   Bface_list one_ring_faces() const;
   
   // Returns the 2-ring faces of a set of vertices:
   Bface_list two_ring_faces() const;

   // Return the set of edges containing at least 1 vertex
   // from this set
   Bedge_list get_outer_edges() const;

   // Return the set of edges that connect 2 vertices
   // from this set
   Bedge_list get_inner_edges() const;

   //******** EDGE CHAINS ********

   // Convenience methods for when a list of vertices
   // actually forms a chain connected by edges.

   // Are the vertices joined sequentially by edges in a
   // connected chain?
   bool forms_chain()        const;
   bool forms_closed_chain() const;     // and first connects to last

   // Returns the chain of edges formed by the vertices
   // (or the empty list). If 'try_close' is true and there
   // is an edge from the last vertex to the first, that
   // edge is added to the output list.
   Bedge_list get_chain(bool try_close=false) const;

   // Same as get_chain(), but includes the edge joining first and
   // last vertices. Returns the empty list if it failed:
   Bedge_list get_closed_chain() const;

 protected:

   //******** UTILITIES ********

   // used in connected_verts():
   static void add_verts_recursively(Bvert_list& L, Bvert* v);
   static void add_verts_recursively(Bvert_list& ret, const Bvert_list& L);

};
typedef const Bvert_list CBvert_list;

#endif // BVERT_H_HAS_BEEN_INCLUDED

// end of file bvert.H
