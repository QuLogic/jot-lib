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

/*!
 *  \file profile.C
 *  \brief Contains the definition of the PROFILE widget.
 *
 *  \ingroup group_FFS
 *  \sa profile.H
 *
 */

#include "disp/colors.H"                // Color::blue_pencil_d
#include "gtex/util.H"                  // for draw_strip
#include "gtex/ref_image.H"             // for VisRefImage
#include "gtex/basic_texture.H"         // GLStripCB
#include "tess/mesh_op.H"               // SUBDIV_OFFSET_CMD
#include "tess/primitive.H"
#include "tess/xf_meme.H"
#include <map>

#include "profile.H"

static bool debug = Config::get_var_bool("DEBUG_PROFILE",false);

using namespace mlib;
using namespace GtexUtil;

/*****************************************************************
 * PROFILE
 *****************************************************************/
PROFILEptr PROFILE::_instance;

PROFILE::PROFILE() :
   DrawWidget()
{
   _draw_start += DrawArc(new TapGuard,    drawCB(&PROFILE::tap_cb));
   _draw_start += DrawArc(new StrokeGuard, drawCB(&PROFILE::stroke_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);
}

void 
PROFILE::clean_on_exit() 
{ 
   _instance = 0; 
}

PROFILEptr
PROFILE::get_instance()
{
   if (!_instance)
      _instance = new PROFILE();
   return _instance;
}

bool 
PROFILE::init(CGESTUREptr& g)
{
   if (_active)
      return false;
   if (!(g && g->is_stroke()))
      return false;

   if (g->below_min_length() || g->below_min_spread())
      return false;

   // is it running along the silhouette
   if (get_instance()->find_matching_sil(g))
      return true;

   // is it running along a cross section
   if (get_instance()->find_matching_xsec(g))
      return true;

   // add more cases here...

   return false;
}

// returns whether or not a face is on the cap.
// we assume that this face is either on the cap or on the side
// of a tube.
inline bool
is_cap(Bface* f)
{
   if (!f)
      return false;
   f = ((Lface*)f)->control_face();

   // the verts of the face should be controlled by XFMemes
   XFMeme* m1 = (XFMeme*)Bbase::find_boss_meme(f->v1());
   XFMeme* m2 = (XFMeme*)Bbase::find_boss_meme(f->v2());
   XFMeme* m3 = (XFMeme*)Bbase::find_boss_meme(f->v3());
   if (!(m1 && m2 && m3))
      return false;

   // if the tube is built by Extender widget, then face is
   // on the cap iff its xfmemes are in the same coord frame
   CoordFrame* f1 = m1->frame();
   CoordFrame* f2 = m2->frame();
   CoordFrame* f3 = m3->frame();
   if ((f1==f2) && (f2==f3))
      return true;

   // if the tube is built by Paper Doll widget, then face is
   // on the cap iff its v coords are the same (0 or 1)
   if (UVdata::quad_has_uv(f)) {
      UVpt uv1, uv2, uv3;
      UVdata::get_uvs(f, uv1, uv2, uv3);
      if ((uv1[1]==uv2[1]) && (uv2[1]==uv3[1]) 
         && (abs(uv1[1]-0.5)==0.5))
         return true;
   }

   return false;
}

// check if edge e is on the side of a tube
inline bool
check_tube_side(Bedge* e)
{
   // we do not want edges on the top or bottom
   // of the tube
   Bsurface_list surfs = Primitive::get_primitives(
      get_top_level(e->get_all_faces()));
   if (surfs.num() != 1 || !Primitive::isa(surfs[0]))
      return false;   
   if (is_cap(e->f1()) || is_cap(e->f2()))
      return false;

   // should have uvpt stored
   if (!UVdata::has_uv(e->f1()) || !UVdata::has_uv(e->f2()))
      return false;

   return true;
}

const double SIL_SEARCH_RAD = 12;
const int MIN_GEST_PTS = 10;

// is it running along a cross section?
bool
PROFILE::find_matching_xsec(CGESTUREptr& g)
{
   err_adv(debug, "PROFILE:find_matching_xsec");

   if (!(g && g->pts().num() >= MIN_GEST_PTS))
      return false;

   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   if (!vis_ref)
      return false;

   //    see if the gesture runs along a cross section
   //    of a primitive

   const  PIXEL_list& pts = g->pts();
   ARRAY<PIXEL_list> pt_lists;
   PIXEL_list temp_list;
   bool recording = false;
   Primitive* p = 0;
   BMESH* mesh = 0;

   // find all the sections
   for (int i=0; i<pts.num(); i++) {
      Primitive* temp_p = Primitive::find_controller(vis_ref->intersect(pts[i]));
      if (temp_p == p && recording) {
         temp_list += pts[i];
         if (i == pts.num()-1) pt_lists += temp_list;
      } else if (temp_p && temp_p != p && !recording) { // start
         recording = true;
         temp_list.clear();
         temp_list += pts[i];
      } else if (temp_p != p && recording) { // end
         recording = false; 
         pt_lists += temp_list;
         i--;
      }
      p = temp_p;

      if (mesh && temp_p && mesh != temp_p->mesh()) {
         err_adv(debug, "   found a second mesh, rejecting");
         return false;
      }
      if (temp_p) mesh = temp_p->mesh();
   }
   if (!LMESH::isa(mesh)) {
      err_adv(debug, "   found non-LMESH, rejecting");
      return false;
   }

   for (int i = 0; i < pt_lists.num(); i++) 
      if (!do_xsec_match(pt_lists[i])) {
         err_adv(debug, "   gesture not aligned with cross section");
         return false;
      }
   err_adv(debug, "   gesture aligns with cross section");
   err_adv(debug, "   mesh level %d", mesh->subdiv_level());

   _mesh = LMESH::upcast(_selected_edges.mesh());
   _n = _a = _b = 0;
   _mode = 1; // _mode is set to editing the cross section
   activate();
   select_faces();

   return true;
}

// get a face from all the neighboring faces, preferrably
// not a face on the cap
inline Bface*
get_preferred_face(Bvert* v)
{
   Bface_list faces = v->get_all_faces();
   for (int i = 0; i < faces.num(); i++)
      if (!is_cap(faces[i]))
         return faces[i];
   return v->get_face();
}

// get_uv wrapper
inline bool
get_uv(Bvert* v, UVpt& uv)
{
   bool ret = false;
   Bface_list faces = v->get_all_faces();
   UVpt temp_uv;
   uv[0] = 1000000; // large enough number
   
   for (int i = 0; i < faces.num(); i++) {
      if (!is_cap(faces[i])) {
         ret |= UVdata::get_uv(v, faces[i], temp_uv);
         if (temp_uv[0] < uv[0]) {
            uv = temp_uv;
         }
      }
   }

   return ret;
}

// find the next n verts in a certain direction on a silhouette 
// with the same v coords (or on a cross section with 
// the same u coords if _mode is 0: editing silhouettes)
Bvert_list
PROFILE::n_next_verts(Bvert* vert, int n, bool dir)
{
   Bvert_list ret;
   bool has_uv = false;

   // get the uvpt stored at vert
   UVpt uv;
   get_uv(vert, uv); 

   double h = uv[!_mode], w = uv[_mode];

   // is reaching the cap
   if (!dir && _mode && fabs(w-1)<0.001) return ret;
   if (dir && _mode && fabs(w)<0.001) return ret;

   // find the 2 candidate neiboring edges in both directions
   Bedge_list star = vert->get_adj();
   Bedge_list cands;
   for (int i = 0; i < star.num(); i++) {
      Bvert* temp_v = star[i]->other_vertex(vert);
      has_uv = get_uv(temp_v, uv);
      // candidate should have the same u (or v if _mode is 0) coordinate as
      // that of the input vert. when we are comparing u coordinates, keep
      // in mind that the u coordinates form a closed ring.
      if (has_uv && (fabs(uv[!_mode]-h)<0.001)) {
         if (!(is_cap(star[i]->f1())&&is_cap(star[i]->f2())))
            cands += star[i];
      }
   } 
   // do some checks
   if (cands.num() != 2) {
      if (!(_mode && ((fabs(w-1)<0.001 && dir) || (fabs(w)<0.001 && !dir)) && cands.num()==1)) {
         if (debug) cerr << "   h: " << h << "; num: " << cands.num() << endl;
         return ret;
      }
   }

   // decide which candidate to choose based on direction
   Bedge* e = cands[0];
   if (cands.num() == 2) {
      Bvert* v1 = cands[0]->other_vertex(vert);
      Bvert* v2 = cands[1]->other_vertex(vert);
      UVpt uv1, uv2;
      has_uv = get_uv(v1, uv1) && get_uv(v2, uv2);
      if (has_uv) {
         if ((uv1[_mode] < w && uv2[_mode] < w) ||
            (uv1[_mode] > w && uv2[_mode] > w)) {
            if (uv1[_mode] < uv2[_mode])
               e = dir ? cands[1] : cands[0];
            else
               e = dir ? cands[0] : cands[1];
         } else {
            if (uv1[_mode] < uv2[_mode])
               e = dir ? cands[0] : cands[1];
            else
               e = dir ? cands[1] : cands[0];
         }
      }
   }

   // do the search iteratively, similar to finding the candidates above
   Bvert* v = vert;
   for (int i = 0; i < n; i++) {
      v = e->other_vertex(v);
      ret += v;
      star = v->get_adj();
      int j = 0;
      for (; j < star.num(); j++) {
         Bvert* temp_v = star[j]->other_vertex(v);
         has_uv = get_uv(temp_v, uv);
         if ((fabs(uv[!_mode]-h)<0.001) && star[j] != e) {
            if (!(is_cap(star[j]->f1())&&is_cap(star[j]->f2()))) {
               e = star[j];
               break;
            }
         }
      }
      if (j == star.num()) break;
   }
   return ret;
}

// is this pixel trail running along a cross section
bool
PROFILE::do_xsec_match(PIXEL_list& pts)
{
   if (pts.num() < max(MIN_GEST_PTS, 1)) {
      err_adv(debug, "   too few points");
      return false;
   }

   // find vert that is closest to the starting pixel
   Wpt hit;
   Bface* f = VisRefImage::Intersect(pts[0], hit);
   if (!f) return false;
   Bvert* v = closest_vert(f, f->mesh()->xform()*hit);

   Primitive* p = Primitive::find_controller(f);
   if (!p) return false;

   int delta_n = 1 << p->rel_cur_level();

   // _mode is set to edit silhouette first
   // (actually, this is temporary setting for finding
   // the cross section)
   _mode = 0;
   Bvert_list xsec_ring(v);
   for (int i = 0; i < 100; i++) { // avoid inf loops
      xsec_ring += n_next_verts(xsec_ring.last(), delta_n, true);
      if (xsec_ring.last() == v) break;
   }

   // the pixel trail should not be too far away from the cross section
   PIXEL_list ring_wpts = xsec_ring.wpts();
   for (int i = 0; i < pts.num(); i++)
      if (ring_wpts.dist(pts[i]) > SIL_SEARCH_RAD)
         return false;
   
   // add to selection
   for (int i = 0; i < xsec_ring.num()-1; i++)
      _selected_edges.add(xsec_ring[i], lookup_edge(xsec_ring[i], xsec_ring[i+1]));   

   return true;
}

// is it running along a silhouette?
bool
PROFILE::find_matching_sil(CGESTUREptr& g)
{
   err_adv(debug, "PROFILE::find_matching_sil");

   if (!(g && g->pts().num() >= MIN_GEST_PTS))
      return false;

   if (BMESH::_freeze_sils)
      return false;

   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   if (!vis_ref)
      return false;

   // 1. see if the gesture runs along a silhouette
   //    of a single mesh.

   SilEdgeFilter sil_filter;
   const  PIXEL_list& pts = g->pts();
   BMESH* mesh = 0;
   for (int i=0; i<pts.num(); i++) {
      Bedge* e = (Bedge*)
         vis_ref->find_near_simplex(pts[i], SIL_SEARCH_RAD, sil_filter);
      if (!(e && e->mesh())) {
         err_adv(debug, "   gesture too far from silhouette");
         return false;
      }
      if (mesh && mesh != e->mesh()) {
         err_adv(debug, "   found a second mesh, rejecting");
         return false;
      }
      mesh = e->mesh();
   }
   if (!LMESH::isa(mesh)) {
      err_adv(debug, "   found non-LMESH, rejecting");
      return false;
   }

   err_adv(debug, "   gesture aligns with silhouette");
   err_adv(debug, "   mesh level %d", mesh->subdiv_level());

   // 2. extract the portion of the silhouette that matches
   //    the gesture, store in _selected_edges

   return find_matching_sil(pts, mesh->sil_strip());
}

inline bool
do_match(CPIXEL_list& pts, Bedge* e)
{
   if (!e)
      return false;
   if (pts.dist(e->v1()->wloc()) > SIL_SEARCH_RAD ||
       pts.dist(e->v2()->wloc()) > SIL_SEARCH_RAD)
      return false;
   // check alignment?
   return true;
}

// is the pixel trail running along a silhouette?
bool
PROFILE::find_matching_sil(CPIXEL_list& pts, CEdgeStrip& sils)
{
   err_adv(debug, "   checking %d edges", sils.num());
   _selected_edges.reset();

   // pixel trail should be near enough to the silhouette and it should
   // be the silhouette of a tube
   for (int i=0; i<sils.num(); i++)
      if (do_match(pts, sils.edge(i)) && check_tube_side(sils.edge(i)))
         _selected_edges.add(sils.vert(i), sils.edge(i));

   _mesh = LMESH::upcast(_selected_edges.mesh());
   _n = _a = _b = 0;
   _mode = 0; // _mode set to editting silhouette
   if (_mesh) {
      int n = _selected_edges.num_line_strips();
      _selected_edges = _selected_edges.get_unified();
      err_adv(debug, "   found %d edges, (%d pieces, was %d)",
              _selected_edges.num(), _selected_edges.num_line_strips(), n);
      activate();
      select_faces();
      return true;
   }
   _selected_edges.reset();
   err_adv(debug, "   failed");
   return false;
}

// get the "vertical" edge of the face on the tube
// vertical: u coordinates are the same
inline Bedge*
get_sil_edge(Bface* f)
{
   UVpt uv1, uv2, uv3;
   UVdata::get_uvs(f, uv1, uv2, uv3);
   Bedge* edge = NULL;
   if (uv1[0] == uv2[0] && uv1[1] != uv2[1])
      edge = f->e1();
   else if (uv1[0] == uv3[0] && uv1[1] != uv3[1])
      edge = f->e3();
   else if (uv2[0] == uv3[0] && uv2[1] != uv3[1])
      edge = f->e2();
   return edge;
}

// given a quad face, and its weak edge
// -------------------------------
// |   / |  / |
// |  /e | /e |
// | /  f|/  f|.............
// ------------------------------
// find the next such a pair in the quad chain
inline Bface_list
get_neibors(Bface*& f, Bedge*& e)
{
   Bface_list ret;
   if (!f->contains(e) || !e->is_weak())
      return ret;

   Bedge* edge = get_sil_edge(f);
   assert(edge && edge != e);

   ret += f;
   ret += edge->other_face(f);
   e = ret[1]->other_edge(edge->other_vertex(edge->shared_vert(e)), edge);
   assert(e);
   f = e->other_face(ret[1]);
   assert(f);
   return ret;
}

// starting with a quad edge (can be weak)
// find the next n quads in a certain direction in the quad chain
Bface_list
PROFILE::n_next_quads(Bedge* edge, int n, bool dir, bool add_bound)
{
   Bface_list ret;

   // find the next n verts first
   Bvert_list vlist1(edge->v1()), vlist2(edge->v2());
   vlist1 += n_next_verts(vlist1.last(), n, dir);
   vlist2 += n_next_verts(vlist2.last(), n, dir);
   int m = min(vlist1.num(), vlist2.num());
   if (m == vlist1.num())
      vlist2 = vlist2.extract(0, m);
   else
      vlist1 = vlist1.extract(0, m);

   // four verts form a quad
   for (int i = 0; i < vlist1.num()-1; i++) {
      Bface* f = NULL;
      f = lookup_face(vlist1[i], vlist1[i+1], vlist2[i]); 
      if (f) ret += f;
      f = lookup_face(vlist1[i], vlist1[i+1], vlist2[i+1]);
      if (f) ret += f;
      f = lookup_face(vlist1[i], vlist2[i+1], vlist2[i]);
      if (f) ret += f;
      f = lookup_face(vlist1[i+1], vlist2[i+1], vlist2[i]);
      if (f) ret += f;
   }

   // set up boundary
   if (vlist1.num() != 1) {
      Bedge* e = lookup_edge(vlist1.last(), vlist2.last());
      if (e && add_bound) {
         _region_boundary.add(e->v1(), e);
         _boundary_side += dir;
      }
   }

   return ret.unique_elements();
}

// identify the area that'll be influenced by future editing
void
PROFILE::select_faces()
{
   if (_selected_edges.empty()) {
      err_adv(debug, "PROFILE::select_faces: no selected edges");
      return;
   }
   LMESH* m = LMESH::upcast(_selected_edges.mesh());
   assert(m && _selected_edges.same_mesh());
   MeshGlobal::deselect_all();

   _selected_region.clear();
   _region_boundary.reset();
   _boundary_side.clear();

   // use quad chains to define the area of interest
   for (int i = 0; i < _selected_edges.num(); i++) {
      Bedge* e = _selected_edges.edge(i);
      Primitive* p = Primitive::find_controller(e);
      int n = (1 << p->rel_cur_level()) + _n; // default size
      if (UVdata::get_uv(e->v1(), e->f1())[0] == UVdata::get_uv(e->v2(), e->f1())[0]
         || e->is_weak() || _mode)
         _selected_region += n_next_quads(e, n+_a, true, true) +
            n_next_quads(e, n+_b, false, true);
   }

   MeshGlobal::select(_selected_region);
}

void
PROFILE::reset()
{
   _selected_edges.reset();
   _selected_region.clear();
   _region_boundary.reset();
   _boundary_side.clear();
   _mesh = 0;
}

int  
PROFILE::cancel_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "PROFILE::cancel_cb");

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we did use up the gesture
}

