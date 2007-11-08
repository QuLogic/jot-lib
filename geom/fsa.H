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
#ifndef FSA_H_IS_INCLUDED
#define FSA_H_IS_INCLUDED

/*!
 *  \file fsa.H
 *  \brief Contains the definition of FSA (finite state automata) classes.
 *
 *  \note Some other classes are in here as well, but they should probably be
 *  moved elsewhere.
 *
 *  \sa fsa.C
 *
 */

#include "std/support.H"
#include "disp/view.H"
#include "dev/dev.H"

template <class EVENT>
class State_t;

/*!
 *  \brief A generic callback Interface for testing a guard condition for an
 *  FSA arc.
 *
 */
template <class EVENT>
class Guard_t {
      
   public:
   
      Guard_t()                      { }
      Guard_t(const EVENT &e):_e(e)  { }
      virtual ~Guard_t()             { }
      
      virtual int exec(const EVENT &e) { return (e == _e); }
      virtual int none() const         { return 1; }
      const EVENT& event() const       { return _e; }
   
   private:
   
      EVENT _e;

};

/*!
 *  \brief Test EVENTs with a given function.
 *
 */
template <class EVENT>
class GuardFunc_t: public Guard_t<EVENT> {
   
   public:
   
      typedef bool (*func_t)(const EVENT &);
   
      GuardFunc_t(const EVENT &e, func_t f) : Guard_t<EVENT>(e), _func(f) {}
      
      virtual int exec(const EVENT &e) { return Guard_t<EVENT>::exec(e) &&
                                                            (*_func)(e); }
      virtual int none()  const        { return 0; }
      
   private:
   
      func_t _func;

};

/*!
 *  \brief Test EVENTs with a given method on an object.
 *
 */
template <class T, class EVENT>
class GuardMeth_t: public Guard_t<EVENT> {
   
   public:
   
      typedef bool (T::*_method)(const EVENT &);
   
      GuardMeth_t(const EVENT &e, T *o, _method m)
         : Guard_t<EVENT>(e),_obj(o),_meth(m) {}
      
      virtual int exec(const EVENT &e)
         { return Guard_t<EVENT>::exec(e) && (_obj->*_meth)(e); }
      
      virtual int none()  const        { return 0; }
   
   private:
   
       T      *_obj;
       _method _meth;

};

/*!
 *  \brief A generic callback Interface for whenever an Arc_t of an FSA is
 *  traversed.
 *  
 *  The callBack function is passed the Event which triggered the Arc and the
 *  default next State of the FSA.  The callBack can change the FSA's next
 *  state by assigning a different value.   This class is templated so that it
 *  can be used with any Event type that the application wants.
 *
 */
template <class EVENT>
class CallBack_t {
    
   public:
   
      CallBack_t():_next(0)                  { }
      CallBack_t(State_t<EVENT> *n):_next(n) { }
      virtual ~CallBack_t() {}
      
      virtual int exec(const EVENT &, State_t<EVENT> *&, State_t<EVENT> *) = 0;
      
      const State_t<EVENT> *next() const { return _next; }
   
   protected:
   
      State_t<EVENT>  *_next;

};

/*!
 *  \brief A subclass of a CallBack that contains a function to call.
 *
 */
template <class EVENT>
class CallFunc_t: public CallBack_t<EVENT> {
   
   public: 
   
      typedef int (*_functor)(const EVENT &, State_t<EVENT> *&);
    
      CallFunc_t(_functor f)
         : _func(f) { }
      CallFunc_t(_functor f, State_t<EVENT> *n)
         : CallBack_t<EVENT>(n),_func(f) { }
      
      int exec(const EVENT &e, State_t<EVENT> *&s, State_t<EVENT> *start)
         { assert(_func); if (_next == (State_t<EVENT> *)-1) s = start;
           if (_next) s = _next; int ret = _func(e,s); 
           if (s == (State_t<EVENT> *)-1) s = start; return ret; }
   
   protected: 
   
      _functor  _func;
      using CallBack_t<EVENT>::_next;

};

/*!
 *  \brief A subclass of a CallBack that contains an object and one if its
 *  methods to call.
 *
 */
