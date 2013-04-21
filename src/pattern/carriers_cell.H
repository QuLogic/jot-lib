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
#ifndef _CARRIERS_CELL_H_
#define _CARRIERS_CELL_H_

#include <vector>
#include "gest/gesture.H"
#include "pattern/gesture_cell.H"
#include "pattern/gesture_stroke.H"
#include "mlib/points.H"

class CarriersCell : public GestureCell{
public:
  // constructor / destructor
  CarriersCell() {}
  CarriersCell(CGESTUREptr& carrier1, CGESTUREptr& carrier2) 
    : _carrier1(carrier1), _carrier2(carrier2){
    compute_carriers_coords(true);
    compute_carriers_coords(false);
    _scale = min(carrier1->length(), carrier2->length());
  }
  virtual ~CarriersCell(){}
  
  // derived accessors
  virtual bool valid () { return (_carrier1 && _carrier1->pts().num()>=2 && 
				  _carrier2 && _carrier2->pts().num()>=2 ); } 
  virtual double scale() const { return _scale*_global_scale;}
  virtual int nb_strokes()                  { return _strokes.size(); }
  virtual CGESTUREptr& gesture(int i) const { return _strokes[i].gesture(); }

  // derived methods
  virtual void clear() { _strokes.clear(); }
  virtual void add_stroke(const std::vector< mlib::UVpt >& pts,
			  const std::vector<double>& pressures);
  virtual double alpha (mlib::CUVpt& pt) const;
private:
  void compute_carriers_coords(bool compute_coords1);
  double closest_pixels (double, mlib::PIXEL& p, mlib::PIXEL& q, 
			 const GESTUREptr carrier, const std::vector<double>& carrier_coords) const;
  

private:
  GESTUREptr _carrier1;
  GESTUREptr _carrier2;
  std::vector<double> _carrier1_coords;
  std::vector<double> _carrier2_coords;
  std::vector<GestureStroke> _strokes;
  double _scale;

  static const double FADE_WIDTH;

};

#endif  // _CARRIERS_CELL_H_
