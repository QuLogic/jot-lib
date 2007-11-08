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
 * sweep.C
 *****************************************************************/

/*!
 *  \file sweep.C
 *  \brief Contains the definitions of SWEEP_BASE, SWEEP_POINT, SWEEP_LINE, and SWEP_DISK.
 *
 *  \ingroup group_FFS
 *  \sa sweep.H
 *
 */

#include "disp/colors.H"                // named colors, e.g. blue_pencil
#include "geom/world.H"                 // for WORLD::undisplay()
#include "gtex/ref_image.H"             // for VisRefImage
#include "gtex/basic_texture.H"         // for GLStripCB
#include "mesh/mi.H"                    // mesh inline methods

#include "tess/bcurve.H"                // Bcurve
#include "tess/action.H"
#include "tess/uv_surface.H"            // UVsurface
#include "tess/ti.H"                    // tess inline methods
#include "std/config.H"                 // variables

#include "ffs/ffs_util.H"

//#include "floor.H"
#include "sweep.H"

using namespace mlib;
using namespace tess;
using namespace FFS;

static bool debug_all = Config::get_var_bool("DEBUG_SWEEP_ALL",false);

//! distance in PIXELS considered "close":
static const double DIST_THRESH_PIXELS = 12;  
static const double GUIDE_LEN          = 500; //!< default length in PIXELs

/*****************************************************************
 * SWEEP_BASE
 *****************************************************************/
SWEEP_BASE::SWEEP_BASE() : DrawWidget(), _line(new StylizedLine3D())
{
   // Define FSA arcs for handling GESTUREs:
   //   Note: the order matters. First matched will be executed.

   _draw_start += DrawArc(new TapGuard,     drawCB(&SWEEP_BASE::tap_cb));
   _draw_start += DrawArc(new LineGuard,    drawCB(&SWEEP_BASE::line_cb));
   _draw_start += DrawArc(new SlashGuard,   drawCB(&SWEEP_BASE::trim_line_cb));

   // Set up guideline properties:
   _line->set_width(3);
   _line->set_color(Color::blue_pencil_l);
   _line->set_alpha(0.8);
   _line->set_do_stipple();
}

/*!
	if mesh is null, returns false.
	otherwise records mesh, initializes guideline
	does other initialization, and returns true.
	'a' is start of guideline, 'b' is end;
	dur is duration of widget before timeout
 */
bool
SWEEP_BASE::setup(LMESHptr mesh, CWpt& a, CWpt& b, double dur)
{

   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_SETUP",false) || debug_all;

   // check mesh
   if (!mesh)
      return false;
   _mesh = mesh;

   // set up to observe when the mesh is undisplayed:
   if (_mesh->geom())
      disp_obs(_mesh->geom());
   else if (debug)
      err_adv(debug_all, "SWEEP_DISK::setup: Warning: mesh->geom is null");
   
   // initialize guideline:
   build_line(a, b);

   // set the timeout duration
   set_timeout(dur);

   // become the active widget and get in the world's DRAWN list:
   activate();

   return true;
}

int
SWEEP_BASE::cancel_cb(CGESTUREptr&, DrawState*& s)
{
   // Turn off widget:
   deactivate();

   // Reset state to start:
   s = &_draw_start;

   return 1; // This means we DID use up the gesture
}

int
SWEEP_BASE::tap_cb(CGESTUREptr& g, DrawState*& s)
{
   err_adv(debug_all, "SWEEP_BASE::tap_cb");

   // tap near end of guideline: do uniform sweep based
   // on guideline length:
   if (g->center().dist(sweep_end()) < DIST_THRESH_PIXELS)
      return do_uniform_sweep(sweep_vec());

   // tap elsewhere on the guideline: do uniform sweep based
   // on tap location:
   if (hits_line(g->end()))
      return do_uniform_sweep(project_to_guideline(g->end()) - sweep_origin());

   // tap elsewhere: cancel:
   return cancel_cb(g,s);
}

//! Extract a consecutive set of points from a point list.
//! Maybe should go in ARRAY class or _point2d_list...
//! needed in stroke_cb() below:
template <class T>
inline T
pts_in_range(const T& pts, int i1, int i2)
{
   assert(pts.valid_index(i1) && pts.valid_index(i2));

   T ret;
   for (int i=i1; i<=i2; i++)
      ret += pts[i];
   return ret;
}

bool
SWEEP_BASE::from_center(CGESTUREptr& g) const
{
   assert(g != 0);
   return g->start().dist(sweep_origin()) < DIST_THRESH_PIXELS;
}

bool
SWEEP_BASE::hits_line(CPIXEL& p) const
{
   return pix_line().dist(p) < DIST_THRESH_PIXELS;
}

