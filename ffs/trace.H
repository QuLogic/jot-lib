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
#ifndef TRACE_H_IS_INCLUDED
#define TRACE_H_IS_INCLUDED

/*!
 *  \file trace.H
 *  \brief Contains the declaration of the TRACE widget.
 *
 *  \ingroup group_FFS
 *  \sa trace.C
 *
 */

#include "geom/image.H"
#include "geom/texturegl.H"
#include "tess/bcurve.H"
#include "gest/draw_widget.H"

enum trace_file_cb_t {
      FILE_LOAD_TRACE_CB = 0,
};

/*!
  TRACE:
  This class is a widget that facilitates the "tracing" and
  matching of a rough 3d model to contours defined in a 2d
  image.
*/

MAKE_PTR_SUBC(TRACE,DrawWidget);
typedef const TRACE    CTRACE;
typedef const TRACEptr CTRACEptr;
class TRACE : public DrawWidget {
 public:
   //******** MANAGERS ********
   TRACE(VIEWptr v);
   virtual ~TRACE();

   static TRACEptr get_instance(VIEWptr v);
   static void clear_instance();

   static TRACEptr get_active_instance() {
      return isa(_active) ? (TRACE*)&*_active : (TRACE*)0;
   }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("TRACE", DrawWidget, CDATA_ITEM*);

   //******** SETTING UP ********

   //! Returns true if the gesture is valid for beginning a
   //! trace session:
   static bool init(CGESTUREptr& g);

   //! After a successful call to init(), this turns on a TRACE
   //! to handle the trace session:
   static bool go(double dur = default_timeout());
   
   //******** DRAW FSA METHODS ********
   virtual int  stroke_cb(CGESTUREptr& gest, DrawState*&);
   virtual int  tap_cb(CGESTUREptr& gest, DrawState*&);
   virtual int  cancel_cb(CGESTUREptr& gest, DrawState*&);

   //******** GEL METHODS ********
   virtual int draw(CVIEWptr& v);   
   virtual bool needs_blend() const { return true; }

   //******** TRACE specific methods ***********

   //! Force the trace widget to go into calibration mode
   void do_calibrate();

	//! True if texture is loaded okay.
	bool valid();

	//! True if calibrating
	bool calibrating();

 protected:
  VIEWptr              _view;
  // hooray for coupling! (sigh)... need this for load dialog.

   //! The 2D Image that is going to be or is being used for "tracing"
   TEXTUREgl* _texture;

   //! Filename of 2D image
   str_ptr      _filename;

   //! Translation in XY obtained from calibration
   //mlib::XYpt         _translation;
   
   //! Rotation obtained from calibration
   //double       _rotation;

   //! Whether we are calibrated
   bool         _calibrated;

   //! Whether we are in calibration mode
   bool         _calib_mode_flag;

   int          _cur_calib_pt_index;

   enum {
      _TOP_LEFT = 0,
      _TOP_RIGHT = 1,
      _BOTTOM_RIGHT = 2,
      _BOTTOM_LEFT = 3
   };

   //! XYpt samples of the four coordinates 
   //! In the order TopLeft, TopRight, BottomRight, BottomLeft
   mlib::XYpt    _samples[4];

   // we don't really use this anymore, since this widget doesn't get init'ed
   //! Frame number of init() call
   uint         _init_stamp;
   
   static TRACEptr      _instance;      // the only one we need

   //******** INTERNAL METHODS ********

   // For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<TRACE, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //******** DrawWidget METHODS ********

   // Clear cached info when deactivated:
   virtual void reset();

  // Open loading dialog
  void load_dia();
  static void file_cbs(void *ptr, int idx, int action, str_ptr path, str_ptr file);

	bool is_valid;
};

/*!
  TraceGuard:
 
  "Guard" that determines if a given GESTURE satifies conditions for
  initiating a trace operation.
*/


class TraceGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return TRACE::init(g); }
};

#endif // TRACE_H_IS_INCLUDED

/* end of file trace.H */
