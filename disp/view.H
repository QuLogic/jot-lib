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
#ifndef VIEW_H
#define VIEW_H

#include "disp/gel.H"
#include "disp/cam.H"
#include "disp/gel_filt.H"
#include "disp/color.H"
#include "disp/ray.H"
#include "disp/light.H"
#include "geom/image.H"         // disp should not include geom
#include "geom/texture.H"       //  ditto
#include "std/support.H"
#include "std/stop_watch.H"

class Animator;
class Recorder;
class WINSYS;
class FRAME_TIME_OBSERVER;

class FRAME_TIME_OBSERVER_list: public ARRAY<FRAME_TIME_OBSERVER*> {
 public:
   FRAME_TIME_OBSERVER_list(int n=0) : ARRAY<FRAME_TIME_OBSERVER*>(n) {}
   FRAME_TIME_OBSERVER_list(const FRAME_TIME_OBSERVER_list& l) :
      ARRAY<FRAME_TIME_OBSERVER*>(l) {}

   void frame_time_changed() const;
};

extern Cstr_ptr RSMOOTH_SHADE;
extern Cstr_ptr RFLAT_SHADE;
extern Cstr_ptr RSPEC_SHADE;
extern Cstr_ptr RHIDDEN_LINE;
extern Cstr_ptr RWIRE_FRAME;
extern Cstr_ptr RNORMALS;
extern Cstr_ptr RNORMALS_ONLY;
extern Cstr_ptr RCOLOR_ID;
extern Cstr_ptr RSHOW_TRI_STRIPS;
extern Cstr_ptr RKEY_LINE;
extern Cstr_ptr RSIL_FRAME;
extern Cstr_ptr SKYBOX_GRADIENT;

MAKE_PTR_SUBC(VIEW, SCHEDULER);
//
// Observe changes to VIEW's
//
class VIEWobs {

 public:
   typedef ARRAYptrs<VIEWobs *> view_list;
 protected:
   static view_list *_view_obs;
   static view_list *viewobs_list() { return _view_obs ? _view_obs: 
      _view_obs = new view_list;}
 public :
   virtual ~VIEWobs() {}
   virtual void notify_view   (CVIEWptr &, int) = 0;
   static  void notify_viewobs(CVIEWptr &v, int f) {
      for (int i=0; i<viewobs_list()->num(); i++)
         (*_view_obs)[i]->notify_view(v,f); }
   void view_obs  ()     { viewobs_list()->add_uniquely(this); }
   void unobs_view()     { viewobs_list()->rem         (this); }
};

#define CVIEWlist const VIEWlist
class VIEWlist : public LIST<VIEWptr> {
 public :
   VIEWlist(int num=16):LIST<VIEWptr>(num) { }

   void rem     (CVIEWptr &v) { LIST<VIEWptr>::rem(v); 
   VIEWobs::notify_viewobs(v,0); }
   void add     (CVIEWptr &v) { LIST<VIEWptr>::add_uniquely(v); 
   VIEWobs::notify_viewobs(v,1); }
};

//
// ThreadObs - get called back before a render thread starts rendering for
// the first time
//
class ThreadObs;
typedef ARRAYptrs<ThreadObs*> Threadobs_list;
class ThreadObs {
 protected:
   static Threadobs_list *_all_thread;
   static Threadobs_list *thread_list() { if (!_all_thread)
      _all_thread = new Threadobs_list; return _all_thread; }
 public:
   virtual ~ThreadObs() {}
   virtual void notify_render_thread    (CVIEWptr &) = 0;
   static  void notify_render_thread_obs(CVIEWptr &v) {
      for (int i=0; i < thread_list()->num(); i++)
         (*thread_list())[i]->notify_render_thread(v); }

   void thread_obs()           { thread_list()->add_uniquely(this); }
   void unobs_thread()         { thread_list()->rem         (this); }
};

class STENCILCB {
 public:
   virtual ~STENCILCB() {}
   virtual int  stencil_cb() = 0;
   virtual void stencil_bounds(mlib::XYpt_list &pts) = 0;
};