int  
SWEEP_BASE::line_cb(CGESTUREptr& g, DrawState*& s)
{
   // Activity occurred to extend the deadline for fading away:
   reset_timeout();

   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_LINE_CB",false) || debug_all;

   err_adv(debug, "SWEEP_BASE::line_cb");

   // If gesture aligns with guideline:
   //    if it starts near the end and extends past the end, extend
   //    if it starts near the beginning, do uniform sweep
   // If it's across the gesture, trim

   // If it's a trim stroke, it has to be short and run
   // across the guideline.
   const double TRIM_MAX_LEN = 65;
   if (g->length() < TRIM_MAX_LEN) {
      const double TRIM_ANGLE_THRESH = 80; // degrees
      double angle = line_angle(g->endpt_vec(), pix_line().vector());
      if (rad2deg(angle) > TRIM_ANGLE_THRESH) {
         // Nice angle. But did it cross?
         if (g->endpt_line().intersect_segs(pix_line()))
            return trim_line_cb(g, s);
      }
   }

   // do uniform sweep if straight gesture starts at sweep origin
   // and ends near the guideline:
   if (from_center(g)) {
      if (hits_line(g->end()))
         return do_uniform_sweep(project_to_guideline(g->end()) - sweep_origin());
      return stroke_cb(g,s);
   }

   // extend the guideline if straight gesture starts near guideline end
   // and is nearly parallel:
   const double ALIGN_ANGLE_THRESH = 15; // degrees
   if (pix_line().endpt().dist(g->start()) < DIST_THRESH_PIXELS &&
       rad2deg(g->endpt_vec().angle(pix_line().vector())) < ALIGN_ANGLE_THRESH)
      return extend_line_cb(g, s);

   return stroke_cb(g,s);
}

//! Internal convenience method.
//! Reset the endpoint of the guideline.
void 
SWEEP_BASE::reset_endpoint(CPIXEL& pix) 
{

   // Ensure things are normal
   assert(_line);

   // Reset the endpoint of the guideline:
   _line->set(1, wline().intersect(Wline(pix)));

   // Reset the clock so the session doesn't end early
   reset_timeout();
}

int
SWEEP_BASE::trim_line_cb(CGESTUREptr& g, DrawState*&)
{
   // Activity occurred to extend the deadline for fading away:
   reset_timeout();

   // A slash gesture across the guideline trims its tip

   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_TRIM",false) || debug_all;

   err_adv(debug, "SWEEP_BASE::trim_cb");

   // Find where the slash intersects the guideline.
   PIXEL p;
   if (g->endpt_line().intersect_segs(pix_line(), p)) {

      // Reset the endpoint to match screen position p:
      reset_endpoint(p);

      return 1;
   }

   // The slash gesture missed the guideline...
   // XXX - policy on failure?

   err_adv(debug, "SWEEP_BASE::trim_line_cb: Missed the guideline");

   return 1; // This means we DID use up the gesture
}

//! They want to make the guideline longer.
//! It's easy.
int
SWEEP_BASE::extend_line_cb(CGESTUREptr& g, DrawState*&)
{

   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_BASE_EXTEND",false) || debug_all;
   if (debug)
      err_msg("SWEEP_BASE::extend_line_cb");

   reset_endpoint(g->end());

   return 1; // This means we DID use up the gesture
}

void
SWEEP_BASE::reset()
{
   // Stop observing the mesh:
   if (_mesh && _mesh->geom())
      unobs_display(_mesh->geom());

   // Clear cached data.
   _line->clear();
   _mesh = 0;
}

void
SWEEP_BASE::build_line(CWpt& a, CWpt& b)
{
   // Convenience method for building the guideline.

   _line->clear();
   _line->add(a);
   _line->add(b);
}

int
SWEEP_BASE::draw(CVIEWptr& v)
{
   // Set the transparency
   _line->set_alpha(alpha());

   // Draw the line:
   int ret = _line->draw(v);

   // When all faded away, undraw
   if (alpha() == 0)
      WORLD::undisplay(this, false);

   return ret;
}

int
SWEEP_BASE::draw_final(CVIEWptr& v)
{     
   return  _line->draw_final(v);    
}

void
SWEEP_BASE::request_ref_imgs()
{
   return _line->request_ref_imgs();
}
int
SWEEP_BASE::draw_id_ref_pre1()
{
   return  _line->draw_id_ref_pre1();
}

int
SWEEP_BASE::draw_id_ref_pre2()
{
   return _line->draw_id_ref_pre2();
}

int
SWEEP_BASE::draw_id_ref_pre3()
{
   return _line->draw_id_ref_pre3();
}

int
SWEEP_BASE::draw_id_ref_pre4()
{
   return _line->draw_id_ref_pre4();
}

/*****************************************************************
 * SWEEP_DISK
 *****************************************************************/
SWEEP_DISKptr SWEEP_DISK::_instance;
ARRAY<Panel*> SWEEP_DISK::panels;
ARRAY<Bpoint_list> SWEEP_DISK::bpoints;
ARRAY<Bcurve_list> SWEEP_DISK::bcurves;
ARRAY<Bsurface_list> SWEEP_DISK::bsurfaces;
ARRAY<Wpt_list> SWEEP_DISK::profiles;

SWEEP_DISK::SWEEP_DISK() : SWEEP_BASE()
{
   // Define FSA arcs for handling GESTUREs:
   _draw_start += DrawArc(new StrokeGuard, drawCB(&SWEEP_DISK::stroke_cb));

   // Set up the clean up routine to free static _instance
   atexit(clean_on_exit);
}

void 
SWEEP_DISK::clean_on_exit() 
{ 
   _instance = 0; 
}

SWEEP_DISKptr
SWEEP_DISK::get_instance()
{
   if (!_instance)
      _instance = new SWEEP_DISK();
   return _instance;
}