// tap on the cap of the tube may trigger sharp end transformation.
// if not, tap will deactivate the widget
int  
PROFILE::tap_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "PROFILE::tap_cb");

   // sharp end transformation is only possible when we are in
   // the mode of editing cross section
   if (_mode) { 
      PIXEL tap = gest->center();
      Bface* f = VisRefImage::Intersect(tap);
      if (Primitive::find_controller(f) && is_cap(f)) {
         
         // it is also required that we're editing the top
         // cross section of the tube
         int i = 0;
         Bvert* v = NULL;
         for (; i < _selected_edges.num(); i++) {
            UVpt uv;
            v = _selected_edges.vert(i);
            get_uv(v, uv);
            if (fabs(uv[1]-1)<0.001 || fabs(uv[1])<0.001) break;
         }

         // do the transformation
         if (i != _selected_edges.num()) {
            err_adv(debug, "   sharp end mode");
            sharp_end_xform(v, tap);
         }

      }
   }

   return cancel_cb(gest, s);
}

int  
PROFILE::stroke_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "PROFILE::stroke_cb");
   assert(gest);

   if (!try_oversketch(gest->pts()))
      try_extend_boundary(gest->pts());

   return 1; // This means we did use up the gesture
}

inline int
get_near_index(CPIXEL_list& path, CPIXEL& p, double max_dist)
{
   int k = -1;
   PIXEL foo;
   if (path.closest(p, foo, k) < max_dist)
      return k;
   return -1;
}

