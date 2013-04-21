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
/*****************************************************************
 * cursor3d.C
 *****************************************************************/

/*!
 *  \file cursor3d.C
 *  \brief Contains the definition of the Cursor3D class, plus helper functions.
 *
 *  \ingroup group_FFS
 *  \sa cursor3d.H
 *
 */

#include "geom/gl_util.H"       // for GL_COL()
#include "geom/gl_view.H"
#include "geom/world.H"         // for WORLD::undisplay
#include "manip/cam_pz.H"       // for CamFocus

#include "ffs/ffs_util.H"
#include "ffs/floor.H"
#include "ffs/cursor3d.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_CURSOR3D",false);
/*****************************************************************
 * Cursor3D
 *****************************************************************/
//! default apparent length of axes (in pixels):
static const double CURSOR3D_LEN = 35;

//! size of GL_POINT used to render the shadow
static const double SHADOW_SIZE_PIX=8.0;

Event                Cursor3D::_down;
Event                Cursor3D::_move;
Event                Cursor3D::_up;
map<GEOM*,Cursor3D*> Cursor3D::_map;

Cursor3D::Cursor3D() :
   DrawWidget(),
   _x(1,0,0),
   _y(0,1,0),
   _z(0,0,1),
   _selected(Y_AXIS),
   _s(10,10,10),
   _show_box(false)
{
   set_transp(1); // so that frame can be transparent

   //*******************************************************
   // FSA states for handling mouse events directly:
   //*******************************************************

   // entry state:
   _entry          += Arc(_down, DACb(&Cursor3D::down_cb));

   // choosing
   _y_choose       += Arc(_move, DACb(&Cursor3D::y_choose_cb));

   // doing
   _xlate_plane    += Arc(_move, DACb(&Cursor3D::xlate_plane_cb));
   _xlate_axis     += Arc(_move, DACb(&Cursor3D::xlate_axis_cb));
   _rotate         += Arc(_move, DACb(&Cursor3D::rotate_cb));
   _frame_resize   += Arc(_move, DACb(&Cursor3D::resize_uni_cb));
   _frame_resize_z += Arc(_move, DACb(&Cursor3D::resize_z_cb));
   _frame_resize_x += Arc(_move, DACb(&Cursor3D::resize_x_cb));


   // returning from doing
   Arc reset(_up, DACb(&Cursor3D::up_cb, (State*)-1));
   _y_choose       += reset;
   _xlate_plane    += reset;
   _xlate_axis     += reset;
   _rotate         += reset;
   _frame_resize   += reset;
   _frame_resize_z += reset;
   _frame_resize_x += reset;

   //*******************************************************
   // FSA states for handling GESTUREs:
   //
   //   Note: the order matters. First matched will be executed.
   //*******************************************************
   _draw_start += DrawArc(new TapGuard,      drawCB(&Cursor3D::tap_cb));
   _draw_start += DrawArc(new ScribbleGuard, drawCB(&Cursor3D::scribble_cb));
}

Cursor3D::~Cursor3D()
{
   if (debug) {
      cerr << "Cursor3D::~Cursor3D"
           << endl;
   }

   // erase reference to this instance in _map:
   detach();
}

void 
Cursor3D::write_xform(CWtransf& xf, CWtransf&, CMOD&)
{
   _origin = xf * _origin;
   _y = (xf * _y).normalized();
   _x = (xf * _x).orthogonalized(_y).normalized();
   _z = cross(_x,_y);
}

void
Cursor3D::detach()
{
   // break the link between this instance and the attached GEOM
   //   need to erase reference to attached geom, and
   //   remove link from attached geom to this instance in _map

   if (!_attached)
      return;

   if (debug) {
      cerr << "Cursor3D::detach: erasing link..."
           << endl;
   }

   iter_t pos = _map.find(_attached);
   assert(pos != _map.end() && pos->second == this);
   pos->second = 0;
   _attached = 0;
}

Cursor3D*
Cursor3D::find_cursor(GEOMptr g)
{
   if (!g)
      return 0;
   
   citer_t pos = _map.find(g);
   return (pos == _map.end()) ? 0 : pos->second;
}

