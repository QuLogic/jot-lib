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
 * gesture_cell_drawer.H
 *
 **********************************************************************/
#ifndef _GESTURE_CELL_DRAWER_H_
#define _GESTURE_CELL_DRAWER_H_

#include <vector>
#include "gest/gesture.H"
#include "mlib/points.H"
#include "disp/gel_set.H"
#include "gesture_cell.H"

/*****************************************************************
 * GestureCellDrawer
 * Draws stored collection of Strokes(gestures) as well as cell 
 * that the stokes are in
 *****************************************************************/
MAKE_PTR_SUBC(GestureCellDrawer, GELset);
typedef const GestureCellDrawer CGestureCellDrawer;
typedef const GestureCellDrawerptr CGestureCellDrawerptr;

class GestureCellDrawer : public GELset {
 public:
  GestureCellDrawer() {}
  virtual ~GestureCellDrawer(); 
  virtual int draw(CVIEWptr& view);
  virtual int  draw_final(CVIEWptr & v);
  
  virtual void add(GELset strokes, GestureCell* cell);
  virtual void pop();    
  virtual void clear(); 
  virtual int num() { return _strokes.size(); };
  int size() { return _strokes.size(); }; 
  GestureCell* get_cell(int i) const { return _cells[i]; }
 private:
  std::vector<GELset>         _strokes;
  std::vector<GestureCell*>   _cells; 

};

#endif  // _GESTURE_CELL_DRAWER_H_
