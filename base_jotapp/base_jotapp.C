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
#include "disp/colors.H"
#include "geom/gl_view.H"
#include "geom/distrib.H"
#include "geom/hspline.H"
#include "geom/text2d.H"
#include "geom/hspline.H"
#include "geom/texture.H"
#include "gtex/ref_image.H"
#include "manip/cam_pz.H"
#include "manip/cam_fp.H"
#include "manip/cam_edit.H"
#include "mesh/bmesh.H"
#include "mesh/objreader.H"
#include "mlib/points.H"
#include "net/io_manager.H"
#include "std/time.H"
#include "widgets/menu.H"
#include "widgets/fps.H"
#include "widgets/kbd_nav.H"

#include "base_jotapp.H"
#include "npr/image_plate.H"

#ifdef WIN32
//Do we need libgen in WIN32?
#else
#include <libgen.h>       
#endif

using namespace mlib;

/*****************************************************************
 * BaseJOTapp
 *****************************************************************/

//////////////////////////////////////////////////////
// BaseJOTapp Static Variables Initialization
//////////////////////////////////////////////////////

ARRAY<BaseJOTapp::WINDOW *>   BaseJOTapp::_windows  = 0;
BaseJOTapp*                   BaseJOTapp::_instance = 0;

//////////////////////////////////////////////////////
// BaseJOTapp Methods
//////////////////////////////////////////////////////

