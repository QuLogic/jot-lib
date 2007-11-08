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
#include "disp/colors.H"
#include "geom/gl_view.H"
#include "geom/world.H"         // for DEBUG_ELLIPSE
#include "gest_int.H"
#include "mlib/point2.H"        // XXX - Probably doesn't need to be included.
#include "std/config.H"
#include "std/run_avg.H"

using namespace mlib;

const double MIN_GESTURE_LENGTH=15;
const double MIN_GESTURE_SPREAD=10;

void
GESTURE::init(CPIXEL& p, CEvent& down, double pressure)
{
   _down = down;
   _start_frame = VIEW::stamp();
   _bbox.set(Wpt(XYpt(p)),Wpt(XYpt(p)));
   _pix_bbox.set(p, p);
   add(p, 1, pressure);
}

void   
GESTURE::add(CPIXEL& p, double min_dist, double pressure) 
{
   if (_pts.empty() || _pts.last().dist(p) >= min_dist) {
      _pts       += p;
      _pressures += pressure;
      _times     += stop_watch::sys_time();

      _bbox.update(Wpt(XYpt(p)));
      _pix_bbox.update(p);

      // XXX - should have incremental version of this:
      _pts.update_length();        
   }
   _end_frame = VIEW::stamp();
}

void
GESTURE::smooth_points()
{
   PIXEL_list s = _pts;
   for (int i=1; i<s.num()-1; i++) {
      // use 1-6-1 mask:
      s[i] = (_pts[i-1] + _pts[i]*6.0 + _pts[i+1])/8.0;
   }
   _pts = s;
}

void
GESTURE::smooth_points(int n)
{
   for (int i=0; i<n; i++)
      smooth_points();
}

void
GESTURE::reflect_points(const PIXELline& l)
{
   PIXEL_list s = _pts;
   for (int i=0; i<s.num(); i++) {
	s[i] = l.reflection(_pts[i]);
   }
   _pts = s;
}

void  
GESTURE::complete(CPIXEL& p, CEvent& up) 
{
   if (_pts.num() < 2)
      add(p,0,_pressures.last());
   _up = up;

   // get rid of jagged tips:
   trim();

   static int n = Config::get_var_int("GEST_SMOOTH_PASSES",1);
   smooth_points(n);

   _complete = true;
}


// XXX - move to ARRAY?
template <class A>
inline void
clip_tip(A& array, int num_to_clip)
{
   // remove first 'num_to_clip' elements from the array
   A tmp = array;
   tmp.reverse();                               // turn it around
   tmp.truncate(array.num() - num_to_clip);     // chop it off
   tmp.reverse();                               // turn it back
   array = tmp;
}

void  
GESTURE::trim()
{
   // Eliminate jagged ends at start and end of gesture.

   // However, skip short or tightly curled gestures
   if (spread() < 15)
      return;

   // Maximum length of gesture to trim at each end:
   static double trim_dist = Config::get_var_dbl("TRIM_DIST", 10,true);

   // Angle considered sharp (default 30 degrees):
   static double trim_angle = Config::get_var_dbl("TRIM_ANGLE", deg2rad(30),true);

   // Don't chop ends of short strokes
   if (length() <= trim_dist*2.5)
      return;

   int n = _pts.num(), i;
   int trim_i = 0;     // "Trim index" - index of new start of pixel trail

   static bool debug = Config::get_var_bool("GEST_DEBUG_TRIM",false);

   // Do the start of the stroke:
   for (i=1; (i < n-1) && (_pts[i].dist(_pts[0]) < trim_dist); i++) {
      if (angle(i) > trim_angle)
         trim_i = i;
   }
   if (trim_i > 0) {
      err_adv(debug, "GESTURE::trim: clipping at %d", trim_i);
      clip_tip(_pts,       trim_i);
      _pts.update_length();
      clip_tip(_times,     trim_i);
      clip_tip(_pressures, trim_i);
   }
      
   // Do the end of the stroke:
   n = _pts.num();
   trim_i = n-1;        // Index of last element after trimming
   for (i=n-2; (i > 0) && (_pts[i].dist(_pts.last()) < trim_dist); i--) {
      if (angle(i) > trim_angle)
         trim_i = i;
   }
   if (trim_i < n-1) {
      // The parameter to ARRAY::truncate() tells how many
      // elements the array should have after truncation. In this
      // case we want trim_i to be the index of the last element,
      // which means there should be trim_i + 1 elements in the
      // truncated array.
      _pts.      truncate(trim_i + 1);
      _pts.update_length();
      _times.    truncate(trim_i + 1);
      _pressures.truncate(trim_i + 1);
      err_adv(debug, "GESTURE::trim: trimming at %d", trim_i);
   }
}

GESTUREptr
GESTURE::prev(int k) const
{
   // k=0 returns this
   // k=1 returns the gesture immediately before this
   //     etc.
   return _gest_int->gesture(_index-k);
}

