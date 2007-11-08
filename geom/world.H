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
#ifndef WORLD_H
#define WORLD_H

#include "std/support.H"
#include "std/time.H"
#include "geom/geom.H"
#include "geom/command.H"

class LOADER {
   protected:
      static LOADER *_all;
      LOADER        *_next;
      virtual bool try_load(Cstr_ptr &path) = 0;
      LOADER();
      virtual ~LOADER() {}
   public:
      static  bool load(Cstr_ptr &path);
};



#define CWORLDptr const WORLDptr
//-----------------------------------------------
// WORLD
//    Top level SCHEDULER, handles UNDO list, messages
//    wraps object create/display/undisplay/deletion,
//-----------------------------------------------
class NetStream;
class RenderThread;
class ThreadSync;
class REF_CLASS(WORLD) : public SCHEDULER {
 protected :
   static WORLD    *_w;
   ARRAY<TTYfd *>   _fds;

   LIST<COMMANDptr> _undoable;
   LIST<COMMANDptr> _redoable;

   HASH             _hash;

   static bool _is_over; // set to true during program exit cleanup

#ifdef USE_PTHREAD
   ThreadSync      *_tsync;
   RenderThread    *_threads;
   bool             _doMultithread;
#endif   

   virtual   void    _add_command(COMMANDptr c);
   virtual   void    _undo();
   virtual   void    _redo();
   virtual   void    _clear_redoable();
   virtual   void    _print_command_list();

   static DATA_ITEM *_default_decoder(STDdstream &, Cstr_ptr &, DATA_ITEM *);

   virtual void  _Message(Cstr_ptr &m, double secs=3,
                          mlib::CXYpt &pos=mlib::XYpt(0,.9));
   virtual void  _Multi_Message(Cstr_list &m, double secs=3,
                                mlib::CXYpt &pos=mlib::XYpt(0,.9));
   //used in _Multi_Message
   virtual str_list format_str(Cstr_ptr &str, const int line_length);
   virtual int      get_next(Cstr_ptr &str, int loc, char chr);
 public:
       WORLD();

static void       set_world     (WORLD    *d) { _w = d; }
static WORLD*     get_world     ()            { return _w; }
static int        world_set     ()            { return _w != 0;}
static void       timer_callback(CFRAMEobsptr &o){_w->schedule(o); }

 static bool       is_over() { return _is_over; }

//static STROKElist*hash          (Cstr_ptr &t) { return _w->_Hash(t); }
//static void       hash_add      (Cstr_ptr &t, STROKElist*l){_w->_Hash_add(t,l);}

       //!METHS:  wrappers on the EXIST list variable
static GEOMptr lookup (Cstr_ptr &s);
static void       create        (CGELptr &o, bool undoable=true);
static void       destroy       (CGELptr &o, bool undoable=true);
static void       destroy       (CGELlist& gels, bool undoable=true);

static str_ptr    unique_name   (Cstr_ptr &s) { return EXIST.unique_name(s);}
static str_ptr    unique_dupname(Cstr_ptr &s) { return EXIST.unique_dupname(s);}
       //!METHS: wrappers on the DRAWN list variable
static void       display       (CGELptr& o,     bool undoable=true);
static void       display_gels  (CGELlist& gels, bool undoable=true);
static void       undisplay     (CGELptr& o,     bool undoable=true);
static void       undisplay_gels(CGELlist& gels, bool undoable=true);
static int        toggle_display(CGELptr& o,     bool undoable=true); 

static bool is_displayed(CGELptr& o);

