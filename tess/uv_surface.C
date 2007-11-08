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
 * uv_surface.C:
 **********************************************************************/
#include "geom/world.H"
#include "mesh/mi.H"
#include "std/config.H"
#include "panel.H"
#include "tess_cmd.H"
#include "uv_surface.H"

using namespace mlib;

/*****************************************************************
 * UVmeme:
 *****************************************************************/
UVmeme::UVmeme(UVsurface* b, Lvert* v, CUVpt& uv, char pole) :
   VertMeme(b, v),
   _uv_valid(true),
   _uv(uv),
   _disp(0),
   _pole(pole)
{
   if (is_boss())
      do_update();              // yank the vert into its place
   else
      set_uv_from_loc();        // correct the uv coord to match vert loc
}

void 
UVmeme::set_uv(CUVpt& uv)
{
   _uv = uv;
   _uv_valid = 1;
   do_update();
}

void 
UVmeme::copy_attribs_v(VertMeme* v)
{
   // this meme comes from a vertex in the parent mesh.
   // we now update our uv coords from our parent meme's
   // uv coords.

   UVmeme* p = upcast(v); // parent

   if (!p) {
      return;
   }

   if (p->_uv_valid)
      set_uv(p->_uv);

   _pole = p->_pole;

   if (p->is_pinned())
      pin();
}

void 
UVmeme::copy_attribs_e(VertMeme* v1, VertMeme* v2)
{
   // this meme comes from a strong edge in the parent mesh.
   // we now update our uv coords, taking an average of the uv
   // coords from the two memes at the parent edge endpoints.

   if (!map())
      return;

   UVmeme* p1 = upcast(v1);
   UVmeme* p2 = upcast(v2);

   if (!(p1 && p2 && p1->_uv_valid && p2->_uv_valid))
      return;

   // if there is a pole, make it be at p2
   if (p1->_pole)
      swap(p1,p2);
   assert(p1->_pole == REGULAR);
   UVpt uv1 = p1->_uv;
   UVpt uv2 = p2->_uv;
   if (p2->_pole == POLE_V)     // if it is a v-pole,
      uv2[0] = uv1[0];          // ignore its u -- use u from p1
   if (p2->_pole == POLE_U)     // if it is a u-pole,
      uv2[1] = uv1[1];          // ignore its v -- use v from p1
   
   set_uv(map()->avg(uv1, uv2));
}

void 
UVmeme::copy_attribs_q(VertMeme* v1, VertMeme* v2, VertMeme* v3, VertMeme* v4)
{
   // this meme comes from a weak edge (quad diagonal) in the
   // parent mesh.  we now update its uv coords, taking an
   // average of the uv coords from the 4 memes at the parent
   // quad corners.

   Map2D3D* m = map();
   if (!m)
      return;

   UVmeme* p1 = upcast(v1);
   UVmeme* p2 = upcast(v2);
   UVmeme* p3 = upcast(v3);
   UVmeme* p4 = upcast(v4);

   if (p1 && p2 && p3 && p4 &&
       p1->_uv_valid && p2->_uv_valid &&
       p3->_uv_valid && p4->_uv_valid)
      set_uv(m->avg(m->avg(p1->_uv, p2->_uv), m->avg(p3->_uv, p4->_uv)));
}

bool
UVmeme::lookup_uv()
{
   // Pull the uv-coord out of the mesh.
   //
   // In case some adjacent faces are owned by another Bbase,
   // we have to be careful to use the uv-coord from a face of
   // our Bbase.

   // XXX -
   //   When all adjacent faces get ripped out, the meme is
   //   helpless, unless it received a valid uv-coord previously.
   //   (which now it usually will have). - 12/2002

   if (!vert())
      return 0;

   // Try the lightweight way first:
   if (UVdata::get_uv(vert(), _uv))
      return _uv_valid = true;

   // Try it heavyweight:
   static ARRAY<FaceMeme*> nbrs;
   get_nbrs(nbrs);
   if (nbrs.empty())
      return 0;

   // The assumption is that either the uv-coords are continuous
   // over the triangles owned by our UVsurface, OR, if we happen
   // to be on a seam (e.g. along the side of a cylinder), then
   // at least our map2D3D maps the distinct uv-coords on either
   // side of the seam to the same Wpt.

   if (UVdata::get_uv(vert(), nbrs[0]->face(), _uv))
      return _uv_valid = true;
      
   return 0;
}

CWpt& 
UVmeme::compute_update()
{
   // Compute 3D vertex location using the surface map.

   // We assume that either there is no uv-discontinuity at
   // this vertex, or if there is, the multiple uv-coords
   // for the vertex all map to the same Wpt (like at the
   // seam of a cylinder).

   Map2D3D* m = get_set();
   return m ? (_update = map()->map(_uv)) : VertMeme::compute_update();
}

bool
UVmeme::solve_delt(CWvec& w_delt, UVvec& uv_delt)
{
   // Compute a delta in uv-space that best matches the
   // given world-space displacement.

   static bool debug = Config::get_var_bool("DEBUG_UV_SURF_SMOOTH_UVS",false);

   // start blank
   uv_delt = UVvec::null();

   // get the map and uv coord
   Map2D3D* m = get_set();
   if (!m) {
      err_adv(debug, "UVmeme::solve_delt: no map or uv coord");
      return false;
   }

   bool ret = m->solve(_uv, w_delt, uv_delt);
   if (debug && !ret)
      err_msg("UVmeme::solve_delt: solve failed (uv: %f,%f", _uv[0], _uv[1]);
   return ret;
}

