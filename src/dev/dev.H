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
#ifndef DEV_H
#define DEV_H

#include "std/support.H"
#include "mlib/points.H"


//---------------------------------------------
//
// DEV: 
//    This package is used to access all devices 
// used by an application.  The general idea is
// that there are a number of generic subclasses
// from an abstract device :
//
//                  DEVice
//
// These subclasses include :
//   DEVice_buttons   - any button device 
//                (including buttons on a mouse!)
//   DEVice_2d        - a 2D valuator device 
//                (just the 2D stuff, no buttons)
//   DEVice_2d_absrel - " + supports abs/rel
//   DEVice_6d        - a 6D valuator device 
//                (once again, no buttons)
//
// The abstract DEVice stores a list of device
// observers.  These observers are all called
// whenever an event occurs on the DEVice.
// The information sent to the observers is
// packaged up in a structure :
// 
//                  Evd
//
// This Evd structure contains 3 pieces of
// information :
//     DEVice *  - a pointer to the device that
//                 generated the event         
//     DEVact    - an enumeration describing 
//                 the type of event that occured
//     DEVmod    - a bit flag of all of the
//                 modifiers active when the 
//                 event occured.
//
// Each observer must be a subclass of a :
//                
//                DEVhandler
//
// A DEVhandler must implement the 'handle_event'
// virtual function, which just takes an Evd
// as input.
//
// Details (coordinate systems, etc):
// ---
//   When a DEVice_2d's event() function is 
// called, the valuator data is mapped and scaled 
// by the DEVice_2d's map and scale parameters.
// Typically, these are setup so that the DEVice
// will report values in the -1 to 1 canonical 
// coordinate system.  (For a tablet, this would
// be from the bottom left to the upper right of
// the tablet).
//   The value of a DEVice_6d is a transformation
// that maps from the tracker coordinate system
// to the world coordinate system.  The tracker
// coordinate system is supposed to be calibrated
// to a display-dependent working volume where
// the boundary of the display map from -1 to 1
// The third coordinate also ranges from -1 to 1
// with -1 being at the display surface and 1 
// being an distance in front of the display
// approximately equal to the width of the display.
//   Since DEVice_buttons usually want to know
// where the cursor is, each DEVice_buttons can
// store a pointer to a DEVice_6d and a DEVice_2d
// object.  Thus a device like a mouse will 
// construct two devices, a DEVice_2d and 
// DEVice_button with the latter also getting a
// pointer to the former.
//
// Connecting generic DEVice_xxx's to real devices
// ----
//   In order for the generic DEVice_xxx objects 
// to get triggered with data, someone must call 
// the 'event()' function on the DEVice_xxx.  This
// event() function is different for each 
// DEVice_xxx subclass, but roughly takes the
// new value of the device and a bit flag of the
// modifiers that are currently active.
//   In practice, the generic DEVice_xxx's event() 
// functions get called by first registering the 
// file descriptor for the real device with the 
// application's highest level FD_MANAGER (see tty.H).  
// Then, when the file descriptor has available data, 
// the device specific callback function is invoked.
// This callback will then read the data from
// the file descriptor (or wherever it gets it)
// and will package it up in the canonical
// coordinate system of the device and finally
// will call the 'event()' function for the 
// appropriate DEVice_xxx.
//
//---------------------------------------------

//----------------------------------------------
// 
//   FORWARD DECLARATIONS
// 
//----------------------------------------------

class DEVice;

//----------------------------------------------
// 
//   DEVact - enumerations of all possible device
//            events.  
//            
//            Perhaps this should be more cleanly
//            separated into individual device 
//            classes, but go blow for now.
// 
//----------------------------------------------

enum DEVact {
   B1U = 0,  // these first events are generated
   B1D,      // by button devices.
   B2U,
   B2D,
   B3U,
   B3D,
   B4U,
   B4D,
   B5U,
   B5D,
   B6U,
   B6D,
   B7U,
   B7D,
   B8U,
   B8D,
   B9U,
   B9D,
   B10U,
   B10D,
   B11U,
   B11D,
   B12U,
   B12D,
   B13U,
   B13D,
   B14U,
   B14D,
   B15U,
   B15D,
   B16U,
   B16D,
   B17U,
   B17D,
   B18U,
   B18D,
   B19U,
   B19D,
   B20U,
   B20D,
   B21U,
   B21D,
   B22U,
   B22D,
   B23U,
   B23D,
   B24U,
   B24D,
   B25U,
   B25D,
   ACTIVE,    // button 9 - is the active/inactive
   INACTIVE,  //            flag for tablet devices
   MOV,       // MOTION - generated by 2d & 3d devices
   KEYU,      // KEYBOARD - generated by a NULL device
   KEYD,      //     
   SHIFT_U,   //     NULL Device Also generates
   SHIFT_D,   //          events when SHIFT or CONTROL
   CTL_U,     //          go up or down
   CTL_D
} ;

//------------------------------------------------------
//
//  Evd - 
//     Event Description. Identifies device events.
//  All device events consist of a DEVice descriptor,
//  and an action.
//
//------------------------------------------------------
#define CEvd const Evd
class Evd {
   public :

