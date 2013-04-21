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
 * bface.C
 **********************************************************************/
#include "mesh/patch.H"
#include "mesh/mi.H"
#include "mesh/uv_data.H"
#include "std/config.H"

using namespace mlib;

Bface::Bface(Bvert* u, Bvert* v, Bvert* w,
             Bedge* e, Bedge* f, Bedge* g) :
   _v1(u), _v2(v), _v3(w),
   _e1(e), _e2(f), _e3(g),
   _area(0),
   _patch(0),
   _patch_index(-1),
   _orient(0),
   _ff_stamp(0),
   _zx_stamp(0),
   _tc(0),
   _layer(0)
{
   *_e1 += this;
   *_e2 += this;
   *_e3 += this;

   geometry_changed();
}

Bface::~Bface()
{
   // remove self from global selection list, if needed
   if (is_selected()) {
      MeshGlobal::deselect(this);
   }

   // tell edges to forget:
   detach();

   // the patch too
   if (_patch && _patch_index >= 0)
      _patch->remove(this);
}

int
Bface::index() const
{
   // index in BMESH Bface array:

   if (!_mesh) return -1;
   return _mesh->faces().get_index((Bface*)this);
}

void
Bface::normal_changed()
{
   // Shouldn't be called on a Bface.
   // Use Bface::geometry_changed() instead.

   assert(0);
}

void
Bface::geometry_changed()
{
   // One of the 3 vertices changed position, which means the
   // face normal has been redefined

   _ff_stamp = 0;
   clear_bit(VALID_NORMAL_BIT);

   _v1->normal_changed();
   _v2->normal_changed();
   _v3->normal_changed();

   _e1->normal_changed();
   _e2->normal_changed();
   _e3->normal_changed();

   Bsimplex::geometry_changed();
}

bool
Bface::zx_mark() const
{ 
  bool ret = ( _zx_stamp == VIEW::stamp() );
  ((Bface*)this)->_zx_stamp = VIEW::stamp();
  return ret;
}

bool
Bface::zx_query() const
{ 
   if ( _zx_stamp == VIEW::stamp() ) return true ;
   return false;
}

int 
Bface::front_facing() const
{
   // this is just for purposes of visualizing the silhouttes from
   // another viewpoint:
   static bool ignore_xf = Config::get_var_bool("SILS_IGNORE_MESH_XFORM",false);

   if (_ff_stamp != VIEW::stamp()) {
      ((Bface*)this)->_ff_stamp = VIEW::stamp();
      int b = ((
         (ignore_xf ?
          VIEW::peek_cam()->data()->from() :
          _mesh->eye_local()
            ) - _v1->loc()) * norm()) > 0;
      ((Bface*)this)->set_bit(FRONT_FACING_BIT,b);
   }
   return is_set(FRONT_FACING_BIT);
}

Bsimplex*
Bface::find_intersect_sim(
   CNDCpt& target_pt,
   Wpt&    hit_pt) const
{
   Bsimplex* ret = ndc_walk(target_pt);
   if (is_face(ret)) 
      hit_pt =
         ((Bface*)ret)->plane_intersect(_mesh->inv_xform()*Wline(target_pt));
   else if(is_edge(ret))
      hit_pt =
         ((Bedge*)ret)->line().intersect(_mesh->inv_xform()*Wline(target_pt));
   else if (is_vert(ret))
      hit_pt = ((Bvert*)ret)->loc();

   return ret;
}  

// Return true if pt lie within boundary + epsilon neighborhood of the face.
bool
Bface::contains(CWpt& pt,double threshold) const 
{
   // NOTE: Normalize behave in the way that if v is zero, it
   // normalize to zero vector
   const double dist_threshold=0.00001;
   if (plane().dist(pt) > dist_threshold)
      return false;

   Wvec vec1 = (pt-v1()->loc()).normalized();
   Wvec vec2 = (pt-v2()->loc()).normalized();
   Wvec vec3 = (pt-v3()->loc()).normalized();
   Wvec ab = (v2()->loc()-v1()->loc()).normalized();
   Wvec bc = (v3()->loc()-v2()->loc()).normalized();
   Wvec ca = (v1()->loc()-v3()->loc()).normalized();

   Wvec c1 = cross(ab,vec1).normalized();
   Wvec c2 = cross(bc,vec2).normalized();
   Wvec c3 = cross(ca,vec3).normalized();

   Wvec n = cross(ab,bc).normalized();

   if(c1*n<-threshold) return false;
   if(c2*n<-threshold) return false;
   if(c3*n<-threshold) return false;

   return true;
}




Bsimplex*
Bface::ndc_walk(
   CNDCpt& target, 
   CWvec &passed_bc, 
   CNDCpt &nearest,
   int is_on_tri, 
   bool use_passed_in_params) const
{
  // just like local_search, but in NDC space
   //
   // start from this face, move in NDC space
   // across the mesh to reach the target
   //
   // if reached, return the simplex that contains
   // the target point.
   // we only move if it will get us closer to the goal.  Hence, we
   // can never wander off forever.

   // if can't reach it, return 0

   NDCpt y (nearest);
   Wvec  bc(passed_bc);

   if (!use_passed_in_params) {
      y = nearest_pt_ndc(target, bc, is_on_tri);
   }
   Bsimplex* sim   = bc2sim(bc);

   if (is_on_tri) {
      // target is on this triangle
      // return the lowest-dimensional
      // simplex on which it lies
      return sim;
   }

   if (is_edge(sim)) {
      Bedge* e = (Bedge*)sim;
      Bface* f = e->is_sil() ? (Bface*)0 : e->other_face(this);
      if (f) {
         Wvec new_bc;
         int  new_on_tri;
         NDCpt new_best = f->nearest_pt_ndc(target, new_bc, new_on_tri);
         if (new_best.dist_sqrd(target) < y.dist_sqrd(target))
            return f->ndc_walk(target, new_bc, new_best, new_on_tri, true); 
         else 
            return 0;
      } else {
         return 0;
      }
   }

   // better be a vertex
   assert(is_vert(sim));
   Bvert* v = (Bvert*)sim;
   if (v->degree(SilEdgeFilter()) > 0)
      return 0;

   Bface_list nbrs(16);
   ((Bvert*)sim)->get_faces(nbrs);
   double dist_sqrd = 1e50;
   Bface* best = 0;
   Wvec   best_bc;
   NDCpt  best_nearest;
   int    best_on_tri = 0;
   Wvec   curr_bc;
   NDCpt  curr_nearest;
   int    curr_on_tri=0;

   for (int k = 0; k < nbrs.num(); k++) {
      if (nbrs[k] != this) {
         curr_nearest = nbrs[k]->nearest_pt_ndc(target, curr_bc, curr_on_tri);
         if (curr_nearest.dist_sqrd(target) < dist_sqrd ) {
            dist_sqrd = curr_nearest.dist_sqrd(target);
            best_bc = curr_bc;
            best_on_tri = curr_on_tri;
            best_nearest = curr_nearest;
            best = nbrs[k];
         }
      }
   }

   if (dist_sqrd < y.dist_sqrd(target)) {
      return best->ndc_walk(target, best_bc, best_nearest, best_on_tri, true); 
   }

   return 0;
}