#define CCLEARobsptr const CLEARobsptr
MAKE_PTR_BASEC(CLEARobs);
class CLEARobs : public REFcounter {
 public :
   virtual void notify_clear(CVIEWptr &) = 0;
};


//
// VIEWimpl is the implementation interface that enables a VIEW to actually 
//          have a side effect on a display surface
//
class REF_CLASS(VIEWimpl) {
  protected:
   VIEWptr  _view;
  public:
   enum stereo_mode {
      NONE = 0,
      HMD,
      LCD,
      TWO_BUFFERS,
      LEFT_EYE_MONO,   // used for IBM's Yotta hardware
      RIGHT_EYE_MONO,  // used for IBM's Yotta hardware
      SCISSOR_STEREO   // XXX - obsolete?
   };
   virtual        ~VIEWimpl()                    {}
   virtual void   set_view(CVIEWptr &v)          { _view = v; }
   virtual void   set_rendering(Cstr_ptr &)      {}
   virtual void   set_stereo (stereo_mode)       {}
   virtual void   set_size   (int, int, int, int){}
   virtual void   set_cursor (int)               {} // Switch to 'i'th cursor
   virtual int    get_cursor ()                  { return -1;} 
   virtual void   set_focus()                    {}
   virtual void   draw_bb    (mlib::CWpt_list &) const {}
   virtual void   setup_lights(CAMdata::eye=CAMdata::MIDDLE) {}
   virtual int    paint()                        { return 0; }
   virtual void   swap_buffers()                 {}
   virtual void   prepare_buf_read()             {}
   virtual void   end_buf_read()                 {}
   virtual void   read_pixels(uchar *,bool a=false)  {}
   virtual int    stencil_draw(STENCILCB *, GELlist * = 0) { return 0; }
   virtual bool   antialias_check() { return false; }

   virtual void    draw_setup() {}
   virtual int     draw_frame(CAMdata::eye = CAMdata::MIDDLE) { return 0; }
};

//----------------------------------------------
//
//VIEW
//     Contains a list of objects to be displayed, dispatches events to FSA's
//
//----------------------------------------------
// MAKE_PTR_SUBC(VIEW, SCHEDULER); it's called higher up in the file

#define CHANGED(X) VIEWobs::notify_viewobs(this, X) 

class ThreadData;
//XXX - Added DATA_ITEM (seem to recall that the
//parent class order matters in some way, hope this
//is okay...)
class VIEW: public SCHEDULER, public DISPobs, public DATA_ITEM  {
 public: 
   enum filt {
      H_GEOM       = 0,
      H_TEXT       = (1 << 0),
      H_UNPICKABLE = (1 << 1),
      H_ALL        = (1 << 8)-1
   };

   enum change_t {
      LIGHTING_CHANGED = 0,
      COLOR_ALPHA_CHANGED,
      PAPER_CHANGED,
      TEXTURE_CHANGED,
      ANTIALIAS_CHANGED,
      UNKNOWN_CHANGED
   };

   enum render_mode_t {
      NORMAL_MODE = 0,
      OPAQUE_MODE,
      TRANSPARENT_MODE,
      RENDER_MODE_NUM
   };

   typedef              VIEWimpl::stereo_mode stereo_mode;

   /////////////////
   //DATA_ITEM stuff
   /////////////////
 private:
   static TAGlist  *_v_tags;
 protected:
   bool    _in_data_file;
 public:

   //XXX SCHEDULER is a base class that already
   //has class_name, etc.  Should use RTTI_METHODS2???
   //DEFINE_RTTI_METHODS_BASE("VIEW", CDATA_ITEM*);
   VIEW () : _view_id(-1)     { err_msg("VIEW::VIEW() - DUMMY CONSTRUCTOR!!!!!"); }
   virtual  DATA_ITEM *dup()  const { return new VIEW; }
   virtual  CTAGlist  &tags() const;
  
