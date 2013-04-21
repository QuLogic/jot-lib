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
 *  \file draw_pen.C
 *  \brief Contains the definition of the DrawPen Pen, plus helpers.
 *  Also: WidgetGuard,
 *
 *  \ingroup group_FFS
 *  \sa draw_pen.H
 *
 */

#include "disp/ray.H"
#include "gtex/ref_image.H"
#include "gtex/solid_color.H"
#include "gtex/control_line.H"
#include "manip/mmenu.H"
#include "map3d/map1d3d.H"
#include "mesh/lmesh.H"
#include "mesh/mi.H"
#include "mesh/mesh_select_cmd.H"

#include "mlib/points.H"
#include "mlib/polyline.H"

#include "tess/action.H"
#include "std/config.H"
#include "std/time.H"
#include "tess/panel.H"
#include "tess/primitive.H"
#include "tess/uv_surface.H"
#include "tess/skin.H"
#include "tess/skin_meme.H"
#include "tess/ti.H"

#include "ffs/circle_widget.H"
#include "ffs/crv_sketch.H"
#include "ffs/cursor3d.H"
#include "ffs/extender.H"
#include "ffs/ffs_util.H"
#include "ffs/floor.H"
#include "ffs/inflate.H"
//#include "ffs/oversketch.H"
#include "ffs/profile.H"
#include "ffs/roof.H"
#include "ffs/paper_doll.H"
#include "ffs/crease_widget.H"
#include "ffs/select_widget.H"
#include "ffs/sweep.H"

#include "draw_pen.H"

using namespace tess;
using namespace FFS;

/*****************************************************************
 * convenient inline methods
 *****************************************************************/
inline FLOOR*
hit_floor(CPIXEL& x)
{
   BMESHray ray(x);
   VIEW::peek()->intersect(ray);
   return ray_geom(ray, FLOOR::null);
}

inline bool
get_hits(CPIXEL& x, Bsurface*& s, Bcurve*& c, Bpoint*& p, FLOOR*& f)
{
   // Do a bunch of intersection calls to find what's at screen point x

   s = Bsurface::hit_ctrl_surface(x);
   c = Bcurve::  hit_ctrl_curve(x);
   p = Bpoint::  hit_ctrl_point(x);
   f = hit_floor(x);

   return (s || c || p || f);
}

inline bool
connectivity_edit()
{
   // When the global rendering style is ControlLineTexture,
   // editing of mesh connectivity is considered to be enabled.
   
   return (VIEW::peek()->rendering() == ControlLineTexture::static_name());
}
/*****************************************************************
 * WidgetGuard:
 *****************************************************************/
//! \brief Accepts a GESTURE if a suitable widget can be found
//! that wants to handle it.
class WidgetGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) {

      // See if some widget wants the GESTURE

      // First try for the active one:
      DrawWidgetptr widget = DrawWidget::get_active_instance();
      if (widget && widget->handle_gesture(g))
         return 1;

      // Try various widgets to see if any of them wants to handle
      // this gesture. When a widget agrees to handle the gesture,
      // typically it becomes the active widget and handles the next
      // sequence of gestures until the given operation is complete.

      if (EXTENDER     ::init(g))       return 1;
      if (ROOF         ::init(g))       return 1;
      if (CREASE_WIDGET::init(g))       return 1;
      if (SELECT_WIDGET::init(g))       return 1;
      if (CIRCLE_WIDGET::init(g))       return 1;
      if (PAPER_DOLL   ::init(g))       return 1;
      if (SWEEP_LINE   ::init(g))       return 1;
      if (SWEEP_DISK   ::init(g))       return 1;
//      if (OVERSKETCH   ::init(g))       return 1;
      if (PROFILE      ::init(g))       return 1;
      if (CRV_SKETCH   ::init(g))       return 1;

      return 0;
   }
};

/*****************************************************************
 * DrawPen
 *****************************************************************/
DrawPen* DrawPen::_instance = 0;

DrawPen::DrawPen(
   CGEST_INTptr &gest_int,
   CEvent&       d,
   CEvent&       m,
   CEvent&       u
   ) :
   Pen(str_ptr("draw"), gest_int, d, m, u),
   _mode(DEFAULT),
   _drag_pt(0),
   _tap_timer(0),
   _tap_callback(false)
{
   // Set up Event FSA:
   // XXX - obsolete:
   _dragging += Arc(m, Cb((_callb::_method) &DrawPen::drag_move_cb));
   _dragging += Arc(u, Cb((_callb::_method) &DrawPen::drag_up_cb,(State*)-1));

   // Set up GESTURE FSA:
   //   Note: the order matters.
   //   First matched will be executed.

   // catch garbage strokes early and reject them
   _draw_start += DrawArc(new GarbageGuard,  drawCB(&DrawPen::garbage_cb));

   // defer to any widgets that want to handle the GESTURE:
   _draw_start += DrawArc(new WidgetGuard,   drawCB(&DrawPen::null_cb));
   _draw_start += DrawArc(new SlashTapGuard, drawCB(&DrawPen::slash_tap_cb));
   _draw_start += DrawArc(new TapGuard,      drawCB(&DrawPen::tap_cb));
   _draw_start += DrawArc(new XGuard,        drawCB(&DrawPen::x_cb));
   _draw_start += DrawArc(new DotGuard,      drawCB(&DrawPen::dot_cb));
   _draw_start += DrawArc(new ScribbleGuard, drawCB(&DrawPen::scribble_cb));
   _draw_start += DrawArc(new CircleGuard,   drawCB(&DrawPen::circle_cb));
   _draw_start += DrawArc(new EllipseGuard,  drawCB(&DrawPen::ellipse_cb));
   _draw_start += DrawArc(new LineGuard,     drawCB(&DrawPen::line_cb));
   _draw_start += DrawArc(new SlashGuard,    drawCB(&DrawPen::slash_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&DrawPen::stroke_cb));

   // Sign up for CAMobs callbacks:
   _view->cam()->data()->add_cb(this);

   // Sign up for FRAMEobs::tick() callbacks:
   _view->schedule(this);

   if (_instance)
      err_msg("DrawPen::DrawPen: Error: instance already exists");
   _instance = this;
}

// ***************** Method from FRAMEobs. Get called per frame
int 
DrawPen::tick(void)
{
   // deal w/ delayed tap:
   // XXX - not sure we want this policy:
   if (_tap_callback && _tap_timer.expired() ){
      _tap_callback = false;
      tapat(_tap_callback_loc);
   }

   // Active Bnodes get a callback each frame 
   // for animation, simulation, relaxation, etc.
   Bnode::apply_frame_cb();

   return 0;
}

//***************** Method from CAMobs. Get called when camera change
void 
DrawPen::notify(CCAMdataptr&) 
{ 
   Bbase::hold();
}

void 
DrawPen::AddToSelection(Bbase* c)
{
   if (!c) {
      err_msg("DrawPen::AddToSelection: Bbase is null");
      return;
   }
   _mode = SELECTED;
   c->set_selected();
}

//! Clean selection list, unselect everything, turn off all
//! helpers, set mode to DEFAULT
void 
DrawPen::ModeReset(MULTI_CMDptr cmd)
{
   if (cmd)
      cmd->add(new MESH_DESELECT_ALL_CMD());
   else {
      if (!MeshGlobal::selected_edges().empty() ||
         !MeshGlobal::selected_faces().empty() ||
         !MeshGlobal::selected_verts().empty())
         WORLD::add_command(new MESH_DESELECT_ALL_CMD());
   }
   Bbase     ::deselect_all();
   _mode = DEFAULT;
}

int
DrawPen::garbage_cb(CGESTUREptr&, DrawState*&)
{
   WORLD::message("Stroke was too fast -- rejected");
   return 1;
}

int
DrawPen::null_cb(CGESTUREptr&, DrawState*&)
{
   // do nothing
   return 1;
}

inline int
debug_win_pick(CPIXEL& p)
{
   // trying to compare the known ID if a triangle with
   // what the vis ref image says it is.
   // to use this set 2 variables to true:
   //   FORCE_FACE_PICK
   //   DEBUG_WIN_PICK
   // then load up  models/surfaces/tri.sm,
   // hide the floor, get into draw pen, and
   // click/release (tap) on the triangle.

   BMESHray ray(p);
   VIEW::peek()->intersect(ray);

   Bsimplex* s = ray.simplex();

   if (!s) {
      err_msg("no hit");
      return 0;
   }
   VisRefImage* vis = VisRefImage::lookup();
   assert(vis);
   err_msg("simplex key %x, rgba %x", s->key(), vis->val(p));
   return 0;
}

