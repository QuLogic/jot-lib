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
 * bvert.C
 **********************************************************************/
#include "disp/ray.H"
#include "mi.H"

using namespace mlib;

Bvert::~Bvert() 
{
   // remove self from global selection list, if needed
   if (is_selected()) {
      MeshGlobal::deselect(this);
   }

   // must be isolated vertex -- make sure
   // adjacent edges are removed:
   if (auto m = mesh()) {
      while (degree() > 0)
         m->remove_edge(_adj.back());
   }
}

int
Bvert::index() const
{
   // index in BMESH Bvert array:

   if (auto m = mesh())
      return m->verts().get_index((Bvert*)this);
   else
      return -1;
}

void
Bvert::operator +=(Bedge* e) 
{
   _adj.push_back(e);
   degree_changed();
   if (e->is_crease())
      crease_changed();
}

int
Bvert::operator -=(Bedge* e) 
{
   // forget about an edge which is disconnecting from this

   Bedge_list::iterator it;
   it = std::find(_adj.begin(), _adj.end(), e);
   int ret = (it != _adj.end());
   _adj.erase(it);
   if (!ret)
      err_msg("Bvert::operator -=(Bedge* e): can't remove e: not in list");
   if (e->is_crease())
      crease_changed();
   degree_changed();
   return ret;
}


NDCZpt
Bvert::ndc() const
{
   return NDCZpt(_loc, mesh()->obj_to_ndc());
}

Wpt
Bvert::wloc() const
{
   return mesh()->xform()*_loc;
}

void
Bvert::get_q_nbrs(vector<Bvert*>& q) const
{
   // Returns "neighboring" vertices that share a quad with
   // this vertex, and that are positioned at the opposite
   // corner.

   q.clear();

   Bface* f;
   Bedge_list a = get_manifold_edges();
   for (Bedge_list::size_type i=0; i<a.size(); i++)
      if (a[i]->is_strong() && (f = ccw_face(a[i], this)) && f->is_quad())
         q.push_back(f->quad_opposite_vert(this));
}

Bsimplex_list
Bvert::neighbors() const
{
   Bface_list faces = get_all_faces();
   Bsimplex_list ret(faces.size() + _adj.size());
   for (Bface_list::size_type i=0; i<faces.size(); i++)
      ret.push_back(faces[i]);
   for (Bedge_list::size_type j=0; j<_adj.size(); j++)
      ret.push_back(_adj[j]);
   return ret;
}

inline void
get_nbrs(CBvert* v, CBedge_list& edges, vector<Bvert*>& nbrs)
{
   nbrs.clear();
   if (!v)
      return;
   for (Bedge_list::size_type i=0; i<edges.size(); i++) {
      Bvert* nbr = edges[i]->other_vertex(v);
      assert(nbr);
      nbrs.push_back(nbr);
   }
}

Bvert_list
Bvert::get_nbrs(CBedge_list& edges) const
{
   // Return a set of neighboring vertices from the
   // given set of edges, all of which must contain
   // this vertex (or an assertion failure will occur):
   Bvert_list ret(edges.size());
   for (Bedge_list::size_type i=0; i<edges.size(); i++) {
      Bvert* v = nbr(edges[i]);
      assert(v);
      ret.push_back(v);
   }
   return ret;
}

inline Bedge* 
leading_ccw_edge(CBvert* v)
{
   // Return a border edge, if any, that forms the leading
   // edge for a fan of triangles running in CCW order
   // around v.

   Bedge* e = nullptr;
   for (int i=0; i<v->degree(); i++)
      if ((e = v->e(i))->is_border() && ccw_face(e, v))
         return e;

   return nullptr;
}