inline double
avg_dist(PIXEL_list& l1, PIXEL_list& l2)
{
   double sum = 0.0;
   for (int i = 0; i < l2.num(); i++)
      sum += l1.dist(l2[i]);
   return sum / l2.num();
}

inline void
make_strip(PIXEL_list pts, Bvert_list chain, int k0, int k1, EdgeStrip& strip)
{
   assert(chain.forms_chain());
   assert(chain.valid_index(k0) && chain.valid_index(k1));
   assert(k0 != k1);
   if (k0 > k1) {
      chain.reverse();
      int n = chain.num()-1;
      k0 = n - k0;
      k1 = n - k1;
   }
   strip.reset();
   for (int i=k0; i<k1; i++) {
      strip.add(chain[i], lookup_edge(chain[i], chain[i+1]));
   }

   if (chain.first() == chain.last()) {
      EdgeStrip alt_strip;
      for (int i = k1; i < chain.num(); i++)
         alt_strip.add(chain[i], lookup_edge(chain[i], chain[i+1]));
      for (int i = 0; i < k0; i++)
         alt_strip.add(chain[i], lookup_edge(chain[i], chain[i+1]));
      PIXEL_list alt_path(alt_strip.verts().wpts());
      PIXEL_list path(strip.verts().wpts());
      if (avg_dist(alt_path, pts) < avg_dist(path, pts))
         strip = alt_strip;
   }
}

