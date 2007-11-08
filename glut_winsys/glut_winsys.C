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
#ifdef macosx
#include <unistd.h>
#endif

#include "glew/glew.H" // must come first

#include "std/config.H"
#include "glut_winsys.H"
#include "tty_glut.H"
#include "mouse.H" 
#include "kbd.H"
#include "glui/glui.h"
#include "glui_menu.H"
#include "glui_dialogs.H"

// obsolete:
#ifdef USE_GLUT_WACOM
#include "glutwacom/glutwacom.h"
#endif

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_GLUT_WINSSYS", false);

/*****************************************************************
 * GLUT_WINSYS
 *****************************************************************/

//////////////////////////////////////////////////////
// Static Variables Initialization
//////////////////////////////////////////////////////

ARRAY<GLUT_WINSYS*>  GLUT_WINSYS::_windows(1);

//////////////////////////////////////////////////////
// WINSYS Factory
//////////////////////////////////////////////////////
WINSYS *
WINSYS::create(int &argc, char **argv)
{
   if (!GLUT_WINSYS::instance()) 
      return new GLUT_WINSYS(argc, argv);

   return GLUT_WINSYS::instance()->copy();
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
GLUT_WINSYS::GLUT_WINSYS(int &argc, char **argv) :
   _id(-1),
   _width (Config::get_var_int("JOT_WINDOW_WIDTH",  640,true)),
   _height(Config::get_var_int("JOT_WINDOW_HEIGHT", 480,true)),
   _init_x(Config::get_var_int("JOT_WINDOW_X",  4,true)),
   _init_y(Config::get_var_int("JOT_WINDOW_Y", 28,true)),
   _map_pending(false),
   _show_special_cursor(false),
   _mouse(0),
   _curpush(0),
   _file_select(0),
   _alert_box(0)
{
  _double_buffered = 1;

#ifdef WIN32
   // Start up the socket library...
   WSADATA wsaData;
   if (WSAStartup(0x0101, &wsaData)) {
      err_ret("GLUT_WINSYS::GLUT_WINSYS() - Error while initializing sockets.");
   }
   SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),ENABLE_PROCESSED_INPUT);
#endif

#ifdef macosx
   // apple's GLUT framework trashes current dir..
   // so get current directory
   char * cwd = getcwd(NULL, 0);
#endif

   glutInit(&argc, argv);

#ifdef macosx
   //and restore directory afterwards
   chdir (cwd);
   free(cwd);
#endif

   FD_MANAGER::set_mgr(new GLUT_MANAGER);
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
GLUT_WINSYS::~GLUT_WINSYS() 
{
#ifdef WIN32
   if (WSACleanup())
      err_ret("GLUT_WINSYS::~GLUT_WINSYS() - **Error** closing sockets");
#endif
}