   //IO Stuff
   void     get_view_animator (TAGformat &d);
   void     put_view_animator (TAGformat &d) const;

   void     get_view_data_file (TAGformat &d);
   void     put_view_data_file (TAGformat &d) const;

   void     get_view_color (TAGformat &d);
   void     put_view_color (TAGformat &d) const;

   void     get_view_alpha (TAGformat &d);
   void     put_view_alpha (TAGformat &d) const;

   void     get_view_paper_use (TAGformat &d);
   void     put_view_paper_use (TAGformat &d) const;

   void     get_view_paper_name (TAGformat &d);
   void     put_view_paper_name (TAGformat &d) const;

   void     get_view_paper_active (TAGformat &d);
   void     put_view_paper_active (TAGformat &d) const;

   void     get_view_paper_brig (TAGformat &d);
   void     put_view_paper_brig (TAGformat &d) const;

   void     get_view_paper_cont (TAGformat &d);
   void     put_view_paper_cont (TAGformat &d) const;

   void     get_view_texture (TAGformat &d);
   void     put_view_texture (TAGformat &d) const;

   void     get_view_light_coords (TAGformat &d);
   void     put_view_light_coords (TAGformat &d) const;

   void     get_view_light_positional (TAGformat &d);
   void     put_view_light_positional (TAGformat &d) const;

   void     get_view_light_cam_space (TAGformat &d);
   void     put_view_light_cam_space (TAGformat &d) const;

   void     get_view_light_color_diff (TAGformat &d);
   void     put_view_light_color_diff (TAGformat &d) const;

   void     get_view_light_color_amb (TAGformat &d);
   void     put_view_light_color_amb (TAGformat &d) const;

   void     get_view_light_color_global (TAGformat &d);
   void     put_view_light_color_global (TAGformat &d) const;

   void     get_view_light_enable (TAGformat &d);
   void     put_view_light_enable (TAGformat &d) const;

   void     get_view_light_spot_direction(TAGformat &d);
   void     put_view_light_spot_direction(TAGformat &d) const;
   
   void     get_view_light_spot_exponent(TAGformat &d);
   void     put_view_light_spot_exponent(TAGformat &d)const;
   
   void     get_view_light_spot_cutoff(TAGformat &d);
   void     put_view_light_spot_cutoff(TAGformat &d)const ;
   
   void     get_view_light_constant_attenuation(TAGformat &d);
   void     put_view_light_constant_attenuation(TAGformat &d)const;
   
   void     get_view_light_linear_attenuation(TAGformat &d);
   void     put_view_light_linear_attenuation(TAGformat &d)const;

   void     get_view_light_quadratic_attenuation(TAGformat &d);
   void     put_view_light_quadratic_attenuation(TAGformat &d)const;

   void     get_view_antialias_enable (TAGformat &d);
   void     put_view_antialias_enable (TAGformat &d) const;

   void     get_view_antialias_mode (TAGformat &d);
   void     put_view_antialias_mode (TAGformat &d) const;

 protected:

   LIST<CLEARobsptr> _clear_obs;// callbacks for when display is cleared


   static VIEWlist _views;       // view stack used by points.H types
   static str_list _rend_types;  // list of rendering styles

   VIEWimpl     *_impl;          // display implementation

   static        int _num_views;
   const         int _view_id;

   ARRAY<STENCILCB*> _stencil_cbs;// Stencil buffers!

   str_ptr       _name;          // view's name
   CAMptr        _cam;           // camera definition for view
   CAMhist       _cam_hist;      // history of previous camera views 
   int           _cam_hist_cur;  // current place in history

   GELlist       _active;        // objects that are active
   GELlist       _drawn;         // objects in field of view (post-cull)

   int           _width;         // width of view
   int           _height;        // height of view
   COLOR         _bkgnd_col;     // background color of view

   stereo_mode   _stereo;        // whether we're in stereo mode
   str_ptr       _render_type;   // the current render technique
   int           _tris;          // number of triangles drawn