double
GESTURE::speed() const
{
   // length divided by elapsed time

   double et = elapsed_time();
   return (et > 1e-12) ? length()/et : 0;
}

double 
GESTURE::startup_time(double dist_thresh) const
{
   // Time spent within the given threshold distance of the 1st point.
   // Used to measure a gesture that begins w/ a pause.

   assert(dist_thresh >= 0);

   if (!is_stroke()) {
      err_msg("GESTURE::startup_time: Error: gesture is not complete");
      return 0;
   }

   if (dist_thresh > length())
      return elapsed_time();

   // Find last index i within the distance threshold:
   int i = -1;
   _pts.interpolate(dist_thresh/length(), 0, &i);
   if (!_times.valid_index(i)) {
      err_msg("GESTURE::startup_time: Error: PIXEL_list::interpolate failed");
      return 0;
   } else if (_times.valid_index(i+1)) {
      // Advance the index to the first slot outside the startup region:
      i++;
   }

   return elapsed_time(i);
}

double
GESTURE::radius() const
{
   // returns average distance to the "center" point

   if (_pts.empty())
      return 0;

   PIXEL c = center();

   double ret=0;
   for (int k=0; k<_pts.num(); k++)
      ret += _pts[k].dist(c);

   return (ret / _pts.num());
}

inline double
angle(CPIXEL& a, CPIXEL& b, CPIXEL& c)
{
   //                                        
   //                            c           
   //                           /            
   //                          /             
   //                         /              
   //                        /               
   //                       /                
   //                      /  angle           
   //   a - - - - - - - - b . . . . . .
   //
   //    Treating a, b, and c as ordered points in a
   //    polyline, calculate the angle of the change in
   //    direction, as shown.  In the diagram, the result is
   //    positive, since the forward direction swivels
   //    around b in the CCW direction. For clockwise
   //    rotations, the computed angle is negative.
   // 

   return (b - a).signed_angle(c - b);
}

double
GESTURE::winding(bool do_trim, bool do_abs) const
{
   static bool debug = Config::get_var_bool("GEST_DEBUG_WINDING",false);

   if (_pts.length() < 1.0)
      return 0;

   // beginning and ending indices of
   // pixel list vertices:
   int k1=0, k2=_pts.num()-1;

   err_adv(debug, "GESTURE::winding");

   if (do_trim) {

      // how many pixels to ignore at beginning 
      // and end of stroke, for purposes of computing
      // the winding number?
      double trim = min(.1 * _pts.length(), 12.0);

      // ignore the first and last "trim" pixels
      // because strokes often have jagged starts and
      // stops that throw off the value for the
      // winding number.

      // convert "trim" from pixel measure
      // to parameter along pixel list
      double t = clamp(trim/_pts.length(), 0.0, 1.0);

      err_adv(debug, "   t: %f, t*len: %f", t, t*_pts.length());

      double v=0;
      _pts.interpolate_length(t,   k1, v);
      _pts.interpolate_length(1-t, k2, v);

   }

   err_adv(debug, "   %d points, k1: %d, k2: %d", _pts.num(), k1, k2);

   // add up the angles
   double ret = 0;
   for (int k = max(k1,1); k < k2; k++) {
      double a = ::angle(_pts[k-1], _pts[k], _pts[k+1]);
      ret += (do_abs ? fabs(a) : a);
   }
   return ret/(M_PI*2);
}

bool
GESTURE::is_corner(int i) const
{
   // endpoints are considered "corners".
   // also internal points where exterior angle is large.

   // endpoints
   if (i == 0 || i ==_pts.num()-1)
      return true;

   // invalid points
   if (!_pts.valid_index(i))
      return false;

   // internal points
   static const double MIN_ANGLE = Config::get_var_int("JOT_CORNER_ANGLE",45);
   return rad2deg(fabs(angle(i))) > MIN_ANGLE;
}

ARRAY<int>
GESTURE::corners() const
{
   ARRAY<int> ret;
   for (int i=0;i < _pts.num(); i++)
      if (is_corner(i))
         ret += i;
   return ret;
}

bool 
GESTURE::is_stroke() const
{
   return (_down._d && _up._d && _pts.num() > 1);
}

bool 
GESTURE::is_closed() const
{
   // How close should endpoints be to consider it closed?
   // Say 7 percent of the total length?
   double thresh = max(10.0, length()*0.07);
   return (
      is_stroke()               &&
      (length() > 50)           &&
      (endpoint_dist() < thresh)
      );
}

bool 
GESTURE::is_tap() const
{
   return (
      is_stroke()               &&
      (elapsed_time() < .20)    &&
      (length() < 10)
      );
}

bool 
GESTURE::is_press_hold() const
{
   return (
      is_stroke()               &&
      (elapsed_time() > 0.5)    &&
      (length() < 10)
      );
}

bool 
GESTURE::is_double_tap() const
{
   GESTUREptr p = prev();

   return (is_tap() && p && p->is_tap() &&
           elapsed_time(p) < 1 && dist(p) < 10);
}

