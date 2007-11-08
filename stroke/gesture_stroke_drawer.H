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
/**********************************************************************
 * gesture_stroke_drawer.H
 *
 **********************************************************************/
#ifndef GESTURE_STROKE_DRAWER_H_HAS_BEEN_INCLUDED
#define GESTURE_STROKE_DRAWER_H_HAS_BEEN_INCLUDED

#include "gest/gesture.H"
#include "stroke/base_stroke.H"


/*****************************************************************
 * GestureStokeDrawer
 *
 *      Draws a gesure using a base stroke.
 *
 *****************************************************************/
class GestureStrokeDrawer : public GestureDrawer {
 public:
   GestureStrokeDrawer();
   virtual ~GestureStrokeDrawer();
   virtual GestureDrawer* dup() const { return new GestureStrokeDrawer; }
   virtual int draw(const GESTURE*, CVIEWptr& view);

   BaseStroke*   base_stroke_proto() { return _base_stroke_proto; }

   void          set_base_stroke_proto(BaseStroke* s) { 
      if(_base_stroke_proto && _base_stroke_proto != s)
         delete _base_stroke_proto;
      _base_stroke_proto = s; 
   }

 protected:
   BaseStroke*             _base_stroke_proto;
};

#endif  // GESTURE_STROKE_DRAWER_H_HAS_BEEN_INCLUDED

/* end of file gesture_stroke_drawer.H */
