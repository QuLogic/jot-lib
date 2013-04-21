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

#include "dev/dev.H"
#include "mlib/points.H"
#include "std/stop_watch.H"
#include "std/config.H"

#include <GL/glut.h>
#include "glut_winsys.H"
#include "tty_glut.H"
#include "mouse.H"

#include "kbd.H"

using mlib::PIXEL;
using mlib::CXYpt;

#ifdef USE_GLUT_WACOM
#include "glutwacom/glutwacom.h"

static bool stylus_down = false;
static bool eraser_down = false;
static bool stylus2_down = false;

static bool debug = Config::get_var_bool("DEBUG_GLUT_WACOM",false);

inline str_ptr
wacom_device_name(int device)
{
   // Convert Wacom device numeric code to string.

   switch(device) {
    case GLUT_WACOM_CURSOR:
      return "cursor";
    case GLUT_WACOM_STYLUS:
      return "stylus";
    case GLUT_WACOM_ERASER:
      return "eraser";
    case GLUT_WACOM_MENU:
      return "menu";
    default:
      return "unkown device";
   }
}

inline str_ptr
wacom_button_state(int state)
{
   // Convert Wacom button state code to string.

   switch (state) {
    case GLUT_WACOM_DOWN:
      return "down";
    case GLUT_WACOM_UP:
      return "up";
    default:
      return "unknown state";
   }
}

inline str_ptr
wacom_tablet_state(int state)
{
   // Convert Wacom tablet state code to string.

   switch (state) {
    case GLUT_WACOM_STATE_OFF_TABLET:
      return "off tablet";
    case GLUT_WACOM_STATE_ON_TABLET:
      return "on tablet";
    case GLUT_WACOM_NO_STATE_INFO:
      return "unknown tablet state";
    default:
      return "undefined tablet state";
   }
}

extern "C"
void
wacom_button_callback(
   int device, int button, int state, int tablet_state,
   float x, float y, float pressure, float xtilt, float ytilt,
   int proximity
   ) 
{
   GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);
   // XXX - Just block events to the blocking window, or all windows?
   //       Rightnow, we only use one window, so this is academic...
   if (mgr->get_blocker() == GLUT_WINSYS::window()) return;

   if (debug || Config::get_var_bool("DEBUG_MOUSE_BUTTON_CALLBACK",false,true)) {
      cerr << wacom_device_name(device) << ", "
           << "(" << wacom_tablet_state(tablet_state) << "), "
           << "button: " << button << ", "
           << "(" << wacom_button_state(state) << ")"
           << endl;
   }

   switch (device) {
    case GLUT_WACOM_CURSOR:
      // The "puck"
      switch (button) {
       case GLUT_WACOM_TOP_LEFT_BUTTON:
       case GLUT_WACOM_TOP_MIDDLE_BUTTON:
       case GLUT_WACOM_TOP_RIGHT_BUTTON:
       case GLUT_WACOM_BOTTOM_RIGHT_BUTTON:
       case GLUT_WACOM_BOTTOM_LEFT_BUTTON:
         err_adv(debug, "wacom_button_callback: cursor button pressed");
         break;
      }
      break;
    case GLUT_WACOM_STYLUS:
      // The "stylus", or pen
      switch (button) {
       case GLUT_WACOM_NO_BUTTON:
         // Translate stylus events into mouse left button events:
         if (tablet_state == GLUT_WACOM_STATE_ON_TABLET) {
            stylus_down = true;
            GLUT_MOUSE::mouse()->buttons().event(
               GLUT_LEFT_BUTTON,  DEVice_buttons::DOWN, Evd::NONE
               );
         } else {
            GLUT_MOUSE::mouse()->buttons().event(
               GLUT_LEFT_BUTTON,  DEVice_buttons::UP, Evd::NONE
               );
         }
         break;
       case GLUT_WACOM_TOP_BUTTON:
         if (state == GLUT_WACOM_DOWN) {
            err_adv(debug, "wacom_button_callback: stylus top button pressed");
         } else {
            err_adv(debug, "wacom_button_callback: stylus top button released");
         }
         break;
       case GLUT_WACOM_BOTTOM_BUTTON:
         if (state == GLUT_WACOM_DOWN) {
            stylus2_down = true;
            GLUT_MOUSE::mouse()->buttons().event(
               GLUT_MIDDLE_BUTTON,  DEVice_buttons::DOWN, Evd::NONE
               );
         } else {
            GLUT_MOUSE::mouse()->buttons().event(
               GLUT_MIDDLE_BUTTON,  DEVice_buttons::UP, Evd::NONE
               );
         }
         break;
      }
      break;
    case GLUT_WACOM_ERASER:
      // Translate eraser events into mouse 3rd button events:
      if (tablet_state == GLUT_WACOM_STATE_ON_TABLET) {
         eraser_down = true;
         GLUT_MOUSE::mouse()->buttons().event(
            GLUT_RIGHT_BUTTON,  DEVice_buttons::DOWN, Evd::NONE
            );
      } else {
         eraser_down = false;
         GLUT_MOUSE::mouse()->buttons().event(
            GLUT_RIGHT_BUTTON,  DEVice_buttons::UP, Evd::NONE
            );
      }
      break;
   }
}

