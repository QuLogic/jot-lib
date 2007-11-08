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
 * gesture_carrier_drawer.H
 *
 **********************************************************************/
#ifndef _GESTURE_CARRIER_DRAWER_H_
#define _GESTURE_CARRIER_DRAWER_H_

#include "gest/gesture.H"
#include "mlib/points.H"


/*****************************************************************
 * GestureCarrierDrawer
 *
 *      Draws a cell using two carrier gestures
 *
 *****************************************************************/
class GestureCarrierDrawer : public GestureDrawer {
 public:
  GestureCarrierDrawer() {}
  virtual ~GestureCarrierDrawer() {}
  virtual GestureDrawer* dup() const { return new GestureCarrierDrawer; }
  virtual int draw(const GESTURE*, CVIEWptr& view);
  void set_first_carrier(CGESTUREptr& carrier) { _carrier_gesture = carrier; }

private:
  void draw_first_carrier(const GESTURE*, CVIEWptr& view);
  void draw_carriers(const GESTURE*, CVIEWptr& view);

private:
  GESTUREptr _carrier_gesture;
};

#endif  // _GESTURE_CARRIER_DRAWER_H_

