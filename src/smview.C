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
#include "glew/glew.H"

#include "std/fstream.H"
#include "disp/animator.H"
#include "disp/recorder.H"
#include "geom/distrib.H"
#include "geom/recorder_ui.H"
#include "geom/texture.H"
#include "geom/world.H"
#include "gtex/ref_image.H"
#include "gtex/buffer_ref_image.H"
#include "gtex/curvature_ui.H"
#include "gtex/fader_texture.H"
#include "gtex/flat_shade.H"
#include "gtex/key_line.H"
#include "gtex/sil_frame.H"
#include "manip/cam_pz.H"
#include "manip/cam_fp.H"
#include "mesh/hybrid.H"
#include "mesh/lmesh.H"
#include "mesh/objreader.H"
#include "mesh/patch.H"
#include "mlib/points.H"

using namespace mlib;

#include "std/run_avg.H"
#include "std/stop_watch.H"
#include "std/support.H"
#include "std/time.H"
#include "widgets/alert_box.H"
#include "widgets/file_select.H"
#include "widgets/fps.H"
#include "widgets/menu.H"

#include "base_jotapp/base_jotapp.H"

/* KeyMenu Callback Function Prototypes */

int bk_camera(const Event& ev, State *& s);
int fwd_camera(const Event& ev, State *& s);

int next_texture(const Event&, State *&);

int rotate_camera(const Event& ev, State *&);

int toggle_buffer(const Event&, State *&);

int toggle_antialias(const Event&, State *&);
int next_antialias(const Event&, State *&);

int freeze_sils(const Event&, State *&);

int toggle_random_sils(const Event&, State *&);

int toggle_hidden_lines(const Event&, State *&);

int debug_cb(const Event& ev, State *&);

int clear_cb(const Event&, State *&);

void alert_cbs(void *ptr, void *dptr, int idx, int but_idx);

void do_clear();

int save_cb(const Event&, State *&);
int load_cb(const Event&, State *&);

void file_cbs(void *ptr, int idx, int action, str_ptr path, str_ptr file);

void do_save(str_ptr fullpath);
void do_load(str_ptr fullpath);

int animation_keys(const Event &e, State *&s);

int render_mode(const Event &e, State *&s);

int toggle_recorder(const Event &, State *&);
int rec_play(const Event &, State *&);
int rec_rec(const Event &, State *&);
int rec_stop(const Event &, State *&);
int rec_pause(const Event &, State *&);

int toggle_repair(const Event &, State *&);

int undo_redo(const Event &e, State *&);

int quit(const Event&, State *&);

int refine(const Event&, State *&);

int cycle_subdiv_loc_calc(const Event&, State *&);

int clear_selections(const Event&, State *&);

int unrefine(const Event&, State *&);

int toggle_transp(const Event&e, State *&);

int write(const Event&, State *&);

int print_mesh(const Event&, State *&);

int write_xformed(const Event&, State *&);

int write_merged_meshes(const Event&, State *&);

int save_config(const Event &e, State *&);

int clear_creases(const Event&, State*&);

int toggle_no_text(const Event&, State*&);

int toggle_show_secondary_faces(const Event&, State *&);

int recreate_creases(const Event&, State *&);
int toggle_crease(const Event &, State *&);

int split_mesh(const Event &e, State *&);

int kill_component(const Event &, State *&);

int print_key_menu(const Event&, State *&);

int toggle_curvature_ui(const Event&, State *&);

/**********************************************************************
 * SMVIEWapp Class
 **********************************************************************/

class SMVIEWapp : public BaseJOTapp {

 public:   
   /******** PUBLIC MEMBER CLASSES ********/

   class WINDOWjot : public BaseJOTapp::WINDOW
   {
    public:
      State          _otherstart;

      WINDOWjot(WINSYS *win) : WINDOW(win) {}
   };

 protected:  
   /******** STATIC MEMBER VARIABLES ********/

   /******** MEMBER VARIABLES ********/

   /******** STATIC MEMBER METHODS ********/

   static void splash_cb(void *j, void *jd, int, int) {
      ((SMVIEWapp *)j)->_windows[0]->_view->set_focus();
   }

