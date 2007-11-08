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

#ifndef MMENU_H
#define MMENU_H

#include "disp/gel.H"
#include "disp/view.H"
#include "geom/geom.H"
#include "geom/text2d.H"
#include "manip/manip.H"


//------------------------------------------------------
//
//  MenuCb_t - 
//     is a generic callback Interface for testing a
//  guard condition for an FSA arc.
//
//------------------------------------------------------
class MenuCb_t {
  public: 
               MenuCb_t()       { }
      virtual ~MenuCb_t()       { }
   virtual void exec()           = 0;
};

//------------------------------------------------------
//
//  MenuCbFunc_t - 
//
//     test EVENTs with a given function
//
//------------------------------------------------------
class MenuCbFunc_t: public MenuCb_t {
 public:
   typedef void (*func_t)();

 protected:
   func_t  _func;

 public: 
       MenuCbFunc_t(func_t f):_func(f) {}
   virtual void exec()      { (*_func)(); }
};

//------------------------------------------------------
//
//  MenuCbMeth_t - 
//     
//  test EVENTs with a given method on an object
//
//------------------------------------------------------
template <class T>
class MenuCbMeth_t: public MenuCb_t {
  public:
    typedef void (T::*_method)();

  protected:
    T      *_obj;
    _method _meth;

  public: 
   MenuCbMeth_t(T *o, _method m) : _obj(o),_meth(m) {}
   virtual void exec()      { (_obj->*_meth)(); }
};



MAKE_PTR_SUBC(MMENU, FRAMEobs);
class MMENU : public FRAMEobs, public Simple_int
{
 public:
  enum DIR { N=0,NE,E,SE,S,SW,W,NW,NUM_DIRS };

 protected:
     struct MMENUMenuItem {
         MenuCb_t *_f;
         GEOMptr _g;
         MMENUMenuItem(){
             _g = NULL; _f = NULL;
         }
         MMENUMenuItem(CGEOMptr g, MenuCb_t *f){
             _g = g; _f = f;
         }

         bool operator==(const MMENUMenuItem &m) const {
             return _f==m._f && _g==m._g;
         }
     };

  ARRAY<MMENUMenuItem> _items;
 public:
  void add(CGEOMptr &g, MenuCb_t *o=0) {
      _items.add(MMENUMenuItem(g, o));
  }
  void add(Cstr_ptr &n, MenuCb_t *o=0){
      add(new TEXT2D("",n), o);
  }
  void clear(){
     _items.clear();
  }

 protected:
  int _sel;

/* cardinal direction implementation
  MenuCb_t        *_funcs[NUM_DIRS];
  GEOMptr          _geoms[NUM_DIRS];
  DIR              _sel;
*/

  VIEWptr          _view;
  double           _t0;
  mlib::XYpt       _d;
  int              _disp;

  CCOLOR           _nhighlight_col;
  CCOLOR           _highlight_col;

 public:
   virtual ~MMENU() {}
   MMENU(CEvent &d, CEvent  &m, CEvent  &u) :
      Simple_int(d,m,u),
      _sel(-1),
      _t0(0),
      _disp(0),
      _nhighlight_col(.2,.2,0.7), 
      _highlight_col(1,0,0) {}

   virtual void  call(int sel);
   virtual int   invoke ( CEvent &, State *&);
   virtual int   down   ( CEvent &, State *&);
   virtual int   move   ( CEvent &, State *&);
   virtual int   up     ( CEvent &, State *&);
   virtual int   tick   ();
};

#endif
