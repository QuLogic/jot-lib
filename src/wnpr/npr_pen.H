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
#ifndef _NPR_PEN_H_IS_INCLUDED_
#define _NPR_PEN_H_IS_INCLUDED_

////////////////////////////////////////////
// NPRPen
////////////////////////////////////////////


#include "stroke/gesture_stroke_drawer.H"
#include "gest/pen.H"

class NPRPenUI;
class NPRTexture;
class FooGestureDrawer;

/*****************************************************************
 * NPRPen
 *****************************************************************/
class NPRPen : public Pen {
 public:

 protected:

   /******** MEMBERS VARS ********/

   NPRTexture*             _curr_npr_tex;		

   FooGestureDrawer*		   _gesture_drawer;			
   GestureDrawer*				_prev_gesture_drawer;	

   NPRPenUI*               _ui;

   /******** MEMBERS METHODS ********/

	void	      select_current_texture(NPRTexture *nt);
	void	      deselect_current_texture();

   // Convenience for setting up Gesture handling:
   typedef		CallMeth_t<NPRPen,GESTUREptr> draw_cb_t;
   draw_cb_t*	drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

 public:

   //******** CONSTRUCTOR/DECONSTRUCTOR ********

  NPRPen(CGEST_INTptr &gest_int,
	 CEvent &d, CEvent &m, CEvent &u,
	 CEvent &shift_down, CEvent &shift_up,
	 CEvent &ctrl_down, CEvent &ctrl_up);

   virtual ~NPRPen();

   //******** UI STUFF ********

   NPRTexture*  curr_npr_tex() { return _curr_npr_tex; }
   
   //******** GESTURE METHODS ********

   virtual int    tap_cb      (CGESTUREptr& gest, DrawState*&);
   virtual int    scribble_cb (CGESTUREptr& gest, DrawState*&);
   virtual int    lasso_cb    (CGESTUREptr& gest, DrawState*&);
   virtual int    line_cb     (CGESTUREptr& gest, DrawState*&);
   virtual int    stroke_cb   (CGESTUREptr& gest, DrawState*&);
   virtual int    slash_cb    (CGESTUREptr& gest, DrawState*&);

   // ******** PEN ACTIVATION ********

   virtual void   activate(State *);
	virtual bool   deactivate(State *);

};

#endif // _NPR_PEN_H_IS_INCLUDED_

/* end of file npr_pen.H */

