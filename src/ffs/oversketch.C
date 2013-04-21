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
 *  \file oversketch.C
 *  \brief Contains the definition of the OVERSKETCH widget.
 *
 *  \ingroup group_FFS
 *  \sa oversketch.H
 *
 */

#include "disp/colors.H"                // Color::blue_pencil_d
#include "gtex/util.H"                  // for draw_strip
#include "gtex/ref_image.H"             // for VisRefImage
#include "gtex/basic_texture.H"         // GLStripCB
#include "tess/mesh_op.H"               // FIT_VERTS_CMD
#include "tess/primitive.H"

#include "oversketch.H"

static bool debug = Config::get_var_bool("DEBUG_OVERSKETCH",false);

using namespace mlib;
using namespace GtexUtil;

/*****************************************************************
 * OVERSKETCH
 *****************************************************************/
OVERSKETCHptr OVERSKETCH::_instance;

OVERSKETCH::OVERSKETCH() :
   DrawWidget()
{
   _draw_start += DrawArc(new TapGuard,    drawCB(&OVERSKETCH::cancel_cb));
   _draw_start += DrawArc(new StrokeGuard, drawCB(&OVERSKETCH::stroke_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);
}

void 
OVERSKETCH::clean_on_exit() 
{ 
   _instance = 0; 
}

OVERSKETCHptr
OVERSKETCH::get_instance()
{
   if (!_instance)
      _instance = new OVERSKETCH();
   return _instance;
}

bool 
OVERSKETCH::init(CGESTUREptr& g)
{
   if (!(g && g->is_stroke()))
      return false;

   if (g->below_min_length() || g->below_min_spread())
      return false;

   if (get_instance()->find_matching_sil(g))
      return true;

   // add more cases here...

   return false;
}

const double SIL_SEARCH_RAD = 12;

bool
OVERSKETCH::find_matching_sil(CGESTUREptr& g)
{
   err_adv(debug, "OVERSKETCH::find_matching_sil");

   const int MIN_GEST_PTS = 10;
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
   //    the gesture, store in _selected_sils

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

bool
OVERSKETCH::find_matching_sil(CPIXEL_list& pts, CEdgeStrip& sils)
{
   err_adv(debug, "   checking %d edges", sils.num());
   _selected_sils.reset();
   for (int i=0; i<sils.num(); i++)
      if (do_match(pts, sils.edge(i)))
         _selected_sils.add(sils.vert(i), sils.edge(i));

   _mesh = LMESH::upcast(_selected_sils.mesh());
   if (_mesh) {
      int n = _selected_sils.num_line_strips();
      _selected_sils = _selected_sils.get_unified();
      err_adv(debug, "   found %d edges, (%d pieces, was %d)",
              _selected_sils.num(), _selected_sils.num_line_strips(), n);
      activate();
      select_faces();
      check_primitive();
      return true;
   }
   _selected_sils.reset();
   err_adv(debug, "   failed");
   return false;
}

inline Bface_list
ctrl_faces(CBedge_list& edges)
{
   if (!LMESH::isa(edges.mesh()))
      return Bface_list();
   Bface_list ret(edges.num());
   for (int i=0; i<edges.num(); i++) {
      Bface* f = ((Ledge*)edges[i])->ctrl_face();
      if (f)
         ret += f;
   }
   return ret.unique_elements().quad_complete_faces();
}
void
OVERSKETCH::select_faces()
{
   if (_selected_sils.empty()) {
      err_adv(debug, "OVERSKETCH::select_faces: no selected sils");
      return;
   }
   LMESH* m = LMESH::upcast(_selected_sils.mesh());
   assert(m && _selected_sils.same_mesh());
   MeshGlobal::deselect_all();
//   MeshGlobal::select(ctrl_faces(_selected_sils.edges()));
   MeshGlobal::select(_selected_sils.edges().get_faces().n_ring_faces(3));
}

void
OVERSKETCH::reset()
{
   _selected_sils.reset();
   _selected_region.clear();
   _mesh = 0;
}

int  
OVERSKETCH::cancel_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "OVERSKETCH::cancel_cb");

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we did use up the gesture
}

int  
OVERSKETCH::tap_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "OVERSKETCH::tap_cb");

   return cancel_cb(gest, s);
}

int  
OVERSKETCH::stroke_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_adv(debug, "OVERSKETCH::stroke_cb");
   assert(gest);

   try_oversketch(gest->pts());

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

inline void
make_strip(Bvert_list chain, int k0, int k1, EdgeStrip& strip)
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
}