extern "C"
void
wacom_motion_callback(
   int device, int tablet_state,
   float x, float y, float pressure,
   float /* xtilt */, float /* ytilt */,
   int /* proximity */
   ) 
{
//   GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);
   // XXX - Just block events to the blocking window, or all windows?
   //       Rightnow, we only use one window, so this is academic...
//   if (mgr->get_blocker() == GLUT_WINSYS::window()) return;

   GLUT_MOUSE *mouse = GLUT_MOUSE::mouse();

   int w, h;
   mouse->winsys()->size(w, h);
   CXYpt cur(PIXEL(x, (double) h - y));
   mouse->pointer().set_pressure(pressure);
   mouse->pointer().event(cur, Evd::NONE);

   // Note: the following lines add significant lag in
   //       the tablet response.
   if (debug) {
      cerr << wacom_device_name(device) << ", "
           << "(" << wacom_tablet_state(tablet_state) << "), "
           << cur << ", "
           << "pressure: " << pressure << endl;
   }
}

#endif

ARRAY<GLUT_MOUSE *> GLUT_MOUSE::_mice(1);

extern "C"
void 
mouse_motion_callback(
   int x,
   int y
   )
{
   GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);
   // XXX - Just block events to the blocking window, or all windows?
   //       Rightnow, we only use one window, so this is academic...
   if (mgr->get_blocker() == GLUT_WINSYS::window()) return;

#ifdef USE_GLUT_WACOM
   err_adv(debug, "mouse_motion_callback");
   // If the tablet is in use we ignore these duplicate
   // "mouse" events that really come from the tablet.
   // (the tablet callbacks are generated too.)
   if (stylus_down || stylus2_down || eraser_down)
      return;
#endif

   GLUT_MOUSE *mouse = GLUT_MOUSE::mouse();
   int w, h;
   mouse->winsys()->size(w, h);
   CXYpt curs(PIXEL(x, (double) h - y));

   // Unfortunately, can't get modifiers in a motion event in glut.
   // So, can't have a shift-move event, for example.
   mouse->pointer().set_pressure(1.0);
   mouse->pointer().event(curs, Evd::NONE);

   // mouse cursor motions turns off window's additional cursor (e.g.,
   // the special cursor associated with the tablet)
   mouse->winsys()->show_special_cursor(false);
}

extern "C"
void 
HACK_mouse_right_button_up()
{
#ifdef USE_GLUT_WACOM
   err_adv(debug, "HACK_mouse_right_button_up called");
#endif
   GLUT_MOUSE::mouse()->buttons().event(
      GLUT_RIGHT_BUTTON,  DEVice_buttons::UP, Evd::NONE
      );
}

extern "C"
void 
mouse_button_callback(
   int button,
   int state,
   int /* x */, 
   int /* y */
   )
{
   GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);
   // XXX - Just block events to the blocking window, or all windows?
   //       Rightnow, we only use one window, so this is academic...
   if (mgr->get_blocker() == GLUT_WINSYS::window()) return;

#ifdef USE_GLUT_WACOM
   // Tablet events get reported redundantly as mouse events too.
   // First they are reported as tablet events, then again as mouse events.
   // On a "down" event for the tablet, the variable 'stylus_down' is
   // set to true. On a mouse up event, we set it back to false.
   // In any case we ignore mouse events that are actually tablet events.
   if (stylus_down) {
      if (state == GLUT_UP)
         stylus_down = false;
      return;
   }
   if (stylus2_down) {
      if (state == GLUT_UP)
         stylus2_down = false;
      return;
   }
   if (eraser_down) {
      if (state == GLUT_UP)
         eraser_down = false;
      return;
   }
#endif

   err_adv(Config::get_var_bool("DEBUG_MOUSE_BUTTON_CALLBACK",false,true),
      "button: %d, %s",
      button,
      (state == GLUT_DOWN) ? "down" : "up"
      );

   GLUT_MOUSE *mouse = GLUT_MOUSE::mouse();

   const int glut_mods = glutGetModifiers();

   Evd::DEVmod mods = Evd::NONE;

   switch (glut_mods) {
    case GLUT_ACTIVE_SHIFT:
      mods = Evd::SHIFT;
      break;
    case GLUT_ACTIVE_CTRL:
      mods = Evd::CONTROL;
      break;
    default:
      ;
   }

   mouse->buttons().event(
      button,
      (state == GLUT_DOWN) ? DEVice_buttons::DOWN : DEVice_buttons::UP,
      mods);
}


GLUT_MOUSE::GLUT_MOUSE(GLUT_WINSYS *winsys) :
   _winsys(winsys)
{
   while (_mice.num() <= winsys->id()) {
      // XXX - Must cast or += ARRAY is used,
      // which makes this an infinite loop
      _mice += (GLUT_MOUSE *) 0;
   }
   _mice[winsys->id()] = this;

#ifdef USE_GLUT_WACOM
   if (glutDeviceGet(GLUT_HAS_WACOM_TABLET)) {
      glutInitWacom();
      glutWacomButtonFunc(wacom_button_callback);
      glutWacomMotionFunc(wacom_motion_callback);
      err_adv(debug, "Initialized glutwacom");
      // return;
   } else {
      err_msg("GLUT_MOUSE::GLUT_MOUSE: Error: Tablet device not found");
   }
#endif

   // Reach here if not using the tablet via the windowing system

   glutMouseFunc(mouse_button_callback);         // Button events
   glutMotionFunc(mouse_motion_callback);        // Motion between button events
   glutPassiveMotionFunc(mouse_motion_callback); // Other motion
}

GLUT_MOUSE::~GLUT_MOUSE()
{
   cerr << "~GLUT_MOUSE" << endl;
   _mice[_winsys->id()] = 0;
}

GLUT_MOUSE *
GLUT_MOUSE::mouse()
{
   return _mice[glutGetWindow()];
}

void 
GLUT_CURSpush::push( 
   CXYpt &pt
   ) 
{
   if (_win) {
      _win->push_cursor(pt);
   }
}


void 
GLUT_CURSpush::handle_event( 
   CEvd &e 
   ) 
{
  DEVice_2d *ice = (DEVice_2d *)e._d;
  push(ice->cur());
}
