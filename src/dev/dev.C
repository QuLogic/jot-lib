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
// dev.C

#include "dev.H"

using namespace mlib;

DEVmod_desc_list   DEVmod_gen::_mods;
DEVmod_gen        *DEVmod_gen::_gen         = 0;
Evd::DEVmod        DEVmod_gen::_forced_mods = Evd::EMPTY;

DEVice_2d *DEVice_2d::last = 0;

// Wtransf DEVice::booth_to_room;
// Wtransf DEVice::room_to_booth;

void
DEVice_2d_absrel::up(
   )
{
  _down_flag = false;
}

void
DEVice_2d_absrel::down(
   )
{
  _down_flag  = true;
  _first_down = true;
}

void
DEVice_2d_absrel::_event(
   CXYpt        &p,
   Evd::DEVmod  mod
   )
{
  _cur_abs_pos = p;

  if (_first_down) { // hack since tablet doesn't report events when just 
                     // above the surface..
    _first_down  = false;
    _old_abs_pos = _cur_abs_pos;
  }

  if ( _rel_flag && _down_flag) {
     _logical_pos += (_cur_abs_pos - _old_abs_pos);
     _old_abs_pos = _cur_abs_pos;
  }

  if (!_rel_flag)
    _logical_pos  = _cur_abs_pos;

  DEVice_2d::_event(_logical_pos, mod);
}

void
print_bits( char *buf, int num_bytes )
{
  for(int i=0;i<num_bytes;i++) {
    for(int b=0;b<8;b++)
      cerr << (((1 << b) & ((int)buf[i])) ? 1 : 0);
    cerr << " ";
  }
  cerr << endl;
}


Evd::DEVmod
DEVmod_gen::mods()
{
   Evd::DEVmod mods = Evd::EMPTY;

   for (int m = 0; m < _mods.num(); m++) {
      DEVice_buttons *btns = _mods[m].ice();
      if (btns->get(_mods[m].btn_ind()))
         mods = Evd::DEVmod(((int)mods) | (int)(_mods[m].mod()));
   }

   Evd::DEVmod gmods = Evd::EMPTY;
   if (_gen)
      gmods = _gen->gen_mods();

   mods = Evd::DEVmod(((int)mods) | ((int)gmods) | ((int)_forced_mods));

   if (mods == Evd::EMPTY)
      mods = Evd::NONE;

   return mods;
}
