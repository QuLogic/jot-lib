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
#ifndef EASEL_MANAGER_H_IS_INCLUDED
#define EASEL_MANAGER_H_IS_INCLUDED

/*!
 *  \file easel_manager.H
 *  \file Contains the definition of the EaselManager class.
 *
 *  \sa easel_manager.H
 *
 */

#include "gest/vieweasel.H"
#include "std/support.H"

class EaselManager {
   
   public:
   
      EaselManager()
         : cur_easel_idx(-1) { }
   
      void make_new_easel(const VIEWptr &v);
   
      VIEW_EASELptr cur_easel() const
         { return easel(cur_easel_idx); }
      
      void undisplay_cur_easel();
      
      void next_easel();
      
      VIEW_EASELptr easel(unsigned idx) const
         { return easel_list.valid_index(idx) ?
                     easel_list[idx] : VIEW_EASELptr(0); }
      
      long num_easels() const
         { return easel_list.num(); }
      
      void clear_easels();
   
   private:
   
      // Not copy constructable or assignable
      EaselManager(const EaselManager&);
      EaselManager& operator=(const EaselManager&);
   
      LIST<VIEW_EASELptr> easel_list;
      long cur_easel_idx;
   
};

#endif // EASEL_MANAGER_H_IS_INCLUDED