//! Being re-activated
bool 
SWEEP_DISK::setup(Panel* p, Bpoint_list points, Bcurve_list curves, Bsurface_list surfs, Wpt_list profile)
{
   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_SETUP",false) || debug_all;

   // XXX - some of the code here is the same as the code in the other setup method
   //     - better to wrap these code into a helper method
   Bface_list faces = p->bfaces();

   _boundary = faces.get_boundary();
   if (_boundary.num_line_strips() != 1) {
      err_adv(debug, "SWEEP_DISK::setup: error: boundary is not a single piece");
      return false;
   }

   // Get the best-fit plane, rejecting if the boundary Wpt_list
   // doesn't lie within 0.1 of its total length from the plane:
   if (!_boundary.verts().pts().get_plane(_plane, 0.1)) {
      err_adv(debug,"SWEEP_DISK::setup: Error: can't find plane");
      return false;
   }
   
   // Find the center
   Wpt o = _boundary.verts().pts().average();

   // decide guideline direction (normal to plane):
   Wvec n = _plane.normal();
   if (VIEW::eye_vec(o) * n > 0)
      n = -n;

   // decide the length for the guideline:
   double len = world_length(o, GUIDE_LEN);

   // compute guideline endpoint:
   Wpt b = o + n.normalized()*len;

   // try basic setup
   if (!SWEEP_BASE::setup(LMESH::upcast(faces.mesh()), o, b, default_timeout()))
      return false;

   // ******** From here on we accept it ********

   _points = points;
   _curves = curves;
   _surfs = surfs;
   _surfs.mesh()->toggle_show_secondary_faces();
   _profile = profile;
   _enclosed_faces = faces;

   return true;
}

//! Given a set of enclosed face, activate the widget to sweep out a
//! shape. Checks for errors, returns true on success.
bool
SWEEP_DISK::setup(CGESTUREptr& gest, double dur)
{

   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_SETUP",false) || debug_all;

   if (!(gest && gest->is_dslash())) {
      err_adv(debug, "SWEEP_DISK::setup: bad gesture");
      return false;
   }

   // XXX - shouldn't require it is a Panel:
   Panel* p = Panel::upcast(Bsurface::hit_ctrl_surface(gest->start()));
   if (!p) {
      err_adv(debug, "SWEEP_DISK::setup: non-panel");
      return false;
   }

   Bface_list faces = p->bfaces();

   _boundary = faces.get_boundary();
   if (_boundary.num_line_strips() != 1) {
      err_adv(debug, "SWEEP_DISK::setup: error: boundary is not a single piece");
      return false;
   }

   // Get the best-fit plane, rejecting if the boundary Wpt_list
   // doesn't lie within 0.1 of its total length from the plane:
   if (!_boundary.verts().pts().get_plane(_plane, 0.1)) {
      err_adv(debug,"SWEEP_DISK::setup: Error: can't find plane");
      return false;
   }
   
   // Find the center
   Wpt o = _boundary.verts().pts().average();

   // decide guideline direction (normal to plane):
   Wvec n = _plane.normal();
   if (VIEW::eye_vec(o) * n > 0)
      n = -n;

   // decide the length for the guideline:
   double len = world_length(o, GUIDE_LEN);

   // compute guideline endpoint:
   Wpt b = o + n.normalized()*len;

   // try basic setup
   if (!SWEEP_BASE::setup(LMESH::upcast(faces.mesh()), o, b, dur))
      return false;

   // ******** From here on we accept it ********

   _enclosed_faces = faces;

   return true;
}

void
SWEEP_DISK::reset()
{
   // Clear cached data.
   _boundary.reset();
   _plane = Wplane();
   _enclosed_faces.clear();

   _points.clear();
   _curves.clear();
   if (!_surfs.empty())
      _surfs.mesh()->toggle_show_secondary_faces();
   _surfs.clear();
   _profile.clear();
}

Wpt_list
fold_points(CWpt_list& pts, CWvec& n, bool is_closed)
{
   if (pts.num() < 2)
      return Wpt_list();
   Wpt_list ret(2);

   // Handle first point specially
   if (!is_closed || is_fold(pts.last(), pts[0], pts[1], n) )
      ret += pts.first();

   // Interior points
   for (int i=1; i<pts.num()-1; i++)
      if (is_fold(pts[i-1], pts[i], pts[i+1], n))
         ret += pts[i];

   // Handle last point specially
   if (!is_closed || is_fold(pts[pts.num()-2], pts.last(), pts.first(), n) )
      ret += pts.last();

   return ret;
}

Wpt_list 
SWEEP_DISK::get_fold_pts() const
{
   CEdgeStrip * strip = _boundary.cur_strip();
   if ( strip ) {
      return fold_points(strip->verts().pts(), sweep_vec(), true);
   }
   
   return Wpt_list();
}

//! Draw the special "fold points" of the curve, where it
//! turns around WRT the sweep direction:
int
SWEEP_DISK::draw(CVIEWptr& v)
{

   GL_VIEW::draw_pts(
      get_fold_pts(),
      Color::red_pencil,
      alpha(),
      v->line_scale()*10.0
      );

   if (!_profile.empty()) {
      GL_VIEW::draw_wpt_list(_profile,
         Color::red1,
         alpha(),
         v->line_scale()*5.0,
         false);
   }

   return SWEEP_BASE::draw(v);
}

