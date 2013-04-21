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
 * bface.H
 **********************************************************************/
#ifndef BFACE_H_HAS_BEEN_INCLUDED
#define BFACE_H_HAS_BEEN_INCLUDED

#include "bvert.H"
//#include "vert_attrib.H"

class EdgeStrip;
class Patch;
/**********************************************************************
 * Bface:
 *
 *      3-sided face, vertices listed in CCW order.
 * <pre>
 *                    v3
 *                   /  \
 *                  /    \
 *                 /      \
 *               e3       e2
 *               /          \
 *              /            \
 *             /              \
 *           v1 -------e1------ v2
 * </pre>
 **********************************************************************/
class Bface  : public Bsimplex {
 public:

   //******** FLAG BITS ********
   // Used to store boolean states (see Bsimplex class):
   enum {
      BAD_BIT = Bsimplex::NEXT_AVAILABLE_BIT, // set in constructor if error
      VALID_NORMAL_BIT,                       // tells if normal is up-to-date
      FRONT_FACING_BIT,                       // tells if it's front-facing
      SECONDARY_BIT,                          // face is not in primary layer
      NEXT_AVAILABLE_BIT                      // derived classes start here
   };

   //******** MANAGERS ********
   Bface(Bvert*, Bvert*, Bvert*, Bedge*, Bedge*, Bedge*);
   virtual ~Bface();

   //******** ACCESSORS ********
   // vertices:
   Bvert *v1()     const  { return _v1; }
   Bvert *v2()     const  { return _v2; }
   Bvert *v3()     const  { return _v3; }
   Bvert *v(int i) const  { return (i>0&&i<4)?(&_v1)[i-1]:0; } // i in [1..3]

   // edges:
   Bedge *e1()     const  { return _e1; }
   Bedge *e2()     const  { return _e2; }
   Bedge *e3()     const  { return _e3; }
   Bedge *e(int i) const  { return (i>0&&i<4)?(&_e1)[i-1]:0; } // i in [1..3]

   // Face normal (cached value -- recomputed as needed):
   CWvec& norm() const {
      if (!is_set(VALID_NORMAL_BIT))
         ((Bface*)this)->set_normal();
      return _norm;
   }

   // Vertex normal - average of face normals around vertex, not
   // crossing any crease edges to reach the other faces, starting
   // from this one. 2nd version returns result by copying.
   CWvec& vert_normal(CBvert* v, Wvec& n) const;
   Wvec vert_normal(CBvert* v) const {
      Wvec n;
      return vert_normal(v, n);
   }

   // Returns 1 if this face faces the camera in the current frame:
   int front_facing() const;
   // for zx silhouettes
   bool zx_mark() const;       //mark face as visited ( with view::stamp )     
   bool zx_query() const;      //has face been visited? ( == view::stamp )

   Patch* patch()  const  { return _patch; }

   // for tri-stripping
   // XXX - should rename
   void   orient_strip(Bvert* a) { _orient = a; }
   Bvert* orient_strip() const { return _orient; }

   // return the vertex's "index." i.e., 1, 2, or 3 according to
   // whether it is vertex _v1, _v2, or _v3, respectively:
   int vindex(CBvert* v) const {
      return (v==_v1) ? 1 : (v==_v2) ? 2 : (v==_v3) ? 3 : -1;
   }

   //******** ADJACENCY ********
   bool contains(CBvert* v) const { return (v==_v1 || v==_v2 || v==_v3); }
   bool contains(CBedge* e) const { return (e==_e1 || e==_e2 || e==_e3); }
   bool contains(CBsimplex* s) const {
      if (s->dim()==1) return contains( (CBedge*)s );
      if (s->dim()==0) return contains( (CBvert*)s );
      return 0;
   }
   bool contains(CWpt& pt, double threshold=0) const ;

   Bvert* other_vertex(CBvert* u, CBvert* v) const {
      return (!(contains(u) && contains(v)) ? 0 :
              (u==_v1) ? ((v==_v2) ? _v3 : (v==_v3) ? _v2 : 0) :
              (u==_v2) ? ((v==_v1) ? _v3 : (v==_v3) ? _v1 : 0) :
              (u==_v3) ? ((v==_v1) ? _v2 : (v==_v2) ? _v1 : 0) :
              0);
   }
   Bvert* other_vertex(CBedge* e) const {
      return e ? other_vertex(e->v1(), e->v2()) : (Bvert*)0;
   }
   Bvert* next_vert_ccw(CBvert* u) const {
      return (u==_v1) ? _v2 : (u==_v2) ? _v3 : (u==_v3) ? _v1 : 0;
   }

   // Return the vertex of the given edge (of this face)
   // that comes first WRT this face, in CCW order:
   Bvert* leading_vert_ccw(CBedge* e) const {
      return (
         e ? ((next_vert_ccw(e->v1()) == e->v2()) ? e->v1() : e->v2()) : 0
         );
   }

