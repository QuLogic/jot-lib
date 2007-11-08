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
#ifndef RUNNING_AVG_IS_INCLUDED
#define RUNNING_AVG_IS_INCLUDED

#include "std/platform.H"

/*****************************************************************
 * RunningAvg:
 *
 *   Convenience class for keeping a running average.
 *   (E.g. average frame rate).
 *****************************************************************/
template <class T>
class RunningAvg {
 public:
   RunningAvg(const T& null)             : _val(null), _num(0) {}
   RunningAvg(const T& null, const T& v) : _val(null), _num(0) { add(v); }

   // Average in a new item:
   void add(const T& v) {
      if (_num == 0) {
         _val = v;
         _num++;
      } else {
         _val = interp(_val, v, 1.0/++_num);
      }
   }

   // Get the current value of the average:
   const T& val()       const   { return _val; }

   // Get the number of items used in the average:
   int num()            const   { return _num; }

 protected:
   T    _val;   // the running average
   int  _num;   // the number of items in the average so far
};

#endif // RUNNING_AVG_IS_INCLUDED

/* end of file run_avg.H */
