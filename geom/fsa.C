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
 *  \file fsa.C
 *  \brief Contains the implementation of FSA (finite state automata) classes.
 *
 *  \note The implementations for some other classes are in here as well, but
 *  they should probably be moved to another file.
 *
 *  \sa fsa.H
 *
 */

#include "fsa.H"

HASH *VIEWint_list::_dhash = 0;

void 
VIEWint::rem_interactor(State *s)
{
   FSA* fsa = 0;
   for (int i=0; i < _cur_states.num(); i++) {
      if ((fsa = _cur_states[i])->start() == s) {
         _cur_states.remove(i);
         // XXX - following line assumes interactor was
         //  added via
         //     VIEWint::add_interactor(State *s),
         //  which allocates a new FSA,
         //  so we delete it now:
         delete fsa;
         break;
      }
   }
}

//
// Forwards events to any matching FSA's
//
void
VIEWint::handle_event(
   CEvd  &e
   )
{
   VIEW::push(_view);
   Event  ev(_view, e);
   for (int i=0; i < _cur_states.num(); i++) {
      _cur_states[i]->handle_event(ev);
   }

   while (!_events.empty()) {
      ev = Event(_view, _events.pop());
      for (int t=0; t < _cur_states.num(); t++) {
         _cur_states[t]->handle_event(ev);
      }
   }
   VIEW::pop();
}

/* end of file fsa.C */
