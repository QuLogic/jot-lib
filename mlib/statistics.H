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
#ifndef STATISTICS_H_IS_INCLUDED
#define STATISTICS_H_IS_INCLUDED

/*!
 *  \file statistics.H
 *  \brief Statistical functions.
 *  \ingroup group_MLIB
 *
 */

namespace mlib {

//! \brief Calculates standard statistical information like average, standard
//! deviation, min and max and optionally print it or return them.
//!
//! The template parameter \p T should be some sort of numerical type for this
//! function to work.
//!
//! \param[in]  list    ARRAY of values over which statistics will be computed.
//! \param[in]  print   Whether or not to print results to stderr.
//! \param[out] average Average of values in \p list.
//! \param[out] std_d   Standard deviation of values in \p list.
//! \param[out] _max    Maximum of values in \p list.
//! \param[out] _min    Minimum of values in \p list.
//!
//! \question Do we want this function to print its results to stderr or to stdout?
//!
//! \remark The sun compiler rejects default argument values in templated
//! functions.
template <class T>
void
statistics(
   CARRAY<T> &list,
   bool print,
   double *average,
   double* std_d,
   T* _max,
   T* _min)
{
   double sum=0; 
   double sqr_sum=0; 
   T max=0;
   T min=DBL_MAX;

   for(int i=0;i<list.num();i++) 
   { 
      sum+=list[i]; 
      sqr_sum+=(list[i]*list[i]);
      if(list[i]>max) max=list[i];
      if(list[i]<min) min=list[i];
   }
   double avg = sum/list.num();
   double std = sqrt(sqr_sum/list.num() - avg*avg);

   if(print)
      cerr << "Statistics ------ # of samples = " << list.num()<<
      ", Average = " << avg << ", Std D= " << std<< ", Max= " << max<< ", Min= " <<min<<endl;

   if(average) *average=avg;
   if(std_d) *std_d=std;
   if(_max) *_max = max;
   if(_min) *_min = min;
}

} // namespace mlib

#endif // STATISTICS_H_IS_INCLUDED

// End of file statistics.H
