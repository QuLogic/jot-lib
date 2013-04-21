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
 * xform_pen.C:
 **********************************************************************/
#include "ffs/cursor3d.H"
#include "ffs/floor.H"
#include "ffs/xform_pen.H"

static bool debug = Config::get_var_bool("DEBUG_XFORM_PEN", false);

/*****************************************************************
 * XformPen
 *****************************************************************/
XformPen::XformPen(
   CGEST_INTptr &gest_int,
   CEvent &d,
   CEvent &m,
   CEvent &u) :
   Pen(str_ptr("xform"), gest_int, d, m, u)
{
   // Set up GESTURE FSA (First matched will be executed)

   _draw_start += DrawArc(new GarbageGuard,  drawCB(&XformPen::garbage_cb));
   _draw_start += DrawArc(new DoubleTapGuard,drawCB(&XformPen::double_tap_cb));
   _draw_start += DrawArc(new       TapGuard,drawCB(&XformPen::       tap_cb));
   _draw_start += DrawArc(new StrokeGuard,   drawCB(&XformPen::stroke_cb));
}

XformPen::~XformPen()
{
}

int
XformPen::garbage_cb(CGESTUREptr&, DrawState*&)
{
   return 0;
}

int
XformPen::tap_cb(CGESTUREptr& tap, DrawState*& s)
{
   assert(tap);
   if (tap->is_double_tap()) {
      // should never happen given order of arcs in XformPen constructor
      cerr << "XformPen::tap_cb: error: gesture is double tap"
           << endl;
      return 0;
   }

   // tap on cursor?
   BMESHray ray(tap->center());
   _view->intersect(ray);
   Cursor3D* c = Cursor3D::upcast(ray.geom());
   if (c) {
      err_adv(debug, "XformPen::tap_cb: hit axis");
      c->handle_gesture(tap);
      return 0;
   }

   // tap on mesh?
   _mesh = 0;
   Bface* f = cur_face();
   if (!f) {
      err_adv(debug, "XformPen::tap_cb: missed face");
      return cancel_cb(tap, s);
   }
   BMESH* m = f->mesh();
   if (!m) {
      err_adv(debug, "XformPen::tap_cb: hit face, no mesh");
      return cancel_cb(tap, s);
   }
   GEOMptr g = bmesh_to_geom(m);
   if (!g) {
      err_adv(debug, "XformPen::tap_cb: hit mesh, no geom");
      return cancel_cb(tap, s);
   }
   // skip floor:
   if (FLOOR::isa(g)) {
      err_adv(debug, "XformPen::tap_cb: hit floor, skipping...");
      return cancel_cb(tap, s);
   }

   // tap on ordinary mesh (not floor):
   _mesh = m;
   assert(_mesh);
   BMESH::set_focus(_mesh, f->patch());
   FLOOR::show();
   FLOOR::realign(_mesh,0); // 0 = no undo command

   Cursor3D::attach(g);

   return 0;
}

int
XformPen::double_tap_cb(CGESTUREptr& tap, DrawState*&)
{
   if (!_mesh) {
      if (debug) {
         cerr << "XformPen::double_tap_cb: no mesh"
              << endl;
      }
      return 0;
   }

   Cursor3D* c = Cursor3D::find_cursor(bmesh_to_geom(_mesh));
   if (c) {
      c->toggle_show_box();
   }

   return 0;
}

int
XformPen::stroke_cb(CGESTUREptr& stroke, DrawState*&)
{
   return 0;
}

int
XformPen::cancel_cb(CGESTUREptr&, DrawState*&)
{
   Cursor3Dptr c = Cursor3D::get_active_instance();
   if (c) {
      c->detach();
      c->deactivate();
   }
   _mesh = 0;
   return 0;
}
// end of file xform_pen.C