int
DrawPen::tap_cb(CGESTUREptr& tap, DrawState*& s)
{
   assert(tap);
   tapat(tap->center());
   return 0;

   _tap_timer.reset(Config::get_var_dbl("JOT_TAP_DELAY", .4, true));
   _tap_callback = true;
   _tap_callback_loc = tap->center();

   return 0;
}

// inline bool
// clear_easel_zone(CPIXEL& pt)
// {
//    // A tap near the bottom of the window clears the easel

//    const double YMAX=20;
//    return pt[1] < YMAX;
// }

void
DrawPen::tapat(PIXEL x)
{
   Bface* f;
   if (!(f = VisRefImage::get_edit_face()) || !f->is_selected()) {
      // Cancel all current selections
      ModeReset(0); 
   }

   Bpoint*   bpt      = 0;
   Bcurve*   bcurve   = 0;
   Bsurface* bsurface = 0;

   bool debug = Config::get_var_bool("DEBUG_TAP",false);
   err_adv(debug, "DrawPen::tapat (%f,%f)", x[0], x[1]);

//    if (clear_easel_zone(x)) {
//       if (cur_easel()) {
//          err_adv(debug, "clear easel");
//          cur_easel()->clear_lines();
//       }
//    } else 
   if ((bpt = Bpoint::hit_ctrl_point(x))) {
      AddToSelection(bpt);
      err_adv(debug, "select bpoint");
   } else if ((bcurve = Bcurve::hit_ctrl_curve(x))) {
      if (Wpt_listMap::isa(bcurve->map()))
         err_adv(debug, "bcurve map is wpt_list map");
      AddToSelection(bcurve);
      err_adv(debug, "select bcurve");
   } else if ((bsurface = Bsurface::hit_ctrl_surface(x))) {
      if (UVsurface::isa(bsurface))
         err_adv(debug, "bsurface is a uvsurface");
      if (Skin::isa(bsurface))
         err_adv(debug, "bsurface is a skin");
      AddToSelection(bsurface);
      err_adv(debug, "select bsurface");
   } else {
      err_adv(debug, "select mesh");
      BMESH* m = VisRefImage::get_mesh();
      if (m == FLOOR::lookup_mesh()) {
         // don't focus on the floor
         BMESH::set_focus(0,0);
      } else {
         // focus on mesh and patch according to user's click:
         BMESH::set_focus(m,VisRefImage::get_patch());
      }
      
   }
}

/*****************************************************************
 * Utilities
 *****************************************************************/

inline VertMapper
subdiv_mapper(CVertMapper& pmap)
{
   Bedge_list a_edges = pmap.a_edges();
   Bedge_list b_edges = pmap.a_to_b(a_edges);
   assert(a_edges.num() == b_edges.num());

   return VertMapper(
      child_verts<Bvert_list,Lvert>(pmap.A()) +
      child_verts<Bedge_list,Ledge>(a_edges),
      child_verts<Bvert_list,Lvert>(pmap.B()) +
      child_verts<Bedge_list,Ledge>(b_edges)
      );
}

inline Bvert_list
parent_verts(CBvert_list& verts, int lev)
{
   Bvert_list ret;
   if (lev == 0) {
      ret = verts;
   } else {
      Bvert_list parents;
      for (int i = 0; i < verts.num(); i++) {
         Lvert* p = ((Lvert*)verts[i])->parent_vert(1);
         if (p) parents += p;
      }
      ret = parent_verts(parents, lev-1);
   }
   return ret;
}

inline Bface_list
subdiv_mapper(VertMapper* m, CBface_list& region)
{
   int m_lev = m->A().mesh()->subdiv_level();
   int r_lev = region.mesh()->subdiv_level();
   int lev = m_lev - r_lev;
   VertMapper pmap(parent_verts(m->A(), lev), parent_verts(m->B(), lev));
   return pmap.a_to_b(region);
}

inline Bface*
subdiv_mapper(VertMapper* m, Bface* f)
{
   int m_lev = m->A().mesh()->subdiv_level();
   int f_lev = f->mesh()->subdiv_level();
   int lev = m_lev - f_lev;
   VertMapper pmap(parent_verts(m->A(), lev), parent_verts(m->B(), lev));
   return pmap.a_to_b(f);
}

inline EdgeStrip
subdiv_mapper(VertMapper* m, CEdgeStrip& strip)
{
   int m_lev = m->A().mesh()->subdiv_level();
   int s_lev = strip.mesh()->subdiv_level();
   int lev = m_lev - s_lev;
   VertMapper pmap(parent_verts(m->A(), lev), parent_verts(m->B(), lev));
   return pmap.a_to_b(strip);
}

inline ARRAY<Bface_list> 
get_regions(CBface_list& cregion)
{
   bool debug = false;

   ARRAY<Bface_list> ret;
   Bface_list region = cregion;

   while(!region.empty()) {
      region.get_verts().clear_flag02();
      region.set_flags(1);
      Bface_list comp(region.num());
      comp.grow_connected(region.first(), 
                          !(BcurveFilter() || UncrossableEdgeFilter()));
      ret += comp;
      region -= comp;
   }

   if (debug) cerr << "num of regions:" << ret.num() << endl;

   return ret;
}

inline int
shift(Bvert_list& b_verts)
{
   bool debug = false;

   int shift_loc;
   for (shift_loc = 0; shift_loc < b_verts.num(); shift_loc++) {
      bool a = Bpoint::find_controller(b_verts[shift_loc]) || 
          Bcurve::find_controller(b_verts[shift_loc]);
      bool b = b_verts[shift_loc]->degree(ProblemEdgeFilter())>0;//b_verts[shift_loc]->is_crease();
      if (a || b)
         break;
   }
   if (shift_loc != b_verts.num()) {
      b_verts.pop();
      b_verts.shift(-shift_loc);
      b_verts += b_verts.first();
   }

   if (debug) cerr << "the shift loc is:" << shift_loc << endl;

   return shift_loc;
}

inline ARRAY<Bvert_list>
get_c_verts(Bvert_list& b_verts)
{
   bool debug = false;

   ARRAY<Bvert_list> ret;
   int shift_loc = shift(b_verts);
   Bvert_list curve_verts;
   bool recording = false;

   for (int j = 0; j < b_verts.num(); j++) {
      bool a = Bpoint::find_controller(b_verts[j]) || Bcurve::find_controller(b_verts[j]);
      bool b = b_verts[j]->degree(ProblemEdgeFilter())>0;//b_verts[j]->is_crease();
      if (a || b) {
         if (recording) {
            curve_verts += b_verts[j];
            recording = false;
            ret += curve_verts;
         }
      } else {
         if (!recording && shift_loc != b_verts.num()) {
            curve_verts.clear();
            curve_verts += b_verts[j-1];
            recording = true;
         }
         curve_verts += b_verts[j];
      }
   }

   if (shift_loc == b_verts.num()) {
      ret += curve_verts;
   }

   if (debug) cerr << "curve no: " << ret.num() << endl;

   return ret;
}

inline Skin*
get_skin(CARRAY<Skin*>& skins, Bface_list& region)
{
   for (int i = 0; i < skins.num(); i++) {
      if (skins[i]->skel_faces().contains_all(region))
         return skins[i];
   }
   return NULL;
}

// given a region on the skeleton of a skin, find the corresponding faces
// on the skin
inline Bface_list
map_skel_to_skin(Skin* skin, CBface_list& region) {
   bool debug = false;

   Bface_list ret;
   Bface_list skin_faces = skin->skin_faces();

   for (int i = 0; i < skin_faces.num(); i++) {
      for (int j = 0; j < region.num(); j++) {
         Bvert* v1 = skin->get_mapper()->a_to_b(region[j]->v1());
         Bvert* v2 = skin->get_mapper()->a_to_b(region[j]->v2());
         Bvert* v3 = skin->get_mapper()->a_to_b(region[j]->v3());
         Bvert* v4 = skin->get_mapper()->a_to_b(region[j]->quad_vert());

         int count = (skin_faces[i]->contains(v1) || skin_faces[i]->contains(region[j]->v1()));
         count += (skin_faces[i]->contains(v2) || skin_faces[i]->contains(region[j]->v2()));
         count += (skin_faces[i]->contains(v3) || skin_faces[i]->contains(region[j]->v3()));
         count += (skin_faces[i]->contains(v4) || skin_faces[i]->contains(region[j]->quad_vert()));
         if (count == 3) {
            ret += skin_faces[i];
            if (skin_faces[i]->is_quad()) ret += skin_faces[i]->quad_partner();
            break;
         }         
      }
   }

   ret = ret.unique_elements();
   if (ret.num() != region.num()) {
      if (debug) cerr << "face map failure: " << ret.num() << endl;    
      return Bface_list();
   }
   return ret;
}

