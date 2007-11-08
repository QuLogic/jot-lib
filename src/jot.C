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
#include "std/fstream.H"
#include "dev/tablet.H"
#include "disp/animator.H"
#include "disp/recorder.H"
#include "ffs/cursor3d.H"
#include "ffs/draw_manip.H"
#include "ffs/xform_pen.H"
#include "ffs/draw_pen.H"
#include "ffs/floor.H"
#include "ffs/trace.H"
#include "ffs/profile.H"
#include "ffs/sweep.H"
#include "geom/distrib.H"
#include "geom/recorder_ui.H"
#include "geom/texture.H"
#include "geom/world.H"
#include "gest/gest_int.H"
#include "gest/patch_pen.H"
#include "gtex/ref_image.H"
#include "gtex/buffer_ref_image.H"
#include "gtex/curvature_ui.H"
#include "gtex/fader_texture.H"
#include "gtex/flat_shade.H"
#include "gtex/normals_texture.H"
#include "gtex/key_line.H"
#include "gtex/sil_frame.H"
#include "gtex/wireframe.H"
#include "manip/cam_pz.H"
#include "manip/cam_fp.H"
#include "manip/cam_edit.H"
#include "mesh/hybrid.H"
#include "mesh/lmesh.H"
#include "mesh/objreader.H"
#include "mesh/patch.H"
#include "mlib/points.H"

using namespace mlib;

#include "npr/npr_control_frame.H"
#include "npr/npr_texture.H"
#include "npr/npr_view.H"
#include "pattern/pattern_pen.H"
#include "proxy_pattern/proxy_pen.H"
#include "std/run_avg.H"
#include "std/stop_watch.H"
#include "std/support.H"
#include "std/time.H"
#include "tess/skin.H"
#include "tess/panel.H"
#include "tess/tex_body.H"
#include "widgets/alert_box.H"
#include "widgets/file_select.H"
#include "widgets/fps.H"
#include "widgets/menu.H"
#include "widgets/collide.H"
#include "wnpr/hatching_pen.H"
#include "wnpr/line_pen.H"
#include "wnpr/npr_pen.H"
#include "wnpr/sil_ui.H"
#include "wnpr/sky_box.H"
#include "wnpr/view_ui.H"
#include "wnpr/halo_sphere.H"
#include "base_jotapp/base_jotapp.H"
#include "gui/img_line_ui.H"

/* KeyMenu Callback Function Prototypes */

int bk_camera(const Event& ev, State *& s);
int fwd_camera(const Event& ev, State *& s);

int next_texture(const Event&, State *&);

int next_ref_img(const Event&, State *&);

int rotate_camera(const Event& ev, State *&);

int toggle_buffer(const Event&, State *&);

int view_ui_toggle(const Event&, State *&);
int sil_ui_toggle(const Event&, State *&);

int toggle_antialias(const Event&, State *&);
int next_antialias(const Event&, State *&);

int toggle_paper(const Event&, State *&);

int freeze_sils(const Event&, State *&);

int toggle_random_sils(const Event&, State *&);

int toggle_hidden_lines(const Event&, State *&);

int toggle_floor(const Event&, State *&);

int debug_cb(const Event& ev, State *&);

void do_clear();

int animation_keys(const Event &e, State *&s);

int render_mode(const Event &e, State *&s);

int toggle_recorder(const Event &, State *&);
int rec_play(const Event &, State *&);
int rec_rec(const Event &, State *&);
int rec_stop(const Event &, State *&);
int rec_pause(const Event &, State *&);

int npr_select_mode(const Event &, State *&);

int toggle_repair(const Event &, State *&);

int set_pen(const Event & ev, State *&);
int pen_key(const Event &e, State *&);

int undo_redo(const Event &e, State *&);

int quit(const Event&, State *&);

int refine(const Event&, State *&);

int cycle_subdiv_loc_calc(const Event&, State *&);

int set_tuft_group(const Event&, State *&);

int clear_selections(const Event&, State *&);

int switch_rep(const Event&, State *&);

int switch_mode(const Event&, State *&);

int unrefine(const Event&, State *&);

int toggle_show_memes(const Event&, State *&);

int toggle_sil_frame(const Event&, State *&);

int toggle_transp(const Event&e, State *&);

int write(const Event&, State *&);

int print_mesh(const Event&, State *&);

int write_xformed(const Event&, State *&);

int save_config(const Event &e, State *&);

int screen_grab(const Event&, State *&);
int rotate_screen_grab(const Event& e, State *&s);

int clear_creases(const Event&, State*&);

int toggle_nulubo(const Event&, State*&);

int toggle_no_text(const Event&, State*&);

int inc_edit_level(const Event&, State*&);
int dec_edit_level(const Event&, State*&);

int toggle_show_quad_diagonals(const Event&, State *&);

int toggle_show_secondary_faces(const Event&, State *&);

int recreate_creases(const Event&, State *&);
int toggle_crease(const Event &, State *&);

int split_mesh(const Event &e, State *&);

int kill_component(const Event &, State *&);

int create_cam_path(const Event&, State *&);

int print_key_menu(const Event&, State *&);

int toggle_trace(const Event&, State*&);

int trace_calibrate(const Event&, State*&);

int bind_trace(const Event& ev, State*&);

int toggle_curvature_ui(const Event&, State *&);

int inc_aoi(const Event&, State *&);
int dec_aoi(const Event&, State *&);

int toggle_img_line_ui(const Event&, State *&);

/**********************************************************************
 * JOTapp Class
 **********************************************************************/

class JOTapp : public BaseJOTapp {

 public:
   /******** PUBLIC MEMBER CLASSES ********/

   class WINDOWjot : public BaseJOTapp::WINDOW {
    public:
      State          _otherstart;

      WINDOWjot(WINSYS *win) : WINDOW(win) {}};

 protected:
   /******** STATIC MEMBER VARIABLES ********/

   /******** MEMBER VARIABLES ********/

   TabletMultimode*     _tablet;
   ButtonMapper*        _tab_map;

   SKY_BOXptr           _sky_box;
   GEST_INTptr          _gest_int;
   DrawManip*           _draw_manip;
   HaloBase*            _halo_maker;

   DrawPen*             _draw_pen;
   LinePen*             _line_pen;

   /******** STATIC MEMBER METHODS ********/

   static void init_fade(GTexture* base, GTexture* fader,
                         double start, double dur) {
      new FaderTexture(base, fader, start, dur);
   }

   static void splash_cb(void *j, void *jd, int, int) {
      ((JOTapp *)j)->_windows[0]->_view->set_focus();
   }

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   JOTapp(int argc, char **argv) :
      BaseJOTapp(argc, argv), _tablet(0), _tab_map(0), _gest_int(0),
      _draw_manip(0), _draw_pen(0), _line_pen(0)
      {}

   //******** LOADING ********
   static int save_cb(const Event&, State *&);
   static int load_cb(const Event&, State *&);
   static int load_image_cb(const Event&, State *&);
   static void file_cbs(void *ptr, int i, int action, str_ptr path, str_ptr file);
   static void do_save(str_ptr fullpath);
   static void do_load(str_ptr fullpath);
   static void do_load_image(str_ptr fullpath);
   static void alert_cbs(void *ptr, void *dptr, int idx, int but_idx);
   static int clear_cb(const Event&, State *&);

   /******** BaseJOTapp METHODS ********/
 protected:

   virtual WINDOW*   new_window(WINSYS *win) { return new WINDOWjot(win);}
   virtual VIEWptr   new_view(WINSYS *win) {
      return new VIEW(str_ptr("NPR View"), win, new NPRview);
   }

   virtual void      init_scene();

   // JOTapp uses LMESHes (supporting subdivision) instead of plain BMESHes:
   virtual BMESH* new_mesh() const { return new LMESH; }

   // JOTapp uses TEXBODYs instead of plain GEOMs:
   virtual GEOM* new_geom(BMESH* mesh, Cstr_ptr& name) const {
      return new TEXBODY(mesh, name);
   }

   virtual void      init_dev_cb(WINDOW &base_window);
   virtual void      init_interact_cb(WINDOW &base_window);

   virtual void      init_kbd(WINDOW &base_window);
   virtual void      init_pens(WINDOW &base_window);

   virtual void      init_cam_manip(WINDOW &win);

   virtual void      init_fsa();

 public:
   virtual void init() {
      srand48((long)the_time());

      BaseJOTapp::init();

      // XXX - Fade between GTextures? Buggy... OFF by default.
      if ( Config::get_var_bool("FADE_TEXTURES",false)) {
         Patch::_init_fade = &init_fade;
      }

      static bool NO_SPLASH = Config::get_var_bool("NO_SPLASH",true);
      if (!NO_SPLASH) {
         AlertBox *box = _windows[0]->_win->alert_box();
         assert(box);
         box->set_title("");
         box->set_icon(AlertBox::JOT_ICON);
         box->add_text("Jot v1.0.1");
         box->add_text("http://jot.cs.princeton.edu");
         box->add_text("(c)2004 All Rights Reserved");
         box->add_button("OK");
         box->set_default(0);
         if (box->display(false, splash_cb, this, NULL, 0)) {
            err_mesg(ERR_LEV_SPAM, "JOTapp::init() - AlertBox displayed.");
         } else {
            err_msg("JOTapp::init() - AlertBox **FAILED** to display!!");
         }
      } else {
         //Puts focus on main window -- in case some
         //GLUI window tries to take initial focus.
         //If the splash alert box shows, this is called
         //after the popup is dismissed instead.
         splash_cb(this,NULL,0,0);
      }
   }
};