 public:
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

    SMVIEWapp(int argc, char **argv) : BaseJOTapp(argc, argv) {}

   /******** BaseJOTapp METHODS ********/
 protected:  

   virtual WINDOW*   new_window(WINSYS *win) { return new WINDOWjot(win);}

   virtual void      init_scene();

   virtual void      init_interact_cb(WINDOW &base_window);
   
   virtual void      init_kbd(WINDOW &base_window);

   virtual void      init_fsa();

 public:
   virtual void init() 
   {
      srand48((long)the_time());

      BaseJOTapp::init();
      
      //Puts focus on main window -- in case some
      //GLUI window tries to take initial focus.
      //If the splash alert box shows, this is called
      //after the popup is dismissed instead.
      splash_cb(this,NULL,0,0);
   }
};

/*****************************************************************
 * SMVIEWapp Methods
 *****************************************************************/

/////////////////////////////////////
// init_scene()
/////////////////////////////////////
void  
SMVIEWapp::init_scene() 
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
// init_interact_cb()
/////////////////////////////////////
void  
SMVIEWapp::init_interact_cb(WINDOW &base_window) 
{
   BaseJOTapp::init_interact_cb(base_window);

//   WINDOWjot &win = (WINDOWjot &) base_window;

//    init_draw_int(win);
}

/////////////////////////////////////
// init_kbd()
/////////////////////////////////////

void
SMVIEWapp::init_kbd(WINDOW &base_window)
{
   
   BaseJOTapp::init_kbd(base_window);
   
   // add a mess of key commands (in alphabetical order!):

   _key_menu->add_menu_item('a', "Toggle antialias", &toggle_antialias);
   _key_menu->add_menu_item('A', "Toggle next antialias", &next_antialias);
   _key_menu->add_menu_item('b', "Toggle buffer", &toggle_buffer);
   _key_menu->add_menu_item('c', "Clear (delete) all objects", &clear_cb);
   // key 'C' falls into AnimationKeys
   _key_menu->add_menu_item('D', "Toggle no_text", &toggle_no_text);
   _key_menu->add_menu_item('d', "Debug callback", &debug_cb);
   _key_menu->add_menu_item('F', "Freeze sils", &freeze_sils);
   _key_menu->add_menu_item('i', "Recreate Creases", &recreate_creases);
   _key_menu->add_menu_item('k', "Clear creases", &clear_creases);   
   _key_menu->add_menu_item('l', "Load File", &load_cb);   
   _key_menu->add_menu_item('N', "Next texture", &next_texture);
   _key_menu->add_menu_item('p', "Print mesh statistics", &print_mesh);
   _key_menu->add_menu_item('r', "Rotate", &rotate_camera);
   _key_menu->add_menu_item('s', "Save", &save_cb);
   _key_menu->add_menu_item('S', "Cycle subdivision loc calc", &cycle_subdiv_loc_calc);
   _key_menu->add_menu_item("t", "Refine mesh", &refine);
   _key_menu->add_menu_item('U', "Unrefine mesh", &unrefine);
   _key_menu->add_menu_item('w', "Write mesh", &write);
   _key_menu->add_menu_item('W', "Write_xformed mesh", &write_xformed);
   // key 'X' is used below by the animation controls
   _key_menu->add_menu_item('Z', "Toggle hidden lines in Key Line rendering style",
           &toggle_hidden_lines);
   _key_menu->add_menu_item('=', "Toggle random sils", &toggle_random_sils);

   // This method handles all key presses to the view's
   // recorder and animator
   _key_menu->add_menu_item("CX/*-+24568", "Animation keys",  &animation_keys);

   _key_menu->add_menu_item("\r\n\x8\x7f", "Undo Redo", &undo_redo);
   
   _key_menu->add_menu_item("!@#", "Render mode", &render_mode);

   _key_menu->add_menu_item("%", "Toggle Curvature gTexture UI", &toggle_curvature_ui);
}