     enum DEVmod {
        EMPTY          = 0,
        NONE           = 1,
        SHIFT          = (1 << 1),
        CONTROL        = (1 << 2),
        MOD1           = (1 << 3),
        MOD2           = (1 << 4),
        MOD3           = (1 << 5),
        MOD4           = (1 << 6),
        MOD5           = (1 << 7),
        MOD6           = (1 << 8),
        SHIFT_MOD1     = (1 << 1) | (1 << 3),
        CONTROL_MOD2   = (1 << 2) | (1 << 4),
        NONE_MOD1_MOD2 = 1 | (1 << 3) | (1 << 4),
        NONE_MOD2      = 1 | (1 << 4),
        ANY            = (1 << 9)-1
     } ;

     DEVice *_d;
     DEVact  _a;
     DEVmod  _m;
     char    _c;

        Evd():_d(0)                                                        { }
        Evd(DEVice *d, DEVact a=MOV, DEVmod m=NONE):_d(d),_a(a),_m(m),_c(0){ }
        Evd(char   c,  DEVact a=KEYD):_d(0),_a(a),_m(ANY),_c(c){}
        Evd(DEVact a) :_d(0),_a(a),_m(ANY),_c(0){}

   Evd &operator = (CEvd &e) { _d=e._d;_a=e._a;_m=e._m;_c=e._c;return *this; }
   int operator == (CEvd &e) const       
                             { return e._d==_d && e._a==_a && e._c==_c && 
                                      (( ((int)_m & (int)(e._m)) != EMPTY) || 
                                      _m   == ANY || 
                                      e._m == ANY); }
};


//------------------------------------------------------
//
//  DEVhandler -
//     Defines an interface for handling Evd events
//
//------------------------------------------------------
class DEVhandler {
  public:
   DEVhandler() {}
   virtual ~DEVhandler() {}

   virtual void handle_event( CEvd & ) = 0;
};


//----------------------------------------------
// 
//   DEVice -
//      Abstract device class. Contains a pointer
//   to an event handler object.
//
//----------------------------------------------

class DEVice {
  protected:
   ARRAY<DEVhandler *>      _handlers;   // Pointer to the event handler
                                         // for this device
  public:
//    static mlib::Wtransf booth_to_room;
//    static mlib::Wtransf room_to_booth;

//    static void set_room_to_booth( mlib::CWtransf &xf )
//       { room_to_booth = xf; booth_to_room = xf.inverse();}
//    static void set_booth_to_room( mlib::CWtransf &xf )
//       { booth_to_room = xf; room_to_booth = xf.inverse();}

                 DEVice()  {}
   virtual      ~DEVice()  {}

   virtual void  add_handler( DEVhandler *h ) { _handlers += h; }
   virtual void  rem_handler( DEVhandler *h ) { _handlers -= h; }
};

//----------------------------------------------
// 
//   DEVice_2d -
//      Generic 2d valuator class.
//
//----------------------------------------------
class DEVice_2d : public DEVice {
  protected:
   mlib::XYpt      _cur;
   mlib::XYpt      _old;
   mlib::XYvec     _offset;
   mlib::XYvec     _scale;
   double    _pressure;

   virtual void _event(mlib::CXYpt &p, Evd::DEVmod mod)
                                            { set_val(map(p));
                                              for(int i=0;i<_handlers.num();i++)
	                                         _handlers[i]->handle_event(
                                                          Evd(this,MOV,mod));
                                            }

  public :
 static DEVice_2d  *last;
                DEVice_2d(): _scale(1,1), _pressure(1) { }
   virtual     ~DEVice_2d()                 { }

   void         offset (mlib::CXYvec &v)          { _offset = v; }
   void         scale  (mlib::CXYvec &s)          { _scale = s;  }

   mlib::XYpt         cur    ()                   { return _cur; }
   mlib::XYpt         old    ()                   { return _old; }
   mlib::XYvec        delta  ()                   { return (_cur - _old); }
   void         set_cur(mlib::CXYpt    &p)        { _cur = p; }
   void         set_old(mlib::CXYpt    &p)        { _old = p; }
   void         set_val(mlib::CXYpt    &p)        { _old = _cur; _cur = p; }
   virtual mlib::XYpt map    (mlib::CXYpt    &p) 
      { return (mlib::XYpt(_scale[0]*p[0], _scale[1]*p[1]) + _offset); }

   // XXX - should be passed along w/ event() parameters.
   // as-is, pressure has to get set *before* event() is called
   void   set_pressure(double p)            { _pressure = p; }
   double pressure()                const   { return _pressure; }

   virtual void event_delta(mlib::CXYvec &v, Evd::DEVmod mod)
      { event(_cur+v, mod); }
   virtual void event      (mlib::CXYpt &p,  Evd::DEVmod mod)
      { last = this; if (map(p)!=_cur) _event(p,mod); }
};

class DEVice_2d_absrel : public DEVice_2d
{
 protected:
  int         _rel_flag;

  int         _down_flag, _first_down;
  mlib::XYpt        _old_abs_pos;
  mlib::XYpt        _cur_abs_pos;
  mlib::XYpt        _logical_pos;