int
SWEEP_DISK::stroke_cb(CGESTUREptr& g, DrawState*&)
{
   bool debug = debug_all;

   err_adv(debug, "SWEEP_DISK::stroke_cb");

   // need non-const copy to fix endpoints
   GESTUREptr gest = g;

   // Activity occurred to extend the deadline for fading away:
   reset_timeout();

   // central axis must be on-screen
   if (!sweep_origin().in_frustum()) {
      err_adv(debug, "SWEEP_DISK::stroke_cb: error: sweep origin off-screen");
      return 1;
   }

   // reject closed or self-intersecting strokes
   if (gest->is_closed()) {
      WORLD::message("Can't use closed stroke");
      return 1;
   }
   if (gest->self_intersects()) {
      WORLD::message("Can't use self-intersecting stroke");
      return 1;
   }

   // Get the "silhouette" point on the boundary curve nearest
   // the start of the stroke:
   Wpt hit;
   if (from_center(gest)) {
      WORLD::message("Stroke from center of region must follow axis");
      return 1;
   }

   // get the oversketch pixel_list
   PIXEL_list profile_pixels;
   bool is_line = false;
   bool is_from_center = false;
   if (!hit_boundary_at_sil(gest->start(), hit)) {
      if (_profile.empty() || !Bcurve::splice_curves(gest->pts(), (PIXEL_list)_profile,
                              15.0, profile_pixels) ||
                              !hit_boundary_at_sil(profile_pixels.first(), hit)) {
         WORLD::message("Stroke must start at a red dot or base of axis");
         return false;
      } 
   } else {
      profile_pixels = gest->pts();
      is_line = gest->is_line();
      is_from_center = from_center(gest);
   }

   // If the stroke ends near the guideline, snap it to it:
   bool cone_top = false;
   if (hits_line(profile_pixels.last())) {
      profile_pixels.fix_endpoints(profile_pixels.first(), pix_line().project(profile_pixels.last()));
      cone_top = true;
   }

   // Don't let the stroke cross the guideline (but it can end at it)
   bool its_bad = false;
   if (cone_top)
      its_bad = pts_in_range(profile_pixels, 0, profile_pixels.num()-2).
         intersects_line(pix_line());
   else its_bad = profile_pixels.intersects_line(pix_line());
   if (its_bad) {
      WORLD::message("Stroke cannot cross the dotted line");
      return 1;
   }

   // A and B are line segments joining start and end of stroke
   // (respectively) to the guideline, in screen space:
   PIXELline A(pix_line().project(profile_pixels.first()), profile_pixels.first());
   PIXELline B(pix_line().project(profile_pixels.last()),   profile_pixels.last());

   // Segment A has to be big enough to see what's going on
   if (A.length() < DIST_THRESH_PIXELS) {
      WORLD::message("Base curve is too small, zoom in to see better");
      return 1;
   }

   // Stroke can't cross either line segment, because then the
   // surface would be self-intersecting
   its_bad = pts_in_range(profile_pixels, 1, profile_pixels.num()-1).intersects_seg(A);
   its_bad = its_bad || !cone_top &&
      pts_in_range(profile_pixels, 0, profile_pixels.num()-2).intersects_seg(B);
   if (its_bad) {
      WORLD::message("Please - no self-intersecting surfaces");
      return 1;
   }

   if (do_sweep(profile_pixels, hit, is_line, is_from_center)) {
      // We're done -- start fading
      reset_timeout(0.5);
   } else {
      err_adv(debug_all, "SWEEP_DISK::stroke_cb: error: do_sweep failed");
   }

   return 1;
}

// Do a uniform sweep operation based on the given
// displacement along the guideline (from its origin):
bool
SWEEP_DISK::do_uniform_sweep(CWvec& v)
{

   // find a "fold" point
   Wpt_list pts = get_fold_pts();
   if (pts.empty()) {
      // no fold points: should never happen
      err_adv(debug_all, "SWEEP_DISK::do_uniform_sweep: error: no fold points");
      return false;
   }
   Wpt hit = pts.first();

   // construct coordinate frame
   Wpt  o = sweep_origin();
   Wvec t = sweep_vec().normalized();
   Wvec n = (hit - o).orthogonalized(t).normalized();
   Wvec b = cross(n,t);

   // construct the sweep line
   Wpt_list spts;
   spts += hit;
   spts += (hit + v);

   if (do_sweep(o, t, n, b, spts)) {
      // We're done -- start fading
      reset_timeout(0.5);
      return true;
   } else {
      err_adv(debug_all, "SWEEP_DISK::do_uniform_sweep: failed");
   }

   return false;
}

bool
SWEEP_DISK::do_sweep(CPIXEL_list& gpts, CWpt& sil_hit, bool is_line, bool from_center)
{
   // set up coordinate frame
   //   o: origin, at center of base region (enclosed faces)
   //   t: points along sweep direction
   //   n: points toward silhouette, in plane of base region
   //   b: perpendicular to t and n

   Wpt  o = sweep_origin();
   Wvec t = sweep_vec().normalized();
   Wvec n = (sil_hit - o).orthogonalized(t).normalized();
   Wvec b = cross(n,t);

   // Project the sweep line to the [t,n] plane
   Wpt_list spts;
   Wplane   P(o, b);
   if (is_line) {
      // ignore points other than first and last
      spts += Wpt(P, Wline(gpts.first()));
      spts += Wpt(P, Wline(gpts.last()));
      if (from_center) {
         // move the sweep line from the origin to sil_hit
         Wvec delt = sil_hit - o;
         spts[0] += delt;
         spts[1] += delt;
      }
   } else {
      gpts.project_to_plane(P, spts);
   }

   // Make sure it starts at the hit point
   spts.fix_endpoints(sil_hit, spts.last());

   return do_sweep(o, t, n, b, spts);
}

