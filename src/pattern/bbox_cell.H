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
#ifndef _BBOX_CELL_H_
#define _BBOX_CELL_H_

#include <vector>
#include "pattern/gesture_cell.H"
#include "pattern/gesture_stroke.H"
#include "disp/bbox.H"
#include "mlib/points.H"

class BBoxCell : public GestureCell {
public:
  // constructor / destructor
  BBoxCell() {}
  BBoxCell(mlib::CPIXEL& p, mlib::CPIXEL& q);
  virtual ~BBoxCell(){}
  
  virtual void draw_cell_bound();

  // derived accessors
  virtual bool valid () { return _bbox.valid(); } 
  virtual double scale() const { return _scale * _global_scale; }
  virtual int nb_strokes()                  { return _strokes.size(); }
  virtual CGESTUREptr& gesture(int i) const { return _strokes[i].gesture(); }

  // derived processe
  virtual void clear() { _strokes.clear(); }
  virtual void add_stroke(const std::vector< mlib::UVpt >& pts,
			  const std::vector<double>& pressures);
  virtual double alpha (mlib::CUVpt& pt) const;
private:
  BBOXpix _bbox;
  std::vector<GestureStroke> _strokes;
  double _scale;
  mlib::VEXEL _offset;

  static const double FADE_WIDTH;

};

#endif  // _BBOX_CELL_H_