bool   
GESTURE::is_dslash() const 
{
   // Delayed slash: starts w/ a pause, then looks like a slash.

   return (
      is_stroke()               &&
      (length() > 10)           &&
      (straightness() > .9)     &&
      (length() < 100)          &&
      (startup_time()   > 0.25) &&
      (remaining_time() < 0.1)
      );
}

bool   
GESTURE::is_tslash() const 
{
    // tap-slash (tap, then slash)

   GESTUREptr p = prev();

   return (is_slash() && p && p->is_tap() &&
           elapsed_time(p) < 1 && dist(p) < 10);
}

bool   
GESTURE::is_slash_tap() const 
{
    // slash-tap

   GESTUREptr p = prev();

   return (is_tap() && p && p->is_slash() && elapsed_time(p) < 1);
}

bool   
GESTURE::is_zip_zap() const 
{
    // forward/back slash

   if (!is_stroke()             ||
       (length() <  20)         ||
       (length() > 200)         ||
       (straightness() > 0.5))
      return false;

   // should have 1 corner
   ARRAY<int> corn = corners();
   if (corn.num() != 3)
      return false;

   // stroke should be straight to corner and back:
   PIXEL c = _pts[corn[1]]; // interior corner location
   return (start().dist(c) + end().dist(c) > 0.95 * length());
}

bool   
GESTURE::is_arc() const 
{
    // slightly curving stroke

   double aw = fabs(winding());
   double wa = winding_abs();
   return (
      is_stroke()                       &&
      (length() >  12)                  &&
      (straightness() > 0.8)            &&
      (aw > 0.08)                       &&
      (aw < 0.35)                       &&
      (fabs(aw - wa) < 0.06)
      );
}

bool   
GESTURE::is_small_arc() const 
{
    // small arc (eg under 60 pixels long)

   return (is_arc() && length() < 60);
}

bool 
GESTURE::is_dot() const
{
   if (!is_stroke() || length() < 40 || straightness() > .2)
      return 0;

   PIXEL c = center();

   for (int k=0; k<_pts.num(); k++)
      if (_pts[k].dist(c) > 25)
         return 0;

   //return (winding() > 2.3);
   // XXX -- what's a reasonable value??
   return ( fabs(winding()) > 1.75);
}

bool 
GESTURE::is_scribble() const
{
   // XXX - needs work:
   //   for small scribbles speed threshold is too high
   //   for large ones it's okay...
   return (
      is_stroke()               &&
      (length() > 85)           &&
      (endpoint_dist() > 17)    &&
      (speed() > 600)           &&
      (straightness() < .45)    &&
      (winding(1,1) > 2.0)
      );
}

bool 
GESTURE::is_loop() const
{
   return (
      is_stroke()                       && // got a down and up
      (length() > 50)                   && // not too short
      (straightness() < .15)            && // ends are close relative to length
      (fabs(1 - fabs(winding())) < .3)  && // winding number is about 1
      (winding(1,1) < 1.5)                 // mostly convex
      );
}

bool 
GESTURE::is_lasso() const
{
   return (
      is_stroke()                       &&
      (speed() > 300)                   &&
      (length() > 50)                   &&
      (straightness() < .2)             &&
      (fabs(1 - fabs(winding())) < .5)  &&
      (winding(1,1) < 1.5)
      );
}

bool 
GESTURE::is_circle(
   double max_ratio
   ) const
{
   static bool debug = Config::get_var_bool("DEBUG_CIRCLE",false);

   PIXEL center;        // return values
   VEXEL axis;          //   for
   double r1=0, r2=0;   //     GESTURE::is_ellipse()
   if (!is_ellipse(center, axis, r1, r2)) {
      err_adv(debug, "GESTURE::is_circle: GESTURE is not an ellipse");
      return false;
   }

   // Check ratio of radii
   // (but avoid division by zero):
   const double MIN_R2 = 1e-8;
   bool success = (r2 >= MIN_R2 && r1/r2 < max_ratio);
   err_adv(debug && !success,
           "GESTURE::is_circle: too distorted (r1/r2 = %f > %f)",
           (r2 < MIN_R2) ? 0.0 : r1/r2, max_ratio);
   return success;
}

bool 
GESTURE::is_small_circle(
   double max_ratio
   ) const
{
   static bool debug = Config::get_var_bool("DEBUG_SMALL_CIRCLE",false);

   err_adv(debug, "GESTURE::is_small_circle:");

   const double SMALL_CIRCLE_SPREAD=10;

   double w  = winding();       // winding number: sum of angle_i
   double aw = fabs(w);         // |w|
   double diffw1 = fabs(1 - aw);// abs difference between w and 1
   double wa = winding(1,1);    // "winding abs", sum of |angle_i|

   err_adv(debug, "   spread: %f", spread());
   err_adv(debug, "   length: %f", length());
   err_adv(debug, "   straightness: %f", straightness());
   err_adv(debug, "   |1 - |w||: %f", diffw1);
   err_adv(debug, "   w abs: %f", wa);

   bool ret = (
      is_stroke()                       && // got a down and up
      spread() < SMALL_CIRCLE_SPREAD    && // Needs to be small
      (length() > 15)                   && // not too short
      (length() < 45)                   && // not too long 
      (straightness() < .20)            && // ends are close relative to length
      (diffw1 < .4)                     && // winding number is about 1
      (wa < 1.75)                          // mostly convex
      );
   if (ret) err_adv(debug, "  passed");
   else     err_adv(debug, "  failed");
   return ret;
}