bool
SWEEP_DISK::do_sweep(CWpt& o, Wvec t, Wvec n, Wvec b, Wpt_list spts)
{
   assert(spts.num() > 1);

   // record before transformation
   _profile = spts;

   // Transform sweep curve to local coords:
   spts.xform(Wtransf(o, t, b, n).inverse());

   // If ending t-val is negative, reverse orientation
   double t1 = spts.last()[0];
   if (t1 < 0) {
      // the stroke ended lower than it started
      // (WRT guideline direction).
      spts.xform(Wtransf::scaling(-1, 1, 1));
      t = -t;
      b = -b;
      t1 = -t1;
   }

   Bpoint_list bpts = Bpoint::get_points(_boundary.verts());

   MULTI_CMDptr cmd = new MULTI_CMD();
   if ( bpts.empty() ) {
      // Create the axis map:
      Wpt_list apts(2);
      apts += o;
      apts += o + t*t1;
      if (build_revolve(apts, n, spts, cmd))
         return( true );
   } else if ( bpts.num() > 2 ) {
      if (build_box(o, t, spts, cmd))
         return( true );
   } else {
      WORLD::message("Too few points in boundary");
   }

   return( false );
}


UVsurface*
SWEEP_DISK::build_revolve(
   CWpt_list&   apts,
   CWvec&       n,
   CWpt_list&   spts,
   MULTI_CMDptr cmd
   )
{
   static bool debug = Config::get_var_bool("DEBUG_BUILD_REVOLVE");

   // Editing or Creating
   bool is_editing = !(_surfs.empty());

   Bcurve* bcurve = Bcurve::get_curve(_boundary.edges());
   if (!bcurve) {
      err_adv(debug, "SWEEP_DISK::build_revolve: can't find curve");
      return 0;
   }
   if (!bcurve->is_control()) {
      err_adv(debug, "SWEEP_DISK::build_revolve: error: non-control curve");
      return 0;
   }

   Map1D3D* axis = new Wpt_listMap(apts, new WptMap(apts[0]), new WptMap(apts[1]), n);

   // XXX - Zachary: add SWEEP_CMD here:

   // Create/Edit the surface of revolution
   UVsurface* ret = NULL;
   Panel* p = Panel::upcast(Bsurface::get_surface(_enclosed_faces));
   assert(p);
   if (is_editing) {
      assert(!_surfs.empty());
      ret = UVsurface::upcast(_surfs[0]);
      assert(ret);

      TubeMap* tmap = TubeMap::upcast(ret->map());
      assert(tmap);

      // reshape the axis
      Wpt_listMap* m = Wpt_listMap::upcast(tmap->axis());
      assert(m);
      WPT_LIST_RESHAPE_CMDptr a_cmd = new WPT_LIST_RESHAPE_CMD(m,apts);
      if (a_cmd->doit())
         cmd->add(a_cmd);

      // reshape the top curve
      double s = spts.last()[2]/spts.first()[2];
      Wtransf M = tmap->axis()->F(1) * Wtransf::scaling(0,s,s) * tmap->axis()->Finv(0);
      WPT_LIST_RESHAPE_CMDptr c1_cmd = new WPT_LIST_RESHAPE_CMD(((Wpt_listMap*)tmap->c1()), M*((Wpt_listMap*)tmap->c0())->get_wpts());
      if (c1_cmd->doit())
         cmd->add(c1_cmd);

      // reshape the profile
      cmd->add(new TUBE_MAP_RESHAPE_CMD(tmap,spts));   

      // change the record
      int loc = panels.get_index(p);
      assert(loc >= 0);
      profiles[loc] = _profile;

   } else {
      // for recording purposes
      Bpoint_list points;
      Bcurve_list curves;
      Bsurface_list surfs;

      ret =
         UVsurface::build_revolve(bcurve, axis, spts, _enclosed_faces, points, curves, surfs, cmd);
      if (ret) {
         //FLOOR::realign(ret->cur_mesh(), cmd);

         panels += p;
         bpoints += points;
         bcurves += curves;
         bsurfaces += surfs;
         profiles += _profile;
      }
   }

   WORLD::add_command(cmd);

   return ret;
}


inline void
build_coons(
   Bpoint* a,                   // points
   Bpoint* b,                   //   in 
   Bpoint* c,                   //     ccw
   Bpoint* d,                   //       order
   Bsurface_list& surfaces,     //!< put result in here
   MULTI_CMDptr cmd
   )
{
   Bsurface* surf = UVsurface::build_coons_patch(a, b, c, d, cmd);
   if (surf)
      surfaces += surf;
}

