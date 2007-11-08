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
 * bedge.C
 **********************************************************************/
#include "disp/ray.H"
#include "mesh/bmesh.H"
#include "mesh/patch.H"
#include "std/run_avg.H"
#include "std/config.H"
#include "bfilters.H"
#include "uv_data.H"
#include "tex_coord_gen.H"

using namespace mlib;

Bedge::Bedge(Bvert* u, Bvert* v) :
   _v1(u),
   _v2(v),
   _f1(0),
   _f2(0),
   _adj(0),
   _sil_stamp(0)
{
   *_v1 += this;
   *_v2 += this;
}

Bedge::~Bedge()
{
   // remove self from global selection list, if needed
   if (is_selected())
      MeshGlobal::deselect(this);

   // adjacent faces may not outlive this edge
   if (_mesh) {
      if (_f1) _mesh->remove_face(_f1);
      if (_f2) _mesh->remove_face(_f2);
      if (_adj) {
         for (int i=0; i<_adj->num(); i++)
            _mesh->remove_face((*_adj)[i]);
      }
   }   

   if (!detach()) 
      err_msg("Bedge::~Bedge: can't detach");
}

int
Bedge::index() const
{
   // index in BMESH Bedge array:

   if (!_mesh) return -1;
   return _mesh->edges().get_index((Bedge*)this);
}
   
int
Bedge::num_all_faces() const
{
   // Number of faces, including multi faces:

   int ret = nfaces();
   if (_adj)
      ret += _adj->num();
   return ret;
}

Bface_list 
Bedge::get_all_faces() const
{
   // return a list of all adjacent faces, including f1 and f2:

   Bface_list ret(num_all_faces());
   if (_adj) ret = *_adj;
   if (_f1) ret += _f1;
   if (_f2) ret += _f2;
   return ret;
}

Bface* 
Bedge::f(int k) const
{
   switch (k) {
    case 1: return _f1;
    case 2: return _f2;
   }
   k -= 3;
   return (_adj && _adj->valid_index(k)) ? (*_adj)[k] : 0;
}

int
Bedge::nfaces() const
{
   // XXX - not counting "multi" edges at this time

   return (_f1?1:0) + (_f2?1:0); // + (_adj?_adj->num():0);
}

Bface*
Bedge::lookup_face(CBvert *v) const
{
   // Reject null pointers.
   // Also, it's not helpful if the vertex belongs
   // to this edge.
   if (!v || this->contains(v))
      return 0;
   // Return an adjacent Bface containing the vertex
   
   if (_f1 && _f1->contains(v))
      return _f1;
   if (_f2 && _f2->contains(v))
      return _f2;
   if (_adj) {
      for (int i=0; i<_adj->num(); i++) {
         Bface* f = (*_adj)[i];
         if (f->contains(v))
            return f;
      }
   }
   return 0;
}

Bface* 
Bedge::lookup_face(CBedge *e) const 
{
   // Reject null pointers.
   // Also, it's not helpful if the edge is this one.
   if (!e || this == e)
      return 0;

   // Return an adjacent Bface containing the edge
   
   if (_f1 && _f1->contains(e))
      return _f1;
   if (_f2 && _f2->contains(e))
      return _f2;
   if (_adj) {
      for (int i=0; i<_adj->num(); i++) {
         Bface* f = (*_adj)[i];
         if (f->contains(e))
            return f;
      }
   }
   return 0;
}

int 
Bedge::detach() 
{
   if (nfaces() > 0 || is_multi()) {
      err_msg("Bedge::detach: error: faces present");
      return 0;
   }
   int ret = (*_v1 -= this);
   return (*_v2 -= this) && ret;
}

Wvec 
Bedge::vec() const
{
   // Vector from vertex 1 to vertex 2
   return (_v2->loc() - _v1->loc()); 
}

Wline
Bedge::line() const
{
   return Wline(_v1->loc(),_v2->loc()); 
}

PIXELline
Bedge::pix_line() const
{
	return PIXELline(_v1->pix(), _v2->pix());
}

bool
Bedge::ndc_intersect(
   CNDCpt& p,
   CNDCpt& q,
   NDCpt&  ret) const
{
   return NDCline(_v1->ndc(), _v2->ndc()).intersect_segs(NDCline(p, q), ret);
}