bool  
GESTURE::is_click_hold() const
{
   return (
      _down._d                  &&
      !_up._d                   &&
      (length() < 10)           &&
      (elapsed_time() > 0.8)
      );
}

bool   
GESTURE::is_line(double min_straightness, double min_len) const 
{
   return (
      is_stroke()                         &&
      (straightness() > min_straightness) &&
      (length()       > min_len)
      );
}

bool   
GESTURE::is_line() const 
{
   static const double min_straightness =
      Config::get_var_dbl("JOT_LINE_MIN_STRAIGHTNESS",0.95);
   static const double min_len =
      Config::get_var_dbl("JOT_LINE_MIN_LEN",20);
   return is_line(min_straightness, min_len);
}

bool   
GESTURE::is_slash() const 
{
   return (
      is_stroke()               &&
      (length() > 8)           &&  //was 10
      (straightness() > .8)     &&  //was .9
      (length() < 80)           &&
      (speed()  > 120)
      );
}

bool 
GESTURE::is_n_line() const 
{
   if (!is_line())
      return 0;

   // check that angle w/ vector (0,1)
   // is less than 23 degrees

   VEXEL u = endpt_vec().normalized();
   return (u[1] > 0.92);
}

bool   
GESTURE::is_e_line() const 
{
   if (!is_line())
      return 0;

   // check that angle w/ vector (1,0)
   // is less than 23 degrees

   VEXEL u = endpt_vec().normalized();
   return (u[0] > 0.92);
}

bool   
GESTURE::is_s_line() const 
{
   if (!is_line())
      return 0;

   // check that angle w/ vector (0,-1)
   // is less than 23 degrees

   VEXEL u = endpt_vec().normalized();
   return (u[1] < -0.92);
}

bool   
GESTURE::is_w_line() const 
{
   if (!is_line())
      return 0;

   // check that angle w/ vector (-1,0)
   // is less than 30 degrees

   VEXEL u = endpt_vec().normalized();
   return (u[1] < -0.92);
}

bool   
GESTURE::is_ne_line() const 
{
   if (!is_line())
      return 0;

   // check that angle w/ vector (1,1)
   // is less than 30 degrees

   VEXEL u = endpt_vec().normalized();
   return ((u * VEXEL(1,1))/M_SQRT2 > 0.92);
}

bool   
GESTURE::is_se_line() const 
{
   if (!is_line())
      return 0;

   // check that angle w/ vector (1,-1)
   // is less than 30 degrees

   VEXEL u = endpt_vec().normalized();
   return ((u * VEXEL(1,-1))/M_SQRT2 > 0.92);
}

bool   
GESTURE::is_sw_line() const 
{
   if (!is_line())
      return 0;

   // check that angle w/ vector (-1,-1)
   // is less than 30 degrees

   VEXEL u = endpt_vec().normalized();
   return ((u * VEXEL(-1,-1))/M_SQRT2 > 0.92);
}

bool   
GESTURE::is_nw_line() const 
{
   if (!is_line())
      return 0;

   // check that angle w/ vector (-1,1)
   // is less than 30 degrees

   VEXEL u = endpt_vec().normalized();
   return ((u * VEXEL(-1,1))/M_SQRT2 > 0.92);
}

bool   
GESTURE::is_x() const 
{
   GESTUREptr p = prev();

   const double STRAIGHT_THRESH = 0.92;
   const double MAX_LENGTH = 100.0;
   const double MIN_LENGTH =  10.0;

   // ways to rule it out:
   if (!is_line(STRAIGHT_THRESH,MIN_LENGTH) ||
       length() > MAX_LENGTH ||
       length() < MIN_LENGTH ||
       !p ||
       !p->is_line(STRAIGHT_THRESH,MIN_LENGTH) ||
       p->length() > MAX_LENGTH ||
       p->length() < MIN_LENGTH ||
       (end_time() - p->start_time() > 2.0) ||
       center().dist(p->center())/max(length(),p->length()) > 0.5)
      return 0;
      
   // it comes down to the angle between the 2 lines
   // (should be greater than 60 degrees)
   double dot = fabs(endpt_vec().normalized() *
                     p->endpt_vec().normalized());
   return (dot < cos(M_PI/3));
}

inline double
linear_interp(double x1, double y1, double x2, double y2, double x)
{
   return ((x - x1)*((y2 - y1)/(x2 - x1))) + y1;
}

