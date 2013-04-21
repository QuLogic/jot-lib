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
 *  \file pen_manager.C
 *  \brief Contains the implementation of the PenManager class.
 *
 *  \sa pen_manager.H pen.H pen.C
 *
 */

#include <iostream>

using namespace std;

#include "gest/pen.H"

#include "pen_manager.H"

PenManager::~PenManager()
{
   
   for(unsigned long i = 0; i < pens.size(); ++i){
      
      delete pens[i];
      pens[i] = 0;
      
   }
   
   pens.clear();
   
}

void
PenManager::add_pen(Pen *pen)
{
   
   pens.push_back(pen);
   
   // Active the Pen if it is the first one to be added:
   if(pens.size() == 1){
      
      // Make sure no other pens were thought to be current:
      assert(cur_pen_idx == 0);
      
      pens[cur_pen_idx]->activate(start);
      
   }
   
}

void
PenManager::select_pen(Pen *pen)
{
   
   long pen_idx = -1;
   
   for(unsigned i = 0; i < pens.size(); ++i){
      
      if(pens[i] == pen){
         
         pen_idx = i;
         break;
         
      }
      
   }
   
   // Do nothing if the specified pen is not part of the manager:
   if(pen_idx == -1)
      return;
   
   // Do nothing if the specified pen is already the current pen:
   if(pen_idx == cur_pen_idx)
      return;
   
   // Switch to the specified pen:
   if(pens[cur_pen_idx]->deactivate(start)){
      
      cur_pen_idx  = pen_idx;
      
      pens[cur_pen_idx]->activate(start);
   
   } else {
      
      cerr << "PenManager::select_pen() - "
           << "Pen change failed! Current pen refuses to deactivate!!"
           << endl;
      
   }
   
}

void
PenManager::cycle_pen(int idx_change)
{
   
   // Do nothing if there are no pens or one pen:
   if((pens.size() == 0) || (pens.size() == 1))
      return;
   
   if(pens[cur_pen_idx]->deactivate(start)){
      
      do {
         
         idx_change += pens.size();
         
      } while(idx_change < 0);
      
      cur_pen_idx  = (cur_pen_idx + idx_change) % pens.size();
      
      pens[cur_pen_idx]->activate(start);
   
   } else {
      
      cerr << "PenManager::cycle_pen() - "
           << "Pen change failed! Current pen refuses to deactivate!!"
           << endl;
      
   }
   
}
