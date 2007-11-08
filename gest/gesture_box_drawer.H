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
 * gesture_box_drawer.H
 *
 **********************************************************************/
#ifndef _GESTURE_BOX_DRAWER_H_
#define _GESTURE_BOX_DRAWER_H_

#include "gest/gesture.H"


/*****************************************************************
 * GestureBoxDrawer
 *
 *      Draws a box using a gesture 
 *
 *****************************************************************/
class GestureBoxDrawer : public GestureDrawer {
 public:
  GestureBoxDrawer() {}
  virtual ~GestureBoxDrawer() {}
   virtual GestureDrawer* dup() const { return new GestureBoxDrawer; }
   virtual int draw(const GESTURE*, CVIEWptr& view);

};

#endif  // _GESTURE_STROKE_DRAWER_H_