   Bedge* next_edge_ccw(CBedge* u) const {
      return (u==_e1) ? _e2 : (u==_e2) ? _e3 : (u==_e3) ? _e1 : 0;
   }
   Bedge* edge_from_vert(CBvert* u) const {
      return (u==_v1) ? _e1 : (u==_v2) ? _e2 : (u==_v3) ? _e3 : 0; 
   }
   Bedge* edge_before_vert(CBvert* u) const {
      return (u==_v1) ? _e3 : (u==_v2) ? _e1 : (u==_v3) ? _e2 : 0; 
   }
   Bface* next_face_ccw(CBvert* u) const {
      return edge_before_vert(u)->other_face(this);
   }

   // neighboring face (i in [1..3]):
   Bface* nbr(int i) const {
      Bedge* ei = e(i);
      return ei ? ei->other_face(this) : 0;
   }

   // return edge shared with given face:
   Bedge* shared_edge(CBface* f) const {
      return ((_e1->other_face(this) == f) ? _e1 :
              (_e2->other_face(this) == f) ? _e2 :
              (_e3->other_face(this) == f) ? _e3 :
              0);
   }

   Bedge* opposite_edge(CBvert* a) const {
      return ((a == _v1) ? _e2 :
              (a == _v2) ? _e3 :
              (a == _v3) ? _e1 : 0
         );
   }
   Bface* opposite_face(CBvert* a) const {
      Bedge* e = opposite_edge(a);
      return e ? e->other_face(this) : 0;
   }
   Bface* next_strip_face() const {
      CBedge* e = opposite_edge(orient_strip());
      return (e && e->is_crossable()) ? e->other_face(this) : 0;
   }
   Bedge* other_edge(CBvert* v, CBedge* e) const {
      return ((_e1 != e && _e1->contains(v)) ? _e1 :
              (_e2 != e && _e2->contains(v)) ? _e2 :
              (_e3 != e && _e3->contains(v)) ? _e3 :
              0);
   }

   int orientation(CBedge* e) const {
      return (contains(e) ? (next_vert_ccw(e->v1()) == e->v2() ? 1 : -1) : 0);
   }    

   //******** FACE TESTS ********
   // set in the constructor if face could not be defined with given
   // vertices and edges:
   bool is_bad()      const  { return is_set(BAD_BIT); }

   // meshes can represent non-manifold surfaces.
   // the "primary" layer is still manifold, however.
   bool is_primary()   const  { return is_clear(SECONDARY_BIT); }
   bool is_secondary() const  { return !is_primary(); }

   ushort layer()                       const   { return _layer; }
   virtual void set_layer(ushort l)             { _layer = l; }

   // For non-manifold meshes:
   // Label the face as "primary" or "secondary." I.e., as being
   // in the primary layer or not, respectively. In Lface, the
   // same action is passed down to all child faces.
   virtual void make_primary();
   virtual void make_secondary();

   //******** GEOMETRIC MEASURES ********
   double area() const {
      if (!is_set(VALID_NORMAL_BIT))
         ((Bface*)this)->set_normal();
      return _area;
   }
   double volume_el() const {
      CWpt& a = _v1->loc();
      CWpt& b = _v2->loc();
      CWpt& c = _v3->loc();
      // nx = cross(b-a, c-a)[0];
      double nx = (b[1] - a[1])*(c[2] - a[2]) - (c[1] - a[1])*(b[2] - a[2]);
      return (nx * (a[0] + b[0] + c[0]) / 6.0);
   }
   double angle(CBvert* v) const {
      if (!contains(v))
         return 0;
      Bvert* a = next_vert_ccw(v);
      Bvert* b = next_vert_ccw(a);
      return (v->loc() - a->loc()).angle(v->loc() - b->loc());
   }    

   Wpt    centroid()     const { return (_v1->loc()+_v2->loc()+_v3->loc())/3;}
   NDCZpt ndc_centroid() const { return (_v1->ndc()+_v2->ndc()+_v3->ndc())/3;}
   Wplane  plane()       const { return Wplane(_v1->loc(), norm()); }

   double signed_area(CWpt& a, CWpt& b, CWpt& c) const {
      return 0.5 * (cross(b-a,c-a) * norm());
   }
   double signed_area(CNDCpt& a, CNDCpt& b, CNDCpt& c) const {
      return 0.5 * det(b-a,c-a);
   }

   double ndc_area() const {
      NDCpt a = _v1->ndc();
      NDCpt b = _v2->ndc();
      NDCpt c = _v3->ndc();
      return fabs(signed_area(a,b,c));
   }

 

   //******** QUAD STUFF ********