/*****************************************************************
 * JOTapp Methods
 *****************************************************************/

/////////////////////////////////////
// init_scene()
/////////////////////////////////////
void
JOTapp::init_scene()
{
   int i;

   //Add fps and keyboard navigator
   BaseJOTapp::init_scene();

   // Set rendering style to smooth shading unless an environment
   // variable gives a different default:
   for (i=0; i<_windows.num(); i++) {
      _windows[i]->_view->set_rendering(
         Config::get_var_str("JOT_RENDER_STYLE", **RSMOOTH_SHADE));
   }

}

/////////////////////////////////////
// init_dev_cb()
/////////////////////////////////////
void
JOTapp::init_dev_cb(WINDOW &base_window)
{
   BaseJOTapp::init_dev_cb(base_window);

   // This code sets up jot to read tablet events directly through
   // the serial port via the built-in tablet driver code (in jot).
   // Only use the following code if window system is NOT running
   // its own tablet driver.
   if (Config::get_var_str("USE_TABLET",NULL_STR,true) != NULL_STR) {
      str_ptr tab_type = Config::get_var_str("USE_TABLET",NULL_STR,true);

      _tablet = new TabletMultimode(
         FD_MANAGER::mgr(),
         tab_type==str_ptr("CROSS")  ? TabletDesc::CROSS        :
         tab_type==str_ptr("LCD")    ? TabletDesc::LCD          :
         tab_type==str_ptr("LARGE")  ? TabletDesc::MULTI_MODE   :
         tab_type==str_ptr("TINY")   ? TabletDesc::TINY         :
         tab_type==str_ptr("INTUOS") ? TabletDesc::INTUOS       :
         TabletDesc::SMALL);
      _tablet->activate();
      _tab_map = new ButtonMapper(&_tablet->stylus(),&_tablet->stylus_buttons());
      _tablet->add_handler(VIEWint_list::get(&*base_window._view));
      _tablet->stylus().add_handler(base_window._win->curspush() );
   }

}

/////////////////////////////////////
// init_interact_cb()
/////////////////////////////////////
void
JOTapp::init_interact_cb(WINDOW &base_window)
{
   BaseJOTapp::init_interact_cb(base_window);

//   WINDOWjot &win = (WINDOWjot &) base_window;

//    init_draw_int(win);
}

/////////////////////////////////////
// init_kbd()
/////////////////////////////////////

void
JOTapp::init_kbd(WINDOW &base_window)
{
   bool do_ffs     =  Config::get_var_bool("ENABLE_FFS",false);
   bool do_wnpr    = !Config::get_var_bool("SUPPRESS_NPR",true);

   // add a mess of key commands (in alphabetical order!):

   // note: BaseJOTapp::init_kbd() also defines key callbacks
   BaseJOTapp::init_kbd(base_window);

   if (do_ffs) {
//       _key_menu->add_menu_item('\t', "<tab key> Next easel", &next_easel);
      _key_menu->add_menu_item(']', "Fwd in camera history", &fwd_camera);
      _key_menu->add_menu_item('[', "Back in camera history", &bk_camera);
      _key_menu->add_menu_item('{', "Toggle trace widget",    &toggle_trace);
      _key_menu->add_menu_item('}', "Calibrate trace widget", &trace_calibrate);
      _key_menu->add_menu_item('|', "Bind trace to easel", &bind_trace);
   }
   _key_menu->add_menu_item(' ', "<space key> Toggle sil frame", &toggle_sil_frame);
   _key_menu->add_menu_item('a', "Toggle antialias", &toggle_antialias);
   _key_menu->add_menu_item('A', "Toggle next antialias", &next_antialias);
   _key_menu->add_menu_item('b', "Toggle buffer", &toggle_buffer);
   if (do_ffs) {
      _key_menu->add_menu_item('B', "toggle secondary faces", &toggle_show_secondary_faces);
   }
   _key_menu->add_menu_item('c', "Clear (delete) all objects", &JOTapp::clear_cb);
   // key 'C' falls into AnimationKeys
   _key_menu->add_menu_item('D', "Toggle no_text", &toggle_no_text);
   _key_menu->add_menu_item('d', "Debug callback", &debug_cb);
   if (do_ffs) {
      _key_menu->add_menu_item('e', "Decrement mesh edit level", &dec_edit_level);
      _key_menu->add_menu_item('E', "Increment mesh edit level", &inc_edit_level);
   }
   _key_menu->add_menu_item('f', "Toggle floor", &toggle_floor);
   _key_menu->add_menu_item('F', "Freeze sils", &freeze_sils);
   _key_menu->add_menu_item('g', "Grab screen", &screen_grab);
   _key_menu->add_menu_item('G', "Rotate screen grab", &rotate_screen_grab);
   // key 'h' is piped to pen_key
//    _key_menu->add_menu_item('H', "Help Menu for key commands", &print_key_menu);
   _key_menu->add_menu_item('i', "Recreate Creases", &recreate_creases);
   _key_menu->add_menu_item('I', "Toggle show quad diagonals", &toggle_show_quad_diagonals);
   // key 'j' is piped to pen_key
   _key_menu->add_menu_item('J', "Debug cb", &debug_cb);
   _key_menu->add_menu_item('L', "Debug callback", &debug_cb);
   _key_menu->add_menu_item('k', "Clear creases", &clear_creases);
   _key_menu->add_menu_item('K', "Load an image", &JOTapp::load_image_cb);
   _key_menu->add_menu_item('l', "Load File", &JOTapp::load_cb);
   if (do_ffs) {
      _key_menu->add_menu_item('m', "Toggle show memes", &toggle_show_memes);
   }
   if (do_ffs) {
//       _key_menu->add_menu_item('n', "New easel", &new_easel);
   }
   _key_menu->add_menu_item('N', "Next texture", &next_texture);
   if (do_wnpr) {
      _key_menu->add_menu_item('o', "Toggle sil ui ", &sil_ui_toggle);
   }
   _key_menu->add_menu_item('p', "Print mesh statistics", &print_mesh);
   if (do_wnpr) {
      _key_menu->add_menu_item('P', "Toggle paper", &toggle_paper);
   }
   _key_menu->add_menu_item('R', "Next reference image", &next_ref_img);
   _key_menu->add_menu_item('r', "Rotate", &rotate_camera);
   _key_menu->add_menu_item('s', "Save", &save_cb);
   _key_menu->add_menu_item('S', "Cycle subdivision loc calc", &cycle_subdiv_loc_calc);
   _key_menu->add_menu_item("t", "Refine mesh", &refine);
   if (do_ffs) {
      _key_menu->add_menu_item('u', "Unrefine mesh", &unrefine);
   }
   _key_menu->add_menu_item('U', "Unrefine mesh", &unrefine);
   _key_menu->add_menu_item('v', "View ui toggle", &view_ui_toggle);
   _key_menu->add_menu_item('w', "Write mesh", &write);
   _key_menu->add_menu_item('W', "Write_xformed mesh", &write_xformed);
   // key 'X' is used below by the animation controls
   // key 'y' is piped to pen_key
   _key_menu->add_menu_item('z', "Debug_cb", &debug_cb);
   _key_menu->add_menu_item('Z', "Toggle hidden lines in Key Line rendering style",
                            &toggle_hidden_lines);
   _key_menu->add_menu_item(",.", "Set pen", &set_pen);

   _key_menu->add_menu_item('=', "Toggle random sils", &toggle_random_sils);

   // This method handles all key presses to the view's
   // recorder and animator
   _key_menu->add_menu_item("CX/*-+24568", "Animation keys",  &animation_keys);

   if (do_wnpr) {
      _key_menu->add_menu_item("'", "NPR select mode", &npr_select_mode);
      _key_menu->add_menu_item("`", "Toggle stroke intersect repair mode",&toggle_repair);
   }

   _key_menu->add_menu_item("\r\n\x8\x7f", "Undo Redo", &undo_redo);

   _key_menu->add_menu_item("!@#", "Render mode", &render_mode);

   // The set of keys that get piped into the active pen
   if (do_wnpr) {
      _key_menu->add_menu_item("hjuy;:~\x1b", "Pen key", &pen_key);
   }
   if (do_ffs) {
      _key_menu->add_menu_item("$", "Clear selections", &clear_selections);
      if (!do_wnpr) {
         _key_menu->add_menu_item('h', "Switch between ribbon and paperdoll", &switch_rep);
         _key_menu->add_menu_item('j', "Switch between revolve mode and current mode", &switch_mode);
      }
   }
   _key_menu->add_menu_item("?", "Debug_cb", &debug_cb);

   _key_menu->add_menu_item("%", "Toggle Curvature gTexture UI", &toggle_curvature_ui);

   bool do_img_line_ui = Config::get_var_bool("ENABLE_IMG_LINE_TEXTURE", true)
      || Config::get_var_bool("ENABLE_IMAGE_LINE_SHADER", true)
      || Config::get_var_bool("ENABLE_RIDGE_SHADER", true);

   if (do_img_line_ui) {
      _key_menu->add_menu_item('^', "Toggle ImageLine UI", &toggle_img_line_ui);
   }

   // this set of keys only apply to the "profile" widget in FFS
   if (do_ffs) {
      _key_menu->add_menu_item(":", "increase area of interest", &inc_aoi);
      _key_menu->add_menu_item("_", "decrease area of interest", &dec_aoi);
   }

}

