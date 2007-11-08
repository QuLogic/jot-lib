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
/*! ********************************************************************
 * \file gesture.H
 *
 **********************************************************************/
#ifndef GESTURE_H_HAS_BEEN_INCLUDED
#define GESTURE_H_HAS_BEEN_INCLUDED

#include "disp/view.H"
#include "geom/command.H"
#include "geom/fsa.H"
#include "mlib/points.H"
#include "mlib/vec2.H"
#include "std/stop_watch.H"
#include "std/config.H"

extern const double MIN_GESTURE_LENGTH;
extern const double MIN_GESTURE_SPREAD;

class GestureDrawer;
class GEST_INT;

MAKE_PTR_SUBC(GESTURE,GEL);
typedef const GESTURE CGESTURE;
typedef const GESTUREptr CGESTUREptr;


/*!***************************************************************
 * GESTURE
 *
 *      Class that defines a gesture -- a pixel trail drawn by
 *      the user, with timing info and ability to "recognize"
 *      certain meanings, like a tap, a straight line, a circle,
 *      a dot, etc.
 *****************************************************************/

class GESTURE : public GEL {
 public:

   //******** MANAGERS ********

   // constructor that begins to build the gesture
   GESTURE(GEST_INT* gi, int index, mlib::CPIXEL& p, double pressure,
           GestureDrawer* drawer=0, CEvent& down = Event()) :
      _gest_int(gi), _index(index),
      _start_frame(0), _end_frame(0),
      _drawer(drawer), _complete(false) { init(p, down, pressure); }


   GESTURE(CGESTURE& gest) :
     _gest_int(0), _index(0), 
     _pts(gest.pts()), _times(gest.timing()), _pressures(gest.pressures()),
     _start_frame(0), _end_frame(0),
     _bbox(gest.bbox(0)), _pix_bbox(gest.pix_bbox()),
     _complete(true){}

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("GESTURE", GESTURE*, GEL, CDATA_ITEM *);

   //******** BUILDING METHODS ********

   void init    (mlib::CPIXEL& p, CEvent& down, double pressure);
   void add     (mlib::CPIXEL& p, double min_dist, double pressure);
   void complete(mlib::CPIXEL& p, CEvent& up = Event());
   void trim();

   //******** CONFIGURING ********

   GestureDrawer* drawer() { return _drawer; }
   void set_drawer(GestureDrawer* drawer) { _drawer = drawer; }

   //******** UNDO ********

   // in case the gesture triggered some action and
   // later it was decided to interpret the gesture in
   // a whole nuther way, it's possible to undo the
   // first action via the following mechanism
   void set_command(CCOMMANDptr& u) { _cmd = u; }
   void undo() {
      if (_cmd)
         _cmd->undoit();
   }
   
   //******** ACCESSORS/EVALUATORS ********

   /// pixel locations:
   const  mlib::PIXEL_list& pts()     const { return _pts; }
  /// first pixel in the gesture
   mlib::PIXEL  start()               const { return _pts[0]; }
  /// last pixel in the gesture
   mlib::PIXEL  end()                 const { return _pts.last(); }
  /// centroid of the gesture computed as the average
   mlib::PIXEL  center()              const { return _pts.average(); }
  /// vector from starting point to endpoint
   mlib::VEXEL  endpt_vec()           const { return (end() - start()); }
  /// mid point of line segment between start and end points in pixel space
   mlib::PIXEL  endpt_midpt()         const { return (end() + start())*0.5; }
  /// length of the gesture along its curve(s)
   double length()              const { return _pts.length(); }
  /// straight-line pixel distance between start and end points
  double endpoint_dist()       const { return start().dist(end()); }
  /// pixel-space distance between this gesture and the one provided, computed as distance between centers
   double dist(CGESTUREptr& g)  const { return center().dist(g->center()); }
   bool   complete()            const { return _complete; }

   /// run a single pass of smoothing on the point list:
   void smooth_points();

   /// run n passes of smoothing on the point list:
   void smooth_points(int n);

   /// max distance to center
   double spread() const { return _pts.spread(); }

   /// reflect points about a line
   void reflect_points(const mlib::PIXELline& l);

   void fix_endpoints(mlib::CPIXEL& a, mlib::CPIXEL& b) {
      _pts.fix_endpoints(a,b);
   }

