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
#ifndef WINSYS_INC
#define WINSYS_INC
#include "disp/view.H"
#include "dev/dev.H"
//#include "widgets/file_select.H"
//#include "widgets/alert_box.H"

// Callback base 
class MAPPED_CB {
   public:
      virtual ~MAPPED_CB() {}
      virtual void mapped() = 0; // Called when window is mapped (appears)
      virtual void icon  () = 0; // Called when window is iconified...
};

// WINSYS base class
//
// All implemented window systems have subclasses of this class implemented,
// instances are created with the WINSYS::create() static method so that all
// window system dependent issues are hidden
//
// This class derives from DEVmod_gen since all WINSYS are supposed to 
// provide the virtual function 'mods()' that determines which modifiers
// are pressed at any given time.
//
class MoveMenu;
class AlertBox;
class FileSelect;

class WINSYS: public DEVmod_gen {
   protected:
      MAPPED_CB  *_map_cb;          // What to call back when the view window
                                    // is mapped and iconified
      VIEWptr     _view;            // View
      int         _double_buffered; // Whether we are double buffering or not
      int         _stencil_buffer;  // Do we have a stencil buffer?

   public:
      WINSYS() : _map_cb(0), _double_buffered(0), _stencil_buffer(0) {}
      virtual ~WINSYS() {}

      static  WINSYS *create(int &argc, char **argv);

      virtual CVIEWptr &view()              {return _view;}
      virtual void    set_focus() {}    // Make *this* window the active one!
      virtual void    set_context()  = 0; // We're drawing in *this* window!
      virtual void    swap_buffers() = 0; // Swaps buffers when double buffering
      

      virtual void    set_cursor(int i)  = 0; // Switches to cursor number i
      virtual int     get_cursor()  = 0; // Set current cursor 

      virtual void    display()      = 0; // Display that window!
      virtual void    setup(CVIEWptr &v) { _view = v; } // do initialization
      virtual WINSYS *copy()         = 0; // Make a new window
      virtual void    size(int &w, int &h) {}
      virtual void    position(int &x, int &y) {}
      virtual void    size_manually(int w, int h) { }
      virtual void    position_manually(int w, int h) { }

      // Bits per pixel in the colorbuffer for r, g, b, a
      virtual uint    red_bits()        const = 0;
      virtual uint    green_bits()      const = 0;
      virtual uint    blue_bits()       const = 0;
      virtual uint    alpha_bits()      const = 0;

      // Bits per pixel in the colorbuffer for r, g, b, a
      virtual uint    accum_red_bits()        const = 0;
      virtual uint    accum_green_bits()      const = 0;
      virtual uint    accum_blue_bits()       const = 0;
      virtual uint    accum_alpha_bits()      const = 0;

      virtual uint    stencil_bits()      const = 0;
      virtual uint    depth_bits()        const = 0;

      virtual void    stereo(VIEWimpl::stereo_mode m) = 0; // Set stereo mode
      virtual void    map_cb             (MAPPED_CB *cb)   { _map_cb = cb; }
      virtual int     double_buffered    () const { return _double_buffered; }
      virtual void    set_double_buffered(int db) { _double_buffered = db; }
      virtual int     stencil_buffer     () const { return _stencil_buffer; }

      // Accessors for window specific mouse, menu, cursor push, and file
      // selection objects
      virtual Mouse*       mouse() = 0;
      virtual MoveMenu*    menu(Cstr_ptr &name) = 0;
      virtual DEVhandler*  curspush() = 0;
      virtual FileSelect*  file_select() = 0;
      virtual AlertBox*    alert_box() = 0;

      virtual int id() const { 
         cerr << "Warning:  dummy WINSYS::id() called" << endl;
         return -1; 
      }
   
      // Connection locking/unlocking for multithreaded apps
      virtual void    lock() = 0;
      virtual void    unlock() = 0;

      // Optional drawing of window features (E.g., a cutom cursor)
      virtual int draw() { return 0; } 

      // Indicates whether or not window needs context to be set.
      // (E.g., to ensure that view will call window's set_context()
      // before rendering.)
      virtual int needs_context() { return 0; }    

      virtual STDdstream & operator<<(STDdstream &ds) 
      {
         //Base call sends dummy position and size
         return ds << 0 << 0 << 640 << 480;
      }

      virtual STDdstream & operator>>(STDdstream &ds) 
      {
         int foo;
         return ds >> foo >> foo >> foo >> foo;
      }
      enum cursor_t {
         CURSOR_RIGHT_ARROW = 0,     //Arrow pointing up and to the right. 
         CURSOR_LEFT_ARROW,          //Arrow pointing up and to the left. 
         CURSOR_INFO,                //Pointing hand. 
         CURSOR_DESTROY,             //Skull & cross bones. 
         CURSOR_HELP,                //Question mark. 
         CURSOR_CYCLE,               //Arrows rotating in a circle. 
         CURSOR_SPRAY,               //Spray can. 
         CURSOR_WAIT,                //Wrist watch. 
         CURSOR_TEXT,                //Insertion point cursor for text. 
         CURSOR_CROSSHAIR,           //Simple cross-hair. 
         CURSOR_UP_DOWN,             //Bi-directional pointing up & down. 
         CURSOR_LEFT_RIGHT,          //Bi-directional pointing left & right. 
         CURSOR_TOP_SIDE,            //Arrow pointing to top side. 
         CURSOR_BOTTOM_SIDE,         //Arrow pointing to bottom side. 
         CURSOR_LEFT_SIDE,           //Arrow pointing to left side. 
         CURSOR_RIGHT_SIDE,          //Arrow pointing to right side. 
         CURSOR_TOP_LEFT_CORNER,     //Arrow pointing to top-left corner. 
         CURSOR_TOP_RIGHT_CORNER,    //Arrow pointing to top-right corner. 
         CURSOR_BOTTOM_RIGHT_CORNER, //Arrow pointing to bottom-left corner. 
         CURSOR_BOTTOM_LEFT_CORNER,  //Arrow pointing to bottom-right corner. 
         CURSOR_FULL_CROSSHAIR,      //Full-screen cross-hair cursor 
         CURSOR_NONE,                //Invisible cursor. 
         CURSOR_INHERIT              //Use parent's cursor.          
      };
};
#endif