/////////////////////////////////////
// init_pens()
/////////////////////////////////////

void
JOTapp::init_pens(WINDOW &base_window)
{
   bool do_ffs     =  Config::get_var_bool("ENABLE_FFS",false);
   bool do_wnpr    = !Config::get_var_bool("SUPPRESS_NPR",true);
   bool do_proxy   =  Config::get_var_bool("ENABLE_PROXY",false);
   bool do_pattern =  Config::get_var_bool("ENABLE_PATTERN",false);

   BaseJOTapp::init_pens(base_window);

   ButtonMapper *map = _tab_map ? _tab_map : &base_window._mouse_map;

   // Events for Pens:
   const Event down = map->b1d();
   const Event move = map->mov();
   const Event up   = map->b1u();

   const Event b2d = map->b2d();
   const Event b2u = map->b2u();

   // XXX - obsolete:
   Event shift_down = map->b1dS();
   Event shift_up   = map->b1u();
   Event ctrl_down  = map->b1dC();
   Event ctrl_up    = map->b1u();

   // create sky box (it is hidden by default):
   _sky_box = new SKY_BOX();

   // XXX - needed?
   _halo_maker = new HaloSphere(); //global halo maker

   // set up gesture interactor to process mouse/stylus events into
   // gestures:
   _gest_int = new GEST_INT(base_window._view, down, move, up, b2d, b2u);

   // set up the DrawManip for manipulating objects:
   _draw_manip = new DrawManip(b2d, move, b2u);
   base_window._start += *(_draw_manip->entry());

   // save events so Cursor3D can create new instances later:
   Cursor3D::cache_events(b2d, move, b2u);

   // create floor, but hide it until needed:
   FLOOR* floor = new FLOOR(b2d, move, b2u);
   assert(floor);
   floor->hide();

   // Add "Pens" (i.e. interaction modes):

   XformPen* xf_pen = new XformPen(_gest_int, down, move, up);
   assert(xf_pen);
   _pen_manager->add_pen(xf_pen);
   _pen_manager->select_pen(xf_pen);

   // Free-From Sketch Pens:
   if (do_ffs) {
      _draw_pen = new DrawPen(_gest_int, down, move, up);
      _pen_manager->add_pen(_draw_pen);
      _pen_manager->select_pen(_draw_pen);
      floor->show();
   }

   // WYSIWYG NPR Pens:
   if (do_wnpr) {

      LinePen* linepen;
      linepen = new LinePen(
         _gest_int, down, move, up,
         shift_down, shift_up, ctrl_down, ctrl_up
         );
      _line_pen = linepen;

      _pen_manager->add_pen(linepen);

      HatchingPen* hatchpen = new HatchingPen(
         _gest_int, down, move, up,
         shift_down, shift_up, ctrl_down, ctrl_up
         );
      _pen_manager->add_pen(hatchpen);

      NPRPen* nprpen = new NPRPen(
         _gest_int, down, move, up,
         shift_down, shift_up, ctrl_down, ctrl_up
         );
      _pen_manager->add_pen(nprpen);

      // Start with DrawPen unless otherwise requested
      if (Config::get_var_bool("START_WITH_LINE_PEN",true,true))
         _pen_manager->select_pen(linepen);
      if (Config::get_var_bool("START_WITH_HATCHING_PEN",false))
         _pen_manager->select_pen(hatchpen);
      if (Config::get_var_bool("START_WITH_BASECOAT_PEN",false))
         _pen_manager->select_pen(nprpen);
   }

   // Pattern Pens:
   if (do_pattern) {
      PatternPen* patternpen =
         new PatternPen(_gest_int, down, move, up,
                        shift_down, shift_up, ctrl_down, ctrl_up);
      _pen_manager->add_pen(patternpen);
      _pen_manager->select_pen(patternpen);

   }

   // turn on patch pen when requested, or when using proxy pen:
   bool do_patch_pen =
      Config::get_var_bool("DEBUG_PATCH_PEN", false) || do_proxy;
   if (do_patch_pen) {
      // create a patch pen so patches can be edited:
      _pen_manager->add_pen(new PatchPen(_gest_int, down, move, up));
   }
   if (do_proxy) {
      ProxyPen* proxy_pen =
         new ProxyPen(_gest_int, down, move, up,
                      shift_down, shift_up, ctrl_down, ctrl_up);
      _pen_manager->add_pen(proxy_pen);
      _pen_manager->select_pen(proxy_pen);

   }
   if (Config::get_var_bool("JOT_SUPPRESS_TEXT",false))
      TEXT2D::toggle_suppress_draw();
}

/////////////////////////////////////
// init_cam_manip()
/////////////////////////////////////
void
JOTapp::init_cam_manip(WINDOW &win)
{

   Event down;
   Event move;
   Event up;

   // Again, only use the following code if window system is
   // NOT running its own tablet driver.
   if (Config::get_var_str("USE_TABLET",NULL_STR) != NULL_STR) {
      down = _tab_map->b4d();
      move = _tab_map->mov();
      up   = _tab_map->b4u();
   } else {
      down = win._mouse_map.b3d();
      move = win._mouse_map.mov();
      up   = win._mouse_map.b3u();
   }

   ButtonMapper *map = &win._mouse_map;

   win._cam1 = new Cam_int(down, move, up,   map->b1d(Evd::ANY), map->b1u(),
                           map->b1dC(), map->b2dC(), map->b3dC(),
                           map->b1u(),  map->b2u(),  map->b3u());

   win._cam2 = new Cam_int_fp(down, move, up,   map->b1d(Evd::ANY), map->b1u(),
                              map->b1dC(), map->b2dC(), map->b3dC(),
                              map->b1u(),  map->b2u(),  map->b3u());

   win._cam3 = new Cam_int_edit(down, move, up,   map->b1d(Evd::ANY), map->b1u(),
                                map->b1dC(), map->b2dC(), map->b3dC(),
                                map->b1u(),  map->b2u(),  map->b3u());
   win._start += *win._cam1->entry();
}

/////////////////////////////////////
// init_fsa()
/////////////////////////////////////
void
JOTapp::init_fsa()
{
   BaseJOTapp::init_fsa();

   for (int i = 0; i < _windows.num(); i++) {
      WINDOWjot *winjot = (WINDOWjot *) _windows[i];
      VIEWint_list::add(_windows[i]->_view, &winjot->_otherstart);
   }
}

/**********************************************************************
 * main()
 **********************************************************************/
int
main(int argc, char **argv)
{
   JOTapp app(argc, argv);

   app.init();

   app.Run();

   return 0;
}

//============================================================================//

/* KeyMenu Callback Functions */

// Dialog box callback constants:

enum alert_cb_t
{
   ALERT_CLEAR_CB = 0,
   ALERT_SAVE_JOT_OVERWRITE_CB,
   ALERT_SAVE_JOT_FAILED_CB,
   ALERT_LOAD_JOT_FAILED_CB
};

enum file_cb_t
{
   FILE_SAVE_JOT_CB = 0,
   FILE_LOAD_JOT_CB,
   FILE_LOAD_IMAGE_CB
   //FILE_SAVE_SM_CB, etc.
};

// find_mesh:
//    (Convenience method.)  Some key callbacks operate on a
//    specific mesh.  Here we return the mesh that is currently
//    the "focus", or (if none), the one that is currently under
//    the cursor.
inline BMESH*
find_mesh()
{
   BMESH* ret = BMESH::focus();
   return ret ? ret : VisRefImage::get_mesh();
}

inline BMESH*
find_ctrl_mesh()
{
   return get_ctrl_mesh(find_mesh());
}

int
bk_camera(const Event& ev, State *& s)
{
   ((VIEWptr) (ev.view()))->bk_cam_hist();
   return 0;
}

int
fwd_camera(const Event& ev, State *& s)
{
   ((VIEWptr) (ev.view()))->fwd_cam_hist();
   return 0;
}

int
next_texture(const Event&, State *&)
{
   Patch* patch = VisRefImage::get_ctrl_patch();

   if (patch) {
      patch->next_texture();
      WORLD::message(patch->cur_tex()->class_name());
   }

   return 0;
}


int
next_ref_img(const Event&, State *&)
{
   NPRview::next_ref_img();

   return 0;
}