bool
UVmeme::compute_delt()
{
   // Compute a delta in uv-space that moves us toward a
   // more "relaxed" position

   if (is_pinned())
      return false;

   return solve_delt(target_delt(), _delt);
}

bool
UVmeme::apply_delt()
{
   // update _uv from _delt

   bool debug = Config::get_var_bool("DEBUG_UV_SURF_SMOOTH_UVS",false);

   // nothing to do:
   if (_delt.is_null())
      return false;

   // Get the map and ensure we have a uv coord:
   Map2D3D* m = get_set();
   if (!m) {
      // Should never happen if _delt is not null
      err_adv(debug, "UVmeme::apply_delt: no map or uv coord");
      return false;
   }

   // Update uv-coord:
   UVpt old_uv = _uv;
   _uv   = m->add(_uv, _delt); // handle wrapping around the seam
   _delt = UVvec::null();

   bool ret = do_update();
   if (ret) {
      set_hot();
      return true;
   }
   _uv = old_uv;
   return false;
}

bool
UVmeme::set_uv_from_loc()
{
   // Goal: update our uv coord to match the vert location.

   static bool debug = Config::get_var_bool("DEBUG_UV_SURF_SMOOTH_UVS");

   // Not expected to happen:
   err_adv(debug && is_boss(),
           "UVmeme::set_uv_from_loc: called on boss meme");

   // Get the map and ensure we have a uv coord:
   Map2D3D* m = get_set();
   if (!m) {
      err_adv(debug, "UVmeme::set_uv_from_loc: no map or uv coord");
      return false;
   }

   // Get a scale for what is "close".
   // Same variable/value found in VertMeme::tracks_boss(),
   // but here we take 1/10th of that value.
   static const double THRESH_SCALE =
      Config::get_var_dbl("VERT_MEME_THRESH_SCALE", 1e-1)/10.0; 
   assert(vert() != 0);
   double t = vert()->avg_edge_len()*THRESH_SCALE;

   // Find where the vert moved to.
   // (Our uv coord has to be updated to match):
   CWpt& p = loc();       // goal position in world space
   Wpt   c = m->map(_uv); // current position within the uv surface
   UVvec d;               // computed delta in uv space
   double old_dist = p.dist(c);
   while (solve_delt(p - c, d)) {
      _uv = m->add(_uv, d); // handle wrapping around the seam
      c   = m->map(_uv);
      double new_dist = p.dist(c);
      if (new_dist < t)
         return true;
      if (new_dist >= old_dist)
         break;
      old_dist = new_dist;
   }
   bool ret = (p.dist(c) <= t);
   err_adv(debug && !ret, "UVmeme::set_uv_from_loc: failed");
   return ret;
}

void 
UVmeme::vert_changed_externally()
{
   // Called when the vertex location changed but this VertMeme
   // didn't cause it.

   // may be useful sometimes, but not always.
   // needs work to decide how to turn it on at the
   // right time. for now disabling:
//   set_uv_from_loc();
}

VertMeme* 
UVmeme::_gen_child(Lvert* lv) const 
{
   UVsurface* c = uv_surf()->child_uv_surf();
   assert(c && !c->find_vert_meme(lv));

   // Produce a child just like this on the given vertex of
   // the subdiv mesh.
   return new UVmeme(c, lv, _uv);
}

VertMeme* 
UVmeme::_gen_child(Lvert* lv, VertMeme* p) const 
{
   // create a child meme with our "partner", giving the child
   // an average of its parents' uv coords

   UVmeme* partner = UVmeme::upcast(p);

   // XXX - may need to be more graceful here
   assert(map() && _uv_valid && partner && partner->_uv_valid);

   UVsurface* c = uv_surf()->child_uv_surf();
   assert(c && !c->find_vert_meme(lv));

   // Produce a child just like this on the given vertex of
   // the subdiv mesh.
   return new UVmeme(c, lv, map()->avg(_uv, partner->_uv));
}

VertMeme* 
UVmeme::_gen_child(Lvert* lv, VertMeme* v1, VertMeme* v2, VertMeme* v3) const 
{
   // create a child meme with our 3 "partners", giving the
   // child an average of its parents' uv coords

   UVmeme* p1 = UVmeme::upcast(v1);
   UVmeme* p2 = UVmeme::upcast(v2);
   UVmeme* p3 = UVmeme::upcast(v3);

   // XXX - may need to be more graceful here
   assert(map() && _uv_valid &&
          p1 && p1->_uv_valid &&
          p2 && p2->_uv_valid &&
          p3 && p3->_uv_valid);

   UVsurface* c = uv_surf()->child_uv_surf();
   assert(c && !c->find_vert_meme(lv));

   // Produce a child just like this on the given vertex of
   // the subdiv mesh.
   return new UVmeme(c, lv, map()->avg(map()->avg(_uv,     p1->_uv),
                                       map()->avg(p2->_uv, p3->_uv)));
}