/////////////////////////////////////
// setup()
/////////////////////////////////////
void
GLUT_WINSYS::setup(CVIEWptr &v)
{
   WINSYS::setup(v);

   // Initial window parameters
   glutInitWindowSize(_width, _height);
   glutInitWindowPosition(_init_x, _init_y);

   glutInitDisplayMode(
      GLUT_RGBA|GLUT_ALPHA|GLUT_DOUBLE|GLUT_DEPTH|GLUT_STENCIL|GLUT_ACCUM
      );
   
   // Create the windows (it becomes 'current')
   _name = Config::get_var_str("JOT_WINDOW_NAME", "Jot",true);
   _id = glutCreateWindow(**_name);

   if (debug) {
      cerr << "GLUT_WINSYS::setup: got window ID: "
           << _id
           << endl;
   }

   while (_windows.num() <= _id)
      _windows += (GLUT_WINSYS*) 0; // XXX - Must cast or += ARRAY is used
   _windows[_id] = this;

   // Set callbacks for 'current' window
   glutVisibilityFunc(visibility_cb);
   glutReshapeFunc(reshape_cb);

   // Frankly, these should be set only for the root window... 
   // but we only ever have one window these days.
   glutDisplayFunc(display_cb);
   GLUI_Master.set_glutIdleFunc(idle_cb);
      
   //Sets the keyboard cbs
   new GLUT_KBD(this);

   //Sets the mouse/tablet cbs
   _mouse = new GLUT_MOUSE(this); 

   //Allocate the dialogs
   _file_select = new GLUIFileSelect(this);  assert(_file_select);
   _alert_box = new GLUIAlertBox(this);  assert(_alert_box);

   GLenum err = glewInit();
   if (err == GLEW_OK) {
      if (Config::get_var_bool("JOT_PRINT_GLEW_INFO", false)) {
         cerr << "\nGLUT_WINSYS::setup: using GLEW "
              << glewGetString(GLEW_VERSION) << endl;
         if (GLEW_VERSION_2_0)
            cerr << "OpenGL 2.0 is supported" << endl;
         else if (GLEW_VERSION_1_5)
            cerr << "OpenGL 1.5 is supported" << endl;
         else if (GLEW_VERSION_1_4)
            cerr << "OpenGL 1.4 is supported" << endl;
         else if (GLEW_VERSION_1_3)
            cerr << "OpenGL 1.3 is supported" << endl;
         else if (GLEW_VERSION_1_2)
            cerr << "OpenGL 1.2 is supported" << endl;
         else if (GLEW_VERSION_1_1)
            cerr << "OpenGL 1.1 is supported" << endl;
         else 
            cerr << "Error: unknown version of OpenGL" << endl;
      }
   } else {
     // Problem: glewInit failed, something is seriously wrong.
     cerr << "GLUT_WINSYS::setup: error calling glewInit: "
	  << glewGetErrorString(err) << endl;
   }
}

/////////////////////////////////////
// mouse()
/////////////////////////////////////
Mouse*
GLUT_WINSYS::mouse()
{
   assert(_mouse);
   return _mouse;
}

/////////////////////////////////////
// curspush()
/////////////////////////////////////
DEVhandler*
GLUT_WINSYS::curspush() 
{ 
   if (!_curpush) {_curpush = new GLUT_CURSpush(this); assert(_curpush); }

   return _curpush;
}

/////////////////////////////////////
// idle_cb()
/////////////////////////////////////
void    
GLUT_WINSYS::idle_cb()      
{ 
   GLUT_MANAGER::idle_cb(); 
}

/////////////////////////////////////
// display_cb()
/////////////////////////////////////
void    
GLUT_WINSYS::display_cb()   
{ 
   GLUT_MANAGER::display_cb(); 
}

/////////////////////////////////////
// visibility_cb()
/////////////////////////////////////
void
GLUT_WINSYS::visibility_cb(int state)
{
   GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);
   // XXX - Just block events to the blocking window, or all windows?
   //       Right now, we only use one window, so this is academic...
   // XXX - Process these callbacks even when blocking. It only does
   //       add/remove on the window in the timeouts list of GLUT_WINSYS.
   //if (mgr->get_blocker() == GLUT_WINSYS::window()) return;

   GLUT_WINSYS *win= _windows[glutGetWindow()];     assert(win->_map_cb);

   // Only do mapped callback if we have height and width for the window
   if (state == GLUT_VISIBLE && win->_width == 0 && win->_height == 0) {
      win->_map_pending = true;
      return;
   }

   switch (state) {
     case      GLUT_VISIBLE:     win->_map_cb->mapped();  break;
     case      GLUT_NOT_VISIBLE: win->_map_cb->icon();    break;
     assert(0);
   }
} 