inline double
ellipse_max_err(const GESTURE* gest)
{
   // Return the allowable "error" for use when deciding
   // whether a GESTURE has the shape of an ellipse.
   //
   // Let the allowable error be proportional to stroke length.
   // A scaling factor of 0.008 seems to work okay.
   double err_factor = Config::get_var_dbl("ELLIPSE_ERR_FACTOR", 0.01,true);
   double max_err = err_factor * gest->length();

   // But tolerate more error for fast gestures:
   //   If speed <= LO_SPEED, multiply max_err by LO_FACTOR.
   //   If speed >= HI_SPEED, multiply max_err by HI_FACTOR.
   //   In between, linearly interpolate.
   double LO_SPEED = 200,
      LO_FACTOR = Config::get_var_dbl("ELLIPSE_LO_FACTOR", 0.9,true);
   double HI_SPEED = 500,
      HI_FACTOR = Config::get_var_dbl("ELLIPSE_HI_FACTOR", 1.4,true);
   double s = gest->speed();
   double r = linear_interp(LO_SPEED, LO_FACTOR, HI_SPEED, HI_FACTOR, s);
   max_err *= clamp(r, LO_FACTOR, HI_FACTOR);

   return max_err;
}

/*****************************************************************
 * ELLIPSE
 *****************************************************************/
class ELLIPSE {
 protected:
   PIXEL        _center;        // center of ellipse
   VEXEL        _x;             // x direction
   VEXEL        _y;             // y direction = x.perp()
   double       _r1;            // "radius" in x
   double       _r2;            // "radius" in y
   PIXEL_list   _pts;           // points computed from above, for rendering
   
 public:

   //******** MANAGERS ********

   ELLIPSE(CPIXEL& c, CVEXEL& x, double r1, double r2, int res=32) {
      set(c,x,r1,r2,res);
   }

   virtual ~ELLIPSE() {}

   //******** BUILDING ********

   virtual void set(CPIXEL& c, CVEXEL& x, double r1, double r2, int res) {
      _center = c;
      _x = x.normalized();
      _y = _x.perpend();
      _r1 = fabs(r1);
      _r2 = fabs(r2);
      rebuild(res);
   }

   // Rebuild points with given resolution
   void rebuild(int res);

   // avg distance of each point in the given list to the ellipse
   double avg_dist(CPIXEL_list& pts) {
      RunningAvg<double> ret(0);
      for (int i=0; i<pts.num(); i++)
         ret.add(_pts.dist(pts[i]));
      return ret.val();
   }
};

void
ELLIPSE::rebuild(int res) 
{
   // Rebuild points with given resolution

   // Need good data:
   assert(_r1 > gEpsAbsMath && _r2 > gEpsAbsMath &&
          !_x.is_null() && !_y.is_null() && res > 4);

   // Prepare to rebuild pts:
   _pts.clear();
   _pts.realloc(res);

   // add points, including last point == first point
   for (int i=0; i<res; i++) {
      double t = (2*M_PI)*i/res;
      _pts += _center + _x*(_r1*cos(t)) + _y*(_r2*sin(t));
   }
}

/*****************************************************************
 * DEBUG_ELLIPSE
 *****************************************************************/
class DEBUG_ELLIPSE : public GEL, public ELLIPSE {
 protected:
   egg_timer    _timer;         // used for timing out (to be undrawn)
   
 public:

   //******** MANAGERS ********

   DEBUG_ELLIPSE(CPIXEL& c, CVEXEL& x, double r1, double r2, int res=32) :
      ELLIPSE(c,x,r1,r2,res),
      _timer(10) {}

   //******** BUILDING ********

   virtual void set(CPIXEL& c, CVEXEL& x, double r1, double r2, int res) {
      ELLIPSE::set(c,x,r1,r2,res);
      _timer.reset(10); // show for 10 seconds, then fade away
   }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("DEBUG_ELLIPSE", DEBUG_ELLIPSE*, GEL, CDATA_ITEM *);

   // Shortly before the timer expires, start fading away.
   // This computes the alpha to use for fading.
   double alpha() const {
      const double FADE_TIME = 0.5; // fade for half a second
      return min(_timer.remaining() / FADE_TIME, 1.0);
   }

   //******** GEL VIRTUAL METHODS ********

   virtual bool needs_blend()    const { return alpha() < 1; }
   virtual int  draw(CVIEWptr &);

   //******** DATA_ITEM VIRTUAL METHODS ********

   virtual DATA_ITEM* dup() const { return 0; }
};

