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
 *  \file global.C
 *  \brief Contains values of epsilon constants from global.H and the
 *  implementation of the setEpsAbsMath function.
 *  \ingroup group_MLIB
 *
 */
 
#include <cassert>

using namespace std;

#include "global.H"

const double mlib::gEpsZeroMath  = 1e-12;    //!< Really a very small value

double       mlib::gEpsAbsMath     = 1e-8;   //!< Absolute epsilon
double       mlib::gEpsAbsSqrdMath = 1e-16;  //!< Absolute epsilon squared

const double mlib::gEpsNorMath     = 1e-10;  //!< Normalized epsilon
const double mlib::gEpsNorSqrdMath = 1e-20;  //!< Normalized epsilon squared

void 
mlib::setEpsAbsMath(double eps)
{
    assert(eps > 0);

    gEpsAbsMath     = eps;
    gEpsAbsSqrdMath = eps * eps;
}

void 
mlib::fn_gdb_will_recognize_so_i_can_set_a_fuggin_breakpoint()
{
   // TODO: Learn how to set a breakpoint in a templated function.
   //       For now using this...

   int n = 3;
   if (n < 2)
      n++;
}

// end of file global.C