bool
Bedge::view_intersect(
   CNDCpt& p,           // Screen point at which to do intersect
   Wpt&    nearpt,      // Point on edge visually nearest to p
   double& dist,        // Distance from nearpt to ray from camera
   double& d2d,         // Distance in pixels nearpt to p
   Wvec& n              // "normal" at nearpt in world space
   ) const
{
   // (Bsimplex virtual method):
   // Intersection w/ ray from given screen point -- returns the point
   // on the Bedge that is nearest to the given screen space point.
   // Note: the returned "near point" and "normal" are both
   //       transformed to world space.

   // Find nearest point on the edge in screen-space, and make a 3D
   // ray out of it (world space, not object space):
   Wline ray(NDCline(_v1->ndc(), _v2->ndc()).project_to_seg(p));

   // Working in world space (applying mesh xf to verts), find
   // nearest point on the edge to the ray:
   nearpt = Wline(_v1->wloc(), _v2->wloc()).project_to_seg(ray);

   // Compute world and screen distances
   dist = nearpt.dist(ray.point());
   d2d  = PIXEL(nearpt).dist(PIXEL(ray.point()));

   // Return a "normal" vector:
   Wvec n1;
   if (nfaces() == 2)
      n1 = norm();
   else if (nfaces() == 1)
      n1 = get_face()->norm();
   else
      n1 = (ray.point() - nearpt).normalized();

   // Transform the normal properly:
   n = (_mesh->inv_xform().transpose()*n1).normalized();

   return 1;
}

bool
Bedge::operator +=(Bface* f) 
{
   if (!(f && f->contains(this))) {
      cerr << "Bedge::operator+=(Bface*): error: "
           << (f ? "face doesn't contain edge" : "null face")
           << endl;
      return 0;
   }

   // Is the face already recorded?
   // (Not supposed to happen):
   if (this->contains(f))
      return true;

   if (_f1 && _f2) {
      if (_f2->is_secondary())
         demote(_f2);
      else if (_f1->is_secondary())
         demote(_f1);
   }
   if (_f1 && _f2) {
      // Can't add as a primary face, so add as a "multi" face:
      bool ret = add_multi(f);
      assert(ret);      // the plan cannot fail
   } else if (_f1) {
      _f2 = f;
   } else {
      _f1 = f;
   }

   faces_changed();
   _v1->star_changed();
   _v2->star_changed();

   return true;
}

bool
Bedge::operator -=(Bface* f) 
{
   if (f == _f1)
      _f1 = 0;
   else if (f == _f2)
      _f2 = 0;
   else if (is_multi(f))
      _adj->rem(f);
   else {
      err_msg("Bedge::operator-=(Bface*): error: unknown face");
      return 0;
   }

   faces_changed();
   _v1->star_changed();
   _v2->star_changed();

   return 1;
}

bool
Bedge::is_patch_boundary() const
{
   // todo: should be able to rely correctly
   //       on PATCH_BOUNDARY_BIT

   // XXX - retessellation for skin is broken when patch
   // boundaries take into acount uv discontinuities.
   //
   // until it gets fixed, allow skin-users to suppress 
   // use of uv-coords to retessellation works.
   static bool USE_UV = !Config::get_var_bool("SKIN_INHIBIT_UV",false);

   return (
      (_f1 && _f2 && _f1->patch() != _f2->patch()) ||
      is_set(PATCH_BOUNDARY_BIT) ||
      (USE_UV && !UVdata::is_continuous(this))
      );
}

Wpt&
Bedge::mid_pt(Wpt& v) const
{
   v = (_v1->loc() + _v2->loc())/2;
 
   return v; 
}

double
Bedge::avg_area() const
{
   RunningAvg<double> avg(0);
   if (_f1) avg.add(_f1->area());
   if (_f2) avg.add(_f2->area());
   if (_adj) {
      for (int i=0; i<_adj->num(); i++)
         avg.add((*_adj)[i]->area());
   }
   return (avg.num() > 0) ? avg.val() : 0.0;
}

Patch*
Bedge::patch() const 
{
   Bface* f = frontfacing_face();
   f = f ? f : _f1 ? _f1 : _f2;
   return f ? f->patch() : 0;
}

Wvec 
Bedge::norm() const 
{
   // XXX - ignoring multi edges

   return ((_f1&&_f2) ? (_f1->qnorm()+_f2->qnorm()).normalized() :
           _f1 ? _f1->qnorm() :
           _f2 ? _f2->qnorm() :
           vec().perpend()
           );
}

// we return a 1 because that indicates that our faces are smooth.
// since we don't have faces, we might as well think of them as smooth.
double
Bedge::dot() const
{
   return (_f1 && _f2) ? (_f1->norm() * _f2->norm()) : 1.0;
}

void 
Bedge::allocate_adj()
{
   if (!_adj) _adj = new Bface_list(1);  assert(_adj);
}

bool 
Bedge::is_multi() const 
{
   // Is the edge a non-manifold ("multi") edge?

   return (_adj && !_adj->empty()); 
}

bool 
Bedge::is_multi(CBface* f) const
{
   // Is the face a secondary face of this edge?

   return (f && _adj && f->contains(this) && _adj->contains((Bface*)f));
}