Bedge_list
Bvert::get_ccw_edges() const
{
   // Return list of edges in CCW order:
   // returns an empty list if the operation is invalid.

   // XXX - needs revision to deal w/ non-manifold surfaces.

   // If no edges, nothing to do:
   if (degree() < 1)
      return Bedge_list();

   // Check number of border edges:
   int border_degree = degree(BorderEdgeFilter());
   if (border_degree > 0 && border_degree != 2) {
      err_msg("Bvert::ccw_edges: error: too many border edges, %d.",
              border_degree);
      return Bedge_list();
   }

   // Find an edge to start from for CCW traversal of star faces
   Bedge* start = (border_degree == 2) ? leading_ccw_edge(this) : e(0);

   if (!start) {
      cerr << "Bvert::ccw_edges: error: can't find starting edge." << endl;
      return Bedge_list();
   }

   // add first edge
   Bedge_list ret(degree());
   ret.push_back(start);

   // Repeatedly advance to next face and next edge going CCW.
   // Add edges to the list until we reach the start edge.
   Bedge* e = start;
   Bface* f = nullptr;
   while ((f = ccw_face(e, this)) &&
          (e = f->opposite_edge(e->other_vertex(this))) &&
          (e != start))
      ret.push_back(e);

   assert(ret.size() == _adj.size());

   return ret;
}

Bvert_list 
Bvert::get_ccw_nbrs() const
{
   return get_nbrs(get_ccw_edges());
}

bool
Bvert::order_edges_ccw()
{
   // Arrange edges in counter-clockwise order.

   // Don't redo it if VALID_CCW_BIT indicates the edges are
   // already ordereed CCW:
   if (is_set(VALID_CCW_BIT))
      return true;

   Bedge_list ordered = get_ccw_edges();
   if ((int)ordered.size() == degree()) {
      _adj = ordered;
      set_bit(VALID_CCW_BIT);
      return true;
   }
   return false;
}

void
Bvert::get_nbrs(vector<Bvert*>& nbrs) const
{
   // return neighboring vertices in an array

   ::get_nbrs(this, _adj, nbrs);
}

void
Bvert::get_primary_nbrs(vector<Bvert*>& nbrs) const
{
   ::get_nbrs(this, get_manifold_edges(), nbrs);
}

void
Bvert::get_p_nbrs(vector<Bvert*>& nbrs) const
{
   ::get_nbrs(this, get_manifold_edges(), nbrs);
}

inline void
mark_pushed_faces(CBvert* v, Bedge* e, Bface* f)
{
   // run a sweep over the fan of nm faces originating at <v,e,f>
   // and mark them
   assert(v && e && f && e->contains(v) && f->contains(e));
   if (f->flag())
      return;
   do {
      f->set_flag();
   } while ((e = f->opposite_edge(e->other_vertex(v))) &&
            (f = e->other_face(f)) &&
            !f->flag());
}

inline void
mark_pushed_faces(CBvert* v, Bedge* e)
{
   // run a sweep over each fan of nm faces originating at <v,e>
   // and mark them
   assert(v && e && e->contains(v));
   if (!e->adj()) return;
   CBface_list& adj = *e->adj();
   for (Bface_list::size_type i=0; i<adj.size(); i++)
      mark_pushed_faces(v, e, adj[i]);
}

inline void
mark_pushed_faces(CBvert* v, CBedge_list& edges)
{
   // for each nm edge, run over nm faces and mark them
   assert(v);
   for (Bedge_list::size_type i=0; i<edges.size(); i++)
      mark_pushed_faces(v, edges[i]);
}

inline bool
has_unmarked_face(Bedge* e)
{
   assert(e);
   return ((e->f1() && e->f1()->flag() == 0) ||
           (e->f2() && e->f2()->flag() == 0));
}

inline void
try_get_nm_edge(Bedge* e, vector<Bedge*>& ret)
{
   if (e && e->flag() == 0 && has_unmarked_face(e)) {
      e->set_flag();
      ret.push_back(e);
   }
}

void 
Bvert::get_manifold_edges(vector<Bedge*>& ret) const 
{
   // Return adjacent edges the normal way, unless it's a
   // "multi" vertex; then return manifold edges.

   if (is_manifold()) {
      ret = _adj;
      return;
   }
   // clear all edge and face flags:
   _adj.clear_flag02();

   // set flags on faces we don't want:
   mark_pushed_faces(this, _adj.filter(MultiEdgeFilter()));

   // collect up edges adjacent to faces we do want:
   ret.clear();
   for (Bedge_list::size_type i=0; i<_adj.size(); i++) {
      try_get_nm_edge(_adj[i], ret);
   }
}

