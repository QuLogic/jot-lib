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
#ifndef _RECT_CELL_H_
#define _RECT_CELL_H_

#include <vector>
#include "gest/gesture.H"
#include "pattern/gesture_cell.H"
#include "pattern/gesture_stroke.H"
#include "mlib/points.H"

class RectCell : public GestureCell{
public:
  // constructor / destructor
  RectCell() : _thickness(0.0) {}
  RectCell(const GESTUREptr& rect, double thickness) 
    : _rect(rect), _thickness(thickness), 
      _scale(_rect->endpoint_dist()), _offset(0.0, (_thickness-_scale)*0.5) {}
  virtual ~RectCell(){}
  
  // derived accessors
  virtual bool valid () { return (_rect && _rect->pts().num()>=2 && _thickness>0.0); } 
  virtual double scale() const { return _scale*_global_scale; }
  virtual int nb_strokes()                  { return _strokes.size(); }
  virtual CGESTUREptr& gesture(int i) const { return _strokes[i].gesture(); }

  // derived methods
  virtual void clear() { _strokes.clear(); }
  virtual void add_stroke(const std::vector< mlib::UVpt >& pts,
			  const std::vector<double>& pressures);
  virtual double alpha (mlib::CUVpt& pt) const;

private:
  GESTUREptr _rect;
  std::vector<double> _rect_coords;
  double _thickness;
  std::vector<GestureStroke> _strokes;
  double _scale;
  mlib::VEXEL _offset;

  static const double FADE_WIDTH;

};

#endif  // _RECT_CELL_H_