   // A quad consists of 2 triangles that share a "weak" edge.
   // None of the other edges of either face should be labelled
   // weak.  The "weak" label just means that the edge is an
   // internal, diagonal edge of a quad.
   int num_weak_edges() const {
      return (
         (_e1->is_weak() ? 1 : 0) +
         (_e2->is_weak() ? 1 : 0) +
         (_e3->is_weak() ? 1 : 0)
         );
   }
   bool is_quad() const  { return (num_weak_edges() == 1); }
   Bedge* weak_edge() const {
      return  (_e1->is_weak() ? _e1 :
               _e2->is_weak() ? _e2 :
               _e3->is_weak() ? _e3 : 0
         );
   }

   // The other Bface making up the quad:
   Bface* quad_partner() const {
      return is_quad() ? weak_edge()->other_face(this) : (Bface*)0;
   }

   // Arbitrarily (but consistently) pick one face to
   // represent the quad:
   Bface* quad_rep() const {
      return min((Bface*)this, quad_partner());
   }

   // Return true if this one is the quad rep:
   bool is_quad_rep() const { return is_quad() && quad_rep() == this; }

   Bvert* quad_vert() const {
      return is_quad() ? weak_edge()->opposite_vert(this) : (Bvert*)0;
   }
   Bvert* quad_opposite_vert(CBvert* v) const {
      Bedge* e = weak_edge();
      if (!e) return 0;
      if (e->contains(v))
         return e->other_vertex(v);
      if (contains(v))
         return e->opposite_vert(this);
      Bface* f = quad_partner();
      return f && f->contains(v) ? e->opposite_vert(f) : 0;
   }

   // Given one edge of quad, return opposite edge
   Bedge* opposite_quad_edge(CBedge* e) const {
      if (is_quad()) {
         Bvert* u1=other_vertex(e->v1(), e->v2());
         Bvert* u2=quad_vert();
         return u1->lookup_edge(u2);
      }
      else
         return 0;
   }

   //added by Karol for the quad mesh traversal
   Bface* other_quad_face(CBedge *e) const {
	   
	   
	 if (!e) return 0; //safety first
	
	   //we may be on the other quad face not directly adjacent to this edge
	   //in that case get the other quad face
	   if ((this!=e->f1())&&(this!=e->f2()))
	   {
			return e->other_face(this->quad_partner());
	   }

	   return e->other_face(this);
   }

   // Convenience. only call these on quads:
   double quad_area() const {
      return area() + quad_partner()->area();
   }
   double quad_ndc_area() const {
      return ndc_area() + quad_partner()->ndc_area();
   }
   Wpt quad_centroid() const {
      return (_v1->loc() + _v2->loc() +
              _v3->loc() + quad_vert()->loc())*0.25;
   }
   NDCZpt ndc_quad_centroid() const {
      return (_v1->ndc() + _v2->ndc() +
              _v3->ndc() + quad_vert()->ndc())*0.25;
   }
   Wvec quad_norm() const {
      return (norm() + quad_partner()->norm()).normalized();
   }

   Wvec qnorm() const { return is_quad() ? quad_norm() : norm(); }
   
   //  Return CCW verts a, b, c, d as in the picture, 
   //  orienting things so that the weak edge runs NE
   //  as shown:
   //
   //    d ---------- c = w->v2()              ^      
   //    |          / |                        |       
   //    |        /   |                        |       
   //    |    w /     |       tan1        tan2 |       
   //    |    /       |    -------->           |       
   //    |  /     f   |                        |       
   //    |/           |                                
   //    a ---------- b                                
   //   a = w->v1()
   //
   bool get_quad_verts(Bvert*& a, Bvert*& b, Bvert*& c, Bvert*& d) const;
   bool get_quad_verts(Bvert_list& verts)                          const;
   bool get_quad_pts(Wpt& a, Wpt& b, Wpt& c, Wpt& d)               const;	
   
   bool get_quad_edges(Bedge*& ab, Bedge*& bc, Bedge*& cd, Bedge*& da) const;
   bool get_quad_edges(Bedge_list& edges)                          const;


   // vectors perpendicular to the quad normal, each one aligned
   // with a pair of opposite sides of the quad:
   Wvec quad_tan1() const;      // runs right/left in diagram above
   Wvec quad_tan2() const;      // runs    up/down in diagram above

   // Based on the 4 verts in standard orientation as above,
   // return the average of the two horizontal or vertical edge
   // lengths, respectively:
   double quad_dim1()    const;    // avg length of ab and dc
   double quad_dim2()    const;    // avg length of ad and bc

   // Convenience methods related to the previous two methods:
   double quad_avg_dim() const { return (quad_dim1() + quad_dim2())/2; }
   double quad_max_dim() const { return max(quad_dim1(), quad_dim2()); }
   double quad_min_dim() const { return min(quad_dim1(), quad_dim2()); }

