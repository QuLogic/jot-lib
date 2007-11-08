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
 * crv_sketch.C
 *****************************************************************/

/*!
 *  \file crv_sketch.C
 *  \brief Contains the definition of the CRV_SKETCH widget.
 *
 *  \ingroup group_FFS
 *  \sa crv_sketch.H
 *
 */

#include "geom/gl_util.H"       // for GL_COL()
#include "geom/gl_view.H"       // for GL_VIEW::init_line_smooth()
#include "geom/world.H"         // for WORLD::undisplay()
#include "gtex/ref_image.H"     // for VisRefImage
#include "mesh/mi.H"            
#include "std/config.H" 

#include "crv_sketch.H"
#include "tess/tess_cmd.H"

using namespace mlib;

//! Convert a bcurve to a list of pixels (Current subdivision level)
// XXX -- is this really at "current subdivion levels"?  (mkowalsk)
static inline void convert_to_list(Bcurve* bcurve, PIXEL_list& list)
{
   Wpt_list pts = bcurve->get_wpts();
   
   list.clear();
   for (int i = 0; i<pts.num(); i++)
      list += bcurve->shadow_plane().project(pts[i]);
   list.update_length();
}

//! grabs wpts and normal vectors from a bcurve
void
CRV_SKETCH::cache_curve(Bcurve* bcurve)
{
   Wpt_list pts = bcurve->get_wpts();
   _shadow_pts.clear();
   _shadow_normals.clear();

   for (int i = 0; i<pts.num(); i++)
      _shadow_pts += bcurve->shadow_plane().project(pts[i]);
   _shadow_pts.update_length();

   for (int i = 0; i < _shadow_pts.num(); i++) {
      _shadow_normals += normal_at_shadow(i);
   }
}

/*****************************************************************
 * CRV_SKETCH
 *****************************************************************/
CRV_SKETCHptr CRV_SKETCH::_instance;

CRV_SKETCHptr
CRV_SKETCH::get_instance()
{
   if (!_instance)
      _instance = new CRV_SKETCH();
   return _instance;
}

CRV_SKETCH::CRV_SKETCH() :
   DrawWidget(),
   _curve(0),
   _mode(INACTIVE),
   _component(NONE),
   _init_stamp(0)
{
   //*******************************************************
   // FSA states for handling GESTUREs:
   //
   //   Note: the order matters. First matched will be executed.
   //*******************************************************
   _draw_start += DrawArc(new TapGuard,     drawCB(&CRV_SKETCH::tap_cb));
   _draw_start += DrawArc(new StrokeGuard,  drawCB(&CRV_SKETCH::stroke_cb));

   // Set up the clean up routine
   atexit(clean_on_exit);
}

void 
CRV_SKETCH::clean_on_exit() 
{ 
   _instance = 0; 
}


void
CRV_SKETCH::reset()
{
   // Stop observing the curve:
   if (_curve && _curve->geom())
      unobs_display(_curve->geom());

   // Clear cached data.

   _guidelines.clear(); // clear LIST of lines
   _curve = NULL;
   _points.clear();
   _mode = INACTIVE;
   _component = NONE;
   _avg_shadow_norm = Wvec();
   _initial_eye_loc = Wpt();
   _init_stamp = 0;
}

int
CRV_SKETCH::draw(CVIEWptr& v)
{
   static bool debug = Config::get_var_bool("DEBUG_CS_DRAW_SPANS",false);

   // recompute the shadow cusps and guidelines
   update();

   // Set the transparency
   _guidelines.set_alpha(alpha());

   // Draw the guidelines:
   int ret =  _guidelines.draw(v);

   if (_component == CURVE) {

      // sanity check
      assert(_curve);

         glMatrixMode(GL_MODELVIEW);
         glPushMatrix();
         glLoadIdentity();
      
         // set up to draw in PIXEL coords:
         glMatrixMode(GL_PROJECTION);
         glPushMatrix();
         glLoadMatrixd(VIEW::peek()->pix_proj().transpose().matrix());
      
         // Set up line drawing attributes
         glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
         glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
      
         // turn on antialiasing for width-2 lines:
         GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*2));
      
         // draw the line strip
            PIXEL_list pts = _shadow_pixels;
            glBegin(GL_LINE_STRIP);
            for (int k=0; k< pts.num(); k++) {
               glColor3d(0.3, 0.3, 0.3);      // GL_CURRENT_BIT
               glVertex2dv(pts[k].data());
            }
            glEnd();
      
         GL_VIEW::end_line_smooth();

         glPopAttrib();
         glPopMatrix();
         glMatrixMode(GL_MODELVIEW);
         glPopMatrix();

      if (debug) {
         cerr << "debug" << endl;
         this->update();
         ARRAY<PIXEL_list> pl;
         // this will only work when there are drawn pixels
         match_spans(pl,
                     _drawn_pixels,
                     _shadow_cusp_list,
                     _shadow_pts,
                     _shadow_pixels,
                     _shadow_normals);

         glMatrixMode(GL_MODELVIEW);
         glPushMatrix();
         glLoadIdentity();
      
         // set up to draw in PIXEL coords:
         glMatrixMode(GL_PROJECTION);
         glPushMatrix();
         glLoadMatrixd(VIEW::peek()->pix_proj().transpose().matrix());
      
         // Set up line drawing attributes
         glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
         glDisable(GL_LIGHTING);              // GL_ENABLE_BIT
      
         // turn on antialiasing for width-2 lines:
         GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*2));
      
         // draw the line strip
         for (int i = 0; i < pl.num(); i++) {
            PIXEL_list pts = pl[i];
            glBegin(GL_LINE_STRIP);
            for (int k=0; k< pts.num(); k++) {
               glColor3d(0.3, 0.3, 0.3);      // GL_CURRENT_BIT
               glVertex2dv(pts[k].data());
            }
            glEnd();
         }
      
         GL_VIEW::end_line_smooth();

         glPopAttrib();
         glPopMatrix();
         glMatrixMode(GL_MODELVIEW);
         glPopMatrix();
      }
   }

   // When all faded away, undraw
   if (alpha() == 0)
      WORLD::undisplay(this, false);
   return ret;
}


bool
CRV_SKETCH::create_shadow(Bpoint* p)
{
   bool debug = false;
   if (!p) {
      err_adv(debug, "CRV_SKETCH::create_shadow: NULL point");
      return false;
   }
   if (p->has_shadow()) {
      err_adv(debug, "CRV_SKETCH::create_shadow: already has shadow");
      return true;
   }

   // XXX - set shadow plane here
   return true;
}

