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
#ifndef PEN_MANAGER_H_IS_INCLUDED
#define PEN_MANAGER_H_IS_INCLUDED

/*!
 *  \file pen_manager.H
 *  \brief Contains the defintion of the PenManager class.
 *
 *  \sa pen_manager.C pen.H pen.C
 *
 */

#include <vector>

#include "geom/fsa.H"

class Pen;

class PenManager {
   
   public:
   
      PenManager(State *start_in)
         : start(start_in), cur_pen_idx(0) { }
      
      ~PenManager();
      
      //! \name Pen Accessor Functions
      //@{
      
      void add_pen(Pen *pen);
      
      void select_pen(Pen *pen);
      
      long num_pens() const
         { return pens.size(); }
      
      Pen *cur_pen() const
         { return pens.size() > 0 ? pens[cur_pen_idx] : 0; }
      
      void next_pen()
         { cycle_pen(1); }
      void prev_pen()
         { cycle_pen(-1); }
      
      //@}
   
   private:
   
      // Not copy constructable or assignable:
      PenManager(const PenManager&);
      PenManager &operator=(const PenManager&);
      
      State *start;
      
      void cycle_pen(int idx_change);
   
      std::vector<Pen*> pens;
      
      long cur_pen_idx;

};

#endif // PEN_MANAGER_H_IS_INCLUDED