   // Convert barycentric coords WRT this face to uv coords
   // WRT the quad that this face is part of:
   UVpt quad_bc_to_uv(CWvec& bc) const;

   // Use bilinear interpolation on the quad vertices to map
   // the given uv coordinate (WRT the quad) to a Wpt:
   Wpt  quad_uv2loc(CUVpt& uv) const;

   //******** TEXTURE COORDINATES ********
   // texture coordinates are stored per vertex per face
   // i.e.-- a given vertex can be assigned separate texture
   // coordinates for each face that contains the vertex

   // XXX - should not be called -- use UVdata::get_uv() to
   //       lookup texture coordinates
   UVpt &tex_coord(int vert_index) const {
      // vert_index must be 1, 2 or 3.
      // explicitly warn about this little-understood fact:
      assert (vert_index > 0 || vert_index < 4);
      return (_tc?(_tc):(((Bface*)this)->_tc = new UVpt[3]))[vert_index - 1];
   }
   UVpt &tex_coord(CBvert* v) const { return tex_coord(vindex(v)); }

   // set coordinates for vertices 1, 2, 3, respectively:
   void set_tex_coords(CUVpt& a, CUVpt& b, CUVpt& c) {
      tex_coord(1) = a; tex_coord(2) = b; tex_coord(3) = c;
   }

   UVpt* tc_array() const { return _tc; }

   //******** BARYCENTRIC COORDINATES ********
   // compute barycentric coords of a point wrt this face,
   virtual void project_barycentric(CWpt& p, Wvec& ret) const {
      double u = signed_area(p, _v2->loc(), _v3->loc()) / area();
      double v = signed_area(_v1->loc(), p, _v3->loc()) / area();
      ret.set(u, v, 1 - u - v);
   }
   // compute barycentric coords of a point wrt this face,
   // measuring vertex positions in NDC coordinates
   void project_barycentric_ndc(CNDCpt& p, Wvec& ret) const {
      NDCpt a = _v1->ndc(), b = _v2->ndc(), c = _v3->ndc();
      double A = signed_area(a, b, c);
      double u = signed_area(p, b, c) / A;
      double v = signed_area(a, p, c) / A;
      ret.set(u, v, 1 - u - v);
   }

   virtual void bc2pos(CWvec& bc, Wpt& pos) const {
      pos = (bc[0]*_v1->loc()) + (bc[1]*_v2->loc()) + (bc[2]*_v3->loc());
   }
   Wpt bc2pos(mlib::CWvec& bc) const {
      Wpt ret;
      bc2pos(bc, ret);
      return ret;
   }

   virtual void bc2norm_blend(CWvec& bc, Wvec& vec) const {
      Wvec v1,v2,v3;
      vert_normal(_v1,v1);
      vert_normal(_v2,v2);
      vert_normal(_v3,v3);
      vec = ((bc[0]*v1) +
             (bc[1]*v2) +
             (bc[2]*v3)).normalized();
   }

   virtual Bsimplex* bc2sim(CWvec& bc) const;

   // returns the closest vertex to the barycentric coords
   Bvert* bc2vert(CWvec& bc) const {
      double max = 0; 
      int best_v;
      for (int i=0; i<3; i++)
         if (bc[i] >= max) {
            max = bc[i];
            best_v = i;
         }
      return v(best_v+1);
   }

   // returns the closest edge to the barycentric coords
   Bedge* bc2edge(CWvec& bc) const {
      double min = 1; 
      int best_v;
      for (int i=0; i<3; i++)
         if (bc[i] <= min) {
            min = bc[i];
            best_v = i;
         }
      return opposite_edge(v(best_v+1));
   }

   Wvec bc2norm(CWvec& bc) const {
      Bsimplex* sim = bc2sim(bc);
      return (is_vert(sim) ? ((Bvert*)sim)->norm() :
              is_edge(sim) ? ((Bedge*)sim)->norm() :
              norm()
         );
   }

   //******** INTERSECTION ********

   // (Bsimplex virtual method):
   // Intersection w/ ray from given screen point -- returns the point
   // on the Bface that is nearest to the given screen space point.
   // Note: the returned "near point" and "normal" are both
   //       transformed to world space.
   virtual bool view_intersect(
      CNDCpt&,  // Given screen point. Following are returned:
      Wpt&,     // Near point on the simplex IN WORLD SPACE (not object space)
      double&,  // Distance from camera to near point
      double&,  // Screen distance (in PIXELs) from given point
      Wvec& n   // "normal" vector at intersection point IN WORLD SPACE
      ) const;

   // similar to view_intersect; just returns the hit point in world space:
   Wpt approx_hit(CNDCpt& ndc) {
      Wpt hit;
      double d1, d2; Wvec v;
      view_intersect(ndc, hit, d1, d2, v); // always returns true
      return hit;
   }