bool
CRV_SKETCH::grow_shadows(Bpoint* p)
{
   bool debug = false;
   if (!p) {
      err_adv(debug, "CRV_SKETCH::grow_shadows: NULL point");
      return false;
   }

   if (!create_shadow(p)) {
      err_adv(debug, "CRV_SKETCH::grow_shadows: create_shadow failed");
      return false;
   }

   // get all the connected curves
   Bcurve_list cl = Bcurve_list::reachable_curves(p);

   for (int i=0; i<cl.num(); i++) {

      assert(cl[i]);

      // add shadows to first endpoint of curve
      if ( !create_shadow(cl[i]->b1()) ) {
         cerr << "shadow creation failed for b1 of curve "
              << i << " returning" << endl;
         return false;
      }

      // add shadows to second endpoint of curve
      if ( !create_shadow(cl[i]->b2())) {
         cerr << "shadow creation failed for b2 of curve "
              << i << " returning" << endl;
         return false;
      }

      if ( !cl[i]->has_shadow() ) {
         // add shadow to the curve itself
//          Map2D3D* surf = cl[i]->constraining_surface();
//          if ( !cl[i]->try_create_shadow(surf) ) {
//             cerr << "shadow creation failed for curve "
//                  << i << " returning" << endl;
//             return false;
//          }
      }
   }
   return true;
}

bool
CRV_SKETCH::move_point(CNDCpt& p) 
{
   // if screen point p is near a guideline, move the
   // point associated with the guideline to the point 
   // along the guidline nearest to p
   // (only works if current component type is POINT)

   if ( _component!=POINT ) {
      return false;
   }

   if ( _mode==INACTIVE ) {
      cerr << "inactive mode, returning" << endl;
      return false;
   }

   // sanity check: there should be a one-to-one correspondnece between
   // guidelines and points

   if (_guidelines.num() != _points.num()) {
      cerr << "error: numbers of guidelines and points differ" << endl;
      return false;
   }

   const int PIX_THRESH = 10;

   // create a world line from eye to screen point 
   Wpt wp = XYpt(p); // conversion from screen to world currently requires XYpt?
   Wline view_line(VIEW::eye(), wp);

   PIXEL pix_pt(p);

   for (int i=0; i<_guidelines.num(); i++) {

      // is p within PIX_THRESH distance to the current guideline?
     
      // sanity check
      if (_guidelines[i]->num() != 2) {
         cerr << "ERROR: line " << i 
              << " does not have exactly 2 points" << endl;
         continue;
      }

      // create world line for guideline
      Wline cur_guideline(_guidelines[i]->point(0),
                          _guidelines[i]->point(1));
     
      // find the closest point along the guideline to the view line 
      Wpt closest(cur_guideline, view_line);

      PIXEL closest_pix(closest);

      if (closest_pix.dist(pix_pt) <= PIX_THRESH) {
         // p is sufficiently close to the guideline

         // find the point associated with the guideline

         Bpoint* bp = _points[i];
         if (!bp) {
            cerr << "ERROR: null point" << endl;
            return false;
         }

         if (_mode == SHADOW && !bp->has_shadow()) {
            if ( !grow_shadows(bp) ) {
               return false;
            } 
         } 

         bp->move_to(closest);

         return true;
      }
   }
  
   // if we got here, we couldn't find a guideline sufficiently close to p
   return false;
}

int  
CRV_SKETCH::tap_cb(CGESTUREptr& g, DrawState*& s)
{
   if ( move_point(g->start()) ) {
      return 1;
   }

   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we DID use up the gesture
}

int  
CRV_SKETCH::stroke_cb(CGESTUREptr& gest , DrawState*&) 
{
   if (!(_curve && _component == CURVE))
      return 1;

   static bool debug = Config::get_var_bool("DEBUG_CRV_SKETCH_STROKE_CB",false);
   if (debug) cerr << "in crv sketch stroke cb" << endl;

   _drawn_pixels = gest->pts();
   Wpt_list result; 
   this->update();
   convert_to_list(_curve, _shadow_pixels);
   cache_curve(_curve);

   SurfaceCurveMap* o = (SurfaceCurveMap*)_curve->map();
            UVpt_list o_uvp = o->uvs();
            UVpt_list uvp;
            for (int i = 0; i < _shadow_pts.num(); i++) {
               double t = _shadow_pts.partial_length(i) / _shadow_pts.length();
               double h = (o->map(t) - _shadow_pts[i]) * _curve->shadow_plane().normal();
               uvp += UVpt(t, h);
               if (debug) cerr << "t, h: <" << t << ", " << h << ">" << endl;
            }
            uvp.update_length();
   o->set_uvs(uvp);

   PIXEL_list upperc;
   _curve->get_pixels(upperc);
   if (re_match(_drawn_pixels,
                upperc,
                result,
                _shadow_cusp_list,
                _shadow_pts,
                _shadow_pixels,
                _shadow_normals)) {
         
   } else {
      o->set_uvs(o_uvp);
      err_adv(debug, "CRV_SKETCH::stroke_cb: re_match upper curve failed");
      if (!oversketch_shadow(_drawn_pixels)) {
         err_adv(debug, "CRV_SKETCH::stroke_cb: oversketch shadow failed");
         return 1;
      } else {
         reset_timeout();
         return 1;
      }
   }

   // we already have an upper so just specify its new position
   specify_upper(result);
   reset_timeout();

   return 1;
}

bool
CRV_SKETCH::specify_upper(CWpt_list& list) 
{
   //return false;

   static bool debug = Config::get_var_bool("DEBUG_SPECIFY_UPPER",false);

   Bcurve* upp = _curve; 
   assert(upp);

   UVpt_list uvp;
   // make sure list from has the same number of points as the shadow
   // curve.
   
   assert (list.num() == _shadow_pts.num());
   _shadow_pts.update_length();

   ARRAY<double> shadow_tvals = _curve->get_map_tvals();
   assert(shadow_tvals.num() == _shadow_pts.num());
   
   for (int i = 0; i < list.num(); i++) {
      double t = shadow_tvals[i];
      double h = (list[i] - _shadow_pts[i]) * _curve->shadow_plane().normal();
      if (debug) {
         cerr << "got t,h pair " <<t << ",  " << h << " wpt = " << list[i] << endl;
      }
      uvp += UVpt(t, h);
   }
   
   uvp.update_length();
   
   /*// set the bpoint end points first, so surfacecurvemap::recompute
   // doesn't thing the bpoints moved when we set_uvs later.
   if (upp->b1()) {
      if (debug) cerr << "Moving first bpoint endpoint" << endl;
      upp->b1()->move_to(list.first());
   }

   if (upp->b2()) {
      if (debug) cerr << "Moving second bpoint endpoint" << endl;
      upp->b2()->move_to(list.last());
   }*/

   // then set the new uv point list
   assert(SurfaceCurveMap::isa(upp->map()));
   CRV_SKETCH_CMDptr  cmd= new CRV_SKETCH_CMD(upp,uvp, list.first(), list.last());
   WORLD::add_command(cmd);
   //((SurfaceCurveMap*)(upp->map()))->set_uvs(uvp);
   
   this->update();
   return true;

}