inline void
add_face(vector<Bface*>& faces, set<Bface*>& unique, Bface *f)
{
   // Helper method used below.
   // Add a non-null face uniquely to the list.
   pair<set<Bface*>::iterator,bool> result;

   if (f) {
      result = unique.insert(f);
      if (result.second)
         faces.push_back(f);
   }
}

inline void
get_faces(CBedge_list& e, vector<Bface*>& ret, set<Bface*>& unique)
{
   // Helper function used below in Bvert::get_faces()

   ret.clear();

   for (Bedge_list::size_type k=0; k<e.size(); k++) {
      add_face(ret, unique, e[k]->f1());
      add_face(ret, unique, e[k]->f2());
   }
}

void
Bvert::get_faces(vector<Bface*>& ret, set<Bface*>& unique) const
{
   // Helper function for the below function.

   if (is_manifold()) {
      ::get_faces(_adj, ret, unique);
   } else {
      ::get_faces(get_manifold_edges(), ret, unique);
   }
}

void
Bvert::get_faces(vector<Bface*>& ret) const
{
   // Return a list of faces in the star of this vertex.
   //
   // At a non-manifold part of the mesh,
   // select faces just from the primary layer
   set<Bface*> unique;
   get_faces(ret, unique);
}

inline void
add_quad_partners(vector<Bface*>& faces, set<Bface*>& unique)
{
   // Expand the given face list to include "quad partners"
   // of any of the faces in the original list.

   // Work backwards to add faces as we go.
   for (int k=faces.size()-1; k>=0; k--)
      add_face(faces, unique, faces[k]->quad_partner());
}

void
Bvert::get_q_faces(vector<Bface*>& ret) const
{
   // Return a list of faces in the star of this vertex,
   // including both faces of every quad adjacent to this vertex.
   set<Bface*> unique;

   get_faces(ret, unique);
   add_quad_partners(ret, unique);
}

inline Bedge* 
next_border_edge_cw(CBvert* v, CBedge_list& edges) 
{
   assert(v);
   for (Bedge_list::size_type i=0; i<edges.size(); i++) {
      Bedge* e = edges[i];
      if (e->is_border() && e->cw_face(v))
         return e;
   }
   return nullptr;
}

Bedge* 
Bvert::next_border_edge_cw() 
{
   return ::next_border_edge_cw(
      this, is_non_manifold() ? get_manifold_edges() : _adj
      );
}

void
Bvert::get_all_faces(vector<Bface*>& ret) const
{
   // Return ALL faces incident to the vertex,
   // including faces adjacent to non-manifold ("multi") edges:
   set<Bface*> unique;

   // get ordinary (manifold) faces:
   get_faces(ret, unique);

   // if there are no "multi" edges, we're done
   if (is_manifold())
      return;

   // collect up additional faces from multi-edges
   for (int i=0; i<degree(); i++) {
      if (e(i)->adj() != nullptr) {
         Bface_list b = e(i)->get_all_faces();
         for (Bface_list::size_type j=0; j<b.size(); j++)
            add_face(ret, unique, b[j]);
      }
   }
}

inline void 
get_quad_faces(CBedge_list& e, vector<Bface*>& ret)
{
   // Helper function used below in Bvert::get_quad_faces()
   set<Bface*> unique;

   ret.clear();

   Bface* f;
   for (Bedge_list::size_type k=0; k<e.size(); k++) {
      if (!e[k]->is_weak()) {
         if ((f = e[k]->f1()) && f->is_quad())
            add_face(ret, unique, f->quad_rep());
         if ((f = e[k]->f2()) && f->is_quad())
            add_face(ret, unique, f->quad_rep());
      }
   }
}

void
Bvert::get_quad_faces(vector<Bface*>& ret) const
{
   // Return a list of quads in the star of this vertex.
   //
   // At a non-manifold part of the mesh,
   // select quads just from the primary layer
   if (is_manifold())
      ::get_quad_faces(_adj, ret);
   else {
      ::get_quad_faces(get_manifold_edges(), ret);
   }
}

int
Bvert::num_quads() const 
{
   Bface_list faces;

   get_quad_faces(faces);
   return faces.size();
}