/////////////////////////////////////
// reshape_cb()
/////////////////////////////////////
void
GLUT_WINSYS::reshape_cb(int width, int height)
{
   GLUT_MANAGER *mgr = (GLUT_MANAGER *)FD_MANAGER::mgr(); assert(mgr);
   // XXX - Just block events to the blocking window, or all windows?
   //       Right now, we only use one window, so this is academic...
   // XXX - Store the new window size, but don't tell the view. 
   //       This will be called by unblock() to handle any pending work...

   GLUT_WINSYS *win = _windows[glutGetWindow()];
   assert(win && win->_view != NULL);

   win->_width = width;
   win->_height = height;

   if (win->_map_pending) {
      win->_map_pending = false;
      visibility_cb(GLUT_VISIBLE);
   }

   if (mgr->get_blocker() == GLUT_WINSYS::window()) return;

   win->_view->set_size(width, height, 0, 0);   
}

/////////////////////////////////////
// block()
/////////////////////////////////////
void
GLUT_WINSYS::block()
{ 
   // Do pre-blocking work...

   if (debug)
      cerr << "GLUT_WINSYS::block" << endl;

   // XXX - Just block this window's gluis?!
   GLUI_Master.block_gluis_by_gfx_window_id(_id);
}

/////////////////////////////////////
// unblock()
/////////////////////////////////////
void
GLUT_WINSYS::unblock()
{ 
   // Do post-blocking work...

   if (debug)
      cerr << "GLUT_WINSYS::unblock" << endl;

   // XXX - Just unblock this window's gluis?!
   GLUI_Master.unblock_gluis_by_gfx_window_id(_id);

   // Now complete reshape callback if req'd...

   int width, height;
   _view->get_size(width, height);   
   if ((width != _width) || (height != _height)) {
      int old_id = glutGetWindow();
      if (debug) {
         cerr << "  old ID: " << old_id
              << ", main ID: " << _id
              << endl;
      }
      glutSetWindow(_id);
      reshape_cb(_width,_height);
      glutSetWindow(old_id);
   }
}

/////////////////////////////////////
// size()
/////////////////////////////////////
void
GLUT_WINSYS::size(int& w, int& h)
{ 
   w = _width; 
   h = _height;
}

/////////////////////////////////////
// position()
/////////////////////////////////////
void
GLUT_WINSYS::position(int& x, int& y)
{ 
   if (debug)
      cerr << "GLUT_WINSYS::position" << endl;

   if (_id == -1) {
      if (debug)
         cerr << "  ID is -1" << endl;
      x = _init_x;
      y = _init_y;
   } else {
      int old_id = glutGetWindow();
      if (debug) {
         cerr << "  old ID: " << old_id
              << ", main ID: " << _id
              << endl;
      }
      glutSetWindow(_id);
      x = glutGet(GLUT_WINDOW_X);
      y = glutGet(GLUT_WINDOW_Y);
      glutSetWindow(old_id);
   }
}

/////////////////////////////////////
// position_manually()
/////////////////////////////////////
void
GLUT_WINSYS::position_manually(int x, int y) 
{
   if (debug)
      cerr << "GLUT_WINSYS::position_manually" << endl;

   int old_id = glutGetWindow();
   if (_id == -1) {
      if (debug)
         cerr << "  ID is -1" << endl;
      _init_x = x;
      _init_y = y;
   } else {
      if (debug) {
         cerr << "  old ID: " << old_id
              << ", main ID: " << _id
              << endl;
      }
      glutSetWindow(_id);
      glutPositionWindow(x,y);
      glutSetWindow(old_id);
   }
}

/////////////////////////////////////
// size_manually()
/////////////////////////////////////
void
GLUT_WINSYS::size_manually(int w, int h) 
{
   if (debug)
      cerr << "GLUT_WINSYS::size_manually" << endl;

   int old_id = glutGetWindow();
   if (_id == -1) {
      if (debug)
         cerr << "  ID is -1" << endl;
      _width = w;
      _height = h;
   } else {
      if (debug) {
         cerr << "  old ID: " << old_id
              << ", main ID: " << _id
              << endl;
      }
      glutSetWindow(_id);
      glutReshapeWindow(w,h);
      glutSetWindow(old_id);
   }
}

