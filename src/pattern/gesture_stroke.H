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
#ifndef _GESTURE_STROKE_H_
#define _GESTURE_STROKE_H_

#include "pattern/stroke.H"
#include "pattern/gesture_cell.H"
#include "stroke/gesture_stroke_drawer.H"
#include "gest/gesture.H"
#include "disp/bbox.H"
#include "mlib/points.H"

class GestureStroke : public Stroke {
public:
  // constructor / destructor
  GestureStroke(CBBOXpix& bbox=BBOXpix()) : _gesture(), _drawer(0), _bbox(bbox){}
  GestureStroke(CGESTUREptr& gest, GestureStrokeDrawer* drawer) : _gesture(gest), _drawer(drawer) {}
  virtual ~GestureStroke(){}

  // accessors
  const CGESTUREptr&         gesture() const { return _gesture; }
  GestureStrokeDrawer* drawer()      const { return _drawer; }
  void                       set_bbox(CBBOXpix& bbox) {_bbox = bbox;} 

  // derived methods
  virtual void copy(GestureCell* target_cell, mlib::CUVpt& offset, bool stretch) const;
  virtual void synthesize(GestureCell* target_cell, double target_pressure, double ref_angle, mlib::CUVpt& ref_pos, 
			  mlib::CUVvec& target_scale, double target_angle, mlib::CUVpt& target_pos) const;
  virtual void add_stroke_pt(mlib::CPIXEL& pt, double pressure);

private:
  GESTUREptr _gesture;
  GestureStrokeDrawer* _drawer;
  BBOXpix _bbox;
};


#endif // _GESTURE_STROKE_H_