inline EdgeStrip
map_skel_to_skin(Skin* skin, CEdgeStrip& strip) {
   bool debug = false;

   EdgeStrip ret;
   Bedge_list skin_edges = skin->skin_edges();
   for (int j = 0; j < strip.num(); j++) {
      for (int i = 0; i < skin_edges.num(); i++) {
         Bvert* v1 = skin->get_mapper()->a_to_b(strip.vert(j));
         Bvert* v2 = skin->get_mapper()->a_to_b(strip.next_vert(j));

         if ((skin_edges[i]->contains(v1) || skin_edges[i]->contains(strip.vert(j))) &&
            (skin_edges[i]->contains(v2) || skin_edges[i]->contains(strip.next_vert(j)))) {
            if (skin_edges[i]->contains(v1))
               ret.add(v1, skin_edges[i]);
            else
               ret.add(strip.vert(j), skin_edges[i]);
            break;
         }
      }
   }

   if (ret.num() != strip.num()) {
      if (debug) cerr << "edgestrip map failure: " << ret.num() << endl;
      return EdgeStrip();
   }
   return ret;
}

// given a region on a skin, find the corresponding faces
// on the skeleton
inline Bface_list
map_skin_to_skel(Skin* skin, CBface_list& region) {
   bool debug = false;

   Bface_list ret;
   Bface_list skel_faces = skin->skel_faces();

   for (int i = 0; i < region.num(); i++) {
      for (int j = 0; j < skel_faces.num(); j++) {
         Bvert* v1 = skin->get_mapper()->a_to_b(skel_faces[j]->v1());
         Bvert* v2 = skin->get_mapper()->a_to_b(skel_faces[j]->v2());
         Bvert* v3 = skin->get_mapper()->a_to_b(skel_faces[j]->v3());
         Bvert* v4 = skin->get_mapper()->a_to_b(skel_faces[j]->quad_vert());

         int count = (region[i]->contains(v1) || region[i]->contains(skel_faces[j]->v1()));
         count += (region[i]->contains(v2) || region[i]->contains(skel_faces[j]->v2()));
         count += (region[i]->contains(v3) || region[i]->contains(skel_faces[j]->v3()));
         count += (region[i]->contains(v4) || region[i]->contains(skel_faces[j]->quad_vert()));
         if (count == 3) {
            ret += skel_faces[j];
            if (skel_faces[j]->is_quad()) ret += skel_faces[j]->quad_partner();
            break;
         }
      }
   }

   ret = ret.unique_elements();
   if (ret.num() != region.num()) {
      if (debug) cerr << "map_skin_to_skel failure: " << ret.num() << endl;
      return Bface_list();
   }
   return ret;
}

// given an edgestrip on a skin, find the corresponding strip
// on the skeleton
inline EdgeStrip
map_skin_to_skel(Skin* skin, CEdgeStrip& strip) {
   bool debug = false;

   EdgeStrip ret;
   Bedge_list skel_edges = skin->skel_edges();
   for (int i = 0; i < strip.num(); i++) {
      for (int j = 0; j < skel_edges.num(); j++) {
         Bvert* v1 = skin->get_mapper()->a_to_b(skel_edges[j]->v1());
         Bvert* v2 = skin->get_mapper()->a_to_b(skel_edges[j]->v2());
         Bedge* e = strip.edge(i);
         Bvert* v = strip.vert(i);

         if ((e->contains(v1) || e->contains(skel_edges[j]->v1())) &&
            (e->contains(v2) || e->contains(skel_edges[j]->v2()))) {
            if (v == v1 || v == skel_edges[j]->v1())
               ret.add(skel_edges[j]->v1(), skel_edges[j]);
            else
               ret.add(skel_edges[j]->v2(), skel_edges[j]);
            break;
         }
      }
   }

   if (ret.num() != strip.num()) {
      if (debug) cerr << "edgestrip map_skin_to_skel failure: " << ret.num() << endl;
      return EdgeStrip();
   }
   return ret;
}

// to extend a strip on a boundary to include all the edges
// on the edge, ccw is true when the edges are couter-clockwisely
// oriented, otherwise false
inline EdgeStrip
get_ext_strip(EdgeStrip s, bool ccw)
{
   bool debug = false;
   EdgeStrip ret;
   Bedge* first_e = s.edge(0);
   Bedge* temp_e = first_e;
   Bvert* first_v = s.vert(0);
   Bvert* temp_v = first_v;
   int count = 0; // just an insurance
   do {
      ret.add(temp_v, temp_e);
      temp_v = temp_e->other_vertex(temp_v);
      Bedge_list star = temp_v->get_manifold_edges();
      for (int i = 0; i < star.num(); i++) {
         Bface* a = star[i]->ccw_face(temp_v);
         Bface* b = star[i]->cw_face(temp_v);
         if ((ccw && (!a||a->is_secondary()) && (b&&b->is_primary())) ||
            (!ccw && (!b||b->is_secondary()) && (a&&a->is_primary()))) {
            temp_e = star[i];
            break;
         }
      }
      count++; // insurance to avoid infinite loop, theoretically ends though
   } while (temp_e != first_e && count<1000);

   if (debug) cerr << "the new boundary verts no: " << ret.num() << endl;
   ret.reverse();
   return ret;
}

inline Bface_list
find_ribbons(EdgeStrip s)
{
   bool debug = false;
   Bface_list ret;
   for (int i = 0; i < s.num(); i++) {
      Bface* f = s.edge(i)->cw_face(s.vert(i));
      if (!Skin::find_controller(f))
         ret += Bface_list::reachable_faces(f, 
            !(BcurveFilter() || UncrossableEdgeFilter()));
   }

   ret = ret.unique_elements();
   if (debug) cerr << "found " << ret.num() << " faces on the ribbon" << endl;
   return ret;
}

inline int
find_start_loc(EdgeStrip s)
{
   for (int i = 0; i < s.num(); i++) {
      Bface* f = s.edge(i)->cw_face(s.vert(i));
      if (Skin::find_controller(f))
         return i;
   }
   return -1;
}

// punch on the cover of either side of the inflation
inline void
cov_inf_punch(Skin* c_skin, CBface_list& region, MULTI_CMDptr cmd)
{
   bool debug = false;

   Skin* under = Skin::find_controller(c_skin->skel_faces()[0]);
   if (under && under->get_partner()) {
      // the inmost partner skin
      Skin* p_skin = under->get_partner(); 

      while (((Lvert*)(under->get_inf_mapper()->A()[0]))->subdiv_vertex()) { 
         // inf skins are not at the sub level
         // subdivide the mapper to the current sub level
         under->set_inf_mapper(subdiv_mapper(*(under->get_inf_mapper()))); 
         p_skin->set_inf_mapper(subdiv_mapper(*(p_skin->get_inf_mapper())));
      }
      // the corresponding region on p_skin
      Bface_list p_region = subdiv_mapper(under->get_inf_mapper(), map_skin_to_skel(c_skin, region));
      if(debug) cerr << "the corresponding region faces num: " << p_region.num() << endl;

      // to see if the other side has a cover skin
      Skin* c_p_skin = get_skin(p_skin->get_covers(), p_region); 
      if (!c_p_skin) { // if not, create one
         Bface_list skel_faces = 
            subdiv_mapper(under->get_inf_mapper(), c_skin->skel_faces()); // use the inf mapper
         // create a cover skin for the other side
         c_p_skin = Skin::create_cover_skin(skel_faces, cmd); 
      }
      // find the corresponding region on c_p_skin
      p_region = map_skel_to_skin(c_p_skin, p_region); 

      // create curve on the other side
      ARRAY<Bvert_list> templists;
      p_region.get_boundary().get_chains(templists);
      templists = get_c_verts(templists[0]);
      for (int i = 0; i < templists.num(); i++) {
         Bcurve* b_curve = new Bcurve(templists[i], c_p_skin,
                                    (LMESH::upcast(region.mesh()))->subdiv_level());
         cmd->add(new SHOW_BBASE_CMD(b_curve));
      }
      push(p_region, cmd);

      // create ribbon
      EdgeStrip boundary = region.get_boundary();
      int loc = find_start_loc(boundary);
      if (loc == -1) return; // nothing needs to be done
      EdgeStrip outer_strip;
      // find the start for the outer strip of the ribbon
      outer_strip.add(boundary.vert(loc), boundary.edge(loc)); 
      EdgeStrip inner_strip = 
         subdiv_mapper(under->get_inf_mapper(), map_skin_to_skel(c_skin, outer_strip)); // use the inf mapper 
      // find the corresponding strip on the cover
      inner_strip = map_skel_to_skin(c_p_skin, inner_strip); 
      Bface_list ribbon_faces = find_ribbons(boundary);
      Bedge_list ribbon_in_edges = ribbon_faces.interior_edges();
      // knock out the ribbons of the adj holes first
      region.mesh()->remove_faces(ribbon_faces); 
      // remove the interior edges of the ribbon
      region.mesh()->remove_edges(ribbon_in_edges);
      // find the strip for the new merged region on both sides
      EdgeStrip outer_strip_ext = get_ext_strip(outer_strip, true); 
      EdgeStrip inner_strip_ext = get_ext_strip(inner_strip, false); 
      CREATE_RIBBONS_CMDptr rib = new CREATE_RIBBONS_CMD(outer_strip_ext, inner_strip_ext);
      if (rib->doit())
         cmd->add(rib);
      //rib->patch()->faces().reverse_faces(); // to ensure correct orientation

   }
}