   mlib::PIXELline  endpt_line() const { return mlib::PIXELline(start(), end()); }
   mlib::PIXEL_list endpt_seg()  const {
      mlib::PIXEL_list ret; ret += start(); ret += end(); return ret;
   }

  /// does gesture intersect itself?
   bool self_intersects()       const { return _pts.self_intersects(); }

  /// does gesture intersect the line provided?
   bool intersects_line(const mlib::PIXELline& l) const {
      return _pts.intersects_line(l);
   }

  /// does gesture intersect the line segment provided?
   bool intersects_seg(const mlib::PIXELline& l) const {
      return _pts.intersects_seg(l);
   }

   // thresholds applied to general strokes
  /// is the gesture length less than the minimum gesture length?
   bool below_min_length() const { return length() < MIN_GESTURE_LENGTH; }
  /// is the gesture length below the minimum allowed spread (max distance to center)
   bool below_min_spread() const { return length() < MIN_GESTURE_SPREAD; }

   // timing:
   CARRAY<double>& timing()     const { return _times; }
  /// time at which first pixel was drawn
   double start_time()          const { return _times[0]; }
  /// time at which last pixel was drawn
   double end_time()            const { return _times.last(); }
  /// time that elapsed between first and last pixels drawn
   double elapsed_time()        const { return end_time() - start_time(); }
  /// time that elapsed between the drawing of the first pixel and pixel whose index is given
   double elapsed_time(int i)   const { return _times[i] - start_time(); }
  /// time that elapsed between the drawing of the pixel whose index is given, and the last pixel in the gesture
   double remaining_time(int i) const { return end_time() - _times[i]; }
  /// time that elapsed between the completion of this gesture and the start of the gesture provided
   double elapsed_time(CGESTUREptr& g) const {
      return fabs(end_time() - g->start_time());
   }
   double between_time(CGESTUREptr& g) const {
      return fabs(start_time() - g->end_time());
   }
   double age() const { return stop_watch::sys_time() - end_time(); }

   // pressure:
   CARRAY<double>& pressures()  const  { return _pressures; }
   void  set_pressure(int i, double p) { if (i>0 && i<_pressures.num()) _pressures[i] = p; }

   // measurements
   double radius()              const; ///< avg distance to center
   double speed()               const; ///< length divided by elapsed time

   /// Time spent within the given threshold distance of the 1st point.
   /// Used to measure a gesture that begins w/ a pause.
   double startup_time(
      double dist_thresh=Config::get_var_dbl("GEST_STARTUP_DIST_THRESH",10.0,true)
      ) const;

   double remaining_time(
      double dist_thresh=Config::get_var_dbl("GEST_STARTUP_DIST_THRESH",10.0,true)
      ) const {
      // Time spent after the "startup" period within the given
      // distance of the 1st point.
      return elapsed_time() - startup_time(dist_thresh);
   }

  /// how straight is the gesture? returns 1 for straight and tends to zero as curvature increases
   double straightness() const {
      double l = length(); return (l > 0) ? endpoint_dist()/length() : 0;
   }
   double winding    (bool do_trim=1, bool abs=0) const;
   double winding_abs(bool do_trim=1) const { return winding(do_trim, true); }

   int  index()                 const   { return _index; }
   void set_index(int k)                { _index = k; }

   GESTUREptr prev(int k=1) const;

   //******** RECOGNIZERS ********

   // Returns true if pts[i] is an endpoint, or an internal
   // point where exterior angle is large:
   bool is_corner(int i)  const;
   int  num_corners()     const  { return corners().num(); }
   ARRAY<int> corners()   const;

   virtual bool is_stroke()      const; // anything formed by down/up events

   // straight line:
   virtual bool is_line(double min_straightness, double min_len) const;
   // gets values from environment variables JOT_LINE_MIN_STRAIGHTNESS
   // and JOT_LINE_MIN_LEN, or uses 0.95 and 20 (pixels) by default:
   virtual bool is_line() const;

   virtual bool is_tap()         const;
   virtual bool is_double_tap()  const;
   virtual bool is_slash()       const; ///< quick, short, straight stroke
   virtual bool is_dslash()      const; ///< delayed slash (starts w/ pause)
   virtual bool is_tslash()      const; ///< tap-slash (tap, then slash)
   virtual bool is_slash_tap()   const; ///< slash/tap
   virtual bool is_dot()         const; ///< small, tight spiral
   virtual bool is_scribble()    const; ///< used for crossing out stuff

