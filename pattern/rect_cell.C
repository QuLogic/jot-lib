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
#include "pattern/rect_cell.H"

using namespace std;
using namespace mlib;

const double RectCell::FADE_WIDTH = 0.1;

//////////////////
// accessors
//////////////////

void 
RectCell::add_stroke(const vector<UVpt>& pts,
		     const vector<double>& pressures){
  int nb_rect_pts = _rect->pts().num();
  BBOXpix dummy_bbox (_rect->pts()[0], _rect->pts()[nb_rect_pts-1]);
  GestureStroke stroke (dummy_bbox);
  
  PIXEL start = _rect->start();
  PIXEL end = _rect->end();
  VEXEL normal = (end-start).perpend().normalized();
  unsigned int nb_pts = pts.size();
  for (unsigned int i=0 ; i<nb_pts ; i++){
    PIXEL pixel_on_axis = (1.0-pts[i][0])*start + pts[i][0]*end;
    PIXEL cur_pix = pixel_on_axis + normal*(pts[i][1]-0.5)*_scale;

    double cur_pressure = pressures[i] * alpha(pts[i]) * GestureCell::alpha(pts[i], cur_pix);
    stroke.add_stroke_pt(cur_pix, cur_pressure);
  } 

  _strokes.push_back(stroke);
}



double 
RectCell::alpha(CUVpt& pt) const{
  double inf = 0.5*(1.0-(_thickness/_scale));
  double sup = 1.0-inf;
  if (pt[0]>=0.0 && pt[0]<=1.0){
    if (pt[1]<inf || pt[1]>sup){
      return 0.0;
    } else if (pt[1]<inf || pt[1]>sup) {
      return min(inf-pt[1], pt[1]-sup)/inf;
    } else {
      return 1.0;
    }
  } else {
    return 0.0;
  }
}
