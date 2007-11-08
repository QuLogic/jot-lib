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
#ifndef CIRCLE_WIDGET_H_IS_INCLUDED
#define CIRCLE_WIDGET_H_IS_INCLUDED

/*!
 *  \file circle_widget.H
 *  \brief Contains the declaration of the CIRCLE_WIDGET widget.
 *
 *  \ingroup group_FFS
 *  \sa circle_widget.C
 *
 */

#include "gest/draw_widget.H"

MAKE_PTR_SUBC(CIRCLE_WIDGET,DrawWidget);
typedef const CIRCLE_WIDGET    CCIRCLE_WIDGET;
typedef const CIRCLE_WIDGETptr CCIRCLE_WIDGETptr;

/*****************************************************************
 * CIRCLE_WIDGET:
 *****************************************************************/
 
//! \brief "Widget" that handles selection.
class CIRCLE_WIDGET : public DrawWidget {
 public:

   //******** MANAGERS ********

   // no public constructor
   static CIRCLE_WIDGETptr get_instance();

   static CIRCLE_WIDGETptr get_active_instance() {
      return isa(_active) ? (CIRCLE_WIDGET*)&*_active : (CIRCLE_WIDGET*)0;
   }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("CIRCLE_WIDGET", CIRCLE_WIDGET*, DrawWidget, CDATA_ITEM*);

   //******** MODE NAME ********

   //! displayed in jot window
   virtual str_ptr      mode_name()     const { return "circle edit"; }

   //******** SETTING UP ********

   //! Returns true if the gesture is valid for beginning a
   //! select session:
   static bool init(CGESTUREptr& g);

 protected:
   //! After a successful call to init(), this turns on a CIRCLE_WIDGET
   //! to handle the select session:
   static bool go(double dur = default_timeout());
 public:

   //******** DRAW FSA METHODS ********

   //! Generic stroke.
   virtual int  stroke_cb     (CGESTUREptr& gest, DrawState*&);

   //! Tap callback (usually cancel):
   virtual int  tap_cb        (CGESTUREptr& gest, DrawState*&);

   //! Turn off the widget:
   virtual int  cancel_cb     (CGESTUREptr& gest, DrawState*&);

   //******** GEL METHODS ********

   virtual int draw(CVIEWptr& v);
   virtual bool create_literal(GESTUREptr gest);
   virtual bool finish_literal(void);


 protected:

   uint                     _init_stamp;  //!< frame number of init() call

   mlib::Wplane             _plane;
   double                   _radius;

   mlib::Wpt                _center;
   mlib::Wpt_list           _preview;
   mlib::Wpt_list           _literal_shape;

   int                      _disk_res;

   Panel                   *_circle;
   MULTI_CMDptr             _cmd;

   Bcurve                  *_border;

   bool                     _suggest_active;

   static CIRCLE_WIDGETptr  _instance;    // the only one we need

   static const int         _PICK_RAD;

   static void clean_on_exit();

   //******** MANAGERS ********
   CIRCLE_WIDGET();
   virtual ~CIRCLE_WIDGET();

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<CIRCLE_WIDGET, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();

   //******** UTILITY METHODS ********

   void make_preview( void );

};

/*****************************************************************/
//! "Guard" that determines if a given GESTURE satifies
//!  conditions for initiating a circle widget operation.
/*****************************************************************/
class CircleWidgetGuard : public DrawGuard {
 public:
   virtual int exec(CGESTUREptr& g) { return CIRCLE_WIDGET::init(g); }
};

#endif // CIRCLE_WIDGET_H_IS_INCLUDED

/* end of file circle_widget.H */
