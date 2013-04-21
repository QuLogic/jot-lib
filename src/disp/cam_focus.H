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
#ifndef CAM_FOCUS_H_IS_INCLUDED
#define CAM_FOCUS_H_IS_INCLUDED

#include "disp/view.H"



/*****************************************************************
 * CamFocus:
 *
 *      Animates a camera to focus on something
 *****************************************************************/
MAKE_PTR_SUBC(CamFocus, FRAMEobs);
class CamFocus : public FRAMEobs {
 public :
   //******** MANAGERS ********
   CamFocus(VIEWptr v, CCAMptr &dest);
   CamFocus(VIEWptr v, mlib::CWpt& from, mlib::CWpt& at, mlib::CWpt& up,
            mlib::CWpt& center, double fw=0, double fh=0);
       
   virtual ~CamFocus();

   //******** ACCESSORS ********
   VIEWptr view()  const { return _view; }
   CAMptr  cam()   const { return _cam; }

   //******** STATIC METHODS ********

   static void cancel_cur();

   //******** FRAMEobs VIRTUAL METHODS ********
   virtual int tick(void);

 protected:
   //******** DATA MEMBERS ********
   VIEWptr      _view;          // the VIEW owning the cam we're animating
   CAMptr       _cam;           // the cam we're animating

   double       _width;         // CAMdata parameter (at target)
   double       _height;        // CAMdata parameter (at target)

   double       _orig_time; // time (VIEW::frame_time()) when starting CamFocus
   double       _last_time; // time (VIEW::frame_time()) of last call to tick()
   double       _duration;  // length of time for animation to run

   double       _speed;
   double       _max_speed; // max allowable speed for moving camera location

   // camera parameters at start of animation (world coords)
   mlib::Wpt     _o1; // "from"
   mlib::Wpt     _a1; // "at"
   mlib::Wvec    _u1; // "up" vector
   mlib::Wpt     _c1; // "center"

   // camera parameters at end of animation
   mlib::Wpt     _o2; // "from"
   mlib::Wpt     _a2; // "at"
   mlib::Wvec    _u2; // "up" vector
   mlib::Wpt     _c2; // "center"

   //******** STATIC DATA ********

   static CamFocusptr  _cur; // only 1 instance active at a time

   //******** UTILITIES ********

   // called in the constructor to activate this CamFocus:
   void schedule() {
      if (view()) {
         view()->save_cam();     // allow undo
         view()->schedule(this); // sign up for per-frame callbacks
      }
   }

   // called in the destructor to deactivate this CamFocus:
   void unschedule() {
      if (view())
         view()->unschedule(this);
   }

   void set_cur(CamFocus* cf);

   // time utilities:
   double cur_time()     const { return VIEW::peek()->frame_time(); }
   double elapsed_time() const { return cur_time() - _orig_time; }
   double t_param()      const {
      // animation parameter:
      //   fraction of time used so far, in range [0,1]
      return clamp(elapsed_time()/_duration, 0.0, 1.0);
   }
   double cartoon_t() const;

   // compute cached values during initialization
   // input parameters are: from, at, up, and center for final camera
   void setup(
      mlib::CWpt& o2, mlib::CWpt& a2, mlib::CWvec& u2, mlib::CWpt& c2
      );
};

/*****************************************************************
 * CamBreathe:
 *
 *      Animates a camera to make it seem like it's "breathing"
 *****************************************************************/
MAKE_PTR_SUBC(CamBreathe, FRAMEobs);
class CamBreathe : public FRAMEobs {
 protected:
   CAMptr       _cam;
   mlib::Wpt    _from;
   mlib::Wpt    _at;
   mlib::Wpt    _up;
   mlib::Wpt    _cent;
   double       _width,_height;
   double       _min, _Kf, _Kc, _Ku;
   double       _speed;
   static double _size;
   double       _tick;
   bool                 _stop;
   bool                 _pause;
   const double kKf, kKc, kKu;
   static CamBreatheptr  _breathe;

 public :
   virtual ~CamBreathe();
   CamBreathe(CAMptr &p);

       
   virtual int tick(void);
   virtual void stop() { _stop = true; }
   virtual void pause()  { _pause = true; }
   virtual void unpause(){ _pause = false; }
   virtual void set_speed(double s) {_speed = s; }
   static void grow(double s) { _size += s; }

   virtual bool stopped() { return _stop; }

   static CamBreatheptr &cur()      { return _breathe; }
};

/*****************************************************************
 * CamOrbit:
 *
 *      Animates a camera to orbit around a object
 *****************************************************************/
MAKE_PTR_SUBC(CamOrbit, FRAMEobs);
class CamOrbit : public FRAMEobs {
 protected:
   CAMptr       _cam;
   mlib::Wpt    _from, _at,_up,_cent;
   double       _width,_height;
   double       _min;
   double       _speed;
   double       _tick;
   bool                 _pause;
   bool         _stop;
   mlib::XYpt         tp; 
   mlib::XYpt         te;
   static CamOrbitptr  _orbit;

 public :
   virtual ~CamOrbit();
   CamOrbit(CAMptr &p, mlib::Wpt center);

       
   virtual int tick(void);
   virtual void pause() { _pause = true; }
   virtual void unpause(){ _pause = false; }
   virtual void stop() { _stop = true; }
   virtual void set_target(mlib::Wpt p) { _cent = p;}

   virtual void set_orbit(mlib::XYpt o, mlib::XYpt e)
      { 
         _pause = false;
         tp    = o;
         te    = e;
      }

   virtual bool stopped() { return _stop; }
   static CamOrbitptr &cur()      { return _orbit; }
};

/*****************************************************************
 * CamCruise:
 *
 *      Animates a camera to Cruise around a object
 *****************************************************************/
MAKE_PTR_SUBC(CamCruise, FRAMEobs);
class CamCruise : public FRAMEobs {
 protected:
   CAMptr       _cam;
   double               _t;
   mlib::Wpt    _from, _at,_up,_cent;
   double       _width,_height;
   double       _min;
   double       _speed;
   double       _tick;
   static double _size;
   bool                 _pause;
   bool         _stop;
   bool                 _target;
   bool                 _travel;
   mlib::Wpt    _start;
   mlib::Wpt    _dest;
   mlib::XYpt   _scale_pt;
   mlib::Wpt    _down_pt;
   mlib::XYpt         tp; 
   mlib::XYpt         te;
   mlib::Wvec	_move_vec;
   mlib::Wpt    _spt;

   stop_watch    _clock;      // clock measuring time
   static CamCruiseptr  _cruise;

 public :
   virtual ~CamCruise();
   CamCruise(CAMptr &p, mlib::Wpt center);

       
   virtual int tick(void);
   virtual void pause() { _pause = true; }
   virtual void unpause(){ _pause = false; }
   virtual void stop() { _stop = true; }
   virtual void speed(double i) { _speed = i; }
   static void grow(double s) { _size += s; }

   //virtual void unset_target() { _target = false;}
   virtual void travel(mlib::Wpt p);
   virtual void set_cruise(mlib::XYpt o, mlib::XYpt e);


   virtual void set_scale_pt(mlib::XYpt s) { _scale_pt = s; }
   virtual void set_down_pt(mlib::Wpt d) { _down_pt = d; }
   
   virtual double get_speed() {return _speed;}
   virtual bool stopped() { return _stop; }
   static CamCruiseptr &cur()      { return _cruise; }
};

#endif // CAM_FOCUS_H_IS_INCLUDED

// end of file cam_focus.H 