// if the region contains 2 sub-connected regins on each side of an inflation
// we need to convert them to be on the same side
inline Bface_list
check_inf_region(Bface_list region)
{
   for (int i = 0; i < region.num(); i++) {
      Skin* ctrl = Skin::find_controller(region[i]);
      if (ctrl && !ctrl->is_inflate() && ctrl->get_partner()) {
         Skin* partner = ctrl->get_partner();
         while (((Lvert*)(ctrl->get_inf_mapper()->A()[0]))->subdiv_vertex()) {
            ctrl->set_inf_mapper(subdiv_mapper(*(ctrl->get_inf_mapper()))); 
            partner->set_inf_mapper(subdiv_mapper(*(partner->get_inf_mapper())));
         }
         region[i] = subdiv_mapper(ctrl->get_inf_mapper(), region[i]);
      }
   }
   return region.unique_elements();
}

static bool
try_punch(CBface_list& region, bool debug)
{
   BMESH* mesh = region.mesh();
   if (!mesh || FLOOR::isa(mesh->geom())) {
      err_adv(debug, "bad mesh");
      return 0;
   }

   // Disallow punching the whole thing -- that's annoying
   if (region.boundary_edges().empty()) {
      err_adv(debug, "try_punch: error: region is maximal (no boundary)");
      return false;
   }
   if (!region.can_push_layer()) {
      err_adv(debug, "try_punch: can't push layer");
      return false;
   } 

   MULTI_CMDptr cmd = new MULTI_CMD; 
   ARRAY<Bface_list> regions = get_regions(check_inf_region(region));
   if (debug) cerr << "   num of regions: " << regions.num() << endl; 
   ARRAY<Bvert_list> b_verts_lists;
   ARRAY<Skin*> skins;

   for (int i = 0; i < regions.num(); i++) {
      regions[i].get_boundary().get_chains(b_verts_lists);

      ARRAY<Bvert_list> curve_verts_lists;
      for (int j = 0; j < b_verts_lists.num(); j++)
         curve_verts_lists += get_c_verts(b_verts_lists[j]);

      Bsurface* owner = Bsurface::find_owner(regions[i][0]);
      if (owner && owner->bfaces().same_elements(regions[0])) {
         push(regions[i], cmd);
         continue;
      }

      if ((Bsurface::get_surfaces(regions[i]).num() != 0) && 
          (UVsurface::isa(Bsurface::get_surfaces(regions[i])[0]))) {
         UVsurface* surf = (UVsurface*)(Bsurface::get_surfaces(regions[i])[0]);
         
         UVpt_list uvpts;
         UVpt pt;
         for (int j = 0; j < curve_verts_lists.num(); j++) {
            uvpts.clear();
            for (int k = 0; k < curve_verts_lists[j].num(); k++) {
               surf->get_uv(curve_verts_lists[j][k], pt);
               uvpts += pt;
            }
            uvpts.update_length();
            Bcurve* new_curve =
               new Bcurve(curve_verts_lists[j], uvpts, surf->map(),
                          (LMESH::upcast(mesh))->subdiv_level());
            cmd->add(new SHOW_BBASE_CMD(new_curve));
         }

         push(regions[i], cmd);

      } else if ((Bsurface::get_surfaces(regions[i]).num() != 0) &&
         (Skin::isa(Bsurface::get_surfaces(regions[i])[0])) &&
         !((Skin*)Bsurface::get_surfaces(regions[i])[0])->get_partner()) {
         Skin* c_skin = (Skin*)(Bsurface::get_surfaces(regions[i])[0]);

         push(regions[i], cmd);
         cov_inf_punch(c_skin, regions[i], cmd);//punch on the cover of either side of the inflation

         for (int j = 0; j < curve_verts_lists.num(); j++) {
            Bcurve* b_curve =
               new Bcurve(curve_verts_lists[j], c_skin,
                          (LMESH::upcast(mesh))->subdiv_level());
            cmd->add(new SHOW_BBASE_CMD(b_curve));
         }

      } else if (get_skin(skins, regions[i])) {
         cerr << "multiple knockout on the same patch" << endl;
         Skin* c_skin = get_skin(skins, regions[i]);

         push(map_skel_to_skin(c_skin, regions[i]), cmd);
         cov_inf_punch(c_skin, map_skel_to_skin(c_skin, regions[i]), cmd);

         for (int j = 0; j < curve_verts_lists.num(); j++) {
            Bcurve* b_curve =
               new Bcurve(curve_verts_lists[j], c_skin,
                          (LMESH::upcast(mesh))->subdiv_level());
            cmd->add(new SHOW_BBASE_CMD(b_curve));
         }
         
      } else {
         
         Bface_list l_region = Bface_list::reachable_faces(regions[i][0], 
            !(BcurveFilter() || UncrossableEdgeFilter()));
         cerr << "   cover region faces no: " << l_region.num() << endl;
         Skin* c_skin = Skin::create_cover_skin(l_region, cmd);

         if (c_skin) {
            skins += c_skin;

            push(map_skel_to_skin(c_skin, regions[i]), cmd);
            cov_inf_punch(c_skin, map_skel_to_skin(c_skin, regions[i]), cmd);

            for (int j = 0; j < curve_verts_lists.num(); j++) {
               Bcurve* b_curve = new Bcurve(curve_verts_lists[j], c_skin, 
                  (LMESH::upcast(mesh))->subdiv_level());
               cmd->add(new SHOW_BBASE_CMD(b_curve));
            }

         } else {
            err_adv(debug, "try_punch:: can't create skin for subregion");
            return false;
         }
      }
   }

   err_adv(debug, "punching region with %d faces", region.num());

   WORLD::add_command(cmd);

   MeshGlobal::deselect_all();
   Config::set_var_bool("NO_FIX_ORIENTATION",true);

   return true;
}

//! Grow a region around the given face, not crossing any
//! edges accepted by the "impassable" filter, then punch
//! out the region (i.e. make a hole there).
//! 
//! Used below in DrawPen::slash_cb.
static bool
try_punch(Bface* f, CSimplexFilter& impassable, bool debug=false)
{

   if (!(f && f->mesh())) {
      err_adv(debug, "try_punch: no face found");
      return 0;
   }

   if (FLOOR::isa(f->mesh()->geom())) {
      err_adv(debug, "try_punch: no punching the floor");
      return 0;
   }

   // Grow a set of faces from f, but don't cross impassable edges:
   Bface_list region = Bface_list::reachable_faces(f, !impassable);

   assert(!region.empty());     // e.g., it has f

   if (region.interior_edges().any_satisfy(!BorderEdgeFilter() + impassable)) {
      err_adv(debug, "try_punch: error: region has interior uncrossable edges");
      return false;
   }
   return try_punch(region, debug);
}