int
Bvert::num_tris() const 
{
   Bface_list faces;
   get_faces(faces);
   int ret=0;
   for (Bface_list::size_type i=0; i<faces.size(); i++)
      if (!faces[i]->is_quad())
         ret++;
   return ret;
}

int 
Bvert::face_degree(CSimplexFilter& f) const
{
   // within the star of this vertex, return the number of
   // faces that satisfy the given filter:

   Bface_list faces;
   get_faces(faces);
   return faces.num_satisfy(f);
}

Bface* 
Bvert::get_face() const
{
   Bface* ret = nullptr;
   for (Bedge_list::size_type k=0; k<_adj.size(); k++)
      if ((ret = e(k)->get_face()))
         return ret;
   return nullptr;
}

bool
Bvert::on_face(CBface* f) const
{
   return f->contains(this);
}

Wvec
Bvert::compute_normal(const vector<Bface*>& faces) const
{
   // Computes angle-weighted average normal from the given faces:

   Wvec ret;
   for (auto & face : faces)
      ret += weighted_vnorm(face, this);
   return ret.normalized();
}

void 
Bvert::set_normal()
{
   set_bit(VALID_NORMAL_BIT,1);

   // if all edges are "polylines" (no adjacent faces), do nothing:
   if (_adj.all_satisfy(PolylineEdgeFilter()))
      return;

   // get star faces
   static Bface_list star(32);
   get_faces(star);
   _norm = compute_normal(star);
}

Wvec
Bvert::norm(CSimplexFilter& f) const
{
   // return a normal WRT faces accepted by the given filter:
   Bface_list faces;
   get_all_faces(faces);
   return compute_normal(faces.filter(f));
}

Wvec
Bvert::wnorm() const
{
   // normal transformed to world space
   // xf.inverse().transpose() * norm

   if (auto m = mesh())
      return (m->inv_xform().transpose() * norm()).normalized();
   else
      return norm();
}

void
Bvert::get_normals(vector<Wvec>& norms) const
{
   // Return a list of distinct "normals." For a vertex that
   // contains no crease edges just return the single vertex
   // normal. Otherwise return 1 distinct normal for each sector
   // of faces between crease edges.

   // XXX - should be more efficient.

   set<Wvec> unique;
   norms.clear();
   if (is_crease()) {
      Bface_list faces;
      get_faces(faces);
      for (Bface_list::size_type i=0; i<faces.size(); i++) {
         Wvec n;
         faces[i]->vert_normal(this, n);
         pair<set<Wvec>::iterator,bool> result = unique.insert(n);
         if (result.second)
            norms.push_back(n);
      }
   } else {
      norms.push_back(norm());
   }
}

void 
Bvert::geometry_changed()
{
   // The location of this vertex changed...

   // Notify simplex data
   Bsimplex::geometry_changed();

   size_t i;

   // Tell adjacent edges they changed shape:
   for (i=0; i<_adj.size(); i++)
      _adj[i]->geometry_changed();

   // Tell 1-ring faces they changed shape, which will cause each
   // of them to call normal_changed() on their adjacent vertices
   // and edges.
   //
   // XXX - This is not optimally efficient, because
   // non-border edges will receive 2 normal_changed()
   // notifications, and so will each vertex at the opposite
   // ends of those edges.
   Bface_list star(32);
   get_all_faces(star);
   for (i=0; i<star.size(); i++) {
      star[i]->geometry_changed();
   }
}

void 
Bvert::degree_changed()
{
   clear_bit(VALID_STRESSED_BIT);

   star_changed();
   normal_changed();
}

void  
Bvert::normal_changed() 
{
   clear_bit(VALID_NORMAL_BIT); 
   clear_bit(VALID_STRESSED_BIT); 

   // notify simplex data
   Bsimplex::normal_changed();
}

double
Bvert::star_sum_angles() const
{
   // add up "interior" angles around the vertex

   Bface_list star(32);
   get_faces(star);

   double ret = 0;
   for (Bface_list::size_type i=0; i<star.size(); i++)
      ret += star[i]->angle(this);
   return ret;
}