int
DEBUG_ELLIPSE::draw(CVIEWptr& v) 
{
   // load identity for model matrix
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();
      
   // set up to draw in PIXEL coords:
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadMatrixd(v->pix_proj().transpose().matrix());

   // Set up line drawing attributes
   glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
   glDisable(GL_LIGHTING);              // GL_ENABLE_BIT

   // Draw ellipse in blue, 2 pixels wide:

   // Turn on antialiasing for width-2 lines:
   GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*2));
   GL_COL(COLOR::blue, alpha());
   glBegin(GL_LINE_STRIP);
   for (int k=0; k<_pts.num(); k++)
      glVertex2dv(_pts[k].data());
   glEnd();
   GL_VIEW::end_line_smooth();

   // Draw axes in dark grey, 1-pixel wide
   GL_VIEW::init_line_smooth(GLfloat(v->line_scale()*1));
   glColor4d(.8,.8,.8,alpha());
   glBegin(GL_LINES);
   glVertex2dv((_center - _x*_r1).data());
   glVertex2dv((_center + _x*_r1).data());
   glVertex2dv((_center - _y*_r2).data());
   glVertex2dv((_center + _y*_r2).data());
   glEnd();
   GL_VIEW::end_line_smooth();

   // Draw center as 3-pixel wide orange dot
   GL_VIEW::init_point_smooth(float(v->line_scale()*3));
   GL_COL(Color::orange, alpha());
   glBegin(GL_POINTS);
   glVertex2dv(_center.data());
   glEnd();
   GL_VIEW::end_point_smooth();

   glPopAttrib();
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   
   if (alpha() == 0)
      WORLD::undisplay(this, false);

   return 0;
}

inline PIXEL_list
get_section(PIXEL_list& pts, double s0, double s1)
{
   // Pull out the section of the given PIXEL_list corresponding
   // to the interval in parameter space from s0 to s1.

   // XXX - belongs in mlib/points.H

   pts.update_length();

   PIXEL_list ret;
   if (!(in_interval(s0, 0.0, 1.0) && in_interval(s1, 0.0, 1.0) && s0 < s1)) {
      err_msg("get_section: bad parameters: %f, %f", s0, s1);
      return ret;
   }
   if (pts.empty()) {
      err_msg("get_section: empty list");
      return ret;
   }
   VEXEL v0, v1; int k0, k1; double t0, t1;
   PIXEL p0 = pts.interpolate(s0, &v0, &k0, &t0);
   PIXEL p1 = pts.interpolate(s1, &v1, &k1, &t1);
   ret += p0;
   for (int k=k0+1; k<=k1; k++)
      ret += pts[k];
   if (t1 > 0)
      ret += p1;
   return ret;
}

inline PIXEL_list
trim_endpt_overlap(CPIXEL_list& pts)
{
   // HACK: for a PIXEL_list that basically forms a closed loop, trim
   // away the last bit of it if that bit overlaps with the start.
   // Used in the ellipse computation, below, since otherwise these
   // overlapping closed loops aren't recognized well, due to the way
   // GESTURE::is_ellipse() calculates the "center": by averaging all
   // the points. When there is an overlap, it skews the "center"
   // toward the side with the overlap.

   // Return value is the original list,
   // with some points removed from the end:
   PIXEL_list ret = pts;

   // Fraction of curve considered to be near either "end"
   double END_ZONE = 0.15; 

   // Extract the first 15% of the polyline as a separate polyline:
   PIXEL_list start = get_section(ret, 0, END_ZONE);

   // Find the index of the point near the start of the last 15%:
   int last_k;
   ret.interpolate(1 - END_ZONE, 0, &last_k, 0);

   // Pick off the last few points that are sufficiently close to the
   // start of the polyline.
   PIXEL foo; int fum;
   for (int k=pts.num()-1; k>last_k; k--) {
      if (start.closest(ret[k], foo, fum) < 10.0)
         ret.pop();
      else
         break;
   }
   return ret;
}