 // For debugging: show locations, lines, etc. graphically:  
 // (width is measured in pixels.)
 // 
 // show a Wpt
 static GELptr show(
    mlib::CWpt &p, double width=8, CCOLOR& col=COLOR::blue, double alpha=1,
    bool depth_test=true
    );
 // show a set of points in world space
 static GELptr show_pts(
    mlib::CWpt_list& pts, double width=8.0, CCOLOR& col=COLOR::blue, double alpha=1,
    bool depth_test=true
    );
 // show a line segment:
 static GELptr show(
    mlib::CWpt& a, mlib::CWpt& b, double width=2, CCOLOR& col=COLOR::blue, double alpha=1,
    bool depth_test=true
    );
 // show a line segment:
 static GELptr show(
    mlib::CWpt& p, mlib::CWvec& v, double width=2, CCOLOR& col=COLOR::blue, double alpha=1,
    bool depth_test=true) {
    return show(p, p+v, width, col, alpha, depth_test);
 }
 // show a polyline in world space
 static GELptr show_polyline(
    mlib::CWpt_list& pts, double width=2.0, CCOLOR& col=COLOR::blue, double alpha=1,
    bool depth_test=true
    );

 // show a Wtransf
 static GELptr show(mlib::CWtransf& xf, double axis_length=15.0);

static void message(Cstr_ptr&m, double sec=3.0, mlib::CXYpt&pos=mlib::XYpt(0,.9)) {
   _w->_Message(m, sec, pos);
}
static void multi_message(Cstr_list &m, double sec=3.0, mlib::CXYpt&pos=mlib::XYpt(0,.9)) {
   _w->_Multi_Message(m, sec, pos);
}

// Execute a command and add it to the undoable list.
static void    add_command(COMMANDptr c) { _w->_add_command(c); }

// Undo or redo the last command.
static void    undo() { _w->_undo(); }
static void    redo() { _w->_redo(); }

static void    print_command_list() { _w->_print_command_list(); }

static void       Quit          () { _w->quit(); }
static void       Clean_On_Exit () { _is_over = true; if (_w) _w->clean_on_exit();}

virtual void      clean_on_exit () const;    // pre-quit cleanup
virtual void      quit() const;              // quits program

   //!METHS: objects that are "active"
   virtual   void    add_fd  (TTYfd *f)       { _fds.add(f); }

   //!METHS: access all the world's lists
          FRAMEobslist &scheduled ()          { return _scheduled; }
         CFRAMEobslist &scheduled ()  const   { return _scheduled; }

   virtual void         poll(void);      // poll pollable dev's

   virtual void         draw(void);      // Draw all views


#ifdef USE_PTHREAD
   //!METHS: thread syncronization
   ThreadSync *get_threadsync() { return _tsync; }
#endif
};

//
// Xfscaler - linearly interpolate between two transforms over time
//
// XXX - need a MAKE_PTR_SUBC macro that works with templated sub-classes
template <class OBJ_TYPE, class OBJ_TYPE_PTR>
class XFscaler : public FRAMEobs
{
 protected:
   double    _dur;
   double    _begin_time;
   mlib::Wtransf   _start, _end;
   OBJ_TYPE  _obj;

   void (OBJ_TYPE_PTR::*_fptr)(mlib::CWtransf &);

 public:
   XFscaler( double dur,
             mlib::CWtransf &s,
             mlib::CWtransf &e,
             OBJ_TYPE obj,
             void (OBJ_TYPE_PTR::*fptr)(mlib::CWtransf &)
      ) : _dur(dur), _start(s), _end(e), _obj(obj), _fptr(fptr)
   {
      // install
      WORLD::timer_callback(this);
      _begin_time = the_time();
   }

   virtual int tick() {
      double u = clamp((the_time() - _begin_time) / _dur, 0.0, 1.0);

      mlib::Wpt start_o, end_o;
      mlib::Wvec start_x, start_y, start_z, end_x, end_y, end_z;
      _start.get_coord_system(start_o, start_x, start_y, start_z);
      _end.get_coord_system(end_o  , end_x,   end_y,   end_z);

      mlib::Wpt  o = start_o + u*(end_o - start_o);
      mlib::Wvec x = start_x + u*(end_x - start_x);
      mlib::Wvec y = start_y + u*(end_y - start_y);
      mlib::Wvec z = start_z + u*(end_z - start_z);

      mlib::Wtransf mat = mlib::Wtransf::align_and_scale(o,x,y,z);
      ((&*_obj)->*_fptr)(mat);

      if (u == 1.0)
         return -1;

      return 0;
   }
};
#endif
