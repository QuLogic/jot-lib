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
#ifndef _COORD_FRAME_H_IS_INCLUDED_
#define _COORD_FRAME_H_IS_INCLUDED_

#include "mlib/points.H"

/*****************************************************************
 * CoordFrame:
 *
 *    Abstract base class defining a right-handed coordinate
 *    frame with origin o and orthonormal vectors t, b, and n.
 *    The names suggest "tangent," "binormal," and "normal," as
 *    in a Frenet frame, though you can also think of them as
 *    generic x, y and z directions.
 *
 *                         n                                   
 *                         |                                   
 *                         |                                   
 *                         |                                   
 *                         o - - - b                          
 *                        /                                    
 *                       /                                     
 *                      t
 *
 *****************************************************************/
class CoordFrame;

/************************************************************
 * CoordFrameObs
 *
 *    "Observer" that gets notification if something
 *    happened to the CoordFrame.
 *
 ************************************************************/
class CoordFrameObs {
 public:
   virtual ~CoordFrameObs() {}
   virtual void notify_frame_deleted(CoordFrame*) = 0;
   virtual void notify_frame_changed(CoordFrame*) = 0;
};

/************************************************************
 * CoordFrameObs_list
 ************************************************************/
class CoordFrameObs_list : public ARRAY<CoordFrameObs*> {
 public:
   // convenience methods:
   void notify_frame_changed(CoordFrame* f) {
      for (int k=0; k<_num; k++)
         _array[k]->notify_frame_changed(f);
   }
   void notify_frame_deleted(CoordFrame* f) {
      for (int k=0; k<_num; k++)
         _array[k]->notify_frame_deleted(f);
   }
};

/*****************************************************************
 * CoordFrame: (see diagram and comment above)
 *****************************************************************/
class CoordFrame {
 public:
   virtual ~CoordFrame() { _observers.notify_frame_deleted(this); }

   //******** CoordFrame VIRTUAL METHODS ********

   virtual mlib::Wpt      o() = 0;
   virtual mlib::Wvec     t() = 0;    // t() and n() should be unit-length
   virtual mlib::Wvec     n() = 0;    // and perpendicular to each other

   // By default, the following are defined in terms of o, t, and n:
   virtual mlib::Wvec      b() { return cross(n(), t()).normalized(); }
   virtual mlib::Wtransf  xf() { return mlib::Wtransf(o(), t(), b(), n()); }
   virtual mlib::Wtransf inv() { return xf().inverse(); }

   //******** OBSERVERS ********

   void add_obs(CoordFrameObs* o) { _observers.add_uniquely(o); }
   void rem_obs(CoordFrameObs* o) { _observers -= o; }

   virtual void changed() { _observers.notify_frame_changed(this); }

 protected:
   CoordFrameObs_list _observers;     // observer list
};

#endif // _COORD_FRAME_H_IS_INCLUDED_

// end of file coord_frame.H
