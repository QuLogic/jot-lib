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
 * draw_manip.C
 **********************************************************************/

/*!
 *  \file draw_manip.C
 *  \brief Class for manipulating FFS objects via middle button.
 *
 *  \ingroup group_FFS
 *  \sa draw_manip.H
 *
 */

#include "geom/world.H"
#include "tess/bcurve.H"

#include "floor.H"
#include "draw_manip.H"
#include "crv_sketch.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_DRAW_MANIP",false);

/**********************************************************************
 * DrawManip:
 **********************************************************************/
DrawManip::DrawManip(CEvent &d, CEvent &m, CEvent &u) :
   Simple_int(d,m,u),
   _down(false)
{
   _entry.set_name("DrawManip Entry");

   // define FSA state transitions
   _choosing    += Arc(m, Cb((_callb::_method)&DrawManip::choose_cb));
   _plane_trans += Arc(m, Cb((_callb::_method)&DrawManip::plane_trans_cb));
   _line_trans  += Arc(m, Cb((_callb::_method)&DrawManip::line_trans_cb));

   // an up event resets any state to the entry state
   Arc ret = Arc(u, Cb((_callb::_method) &DrawManip::up,(State*)-1));
   _choosing    += ret;
   _plane_trans += ret;
   _line_trans  += ret;
}

//! Find a plane associated with the Bcurve.
//! Used by find_xlate_plane(), below.
inline Wplane
find_plane(CBcurve* c)
{
   // No curve, no plane
   if (!c)
      return Wplane();

   // If it has a shadow, use the plane of the shadow:
   if (c->has_shadow())
      return c->shadow_plane();

   // If it lies inside a constraining surface, use the plane
   // of the surface, if any:
   if (c->constraining_surface()) {
      PlaneMap* pm = PlaneMap::upcast(c->constraining_surface());
      return pm ? pm->plane() : Wplane();
   }

   // Use whatever plane the curve offers:
   return c->plane();
}

//! Find a plane associated with the Bcurve; return a
//! parallel plane that contains the given point p.  
//! To be used for translating the curve in the plane.
inline Wplane
find_xlate_plane(Bcurve* c, CWpt& p)
{

   Wplane P = find_plane(c);
   if (P.is_valid())
      return Wplane(p, P.normal());
   return Wplane();
}

inline void
init_cmd(MOVE_CMDptr& c, CBbase_list& bbases)
{
   c = new MOVE_CMD(bbases);
   assert(c);
   WORLD::add_command(c);
}

bool
DrawManip::init_point(Bpoint* p, State*& s)
{
   if (!p)
      return false;

   _down_pt = p->loc();
   _vec     = p->plane().normal();
   init_cmd(_cmd, Bbase_list(p));

   if (debug)
      cerr << "point normal: " << _vec << endl;

   if (p->has_shadow()) {
      // if it has a shadow, constrain it to the line thru the shadow:
      s = &_line_trans;
      err_adv(debug, "  pt line trans");
   } else if (p->constraining_surface() && !p->is_selected()) {
      // XXX - why "not selected"?
      s = &_plane_trans;
      err_adv(debug, "  pt plane trans");
   } else {
      // decide based on direction of user's stroke
      s = &_choosing;
      err_adv(debug, "  pt choosing");
   }
   return true;
}

bool
DrawManip::init_curve(Bcurve* c, State*& s)
{
   if (!c)
      return false;

   // _down_pt is set in DrawManip::down()

   Bbase_list bb(3);
   if (c->b1()) bb += c->b1();
   if (c->b2()) bb += c->b2();
   bb += c;

   init_cmd(_cmd, bb);

   // choose constraint plane/line:
   if (c->has_shadow()) {
      // If it has a shadow, constrain it to the line thru the shadow:

      // XXX - Need the direction for an axis constraint ... but
      // this won't be right for non-planar shadows when they are
      // implemented.
      _vec = find_plane(c).normal(); // use plane of shadow curve
      s = &_line_trans;
      err_adv(debug, "  curve line trans");
   } else {
      _vec = find_xlate_plane(c, _down_pt).normal();
      s = &_plane_trans;
      err_adv(debug, "  curve plane trans");
   }

   if (debug)
      cerr << "constraint vec: " << _vec << endl;

   return true;
}

int
DrawManip::down(CEvent& e, State*& s)
{
   err_adv(debug, "DrawManip::down");

   _down_pix = ptr_cur(); // used in choose_cb()

   // special handling for active curve sketch
   // XXX - needs comments
   CRV_SKETCHptr cs = CRV_SKETCH::get_instance();
   if (cs && cs->is_active()) {
      PIXEL_list pix = cs->shadow_pix();
      if (pix[0].dist(_down_pix) < 8) {
         _down = true;
         _first = true;
      } else if (pix.last().dist(_down_pix) < 8) {
         _down = true;
         _first = false;
      }
   }

   assert(e.view() != 0);
   BMESHray ray(ptr_cur());
   e.view()->intersect(ray);
   GEOMptr geom;
   if (ray.success()) {
      geom = GEOM::upcast(ray.geom());
      if (geom && geom->interactive(e, s, &ray)) {
         return 1; // geom is taking over remaining events until up event
      }
      _down_pt = ray.surf();                    
      _vec = ray.norm();
   }

   if (init_point(Bpoint::hit_ctrl_point(ptr_cur()),s) ||
       init_curve(Bcurve::hit_ctrl_curve(ptr_cur(), 6, _down_pt),s)) {
      return 0; // all set
   }
   if (FLOOR::isa(geom)) {
      // XXX - should handle floor manipulation
      return 0;
   }
   // this part seems to crash on windows in some cases...
   // turning it off for now:
   bool find_components = false;
   Bvert_list verts;
   if (find_components)
      verts = Bvert_list::connected_verts(VisRefImage::get_simplex(ptr_cur()));

   // map vertices to Bbase controllers at top level of subdiv hierarchy:
   Bbase_list bbases = Bbase::find_controllers(verts).get_ctrl_list();
   if (debug) {
      cerr << "  found " << bbases.num() << " controllers:" << endl << "  ";
      bbases.print_identifiers();
   }
   err_adv(debug, "  same mesh: %s", bbases.same_mesh() ? "true" : "false");

   if (bbases.empty() && !geom)
      return 0;

   _cmd = bbases.empty() ? new MOVE_CMD(geom) : new MOVE_CMD(bbases);
   assert(_cmd);
   WORLD::add_command(_cmd);
   _cmd->notify_xf_obs(XFORMobs::START);

   return 0;
}