void
Cursor3D::set_attached(GEOMptr g)
{
   assert(g && !_attached && !find_cursor(g));

   _attached = g;       // record g as attached to this
   _map[g] = this;      // record this as cursor controlling g

   // adjust position and box dimensions to fit g:
   BBOX bb = g->bbox();
   if (!bb.valid()) {
      if (debug) {
         cerr << "Cursor3D::set_attached: error: invalid bbox in "
              << g->class_name() << " : " << g->name()
              << endl;
      }
      return;
   }
   _s = bb.dim()/2;
   set_origin(bb.min() + Wvec(_s[0],0,_s[2]));
   if (debug) {
      cerr << "Cursor3D::set_attached: using scale factors: "
           << _s
           << endl;
   }
}

Cursor3D*
Cursor3D::attach(GEOMptr g)
{
   if (!g)
      return 0;

   Cursor3D* ret = find_cursor(g);
   if (!ret) {
      ret = new Cursor3D();
      assert(ret);
      ret->set_attached(g);
   }
   ret->activate();
   if (debug) {
      cerr << "Cursor3D::attach: geom is " << g->name()
           << endl;
   }
   return ret;
}

void 
Cursor3D::set_origin(CWpt& o) 
{
   if (_cmd) {
      _cmd->concatenate_xf(Wtransf::translation(o - _origin));
   } else {
      _origin = o;
   }
}
   

//! find point q on segment ab nearest to p,
//! return barycentric coord of p on segment ab
inline bool
intersect_seg(CPIXEL& a, CPIXEL& b, CPIXEL& p, double& t, double& min_dist)
{

   VEXEL v = (b - a);
   double vv = v*v;
   double u = (vv < 1e-6) ? 0 : clamp(((p - a)*v)/vv, 0.0, 1.0);
   PIXEL q = a + v*u;

   double r = (q - p).length();
   if (r > min_dist)
      return 0;

   t        = u;
   min_dist = r;

   return 1;
}

#ifdef JOT_NEEDS_FULL_CLASS_TYPE
Cursor3D::intersect_t
#else
intersect_t
#endif
Cursor3D::intersect(
   CPIXEL& p,           // ray position (pixels)
   Wpt& hit,            // hit point
   Wvec& norm           // "normal" at hit point
   ) const
{
   if (!_origin.in_frustum())
      return INTERSECT_NONE;

   // check origin
   const double DIST_THRESH=8;
   PIXEL o = _origin;
   if (p.dist(o) < DIST_THRESH) {
      hit  = _origin;
      norm = _y;
      return INTERSECT_ORIGIN;
   }
   Wpt s;
   if (get_shadow(s) && p.dist(s) < SHADOW_SIZE_PIX) {
      FLOORptr f = FLOOR::lookup();
      assert(f);
      hit = s;
      norm = f->n();
      return INTERSECT_SHADOW;
   }

   // check axes (try all three and choose the best):
   PIXEL a = x_tip();
   PIXEL b = y_tip();
   PIXEL c = z_tip();

   intersect_t ret = INTERSECT_NONE;
   double        t = -1; // barycentric coord along intersected PIXEL segment
   double min_dist = 12; // maximum screen distance for intersection

   
   // X
   if (intersect_seg(o,a,p,t,min_dist)) {
      hit  = _origin + _x*(t*axis_scale());    // this is approximate
      norm = _x;
      ret  = INTERSECT_X;
   }

   // Y
   if (intersect_seg(o,b,p,t,min_dist)) {
      hit  = _origin + _y*(t*axis_scale());
      norm = _y;
      ret  = INTERSECT_Y;
   }

   // Z
   if (intersect_seg(o,c,p,t,min_dist)) {
      hit  = _origin + _z*(t*axis_scale());   
      norm = _z;
      ret  = INTERSECT_Z;
   }

   // axes have been checked...
   //   return now if success occurred
   if (ret != INTERSECT_NONE)
      return ret;

   // last, check frame:
   if (is_active()) {
      Wpt wa, wb, wc, wd;
      get_plane_corners(wa,wb,wc,wd);

      if (p.dist(PIXEL(wa)) < 8) {
         hit = wa;
         ret = INTERSECT_FRAME_CORNER;
      } else if (p.dist(PIXEL(wb)) < 8) {
         hit = wb;
         ret = INTERSECT_FRAME_CORNER;
      } else if (p.dist(PIXEL(wc)) < 8) {
         hit = wc;
         ret = INTERSECT_FRAME_CORNER;
      } else if (p.dist(PIXEL(wd)) < 8) {
         hit = wd;
         ret = INTERSECT_FRAME_CORNER;
      }

      // return now if success occurred
      if (ret != INTERSECT_NONE) {
         norm = sel();
         return ret;
      }

      // test frame sides.
      // first reset min_dist which was modified
      //  in intersect_seg() above:
      min_dist = 12;

      if (intersect_seg(PIXEL(wa),PIXEL(wb),p,t,min_dist)) {
         hit  = ((1-t)*wa) + ((t)*wb);
         ret  = INTERSECT_FRAME_X_SIDE;
      }
      else if (intersect_seg(PIXEL(wb),PIXEL(wc),p,t,min_dist)) {
         hit  = ((1-t)*wb) + ((t)*wc);
         ret  = INTERSECT_FRAME_Z_SIDE;
      }
      else if (intersect_seg(PIXEL(wc),PIXEL(wd),p,t,min_dist)) {
         hit  = ((1-t)*wc) + ((t)*wd);
         ret  = INTERSECT_FRAME_X_SIDE;
      }
      else if (intersect_seg(PIXEL(wd),PIXEL(wa),p,t,min_dist)) {
         hit  = ((1-t)*wd) + ((t)*wa);
         ret  = INTERSECT_FRAME_Z_SIDE;
      }

      if (ret != INTERSECT_NONE) {
         norm = sel();
      }
   }
   return ret;
}