int
rotate_camera(const Event& ev, State *&)
{
   CAMptr      cam (ev.view()->cam());
   CAMdataptr  data(cam->data());

   cam->set_zoom(1);
   cam->set_min(NDCpt(XYpt(-1,-1)));
   cam->data()->changed();

   XYpt        cpt   = data->center();
   double      radsq = sqr(1+fabs(cpt[0])); // squared rad of virtual cylinder

   //Hacking - Toss in some XY pts
   //to simulate a mouse movement
   //that produces rotation...
   //XYpt        tp    = ptr->old();
   //XYpt        te    = ptr->cur();
   XYpt        tp    = XYpt(0.485, 0.5);
   XYpt        te    = XYpt(0.5,0.5);


   Wvec   op  (tp[0], 0, 0);             // get start and end X coordinates
   Wvec   oe  (te[0], 0, 0);             //    of cursor motion
   double opsq = op * op, oesq = oe * oe;
   double lop  = opsq > radsq ? 0 : sqrt(radsq - opsq);
   double loe  = oesq > radsq ? 0 : sqrt(radsq - oesq);
   Wvec   nop  = Wvec(op[0], 0, lop).normalized();
   Wvec   noe  = Wvec(oe[0], 0, loe).normalized();
   double dot  = nop * noe;

   if (fabs(dot) > 0.0001) {
      data->rotate(Wline(data->center(), Wvec::Y()),
                   -2*Acos(dot) * Sign(te[0]-tp[0]));

      double rdist = te[1]-tp[1];

      CAMdata   dd = CAMdata(*data);

      Wline raxe(data->center(),data->right_v());
      data->rotate(raxe, rdist);
      data->set_up(data->from() + Wvec::Y());
      if (data->right_v() * dd.right_v() < 0)
         *data = dd;
   }

   return 0;
}


int
toggle_buffer(const Event& ev, State *&)
{
   BufferRefImage *buf = BufferRefImage::lookup(ev.view());
   if (buf) {
      if (buf->is_observing()) {
         cerr << "DrawInt: BufferRefImage was observing -- Toggling OFF..\n";
         buf->unobserve();
      } else {
         cerr << "DrawInt: BufferRefImage was NOT observing -- Toggling ON...\n";
         buf->observe();
      }
   }

   return 0;
}

int
view_ui_toggle(const Event&, State *&)
{
   if (ViewUI::is_vis(VIEW::peek())) {
      ViewUI::hide(VIEW::peek());
   } else {
      ViewUI::show(VIEW::peek());
      VIEW::peek()->set_focus();
   }

   return 0;
}

int
sil_ui_toggle(const Event&, State *&)
{

   if (SilUI::is_vis(VIEW::peek())) {
      SilUI::hide(VIEW::peek());
   } else {
      SilUI::show(VIEW::peek());
      VIEW::peek()->set_focus();
   }

   return 0;
}

int
toggle_antialias(const Event&, State *&)
{

   int a = VIEW::peek()->get_antialias_enable();

   VIEW::peek()->set_antialias_enable(!a);

   if (VIEW::peek()->get_antialias_enable()) {
      WORLD::message(str_ptr("Antialiasing: ENALBED Jitters: ") +
                     str_ptr(VIEW::get_jitter_num(VIEW::peek()->get_antialias_mode())));
   } else {
      WORLD::message(str_ptr("Antialiasing: DISABLED Jitters: ") +
                     str_ptr(VIEW::get_jitter_num(VIEW::peek()->get_antialias_mode())));
   }

   if (ViewUI::is_vis(VIEW::peek())) {
      ViewUI::update(VIEW::peek());
   }

   return 0;
}

int
next_antialias(const Event&, State *&)
{

   int m = VIEW::peek()->get_antialias_mode();

   VIEW::peek()->set_antialias_mode((m+1)%VIEW::get_jitter_mode_num());

   if (VIEW::peek()->get_antialias_enable()) {
      WORLD::message(str_ptr("Antialiasing: ENALBED Jitters: ") +
                     str_ptr(VIEW::get_jitter_num(VIEW::peek()->get_antialias_mode())));
   } else {
      WORLD::message(str_ptr("Antialiasing: DISABLED Jitters: ") +
                     str_ptr(VIEW::get_jitter_num(VIEW::peek()->get_antialias_mode())));
   }

   if (ViewUI::is_vis(VIEW::peek())) {
      ViewUI::update(VIEW::peek());
   }

   return 0;
}

int
toggle_paper(const Event&, State *&)
{

   PaperEffect::toggle_active();

   if (ViewUI::is_vis(VIEW::peek())) {
      ViewUI::update(VIEW::peek());
   }

   return 0;
}

int
freeze_sils(const Event&, State *&)
{
   BMESH::_freeze_sils = !BMESH::_freeze_sils;

   return 0;
}

int
toggle_random_sils(const Event&, State *&)
{
   BMESH::toggle_random_sils();
   char msg[256];
   sprintf(msg, "Randomized silhouettes: %s",
           BMESH::_random_sils ? "ON" : "OFF");
   WORLD::message(msg);
   return 0;
}

int
toggle_hidden_lines(const Event&, State *&)
{
   KeyLineTexture::toggle_show_hidden_lines();

   return 0;
}

int
toggle_floor(const Event&, State *&)
{
//    // don't bother with the floor unless the draw pen is active:
//    if (_draw_pen && _cur_pen == _draw_pen)

   FLOOR::toggle();

   return 0;
}

// temporary debugging function
inline bool
is_sec(Bface* f)
{
   return f && f->is_secondary();
}

// temporary debugging class
class BadEdgeFilter : public SimplexFilter
{
 public:
   virtual bool accept(CBsimplex* s) const
      {
         if (!is_edge(s))
            return false;
         Bedge* e = (Bedge*)s;
         return (e->is_multi() && (is_sec(e->f1()) || is_sec(e->f2())));
      }
};

/*****************************************************************
 * avg_bface_pix_area()
 *
 *   stuff for measuring average apparent size of triangles
 *   in PIXELS. invoked in debug_cb() below.
 *****************************************************************/
inline Wpt
map_centroid(CBface* f, CWpt& eye, CWvec& t)
{
   // Return the point along the line of sight that is the
   // same distance from the "eye" as the distance to the
   // centroid of the face.

   assert(f);
   Wpt c = (f->is_quad() ? f->quad_centroid() : f->centroid());
   return eye + t*c.dist(eye);
}

inline double
get_area(CBface* f)
{
   return (f->is_quad() ? (f->quad_area()/2) : f->area());
}

inline double
normalized_pix_area(CBface* f, CWpt& eye, CWvec& t, CWvec& x, CWvec& y)
{
   //    c ---------- .
   //    |            |
   //    |            |
   //    |     o      |
   //    |            |
   //    |            |
   //    a ---------- b
   //
   // In above diagram, o is center of screen,
   // the square is parallel w/ the image plane
   // and has the same area as f. We project
   // the corners a, b, c into PIXEL space and
   // return the area of the square.

   // XXX - ignoring mesh xform

   if (!(f && f->mesh())) return 0;
   Wpt o = map_centroid(f, eye, t);
   double s = sqrt(get_area(f))/2;
   PIXEL a = (o - (x + y)*s);
   PIXEL b = (o + (x - y)*s);
   PIXEL c = (o - (x - y)*s);
   return fabs(det(b-a, c-a));
}

inline double
avg_bface_pix_area(BMESH* mesh)
{
   if (!mesh) return 0;

   CAMdataptr cam = VIEW::peek_cam()->data();
   Wpt eye = cam->from();
   Wvec  t = cam->   at_v().normalized();
   Wvec  x = cam->right_v().normalized();
   Wvec  y = cam->  pup_v().normalized();

   RunningAvg<double> ret(0);
   for (int i=0; i<mesh->nfaces(); i++)
      ret.add(normalized_pix_area(mesh->bf(i), eye, t, x, y));
   return ret.val();
}

extern bool draw_skin_only;

inline bool
print_edge_info(CBedge_list& edges)
{
   bool ret = false;
   err_msg("********");
   for (int i=0; i<edges.num(); i++) {
      int n = edges[i]->nfaces();
      int a = edges[i]->num_all_faces();
      err_msg("  %2d: %d top, %d lower", i, n, a - n);

      if (a > n) {
         WORLD::show(edges[i]->v1()->loc(), edges[i]->v2()->loc(), 4);
         ret = true;
      }
   }
   return ret;
}

inline void
debug_vis_ref_img(CNDCpt& p)
{
   VisRefImage* vis = VisRefImage::lookup();
   if (!vis) {
      cerr << endl << "debug_vis_ref_img: no vis ref image" << endl;
      return;
   }
   if (vis->need_update()) {
      cerr << "debug_vis_ref_img: vis ref image is out of date!" << endl;
      return;
   }
   cerr << "debug_vis_ref_img: NDCpt " << p << endl
        << "  val: " << vis->val  (p) << endl
        << "  red: " << vis->red  (p) << ", "
        << "  green: " << vis->green(p) << ", "
        << "  blue: " << vis->blue (p) << ", "
        << "  alpha: " << vis->alpha(p) << endl;

   Bsimplex* s = vis->simplex(p);
   cerr << "simplex: " << s << endl;
   cerr << "key: " << vis->rgba_to_key(vis->val(p)) << endl;
}