bool
Bedge::demote(Bface* f)
{
   // Move f from _f1 or _f2 to the _adj list:

   static bool debug = Config::get_var_bool("DEBUG_NON_MANIFOLD",false);
   if (!(f && f->contains(this))) {
      if (debug)
         MeshGlobal::select(this);
      err_msg("Bedge::demote: error: %s face", f ? "unknown" : "null");
      return false;
   }

//   UVdata::split(this);

   if (_f1 == f) {
      _f1 = 0;
   } else if (_f2 == f) {
      _f2 = 0;
   } else {
      assert(_adj && _adj->contains(f));

      // It's already demoted. Return true indicating "success".
      return true;
   }
   bool ret = add_multi(f);
   assert(ret);
   return true;
}

bool
Bedge::add_multi(Bface* f)
{
   // Protected virtual method used in Bedge::demote() and
   // Bedge::operator+=(Bface*).  Put the face into the _adj
   // list of non-primary faces.  In Ledge, do the same for
   // the appropriate child faces.

   if (!(f && f->contains(this)))
      return false;
   if (f == _f1 || f == _f2) {
      // (Should this be an error?)
      // It's currently a primary face. Demote it.
      return demote(f);
   } else {
      allocate_adj();
      _adj->add_uniquely(f);
      faces_changed();
      return true;
   }
}

bool
Bedge::can_promote() const
{
   // Returns true if there is at least one available primary slot (_f1 or _f2).
   // In Ledge, all child edges must also have available slots.

   return (!_f1 || _f1->is_secondary()) || (!_f2 || _f2->is_secondary());
}

bool
Bedge::promote(Bface* f)
{
   // Move f from the _adj list to _f1 or _f2:
   // It's up to the caller to check Bedge::can_promote().

   if (!(f && f->contains(this))) {
      if (Config::get_var_bool("DEBUG_NON_MANIFOLD",false))
         MeshGlobal::select(this);
      err_msg("Bedge::promote: error: %s face", f ? "unknown" : "null");
      return false;
   }

   if (!(_adj && _adj->contains(f))) {
      // Allow this case only if f is already _f1 or _f2.
      // Then return true, indicating "success".
      assert(f == _f1 || f == _f2);
      return true;
   }
   _adj->rem(f);
   bool ret = add_primary(f);
   assert(ret);
   return ret;
}

bool
Bedge::add_primary(Bface* f)
{
   // Add the face as a primary face (in slot _f1 or _f2).
   // In Ledge, do the same for the appropriate child faces.

   if (!(f && f->contains(this)))
      return false;

   // Could relax this...
   assert(!this->contains(f));

   // Find an available slot for f (or make one if possible):
   if (!_f1) {
      _f1 = f;
   } else if (!_f2) {
      _f2 = f;
   } else if (_f1->is_secondary()) {
      demote(_f1);
      _f1 = f;
   } else if (_f2->is_secondary()) {
      demote(_f2);
      _f2 = f;
   } else {
      return false;     // no available slot
   }
   return true;
}

void 
Bedge::fix_multi()
{
   // If any faces in the _adj listed are labelled "primary",
   // then move them to a primary slot (_f1 or _f2). Used when
   // reading a mesh from file, when primary/secondary face
   // labels are specified only after all faces are created.

   if (!_adj)   // no _adj, so no fixing required
      return;

   // It's not possible if the total number of "primary" faces
   // exceeds 2.
   if (nfaces_satisfy(PrimaryFaceFilter()) > 2) {
      cerr << "Bedge::fix_multi: error: more than 2 primary faces" << endl;
      return;
   }

   // Work backwards to remove items from the list:
   for (int i=_adj->num()-1; i>=0; i--) {
      Bface *face = (*_adj)[i];
      if (face->is_primary()) {
         promote(face);
      }
   }
}

Bface*
Bedge::ccw_face(CBvert* v) const
{
   // Given vertex v belonging to this edge, return the adjacent
   // face (_f1 or _f2) for which this edge follows v in CCW
   // order within the face.

   return (!contains(v) ? 0 :
           (_f1 && _f1->edge_from_vert(v) == this) ? _f1 :
           (_f2 && _f2->edge_from_vert(v) == this) ? _f2 : 0
      );
}

bool
Bedge::consistent_orientation() const
{
   return ((nfaces()<2) ? 1 :
           ((_f1->orientation(this) * _f2->orientation(this)) == -1) ? 1 : 0
           );
}

bool 
Bedge::oriented_ccw(Bface* f) const
{
   // if _v1 and _v2 in this edge are ordered CCW in the given face,
   // return true.

   if (!f && !(f = get_face()))
      return 0;

   return f->orientation(this) == 1;
}

void
Bedge::bc2pos(CWvec& bc, Wpt& pos) const
{
   pos = (bc[0]*_v1->loc()) + (bc[1]*_v2->loc());
}

void 
Bedge::project_barycentric(CWpt &p, Wvec &bc) const 
{
   double t = ((p - _v1->loc()) * vec()) / sqr(length());
   bc.set(1.0 - t, t, 0);
}

