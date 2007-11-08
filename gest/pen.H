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
#ifndef PEN_H_IS_INCLUDED
#define PEN_H_IS_INCLUDED

/*!
 *  \file pen.H
 *  \brief Contains the definition of the Pen class.
 *
 *  \sa pen.C
 *
 */

#include "disp/view.H"
#include "geom/world.H"
#include "manip/manip.H"
#include "mesh/bface.H"
#include "std/stop_watch.H"

#include "gest/gest_int.H"
#include "gest/gest_guards.H"
#include "gest/mode_name.H"

using namespace mlib;
/******************************************************************
 * Pen:
 ******************************************************************/
class Pen : public Simple_int, public GestObs {
 protected:
   
   // ******** MEMBERS ********
   GEST_INTptr  _gest_int;
   State*       _shift_fsa;
   State*       _ctrl_fsa;
   str_ptr      _pen_name;
   VIEWptr      _view;
   DrawState    _draw_start;
   DrawFSA      _fsa;

   // ******** INTERNAL METHODS ********
   typedef int (Pen::*callback_meth_t)(CEvent&, State *&);
   State  *create_fsa(CEvent &d, CEvent &m, CEvent &u,
                      callback_meth_t down, callback_meth_t move,
                      callback_meth_t up);
   int     check_interactive(CEvent &e, State *&s);

 public:
   //******** MANAGERS ********
   Pen(Cstr_ptr& pen_name,
       CGEST_INTptr &gest_int = GEST_INTptr(),
       CEvent &d = Event(), CEvent &m = Event(), CEvent &u = Event(),
       CEvent &shift_d = Event(), CEvent &shift_u  = Event(),
       CEvent &ctrl_d  = Event(),  CEvent &ctrl_u  = Event());

   // ******** CONVENIENCE ********
   str_ptr pen_name()        const { return _pen_name; }
   XYpt get_ptr_position ()  const {
      return DEVice_2d::last ? DEVice_2d::last->cur() : XYpt();
   }
   XYpt get_last_position()  const {
      return DEVice_2d::last ? DEVice_2d::last->old() : XYpt();
   }

   VIEWptr view() const { return _view; }

   // ******** PEN ACTIVATION ********
   virtual void activate(State *);
   //A false return prohibits a pen change
   //Some WORLD msg should say how to alleviate the challenge
   virtual bool deactivate(State *);

   // ******** GESTURE RECOGNITION ********
   virtual void notify_gesture(GEST_INT*, CGESTUREptr&);

   // ******** KEY PRESSES ********
   virtual void key(CEvent &) { return; }

   // ******** ERASER METHODS ********
   virtual int erase_down  (CEvent &,State *&);
   virtual int erase_move  (CEvent &,State *&);
   virtual int erase_up    (CEvent &,State *&);

   // ******** CONTROL-KEY METHODS ********
   virtual int ctrl_down(CEvent &,State *&)  {return 0;}
   virtual int ctrl_move(CEvent &,State *&)  {return 0;}
   virtual int ctrl_up  (CEvent &,State *&)  {return 0;}   

   // ******** Simple_int METHODS ********
   virtual int down(CEvent &,State *&);
   virtual int move(CEvent &,State *&);
   virtual int up  (CEvent &,State *&);
};

#endif  // PEN_H_IS_INCLUDED

/* end of file pen.H */
