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
#include "gesture_cell.H"
#include "mlib/points.H"
#include "pattern_pen_ui.H"

using namespace mlib;


double 
GestureCell::alpha (CUVpt& pt, CPIXEL& pix) const
{
 // return alpha(pt);
    
  double val = 0.0;	
  if (pt[0]>=0.0 && pt[0]<=1.0){
    if (pt[1]<0.0 || pt[1]>1.0){
      val = 0.0;   
    } else {
      val = 1.0;
    }
	
    CTEXTUREptr back_t = VIEW::peek()->get_bkg_tex();
	if(back_t && (PatternPenUI::use_image_pressure)){
        Image back_image = back_t->image();
        uint r = back_image.pixel_r((uint)pix[0],(uint)pix[1]);
        uint g = back_image.pixel_g((uint)pix[0],(uint)pix[1]);
        uint b = back_image.pixel_b((uint)pix[0],(uint)pix[1]);
		uint a = back_image.pixel_a((uint)pix[0],(uint)pix[1]);
        COLOR bla(r,g,b);
		val *= PatternPenUI::luminance_function(min(double(a),double(1.0-(bla.luminance()/255))));
    }
  } else {
    val = 0.0;
  }
  return val;	
}