   virtual bool is_zip_zap()     const; ///< forward/back slash
   virtual bool is_arc()         const; ///< slightly curving stroke
   virtual bool is_small_arc()   const; ///< a small arc (eg under 60 pixels long)

   virtual bool is_closed()      const;
   virtual bool is_loop()        const;
   virtual bool is_lasso()       const;

   virtual bool is_circle(double max_ratio=1.25) const;

   virtual bool is_small_circle(double max_ratio=1.25) const;

   virtual bool is_ellipse(
      mlib::PIXEL& center, mlib::VEXEL& axis,
      double& r1, double& r2, double err_mult=1
      ) const;

   virtual bool is_almost_ellipse(mlib::PIXEL& center, mlib::VEXEL& axis,
                                  double& r1, double& r2) const {
      return is_ellipse(center, axis, r1, r2,
      Config::get_var_dbl("ALMOST_ELLIPSE_MULTIPLIER", 3,true)
         );
   }

   virtual bool is_ellipse() const {
      mlib::PIXEL center; mlib::VEXEL axis; double r1=0, r2=0;
      return is_ellipse(center, axis, r1, r2);
   }

   virtual bool is_n_line()      const;
   virtual bool is_e_line()      const;
   virtual bool is_s_line()      const;
   virtual bool is_w_line()      const;
   virtual bool is_ne_line()     const;
   virtual bool is_se_line()     const;
   virtual bool is_sw_line()     const;
   virtual bool is_nw_line()     const;
   virtual bool is_x()           const; // two short straight lines forming X

   virtual bool is_click_hold()  const;
   virtual bool is_press_hold()  const;

   //******** DIAGNOSTIC ********

   void print_stats() const;
   void print_types() const;

   //******** GEL METHODS ********

   virtual int draw(CVIEWptr &v);
   virtual int draw_final(CVIEWptr &v);

   // because gestures are antialiased they have "transparency":
   virtual bool needs_blend()           const { return true; }

   virtual BBOX        bbox(int)        const { return _bbox; }
   virtual BBOXpix     pix_bbox()    const { return _pix_bbox; }

   //******** DATA_ITEM METHODS ********

   virtual DATA_ITEM *dup() const {
      // XXX - no need for duping
      return 0;
      // return new GESTURE(_drawer);
   }

 protected:

   //******** MEMBER DATA ********

   GEST_INT*            _gest_int;      ///< owner of this
   int                  _index;         ///< index of this in owner's list
   mlib::PIXEL_list           _pts;     ///< pixel trail
   ARRAY<double>        _times;         ///< times for each pixel added
   ARRAY<double>        _pressures;     ///< pressures recorded for each pixel
   int                  _start_frame;   ///< frame number of down event
   double               _end_frame;     ///< frame number of up event
   Event                _down, _up;     ///< down and up events
   GestureDrawer*       _drawer;        ///< thing that can draw this
   COMMANDptr           _cmd;           ///< how to undo previous result of this
   BBOX                 _bbox;
   BBOXpix              _pix_bbox;
   bool                 _complete;     ///< if complete() was called...

   //******** INTERNAL METHODS ********

   // one reason they're internal is they don't do error checking
   mlib::VEXEL  vec  (int i) const { return _pts[i] - _pts[i-1]; }
   mlib::VEXEL  vecn (int i) const { return vec(i).normalized(); }
   double angle(int i) const { return mlib::Acos(vecn(i)*vecn(i+1)); }
};

/*****************************************************************
 * GestureDrawer
 *
 *      Base class for object that can draw a PIXEL_list
 *      (I.e. a screen-space polyline). Draws using an OpengGL
 *      line strip; derived types might use strokes or
 *      something.
 *****************************************************************/
class GestureDrawer {
 public:
   virtual ~GestureDrawer() {}
   virtual GestureDrawer* dup() const { return new GestureDrawer; }
   virtual int draw(const GESTURE*, CVIEWptr& view);
};

#endif  // GESTURE_H_HAS_BEEN_INCLUDED

// end of file gesture.H