inline bool
has_sel_edges(BMESH* m)
{
   return !MeshGlobal::selected_edges(m).empty();
}

int
DrawPen::slash_tap_cb(CGESTUREptr& tap, DrawState*&)
{ 
   bool debug = Config::get_var_bool("DEBUG_SLASH_TAP",false);
   err_adv(debug, "DrawPen::slash_tap_cb");

   // A slash_tap is a slash followed by a tap.
   //
   // Interpretation is to "punch out" a region of surface.

   Bface* f = VisRefImage::get_edit_face(tap->center());
   if (!f) {
      err_adv(debug, "DrawPen::slash_tap_cb: no face found");
      return 0;
   }

   BMESH* m = f->mesh();
   assert(f->mesh());

   // punch out the whole set of selected faces for this mesh
   if (f->is_selected() && try_punch(MeshGlobal::selected_faces(m), debug))
      return 0;

   // punch a hole bounded by selected edges:
   if (has_sel_edges(f->mesh()) && try_punch(f, SelectedSimplexFilter(), debug))
      return 0;

   // punch a hole bounded by edges that are either owned by Bcurves,
   // selected, or "uncrossable" (e.g. patch boundary, crease, etc.):
   try_punch(f, BcurveFilter()          ||
             UncrossableEdgeFilter()    ||
             SelectedSimplexFilter(), debug);

   return 0;
}


//! A slash is a quick, short, straight stroke.
//! It is treated as a gestural hint, not a normal stroke.
int
DrawPen::slash_cb(CGESTUREptr& slash, DrawState*&)
{

   bool debug = Config::get_var_bool("DEBUG_SLASH",false);;
   err_adv(debug, "DrawPen::slash_cb");

   return 0;
}

int
DrawPen::dot_cb(CGESTUREptr& gest, DrawState*&)
{
   // Create a Bpoint out there in the world

   XYpt c = gest->center();

   Bpoint* bpt = Bpoint::hit_point(c, 8);      // search rad is 8
   if (bpt) {
      // select point? for now do nothing
      return 0;
   }

   Bedge* be = VisRefImage::get_edge(c, 6);  // search rad is 6
   if (be) {
      Bvert* bv = be->v1();
      if (c.dist(bv->loc()) > c.dist(be->v2()->loc()))
         bv = be->v2();
      // the vert meme gets pinned
      VertMeme* m = Bbase::find_boss_vmeme(bv);
      if (m) m->pin();
   }

   Bcurve* bcurve = Bcurve::hit_ctrl_curve(c, 6); // search rad is 6
   if (bcurve) {
      // create a point on the curve, like a bead on a wire?
      // for now do nothing
      return 0;
   }

   MULTI_CMDptr cmd = new MULTI_CMD;
   LMESHptr mesh    = TEXBODY::get_skel_mesh(cmd);
   int      res_lev = get_res_level(mesh);

   // Use Axis if available:
   Cursor3Dptr ax = Cursor3D::get_active_instance();
   if (ax) {
      // Create a new point on the axis plane
      BpointAction::create(mesh, Wpt(ax->get_plane(), Wline(c)),
                   ax->Y(), ax->Z(), res_lev, cmd);
      WORLD::add_command(cmd);
      WORLD::message("Point created on current plane");
      return 0;
   }

   // Use floor:
   BMESHray ray(c);
   _view->intersect(ray);
   FLOOR* floor = ray_geom(ray, FLOOR::null);
   if (floor) {
      Wplane P = floor->plane();
      BpointAction::create(mesh, Wpt(P, Wline(c)), floor->n(), floor->t(), res_lev, cmd);
      WORLD::add_command(cmd);
      WORLD::message("Point created on the Floor plane");
      return 0;
   }

   // XXX - really just for testing: put a point on a uv surface:
   Bface* f=0;
   Bsurface* bsurf = Bsurface::hit_ctrl_surface(c, 1, &f);
   bool debug = Config::get_var_bool("DEBUG_UV_INTERSECT",false);
   if (0) {
      if (bsurf)
         err_adv(debug, "surface: %s", **bsurf->class_name());
      if (bsurf && f) {
         UVsurface* uvsurf = UVsurface::upcast(bsurf);
         err_adv(debug, "uvsurf: %p, map: %s",
                 uvsurf, uvsurf ? **uvsurf->map()->class_name() : "null");
         if (!(uvsurf && uvsurf->map())) {
            return 0;
         }
         UVpt uv;
         if (!uvsurf->get_uv(get_ctrl_face(f), uv))
            err_adv(debug, "get_uv failed");
         else if (uvsurf->map()->invert_ndc(NDCpt(c), uv, uv, 25)) {
            // XXX - obsolete
            assert(uvsurf->texbody());
            LMESHptr m = uvsurf->texbody()->get_skel_mesh();
            Bpoint* bp = new Bpoint(m, uvsurf->map(), uv);
            WORLD::message("Point created on UV surface");
            cerr << "ndcz: " << NDCZpt(bp->loc()) << endl;
            Wtransf P = VIEW::peek_cam()->ndc_projection();
            cerr << "norm: "
                 << NDCZvec(bp->norm(), P.derivative(bp->loc()))
                 << endl;
            return 0;
         } else
            err_adv(debug, "invert failed");
      }
   }

   if (bsurf)
      return 0;

   // Create a point in mid-air.
   Wpt p = Wpt(gest->center(), VIEW::peek_cam()->data()->center());
   BpointAction::create(mesh, p, Wvec::Y(), Wvec::Z(), res_lev, cmd);
   WORLD::add_command(cmd);
   WORLD::message("Point created in mid-air");

   return 0;
}

int
DrawPen::scribble_cb(CGESTUREptr& gest, DrawState*&)
{
   // XXX - misnomer? scribble here means crossing something out. the
   // action is to delete whatever is under the scribble

   // XXX Should implement this better

   cerr << "DrawPen::scribble_cb" << endl;

   Bsurface* surf_sel  = Bsurface::selected_surface();
   if (_mode==SELECTED && surf_sel) {
      if (panel_op(surf_sel, gest))
         return 0;
   }

   // cross out either the recently tapped object, or the one at the
   // center of the scribble (if nothing was tapped):
   XYpt xy = gest->center();
   if (gest->prev() &&
       gest->prev()->is_tap() &&
       gest->prev()->age() < 3)
      xy = gest->prev()->center();


   // Deactivate selections
   ModeReset(0);

   GEL* gel = 0;
   Bpoint* bpt = Bpoint::hit_point(xy, 8);
   if (bpt) {
      gel = bpt->geom();
   } else {
      Bcurve* bcurve = Bcurve::hit_ctrl_curve(xy, 6); // search rad is 6
      if (bcurve) {
         gel = bcurve->geom();
      } else {
         BMESHray ray(xy);
         _view->intersect(ray);
         // Don't erase floor this way
         if (ray.success() && !ray_geom(ray, FLOOR::null))
            gel = ray.geom();
      }
   }
   WORLD::undisplay(gel, true); // true: make it undoable

   return 0;
}

int
DrawPen::circle_cb(CGESTUREptr& gest, DrawState*& s)
{
   //// Inflate a selected (isolated) point into a ball
   //Bpoint* bp= Bpoint::selected_point();
   //if (bp && bp->vert() && bp->vert()->degree() == 0) {

   //   PIXEL center; VEXEL axis; double r1=0, r2=0;
   //   if (gest->is_ellipse(center, axis, r1, r2))
   //      create_sphere(bp, r1);
   //   else
   //      err_msg("DrawPen::circle_cb: error: gesture is not an ellipse?!");

   //   ModeReset(0); 
   //   return 0;
   //}

   // That didn't work, try something more basic:
   return ellipse_cb(gest, s);
}

int
DrawPen::small_circle_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_msg("DrawPen::small_circle_cb: error, should not be called");

   return 0;
}