int
Bedge::redefine(Bvert *v, Bvert *u)
{
   // redefine this edge, replacing v with u

   // precondition:
   //   edge does not already contain u.
   //   v is a vertex of this edge.
   //   faces have already been detached.
   //   can't duplicate an existing edge.

   assert(contains(v) && nfaces() == 0);

   if (contains(u))
      return 0;

   Bedge* dup = 0;
   if (v == _v1) {
      if ((dup = u->lookup_edge(_v2))) {
         // can't redefine, but if this is a crease edge
         // should ensure the duplicated edge is also
         if (is_crease())
            dup->set_crease(crease_val());
         return 0;
      }
      // can redefine:
      *_v1 -= this;     // say bye to old _v1
      _v1 = u;          // record new _v1
      *_v1 += this;     // say hi to new _v1
   } else if (v == _v2) {
      // see comments above
      if ((dup = u->lookup_edge(_v1))) {
         if (is_crease())
            dup->set_crease(crease_val());
          return 0;
      }
      *_v2 -= this;
      _v2 = u;
      *_v2 += this;
   } else assert(0);

   geometry_changed();

   return 1;
}

bool
Bedge::redef2(Bvert *v, Bvert *u)
{
   static bool debug = Config::get_var_bool("DEBUG_MESH_OPS",false);

   // redefine this edge, replacing v with u

   // preconditions:
   //   v is a vertex of this edge.
   //   edge does not already contain u.
   //   can't duplicate an existing edge.

   if (!contains(v)) {
      err_adv(debug, "Bedge::redef2: error: edge doesn't contain v");
      return false;
   }
   if (contains(u)) {
      err_adv(debug, "Bedge::redef2: error: edge already contains u");
      return false;
   }
   Bvert* keeper = other_vertex(v);
   if (keeper->lookup_edge(u)) {
      err_adv(debug, "Bedge::redef2: error: would duplicate existing edge");
      return false;
   }

   if (v == _v1) {
      *_v1 -= this;     // say bye to old _v1
      _v1 = u;          // record new _v1
      *_v1 += this;     // say hi to new _v1
   } else if (v == _v2) {
      *_v2 -= this;
      _v2 = u;
      *_v2 += this;
   } else assert(0);

   geometry_changed();

   return true;
}

void
Bedge::set_new_vertices(
   Bvert *v1, 
   Bvert *v2)
{
   // used in edge swap

   assert(nfaces() == 0 && !v1->lookup_edge(v2));

   *_v1 -= this;
   *_v2 -= this;
   _v1 = v1;
   _v2 = v2;

   *_v1 += this;
   *_v2 += this;
   
   geometry_changed();
}

void 
Bedge::notify_split(
   Bsimplex *new_simp)
{
   Bsimplex::notify_split(new_simp);
   if (is_crease() && new_simp->dim() == 1)
      ((Bedge*)new_simp)->set_crease(crease_val());
}

bool
Bedge::is_polyline_end() const
{
   return (is_polyline() && (_v1->is_polyline_end() || _v2->is_polyline_end()));
}

bool
Bedge::is_crease_end() const
{
   return (is_crease() && (_v1->is_crease_end() || _v2->is_crease_end()));
}

bool
Bedge::is_chain_tip(CSimplexFilter& filter) const
{
   return (filter.accept((Bedge*)this) &&
           (_v1->degree(filter) != 2 || _v2->degree(filter) != 2));
}

// -----------

bool 
Bedge::is_sil() const
{
   // Border edge: yes
   if (is_border())
      return true;

   // Polyline edge: no
   if (!(_f1 || _f2))
      return false;

   // 2 faces, both valid normals: regular case
   if (!(_f1->norm().is_null() || _f2->norm().is_null()))
      return (_f1->front_facing() != _f2->front_facing());

   // Fuked-up case
   if (_f1->norm().is_null() && _f2->norm().is_null())
      return false;

   // There's exactly one adjacent face with zero area.  This
   // happens sometimes with quads that are trying to look like
   // triangles, so we're gonna check for that common case. If
   // it's not the common case we just give up below, at this
   // time.

   // Quad diagonal? Then say no:
   if (is_weak())
      return false;

   // Identify the zero-area triangle
   Bface* good_face = _f1;
   Bface* null_face = _f2;
   if (good_face->norm().is_null())
      swap(good_face, null_face);
   assert(null_face->norm().is_null());

   if (null_face->is_quad() && !null_face->quad_partner()->norm().is_null())
      return (good_face->front_facing() !=
              null_face->quad_partner()->front_facing());

   // We could keep trying but it's easier to give up:
   return false;
}

Bsimplex_list
Bedge::neighbors() const
{
   Bsimplex_list ret(4);
   ret += _v1;
   ret += _v2;
   if (_f1) ret += _f1;
   if (_f2) ret += _f2;
   if (_adj) {
      for (int j=0; j<_adj->num(); j++)
         ret += (*_adj)[j];
   }
   return ret;
}