bool
PROFILE::match_substrip(CPIXEL_list& pts, CBvert_list& chain, EdgeStrip& strip)
{
   err_adv(debug, "PROFILE::match_substrip:");
   if (!chain.forms_chain()) {
      err_adv(debug, "  non-chain");
      return false;
   }

   const double MAX_DIST = 8;

   PIXEL_list chain_path(chain.wpts());

   int k0 = get_near_index(chain_path, pts.first(), MAX_DIST);
   int k1 = get_near_index(chain_path, pts.last(),  MAX_DIST);

   if (!(chain.valid_index(k0) && chain.valid_index(k1))) {
      err_adv(debug, "  bad k0/k1: %d/%d", k0, k1);
      return false;
   } if (k0 == k1) {
      err_adv(debug, "  bad k0/k1: %d == %d", k0, k1);
      return false;
   }

   make_strip(pts, chain, k0, k1, strip);

   return true;
}

bool
PROFILE::get_sil_substrip(CPIXEL_list& pts, EdgeStrip& strip)
{
   Bvert_list chain;
   for (int k=0; _selected_edges.get_chain(k, chain); ) 
      if (match_substrip(pts, chain, strip))
         return true;
   return false;
}

inline double
compute_offset(Bvert* v, CPIXEL_list& pts, double yardstick)
{
   assert(v);
   PIXEL p(v->loc()), hit;
   VEXEL n(v->loc(), v->norm()*yardstick);

   // require that oversketch is in a reasonable range.
   if (!(pts.intersects_seg(PIXELline(p, n)) ||
         pts.intersects_seg(PIXELline(p, n*(-0.5)))))
      return 0;

   bool inside = false;
   if (!pts.ray_intersect(p, n, hit)) {
      if (!pts.ray_intersect(p, -n, hit)) {
         err_adv(debug, "  missed");
         return 0;
      }
      inside = true;
   }
   Wline ray = XYpt(hit);
   double ret = Wline(v->loc(), v->norm()).intersect(ray).dist(v->loc());
   if (inside)
      ret = -ret;
   err_adv(debug, "  found offset %s", inside ? "inward" : "outward");
   return ret;
}

