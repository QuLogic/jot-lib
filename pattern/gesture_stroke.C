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
#include "gesture_stroke.H"
#include "mlib/points.H"
#include "disp/gel.H"

using namespace mlib;

///////////////
// processes
///////////////

void 
GestureStroke::copy(GestureCell* target_cell, const UVpt& offset, bool stretch) const{

  const PIXEL_list& src_pts = _gesture->pts();
  CARRAY<double>& src_press = _gesture->pressures();
  int nb_pts = src_pts.num();
  if (nb_pts==0) return;
  
  if (!_bbox.valid()) return;

  vector<UVpt> target_uv_pts;
  vector<double> target_pressures;
  
  double inv_width = 1.0;
  double inv_height = 1.0;
  if (stretch){
    inv_width = 1.0/_bbox.width();
    inv_height = 1.0/_bbox.height();
  } else {
    inv_width = 1.0/target_cell->scale();
    inv_height = 1.0/target_cell->scale();
  }
  for (int i=0 ; i<nb_pts ; i++){
    VEXEL src_vec (src_pts[i]-_bbox.min());
    UVpt src_uv_pt (src_vec[0]*inv_width, src_vec[1]*inv_height);
    
    UVpt target_uv_pt =  offset + src_uv_pt;
    target_uv_pts.push_back(target_uv_pt);
    target_pressures.push_back(src_press[i]);
  }
  
  target_cell->add_stroke(target_uv_pts, target_pressures);
}



void 
GestureStroke::synthesize(GestureCell* target_cell, double target_pressure, double ref_angle, CUVpt& ref_pos, 
			  CUVvec& target_scale, double target_angle, CUVpt& target_pos) const{

  const PIXEL_list& ref_pts = _gesture->pts();
  CARRAY<double>& ref_press = _gesture->pressures();
  int nb_pts = ref_pts.num();
  if (nb_pts==0) return;
  
  if (!_bbox.valid()) return;

  vector<UVpt> target_pts;
  vector<double> target_pressures;

  double inv_scale = 1.0/target_cell->scale();
  for (int i=0 ; i<nb_pts ; i++){
    // first express the stroke point in the reference path's frame
    UVpt ref_pt ((ref_pts[i][0]-ref_pos[0]), 
 		 (ref_pts[i][1]-ref_pos[1]));
    ref_pt =  UVpt(ref_pt[0]*cos(-ref_angle) - ref_pt[1]*sin(-ref_angle),
  		   ref_pt[0]*sin(-ref_angle) + ref_pt[1]*cos(-ref_angle)); 

    // then transform to target frame
    UVpt target_pt (ref_pt[0]*target_scale[0], ref_pt[1]*target_scale[1]);
    target_pt = UVpt(target_pt[0]*cos(target_angle) - target_pt[1]*sin(target_angle),
  		     target_pt[0]*sin(target_angle) + target_pt[1]*cos(target_angle));
    target_pt *= inv_scale;
    target_pt += target_pos;

    // and finally add the newly created point
    target_pts.push_back(target_pt);
    target_pressures.push_back(ref_press[i]*target_pressure);
  }
  
  target_cell->add_stroke(target_pts, target_pressures);
}



void 
GestureStroke::add_stroke_pt(CPIXEL& pt, double pressure) {
  if (!_bbox.valid()) return;
  
  if (!_gesture){
    _gesture = new GESTURE(NULL, -1, pt, pressure);
    EXIST.rem(_gesture);
  } else {
    _gesture->add(pt, -1.0, pressure);
  }
}


