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
/*****************************************************************
 * draw_widget.C
 *****************************************************************/

#include "geom/world.H"         // for WORLD::create() etc.

#include "gest/mode_name.H"

#include "draw_widget.H"

/*****************************************************************
 * DrawWidget
 *****************************************************************/

DrawWidgetptr DrawWidget::_active;
DrawWidget*   DrawWidget::null = 0;

DrawWidget::DrawWidget(double dur) :
   GEOM(static_name()),
   _fsa(&_draw_start),
   _timer(dur)
{
   // Must set NO_NETWORK before calling WORLD::create()
   // because of observers that are called in
   // WORLD::create()
   REFlock me(this);
   NETWORK.set(this, 0);
   // XXX -- are these lines needed?
   // i.e., not sure if ref counted pointer gets deleted
   // if object is not in exist list
   //WORLD::create(this);
   //WORLD::undisplay(this, false);

   // Turn on observations of when we are displayed/undisplayed
   disp_obs(this);
}

DrawWidget::~DrawWidget() 
{
   deactivate();
   unobs_display(this); 
}

void
DrawWidget::activate()
{
   if (_active && _active != this)
      _active->deactivate();
   _active=this;

   if (has_mode_name())
      ModeName::push_name(mode_name());

   reset_timeout();

   // Sign up for CAMobs callbacks:
   VIEW::peek_cam()->data()->add_cb(this);
   
   // If not currently displayed, get displayed:
   if (!DRAWN.contains(this))
      WORLD::display(this, false); // XXX -- should be undoable???
}

void
DrawWidget::deactivate()
{
   if (_active != this)
      return;
   _active=0;

   if (has_mode_name())
      ModeName::pop_name();

   // Un-sign up for callbacks:
   // (so we don't get callbacks when we are inactive)
   VIEW::peek_cam()->data()->rem_cb(this);

   // If currently displayed, stop:
   if (DRAWN.contains(this))
      WORLD::undisplay(this, false);

   // Let subclasses clear cached info at this time:
   reset();
}

void
DrawWidget::toggle_active() 
{
   // Do it:
   is_active() ? deactivate() : activate();
}

void 
DrawWidget::notify(CGELptr &g, int is_displayed)
{
   if (!g) {
      err_msg("DrawWidget::notify: error: GEL is NULL");
      return;
   }

   // The only cases we care about are when object is undisplayed:
   if (is_displayed)
      return;

   // Deactivate in each case:
   //  - the mesh associated with this widget was undisplayed
   //  - the widget itself was undisplayed
   if ((bmesh() && bmesh()->geom() == g) || 
       (g == this && !is_displayed)) {
      deactivate();
   }
}

void 
DrawWidget::notify(CCAMdataptr&) 
{
   // Stay hungry.
   // (Don't timeout while the user changes the camera).
   reset_timeout();
}

// end of file draw_widget.C