/*!
  To get here, circle_cb() may have already been tried ...
  now try the following:

  If it is an ellipse, and there's a plane to project to,
  and the projection is reasonably circular, create a
  circular panel in the plane. Otherwise, do the normal
  stroke callback.
*/
int
DrawPen::ellipse_cb(CGESTUREptr& gest, DrawState*& s)
{

   // Inflate a selected (isolated) point into a ball
   Bpoint* bp= Bpoint::selected_point();
   if (bp && bp->vert() && bp->vert()->degree() == 0) {

      PIXEL center; VEXEL axis; double r1=0, r2=0;
      if (gest->is_ellipse(center, axis, r1, r2))
         create_sphere(bp, r1);
      else
         err_msg("DrawPen::ellipse_cb: error: gesture is not an ellipse?!");

      ModeReset(0); 
      return 0;
   }

   // Get the ellipse description:
   PIXEL center;  // the center
   VEXEL axis;    // the long axis (unit length)
   double r1, r2; // the long and short radii, respectively
   if (!gest->is_ellipse(center, axis, r1, r2)) {
      // Should not happen
      err_msg("DrawPen::ellipse_cb: error: gesture is not an ellipse.");
      return stroke_cb(gest, s);
   }

   // Find an appropriate plane and try to project the
   // ellipse as a circular Panel.

   // Find a plane to project to
   Wplane P = get_draw_plane(center);
   if (!P.is_valid()) {
      // didn't work out this time
      return stroke_cb(gest, s);
   }

   ///////////////////////////////////////////////////////
   //
   //            y1               
   //                    
   //    x0       c ------> x1          
   //                axis              
   //            y0
   //
   //  Project the extreme points of the ellipse to the
   //  plane and check that after projection they are
   //  reasonably equidistant from the center.
   //
   //  I.e., the ellipse should reasonably match the
   //  foreshortened circle as it would actually appear in
   //  the given plane.
   ///////////////////////////////////////////////////////
   VEXEL perp = axis.perpend();
   Wpt x0 = Wpt(P, Wline(XYpt(center - axis*r1)));
   Wpt x1 = Wpt(P, Wline(XYpt(center + axis*r1)));
   Wpt y0 = Wpt(P, Wline(XYpt(center - perp*r2)));
   Wpt y1 = Wpt(P, Wline(XYpt(center + perp*r2)));

   Wpt c = Wpt(P, Wline(XYpt(center)));

   // The two diameters -- we'll check their ratio
   double dx = x0.dist(x1);
   double dy = y0.dist(y1);

   // XXX - Use environment variable for testing phase:
   // XXX - don't make static (see below):
   double MIN_RATIO = Config::get_var_dbl("EPC_RATIO", 0.85,true);

   // Be more lenient when the plane is more foreshortened:
   //
   // XXX - not sure what the right policy is, but for a
   // completely edge-on plane there's certainly no sense in
   // doing a real projection... the following drops the
   // min_ratio lower for more foreshortened planes:
   double cos_theta = VIEW::eye_vec(c) * P.normal();
   double scale = sqrt(fabs(cos_theta)); // XXX - just trying this out
   MIN_RATIO *= scale;  // XXX - this is why it can't be static
   double ratio = min(dx,dy)/max(dx,dy);
   static bool debug = Config::get_var_bool("DEBUG_ELLIPSE",false);
   err_adv(debug, "DrawPen::ellipse_cb: projected ratio: %f, min: %f",
           ratio, MIN_RATIO);
   if (ratio < MIN_RATIO)
      return stroke_cb(gest, s);

   // Make the Panel
   MULTI_CMDptr cmd = new MULTI_CMD;
   PanelAction::create(P, c, dx/2, TEXBODY::get_skel_mesh(cmd), 4, cmd);
   WORLD::add_command(cmd);

   return 1;
}

int
DrawPen::lasso_cb(CGESTUREptr& gest, DrawState*& s)
{
   err_msg("DrawPen::lasso_cb: not implemented");

   return stroke_cb(gest, s);
}

int
DrawPen::line_cb(CGESTUREptr& gest, DrawState*& s)
{
   // Draw a straight line cross-ways to an existing
   // straight Bcurve to create a rectangle:
//    if (create_rect(gest))
//       return 0;

   // for the tablet-challenged, an alternate way to
   // draw a circular panel is: draw a point, tap it,
   // then draw a straight line from it.  result is a
   // circular panel centered at the point, in the
   // plane of the point, with size corresponding to
   // the line stroke.

   bool debug = Config::get_var_bool("DEBUG_LINE_CB",false);
   if (debug) cerr << "DrawPen::line_cb: " << endl;

   Bpoint* bp = 0;
   if (gest && gest->prev() && gest->prev()->is_tap() &&
       gest->prev()->age() < 3 &&
       gest->start().dist(gest->prev()->start()) < 8 &&
       (bp = Bpoint::hit_point(gest->prev()->start(), 10)) &&
       bp->vert() && bp->vert()->degree() == 0) {
      if (debug) cerr << "  line across bpoint..." << endl;

      // XXX - should stroke lie cross-wise to plane normal?

      Wpt    c = bp->loc();             // center of circle
      Wplane P = Wplane(c, bp->norm()); // plane of circle
      if (!P.is_valid()) {
         // this may happen but isn't supposed to.
         err_msg("DrawPen::line_cb: error: can't get plane of Bpoint");
         return stroke_cb(gest, s);
      }

      // Create the circular panel
      if (debug) cerr << "  creating disk..." << endl;
      double rad = world_length(c, gest->length());
      MULTI_CMDptr cmd = new MULTI_CMD;
      PanelAction::create(P, c, rad, TEXBODY::get_skel_mesh(cmd), 4, cmd);
      WORLD::add_command(cmd);

      return 0;
   }

   if (debug) cerr << "  falling back to stroke cb..." << endl;

   return stroke_cb(gest,s);
}

int
DrawPen::x_cb(CGESTUREptr& gest, DrawState*&)
{
   cerr << "x_cb" << endl;

   // for this compound gesture, make sure the first stroke did
   // not trigger some action ... or if it did undo it.
   // XXX - obsolete?
   gest->prev()->undo();

   PIXEL c = interp(gest->center(), gest->prev()->center(), 0.5);

   BMESHray ray(c);
   _view->intersect(ray);

   Cursor3D* ax = Cursor3D::upcast(ray.geom());
   if (ax) {
      cerr << "hit axis" << endl;
      ax->get_x(gest->center());
      return 0;
   }
   ax = new Cursor3D();
   ax->activate();

   Bpoint *bpt = Bpoint::hit_point(c, 10);
   if (bpt) {
      cerr << "hit bpoint" << endl;
      ax->move_to(bpt->o(), bpt->b(), bpt->n());
   } else if (ray.success()) {
      cerr << "hit " << ray.geom()->class_name() << endl;
      ax->move_to(ray.surf(), ray.norm().perpend(), ray.norm());
   } else {
      ax->move_to(Wpt(c, VIEW::peek_cam()->data()->center()));
   }

   return 0;
}

inline bool
check_stroke_length(str_ptr msg, CGESTUREptr& gest, bool debug)
{
   const double MIN_LEN  = Config::get_var_dbl("JOT_MIN_STROKE_LEN", 25.0, true);
   assert(gest && gest->is_stroke());
   if (gest->length() > MIN_LEN)
      return true;

   if (debug)
      cerr << msg << ": stroke too short (" << gest->length() << " pixels)"
           << endl;
   return false;
}

inline bool
check_stroke_duration(str_ptr msg, CGESTUREptr& gest, bool debug)
{
   const double MIN_TIME = Config::get_var_dbl("JOT_MIN_STROKE_TIME", 0.5, true);
   assert(gest && gest->is_stroke());
   if (gest->elapsed_time() > MIN_TIME)
      return true;

   if (debug)
      cerr << msg << ": stroke too quick (" << gest->elapsed_time() << " seconds)"
           << endl;
   return false;
}

bool
DrawPen::panel_op(Bsurface* surf, CGESTUREptr& gest)
{
   static bool debug = Config::get_var_bool("DEBUG_PANEL_OP",false);

   // Disallow strokes that are too short or quick
   if (!(check_stroke_length  ("DrawPen::panel_op", gest, debug) &&
         check_stroke_duration("DrawPen::panel_op", gest, debug)))
      return 0;

   // XXX - Should generalize to find the panel underlying
   //       other surface types (e.g. inflate or skin patch).
   //       For now reject if the surface itself is not a panel:
   Panel* pnl = Panel::upcast(surf);
   if (!pnl) {
      err_adv(debug, "DrawPen::panel_op: no panel");
      return false;
   }

   // oversketch of skeleton
   if (Panel::is_showing_skel()) {
      if (pnl->oversketch(gest->pts()))
         return true;
   }

   // re-tessellation
   if (!pnl->re_tess(gest->pts(), gest->is_scribble())) {
      err_adv(debug, "DrawPen::panel_op: failure when re-tessing panel");
      return false;
   }

   return true;
}