bool
GESTURE::is_ellipse(
   PIXEL& center,       // returned: center of the ellipse
   VEXEL& axis,         // returned: principle axis (unit length)
   double& r1,          // returned: magnitude of main radius
   double& r2,          // returned: magnitude of smaller radius
   double err_mult      // multiplier for max_err
   ) const
{
   ////////////////////////////////////////////////////////////
   // Find the "best-fit" ellipse for the 2D pixel trail of
   // the GESTURE. Return true if the ellipse satisfies the
   // given error tolerance, false otherwise.
   //
   // The error is computed by averaging the "distance" from
   // each point to the ellipse, measured along the line
   // thru its center.
   //
   // 5-step plan:
   //
   // (1) Compute the "center" of the ellipse, for now using
   //     a possibly erroneous method (see comment below).
   //
   // (2) Express the 2D points in the coordinate system with
   //     that center at its origin.
   //
   // (3) We seek 3 values a, b, and c such that:
   //
   //        ax^2 + 2bxy + cy^2 = 1
   //
   //     for each 2D point (x,y). For n points, this amounts to
   //     n equations with 3 unknowns, which can be expressed as:
   //
   //       (nx3)(3x1)  (nx1)
   //         X    A   =  J
   //
   //     where A[0] = a, A[1] = 2b, A[2] = c, J is the (nx1)
   //     matrix of 1's, and row i of the matrix X consists of:
   //
   //        x^2  xy  y^2
   //
   //     computed from the ith 2D point (x,y).
   //
   //     Let Xt denote the transpose of X. We compute a
   //     least-squares solution for A:
   //
   //      (3x1)   (3x3)  (3xn)(nx1)   
   //        A  = (Xt X)^-1 Xt   J
   //
   //  (4) Diagonalize the matrix:
   //       
   //         a  b
   //         b  c
   //
   //      to find the axes and radii of the ellipse.
   //
   //  (5) Check the goodness of the fit and reject anything
   //      that's not sufficiently ellipse-like.
   // 
   ////////////////////////////////////////////////////////////

   // XXX - Print debug messages if needed.
   // 
   // In case this method gets called more than once on the same
   // GESTURE, just print messages the first time.
   static const GESTURE* last_gesture = 0;
   static bool do_debug = Config::get_var_bool("DEBUG_ELLIPSE",false);
   bool debug = do_debug && last_gesture != this;
   last_gesture = this;

   // Step 0.
   //
   // Trivial reject.
   int n = _pts.num(), k;
   if (n < 8) {
      err_adv(debug, "GESTURE::is_ellipse: too few points", n);
      return false;
   }

   if (!is_loop()) {
      err_adv(debug, "GESTURE::is_ellipse: it's not a loop");
      return false;
   }

   // Step 1.
   //
   // XXX - In computing the "center," a small error can
   // have a large impact. The following requires regular
   // spacing of the input points. A more robust approach
   // would be to compute the "center of mass" of the
   // polyline, treating it as a thin wire with uniform
   // density. Or probably better, include the computation
   // of the "center" in the least-squares computation.
   //
   // XXX - because overlaps really skew the results, we're
   //       using this hack to trim away overlaps:
   center = trim_endpt_overlap(_pts).average();      

   // Step 2.
   //
   // Change of coordinates -- put origin at center
   ARRAY<VEXEL> P(n); // transformed points
   for (k=0; k<n; k++)
      P[k] += (_pts[k] - center);

   // Step 3.
   //
   // Set up (3 x n) matrix Xt, where column i contains
   // values x^2, xy, y^2 from point i:
   //
   // XXX - Could use a general matrix class about now
   double* Xt[3];
   Xt[0] = new double [ n ];
   Xt[1] = new double [ n ];
   Xt[2] = new double [ n ];

   for (k=0; k<n; k++) {
      Xt[0][k] = P[k][0] * P[k][0];     // x^2
      Xt[1][k] = P[k][0] * P[k][1];     // xy
      Xt[2][k] = P[k][1] * P[k][1];     // y^2
   }
   
   Wvec XtJ;      // Xt times (n x 1) vector of 1's
   Wtransf XtX;   // Xt times X, a (3x3) matrix
   XtX(0,0) = XtX(1,1) = XtX(2,2) = 0;  // start with all zeros
   for (k=0; k<n; k++) {
      // fill in entries of XtX and XtJ
      for (int i=0; i<3; i++) {
         XtJ[i] += Xt[i][k];       // XtJ[i] = row sum i
         for (int j=0; j<3; j++) {
            XtX(i,j) += Xt[i][k] * Xt[j][k];
         }
      }
   }

   // Thanks for the memories
   delete [] Xt[0];
   delete [] Xt[1];
   delete [] Xt[2];

   // Least-squares solve:
   Wvec A = XtX.inverse() * XtJ;

   // Coordinates of symmetric matrix describing the
   // quadratic form associated with the ellipse:
   //
   //    a  b
   //    b  c
   //
   double a = A[0], b = A[1]/2, c = A[2];

   // Step 4.
   //
   // Find eigenvalues of the matrix:
   double det = sqrt(sqr(a-c) + 4*sqr(b));
   double lambda_1 = (a + c + det)/2;
   double lambda_2 = (a + c - det)/2;

   if (lambda_1 <= 0 || lambda_2 <= 0) {
      err_adv(debug, "GESTURE::is_ellipse: non-positive eigenvalues in matrix");
      return false;
   }

   // Make sure lambda_1 has greatest magnitude
   if (lambda_2 > lambda_1) {
      // I think this never happens
      err_adv(debug, "GESTURE::is_ellipse: warning: a + c < 0");
      swap(lambda_1, lambda_2);
   }

   // Both must be non-negligible:
   if (lambda_2 < 1e-6) {
      err_adv(debug, "GESTURE::is_ellipse: vanishing eigenvalue (%f)", lambda_2);
      return false;
   }

   // Find the long axis of the ellipse, which is aligned
   // with the 2nd eigenvector (i.e. for lambda_2). The
   // following two rows must both be perpendicular to the
   // 1st eigenvector, and so both are parallel to the 2nd
   // eigenvector.
   VEXEL row1(a - lambda_1,      b      );
   VEXEL row2(     b,       c - lambda_1);

   // For robustness, take the longest one:
   axis = (row1.length() > row2.length()) ? row1 : row2;

   // Normalize it:
   axis = axis.normalized();
   if (axis.is_null()) {
      err_adv(debug, "GESTURE::is_ellipse: can't find principle axis");
      return false;
   }

   // Compute the radii of the ellipse
   r1 = 1 / sqrt(lambda_2);
   r2 = 1 / sqrt(lambda_1);

   // Step 5.
   //
   // Compute average distance of each gesture point to
   // the ellipse. If it is under the max allowable error,
   // the gesture is an ellipse.

   double max_err = err_mult * ellipse_max_err(this);
   double     err = ELLIPSE(center, axis, r1, r2).avg_dist(_pts);
   err_adv(debug, "GESTURE::is_ellipse: err %f, max: %f", err, max_err);
   bool success = (err <= max_err);
   if (debug)
      WORLD::create(new DEBUG_ELLIPSE(center, axis, r1, r2), false);

   return success;
}