Bface*
Bface::plane_walk(Bedge* cur_edge, CWplane& plane, Bedge*& next_edge) const
{
   int i;
   assert(cur_edge == 0 || cur_edge->which_side(plane) == 0);

   for (i=1; i<4; i++)
      if (e(i) != cur_edge && e(i)->which_side(plane)==0)
         break;
   
   if (i==4)
      return 0;

   assert(e(i)->which_side(plane)==0);
   
   next_edge = e(i);
   return e(i)->other_face(this);
}

bool
Bface::ray_intersect(
   CWpt& p,                   // point on ray
   CWvec& r,                  // direction of ray
   Wpt& hit,                  // returned intersect point
   double& depth) const       // distance from hit to p
{
   // get points of triangle
   CWpt &a=_v1->loc();
   CWpt &b=_v2->loc();
   CWpt &c=_v3->loc();

   // now find scalar t such that  d = p + rt  is in plane of face...
   // can't proceed if dot product is zero
   double dot = r * norm();
   if(fabs(dot) < 1e-16)
      return 0;

   // compute t:
   double t = ((a - p) * norm()) / dot;

   // compute point d on ray and in plane of face:
   Wpt d = p + (r * t);

   // test if d is inside triangle
   Wvec da = d-a;
   Wvec ba = b-a;
   Wvec db = d-b;
   if ((cross(da,ba) * cross(da,c-a) <= 0) &&
       (cross(db,ba) * cross(db,c-b) > 0)) {
      hit = d;
      depth = (hit - p).length();
      return 1;
   }
   return 0;
}

bool
Bface::view_intersect(
   CNDCpt& p,           // Screen point at which to do intersect
   Wpt&    nearpt,      // Point on face visually nearest to p
   double& dist,        // Distance from nearpt to ray from camera
   double& d2d,         // Distance in pixels nearpt to p
   Wvec& n              // "normal" at nearpt in world coordinates
   ) const
{
   // (Bsimplex virtual method):
   // Intersection w/ ray from given screen point -- returns the point
   // on the Bface that is nearest to the given screen space point.
   // Note: the returned "near point" and "normal" are both
   //       transformed to world space.

   // Get "eye point" for computing distance.
   // Not sure if this is the same as cam->from(),
   // but it seems to be the way it's done in other
   // intersection code.
   Wpt eye = XYpt(p);

   // Make object-space ray:
   Wline ray = _mesh->inv_xform()*Wline(p); // ray in object space
   Wpt hit;

   // Try for exact intersection:
   double d;
   if (ray_intersect(ray, hit, d)) {
      // Direct hit
      nearpt = _mesh->xform()*hit;
      dist   = nearpt.dist(eye);
      d2d    = PIXEL(nearpt).dist(PIXEL(p));
      n      = (_mesh->inv_xform().transpose()*norm()).normalized();
      return true;
   }

   Wpt    hit1, hit2, hit3;
   double d1 = DBL_MAX, d2 = DBL_MAX, d3 = DBL_MAX;
   Wvec   n1, n2, n3;
   _e1->view_intersect(p, hit1, d, d1, n1);
   _e2->view_intersect(p, hit2, d, d2, n2);
   _e3->view_intersect(p, hit3, d, d3, n3);

   // Rename so d1 represents closest hit
   if (d1 > d2) {
      swap(d1, d2);
      swap(hit1, hit2);
      swap(n1, n2);
   }
   if (d1 > d3) {
      swap(d1, d3);
      swap(hit1, hit3);
      swap(n1, n3);
   }
   nearpt = _mesh->xform()*hit1;
   dist   = nearpt.dist(eye);
   d2d    = PIXEL(nearpt).dist(PIXEL(p));
   n      = n1;
   return true;
}

inline Wpt
pt_near_seg(CWpt& a, CWpt& b, CWpt& p, double& t)
{
   // mlib version of this is not as good
   Wvec v = (b - a);
   double vv = v*v;
   t = (vv < gEpsZeroMath) ? 0 : clamp(((p - a)*v)/vv,0.0,1.0);
   return (a + v*t);
}

inline NDCpt
pt_near_seg_ndc(CNDCpt& a, CNDCpt& b, CNDCpt& p, double& t)
{
   // mlib version of this is not as good
   NDCvec v = (b - a);
   double vv = v*v;
   t = (vv < gEpsZeroMath) ? 0 : clamp(((p - a)*v)/vv,0.0,1.0);
   return (a + v*t);
}

inline void
get_other_face(CBedge* e, CBface* f, Bsimplex_list& ret)
{
   assert(e && f && f->contains(e));
   if (e->is_weak())
      return;
   Bface* g = e->other_face(f);
   if (!g)
      return;
   ret += g;
   g = g->quad_partner();
   if (g)
      ret += g;
}

