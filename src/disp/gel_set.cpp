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
#include "gel_set.H"
#include "ray.H"        // for GEL_list<T>::intersect

using namespace mlib;

BBOX 
GELset::bbox(int i) const{
  BBOX cur_bbox;
  unsigned int nb_gel = _gel_list.num();
  for (unsigned int i=0 ; i<nb_gel ; i++){
    cur_bbox += _gel_list[i]->bbox();

  }
  return cur_bbox;
}
