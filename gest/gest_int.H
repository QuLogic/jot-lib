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
/**********************************************************************
 * gest_int.H
 **********************************************************************/
#ifndef GEST_INT_H_HAS_BEEN_INCLUDED
#define GEST_INT_H_HAS_BEEN_INCLUDED

#include "geom/fsa.H"
#include "manip/manip.H"
#include "std/stop_watch.H"

#include "gesture.H"

// gesture observer class (defined below)
class GestObs;

/*****************************************************************
 * GestObs_list:
 *****************************************************************/
class GestObs_list : public ARRAY<GestObs*> {
 public:
   GestObs_list(int n=0)                  : ARRAY<GestObs*>(n)    {}
   GestObs_list(const GestObs_list& list) : ARRAY<GestObs*>(list) {}
   
   //******** CONVENIENCE ********

   // apply various GestObs methods to each observer in the list
   // (the following are defined at the end of this file):
   void notify_gesture(GEST_INT* gi, CGESTUREptr& g);
   int  handle_event(CEvent&, State* &);
   void notify_down();
   void notify_move();
   void notify_up  ();
};

/*****************************************************************
 * GESTURE_list:
 *****************************************************************/
class GESTURE_list : public LIST<GESTUREptr> {
 public:
   //******** MANAGERS ********
   GESTURE_list(int n=0) : LIST<GESTUREptr>(n) { begin_index(); }
   GESTURE_list(const GESTURE_list& list) : LIST<GESTUREptr>(list) {
      begin_index();
   }

   virtual ~GESTURE_list() { end_index(); }

   //******** ARRAY INDEXING ********
   virtual void set_index(CGESTUREptr& g, int i) const {
      // XXX -
      //  Should confirm g belongs to this list?
      //  Yes but, for now, assume GESTURE_list 
      //    is only used by GEST_INT.
      //
      if (g)
         ((GESTUREptr&)g)->set_index(i);
   }
   virtual void clear_index(CGESTUREptr& g) const { set_index(g, BAD_IND); }
   virtual int  get_index  (CGESTUREptr& g) const {
      return g ? ((GESTUREptr&)g)->index() : BAD_IND;
   }
};

/*****************************************************************
 * GEST_INT
 *
 *      Interactor for drawing stroke-based gestures.
 *
 *      Handles down/move/up events to record gestures, then hands off
 *      the gestures to "observers," i.e. Pens that trigger
 *      appropriate callbacks.
 *****************************************************************/
MAKE_PTR_SUBC(GEST_INT,FRAMEobs);
typedef const GEST_INTptr CGEST_INTptr;
class GEST_INT : public Simple_int, public FRAMEobs, public CAMobs {
 protected:
   VIEWptr              _view;          // view
   GestObs_list         _obs;           // observers of gestures
   GESTURE_list         _stack;         // recent gestures
   GestureDrawer*       _drawer;        // for drawing gestures
   FSA                  _fsa;           // the fsa for this interactor
   State                _b2_pressed;

   void activate();
   void deactivate();

 public:
   //******** MANAGERS ********
   GEST_INT(CVIEWptr& view,
            CEvent &d, CEvent &m, CEvent &u,
            CEvent &d2, CEvent &u2);
   virtual ~GEST_INT();

   // add/remove observers
   // (e.g. called in Pen::activate/deactivate):
   void rem_obs(GestObs* g);
   void add_obs(GestObs* g);

   // restore FSA to initial state:
   void reset() { _fsa.reset(); }

   // set the thing that draws Gestures:
   void set_drawer(GestureDrawer* drawer)     { _drawer = drawer; }
   GestureDrawer* drawer()                    { return _drawer; }

   // gesture accessors
   // (get gesture number k out of the entire gesture history):
   GESTUREptr gesture(int k) const {
      return _stack.valid_index(k) ? _stack[k] : GESTUREptr(0);
   }
   GESTUREptr last() const { return gesture(_stack.num()-1); }

   virtual int null_cb(CEvent&, State*&) { return 0; }

   //******** Simple_int methods ********
   virtual int down(CEvent &,State *&);
   virtual int move(CEvent &,State *&);
   virtual int up  (CEvent &,State *&);

   //******** CAMobs Method *************
   virtual void notify(CCAMdataptr &data);

   //******** FRAMEobs methods ********
   virtual int  tick();
};

/*****************************************************************
 * GestObs:
 *
 *      Abstract base class for "gesture observers" that receive
 *      gestures from the gesture interactor.
 *****************************************************************/
class GestObs {
 public:
   //******** MANAGERS ********
   virtual ~GestObs() {}

   // get notification of the latest gesture
   virtual void notify_gesture(GEST_INT* gest_int, CGESTUREptr& gest) = 0;

   // take over the given down/move/up event from the GEST_INT:
   // can be used to invoke a menu, e.g., depending on context.
   virtual int handle_event(CEvent&, State* &) { return 0; }

   virtual void notify_down() {}
   virtual void notify_move() {}
   virtual void notify_up  () {}
};

inline void 
GestObs_list::notify_gesture(GEST_INT* gi, CGESTUREptr& g)
{
   for (int i=0; i<num(); i++) {
      (*this)[i]->notify_gesture(gi,g);
   }
}

inline int
GestObs_list::handle_event(CEvent& e, State*& s)
{
   // first one to volunteer gets it:
   for (int i=0; i<num(); i++) {
      if ((*this)[i]->handle_event(e,s))
         return 1;
   }
   return 0;
}

inline void 
GestObs_list::notify_down()
{
   for (int i=0; i<num(); i++) {
      (*this)[i]->notify_down();
   }
}

inline void 
GestObs_list::notify_move()
{
   for (int i=0; i<num(); i++) {
      (*this)[i]->notify_move();
   }
}

inline void 
GestObs_list::notify_up()
{
   for (int i=0; i<num(); i++) {
      (*this)[i]->notify_up();
   }
}

#endif  // GEST_INT_H_HAS_BEEN_INCLUDED

// end of file gest_int.H
