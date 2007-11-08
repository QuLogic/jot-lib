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
#include "geom/world.H"
#include "gtex/buffer_ref_image.H"
#include "std/config.H"

#include "gest_int.H"

using mlib::PIXEL;

static bool debug = Config::get_var_bool("DEBUG_GEST_INT",false);

GEST_INT::GEST_INT(
   CVIEWptr& view,
   CEvent &d, CEvent &m, CEvent &u,
   CEvent &d2, CEvent &u2) :
   Simple_int(d,m,u),
   _view(view),
   _obs(0),
   _stack(16),
   _drawer(new GestureDrawer()),
   _fsa(&_entry)
{
   // turn on indexing:
   _stack.begin_index();

   // for safety, no observers can get listed twice:
   _obs.set_unique();

   // If button 2 is pressed, jump to a useless state until button 2
   // is released. That way if button 1 gets pressed *while* button 2
   // is already pressed, no gesture is processed. This is useful,
   // because with the Wacom stylus, it's natural to press button 2
   // and also put the stylus to the tablet (which presses button 1).

   // This is a subclass of Simple_int, so the callback paramater to
   // Cb must be a CallMeth_t<Simple_int,Event>::_method), so we do a
   // cast:
   _entry +=
      Arc(d2, Cb((_callb::_method) &GEST_INT::null_cb, &_b2_pressed));
   _b2_pressed +=
      Arc(u2, Cb((_callb::_method) &GEST_INT::null_cb, (State *)-1));

   // Sign up for CAMobs callbacks:
   _view->cam()->data()->add_cb(this);
}

GEST_INT::~GEST_INT() 
{ 
}

void 
GEST_INT::rem_obs(GestObs* g) 
{
   _obs -= g;
   if (_obs.empty())
      deactivate();
}

void 
GEST_INT::add_obs(GestObs* g)
{
   _obs += g;
   activate();
}

void
GEST_INT::activate()
{
   // get tick callbacks:
   _view->schedule(this);

   // get down/motion/up events:
   VIEWint_list::get(_view)->add_interactor(&_fsa);
}

void
GEST_INT::deactivate()
{
   // tick callbacks:
   _view->unschedule(this);

   // down/motion/up events:
   VIEWint_list::get(_view)->rem_interactor(&_fsa);
}

int
GEST_INT::down(CEvent &e, State *&s)
{
   // see if any of them observers want it:
   if (_obs.handle_event(e,s))
      return 0;
   
   // as usual, none did. but just check whether there are any
   // interactive widgets begging for it:
   DEVice_buttons *btns=(DEVice_buttons *)e._d;
   DEVice_2d      *ptr=btns->ptr2d();
   RAYhit r(ptr->cur());

   // XXX - fix for mac os x issue on tablet:
   ptr->set_old(ptr->cur());

   // here they go:
   if (e.view()->intersect(r).success() &&
       ray_geom(r,GEOM::null) &&
       ray_geom(r,GEOM::null)->interactive(e,s))
      return 0;

   // as usual, it's up to us to handle this gesture ourselves!

   // start a new one and make sure it's drawn
   _stack += new GESTURE(
      this, _stack.num(), ptr->cur(), ptr->pressure(), _drawer, e
      );

   BufferRefImage *buf = BufferRefImage::lookup(_view);
   if (buf && (buf->is_observing())) {
      buf->add(_stack.last());
   } else {
      WORLD::display(_stack.last(), false);
   }

   _obs.notify_down();

   return 0;
}

int
GEST_INT::move(CEvent &, State *&)
{
   // mac version can get an up without a down, so this can be 0
   if (!DEVice_2d::last)
      return 0;

   DEVice_2d* ptr = DEVice_2d::last;

   double MIN_DIST = Config::get_var_dbl("GEST_ADD_MIN_DIST", 2.5, true);
   
   // XXX - If the camera is moving, then the GESTURE might
   // have started in a previous frame, and now be gone from
   // da stack.  Just punt this event if so...

   if ( !_stack.empty() ) {
      _stack.last()->add(
         PIXEL(ptr->cur()),        // location
         MIN_DIST,                 // threshold distance for accepting
         ptr->pressure()           // pressure
         );
      _obs.notify_move();
   } else {
      err_adv(debug, "GEST_INT::move: tossing event for missing gesture.");
   }

   return 0;
}

int
GEST_INT::up(CEvent &e, State *&)
{
   // mac version can get an up without a down, so this can be 0
   if (!DEVice_2d::last)
      return 0;

   if (_stack.empty()) {
      // XXX - If the camera is moving, then the GESTURE might
      // have started in a previous frame, and now be gone from
      // da stack.  Just punt this event if so...
      err_adv(debug, "GEST_INT::move: tossing event for missing gesture.");
   } else {
      _stack.last()->complete(DEVice_2d::last->cur(), e);

      // Remove from BufferRefImage list if it's observing, else remove
      // from the WORLD's list...
        
      BufferRefImage *buf = BufferRefImage::lookup(_view);
      if (buf && buf->is_observing())
         buf->rem(_stack.last());
      else
         WORLD::undisplay(_stack.last(), false);

      _obs.notify_up();
      _obs.notify_gesture(this, _stack.last());
   }

   return 0;
}

void
GEST_INT::notify(CCAMdataptr&)
{
   // CAMobs virtual method: notification that camera changed.

   // Undisplay any dangling GESTURE

   if (!(_stack.empty() || _stack.last()->complete())) 
   {
      err_adv(debug, "GEST_INT::move: undisplaying 'dangling' gesture.");
      BufferRefImage *buf = BufferRefImage::lookup(_view);
      if (buf && buf->is_observing())
         buf->rem(_stack.last());
      else
         WORLD::undisplay(_stack.last(), false);
   }

   // Forget previous gestures since they go with the old camera:
    _stack.clear();
}

int
GEST_INT::tick()
{
   // look for timeouts etc. here

   return 1;
}

/* end of file gest_int.C */