inline Bsimplex_list
bfa_to_bsa(CBface_list& faces)
{
   Bsimplex_list ret(faces.num());
   for (int i=0; i<faces.num(); i++)
      ret += faces[i];
   return ret;
}

Bsimplex_list
Bface::neighbors() const
{
   // XXX - still in progress of figuring out what this should mean

   // try the one-ring faces:

   // XXX - need Bsimplex_list constructor from Bface_list etc.

   return bfa_to_bsa(Bface_list((Bface*)this).one_ring_faces());

   Bsimplex_list ret(9);
   ret += _v1;
   ret += _v2;
   ret += _v3;
   ret += _e1;
   ret += _e2;
   ret += _e3;
   get_other_face(_e1, this, ret);
   get_other_face(_e2, this, ret);
   get_other_face(_e3, this, ret);
   return ret;
}

Bsimplex* 
Bface::bc2sim(CWvec& bc) const 
{
   return (
      (bc[0]==1) ? (Bsimplex*)_v1 :
      (bc[1]==1) ? (Bsimplex*)_v2 :
      (bc[2]==1) ? (Bsimplex*)_v3 :
      (bc[0]==0) ? (Bsimplex*)_e2 :
      (bc[1]==0) ? (Bsimplex*)_e3 :
      (bc[2]==0) ? (Bsimplex*)_e1 :
      (Bsimplex*)this
      );
}

bool 
Bface::local_search(
   Bsimplex            *&end, 
   Wvec             &final_bc,
   CWpt             &target, 
   Wpt              &reached,
   Bsimplex         *repeater,
   int               iters) 
{
   // this is a hack to prevent recursion that goes too deep.
   // however if that is a problem the real cause should be
   // tracked down and fixed.

   if (iters <= 0)
      return 0;

   Wvec      bc;
   bool      is_on_tri = 0;
   Wpt       nearpt    = nearest_pt(target, bc, is_on_tri);
   Bsimplex* sim       = bc2sim(bc);

   if (!is_on_tri) {
      
      if (sim == repeater) // We hit a loop in the recursion, so stop
         return 0;

      if (sim != this) { // We're on the boundary of the face
         if (is_edge(sim)) {
            Bedge* e = (Bedge*)sim;

            // Can't cross a border
            if (!e->is_border()) {

               // recurse starting from the other face.
               int good_path = e->other_face(this)->local_search(
                  end, final_bc, target, reached, sim, iters-1);
               if (good_path == 1)
                  return 1;
               else if (good_path == -1)
                  return repeater ? true : false; // XXX - changed from -1 : 0 -- should check
            }

            else {
               return repeater ? true : false; // XXX - changed from -1 : 0 -- should check
            }
         } else {
            // Try to follow across a vertex
            Bface_list near_faces(16);
            assert(is_vert(sim));
            ((Bvert*)sim)->get_faces(near_faces);
            for (int k = near_faces.num()-1; k>=0; k--) {
               if (near_faces[k] != this) {
                  int good_path = near_faces[k]->local_search(
                           end, final_bc, target, reached, sim, iters-1);
                  if (good_path == 1)
                     return 1;
                  else if (good_path==-1)
                     return repeater ? true : false; // XXX - changed from -1 : 0 -- should check
               }
            }
         }
      }
   }

   reached = nearpt;
   end = this;
   final_bc = bc;

   return 1;
}

inline void
snap(Wvec& bc)
{
   // bc is a barycentric coord vector
   // set near-zero values to 0,
   // renormalize
   //
   // should be a class for barycentric coords
   // and this should be a method of the class

   if (fabs(bc[0]) < gEpsZeroMath) bc[0] = 0;
   if (fabs(bc[1]) < gEpsZeroMath) bc[1] = 0;
   if (fabs(bc[2]) < gEpsZeroMath) bc[2] = 0;

   double sum = bc[0] + bc[1] + bc[2];

   bc /= sum;
}

Wpt 
Bface::nearest_pt(CWpt& p, Wvec &bc, bool &is_on_tri) const 
{
   // returns the point on this face that is closest to p
   // also returns the barycentric coords of the near point.

   // get barycentric coords:
   project_barycentric(p, bc);
   
   // to account for numerical errors, snap
   // near-zero values to 0 and renormalize
   snap(bc);

   Wpt ret;
   if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0) {
      // projected point is outside the triangle.
      // find closest point to an edge:
      is_on_tri = 0;
      double t1, t2, t3;
      CWpt& a = _v1->loc();
      CWpt& b = _v2->loc();
      CWpt& c = _v3->loc();
      Wpt p1 = pt_near_seg(a,b,p,t1);
      Wpt p2 = pt_near_seg(b,c,p,t2);
      Wpt p3 = pt_near_seg(c,a,p,t3);
      double d1 = (p1 - p).length_sqrd();
      double d2 = (p2 - p).length_sqrd();
      double d3 = (p3 - p).length_sqrd();

      if (d1 < d2) {
         if (d1 < d3) {
            bc.set(1-t1,t1,0);
            return p1;
         }
         bc.set(t3,0,1-t3);
         return p3;
      }
      if (d2 < d3) {
         bc.set(0,1-t2,t2);
         return p2;
      }
      bc.set(t3,0,1-t3);
      return p3;
   }

   is_on_tri = 1;
   bc2pos(bc, ret);
      
   return ret;
}

inline double
signed_area_ndc(CNDCpt& a, CNDCpt& b, CNDCpt& c)
{
   return 0.5 * det(b-a,c-a);
}

