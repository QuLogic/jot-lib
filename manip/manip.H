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
#ifndef MANIP_H_IS_INCLUDED
#define MANIP_H_IS_INCLUDED

#include "std/support.H"
#include "dev/dev.H"
#include "disp/view.H"
#include "disp/cam.H"
#include "geom/geom.H" // Defines State as State_t <- Consider moving this?
#include "geom/fsa.H" // for Interactor<>, State_t

class UPobs {
   public :
    virtual ~UPobs() {}
    virtual void reset(int is_reset) = 0;
};

class Key_int : public Interactor<Key_int,Event,State> {
  protected: 
   State   _key_down;
   
  public:
   virtual int down(CEvent &,State *&)  { return 0; }
   virtual int up  (CEvent &,State *&)  { return 0; }

   Key_int(char k,  State *start)       { add_event(k, start); }
   Key_int(char k[],State *start)       { add_event(k, start); }
   Key_int()                            { _entry.set_name("Key_int entry"); }

   void add_event(char k[], State *start);
   void add_event(char k,   State *start);
};

class Simple_int : public Interactor<Simple_int,Event,State> {
  protected: 
   State  _manip_move;
   CAMptr _cam;
  public:
   virtual int  down(CEvent &,State *&)  { cerr << "Manip Down\n"; return 0; }
   virtual int  move(CEvent &,State *&)  { cerr << "Manip Move\n"; return 0; }
   virtual int  up  (CEvent &,State *&)  { cerr << "Manip Up\n"  ; return 0; }
   virtual int  noop(CEvent &,State *&)  { return 0; }

   virtual void add_events(CEvent &d, CEvent &m, CEvent &u);

   virtual     ~Simple_int() { }
                Simple_int(CEvent &d, CEvent &m, CEvent &u);
};


class FilmTrans : public Simple_int {
  protected:
   VIEWptr    _view;            // we
   GEOMptr    _obj;             //  don't
   mlib::Wpt  _down_pt;         //   need
   mlib::Wvec _down_norm;       //    no
   bool       _no_xform;        //     stinkin'
   bool       _call_xform_obs;  //      comments

  public:
   FilmTrans(CEvent &, CEvent &, CEvent &);

   //******** Simple_int METHODS ********
   virtual int down(CEvent &e, State *&s);
   virtual int move(CEvent &e, State *&s);
   virtual int up  (CEvent &e, State *&s);
};


extern void scale_along_normal(
   CGEOMptr  &obj,
   mlib::CWpt      &scale_cent,
   mlib::CWpt      &down_pt,
   mlib::CWvec     &down_norm,
   mlib::CXYpt     &cur
   );

#endif // MANIP_H_IS_INCLUDED