bool
SWEEP_DISK::build_box(CWpt& o, CWvec& t, CWpt_list& spts, MULTI_CMDptr cmd)
{
   // Editing or Creating
   bool is_editing = !(_surfs.empty());

   // get list of Bpoints around the boundary of the base surface:
   ARRAY<Bpoint*> bot_pts = is_editing ? _points.extract(_points.num()/2, _points.num()/2) : 
      Bpoint::get_points(_boundary.verts());  
   assert ( bot_pts.num() > 2 );
   int n = bot_pts.num();
   Bpoint_list top_pts(n);

   // If surface normals of base surface point along the sweep
   // direction, they have to be reversed. otherwise, the
   // boundary runs CW, so we reverse the order of the bottom
   // points to get them to run CCW:
   //
   // XXX - needs fix to work on embedded region, similar to
   //       build_tube() above.

   if ( _plane.normal()*t  < 0 ) {
      if (!is_editing) bot_pts.reverse();
   } else {
      reverse_faces( _enclosed_faces );
      if(cmd)
         cmd->add(new REVERSE_FACES_CMD(_enclosed_faces, true));
   }

   double avg_len = _boundary.edges().avg_len();
   if ( isZero(avg_len) ) {
      cerr << "SWEEP_DISK::build_box(): ERROR, boundary avg len is zero" << endl;
      return false;
   }

   int num_edges = max( (int)round( spts.length()/avg_len ), 1 );
   int res_level = Config::get_var_int("BOX_RES_LEVEL", 2,true);

   // we'll keep lists of all curves and surfaces for the box,
   // for setting their res level uniformly:
   Bcurve_list   curves   = Bcurve::get_curves(_boundary.edges());
   Bsurface_list surfaces = Bsurface::get_surfaces(_enclosed_faces);

   if (!surfaces.empty())
      res_level = surfaces.min_res_level();
   else if (!curves.empty())
      res_level = curves.min_res_level();

   curves.clear();
   surfaces.clear();

   // XXX - Zachary: add SWEEP_CMD (BOX_CMD?) here:

   // Create/Edit the top points matching the bottom points
   // and the curves running vertically between them
   int i = 0;
   for ( i=0; i<n; i++ ) {
      Wvec n = (bot_pts[i]->loc() - o).orthogonalized(t).normalized();
      Wvec b = cross(n,t);
      Wpt_list cpts = spts;
      cpts.xform( Wtransf(o, t, b, n) );

      // XXX - should be undoable
      if (is_editing) {
         Wpt_listMap* m = Wpt_listMap::upcast(_curves[i]->map());
         cmd->add(new WPT_LIST_RESHAPE_CMD(m,cpts));
      } else {
         top_pts += BpointAction::create(_mesh, cpts.last(), b, n, res_level, cmd);
         curves  += BcurveAction::create(_mesh, cpts, b, num_edges , res_level,
                                 bot_pts[i], top_pts[i], cmd);
      }
   }

   if (!is_editing) {
      // Create curves joining each top point to the next.
      for ( i=0; i<n; i++ ) {
         int j = (i+1) % n;

         Bcurve* c = bot_pts[i]->lookup_curve( bot_pts[j] );
         if ( !c ) {
            cerr << "SWEEP_DISK::build_box(): ERROR, can't find boundary curve"
               << endl;
            continue;
         }

         // Ensure orientation of top and bottom curves is the same.
         int i1 = i;
         int i2 = j;
         if ( c->b1() != bot_pts[i1] ) 
            swap( i1, i2 );
         assert( c->b1() == bot_pts[i1] &&
               c->b2() == bot_pts[i2] );

         // Create the new top curve with the same shape as the bottom curve.
         Wpt_list cpts = c->get_wpts();
         cpts.fix_endpoints( top_pts[i1]->loc(), top_pts[i2]->loc() );
         
         curves += BcurveAction::create(_mesh, cpts, t, c->num_edges(),
                              res_level, top_pts[i1], top_pts[i2], cmd);

         build_coons(bot_pts[i], bot_pts[j], top_pts[j], top_pts[i], surfaces, cmd);
      }

      // Slap on a top if base is quadrilateral
      // XXX - should handle other cases
      if (n == 4)
         build_coons(top_pts[0], top_pts[1], top_pts[2], top_pts[3], surfaces, cmd);
   }

   _mesh->changed();

   // set the res level uniformly over the box:
   for (int i = 0; i < bot_pts.num(); i++)
      bot_pts[i]->set_res_level(res_level);
   top_pts.set_res_level(res_level);
   curves.set_res_level(res_level);
   surfaces.set_res_level(res_level);
   _mesh->update_subdivision(res_level);

   // Record data necessary to return to this mode
   Panel* p = Panel::upcast(Bsurface::get_surface(_enclosed_faces));
   assert(p);
   if (is_editing) {
      int loc = panels.get_index(p);
      assert(loc >= 0);
      profiles[loc] = _profile;
   } else {
      panels += p;
      top_pts += bot_pts;
      bpoints += top_pts;
      bcurves += curves;
      bsurfaces += surfaces;
      profiles += _profile;
   }
   
   //FLOOR::realign(_mesh->cur_mesh(), cmd);

   WORLD::add_command(cmd);

   return true;
}