NDCpt 
Bface::nearest_pt_ndc(CNDCpt& p, Wvec &bc, int &is_on_tri) const 
{
   // Bsimplex virtual method

   // same as above, but operates in NDC space

   // get barycentric coords:
   NDCpt a = _v1->ndc();
   NDCpt b = _v2->ndc();
   NDCpt c = _v3->ndc();
   double A = signed_area_ndc(a, b, c);
   double u = signed_area_ndc(p, b, c) / A;
   double v = signed_area_ndc(a, p, c) / A;
   bc.set(u, v, 1 - u - v);
   
   // to account for numerical errors, snap
   // near-zero values to 0 and renormalize
   snap(bc);

   if (bc[0] < 0 || bc[1] < 0 || bc[2] < 0) {
      // p is outside the triangle.
      // find closest point to an edge:
      is_on_tri = 0;
      double t1, t2, t3;

      NDCpt p1 = pt_near_seg_ndc(a,b,p,t1);
      NDCpt p2 = pt_near_seg_ndc(b,c,p,t2);
      NDCpt p3 = pt_near_seg_ndc(c,a,p,t3);

      double d1 = p.dist_sqrd(p1);
      double d2 = p.dist_sqrd(p2);
      double d3 = p.dist_sqrd(p3);

      if (d1 < d2) {
         if (d1 < d3) {
            bc.set(1-t1,t1,0);
            return p1;
         }
         bc.set(t3,0,1-t3);
         return p3;
      }
      if (d2 < d3) {
         bc.set(0,1-t2,t2);
         return p2;
      }
      bc.set(t3,0,1-t3);
      return p3;
   }

   is_on_tri = 1;
   return (a*bc[0]) + (b*bc[1]) + (c*bc[2]);
}

// Similar to above, but returns the hit point 
// both in local and barycentric coords:
Wpt
Bface::near_pt(CNDCpt& ndc, Wvec& hit_bc) const
{
   // XXX - failure is not an option?
   return Bsimplex::nearest_pt(plane_intersect(nearest_pt_ndc(ndc)), hit_bc);
}

int 
Bface::detach() 
{
   // tell edges to forget about this face
   int ret = 1;

   if (_e1) ret = (*_e1 -= this) && ret;
   if (_e2) ret = (*_e2 -= this) && ret;
   if (_e3) ret = (*_e3 -= this) && ret;

   _e1 = _e2 = _e3 = 0;

   return ret;
}

void  
Bface::reverse() 
{
   // do it:
   swap(_v2,_v3);
   swap(_e1,_e3);
   if (_tc)
      swap(tex_coord(2),tex_coord(3));

   // tell patch tri strips are invalid:
   if (_patch)
      _patch->triangulation_changed();
   _orient = 0;

   geometry_changed();
}

void 
Bface::make_primary()
{
   clear_bit(SECONDARY_BIT); 
   geometry_changed();
}

void 
Bface::make_secondary() 
{
   set_bit(SECONDARY_BIT, 1); 
   geometry_changed();
}

bool 
Bface::check() const
{
   if (!(_v1 && _v2 && _v3 && _e1 && _e2 && _e3))
      return false;

   return ((_v1->lookup_edge(_v2) ==_e1) &&
           (_v2->lookup_edge(_v3) ==_e2) &&
           (_v3->lookup_edge(_v1) ==_e3));
}

bool 
Bface::redef2(Bedge *a, Bedge *b)
{
   static bool debug = Config::get_var_bool("DEBUG_MESH_OPS",false);

   // redefine this face, replacing a with b
   // update the old edge to forget this face,
   // and the new edge to remember it.

   // preconditions:
   //   a is an edge of this face.
   //   face does not already contain b.

   if (!contains(a)) {
      err_adv(debug, "Bface::redef2: error: face doesn't contain a");
      return false;
   }
   if (contains(b)) {
      err_adv(debug, "Bface::redef2: error: face already contains b");
      return false;
   }

   *a -= this;

   if      (a == _e1)   _e1 = b;
   else if (a == _e2)   _e2 = b;
   else if (a == _e3)   _e3 = b;
   else                 assert(0);

   *b += this;

   geometry_changed();

   return true;
}

bool
Bface::redef2(Bvert *a, Bvert *b)
{
   // Redefine this face, replacing vert a with vert b.

   // Different from Bface::redefine() below: here we assume
   // the edges are also being redefined to switch from
   // vert a to vert b, so we don't shed our current edges
   // and try to find our new ones.

   // precondition:
   //    a is a vertex of this face.
   //    b is not a vertex of this face.

   static bool debug = Config::get_var_bool("DEBUG_MESH_OPS",false);
   if (!contains(a)) {
      err_adv(debug, "Bface::redef2: error: vert to be replaced not found");
      return false;
   }
   if (contains(b)) {
      err_adv(debug, "Bface::redef2: error: new vert already in face");
      return false;
   }

   if      (_v1 == a)   _v1 = b;
   else if (_v2 == a)   _v2 = b;
   else if (_v3 == a)   _v3 = b;
   else                 assert(0);

   // tell patch strips are invalid:
   if (_patch)
      _patch->triangulation_changed();
   _orient = 0;

   // invalidate cached state in this face
   // and neighboring simplices:
   geometry_changed();

   return true;
}

int
Bface::redefine(Bvert *v, Bvert *u)
{
   // redefine the face, replacing vertex v with u

   // precondition:
   //    v is a vertex of this face.
   //    u is not a vertex of this face.
   //    this face has already detached from its edges.
   //    new edges have already been created appropriately.
   //    new face will not duplicate an existing face

   static bool debug = Config::get_var_bool("DEBUG_MESH_OPS",false);

   if (!contains(v)) {
      err_adv(debug, "Bface::redefine: error: old vert not found");
      return 0;
   }

   if (contains(u)) {
      err_adv(debug, "Bface::redefine: error: new vert already in face");
      return 0;
   }

   if      (_v1 == v)   _v1 = u;
   else if (_v2 == v)   _v2 = u;
   else if (_v3 == v)   _v3 = u;
   else                 assert(0);

   assert(!lookup_face(_v1, _v2, _v3));

   Bedge* e1 = _v1->lookup_edge(_v2);
   Bedge* e2 = _v2->lookup_edge(_v3);
   Bedge* e3 = _v3->lookup_edge(_v1);

   if (!(e1 && e2 && e3)) {
      err_adv(debug, "Bface::redefine: error: 1 or more edges not defined");
      return 0;
   }

   // coast is clear -- do it
   _e1 = e1;
   _e2 = e2;
   _e3 = e3;

   *_e1 += this;
   *_e2 += this;
   *_e3 += this;

   // tell patch strips are invalid:
   if (_patch)
      _patch->triangulation_changed();
   _orient = 0;

   // invalidate cached state in this face
   // and neighboring simplices:
   geometry_changed();

   return 1;
}