//! Returns true if the gesture is valid for beginning a crv_sketch
//! session. It also caches info like what object will be
//! swept, so that a subsequent call to CRV_SKETCH::go() will
//! activate the widget for handling the crv_sketch session.
bool
CRV_SKETCH::init(CGESTUREptr& g)
{
   
   static bool debug = Config::get_var_bool("DEBUG_CRV_SKETCH_INIT",false);

   err_adv(debug, "CRV_SKETCH::init");

   CRV_SKETCHptr me = get_instance();

   // Should only be called when no CRV_SKETCH is active:
   if (me->is_active()) {
      return false;
   }

   // Get ready to start fresh (clear cached info):
   me->reset();

   mode_t temp_mode = INACTIVE;

   if (g->is_dot()) {
      NDCpt c = g->center();
      Bedge* e = VisRefImage::get_edge(c, 6);
      double radius = VIEW::pix_to_ndc_scale() * 6;
      if (e) {
         cerr << c << endl;
         if (c.dist(e->v1()->loc()) <= radius ||
            c.dist(e->v2()->loc()) <= radius)
            return false;
      }
      err_adv(debug, "stereo mode");
      temp_mode = STEREO;
   } else if (g->is_slash()) {
      // XXX - probably a slower line should be okay too
      err_adv(debug, "shadow mode");
      temp_mode = SHADOW;
   } else {
      err_adv(debug, "gesture is not a dot or slash");
      return false;
   }

   component_t temp_component = NONE;
   if ( me->init_point(g->start(), temp_mode) ) {
      temp_component = POINT;
      err_adv(debug, "point found");
   }
   else if ( me->init_curve(g->start(), temp_mode) ) {
      err_adv(debug, "curve found");
      temp_component = CURVE;
   } else {
      err_adv(debug, "gesture does not begin near point or curve");
      return false;
   }

   if (temp_mode == SHADOW) {

      // Test whether the gesture lines up with the plane normal
      // direction of the curve or point
      Wvec test_norm;

      if (temp_component == CURVE) {
         // use the average normal between the two endpoints of the bcurve
         Wvec n1 = me->_shadow_map->norm(0);
         Wvec n2 = me->_shadow_map->norm(1);
         test_norm = ((n1 + n2) / 2).normalized();
      } else if (temp_component == POINT){
         // XXX -- average normals in some way here
         test_norm = me->_points[0]->norm(); 
      }

      Wpt wp = XYpt(g->start());
      VEXEL nvex(wp, test_norm);
      VEXEL gvex = g->endpt_vec().normalized();
      if (nvex.is_null() || gvex.is_null()) {
         err_adv(debug, "can't get screen projection of normal vectors");
         me->reset();
         return false;
      }

      // Require the angle between them be less than 15 degrees.
      // I.e., cos(theta) > cos(15):
      const double CRV_SKETCH_ANGLE_THRESH = 15;

      double angle = line_angle(nvex, gvex);

      if (rad2deg(angle) > CRV_SKETCH_ANGLE_THRESH) {
         // angle too big
         err_adv(debug, "screen angle too great (%3.0f > % 3.0f)",
                 rad2deg(angle), CRV_SKETCH_ANGLE_THRESH);
         me->reset();
         return false;
      }

      // once we're here we've accepted the curve or point

      if (temp_component == CURVE) {

         
         if (!SurfaceCurveMap::isa(me->_curve->map())) {

            Map2D3D* surface = new RuledSurfCurveVec(me->_shadow_map);
            UVpt_list uvp;
            for (int i = 0; i < me->_shadow_pts.num(); i++) {
               double t = me->_shadow_pts.partial_length(i) / me->_shadow_pts.length();
               double h = 0.0;
               uvp += UVpt(t, h);
               //if (debug) cerr << "norm: " << me->_curve->map()->norm(t) << endl;
            }
            uvp.update_length();

            //Map0D3D* p0 = me->_shadow_map->p0();
            //Map0D3D* p1 = me->_shadow_map->p1();


            Map1D3D* new_map;
            if (!me->_shadow_pts.is_closed()) {
               Map0D3D* b1 = me->_curve->b1()->map();
               Map0D3D* b2 = me->_curve->b2()->map();
               new_map = new SurfaceCurveMap(surface, uvp, b1, b2);
            } else {
               new_map = new SurfaceCurveMap(surface, uvp, 0, 0);
            }
            cerr << "can reach here" << endl;
            assert(new_map);
            me->_curve->set_map(new_map);
         } else {
            ((RuledSurfCurveVec*)(((SurfaceCurveMap*)(me->_curve->map()))->surface()))
               ->set_curve(me->_shadow_map);
         }

         convert_to_list(me->_curve,me->_shadow_pixels);
         me->cache_curve(me->_curve);
      }

   } else {
      me->_initial_eye_loc = VIEW::eye();
   }

   // Record the frame number and modes
   me->_init_stamp = VIEW::stamp();
   me->_mode = temp_mode;
   me->_component = temp_component;

   // from shadow_match constructor
   // ??? mak
   me->update();

   err_adv(debug, "CRV_SKETCH:init: returning successfully.. ");

   // inform user of new state

   str_ptr msg;

   if (me->_mode == SHADOW) {
      msg = "Shadow sketch ";
      cerr<< "Shadow sketch "<<endl;
   } else if (me->_mode == STEREO) {
      msg = "Stereo sketch ";
      cerr<< "Stereo sketch "<<endl;
   }

   if (me->_component == POINT) {
      msg = msg + "for points";
      cerr<<"for points" <<endl;
   } else if (me->_component == CURVE) {
      msg = msg + "for curves";
      cerr<< "for curves"<<endl;
   }

   WORLD::message(msg);

   return go();
}

void 
CRV_SKETCH::set_shadow_map()
{
   _shadow_pts.clear();
   Wpt_list wpts = _curve->get_wpts();
   for (int i = 0; i < wpts.num(); i++)
      _shadow_pts += _curve->shadow_plane().project(wpts[i]);
   _shadow_pts.update_length();

   if (!_shadow_pts.is_closed())
      _shadow_map = new Wpt_listMap(_shadow_pts, new WptMap(_shadow_pts[0]),
         new WptMap(_shadow_pts.last()), _curve->shadow_plane().normal());
   else
      _shadow_map = new Wpt_listMap(_shadow_pts, 0, 0, _curve->shadow_plane().normal());
}

bool
CRV_SKETCH::init_curve(CNDCpt& p, mode_t mode, int rad)
{
   // The gesture had to start near a Bcurve:
   const double SCREEN_RAD = 10;        // search radius
   Wpt hit;                             // intersect point
   Bedge* e = NULL;                     // intersect edge
   Bcurve* curve = Bcurve::hit_curve(p, SCREEN_RAD, hit, &e);

   if (!curve) {
      return false;
   } else if (!Wpt_listMap::isa(curve->map()) && !SurfaceCurveMap::isa(curve->map())) {
      return false;
   } else if (SurfaceCurveMap::isa(curve->map()) && 
      !RuledSurfCurveVec::isa(((SurfaceCurveMap*)curve->map())->surface())) {
      return false;
   }

   if ( curve->mesh() ) {
      cerr << "CRV_SKETCH::init: have curve mesh" << endl;
   }

   // we only deal with control curves   
   _curve = curve->ctrl_curve();

   if (_curve->shadow_plane().normal() == Wvec(0,0,0))
      _curve->set_shadow_plane(_curve->plane());
   set_shadow_map();

   /*
     3 cases: 
     -curve is a shadow
     -curve has a shadow 
     -curve neither has a shadow nor is a shadow, it must be
     constrained in a surface
   */
/*
   if (curve->is_shadow()) {
      _curve = curve;
   } else if (curve->has_shadow()) {
      _curve = curve->shadow();
   } else {
      // we check that this shadow is constrained in a surface
      _curve = curve;
      _curve_surf = _curve->constraining_surface(); 
      if (!_curve->is_closed()) {
         assert(_curve->b1());
         assert(_curve->b2());
      }
   } 
*/
   return true;
}