bool 
Bedge :: is_texture_seam()const
{
   //this function is under construction
   //do not use yet
   
   //fix for skin users
   static bool USE_UV = !Config::get_var_bool("SKIN_INHIBIT_UV",false);
   if(  USE_UV && !UVdata::is_continuous(this)) return true;
   
   
   if (_f1 && _f2 && !is_patch_boundary())//patches on both sides are the same
   {
    
      TexCoordGen* tg = patch()->tex_coord_gen();
      if (tg)
      {
         UVpt tx1A = tg->uv_from_vert(_v1,_f1);
         UVpt tx1B = tg->uv_from_vert(_v1,_f2);
         UVpt tx2A = tg->uv_from_vert(_v2,_f1);
         UVpt tx2B = tg->uv_from_vert(_v2,_f2);

         if ((tx1A!=tx1B) || (tx2A!=tx2B)) return true;

      }
   }

   return false;
}

bool
Bedge::is_crossable() const
{

   return (consistent_orientation() &&
           !(is_crease() || is_patch_boundary() || is_texture_seam()) );
}

bool
Bedge::is_stressed() const
{
   return !is_crease() && _f1 && _f2 && (_f1->norm() * _f2->norm() < -.5);
}

void  
Bedge::set_crease(ushort c) 
{
   set_bit(CREASE_BIT,c);
   crease_changed();
}

void 
Bedge::inc_crease(ushort max_val)
{
//    cerr << "inc_crease: from " << crease_val() << endl;

   ushort c = crease_val();
   if (c == USHRT_MAX)
      return;
   else if (c >= max_val)
      c = USHRT_MAX;
   else
      c++;
   set_crease(c);
}

void 
Bedge::dec_crease(ushort max_val)
{
//    cerr << "dec_crease: from " << crease_val() << endl;

   ushort c = crease_val();
   if (c == 0)
      return;
   else if (c <= max_val)
      c--;
   else
      c = max_val;
   set_crease(c);
}

void
Bedge::compute_crease(double d)
{
   // in case this is an Ledge, specify "infinitely sharp" crease.

   set_crease((dot() < d) ? USHRT_MAX : 0);
}

void
Bedge::set_convex()
{
   set_bit(CONVEX_VALID_BIT);
   int b = (!_f1 || !_f2 ||
            ((_f1->norm() + _f2->norm()) *
             (_f1->other_vertex(_v1,_v2)->loc()-_v1->loc()) < 0));
   set_bit(CONVEX_BIT,b);
}

Bface*
Bedge::frontfacing_face() const
{
   // Only check primary edges

   return ((_f1 && _f1->front_facing()) ? _f1 :
           (_f2 && _f2->front_facing()) ? _f2 :
           0);
}

ostream& operator <<(ostream &os, CBedge &e) 
{
   return os << e.v(1)->index() << " " << e.v(2)->index() << " " ;
}

