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
#ifndef EXTENDER_H_IS_INCLUDED
#define EXTENDER_H_IS_INCLUDED

/*!
 *  \file extender.H
 *  \brief Contains the declaration of the EXTENDER widget.
 *
 *  \ingroup group_FFS
 *  \sa extender.C
 *
 */

#include "mesh/ledge_strip.H"

#include "gest/draw_widget.H"

/*****************************************************************
 * EXTENDER:
 *****************************************************************/
MAKE_PTR_SUBC(EXTENDER,DrawWidget);
typedef const EXTENDER    CEXTENDER;
typedef const EXTENDERptr CEXTENDERptr;

//! \brief "Widget" that handles attaching new branches to a primitive.
class EXTENDER : public DrawWidget {
 public:

   //******** MANAGERS ********

   // no public constructor
   static EXTENDERptr get_instance();

   static EXTENDERptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("EXTENDER", EXTENDER*, DrawWidget, CDATA_ITEM *);

   //******** SETTING UP ********

   //! \brief If the gesture is valid for activating the widget,
   //! activates it and returns true.
   static bool init(CGESTUREptr& g);

   //******** DRAW FSA METHODS ********

   virtual int  cancel_cb (CGESTUREptr& gest, DrawState*&);
   virtual int  ellipse_cb(CGESTUREptr& gest, DrawState*&);
   virtual int  circle_cb (CGESTUREptr& gest, DrawState*&);
   virtual int  line_cb   (CGESTUREptr& gest, DrawState*&);
   virtual int  stroke_cb (CGESTUREptr& gest, DrawState*&);

   //******** DrawWidget METHODS ********

   virtual void toggle_active() {
      DrawWidget::toggle_active();
      if (!is_active())
         reset();
   }

   //******** MODE NAME ********

   //! displayed in jot window
   virtual str_ptr      mode_name()     const { return "tube edit"; }

   //******** GEL METHODS ********

   virtual int draw(CVIEWptr &v);

 protected:
   Bface_list       _base1;         //!< 1st selected base
   Bface_list       _base2;         //!< 2nd selected base
   LedgeStrip   _strip;         //!< edge strip around base(s)

   mlib::Wtransf      _plane_xf;      //!< describes draw plane 

   static EXTENDERptr _instance;

   static void clean_on_exit();

   //******** INTERNAL METHODS ********

   EXTENDER();
   virtual ~EXTENDER() {}

   void compute_plane();
   void build_strip();

   void reset();
   bool add(Bface_list faces, double dur=default_timeout());

   bool sweep_ball(CGESTUREptr& g);

   //! \brief for defining callbacks to use with GESTUREs:
   typedef CallMeth_t<EXTENDER, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }
};

#endif // EXTENDER_H_IS_INCLUDED

/* end of file extender.H */
