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
#include "std/support.H"
#include "glew/glew.H"

#include "glut_winsys.H"
#include "tty_glut.H"
#include "glui/glui.h"
#include "kbd.H"
#include "geom/fsa.H"
#include "std/config.H"

using mlib::PIXEL;
using mlib::CXYpt;

ARRAY<GLUT_KBD *> GLUT_KBD::_kbds(1);

extern "C" void
normal_keydown_callback(unsigned char k, int x, int y)
{
   GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);
   // XXX - Just block events to the blocking window, or all windows?
   //       Rightnow, we only use one window, so this is academic...
   if (mgr->get_blocker() == GLUT_WINSYS::window()) return;

   static bool debug = Config::get_var_bool("JOT_DEBUG_KEY_CALLBACKS",false,true);
   if (debug)
      err_msg("key pressed: %d", int(k));

   GLUT_KBD *kbd = GLUT_KBD::kbd();

   int w, h;
   kbd->winsys()->size(w, h);

   VIEWptr view = GLUT_WINSYS::window()->view();

   VIEWint_list::get(view)->handle_event(Evd(k));
   
   CXYpt curs(PIXEL(x, (double) h - y));

   kbd->update_mods(curs);
}

extern "C" void
normal_keyup_callback(unsigned char k, int x, int y)
{
   GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);
   // XXX - Just block events to the blocking window, or all windows?
   //       Rightnow, we only use one window, so this is academic...
   if (mgr->get_blocker() == GLUT_WINSYS::window()) return;

   GLUT_KBD *kbd = GLUT_KBD::kbd();
   VIEWptr view = GLUT_WINSYS::window()->view();

   VIEWint_list::get(view)->handle_event(Evd(k,KEYU));

   int w, h;
   kbd->winsys()->size(w, h);
   CXYpt curs(PIXEL(x, (double) h - y));

   kbd->update_mods(curs);
}

//
// GLUT can't send shift/ctrl up/down as an event, so before
// every event is sent we have to check to see if the modifier
// status has changed, and if it has we have to send the change
// event
//
void
GLUT_KBD::update_mods(CXYpt &/*cur*/) {

   const int mods = glutGetModifiers();
   const bool new_shift = (mods & GLUT_ACTIVE_SHIFT) ? 1 : 0;
   const bool new_ctrl  = (mods & GLUT_ACTIVE_CTRL)  ? 1 : 0;

   if (new_shift != _shift) {
      _shift = new_shift;
   }
   if (new_ctrl != _ctrl) {
      _ctrl = new_ctrl;
   }
}

GLUT_KBD::GLUT_KBD(GLUT_WINSYS *winsys)
   : _winsys(winsys), _shift(false), _ctrl(false)
{
   while (_kbds.num() <= winsys->id()) {
      // XXX - Must cast or += ARRAY is used, which makes this an infinite loop
      _kbds += (GLUT_KBD *) 0;
   }
   _kbds[winsys->id()] = this;
   glutKeyboardFunc(normal_keydown_callback);
   glutKeyboardUpFunc(normal_keyup_callback);
   //For imbedded GLUI windows that must 'share' the callbacks...
   //GLUI_Master.set_glutKeyboardFunc(normal_keydown_callback);
}

GLUT_KBD *
GLUT_KBD::kbd()
{
   return _kbds[glutGetWindow()];
}