int
Bface::redefine(
   Bvert *u, Bvert *nu,
   Bvert *v, Bvert *nv)
{
   // redefine the face, replacing vertices u and v with nu and nv

   // precondition:
   //    u,v are vertices of this face.
   //    this face has already detached from its edges.
   //    new edges have already been created appropriately.
   //    new face will not duplicate an existing face

   if (!contains(u) || !contains(v))
      return 0;

   if      (u == _v1)   _v1 = nu;
   else if (u == _v2)   _v2 = nu;
   else if (u == _v3)   _v3 = nu;
   else                 return 0;

   if      (v == _v1)   _v1 = nv;
   else if (v == _v2)   _v2 = nv;
   else if (v == _v3)   _v3 = nv;
   else                 return 0;

   Bedge* e1 = _v1->lookup_edge(_v2);
   Bedge* e2 = _v2->lookup_edge(_v3);
   Bedge* e3 = _v3->lookup_edge(_v1);

   if (!e1 || !e2 || !e3 ||
      lookup_face(_v1, _v2, _v3)) {
      return 0;
   }
      
   // coast is clear -- do it
   _e1 = e1;
   _e2 = e2;
   _e3 = e3;

   *_e1 += this;
   *_e2 += this;
   *_e3 += this;

   // tell patch strips are invalid:
   if (_patch)
      _patch->triangulation_changed();
   _orient = 0;

   // invalidate cached state in this face
   // and neighboring simplices:
   geometry_changed();

   return 1;
}

CWvec&
Bface::vert_normal(CBvert* v, Wvec& n) const
{
   // for gouraud shading: return appropriate 
   // normal to use at a vertex of this face 

   assert(this->contains(v));

   if(!v->is_crease()) {
      n = v->norm();
      return n;
   }

   // take average of face normals in star of v,
   // using faces which can be reached from this
   // face without crossing a crease edge

   n = weighted_vnorm(this, v); // add weighted normal from this face
   int count = 1;       // count of faces processed

   // wind around v in clockwise direction
   // but don't cross a crease edge
   CBface* f = this;
   Bedge* e = edge_from_vert(v);
   for (; e&&!e->is_crease() && (f=e->other_face(f)); e=f->edge_from_vert(v)) {
      n += weighted_vnorm(f, v);
      if (++count > v->degree()) {
         // this should never happen, but it does
         // happen on effed up models
         // (i.e. when "3rd faces" get added)
         break;
      }
   }

   // wind around v in counter-clockwise direction;
   // as before, don't cross a crease edge
   f = this;
   e = edge_before_vert(v);
   for(; e&&!e->is_crease()&&(f=e->other_face(f)); e=f->edge_before_vert(v)) {
      n += weighted_vnorm(f, v);
      if(++count > v->degree())
         break;
   }

   n = n.normalized();
   return n;
}

bool
Bface::get_quad_verts(Bvert*& a, Bvert*& b, Bvert*& c, Bvert*& d) const
{
   //  Return CCW verts a, b, c, d as in the picture, orienting
   //  things so that the weak edge runs NE as shown:
   //
   //    d ---------- c = w->v2()              ^      
   //    |          / |                        |       
   //    |        /   |                        |       
   //    |    w /     |       tan1        tan2 |       
   //    |    /       |    -------->           |       
   //    |  /     f   |                        |       
   //    |/           |                                
   //    a ---------- b                                
   //     = w->v1()
   //
   if (!is_quad())
      return 0;

   Bedge* w = weak_edge();
   Bface* f = w->ccw_face(w->v2());
   a = w->v1();
   b = f->next_vert_ccw(a);
   c = w->v2();
   d = f->quad_vert();

   return true;
}



bool 
Bface::get_quad_verts(Bvert_list& verts) const 
{
   verts.clear();
   Bvert *v1=0, *v2=0, *v3=0, *v4=0;
   if (!get_quad_verts(v1,v2,v3,v4))
      return 0;
   verts += v1;
   verts += v2;
   verts += v3;
   verts += v4;
   return 1;

}
bool
Bface::get_quad_pts(Wpt& a, Wpt& b, Wpt& c, Wpt& d) const 
{
   Bvert *v1=0, *v2=0, *v3=0, *v4=0;
   if (!get_quad_verts(v1,v2,v3,v4))
      return 0;
   a = v1->loc();
   b = v2->loc();
   c = v3->loc();
   d = v4->loc();
   return 1;
}

bool 
Bface::get_quad_edges(Bedge*& a, Bedge*& b, Bedge*& c, Bedge*& d) const
{
	Bvert *v1=0, *v2=0, *v3=0, *v4=0;
   if (!get_quad_verts(v1,v2,v3,v4))
      return 0;

	assert(v1&&v2&&v3&&v4);

   a=v1->lookup_edge(v2);
   b=v2->lookup_edge(v3);
   c=v3->lookup_edge(v4);
   d=v4->lookup_edge(v1);

   assert(a&&b&&c&&d);
   return 1;

}
bool 
Bface::get_quad_edges(Bedge_list& edges)                          const
{
  edges.clear();
   Bedge *e1=0, *e2=0, *e3=0, *e4=0;
   if (!get_quad_edges(e1,e2,e3,e4))
      return 0;
   edges += e1;
   edges += e2;
   edges += e3;
   edges += e4;
   return 1;
}




Wvec
Bface::quad_tan1() const 
{
   // Based on the 4 verts in standard orientation as above,
   // return the tangent vector running right:
   Wpt a, b, c, d;
   get_quad_pts(a,b,c,d);

   Wvec t = ((b - a) + (c - d))*0.5;
   return t.orthogonalized(quad_norm()).normalized();
}