inline void
check_topo_sort(Bnode_list nodes)
{
   cerr << "checking Bnode list..." << endl << "  ";
   nodes.print_identifiers();
   cerr << "Before sort: was "
        << (nodes.is_topo_sorted() ? "" : "not ")
        << " sorted." << endl;
   nodes = nodes.topological_sort();
   cerr << "after sort: is "
        << (nodes.is_topo_sorted() ? "" : "STILL NOT ")
        << "sorted." << endl << "  ";
   nodes.print_identifiers();
}

inline void
debug_primary_edges()
{
   // show the primary edges of the vertex under the cursor.
   // used for debugging subdivision of non-manifold surfaces
   BMESH* mesh = get_cur_mesh(find_mesh());
   if (!mesh) {
      err_msg("No mesh found");
      return;
   }
   Wpt p;
   Bface* f = mesh->pick_face(Wline(VisRefImage::get_cursor()), p);
   if (!f) {
      err_msg("BMESH::pick_face() failed");
      return;
   }
   Bvert* v = closest_vert(f, p);
   assert(v);
   MeshGlobal::deselect_all_edges();
   MeshGlobal::select(v->get_manifold_edges().filter(StrongEdgeFilter()));

   err_msg("");
   err_msg("vertex is %s", v->is_manifold() ? "manifold" : "non-manifold");
   err_msg("full degree:    %d", v->degree());
   err_msg("primary degree: %d", v->p_degree());
   err_msg("strong degree:  %d",
           v->get_manifold_edges().filter(StrongEdgeFilter()).num());
   err_msg("crease degree:  %d",
           v->get_manifold_edges().filter(StrongPolyCreaseEdgeFilter()).num());
   err_msg("num quads:      %d", v->num_quads());
   err_msg("num tris:       %d", v->num_tris());

   if (print_edge_info(v->get_adj()))
      WORLD::show(v->loc(), 8, Color::orange);

   Bface_list faces;
   v->get_quad_faces(faces);
   MeshGlobal::deselect_all_faces();
   MeshGlobal::select(faces);
}

inline void
debug_memes()
{
   BMESH* mesh = get_cur_mesh(find_mesh());
   if (!mesh) {
      err_msg("No mesh found");
      return;
   }
   Wpt p;
   Bface* f = mesh->pick_face(Wline(VisRefImage::get_cursor()), p);
   if (!f) {
      err_msg("BMESH::pick_face() failed");
      return;
   }
   Bvert* v = closest_vert(f, p);
   assert(v);
   static VertMeme* vm = 0;
   if (vm)
      vm->set_do_debug(false);
   vm = Bbase::find_boss_vmeme(v);
   if (vm) {
      vm->set_do_debug(true);
      cerr << "found " << vm->class_name() << endl;
   } else {
      cerr << "no meme" << endl;
   }
}

void
unrefine_to_ctrl(BMESH* m)
{
   LMESH* ctrl = ((LMESH*)m)->control_mesh();
   while (ctrl->cur_mesh() != ctrl) {
      ctrl->unrefine();
   }
}

int
debug_cb(const Event& ev, State *&)
{
   switch (ev._c) {
   case 'd': {
      Patch* patch = VisRefImage::get_ctrl_patch();
      if (patch) {
         GTexture* cur_texture = patch->cur_tex();
         if (cur_texture->is_of_type(ProxyTexture::static_name())) {
            ProxyTexture* pt = (ProxyTexture*)cur_texture;
            pt->animate_samples();
         }
      }
      break;
   }
   case 'L': {

      FlatShadeTexture::toggle_debug_uv();
      NormalsTexture::toggle_uv_vectors();

      //WORLD::message(
      //   str_ptr("Debug UV: ") + (FlatShadeTexture::debug_uv() ? "ON" : "OFF")
      //   );
      //break;

      unrefine_to_ctrl(find_mesh());
      Panel::toggle_show_skel();
      break;

      // the following were used at one time...
      debug_memes();
      break;

      debug_vis_ref_img(VisRefImage::get_cursor());
      break;

      debug_primary_edges();
      break;
   }
   case 'J' : {
      draw_skin_only = !draw_skin_only;
      WORLD::message(str_ptr("Skin only: ") + (draw_skin_only ? "ON" : "OFF" ));
      break;
   }
   case 'z': {
      WORLD::print_command_list();
//       check_topo_sort(Bnode::get_all_bnodes());
    }
    default:
      ;
   }

   return 0;
}

int
JOTapp::clear_cb(const Event&, State *&)
{

   AlertBox *box = VIEW::peek()->win()->alert_box();

   box->set_title("Warning");
   box->set_icon(AlertBox::EXCLAMATION_ICON);
   box->add_text("You are about to delete everything!");
   box->add_text("Sure?");
   box->add_button("Yes");
   box->add_button("No");
   box->set_default(0);

   if (box->display(true, alert_cbs, NULL, NULL, ALERT_CLEAR_CB))
      cerr << "clear_cb() - AlertBox displayed.\n";
   else
      cerr << "clear_cb() - AlertBox **FAILED** to display!!\n";

   return 0;
}

void
JOTapp::alert_cbs(void *ptr, void *dptr, int idx, int but_idx)
{
   // Dispatch the appropriate alertbox callback...
   switch (idx) {
    case ALERT_CLEAR_CB:
      if (but_idx == 0) {
         // yes
         do_clear();
      } else {
         //no
         assert(but_idx == 1);
         //chill
      }
      break;
    case ALERT_SAVE_JOT_OVERWRITE_CB:
      if (but_idx == 0) {
         //yes 
         do_save((char *)dptr);
      } else if (but_idx == 1) {
         //no
         //try again...
         Event e;
         State *s=NULL;
         save_cb(e, s);
      } else {
         //cancel
         assert(but_idx == 2);
         //do nothing
      }
      //Free up that char buffer...
      delete[] (char *)dptr;
      break;
    case ALERT_SAVE_JOT_FAILED_CB:
      assert(but_idx == 0);
      break;
    case ALERT_LOAD_JOT_FAILED_CB:
      assert(but_idx == 0);
      break;
    default:
      assert(0);
   }
}

void
do_clear()
{
   // wipe out everything!!!!!!!!!!!!!!
   // except the floor and non-meshes

//    // clear strokes in current easel
//    if (cur_easel())
//       cur_easel()->removeEasel();

   // copy this list because otherwise it's hard to iterate over it
   // without crashing while it's changing radically underfoot:
   GELlist drawn = DRAWN;

   // undraw all meshes
   for (int i=0; i<drawn.num(); i++) {
      if (gel_to_bmesh(drawn[i]))
         // XXX -- should create undoable multi command
         WORLD::undisplay(drawn[i]);
   }

   // XXX - hack, fix this
   // clear selection list in DrawPen
   DrawPen *draw_pen;
   if (BaseJOTapp::instance() &&
       (draw_pen = dynamic_cast<DrawPen*>(BaseJOTapp::instance()->cur_pen()))) {

      draw_pen->ModeReset(0);

   }
}

int
JOTapp::save_cb(const Event&, State *&)
{
   FileSelect *sel = VIEW::peek()->win()->file_select();

   sel->set_title("Save Scene");
   sel->set_action("Save");
   sel->set_icon(FileSelect::SAVE_ICON);
   sel->set_path(".");
   sel->set_filter("*.jot");

   str_ptr fname = ((IOManager::basename() != NULL_STR) ?
                    (IOManager::basename()) : (str_ptr("out"))) + ".jot";

   sel->set_file(fname);


   if (sel->display(true, file_cbs, NULL, FILE_SAVE_JOT_CB))
      cerr << "save_cb() - FileSelect displayed.\n";
   else
      cerr << "save_cb() - FileSelect **FAILED** to display!!\n";

   return 0;
}

int
JOTapp::load_image_cb(const Event&, State*&)
{
   FileSelect *sel = VIEW::peek()->win()->file_select();

   sel->set_title("Load Image");
   sel->set_action("Load");
   sel->set_icon(FileSelect::LOAD_ICON);
   sel->set_path(".");
   sel->set_file("");
   sel->set_filter("*.png");
   

   if (sel->display(true, file_cbs, NULL, FILE_LOAD_IMAGE_CB))
      cerr << "load_image() - FileSelect displayed.\n";
   else
      cerr << "load_image() - FileSelect **FAILED** to display!!\n";
   return 0;
}

int
JOTapp::load_cb(const Event&, State *&)
{
   FileSelect *sel = VIEW::peek()->win()->file_select();

   sel->set_title("Load Scene");
   sel->set_action("Load");
   sel->set_icon(FileSelect::LOAD_ICON);
   sel->set_path(".");
   sel->set_file("");
   sel->set_filter("*.jot");
   sel->add_filter("*.sm");
   sel->add_filter("*.obj");

   if (!sel->display(true, file_cbs, NULL, FILE_LOAD_JOT_CB)) {
      cerr << "JOTapp::load_cb: error: FileSelect failed to display"
           << endl;
   }

   return 0;
}