double
Bvert::star_area() const
{
   // add up the areas of the triangles around the vertex

   Bface_list star(32);
   get_faces(star);

   double ret = 0;
   for (Bface_list::size_type i=0; i<star.size(); i++)
      ret += star[i]->area();
   return ret;
}

bool 
Bvert::is_sharp_point(double sharpness)
{
   // add up angles around this vertex.
   // if it's much less than 2 pi say this is "sharp".

   return (2*M_PI) - star_sum_angles() > sharpness;
}

Wpt
Bvert::eye_local() const
{
   return mesh() ? mesh()->eye_local() : VIEW::eye();
}

inline Bvert_list
get_nbrs(const Bvert* v, CBedge_list& edges)
{
   // The edges must contain vertex v;
   // Returns corresponding opposite verts.

   assert(v);
   Bvert_list ret(edges.size());
   for (Bvert_list::size_type i=0; i<edges.size(); i++) {
      assert(v->nbr(edges[i]));
      ret.push_back(v->nbr(edges[i]));
   }
   return ret;
}

Wpt
Bvert::centroid() const 
{
   Wpt ret;                                     // computed centroid to be returned
   double w = 0;                                // net weight
   Bedge_list star = get_manifold_edges();      // work with primary layer edges

   if (star.empty())
      return loc();

   if (star.any_satisfy(BorderEdgeFilter()))
      return ::get_nbrs(this, star.filter(BorderEdgeFilter())).center();

   for (Bedge_list::size_type i=0; i<star.size(); i++) {
      ret = ret + nbr_loc(star[i]);
      w += 1.0;
   }
   assert(w > 0);
   return ret/w;
}

Wpt
Bvert::area_centroid() const
{
   Wpt ret;                                     // computed centroid to be returned
   double total_weight = 0;                     // net weight
   double w=0;                                  // weight for a given neighbor
   Bedge_list star = get_manifold_edges();      // work with primary layer edges
   
   if (star.empty())
      return loc();

   if (star.any_satisfy(BorderEdgeFilter()))
      return ::get_nbrs(this, star.filter(BorderEdgeFilter())).center();

   for (Bedge_list::size_type i=0; i < star.size(); i++) {
      Bedge* e = star[i];
      w = e->avg_area();
      total_weight += w;
      ret = ret + (w*nbr_loc(e));
   }
   assert(total_weight > 0);
   return ret / total_weight;
}

Wpt
Bvert::qr_centroid() const
{
   //   Special kind of centroid that weights regular ("r")
   //   neighbors differently from quad ("q") neighbors:
   //
   //              q -------- r -------- q              
   //              |          |          |            
   //              |          |          |            
   //              |          |          |            
   //              |          |          |            
   //              |          |          |            
   //              r -------- v -------- r              
   //              |         / \         |               
   //              |       /     \       |              
   //              |     /         \     |               
   //              |   /             \   |               
   //              | /                 \ |               
   //             r -------------------- r  
   // 
   //    There are 2 kinds of "neighboring" vertices: regular (r)
   //    vertices, joined to this vertex by a strong edge, and
   //    quad (q) vertices, which lie at the opposite corner of a
   //    quad from this vertex.  q vertices are not necessarily
   //    connected to this vertex by an edge. r vertices are
   //    weighted by 1, q vertices are weighted by 1/2.

   Wpt ret;  // computed centroid to be returned
   double net_weight = 0; 
   Bedge_list star = get_manifold_edges(); // work with primary layer edges

   if (star.empty())
      return loc();
/*
   if (0) {
      BorderEdgeFilter b;
      PolylineEdgeFilter p;
      if (star.any_satisfy(b || p))
         return ::get_nbrs(this, star.filter(b || p)).center();
   }
*/
   double avg_len = star.avg_len();
   assert(avg_len > 0);
   for (Bedge_list::size_type i=0; i<star.size(); i++) {
      Bedge* e = star[i];
      if (e->is_strong()) {
         // add contribution from r neighbor
         double w = e->length()/avg_len;
         ret = ret + w*nbr_loc(e);
         net_weight += w;
         Bface* f = ccw_face(e, this);
         if (f && f->is_quad() && f->quad_opposite_vert(this)) {
            // add contribution from q neighbor
            Bvert* q = f->quad_opposite_vert(this);
            w = 0.5*q->loc().dist(loc())/avg_len;
            ret = ret + w*q->loc();
            net_weight += w;
         }
      }
   }
   assert(net_weight > 0);
   return ret/net_weight;
}