template <class T, class EVENT>
class CallMeth_t : public CallBack_t<EVENT> {
   
   public:
   
      typedef int (T::*_method)(const EVENT &, State_t<EVENT> *&);

      CallMeth_t(T *obj, _method meth): _obj(obj), _meth(meth){ }
      CallMeth_t(T *obj, _method meth, State_t<EVENT> *n): 
                  CallBack_t<EVENT>(n), _obj(obj), _meth(meth){ }

      int exec(const EVENT &e, State_t<EVENT> *&s, State_t<EVENT> *start)
         {
// VC++ 5.0 can't deal with the following line of code
#ifndef WIN32
            assert(_obj && _meth); 
#endif
            if (_next) s = _next; int ret = (_obj->*_meth)(e,s);
            if (s == (State_t<EVENT> *)-1) s = start; return ret; }

      int equals(_method meth) {return meth == _meth;}
      _method get_method() {return _meth;}
   
   protected:
   
       T       *_obj;
       _method  _meth;

      using CallBack_t<EVENT>::_next;

};

/*!
 *  \brief Represents an Arc of an FSA.
 *
 *  An Arc is traversed when its specific Event is generated.  Traversing an
 *  Arc results in the Callback associated with the Arc being executed.  The
 *  next state of the FSA is returned by the callback.
 *
 */
template <class EVENT>
class Arc_t {
   
   public: 
 
      Arc_t():_c(0), _guard(new Guard_t<EVENT>()) { }
      Arc_t(const EVENT &e, CallBack_t<EVENT> *c):_c(c),
                                                _guard(new Guard_t<EVENT>(e)){ }
      Arc_t(Guard_t<EVENT> *g, CallBack_t<EVENT> *c): _c(c),_guard(g)   { }

      int execute (State_t<EVENT> *&s, const EVENT &e, 
                                 State_t<EVENT> *start) const
                                          { return _c->exec(e, s, start);}
      
      int match    (const EVENT  &e)
                                   const  { return _guard->exec(e); }
      int guarded  () const  { return !_guard->none(); }
      
      const Guard_t<EVENT>   * guard    () const  { return _guard; }
      const State_t<EVENT>   * next     () const  { return _c->next(); }
      const EVENT&             event    () const  { return _guard->event(); }
      const CallBack_t<EVENT>* callback () const  { return _c;    }
      CallBack_t<EVENT>* get_callback ()    { return _c;    }
      
      int   operator ==   (const Arc_t<EVENT> &a) const 
         { cerr << "Nooo ... don't compare Arcs" << endl;
           return _c == a._c && _guard == a._guard; }
   
   protected:

      CallBack_t<EVENT> *_c;     // must be a pointer since we allow subclasses
      Guard_t<EVENT>    *_guard; // must be a pointer since we allow subclasses

};

// #define CState_t const State_t

/*!
 *  \brief A State in an FSA.
 *
 *  The State may contain multiple Arcs.  Only one Arc out of a State will be
 *  traversed.  When an Event is passed to a state, each Arc will be matched to
 *  the Event.  The first Arc that matches the Event is traversed.
 *
 */
template <class EVENT>
class State_t {

   public:
   
      typedef Arc_t<EVENT> arc_type;
   
      State_t()                     { }
      State_t(Cstr_ptr &n):_name(n) { }
      
      int is_empty() { return !_arcs.num(); }
      
      void operator+=(const State_t<EVENT> &s);
      void operator-=(const State_t<EVENT> &s);
      
      void operator+=(const Arc_t<EVENT> &a)
         { if (a.guarded()) _arcs.push(a);    // first for priority
           else _arcs += a; }
      void operator -=(const Arc_t<EVENT> &a) { _arcs -= a; }
      
      int consumes(const EVENT   &e) const
         { for(int i = 0; i < _arcs.num(); ++i) if(_arcs[i].match(e)) return 1;
           return 0; }

      //! \brief Process the event, returning the next state. If
      //! requested, return the result of the callback in ret.
      State_t<EVENT> *event(const EVENT& e, State_t<EVENT>* start=0,
                            int* ret=0) const;

      void clear() { _arcs.clear(); }
      