bool
CRV_SKETCH::init_point(CNDCpt& screen_pt, mode_t mode, int rad)
{
   Bpoint* p = Bpoint::hit_point(screen_pt, rad);

   if (!p) {
      return false;
   }
/*
   if ( p->is_shadow() ) {
      p = p->upper();
      if (!p) {
         cerr << "CRV_SKETCH::init_point(), failed to get upper" << endl;
         return false;
      }
   }
*/
   // XXX -- should check if more than one mesh is involved?
   if ( p->mesh() ) {
      // XXX -- once the widgets become mesh observers
      // should subscribe to observe mesh here
      // and also unsubscribe in reset()
   }

   _points.clear();
   _points = Bpoint_list::reachable_points(p);

   if ( mode == STEREO ) {
      int i=0; // loop index
      // get all the connected curves
      Bcurve_list cl = Bcurve_list::reachable_curves(p); 

      // remove shadows, if any, from the curves
      for ( i=0; i<cl.num(); i++) {
         Bcurve* c = cl[i];
         assert(c);
         c->remove_shadow();
      }

      // remove shadows, if any, from the points
      // (should go here, after curve shadows are removed, due to
      // Bnode dependencies in corresponding maps)
      for ( i=0; i<_points.num(); i++ ) {
         if (_points[i]->has_shadow() || 
             _points[i]->constraining_surface()) {
            _points[i]->remove_constraining_surface();
         }
      }

      for ( i=0; i<cl.num(); i++) {
         Bcurve* c = cl[i];
         assert(c);
         if ( !Wpt_listMap::isa(c->map()) ) {
            // maps on curve's endpoints b1 and b2 have been updated
            // above and will be used to create map here
            Map1D3D* new_map = new Wpt_listMap(c->get_wpts(),
                                               c->b1()->map(),
                                               c->b2()->map(),
                                               c->map()->norm());
            assert(new_map);  // be sure allocation worked
            c->set_map(new_map);
         }
      }
   }

   return true;
}


bool
CRV_SKETCH::go(double)
{
   // After a successful call to init(), this turns on a CRV_SKETCH
   // to handle the crv_sketch session:

   CRV_SKETCHptr me = get_instance();

   // Should be good to go:
   if (me->_mode == INACTIVE ||
       me->_component == NONE || 
       me->_init_stamp != VIEW::stamp())
      return 0;

   // Set up to observe when the curve is undisplayed:
   // XXX -- add a similar call to monitor if points get undisplayed
   if (me->_component == CURVE && me->_curve->geom())
      me->disp_obs(me->_curve->geom());

   // Set the timeout duration
   me->reset_timeout();

   // Become the active widget and get in the world's DRAWN list:
   me->activate();

   return true;
}


void
set_guideline_endpoints(CWpt& p, CWvec& n, LINE3Dptr line)
{
   if (!line) {
      cerr << "set_guideline_endpoints(): ERROR, null line" << endl;
      return;
   }
  
   line->clear();

   // XXX fix this to be something intelligent
   const double scale = 100.0;
   
   line->add(p - n * scale);
   line->add(p + n * scale);

}

StylizedLine3Dptr
create_new_guideline() 
{
   StylizedLine3Dptr ret = new StylizedLine3D(2);
   assert(ret);
   ret->set_width(1.0);
   ret->set_color(COLOR(0.8, 0.0, 0.0));
   ret->set_do_stipple(true);
   return ret;
}


void
CRV_SKETCH::build_guidelines(CWpt_list& line_locs,
                             CWvec& n,                // projection direction
                             LINE3D_list& guidelines // the result
   )
{
   guidelines.clear();

   // build new lines at every cusp point
   for (int i = 0 ;i<line_locs.num();i++) {      

      // If pool is exhausted, create a new line
      if ( i == _line_pool.num() ) {
         _line_pool += create_new_guideline();
      }
      // Grab a line from the pool
      guidelines += _line_pool[i];

      set_guideline_endpoints(line_locs[i], n, guidelines[i]);
   }
}


void
CRV_SKETCH::build_guidelines(CWpt_list& line_locs,
                             CWpt& focus,
                             LINE3D_list& guidelines // the result
   )
{
   guidelines.clear();

   // build new lines at every cusp point
   for (int i = 0 ;i<line_locs.num(); i++) {      

      // If pool is exhausted, create a new line
      if ( i == _line_pool.num() ) {
         _line_pool += create_new_guideline();
      }
      // Grab a line from the pool
      guidelines += _line_pool[i];

      Wvec dir = (line_locs[i] - focus).normalized();
      set_guideline_endpoints(line_locs[i], dir, guidelines[i]);
   }
}

StylizedLine3Dptr
CRV_SKETCH::new_guideline(CWpt& p, CWvec& n) const {

   // XXX fix this to be something intelligent
   const double scale = 100.0;
   
   StylizedLine3Dptr ret = new StylizedLine3D(2);
   assert(ret) ;
   ret->add(p - n * scale);
   ret->add(p + n * scale);
   ret->set_width(1.0);
   ret->set_color(COLOR(0.8, 0.0, 0.0));
   ret->set_do_stipple(true);
   return ret;
}


// Updates the shadow cusp list and the guidelines for current view
void 
CRV_SKETCH::update(void)
{
   static bool debug = Config::get_var_bool("DEBUG_CRV_SKETCH_UPDATE",false);

   err_adv(debug, " -- In CRV_SKETCH:: udpate "); 

   if ( _component == POINT ) {
      err_adv(debug, "setting point guideline");
      if (_points.empty()) {
         err_adv(debug, "empty points, returning");
         return;
      }

      Wpt_list locs;
      //locs += _points[0]->loc();
      for (int j=0; j<_points.num(); j++) {
         locs += _points[j]->loc();
      }

      if ( _mode == SHADOW ) {
         // XXX -- should use something different for the norm
         // (an average? or just lookup the norm for every point?
         Wvec n= _points[0]->norm();
         build_guidelines(locs, n, _guidelines);
      } else if ( _mode == STEREO ) {
         build_guidelines(locs, _initial_eye_loc, _guidelines);
      }

   } else if ( _component == CURVE ) {
      // update list of shadow pixels since it is view dependent
      convert_to_list(_curve,_shadow_pixels);

      // clear the cusp list and rebuild
      _shadow_cusp_list.clear();

      // XXX -- we don't want shadow's world points, do we?
      //_shadow_pts = _shadow_map->pts();
      //Wpt_list shadow_pts = _curve->cur_subdiv_pts();
      Wpt_list shadow_pts = _shadow_map->pts();
      Wpt_list cusp_pts;  // XXX -- redundant with _shadow_cusp_list
   
      // first one is always a cusp
      _shadow_cusp_list += 0;
      cusp_pts += shadow_pts[0];
   

      for (int i = 1 ;i<shadow_pts.num() - 1;i++) {
         if (CRV_SKETCH::is_cusp(shadow_pts[i-1], 
                                 shadow_pts[i], 
                                 shadow_pts[i+1], 
                                 //Wvec(0,1,0)) // XXX -- replace with line below
             normal_at_shadow(i))
            ) {
            _shadow_cusp_list+=i;
            cusp_pts += shadow_pts[i];
         }
      
      }
   
      // XXX - this procedure may find too many cusps.. there should be
      // a way to filter this down so that it only finds a reasonable
      // number.. this is especially bad when curves are really subdivided


      // last one is always a cusp
      _shadow_cusp_list += shadow_pts.num() - 1;
      cusp_pts += shadow_pts[shadow_pts.num() - 1];

      if (debug) {
         cerr << " CRV_SKETCH::update building guidelines " << endl;
      }
      Wvec projection_dir = _curve->shadow_plane().normal();//Wvec(0,1,0); // for debug only
      build_guidelines(cusp_pts, projection_dir, _guidelines);

   }
}