void
JOTapp::file_cbs(void *ptr, int idx, int action, str_ptr path, str_ptr file)
{
   str_ptr fullpath = path + file;
   //Dispatch the appropriate fileselect callback...
   switch (idx) {
    case FILE_SAVE_JOT_CB:
      if (action == FileSelect::OK_ACTION) {
         FILE* foo = 0;
         bool exists = !!(foo=fopen(**(fullpath),"r"));
         if (exists) {
            fclose(foo);
            AlertBox *box = VIEW::peek()->win()->alert_box();

            box->set_title("Warning");
            box->set_icon(AlertBox::EXCLAMATION_ICON);
            box->add_text("Destination exists:");
            box->add_text(fullpath);
            box->add_text("Overwrite?");
            box->add_button("Yes");
            box->add_button("No");
            box->add_button("Cancel");
            box->set_default(0);

            // Can't send the fullpath str_ptr in a void * because
            // then it vanishes when this function end, but before
            // the alert box generates a callback (with the void *
            // passed along for use). We'll alloc a string instead,
            // and free it in the callback...
            char *fp = new char[(int)fullpath.len()+1];
            assert(fp);
            strcpy(fp,**fullpath);

            if (box->display(true, alert_cbs, ptr, fp,
                             ALERT_SAVE_JOT_OVERWRITE_CB))
               cerr << "clear_cb() - AlertBox displayed.\n";
            else
               cerr << "clear_cb() - AlertBox **FAILED** to display!!\n";
         } else {
            do_save(fullpath);
         }
      }
      break;
    case FILE_LOAD_JOT_CB:
      if (action == FileSelect::OK_ACTION) {
         do_load(fullpath);
      }
      break;
    case FILE_LOAD_IMAGE_CB:
      if (action == FileSelect::OK_ACTION) {
	 do_load_image(fullpath);
      }
      break;
    default:
      assert(0);
   }
}

void
JOTapp::do_save(str_ptr fullpath)
{
   SAVEobs::save_status_t status;

   cerr << "\ndo_save() - Saving...\n";

   NetStream s(fullpath, NetStream::ascii_w);

   int old_cursor = VIEW::peek()->get_cursor();
   VIEW::peek()->set_cursor(WINSYS::CURSOR_WAIT);
   SAVEobs::notify_save_obs(s, status, true, true);
   VIEW::peek()->set_cursor(old_cursor);

   if (status == SAVEobs::SAVE_ERROR_NONE) {
      cerr << "do_save() - ...done.\n";

      WORLD::message(str_ptr("Saved '") + fullpath + "'");
   } else {
      cerr << "do_save() - ...aborted!!!" << endl;

      WORLD::message(str_ptr("Problem saving '") + fullpath + "'");

      AlertBox *box = VIEW::peek()->win()->alert_box();

      box->set_title("Warning");
      box->set_icon(AlertBox::WARNING_ICON);
      box->add_text("Problem saving scene to file:");
      box->add_text(fullpath);
      box->add_button("OK");
      box->set_default(0);

      switch (status) {
       case SAVEobs::SAVE_ERROR_STREAM:
         box->add_text("Couldn't create output stream.");
         break;
       case SAVEobs::SAVE_ERROR_WRITE:
         box->add_text("Error occurred during write.");
         break;
       case SAVEobs::SAVE_ERROR_CWD:
         box->add_text("Error changing current working directory.");
         break;
       default:
         assert(0);
      }

      if (box->display(true, alert_cbs, NULL, NULL, ALERT_SAVE_JOT_FAILED_CB))
         cerr << "do_save() - AlertBox displayed.\n";
      else
         cerr << "do_save() - AlertBox **FAILED** to display!!\n";
   }

}

inline str_ptr
get_extension(Cstr_ptr& str)
{
   string s(**str);
   return s.substr(s.rfind('.')+1).c_str();
}

void
JOTapp::do_load_image(str_ptr fullpath)
{
   assert(instance());

   instance()->load_png_file(fullpath);
}

void
JOTapp::do_load(str_ptr fullpath)
{
   LOADobs::load_status_t status;

   NetStream s(fullpath, NetStream::ascii_r);

   str_ptr ext = get_extension(fullpath);
   assert(instance());

   // prepare alert box to report errors (if any):
   AlertBox *box = VIEW::peek()->win()->alert_box();
   box->set_title("Warning");
   box->set_icon(AlertBox::WARNING_ICON);
   box->add_text("Problem loading scene from file:");
   box->add_text(fullpath);
   box->add_button("OK");
   box->set_default(0);

   if (ext == "sm") {
      // load .sm file
      if (!instance()->load_sm_file(fullpath)) {
         box->add_text("Failed to load .sm file.");
         box->display(true, alert_cbs, NULL, NULL, ALERT_LOAD_JOT_FAILED_CB);
      }
      return;
   } else if (ext == "obj") {
      // load .obj file
      if (!instance()->load_obj_file(fullpath)) {
         box->add_text("Failed to load .obj file.");
         box->display(true, alert_cbs, NULL, NULL, ALERT_LOAD_JOT_FAILED_CB);
      }
      return;
   } else if (ext == "jot") {
      // load .jot file
      // Clear the scene first, then hope nothing goes wrong in
      // loading cuz if it does we should then unclear the
      // scene, but we're not prepared to do that...
      do_clear();

      int old_cursor = VIEW::peek()->get_cursor();
      VIEW::peek()->set_cursor(WINSYS::CURSOR_WAIT);
      LOADobs::notify_load_obs(s, status, true, true);
      VIEW::peek()->set_cursor(old_cursor);

      if (status == LOADobs::LOAD_ERROR_NONE) {

         WORLD::message(str_ptr("Loaded '") + fullpath + "'");

      } else {
         // replace entire scene with one loaded from file.
         // XXX - should be undoable

         // XXX  - should unclear the scene to restore current state
         //        to what it was before the load attempt.

         cerr << "JOTapp::do_load: error loading file: "
              << fullpath
              << endl;

         WORLD::message(str_ptr("Problem loading '") + fullpath + "'");

         switch (status) {
          case LOADobs::LOAD_ERROR_STREAM:
            box->add_text("Couldn't create input stream.");
            break;
          case LOADobs::LOAD_ERROR_JOT:
            box->add_text("The anticipated #jot header was found.");
            box->add_text("Error occurred while reading remaining file.");
            break;
          case LOADobs::LOAD_ERROR_CWD:
            box->add_text("Error changing current working directory.");
            break;
          case LOADobs::LOAD_ERROR_AUX:
            box->set_icon(AlertBox::INFO_ICON);
            box->add_text("Failed to load as jot format file.");
            box->add_text("Succeeded with an auxillary file parser!");
            break;
          case LOADobs::LOAD_ERROR_READ:
            box->add_text("Error occurred during read.");
            break;
          default:
            assert(0);
         }

         if (!box->display(true, alert_cbs, NULL, NULL,
                           ALERT_LOAD_JOT_FAILED_CB))
            cerr << "do_save() - AlertBox **FAILED** to display!!\n";
      }
   } else {
      box->add_text("Unknown file type.");
      box->display(true, alert_cbs, NULL, NULL, ALERT_LOAD_JOT_FAILED_CB);
   }
}

inline void
demote_surface_meme(CBvert* v)
{
   VertMeme* vm = Bbase::find_boss_vmeme(v);
   if (vm && Bsurface::isa(vm->bbase()))
      vm->get_demoted();
}

inline void
demote_surface_memes(CBvert_list& verts)
{
   for (int i=0; i<verts.num(); i++)
      demote_surface_meme(verts[i]);
}

int
animation_keys(const Event &e, State *&s)
{
   //Here we handle the keys for both the Recorder and Animator

   //*It cannot be the case that both are 'ON' simultaneously*

   assert(!(VIEW::peek()->recorder()->on() && VIEW::peek()->animator()->on()));

   //If they're both 'off' just check the activation keys
   if (!VIEW::peek()->recorder()->on() && !VIEW::peek()->animator()->on()) {
      switch (e._c) {
       case 'C':
         return toggle_recorder(e,s);
         break;
       case 'X':
         VIEW::peek()->animator()->toggle_activation();
         break;
      };
   }
   //If the recorder's on just check its keys
   else if (VIEW::peek()->recorder()->on()) {
      switch (e._c) {
       case 'X':
         WORLD::message("Deactivate recorder first!");
         break;
       case 'C':
         return toggle_recorder(e,s);
         break;
       case '/':
         return rec_play(e,s);
         break;
       case '*':
         return rec_rec(e,s);
         break;
       case '-':
         return rec_stop(e,s);
         break;
       case '+':
         return rec_pause(e,s);
         break;
      };
   }
   //If the animators's on just check its keys
   else if (VIEW::peek()->animator()->on()) {
      switch (e._c) {
       case 'C':
         WORLD::message("Deactivate recorder first!");
         break;
       case 'X':
         VIEW::peek()->animator()->toggle_activation();
         break;
       case '/':
         VIEW::peek()->animator()->press_play();
         break;
       case '*':
         VIEW::peek()->animator()->press_render();
         break;
       case '-':
         VIEW::peek()->animator()->press_sync();
         break;
       case '+':
         VIEW::peek()->animator()->press_stop();
         break;
       case '5':
         VIEW::peek()->animator()->press_beginning();
         break;
       case '4':
         VIEW::peek()->animator()->press_step_rev();
         break;
       case '6':
         VIEW::peek()->animator()->press_step_fwd();
         break;
       case '2':
         VIEW::peek()->animator()->press_jog_rev();
         break;
       case '8':
         VIEW::peek()->animator()->press_jog_fwd();
         break;
      };
   } else {
      //HUH!?
      assert(0);
      return 0;
   }

   return 0;
}