/*****************************************************************
 * UVsurface:
 *****************************************************************/
UVsurface::UVsurface(Map2D3D* map, CLMESHptr& mesh) :
   Bsurface(mesh),
   _map(map)
{
   hookup(); // create dependencies
}

UVsurface::UVsurface(
   Map2D3D*     map,
   CLMESHptr&   mesh,
   int          num_rows,
   int          num_cols,
   bool         do_cap
   ) :
   Bsurface(mesh),
   _map(map)
{
   hookup(); // create dependencies

   tessellate_rect(num_rows, num_cols, do_cap);
}

UVsurface::UVsurface(UVsurface* parent) :
   _map(0)
{
   // Create a child UVsurface of the given parent:

   assert(parent);

   // Record the map before producing children
   _map = parent->_map;

   // Record the parent and produce children if needed:
   set_parent(parent);

   hookup(); // create dependencies

   // Make sure we'll be recomputed
   invalidate();
}

UVsurface::~UVsurface()
{
   destructor();
   _map = 0;
}

Bnode_list 
UVsurface::inputs() const 
{
   Bnode_list ret = Bsurface::inputs();

   if (_map)    ret += (Bnode*) _map;
   return ret;
}


void
UVsurface::produce_child()
{
   if (_child)
      return;
   if (!_mesh->subdiv_mesh()) {
      err_msg("UVsurface::produce_child: Error: no subdiv mesh");
      return;
   }

   // It hooks itself up and takes care of everything...
   // even setting the child pointer is not necessary here.
   _child = new UVsurface(this);
}

bool
UVsurface::apply_xf(CWtransf& xf, CMOD& mod)
{
   bool debug = Config::get_var_bool("DEBUG_BBASE_XFORM");
   if (!_map) {
      err_adv(debug, "UVsurface::apply_xf: error: null map");
      return false;
   }
   if (!is_control())
      return true;
   if (!_map->can_transform()) {
      if (debug) {
         cerr << "UVsurface::apply_xf: can't transform map: "
              << _map->class_name() << endl;
      }
      return false;
   }
   _map->transform(xf, mod);
   return true;
}

inline bool
contained_face(Bface* f, CBface_list& faces)
{
   return f && faces.contains(f);
}

inline Bface*
internal_face(Bvert* v1, Bvert* v2, CBface_list& enclosed_faces)
{
   // Helper function used below: given ordered vertices from a closed
   // curve, and the set of faces enclosed by the curve, return an
   // internal face (adjacent face in the set of enclosed faces.)

   assert(v1 && v2);
   Bedge* e = v1->lookup_edge(v2);
   assert(e);
   if (e->f1() && enclosed_faces.contains(e->f1()))
      return e->f1();
   if (e->f2() && enclosed_faces.contains(e->f2()))
      return e->f2();
   return 0;
}

inline bool
runs_ccw_around_enclosed_surface(
   Bvert* v1,
   Bvert* v2,
   CBface_list& enclosed_faces
   )
{
   // Helper function used below: given ordered vertices from a closed
   // curve, and the set of faces enclosed by the curve, report
   // whether the curve winds clockwise around the enclosed faces.
   // (If so return true).

   Bface* f = internal_face(v1, v2, enclosed_faces);
   if (!f) return false;
   assert(f->contains(v1) && f->contains(v2));
   return (f->next_vert_ccw(v1) == v2);
}

inline Bface*
external_face(Bvert* v1, Bvert* v2, CBface_list& enclosed_faces)
{
   // Like above.

   assert(v1 && v2);
   Bedge* e = v1->lookup_edge(v2);
   assert(e);
   if (e->f1() && !enclosed_faces.contains(e->f1()))
      return e->f1();
   if (e->f2() && !enclosed_faces.contains(e->f2()))
      return e->f2();
   return 0;
}

inline bool
is_inconsistent_wrt_external_surface(
   Bcurve* bcurve,
   CBface_list& enclosed_faces
   )
{
   // Returns true if curve is oriented INCONSISTENTLY with
   // external surface.

   if (!bcurve) return false;
   Bvert_list verts = bcurve->verts();
   assert(verts.num() > 1);
   Bvert *v1 = verts[0], *v2 = verts[1];

   Bface* f = external_face(v1, v2, enclosed_faces);
   if (!f) return false; // i.e., no worries

   assert(f->contains(v1) && f->contains(v2));
   return (f->next_vert_ccw(v1) == v2);
}

inline bool
is_enclosed(Bcurve* bcurve, CBface_list& enclosed_faces)
{
   assert(bcurve != 0);

   if (bcurve->is_polyline())
      return false;
   else if (bcurve->is_embedded())
      return true;
   else if (bcurve->is_border())
      return enclosed_faces.empty();
   else
      return false;
}

