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
 *  \file easel_manager.C
 *  \brief Contains the implementation of the EaselManager class.
 *
 *  \sa easel_manager.H
 *
 */

#include "geom/world.H"
#include "gest/vieweasel.H"

#include "easel_manager.H"

void
EaselManager::make_new_easel(const VIEWptr &v)
{
   
   if(!cur_easel()){
      
      VIEW_EASELptr easel = new VIEW_EASEL(v);
      
      assert(easel);

      cur_easel_idx = easel_list.num();
      easel_list += easel;

      WORLD::message("Created new easel");
      
   }
   
}

void
EaselManager::undisplay_cur_easel()
{
   
   if(cur_easel())
      cur_easel()->removeEasel();
   
   cur_easel_idx = -1;
   
}

void
EaselManager::next_easel()
{
   
   if(num_easels() == 0)
      return;

   if(cur_easel())
      cur_easel()->removeEasel();
   
   // Is this a no-op?
   cur_easel() = NULL;

   // Gently find and set aside the easel
   // that will be the new one:
   long k = 0;                                  // index of new one
   if(cur_easel())
      k = (cur_easel_idx + 1) % num_easels();
   
   VIEW_EASELptr cur = easel(k);                // new one
   
   // Make sure next easel exists and was fetched properly:
   assert(cur);

   // Now pretend there is no current easel:
   cur_easel_idx = -1;

   // ... and tell the new one to take charge:
   cur->restoreEasel();

   // Whew! That generated camera callbacks, triggering
   // Draw_int::notify(), which tried to deactivate the current
   // easel. But there is no "current easel," get it?? (wink, wink).

   // Enough of the charade. Back to business:
   cur_easel_idx = k;

}
      
void
EaselManager::clear_easels()
{
   
   easel_list.clear();
   cur_easel_idx = -1;

   WORLD::message("cleared easels");
   
}
