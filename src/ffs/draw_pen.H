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
#ifndef _DRAW_PEN_H_IS_INCLUDED_
#define _DRAW_PEN_H_IS_INCLUDED_

/*!
 *  \file draw_pen.H
 *  \brief Contains the declaration of the DrawPen Pen.
 *
 *  \ingroup group_FFS
 *  \sa draw_pen.C
 *
 */

#include "tess/bsurface.H"

#include "gest/pen.H"
#include "ffs/cursor3d.H"
#include "ffs/floor.H"
#include "ffs/ffs_util.H"

/*****************************************************************
 * DrawPen
 *****************************************************************/
 //! \brief Read in the mouse/tablet input (gesture) and determine 
 //! what action the user intended.
class DrawPen : public Pen, public CAMobs, public FRAMEobs {
 protected:
   enum draw_mode {
      DEFAULT,
      SELECTED,
      TUBE_CREATION
   };
 public:

   //******** MANAGERS ********

   DrawPen(CGEST_INTptr &gest_int, CEvent &d, CEvent &m, CEvent &u);
   virtual ~DrawPen() {}

   //******** STATICS ********

   static DrawPen* instance() { return _instance; }

   //******** GESTURE CALLBACK METHODS ********

   virtual int  garbage_cb      (CGESTUREptr& gest, DrawState*&);
   virtual int  null_cb         (CGESTUREptr& gest, DrawState*&);
   virtual int  dot_cb          (CGESTUREptr& gest, DrawState*&);
   virtual int  scribble_cb     (CGESTUREptr& gest, DrawState*&);
   virtual int  lasso_cb        (CGESTUREptr& gest, DrawState*&);
   virtual int  small_circle_cb (CGESTUREptr& gest, DrawState*&);
   virtual int  circle_cb       (CGESTUREptr& gest, DrawState*&);
   virtual int  ellipse_cb      (CGESTUREptr& gest, DrawState*&);
   virtual int  line_cb         (CGESTUREptr& gest, DrawState*&);
   virtual int  x_cb            (CGESTUREptr& gest, DrawState*&);
   virtual int  tap_cb          (CGESTUREptr& gest, DrawState*&);
   virtual int  slash_cb        (CGESTUREptr& gest, DrawState*&);
   virtual int  slash_tap_cb    (CGESTUREptr& gest, DrawState*&);
   virtual int  stroke_cb       (CGESTUREptr& gest, DrawState*&);

   void tapat(PIXEL x);

   //******** EVENT CALLBACK METHODS ********

   virtual int drag_move_cb(CEvent &, State *&);
   virtual int drag_up_cb  (CEvent &, State *&);
   
   //******** Pen VIRTUAL METHODS ********

   virtual void activate(State*);
   virtual bool deactivate(State*);

   //******** GestObs METHODS ********

   virtual void notify_down();

   //******** CAMobs Method *************

   virtual void notify(CCAMdataptr &data);

   //******** FRAMEobs Method*************

   virtual int  tick();

   //********** PROTECTED *********

 protected:

   //******** MEMBERS ********

   draw_mode            _mode;

   Bpoint*              _drag_pt;  //!< Bpoint that gets dragged
   State                _dragging; //!< state for dragging the point

   //! timeout for tap (to allow sufficient time for tap-slash- tslash)
   egg_timer            _tap_timer;
   bool                 _tap_callback;
   PIXEL                _tap_callback_loc;

   static DrawPen*      _instance; // there's just one

   //! For creating callbacks to use with GESTUREs:
   typedef CallMeth_t<DrawPen,GESTUREptr> draw_cb_t;
   draw_cb_t* drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

   //******** GEOMETRY CREATION ********

   void create_sphere  (Bpoint*, double rad) const;
//    int  create_scribble(GESTUREptr gest);

   int  create_curve   (GESTUREptr gest);
   int  create_rect    (GESTUREptr gest);
   bool select_skel_curve(Bsurface* surf_sel, CGESTUREptr& gest);
   bool handle_extrude   (Bsurface* surf_sel, CGESTUREptr& gest);
   bool panel_op         (Bsurface* surf_sel, CGESTUREptr& gest);
   
   //******** UTILITY METHODS ********

   //! project_to_plane(): Calls gest->pts().project_to_plane(P, ret)
   //! but then fixes up the result a little. Treats straight lines
   //! special, and cleans up closed curves a bit:
   void project_to_plane(
      CGESTUREptr& gest,   //!< gesture to project
      CWplane     &P,      //!< plane to project onto
      Wpt_list    &ret     //!< RETURN: projected points
      );

   void  AddToSelection(Bbase* c);

 public:

   //******** PUBLIC METHODS ACCESSED BY DRAW_INT *************

   //! Reset to DEFAULT mode, Clean up any helpers (e.g Shadowmatcher):
   void ModeReset(MULTI_CMDptr cmd);    
};

#endif // _DRAW_PEN_H_IS_INCLUDED_

// end of file draw_pen.H
