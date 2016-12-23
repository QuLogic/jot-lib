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
#ifndef BASEJOTAPP_H
#define BASEJOTAPP_H

#include <vector>
#include "std/config.hpp"
#include "disp/view.hpp"
#include "geom/winsys.hpp"
#include "geom/world.hpp"
#include "gest/easel_manager.hpp"
#include "gest/pen_manager.hpp"
#include "manip/manip.hpp"
#include "manip/key_menu.hpp"
#include "net/data_item.hpp"
#include "geom/icon2d.hpp"

class MoveMenu;
class Cam_int;
class Cam_int_fp;
class Cam_int_edit;
MAKE_SHARED_PTR(BMESH);
class GEOM;

/**********************************************************************
 * Simple_Keyboard
 **********************************************************************/

class Simple_Keyboard : public Key_int {
 public :
   Simple_Keyboard(State *start) {
      add_event("abcdefghijklmnopqrstuvwxyz", start);
      add_event("ABCDEFGHIJKLMNOPQRSTUVWXYZ", start);
      add_event("1234567890_=!@#$%^&*()", start);
      add_event(" _+-[]{}|;:'\",<.>/?~`", start);
      add_event("\n\x8\x7f\x1b", start);
      *start += _entry;
   }
   virtual ~Simple_Keyboard() {}


   virtual int down(const Event &e, State *&) {
      switch (e._c) {
      case 'Q':
         WORLD::Quit();
         break;
      default:
         cerr << "Simple_Keyboard::down() - Unhandled key: '" << e._c << "'\n";
         break;
      }
      return 0;
   }
};

/*!
 *  \brief The base class for a Jot application.
 *
 */
class BaseJOTapp : public MAPPED_CB, public FD_TIMEOUT {
 public:

   
   class WINDOW {

    public:

      MoveMenu*      _menu;
      Cam_int*       _cam1;
	  Cam_int_fp*    _cam2;
	  Cam_int_edit*  _cam3;
      WINSYS*        _win;
      ButtonMapper   _mouse_map;
      VIEWptr        _view;
      State          _start;


      WINDOW() :
         _menu(nullptr),_cam1(nullptr), _cam2(nullptr), _win(nullptr),  _mouse_map(nullptr,nullptr) {
         _start.set_name("WINDOW::start");
      }
      WINDOW(WINSYS *win) :
         _menu(nullptr),_cam1(nullptr),_cam2(nullptr), _win(win),_mouse_map(nullptr,nullptr) {
         _start.set_name("WINDOW::start");
      }

      virtual ~WINDOW() {}
   };

   static BaseJOTapp *instance()        { return _instance;     }
   static WINDOW *window(int i=0)       { return _windows[i];   }
   static void Clean_On_Exit () {
      if (_instance)
         _instance->clean_on_exit();
   }

   BaseJOTapp(int argc, char **argv);
   BaseJOTapp(const string &name);

   virtual ~BaseJOTapp() {
      if (FD_MANAGER::mgr())
         FD_MANAGER::mgr()->rem_timeout(this);
   }

   virtual void init();
   virtual void Run();

   //! \name EaselManager Related Functions
   //@{

   EaselManager &easels() { return _easel_manager; }

   //@}

   //! \name PenManager Related Functions
   //@{

   Pen *cur_pen() { return _pen_manager ? _pen_manager->cur_pen() : nullptr; }
   void next_pen() { if(_pen_manager)
         _pen_manager->next_pen(); }
   void prev_pen() { if(_pen_manager)
         _pen_manager->prev_pen(); }

   //@}

   //! \name Render Mode Menu Related Functions
   //@{

   bool menu_is_shown();
   void show_menu();
   void hide_menu();
   void toggle_menu();

   //(de)activate on screen camera button
   static void activate_button(const string &file);
   static void update_button(const string &file);
   static void toggle_button(const string &file);
   static void deactivate_button();


   //@}

   //! \name KeyMenu Callback Functions
   //@{
   //! \brief Callback to switch cam mode
   static int cam_switch(const Event&, State *&);
   //! \brief Callback to toggle camera buttons
   static int button_toggle(const Event&, State *&);
   //! \brief Callback to display KeyMenu help menu.
   static int keymenu_help_cb(const Event&, State *&);

   //! \brief Callback to quit application.
   static int keymenu_quit_cb(const Event&, State *&);

   //! \brief Callback to toggle showing render style menu.
   static int show_menu_cb(const Event&, State *&);

   //! \brief Callback to reset camera to fit whole scene in view:
   static int viewall(const Event&, State *&);



   //@}

 protected:

   //! \name Utility Functions for Subclasses
   //@{


   void pop_arg();

   //@}

   //! \name Creation functions to be overriden by subclasses
   //@{

   virtual WINDOW *new_window(WINSYS *win) { return new WINDOW(win);}
   virtual VIEWptr new_view(WINSYS *win);

   //@}

   //! \name Hooks to be overriden/extended by subclasses
   //@{

   virtual void clean_on_exit();



   virtual void print_usage() const;

   // Create top level widget
   virtual void init_top();
   // Create world
   virtual void init_world();
   // Create view and add mapped handler
   virtual void init_view(WINDOW &window);
   virtual void init_camera(CVIEWptr &v);
   // Create on-screen controls
   virtual void init_buttons(CVIEWptr &v);
   
   // Create basic scene
   virtual void init_scene();
   // Load world or set up for networking
   virtual void load_scene();

 public:
   // load different file types: .jot, .sm, .obj:
   virtual bool load_jot_file(const string &file);
   virtual bool load_sm_file (const string &file);
   virtual bool load_obj_file(const string &file);
   virtual bool load_png_file(const string &file);
 protected:

   // Default method for creating a new BMESH or derived type:
   virtual BMESHptr new_mesh() const;

   // Default method for creating GEOM or derived type to hold a mesh:
   virtual GEOM* new_geom(BMESHptr mesh, const string& name) const;

   // Put the mesh in GEOM and add it to the scene:
   bool create_mesh(BMESHptr mesh, const string &file) const;

   // Add Interaction stuff
   virtual void init_win_cb(WINDOW &win);
   virtual void init_dev_cb(WINDOW &win);
   virtual void init_interact_cb(WINDOW &win);

   virtual void init_menu(WINDOW &win);
   virtual void init_kbd(WINDOW &win);
   virtual void init_kbd_nav(WINDOW &win);
   virtual void init_pens(WINDOW &win);
   virtual void init_cam_manip(WINDOW &win);
   virtual void init_obj_manip(WINDOW &) {}
   virtual void init_fsa();

   //@}

   //! \name FD_TIMEOUT Methods
   //@{

   virtual void         timeout();

   //@}

   //! \name MAPPED_CB Methods
   //@{

   virtual void         mapped();
   virtual void         icon();

   //@}

   static BaseJOTapp*      _instance;
   static vector<WINDOW*>  _windows;

   string                  _prog_name;  // from original argv[0]
   char**                  _argv;
   int                     _argc;
   int                     _wins_to_map;
   


   EaselManager            _easel_manager;
   PenManager*             _pen_manager;
   KeyMenu*                _key_menu;
   WORLDptr                _world;
   vector<ICON2D *>        _buttons; // Array of camera buttons
   private:

   int cam_num; //determines which camera is being used
	      
};

#endif // BASEJOTAPP_H