Wvec
Bface::quad_tan2() const 
{
   // Based on the 4 verts in standard orientation as above,
   // return the tangent vector running up
   Wpt a, b, c, d;
   get_quad_pts(a,b,c,d);

   Wvec t = ((d - a) + (c - b))*0.5;
   return t.orthogonalized(quad_norm()).normalized();
}

double
Bface::quad_dim1() const
{
   // Based on the 4 verts in standard orientation as above,
   // return the average of the two horizontal edge lengths
   Wpt a, b, c, d;
   get_quad_pts(a,b,c,d);

   return (b.dist(a) + c.dist(d))/2;
}

double
Bface::quad_dim2() const
{
   // Based on the 4 verts in standard orientation as above,
   // return the average of the two vertical edge lengths
   Wpt a, b, c, d;
   get_quad_pts(a,b,c,d);

   return (b.dist(c) + a.dist(d))*0.5;
}

UVpt
Bface::quad_bc_to_uv(CWvec& bc) const
{
   // We got some barycentric coords WRT this face (a quad),
   // and now we want to convert them to uv-coords WRT to
   // the 4 quad vertices in standard order.

   //
   //    d ---------- c                               
   //    |          / |                                
   //    |        /   |                                
   //    |      /     |                                
   //    |    /       |                                
   //    |  /         |                                
   //    |/           |                                
   //    a ---------- b                                
   //
   //   STANDARD QUAD ORDER
   
   Bvert *a=0, *b=0, *c=0, *d=0;
   if (!get_quad_verts(a, b, c, d)) {
      err_msg("Bface::quad_bc_to_uv: Error: can't get quad verts");
      return UVpt();
   }

   // Barycentric coords are given WRT to vertices v1, v2, v3.
   // We want them WRT:
   //   a, b, c (lower face), or
   //   a, c, d (upper face).
   //
   // k = 0, 1, or 2 according to whether a = v1, v2, or v3.
   int k = vindex(a) - 1;
   if (k < 0 || k > 2) {
      err_msg("Bface::quad_bc_to_uv: Error: can't get oriented");
      return UVpt();
   }
   if (contains(b)) {
      // We are the lower face.
      return UVpt(1 - bc[k], bc[(k + 2)%3]);
   } else if (contains(d)) {
      // We are the upper face.
      return UVpt(bc[(k + 1)%3], 1 - bc[k]);
   } else {
      // Should not happen
      err_msg("Bface::quad_bc_to_uv: Error: can't get oriented");
      return UVpt();
   }
}

Wpt
Bface::quad_uv2loc(CUVpt& uv) const
{
   // Maps the given uv coordinate (WRT the quad) to a Wpt:
   //
   //
   //    d ---------- c                               
   //    |          / |                                
   //    |        /   |                                
   //    |      /     |                                
   //    |    /       |                                
   //    |  /         |                                
   //    |/           |                                
   //    a ---------- b                                
   //
   //   STANDARD QUAD ORDER
   
   Bvert *a=0, *b=0, *c=0, *d=0;
   if (!get_quad_verts(a, b, c, d)) {
      err_msg("Bface::quad_uv2loc: Error: can't get quad verts");
      return Wpt();
   }

   // ensure coordinates are interior to quad
   double u = clamp(uv[0], 0.0, 1.0);
   double v = clamp(uv[1], 0.0, 1.0);

   // lower triangle
   if (v <= u) {
      double w = ((u == 0) ? 0.0 : (v/u));
      return interp(interp(a->loc(), b->loc(), u),
                    interp(a->loc(), c->loc(), u), w);
   } else {
      double w = ((u == 1) ? 1.0 : ((v - u)/(1 - u)));
      return interp(interp(a->loc(), c->loc(), u),
                    interp(d->loc(), c->loc(), u), w);
   }
}

/************************************************************
 * Bface_list
 ************************************************************/

void
Bface_list::clear_vert_flags() const
{   
   // Clear the flag of each vertex of each face

   for (int i=0; i<_num; i++)
      for (int j=1; j<4; j++)
         _array[i]->v(j)->clear_flag();
}

void
Bface_list::clear_edge_flags() const
{   
   // Clear the flag of each edge of each face:
   for (int i=0; i<_num; i++)
      for (int j=1; j<4; j++)
         _array[i]->e(j)->clear_flag();
}

Bvert_list 
Bface_list::get_verts() const
{
   // Extract a list of the verts found in the given faces.

   // Get clean slate
   clear_vert_flags();

   // Put verts into output array uniquely:
   Bvert_list ret(_num);       // pre-allocate plenty
   for (int i=0; i<_num; i++) {
      for (int j=1; j<4; j++) {
         Bvert* v = _array[i]->v(j);
         if (!v) {
            err_msg("Bface_list::get_verts: null vert");
         } else if (v->flag() == 0) {
            v->set_flag(1);
            ret += v;
         }
      }
   }
   return ret;
}

Bedge_list 
Bface_list::get_edges() const
{
   // Extract a list of the edges found in the given faces.

   // Get clean slate
   clear_edge_flags();

   // Put edges into output array uniquely:
   Bedge_list ret(_num*2);       // pre-allocate plenty
   for (int i=0; i<_num; i++) {
      for (int j=1; j<4; j++) {
         Bedge* e = _array[i]->e(j);
         if (e->flag() == 0) {
            e->set_flag(1);
            ret += e;
         }
      }
   }
   return ret;
}

Wvec
Bface_list::avg_normal() const
{
   // Returns the average of the face normals
   Wvec ret;
   for (int i=0; i<_num; i++)
      ret += _array[i]->norm();
   return ret.normalized();
}

double
Bface_list::max_norm_deviation(CWvec& n) const
{
   // Returns the maximum deviation, in radians, of a normal from the
   // value passed in

   double ret = 0;
   for (int i=0; i<_num; i++)
      ret = max(ret, n.angle(_array[i]->norm()));
   return ret;
}

bool 
Bface_list::same_patch() const 
{
   for (int k=1; k<num(); k++) {
      if ((*this)[k-1]->patch() != (*this)[k]->patch())
         return false;
   }
   return true;
}