/////////////////////////////////////
// file_select()
/////////////////////////////////////
FileSelect*
GLUT_WINSYS::file_select()
{ 
   //Shouldn't ask for this
   //before it's initialized 
   //in setup()
   assert(_file_select);
   return _file_select; 
}

/////////////////////////////////////
// alert_box()
/////////////////////////////////////
AlertBox*
GLUT_WINSYS::alert_box()
{ 
   //Shouldn't ask for this
   //before it's initialized 
   //in setup()
   assert(_alert_box);
   return _alert_box; 
}

/////////////////////////////////////
// menu()
/////////////////////////////////////
MoveMenu*
GLUT_WINSYS::menu(Cstr_ptr &name)
{
   assert(_id != -1);  // menu() shouldn't be called before setup()
   return new GLUIMoveMenu(name, _id);
}

/////////////////////////////////////
// swap_buffers()
/////////////////////////////////////
void
GLUT_WINSYS::swap_buffers()
{
   glutSwapBuffers(); 
}

/////////////////////////////////////
// set_focus()
/////////////////////////////////////
void
GLUT_WINSYS::set_focus()
{
   if (debug)
      cerr << "GLUT_WINSYS::set_focus" << endl;

#ifdef WIN32
   HWND hwnd = FindWindow("GLUT", **_name);
   if(hwnd != NULL) {
      //SetFocus(hwnd);
      SetActiveWindow(hwnd);
   } else {
      cerr << "GLUT_WINSYS::set_focus: error: can't get window handle: "
           << _name
           << endl;
   }
#else
   int old_id=glutGetWindow();
   if (debug) {
      cerr << "  old ID: " << old_id
           << ", main ID: " << _id
           << endl;
   }
   glutSetWindow(_id);
   glutPopWindow();
   glutSetWindow(old_id);
#endif
}

/////////////////////////////////////
// get_cursor()
/////////////////////////////////////
int
GLUT_WINSYS::get_cursor()
{
   if (debug)
      cerr << "GLUT_WINSYS::get_cursor" << endl;

   int old_id = glutGetWindow();
   if (debug) {
      cerr << "  old ID: " << old_id
           << ", main ID: " << _id
           << endl;
   }
   glutSetWindow(_id); 
   int glut_cursor = glutGet(GLUT_WINDOW_CURSOR);
   glutSetWindow(old_id);

   int winsys_cursor;
   switch(glut_cursor) {
    case GLUT_CURSOR_RIGHT_ARROW:         winsys_cursor = CURSOR_RIGHT_ARROW;
      break;
    case GLUT_CURSOR_LEFT_ARROW:          winsys_cursor = CURSOR_LEFT_ARROW;
      break;
    case GLUT_CURSOR_INFO:                winsys_cursor = CURSOR_INFO;
      break;
    case GLUT_CURSOR_DESTROY:             winsys_cursor = CURSOR_DESTROY;
      break;            
    case GLUT_CURSOR_HELP:                winsys_cursor = CURSOR_HELP;
      break;                
    case GLUT_CURSOR_CYCLE:               winsys_cursor = CURSOR_CYCLE;
      break;               
    case GLUT_CURSOR_SPRAY:               winsys_cursor = CURSOR_SPRAY;
      break;               
    case GLUT_CURSOR_WAIT:                winsys_cursor = CURSOR_WAIT;
      break;                
    case GLUT_CURSOR_TEXT:                winsys_cursor = CURSOR_TEXT;
      break;                
    case GLUT_CURSOR_CROSSHAIR:           winsys_cursor = CURSOR_CROSSHAIR;
      break;           
    case GLUT_CURSOR_UP_DOWN:             winsys_cursor = CURSOR_UP_DOWN;
      break;             
    case GLUT_CURSOR_LEFT_RIGHT:          winsys_cursor = CURSOR_LEFT_RIGHT;
      break;          
    case GLUT_CURSOR_TOP_SIDE:            winsys_cursor = CURSOR_TOP_SIDE;
      break;            
    case GLUT_CURSOR_BOTTOM_SIDE:         winsys_cursor = CURSOR_BOTTOM_SIDE;
      break;         
    case GLUT_CURSOR_LEFT_SIDE:           winsys_cursor = CURSOR_LEFT_SIDE;
      break;           
    case GLUT_CURSOR_RIGHT_SIDE:          winsys_cursor = CURSOR_RIGHT_SIDE;
      break;          
    case GLUT_CURSOR_TOP_LEFT_CORNER:     winsys_cursor = CURSOR_TOP_LEFT_CORNER;
      break;     
    case GLUT_CURSOR_TOP_RIGHT_CORNER:    winsys_cursor = CURSOR_TOP_RIGHT_CORNER;
      break;    
    case GLUT_CURSOR_BOTTOM_RIGHT_CORNER:
      winsys_cursor = CURSOR_BOTTOM_RIGHT_CORNER;
      break; 
    case GLUT_CURSOR_BOTTOM_LEFT_CORNER:
      winsys_cursor = CURSOR_BOTTOM_LEFT_CORNER;
      break; 
    case GLUT_CURSOR_FULL_CROSSHAIR:
      winsys_cursor = CURSOR_FULL_CROSSHAIR;
      break;      
    case GLUT_CURSOR_NONE:                winsys_cursor = CURSOR_NONE;
      break;                
    case GLUT_CURSOR_INHERIT:             winsys_cursor = CURSOR_INHERIT;
      break;             
    default:                              winsys_cursor = CURSOR_NONE;
      break;                
   }
   return winsys_cursor;
}