   bool          _is_clipping;   // Is a clipping plane in effect?
   mlib::Wplane  _clip_plane;

   int           _dont_swap;     // a buffer swap is pending a network synch
   int           _dont_draw;
   int           _messages_sent; // number of displays to synch with

   int           _has_scissor_region;
   double        _sxmin, _sxmax; // scissoring range (from -1 to 1)

   mlib::Wtransf _lens;          // "lens" for selecting a particular subregion
                                 // (or tile) of current window to render at 
                                 // full resolution of window:

   mlib::Wtransf _jitter;        // fullscene antialiasing

   Recorder*         _recorder;

   enum { MAX_LIGHTS = 8 };
 public:
   static int max_lights() { return MAX_LIGHTS; }
 protected:

   Light        _lights[MAX_LIGHTS];    // individual lights
   COLOR        _light_global_ambient;  // global ambient light

   static ARRAY<ARRAY<mlib::VEXEL>*> _jitters;

   //Used in constructor to setup defaults
   void              init_lights();
   void              init_jitter();

   double            _alpha;         // alpha of background (used by npr_view for papering)
   bool              _use_paper;     // whether paper effect should affect bkg
   str_ptr           _bkg_file;      // filename of background image (NULL_STR = none)
   TEXTUREptr        _bkg_tex;       // texture of the above image (setup by npr_view)
   mlib::Wtransf     _bkg_tf;        // NDC->UV bkg tex transform

   str_ptr           _data_file;     // if not null_str, the view is seralized to _data_file.view

   Animator*         _animator;

   render_mode_t     _render_mode;

   int               _antialias_mode;
   int               _antialias_enable;
   bool              _antialias_init;
  
 public:
   void              set_focus()                            { if (_impl) _impl->set_focus();  }

   void              set_jitter(int n, int i); //n==-1 resets to identity
   static int        get_jitter_num(int n)                  { assert(_jitters.valid_index(n)); return (*(_jitters[n])).num(); }
   static int        get_jitter_mode_num()                  { return _jitters.num(); }

   int               get_antialias_enable() const           { return _antialias_enable; }
   void              set_antialias_enable(bool a)           
      { _antialias_enable = a; if (_impl && !_impl->antialias_check()) _antialias_enable = 0; CHANGED(ANTIALIAS_CHANGED);}

   int               get_antialias_mode() const             { return _antialias_mode; }
   void              set_antialias_mode(int a) 
      { _antialias_mode = a; if (_impl && !_impl->antialias_check()) _antialias_enable = 0; CHANGED(ANTIALIAS_CHANGED);}

   render_mode_t     get_render_mode() const                { return _render_mode; }
   void              set_render_mode(render_mode_t r);

   Animator*         animator()                             { return _animator; }

   void              set_data_file(str_ptr f)               { _data_file = f; }
   Cstr_ptr&         get_data_file() const                  { return _data_file; }

   void              set_alpha(double a);
   double            get_alpha() const                      { return _alpha; }

   void              set_use_paper(bool p)                  { _use_paper = p; CHANGED(PAPER_CHANGED);}
   bool              get_use_paper() const                  { return _use_paper; }

   // Setting filename kills texture:
   void              set_bkg_file(str_ptr f)                { _bkg_file = f; _bkg_tex = NULL; CHANGED(TEXTURE_CHANGED);}
   
   // npr_view sets this from the filename:
   Cstr_ptr&         get_bkg_file() const                   { return _bkg_file; }
                                             
   void              set_bkg_tex(TEXTUREptr t)              { _bkg_tex = t; CHANGED(TEXTURE_CHANGED);}
   CTEXTUREptr&      get_bkg_tex() const                    { return _bkg_tex; }

   void              set_bkg_tf(mlib::CWtransf &tf)               { _bkg_tf = tf; }
   mlib::CWtransf&         get_bkg_tf() const                     { return _bkg_tf; }


   //******** GLOBAL AMBIENT LIGHT ********