Patch*
Bface_list::get_patch() const 
{
   return same_patch() && !empty() ? first()->patch() : 0;
}

double 
Bface_list::volume() const 
{
   // Returns the sum of the "volume elements" for each face.
   // Really only makes sense for a list of faces forming a
   // closed surface; though if the faces form a nearly-closed
   // surface it can tell you if they are oriented mostly
   // inside-out or not. (Negative volume == inside out.)
   // Uses the divergence theorem.

   double ret = 0;
   for (int k=0; k<_num; k++)
      ret += _array[k]->volume_el();
   return ret;
}

void
Bface_list::mark_faces() const
{
   // Set the flag of each face to 1, and clear the flags of
   // each face around the boundary of this set of faces.

   // Clear vertex, edge, and face flags within a 1-neighborhood
   // of any vertex of this set of faces:
   get_verts().clear_flag02();

   // Set face flags to 1:
   set_flags(1);
}

Bface_list
Bface_list::exterior_faces() const 
{
   // Like the one-ring, but doesn't include faces of this set:

   Bface_list ret = one_ring_faces();
   mark_faces();
   return ret.filter(SimplexFlagFilter(0));
}

inline Bface*
check_partner(Bface* f)
{
   // helper function used in quad_complete_faces() below

   if (!(f && f->is_quad()))
      return 0;
   Bface* p = f->quad_partner();
   return p->flag() ? 0 : p;
}

Bface_list
Bface_list::quad_complete_faces() const
{
   // If the list contains any "quad" faces but not their partners,
   // returns the "completed" list, containing the missing partners:

   mark_faces();
   Bface* p=0;
   Bface_list ret = *this;
   for (int i=0; i<_num; i++)
      if ((p = check_partner(_array[i])))
         ret.add(p);
   return ret;
}

Bface_list
Bface_list::reachable_faces(Bface* f, CSimplexFilter& pass)
{
   // Returns the list of faces reachable from f, crossing
   // only edges accepted by the filter. It sets needed
   // flags (on the entire mesh) then calls grow_connected().

   Bface_list ret;

   if (!(f && f->mesh()))
      return ret;

   // Ensure all reachable face flags are set to 1:

   // XXX -
   //   touches every face in the mesh; a better
   //   implementation would just touch reachable ones
   //   (or is that impossible to implement?)
   f->mesh()->faces().set_flags(1);

   ret.grow_connected(f, pass);
   return ret;
}

bool
Bface_list::grow_connected(Bface* f, CSimplexFilter& pass)
{
   // Collect all reachable faces whose flag == 1, starting at f,
   // crossing only edges that are accepted by the given filter.

   if (!(f && f->flag() == 1))
      return false;

   f->set_flag(2);
   add(f);

   // check each neighboring edge:
   for (int i=1; i<4; i++) {
      Bedge* e = f->e(i);
      if (pass.accept(e)) {
         // check each adjacent face
         // (includes this, but that will be a no-op):
         for (int j=1; j<=e->num_all_faces(); j++)
            grow_connected(e->f(j), pass);
      }
   }

   return true;
}

bool
Bface_list::is_connected() const
{
   // Returns true if the faces form a single connected component:
   // Note: two faces that share a vertex but not an edge are
   //       NOT connected.

   // reject if empty or not all from one mesh:
   if (mesh() == NULL)
      return false;

   // Find all faces connected to the first triangle.
   // if the number found is the number in the entire list,
   // the whole list is connected.

   // Mark internal faces with flag = 1,
   // clear flags in the 1-ring around these faces:
   mark_faces();
                                                                                
   // Find the component that contains the first face.
   // The search will not go outside this Bface_list,
   // because every neighboring outside face has flag == 0
   // and will therefore be ignored.
   Bface_list comp1(_num);
   comp1.grow_connected(first());

   bool debug = Config::get_var_bool("DEBUG_BFACE_LIST_IS_CONNECTED", false);

   err_adv(debug, "Bface_list::is_connected:");
   err_adv(debug, "  nfaces: %d, comp 1: %d, connected: %s",
           _num, comp1.num(), ((comp1.num() == _num) ? "yes" : "no"));

   // Return true iff that component is the whole Bface_list:
   return comp1.num() == _num;
}

bool
Bface_list::is_disk() const
{
   // Returns true if the set is connected, the boundary is a simple
   // loop, and the faces form a 2-manifold (WRT faces of this set):

   return (is_connected() &&
           get_boundary().num_line_strips() == 1 &&
           is_2_manifold());
}

EdgeStrip
Bface_list::get_boundary() const
{
   // Returns the boundary of this set of faces.
   // I.e., returns an edge strip describing chains of edges
   // that wind CCW around the faces in this list.

   // Mark internal faces with flag = 1,
   // neighboring faces outside the list with flag = 0:
   mark_faces();

   EdgeStrip ret;
   ret.build_ccw_boundaries(get_edges(), SimplexFlagFilter(1));
   return ret;
}

Bedge_list 
Bface_list::boundary_edges() const 
{
   return get_boundary().edges();
}

Bedge_list 
Bface_list::interior_edges() const 
{
   // Return list of edges that are not adjacent to any external face.

   // mark internal faces w/ flag 1, external faces with flag 0:
   mark_faces(); 
   return get_edges().filter(!BoundaryEdgeFilter(SimplexFlagFilter(1)));
}

Bvert_list 
Bface_list::interior_verts() const 
{
   // Return list vertices that are not adjacent to any external face.

   Bvert_list ret = get_verts();
   ret -= get_boundary().verts();
   return ret;

   // mark internal faces w/ flag 1, external faces with flag 0:
   mark_faces(); 
   return get_verts().filter(VertFaceDegreeFilter(0, SimplexFlagFilter(0)));
}