RAYhit&
Cursor3D::intersect(RAYhit &r, CWtransf&, int) const
{
   Wpt    hit;
   Wvec   norm;
   if (intersect(r.screen_point(), hit, norm)) {
      // Treat axis as a 3D object (not a screen-space object),
      // but register a slightly closer distance than the real one:
      Wpt eye = VIEW::peek()->cam()->data()->from();
      double dist = eye.dist(hit) - world_length(_origin, 5);

      r.check(dist, 1, 0, (GEL*)this, norm, hit, hit, (APPEAR*)this, XYpt());
   }
   return r;
}

void
Cursor3D::toggle_active()
{ 
   DrawWidget::toggle_active();
        
   // Addy added this: if the toggled plane is not too
   // reasonable to draw on, switch to a better one
   if (is_active()) {
      // line from cam from to axis origin:
      Wvec view_vec = Wvec(XYpt(_origin)).normalized();
        
      const double max_dot_prod = 0.3;
      if (fabs(view_vec*_y) < max_dot_prod) {
         // need to switch
         double dotx = fabs(view_vec*_x);
         double doty = fabs(view_vec*_z);
         if (dotx>doty) set(_z, _x);
         else           set(_x, _z);
      }
   }
}

int  
Cursor3D::tap_cb(CGESTUREptr& gest, DrawState*&)
{
        
   err_msg("Cursor3D::tap_cb");

   switch (intersect(PIXEL(gest->center()))) {
    case INTERSECT_ORIGIN:
      toggle_active();
      break;
    case INTERSECT_X:
      _selected = X_AXIS;
      break;
    case INTERSECT_Y:
      _selected = Y_AXIS;
      break;
    case INTERSECT_Z:
      _selected = Z_AXIS;
      break;
    default:
      deactivate();
      // The tap missed the axis. It may have affected the
      // axis (deactivating it), but we return 0 anyway so
      // the tap can be consumed elsewhere.
      return 0; 
   }
   return 1; // this means we used up the gesture
}

int  
Cursor3D::scribble_cb(CGESTUREptr& gest, DrawState*&)
{
   cerr << "Cursor3D::scribble_cb" << endl;

   switch (intersect(PIXEL(gest->center()))) {
    case INTERSECT_NONE:
      cerr << "no intersect" << endl;
      return 0; // we're not using the gesture
    case INTERSECT_FRAME_CORNER:
    case INTERSECT_FRAME_Z_SIDE:
    case INTERSECT_FRAME_X_SIDE:
      toggle_active();
    brdefault:
      // XXX -- need to handle undo
      WORLD::undisplay(this, false);
   }

   return 1; // this means we used up the gesture
}

