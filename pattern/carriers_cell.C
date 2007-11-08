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
#include "pattern/carriers_cell.H"

using namespace std;
using namespace mlib;

const double CarriersCell::FADE_WIDTH = 0.1;

//////////////////
// accessors
//////////////////

void 
CarriersCell::add_stroke(const vector<UVpt>& pts,
			 const vector<double>& pressures){
  int nb_carrier1_pts = _carrier1->pts().num();
  BBOXpix dummy_bbox1 (_carrier1->pts()[0], _carrier1->pts()[nb_carrier1_pts-1]);
  int nb_carrier2_pts = _carrier2->pts().num();
  BBOXpix dummy_bbox (_carrier2->pts()[0], _carrier2->pts()[nb_carrier2_pts-1]);
  dummy_bbox += dummy_bbox1;
  GestureStroke stroke (dummy_bbox);

  PIXEL start1 = _carrier1->start();
  PIXEL end1 = _carrier1->end();
  PIXEL start2 = _carrier2->start();
  PIXEL end2 = _carrier2->end();
  double min_thickness = min((start1-start2).length(), (end1-end2).length());
  
  unsigned int nb_pts = pts.size();
  for (unsigned int i=0 ; i<nb_pts ; i++){
    PIXEL p, q;
    double beta = closest_pixels(pts[i][0], p, q, _carrier1, _carrier1_coords);
    PIXEL pixel_on_carrier1 = (1.0-beta)*p + beta*q;
    double gamma = closest_pixels(pts[i][0], p, q, _carrier2, _carrier2_coords);
    PIXEL pixel_on_carrier2 = (1.0-gamma)*p + gamma*q;

    PIXEL mid_pixel = (pixel_on_carrier1+pixel_on_carrier2)*0.5;
    VEXEL pixel_vec = (pixel_on_carrier2-pixel_on_carrier1);
    double local_scale = _scale*pixel_vec.length()/min_thickness;
    
    PIXEL cur_pix = mid_pixel+(pts[i][1]-0.5)*pixel_vec.normalized()*local_scale;

//     PIXEL cur_pix = pts[i][1]*pixel_on_carrier1 + (1.0-pts[i][1])*pixel_on_carrier2;

    double cur_pressure = pressures[i]* GestureCell::alpha(pts[i], cur_pix);
    stroke.add_stroke_pt(cur_pix, cur_pressure);
  }

  _strokes.push_back(stroke);
}


double 
CarriersCell::closest_pixels (double u, PIXEL& p, PIXEL& q, 
			      const GESTUREptr carrier, const vector<double>& carrier_coords) const{
 
  const PIXEL_list& pts = carrier->pts();
  unsigned int nb_carrier_pixels = carrier_coords.size();
  bool found = false;
  double alpha = 0.0;
  
  for (unsigned int i=1 ; i<nb_carrier_pixels && !found; i++){
    if (u<=carrier_coords[i]){
      p = pts[i-1];
      q = pts[i];
      alpha = (u-carrier_coords[i-1])/(carrier_coords[i]-carrier_coords[i-1]);
      found = true;
    }
  }

  if (found) {
    return alpha;
  } else { // u>1.0
      p = pts[nb_carrier_pixels-2];
      q = pts[nb_carrier_pixels-1];
      return (u-carrier_coords[nb_carrier_pixels-2])/(carrier_coords[nb_carrier_pixels-1]-carrier_coords[nb_carrier_pixels-2]);
  }  
}


void 
CarriersCell::compute_carriers_coords (bool compute_coords1){
  GESTUREptr carrier;
  vector<double>* carrier_coords;
  if (compute_coords1) {
    carrier = _carrier1;
    carrier_coords = &_carrier1_coords;
  } else {
    carrier = _carrier2;
    carrier_coords = &_carrier2_coords;
  }

  const PIXEL_list& pts = carrier->pts();
  unsigned int nb_carrier_pixels = pts.num();
  double length = carrier->length();
  
  if (nb_carrier_pixels<2) return;
  
  double carrier_coord = 0.0;
  PIXEL last_pixel = pts[0];
  for (unsigned int i=0;  i<nb_carrier_pixels-1 ; i++){
    PIXEL cur_pixel = pts[i];
    carrier_coord += cur_pixel.dist(last_pixel)/length;
    last_pixel = cur_pixel;
    carrier_coords->push_back(carrier_coord);
  }
  carrier_coords->push_back(1.0);
}



double 
CarriersCell::alpha(CUVpt& pt) const{
  if (pt[0]>=0.0 && pt[0]<=1.0){
    if (pt[1]<0.0 || pt[1]>1.0){
      return 0.0;
    } else if (pt[1]<FADE_WIDTH || pt[1]>1.0-FADE_WIDTH) {
      return min(pt[1], 1.0-pt[1])/FADE_WIDTH;
    } else {
      return 1.0;
    }
  } else {
    return 0.0;
  }
}