      const ARRAY<arc_type> &arcs() const { return _arcs; }
      ARRAY<arc_type> &arcs()             { return _arcs; }
      
      Cstr_ptr &name() const { return _name; }
      void set_name(Cstr_ptr &n) { _name = n; }
      
      int operator == (const State_t<EVENT> &a) const
         { cerr << "XXX: Dummy State_t::operator== got called\n"; return 0; }

   private:
   
      ARRAY<arc_type> _arcs;
      str_ptr         _name;

};

template <class EVENT>
void
State_t<EVENT>::operator+=(const State_t<EVENT>&s)
{
   
   for(int i = 0; i < s._arcs.num(); ++i){
      
      int j = 0;
      
      for(; j < _arcs.num(); ++j){
         
         if(_arcs[j].match(s._arcs[i].event())){
            
            _arcs[j] = s._arcs[i]; 
            break;
         
         }
         
      }
      
      if(j == _arcs.num()){
         
         if(!s._arcs[i].guarded()) {
            
            _arcs += s._arcs[i];
            
         } else {
            
            // arcs w/guards come first for priority
            // over more general arcs
            _arcs.push(s._arcs[i]); 
            
         }
         
      }
      
   }
   
}

template <class EVENT>
void
State_t<EVENT>::operator-=(const State_t<EVENT>&s)
{

   for(int i = 0; i < s._arcs.num(); i++){
      
      int j = _arcs.num() - 1;
      
      for(; j >= 0; j--){
         
         if(_arcs[j].match(s._arcs[i].event())){
            
            _arcs.remove(j);
            break;
            
         }
         
      }
      
   }
      
}

template <class EVENT>
State_t<EVENT>*
State_t<EVENT>::event(const EVENT& e, State_t<EVENT>* start, int* ret) const
{
   
   State_t<EVENT> *next = (State_t<EVENT>*)this;
   
   for(int i = 0; i < _arcs.num(); i++){
      
      if(_arcs[i].match(e)){
         
         int k = _arcs[i].execute(next, e, start);
         if (ret) *ret = k;
         break;
         
      }
      
   }
   
   return next;

}

/*!
 *  \brief A convenience class that makes it easy for an object that has
 *  CallBack functions to generate the proper CallBack data structure.
 *
 *  \todo Try to remove use of TYPENAME macro if possible.
 *
 */
template <class T, class EVENT, class STATE>
class Interactor {
   
   public:
   
      typedef CallMeth_t<T,EVENT>  _callb;
      typedef GuardMeth_t<T,EVENT> _guard;
   
      virtual ~Interactor() { }
      
      _callb *Cb(TYPENAME _callb::_method m)
         { return new _callb((T*)this,m);  }
         
      _callb *Cb(TYPENAME _callb::_method m,State_t<EVENT> *s)
         { return new _callb((T*)this,m,s);}
                                        
      _guard *Gd(const EVENT &e, TYPENAME _guard::_method m)
         { return new _guard(e,(T*)this,m);}
                                        
      STATE *entry() { return &_entry; }
      const STATE *entry() const { return &_entry; }
   
   protected:
      
      STATE _entry;

};



#ifdef WIN32
#define Arc JOTArc
#endif

class WINSYS;

class Event;
typedef const Event CEvent;

/*!
 *  \brief Wraps up the raw event data with the view that the event occurs in.
 *
 */
class Event : public Evd {

   public:
   
      Event()
         : _view(0) { }
      Event(VIEWptr v, DEVice *d, DEVact a=MOV, DEVmod m=NONE)
         : Evd(d,a,m),_view(v) { }
      Event(DEVice *d, DEVact a=MOV, DEVmod m=NONE)
         : Evd(d,a,m),_view(0) { }
      Event(CEvd &e)
         : _view(0) { *(static_cast<Evd*>(this)) = e; }
      Event(VIEWptr p, CEvd &e)
         : _view(p) { *(static_cast<Evd*>(this)) = e; }
      
      const VIEWptr&  view () const      { return _view; }
      
      Event &operator=(CEvent &e)
         { *(static_cast<Evd*>(this)) = e; _view = e._view; return *this; }
      
   private:
   
