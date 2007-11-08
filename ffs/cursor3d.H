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
 * cursor3d.H
 *****************************************************************/
#ifndef CURSOR3D_H_IS_INCLUDED
#define CURSOR3D_H_IS_INCLUDED

/*!
 *  \file cursor3d.H
 *  \brief Contains the declaration of the Cursor3D class.
 *
 *  \ingroup group_FFS
 *  \sa cursor3d.C
 *
 */

#include <map>
#include "disp/colors.H"
#include "gest/draw_widget.H"
#include "tess/tess_cmd.H"

using namespace mlib;

/*****************************************************************
 * Cursor3D:
 *****************************************************************/
MAKE_PTR_SUBC(Cursor3D,DrawWidget);
typedef const Cursor3D    CCursor3D;
typedef const Cursor3Dptr CCursor3Dptr;

//! \brief Axis that can be used to interactively define a plane
//! to draw in. Vector y defines the plane normal.
class Cursor3D : public DrawWidget {
 public:

   enum axis_t { X_AXIS=0, Y_AXIS, Z_AXIS, NUM_AXES };

   //******** MANAGERS ********

   Cursor3D();
   virtual ~Cursor3D();

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("Cursor3D",Cursor3D*, DrawWidget, CDATA_ITEM *);

   //******** STATICS ********

   static Cursor3Dptr get_active_instance() { return upcast(_active); }

   static Cursor3D* find_cursor(GEOMptr g);
   static Cursor3D* attach(GEOMptr g);
   void detach();

   //******** ACCESSORS ********

   CWpt&  origin()      const { return _origin; }

   CWvec& X()           const { return _x; }
   CWvec& Y()           const { return _y; }
   CWvec& Z()           const { return _z; }

   Wvec scaled_x()      const { return _x*axis_scale(); }
   Wvec scaled_y()      const { return _y*axis_scale(); }
   Wvec scaled_z()      const { return _z*axis_scale(); }
   
   Wpt   x_tip()        const { return _origin + scaled_x(); }
   Wpt   y_tip()        const { return _origin + scaled_y(); }
   Wpt   z_tip()        const { return _origin + scaled_z(); }
   
   VEXEL film_x()       const { return PIXEL(x_tip())-PIXEL(_origin); }
   VEXEL film_y()       const { return PIXEL(y_tip())-PIXEL(_origin); }
   VEXEL film_z()       const { return PIXEL(z_tip())-PIXEL(_origin); }

   void toggle_show_box() { _show_box = !_show_box; }

   // selected axis:
   int sel_i() const { return _selected; }
   CWvec& ax(int i) const {
      assert(0 <= i && i < 3);
      return (&_x)[i];
   }
   CWvec& sel() const { return ax(sel_i()); }

   // how it appears on-screen:
   Wpt sel_tip() const { return origin() + sel()*axis_scale(); }
   VEXEL film_sel() const { return PIXEL(sel_tip()) - PIXEL(origin()); }

   Wplane get_plane() const { return Wplane(_origin, sel()); }
   Wline  get_line()  const { return Wline(_origin, sel()); }

   Wline view_ray() const { return Wline(DEVice_2d::last->cur()); }

   void move_to(CWpt& o, CWvec& x=Wvec::X(), CWvec& y=Wvec::Y()) {
      set(o, x, y);
   }

   void set_origin(CWpt& o);
   void set(CWvec x, CWvec y) {
      _y = y.normalized();                       // preference to the normal
      _x = x.orthogonalized(_y).normalized();
      _z = cross(_x,_y).normalized();
   }
   void set(CWpt& o, CWvec& x, CWvec& y) {
      set_origin(o);
      set(x,y);
   }
   void set_scale_x(double s) { _s[0] = s; }
   void set_scale_y(double s) { _s[1] = s; }
   void set_scale_z(double s) { _s[2] = s; }

   double view_scale()    const;
   double axis_scale()    const;

   bool get_shadow(Wpt& shadow) const;

   //******** COLORS ********

   CCOLOR&  regular_color() const { return Color::orange; }
   CCOLOR& selected_color() const { return Color::grey4;  }
   CCOLOR&    plane_color() const { return Color::white;  }
   CCOLOR&      box_color() const { return Color::grey9;  }
   CCOLOR&   shadow_color() const { return Color::grey2;  }

   COLOR axis_color(axis_t a) const {
      return (_selected == a) ? selected_color() : regular_color();
   }
   
   //******** EVENTS ********

   void get_x       (CXYpt& xy);
   int  get_cam_tap (CXYpt& xy);