inline bool 
joined_at_top(TubeMap* tube, Wpt& top)
{
   // sample the upper curve of the tube some number of times,
   // if all points project close to the first point, return true.
   // i.e. the upper curve is really shrunken to a point.

   assert(tube);

   // create 11 parameters from 0.0 to 1.0, spacing 1/steps:
   int steps = 16;      // number of samples
   param_list_t params = make_params(steps);

   // remove last one (1.0):
   params.pop();

   // map samples to world space
   Wpt_list pts = tube->c1()->map_all(params);

   // if the spread is wide return false:
   const double JOINED_DIST_THRESH = 1.0;
   if (PIXEL_list(pts).spread() > JOINED_DIST_THRESH)
      return false;

   // record average of points, and return true:
   top = pts.average();
   return true;
}

UVsurface*
UVsurface::build_revolve(
   Bcurve*      bcurve,         // curve around base region
   Map1D3D*     axis,           // central axis (should be straight)
   CWpt_list&   spts,           // "sweep" points, or profile of the revolve
   CBface_list& enclosed_faces, // base region
   Bpoint_list& points,         // points created
   Bcurve_list& curves,         // curves created
   Bsurface_list& surfs,        // surfaces created
   MULTI_CMDptr cmd             // place to put commands used to carry out the op
   )
{
   // Check preconditions:
   assert(bcurve && (bcurve->num_edges() > 2) && axis && spts.num() > 1);
   assert(bcurve->mesh()->is_control_mesh());
   static bool debug = Config::get_var_bool("DEBUG_BUILD_TUBE",false);
   if (!bcurve->is_closed()) {
      err_adv(debug, "UVsurface::build_revolve: rejecting non-closed base curve");
      return 0;
   }
   if (!(bcurve->is_embedded() || bcurve->is_border() || bcurve->is_polyline())) {
      if (debug)
         cerr << "UVsurface::build_revolve: error: curve is not embedded, "
              << "border, or polyline" << endl;
      return 0;
   }
   // Check how curve winds around axis:
   double w = bcurve->winding_number(axis->map(0), axis->tan(0));
   err_adv(debug, "winding: %1.1f", w);
   if (!(isEqual(w,1) || isEqual(w,-1))) {
      err_adv(debug, "rejecting curve w/ bad winding number: %f", w);
      return 0;
   }

   // Get curve verts and curve map;
   // may need to reverse their order in the next step:
   Bvert_list bot_verts = bcurve->full_verts();
   Map1D3D* bot_map = bcurve->map();
   bool need_reverse = false;
   if (is_enclosed(bcurve, enclosed_faces)) {
      need_reverse = is_inconsistent_wrt_external_surface(bcurve, enclosed_faces);
   } else {
      need_reverse = isEqual(w, -1);
   }
   if (need_reverse) {
      err_adv(debug, "build_revolve: reversing curve map");
      bot_map = ReverseMap1D3D::get_reverse_map(bot_map);
      bot_verts.reverse();
   }
   if (bcurve->is_embedded_any_level()) {
      // Enclosed portion of surface gets pushed down to be a
      // non-primary piece of the surface. The surrounding curve will
      // lie along a non-manifold part of the surface.
      if (cmd)
         cmd->add(new PUSH_FACES_CMD(enclosed_faces));
      enclosed_faces.push_layer();
   } else if (bcurve->is_border() && !enclosed_faces.empty()) {
      Bvert *v1 = bot_verts[0], *v2 = bot_verts[1];
      Bface* f = internal_face(v1, v2, enclosed_faces);
      assert(f->contains(v1) && f->contains(v2));
      if (f->next_vert_ccw(v1) == v2) {
         // Normals of base surface point up into the inside of the tube.
         // Flip them the other way to point down.
         err_adv(debug, "build_revolve: reversing enclosed faces");
         // XXX - should be undoable
         reverse_faces(enclosed_faces);
      }
   }

   // Choose appropriate subdiv level to introduce the UVsurface
   // (relative to level of bcurve)
   double L = bcurve->avg_edge_length();
   double h = spts.length();
   int k = max(0, int(round(log2(L/h))));
   err_adv(debug, "UVsurface::build_revolve: L: %f, h: %f, k: %d", L, h, k);
   const int MAX_K = 3;
   if (k > MAX_K) {
      // We fear going too deep in the subdivision:
      err_adv(debug, "Warning: high k value (%d), using %d instead", k, MAX_K);
      k = MAX_K;
   }

   // Create tube map and uv grid to use for mesh
   TubeMap* tube_map = new TubeMap(axis, bot_map, spts);
   // XXX - should fix to compute the real uv coords properly:
   ARRAY<double> uvals = make_params(bcurve->num_edges() * (1 << k));
   ARRAY<double> vvals;
   double aspect = Config::get_var_dbl("TUBE_ASPECT", 1.0,true);
   tube_map->get_min_distortion_v_vals(uvals, vvals, aspect);

   Bcurve*    tcurve = 0;
   Bpoint*    tpoint = 0;
   Bvert_list top_verts;
   Wpt        top_pt;
   bool is_cone_top = joined_at_top(tube_map, top_pt);
   if (is_cone_top) {
      Lvert* v = (Lvert*)bcurve->mesh()->add_vertex(top_pt);
      tpoint = new Bpoint(v, UVpt(0,1), tube_map);
      points += tpoint;
      top_verts += v;
      if (cmd) cmd->add(new SHOW_BBASE_CMD(tpoint));
   } else {
      // Make the top curve to resemble the bottom one
      tcurve = new Bcurve(
         bcurve->mesh(),
         tube_map->c1(),
         make_params(bcurve->num_edges()),
         bcurve->res_level()
         );
      curves += tcurve;
      top_verts = tcurve->full_verts();
      if (cmd) cmd->add(new SHOW_BBASE_CMD(tcurve));
   }

   // Get subdiv mesh and vertex lists at chosen level k
   LMESHptr tube_mesh;
   Bvert_list bot_sub_verts;    // bottom curve verts at level k
   Bvert_list top_sub_verts;    // top curve verts at level k
   if (k > 0) {
      // The body of the "tube" is just a narrow band.
      int abs_k = bcurve->subdiv_level() + k; // k relative to control mesh:
      if (!bcurve->ctrl_mesh()->update_subdivision(abs_k)) {
         assert(0);
      }
      tube_mesh = bcurve->cur_mesh();
      assert(tube_mesh && tube_mesh->subdiv_level() == abs_k);
      get_subdiv_chain(bot_verts, k, bot_sub_verts);
      if (is_cone_top)
         top_sub_verts += ((Lvert*)top_verts[0])->subdiv_vert(k);
      else
         get_subdiv_chain(top_verts, k, top_sub_verts);
   } else {
      tube_mesh = bcurve->mesh();
      bot_sub_verts = bot_verts;      
      top_sub_verts = top_verts;      
   }

   // Generate the surface (empty still):
   UVsurface* ret = new UVsurface(tube_map, tube_mesh);
   surfs += ret;

   int ncols = bot_sub_verts.num();
   int nrows = vvals.num();

   // Make a 2D array of vertices and their uv-coords:
   ARRAY<Bvert_list> verts(nrows);
   ARRAY<UVpt_list>  uvpts(nrows);

   // First row:
   uvpts += make_uvpt_list(uvals, vvals[0]);
   verts += bot_sub_verts;
   ret->add_memes(verts[0], uvpts[0]);

   // Remaining rows:
   for (int j=1; j<nrows; j++) {
      verts += ((j == nrows-1) ? top_sub_verts : Bvert_list());
      uvpts +=  UVpt_list();
      if (verts[j].num() == 1) {
         // cone top
         assert(j == nrows-1 && is_cone_top);
         err_adv(debug, "build_revolve: creating cone top");
         Lvert* v = (Lvert*)verts[j][0];
         UVpt  uv =  UVpt(0, vvals[j]);
         new UVmeme(ret, v, uv, UVmeme::POLE_V);
         ret->build_fan(v, uv, verts[j-1], uvpts[j-1]);
      } else {
         ret->build_row(vvals[j], ncols, verts[j], uvpts[j]);
         ret->build_band(verts[j-1], verts[j],
                         uvpts[j-1], uvpts[j]);
      }
   }
   if (cmd) cmd->add(new SHOW_BBASE_CMD(ret));

   // Set rest length of edges to the average initial length
   // XXX - avg len is okay for panel, testing on uv surface
//   ret->ememes().set_rest_length(ret->bedges().avg_len());

   Panel* top = 0;
   if (!is_cone_top) {
      // Slap on a cap at level 0 relative to the two curves
      Bvert_list final = top_verts; // Get the last row, 
      final.pop();                  // sans the duplicate vertex at the end:
      // XXX - use action
      top = Panel::create(final);
      if (top) {
         surfs += top;
         //       top->ememes().set_rest_length(top->bedges().avg_len());
         if (cmd) cmd->add(new SHOW_BBASE_CMD(top));
      }
   }
   
   bcurve->mesh()->changed(BMESH::TOPOLOGY_CHANGED);
   tube_mesh->changed(BMESH::TOPOLOGY_CHANGED);

   // Set res levels on various pieces
   int kt = ret->subdiv_level();        // tube's subdiv level
   int kc = bcurve->subdiv_level();     // curve's subdiv level
   int kd = kt - kc;                    // difference between 'em
   int rlev = Config::get_var_int("TUBE_RES_LEVEL", 2);
   rlev = max(rlev, kd);

   // have to set res level in order: surfaces, curves, points
   // because setting res level can delete child Bbases,
   // and they have to be deleted in that order.

   // surfaces:
   Bsurface::get_surfaces(enclosed_faces).set_res_level(rlev);
   if (top) top->set_res_level(rlev);
   ret->set_res_level(rlev - kd);

   // curves:
   bcurve->set_res_level(rlev);
   if (tcurve) tcurve->set_res_level(rlev);

   // points:
   if (tpoint) tpoint->set_res_level(rlev);

   bcurve->ctrl_mesh()->update_subdivision(rlev + kc);

   // In case subdiv elements have been generated at lower
   // levels of the mesh already, now is the time to shower
   // down memes upon them:
   // XXX - needed?
   ret->gen_subdiv_memes();

   return ret;
}