int
DrawManip::move(CEvent &e, State *&s)
{
   if (!_cmd)
      return 0;

   // XXX - needs work, doing plane constraint ATM...
   if (plane().is_valid()) {
      apply_translation(plane().intersect(ptr_ray()) - _down_pt);
      return 0;
   }

   // XXX - need comments...
   CRV_SKETCHptr cs = CRV_SKETCH::get_instance();
   if (cs->is_active() & _down) {
      Bcurve* curve = cs->curve();
      Wpt_listMap* map = cs->shadow_map();
      Wpt pt = Wpt(curve->shadow_plane(), Wline(ptr_cur()));
      if (_first) 
         map->set_p0(new WptMap(pt));
      else
         map->set_p1(new WptMap(pt));
      map->recompute();

      SurfaceCurveMap* o = (SurfaceCurveMap*)curve->map();
      Wpt first = o->map(0), last = o->map(1);
      if (curve->b1()) {
         curve->b1()->move_to(first);
      }
      if (curve->b2()) {
         curve->b2()->move_to(last);
      }

      cs->reset_timeout();

      return 0;
   }

   return 0;
}

int
DrawManip::up(CEvent &e, State *&s)
{
   _down = false;
   if (_cmd)
      _cmd->notify_xf_obs(XFORMobs::END);
   _cmd = 0;
   return 0; 
}

int
DrawManip::choose_cb(CEvent &e, State *&s)
{
   // Based on initial motion, decide whether to constrain motion
   // to the selected Bpoint's y-axis, or to the plane
   // perpendicular to its y-axis.
   //
   // If the plane is nearly edge-on don't prefer constraining in
   // plane
   //
   // If y-axis is nearly tip-on, don't prefer motion along it.
   err_adv(debug, "DrawManip::choose_cb");

   if (!plane().is_valid()) {
      // _vec and _down_pt must be set in down() callback
      err_adv(debug, "  error: invalid plane");
      return 0;
   }

   PIXEL cur = ptr_cur();
   if (_down_pix.dist(cur) > 6) {
      // Pointer moved over 6 pixels... we're ready to choose

      _down_pix = cur;

      Wpt      eye = e.view()->cam()->data()->from();
      Wvec eye_vec = (eye - _down_pt).normalized();
      Wvec       n = _vec.normalized();
      double dot = fabs(eye_vec * n);
      if (dot > cos(deg2rad(15))) {
         // Angle between view vector and plane normal is less
         // than 15 degrees.  Constrain motion to plane.
         err_adv(debug, "  onto plane");
         s = &_plane_trans;
      } else if (dot < cos(deg2rad(85))) {
         // Angle between view vector and plane normal is
         // greater than 85 degrees. I.e., plane is
         // foreshortened.  Constrain motion to axis.
         err_adv(debug, "  foreshortened plane");
         s = &_line_trans;
      } else {
         // Viewing is not too extreme either way.  For a
         // Bpoint with no ability to do a line constraint,
         // do a plane constraint.

         // Check angle in film plane; if motion is aligned
         // w/ the axis we'll do constraint along the
         // axis. Otherwise in the plane:
         Wpt   o = _down_pt;
         PIXEL film_o = o;
         VEXEL film_y = (PIXEL(o + n) - film_o).normalized();
         VEXEL film_d = (cur     - film_o).normalized();
         dot = fabs(film_d * film_y);
         if (dot > cos(deg2rad(25))) {
            // moving along axis
            // constrain motion to axis
            s = &_line_trans;
            err_adv(debug, "  line constraint");
         } else {
            // moving cross-ways to axis
            // constrain motion to plane
            s = &_plane_trans;
            err_adv(debug, "  plane constraint");
         }
      }
   }

   return 0;
}

void
DrawManip::apply_translation(CWvec& delt)
{
   _down_pt += delt; // update _down_pt:
   if (_cmd) {
      _cmd->add_delt(delt);
      _cmd->notify_xf_obs(XFORMobs::MIDDLE);
   } else {
      err_adv(debug, "DrawManip::apply_translation: error: command is null");
   }
}

int
DrawManip::plane_trans_cb(CEvent&, State*&)
{
   err_adv(0 && debug, "DrawManip::plane_trans_cb");
   if (plane().is_valid()) {
      apply_translation(plane().intersect(ptr_ray()) - _down_pt);
   }
   return 0;
}

int
DrawManip::line_trans_cb(CEvent&, State *&)
{
   err_adv(0 && debug, "DrawManip::line_trans_cb");

   if (_vec.is_null()) {
      err_msg("DrawManip::line_trans_cb: error: null constraint vector");
      return 0;
   }
   apply_translation(line().intersect(ptr_ray()) - _down_pt);

   return 0;
}

int
DrawManip::up_cb(CEvent&, State*&)
{
   return 0;
}

// end of file pen.C