   // Set global ambient light color:
   void light_set_global_ambient(CCOLOR &c) {
      _light_global_ambient = c;
      CHANGED(LIGHTING_CHANGED);
   }
   // Get global ambient light color:
   COLOR light_get_global_ambient() const {
      return _light_global_ambient;
   }

   //******** SETTING VALUES FOR LIGHT i ********

   // Set all values for light i:
   void set_light(int i, const Light& l) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i] = l;
         CHANGED(LIGHTING_CHANGED);
      }
   }
   // Set type of light i to positional
   void light_set_positional(int i, bool p) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._is_positional = p;
         CHANGED(LIGHTING_CHANGED);
      }
   }
   // Sets a directional light:
   void light_set_coordinates_v(int i, mlib::CWvec &v) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i].set_direction(v);
         CHANGED(LIGHTING_CHANGED);
      }
   }
   // Sets a positional light:
   void light_set_coordinates_p(int i,mlib::CWpt &p) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i].set_position(p);
         CHANGED(LIGHTING_CHANGED);
      }
   }
   // Tell if light coordinates are defined in camera space or world space:
   void light_set_in_cam_space(int i, bool c) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._is_in_cam_space = c;
         CHANGED(LIGHTING_CHANGED);
      }
   }
   // Set the ambient color for light i:
   void light_set_ambient(int i, CCOLOR& c) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._ambient_color = c;
         CHANGED(LIGHTING_CHANGED);
      }
   }
   // Set the diffuse color for light i:
   void light_set_diffuse(int i, CCOLOR& c) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._diffuse_color = c;
         CHANGED(LIGHTING_CHANGED);
      }
   }
   // Enable or disable light i
   void light_set_enable(int i, bool e) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._is_enabled = e;
         CHANGED(LIGHTING_CHANGED);
      }
   }

   void light_set_spot_direction(int i, mlib::CWvec &v) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._spot_direction = v;
         CHANGED(LIGHTING_CHANGED);
      }
   }
   
   void light_set_spot_exponent(int i, float s) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._spot_exponent = s;
         CHANGED(LIGHTING_CHANGED);
      }
   }

   void light_set_spot_cutoff(int i, float s) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._spot_cutoff = s;
         CHANGED(LIGHTING_CHANGED);
      }
   }

   void light_set_constant_attenuation(int i, float s) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._k0 = s;
         CHANGED(LIGHTING_CHANGED);
      }
   }
   void light_set_linear_attenuation(int i, float s) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._k1 = s;
         CHANGED(LIGHTING_CHANGED);
      }
   }
   void light_set_quadratic_attenuation(int i, float s) {
      if (0 <= i && i < MAX_LIGHTS) {
         _lights[i]._k2 = s;
         CHANGED(LIGHTING_CHANGED);
      }
   }



   //******** GETTING VALUES FOR LIGHT i ********

   // Get all properties for light i:
   const Light& get_light(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i];
   }
   // Return coordinates for light i:
   mlib::Wvec light_get_coordinates_v(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i].get_direction();
   }
   mlib::Wpt light_get_coordinates_p(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i].get_position();
   }
   bool light_get_positional(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i]._is_positional;
   }   
   bool light_get_in_cam_space(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i]._is_in_cam_space;
   }
   COLOR light_get_diffuse(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i]._diffuse_color;
   }
   COLOR light_get_ambient(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i]._ambient_color;
   }

   bool light_get_enable(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i]._is_enabled;
   }

   mlib::Wvec light_get_spot_direction(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i]._spot_direction;     
   }
   
   float light_get_spot_exponent(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i]._spot_exponent;
   }

   float light_get_spot_cutoff(int i) const {
      assert(0 <= i && i < MAX_LIGHTS);
      return _lights[i]._spot_cutoff;   
   }

   float light_get_constant_attenuation(int i) const {
       assert(0 <= i && i < MAX_LIGHTS);
       return _lights[i]._k0;
   }
   float light_get_linear_attenuation(int i) const {
       assert(0 <= i && i < MAX_LIGHTS);
       return _lights[i]._k1;
   }
   float light_get_quadratic_attenuation(int i) const {
       assert(0 <= i && i < MAX_LIGHTS);
       return _lights[i]._k2;
   }

   // Return true if any light is enabled:
   bool lights_on() const {
      for (int i=0; i<MAX_LIGHTS; i++)
         if (_lights[i]._is_enabled)
            return true;
      return false;
   }

 protected:

   // scale factor for drawing lines when grabbing screen
   // image at hi-res:
   double               _line_scale;

   bool                 _grabbing_screen;

   double               _spf;              // target seconds per frames
   stop_watch           _spf_timer;        // timer for maintaining target fps
   double               _frame_time;       // the "time" of the current frame
   static unsigned int  _stamp;            // frame #: incremented by draw()
   static double        _pix_to_ndc_scale; // scale from pixel coords to NDC
                                           // coords. updated by draw()

   // objects that get notified when _frame_time changes:
   FRAME_TIME_OBSERVER_list _frame_time_observers;

   WINSYS              *_win;

   SCREENptr            _screen;

 public:
   VIEW (Cstr_ptr &s, WINSYS *win, VIEWimpl *i);
   virtual              ~VIEW () { while (_jitters.num() > 0) delete _jitters.pop(); }

   static int           stack_size()               { return _views.num(); }
   static void          push(CVIEWptr &view)       { _views += view; }
   static void          pop ()                     { _views.pop(); }