UVsurface* 
UVsurface::build_coons_patch(
      Bpoint* a,                                       
      Bpoint* b,                                       
      Bpoint* c,                                       
      Bpoint* d,
      MULTI_CMDptr cmd
   )
{
   //          c2               
   //    d ---------- c                               
   //    |            |                                
   //    |            |                                
   // d1 |            | d2                             
   //    |            |                                
   //    |            |                                
   //    a ---------- b 
   //          c1
   //
   // Build a Coons patch from the 4 points.
   // 

  if ( !(a && b && c && d) ) {
    cerr << "UVsurface::build_coons_patch: ERROR, one or more points are null"
         << endl;
    return 0;
  }

  Bcurve* ab = a->lookup_curve(b);
  Bcurve* bc = b->lookup_curve(c);
  Bcurve* cd = c->lookup_curve(d);
  Bcurve* da = d->lookup_curve(a);

  if ( !( ab && bc && cd && da) ){
    cerr << "UVsurface::build_coons_patch: ERROR, one or more curves are null"
         << endl;
    return 0;
  }

  if ( ab->is_embedded() || 
       bc->is_embedded() || 
       cd->is_embedded() || 
       da->is_embedded() ) {
    cerr << "UVsurface::build_coons_patch: ERROR, one or more curves are embedded"
         << endl;
    return 0;
  }
    
  if ( !(ab->num_edges() == cd->num_edges() &&
         bc->num_edges() == da->num_edges()) ) {
    cerr << "UVsurface::build_coons_patch: ERROR, unequal num edges "
         << "on opposite curves" << endl;
    return 0;
  }

  Map1D3D* c1 = ab->map();
  Map1D3D* c2 = cd->map();
  Map1D3D* d1 = da->map();
  Map1D3D* d2 = bc->map();

  if ( !(c1 && c2 && d1 && d2) ) {
    cerr << "UVsurface::build_coons_patch: ERROR, can't get curve maps" << endl;
    return 0;
  }

  // Done validating ... from here on, we shall surely succeed!!! 

  ARRAY<double> c1_uvals = ab->tvals();
  ARRAY<double> c2_uvals = cd->tvals();
  ARRAY<double> d1_vvals = da->tvals();
  ARRAY<double> d2_vvals = bc->tvals();

  Bvert_list c1_verts = ab->verts();
  Bvert_list c2_verts = cd->verts();
  Bvert_list d1_verts = da->verts();
  Bvert_list d2_verts = bc->verts();

  if ( ab->b1() != a ) {
    c1 = ReverseMap1D3D::get_reverse_map( c1 );
    ReverseMap1D3D::reverse_tvals(c1_uvals);
    c1_verts.reverse();
  }
  if ( bc->b1() != b ) {
    d2 = ReverseMap1D3D::get_reverse_map( d2 );
    ReverseMap1D3D::reverse_tvals(d2_vvals);
    d2_verts.reverse();
  }
  if ( cd->b1() != d ) {
    c2 = ReverseMap1D3D::get_reverse_map( c2 );
    ReverseMap1D3D::reverse_tvals(c2_uvals);
    c2_verts.reverse();
  }
  if ( da->b1() != a ) {
    d1 = ReverseMap1D3D::get_reverse_map( d1 );
    ReverseMap1D3D::reverse_tvals(d1_vvals);
    d1_verts.reverse();
  }

  CoonsPatchMap* coons = new CoonsPatchMap( c1, c2, d1, d2 );

   UVsurface* ret = new UVsurface(coons, a->mesh());

   int ncols = c1_verts.num();
   int nrows = d1_verts.num();

   // Make a 2D array of vertices and their uv-coords:
   ARRAY<Bvert_list> verts(nrows);
   ARRAY<UVpt_list>  uvpts(nrows);

   // First row:
   // XXX - should fix to compute the real uv coords properly
   uvpts += make_uvpt_list(c1_uvals, d1_vvals[0]);
   verts += c1_verts;
   ret->add_memes(verts[0], uvpts[0]);

   // Remaining rows:
   for (int j=1; j<nrows; j++) {
      if (j == nrows-1) {
         // last row
         // XXX - should fix to compute the real uv coords properly
         uvpts += make_uvpt_list(c2_uvals, d1_vvals.last());
         verts += c2_verts;
         ret->add_memes(verts.last(), uvpts.last());
      } else {
         // middle rows
         verts += Bvert_list();
         uvpts +=  UVpt_list();
         // XXX - use uvals
         ret->build_row(d1_vvals[j], ncols, verts[j], uvpts[j],
                     d1_verts[j], d2_verts[j]);
      }
      ret->build_band(verts[j-1], verts[j], uvpts[j-1], uvpts[j]);
   }

   ret->mesh()->changed(BMESH::TOPOLOGY_CHANGED);

   // In case subdiv elements have been generated at lower
   // levels of the mesh already, now is the time to shower
   // down memes upon them:
   ret->gen_subdiv_memes();

   // Make it undoable:
   if (cmd)
      cmd->add(new SHOW_BBASE_CMD(ret));

   return ret;
}



