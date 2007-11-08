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
 * gesture_rect_drawer.H
 *
 **********************************************************************/
#ifndef _GESTURE_RECT_DRAWER_H_
#define _GESTURE_RECT_DRAWER_H_

#include "gest/gesture.H"
#include "mlib/points.H"


/*****************************************************************
 * GestureRectDrawer
 *
 *      Draws an oriented rectangle using two gestures
 *
 *****************************************************************/
class GestureRectDrawer : public GestureDrawer {
 public:
  GestureRectDrawer() {}
  virtual ~GestureRectDrawer() {}
  virtual GestureDrawer* dup() const { return new GestureRectDrawer; }
  virtual int draw(const GESTURE*, CVIEWptr& view);
  void set_axis(CGESTUREptr& axis) { _axis_gesture = axis; }

private:
  void draw_skeleton(const GESTURE*, CVIEWptr& view);
  void draw_rect(const GESTURE*, CVIEWptr& view);
  void compute_rect_pt(const mlib::PIXEL_list& pts, int k, double thickness, mlib::PIXEL& pt);

private:
  GESTUREptr _axis_gesture;
};

#endif  // _GESTURE_RECT_DRAWER_H_