/////////////////////////////////////
// Constructor
/////////////////////////////////////
BaseJOTapp::BaseJOTapp(int argc, char** argv) :
   _prog_name(argv[0]),
   _argv(argv),
   _argc(argc),
   _pen_manager(0),
   _key_menu(0)
{
   assert(!_instance);

   _instance = this;

   atexit(BaseJOTapp::Clean_On_Exit);
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
BaseJOTapp::BaseJOTapp(Cstr_ptr& name) :
   _prog_name(name),
   _argv(0),
   _argc(0),
   _pen_manager(0),
   _key_menu(0)
{
   assert(!_instance);
   
   _instance = this;

   atexit(BaseJOTapp::Clean_On_Exit);
}

/////////////////////////////////////
// clean_on_exit()
/////////////////////////////////////
void
BaseJOTapp::clean_on_exit() 
{
   if (Config::get_var_bool("DEBUG_CLEAN_ON_EXIT",false))
      cerr << "BaseJOTapp::clean_on_exit" << endl;

   while (!_events.empty()) {
      delete _events.pop();
   }
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
BaseJOTapp::init()
{
   // initialize random number generator:
   srand48((long)the_time());

   init_top();                          // Create top level widget

   init_world();                        // Create world

   for (int i=0;i<_windows.num(); i++) {
      init_view(*_windows[i]);          // Create views,add mapped handler
      init_camera(_windows[i]->_view);
      init_buttons(_windows[i]->_view); // Create on-screen controls
   }
   VIEW::push(&*_windows[0]->_view);    // VIEW #0 is the default view

   init_scene();                        // Create basic scene

   load_scene();                        // Load world, setup networking

   for (int j=0; j<_windows.num(); j++) {
      init_win_cb(*_windows[j]);
      init_dev_cb(*_windows[j]);
      init_interact_cb(*_windows[j]);   // Add Interaction stuff
   }

   init_fsa();                          // activate all interactor FSAs

   // Call this now so OpenGL info gets printed sooner.
   IDRefImage::setup_bits(VIEWS[0]);

   // Add fps counter
   if (!Config::get_var_bool("JOT_SUPPRESS_FPS",false))
      WORLD::timer_callback(new FPS());

   // Print helpful info.
   cerr << "Type 'H' to see the list of key commands" << endl << endl;
}

/////////////////////////////////////
// init_top()
/////////////////////////////////////

void
BaseJOTapp::init_top()
{
   _windows += new_window(WINSYS::create(_argc, _argv));

   _wins_to_map = 1;
   
   //XXX - Obsolete! Way too much code expects
   //      that there's a single window...
  
   int num_wins = Config::get_var_int("JOT_NUM_WINS",1,true);
 
   for (int i = 1; i < num_wins; i++) 
      {
         _windows += new_window(_windows[0]->_win->copy());
      }
   _wins_to_map = num_wins;
}

/////////////////////////////////////
// init_world()
/////////////////////////////////////
void
BaseJOTapp::init_world()
{
   _world = new WORLD();

   WORLD::set_world  (&*_world);
}


/////////////////////////////////////
// create_view()
/////////////////////////////////////
void
BaseJOTapp::init_view(WINDOW &win)
{
   VIEWptr v = new_view(win._win);
   assert(v != NULL);
   v->set_screen(new SCREENbasic(v->cam()));
   v->set_color(Color::get_var_color("BGCOLOR", Color::tan));
   win._view = v;

   if (Config::get_var_bool("USE_STEREO",false,true)) {
      v->stereo(VIEWimpl::TWO_BUFFERS);
   }
   
   _world->schedule(v);
}

/////////////////////////////////////
// new_view()
/////////////////////////////////////
VIEWptr
BaseJOTapp::new_view(WINSYS *win)
{
   // Create a new instance of a view. This is a factory method that
   // allows subclasses to make their own types of views without
   // repeating the code in BaseJOTapp::create_view.

   return new VIEW(str_ptr("OGL View"), win, new GL_VIEW);
}


/////////////////////////////////////
// init_camera()
/////////////////////////////////////
void
BaseJOTapp::init_camera(CVIEWptr &view)
{
   // Set up the camera
   CAMptr cam(view->cam());

   cam->data()->set_focal(0.1);
   cam->data()->set_width(0.1);
   cam->data()->set_height(0.1);

   cam->data()->set_from(Wpt(20, 30, 50));
   cam->data()->set_at  (Wpt::Origin());
   cam->data()->set_up  (Wpt(20, 31, 50));
   
   if (Config::get_var_bool("BIRD_CAM",false,true)) {
      cam->data()->set_from(Wpt(0,0,0));
      cam->data()->set_at  (Wpt(0,0,-1));
      cam->data()->set_up  (Wpt(0,1,0));
   } if (Config::get_var_bool("ORTHO_CAM",false,true)) {
      cam->data()->set_width(50);
      cam->data()->set_height(50);
      cam->data()->set_persp(false);
   }
}

/////////////////////////////////////
// init_buttons()
// create's icon2d Buttons:
// constructor sets name, filename, 
// camera number, toggle, and 
// screen position, respectively
/////////////////////////////////////
void
BaseJOTapp::init_buttons(CVIEWptr &view)
{
   //////////////////////////
   //Cam Switch Button
   //////////////////////////
   //set to "cam 0" since its used in cam1 and cam2
   _buttons.push(
      new ICON2D("eye_button", "nprdata/icons/eye", 0, false, PIXEL(0,0))
      );
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Cam Orbit Button
   //////////////////////////
   //set to cam 2, and initially not displayed
   _buttons.push(
      new ICON2D("orbit", "nprdata/icons/orbit", 2, true, PIXEL(0,32))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Cam Breathe Button
   //////////////////////////
   //set to cam 2 so not initally displayed
   _buttons.push(
      new ICON2D("breathe", "nprdata/icons/breathe", 2, true, PIXEL(0,64))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Cam Cruise Button
   //////////////////////////
   //set to cam 2 so not initally displayed
   _buttons.push(
      new ICON2D("cruise", "nprdata/icons/control", 2, true, PIXEL(0,96))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Cam Grow Button
   //////////////////////////
   //set to cam 2 so not initally displayed
   _buttons.push(
      new ICON2D("grow", "nprdata/icons/grow", 2, true, PIXEL(0,128))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Gravity Buttons
   //////////////////////////
   //set to cam 2 so not initally displayed
   _buttons.push(
      new ICON2D("gravity", "nprdata/icons/gravity1", 2, false, PIXEL(0,160))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   _buttons[0]->add_skin("nprdata/icons/gravity2");
   _buttons[0]->add_skin("nprdata/icons/gravity3");

   //////////////////////////
   //Edit Scale Button
   //////////////////////////
   //set to cam 3 so not initally displayed
   _buttons.push(
      new ICON2D("scale", "nprdata/icons/scale", 3, true, PIXEL(0,32))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);
   
   //////////////////////////
   //Edit Rotate Button
   //////////////////////////
   //set to cam 3 so not initally displayed
   _buttons.push(
      new ICON2D("rotateX", "nprdata/icons/rotatex", 3, true, PIXEL(0,64))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Edit RotateY Button
   //////////////////////////
   //set to cam 3 so not initally displayed
   _buttons.push(
      new ICON2D("rotateY", "nprdata/icons/rotatey", 3, true, PIXEL(0,96))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Edit RotateZ Button
   //////////////////////////
   //set to cam 3 so not initally displayed
   _buttons.push(
      new ICON2D("rotateZ", "nprdata/icons/rotatez", 3, true, PIXEL(0,128))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Edit ScaleX Button
   //////////////////////////
   //set to cam 3 so not initally displayed
   _buttons.push(
      new ICON2D("scalex", "nprdata/icons/scalex", 3, true, PIXEL(0,160))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Edit ScaleY Button
   //////////////////////////
   //set to cam 3 so not initally displayed
   _buttons.push(
      new ICON2D("scaley", "nprdata/icons/scaley", 3, true, PIXEL(0,192))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   //////////////////////////
   //Edit Scale Button
   //////////////////////////
   //set to cam 3 so not initally displayed
   _buttons.push(
      new ICON2D("scalez", "nprdata/icons/scalez", 3, true, PIXEL(0,224))
      );
   _buttons[0]->toggle_suppress_draw();
   NETWORK.set(_buttons[0], 0);
   WORLD::create(_buttons[0], false);

   // make them initially hidden:
   for(int i = 0; i < BaseJOTapp::instance()->_buttons.num(); i++)
      BaseJOTapp::instance()->_buttons[i]->toggle_hidden();
}


/////////////////////////////////////
// init_scene()
/////////////////////////////////////
void
BaseJOTapp::init_scene()
{
   // Init the IOManager before attempting to load anything...
   IOManager::init();
}

void 
BaseJOTapp::print_usage() const
{
   cerr << "Usage: " << endl << "  " << _prog_name
        << " [ -j ] scene.jot" << endl
        << "or: "  << endl << "  " << _prog_name
        << " [ [ -m ] mesh.sm | -o mesh.obj ]+" << endl
        << endl
        << "  There can be just 1 scene file (-j switch), but any number of "
        << endl
        << "  mesh files, in either .sm (native jot) or .obj format."
        << endl << endl;
}

/////////////////////////////////////
// load_scene()
/////////////////////////////////////
void
BaseJOTapp::load_scene()
{
   while (_argc > 1) {
      if (_argv[1][0] == '-') {
         char flag = _argv[1][1];
         pop_arg();
         switch(flag) {
          case 'm':
            load_sm_file(_argv[1]);
            break;
          case 'o':
            load_obj_file(_argv[1]);
            break;
          case 'j':
            load_jot_file(_argv[1]);
            break;
          default:
            print_usage();
            WORLD::Quit();
         }
      } else {
         if (!load_sm_file(_argv[1]))
            load_jot_file(_argv[1]);
      }
      pop_arg();
   }

   if (0) {
      LMESHptr m = new LMESH;
      m->Sphere();
      create_mesh(m, "sphere");
   }

   // Do "viewall" if:
   //   (1) the camera was not specified in file, and
   //   (2) at least one 3D object is outside the view frustum.

   for (int i=0; i<_windows.num(); i++)
      if (_windows[i]->_view->cam()->data()->loaded_from_file())
         return;

   for (int j=0; j<DRAWN.num(); j++) {
      GEOM* geom = GEOM::upcast(DRAWN[j]);
      if (geom && geom->bbox().is_off_screen()) {
         VIEW::peek()->viewall();
         return; // once is enough
      }
   }
}

inline string
strip_ext(const string& base)
{
   // Remove the filename extension, if any
   // (e.g., "bunny.sm" becomes "bunny"):
   return base.substr(0, base.rfind('.'));
}

bool
BaseJOTapp::create_mesh(BMESH* mesh, Cstr_ptr &file) const
{
   // Put the mesh in GEOM and add it to the scene:

   if (!mesh || mesh->empty())
      return false;

   if (Config::get_var_bool("JOT_RECENTER",false))
      mesh->recenter();

   // set unique name for lookup:
   if (!mesh->has_name()) {
#ifdef WIN32
      // XXX - how to get basename on WIN32??
      // A: #include <libgen.h>
      mesh->set_unique_name(strip_ext(**file));
#else
      mesh->set_unique_name(strip_ext(basename(**file)));
#endif
   }

   BMESH::set_focus(mesh);
   WORLD::create(new_geom(mesh, file));

   return true;
}

BMESH* 
BaseJOTapp::new_mesh() const
{
   return new BMESH;
}

GEOM* 
BaseJOTapp::new_geom(BMESH* mesh, Cstr_ptr& name) const
{
   // Default method for creating GEOM or derived type to hold a mesh:

   return new GEOM(name, mesh);
}

bool
BaseJOTapp::load_sm_file(Cstr_ptr &file)
{
   // read an .sm file

   BMESHptr mesh = new_mesh();
   if (!mesh)
      return false;

   mesh->read_file(**file);
   return create_mesh(mesh, file);
}

bool
BaseJOTapp::load_png_file(Cstr_ptr &file)
{
/*   LMESHptr m = new LMESH;
   m->Sphere();
   m->set_render_style("Flat Shading");
   return create_mesh(m, "sphere");*/

   ImagePlateptr im = new ImagePlate(file);

   WORLD::create(im);

   return true;
}

bool
BaseJOTapp::load_obj_file(Cstr_ptr &file)
{
   // read an .obj file

   ifstream obj_stream(**file);
   if (!obj_stream.is_open()) {
      cerr << "BaseJOTapp::load_obj_file: error: couldn't open .obj file"
           << endl;
      return 0;
   }

   OBJReader reader;
   if (!reader.read(obj_stream)) {
      cerr << "BaseJOTapp::load_obj_file: error: couldn't understand .obj file"
           << endl;
      return 0;
   }

   BMESHptr mesh = new_mesh();
   reader.get_mesh(mesh);

   return create_mesh(mesh, file);
}

/////////////////////////////////////
// load_jot_file()
/////////////////////////////////////
bool
BaseJOTapp::load_jot_file(Cstr_ptr &file) 
{
   LOADobs::load_status_t status;

   NetStream s(file, NetStream::ascii_r);

   LOADobs::notify_load_obs(s, status, true, true);

   return (status == LOADobs::LOAD_ERROR_NONE);
}

/////////////////////////////////////
// activate_button()
/////////////////////////////////////
void 
BaseJOTapp::activate_button(Cstr_ptr &file)
{
   for(int i = 0; i < BaseJOTapp::instance()->_buttons.num(); i++) {
      if(BaseJOTapp::instance()->_buttons[i]->name() == file)
         BaseJOTapp::instance()->_buttons[i]->activate();
   }
}

/////////////////////////////////////
// update_button()
/////////////////////////////////////
void 
BaseJOTapp::update_button(Cstr_ptr &file)
{
   for(int i = 0; i < BaseJOTapp::instance()->_buttons.num(); i++) {
      if(BaseJOTapp::instance()->_buttons[i]->name() == file)
         BaseJOTapp::instance()->_buttons[i]->update_skin();
   }
}

/////////////////////////////////////
// deactivate_button()
/////////////////////////////////////
void 
BaseJOTapp::deactivate_button()
{
   for(int i = 0; i < BaseJOTapp::instance()->_buttons.num(); i++)
      BaseJOTapp::instance()->_buttons[i]->deactivate();
}

/////////////////////////////////////
// toggle_button()
/////////////////////////////////////
void 
BaseJOTapp::toggle_button(Cstr_ptr &file)
{
   for(int i = 0; i < BaseJOTapp::instance()->_buttons.num(); i++) {
      if(BaseJOTapp::instance()->_buttons[i]->name() == file)
         BaseJOTapp::instance()->_buttons[i]->toggle_active();
   }
}


/////////////////////////////////////
// init_win_cb()
/////////////////////////////////////
void
BaseJOTapp::init_win_cb(WINDOW &window)
{
   WINSYS *w = window._win;
   w->setup(window._view);          // Set up WINSYS for view
   w->map_cb(this);                 // Get callbacks when this view is mapped

}

/////////////////////////////////////
// init_dev_cb()
/////////////////////////////////////
void
BaseJOTapp::init_dev_cb(WINDOW &window)
{
   WINSYS *w = window._win;

   window._mouse_map.set_devs(&w->mouse()->pointer(), &w->mouse()->buttons());

}


/////////////////////////////////////
// init_interact_cb()
/////////////////////////////////////

void
BaseJOTapp::init_interact_cb(WINDOW &win)
{
   static MoveMenu *menu = 0;

   if (win._view->view_id() == 0){
      init_kbd_nav(win);
      init_kbd(win);
      init_pens(win);
      init_menu(win);
      init_obj_manip(win);
      menu = win._menu;
   } else {
      win._menu = menu;
   }
   
   init_cam_manip(win);
}

/////////////////////////////////////
// init_menu()
/////////////////////////////////////

class RenderSet : public MenuItem {
 protected:
   VIEWptr  _view;
   Cstr_ptr  _render_mode;
 public:
   RenderSet(CVIEWptr &view, Cstr_ptr &m) :
      MenuItem(**m),
      _view(view),
      _render_mode(m) {}
   virtual void exec(CXYpt &) { _view->set_rendering(_render_mode); }
};

inline void
add_render_styles(CVIEWptr& view, MoveMenu* menu)
{
   str_list rend_modes = view->rend_list();
   for (int i=0; i<rend_modes.num(); i++) {
      menu->items() += new RenderSet(view, rend_modes[i]); 
   }
}

void
BaseJOTapp::init_menu(WINDOW &win)
{
   // Get an empty menu:
   win._menu = win._win->menu("Render Mode");

   // Add render style names to menu:
   add_render_styles(win._view, win._menu);


   // hide menu:
   win._menu->hide();

   if (Config::get_var_bool("SHOW_JOT_MENU",true)) {
      win._menu->menu();
      win._menu->move_local(XYpt(-1,0));
      win._menu->show();
   }
}

/////////////////////////////////////
// init_cam_manip()
/////////////////////////////////////
void
BaseJOTapp::init_cam_manip(
   WINDOW       &win)
{

   ButtonMapper *map = &win._mouse_map;   
   State      *start = &win._start;
   VIEWptr     view  =  win._view;
   
   //initialize all cameras

   //unicam
   win._cam1 = new Cam_int(map->b3d(), map->mov(), map->b3u(),
                           map->b1d(Evd::ANY), map->b1u(), 
                           map->b1dC(), map->b2dC(), map->b3dC(),
                           map->b1u(), map->b2u(), map->b3u()
      );

   //first person cam
   win._cam2 = new Cam_int_fp(map->b3d(), map->mov(), map->b3u(),
                              map->b1d(Evd::ANY), map->b1u(), 
                              map->b1dC(), map->b2dC(), map->b3dC(),
                              map->b1u(), map->b2u(), map->b3u()
      );

   win._cam3 = new Cam_int_edit(map->b3d(), map->mov(), map->b3u(),
                                map->b1d(Evd::ANY), map->b1u(), 
                                map->b1dC(), map->b2dC(), map->b3dC(),
                                map->b1u(), map->b2u(), map->b3u()
      );

   //start off using cam 1(the unicam)
   *start += *win._cam1->entry();
}

/////////////////////////////////////
// init_kbd()
/////////////////////////////////////

void
BaseJOTapp::init_kbd(WINDOW &win)
{
   // Make sure we only create one KeyMenu
   // XXX - This may need to be removed at some point if we need to support
   // KeyMenu on multiple windows.
   //assert(_key_menu == 0);
   
   // Create the KeyMenu and link it to the supplied window:
   _key_menu = new KeyMenu(&win._start);
   
   // Add some default keys to the menu:   
   _key_menu->add_menu_item('Y',  "Toggle Buttons",     &button_toggle);
   _key_menu->add_menu_item('H',  "Show this help",     &keymenu_help_cb);
   _key_menu->add_menu_item('M',  "Toggle render menu", &show_menu_cb);
   _key_menu->add_menu_item("qQ", "Quit",               &keymenu_quit_cb);
   _key_menu->add_menu_item('V',  "Viewall",            &viewall);
}

/////////////////////////////////////
// init_pens()
/////////////////////////////////////

void
BaseJOTapp::init_pens(WINDOW &win)
{
   // Make sure we only create one PenManager
   // XXX - This may need to be remove at some point if we need to support
   // Pens on multiple windows.
   assert(_pen_manager == 0);
   
   // Create the PenManager and link it to the supplied window:
   _pen_manager = new PenManager(&win._start);
}

/////////////////////////////////////
// init_kbd_nav()
/////////////////////////////////////

void
BaseJOTapp::init_kbd_nav(WINDOW &win)
{
   new kbd_nav(win._view);
}

/////////////////////////////////////
// init_fsa()
/////////////////////////////////////
void
BaseJOTapp::init_fsa()
{
   for (int i = 0; i < _windows.num(); i++) {
      VIEWint_list::add(_windows[i]->_view, &_windows[i]->_start);
   }
}

/////////////////////////////////////
// Run()
/////////////////////////////////////
void
BaseJOTapp::Run()
{

   if (_windows.num() == 0) {
      cerr << "BaseJOTapp::Run" << endl;
   }

   for (int i=0; i<_windows.num(); i++)  {
      _windows[i]->_win->mouse()->add_handler(
         VIEWint_list::get(_windows[i]->_view)
         );
      _windows[i]->_win->display();
   }

   FD_MANAGER::mgr()->loop();
}


/////////////////////////////////////
// pop_arg()
/////////////////////////////////////
void
BaseJOTapp::pop_arg()
{
   for (int i = 1; i <_argc - 1; i++) {
      _argv[i] = _argv[i+1];
   }
   _argc--;
}


/////////////////////////////////////
// timeout()
/////////////////////////////////////
void
BaseJOTapp::timeout()
{
   // This method is the timeout callback, 
   // which is called periodically to redraw

   _world->poll();      // does nothing (at least for desktop jot)
   _world->tick();      // frame observers get their shot at the CPU
   _world->draw();      // each view gets a draw call
}

/////////////////////////////////////
// mapped()
/////////////////////////////////////
void
BaseJOTapp::mapped()
{
   if (_wins_to_map) {
      if (--_wins_to_map != 0) {
         return;
      }
   }
   FD_MANAGER::mgr()->add_timeout(this);
}

/////////////////////////////////////
// icon()
/////////////////////////////////////
void
BaseJOTapp::icon()
{
   FD_MANAGER::mgr()->rem_timeout(this);
}

/*!
 *  \note To support multiple windows, this code may need to be modified to
 *  determine which window's menu to work with.
 *
 */
bool
BaseJOTapp::menu_is_shown()
{
   
   return (window(0)->_menu) ? window(0)->_menu->is_shown() : false;
   
}

/*!
 *  \copydoc BaseJOTapp::menu_is_shown()
 *
 */
void
BaseJOTapp::show_menu()
{
   
   if(!(window(0)->_menu))
      return;
   
   //_menu->menu(true);
   // XXX -- I changed the above line to the one below.
   // Passing 'true' as a parameter causes the menu 
   // to be recreated every time, which seems unnecessary. (mak)
   window(0)->_menu->menu();
   window(0)->_menu->move_local(XYpt(-1,0));
   window(0)->_menu->show();
   VIEW::peek()->set_focus();
   
}

/*!
 *  \copydoc BaseJOTapp::menu_is_shown()
 *
 */
void
BaseJOTapp::hide_menu()
{
   
   if(window(0)->_menu)
      window(0)->_menu->hide();
   
}

/*!
 *  \copydoc BaseJOTapp::toggle_menu()
 *
 */
void 
BaseJOTapp::toggle_menu() 
{
   if (menu_is_shown())
      hide_menu();
   else
      show_menu();
}

/* KeyMenu Callback Functions */

inline str_ptr
get_name(const State* s)
{
   return (!s ? "null" : s->name() == "" ? "\"\"" : s->name());
}

int
BaseJOTapp::cam_switch(const Event&, State *&)
{

   //get window and start state
   WINDOW *win = (BaseJOTapp::instance()->window(0)); 
   State *start = &win->_start;

   if (Config::get_var_bool("DEBUG_CAM_FSA",false)) {
      cerr << "BaseJOTapp::cam_switch: before switch: start: "
           << "(" << get_name(start) << ")"
           << endl;
      for (int i=0; i<start->arcs().num(); i++) {
         cerr << "  " << get_name(start->arcs()[i].next()) << endl;
      }
      cerr << endl;
   }

   int camNum,lastCam;

   if (BaseJOTapp::instance()->cam_num == 3) {
      //remove cam 3 and replace with cam 1
      cerr << "Switching to UniCam" << endl;
      *start -= *win->_cam3->entry();
      *start += *win->_cam1->entry();
      BaseJOTapp::instance()->cam_num = 1;
      camNum=1;
   } else if (BaseJOTapp::instance()->cam_num == 2) {
      //remove cam 2 and replace with cam 3
      cerr << "Switching to Editing mode" << endl;
      *start -= *win->_cam2->entry();
      *start += *win->_cam3->entry();
      BaseJOTapp::instance()->cam_num = 3;
      camNum=3;
   } else {
      //remove cam 1 and replace with cam 2
      cerr << "Switching to First Person Cam" << endl;
      *start -= *win->_cam1->entry();
      *start += *win->_cam2->entry();
      BaseJOTapp::instance()->cam_num = 2;
      camNum=2;
   }

   for (int i = 0; i < BaseJOTapp::instance()->_buttons.num(); i++) {
      if (camNum-1==0)lastCam=3; else lastCam=camNum-1;
      if (BaseJOTapp::instance()->_buttons[i]->cam_num() == camNum ||
          BaseJOTapp::instance()->_buttons[i]->cam_num() == lastCam )
         BaseJOTapp::instance()->_buttons[i]->toggle_suppress_draw();
   }

   if (Config::get_var_bool("DEBUG_CAM_FSA",false)) {
      cerr << "BaseJOTapp::cam_switch: after switch: start: "
           << "(" << get_name(start) << ")"
           << endl;
      for (int i=0; i<start->arcs().num(); i++) {
         cerr << "  " << get_name(start->arcs()[i].next()) << endl;
      }
      cerr << endl;
   }

   return 0;
}

//toggles buttons to display or not display
int
BaseJOTapp::button_toggle(const Event&, State *&)
{
   for(int i = 0; i < BaseJOTapp::instance()->_buttons.num(); i++)
      BaseJOTapp::instance()->_buttons[i]->toggle_hidden();

   return 0;
}


int
BaseJOTapp::keymenu_help_cb(const Event&, State *&)
{
   
   cerr << "Help Menu: mapping of keys to their functions:" << endl;
   
   BaseJOTapp::instance()->_key_menu->display_menu(cerr);
   
   return 0;
}

int
BaseJOTapp::show_menu_cb(const Event&, State *&)
{
   instance()->toggle_menu();

   return 0;
}

int
BaseJOTapp::viewall(const Event&, State *&)
{
   VIEW::peek()->viewall();

   return 0;
}

int
BaseJOTapp::keymenu_quit_cb(const Event&, State *&)
{
   
   if(Config::get_var_bool("DEBUG_STRPOOL",false))
      err_msg("strpool load factor: %f", STR::load_factor());

   WORLD::Quit();

   return 0;
}

// end of file base_jotapp.C
