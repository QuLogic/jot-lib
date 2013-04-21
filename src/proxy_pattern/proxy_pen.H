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
/***************************************************************************
    proxy_pen.H
***************************************************************************/
#ifndef _PROXY_PEN_H_IS_INCLUDED_
#define _PROXY_PEN_H_IS_INCLUDED_

#include "stroke/gesture_stroke_drawer.H"
#include "gest/pen.H"
#include "proxy_texture.H"

class ProxyPenUI;
class ProxySurface;

class ProxyPen : public Pen { 
 protected:  
   /******** MEMBERS VARS ********/  
   GestureDrawer*          _blank_gesture_drawer; // Non stylized gesture
   ProxyPenUI*             _ui;   
    
   // Convenience for setting up Gesture handling:
   typedef     CallMeth_t<ProxyPen,GESTUREptr> draw_cb_t;
   draw_cb_t*  drawCB(draw_cb_t::_method m) { return new draw_cb_t(this,m); }

   bool add_direcction_stroke(Patch* p, NDCpt_list& pts);

 public:
   //******** CONSTRUCTOR/DECONSTRUCTOR ********  
   ProxyPen(CGEST_INTptr &gest_int,
            CEvent &d, CEvent &m, CEvent &u,
            CEvent &shift_down, CEvent &shift_up,
            CEvent &ctrl_down, CEvent &ctrl_up);

   virtual ~ProxyPen();
  
   //******** GESTURE METHODS ********
   virtual int    tap_cb      (CGESTUREptr& gest, DrawState*&);
   virtual int    stroke_cb   (CGESTUREptr& gest, DrawState*&);
   // ******** PEN ACTIVATION ********
   virtual void   activate(State *);
   virtual bool   deactivate(State *);  
};

#endif   // _PATTERN_PEN_H_IS_INCLUDED_

/* end of file proxy_pen.H */