void  
Cursor3D::get_x(CXYpt&) 
{
   // XXX - handle as gesture

   // reset axes

   _x = Wvec(1,0,0);;
   _y = Wvec(0,1,0);
   _z = Wvec(0,0,1);

   // should be able to undo
}

int
Cursor3D::interactive(CEvent &e, State *&s, RAYhit*) const
{
   if (intersect(PIXEL(DEVice_2d::last->cur())) != INTERSECT_NONE &&
       _entry.consumes(e)) {
      s = _entry.event(e);
      return true;
   }
   return 0;
}
 
//! return scale value to use for making entire
//! axis larger when the window is larger...
double
Cursor3D::view_scale() const
{
   int w, h;
   VIEW::peek_size(w,h);
   int min_dim = min(w,h);
   return ((min_dim > 512) ? min_dim/512.0 : 1.0);
}


double
Cursor3D::axis_scale() const
{
   return view_scale() * world_length(_origin, CURSOR3D_LEN);
}

bool
Cursor3D::get_shadow(Wpt& shadow) const
{
   FLOORptr f = FLOOR::lookup();
   if (!f) {
      err_adv(debug, "Cursor3D::get_shadow: no floor");
      return false;
   }

   Wpt  o = f->o();
   Wvec n = f->n();
   double d = (_origin - o)*n;
   if (d <= 0) {
      err_adv(debug, "Cursor3D::get_shadow: cursor is below floor");
      return false;
   }

   Wpt s = _origin - n*d;

   // check relative position within floor:
   UVpt uv;
   assert(f->get_map());
   if (!f->get_map()->invert(s, uv, uv)) {
      assert(0); // should never fail
   }
   if (uv[0] < 0 || uv[0] > 1 ||
       uv[1] < 0 || uv[1] > 1) {
      if (debug) {
         cerr << "Cursor3D::get_shadow: shadow is outside floor: "
              << uv
              << endl;
      }
      return false;
   }

   const double MIN_PIX_DIST = 10;
   if (PIXEL(_origin).dist(shadow) < MIN_PIX_DIST) {
      err_adv(0 && debug, "Cursor3D::get_shadow: shadow too close to cursor");
      return false;
   }
   shadow = s;
   return true;
}

void
Cursor3D::get_plane_corners(Wpt& a, Wpt& b, Wpt& c, Wpt& d) const
{
   // selected axis pointing at camera
   //
   //          X/Z:               Y:
   //    d------------c    d------------c                           
   //    |            |    |            |
   //    |            |    |      |     |
   //    |            |    |     -o-    |                                 
   //    |            |    |      |     |
   //    |            |    |            |
   //    a------|-----b    a------------b
   //           o                                                     

   Wvec du, dv;
   switch(_selected) {
    case X_AXIS:
      a  =  _origin + _z*_s[2]/2;
      du = -_z*_s[2];
      dv =  _y*_s[1];
      break;
    case Y_AXIS:
      a  =  _origin - _x*_s[0]/2 + _z*_s[2]/2;
      du =  _x*_s[0];
      dv = -_z*_s[2];
      break;
    case Z_AXIS:
      a  = _origin - _x*_s[0]/2;
      du = _x*_s[0];
      dv = _y*_s[1];
      break;
    default:
      assert(0);
   }
   b = a + du;
   c = b + dv;
   d = a + dv;
}