      VIEWptr  _view;

};


//--------------------------------------------------------
// These type definitions are here as a convenience.
// Since the generic FSA class doesn't know anything
// about DEV's events, these types don't really belong
// here.  However, DEV doesn't know anything about FSA's
// either, so it doesn't belong there.   Perhaps there
// should be an event.H or something that wraps FSA's and
// DEV events together....
//--------------------------------------------------------
typedef Arc_t<Event>   Arc  ;
typedef State_t<Event> State;
typedef Guard_t<Event> Guard;


/*!
 *  \brief Provides a simple interface to the events that can be generated
 *  by a Mouse object (or an equivalent combination of a 2 DOF device and a
 *  buttons device).
 *
 */
class ButtonMapper 
{
      DEVice_buttons  *_btn;
      DEVice_2d       *_ptr;
   public:
      ButtonMapper():_btn(0), _ptr(0) {}
      ButtonMapper(Mouse *m):_btn(&m->buttons()), _ptr(&m->pointer()){}
      ButtonMapper(DEVice_2d *p, DEVice_buttons *b):_btn(b), _ptr(p){}

      DEVice_buttons  *buttons()                 { return _btn; }
      DEVice_2d       *pointer()                 { return _ptr; }

      void  set_devs(DEVice_2d *p, DEVice_buttons *b) { _btn = b; _ptr = p; }