int
render_mode(const Event &e, State *&s)
{
   switch (e._c) {
    case '!':
      WORLD::message("Rendering ALL Objects.");
      VIEW::peek()->set_render_mode(VIEW::NORMAL_MODE);
      break;
    case '@':
      WORLD::message("Rendering OPAQUE Objects.");
      VIEW::peek()->set_render_mode(VIEW::OPAQUE_MODE);
      break;
    case '#':
      WORLD::message("Rendering TRANSPARENT Objects.");
      VIEW::peek()->set_render_mode(VIEW::TRANSPARENT_MODE);
      break;
    default:
      //huh!?
      break;
   };

   return 0;
}

int
toggle_recorder (const Event &, State *&)
{
   Recorder* _rec =VIEW::peek()->recorder();
   if ( _rec == NULL)
      return 0;
   if (_rec->on())
      _rec->deactivate();
   else {
      if (_rec->get_ui() == NULL) {
         _rec->set_ui(new RecorderUI(_rec));
         _rec->_name_buf = str_ptr ("default");
         _rec->new_path();
      }
      _rec->activate();
   }
   return 1;
}

int
rec_play (const Event &, State *&)
{

   Recorder* _rec = NULL;
   if ( ( _rec =VIEW::peek()->recorder()) == NULL ) return 0;
   _rec->rec_play();
   return 1;
}

int
rec_rec (const Event &, State *&)
{

   Recorder* _rec = NULL;
   if ( ( _rec =VIEW::peek()->recorder()) == NULL ) return 0;
   _rec->rec_record();
   return 1;
}

int
rec_stop (const Event &, State *&)
{

   Recorder* _rec = NULL;
   if ( ( _rec =VIEW::peek()->recorder()) == NULL ) return 0;
   _rec->rec_stop();
   return 1;
}

int
rec_pause (const Event &, State *&)
{

   Recorder* _rec = NULL;
   if ( ( _rec =VIEW::peek()->recorder()) == NULL ) return 0;
   _rec->rec_pause();
   return 1;
}

int
npr_select_mode (const Event &, State *&)
{
   NPRControlFrameTexture::next_select_mode();
   return 0;
}

int
toggle_repair (const Event &, State *&)
{
   bool r = !(BaseStroke::get_repair());

   if (r)
      WORLD::message("Using Stroke Intersect Repair Mode");
   else
      WORLD::message("NOT using Stroke Intersect Repair Mode");

   BaseStroke::set_repair(r);

   return 0;
}

int
set_pen(const Event & ev, State *&)
{

   if (ev._c == '.') {
      BaseJOTapp::instance()->next_pen();
   } else if (ev._c == ',') {
      BaseJOTapp::instance()->prev_pen();
   }

   VIEW::peek()->set_focus();

   return 0;
}

int
pen_key(const Event &e, State *&)
{
   if (!BaseJOTapp::instance()->cur_pen()) return 0;

   switch (e._c) {
    case ';':
      NPRTexture::toggle_strokes();
      break;
    case ':':
      NPRTexture::toggle_coats();
      break;
    default:
      BaseJOTapp::instance()->cur_pen()->key(e);
      break;
   }
   return 0;
}

int
undo_redo(const Event &e, State *&)
{
   switch (e._c) {
    case '\n':
    case '\r':
      WORLD::redo();
      break;
    case '\x8' :
    case '\x7f':
      WORLD::undo();
      break;
    default:
      err_msg("undo_redo: unknown key: %d (ASCII decimal code)", int(e._c));
   }

   return 0;
}


int
quit(const Event&, State *&)
{
   if (Config::get_var_bool("DEBUG_STRPOOL",false))
      err_msg("strpool load factor: %f", STR::load_factor());

   WORLD::Quit();

   return 0;
}

int
refine(const Event&, State *&)
{
   BMESH* m = find_ctrl_mesh();
   if (!m || !LMESH::isa(m))
      return 0;

   LMESH* ctrl_mesh = (LMESH*)m;

   if (Config::get_var_bool("DEBUG_VOLUME_PRESERVATION",false))
      cerr << "Current mesh volume=" << ctrl_mesh->volume() <<endl;

   if (ctrl_mesh && ctrl_mesh->loc_calc() && **ctrl_mesh->loc_calc()->name())
      WORLD::message(**ctrl_mesh->loc_calc()->name());

   ctrl_mesh->refine();
   if (Config::get_var_bool("DEBUG_VOLUME_PRESERVATION",false))
      cerr << "Refined mesh volume=" << ctrl_mesh->volume() <<endl;

   return 0;
}

int
cycle_subdiv_loc_calc(const Event&, State *&)
{
   BMESH* m = find_ctrl_mesh();
   if (!m || !LMESH::isa(m))
      return 0;

   static int k=0;
   LMESH* ctrl_mesh = (LMESH*)m;
   SubdivLocCalc* calc = 0;
   switch (++k % 3) {
    case 0:
      calc = new LoopLoc;
      break;
    case 1:
      calc = new Hybrid2Loc;
      break;
    case 2:
      calc = new HybridVolPreserve;
      break;
   }
   assert(calc != 0);
   ctrl_mesh->set_subdiv_loc_calc(calc);
   ctrl_mesh->update();
   WORLD::message(calc->name() + " scheme in use");

   return 0;
}

int
set_tuft_group(const Event&, State *&)
{
//   SGraftalPen::set_group_callback();
   return 0;
}


int
clear_selections(const Event&, State *&)
{
   MeshGlobal::deselect_all_edges();
   MeshGlobal::deselect_all_faces();
   return 0;
}

int
switch_rep(const Event&, State *&)
{
   TEXBODY* tex = TEXBODY::cur_tex_body();
   if (!tex) return 0;

   BMESH_list   meshes = tex->meshes();
   if (meshes.num()!=2 ||
       meshes[0]->draw_enabled()==meshes[1]->draw_enabled() ||
       meshes[0]->empty() || meshes[1]->empty())
      return 0;

   for (int j = 0; j < meshes.num(); j++) {
      if (meshes[j]->draw_enabled())
         meshes[j]->disable_draw();
      else
         meshes[j]->enable_draw();
   }

   return 0;
}

int
switch_mode(const Event&, State *&)
{
   BMESH* m = find_ctrl_mesh();
   if (!m || !LMESH::isa(m))
      return 0;

   if (SWEEP_DISK::get_active_instance()) {
      SWEEP_DISK::get_active_instance()->deactivate();
      return 0;
   }

   // choose a panel from the candidates in SWEEP_DISK
   ARRAY<Panel*> panels = SWEEP_DISK::panels;
   int loc;
   for (loc = 0; loc < panels.num(); loc++) {
      if (panels[loc]->mesh() == m)
         break;
   }
   if (loc == panels.num())
      return 0;

   // shouldn't happen
   if (panels.num() != SWEEP_DISK::bpoints.num() ||
      panels.num() != SWEEP_DISK::bcurves.num() ||
      panels.num() != SWEEP_DISK::bsurfaces.num() ||
      panels.num() != SWEEP_DISK::profiles.num())
      return 0;

   SWEEP_DISK::init(panels[loc], SWEEP_DISK::bpoints[loc], 
      SWEEP_DISK::bcurves[loc], SWEEP_DISK::bsurfaces[loc],
      SWEEP_DISK::profiles[loc]);

   return 0;
}

int
unrefine(const Event&, State *&)
{
   BMESH* m = find_ctrl_mesh();
   if (!m || !LMESH::isa(m))
      return 0;

   ((LMESH*)m)->unrefine();

   return 0;
}

int
toggle_show_memes(const Event&, State *&)
{
   Bbase::toggle_show_memes();
   return 0;
}

inline BMESHptr
control_mesh(CBMESHptr& m)
{
   // return the control mesh for the given mesh.
   // if it's not a LMESH just return the mesh itself.
   return (m && LMESH::isa(&*m)) ? BMESHptr(((LMESH*)&*m)->control_mesh()) : m;
}

int
toggle_sil_frame(const Event&, State *&)
{
   BMESH* mesh = find_ctrl_mesh();
   if (!mesh)
      return 0;

   mesh->toggle_render_style(SilFrameTexture::static_name());

   return 0;
}