bool
Bvert::view_intersect(
   CNDCpt& p,           // Screen point at which to do intersect
   Wpt&    nearpt,      // Point on vert visually nearest to p
   double& dist,        // Distance from nearpt to ray from camera
   double& d2d,         // Distance in pixels nearpt to p
   Wvec& n              // "normal" at nearpt
   ) const
{
   // (Bsimplex virtual method):
   // Intersection w/ ray from given screen point -- returns the point
   // on the Bvert that is nearest to the given screen space point.
   // Note: the returned "near point" and "normal" are both
   //       transformed to world space.

   // HEY! This is easy!!!
   nearpt = wloc();
   Wpt eye = XYpt(p);
   dist = nearpt.dist(eye);
   d2d  = PIXEL(nearpt).dist(PIXEL(p));

   // Go for average of face normals if available, or else vector
   // to camera:
   Wvec n1 = norm();
   if (n1.is_null())
      n1 = eye - loc();

   // Transform the normal properly:
   n = (mesh()->inv_xform().transpose()*n1).normalized();

   return 1;
}

bool 
Bvert::local_search(
   Bsimplex           *&end,
   Wvec                &final_bc,
   CWpt                &target,
   Wpt                 &reached,
   Bsimplex            *repeater,
   int                  iters)
{
   Bface_list star(16);
   get_faces(star);
   for (int k = star.size()-1; k>=0; k--) {
      int good_path = star[k]->local_search(
         end, final_bc, target, reached, this, iters-1);
      if (good_path == 1)
         return 1;
      else if (good_path==-1)   // XXX - TODO: figure out what this means
         return repeater ? true : false; // XXX - used to return -1 : 0 -- should check
   }
   end = this;
   final_bc = Wvec(1,0,0);
   reached = _loc;

   return 1;
}

// TODO: unreadable arguments!
// what's input? what's output?
//              -tc
NDCpt 
Bvert::nearest_pt_ndc(
   CNDCpt &/*p*/,
   Wvec   &bc,
   int    &is_on_simplex
   ) const
{ 
   bc = Wvec(1,0,0); 
   is_on_simplex = 1; 
   return _loc; 
}

Wpt   
Bvert::nearest_pt(CWpt&, Wvec& bc, bool& is_on_simplex) const
{ 
   bc = Wvec(1,0,0); 
   is_on_simplex = true; 
   return _loc; 
}

BMESHcurvature_data::curv_tensor_t
Bvert::curv_tensor() const
{
   
   return mesh()->curvature()->curv_tensor(this);
   
}

BMESHcurvature_data::diag_curv_t
Bvert::diag_curv() const
{
   
   return mesh()->curvature()->diag_curv(this);
   
}

double
Bvert::k1() const
{
   
   return mesh()->curvature()->k1(this);
   
}

double
Bvert::k2() const
{
   
   return mesh()->curvature()->k2(this);
   
}

Wvec
Bvert::pdir1() const
{
   
   return mesh()->curvature()->pdir1(this);
   
}

Wvec
Bvert::pdir2() const
{
   
   return mesh()->curvature()->pdir2(this);
   
}

BMESHcurvature_data::dcurv_tensor_t
Bvert::dcurv_tensor() const
{
   
   return mesh()->curvature()->dcurv_tensor(this);
   
}

/*****************************************************************
 * Stuff for computing curvatures at vertices
 *****************************************************************/

// First, some 3x3 matrix stuff

// 3x3 matrix: t1 + t2
inline Wtransf mat3_add(CWtransf t1, CWtransf t2)
{
    Wtransf t = t1;
    
    t(0,0) += t2(0,0);
    t(0,1) += t2(0,1);
    t(0,2) += t2(0,2);

    t(1,0) += t2(1,0);
    t(1,1) += t2(1,1);
    t(1,2) += t2(1,2);

    t(2,0) += t2(2,0);
    t(2,1) += t2(2,1);
    t(2,2) += t2(2,2);

    return t;
}