   // Similar to above, but returns the hit point 
   // both in local and barycentric coords:
   Wpt near_pt(CNDCpt& ndc, Wvec& hit_bc) const;

   // Ditto, but hold the barycentric coords:
   Wpt near_pt(CNDCpt& ndc) const {
      Wvec hit_bc;
      return near_pt(ndc, hit_bc);
   }

   // Ray intersection: returns true if given ray exactly intersects
   // the face:
   bool ray_intersect(CWpt&, CWvec&, Wpt& ret, double& depth) const;
   bool ray_intersect(CWline& ray, Wpt& ret, double& depth) const {
      return ray_intersect(ray.point(), ray.vector(), ret, depth);
   }
   // Similar to above, but return barycentric coords of hit point:
   bool ray_intersect(CWline& ray, Wvec& bc) const {
      Wpt hit; double depth;
      if (ray_intersect(ray, hit, depth)) {
         project_barycentric(hit, bc);
         return 1;
      }
      return 0;
   }

   // Intersect a line with the plane of the face:
   Wpt plane_intersect(CWline& line) const {
      return plane().intersect(line);
   }
   Wpt plane_intersect(CNDCpt& ndc) const {
      return plane_intersect(Wline(XYpt(ndc)));
   }
   Bsimplex* find_intersect_sim(CNDCpt& target, Wpt& hit_pt) const;

   bool ndc_contains(CNDCpt &p) {
      NDCpt a(_v1->ndc());
      NDCpt b(_v2->ndc());
      NDCpt c(_v3->ndc());
      return (det(b-a, p-a) * det(p-a, c-a) >= 0) &&
         (det(c-b, p-b) * det(p-b, a-b) >= 0);
   }
   
   Bface* plane_walk(Bedge* cur_edge, CWplane& plane, Bedge*& next_edge) const;

   // Returns the NDC point on this face that is nearest the given
   // NDC point; optionally also returns the corresponding (NDC)
   // barycentric coords and a flag indicating if the original
   // screen point was inside the triangle:
   NDCpt nearest_pt_ndc(CNDCpt& p, Wvec &bc, int &is_on_tri) const;
   NDCpt nearest_pt_ndc(CNDCpt& p, Wvec &bc) const {
      int i; return nearest_pt_ndc(p, bc, i);
   }
   NDCpt nearest_pt_ndc(CNDCpt& p) const {
      Wvec bc; return nearest_pt_ndc(p, bc);
   }

   // Returns the object-space point on this face that is nearest
   // the given object-space point p; optionally also returns the
   // corresponding barycentric coords and a flag indicating if the
   // original point p projects to the interior of the triangle:
   virtual Wpt nearest_pt(CWpt& p, Wvec &bc, bool &is_on_tri) const;

   //******** BUILDING/REDEFINING ********
   virtual int  detach();
   virtual void reverse();

   int  redefine(Bvert *v, Bvert *u);
   int  redefine(Bvert *u, Bvert *nu, Bvert *v, Bvert *nv);

   // New versions of face redefinition, under development 12/2004:

   // Perform a lightweight face redefinition, replacing
   // vert 'a' with vert 'b', but not changing the edges.
   virtual bool redef2(Bvert *a, Bvert *b);

   // Redefine this face, replacing edge 'a' with 'b'.
   // Update the old edge to forget this face,
   // and the new edge to remember it.
   virtual bool redef2(Bedge *a, Bedge *b);

   // Return true if stored edges match stored verts;
   // used in redef ops to ensure face is not all bollocksed up:
   bool check() const;

   //******** HANDLING CACHED VALUES ********
   virtual void geometry_changed();
   virtual void normal_changed();

   //******** I/O ********
   friend ostream &operator <<(ostream &os, CBface &f) {
      return os << f.v(1)->index() << " "
                << f.v(2)->index() << " "
                << f.v(3)->index() << endl;
   }
   friend STDdstream &operator <<(STDdstream &d, CBface &f) {
      return d << f.v(1)->index() << f.v(2)->index() << f.v(3)->index();
   }

   //******** Bsimplex VIRTUAL METHODS ********

   // dimension:
   virtual int dim() const { return 2; }

   // index in BMESH Bedge array:
   virtual int  index() const;
   
   virtual Bface* get_face()            const   { return (Bface*)this; }
   virtual bool   on_face(CBface* f)    const   { return f == this; }

   // Return a list of adjacent Bsimplexes:
   // (returns v1, v2, v3, e1, e2, e3);
   virtual Bsimplex_list neighbors() const;

   //******** XXX - OBSOLETE -- OLD TESSELLATOR STUFF
   bool local_search(Bsimplex *&end, Wvec &final_bc,
                     CWpt &target, Wpt &reached, 
                     Bsimplex *repeater = 0, int iters = 30);

