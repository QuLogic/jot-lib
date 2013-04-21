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
#ifndef JOT_CAM_FP_H
#define JOT_CAM_FP_H

#include "disp/cam.H"
#include "manip/manip.H"
#include "manip/cam_pz.H"
#include "widgets/collide.H"
#include "std/stop_watch.H"
#include "base_jotapp/base_jotapp.H"
#include "widgets/collide.H"




/*****************************************************************
 * Cam_int_fp:
 *
 *      The jot camera interactor.
 *****************************************************************/
class Cam_int_fp : public Interactor<Cam_int_fp,Event,State>,
                          public UPobs {
private:
  static void schedule_in_view(VIEWptr v, CamFocus* cf);

  protected:

   // is this used?
   class REF_CLASS(CAMwidget) {
      protected:
	GEOMptr    _anchor; 

	int        _a_displayed;
      public:
	       CAMwidget();
	int    anchor_displayed(){ return _a_displayed; }
	mlib::Wpt    anchor_wpt()      { return mlib::Wpt(_anchor->xform()); }
	mlib::XYpt   anchor_pos()      { return mlib::XYpt(anchor_wpt()); }
	double anchor_size()     { return _anchor->xform().X().length(); }
	void   undisplay_anchor();
	void   display_anchor(mlib::CWpt  &p);
	void   drag_anchor   (mlib::CXYpt &p2d);
   };

   Gravity*		 _gravity; 
   Collide*		 _collision;
   CAMwidget     _camwidg;      // ?
   double        _dtime;
   double        _dist, _speed, _size;   //size: size of the 'player'
   mlib::PIXEL         _start_pix;
   mlib::XYpt          _scale_pt;
   mlib::Wpt           _down_pt;
   mlib::XYpt          _down_pt_2d;
   VIEWptr       _view;
   State         _cam_rot, _cam_forward, _cam_back, _cam_left, _cam_right, _cam_choose;
   State         _orbit, _breathe, _cruise, _grow, _grow_down;
   State         _orb_rot, _orb_zoom, _cruise_zoom;
   State         _breathe_rot, _cruise_rot;
   State         _move_view, _icon_click;

   State         _entry2, _but_trans, _but_rot, _but_zoom, _but_drag;

   ARRAY<UPobs*> _up_obs;
   int           _do_reset;
   GEOMptr       _geom;
   bool          _resizing, _breathing;
   CamIcon      *_icon;         // ?
   ICON2Dptr     _button;        //activated camera button
   XYpt			 _tp; 
   XYpt			 _te;
   stop_watch    _clock;         // clock measuring time 
   stop_watch    _land_clock;    // clock measuring time from
								 // cruise down commands
   stop_watch    _move_clock;    // clock measuring time from 
								 // previous mouse movement
   

  public:
           void add_up_obs(UPobs *o)   { _up_obs += o; }
   virtual int  predown(CEvent &e,State *&s);
   virtual int  down   (CEvent &e,State *&s);
   virtual int  down2   (CEvent &e,State *&s);


   virtual int  rot    (CEvent &e,State *&s); 
   virtual int  focus  (CEvent &e,State *&s);
   virtual int  choose  (CEvent &e,State *&s);
   virtual int  stop_orbit  (CEvent &e,State *&s);
   virtual int  orbit  (CEvent &e,State *&s);
   virtual int  cruise  (CEvent &e,State *&s);
   virtual int  cruise_zoom(CEvent &e,State *&s);
   virtual int  cruise_zoom_up(CEvent &e,State *&s);
   virtual int  cruise_down(CEvent &e,State *&s);
   virtual int  stop_actions (CEvent &e,State *&s);
   virtual int  stop_cruise  (CEvent &e,State *&s);
   virtual int  breathe  (CEvent &e,State *&s);
   virtual int  stop_breathe  (CEvent &e,State *&s);
   virtual int  orbit_zoom  (CEvent &e,State *&s);
   virtual int  orbit_rot  (CEvent &e,State *&s);
   virtual int  orbit_rot_up  (CEvent &e,State *&s);
   virtual int  move   (CEvent &e,State *&s);
   virtual int  forward (CEvent &e,State *&s);
   virtual int  back   (CEvent &e,State *&s);
   virtual int  left   (CEvent &e,State *&s);
   virtual int  right  (CEvent &e,State *&s);
   virtual int  moveup (CEvent &e,State *&s);
   virtual int  iconmove(CEvent &e,State *&s);
   virtual int  iconup (CEvent &e,State *&s);
   virtual int  up     (CEvent &e,State *&s);
   //virtual int  travel (CEvent &e,State *&s);
   virtual int  grow (CEvent &e,State *&s);
   virtual int  grow_change (CEvent &e,State *&s);

   virtual int  dragup (CEvent &e,State *&s);
   virtual int  noop   (CEvent & ,State *&) { return 0; }

   virtual int  toggle_buttons  (CEvent &e,State *&s);

   virtual ~Cam_int_fp() {}
            Cam_int_fp(CEvent &d, CEvent &m, CEvent &u, CEvent &d2, CEvent &u2, 
                    CEvent &dt, CEvent &dr, CEvent &dz, 
                    CEvent &ut, CEvent &ur, CEvent &uz);
   void     make_view (mlib::CXYpt &);
   virtual  void reset(int rst);
   State   *entry2    () { return &_entry2; }
};



#endif // JOT_CAM_FP_H