bool 
CRV_SKETCH::oversketch_shadow(CPIXEL_list& sketch_pixels)
{
   bool debug = Config::get_var_bool("OVERSKETCH_SHADOW_DEBUG",false);
   err_adv(debug, "CRV_SKETCH::oversketch shadow");

   // First, check parameters 

   if (sketch_pixels.num() < 2) {
      cerr << "CRV_SKETCH::oversketch_shadow(): too few sketch pixels" << endl;
      return false;
   }

   // Attempt to slice the sketch pixels into the current screen path of the curve
   PIXEL_list& curve_pixels = _shadow_pixels;

   if (curve_pixels.num()<2) {
      cerr << "CRV_SKETCH::oversketch_shadow(): insufficient number of shadow pixel points"
           << endl;
      return false;
   }

   double DIST_THRESH = 15.0;
   PIXEL_list new_curve;
// change below
   if (_shadow_map->is_closed()) {
      if (!Bcurve::splice_curve_on_closed(sketch_pixels, curve_pixels,
                                  DIST_THRESH, new_curve)) {
         err_adv(debug, "CRV_SKETCH::oversketch_shadow: oversketch closed curve failed");
         return false;
      }
   } else if (!Bcurve::splice_curves(sketch_pixels, curve_pixels,
                             DIST_THRESH, new_curve)) {
      err_adv(debug, "CRV_SKETCH::oversketch_shadow: can't splice in oversketch stroke");
      return false;
   }

   _shadow_pixels.clear();
   for (int i = 0; i < new_curve.num(); i++)
      _shadow_pixels += new_curve[i];
   _shadow_pixels.update_length();

   // Now reshape the curve to the new desired pixel path.
   return reshape_shadow(new_curve);
}

bool 
CRV_SKETCH::reshape_shadow(const PIXEL_list &new_curve)
{

   bool debug = Config::get_var_bool("CRV_SKETCH_RESHAPE_SHADOW",false);
   Wpt_listMap* m = Wpt_listMap::upcast(_shadow_map);
   if (!m) {
      err_adv(debug, "CRV_SKETCH::reshape_shadow: rejecting non Wpt_list map");
      return false;
   }
   Wplane P = _curve->shadow_plane();

   _shadow_pts.clear();
   new_curve.project_to_plane(P, _shadow_pts);

   // get old shape from map: m->get_wpts();
   // new shape:              pts
   // create WPT_LIST_RESHAPE_CMDptr from old, new, and m
   // WORLD::add_command(cmd)
   CRV_SKETCH_CMDptr  cmd= new CRV_SKETCH_CMD(_curve,_shadow_pts);
   WORLD::add_command(cmd);
   //m->set_pts(_shadow_pts);
   _shadow_normals.clear();
   for (int i = 0; i < _shadow_pts.num(); i++)
      _shadow_normals += _curve->shadow_plane().normal();

   //or can be represented in one line as  //   WORLD::add_command(new (WPT_LIST_RESHAPE_CMD(m,pts)));
   // m->set_pts(pts);

   err_adv(debug, "CRV_SKETCH::reshape_shadow: succeeded");
   return true;
}

/*!
  Main procedure to do the initial match, putting resulting
  wpt_list in result_list, return false if match fails.
*/
/*bool 
CRV_SKETCH::do_match(PIXEL_list& drawnlist,
                     Wpt_list& result_list,
                     CARRAY<int>& shadow_cusps,
                     CWpt_list& shadow_pts,
                     CPIXEL_list& shadow_pixels,
                     CARRAY<Wvec>& shadow_normals)
{
   // Finding orientation of drawing with respect to shadow
   PIXELline first_norm(
      CRV_SKETCH::normal_to_screen_line(shadow_normals[0],
                                        shadow_pts[0]));
   PIXELline last_norm(
      CRV_SKETCH::normal_to_screen_line(shadow_normals[shadow_pts.num()-1],
                                        shadow_pts.last()));

   const float same_loc_threshold = 14.0;

   // If curve closed.. check criteria
   if (shadow_pts.is_closed())  {
      cerr << "Closed curve matching.." <<endl;
      if ( drawnlist[0].dist(drawnlist.last()) > same_loc_threshold)  {
         cerr << "Drawing not closed!" <<endl;
         return false;
      }
      if (first_norm.dist(drawnlist[0])> same_loc_threshold) {
         cerr << "Wrong starting pt!"<<endl;
         return false;
      }

      // Figure out orientation
      VEXEL draw_dir = drawnlist[1]-drawnlist[0];
      VEXEL shadow_dir = shadow_pixels[1]-shadow_pixels[0];
 
      int i = 2;
      // Go further until a vector can be built
      while (draw_dir.length()<0.5 && i<drawnlist.num())
         draw_dir=drawnlist[i++]-drawnlist[0];
      i = 2;
      while (shadow_dir.length()<0.5 && i<drawnlist.num())
         shadow_dir=shadow_pixels[i++]-shadow_pixels[0];

      if (draw_dir*shadow_dir <0) {
         cerr << "Different direction"<<endl;
         drawnlist.reverse();
      }
      else cerr << "Same direction" <<endl;

   } else {
      // Figure out orientation by checking end point locations
      // NOTE: Ambiguous when the normals at end point
      // coincide. Behavior undefined
      double d0 = first_norm.dist(drawnlist[0]);  
      double d1 = last_norm.dist(drawnlist.last());
      double d2 = last_norm.dist(drawnlist[0]);
      double d3 = first_norm.dist(drawnlist.last());      
           
      if (d0<same_loc_threshold && d1< same_loc_threshold)
         cerr << "Positive orientation!\n";
      else if (d2<same_loc_threshold && d3<same_loc_threshold)  {
         cerr << "Negative Orientation!\n";
         drawnlist.reverse();
      } else {
         cerr << "Not matched! Drawing ignored\n";
         return false;
      }
   }

   // match_spans is now responsible to make sure that the drawnlist
   // "fits" the shadow_pixels, and that each cusp point of the shadow
   // has a directly corresponding cusp point in the upper curve

   ARRAY<PIXEL_list> spans;

   if (!CRV_SKETCH::match_spans(spans,
                                drawnlist,
                                shadow_cusps,
                                shadow_pts,
                                shadow_pixels,
                                shadow_normals))  {
      cerr << "CRV_SKETCH::do_match(): could not match spans" << endl;
      return false;
   }

   // Using the cusp correspondence, build the new curve
   return CRV_SKETCH::build_curve(spans, 
                                  result_list,
                                  shadow_cusps,
                                  shadow_pts,
                                  shadow_pixels,
                                  shadow_normals);
   
}
*/