bool 
UVsurface::tessellate_rect(int rows, int cols, bool do_cap)
{
   // We'll tessellate the unit square in UV-space,
   // oriented so u runs horizontally and v runs vertically.

   // Error check
   if (!_map) {
      err_msg("UVsurface::tessellate_rect: Error: map is NULL");
      return false;
   }
   if (!_mesh) {
      err_msg("UVsurface::tessellate_rect: Error: mesh is NULL");
      return false;
   }

   // Wipe out all of our mesh elements and memes, we're
   // starting over.
   delete_elements();

   // Ensure input arguments are consistent:
   if (do_cap && !_map->is_topological_cylinder()) {
      err_msg("UVsurface::tessellate_rect: Error: non-cylinder, can't cap");
      do_cap = false;
   }

   // When _map->wrap_u() is true, that means a curve
   // traced out in the u-direction forms a closed
   // loop. (E.g. a path running around a cylinder).
   //
   // MIN_WRAP_NUM is the bare minimum number of vertices in a
   // direction if the surface wraps in that direction. I.e.,
   // the first and last vertices are the same in that case,
   // so the number of distinct vertices is one less.
   const int MIN_WRAP_NUM = 3; 
   if (do_cap && _map->wrap_u() && cols < MIN_WRAP_NUM) {
      err_msg("UVsurface::tessellate_rect: Error: can't wrap u with %d cols",
              cols);
      cols = MIN_WRAP_NUM;
   }
   if (do_cap && _map->wrap_v() && rows < MIN_WRAP_NUM) {
      err_msg("UVsurface::tessellate_rect: Error: can't wrap v with %d rows",
              rows);
      rows = MIN_WRAP_NUM;
   }
   // Screen out the crazies.
   rows = max(2, rows);
   cols = max(2, cols);

   // Step size in v:
   double dv = 1.0 / (rows - 1);

   // Make a 2D array of vertices and their uv-coords:
   ARRAY<Bvert_list> verts(rows);
   ARRAY<UVpt_list>  uvpts(rows);
   for (int j=0; j<rows; j++) {
      verts += Bvert_list();
      uvpts +=  UVpt_list();
      if (_map->wrap_v() && j == rows - 1) {
         // Copy first row of vertices:
         verts[j] = verts[0];

         // Copy UVpts from first row, then add 1 to v-coord:
         uvpts[j] = uvpts[0];
         uvpts[j].translate(UVvec(0,1));
      } else {
         build_row(dv*j, cols, verts[j], uvpts[j]);
      }

      if (j > 0)
         build_band(verts[j-1], verts[j],
                    uvpts[j-1], uvpts[j]);
   }

   if (do_cap && _map->wrap_u()) {
      CBvert_list& r1 = verts.first();
      CBvert_list& r2 = verts.last();
      CUVpt_list&  u1 = uvpts.first();
      CUVpt_list&  u2 = uvpts.last();
      if (cols == 5) {
         // 5 total, 4 unique
         // Cap off both ends with quads:
         add_quad(r1[0], r1[3], r1[2], r1[1],   // bottom
                  u1[0], u1[3], u1[2], u1[1]);
         add_quad(r2[0], r2[1], r2[2], r2[3],   // top
                  u2[0], u2[1], u2[2], u2[3]);
      } else if (cols == 4) {
         // 4 total, 3 unique
         // Cap off both ends with triangles:
         add_face(r1[0], r1[2], r1[1],          // bottom
                  u1[0], u1[2], u1[1]);
         add_face(r2[0], r2[1], r2[2],          // top
                  u2[0], u2[1], u2[2]);
      } else {
         cerr << "UVsurface::tessellate_rect: end caps not implemented for "
              << cols << " vertices" << endl;
      }
   } else if (do_cap && _map->wrap_v()) {
      cerr << "UVsurface::tessellate_rect: end caps not implemented for "
           << "wrapping in v" << endl;
   }

   _mesh->changed(BMESH::TOPOLOGY_CHANGED);
   err_msg("mesh type: %d", _mesh->type());

   return true;
}