bool
SWEEP_DISK::hit_boundary_at_sil(
   CPIXEL& p,   // given screen point to test
   Wpt& ret     // nearest Wpt on boundary edges at "silhouette"
   ) const
{
   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_HIT_BOUNDARY",false) || debug_all;

   Wpt_list fpts = get_fold_pts();
   if (!_profile.empty()) fpts += _profile.first();
   int k = fpts.closest_vertex(p);
   if (fpts.valid_index(k) && p.dist(fpts[k]) < DIST_THRESH_PIXELS) {
      ret = fpts[k];
      return 1;
   }
   err_adv(debug, "SWEEP_DISK::hit_boundary_at_sil: missed");
   return 0;
}

/*****************************************************************
 * SWEEP_LINE
 *****************************************************************/
SWEEP_LINEptr SWEEP_LINE::_instance;

SWEEP_LINE::SWEEP_LINE() : SWEEP_BASE()
{
   // Set up the clean up routine to free static _instance
   atexit(clean_on_exit);
}

void 
SWEEP_LINE::clean_on_exit() 
{ 
   _instance = 0; 
}

SWEEP_LINEptr
SWEEP_LINE::get_instance()
{
   if (!_instance)
      _instance = new SWEEP_LINE();
   return _instance;
}

inline Wvec
endpt_vec(CGESTUREptr& g, CWplane& P)
{
   return Wpt(P, Wline(g->end())) - Wpt(P, Wline(g->start()));

}

//! Given an initial slash gesture (or delayed slash) near the
//! center of an existing straight Bcurve, set up the widget to
//! do a sweep cross-ways to the Bcurve:
bool
SWEEP_LINE::setup(CGESTUREptr& slash, double dur)
{

   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_SETUP",false) || debug_all;

   err_adv(debug, "SWEEP_LINE::setup");

   // check the gesture
   if (!(slash && slash->straightness() > 0.99)) {
      err_adv(debug, "SWEEP_LINE::setup: gesture is bad");
      return false;
   }

   // find the (straight) Bcurve near slash start
   _curve = Bcurve::hit_ctrl_curve(slash->start());
   if (!(_curve && _curve->is_straight())) {
      err_adv(debug, "SWEEP_LINE::setup: no straight curve at start");
      return false;
   }

   // find endpoints
   Bpoint *b1 = _curve->b1(), *b2 = _curve->b2();
   assert(b1 && b2);    // straight curve must have endpoints

   // curve cannot be connected to other curves
   if (b1->vert()->degree() != 1 || b2->vert()->degree() != 1) {
      err_adv(debug, "SWEEP_LINE::setup: curve is not isolated");
      return false;
   }

   // ensure the gesture starts near the center of the straight line Bcurve:
   {
      PIXEL a = b1->vert()->pix();
      PIXEL b = b2->vert()->pix();
      double t = (slash->start() - a).tlen(b-a);
      if (t < 0.35 || t > 0.65) {
         err_adv(debug, "SWEEP_LINE::setup: gesture not near center of line");
         return false;
      }
   }

   // find the plane to work in
   _plane = check_plane(shared_plane(b1, b2));
   if (!_plane.is_valid()) {
      err_adv(debug, "SWEEP_LINE::setup: no valid plane");
      return false;
   }

   // check that slash is perpendicular to line
   Wpt  a = b1->loc();  // endpoint at b1
   Wpt  b = b2->loc();  // endpoint at b2
   Wvec t = b - a;      // vector from endpt a to endpt b
   Wpt  o = a + t/2;    // center of straight line curve
   Wvec n = cross(_plane.normal(), t); // direction across line ab

   Wvec slash_vec = endpt_vec(slash, _plane);
   const double ALIGN_ANGLE_THRESH = 15;
   double angle = rad2deg(slash_vec.angle(n));
   if (angle > 90) {
      angle = 180 - angle;
      n = -n;
   }
   if (angle > ALIGN_ANGLE_THRESH) {
      err_adv(debug, "SWEEP_LINE::setup: slash is not perpendicular to line");
      err_adv(debug, "                   angle: %f", angle);
      return false;
   }

   // compute guideline endpoint:
   Wpt endpt = o + n.normalized()*a.dist(b);

   return SWEEP_BASE::setup(_curve->mesh(), o, endpt, dur);
}

void
SWEEP_LINE::reset()
{
   // Clear cached data.
   _curve = 0;
   _plane = Wplane();
}

int
SWEEP_LINE::draw(CVIEWptr& v)
{
   // this is here in case we decide to draw more than the guideline

   // for now, just the guideline:
   return SWEEP_BASE::draw(v);
}

//! Do a uniform sweep operation based on the given
//! displacement along the guideline (from its origin):
bool
SWEEP_LINE::do_uniform_sweep(CWvec& v)
{

   if (create_rect(v)) {
      // We're done -- start fading
      reset_timeout(0.5);
      return true;
   } else {
      err_adv(debug_all, "SWEEP_LINE::do_uniform_sweep: error: create_rect failed");
   }

   return false;
}

int
SWEEP_LINE::stroke_cb(CGESTUREptr& gest, DrawState*&)
{
   // curved sweeps not implemented yet...
   err_adv(1, "SWEEP_LINE::stroke_cb: not implemented");

   return 1;
}