/*!  
  procedure to modify the existing world curve during an
  oversketch.  
  
  param drawnlist the list of pixels from the gesture that was the
  oversketch of the original curve
*/

bool 
CRV_SKETCH::re_match(PIXEL_list& drawnlist,
                     CPIXEL_list& upperlist,
                     Wpt_list& result_list,
                     CARRAY<int>& shadow_cusps,
                     CWpt_list& shadow_pts,
                     CPIXEL_list& shadow_pixels,
                     CARRAY<Wvec>& shadow_normals)
{

   static bool debug = Config::get_var_bool("DEBUG_CRV_SKETCH_OVERSKETCH",false);

   if(debug) cerr << "-------- In CRV_SKETCH::oversketch" << endl;

   assert(drawnlist.num() > 1);
   PIXEL start = drawnlist.first(); 
   PIXEL   end = drawnlist.last();

   const int PIX_THRESH = 15; // max distance to be considered "close"

   // Find indices of points on upperlist closest to start
   // and end points, respectively:
   int start_x = -1;
   int   end_x = -1;

   // If not closed curve, allow to not intersect curve, but
   // start at the same guideline.
   if (!shadow_pts.is_closed()) {
      // See if start from beginning/ending (open curve), which
      // will allow drawing starting from non-curve point
      if (shadow_pix_line(0, shadow_pts, shadow_normals).dist(start) < PIX_THRESH)
         start_x = 0;
      else if (shadow_pix_line(shadow_pts.num()-1, shadow_pts, shadow_normals).dist(start) < PIX_THRESH)
         start_x = shadow_pts.num()-1;

      if (start_x != 0 & shadow_pix_line(0, shadow_pts, shadow_normals).dist(end) < PIX_THRESH)
         end_x = 0;
      else if (start_x != shadow_pts.num()-1 &
         shadow_pix_line(shadow_pts.num()-1, shadow_pts, shadow_normals).dist(end) < PIX_THRESH)
         end_x = shadow_pts.num()-1;
   }

   PIXEL foo; // dummy variable
   int idx;
   if (start_x == -1 & upperlist.closest(start, foo, idx) < PIX_THRESH)
      start_x = idx;
   if (end_x == -1 & upperlist.closest(  end, foo, idx) < PIX_THRESH)
      end_x   = idx;

   /*// If not closed curve, allow to not intersect curve, but
   // start at the same guideline.
   if (!shadow_pts.is_closed()) {
      // See if start from beginning/ending (open curve), which
      // will allow drawing starting from non-curve point
      if (start_x == -1) {
         if (shadow_pix_line(0, shadow_pts, shadow_normals).dist(start) < PIX_THRESH)
            start_x = 0;
         if (shadow_pix_line(shadow_pts.num()-1, shadow_pts, shadow_normals).dist(start) < PIX_THRESH)
            start_x = shadow_pts.num()-1;
      }
      if (end_x == -1) {
         if (shadow_pix_line(0, shadow_pts, shadow_normals).dist(end) < PIX_THRESH)
            end_x = 0;
         if (shadow_pix_line(shadow_pts.num()-1, shadow_pts, shadow_normals).dist(end) < PIX_THRESH)
            end_x = shadow_pts.num()-1;
      }
   }*/

   // If start and end points weren't found, give up
   if (start_x == -1) {
      if (debug) cerr << "couldn't find start point" << endl;
      return false;
   }

   if (end_x == -1) {
      if (debug) cerr << "couldn't find end point" << endl;
      return false;
   }

   // Copy the input pixel trail so we can modify it:
   PIXEL_list drawn = drawnlist;

   // If the drawn stroke is oriented in opposite direction from
   // the curve, reverse it:
   if (start_x > end_x) {
      drawn.reverse();
      swap(start_x, end_x); // switch their names
   }

   if (debug)
      cerr << "oversketch Start : " << start_x << ", End : " << end_x <<endl;

   // We're going to create a composite of the pixel trail of the
   // curve as it is, with the modified section just drawn
   // grafted into the list. Get ready:
   PIXEL_list composite;

   int i;
   // Copy first (unmodified) part:
   for (i=0; i < start_x; i++)
      composite += upperlist[i];

   // Copy the new part
   for (i=0; i < drawn.num(); i++)
      composite += drawn[i];

   // Copy last (unmodified) part:
   for (i=end_x+1; i < upperlist.num(); i++)
      composite += upperlist[i];

   // If it started at the guideline, make it exact:
   //if (start_x == 0)
   //   composite[0] = shadow_pix_line(0).project(composite[0]);

   composite.update_length();

   if (debug) {

      cerr << "drawn pixels list" << endl;
      for ( i = 0; i < drawn.num(); i++) {
         cerr << i << " " << drawn[i] << endl;
      }
      
      cerr << "upperlist contents" << endl;
      for ( i = 0; i < upperlist.num(); i++) {
         cerr << i << " " << upperlist[i] << endl;
      }
      
      cerr << "combined pixels " << endl;
      for ( i = 0; i < composite.num(); i++) {
         cerr << i << " " << composite[i] << endl;
      }
   }

   ARRAY<PIXEL_list> spans;


   if(!CRV_SKETCH::match_spans(spans,
                               composite,
                               shadow_cusps,
                               shadow_pts,
                               shadow_pixels,
                               shadow_normals)) {
      if (debug) {
         cerr << "could not match_spans in CRV_SKETCH::oversketch()" << endl;
      }
      return false;
   }

   if (!CRV_SKETCH::build_curve(spans,
                                result_list,
                                shadow_cusps,
                                shadow_pts,
                                shadow_pixels,
                                shadow_normals)) {
      if (debug) {
         cerr << "could not build_curve in CRV_SKETCH::oversketch()" << endl;
      }
      return false;
   }
   
   return true;
        
}

//! Check if ith point on curve is a cusp, w.r.t to the normal given
bool  
CRV_SKETCH::is_cusp(CWpt& a, CWpt& p, CWpt& b, CWvec& n)
{
   // this is the same version as in sweep.C in draw

   // Returns true if the sequence of points a, p, b
   // projected to image space lies all on one side of the
   // image space line defined by the middle point p and the
   // given vector n.

   VEXEL nv = VEXEL(p, n).perpend();
   return XOR((VEXEL(a, p-a) * nv) < 0, (VEXEL(b, b-p) * nv) < 0);

}

