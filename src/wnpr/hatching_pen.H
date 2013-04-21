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
#ifndef _HATCHING_PEN_H_IS_INCLUDED_
#define _HATCHING_PEN_H_IS_INCLUDED_

////////////////////////////////////////////
// HatchingPen
////////////////////////////////////////////
//
// -Pen class which manages hatching groups
// -Talks to GLUI widget HatchingPenUI
// -Sets patches to HatchStokeTexture (soon to change name)
// -Talks to HatchingCollection in the texture
//
////////////////////////////////////////////

#include "npr/hatching_group.H"
#include "stroke/gesture_stroke_drawer.H"
#include "gest/pen.H"

class NPRTexture;
class HatchingPenUI;

/*****************************************************************
 * HatchingPen
 *****************************************************************/
class HatchingPen : public Pen {
 public:

 protected:

   /******** MEMBERS VARS ********/

	int							_curr_create_type;	
	int							_curr_curve_mode;	
   HatchingGroup*				_curr_hatching_group;		
   NPRTexture*             _curr_npr_texture;

	HatchingGroupParams		_params;

   GestureStrokeDrawer*		_gesture_drawer;			
   GestureDrawer*				_prev_gesture_drawer;	

   HatchingPenUI*          _ui;

   /******** MEMBERS METHODS ********/

	// passes stroke to current/new hatchgroup
   int	generate_stroke(CGESTUREptr& gest);

	// ui related internal methods
	void	destroy_current();
	void	select(HatchingGroup *hg, NPRTexture *nt);
	void	select_tex(NPRTexture *nt);
	void	deselect_current();
	void	get_parameters();

	void	update_gesture_proto();

   // convenience for setting up Gesture handling:
   typedef		CallMeth_t<HatchingPen,GESTUREptr> draw_cb_t;
   draw_cb_t*	drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

 public:

   //******** CONSTRUCTOR/DECONSTRUCTOR ********

	 HatchingPen(CGEST_INTptr &gest_int,
            CEvent &d, CEvent &m, CEvent &u,
	         CEvent &shift_down, CEvent &shift_up,
	         CEvent &ctrl_down, CEvent &ctrl_up);

   virtual ~HatchingPen();

   //******** UI STUFF ********

	void  delete_current();
   void  undo_last();

   void  next_hatch_mode();
	void  next_curve_mode();
   void  next_style_mode();

   void	apply_parameters();
	
   int	hatch_mode() const   { return _curr_create_type; }
   int	curve_mode() const   { return _curr_curve_mode; }
   
   //******** GESTURE METHODS ********

   virtual int  tap_cb			(CGESTUREptr& gest, DrawState*&);
   virtual int  scribble_cb	(CGESTUREptr& gest, DrawState*&);
   virtual int  lasso_cb		(CGESTUREptr& gest, DrawState*&);
   virtual int  line_cb			(CGESTUREptr& gest, DrawState*&);
   virtual int  stroke_cb		(CGESTUREptr& gest, DrawState*&);
   virtual int  slash_cb		(CGESTUREptr& gest, DrawState*&);

   //******** GestObs METHODS ********
   
	virtual int handle_event(CEvent&, State* &);

   //******** STROKE PARAM MANAGEMENT ********

	HatchingGroupParams & params() {return _params; }

   // ******** PEN MTHODS ********

   virtual void key(CEvent &e);
   virtual void activate(State *);
	virtual bool deactivate(State *);

};

#endif // _HATCHING_PEN_H_IS_INCLUDED_

/* end of file hatch_pen.H */