// 3x3 matrix: t1 * sc
inline Wtransf mat3_scale(CWtransf t1, double sc)
{
    Wtransf t = t1;
    
    t(0,0) *= sc;
    t(0,1) *= sc;
    t(0,2) *= sc;
    t(1,0) *= sc;
    t(1,1) *= sc;
    t(1,2) *= sc;
    t(2,0) *= sc;
    t(2,1) *= sc;
    t(2,2) *= sc;

    return t;
}

// 3x3 matrix:  I-t1
inline Wtransf mat3_identminus(CWtransf t1)
{
    Wtransf t = mat3_scale(t1, -1);

    t(0,0) += 1;
    t(1,1) += 1;
    t(2,2) += 1;
    
    return t;
}

// outer product of two vectors
inline Wtransf mat3_outerProd(CWvec x, CWvec y)
{
    Wtransf ret(x * y[0], x * y[1], x * y[2]);

    return ret;
}

// outer product of vector with itself
inline Wtransf mat3_outerProd(CWvec x)
{
    return mat3_outerProd(x, x);
}

// zero out this matrix
inline void mat3_zero(Wtransf &t)
{
    t(0,0) = 0;
    t(0,1) = 0;
    t(0,2) = 0;
    t(1,0) = 0;
    t(1,1) = 0;
    t(1,2) = 0;
    t(2,0) = 0;
    t(2,1) = 0;
    t(2,2) = 0;
}

/*****************************************************************
 * Bvert filters
 *****************************************************************/
bool 
FoldVertFilter::accept(CBsimplex* s) const 
{
   if (!is_vert(s))
      return false;
   Bvert* v = (Bvert*)s;

   Bedge_list edges = v->get_adj().filter(_edge_filter);

   if (edges.size() != 2)
      return false;

   return is_fold(
      edges[0]->other_vertex(v)->loc(),
      v->loc(),
      edges[1]->other_vertex(v)->loc(),
      v->norm()
      );
}

bool 
VertDegreeFilter::accept(CBsimplex* s) const 
{
   if (!is_vert(s))
      return false;
   Bvert* v = (Bvert*)s;
   return v->degree(_edge_filter) == _n;
}

/*****************************************************************
 * Bvert_list
 *****************************************************************/
BBOX 
Bvert_list::get_bbox_obj()
{
   // return the bounding box of the vertices in object space:
   BBOX ret;
   ret.update(pts());
   return ret;
}

BBOX 
Bvert_list::get_bbox_world()
{
   // return the bounding box of the vertices in world space:
   BBOX ret;
   ret.update(wpts());
   return ret;
}

inline bool
get_edge(Bedge_list& list, Bedge* e)
{
   if (!e) return false;
   list.push_back(e);
   return true;
}

Bedge_list
Bvert_list::get_chain(bool try_close) const
{
   // Return the chain of edges formed by the vertices
   // (or the empty list)

   Bedge_list ret(size());
   for (Bvert_list::size_type i=1; i<size(); i++)
      if (!get_edge(ret, at(i)->lookup_edge(at(i-1))))
         return Bedge_list(0);
   if (try_close && size() > 2)
      get_edge(ret, front()->lookup_edge(back())); // gets the edge if it exists
      
   return ret;
}

Bedge_list
Bvert_list::get_closed_chain() const
{
   // Same as get_chain(), but includes the edge
   // joining first and last vertices. Returns
   // the empty list if it failed.

   Bedge_list ret = get_chain();
   if (ret.empty()) return ret;
   if (!get_edge(ret, front()->lookup_edge(back())))
      return Bedge_list(0);
   return ret;
}

bool 
Bvert_list::forms_chain() const 
{
   // Are the vertices joined sequentially by edges in a
   // connected chain?

   return !get_chain().empty(); 
}

bool 
Bvert_list::forms_closed_chain() const 
{
   return (size() > 1) && forms_chain() && front()->lookup_edge(back());
}

inline void
try_get_face(Bface* f, Bface_list& ret)
{
   // Convenience method used in 1-ring extractions, below

   if ( f && !f->flag() ) {
      f->set_flag();
      ret.push_back(f);
   }
}