//! Match the spans and do the scale/rotate transform to match
//! the cusp points exactly.

//! puts result in array of pixel_list one for each span

//! Return false if matching fails.

bool  
CRV_SKETCH::match_spans(ARRAY<PIXEL_list>& spans,
                        CPIXEL_list& drawnpixels,
                        CARRAY<int>& shadow_cusps,
                        CWpt_list& shadow_pts,
                        CPIXEL_list& shadow_pixels,
                        CARRAY<Wvec>& shadow_normals)
{

   static bool debug = Config::get_var_bool("DEBUG_MATCH_SPANS",false);

//   static const double match_dist = 35.0;

   // distance in pixel space that a cusp can occur away from
   // where it is supposed to happen
   static const double the_zone = 15.0;

   // temporary copy of drawn.. so we don't clobber drawnpixels
   // in case of a bad match
   PIXEL_list drawn = drawnpixels;

   int i;

   // array of indeces that represent cusps in the drawn curve
   ARRAY<int> drawn_cusp_list;
   
   int cur_shadow_cusp = 0;
   
   // the signed distance of the second to lastly visited drawn pixel
   // from the cusp we are approaching
   double last_last_dist = 1000000000.0;
      
   // the signed distance of the last visited drawn pixel from the
   // cusp we are approaching
   double last_dist = 1000000000.0;
   
   // parallel arrays storing indexes and values of minima found
   // while approaching the next cusp
   ARRAY<int> minima_ind;
   ARRAY<double> minima_val;
   
   // whether we have entered the zone.. so we know when we leave the zone
   bool in_the_zone = false;
   
   // vector that points from the cusp line towards the direction in
   // which we are approaching from.
   VEXEL ortho;
   
   // vector that represents the cusp line in pixel space;
   VEXEL scusp;

   if (debug) {
      cerr << " -- In CRV_SKETCH::match_spans() " << endl;
      cerr << "    The shadow curve has " << shadow_cusps.num() << " cusps" << endl;
      cerr << "    The drawn pixels list has " << drawn.num() << " pixels " << endl;
   }

   for (i = 0; i < drawn.num(); i++) {
      if (debug) {
         cerr << "i = " << i << " approaching cusp = " << cur_shadow_cusp << endl;
         cerr << " next shadow cusp vector " << scusp << endl
              << "                 base at " << shadow_pixels[shadow_cusps[cur_shadow_cusp]] << endl;
         //<< " ortho = " << ortho << endl;
      } 


      //cur_shadow_cusp is the current cusp in the shadow we are
      //approaching
      
      if (cur_shadow_cusp == 0) {
         // if we are looking for the first cusp, we just take the
         // first drawn point
         assert(i == 0);
         drawn_cusp_list += i;
         cur_shadow_cusp++;
         
         // should check whether beginning of curve is close
         // enough to starting cusp

         // since next cusp changed, update vectors
         scusp = (VEXEL(shadow_pts[shadow_cusps[cur_shadow_cusp]], shadow_normals[shadow_cusps[cur_shadow_cusp]])).normalized();
         VEXEL line_to_i = drawn[i] - shadow_pixels[shadow_cusps[cur_shadow_cusp]];
         ortho = line_to_i.orthogonalized(scusp).normalized();
         //ortho = scusp.orthogonalized(line_to_i).normalized();
      } else if (cur_shadow_cusp >= shadow_cusps.num() - 1) {
         // we are moving towards the last cusp. 

         // for now we just wait til the last point in the drawn list,
         // and check to see if it is of reasonable distance to the 
         // last cusp
         if (i == (drawn.num() - 1)) {
            VEXEL line_to_i = drawn[i] - shadow_pixels[shadow_cusps[cur_shadow_cusp]];
            ortho = line_to_i.orthogonalized(scusp).normalized();
            
            double dist = (line_to_i * ortho);

            if (debug) cerr << "vexel length: " << line_to_i.length() << endl;
            if (debug) cerr << "scusp: " << scusp << endl;
            
            if (debug) cerr << "last dist: " << dist << endl;

            if ((line_to_i * ortho) > the_zone) {
               // if we ended up too far away from the last cusp, then
               // we fail
               if (debug) cerr << "match_spans(): last point was outside the zone" << endl;
               return false;
            } else {
               if (debug) cerr << "adding last pixel as last cusp" << endl;
               drawn_cusp_list += i;
            }
         } else {
            if (debug) cerr << "looking for last cusp, waiting for last drawn pixel" << endl;
         }
      } else {
         VEXEL line_to_i = drawn[i] - shadow_pixels[shadow_cusps[cur_shadow_cusp]];
         ortho = line_to_i.orthogonalized(scusp).normalized();

         double dist = (line_to_i * ortho);

         if (debug) cerr << " dist: " << dist << endl;
         
         if(!in_the_zone) {
            //if (debug)
            //   cerr << "NOT in the zone" << endl;
            
            // not in the zone yet
            if (fabs(dist) < the_zone) {
               if (debug)
                  cerr << " %%%%%%%%%%%%%% ENTERED the zone " << endl;
               // we just entered the zone
               in_the_zone = true;
               last_last_dist = last_dist;
               last_dist = dist;
            } else {
               // we're not in the zone yet
               last_last_dist = last_dist;
               last_dist = dist;
            }
         } else {
            // in the zone

            if (fabs(dist) < the_zone) {
               // still in the zone, check for minimum
               if ((last_dist < last_last_dist) && (last_dist <= dist)) {
                  // last_dist was less than both this point and
                  // two points ago, that means last_dist must have
                  // been a minimum, and thus a potential cusp
                  
                  // add the previous one to the list of possibilities
                  minima_ind += i - 1;
                  // also add the distance
                  minima_val += last_dist;

                  last_last_dist = last_dist;
                  last_dist = dist;
               } else {
                  // the previous was not a minimum, so just do nothing
                  last_last_dist = last_dist;
                  last_dist = dist;
               }
            } else {

               if(debug)
                  cerr << " %%%%%%%%%%%%%%%% LEFT the zone" << endl;

               // we just left the zone
               if (last_dist < last_last_dist) {
                  minima_ind += i-1;
                  minima_val += last_dist;
               }
               
               // check to see if there is anything in the minima_ind array
               if (minima_ind.num() == 0) {
                  // there were no minima, we fail
                  if (debug) {
                     cerr << "Could not match for cusp " << cur_shadow_cusp << endl;
                  }
                  return false;
               }

               int j;
               int best = -1;
               double minimum = 100000000000.0;
               // search for the minimum minima
               for (j = 0; j < minima_ind.num(); j++) {
                  if (minima_val[minima_ind[j]] < minimum) {
                     best = minima_ind[j];
                     minimum = minima_val[minima_ind[j]];
                  }
               }
               minima_ind.clear();
               minima_val.clear();

               assert (best != -1);
               // now 'best' is the index we want to add
               drawn_cusp_list += best;

               if (debug)
                  cerr << "found cusp " << best << " distance = " << minimum << endl;

               // move to the next shadow_cusp
               cur_shadow_cusp++;
               
               // reset distances to something really big
               last_dist =      100000000000.0;
               last_last_dist = 1000000000000.0;
                  

               // recalc pixel vectors
               scusp = (VEXEL(shadow_pts[shadow_cusps[cur_shadow_cusp]], shadow_normals[shadow_cusps[cur_shadow_cusp]])).normalized();
               VEXEL line_to_i = drawn[i] - shadow_pixels[shadow_cusps[cur_shadow_cusp]];
               //ortho = scusp.orthogonalized(line_to_i).normalized();
               ortho = line_to_i.orthogonalized(scusp).normalized();

               // since current we are at the point that left the zone,
               // we should back up one.
               i--;

               // we start out not in the zone for the next one
               in_the_zone = false;
            }
         }
      }
   } // end big for loop

   if (debug) {
      cerr << "Found Cusps at " << endl;
      for (i =0; i < drawn_cusp_list.num(); i++) {
         cerr << drawn_cusp_list[i] << " ";
      }
      cerr << endl;
   }
           
   // at this point we should have all the cusps for the
   // drawn list in drawn_cusps_ind

   if(shadow_cusps.num() != drawn_cusp_list.num()) {
      if (debug) {
         cerr << "cusp counts don't match: shadow = " << shadow_cusps.num()
              << " drawn = " << drawn_cusp_list.num() << endl;
      }
      return false;
   }

   if(debug) cerr << "MATCHING SPANS ACTUALLY WORKED!!" << endl;


   // we need to fix the endpoints for each span now.

   spans.clear();

   int cur_span;

   if (debug) cerr << " building spans array " << endl;

   // build up the array of spans : NOTE : each span pixel list has a
   // copy of its endpoints, so interior cusp points are duplicated in
   // adjacent spans
   for (cur_span = 0; cur_span < drawn_cusp_list.num() - 1; cur_span++) {
      PIXEL_list pl;
      if (debug) cerr << " Span #" << cur_span << endl;
      for (i = drawn_cusp_list[cur_span]; i <= drawn_cusp_list[cur_span + 1]; i++) {
         pl += drawn[i];
         if (debug) cerr << " " << i << " added pixel" << drawn[i] << endl;
      }
      pl.update_length();
      spans += pl;
   }

   for (cur_span = 0; cur_span < drawn_cusp_list.num() - 1; cur_span++) {
      // copy the pixel list out
      PIXEL_list pl = spans[cur_span];
      // figure out best endpoints
      PIXEL firstp = pl.first();
      PIXEL lastp = pl.last();

      PIXEL correctfirst = PIXEL( (Wline(shadow_pts[shadow_cusps[cur_span]],
                                         shadow_normals[shadow_cusps[cur_span]])
                                     ).intersect(Wline(firstp)) );

      PIXEL correctlast = PIXEL( (Wline(shadow_pts[shadow_cusps[cur_span+1]],
                                        shadow_normals[shadow_cusps[cur_span+1]])
                                    ).intersect(Wline(lastp)) );

      // pl is a reference so this will modify the copy in the array
      pl.fix_endpoints(correctfirst, correctlast);
      pl.update_length();
      // copy the pixel list back in
      spans[cur_span] = pl;
   } 

   if (debug) {
      cerr << "Corrected SPANS" << endl;
      for (cur_span = 0; cur_span < spans.num(); cur_span++) {
         PIXEL_list pl = spans[cur_span];
         cerr << "Span #" << cur_span << endl;
         for (i = 0; i < pl.num(); i++) {
            cerr << pl[i] << endl;
         }
      }
   }
         

   return true;

}