   void cam_focus(CWvec& axis);

   //******** DrawWidget METHODS ********

   virtual void toggle_active();

   virtual BMESHptr bmesh() const { return gel_to_bmesh(_attached); }

   //******** DRAW FSA METHODS ********

   virtual int  tap_cb          (CGESTUREptr& gest, DrawState*&);
   virtual int  scribble_cb     (CGESTUREptr& gest, DrawState*&);

   //******** INTERACTOR METHODS ********

   virtual int down_cb          (CEvent&, State*&);
   virtual int up_cb            (CEvent&, State*&);

   virtual int y_choose_cb      (CEvent&, State*&);

   virtual int xlate_axis_cb    (CEvent&, State*&);
   virtual int xlate_plane_cb   (CEvent&, State*&);
   virtual int rotate_cb        (CEvent&, State*&);
   virtual int resize_z_cb      (CEvent&, State*&);
   virtual int resize_x_cb      (CEvent&, State*&);
   virtual int resize_uni_cb    (CEvent&, State*&);

   //******** GEOM methods ********

   virtual int  interactive(CEvent&, State*&, RAYhit* =0) const;

   // first argument is new transform,
   // 2nd and 3rd arguments are ignored:
   virtual void write_xform(mlib::CWtransf&, mlib::CWtransf&, CMOD&);

   //******** GEL METHODS ********

   virtual RAYhit &intersect  (RAYhit&, CWtransf& =Identity, int=0)const;
   virtual int     draw       (CVIEWptr &v);
   virtual BBOX    bbox       (int i=0) const;
   virtual bool    needs_blend() const { return 1; }

 protected:
   enum intersect_t {
      INTERSECT_NONE = 0,
      INTERSECT_ORIGIN,
      INTERSECT_X,
      INTERSECT_Y,
      INTERSECT_Z,
      INTERSECT_FRAME_CORNER,
      INTERSECT_FRAME_Z_SIDE,
      INTERSECT_FRAME_X_SIDE,
      INTERSECT_SHADOW
   };

   intersect_t intersect(CPIXEL& p, Wpt& hit, Wvec& norm) const;
   intersect_t intersect(CPIXEL& p) const {
      Wpt hit; Wvec norm; return intersect(p, hit, norm);
   }

   Wpt          _origin;          //!< origin of axis
   Wvec         _x;               //!< the
   Wvec         _y;               //!<   three
   Wvec         _z;               //!<     axes
   axis_t       _selected;        //!< selected axis

   intersect_t  _down_intersect;  //!< records last intersect result
   PIXEL        _last_pt;         //!< pixel location of last mouse event
   Wpt          _last_wpt;        //!< intersected Wpt for last mouse event
   Wvec         _cur_axis;        //!< axis to translate along

   Wvec         _s;               //!< x,y,z scale factors

   bool         _show_box;

   GEOMptr      _attached;        //!< object attached to this
   XFORM_CMDptr _cmd;             //!< command for transforming _attached

   // FSA states:
   State        _y_choose;
   State        _xlate_plane;
   State        _xlate_axis;
   State        _rotate;
   State        _frame_resize;
   State        _frame_resize_z;
   State        _frame_resize_x;

   stop_watch   _clock;         //!< for timing events

   static Event _down;
   static Event _move;
   static Event _up;
 public:
   static void cache_events(CEvent& d, CEvent& m, CEvent& u) {
      _down = d;
      _move = m;
      _up = u;
   }
 protected:
   typedef map<GEOM*,Cursor3D*>::const_iterator citer_t;
   typedef map<GEOM*,Cursor3D*>::      iterator  iter_t;
   static map<GEOM*,Cursor3D*> _map;

   //******** INTERNAL METHODS ********

   //! for defining callbacks for regular events
   typedef CallMeth_t<Cursor3D,Event> dcb_t;
   dcb_t* DACb(dcb_t::_method m)           { return new dcb_t(this, m); }
   dcb_t* DACb(dcb_t::_method m, State* s) { return new dcb_t(this, m, s); }

   //! for defining callbacks to use with GESTUREs:
   typedef CallMeth_t<Cursor3D, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

   void set_attached(GEOMptr g);

   void get_plane_corners(Wpt& a, Wpt& b, Wpt& c, Wpt& d) const;

   //******** GEOM VIRTUAL METHODS ********

   // animate camera to look straight onto floor
   virtual bool do_cam_focus(CVIEWptr& view, CRAYhit& r);
};

#endif // CURSOR3D_H_IS_INCLUDED

// end of file cursor3d.H
