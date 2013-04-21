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
#ifndef FRAME_TIME_OBSERVER_H_IS_INCLUDED
#define FRAME_TIME_OBSERVER_H_IS_INCLUDED

#include "disp/view.H"

/*****************************************************************
 * FRAME_TIME_OBSERVER:
 *
 *    Base class for objects that get notifications when the frame
 *    time variable stored in a VIEW changes.
 *****************************************************************/
class FRAME_TIME_OBSERVER {
 public :
   virtual ~FRAME_TIME_OBSERVER() {}

   // called when the frame time has changed:
   virtual void  frame_time_changed() = 0;

   // sign up to track changes in frame time for the given view:
   void observe_frame_time(CVIEWptr& view = VIEW::peek()) {
      // if observing some other view, sign off first:
      unobserve_frame_time();
      // now record the new view:
      _view = view;
      if (_view)
         _view->add_frame_time_observer(this);
   }
   // sign off from observing changes in frame time for the given view:
   void unobserve_frame_time() {
      if (_view)
         _view->remove_frame_time_observer(this);
      _view = 0;
   }

 protected:
   VIEWptr      _view;
};

#endif // FRAME_TIME_OBSERVER_H_IS_INCLUDED

// end of file frame_time_observer.H
