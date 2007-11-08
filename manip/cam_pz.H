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
#ifndef CAM_PZ_H_IS_INCLUDED
#define CAM_PZ_H_IS_INCLUDED

#include "disp/cam_focus.H"
#include "manip/manip.H"
#include "base_jotapp/base_jotapp.H"

/*****************************************************************
 * CamIcon
 *****************************************************************/
class CamIcon {
 protected:
   static CamIcon  *_orig_icon;
   virtual CamIcon *copy_icon(CXYpt &where, CCAMptr &cam) = 0;
   static ARRAY<CamIcon *> _icons;
 public:
   enum RESULT {
      RESIZE,
      FOCUS,
      MOVE
   };
   CamIcon() { _orig_icon = this; }
   virtual ~CamIcon() {}
   virtual RESULT    test_down(CEvent &e, State *&s)       = 0;
   virtual int       icon_move(CEvent &e, State *&s)       = 0;
   virtual int       resize_up(CEvent &e, State *&s)       = 0;
   virtual CCAMptr  &cam      () const                     = 0;
   virtual void      set_icon_loc(CXYpt &pt)               = 0;
   virtual bool     intersect_icon(CXYpt &pt)              = 0;
   virtual void     remove_icon   ()                       = 0;
   static CamIcon *intersect_all(CXYpt &pt);
   static CamIcon *create   (CXYpt &where, CCAMptr &cam) {
      if (_orig_icon) return _orig_icon->copy_icon(where, cam);
      return 0;
   }
};

/*****************************************************************
 * Cam_int:
 *
 *      The jot camera interactor.
 *****************************************************************/
class Cam_int : public Interactor<Cam_int,Event,State>,
                public UPobs {
 private:
   static void schedule_in_view(VIEWptr v, CamFocus* cf);

 protected:

   // represents the blue "camera ball":
   class REF_CLASS(CAMwidget) {
     protected:
      GEOMptr    _anchor; 
      bool       _a_displayed; // is anchor currently displayed?
     public:
      CAMwidget();
      bool   anchor_displayed(){ return _a_displayed; }
      Wpt    anchor_wpt()      { return Wpt(_anchor->xform()); }
      XYpt   anchor_pos()      { return XYpt(anchor_wpt()); }
      double anchor_size()     { return _anchor->xform().X().length(); }
      void   undisplay_anchor();
      void   display_anchor(CWpt  &p);
      void   drag_anchor   (CXYpt &p2d);
   };

   CAMwidget     _camwidg;
   double        _dtime;
   double        _dist;
   PIXEL         _start_pix;
   XYpt          _scale_pt;
   Wpt           _down_pt;
   XYpt          _down_pt_2d;
   VIEWptr       _view;
   State         _cam_pan, _cam_zoom, _cam_rot, _cam_choose, _cam_drag;
   State         _move_view, _icon_click;

   State         _entry2, _but_trans, _but_rot, _but_zoom, _but_drag;

   ARRAY<UPobs*> _up_obs;
   int           _do_reset;
   GEOMptr       _geom;
   bool          _resizing;
   CamIcon      *_icon;         // ?

 public:
   void add_up_obs(UPobs *o)   { _up_obs += o; }
   virtual int  predown(CEvent &e,State *&s);
   virtual int  down   (CEvent &e,State *&s);
   virtual int  down2  (CEvent &e,State *&s);
   virtual int  drag   (CEvent &e,State *&s);
   virtual int  choose (CEvent &e,State *&s);
   virtual int  rot    (CEvent &e,State *&s);
   virtual int  zoom   (CEvent &e,State *&s);
   virtual int  pan    (CEvent &e,State *&s);
   virtual int  rot2   (CEvent &e,State *&s);
   virtual int  zoom2  (CEvent &e,State *&s);
   virtual int  pan2   (CEvent &e,State *&s);
   virtual int  focus  (CEvent &e,State *&s);
   virtual int  move   (CEvent &e,State *&s);
   virtual int  moveup (CEvent &e,State *&s);
   virtual int  iconmove(CEvent &e,State *&s);
   virtual int  iconup (CEvent &e,State *&s);
   virtual int  up     (CEvent &e,State *&s);
   virtual int  dragup (CEvent &e,State *&s);
   virtual int  noop   (CEvent & ,State *&) { return 0; }

   virtual ~Cam_int() {}
   Cam_int(CEvent &d, CEvent &m, CEvent &u, CEvent &d2, CEvent &u2, 
           CEvent &dt, CEvent &dr, CEvent &dz, 
           CEvent &ut, CEvent &ur, CEvent &uz);
   void     make_view (CXYpt &);
   virtual  void reset(int rst);
   State   *entry2    () { return &_entry2; }
};

/*****************************************************************
 * Screen_pan_int:
 *
 *      This interactor translates a camera in the film plane by
 *      mapping the motion events from a 2D input device.
 *****************************************************************/
class Screen_pan_int : public Simple_int
{
 public:
   virtual ~Screen_pan_int() {}
   Screen_pan_int(CEvent  &d,
                  CEvent  &m,
                  CEvent  &u): Simple_int(d,m,u)
      { _entry += Arc(m,Cb((_callb::_method)&Screen_pan_int::idle)); }

   int  idle(CEvent &e,State *&) {
      DEVice_2d *dev = (DEVice_2d *)e._d;
      CAMdataptr data(e.view()->cam()->data());
      Wpt        dpt(data->from() + data->at_v() * data->distance());
      data->translate(-(Wpt(dev->cur(),dpt) - Wpt(dev->old(),dpt)));
   
      return 0;
   }
};

#endif // CAM_PZ_H_IS_INCLUDED