      Event b1d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B1D,mods);}
      Event b1u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B1U,mods);}
      Event b1dS ()                              { return b1d(Evd::SHIFT); }
      Event b1uS ()                              { return b1u(Evd::SHIFT);}
      Event b1dC ()                              { return b1d(Evd::CONTROL);}
      Event b1uC ()                              { return b1u(Evd::CONTROL);}
      Event b1dM1()                              { return b1d(Evd::MOD1);}
      Event b1uM1()                              { return b1u(Evd::MOD1);}
      Event b1dM2()                              { return b1d(Evd::MOD2);}
      Event b1uM2()                              { return b1u(Evd::MOD2);}
      Event b1dM3()                              { return b1d(Evd::MOD3);}
      Event b1uM3()                              { return b1u(Evd::MOD3);}

      Event b2d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B2D,mods);}
      Event b2u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B2U,mods);}
      Event b2dS ()                              { return b2d(Evd::SHIFT); }
      Event b2uS ()                              { return b2u(Evd::SHIFT);}
      Event b2dC ()                              { return b2d(Evd::CONTROL);}
      Event b2uC ()                              { return b2u(Evd::CONTROL);}
      Event b2dM1()                              { return b2d(Evd::MOD1);}
      Event b2uM1()                              { return b2u(Evd::MOD1);}
      Event b2dM2()                              { return b2d(Evd::MOD2);}
      Event b2uM2()                              { return b2u(Evd::MOD2);}
      Event b2dM3()                              { return b2d(Evd::MOD3);}
      Event b2uM3()                              { return b2u(Evd::MOD3);}

      Event b3d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B3D,mods);}
      Event b3u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B3U,mods);}
      Event b3dS ()                              { return b3d(Evd::SHIFT); }
      Event b3uS ()                              { return b3u(Evd::SHIFT);}
      Event b3dC ()                              { return b3d(Evd::CONTROL);}
      Event b3uC ()                              { return b3u(Evd::CONTROL);}
      Event b3dM1()                              { return b3d(Evd::MOD1);}
      Event b3uM1()                              { return b3u(Evd::MOD1);}
      Event b3dM2()                              { return b3d(Evd::MOD2);}
      Event b3uM2()                              { return b3u(Evd::MOD2);}
      Event b3dM3()                              { return b3d(Evd::MOD3);}
      Event b3uM3()                              { return b3u(Evd::MOD3);}

      Event b4d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B4D,mods);}
      Event b4u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B4U,mods);}
      Event b4dS ()                              { return b4d(Evd::SHIFT); }
      Event b4uS ()                              { return b4u(Evd::SHIFT);}
      Event b4dC ()                              { return b4d(Evd::CONTROL);}
      Event b4uC ()                              { return b4u(Evd::CONTROL);}
      Event b4dM1()                              { return b4d(Evd::MOD1);}
      Event b4uM1()                              { return b4u(Evd::MOD1);}
      Event b4dM2()                              { return b4d(Evd::MOD2);}
      Event b4uM2()                              { return b4u(Evd::MOD2);}
      Event b4dM3()                              { return b4d(Evd::MOD3);}
      Event b4uM3()                              { return b4u(Evd::MOD3);}

      Event b5d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B5D,mods);}
      Event b5u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B5U,mods);}
      Event b5dS ()                              { return b5d(Evd::SHIFT); }
      Event b5uS ()                              { return b5u(Evd::SHIFT);}
      Event b5dC ()                              { return b5d(Evd::CONTROL);}
      Event b5uC ()                              { return b5u(Evd::CONTROL);}
      Event b5dM1()                              { return b5d(Evd::MOD1);}
      Event b5uM1()                              { return b5u(Evd::MOD1);}
      Event b5dM2()                              { return b5d(Evd::MOD2);}
      Event b5uM2()                              { return b5u(Evd::MOD2);}
      Event b5dM3()                              { return b5d(Evd::MOD3);}
      Event b5uM3()                              { return b5u(Evd::MOD3);}

      Event b6d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B6D,mods);}
      Event b6u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B6U,mods);}
      Event b6dS ()                              { return b6d(Evd::SHIFT); }
      Event b6uS ()                              { return b6u(Evd::SHIFT);}
      Event b6dC ()                              { return b6d(Evd::CONTROL);}
      Event b6uC ()                              { return b6u(Evd::CONTROL);}
      Event b6dM1()                              { return b6d(Evd::MOD1);}
      Event b6uM1()                              { return b6u(Evd::MOD1);}
      Event b6dM2()                              { return b6d(Evd::MOD2);}
      Event b6uM2()                              { return b6u(Evd::MOD2);}
      Event b6dM3()                              { return b6d(Evd::MOD3);}
      Event b6uM3()                              { return b6u(Evd::MOD3);}

      Event b7d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B7D,mods);}
      Event b7u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B7U,mods);}
      Event b7dS ()                              { return b7d(Evd::SHIFT); }
      Event b7uS ()                              { return b7u(Evd::SHIFT);}
      Event b7dC ()                              { return b7d(Evd::CONTROL);}
      Event b7uC ()                              { return b7u(Evd::CONTROL);}
      Event b7dM1()                              { return b7d(Evd::MOD1);}
      Event b7uM1()                              { return b7u(Evd::MOD1);}
      Event b7dM2()                              { return b7d(Evd::MOD2);}
      Event b7uM2()                              { return b7u(Evd::MOD2);}
      Event b7dM3()                              { return b7d(Evd::MOD3);}
      Event b7uM3()                              { return b7u(Evd::MOD3);}
   
      Event b8d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B8D,mods);}
      Event b8u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B8U,mods);}
      Event b8dS ()                              { return b8d(Evd::SHIFT); }
      Event b8uS ()                              { return b8u(Evd::SHIFT);}
      Event b8dC ()                              { return b8d(Evd::CONTROL);}
      Event b8uC ()                              { return b8u(Evd::CONTROL);}
      Event b8dM1()                              { return b8d(Evd::MOD1);}
      Event b8uM1()                              { return b8u(Evd::MOD1);}
      Event b8dM2()                              { return b8d(Evd::MOD2);}
      Event b8uM2()                              { return b8u(Evd::MOD2);}
      Event b8dM3()                              { return b8d(Evd::MOD3);}
      Event b8uM3()                              { return b8u(Evd::MOD3);}

      Event b9d  (Evd::DEVmod mods=Evd::NONE)    { return Event(_btn,B9D,mods);}
      Event b9u  (Evd::DEVmod mods=Evd::ANY)     { return Event(_btn,B9U,mods);}
      Event b9dS ()                              { return b9d(Evd::SHIFT); }
      Event b9uS ()                              { return b9u(Evd::SHIFT);}
      Event b9dC ()                              { return b9d(Evd::CONTROL);}
      Event b9uC ()                              { return b9u(Evd::CONTROL);}
      Event b9dM1()                              { return b9d(Evd::MOD1);}
      Event b9uM1()                              { return b9u(Evd::MOD1);}
      Event b9dM2()                              { return b9d(Evd::MOD2);}
      Event b9uM2()                              { return b9u(Evd::MOD2);}
      Event b9dM3()                              { return b9d(Evd::MOD3);}
      Event b9uM3()                              { return b9u(Evd::MOD3);}

      Event mov  (Evd::DEVmod mods=Evd::ANY)     { return Event(_ptr,MOV,mods);}
      Event movC ()                              { return mov(Evd::CONTROL); }
      Event movS ()                              { return mov(Evd::SHIFT); }
      Event movM1()                              { return mov(Evd::MOD1); }
      Event movM2()                              { return mov(Evd::MOD2); }
      Event movM3()                              { return mov(Evd::MOD3); }

      Event shiftD()                             { return Event(0,SHIFT_D); }
      Event ctrlD ()                             { return Event(0,CTL_D); }
      Event shiftU()                             { return Event(0,SHIFT_U); }
      Event ctrlU ()                             { return Event(0,CTL_U); }
};