bool
Bface_list::is_consistently_oriented() const
{
   // Returns true if no edge *interior* to this set of faces 
   // has 2 adjacent faces with inconsistent orientation:

   SimplexFlagFilter    me(1);                  // accepts my faces (flag = 1)
   BoundaryEdgeFilter   boundary(me);           // accepts my boundary edges
   NotFilter            interior(boundary);     // accepts my interior edges
   ConsistentEdgeFilter consistent;
   NotFilter            inconsistent(consistent); // accepts inconsistent edges

   // Mark internal faces with flag = 1,
   // neighboring faces outside the list with flag = 0:
   mark_faces();

   if (Config::get_var_bool("DEBUG_INFLATE_ALL",false)) {
      err_msg("Total faces: %d", _num);
      err_msg("Total edges: %d", get_edges().num());
      err_msg("Boundary edges: %d", get_edges().filter(boundary).num());
      err_msg("Interior edges: %d", get_edges().filter(!boundary).num());
      err_msg("Inconsistent edges: %d", get_edges().filter(inconsistent).num());
   }

   return get_edges().filter(interior + inconsistent).empty();
}


bool
Bface_list::is_2_manifold() const
{
   // Returns true if every contained edge is adjacent to no more than
   // 2 faces of *this* face set:

   // Mark these faces 1, all adjacent ones 0
   mark_faces();

   // Returns true if no edge is adjacent to more than 2 of *these* faces
   return get_edges().all_satisfy(ManifoldEdgeFilter(SimplexFlagFilter(1)));
}

bool 
Bface_list::can_push_layer() const
{
   // Reports whether it valid to call push_layer() on this face set.
   //
   // Preliminary version.

   // If the region is already entirely secondary,
   // our job is done, so report "success":
   if (is_all_secondary())
      return true;

   return (!empty() && same_mesh());
}

inline void
demote(Bface* f)
{
   // Used by Bface_list::push_layer() below.
   // If any adjacent edge has its flag set,
   // then demote this face WRT that edge.

   if (!f) return;
   if (f->e1()->flag()) f->e1()->demote(f);
   if (f->e2()->flag()) f->e2()->demote(f);
   if (f->e3()->flag()) f->e3()->demote(f);
}

bool 
Bface_list::push_layer(bool push_boundary) const
{
   // Given a set of faces from a single mesh, mark the
   // faces as a separate "layer" of the mesh, with
   // lower priority than the primary, manifold layer:

   if (!can_push_layer()) {
      err_msg("Bface_list::push_layer: invalid operation");
      return false;
   }

   // If the region is already entirely secondary,
   // our job is done, so report "success":
   if (is_all_secondary())
      return true;

   // Mark all faces non-primary.
   // (Bface virtual method sets bit Bface::SECONDARY_BIT;
   //  in Lface it's propagated to lower subdiv levels.)
   make_secondary();

   // XXX - hack; still trying to work out the correct policy...
   if (push_boundary) {

      // Distinguish boundary edges by setting their flags to 1
      // while all other flags are 0.
      EdgeStrip bdry = get_boundary();

      // pass the word down the subdiv hierarchy
      // that uv coords are splitting
      UVdata::split(bdry);

      // Now visit boundary edges and ensure faces of this list
      // do not occupy primary _f1 or _f2 slots in each edge.

      get_edges().clear_flags();
//   bdry.edges().filter(InteriorEdgeFilter()).set_flags();
      bdry.edges().set_flags();

      // See inlined demote() above
      for (int i=0; i<num(); i++)
         demote(_array[i]);

   }

   // Notify mesh to rebuild tri-strips, check topology etc.
   assert(mesh());
   mesh()->changed();

   return true;
}

bool 
Bface_list::can_unpush_layer() const
{
   // Reports whether unpush_layer() can successfully be called for
   // this face set.

   // If the region is already entirely primary,
   // our job is done, so report "success":
   if (is_all_primary())
      return true;

   static bool debug = Config::get_var_bool("DEBUG_PUSH_FACES",false);
   if (debug) {
      bool b = boundary_edges().all_satisfy(CanPromoteEdgeFilter());
      cerr << "Bface_list::can_unpush_layer: " << endl
           << "   " << num() << " faces, "
           << (same_mesh() ? "same mesh" : "different meshes") << ", "
           << (is_2_manifold() ? "" : "non-") << "manifold, "
           << "boundary: " << (b ? "good" : "bad")
           << " (" << boundary_edges().num() << " edges)"
           << endl;
      Bedge_list bad_edges = boundary_edges().filter(!CanPromoteEdgeFilter());
      cerr << bad_edges.num() << " bad edges" << endl;
      if (!b) {
         MeshGlobal::select(bad_edges);
      }
   }
   return (!empty() && same_mesh() &&
           is_2_manifold() && 
           boundary_edges().all_satisfy(CanPromoteEdgeFilter()));
}

inline void
promote(Bface* f)
{
   // Used by Bface_list::unpush_layer() below.
   // If any adjacent edge has its flag set,
   // then promote this face WRT that edge.

   if (!f) return;
   if (f->e1()->flag()) f->e1()->promote(f);
   if (f->e2()->flag()) f->e2()->promote(f);
   if (f->e3()->flag()) f->e3()->promote(f);
}

bool 
Bface_list::unpush_layer(bool unpush_boundary) const
{
   // Given a set of faces from a single mesh,
   // restore the faces to "primary" status.

   if (!can_unpush_layer()) {
      err_msg("Bface_list::unpush_layer: invalid operation");
      return false;
   }

   // If the region is already entirely primary,
   // our job is done, so report "success":
   if (is_all_primary())
      return true;

   // Mark all faces primary
   // (virtual method propagates change to lower subdiv levels)
   make_primary();

   if (unpush_boundary) {
      // Ensure all edge flags are clear except the boundary ones.
      EdgeStrip bdry = get_boundary();
      get_edges().clear_flags();
      bdry.edges().set_flags();

      // See inlined promote() above
      for (int i=0; i<num(); i++)
         promote(_array[i]);
   }

   // Notify mesh to rebuild tri-strips, check topology etc.
   assert(mesh());
   mesh()->changed();

   return true;
}

/* end of file bface.C */