inline double
signed_area(CWpt& a, CWpt& b, CWpt& c, CWvec& n)
{
   return 0.5 * (cross(b-a,c-a) * n);
}

inline void
project_barycentric(Wpt a, Wpt b, Wpt c, CWpt& p, Wvec& ret)
{
   Wvec norm = (cross(b-a,c-a)).normalized();
   double u = signed_area(p, b, c, norm) / signed_area(a, b, c, norm);
   double v = signed_area(a, p, c, norm) / signed_area(a, b, c, norm);
   ret.set(u, v, 1 - u - v);
}

class CapFaceFilter : public SimplexFilter {
 public:
    virtual bool accept(CBsimplex* s) const {
       Bface* f = (Bface*)s;
       return is_cap(f);
    }
};

class CapFaceBoundaryEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      Bedge* e = (Bedge*)s;
      return e->nfaces_satisfy(CapFaceFilter())==1;
   }
};

bool
PROFILE::sharp_end_xform(Bvert* v, PIXEL tap)
{
   Wpt_list new_locs;
   
   Primitive* p = Primitive::find_controller(v);
   int lev = p->rel_cur_level();

   assert(_mode);
   _mode = 0;

   // find cap ring
   Bvert_list ring0(v);
   for (int i = 0; i < 100; i++) { // avoid inf loops
      ring0 += n_next_verts(ring0.last(), 1<<lev, true);
      if (ring0.last() == v) break;
   }

   // find a control vert of the cap ring
   Bvert* ctrl = NULL;
   for (int i = 0; i < ring0.num(); i++)
      if ((ctrl = ((Lvert*)ring0[i])->ctrl_vert()))
         break;
   if (!ctrl) return false;

   // find cap ring at ctrl level
   Bvert_list verts(ctrl);   
   for (int i = 0; i < 100; i++) { // avoid inf loops
      verts += n_next_verts(verts.last(), 1, true);
      if (verts.last() == ctrl) break;
   }
   verts.pop();
   if (verts.num() < 3) return false;

   // find the intersection point on the cap
   Wpt o = verts.center();
   Wpt target0 = Wplane(verts[0]->loc(), verts[1]->loc(), o).intersect(Wline(tap));
   Wvec bc;
   project_barycentric(verts[0]->loc(), verts[1]->loc(), o, target0, bc);
   if (debug) cerr << "   bc: " << bc << endl;

   // set up the target locs for cap ring verts
   for (int i = 0; i < verts.num(); i++)
      new_locs += target0;

   // find the area of influence
   _mode = 1;
   int n = ((_n+_a)/(1<<lev))+1, ring_len = verts.num();
   Bvert_list affected_rings = n_next_verts(verts.last(), n, true);
   if (affected_rings.empty()) {
      n = ((_n+_b)/(1<<lev))+1;
      affected_rings = n_next_verts(verts.last(), n, false);
   }
   if (debug) cerr << "   found " << affected_rings.num() << " affected rings" << endl;

   // define target locs for cap interior verts
   Bedge* e = lookup_edge(verts[0], verts[1]);
   Bface* f = is_cap(e->f1()) ? e->f1() : e->f2();
   Bvert_list interior_verts = 
      Bface_list::reachable_faces(f, !CapFaceBoundaryEdgeFilter()).interior_verts();
   verts += interior_verts;
   for (int i = 0; i < interior_verts.num(); i++)
      new_locs += target0;

   // define target locs for verts influenced by the xform
   _mode= 0;
   for (int i = 0; i < affected_rings.num(); i++) {
      Bvert_list ring = n_next_verts(affected_rings[i], ring_len, true);
      if (ring.num() != ring_len) return false;
      Wpt target = bc[0]*ring[0]->loc() + bc[1]*ring[1]->loc() + bc[2]*ring.center();
      verts += ring;
      for (int j = 0; j < ring.num(); j++) {
         Wvec dir = ring[j]->loc() - target;
         new_locs += target + dir*(((double)i+1)/(affected_rings.num()+1));
      }
   }

   _mode = 1;
   FIT_VERTS_CMDptr cmd = new FIT_VERTS_CMD(verts, new_locs);
   WORLD::add_command(cmd);
   return true;
}