//! With the spans correctly matched, build the list of points in
//! world.
bool
CRV_SKETCH::build_curve(CARRAY<PIXEL_list>& spans, 
                        Wpt_list& ret,
                        CARRAY<int>& shadow_cusps,
                        CWpt_list& shadow_pts,
                        CPIXEL_list& shadow_pixels,
                        CARRAY<Wvec>& shadow_normals)
{

   assert(spans.num() == (shadow_cusps.num() - 1));

   static bool debug = Config::get_var_bool("DEBUG_BUILD_CURVE",false);

   ret.clear();

   if (debug) {
      cerr << "Build_curve() : correlating points" << endl;
      cerr << "Shadow has cusps at ";
      int k;
      for ( k = 0; k < shadow_cusps.num(); k++) {
         cerr << shadow_cusps[k] << " ";
      }
      cerr << endl;
   }

   int i;
   int cur_span = 0;
   for (i = 0; i < shadow_pts.num(); i++) {

      if (i == shadow_cusps[cur_span + 1] && i != (shadow_pts.num() - 1)) {
         // if its the last one, don't increment
         cur_span++;
      }
      
      PIXEL_list& plist = spans[cur_span];
      PIXELline pline(shadow_pixels[i], VEXEL(shadow_pts[i], shadow_normals[i]));
      
      int j;
      bool first = true;
      PIXEL best_intersection;
      int best_seg;

      // search through each span for the best intersection
      for (j = 0; j < plist.num() - 1; j++) {
         PIXELline dseg(plist[j], plist[j+1]);
         PIXEL isect;
         
         isect = dseg.intersect(pline);
         double t = ((isect - plist[j]) * dseg.vector()) / (dseg.vector() * dseg.vector());
         t = clamp(t, 0.0, 1.0);
         isect = plist[j] + dseg.vector() * t;
         
         if ((pline.dist(isect) < pline.dist(best_intersection)) || first) {
            best_intersection = isect;
            best_seg = 0;
            first = false;
         }
      }
      ret += (Wline(shadow_pts[i], shadow_normals[i])).intersect(Wline(best_intersection));
   }

   assert(ret.num() == shadow_pts.num());
   ret.update_length();

   return true;

}

void
CRV_SKETCH::notify(CCAMdataptr &data) 
{
   DrawWidget::notify(data);
   update();
}  

/*****************************************************************
 * CRV_SKETCH_CMD
 *****************************************************************/
bool
CRV_SKETCH_CMD::doit()
{
   if (is_done())
      return true;

   if (_type) {
      if (_curve->b1()) {
         _curve->b1()->move_to(_new_b1);
      }
      if (_curve->b2()) {
         _curve->b2()->move_to(_new_b2);
      }
      _map->set_uvs(_new_uvs);
   } else {
      s_map->set_pts(_new_shadow);
      _curve->b1()->move_to(_map->map(0));
      _curve->b2()->move_to(_map->map(1));
   }
   
   return COMMAND::doit();
}

bool
CRV_SKETCH_CMD::undoit()
{
   if (!is_done())
      return true;

   if (_type) {
      _curve->b1()->move_to(_old_b1);
      _curve->b2()->move_to(_old_b2);
      _map->set_uvs(_old_uvs);
   } else {
      s_map->set_pts(_old_shadow);
      _curve->b1()->move_to(_map->map(0));
      _curve->b2()->move_to(_map->map(1));
   }

   return COMMAND::undoit();
}

// end of file crv_sketch.C