bool
Bedge::swapable(Bface*& face1,
                Bface*& face2,
                Bvert*& verta,
                Bvert*& vertb,
                Bvert*& vertc,
                Bvert*& vertd,
                bool    favor_degree_six)
{
   /*
   //                  c
   //                / | \
   //              /   |   \
   //            /     |     \
   //          /       |       \
   //        d    f1  /|\   f2   b
   //          \       |       /
   //            \     |     /
   //              \   |   /
   //                \ | /
   //                  a
   //
   //      this edge goes from a to c.
   //  "swapping" the edge means deleting it
   //   and putting in an edge from d to b.
   //  an edge is "swapable" if the operation
   //  is both legal (no change to topology)
   //   and preferable (e.g. it leads to a 
   //           bigger minimum angle).
   */

   static bool debug = Config::get_var_bool("DEBUG_EDGE_SWAP",false);
   if (is_patch_boundary() || is_crease() || is_multi()) {
      err_adv(debug, "Bedge::swapable: bad edge type");
      return 0;
   }

   // get faces as above
   face1 = ccw_face();
   face2 = other_face(face1);

   // gotta have two faces
   if (!(face1 && face2)) {
      err_adv(debug, "Bedge::swapable: need 2 faces");
      return 0;
   }

   // if dihedral angle is sharp, refuse
   static const double DOT_THRESH = cos(60.0 * M_PI / 180.0);
   if ((face1->norm() * face2->norm()) < DOT_THRESH) {
      err_adv(debug, "Bedge::swapable: too sharp");
      return 0;
   }

   // get vertices as above
   verta = _v1;
   vertc = _v2;
   vertb = face2->other_vertex(verta, vertc);
   vertd = face1->other_vertex(verta, vertc);

   // hoppe et. al. (siggraph 93) say:
   // edge swap is valid iff edge db is not already defined.
   if (!vertb || !vertd || vertb->lookup_edge(vertd)) {
      err_adv(debug, "Bedge::swapable: topology violation");
      return 0;
   }

   CWpt& a=verta->loc();
   CWpt& b=vertb->loc();
   CWpt& c=vertc->loc();
   CWpt& d=vertd->loc();

   // don't proceed if new edge is sharp
   if ((cross(a-d,b-d).normalized() * cross(b-d,c-d).normalized()) <= DOT_THRESH) {
      err_adv(debug, "Bedge::swapable: new edge too sharp");
      return 0;
   }

   Wvec  ca = vec().normalized();
   Wvec  ba = (b-a).normalized();
   Wvec  da = (d-a).normalized();
   Wvec  bc = (b-c).normalized();
   Wvec  dc = (d-c).normalized();

   // find smallest angle in current arrangement
   // (bigger dot means teenier angle):
   double cur_max_dot = ca * ba;
   cur_max_dot = max(cur_max_dot, (ca * da));
   cur_max_dot = max(cur_max_dot,-(ca * bc));
   cur_max_dot = max(cur_max_dot,-(ca * dc));

   Wvec  bd = (b-d).normalized();
   double swap_max_dot = -(bd * da);
   swap_max_dot = max(swap_max_dot, -(bd * dc));
   swap_max_dot = max(swap_max_dot,  (bd * ba));
   swap_max_dot = max(swap_max_dot,  (bd * bc));

   // if we keep these lines, probably want to
   // use a tabulated function for acos:
   double cur_min_angle  = Acos(cur_max_dot);
   double swap_min_angle = Acos(swap_max_dot);

   // do handicapping to promote degree-6 verts:
   int k = 0;
   if (favor_degree_six) {
      k += ((verta->degree() <= 6) ? 1 : -1);      // k measures how bad
      k += ((vertc->degree() <= 6) ? 1 : -1);      //   is the swap.
      k += ((vertd->degree() >= 6) ? 1 : -1);      // higher (positive) numbers
      k += ((vertb->degree() >= 6) ? 1 : -1);      //   are worse.
   }

   // for each unit of k, penalize the
   // swapped min angle by 8 degrees:
   static const double handicap_unit = 8.0 / 180.0 * M_PI;
   return (swap_min_angle > (cur_min_angle + k*(handicap_unit)));
}

bool
Bedge::swap_is_legal() const
{
   static bool debug = Config::get_var_bool("DEBUG_EDGE_SWAP",false);

   if (!is_interior() || is_patch_boundary() || is_crease() || is_multi()) {
      err_adv(debug, "Bedge::swap_is_legal: bad edge type");
      return false;
   }
   if (!is_weak() && (_f1->is_quad() || _f2->is_quad())) {
      err_adv(debug, "Bedge::swap_is_legal: swap would wreck quad-ness");
      return false;
   }
   if ((UVdata::has_uv(_f1) || UVdata::has_uv(_f2)) &&
       !UVdata::is_continuous(this)) {
      err_adv(debug, "Bedge::swap_is_legal: can't swap uv-discontinuous edge");
      return false;
   }
   Bvert* o1 = opposite_vert1();
   Bvert* o2 = opposite_vert2();
   assert(o1 && o2); // must be: we checked for 2 faces

   return !lookup_edge(o1,o2);
}

bool
Bedge::do_swap()
{
   /*
   //                  v2                           v2                          
   //                / | \                        /   \                        
   //              /   |   \                    /       \                       
   //            /     |     \                /     g2    \                      
   //          /       |       \            /               \                    
   //       o1    g1  /|\  g2    o2      o1 - - - - - - - - - o2                 
   //          \       |       /            \               /                    
   //            \     |     /                \     g1    /                    
   //              \   |   /                    \       /                      
   //                \ | /                        \   /                       
   //                  v1                           v1                          
   //
   //                 old                          new
   //
   //  This edge goes from v1 to v2, and we will "swap" it to go
   //  from o2 to o1. We also redefine adjacent faces g1 and g2.
   //  Requires that the operation is topologically legal.
   //
   */

   static bool debug = Config::get_var_bool("DEBUG_MESH_OPS",false);

   if (!swap_is_legal()) {
      err_adv(debug, "Bedge::do_swap: illegal");
      return false;
   }

   // get faces as above
   Bface* g1 = ccw_face();
   Bface* g2 =  cw_face();
   assert(g1 && g2); // check for 2 faces is in swap_is_legal()

   // get vertices as above
   Bvert* o1 = g1->other_vertex(this);
   Bvert* o2 = g2->other_vertex(this);
   Bvert* v1 = _v1;     // save these now
   Bvert* v2 = _v2;     // before we forget

   // deal w/ uv if needed
   UVpt uvo1, uvo2, uvv1, uvv2;
   bool do_uv = false;
   if ((UVdata::has_uv(_f1) || UVdata::has_uv(_f2))) {
      assert(UVdata::has_uv(_f1) && UVdata::has_uv(_f2));
      assert(UVdata::is_continuous(this));
      do_uv = true;
      uvo1 = UVdata::get_uv(o1,g1);
      uvo2 = UVdata::get_uv(o2,g2);
      uvv1 = UVdata::get_uv(v1,g1);
      uvv2 = UVdata::get_uv(v2,g1);
   }
   
   // detach faces
   g1->Bface::detach(); // XXX - hack to prevent subdiv elements from
   g2->Bface::detach(); //       being deleted; swap will be propagated down

   // redefine edge
   Bedge::set_new_vertices(o2,o1);

   // redefine faces
   g1->Bface::redefine(v2,o2);
   g2->Bface::redefine(v1,o1);

   if (do_uv) {
      UVdata::set(o1,g1,uvo1);
      UVdata::set(v1,g1,uvv1);
      UVdata::set(o2,g1,uvo2);
      UVdata::set(o2,g2,uvo2);
      UVdata::set(v2,g2,uvv2);
      UVdata::set(o1,g2,uvo1);
   }

   if (!(g1->check() && g2->check())) {
      err_adv(debug, "Bedge::do_swap: check failed");
   }

   return true;
}