inline void
get_parents(
   CBvert_list&         children,
   CARRAY<double>&      child_offsets,
   Bvert_list&          parents,        // return val
   ARRAY<double>&       parent_offsets  // return val
   )
{
   assert(children.num() == child_offsets.num());
   parents.clear();
   parent_offsets.clear();
   if (!LMESH::isa(children.mesh()))
      return;
   for (int i=0; i<children.num(); i++) {
      Lvert* p = ((Lvert*)children[i])->parent_vert(1);
      if (p) {
         parents += p;
         parent_offsets += child_offsets[i];
      }
   }
}

bool
PROFILE::apply_offsets(CBvert_list& sil_verts, CARRAY<double>& sil_offsets)
{
   assert(sil_verts.num() == sil_offsets.num());
   if (sil_verts.empty())
      return false;

   Primitive* p = Primitive::find_controller(sil_verts[0]);
   int n = (1 << p->rel_cur_level()) + _n;

   map<Bvert*, double> os_map;
   map<Bvert*, double>::iterator it, end = os_map.end();
   for (int i = 0; i < sil_verts.num(); i++) {
      os_map[sil_verts[i]] = sil_offsets[i];
      Bvert_list v_list1 = n_next_verts(sil_verts[i], n+_a, true);
      for (int j = 0; j < v_list1.num(); j++) {
         double offset = sil_offsets[i] * cos(((double)(j+1))/v_list1.num() * (M_PI/2));
         if (os_map.find(v_list1[j])==os_map.end() || fabs(os_map[v_list1[j]])<fabs(offset))
            os_map[v_list1[j]] = offset;
      }

      Bvert_list v_list2 = n_next_verts(sil_verts[i], n+_b, false);
      for (int j = 0; j < v_list2.num(); j++) {
         double offset = sil_offsets[i] * cos(((double)(j+1))/v_list2.num() * (M_PI/2));
         if (os_map.find(v_list2[j])==os_map.end() || fabs(os_map[v_list2[j]])<fabs(offset))
            os_map[v_list2[j]] = offset;
      }

   }

   Bvert_list region_verts;
   ARRAY<double> offsets;
   for (it = os_map.begin(); it != os_map.end(); it++)  {
      region_verts += it->first;
      offsets += it->second;
   }

   Bvert_list parent_verts;
   ARRAY<double> parent_offsets;
   get_parents(region_verts, offsets, parent_verts, parent_offsets);

   SUBDIV_OFFSET_CMDptr cmd = new SUBDIV_OFFSET_CMD(parent_verts, parent_offsets);
   WORLD::add_command(cmd);
   return true;
}


