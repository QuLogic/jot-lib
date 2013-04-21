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

#ifndef GLUT_WINSYS_H_IS_INCLUDED
#define GLUT_WINSYS_H_IS_INCLUDED

#include "std/support.H"
#include "std/iostream.H"
#include "geom/winsys.H"

#include <GL/glut.h>

class GLUT_MOUSE;
class GLUIFileSelect;
class GLUIAlertBox;

/**********************************************************************
 * GLUT_WINSYS
 **********************************************************************/

class GLUT_WINSYS : public WINSYS {
   /******** STATIC MEMBER VARIABLES ********/
 protected:
   static ARRAY<GLUT_WINSYS*>   _windows;

   /******** STATIC MEMBER METHODS ********/
 public:
   static GLUT_WINSYS*  instance() { return _windows.empty() ? 0 : _windows[0];}
   static GLUT_WINSYS*  window()   { return _windows[glutGetWindow()]; }

 protected:
   //GLUT Callbacks
   static void    idle_cb();   
   static void    display_cb();
   static void    visibility_cb(int state);          
   static void    reshape_cb(int width, int height);

   /******** MEMBER VARIABLES ********/
   
   int               _id;            
   str_ptr           _name;
   int               _width;         
   int               _height;        
   int               _init_x;        
   int               _init_y;        
   bool              _map_pending;   
   bool              _show_special_cursor;
   mlib::XYpt              _cursor_pt;  
   GLUT_MOUSE*       _mouse;         
   DEVhandler*       _curpush;       
   GLUIFileSelect*   _file_select;
   GLUIAlertBox*     _alert_box;

public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/
   GLUT_WINSYS(int &argc, char **argv);
   virtual ~GLUT_WINSYS();

   //******** METHODS ********
   void              block();
   void              unblock();

   //******** UTILITY METHODS ********
   void              push_cursor(mlib::CXYpt& pt);
   void              show_special_cursor(bool b)    { _show_special_cursor = b; }

   //******** WINSYS METHODS ********

   virtual int       id() const              { return _id; }

   virtual void      set_context()           { glutSetWindow(_id);  }
   virtual int       needs_context()         { return ((_id != glutGetWindow())?(1):(0)); }

   virtual void      set_focus();

   virtual void      display();
   virtual void      setup(CVIEWptr &v);

   virtual int       draw();   
   virtual void      set_cursor(int);
   virtual int       get_cursor();
   virtual void      swap_buffers();
   virtual void      size(int& w, int& h);
   virtual void      position(int &x, int &y);
   virtual void      size_manually(int w, int h);
   virtual void      position_manually(int x, int y);

   virtual WINSYS*      copy()                     { return new GLUT_WINSYS(*this); }
   virtual Mouse*       mouse();
   virtual DEVhandler*  curspush();
   virtual MoveMenu*    menu(Cstr_ptr & name);
   virtual FileSelect*  file_select();
   virtual AlertBox*    alert_box();
   virtual void         stereo(VIEW::stereo_mode)  { cerr << "GLUT_WINSYS::stereo() - *DUMMY*\n"; }
   
   virtual uint   red_bits() const         { GLint ret; glGetIntegerv(GL_RED_BITS, &ret); return (uint)ret; }
   virtual uint   green_bits() const       { GLint ret; glGetIntegerv(GL_GREEN_BITS, &ret); return (uint)ret; }
   virtual uint   blue_bits() const        { GLint ret; glGetIntegerv(GL_BLUE_BITS, &ret); return (uint)ret; }
   virtual uint   alpha_bits() const       { GLint ret; glGetIntegerv(GL_ALPHA_BITS, &ret); return (uint)ret; }
   virtual uint   accum_red_bits() const   { GLint ret; glGetIntegerv(GL_ACCUM_RED_BITS, &ret); return (uint)ret; }
   virtual uint   accum_green_bits() const { GLint ret; glGetIntegerv(GL_ACCUM_GREEN_BITS, &ret); return (uint)ret; }
   virtual uint   accum_blue_bits() const  { GLint ret; glGetIntegerv(GL_ACCUM_BLUE_BITS, &ret); return (uint)ret; }
   virtual uint   accum_alpha_bits() const { GLint ret; glGetIntegerv(GL_ACCUM_ALPHA_BITS, &ret); return (uint)ret; }
   virtual uint   stencil_bits() const     { GLint ret; glGetIntegerv(GL_STENCIL_BITS, &ret); return (uint)ret; }
   virtual uint   depth_bits() const       { GLint ret; glGetIntegerv(GL_DEPTH_BITS, &ret); return (uint)ret; }

   virtual void      lock()   {}
   virtual void      unlock() {}

   virtual STDdstream &    operator<<(STDdstream &ds);
   virtual STDdstream &    operator>>(STDdstream &ds);

};

#endif // GLUT_WINSYS_H_IS_INCLUDED

/* end of file glut_winsys.H */