void   
GESTURE::print_stats() const
{
   // Show various measurements of the gesture

   cerr << endl;
   err_adv(0, "startup time:   %1.2f", startup_time());
   err_adv(0, "remaining time: %1.2f", remaining_time());
   err_adv(1, "speed:          %1.1f", speed());
   err_adv(1, "winding:        %1.4f", winding());
   err_adv(1, "winding_abs:    %1.4f", winding_abs());
   err_adv(0, "endpoint_dist:  %1.1f", endpoint_dist());
   err_adv(1, "length:         %1.1f", length());
   err_adv(1, "straightness:   %1.3f", straightness());
   err_adv(0, "Corners :       %d",    num_corners());
   cerr << endl;
}

void
GESTURE::print_types() const
{
   // get the error messages out of the way first:
   is_circle();

   cerr << "gesture is: ";
   if (is_stroke())     cerr << "stroke, ";
   if (is_tap())        cerr << "tap, ";
   if (is_double_tap()) cerr << "double tap, ";
   if (is_line())       cerr << "line, ";
   if (is_slash())      cerr << "slash, ";
   if (is_dslash())     cerr << "dslash, ";
   if (is_tslash())     cerr << "tslash, ";
   if (is_zip_zap())    cerr << "zip-zap, ";
   if (is_small_arc())  cerr << "small arc, ";
   if (is_dot())        cerr << "dot, ";
   if (is_scribble())   cerr << "scribble, ";
   if (is_closed())     cerr << "closed, ";
   if (is_loop())       cerr << "loop, ";
   if (is_lasso())      cerr << "lasso, ";
   if (is_circle())     cerr << "circle, ";
   if (is_small_circle())cerr << "small circle, ";
   if (is_ellipse())    cerr << "ellipse, ";
   if (is_n_line())     cerr << "n_line, ";
   if (is_e_line())     cerr << "e_line, ";
   if (is_s_line())     cerr << "s_line, ";
   if (is_w_line())     cerr << "w_line, ";
   if (is_ne_line())    cerr << "ne_line, ";
   if (is_se_line())    cerr << "se_line, ";
   if (is_sw_line())    cerr << "sw_line, ";
   if (is_nw_line())    cerr << "nw_line, ";
   if (is_x())          cerr << "x, ";
   if (is_click_hold()) cerr << "click_hold, ";
   if (is_press_hold()) cerr << "press_hold, ";
   cerr << endl;
}

int 
GESTURE::draw(CVIEWptr &v) 
{
   return 0;
}

int 
GESTURE::draw_final(CVIEWptr &v) 
{
   return _drawer ? _drawer->draw(this, v) : 0;
}

inline double
pressure_to_grey(double p)
{
   // map stylus pressure to greyscale non-linearly.

   // pressure is in range [0,1]

   // treat all pressures as being at least this:
   const double MIN_PRESSURE = 0.3; 

   // greyscale color at min pressure
   const double GREY0 = 0.6; 

   // this function increases in darkness from GREY0 at MIN_PRESSURE
   // to black at pressure 1. the square root makes darkness increase
   // rapidly for light pressures, then taper off to black as pressure
   // nears 1:
   return GREY0*(1 - sqrt(max(p, MIN_PRESSURE) - MIN_PRESSURE));
}

int
GestureDrawer::draw(const GESTURE* gest, CVIEWptr& v)
{
   // default drawing functionality
   // subclasses can handle fancy stroke drawing

   // load identity for model matrix
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
   const PIXEL_list&    pts   = gest->pts();
   const ARRAY<double>& press = gest->pressures();
   glBegin(GL_LINE_STRIP);
   for (int k=0; k< pts.num(); k++) {
      double grey = pressure_to_grey(press[k]);
      glColor3d(grey, grey, grey);      // GL_CURRENT_BIT
      glVertex2dv(pts[k].data());
   }
   glEnd();

   // I tried to implement some code for corner getting highlighted
   // it is on the gesture.C version on Unnameable

   GL_VIEW::end_line_smooth();

   glPopAttrib();
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();

   return 0;
}

/* end of file gesture.C */