inline double
compute_yardstick(CBedge_list& edges, bool debug=false)
{
   double ret = 0.6 * edges.strong_edges().avg_len();

   BMESH* mk = edges.mesh();
   BMESH* m0 = get_top_level(edges.get_faces()).mesh();

   int    lk = 0;       // mesh level of edges
   int    l0 = 0;       // mesh level of edges' control region
   double  s = 1;       // scaling factor
   if (mk && m0) {
      lk = mk->subdiv_level();
      l0 = m0->subdiv_level();
      s = (1 << (lk - l0));
   }
   err_adv(debug, "lk: %d, l0: %d, scaling: %f", lk, l0, s);
   return s * ret;
}

bool
PROFILE::compute_offsets(CPIXEL_list& pts, CEdgeStrip& sils)
{
   double yardstick = compute_yardstick(sils.edges(), debug);

   assert(sils.num_line_strips() == 1);
   Bvert_list chain;
   int k = 0;
   sils.get_chain(k, chain);
   int count = 0;
   ARRAY<double> offsets;
   Wpt_list new_locs = chain.pts();
   for (int i=0; i<chain.num(); i++) {
      offsets += compute_offset(chain[i], pts, yardstick);
      if (offsets.last()==0 && offsets.num()!=1 && offsets.num()!=chain.num())
         return false;
      if (offsets.last() > 0) {
         count++;
      }
   }
   err_adv(debug, "found %d/%d positive offsets", count, chain.num());

   apply_offsets(chain, offsets);

   return true;
}

bool
PROFILE::try_oversketch(CPIXEL_list& pts)
{
   err_adv(debug, "PROFILE::try_oversketch:");

   // find best-matching subset of existing selected sils

   // if none, reject

   if (_selected_edges.empty()) {
      err_adv(debug, "  no selected sils");
      return false;
   }

   EdgeStrip sils;
   if (!get_sil_substrip(pts, sils)) {
      err_adv(debug, "  no matching substrip");
      return false;
   }
   err_adv(debug, "  found substrip");

   // compute offsets per vertex of matching sil
   if (!compute_offsets(pts, sils)) {
      err_adv(debug, "  compute offsets failed");
      return false;
   }

   // propagate to surrounding region
   
   return true;
}

inline bool
visible(Bvert* v)
{
   Bface* f = VisRefImage::Intersect(v->ndc());
   return f && f->contains(v);
}

inline int
find_nearest_edge(EdgeStrip& strip, PIXEL pt, double& dist)
{
   int ret = -1;
   dist = 1e10;
   for (int i = 0; i < strip.num(); i++) {
      if (!visible(strip.edge(i)->v1()) || !visible(strip.edge(i)->v2()))
         continue;
      Wvec bc; int foo;
      PIXEL nearest = PIXEL(strip.edge(i)->nearest_pt_ndc(pt, bc, foo));
      double temp_dist = nearest.dist(pt);
      if (temp_dist < dist) {
         ret = i;
         dist = temp_dist;
      }
   }

   return ret;
}

inline bool
find_intersects(CPIXEL_list& pts, Bface_list& faces)
{
   for (int i = 0; i < pts.num(); i++) {
      Bface* f = VisRefImage::Intersect(pts[i]);
      if (!f) return false;
      faces += f;
   }
   return true;
}