bool
DrawPen::select_skel_curve(Bsurface* surf, CGESTUREptr& gest) 
{
   static bool debug = Config::get_var_bool("DEBUG_SELECT_SKEL_CURVE",false);

   // Disallow strokes that are too short or quick
   if (!(check_stroke_length  ("DrawPen::select_skel_curve", gest, debug) &&
         check_stroke_duration("DrawPen::select_skel_curve", gest, debug)))
      return 0;

   // XXX - Should generalize to find the primitive underlying
   //       other surface types (e.g. inflate or skin patch).
   //       For now reject if the surface itself is not a primitive:
   Primitive* prim = Primitive::upcast(surf);
   if (!prim) {
      err_adv(debug, "DrawPen::select_skel_curve: no primitive");
      return false;
   }

   double err_thresh =
      Config::get_var_dbl("JOT_STROKE_ALIGN_ERR_THRESH", 10, true);
   Bcurve* skel = prim->find_skel_curve(gest->pts(), err_thresh);
   if (!skel) {
      err_adv(debug, "DrawPen::select_skel_curve: no skeleton found");
      return false;
   }

   // cancel current selections:
   ModeReset(0);
   err_adv(debug, "DrawPen::select_skel_curve: selecting skeleton curve");
   AddToSelection(skel);
   
   return true;
}

bool
DrawPen::handle_extrude(Bsurface* surf_sel, CGESTUREptr& gest) 
{
   static bool debug = Config::get_var_bool("DEBUG_EXTRUDE",false);

   Bface* face = 0;

   if (!(gest->straightness() > 0.8)) {
      err_adv(debug, "DrawPen::handle_extrude: gesture not straight");
      return false;
   }
   if (!(surf_sel == Bsurface::hit_ctrl_surface(gest->start(), 1, &face))) {
      err_adv(debug, "DrawPen::handle_extrude: hit surface != selected surf");
      return false;
   }
   if (!(face)) {
      err_adv(debug, "DrawPen::handle_extrude: can't get hit face");
      return false;
   }

   VEXEL fvec  = VEXEL(face->v1()->loc(), face->norm());
   VEXEL fgest = gest->endpt_vec();

   // If gesture nearly parallel to normal:
   double a = rad2deg(line_angle(fvec,fgest));
   err_adv(debug, "DrawPen::handle_extrude: angle: %f %s",
           a, (a > 15) ? "(bad)" : "(good)");
   if (a > 15) {
      return false;
   }

   // calculate extrude width
   double dist = fgest.length()/fvec.length();       
   if (fvec*fgest<0)
      dist=-dist; // Get the sign right

   if (is_selected_any((Lface*)face)) {
      Bface_list faces = Bface_list::reachable_faces(face, !SelectedFaceBoundaryEdgeFilter());
      INFLATE::init(faces, dist);
   } else {
      if (!INFLATE::init( face, dist ) && Skin::isa(surf_sel)) {
         Skin* skin = Skin::upcast(surf_sel);
         if (skin->is_inflate()) {
            skin->add_offsets(dist);
         }
      }
   }
   ModeReset(0);

   return true;
}

//! In general, drawing that's not classified as anything
//! else end up here. So it's some drawing that doesn't wind
//! too much, not drawn too fast and straight
int
DrawPen::stroke_cb(CGESTUREptr& gest, DrawState*& state)
{
   static bool debug = Config::get_var_bool("DEBUG_STROKE_CB",false);

   err_adv(debug, "DrawPen::stroke_cb");

   if (gest->spread() < MIN_GESTURE_SPREAD) {
      err_adv(debug, "short spread: %f, skipping", gest->spread());
      return 0;
   }

   Bcurve*   curve_sel =   Bcurve::selected_curve();
   Bsurface* surf_sel  = Bsurface::selected_surface();
   
   if (_mode==SELECTED) {
      // SELECTED : Something, possibly more than one, is selected

      if (surf_sel) {
         // If one surface is selected
         err_adv(debug, "DrawPen::stroke_cb: surface selected");

         // Select an underlying skeleton curve?
         // XXX - "for now" have to select the Primitive first
         if (select_skel_curve(surf_sel, gest))
            return 0;

         // Invoke inflate operation
         // XXX - should be in line_cb
         // XXX - or better yet in INFLATE widget
         if (handle_extrude(surf_sel, gest))
            return 0;

         // user interaction to determine how a panel should
         // be tessellated
         // XXX - have to select the panel first
         if (panel_op(surf_sel, gest))
            return 0;

         err_adv(debug, "DrawPen::stroke_cb: no action taken");
         return 0;
    
      } else if (curve_sel) {

         // Reach here to do curve oversketch.
         // No short or quick strokes allowed.
         if (!( check_stroke_length("DrawPen::stroke_cb", gest, debug) &&
                check_stroke_duration("DrawPen::stroke_cb", gest, debug)))
            return 0;

         // ----------------Oversketch the curve
         if (//!curve_sel->has_shadow() &&
             curve_sel->oversketch(gest->pts())) {
            return 0;
         }
         // ----------------Oversketch curve ends

         // we failed
         return 0; 
      } // If surface selected END

   } // SELECTED MODE End
   
   // if nothing happen to the stroke, try to create a curve out
   // of it
   if (create_curve(gest))
      return 0;

//    // Fallback
//    create_scribble(gest);

   return 0;
}

// int
// DrawPen::create_scribble(GESTUREptr gest)
// {
//    if (!cur_easel())
//       _drawer->make_new_easel(_view);

//    // add gesture to easel
//    WORLD::undisplay(gest, false); // false: don't create an UNDO command
//    cur_easel()->add_line(gest);
//    return 1;
// }

//! Draw a straight line cross-ways to an existing
//! straight Bcurve to create a rectangle:
int
DrawPen::create_rect(GESTUREptr line)
{
   static bool debug = Config::get_var_bool("DEBUG_CREATE_RECT",false);
   err_adv(debug, "DrawPen::create_rect");

   if (!(line && line->is_line())) {
      err_adv(debug, "DrawPen::create_rect: gesture is not a line");
      return 0;
   }
   Bcurve* bcurve = Bcurve::hit_ctrl_curve(line->start());
   if (!(bcurve && bcurve->is_straight())) {
      err_adv(debug, "DrawPen::create_rect: no straight curve at start");
      return 0;
   }

   Bpoint *b1 = bcurve->b1(), *b2 = bcurve->b2();
   assert(b1 && b2);
   Wplane P = check_plane(shared_plane(b1, b2));
   if (!P.is_valid()) {
      err_adv(debug, "DrawPen::create_rect: no valid plane");
      return 0;
   }

   Wpt_list stroke;
   line->endpt_seg().project_to_plane(P, stroke);
   assert(stroke.num() == 2);
   Wvec u = b2->loc() - b1->loc();      // vector along existing straight line
   Wvec v = stroke[1] - stroke[0];      // vector along input stroke
   double a = rad2deg(line_angle(u, v));
   static double perp_thresh =
      Config::get_var_dbl("CREATE_RECT_PERP_THRESH", 84.0, true);
   err_adv(debug, "cross-stroke angle: %f", a);
   if (a < perp_thresh) {
      err_adv(debug, "DrawPen::create_rect: angle too low: %f < %f",
              a, perp_thresh);
      return 0;
   }

   //   Get oriented as follows, looking down onto the plane:
   //                                                       
   //      b1 . . . . . . . b4                              
   //      |                 .                              
   //      |                 .                              
   //      |                 .                              
   //      | ------- v ----->.                              
   //      |                 .                              
   //      |                 .                              
   //      |                 .                              
   //      b2 . . . . . . . b3                              

   // Swap b1 and b2 if necessary:
   Wvec n = P.normal();
   if (det(v,n,u) < 0) {
      swap(b1,b2);
      u = -u;
   }
   // Make v perpendicular to u:
   v = cross(n,u).normalized() * v.length();

   // Decide number of edges "horizontally" (see diagram above)
   int num_v = bcurve->num_edges(); // number of edges "vertically"
   double H = u.length();           // "height"
   double W = v.length();           // "width"
   double l = H/num_v;              // length of an edge "vertically"
   int num_h = (int)round(W/l);     // number of edges "horizontally"
   if (num_h < 1) {
      // Needs more work to handle this case. Bail for now:
      err_adv(debug, "DrawPen::create_rect: cross-stroke too short");
      return 0;
   }

   // Accept it now

   LMESHptr m = bcurve->mesh();
   Wpt p1 = b1->loc(), p2 = b2->loc(), p3 = p2 + v, p4 = p1 + v;

   MULTI_CMDptr cmd = new MULTI_CMD;

   // Create points b3 and b4
   Bpoint* b3 = BpointAction::create(m, p3, n, u, b2->res_level(), cmd);
   Bpoint* b4 = BpointAction::create(m, p4, n, u, b1->res_level(), cmd);

   // Create the 3 curves: bottom, right and top
   Wpt_list side;
   int res_lev = bcurve->res_level();
   Bcurve_list contour;
   contour += bcurve;

   // Bottom curve
   side.clear(); side += p2; side += p3;
   contour += BcurveAction::create(m, side, n, num_h, res_lev, b2, b3, cmd);

   // Right curve
   side.clear(); side += p3; side += p4;
   contour += BcurveAction::create(m, side, n, num_v, res_lev, b3, b4, cmd);

   // Top curve
   side.clear(); side += p4; side += p1;
   contour += BcurveAction::create(m, side, n, num_h, res_lev, b4, b1, cmd);

   // Interior
   PanelAction::create(contour, cmd);
   
   // Cancel selections and start fresh
   ModeReset(cmd);

   WORLD::add_command(cmd);

   return 1;
}

