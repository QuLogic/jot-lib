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
 *  \file circle_widget.C
 *  \brief Contains the definition of the CIRCLE_WIDGET widget.
 *
 *  \ingroup group_FFS
 *  \sa circle_widget.H
 *
 */
#include "disp/colors.H"
#include "geom/gl_view.H"     // for GL_VIEW::draw_pts()
#include "geom/world.H"       // for message display
#include "gtex/ref_image.H"
#include "mesh/lmesh.H"
#include "mesh/mi.H"
#include "tess/panel.H"
#include "tess/bsurface.H"
#include "tess/tex_body.H"
#include "tess/ti.H"
#include "tess/action.H"
#include "std/config.H"

#include "ffs/ffs_util.H"
#include "ffs/draw_pen.H"

#include "circle_widget.H"

using namespace mlib;
using namespace tess;
using namespace FFS;

static bool debug_all =
Config::get_var_bool("DEBUG_CIRCLE_WIDGET_ALL",false);

const double PIXEL_DIST_THRESH = 10;

/*****************************************************************
 * CIRCLE_WIDGET
 *****************************************************************/
CIRCLE_WIDGETptr CIRCLE_WIDGET::_instance;

CIRCLE_WIDGETptr
CIRCLE_WIDGET::get_instance()
{
   if (!_instance)
      _instance = new CIRCLE_WIDGET();
   return _instance;
}

CIRCLE_WIDGET::CIRCLE_WIDGET() :
   DrawWidget(),
   _init_stamp(0)
{
   //*******************************************************
   // FSA states for handling GESTUREs:
   //
   //   Note: the order matters. First matched will be executed.
   //*******************************************************
   _draw_start += DrawArc(new TapGuard,         drawCB(&CIRCLE_WIDGET::tap_cb));
   _draw_start += DrawArc(new StrokeGuard,      drawCB(&CIRCLE_WIDGET::stroke_cb));

   _init_stamp = 0;
   _circle = 0;
   _cmd = 0;
   _suggest_active = false;

   // Set up the clean up routine
   atexit(clean_on_exit);

   _cmd = new MULTI_CMD;
}

CIRCLE_WIDGET::~CIRCLE_WIDGET()
{}

void
CIRCLE_WIDGET::clean_on_exit()
{
   _instance = 0;
}

int
CIRCLE_WIDGET::cancel_cb(CGESTUREptr&, DrawState*& s)
{
   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we DID use up the gesture
}

int
CIRCLE_WIDGET::tap_cb(CGESTUREptr& g, DrawState*& s)
{
   err_adv(debug_all, "CIRCLE_WIDGET::tap_cb()");
   PIXEL pdummy;
   int idummy;

   if( _suggest_active ) {
      _preview.update_length();
      _literal_shape.update_length();
      double preview_dist = PIXEL_list(_preview).closest( PIXEL(g->start()), pdummy, idummy );
      double literal_dist = PIXEL_list(_literal_shape).closest( PIXEL(g->start()), pdummy, idummy );
      if( preview_dist < literal_dist && preview_dist < PIXEL_DIST_THRESH ) {
         _suggest_active = false;
         make_preview();
         return 1;
      } else if( literal_dist < preview_dist && literal_dist < PIXEL_DIST_THRESH ) {
         finish_literal();
         return cancel_cb(g,s);
      } else {
         return cancel_cb(g,s);
      }
   }

   if (_cmd)
      WORLD::add_command(_cmd);

   return cancel_cb(g,s);
}

int
CIRCLE_WIDGET::stroke_cb(CGESTUREptr& g, DrawState*&)
{
   err_adv(debug_all, "CIRCLE_WIDGET::stroke_cb()");

   // Activity occurred to extend the deadline for fading away:
   reset_timeout();

   // sanity check
   assert(g);

   if( !g->is_line() )
      return 1;
   _preview.update_length();
   if (PIXEL_list(_preview).dist(g->start()) < PIXEL_DIST_THRESH  ||
       g->start().dist(_center) < PIXEL_DIST_THRESH  ) {
      if ( _circle ) {
         Bcurve *border = Bcurve::lookup(_circle->bfaces().get_boundary().edges());
         if ( border != 0 ) {
            Wplane plane = border->plane();
            _radius = _center.dist(Wpt(plane, Wline(XYpt(g->end()))));
         }
      } else {
         Wplane P = get_draw_plane(g->end());
         if (!P.is_valid())
            return 1;
         _radius = _center.dist(Wpt(P, Wline(XYpt(g->end()))));
      }
      make_preview();
   }

   return 1;
}