bool
OVERSKETCH::match_substrip(CPIXEL_list& pts, CBvert_list& chain, EdgeStrip& strip)
{
   err_adv(debug, "OVERSKETCH::match_substrip:");
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

   make_strip(chain, k0, k1, strip);

   return true;
}

bool
OVERSKETCH::get_sil_substrip(CPIXEL_list& pts, EdgeStrip& strip)
{
   Bvert_list chain;
   for (int k=0; _selected_sils.get_chain(k, chain); ) 
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
   // the following is just a hack that needs improvement,
   // but can prevent some bad cases "for now"
   if (!(pts.intersects_seg(PIXELline(p, n)) ||
         pts.intersects_seg(PIXELline(p, n*(-0.25)))))
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
min_dist(CWpt_list& pts, CWpt_list& path)
{
   if (pts.empty())
      return 0;
   double ret = path.dist(pts[0]);
   for (int i=1; i<pts.num(); i++)
      ret = min(ret, path.dist(pts[i]));
   return ret;
}

inline double
swell_profile(double x)
{
   // x should be between 0 and 1:
   //   0: center of region
   //   1: outer boundary of region

//   return sqrt(max(1 - sqr(x), 0.0));

   x = min(fabs(x), 1.0);
   return cos(x*(M_PI/2));
}

bool
OVERSKETCH::apply_offsets(CBvert_list& sil_verts, CARRAY<double>& sil_offsets)
{
   // XXX - preliminary...

   assert(sil_verts.num() == sil_offsets.num());

   // Expand region around oversketched silhouette verts.
   // XXX - Should compute the one-ring size, not use "3"
   Bface_list region = sil_verts.one_ring_faces().n_ring_faces(3);

   // Find the minimum distance to the silhouette verts from the
   // outer boundary of the region
   Wpt_list sil_path = sil_verts.pts();
   double R = min_dist(region.get_boundary().verts().pts(), sil_path);

   Bvert_list region_verts = region.get_verts();
   Wpt_list   new_locs     = region_verts.pts();
   ARRAY<double> offsets(region_verts.num());
   for (int i=0; i<region_verts.num(); i++) {
      Wpt foo;
      int k = -1;
      double d = sil_path.closest(region_verts[i]->loc(), foo, k);
      if (!sil_offsets.valid_index(k)) {
         err_adv(debug, "OVERSKETCH::apply_offsets: error: can't find closest");
         continue;
      }
      double s = swell_profile(d/R);
      double h = sil_offsets[k] * s;
//      err_adv(debug, "  d: %f, d/R: %f, s: %f", d, d/R, s);
      offsets += h;
      new_locs[i] += region_verts[i]->norm()*h;
//       WORLD::show(region_verts[i]->loc(), new_locs[i], 1);
   }

   // now apply new locs
//   FIT_VERTS_CMDptr cmd = new FIT_VERTS_CMD(region_verts, new_locs);
   SUBDIV_OFFSET_CMDptr cmd = new SUBDIV_OFFSET_CMD(region_verts, offsets);
   cmd->doit();
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
OVERSKETCH::compute_offsets(CPIXEL_list& pts, CEdgeStrip& sils)
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
      if (offsets.last() > 0) {
         count++;
      }
   }
   err_adv(debug, "found %d/%d offsets", count, chain.num());

   apply_offsets(chain, offsets);

   return true;
}

bool
OVERSKETCH::check_primitive()
{
   err_adv(debug, "OVERSKETCH::check_primitive:");

   Bsurface_list surfs = Primitive::get_primitives(
      get_top_level(_selected_sils.edges().get_faces())
      );

   err_adv(debug, "found %d primitives along oversketch", surfs.num());

   return true;
}

bool
OVERSKETCH::try_oversketch(CPIXEL_list& pts)
{
   err_adv(debug, "OVERSKETCH::try_oversketch:");

   // find best-matching subset of existing selected sils

   // if none, reject

   if (_selected_sils.empty()) {
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
      err_adv(debug, "  compute offsets found");
      return false;
   }

   // propagate to surrounding region
   
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
OVERSKETCH::draw(CVIEWptr& v)
{
   if (_selected_sils.empty())
      return 0;

   draw_strip(_selected_sils, 8, Color::blue_pencil_d, 0.8);

   if (debug)
      show_yardstick(_selected_sils.edges().get_verts(),
                     compute_yardstick(_selected_sils.edges()));
   return 0;
}

// end of file oversketch.C
