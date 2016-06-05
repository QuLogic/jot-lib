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
/*!
 *  \file random.C
 *  \brief Contains the implementation of the non-inline functions of the
 *  RandomGen class.
 *  \ingroup group_MLIB
 *
 */

#include "std/support.H"
#include "random.H"

long mlib::RandomGen::R_MAX = RAND_MAX;

void 
mlib::RandomGen::update()  {
  //REPLACE THIS WITH OUR SLY NEW CROSS PLATFORM RANDOM FUNCTION;
  //WORKS AS LONG AS A SEED DOES NOT RETURN ITSELF

  srand48 ( _seed ) ;
  _seed =  lrand48();

}