void
Bedge::geometry_changed()
{
   // an adjacent vertex changed location

   Bsimplex::geometry_changed();
}

void 
Bedge::crease_changed()
{
   _v1->crease_changed();
   _v2->crease_changed();
}

void 
Bedge::normal_changed()
{
   clear_bit(CONVEX_VALID_BIT);
   _sil_stamp = 0;

   Bsimplex::normal_changed();
}

void 
Bedge::faces_changed()
{
   // called when faces are added or deleted
   normal_changed();

   // This could be finer grained, and just call
   // the following when the non-manifold status
   // of the edge changes:
   _v1->clear_bit(Bvert::VALID_NON_MANIFOLD_BIT);
   _v2->clear_bit(Bvert::VALID_NON_MANIFOLD_BIT);
}

// returns 0 if plane intersects edge, 
//        -1 if edge is on negative side of plane normal,
//         1 if edge is on positive side of plane normal
int
Bedge::which_side(CWplane& plane) const
{
   Wpt  p = plane.origin();
   Wvec n = plane.normal();
   double dot1 = (p - _v1->loc()) * n;
   double dot2 = (p - _v2->loc()) * n;
   return dot1*dot2 < 1e-20 ? 0 : Sign(dot1);
}


// TODO: unreadable arguments!
// what's input? what's output?
//              -tc
bool 
Bedge::local_search(Bsimplex *&end, Wvec &final_bc,
                    CWpt &target, Wpt &reached, 
                    Bsimplex *repeater, int iters) 
{ 
   if (iters <= 0)
      return 0;

   Wvec      bc;
   bool      is_on_sim = 0;
   Wpt       nearpt    = nearest_pt(target, bc, is_on_sim);
   Bsimplex* sim       = bc2sim(bc);

   if (!is_on_sim) {
      
      if (sim == repeater) // We hit a loop in the recursion, so stop
         return 0;

      if (sim != this) { // We're on the boundary of the edge
         assert(is_vert(sim));
         Bvert* v = (Bvert*)sim;

         for (int i=0; i<v->degree(); i++) {
            int good_path = v->e(i)->local_search(end, final_bc, target,
                                                  reached, sim, iters-1);
            if (good_path == 1)
               return 1;
            else if (good_path==-1)
               return repeater ? true : false; // XXX - changed from -1 : 0 -- should check

         }
      }
   }

   reached = nearpt;
   end = this;
   final_bc = bc;

   return 1;
}

NDCpt 
Bedge::nearest_pt_ndc(CNDCpt& p, Wvec &bc, int &is_on_simplex) const
{ 
   NDCpt a = _v1->ndc();
   NDCpt b = _v2->ndc();

   NDCvec ab = b - a;
   NDCvec ac = p - a;

   double dot = (ab * ac) / ab.length_sqrd();
   bc.set(1-dot, dot, 0);

   if (dot < gEpsZeroMath) {
      bc.set(1, 0, 0);
      is_on_simplex = 0;
   } else if (1-dot < gEpsZeroMath) {
      bc.set(0, 1, 0);
      is_on_simplex = 0;
   }

   return (bc[0] * a) + (bc[1] * b); 
}

Wpt   
Bedge::nearest_pt(CWpt& p, Wvec &bc, bool &is_on_simplex) const
{ 
   Wvec ab = _v2->loc() - _v1->loc();
   Wvec ac = p - _v1->loc();

   double dot = (ab * ac) / ab.length_sqrd();
   bc.set(1-dot, dot, 0);

   if (dot < gEpsZeroMath) {
      bc.set(1, 0, 0);
      is_on_simplex = (dot >= 0);
   } else if (1-dot < gEpsZeroMath) {
      bc.set(0, 1, 0);
      is_on_simplex = (dot <= 1);
   }

   return (bc[0] * _v1->loc()) + (bc[1] * _v2->loc());
}

