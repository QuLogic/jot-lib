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
#include "pattern/path_cell.H"

using namespace std;
using namespace mlib;

const double PathCell::FADE_WIDTH = 0.1;

//////////////////
// accessors
//////////////////

void 
PathCell::add_stroke(const vector<UVpt>& pts,
		     const vector<double>& pressures){
  int nb_path_pts = _path->pts().num();
  BBOXpix dummy_bbox (_path->pts()[0], _path->pts()[nb_path_pts-1]);
  GestureStroke stroke (dummy_bbox);
  
  unsigned int nb_pts = pts.size();
  for (unsigned int i=0 ; i<nb_pts ; i++){
    PIXEL p, q;
    double beta = closest_pixels(pts[i][0], p, q);
    PIXEL pixel_on_path = (1.0-beta)*p + beta*q;
    VEXEL normal_on_path = (q-p).perpend().normalized();
    PIXEL cur_pix = pixel_on_path + normal_on_path*(pts[i][1]-0.5)*_scale;

    double cur_pressure = pressures[i] * alpha(pts[i]) * GestureCell::alpha(pts[i], cur_pix);
    stroke.add_stroke_pt(cur_pix, cur_pressure);
  }

  _strokes.push_back(stroke);
}


double 
PathCell::closest_pixels (double u, PIXEL& p, PIXEL& q){
  const PIXEL_list& pts = _path->pts();
  unsigned int nb_path_pixels = _path_coords.size();
  bool found = false;
  double alpha = 0.0;
  
  for (unsigned int i=1 ; i<nb_path_pixels && !found; i++){
    if (u<=_path_coords[i]){
      p = pts[i-1];
      q = pts[i];
      alpha = (u-_path_coords[i-1])/(_path_coords[i]-_path_coords[i-1]);
      found = true;
    }
  }

  if (found) {
    return alpha;
  } else { // u>1.0
      p = pts[nb_path_pixels-2];
      q = pts[nb_path_pixels-1];
      return (u-_path_coords[nb_path_pixels-2])/(_path_coords[nb_path_pixels-1]-_path_coords[nb_path_pixels-2]);
  }  
}


void 
PathCell::compute_path_coords (){
  const PIXEL_list& pts = _path->pts();
  unsigned int nb_path_pixels = pts.num();
  double length = _path->length();
  
  if (nb_path_pixels<2) return;
  
  double path_coord = 0.0;
  PIXEL last_pixel = pts[0];
  for (unsigned int i=0;  i<nb_path_pixels-1 ; i++){
    PIXEL cur_pixel = pts[i];
    path_coord += cur_pixel.dist(last_pixel)/length;
    last_pixel = cur_pixel;
    _path_coords.push_back(path_coord);
  }
  _path_coords.push_back(1.0);
}


double 
PathCell::alpha(CUVpt& pt) const{
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
