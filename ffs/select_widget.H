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
 * select_widget.H
 *****************************************************************/
#ifndef SELECT_WIDGET_H_IS_INCLUDED
#define SELECT_WIDGET_H_IS_INCLUDED

/*!
 *  \file select_widget.H
 *  \brief Contains the declaration of the SELECT_WIDGET widget.
 *
 *  \ingroup group_FFS
 *  \sa select_widget.C
 *
 */

#include "gest/draw_widget.H"
#include "gtex/line_drawing.H"

/*****************************************************************
 * SELECT_WIDGET:
 *****************************************************************/
MAKE_PTR_SUBC(SELECT_WIDGET,DrawWidget);
typedef const SELECT_WIDGET    CSELECT_WIDGET;
typedef const SELECT_WIDGETptr CSELECT_WIDGETptr;

#define MAX_PATTERN_SIZE 32  //maximum pattern length

//! \brief "Widget" that handles selection.
class SELECT_WIDGET : public DrawWidget {
 public:

   //! indicates which type of component is currently being selected
   enum sel_mode_t {
      SEL_NONE,
      SEL_FACE,
      SEL_EDGE,
	  SLASH_SEL,
      NUM_SEL_MODES
   };

   //******** MANAGERS ********

   // no public constructor
   static SELECT_WIDGETptr get_instance();

   static SELECT_WIDGETptr get_active_instance() {
      return isa(_active) ? (SELECT_WIDGET*)&*_active : (SELECT_WIDGET*)0;
   }

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SELECT_WIDGET", SELECT_WIDGET*, DrawWidget, CDATA_ITEM*);

   //******** SETTING UP ********

   //! If the gesture is valid for activating the widget,
   //! activates it and returns true.
   static bool init(CGESTUREptr& g);

   //******** DRAW FSA METHODS ********

   //! Generic stroke.
   virtual int  stroke_cb     (CGESTUREptr& gest, DrawState*&);

   //! Tap callback (usually cancel):
   virtual int  tap_cb        (CGESTUREptr& gest, DrawState*&);

   //! Turn off the widget:
   virtual int  cancel_cb     (CGESTUREptr& gest, DrawState*&);

   //! Small circle callback: select a face or edge
   virtual int  sm_circle_cb(CGESTUREptr& gest, DrawState*&);

  //! slash select, eventually it will select a row at a time
   virtual int slash_cb(CGESTUREptr& gest, DrawState*&);

   //******** MODE NAME ********

   //! displayed in jot window
   virtual str_ptr      mode_name()     const { return "selection"; }

   //******** DISPobs METHODS ********

   //! Used to monitor if a gel (possibly a mesh with selected
   //! components) is undisplayed:
   virtual void notify(CGELptr &g, int);

   //******** CAMobs Method *************

   //! Notification that the camera just changed:
   //! Need this line to prevent warnings due to notify(CGELptr&, int):
   virtual void notify(CCAMdataptr& data) { DrawWidget::notify(data); }

   //******** GEL METHODS ********

   virtual int draw(CVIEWptr& v);

 protected:

   //! indicates the component type (faces or edges) we're
   //! currently selecting:
   sel_mode_t               _mode;      // phased out?

   // slash select variables
   Bface_list			select_list;    //!< preliminary selection list
   int					end_face; //!< zero means all the way around
   int					pattern;
   bool					pattern_array[MAX_PATTERN_SIZE];

   static SELECT_WIDGETptr  _instance;
   static void clean_on_exit();

   //******** MANAGERS ********

   SELECT_WIDGET();

   virtual ~SELECT_WIDGET() {}

   //! For defining callbacks to use with GESTUREs:
   typedef CallMeth_t<SELECT_WIDGET, GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) {
      return new draw_cb_t(this, m);
   }

   //******** DrawWidget METHODS ********

   //! Clear cached info when deactivated:
   virtual void reset();

   //******** UTILITY METHODS ********

   bool init_select_face(CGESTUREptr&);
   bool init_select_edge(CGESTUREptr&);

   //! Select an edit-level face if one is found at screen
   //! location 'pix' corresponding to a barycentric location
   //! outside of the given margin of the edges of the face.
   //! (I.e., pix is sufficiently near the center of the face.)
   bool try_select_face(mlib::CPIXEL& pix, double margin);

   //! Same as try_select_face(), but deselects.
   bool try_deselect_face(mlib::CPIXEL& pix, double margin);

   bool select_faces(mlib::CPIXEL_list& pts);
   bool select_edges(mlib::CPIXEL_list& pts);

   bool try_select_edge(mlib::CPIXEL& pix); 
   bool try_deselect_edge(mlib::CPIXEL& pix); 
};

#endif // SELECT_WIDGET_H_IS_INCLUDED

// end of file select_widget.H