  virtual void    _event(mlib::CXYpt  &p, Evd::DEVmod mod);

 public:
  virtual        ~DEVice_2d_absrel() {}
                  DEVice_2d_absrel() : _rel_flag(false), _down_flag(true), 
                                       _first_down(true), 
                                       _old_abs_pos(0,0), _logical_pos(0,0)
                                       {}

          void    set_as_abs_device() { _rel_flag = false; }
          void    set_as_rel_device() { _rel_flag = true;  }

  virtual void    up();
  virtual void    down();
};

//----------------------------------------------
// 
//   DEVice_buttons -
//      Generic button device
//
//----------------------------------------------
class DEVice_buttons : public DEVice {
  protected:
   int _state;
   DEVice_2d *_dev2;

  public:
   enum        DEVbutton_action { UP, DOWN };

               DEVice_buttons()              : _state(0),_dev2(0) /*,_dev6(0)*/ {}
               DEVice_buttons(DEVice_2d *d2) : _state(0),_dev2(d2)/*,_dev6(0)*/ {}
   virtual    ~DEVice_buttons()             {}

   void        set_ptr(DEVice_2d *d)  { _dev2 = d; }
   DEVice_2d  *ptr2d()                { return _dev2; }
   int         get  (int i)           { return ((_state & (1<<i)) ? 1 : 0); }
   void        set  (int i, int b)    { if (b) _state |= (1<<i); 
                                         else   _state &= (~(1<<i)); }

   virtual void event(int num, DEVbutton_action action, Evd::DEVmod mod) {
        set(num,action);
        for (int i=0; i<_handlers.num(); i++)
          _handlers[i]->handle_event(Evd(this,
                              (DEVact)(num*2 + ((action == UP) ? 0 : 1)),mod));
   }
};




//------------------------------------------------------
//
//  DEVmod_desc - 
//     Description of buttons states that are equivalent
//  to a modifier key being pressed.  e.g., its the way to
//  turn a button on a serial mouse (etc) into a control
//  key.
//
//------------------------------------------------------
#define CDEVmod_desc_list const DEVmod_desc_list
#define CDEVmod_desc      const DEVmod_desc
class DEVmod_desc {
   protected :
     Evd::DEVmod     _mod;
     DEVice_buttons *_ice;
     int             _ind;
   public :
     DEVmod_desc():_mod(Evd::NONE),_ice(0),_ind(-1)    { }
     DEVmod_desc(DEVice_buttons *ice, int btn_ind, Evd::DEVmod mod):
                  _mod(mod), _ice(ice), _ind(btn_ind)  { }
     DEVice_buttons *ice()                             { return _ice; }
     int             btn_ind()                         { return _ind; }
     Evd::DEVmod     mod()                             { return _mod; }
     int             operator == (CDEVmod_desc &d) const
	       { return d._mod==_mod && d._ice==_ice && d._ind==_ind; }
};
typedef ARRAYptrs<DEVmod_desc> DEVmod_desc_list;


//------------------------------------------------------
//
//  DEVmod_gen -
//    Virtual base class for the object that determines 
//  which window system modifiers are currently active.
//
//------------------------------------------------------
class DEVmod_gen {
   static DEVmod_desc_list  _mods;
   static DEVmod_gen       *_gen;
   static Evd::DEVmod _forced_mods;

  public :
   virtual ~DEVmod_gen() {}
   static  void         set_gen(DEVmod_gen     *gen)  { _gen = gen; }
   static  void         force_mods(Evd::DEVmod  mods) { _forced_mods = mods; }
   static  Evd::DEVmod  force_mods()                  { return _forced_mods; }
   static  void         add_mod(CDEVmod_desc &desc)   { _mods += desc; }
   virtual Evd::DEVmod  gen_mods()                    { return Evd::EMPTY; }

   static  Evd::DEVmod  mods();
};

//
// Base class for mice
// 
//
//
class Mouse {
 protected:
   int                 _stereo_view_flag;

   DEVice_2d           _pointer;
   DEVice_buttons      _buttons;

 public:
   virtual ~Mouse() {}
            Mouse() : _stereo_view_flag(0), _buttons(&_pointer) {}

   DEVice_2d        &pointer()                   { return _pointer; }
   DEVice_buttons   &buttons()                   { return _buttons; }
   virtual void      set_size(int, int) = 0;

   virtual void      stereo_view(int b)          { _stereo_view_flag = b; }
   virtual int       stereo_view()         const { return _stereo_view_flag; }

           void      add_handler(DEVhandler *h)  { _pointer.add_handler(h); 
                                                   _buttons.add_handler(h); }
};

#ifdef macosx
#   define DEV_DFLT_SERIAL_PORT "placeholder"
#elif sgi
#   define DEV_DFLT_SERIAL_PORT "/dev/ttyd2"
#elif linux
#   define DEV_DFLT_SERIAL_PORT "/dev/ttyS1"
#elif WIN32
#   define DEV_DFLT_SERIAL_PORT "/dev/com1"
#elif defined(_AIX) || defined(sun)
#   define DEV_DFLT_SERIAL_PORT "/dev/ttya"
#endif

#endif