// When using clip plane, this allows certain objects to be excluded from
// clipping
extern hashvar<int> DONOT_CLIP_OBJ;

//
// FSA Template:
//
template <class EVENT>
class FSAT {
   protected:
     State_t<EVENT>  *_cur;
     State_t<EVENT>  *_start;

   public:
     FSAT():_cur(0), _start(0)                   { }
     FSAT(State_t<EVENT> *x):_cur(x), _start(x)  { }

     State_t<EVENT> *cur()                      { return _cur; }
     State_t<EVENT> *start()                    { return _start; }
     void   set_cur(State_t<EVENT> *x)          { _cur = x; }
     void   reset()                             { _cur = _start; }
     bool   is_reset()                  const   { return _cur == _start; }

     // Try to handle the event. If so, return the result of
     // the callback function.
     bool handle_event(const EVENT &e) {
        int ret=0;
        _cur = _cur->event(e, _start, &ret);
        return (ret != 0);
     }
};

typedef FSAT<Event> FSA;

class VIEWint : public DEVhandler {
   protected:
     VIEWptr      _view;
     ARRAY<Evd>   _events;        // events posted by application
     ARRAY<FSA*>  _cur_states;    // interactors listening to events on view

   public:
              VIEWint(CVIEWptr &v)          { _view = v; }

     VIEWptr  view() const                  { return _view;} 
     VIEWptr  view()                        { return _view;} 

     void     handle_event(CEvd &e);
     void     post_event  (CEvd &e)         { _events += e; }

     void     add_interactor(FSA* fsa)      { _cur_states += fsa; }
     void     rem_interactor(FSA* fsa)      { _cur_states -= fsa; }

     void     add_interactor(State *s)      { add_interactor(new FSA(s)); }
     void     rem_interactor(State *s);
     void     clear_interactors()           { _cur_states.clear();   }
};

class VIEWint_list {
   protected:
     static HASH *_dhash; // hash map from VIEW to VIEWint

     static HASH    *dhash()  { return _dhash ? _dhash : (_dhash=new HASH(16));}

     static VIEWint *find(CVIEWptr &v)             { return (VIEWint *)
                                                     dhash()->find((long) &*v);}
     static void     add (CVIEWptr &v, VIEWint *d) { dhash()->add((long)&*v,d);}
     static VIEWint *make(CVIEWptr &v)             { if(find(v)) return find(v);
                                                     VIEWint *x=new VIEWint(v);
                                                     add(v, x); return x; }
     static void     del (CVIEWptr &v)             { if (find(v))
                                                     dhash()->del((long) &*v); }
   public:
     static VIEWint *get(CVIEWptr &v)            { return make(v); }
     static void     set(CVIEWptr &v, VIEWint *d){ del(v); add(v, d); }
     static void     add(CVIEWptr &v, State *s)  { get(v)->add_interactor(s); }
     static void     rem(CVIEWptr &v, State *s)  { get(v)->rem_interactor(s); }
     static void     clear(CVIEWptr &v)          { get(v)->clear_interactors();}
};


#endif
