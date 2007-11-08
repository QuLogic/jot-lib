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
 * crease_widget.H
 *****************************************************************/
#ifndef CREASE_WIDGET_H_IS_INCLUDED
#define CREASE_WIDGET_H_IS_INCLUDED

/*!
 *  \file crease_widget.H
 *  \brief Contains the declaration of the CREASE_WIDGET widget.
 *
 *  \ingroup group_FFS
 *  \sa crease_widget.C
 *
 */
 
#include "mesh/lmesh.H"
#include "gest/draw_widget.H"

/*****************************************************************
 * CREASE_WIDGET:
 *****************************************************************/
MAKE_PTR_SUBC(CREASE_WIDGET,DrawWidget);
typedef const CREASE_WIDGET    CCREASE_WIDGET;
typedef const CREASE_WIDGETptr CCREASE_WIDGETptr;

//! \brief Widget for editing crease edges on a mesh.
class CREASE_WIDGET : public DrawWidget {
 public:

   //******** MANAGERS ********

   // no public constructor
   static CREASE_WIDGETptr get_instance();

   static CREASE_WIDGETptr get_active_instance() { return upcast(_active); }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("CREASE_WIDGET", CREASE_WIDGET*, DrawWidget, CDATA_ITEM*);

   //******** SETTING UP ********

   //! \brief If the gesture is valid for activating the widget,
   //! activates it and returns true.
   static bool init(CGESTUREptr& g);

   //******** DrawWidget virtual methods ********

   virtual BMESHptr bmesh() const { return _mesh; }
   
   //******** GEL METHODS ******** 

   virtual int draw(CVIEWptr& v);

 protected:

   //******** MEMBER DATA ********

   BMESHptr     _mesh;
   EdgeStrip*   _strip;

   static CREASE_WIDGETptr _instance;

   static void clean_on_exit();

   //******** MANAGERS ********

   CREASE_WIDGET();
   virtual ~CREASE_WIDGET() {}

   bool init(Bedge* e);
   bool add(Bedge* e);

   SimplexFilter& get_filter(Bedge* e);

   //******** MODE NAME ********

   //! displayed in jot window
   virtual str_ptr      mode_name()     const { return "edit creases"; }

   //******** DRAW FSA METHODS ********

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<CREASE_WIDGET, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //! Turn off the widget:
   virtual int  cancel_cb(CGESTUREptr& gest, DrawState*&);

   //! Tap callback (usually cancel):
   virtual int  tap_cb(CGESTUREptr& gest, DrawState*&);

   //! sharpen (increment crease value)
   virtual int  sharpen_cb(CGESTUREptr&, DrawState*&);

   //! smooth (decrement crease value)
   virtual int  smooth_cb(CGESTUREptr&, DrawState*&);

   //! general stroke
   virtual int  stroke_cb(CGESTUREptr&, DrawState*&);


   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();
};

#endif // CREASE_WIDGET_H_IS_INCLUDED

// end of file crease_widget.H
