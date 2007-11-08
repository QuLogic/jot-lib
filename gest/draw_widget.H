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
#ifndef DRAW_WIDGET_H_IS_INCLUDED
#define DRAW_WIDGET_H_IS_INCLUDED

/*!
 *  \file draw_widget.H
 *  \brief Contains the definition of the DrawWidget class.
 *
 *  \sa draw_widget.C
 *
 */

#include "geom/geom.H"          // for GEOM baseclass
#include "manip/manip.H"        // for Interactor base class
#include "mesh/lmesh.H"         // for BMESHobs and LMESH
#include "std/stop_watch.H"     // for timing

#include "gest/gest_guards.H"   // GESTURE FSA

MAKE_PTR_SUBC(DrawWidget,GEOM);
typedef const DrawWidget    CDrawWidget;
typedef const DrawWidgetptr CDrawWidgetptr;

/*!
 *  \brief Base class for gesture recognizing widgets that can display
 *  interactive controls.
 *
 *  They can draw. They are interactive in the sense of GEOMs
 *  (i.e., can process mouse/tablet Events). They can also act
 *  as an FSA that processes GESTUREs as high-level events. Only
 *  one DrawWidget is activated at any time.
 *
 *  Uses a timer to fade away after a specified period of
 *  inactivity.
 *
 *  Derives from GEOM to be interactive, and for draw() and
 *  intersect().
 *
 *  Derives from DISPobs to observe when it is displayed or
 *  undisplayed (to deactivate in the latter case).
 *
 *  Derives from CAMobs to observe when the camera is changing
 *  so it can reset its timer (and not fade away early).
 *
 */
class DrawWidget : public GEOM,
                   public DISPobs,
                   public CAMobs, 
                   public BMESHobs,
                   public Interactor<DrawWidget,Event,State> {
   public:
   
      //******** RUN-TIME TYPE ID ********
   
      DEFINE_RTTI_METHODS3("DrawWidget", DrawWidget*, GEOM, CDATA_ITEM*);
   
      //******** STATICS ********
   
      //! \brief Anyone can get their hands on the current active widget.
      static DrawWidgetptr get_active_instance() { return _active; }
   
      //******** ACTIVATION *******
   
      //! \brief Toggles on or off.
      virtual void toggle_active();
   
      bool is_active()     const   { return _active == this; }
      void activate();
      void deactivate();
   
      //! \brief Default duration in seconds for widgets to last before
      //! fading away.
      static double default_timeout() { return 1e6; }
   
      void set_timeout  (double dur)                   { _timer.reset(dur); }
      void reset_timeout(double dur=default_timeout()) { set_timeout(dur); }
   
      //******** VIRTUAL METHODS ********
   
      // If derived classes operate on a particular mesh, they should
      // return a pointer to the mesh here. This is used in the virtual
      // method DISPobs::notify() to check whether the mesh has been
      // undisplayed (and if so, deactivate the widget).
      virtual BMESHptr bmesh() const { return 0; }
      virtual LMESH* lmesh()   const { return LMESH::upcast(bmesh()); }
   
      //******** MODE NAME ********
   
      // displayed in jot window
      virtual str_ptr      mode_name()     const { return NULL_STR; }
      bool                 has_mode_name() const { return mode_name() != NULL_STR; }
   
      //******** GESTURE PROCESSING ********
   
      bool handle_gesture(CGESTUREptr& gest) { return _fsa.handle_event(gest); }
   
      //******** CONVENIENCE ********
   
      // DrawWidget::null is used in ray_geom() to get a DrawWidget
      // from a RAYhit after doing intersection, e.g.:
      //   DrawWidgetptr widget = ray_geom(ray, DrawWidget::null);
      static DrawWidget *null; 
   
      //******** GEL METHODS ********
   
      virtual bool needs_blend() const { return alpha() < 1; }
   
      //******** DISPobs METHODS ********
   
      // Nofification that this widget was displayed or undisplayed:
      virtual void notify(CGELptr &g, int);
   
      //******** CAMobs Method *************
   
      // Notification that the camera just changed:
      virtual void notify(CCAMdataptr &data);

   protected:
   
      DrawFSA      _fsa;           // FSA for processing GESTUREs
      DrawState    _draw_start;    // the start state of the FSA
      egg_timer    _timer;         // used for timing out (to be undrawn)
   
      static DrawWidgetptr _active;  // the lone active one
   
      //******** MANAGERS ********
   
      // 'dur' parameter tells how long until the fadeout:
      DrawWidget(double dur=default_timeout());
   
      virtual ~DrawWidget();
   
      //******** PROTECTED METHODS ********
   
      // Shortly before the timer expires, start fading away.
      // this computes the alpha to use for fading.
      double alpha() const {
         const double FADE_TIME = 0.5; // fade for half a second
         return min(_timer.remaining() / FADE_TIME, 1.0);
      }
   
      // Called when deactivated, for subclasses that want to do
      // something at that time:
      virtual void reset() {}
};

#endif // DRAW_WIDGET_H_IS_INCLUDED

/* end of file draw_widget.H */