/*!
  Create a 3D curve (Bcurve) from the given gesture.  Do this by
  finding a plane to project the gesture into.  That can be the
  plane of the axis widget, the plane of the floor, or a plane
  containing the 2 Bpoints intersected by the first and last
  points of the gesture (if any).
*/
int
DrawPen::create_curve(GESTUREptr gest)
{
   static bool debug = Config::get_var_bool("DEBUG_CREATE_CURVE",false);

   err_adv(debug, "DrawPen::create_curve");

   // Do nothing for short gestures
   if (gest->length() < MIN_GESTURE_LENGTH) {
      err_adv(debug, "  gesture too short");
      return 0;
   }

   PIXEL hit_start, hit_end;
   Bpoint* b1 = Bpoint::hit_point(gest->start(), 8, hit_start);
   Bpoint* b2 = Bpoint::hit_point(gest->end(),   8, hit_end);

   // tangent vectors in the draw plane:
   Wvec t, b;

   // Find a plane to project the stroke into, but only
   // accept planes that are sufficiently parallel to the
   // film plane.
   Wplane P = get_plane(b1, b2);
   if (!P.is_valid()) {
      b1 = b2 = 0;
      // Ignoring the endpoints, try for a plane from the FLOOR
      // or AxisWidget:
      P = get_draw_plane(gest->pts(), t, b);
      if (!P.is_valid()) {
         err_adv(debug, "  can't find valid plane");
         return 0;
      }
   }

   // Project pixel trail to the plane, and if the gesture
   // is "closed", make the Wpt_list form a closed loop.
   Wpt_list wpts;
   project_to_plane(gest, P, wpts);

   // If user draws a straight line that is nearly aligned w/ the
   // major axes of the floor or draw axis, make it align exactly:
   static const double ALIGN_THRESH =
      Config::get_var_dbl("STRAIGHT_LINE_ALIGN_THRESH", 7); // 7 degrees
   try_align_line(wpts, t, b, ALIGN_THRESH, debug);

   // If closed curve, don't use endpoints:
   if (wpts.is_closed()) {
      b1 = b2 = 0;
   }

   MULTI_CMDptr cmd = new MULTI_CMD;
   LMESHptr mesh = tess::get_common_mesh(b1,b2,cmd);
   if (!mesh) {
      cerr << "DrawPen::create_curve: error: "
           << "can't find mesh to use" << endl;
      return 0;
   }
   assert(mesh != 0);

   int r = get_res_level(mesh);
   int num_edges = 4; // XXX - default is 4 edges; should be smarter:

   err_adv(debug, "  %d edges, res level %d", num_edges, r);

   // Create endpoints as needed:
   if (b1 && b2)
      wpts.fix_endpoints(b1->loc(),b2->loc());
   if (t.is_null()) {
      t = b1 ? b1->t() : b2 ? b2->t() : P.normal().perpend();
   }
   if (!wpts.is_closed() && !b1)
      b1 = BpointAction::create(mesh, wpts.first(), P.normal(), t, r, cmd);
   if (!wpts.is_closed() && !b2)
      b2 = BpointAction::create(mesh, wpts.last(),  P.normal(), t, r, cmd);

   // Create the curve
   
   Bcurve* curve = BcurveAction::create(
      mesh, wpts, P.normal(), num_edges, r, b1, b2, cmd
      ); 
   err_adv(debug, "  succeeded");

   Bcurve_list contour = curve->extend_boundaries();
   if (PAPER_DOLL::init(contour)) {
      
   } else if (!contour.empty()) {
      // If the curve completes a closed loop (on its own or by
      // joining existing curves), fill the interior with a
      // "panel" surface:
      Bsurface* surf = PanelAction::create(contour, cmd);
      if (surf) {
         WORLD::message(str_ptr("built ") + surf->class_name());
      }
   }
   FLOOR::realign(mesh, cmd);
   ModeReset(cmd);
   WORLD::add_command(cmd);

   return 1;
}

void
DrawPen::project_to_plane(
   CGESTUREptr& gest,   // gesture to project
   CWplane     &P,      // plane to project onto
   Wpt_list    &ret     // RETURN: projected points
   )
{
   static bool debug = Config::get_var_bool("DEBUG_CREATE_CURVE",false);

   ret.clear();
   err_adv(debug, "DrawPen::project_to_plane: straightness: %f",
           gest->straightness());
   if (gest->is_line(.991, 12)) {
      if (debug) 
         cerr << "gesture interpreted as line" << endl;
      ret += P.intersect(Wline(XYpt(gest->start())));
      ret += P.intersect(Wline(XYpt(gest->end  ())));
      ret.update_length();
      return;
   }

   gest->pts().project_to_plane(P, ret);

   if (gest->is_closed()) {
      // If closed, remove the final few points to prevent jagginess
      for (int i = ret.num()-1;i>=0;i--)
         if (PIXEL(ret[0]).dist(PIXEL(ret[i])) < 15)
            ret.remove(i);
         else break;    

      // add the first point as the last point
      ret += ret[0];
       
      ret.update_length();
   } 

   // testing 4-point interpolating subdivision
   if (Config::get_var_bool("TEST_4_PT_SUBDIV_CURVES",false)) {
      Wpt_list pts = refine_polyline_interp(resample_polyline(ret, 9), 3);
      WORLD::show_polyline(pts);
      WORLD::show_pts(pts, 4, Color::yellow);
   }
}

//! Create a "ball" centered at the given point, with the given
//! screen-space radius in PIXELs (to be converted to world space)
void
DrawPen::create_sphere(Bpoint* skel, double pix_rad) const
{
   MULTI_CMDptr cmd = new MULTI_CMD;
   Primitive* ball = Primitive::create_ball(skel, pix_rad, cmd);
   if (!ball) {
      err_msg("DrawPen::create_sphere: failed");
      return;
   }
   ball->set_name("ball");
   //FLOOR::realign(get_cur_mesh(ball->mesh()), cmd);
   WORLD::add_command(cmd);
}

/*!
  This is called by the GEST_INT on a down event.

  Currently used just to hold selected things 
  (so they don't fade out mid-gesture).
*/

void
DrawPen::notify_down()
{
   // HOLD everything that's selected
   Bbase::hold();
   _tap_callback = false;
}

void
DrawPen::activate(State *start) 
{
   Pen::activate(start);

   // Change to control line texture
   if (_view)
      _view->set_rendering(ControlLineTexture::static_name());

   // Turn on floor
   FLOOR::show();
}

bool
DrawPen::deactivate(State *start)
{
   bool ret;

   ret = Pen::deactivate(start);
   assert(ret);

   // Hide floor:
   FLOOR::hide();

   return true;
}

int
DrawPen::drag_move_cb(CEvent &, State *&)
{
//     // move the drag point (it handles the axis constraint)
//     _drag_pt->move_to(DEVice_2d::last->cur());

   return 0;
}

int
DrawPen::drag_up_cb(CEvent &, State *&)
{
//     _drag_pt->set_show_strut(0);
//     _drag_pt = 0;
   
   return 0;
}

// end of file draw_pen.C