/////////////////////////////////////
// set_cursor()
/////////////////////////////////////
void
GLUT_WINSYS::set_cursor(int winsys_cursor)
{
   if (debug)
      cerr << "GLUT_WINSYS::get_cursor" << endl;

   int glut_cursor;
   switch(winsys_cursor) {  
    case CURSOR_RIGHT_ARROW:         glut_cursor = GLUT_CURSOR_RIGHT_ARROW;          break;
    case CURSOR_LEFT_ARROW:          glut_cursor = GLUT_CURSOR_LEFT_ARROW;           break;
    case CURSOR_INFO:                glut_cursor = GLUT_CURSOR_INFO;                 break;
    case CURSOR_DESTROY:             glut_cursor = GLUT_CURSOR_DESTROY;              break;            
    case CURSOR_HELP:                glut_cursor = GLUT_CURSOR_HELP;                 break;                
    case CURSOR_CYCLE:               glut_cursor = GLUT_CURSOR_CYCLE;                break;               
    case CURSOR_SPRAY:               glut_cursor = GLUT_CURSOR_SPRAY;                break;               
    case CURSOR_WAIT:                glut_cursor = GLUT_CURSOR_WAIT;                 break;                
    case CURSOR_TEXT:                glut_cursor = GLUT_CURSOR_TEXT;                 break;                
    case CURSOR_CROSSHAIR:           glut_cursor = GLUT_CURSOR_CROSSHAIR;            break;           
    case CURSOR_UP_DOWN:             glut_cursor = GLUT_CURSOR_UP_DOWN;              break;             
    case CURSOR_LEFT_RIGHT:          glut_cursor = GLUT_CURSOR_LEFT_RIGHT;           break;          
    case CURSOR_TOP_SIDE:            glut_cursor = GLUT_CURSOR_TOP_SIDE;             break;            
    case CURSOR_BOTTOM_SIDE:         glut_cursor = GLUT_CURSOR_BOTTOM_SIDE;          break;         
    case CURSOR_LEFT_SIDE:           glut_cursor = GLUT_CURSOR_LEFT_SIDE;            break;           
    case CURSOR_RIGHT_SIDE:          glut_cursor = GLUT_CURSOR_RIGHT_SIDE;           break;          
    case CURSOR_TOP_LEFT_CORNER:     glut_cursor = GLUT_CURSOR_TOP_LEFT_CORNER;      break;     
    case CURSOR_TOP_RIGHT_CORNER:    glut_cursor = GLUT_CURSOR_TOP_RIGHT_CORNER;     break;    
    case CURSOR_BOTTOM_RIGHT_CORNER: glut_cursor = GLUT_CURSOR_BOTTOM_RIGHT_CORNER;  break; 
    case CURSOR_BOTTOM_LEFT_CORNER:  glut_cursor = GLUT_CURSOR_BOTTOM_LEFT_CORNER;   break; 
    case CURSOR_FULL_CROSSHAIR:      glut_cursor = GLUT_CURSOR_FULL_CROSSHAIR;       break;      
    case CURSOR_NONE:                glut_cursor = GLUT_CURSOR_NONE;                 break;                
    case CURSOR_INHERIT:             glut_cursor = GLUT_CURSOR_INHERIT;              break;             
    default:                         glut_cursor = GLUT_CURSOR_NONE;                 break;                
   }

   int old_id = glutGetWindow();
   if (debug) {
      cerr << "  old ID: " << old_id
           << ", main ID: " << _id
           << endl;
   }
   glutSetWindow(_id); 
   glutSetCursor(glut_cursor);
   glutSetWindow(old_id);
}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int
GLUT_WINSYS::draw()
{
   if (!_show_special_cursor) return 0;

   // compute pixel coordinates of cursor
   int x = int(((_cursor_pt[0] + 1.0)/2.0) * (double)_width);
   int y = int(((_cursor_pt[1] + 1.0)/2.0) * (double)_height);

   // render the 'cross-hairs' cursor
   glPushMatrix();

   glViewport(0, 0, (GLsizei)_width, (GLsizei)_height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0, (GLdouble)_width, 0, (GLdouble)_height);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);

   glDisable(GL_LIGHTING);          // GL_ENABLE_BIT
   glColor3dv(COLOR::red.data());   // GL_CURRENT_BIT

   glBegin(GL_LINES);

   // draw 20-pixel-long cross hairs

   glVertex2i(x, y+10);
   glVertex2i(x, y-10);

   glVertex2i(x+10, y);
   glVertex2i(x-10, y);

   glEnd();

   glPopAttrib();
   glPopMatrix();

   return 0;
}