inline void
try_get_faces(CBedge* e, Bface_list& ret)
{
   // Convenience method used in 1-ring extractions, below

   if (e) {
      try_get_face(e->f1(), ret);
      try_get_face(e->f2(), ret);
   }
}

inline void
try_get_faces(CBedge_list& edges, Bface_list& ret)
{
   // Convenience method used in 1-ring extractions, below

   for (Bedge_list::size_type i=0; i<edges.size(); i++) {
      try_get_faces(edges[i], ret);
   }
}

Bface_list
Bvert_list::one_ring_faces() const
{
   // Returns the 1-ring faces of a set of vertices

   clear_flag02();

   Bface_list ret;
   for (Bvert_list::size_type i=0; i<size(); i++) {
      try_get_faces(at(i)->get_manifold_edges(), ret);
   }
   return ret.quad_complete_faces();
}

Bface_list 
Bvert_list::two_ring_faces() const 
{
   // Returns the 2-ring faces of a set of vertices:

   return one_ring_faces().one_ring_faces();
}

inline void
try_get_edge(Bedge* e, Bedge_list& ret)
{
   if (e && !e->flag()) {
      e->set_flag();
      ret.push_back(e);
   }
}

inline void
try_get_edges(CBedge_list& edges, Bedge_list& ret)
{
   // Convenience method used in 1-ring extractions, below

   for (Bedge_list::size_type i=0; i<edges.size(); i++) {
      try_get_edge(edges[i], ret);
   }
}

Bedge_list
Bvert_list::get_outer_edges() const
{
   // Return the set of edges containing at least 1 vertex
   // from this set

   clear_flag02();
   Bedge_list ret;
   for (Bvert_list::size_type i=0; i<size(); i++) {
      try_get_edges(at(i)->get_adj(), ret);
   }
   return ret;
}

inline bool
both_set(Bedge* e)
{
   return e && e->v1()->flag() && e->v2()->flag();
}

class VertsSetEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && both_set((Bedge*)s);
   }
};

Bedge_list
Bvert_list::get_inner_edges() const
{
   // Return the set of edges that connect 2 vertices
   // from this set

   Bedge_list outer = get_outer_edges();
   outer.get_verts().clear_flags();
   set_flags(1);
   return outer.filter(VertsSetEdgeFilter());
}

Bvert_list 
Bvert_list::get_verts(Bsimplex* s)
{
   Bvert_list ret;
   if (!s)
      return ret;
   if (is_vert(s)) {
      ret.push_back((Bvert*)s);
   } else if (is_edge(s)) {
      ret.push_back(((Bedge*)s)->v1());
      ret.push_back(((Bedge*)s)->v2());
   } else {
      assert(is_face(s));
      ret.push_back(((Bface*)s)->v1());
      ret.push_back(((Bface*)s)->v2());
      ret.push_back(((Bface*)s)->v3());
   }
   return ret;
}

void
Bvert_list::add_verts_recursively(Bvert_list& ret, Bvert* v)
{
   // utility used in add_verts_recursively, below
   
   assert(v);

   // if already added, skip
   if (v->flag())
      return;
   v->set_flag(); // mark as added

   // add it to the list and check its neighbors:
   ret.push_back(v);
   add_verts_recursively(ret, get_nbrs(v));
}

void
Bvert_list::add_verts_recursively(Bvert_list& ret, CBvert_list& L)
{
   // utility used in connected_verts, below.
   // requires that all vertex flags have been cleared initially.
   
   for (Bvert_list::size_type i=0; i<L.size(); i++) {
      add_verts_recursively(ret, L[i]);
   }
}

Bvert_list
Bvert_list::connected_verts(Bsimplex* s)
{
   static bool debug = Config::get_var_bool("DEBUG_CONNECTED_LIST",false);

   Bvert_list L = get_verts(s), ret;
   if (L.empty()) {
      return ret;
   }
   BMESHptr m = L.mesh();
   assert(m);
   m->verts().clear_flags();
   add_verts_recursively(ret, L);
   err_adv(debug, "Bvert_list::connected_verts: found %d verts", ret.size());
   return ret;
}

// end of file bvert.C