int
Cursor3D::draw(CVIEWptr &v)
{
   if (!_origin.in_frustum())
      return 0;

   double   s = view_scale();
   double len = axis_scale();

   GLfloat w = GLfloat(v->line_scale()*2*s);
   GL_VIEW::init_line_smooth(w, GL_CURRENT_BIT);// calls glPushAttrib
   glDisable(GL_LIGHTING);                      // GL_ENABLE_BIT
   glDisable(GL_CULL_FACE);                     // GL_ENABLE_BIT

   glBegin(GL_LINES);

   // x
   glColor4fv(float4(axis_color(X_AXIS), alpha()));     // GL_CURRENT_BIT
   glVertex3dv(_origin.data());
   glVertex3dv((_origin + _x*len).data());

   // z
   glColor4fv(float4(axis_color(Z_AXIS), alpha()));     // GL_CURRENT_BIT
   glVertex3dv(_origin.data());
   glVertex3dv((_origin + _z*len).data());

   // y
   glColor4fv(float4(axis_color(Y_AXIS), alpha()));     // GL_CURRENT_BIT
   glVertex3dv(_origin.data());
   glVertex3dv((_origin + _y*len).data());

   // box
   if (_show_box) {
      glColor4fv(float4(box_color(),alpha()));

      Wtransf B =
         Wtransf::translation(_origin) *
         Wtransf(_x*_s[0], _y*_s[1], _z*_s[2]);

      // bottom xz plane
      glVertex3dv((B*Wpt(-1, 0,-1)).data());
      glVertex3dv((B*Wpt(-1, 0, 1)).data());

      glVertex3dv((B*Wpt( 1, 0,-1)).data());
      glVertex3dv((B*Wpt( 1, 0, 1)).data());

      glVertex3dv((B*Wpt(-1, 0,-1)).data());
      glVertex3dv((B*Wpt( 1, 0,-1)).data());

      glVertex3dv((B*Wpt(-1, 0, 1)).data());
      glVertex3dv((B*Wpt( 1, 0, 1)).data());

      // top xz plane
      glVertex3dv((B*Wpt(-1, 2,-1)).data());
      glVertex3dv((B*Wpt(-1, 2, 1)).data());

      glVertex3dv((B*Wpt( 1, 2,-1)).data());
      glVertex3dv((B*Wpt( 1, 2, 1)).data());

      glVertex3dv((B*Wpt(-1, 2,-1)).data());
      glVertex3dv((B*Wpt( 1, 2,-1)).data());

      glVertex3dv((B*Wpt(-1, 2, 1)).data());
      glVertex3dv((B*Wpt( 1, 2, 1)).data());

      // edges parallel to y
      glVertex3dv((B*Wpt(-1, 0,-1)).data());
      glVertex3dv((B*Wpt(-1, 2,-1)).data());

      glVertex3dv((B*Wpt(-1, 0, 1)).data());
      glVertex3dv((B*Wpt(-1, 2, 1)).data());

      glVertex3dv((B*Wpt( 1, 0,-1)).data());
      glVertex3dv((B*Wpt( 1, 2,-1)).data());

      glVertex3dv((B*Wpt( 1, 0, 1)).data());
      glVertex3dv((B*Wpt( 1, 2, 1)).data());
   }

   glEnd(); // GL_LINES

   // shadow
   Wpt shadow;
   if (get_shadow(shadow)) {
      w = GLfloat(SHADOW_SIZE_PIX*s);
      GL_VIEW::init_point_smooth(w, GL_CURRENT_BIT);// calls glPushAttrib
      glColor4fv(float4(Color::firebrick,0.5*alpha()));
      glBegin(GL_POINTS); {
         glVertex3dv(shadow.data());
      } glEnd();
      GL_VIEW::end_point_smooth();
   }

   // draw frame...
   GL_VIEW::init_polygon_offset(0,-1);  // GL_ENABLE_BIT:
   if (is_active()) {
      GL_COL(plane_color(), .6*alpha());     // GL_CURRENT_BIT
      Wpt a, b, c, d;
      get_plane_corners(a,b,c,d);
      glBegin(GL_QUADS); {
         glVertex3dv(a.data());
         glVertex3dv(b.data());
         glVertex3dv(c.data());
         glVertex3dv(d.data());
      } glEnd();
   }
   GL_VIEW::end_polygon_offset();       // GL_ENABLE_BIT

   GL_VIEW::end_line_smooth();

   // when all faded away, undraw
   if (alpha() == 0)
      WORLD::undisplay(this, false);

   return 0;
}

BBOX
Cursor3D::bbox(int) const
{
   BBOX bbox;
   double len = axis_scale();

   // x
   bbox.update(_origin + _x*len);

   // y
   bbox.update(_origin + _y*len);

   // z
   bbox.update(_origin + _z*len);

   return bbox;
}


