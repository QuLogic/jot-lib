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
#ifndef NEAREST_PT_H_IS_INCLUDED
#define NEAREST_PT_H_IS_INCLUDED

/*!
 *  \file nearest_pt.H
 *  \brief Contains the nearest_pt_to_line_seg function.
 *  \ingroup group_MLIB
 *
 */

namespace mlib {

//! \brief Returns the nearest point on the line segment (\p st,\p en) to the
//! point, \p pt.
//!
//! The template paramenter \p P should be some sort of point class.
//!
//! \param[in] pt The point to find the nearest point on the line segment to.
//! \param[in] st The starting endpoint of the line segment.
//! \param[in] en The ending endpoint of the line segment.
//!
//! \return The point on the line segment from \p st to \p en that is nearest to
//! the point \p pt.
template <class P>
inline P
nearest_pt_to_line_seg(const P &pt, const P &st, const P &en)
{
   
   P npt = st + (en-st)*((en-st)*(pt-st)/(en-st).length_sqrd());
   if ((npt - st) * (npt - en) < 0)
      return npt;
   return ((pt-st).length_sqrd()<(pt-en).length_sqrd()) ? st : en;
   
}

} // namespace mlib
    
#endif // NEAREST_PT_H_IS_INCLUDED
