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
#ifndef KBD_NAV_H_IS_INCLUDED
#define KBD_NAV_H_IS_INCLUDED

#include "std/time.H"
#include "disp/gel.H"
#include "disp/view.H"
#include "manip/manip.H"

MAKE_PTR_SUBC(kbd_nav, FRAMEobs);
class kbd_nav : public FRAMEobs, public Key_int {
  protected:
   int        _kmap[256];
   CAMdataptr _data;
   int        _speed_lr,
              _speed_fb,
              _speed_ud,
              _speed_tilt,
              _speed_rot;

  public:

         kbd_nav(VIEWptr &);
   int   tick(void);
   int   down(CEvent &e, State *&)  { _kmap[int(e._c)]  = 3; return 0; }
   int   up  (CEvent &e, State *&)  { _kmap[int(e._c)] &= 2; return 0; }

   void  step_right    (int rate)   { _speed_lr   -= rate; }
   void  step_left     (int rate)   { _speed_lr   += rate; }
   void  step_front    (int rate)   { _speed_fb   -= rate; }
   void  step_back     (int rate)   { _speed_fb   += rate; }
   void  step_up       (int rate)   { _speed_ud   -= rate; }
   void  step_down     (int rate)   { _speed_ud   += rate; }
   void  step_tilt_up  (int rate)   { _speed_tilt -= rate; }
   void  step_tilt_down(int rate)   { _speed_tilt += rate; }
   void  step_rot_left (int rate)   { _speed_rot  -= rate; }
   void  step_rot_right(int rate)   { _speed_rot  += rate; }
};

#endif // KBD_NAV_H_IS_INCLUDED