int
Cursor3D::down_cb(CEvent &, State *& s)
{
   _clock.set();
   _last_pt = DEVice_2d::last->cur();

   Wvec norm;
   switch (_down_intersect = intersect(_last_pt, _last_wpt, norm)) {
    case INTERSECT_ORIGIN:
      s = &_y_choose;
      break;
    case INTERSECT_SHADOW: {
      FLOORptr f = FLOOR::lookup();
      assert(f);
      _cur_axis = f->n();
      s = &_xlate_plane;
    }
      break;
    case INTERSECT_Y:
      s = (_selected == Y_AXIS) ? &_y_choose : &_rotate;
      break;
    case INTERSECT_X:
      s = (_selected == X_AXIS) ? &_y_choose : &_rotate;
      break;
    case INTERSECT_Z:
      s = (_selected == Z_AXIS) ? &_y_choose : &_rotate;
      break;
    case INTERSECT_FRAME_CORNER:
      s = &_frame_resize;
      break;
    case INTERSECT_FRAME_Z_SIDE:
      // when attached to an object,
      // enforce uniform scaling:
      s = _attached ? &_frame_resize : &_frame_resize_z;
      break;
    case INTERSECT_FRAME_X_SIDE:
      s = _attached ? &_frame_resize : &_frame_resize_x;
      break;
    default:
      ;
   }
   if (_down_intersect != INTERSECT_NONE) {
      // use an xform command so it is undoable:
      _cmd = new XFORM_CMD(this);
      assert(_cmd);
      if (_attached)
         _cmd->add(_attached);
      WORLD::add_command(_cmd);
   }
   return 0;
}

int
Cursor3D::up_cb(CEvent &, State *&)
{
   _cmd = 0;
   return 0;
}


/*! 
  based on initial motion, decide whether
  to constrain motion to the selected axis or the plane
  perpendicular to the selected axis.

  if the plane is nearly edge-on don't
  prefer constraining in plane

  if axis is nearly tip-on don't prefer
  motion along it
*/
int
Cursor3D::y_choose_cb(CEvent &, State *& s)
{
   // "y" really means selected axis

   PIXEL cur = DEVice_2d::last->cur();

   if (_last_pt.dist(cur) <= 6)
      return 0;

   _last_pt = cur;

   Wpt eye = VIEW::peek()->cam()->data()->from();
   double a = rad2deg(line_angle(eye-_origin,sel()));
   Wplane P = Wplane(_last_wpt, sel());
   Wline  L = Wline (_last_wpt, sel());
   Wline  V = Wline(cur);
   if (a < 10) {
      // axis is forshortened
      // constrain motion to plane
      _last_wpt = P.intersect(V);
      s = &_xlate_plane;
   } else if (a > 80) {
      // plane is forshortened
      _last_wpt = L.intersect(V);
      s = &_xlate_axis;
   } else {
      // check angle in film plane
      if (rad2deg(line_angle(_last_pt - PIXEL(_last_wpt), film_sel())) < 12) {
         // moving along axis
         _last_wpt = L.intersect(V);
         s = &_xlate_axis;
      } else {
         // moving cross-ways to axis
         _last_wpt = P.intersect(V);
         s = &_xlate_plane;
      }
   }
   _cur_axis = sel();

   return 0;
}


int
Cursor3D::xlate_axis_cb(CEvent& e, State *&)
{
   PIXEL cur = DEVice_2d::last->cur();

   Wvec delt = Wline(_origin,_cur_axis).intersect(Wline(cur)) - _last_wpt;

   if ((_origin + delt).in_frustum()) {
      _last_pt   = cur;
      _last_wpt += delt;
      set_origin(_origin + delt);
   }

   return 0;
}

int
Cursor3D::xlate_plane_cb(CEvent &e, State *&)
{
   PIXEL cur = DEVice_2d::last->cur();

   // project cursor to current plane,
   // move origin there:
   Wvec delt = Wplane(_last_wpt,_cur_axis).intersect(Wline(cur)) - _last_wpt;

   if ((_origin + delt).in_frustum()) {
      _last_pt   = cur;
      _last_wpt += delt;
      set_origin(_origin + delt);
   }

   return 0;
}

int
Cursor3D::resize_z_cb(CEvent &, State *&)
{
   // "z" means selected plus 1

   Wpt curr_pt = get_plane().intersect(view_ray());
   int k = (sel_i()+1)%3;
   _s[k] = fabs((curr_pt - _origin) * ax(k));

   return 0;
}