/////////////////////////////////////
// push_cursor()
/////////////////////////////////////
void
GLUT_WINSYS::push_cursor(CXYpt & pt) 
{
   if (debug)
      cerr << "GLUT_WINSYS::push_cursor" << endl;

   _cursor_pt = pt; 
   _show_special_cursor = true;

   const int xpos = (int)( pt[0] /2* _width  + _width /2.);
   const int ypos = (int)(-pt[1] /2* _height + _height/2.);

   int old_id = glutGetWindow();
   if (debug) {
      cerr << "  old ID: " << old_id
           << ", main ID: " << _id
           << endl;
   }

   glutSetWindow(_id); 
   // XXX - If _show_special_cursor is true, we draw a GL cursor
   //       So why do we warp the OS's system cursor, too?
   glutWarpPointer(xpos, ypos);
   glutSetWindow(old_id);
}

/////////////////////////////////////
// display()
/////////////////////////////////////
void 
GLUT_WINSYS::display() 
{
   // Nothing needed... window is already built and showing...
}
 

/////////////////////////////////////
// operator>>()
/////////////////////////////////////
STDdstream& 
GLUT_WINSYS::operator>>(STDdstream &ds) 
{
   int x,y;
   position(x,y);

   return ds << x << y << _width << _height; 
}

/////////////////////////////////////
// operator<<()
/////////////////////////////////////
STDdstream&
GLUT_WINSYS::operator<<(STDdstream &ds) 
{
   int x, y, w, h;

   ds >> x >> y >> w >> h;
   position_manually(x,y);
   size_manually(w,h);

   return ds;
}

// end of file glut_winsys.C
