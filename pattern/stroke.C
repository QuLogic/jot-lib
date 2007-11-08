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
#include "stroke.H"
#include "mlib/points.H"

using namespace mlib;

//////////////////////////////
// constructor/destructor
//////////////////////////////

Stroke::Stroke(const Stroke& ref_stroke, CBBOXpix& ref_bbox, CBBOXpix& target_bbox)
  : _gesture (0),
    _id(ref_stroke.id()), _bbox(target_bbox){

  // first compute the transformed pixels list
  const PIXEL_list& ref_pixels = ref_stroke.pts();
  int nb_ref_pixels = ref_pixels.num();
  if (nb_ref_pixels == 0){
    return;
  }

  PIXEL_list target_pixels;
  ref_to_target(ref_bbox, ref_pixels, target_bbox, target_pixels);


  // then build new gesture
  CARRAY<double>& ref_pressures = ref_stroke.gesture()->pressures();
  _gesture = new GESTURE(0, 0, target_pixels[0], ref_pressures[0]);
  for (int i=1 ; i<nb_ref_pixels ; i++){
    _gesture->add(target_pixels[i], -1.0, ref_pressures[i]);
  }

}


void 
Stroke::ref_to_target(CBBOXpix& ref_bbox, CPIXEL_list& ref_pixels, 
		      CBBOXpix& target_bbox, PIXEL_list& target_pixels){

  // first determined transformation scaling and traslating parameters
  PIXEL origin;
  VEXEL ref_translation = ref_bbox.min() - origin;
  VEXEL target_translation = target_bbox.min()  - origin;
  PIXEL ref_to_target_scale (target_bbox.width()/ref_bbox.width(), target_bbox.height()/ref_bbox.height());

  // then transform the ref list to get the target list
  int nb_ref_pixels = ref_pixels.num();
  for (int i=0 ; i<nb_ref_pixels ; i++){
    PIXEL target_pixel = (ref_pixels[i]-ref_translation)*ref_to_target_scale + target_translation;
    target_pixels += target_pixel;
  }
}