   //*******************************************************
   // PROTECTED
   //*******************************************************
 protected:
   friend       class Patch;

   Bvert*       _v1;            // vertex 1 (vertices listed in CCW order)
   Bvert*       _v2;            // vertex 2
   Bvert*       _v3;            // vertex 3
   Bedge*       _e1;            // edge from _v1 to _v2
   Bedge*       _e2;            // edge from _v2 to _v3
   Bedge*       _e3;            // edge from _v3 to _v1
   Wvec         _norm;          // unit length face normal
   double       _area;          // area of this face
   Patch*       _patch;         // patch which owns this face
   int          _patch_index;   // index of face in list on patch
   Bvert*       _orient;        // for triangle stripping
   uint         _ff_stamp;      // for computing front-facing
   uint         _zx_stamp;      // zero-cross path flag
   UVpt*        _tc;            // texture coords
   ushort       _layer;         // layer number (used for non-manifold meshes)

   // Setting the Patch -- nobody's bizness but the Patch's.
   // (You want to set it? call Patch::add(Bface*).)
   virtual void set_patch(Patch* p)      { _patch = p; }
   void    set_patch_index(int k)        { _patch_index = k; }
   int     patch_index() const           { return _patch_index; }

   //******** INTERNAL METHODS ********

   Bsimplex* ndc_walk(CNDCpt& target, CWvec &bc=Wvec(),
                      CNDCpt &nearest=NDCpt(),
                      int is_on_tri=0, bool use_passed_in_params=false) const;

   // methods for recomputing cached values when needed:
   void set_normal() {
      set_bit(VALID_NORMAL_BIT,1);
      _norm = (cross(_v2->loc()-_v1->loc(),_v3->loc()-_v1->loc())).normalized();
      _area = signed_area(_v1->loc(),_v2->loc(),_v3->loc());
   }
};

/*******************************************************
 * Inline methods
 *******************************************************/

// Convenient "upcast":
inline Bface*
bface(Bsimplex* sim)
{
   return is_face(sim) ? (Bface*)sim : 0;
}

inline Bface*
lookup_face(CBedge* e, CBvert* v)
{
   return (e && v) ? e->lookup_face(v) : 0;
}

inline Bface*
lookup_face(CBedge* a, CBedge* b)
{
   return (a && b) ? a->lookup_face(b) : 0;
}

inline Bface*
lookup_face(CBvert* a, CBvert* b, CBvert* c)
{
   Bedge* e = a ? a->lookup_edge(b) : 0;

   return e ? e->lookup_face(c) : 0;
}

inline Bvert* 
next_vert_ccw(CBface* f, CBvert* v) 
{
   // Returns the next vertex of f following v in CCW order.

   // v must be a vertex of f.

   return f ? f->next_vert_ccw(v) : 0; 
}

inline Bface* 
ccw_face(CBedge* e, CBvert* v) 
{
   // Returns the Bface adjacent to e for which the next
   // vertex in CCW order following v also belongs to e.

   // v must be a vertex of e.

   Bvert* u = e ? e->other_vertex(v) : 0;

   return ((u == 0)                        ? 0       :
           (next_vert_ccw(e->f1(),v) == u) ? e->f1() :
           (next_vert_ccw(e->f2(),v) == u) ? e->f2() :
           0);
}

inline Bface* 
ccw_face(CBedge* e) 
{
   return e ? ccw_face(e, e->v1()) : 0;
}

inline void
reverse_faces(CARRAY<Bface*>& faces)
{
   for (int k=faces.num()-1; k>=0; k--)
      faces[k]->reverse();
}

// Return the closest vertex to the 
// given point (in world space):
inline Bvert*
closest_vert(Bface* f, CWpt& p)
{
   if (!f) return 0;
   double d1 = f->v1()->wloc().dist(p);
   double d2 = f->v2()->wloc().dist(p);
   double d3 = f->v3()->wloc().dist(p);
   if (d1 < d2)
      return (d3 < d1) ? f->v3() : f->v1();
   else
      return (d3 < d2) ? f->v3() : f->v2();
}

// Like above, but work in NDC space
inline Bvert*
closest_vert(Bface* f, CNDCpt& p)
{
   if (!f) return 0;
   double d1 = NDCpt(f->v1()->wloc()).dist(p);
   double d2 = NDCpt(f->v2()->wloc()).dist(p);
   double d3 = NDCpt(f->v3()->wloc()).dist(p);
   if (d1 < d2)
      return (d3 < d1) ? f->v3() : f->v1();
   else
      return (d3 < d2) ? f->v3() : f->v2();
}

/*****************************************************************
 * Bface filters
 *****************************************************************/
class FrontFacingBfaceFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_face(s) && ((Bface*)s)->front_facing();
   }
};

class PrimaryFaceFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_face(s) && ((Bface*)s)->is_primary();
   }
};

class SecondaryFaceFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_face(s) && ((Bface*)s)->is_secondary();
   }
};

class QuadFaceFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_face(s) && ((Bface*)s)->is_quad();
   }
};

class QuadRepFaceFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_face(s) && ((Bface*)s)->is_quad_rep();
   }
};

/************************************************************
 * Bface_list
 ************************************************************/
class Bface_list;
typedef const Bface_list CBface_list;
class Bface_list : public SimplexArray<Bface_list,Bface*> {
 public:

   //******** MANAGERS ********

   Bface_list(CARRAY<Bface*>& list) : SimplexArray<Bface_list,Bface*>(list) {}
   explicit Bface_list(int n=0)     : SimplexArray<Bface_list,Bface*>(n)    {}
   explicit Bface_list(Bface* f)    : SimplexArray<Bface_list,Bface*>(f) {
      if (f && f->is_quad())
         add(f->quad_partner());
   }

   //******** ADJACENCY ********

   // Returns list of verts or edges contained in a given
   // set of faces:
   Bvert_list get_verts() const;
   Bedge_list get_edges() const;

   // Returns true if all faces belong to the same patch:
   bool same_patch() const;

   // Return the patch that owns these faces if there is exactly 1.
   // (Otherwise return the null pointer.)
   Patch* get_patch() const;

   //******** GEOMETRY  ********

   // Returns the average of the contained face normals:
   Wvec avg_normal() const;

   // Returns the maximum deviation, in radians, of a contained face
   // normal from the given vector n:
   double max_norm_deviation(CWvec& n) const;

   // Returns true if the angle between each face normal and the
   // average normal is less than the given max_angle (in radians):
   bool is_planar(double max_angle) const {
      return(max_norm_deviation(avg_normal()) < max_angle);
   }

   // Returns the sum of the "volume elements" for each face.
   // Really only makes sense for a list of faces forming a
   // closed surface; though if the faces form a nearly-closed
   // surface it can tell you if they are oriented mostly
   // inside-out or not. (Negative volume == inside out.)
   // Uses the divergence theorem.
   double volume() const;

   //******** TOPOLOGY ********

   // Returns true if the faces form a single connected component:
   // Note: two faces that share a vertex but not an edge are
   //       NOT connected.
   bool is_connected()                  const;

   // Returns true if no edge *interior* to this set of faces 
   // has 2 adjacent faces with inconsistent orientation:
   bool is_consistently_oriented()      const;

   // Returns true if every contained edge is adjacent to no more than
   // 2 faces of *this* face set:
   bool is_2_manifold()                 const;

   // Returns true if the set is connected, the boundary is a simple
   // loop, and the faces form a 2-manifold (WRT faces of this set):
   bool is_disk()                       const;

   // Flip the orientation:
   void reverse_faces() const { 
      for (int k=0; k<_num; k++)
         _array[k]->reverse();
   }

   // Extract boundary edges in one or more continuous chains
   // running counter-clockwise around the face set:
   EdgeStrip get_boundary() const;

   Bedge_list boundary_edges() const;
   Bedge_list interior_edges() const;
   Bvert_list interior_verts() const;

   // Returns the 1-ring faces of a set of faces.  The
   // returned list contains the original faces, plus the
   // external faces reachable within 1 edge length.
   Bface_list one_ring_faces() const { return get_verts().one_ring_faces(); }

   // Like the one-ring, but doesn't include faces of this set:
   Bface_list exterior_faces() const;

   // Returns the 2-ring faces of a set of faces:
   Bface_list two_ring_faces() const { return one_ring_faces().one_ring_faces();}

   // Like above, generalized to n
   Bface_list n_ring_faces(int n) const {
      Bface_list ret = *this;
      for (int i=0; i<n; i++)
         ret = ret.one_ring_faces();
      return ret;
   }

   // If the list contains any "quad" faces but not their partners,
   // returns the "completed" list, containing the missing partners:
   Bface_list quad_complete_faces() const;

   //************ GROW CONNECTED *******

   // reachable_faces:
   // 
   //   Returns the list of faces reachable from f,
   //   crossing only edges accepted by the filter.
   //   (The default filter accepts all edges.)
   //   It sets needed flags (on the entire mesh)
   //   then calls grow_connected(), below:
   static Bface_list reachable_faces(Bface* f, CSimplexFilter& =BedgeFilter());

   // grow_connected:
   // 
   //   Collect all reachable faces whose flag == 1, starting at f,
   //   crossing only edges that are accepted by the given
   //   filter. (The default filter accepts all edges.)
   //
   //   NOTE:
   //     Assumes all face flags are initialized to 1.
   //     This method sets face flags to 2 as it
   //     collects each face.
   //
   bool grow_connected(Bface* f, CSimplexFilter& =BedgeFilter());

