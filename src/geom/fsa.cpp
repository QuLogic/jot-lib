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

map<CVIEWptr,VIEWint*> *VIEWint_list::_dhash = nullptr;

void 
VIEWint::rem_interactor(State *s)
{
   FSA* fsa = nullptr;
   vector<FSA*>::iterator it;
   for (it=_cur_states.begin(); it!=_cur_states.end(); ++it) {
      fsa = *it;
      if (fsa->start() == s) {
         _cur_states.erase(it);
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
   for (auto & state : _cur_states) {
      state->handle_event(ev);
   }

   while (!_events.empty()) {
      ev = Event(_view, _events.back());
      _events.pop_back();
      for (auto & state : _cur_states) {
         state->handle_event(ev);
      }
   }
   VIEW::pop();
}