int
Cursor3D::resize_x_cb(CEvent &, State *&)
{
   // "x" means selected minus 1

   Wpt curr_pt = get_plane().intersect(view_ray());

   int k = (sel_i()+2)%3;
   _s[k] = fabs((curr_pt - _origin) * ax(k));

   return 0;
}

int
Cursor3D::resize_uni_cb(CEvent &, State *&)
{
   Wpt curr_pt = get_plane().intersect(view_ray());
   Wpt last_pt = get_plane().intersect(Wline(_last_pt));

   double s = (_origin.dist(curr_pt) / _origin.dist(last_pt));
   _s *= s;

   _last_pt = DEVice_2d::last->cur();

   assert(_cmd);
   _cmd->concatenate_xf(Wtransf::scaling(_origin,s));

   return 0;
}

//! always rotate around selected axis
//! intersected axis tracks the mouse
int
Cursor3D::rotate_cb(CEvent &, State *&)
{
   if (debug)
      cerr << "Cursor3D::rotate_cb" << endl;

   Wtransf R;
   Wpt p = get_plane().intersect(view_ray());
   switch (_down_intersect) {
    case INTERSECT_X:
      assert(_selected != X_AXIS);
      R = Wtransf::rotation(Wquat(_x, (p - _origin).normalized()));
      break;
    case INTERSECT_Y:
      assert(_selected != Y_AXIS);
      R = Wtransf::rotation(Wquat(_y, (p - _origin).normalized()));
      break;
    case INTERSECT_Z:
      assert(_selected != Z_AXIS);
      R = Wtransf::rotation(Wquat(_z, (p - _origin).normalized()));
      break;
    default:
      cerr << "Cursor3D::rotate_cb: error: unexpected intersect" << endl;
   }

   assert(_cmd);
   _cmd->concatenate_xf(
      Wtransf::translation(_origin) *
      R *
      Wtransf::translation(Wpt::Origin() - _origin)
      );

   return 0;
}

int
Cursor3D::get_cam_tap(CXYpt& xy)
{
   switch (intersect(PIXEL(xy))) {
    case INTERSECT_Y: cam_focus(_y); return 1; break;
    case INTERSECT_X: cam_focus(_x); return 1; break;
    case INTERSECT_Z: cam_focus(_z); return 1; break;
    default:                         return 0;
   }
}

void
Cursor3D::cam_focus(CWvec& axis)
{
   // not used... i think

   VIEWptr view = VIEW::peek();
   CAMptr   cam = view->cam();

   double dist = cam->data()->from().dist(_origin);

   Wvec up(0,1,0);
   if (axis * up > .99) {
      up = cross(cross(axis, cam->data()->at_v()), axis);
      if (up.is_null())
         return;   // already aligned w/ axis
   }

   // make a new one
   Wpt from = _origin + axis*dist;
   new CamFocus(
      view,
      from,
      _origin,
      from + up,
      _origin,
      cam->data()->width(),
      cam->data()->height()
      );
}

bool
Cursor3D::do_cam_focus(CVIEWptr& view, CRAYhit& r)
{
   if (Config::get_var_bool("DEBUG_CAM_FOCUS",false))
      cerr << "Cursor3D::do_cam_focus" << endl;

   assert(r.geom() == this);

   CAMdataptr data(view->cam()->data());

   Wpt     o = data->from();                       // old camera location
   Wpt     c = r.surf();                           // new "center" point
   Wvec    n = r.norm();                           // surface normal

   // ensure normal points toward us:
   if ((o - c) * n < 0)
      n = -n;

   Wpt     a = c;                                  // new "at" point
   Wpt     f = c + n * o.dist(c);                  // new "from" point
   Wtransf R = Wtransf::anchor_scale_rot(c, o, f); // rotation for old --> new
   Wvec    u = (R * (data->pup_v())).normalized(); // new "up" vector

   // value that is 0 when up vector is nearly parallel to Wvec::Y(),
   // 1 when the vectors are not almost parallel, with a smooth
   // transition between the two values:
   double d = smooth_step(M_PI/12, M_PI/6, line_angle(a - f,Wvec::Y()));

   // choose "up" to follow old up except when it aligns closely with Y:
   u = interp(u, Wvec::Y(), d);

   new CamFocus(view, f, a, f + u, c, data->width(), data->height());
   return true;
}

// end of file cursor3d.C