//! Draw a straight line cross-ways to an existing
//! straight Bcurve to create a rectangle:
bool 
SWEEP_LINE::create_rect(CGESTUREptr& gest)
{

   static bool debug =
      Config::get_var_bool("DEBUG_CREATE_RECT",false) || debug_all;
   err_adv(debug, "SWEEP_LINE::create_rect");

   if (!(gest && gest->is_line())) {
      err_adv(debug, "SWEEP_LINE::create_rect: gesture is not a line");
      return false;
   }

   // central axis must be on-screen
   if (!sweep_origin().in_frustum()) {
      err_adv(debug_all, "SWEEP_LINE::stroke_cb: error: sweep origin off-screen");
      return false;
   }

   if (!from_center(gest)) {
      WORLD::message("Stroke must begin on curve at guideline");
      return false;
   }
   if (!hits_line(gest->end())) {
      WORLD::message("Line must follow axis");
      return false;
   }

   // compute vector along guideline, based on input stroke:
   Wvec v = project_to_guideline(gest->end()) - sweep_origin();

   return create_rect(v);
}

bool 
SWEEP_LINE::create_rect(CWvec& v)
{
   // create a rectangular Panel based on given vector along the guideline

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

   static bool debug =
      Config::get_var_bool("DEBUG_CREATE_RECT",false) || debug_all;

   assert(_curve != 0);
   Bpoint *b1 = _curve->b1(), *b2 = _curve->b2();
   assert(b1 && b2);
   Wvec u = b2->loc() - b1->loc();      // vector along existing straight line

   // Swap b1 and b2 if necessary:
   Wvec n = _plane.normal();
   if (det(v,n,u) < 0) {
	  err_adv(debug, "SWEEP_LINE::create_rect: b1 and b2 swapped");
      //swap(b1,b2);
      //u = -u;
   }

   // Decide number of edges "horizontally" (see diagram above)
   int num_v = _curve->num_edges(); // number of edges "vertically"
   double H = u.length();           // "height"
   double W = v.length();           // "width"
   double l = H/num_v;              // length of an edge "vertically"
   int num_h = (int)round(W/l);     // number of edges "horizontally"
   if (num_h < 1) {
      // Needs more work to handle this case. Bail for now:
      err_adv(debug, "SWEEP_LINE::create_rect: cross-stroke too short");
      return false;
   }

   // Accept it now

   LMESHptr m = _curve->mesh();
   Wpt p1 = b1->loc(), p2 = b2->loc(), p3 = p2 + v, p4 = p1 + v;

   MULTI_CMDptr cmd = new MULTI_CMD;

   // Create points b3 and b4
   Bpoint* b3 = BpointAction::create(m, p3, n, v, b2->res_level(), cmd);
   Bpoint* b4 = BpointAction::create(m, p4, n, v, b1->res_level(), cmd);

   // Create the 3 curves: bottom, right and top
   Wpt_list side;
   int res_lev = _curve->res_level();

   err_adv(debug, "SWEEP_LINE::create_rect: curve res level: %d", res_lev);

   Bcurve_list contour;
   contour += _curve;

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
   
   WORLD::add_command(cmd);
   
   return true;
}

bool 
SWEEP_LINE::create_ribbon(CGESTUREptr& g)
{
   // sweep a straight line crossways along a curve to create a "ribbon"

   return false;
}

/*****************************************************************
 * SWEEP_POINT
 *****************************************************************/
SWEEP_POINTptr SWEEP_POINT::_instance;

SWEEP_POINT::SWEEP_POINT() : SWEEP_BASE()
{
   // Set up the clean up routine to free static _instance
   atexit(clean_on_exit);
}

void 
SWEEP_POINT::clean_on_exit() 
{ 
   _instance = 0; 
}

SWEEP_POINTptr
SWEEP_POINT::get_instance()
{
   if (!_instance)
      _instance = new SWEEP_POINT();
   return _instance;
}

//! Given an initial slash gesture (or delayed slash) on a
//! defined plane (FLOOR, Cursor3D, or existing Bpoint), set up
//! the widget to provide a guideline for drawing a straight line.
bool
SWEEP_POINT::setup(CGESTUREptr& slash, double dur)
{

   static bool debug =
      Config::get_var_bool("DEBUG_SWEEP_SETUP",false) || debug_all;

   err_adv(debug, "SWEEP_POINT::setup");

   // check the gesture
   if (!(slash && slash->straightness() > 0.9)) {
      err_adv(debug, "SWEEP_POINT::setup: gesture is bad");
      return false;
   }

   // find the (straight) Bpoint near slash start
   _point = Bpoint::hit_ctrl_point(slash->start());

   return false;
//   return SWEEP_BASE::setup(_curve->mesh(), o, endpt, dur);
}

void
SWEEP_POINT::reset()
{
   // Clear cached data.
   _point = 0;
   _plane = Wplane();
}

int
SWEEP_POINT::draw(CVIEWptr& v)
{
   // this is here in case we decide to draw more than the guideline

   // for now, just the guideline:
   return SWEEP_BASE::draw(v);
}

//! Do a uniform sweep operation based on the given
//! displacement along the guideline (from its origin):
bool
SWEEP_POINT::do_uniform_sweep(CWvec& v)
{

   if (create_line(v)) {
      // We're done -- start fading
      reset_timeout(0.5);
      return true;
   } else {
      err_adv(debug_all,
              "SWEEP_POINT::do_uniform_sweep: error: create_rect failed");
   }

   return false;
}

bool 
SWEEP_POINT::create_line(CWvec& v)
{
   return false;
}

// end of file sweep.C
