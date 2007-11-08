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
#ifndef _GESTURE_CELL_H_
#define _GESTURE_CELL_H_

#include <vector>
#include "gest/gesture.H"
#include "mlib/points.H"

class GestureCell{
public:
  GestureCell() : _global_scale(1.0){}
   virtual ~GestureCell() {}

  // accessors
  virtual bool valid() = 0;
  virtual void set_global_scale(double s) { _global_scale = s; } 
  virtual double scale() const =0;
  virtual int nb_strokes() = 0;
  virtual CGESTUREptr& gesture(int i) const = 0;
  virtual void draw_cell_bound() {};

  // processes
  virtual void clear() = 0;
  virtual void add_stroke(const std::vector< mlib::UVpt >& pts,
			  const std::vector<double>& pressures) = 0;
  virtual double alpha (mlib::CUVpt& pt) const = 0;
  double alpha (mlib::CUVpt& pt, mlib::CPIXEL& pix) const;

protected:
  double _global_scale;
};

#endif // _GESTURE_CELL_H_