/////////////////////////////////////
// init_fsa()
/////////////////////////////////////
void        
SMVIEWapp::init_fsa() 
{
   BaseJOTapp::init_fsa();

   for (int i = 0; i < _windows.num(); i++) 
   {
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
   SMVIEWapp app(argc, argv);
   
   app.init();
   app.Run();

   return 0;
}

//============================================================================//

/* KeyMenu Callback Functions */

// Dialog box callback constants:

enum alert_cb_t {
   ALERT_CLEAR_CB = 0,
   ALERT_SAVE_JOT_OVERWRITE_CB,
   ALERT_SAVE_JOT_FAILED_CB,
   ALERT_LOAD_JOT_FAILED_CB
};

enum file_cb_t {
   FILE_SAVE_JOT_CB = 0,
   FILE_LOAD_JOT_CB
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
   if (buf)
      {
         if (buf->is_observing())
            {
               cerr << "DrawInt: BufferRefImage was observing -- Toggling OFF..\n";
               buf->unobserve();
            }
         else
            {
               cerr << "DrawInt: BufferRefImage was NOT observing -- Toggling ON...\n";
               buf->observe();
            }
      }

   return 0;
}

int
toggle_antialias(const Event&, State *&)
{

   int a = VIEW::peek()->get_antialias_enable();

   VIEW::peek()->set_antialias_enable(!a);

   if (VIEW::peek()->get_antialias_enable())
      {
         WORLD::message(str_ptr("Antialiasing: ENALBED Jitters: ") + 
                        str_ptr(VIEW::get_jitter_num(VIEW::peek()->get_antialias_mode())));
      }
   else
      {
         WORLD::message(str_ptr("Antialiasing: DISABLED Jitters: ") + 
                        str_ptr(VIEW::get_jitter_num(VIEW::peek()->get_antialias_mode())));
      }

   return 0;
}

int
next_antialias(const Event&, State *&)
{

   int m = VIEW::peek()->get_antialias_mode();

   VIEW::peek()->set_antialias_mode((m+1)%VIEW::get_jitter_mode_num());

   if (VIEW::peek()->get_antialias_enable())
      {
         WORLD::message(str_ptr("Antialiasing: ENALBED Jitters: ") + 
                        str_ptr(VIEW::get_jitter_num(VIEW::peek()->get_antialias_mode())));
      }
   else
      {
         WORLD::message(str_ptr("Antialiasing: DISABLED Jitters: ") + 
                        str_ptr(VIEW::get_jitter_num(VIEW::peek()->get_antialias_mode())));
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

// temporary debugging function
inline bool
is_sec(Bface* f)
{
   return f && f->is_secondary();
}

// temporary debugging class
class BadEdgeFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
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
      cerr << "debug_vis_ref_img: no vis ref image" << endl;
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
}

int
debug_cb(const Event& ev, State *&)
{
   switch (ev._c) {
    case 'd': {
       FlatShadeTexture::toggle_debug_uv();
       WORLD::message(
          str_ptr("Debug UV: ") + (FlatShadeTexture::debug_uv() ? "ON" : "OFF")
          );
       break;
    }
    default:
      ;
   }

   return 0;
}

int
clear_cb(const Event&, State *&)
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
alert_cbs(void *ptr, void *dptr, int idx, int but_idx)
{
   //Dispatch the appropriate alertbox callback...
   switch(idx)
   {
      case ALERT_CLEAR_CB:
         if (but_idx == 0)  //yes
         {
            do_clear();
         }
         else //no
         {
            assert(but_idx == 1);
            //chill
         }
      break;
      case ALERT_SAVE_JOT_OVERWRITE_CB:
         if (but_idx == 0) //yes
         {
            do_save((char *)dptr);
         }
         else if (but_idx == 1) //no
         {
            //try again...
            Event e; State *s=NULL;
            save_cb(e, s);
         }
         else //cancel
         {
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

   // undraw all meshes and draw axes
   for (int i=0; i<drawn.num(); i++) {
      if (gel_to_bmesh(drawn[i]))
         // XXX -- should create undoable multi command
         WORLD::undisplay(drawn[i]);
   }
}

int
save_cb(const Event&, State *&)
{

   FileSelect *sel = VIEW::peek()->win()->file_select();

   sel->set_title("Save Scene");
   sel->set_action("Save");
   sel->set_icon(FileSelect::SAVE_ICON);
   sel->set_path(".");
   sel->set_filter("*.jot");

   str_ptr fname = ((IOManager::basename() != NULL_STR) ? (IOManager::basename()) : (str_ptr("out"))) + ".jot";

   sel->set_file(fname);
   

   if (sel->display(true, file_cbs, NULL, FILE_SAVE_JOT_CB))
      cerr << "save_cb() - FileSelect displayed.\n";
   else
      cerr << "save_cb() - FileSelect **FAILED** to display!!\n";

   return 0;
}


int
load_cb(const Event&, State *&)
{

   FileSelect *sel = VIEW::peek()->win()->file_select();

   sel->set_title("Load Scene");
   sel->set_action("Load");
   sel->set_icon(FileSelect::LOAD_ICON);
   sel->set_path(".");
   sel->set_file("");
   sel->set_filter("*.jot");
   

   if (sel->display(true, file_cbs, NULL, FILE_LOAD_JOT_CB))
      cerr << "load_cb() - FileSelect displayed.\n";
   else
      cerr << "load_cb() - FileSelect **FAILED** to display!!\n";

   return 0;
}

void 
file_cbs(void *ptr, int idx, int action, str_ptr path, str_ptr file)
{
   str_ptr fullpath = path + file;
   //Dispatch the appropriate fileselect callback...
   switch(idx)
   {
      case FILE_SAVE_JOT_CB:
         if (action == FileSelect::OK_ACTION)
         {
            bool exists; FILE *foo; exists=!!(foo=fopen(**(fullpath),"r"));if(exists)fclose(foo); 

            if (!exists)
            {
               do_save(fullpath);
            }
            else
            {
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
               
               //Can't send the fullpath str_ptr in a void * because
               //the it vanishes when this function end, but before
               //the alert box generates a callback (with the void *
               //passed along for use). We'll alloc a string instead,
               //and free it in the callback...

               char *fp = new char[(int)fullpath.len()+1]; assert(fp); strcpy(fp,**fullpath); 

               if (box->display(true, alert_cbs, ptr, fp, ALERT_SAVE_JOT_OVERWRITE_CB))
                  cerr << "clear_cb() - AlertBox displayed.\n";
               else
                  cerr << "clear_cb() - AlertBox **FAILED** to display!!\n";
            }
         }
      break;
      case FILE_LOAD_JOT_CB:
         if (action == FileSelect::OK_ACTION)
         {
            do_load(fullpath);
         }
      break;
      default:
         assert(0);
   }
}

void 
do_save(str_ptr fullpath)
{
   SAVEobs::save_status_t status;

   cerr << "\ndo_save() - Saving...\n";

   NetStream s(fullpath, NetStream::ascii_w); 

   int old_cursor = VIEW::peek()->get_cursor();
   VIEW::peek()->set_cursor(WINSYS::CURSOR_WAIT);
   SAVEobs::notify_save_obs(s, status, true, true);   
   VIEW::peek()->set_cursor(old_cursor);

   if (status == SAVEobs::SAVE_ERROR_NONE)
   {
      cerr << "do_save() - ...done.\n";

      WORLD::message(str_ptr("Saved '") + fullpath + "'");
   }
   else
   {
      cerr << "do_save() - ...aborted!!!" << endl;
      
      WORLD::message(str_ptr("Problem saving '") + fullpath + "'");

      AlertBox *box = VIEW::peek()->win()->alert_box();

      box->set_title("Warning");
      box->set_icon(AlertBox::WARNING_ICON);
      box->add_text("Problem saving scene to file:");
      box->add_text(fullpath);
      box->add_button("OK");
      box->set_default(0);

      switch(status)
      {
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

void 
do_load(str_ptr fullpath)
{
   LOADobs::load_status_t status;

   cerr << "\ndo_load() - Loading...\n";

   NetStream s(fullpath, NetStream::ascii_r);

   // Clear the scene first, then hope nothing goes wrong in
   // loading cuz if it does we should then unclear the
   // scene, but we're not prepared to do that...
   do_clear();
	
   int old_cursor = VIEW::peek()->get_cursor();
   VIEW::peek()->set_cursor(WINSYS::CURSOR_WAIT);
   LOADobs::notify_load_obs(s, status, true, true);
   VIEW::peek()->set_cursor(old_cursor);

   if (status == LOADobs::LOAD_ERROR_NONE) {
      cerr << "do_load() - ...done.\n";

      WORLD::message(str_ptr("Loaded '") + fullpath + "'");

   } else {
      // XXX  - should unclear the scene to restore current state
      //        to what it was before the load attempt.

      cerr << "do_load() - ...aborted!!!" << endl;
      
      WORLD::message(str_ptr("Problem loading '") + fullpath + "'");

      AlertBox *box = VIEW::peek()->win()->alert_box();

      box->set_title("Warning");
      box->set_icon(AlertBox::WARNING_ICON);
      box->add_text("Problem loading scene from file:");
      box->add_text(fullpath);
      box->add_button("OK");
      box->set_default(0);

      switch(status) {
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

      if (box->display(true, alert_cbs, NULL, NULL, ALERT_LOAD_JOT_FAILED_CB))
         cerr << "do_save() - AlertBox displayed.\n";
      else
         cerr << "do_save() - AlertBox **FAILED** to display!!\n";
   }
}

int
animation_keys(const Event &e, State *&s)
{
   //Here we handle the keys for both the Recorder and Animator
   
   //*It cannot be the case that both are 'ON' simultaneously*

   assert(!(VIEW::peek()->recorder()->on() && VIEW::peek()->animator()->on()));

   //If they're both 'off' just check the activation keys
   if(!VIEW::peek()->recorder()->on() && !VIEW::peek()->animator()->on())
      {
         switch (e._c) 
            {
             case 'C': 
               return toggle_recorder(e,s);
               break;
             case 'X':
               VIEW::peek()->animator()->toggle_activation();
               break;
            };
      }
   //If the recorder's on just check its keys
   else if(VIEW::peek()->recorder()->on())
      {
         switch (e._c) 
            {
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
   else if(VIEW::peek()->animator()->on())
      {
         switch (e._c) 
            {
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
      }
   else
      {
         //HUH!?
         assert(0);
         return 0;
      }

   return 0;
}

int
render_mode(const Event &e, State *&s)
{
   switch (e._c) 
      {
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
set_pen(const Event & ev, State *&)
{
   
   if (ev._c == '.') 
   {
      BaseJOTapp::instance()->next_pen();
   }
   else if (ev._c == ',') 
   {
      BaseJOTapp::instance()->prev_pen();
   }

   VIEW::peek()->set_focus();

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
      err_msg("undo_redo: unknown key: %d (ASCII decimal code)",
              int(e._c));
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
    case 0: calc = new LoopLoc;           break;
    case 1: calc = new Hybrid2Loc;        break; 
    case 2: calc = new HybridVolPreserve; break;
   }
   assert(calc != 0);
   ctrl_mesh->set_subdiv_loc_calc(calc);
   ctrl_mesh->update();
   WORLD::message(calc->name() + " scheme in use");

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
unrefine(const Event&, State *&)
{
   BMESH* m = find_ctrl_mesh();
   if (!m || !LMESH::isa(m))
      return 0;

   ((LMESH*)m)->unrefine();

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
      }
      else {
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

   str_ptr fname = "out.sm";   

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
      WORLD::message(str_ptr("Wrote config to ") + Config::JOT_ROOT() + "jot.cfg");
   else
      WORLD::message(str_ptr("FAILED!! Writing config to ") + Config::JOT_ROOT() + "jot.cfg");

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
toggle_no_text(const Event&, State*&)
{
   if (!TEXT2D::toggle_suppress_draw())
      WORLD::message("Text on");
      
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
toggle_curvature_ui(const Event&, State *&)
{
   
   if(CurvatureUISingleton::Instance().is_vis(VIEW::peek())){
      
      CurvatureUISingleton::Instance().hide(VIEW::peek());
      
   } else {
      
         CurvatureUISingleton::Instance().show(VIEW::peek());
         VIEW::peek()->set_focus();
         
   }

   return 0;
   
}

// end of file jot.C