Wpt  
Bedge::mid_pt() const 
{ 
   return (_v1->loc() + _v2->loc())/2; 
}

int 
Bedge::nfaces_satisfy(CSimplexFilter& f) const
{
   return ((f.accept(_f1) ? 1 : 0) +
           (f.accept(_f2) ? 1 : 0) +
           (_adj ? _adj->num_satisfy(f) : 0));
}

/*****************************************************************
 * BoundaryEdgeFilter:
 *
 *   Accepts edges along the boundary between faces of a
 *   given type and faces not of that type.
 *****************************************************************/
bool 
BoundaryEdgeFilter::accept(CBsimplex* s) const 
{
   // What makes an edge a "boundary" WRT faces of a given type?
   // We'll say the number of adjacent faces of the given type
   // is > 0 but not == 2. I.e., faces of the given type do not
   // locally form a 2-manifold along the edge.

   if (!is_edge(s))
      return false;
   int n = ((Bedge*)s)->nfaces_satisfy(_filter);
   return (n > 0 && n != 2);
}

/************************************************************
 * Bedge_list
 ************************************************************/

void
Bedge_list::clear_vert_flags() const
{   
   // Clear the flag of each vertex of each edge

   for (int i=0; i<_num; i++) {
      _array[i]->v1()->clear_flag();
      _array[i]->v2()->clear_flag();
   }
}

// l'il helper used in Bedge_list::get_verts():
inline void
screen(Bvert_list& list, Bvert* v)
{
   if (v && !v->flag()) {
      v->set_flag();
      list += v;
   }
}

Bvert_list 
Bedge_list::get_verts() const
{
   // Extract a list of the verts found in the given edges.

   // Get clean slate
   clear_vert_flags();

   // Put verts into output array uniquely:
   Bvert_list ret;
   for (int i=0; i<_num; i++) {
      screen(ret, _array[i]->v1());
      screen(ret, _array[i]->v2());
   }
   return ret;
}

inline void
add_face(Bface* f, Bface_list& ret)
{
   // Helper used in both add_faces(), below.
   //
   // Add the face to the list if it is non-null and
   // its flag is not set. Set the flag to prevent it
   // from being added again subsequently.

   if (f && !f->flag()) {
      f->set_flag();
      ret += f;
   }
}

inline void
add_faces(Bface_list* faces, Bface_list& ret)
{
   // Helper used in add_faces(), below.

   if (!faces)
      return;
   for (int i=0; i<faces->num(); i++)
      add_face((*faces)[i], ret);
}

inline void
add_faces(Bedge* e, Bface_list& ret)
{
   // Helper used in Bedge_list::get_faces(), below.
   // Get faces adjacent to e and add them to the return list
   // avoiding duplicates by testing and setting flags.

   if (e) {
      add_face(e->f1(), ret);
      add_face(e->f2(), ret);
      add_faces(e->adj(), ret);
   }
}

Bface_list 
Bedge_list::get_faces() const
{
   // Returns list of faces adjacent to these edges:

   // Clear flags of adjacent faces
   clear_flag02();

   // Put faces into output array uniquely:
   Bface_list ret;
   for (int i=0; i<_num; i++)
      add_faces(_array[i], ret);
   return ret;
}

void
Bedge_list::mark_edges() const
{
   // Set the flag of each edge to 1, and clear the flag of
   // any other edge connected to one of these.

   // Clear vertex, edge, and face flags within a 1-neighborhood
   // of any vertex of this set of edges:
   get_verts().clear_flag02();

   // Set edge flags to 1:
   set_flags(1);
}

Bvert_list
Bedge_list::fold_verts() const
{
   // set internal edge flags to 1, neighboring edge flags to 0:
   mark_edges();

   // Using an edge filter that screens for edges with flag = 1,
   // construct a "fold vertex" filter that checks for vertics
   // with 2 of the acceptable edges arranged view-dependently
   // in a "fold" configuration:

   return get_verts().filter(FoldVertFilter(SimplexFlagFilter(1)));
}

bool
Bedge_list::is_simple() const
{
   mark_edges();
   return
      get_verts().filter(
         !(VertDegreeFilter(2, SimplexFlagFilter(1)) ||
           VertDegreeFilter(1, SimplexFlagFilter(1)))).empty();
}

bool 
PatchBlendBoundaryFilter::accept(CBsimplex* s) const 
{
   // used in defining blend weights between patches:
   CBedge* e = dynamic_cast<CBedge*>(s);
   return (
      e                 &&                 // is an edge
      e->is_interior()  &&                 // has two faces
      !e->is_crease()   &&                 // not a crease
      e->f1()->patch() != e->f2()->patch() // between different patches
      );
}

// end of file bedge.C