   //******** PUSH LAYER ********

   // Given a set of faces from a single mesh, mark the
   // faces as a separate "layer" of the mesh, with
   // lower priority than the primary, manifold layer:
   bool push_layer(bool push_boundary=true) const;

   // Reports whether it valid to call push_layer() on this face set:
   bool can_push_layer() const;

   // unpush_layer() undoes the effect of push_layer(), if possible:
   bool unpush_layer(bool unpush_boundary=true) const;

   // Tells if it's possible:
   bool can_unpush_layer() const;

   // Label each face as "primary"
   void make_primary() const {
      for (int i=0; i<_num; i++)
         _array[i]->make_primary();
   }

   // Label each face as "secondary",
   // i.e., as not being in the primary layer
   void make_secondary() const {
      for (int i=0; i<_num; i++)
         _array[i]->make_secondary();
   }

   //******** PRIMARY/SECONDARY FACES ********
   // Convenience: get the primary or secondary faces
   Bface_list secondary_faces() const{
      return filter(BitSetSimplexFilter(Bface::SECONDARY_BIT));
   }
   Bface_list primary_faces() const{
      return filter(BitClearSimplexFilter(Bface::SECONDARY_BIT));
   }

   bool is_all_primary() const {
      return all_satisfy(BitClearSimplexFilter(Bface::SECONDARY_BIT));
   }
   bool is_all_secondary() const {
      return all_satisfy(BitSetSimplexFilter(Bface::SECONDARY_BIT));
   }
   bool has_any_primary() const {
      return any_satisfy(BitClearSimplexFilter(Bface::SECONDARY_BIT));
   }
   bool has_any_secondary() const {
      return any_satisfy(BitSetSimplexFilter(Bface::SECONDARY_BIT));
   }
   bool num_primary() const {
      return num_satisfy(BitClearSimplexFilter(Bface::SECONDARY_BIT))!=0;
   }
   bool num_secondary() const {
      return num_satisfy(BitSetSimplexFilter(Bface::SECONDARY_BIT))!=0;
   }
   
 protected:

   //******** PROTECTED METHODS ********
   void clear_vert_flags() const;
   void clear_edge_flags() const;

   // Set the flag of each face to 1, and clear the flags of
   // each face around the boundary of this set of faces:
   void mark_faces() const;
};

/*****************************************************************
 * Inlined Bvert methods
 *****************************************************************/
inline Bface_list 
Bvert::get_all_faces() const
{
   Bface_list ret;
   get_all_faces(ret);
   return ret;
}

inline Bface_list
Bvert::get_faces() const
{
   Bface_list ret;
   get_faces(ret);
   return ret;
}

/*****************************************************************
 * Inlined Bedge methods
 *****************************************************************/
inline void
Bedge::clear_flag02() 
{
   // Clear flag of this edge and adjacent faces:
   // (clearing propagates from dim 0 to dim 2)

   clear_flag();
   if (_f1)  _f1->clear_flag();
   if (_f2)  _f2->clear_flag();
   if (_adj) _adj->clear_flags();
}

inline int
Bedge::num_quads() const
{
   return (
      (_f1 && _f1->is_quad() ? 1 : 0) +
      (_f2 && _f2->is_quad() ? 1 : 0)
      );
}

inline Bface* 
Bedge::screen_face(CSimplexFilter& filter) const 
{
   // Return the first face accepted by the filter:

   return (
      filter.accept(_f1) ? _f1 :
      filter.accept(_f2) ? _f2 :
      _adj ? _adj->first_satisfies(filter) : 0
      );
}

inline bool
Bedge::is_primary() const
{
   // Return true if one or more adjacent faces are primary

   return ((_f1 && _f1->is_primary()) || (_f2 && _f2->is_primary()));
}

inline Bvert* 
Bedge::opposite_vert(CBface* f) const 
{
   // Only meaningful for primary faces:
   // If f is _f1, return the other vertex of _f2;
   // and vice versa:

   Bface* g = other_face(f);
   return g ? g->other_vertex(_v1,_v2) : 0;
}

inline bool 
Bedge::contains(CBface* f) const 
{
   return (_f1 == f || _f2 == f || (_adj && _adj->contains((Bface*)f)));
}

/*****************************************************************
 * Inlined Bedge_list methods
 *****************************************************************/
inline Bface_list 
Bedge_list::get_primary_faces() const
{
   return get_faces().primary_faces();
}

/**********************************************************************
 * PrimaryVertFilter:
 *
 *   Accept a Bvert if it is adjacent to at least 1 primary face
 *****************************************************************/
class PrimaryVertFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_vert(s) &&
         !((Bvert*)s)->get_all_faces().primary_faces().empty();
   }
};

#endif  // BFACE_H_HAS_BEEN_INCLUDED

/* end of file bface.H */