//! Returns true if the gesture is valid for beginning a circle_widget
//! session.
bool
CIRCLE_WIDGET::init(CGESTUREptr&  gest)
{

   static bool debug =
      Config::get_var_bool("DEBUG_CIRCLE_WIDGET_INIT",false) || debug_all;

   err_adv(debug, "CIRCLE_WIDGET::init");

   // Get the ellipse description:
   PIXEL center;  // the center
   VEXEL axis;    // the long axis (unit length)
   double r1, r2; // the long and short radii, respectively
   bool suggest_active;

   if (!gest->is_ellipse(center, axis, r1, r2)) {
      if (!gest->is_almost_ellipse(center, axis, r1, r2)) {
         return 0;
      } else {
         suggest_active = true;
      }
   } else {
      suggest_active = false;
   }

   Wplane plane = get_draw_plane(center);
   if (!plane.is_valid()) {
      // didn't work out this time
      return false;
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
   Wpt x0 = Wpt(plane, Wline(XYpt(center - axis*r1)));
   Wpt x1 = Wpt(plane, Wline(XYpt(center + axis*r1)));
   Wpt y0 = Wpt(plane, Wline(XYpt(center - perp*r2)));
   Wpt y1 = Wpt(plane, Wline(XYpt(center + perp*r2)));

   Wpt c = Wpt(plane, Wline(XYpt(center)));

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
   double cos_theta = VIEW::eye_vec(c) * plane.normal();
   double scale = sqrt(fabs(cos_theta)); // XXX - just trying this out
   MIN_RATIO *= scale;  // XXX - this is why it can't be static
   double ratio = min(dx,dy)/max(dx,dy);
   err_adv(debug, "CIRCLE_WIDGET::init: projected ratio: %f, min: %f",
           ratio, MIN_RATIO);
   if (ratio < MIN_RATIO)
      return false;

   CIRCLE_WIDGETptr me = get_instance();

   static const int DISK_RES = Config::get_var_int("DISK_RES", 4,true);
   me->_plane = plane;
   me->_radius = dx/2;
   me->_disk_res = DISK_RES;

   me->_center = c;

   me->_init_stamp = VIEW::stamp();
   if( me->_suggest_active = suggest_active ) {
      me->create_literal(gest);
   }
   //PIXEL gest_center = gest->center();

   me->make_preview();

   return go();
}

void
CIRCLE_WIDGET::reset()
{
   err_adv(debug_all, "CIRCLE_WIDGET::reset()");
   _init_stamp = 0;
   _circle = 0;
   _cmd = 0;
   //_border = 0;

}

bool
CIRCLE_WIDGET::create_literal(GESTUREptr gest)
{
   PIXEL hit_start, hit_end;
   Bpoint* b1 = Bpoint::hit_point(gest->start(), 8, hit_start);
   Bpoint* b2 = Bpoint::hit_point(gest->end(),   8, hit_end);

   // Find a plane to project the stroke into, but only
   // accept planes that are sufficiently parallel to the
   // film plane.
   Wplane P = get_plane(b1, b2);
   if (!P.is_valid()) {
      b1 = b2 = 0;
      // Ignoring the endpoints, try for a plane from the FLOOR
      // or AxisWidget:
      P = get_draw_plane(gest->pts());
      if (!P.is_valid()) {
         return false;
      }
   }

   // Project pixel trail to the plane, and if the gesture
   // is "closed", make the Wpt_list form a closed loop.
   //DrawPen::project_to_plane(gest, P, _literal_shape);
   _literal_shape.clear();

   gest->pts().project_to_plane(P, _literal_shape);

   if (gest->is_closed()) {
      // If closed, remove the final few points to prevent jagginess
      for (int i = _literal_shape.num()-1;i>=0;i--)
         if (PIXEL(_literal_shape[0]).dist(PIXEL(_literal_shape[i])) < 15)
            _literal_shape.remove(i);
         else
            break;

      // add the first point as the last point
      _literal_shape += _literal_shape[0];

      _literal_shape.update_length();
   }
   return true;
}

bool
CIRCLE_WIDGET::finish_literal(void)
{
   MULTI_CMDptr cmd = new MULTI_CMD;

   // Get a mesh to put the new curve into.
   LMESHptr mesh = TEXBODY::get_skel_mesh(cmd);
   assert(mesh != 0);

   Wplane P = get_draw_plane(_literal_shape);

   int res_level = 3;
   int num_edges = 4; // XXX - default is 4 edges; should be smarter:
   // Create the curve

   Bcurve* curve = BcurveAction::create(
      mesh, _literal_shape, P.normal(), num_edges, res_level, 0, 0, cmd
      );
   curve->mesh()->update_subdivision(res_level);

   // If the curve completes a closed loop (on its own or by
   // joining existing curves), fill the interior with a
   // "panel" surface:
   PanelAction::create(curve->extend_boundaries(), cmd);
   WORLD::add_command(cmd);
   return true;
}

//! After a successful call to init(), this turns on a CIRCLE_WIDGET
//! to handle the session:
bool
CIRCLE_WIDGET::go(double dur)
{

   static bool debug =
      Config::get_var_bool("DEBUG_CIRCLE_WIDGET_GO",false) || debug_all;
   err_adv(debug,"circle widget go");

   CIRCLE_WIDGETptr me = get_instance();

   // Should be good to go:
   if (me->_init_stamp != VIEW::stamp())
      return 0;

   // Set the timeout duration
   me->set_timeout(dur);

   // Become the active widget and get in the world's DRAWN list:
   me->activate();

   return 1;
}

void
CIRCLE_WIDGET::make_preview( void )
{
   _preview.clear();

   // Get a coordinate system
   Wvec Z = _plane.normal();
   Wvec X = Z.perpend();
   Wvec Y = cross(Z,X);
   Wtransf xf(_center, X, Y, Z);

   // Make the hi-res circle for the curve's map1d3d:
   const int ORIG_RES = 256;
   _preview.realloc(ORIG_RES + 1);
   double dt = (2*M_PI)/ORIG_RES;
   for (int i=0; i<ORIG_RES; i++) {
      double t = dt*i;
      _preview += xf*Wpt(_radius*cos(t), _radius*sin(t), 0);
   }
   _preview += _preview[0];       // make it closed

   if( _suggest_active ) {
      return;
   }

   if( _circle == 0 ) {
      // XXX - no undo! should fix
      _circle = PanelAction::create(
         _plane, _center, _radius, TEXBODY::get_skel_mesh(0), _disk_res, 0
         );
   } else {
      Bcurve *border = Bcurve::lookup(_circle->bfaces().get_boundary().edges());
      if( border != 0 ) {
         Wpt_listMap *map = Wpt_listMap::upcast(border->map());
         if( map )
            map->set_pts(_preview);
      }
   }

}

int
CIRCLE_WIDGET::draw(CVIEWptr& v)
{
   if( _suggest_active ) {
      GL_VIEW::draw_wpt_list( _literal_shape,
                              COLOR(.15,.85,.25),
                              alpha(),
                              3,
                              true
         );
      GL_VIEW::draw_wpt_list( _preview,
                              COLOR(.075,.425,.125),
                              alpha(),
                              3,
                              true
         );
   } else {
      GL_VIEW::draw_wpt_list( _preview,
                              Color::yellow,
                              alpha(),
                              3,
                              true
         );
   }
   GL_VIEW::init_point_smooth(float(6), GL_CURRENT_BIT);
   GL_COL(Color::orange, 1);             // GL_CURRENT_BIT
   glDisable(GL_LIGHTING);           // GL_ENABLE_BIT
   glDisable(GL_DEPTH_TEST);
   glBegin(GL_POINTS);
   glVertex3dv(Wpt(NDCZpt(_center)).data());
   glEnd();
   GL_VIEW::end_point_smooth();      // pop state

   return 1;
}

// end of file circle_widget.C