bool
PROFILE::try_extend_boundary(CPIXEL_list& pts)
{
   err_adv(debug, "PROFILE::try_extend_boundary:");
   if (pts.empty()) return false;

   double dist;
   int loc = find_nearest_edge(_region_boundary, pts[0], dist);
   if (dist > SIL_SEARCH_RAD) {
      err_adv(debug, "   too far from boundary edges");
      return false;
   }
   
   Bface_list faces;
   if (!find_intersects(pts, faces)) {
      err_adv(debug, "   part of the trail has no intersection");
      return false;
   }
   faces = faces.unique_elements();
   if (faces.num()>1) faces -= faces[0]; // allow some gesture inaccuracy

   int n = faces.num();
   Bedge* e = _region_boundary.edge(loc);
   Bface_list neibors1 = n_next_quads(e, n, false, false);
   Bface_list neibors2 = n_next_quads(e, n, true, false);
   if (!neibors1.contains_all(faces) && !neibors2.contains_all(faces)) {
      err_adv(debug, "   trail should be in the quad chain of the bound");
      return false;
   }

   int i1 = neibors1.get_index(faces.last()), 
      i2 = neibors2.get_index(faces.last());
   if (i1!=-1) i1 = i1/2 + 1;
   if (i2!=-1) i2 = i2/2 + 1;
   bool side = _boundary_side[loc];
   if (side && test_bound_dec(i1, side))
      _a -= i1;
   else if (side && test_bound_inc(i2))
      _a += i2;
   else if (!side && test_bound_inc(i1))
      _b += i1;
   else if (!side && test_bound_dec(i2, side))
      _b -= i2;
   select_faces();
   
   return true;
}

inline void
show_yardstick(CBvert_list& verts, double yardstick)
{
   ARRAY<Wline> lines(verts.num());
   for (int i=0; i<verts.num(); i++)
      lines += Wline(verts[i]->loc(), verts[i]->norm()*yardstick);
   GL_VIEW::draw_lines(lines, Color::yellow, 0.8, 1, false);
}

int 
PROFILE::draw(CVIEWptr& v)
{ 
   if (_selected_edges.empty())
      return 0;

   glDisable(GL_DEPTH_TEST);
   draw_strip(_selected_edges, 8, Color::blue_pencil_d, 0.8);
   draw_strip(_region_boundary, 5, Color::red_pencil, 0.8);

   show_yardstick(_selected_edges.edges().get_verts(),
                  compute_yardstick(_selected_edges.edges()));
   return 0;
}

bool
PROFILE::test_bound_inc(int inc)
{
   if (inc<0) return false;
   if (_selected_edges.empty()) return false;
      
   Bsurface_list surfs = Primitive::get_primitives(
      get_top_level(_selected_edges.edges().get_faces())
      );
   for (int i = 0; i < surfs.num(); i++) {
      Primitive* p = (Primitive*)surfs[i];
      int n = (1<<p->rel_cur_level()) + _n;
      if (!_mode && ((n+_a) + (n+_b) + inc) > (4<<p->rel_cur_level()))
         return false;
   }

   return true;
}

bool
PROFILE::test_bound_dec(int dec, bool side)
{
   if (dec<0) return false;
   if (_selected_edges.empty()) return false;
      
   Bsurface_list surfs = Primitive::get_primitives(
      get_top_level(_selected_edges.edges().get_faces())
      );
   for (int i = 0; i < surfs.num(); i++) {
      Primitive* p = (Primitive*)surfs[i];
      int n = (1<<p->rel_cur_level()) + _n;
      if ((side&&(n+_a-dec)<0) || (!side&&(n+_b-dec)<0))
         return false;
   }

   return true;
}

void
PROFILE::inc_aoi()
{
   err_adv(debug, "   increase of aoi");
   if (_selected_edges.empty()) return;
   
   Bsurface_list surfs = Primitive::get_primitives(
      get_top_level(_selected_edges.edges().get_faces())
      );
   for (int i = 0; i < surfs.num(); i++) {
      Primitive* p = (Primitive*)surfs[i];
      int n = (1<<p->rel_cur_level()) + _n;
      if (!_mode && ((n+_a) + (n+_b)) >= (4<<p->rel_cur_level()))
         return;
   }

   _n++;
   select_faces();
}

void
PROFILE::dec_aoi()
{
   err_adv(debug, "   decrease of aoi");
   if (_selected_edges.empty()) return;

   Bsurface_list surfs = Primitive::get_primitives(
      get_top_level(_selected_edges.edges().get_faces())
      );
   for (int i = 0; i < surfs.num(); i++) {
      Primitive* p = (Primitive*)surfs[i];
      int n = (1<<p->rel_cur_level()) + _n;
      if ((n+_a)<=0 || (n+_b)<=0)
         return;
   }

   _n--;
   select_faces();
}

// end of file profile.C