#ifdef USE_PTHREAD
   static VIEWptr       peek();
   static const VIEW    *peek_ptr();
#else
   static VIEWptr       peek()                     { return _views.last(); }
   static const VIEW    *peek_ptr()                 { return &*_views.last(); }
#endif
   static CAMptr        peek_cam()                 { return peek_ptr()->cam(); }
   static CCAMptr       &peek_cam_const()          { return peek_ptr()->cam(); }
   static void          peek_size(int &w, int &h)  { peek_ptr()->get_size(w,h);}

   // return width and height of current view in one convenient package:
   static mlib::Point2i cur_size() {
      mlib::Point2i ret;
      peek_size(ret[0],ret[1]);
      return ret;
   }

   double frame_time() const { return _frame_time; }
   void set_frame_time(double t);

   void add_frame_time_observer(FRAME_TIME_OBSERVER* obs) {
      _frame_time_observers.add_uniquely(obs);
   }
   void remove_frame_time_observer(FRAME_TIME_OBSERVER* obs) {
      _frame_time_observers.rem(obs);
   }

   static unsigned int  stamp()                    { return _stamp; }

   static void          add_rend_type(Cstr_ptr &r) { _rend_types += r; }
   static Cstr_list&    rend_list()                { return _rend_types; }
   static void          set_rend_list(Cstr_list& l){ _rend_types = l; }

   static int           num_views()                { return _num_views;}

   //recorder access
   Recorder*    recorder()                 { return _recorder;}

   /// Returns the location of the camera:
   static mlib::Wpt       eye()            { return peek_cam()->data()->from(); }
   /// Returns a unit vector pointing from the camera to point p:
   static mlib::Wvec      eye_vec(mlib::CWpt& p) { return (p - eye()).normalized(); }

   //!METHS: core components of a view
   str_ptr              name()               const { return str_ptr(_name); }
   VIEWimpl             *impl()               const { return _impl; }
   CGELlist             &drawn()              const { return _drawn; }
   GELlist              &drawn()                    { return _drawn; }
   CGELlist             &active()             const { return _active; }
   CCAMptr              &cam()                const { return _cam; }
   CAMptr               cam()                      { return _cam; }

   // Camera history
   void save_cam(CCAMptr c=0);
   void fwd_cam_hist();
   void bk_cam_hist();

   // Copies camera params into VIEW's cam:
   void copy_cam(CCAMptr &c);
  
   // VIEW switches to using the given CAM:
   void use_cam(CCAMptr &c);
   
   int         tick();
   int         view_id()            const { return _view_id; }

   //!METHS: general rendering parameters
   void        stereo(stereo_mode m)      { _stereo      = m;}
   stereo_mode stereo()             const { return   _stereo;}
   void        set_rendering(Cstr_ptr &s) { _render_type = s;
   if (_impl) _impl->set_rendering(s);}
   Cstr_ptr&   rendering()          const { return _render_type; }
   int         tris()               const { return _tris; }
   double      line_scale()         const { return _line_scale; }
   bool        grabbing_screen()    const { return _grabbing_screen; }
   void        set_grabbing_screen( bool g) { _grabbing_screen = g; }
   SCREENptr   screen()             const { return _screen; }
   const SCREEN  *screen_ptr()      const { return &*_screen;}
   SCREEN     *screen_ptr()               { return &*_screen;}
   bool        is_clipping()        const { return _is_clipping;}
   mlib::CWplane    &clip_plane ()        const { return _clip_plane;}
   CCOLOR&     color()              const { return _bkgnd_col; }

   ARRAY<STENCILCB*> &stencil_cbs()       { return _stencil_cbs; }
   void        add_stencil (STENCILCB *cb){ _stencil_cbs += cb; }
   void        rem_stencil (STENCILCB *cb){ _stencil_cbs -= cb; }
   void        set_color(CCOLOR &c);
   void        set_lens(mlib::CWtransf &l)      { _lens = l; }
   mlib::Wtransf     get_lens() { return _lens; }
   mlib::Wtransf     get_jitter() { return _jitter; }
   void        set_screen(CSCREENptr &s)  { _screen = s; }
   void        set_is_clipping(bool clip) { _is_clipping = clip;}
   void        set_clip_plane(mlib::CWplane &p) { set_is_clipping(true); 
   _clip_plane = p; }
   void        set_view_impl(VIEWimpl *i) { _impl = i; }

   int         display  (CGELptr &);
   int         undisplay(CGELptr &);
   void        notify(CGELptr &g, int f)  { if(f) display(g);else undisplay(g);}

   //!METHS: core functions provided by a view
   void        paint           ();
   void        setup_lights    (CAMdata::eye e=CAMdata::MIDDLE) {
      if (_impl) _impl->setup_lights(e);
   }
   RAYhit      intersect_others(RAYhit&, CGELlist&,     filt f=H_GEOM) const;
   RAYhit      intersect       (mlib::CXYpt  &x,              filt f=H_GEOM) const;
   RAYhit      intersect       (RAYhit &r,              filt f=H_GEOM) const;
   RAYhit      intersect       (RAYhit &r, GELptr &,    filt f=H_GEOM) const;
   RAYhit      intersect       (RAYhit &r, Cstr_ptr &cn,filt f=H_GEOM) const;
   RAYhit      intersect       (RAYhit &r, const GELFILTlist &)        const;
   RAYnear     nearest         (RAYnear&r,              filt f=H_GEOM) const;
   RAYnear     nearest         (RAYnear&r, Cstr_ptr&,   filt f=H_GEOM) const;
   GELlist     inside          (mlib::CXYpt_list &lasso) const;

   //!METHS: scissor region accessors/setters
   void        set_scissor_flag(int b)    { _has_scissor_region = b;   }
   void        set_scissor_xmin(double x) { _sxmin=clamp(x,-1.,1.);
   set_scissor_flag(1); }
   void        set_scissor_xmax(double x) { _sxmax=clamp(x,-1.,1.);
   set_scissor_flag(1); }
   int         has_scissor_region() const { return _has_scissor_region;}
   double      scissor_xmin()       const { return _sxmin; }
   double      scissor_xmax()       const { return _sxmax; }

   //!METHS: viewport specific functions
   WINSYS     *win()                 const { return _win; }
   int         width()               const { return _width; }
   int         height()              const { return _height; }
   void        get_size(int&w, int&h)const { w = _width; 
   h = (stereo()!=VIEWimpl::HMD) ?
      _height : _height/2; }
   double      ndc2pix_scale()       const { return min(_width,_height)/2.0; }
   double      aspect_x()            const { return (_width>_height) ?
                                                double(_width)/_height : 1.0; }
   double      aspect_y()            const { return (_width>_height) ? 1.0 :
      double(_height)/_width; }

   // Bring the camera back to where it 
   // can see everything in the scene.
   // (Useful when the camera gets lost in space):
   void viewall();

   static double pix_to_ndc_scale()         { return _views.empty() ? 
                                                 1.0 : 1.0/peek()->ndc2pix_scale();}

   //!METHS: interface delegated to an implementation object
   int         stencil_draw(STENCILCB *cb, GELlist *objs= 0) { return _impl ? 
                                                                  _impl->stencil_draw(cb,objs) : 0; }
   void        set_size      (int w, int h, int x, int y);
   void        set_cursor    (int i)       { if (_impl) _impl->set_cursor(i); }
   int         get_cursor    ()            { return ((_impl)?(_impl->get_cursor()):(-1)); }
   void        draw_bb(mlib::CWpt_list &p) const { if (_impl) _impl->draw_bb(p); }

   // Grabs current scene as a hi-res image, scaled n times
   // the size of the current window in each dimension:
   int         screen_grab(int scale, Cstr_ptr &filename);
   void        screen_grab(int scale, Image &im);

   //!METHS: callbacks provided by the view 
   void         notify_clearobs()       { for (int i=0; i<_clear_obs.num(); i++)
      _clear_obs[i]->notify_clear(this);}
   void         clear_obs  (CCLEARobsptr &o) { _clear_obs.add_uniquely(o); }
   void         unobs_clear(CCLEARobsptr &o) { _clear_obs.rem(o); }


   //!METHS: methods for synchronizing multiple views
   int         messages_sent()      const  { return _messages_sent;}
   void        message_recvd()             { _messages_sent--;}
   int         dont_swap()          const  { return _dont_swap; }
   int         dont_draw()          const  { return _dont_draw; }
   void        swap();
   void        set_dont_swap(int f=1)      { _dont_swap = f; }
   void        set_dont_draw(int f=1)      { _dont_draw = f; }
   void        wait_for_displays(int n)    { assert(!_dont_draw && !_dont_swap);
   _messages_sent = n;
   if (n > 0) 
      _dont_draw = _dont_swap = 1; }

   // Projection matrices:
   //   Each returns the matrix to use for drawing 
   //   the given type of point to the screen.
   mlib::Wtransf wpt_proj(SCREENptr s = SCREENptr(),
                          CAMdata::eye e=CAMdata::MIDDLE) const;
   mlib::Wtransf xypt_proj()            const;
   mlib::Wtransf ndc_proj()             const;
   mlib::Wtransf pix_proj()             const;
   mlib::Wtransf wpt_to_pix_proj(SCREENptr s = SCREENptr(),
				     CAMdata::eye e=CAMdata::MIDDLE) const;

   mlib::Wtransf eye_to_pix_proj(SCREENptr s = SCREENptr(),
				     CAMdata::eye e=CAMdata::MIDDLE) const;

   // The matrix that transforms points from world space to eye space:
   mlib::Wtransf world_to_eye(CAMdata::eye e=CAMdata::MIDDLE) const {
      return cam()->data()->xform(screen(), e);
   }
   mlib::Wtransf eye_to_world(CAMdata::eye e=CAMdata::MIDDLE) const {
      return world_to_eye(e).inverse();
   }

   DEFINE_RTTI_METHODS("VIEW", SCHEDULER, CSCHEDULERptr);
};

extern VIEWlist  VIEWS;              // List of all views 
extern hashvar<int> DONOT_CLIP_OBJ;  // objs unaffected by clip plane

// Gives length in pixels of a segment strarting at the wpt given and 
// going in an arbitrary direction by length given, does it by moving
// the point onto a at_vector and then projecting world length
// into PIXEL length
double at_length(mlib::CWpt& p, double length);

#endif // VIEW_H

// end of file view.H