void
UVsurface::build_row(
   double       v,       // v-coord for whole row
   int          n,       // num verts in row
   Bvert_list&  verts,   // vert list: returned
   UVpt_list&   uvs,     // UVpt list: returned
   Bvert*       first_v, // optional first vertex of row
   Bvert*       last_v   // optional last vertex of row
   )
{
   assert(_map);
   assert(n > 1);
   double du = 1.0 / (n - 1);

   bool build_verts = true;
   if (!verts.empty()) {
      assert(verts.num() == n);
      build_verts = false;
   } else {
      verts.clear();
      verts.realloc(n);
   }
   uvs.clear();
   uvs.realloc(n);

   for (int i=0; i<n; i++) {
      uvs += UVpt(du*i, v);
      if (_map->wrap_u() && i == n - 1) {
         // Last one: no need to create a meme, just
         // copy the first vertex to the last slot:
         if (build_verts)
            verts += verts[0];
      } else {
         if (build_verts) {
            // Don't really have to set position now since memes will do it:
            if ( i == 0 && first_v )
               verts += first_v;
            else if ( i == n-1 && last_v )
               verts += last_v;
            else 
               verts += _mesh->add_vertex(_map->map(uvs.last()));
         }
         new UVmeme(this, (Lvert*)verts[i], uvs[i]);
      }
   }
}

void
UVsurface::add_memes(CBvert_list& verts, CUVpt_list& uvs)
{
   if (verts.empty())
      return;
   assert(verts.mesh() == _mesh);
   assert(verts.num() == uvs.num());

   for (int i=0; i<verts.num(); i++)
      if (!find_meme(verts[i]))
         new UVmeme(this, (Lvert*)verts[i], uvs[i]);
}