int
toggle_transp(const Event&e, State *&)
{
   RAYhit ray(VisRefImage::get_cursor());
   e.view()->intersect(ray);

   if (ray.success() && ray.appear() ) {
      if (ray.appear()->has_transp() ) {
         ray.appear()->unset_transp();
      } else {
         ray.appear()->set_transp(.5);
      }
   }

   return 0;
}

int
write(const Event&, State *&)
{
   BMESH* mesh = find_mesh();

   if (!mesh) {
      cerr << "write - No mesh... aborting.\n";
      return 0;
   }

   mesh->print();

   str_ptr fname;

   if ( TEXBODY::isa(mesh->geom()) ) {
      TEXBODY *t = (TEXBODY*)mesh->geom();

      if (t->mesh_file() == NULL_STR) {
         fname = "out.sm";
      } else {
         fname = t->mesh_file();
      }
   } else {
      fname = "out.sm";
   }

   if (mesh->write_file(**fname)) {
      WORLD::message(str_ptr("Wrote mesh: ") + fname);
   } else {
      WORLD::message(str_ptr("Failed to write mesh: ") + fname);
   }

   return 1;
}

int
print_mesh(const Event&, State *&)
{
   BMESH* mesh = find_mesh();
   if (mesh)
      mesh->print();
   return 0;
}

int
write_xformed(const Event&, State *&)
{
   BMESH* mesh = find_mesh();
   if (!mesh) {
      return 0;
   }

   //Same as above, but it applies the xform
   //to the mesh before saving it.

   MOD::tick();
   mesh->transform(mesh->xform(), MOD());

   // write it
   if (mesh->write_file("out.sm")) {
      WORLD::message("Wrote ***TRANSFORMED*** mesh to out.sm");
   } else {
      WORLD::message("Couldn't write mesh file: out.sm");
   }

   MOD::tick();
   mesh->transform(mesh->inv_xform(), MOD());

   return 1;
}

int
save_config(const Event &e, State *&)
{
   bool ret;

   ret = Config::save_config(Config::JOT_ROOT() + "jot.cfg");

   if (ret)
      WORLD::message(str_ptr("Wrote config to ") +
                     Config::JOT_ROOT() + "jot.cfg");
   else
      WORLD::message(str_ptr("FAILED!! Writing config to ") +
                     Config::JOT_ROOT() + "jot.cfg");

   return 1;
}


int
screen_grab(const Event&, State *&)
{
   cerr << "screen_grab()" << endl;
   NPRview::_capture_flag = 1;
   return 1;
}

int
rotate_screen_grab(const Event& e, State *&s)
{

   cerr << "rotate_screen_grab()" << endl;

   if (NPRview::_capture_flag == 0) {
      rotate_camera(e,s);
      NPRview::_capture_flag = 1;
   }

   return 1;
}


/*************************************************************************
 * Function Name: clear_creases
 * Parameters: const Event&, State *&
 * Returns: int
 * Effects:
 *************************************************************************/
int
clear_creases(const Event&, State*&)
{
   BMESH* mesh = find_ctrl_mesh();
   if (!mesh)
      return 0;

   WORLD::message("Clearing creases");
   mesh->clear_creases();

   return 0;
}


int
toggle_nulubo(const Event&, State*&)
{
   if (ZXedgeStrokeTexture::toggle_idref_method())
      WORLD::message("Using parameterized id ref");
   else
      WORLD::message("Using standard id ref");

   return 0;
}

int
toggle_no_text(const Event&, State*&)
{
   if (!TEXT2D::toggle_suppress_draw())
      WORLD::message("Text on");

   return 0;
}

int
inc_edit_level(const Event&, State*&)
{
   BMESH* mesh = find_ctrl_mesh();
   if (!mesh)
      return 0;

   mesh->inc_edit_level();

   char msg[256];
   sprintf(msg, "Edit level: %d", mesh->edit_level());
   WORLD::message(msg);

   return 0;
}

int
dec_edit_level(const Event&, State*&)
{
   BMESH* mesh = find_ctrl_mesh();
   if (!mesh)
      return 0;

   mesh->dec_edit_level();

   char msg[256];
   sprintf(msg, "Edit level: %d", mesh->edit_level());
   WORLD::message(msg);

   return 0;
}

int
toggle_show_quad_diagonals(const Event&, State *&)
{
   WireframeTexture::toggle_show_quad_diagonals();
   char msg[256];
   sprintf(msg, "Quad diagonals: %s in wireframe rendering",
           WireframeTexture::show_quad_diagonals() ? "ON" : "OFF");
   WORLD::message(msg);
   return 0;
}

int
toggle_show_secondary_faces(const Event&, State *&)
{
   // Toggle the state:
   BMESH::toggle_show_secondary_faces();

   // Rebuild display lists to see the change:
   TEXBODY::all_drawn_meshes().changed();

   // Display a message:
   char msg[256];
   sprintf(msg, "Secondary faces: %s",
           BMESH::show_secondary_faces() ? "drawn" : "not drawn");
   WORLD::message(msg);

   return 0;
}

/*************************************************************************
 * Function Name: recreate_creases
 * Parameters: const Event&, State *&
 * Returns: int
 * Effects:
 *************************************************************************/
int
recreate_creases(const Event&, State *&)
{
   BMESH* mesh = find_ctrl_mesh();
   if (!mesh)
      return 0;

   WORLD::message("Recreating creases");
   for (int k = mesh->nedges()-1; k>=0; k--)
      mesh->be(k)->compute_crease(0.5);
   mesh->changed();

   return 0;
}

int
toggle_crease(const Event &, State *&)
{
   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   vis_ref->update();
   Wpt obj_pt;
   Bface* face = vis_ref->intersect(VisRefImage::get_cursor(), obj_pt);

   if (face) {
      Wvec bc;
      face->project_barycentric(obj_pt, bc);
      int i=0;
      if (bc[0] < bc[1])
         i = (bc[0] < bc[2]) ? 0 : 2;
      else
         i = (bc[1] < bc[2]) ? 1 : 2;
      Bvert *vert = face->v(i+1);
      Bedge *edge = face->opposite_edge(vert);

      edge->set_crease(!edge->is_crease());
      face->mesh()->changed();
   }
   return 0;
}

int
split_mesh(const Event &e, State *&)
{
   BMESHray r(VisRefImage::get_cursor());
   BMESHptr mesh;
   if (e.view()->intersect(r).success() &&
       (mesh = gel_to_bmesh(r.geom()))) {
      WORLD::message("Split components");

      ARRAY<BMESH*> new_meshes = mesh->split_components();

      for (int i=0; i<new_meshes.num(); i++)
         WORLD::create(new TEXBODY(new_meshes[i],
                                   WORLD::unique_name(BMESH::static_name())));

   } else WORLD::message("Missed mesh");
   return 0;
}


int
kill_component(const Event &, State *&)
{
   VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());
   vis_ref->update();
   Wpt p;
   Bface *face = vis_ref->intersect(VisRefImage::get_cursor(),p);

   if (face) {
      WORLD::message("Split components");

      face->mesh()->kill_component(face->v1());

   } else WORLD::message("Missed mesh");
   return 0;
}

int
create_cam_path(const Event&, State *&)
{
   WORLD::message("Camera paths not available");

   return 0;
}

int
toggle_trace(const Event&, State*&)
{
   if (TRACEptr t=TRACE::get_instance(VIEW::peek())) {
      t->toggle_active();
   }

   return 0;
}


int
trace_calibrate(const Event&, State*&)
{
   if (TRACEptr t=TRACE::get_instance(VIEW::peek())) {
      t->activate();
      t->do_calibrate();
   }

   return 0;
}

int
bind_trace(const Event& ev, State*&)
{
   TRACEptr t;
   if (!(t=TRACE::get_instance(VIEW::peek()))) return 0;
   if (!t->valid()) return 0;
   if (t->calibrating()) return 0;
   t->deactivate();

//    make_new_easel(ev.view());

//    assert(cur_easel());
//    cur_easel()->bindTrace(t);
   TRACE::clear_instance();

   return 0;
}

int
toggle_curvature_ui(const Event&, State *&)
{

   if (CurvatureUISingleton::Instance().is_vis(VIEW::peek())) {

      CurvatureUISingleton::Instance().hide(VIEW::peek());

   } else {

      CurvatureUISingleton::Instance().show(VIEW::peek());
      VIEW::peek()->set_focus();

   }

   return 0;
}

int
inc_aoi(const Event&, State *&)
{
   PROFILEptr p = PROFILE::get_active_instance();
   if (!p) return 0;

   p->inc_aoi();
   return 0;
}

int
dec_aoi(const Event&, State *&)
{
   PROFILEptr p = PROFILE::get_active_instance();
   if (!p) return 0;

   p->dec_aoi();
   return 0;
}

int
toggle_img_line_ui(const Event&, State *&)
{
   if (ImageLineUI::is_vis_external(VIEW::peek())) {
      ImageLineUI::hide_external(VIEW::peek());
   } else {
      ImageLineUI::show_external(VIEW::peek());
   }
   return 0;
}

// end of file jot.C
