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
 * gesture_path_drawer.H
 *
 **********************************************************************/
#ifndef _GESTURE_PATH_DRAWER_H_
#define _GESTURE_PATH_DRAWER_H_

#include "gest/gesture.H"
#include "mlib/points.H"


/*****************************************************************
 * GesturePathDrawer
 *
 *      Draws a thick path using two gestures
 *
 *****************************************************************/
class GesturePathDrawer : public GestureDrawer {
 public:
  GesturePathDrawer() {}
  virtual ~GesturePathDrawer() {}
  virtual GestureDrawer* dup() const { return new GesturePathDrawer; }
  virtual int draw(const GESTURE*, CVIEWptr& view);
  void set_path(CGESTUREptr& path) { _path_gesture = path; }

private:
  void draw_skeleton(const GESTURE*, CVIEWptr& view);
  void draw_path(const GESTURE*, CVIEWptr& view);
  void compute_path_pt(const mlib::PIXEL_list& pts, int k, double thickness, mlib::PIXEL& pt);

private:
  GESTUREptr _path_gesture;
};

#endif  // _GESTURE_PATH_DRAWER_H_