void
UVsurface::build_band(
   CBvert_list& p,      // "previous" ring of vertices
   CBvert_list& c,      // "current" ring of vertices
   CUVpt_list& puv,     // uv-coords for previous ring
   CUVpt_list& cuv      // uv-coords for current ring
   )
{
   // Build a band of quads joining the given vertex lists.
   //
   //  c0 -------- c1 -------- c2 -- ... -- cn-1
   //   |           |           |            |         
   //   |           |           |            |         
   //   |           |           |            |         
   //   |           |           |            |         
   //   |           |           |            |         
   //   p0 -------- p1 -------- p2 - ... -- pn-1
   //
   //   As shown, surface normal points toward you.

   assert(p.num() == c.num() && p.num() == puv.num() && c.num() == cuv.num());
   int n = c.num();
   for (int i=0; i<n-1; i++) {
      add_quad(  p[i],   p[i+1],   c[i+1],   c[i],
               puv[i], puv[i+1], cuv[i+1], cuv[i]);
   }
}

void
UVsurface::build_fan(
   Bvert*       c,      // center vertex
   CUVpt&       cuv,    // center UV
   CBvert_list& r,      // surrounding ring of vertices
   CUVpt_list&  ruv     // uv-coords for surrounding ring
   )
{
   /*
   //                                                       
   //              rn-2                                     
   //                /\      ...                            
   //               /  \                                    
   //              /    \   /                               
   //             /      \ /                                
   //   rn-1 = r0 ------- c ------- r4                      
   //             \      / \      /                         
   //              \    /   \    /                          
   //               \  /     \  /                           
   //                \/_______\/                            
   //              r1           r3                          
   //                                                       
   //   As shown, surface normal points toward you.
   */

   assert(r.num() > 2 && r.first() == r.last() && r.num() == ruv.num());
   int n = r.num();
   for (int i=0; i<n-1; i++) {
      add_face(  r[i],   r[i+1],   c,
               ruv[i], ruv[i+1], cuv); 
   }
}

Lface*
UVsurface::inset_quad(
   Bvert* v1, Bvert* v2, Bvert* v3, Bvert* v4,
   CUVpt& ua, CUVpt& ub, CUVpt& uc, CUVpt& ud)
{
   //   v4 ---------- v3                              
   //    |            |                                
   //    |   ud---uc  |                                
   //    |    |   |   |                                
   //    |    |   |   |                                
   //    |   ua---ub  |                                
   //    |            |                                
   //   v1 ---------- v2
   // 
   //    Stitch it up at given uv-coords, return the inset quad:
   //

   bool debug = Config::get_var_bool("DEBUG_INSET_QUAD",false);

   // Get the quad face and make sure we own it:
   Bedge* e = v1->lookup_edge(v3);
   if (!e)
      e = v2->lookup_edge(v4);
   if (!(e && e->is_weak())) {
      if (debug) err_msg("UVsurface::inset_quad: can't find the quad");
      return 0;
   }

   Bface* f = e->f1();
   if (find_owner(f) != this) {
      if (debug) err_msg("UVsurface::inset_quad: not owner of quad");
      return 0;
   }

   // Get existing uv coords of the quad
   // XXX - things changed - need to get uv-coords from the memes
   UVpt u1, u2, u3, u4;
   if (!UVdata::get_quad_uvs(v1,v2,v3,v4,u1,u2,u3,u4))
      return 0;

   // Create the new vertices:
   assert(_map);
   Lvert* va = (Lvert*)_mesh->add_vertex(_map->map(ua));
   Lvert* vb = (Lvert*)_mesh->add_vertex(_map->map(ub));
   Lvert* vc = (Lvert*)_mesh->add_vertex(_map->map(uc));
   Lvert* vd = (Lvert*)_mesh->add_vertex(_map->map(ud));

   // Make the new vert memes:
   new UVmeme(this, va, ua);
   new UVmeme(this, vb, ub);
   new UVmeme(this, vc, uc);
   new UVmeme(this, vd, ud);

   // Blow out the quad (this deletes f)
   _mesh->remove_edge(f->weak_edge());

   add_quad(v1, v2, vb, va, u1, u2, ub, ua);
   add_quad(v2, v3, vc, vb, u2, u3, uc, ub);
   add_quad(v3, v4, vd, vc, u3, u4, ud, uc);
   add_quad(v4, v1, va, vd, u4, u1, ua, ud);
   add_quad(va, vb, vc, vd, ua, ub, uc, ud);

   _mesh->changed(BMESH::TRIANGULATION_CHANGED);

   invalidate();

   // Return the new quad:
   return (Lface*)lookup_face(va, vb, vc);
}

void 
UVsurface::notify_xform(BMESH*, CWtransf& xf, CMOD& mod)
{
   bool debug = Config::get_var_bool("DEBUG_BNODE_XFORM");
   if (is_control() && _map) {
      if (!_map->can_transform()) {
         err_adv(debug,
                 "UVsurface::notify_xform: can't transform the %s map",
                 **_map->class_name());
      } else 
         _map->transform(xf, mod);
   }
}

/* end of file uv_surface.C */
