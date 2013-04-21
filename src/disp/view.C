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

#include "std/config.H"

#include "std/stop_watch.H"
#include "std/thread.H"

#include "disp/colors.H"
#include "disp/ray.H"
#include "disp/gel.H"
#include "disp/cam_focus.H"
#include "disp/recorder.H"
#include "disp/animator.H"
#include "disp/frame_time_observer.H"
#include "disp/paper_effect_base.H"
#include "disp/jitter.H"
#include "net/io_manager.H"

using namespace mlib;

#define DEFAULT_LIGHT_COORD_0          Wvec::Z()
#define DEFAULT_LIGHT_POSITIONAL_0     false
#define DEFAULT_LIGHT_IN_CAM_SPACE_0   true
#define DEFAULT_LIGHT_DIFFUSE_0        Color::grey7
#define DEFAULT_LIGHT_AMBIENT_0        COLOR::black
#define DEFAULT_LIGHT_SPECULAR_0       Color::grey4
#define DEFAULT_LIGHT_ENABLE_0         true

#define DEFAULT_LIGHT_COORD_1          Wvec::Z()
#define DEFAULT_LIGHT_POSITIONAL_1     false
#define DEFAULT_LIGHT_IN_CAM_SPACE_1   true
#define DEFAULT_LIGHT_DIFFUSE_1        Color::grey4
#define DEFAULT_LIGHT_AMBIENT_1        COLOR::black
#define DEFAULT_LIGHT_SPECULAR_1       COLOR::black
#define DEFAULT_LIGHT_ENABLE_1         false

#define DEFAULT_LIGHT_COORD_2          Wvec::Z()
#define DEFAULT_LIGHT_POSITIONAL_2     false
#define DEFAULT_LIGHT_IN_CAM_SPACE_2   true
#define DEFAULT_LIGHT_DIFFUSE_2        Color::grey4
#define DEFAULT_LIGHT_AMBIENT_2        COLOR::black
#define DEFAULT_LIGHT_SPECULAR_2       COLOR::black
#define DEFAULT_LIGHT_ENABLE_2         false

#define DEFAULT_LIGHT_COORD_3          Wvec::Z()
#define DEFAULT_LIGHT_POSITIONAL_3     false
#define DEFAULT_LIGHT_IN_CAM_SPACE_3   true
#define DEFAULT_LIGHT_DIFFUSE_3        Color::grey4
#define DEFAULT_LIGHT_AMBIENT_3        COLOR::black
#define DEFAULT_LIGHT_SPECULAR_3       COLOR::black
#define DEFAULT_LIGHT_ENABLE_3         false

#define DEFAULT_LIGHT_GLOBAL_AMBIENT   COLOR(0.1,0.1,0.1)

hashvar<int>  DONOT_CLIP_OBJ("DONOT_CLIP_OBJ", 0, 1);
int VIEW::_num_views = 0;
Threadobs_list *ThreadObs::_all_thread;

bool multithread = Config::get_var_bool("JOT_MULTITHREAD",false,true);

//
// VIEW - contains a list of objects to be displayed and handles events that
// operate on those objects
//

//
// Standard rendering modes
//
Cstr_ptr RSMOOTH_SHADE   ("Smooth Shading");
Cstr_ptr RFLAT_SHADE     ("Flat Shading");
Cstr_ptr RHIDDEN_LINE    ("Hidden Line");
Cstr_ptr RWIRE_FRAME     ("Wireframe");
Cstr_ptr RNORMALS        ("Normals");
Cstr_ptr RCOLOR_ID       ("Color ID");
Cstr_ptr RSHOW_TRI_STRIPS("Show tri-strips");
Cstr_ptr RKEY_LINE       ("Key Line");
Cstr_ptr RSIL_FRAME      ("Sil Frame");

//
// List of rendering types
//
str_list                   VIEW::_rend_types(25); 
VIEWlist                   VIEW::_views;
VIEWlist                   VIEWS;
VIEWobs::view_list        *VIEWobs::_view_obs;

unsigned int    VIEW::_stamp                    = 0;
double          VIEW::_pix_to_ndc_scale         = 0;

void 
FRAME_TIME_OBSERVER_list::frame_time_changed() const 
{
   for (int i=0; i<num(); i++)
      (*this)[i]->frame_time_changed();
}

#ifdef USE_PTHREAD
class ThreadToView : public ThreadObs {
   public:
      virtual void notify_render_thread(CVIEWptr &v) {
         assert(_instance); 
         _data.set((VIEW *) &*v);
      }
      static VIEW *get() { assert(_instance); return (VIEW *) _data.get();}
      static void make_if_needed() {
         if (!_instance) {_instance = new ThreadToView();}
      }
   protected:
      ThreadToView() { thread_obs(); }
      static ThreadData    _data;
      static ThreadToView *_instance;
};

ThreadData ThreadToView::_data;
ThreadToView *ThreadToView::_instance = 0;
#endif


/* ------------------ VIEW class definitions --------------------- */

TAGlist *            VIEW::_v_tags = 0;

ARRAY<ARRAY<VEXEL>*> VIEW::_jitters;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
VIEW::tags() const
{
   if (!_v_tags) {
      _v_tags = new TAGlist;

      *_v_tags += new TAG_meth<VIEW>(
         "view_animator",
         &VIEW::put_view_animator,
         &VIEW::get_view_animator,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_data_file",
         &VIEW::put_view_data_file,
         &VIEW::get_view_data_file,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_color",
         &VIEW::put_view_color,
         &VIEW::get_view_color,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_alpha",
         &VIEW::put_view_alpha,
         &VIEW::get_view_alpha,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_paper_use",
         &VIEW::put_view_paper_use,
         &VIEW::get_view_paper_use,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_paper_name",
         &VIEW::put_view_paper_name,
         &VIEW::get_view_paper_name,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_paper_active",
         &VIEW::put_view_paper_active,
         &VIEW::get_view_paper_active,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_paper_brig",
         &VIEW::put_view_paper_brig,
         &VIEW::get_view_paper_brig,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_paper_cont",
         &VIEW::put_view_paper_cont,
         &VIEW::get_view_paper_cont,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_texture",
         &VIEW::put_view_texture,
         &VIEW::get_view_texture,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_coords",
         &VIEW::put_view_light_coords,
         &VIEW::get_view_light_coords,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_positional",
         &VIEW::put_view_light_positional,
         &VIEW::get_view_light_positional,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_cam_space",
         &VIEW::put_view_light_cam_space,
         &VIEW::get_view_light_cam_space,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_color_diff",
         &VIEW::put_view_light_color_diff,
         &VIEW::get_view_light_color_diff,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_color_amb",
         &VIEW::put_view_light_color_amb,
         &VIEW::get_view_light_color_amb,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_global",
         &VIEW::put_view_light_color_global,
         &VIEW::get_view_light_color_global,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_enable",
         &VIEW::put_view_light_enable,
         &VIEW::get_view_light_enable,
         1);
      
      *_v_tags += new TAG_meth<VIEW>(
         "view_light_spot_direction",
         &VIEW::put_view_light_spot_direction,
         &VIEW::get_view_light_spot_direction,
         1);
      
      *_v_tags += new TAG_meth<VIEW>(
         "view_light_spot_exponent",
         &VIEW::put_view_light_spot_exponent,
         &VIEW::get_view_light_spot_exponent,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_spot_cutoff",
         &VIEW::put_view_light_spot_cutoff,
         &VIEW::get_view_light_spot_cutoff,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_light_constant_attenuationt",
         &VIEW::put_view_light_constant_attenuation,
         &VIEW::get_view_light_constant_attenuation,
         1);
      *_v_tags += new TAG_meth<VIEW>(
         "view_light_linear_attenuation",
         &VIEW::put_view_light_linear_attenuation,
         &VIEW::get_view_light_linear_attenuation,
         1);
      *_v_tags += new TAG_meth<VIEW>(
         "view_light_quadratic_attenuation",
         &VIEW::put_view_light_quadratic_attenuation,
         &VIEW::get_view_light_quadratic_attenuation,
         1);
   
     
      *_v_tags += new TAG_meth<VIEW>(
         "view_antialias_enable",
         &VIEW::put_view_antialias_enable,
         &VIEW::get_view_antialias_enable,
         1);

      *_v_tags += new TAG_meth<VIEW>(
         "view_antialias_mode",
         &VIEW::put_view_antialias_mode,
         &VIEW::get_view_antialias_mode,
         1);


   }
   return *_v_tags;
}


/////////////////////////////////////
// get_view_data_file()
/////////////////////////////////////
void
VIEW::get_view_data_file (TAGformat &d) 
{
   // Sanity check...
   assert(!_in_data_file);

   str_ptr str;
   *d >> str;      

   if (str == "NULL_STR") {
      err_mesg(ERR_LEV_SPAM, "VIEW::get_view_data_file() - Loaded NULL string.");
      _data_file = NULL_STR;
   } else {
      err_mesg(ERR_LEV_SPAM, "VIEW::get_view_data_file() - Loaded string: '%s'.", **str);
      _data_file = str;

      str_ptr fname = IOManager::load_prefix() + str + ".view";

      err_mesg(ERR_LEV_SPAM, "VIEW::get_view_data_file() - Opening: '%s'...", **fname);

      fstream fin;
#if (defined (WIN32) && defined(_MSC_VER) && (_MSC_VER <=1300)) /*VS 6.0*/
      fin.open(**(fname),ios::in | ios::nocreate);
#else
      fin.open(**(fname),ios::in);
#endif
      if (!fin) 
      {
         err_msg("VIEW::get_view_data_file() - Could not open: '%s'!!", **fname);
         //but leave the _data_file as is so future
         //serialization could create it
      }
      else
      {
         STDdstream s(&fin);
         s >> str;

         if (str != VIEW::static_name()) 
         {
            err_msg("VIEW::get_view_data_file() - Not 'VIEW': '%s'!!", **str);
         }
         else
         {
            _in_data_file = true;
            decode(s);
            _in_data_file = false;
         }
      }
   }
}

/////////////////////////////////////
// put_view_data_file()
/////////////////////////////////////
void
VIEW::put_view_data_file (TAGformat &d) const
{
   //Ignore this tag if we're presently writing to
   //an external view file...
   if (_in_data_file) return;

   //If there's not view file, dump the null string
   if (_data_file == NULL_STR)
   {
      d.id();
      *d << str_ptr("NULL_STR");
      d.end_id();
   }
   //Otherwise, try to open the file, then write the
   //tags to the external file, and dump the filename 
   //to the given tag stream
   else
   {
      str_ptr fname = IOManager::save_prefix() + _data_file + ".view";
      fstream fout;
      fout.open(**fname,ios::out);
      //If this fails, then dump the null string
      if (!fout) 
      {
         err_msg("VIEW::put_view_data_file -  Could not open: '%s', so changing to using no external npr file...!", **fname);   

         //and actually change view's policy so the
         //view tags serialize into the main stream
         ((str_ptr)_data_file) = NULL_STR;

         d.id();
         *d << str_ptr("NULL_STR");
         d.end_id();
      }
      //Otherwise, do the right thing
      else
      {
         d.id();
         *d << _data_file;
         d.end_id();

         //Set the flag so tags will know we're in a data file
         
         // the ((VIEW*)this)-> stuff is to "cast away" the 
         // const of this function so that the _in_data_file
         // member can be modified
         ((VIEW*)this)->_in_data_file = true;
         STDdstream stream(&fout);
         format(stream);
         ((VIEW*)this)->_in_data_file = false;

      }
   }
}

/////////////////////////////////////
// get_view_animator()
/////////////////////////////////////
void
VIEW::get_view_animator(TAGformat &d) 
{
   //Sanity check...
   assert(!_in_data_file);

   str_ptr str;
   *d >> str;      

   if (str != Animator::static_name()) 
   {
      err_msg("VIEW::get_view_animator() - 'Not Animator': '%s'!!", **str);
   }
   else
   {
      assert(_animator);

      _animator->decode(*d);
   }
}

/////////////////////////////////////
// put_view_animator()
/////////////////////////////////////
void
VIEW::put_view_animator(TAGformat &d) const
{
   //Ignore this tag if we're presently writing to
   //an external view file...
   if (_in_data_file) return;

   assert(_animator);

   d.id();
   _animator->format(*d);
   d.end_id();

}

/////////////////////////////////////
// get_view_color()
/////////////////////////////////////
void
VIEW::get_view_color(TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   COLOR c;
   *d >> c;
   set_color(c);
 
}

/////////////////////////////////////
// put_view_color()
/////////////////////////////////////
void
VIEW::put_view_color(TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << color();
   d.end_id();
  
}

/////////////////////////////////////
// get_view_alpha()
/////////////////////////////////////
void
VIEW::get_view_alpha (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   double a;
   *d >> a;
   set_alpha(a);
}

/////////////////////////////////////
// put_view_alpha()
/////////////////////////////////////
void
VIEW::put_view_alpha (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << get_alpha();
   d.end_id();
   
}

/////////////////////////////////////
// get_view_paper_use()
/////////////////////////////////////
void
VIEW::get_view_paper_use (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   int p;
   *d >> p;
   set_use_paper((p==1)?true:false);   
}

/////////////////////////////////////
// put_view_paper_use()
/////////////////////////////////////
void
VIEW::put_view_paper_use (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << ((get_use_paper())?(1):(0));
   d.end_id();
   
}

/////////////////////////////////////
// get_view_paper_name()
/////////////////////////////////////
void
VIEW::get_view_paper_name (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   //XXX - May need something to handle filenames with spaces
   str_ptr str, tex, space;
   *d >> str;      

   //XXX - Ooops... serializing "NULL_STR" and str_ptr("NULL_STR")
   //      yields different results.  The former doesn't put a trialing
   //      space, so we've been adding it manually, but that
   //      needs to be parsed on binary streams... I should go thourgh and
   //      clean all of this up in backward compatible way, chanigng
   //      all cases to the later...
   if (!(*d).ascii()) *d >> space; 

   if (str == "NULL_STR") 
   {
      tex = NULL_STR;
      err_mesg(ERR_LEV_SPAM, "VIEW::get_view_paper_name() - Loaded NULL string.");
   }
   else
   {
      tex = str;
      err_mesg(ERR_LEV_SPAM, "VIEW::get_view_paper_name() - Loaded string: '%s'", **tex);
   }
   PaperEffectBase::set_paper_tex(tex);
   
}

/////////////////////////////////////
// put_view_paper_name()
/////////////////////////////////////
void
VIEW::put_view_paper_name (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));
   //XXX - May need something to handle filenames with spaces

   d.id();
   if (PaperEffectBase::get_paper_tex() == NULL_STR)
   {
      err_mesg(ERR_LEV_SPAM, "VIEW::put_view_paper_name() - Wrote NULL string.");
      *d << "NULL_STR";
      *d << " ";
   }
   else
   {
      //Here we strip off JOT_ROOT
      *d << PaperEffectBase::get_paper_tex();
      *d << " ";
      err_mesg(ERR_LEV_SPAM, "VIEW::put_view_paper_name() - Wrote string: '%s'", **PaperEffectBase::get_paper_tex());
   }
   d.end_id();
}

/////////////////////////////////////
// get_view_paper_active()
/////////////////////////////////////
void
VIEW::get_view_paper_active (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   int a;
   *d >> a;
   PaperEffectBase::set_delayed_activate(a==1);
}

/////////////////////////////////////
// put_view_paper_active()
/////////////////////////////////////
void
VIEW::put_view_paper_active (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << ((PaperEffectBase::is_active())?(1):(0));
   d.end_id();
   
}

/////////////////////////////////////
// get_view_paper_cont()
/////////////////////////////////////
void
VIEW::get_view_paper_cont (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   float c;
   *d >> c;
   PaperEffectBase::set_cont(c);
}

/////////////////////////////////////
// put_view_paper_cont()
/////////////////////////////////////
void
VIEW::put_view_paper_cont (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << PaperEffectBase::get_cont();
   d.end_id();
   
}

/////////////////////////////////////
// get_view_paper_brig()
/////////////////////////////////////
void
VIEW::get_view_paper_brig (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   float b;
   *d >> b;
   PaperEffectBase::set_brig(b);
}

/////////////////////////////////////
// put_view_paper_brig()
/////////////////////////////////////
void
VIEW::put_view_paper_brig (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << PaperEffectBase::get_brig();
   d.end_id();
   
}
/////////////////////////////////////
// get_view_texture()
/////////////////////////////////////
void
VIEW::get_view_texture (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   str_ptr str, space;
   *d >> str;      

   if (!(*d).ascii()) *d >> space; 
   
   if (str == "NULL_STR") 
   {
      err_mesg(ERR_LEV_SPAM, "VIEW::get_view_texture() - Loaded NULL string.");
      set_bkg_file(NULL_STR);
   }
   else
   {
      err_mesg(ERR_LEV_SPAM, "VIEW::get_view_texture() - Loaded string: '%s'", **str);
      set_bkg_file(Config::JOT_ROOT() + str);
   }
   
}

/////////////////////////////////////
// put_view_texture()
/////////////////////////////////////
void
VIEW::put_view_texture (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   //XXX - May need something to handle filenames with spaces

   d.id();
   if (get_bkg_file() == NULL_STR)
   {
      err_mesg(ERR_LEV_SPAM, "VIEW::put_view_texture() - Wrote NULL string.");
      *d << "NULL_STR";
      *d << " ";
   }
   else
   {
      //Here we strip off JOT_ROOT
      str_ptr tex = get_bkg_file();
      str_ptr str;
      int i;
      for (i=Config::JOT_ROOT().len(); i<(int)tex.len(); i++)
         str = str + str_ptr(tex[i]);
      *d << **str;
      *d << " ";
      err_mesg(ERR_LEV_SPAM, "VIEW::put_view_texture() - Wrote string: '%s'", **str);
   }
   d.end_id();
   
}

/////////////////////////////////////
// get_view_light_coords()
/////////////////////////////////////
void
VIEW::get_view_light_coords (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   ARRAY<Wvec> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<c.num(); i++)
      light_set_coordinates_v(i,c[i]);
   
}

/////////////////////////////////////
// put_view_light_coords()
/////////////////////////////////////
void
VIEW::put_view_light_coords (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;

   ARRAY<Wvec> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add(light_get_coordinates_v(i));
    d.id();
   *d << c;
   d.end_id();
}

/////////////////////////////////////
// get_view_light_positional()
/////////////////////////////////////
void
VIEW::get_view_light_positional (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   ARRAY<int> p;
   *d >> p;
   assert(p.num() <= MAX_LIGHTS);
   for (int i=0; i<MAX_LIGHTS; i++)
      light_set_positional(i,(p[i]==1)?(true):(false));
}

/////////////////////////////////////
// put_view_light_positional()
/////////////////////////////////////
void
VIEW::put_view_light_positional (TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;

   ARRAY<int> p;
   for (int i=0; i<MAX_LIGHTS; i++)
      p.add((light_get_positional(i))?(1):(0));
   d.id();
   *d << p;
   d.end_id();
  
}

/////////////////////////////////////
// get_view_light_cam_space()
/////////////////////////////////////
void
VIEW::get_view_light_cam_space (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   ARRAY<int> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<MAX_LIGHTS; i++)
      light_set_in_cam_space(i,((c[i]==1)?(true):(false)));
   
}

/////////////////////////////////////
// put_view_light_cam_space()
/////////////////////////////////////
void
VIEW::put_view_light_cam_space (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));

   ARRAY<int> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add((light_get_in_cam_space(i))?(1):(0));
    d.id();
   *d << c;
   d.end_id();
      
}

/////////////////////////////////////
// get_view_light_color_diff()
/////////////////////////////////////
void
VIEW::get_view_light_color_diff (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   ARRAY<COLOR> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<MAX_LIGHTS; i++)
      light_set_diffuse(i,c[i]);
}

/////////////////////////////////////
// put_view_light_color_diff()
/////////////////////////////////////
void
VIEW::put_view_light_color_diff (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));

   ARRAY<COLOR> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add(light_get_diffuse(i));
   d.id();
   *d << c;
   d.end_id();   
   
}

/////////////////////////////////////
// get_view_light_color_amb()
/////////////////////////////////////
void
VIEW::get_view_light_color_amb (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   ARRAY<COLOR> a;
   *d >> a;
   assert(a.num() <= MAX_LIGHTS);
   for (int i=0; i<MAX_LIGHTS; i++)
      light_set_ambient(i,a[i]);
   
}

/////////////////////////////////////
// put_view_light_color_amb()
/////////////////////////////////////
void
VIEW::put_view_light_color_amb (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));

   ARRAY<COLOR> a;
   for (int i=0; i<MAX_LIGHTS; i++)
      a.add(light_get_ambient(i));

   d.id();
   *d << a;
   d.end_id();   
   
}

/////////////////////////////////////
// get_view_light_color_global()
/////////////////////////////////////
void
VIEW::get_view_light_color_global (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   COLOR g;
   *d >> g;
   light_set_global_ambient(g);
 
}

/////////////////////////////////////
// put_view_light_color_global()
/////////////////////////////////////
void
VIEW::put_view_light_color_global (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << light_get_global_ambient();
   d.end_id();
  
}

/////////////////////////////////////
// get_view_light_enable()
/////////////////////////////////////
void
VIEW::get_view_light_enable (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   ARRAY<int> e;
   *d >> e;
   assert(e.num() <= MAX_LIGHTS);
   for (int i=0; i<MAX_LIGHTS; i++)
      light_set_enable(i,(e[i]==1)?(true):(false));
  
}

/////////////////////////////////////
// put_view_light_enable()
/////////////////////////////////////
void
VIEW::put_view_light_enable (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity checl
   assert(!(_in_data_file && _data_file == NULL_STR));

   ARRAY<int> e;
   for (int i=0; i<MAX_LIGHTS; i++)
      e.add((light_get_enable(i))?(1):(0));
   d.id();
   *d << e;
   d.end_id();
   
}

void     
VIEW::put_view_light_spot_direction(TAGformat &d) const
{
   if ((!_in_data_file) && (_data_file != NULL_STR))
      return;

   ARRAY<Wvec> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add(light_get_spot_direction(i));
    d.id();
   *d << c;
   d.end_id();
}
 
void     
VIEW::get_view_light_spot_direction(TAGformat &d) 
{
   ARRAY<Wvec> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<c.num(); i++)
      light_set_spot_direction(i,c[i]);
}

void     
VIEW::put_view_light_spot_exponent(TAGformat &d) const
{
   ARRAY<float> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add(light_get_spot_exponent(i));
    d.id();
   *d << c;
   d.end_id();
}
 
void
VIEW::get_view_light_spot_exponent(TAGformat &d) 
{
   ARRAY<float> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<c.num(); i++)
      light_set_spot_exponent(i,c[i]);
}

void     
VIEW::put_view_light_spot_cutoff(TAGformat &d) const
{
   ARRAY<float> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add(light_get_spot_cutoff(i));
    d.id();
   *d << c;
   d.end_id();
}

void     
VIEW::get_view_light_spot_cutoff(TAGformat &d) 
{
   ARRAY<float> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<c.num(); i++)
      light_set_spot_cutoff(i,c[i]);
}

void     
VIEW::put_view_light_constant_attenuation(TAGformat &d) const
{
   ARRAY<float> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add(light_get_constant_attenuation(i));
    d.id();
   *d << c;
   d.end_id();
}

void
VIEW::get_view_light_constant_attenuation(TAGformat &d) 
{
   ARRAY<float> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<c.num(); i++)
      light_set_constant_attenuation(i,c[i]);
}

void     
VIEW::put_view_light_linear_attenuation(TAGformat &d) const
{
   ARRAY<float> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add(light_get_linear_attenuation(i));
    d.id();
   *d << c;
   d.end_id();
}

void     
VIEW::get_view_light_linear_attenuation(TAGformat &d) 
{
   ARRAY<float> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<c.num(); i++)
      light_set_linear_attenuation(i,c[i]);
}

void     
VIEW::put_view_light_quadratic_attenuation(TAGformat &d) const
{
   ARRAY<float> c;
   for (int i=0; i<MAX_LIGHTS; i++)
      c.add(light_get_quadratic_attenuation(i));
    d.id();
   *d << c;
   d.end_id();
}

void     
VIEW::get_view_light_quadratic_attenuation(TAGformat &d) 
{
   ARRAY<float> c;
   *d >> c;
   assert(c.num() <= MAX_LIGHTS);
   for (int i=0; i<c.num(); i++)
      light_set_quadratic_attenuation(i,c[i]);
}

/////////////////////////////////////
// get_view_antialias_enable()
/////////////////////////////////////
void
VIEW::get_view_antialias_enable (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   int e;
   *d >> e;

   // XXX - commenting out cuz it crashes on mac os x:
//   set_antialias_enable(e!=0);
  
}

/////////////////////////////////////
// put_view_antialias_enable()
/////////////////////////////////////
void
VIEW::put_view_antialias_enable (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << get_antialias_enable();
   d.end_id();
}


/////////////////////////////////////
// get_view_antialias_mode()
/////////////////////////////////////
void
VIEW::get_view_antialias_mode (TAGformat &d) 
{
   //Sanity check - might change policy on 2nd part
   assert( ( _in_data_file && (_data_file != NULL_STR)) ||
           (!_in_data_file && (_data_file == NULL_STR))    );

   int m;
   *d >> m;

   // XXX- commenting out cuz it crashes on mac os x:
//   set_antialias_mode(m);
  
}

/////////////////////////////////////
// put_view_antialias_mode()
/////////////////////////////////////
void
VIEW::put_view_antialias_mode (TAGformat &d) const 
{
   if ((!_in_data_file) && (_data_file != NULL_STR)) return;
   //Sanity check
   assert(!(_in_data_file && _data_file == NULL_STR));

   d.id();
   *d << get_antialias_mode();
   d.end_id();
}

#ifdef USE_PTHREAD
VIEWptr
VIEW::peek()
{
   VIEW   *thread_view = ThreadToView::get();
   return thread_view ? VIEWptr(thread_view) : _views.last();
}

const VIEW *
VIEW::peek_ptr()
{
   VIEW   *thread_view = ThreadToView::get();
   return thread_view ? thread_view : &*_views.last();
}
#endif


VIEW::VIEW(
   Cstr_ptr &s,
   WINSYS   *w,
   VIEWimpl *i ) :
   _in_data_file(false),
   _impl(i),
   _view_id(_num_views), 
   _cam(new CAM(str_ptr("camera"))),
   _cam_hist(100),
   _cam_hist_cur(0), // current camera starts at first index to set
   _width(0), 
   _height(0),
   _tris(0),
   _is_clipping(0), 
   _dont_swap(0), 
   _dont_draw(0),
   _messages_sent(0), 
   _has_scissor_region(0),
   _sxmin(-1), 
   _sxmax(1),// left- & right-most edges of scissored view
   _recorder(0), 
   _alpha(1), 
   _use_paper(false), 
   _bkg_file(NULL_STR), 
   _bkg_tex(NULL),
   _data_file(NULL_STR),
   _animator(0),
   _render_mode(NORMAL_MODE),
   _antialias_mode(2),
   _antialias_enable(0),
   _antialias_init(false),
   _line_scale(1.0), 
   _grabbing_screen(0),
   _spf(1.0 / Config::get_var_dbl("JOT_FPS",60)),
   _frame_time(0),
   _win(w)
{
#ifdef USE_PTHREAD
   ThreadToView::make_if_needed();
#endif
   init_lights();
   init_jitter();

   _stereo      = VIEWimpl::NONE;
   _render_type = Config::get_var_str("JOT_RENDER_STYLE",RSMOOTH_SHADE);
   _name        = s;
   _tris        = 0;

   disp_obs();  // observe all changes to DRAWN

   _animator = new Animator(this);
   assert(_animator);

   _recorder = new Recorder(this);
   assert(_recorder);

   _num_views++;

   if (_rend_types.empty()) {
      
      add_rend_type(RSMOOTH_SHADE);
      add_rend_type(RFLAT_SHADE);
      add_rend_type(RHIDDEN_LINE);

      if (!Config::get_var_bool("JOT_MINIMAL_RENDER_STYLES",false)) {
         add_rend_type(RWIRE_FRAME);
         add_rend_type(RNORMALS);
         add_rend_type(RSHOW_TRI_STRIPS);
         add_rend_type(RKEY_LINE);
         add_rend_type(RSIL_FRAME);
      }
   }
   
   VIEWS.add(this);

   _impl->set_view(this);
}

void
VIEW::set_jitter(
   int n,
   int i
   )
{
   if (n==-1)
   {
      _jitter(0,3) = 0;
      _jitter(1,3) = 0;
   }
   else
   {
      assert(_jitters.valid_index(n));
      assert((*(_jitters[n])).valid_index(i));
      XYvec jit((*(_jitters[n]))[i]);

      _jitter(0,3) = -jit[0];
      _jitter(1,3) = -jit[1];
   }
}


void
VIEW::init_jitter()
{
   
   for(int n=0; n<JITTER_NUM; n++)
   {
      ARRAY<VEXEL>* jits = new ARRAY<VEXEL>(jnum[n]);
      _jitters.add(jits);

      for (int i=0; i<jnum[n]; i++)
         jits->add(VEXEL(j[n][i].x,j[n][i].y));
   }

}

void
VIEW::init_lights()
{
   _lights[0] = Light(
      DEFAULT_LIGHT_COORD_0,
      DEFAULT_LIGHT_ENABLE_0,
      DEFAULT_LIGHT_IN_CAM_SPACE_0,
      DEFAULT_LIGHT_AMBIENT_0,
      DEFAULT_LIGHT_DIFFUSE_0,
      DEFAULT_LIGHT_SPECULAR_0
      );

   _lights[1] = Light(
      DEFAULT_LIGHT_COORD_1,
      DEFAULT_LIGHT_ENABLE_1,
      DEFAULT_LIGHT_IN_CAM_SPACE_1,
      DEFAULT_LIGHT_AMBIENT_1,
      DEFAULT_LIGHT_DIFFUSE_1,
      DEFAULT_LIGHT_SPECULAR_1
      );

   _lights[2] = Light(
      DEFAULT_LIGHT_COORD_2,
      DEFAULT_LIGHT_ENABLE_2,
      DEFAULT_LIGHT_IN_CAM_SPACE_2,
      DEFAULT_LIGHT_AMBIENT_2,
      DEFAULT_LIGHT_DIFFUSE_2,
      DEFAULT_LIGHT_SPECULAR_2
      );

   _lights[3] = Light(
      DEFAULT_LIGHT_COORD_3,
      DEFAULT_LIGHT_ENABLE_3,
      DEFAULT_LIGHT_IN_CAM_SPACE_3,
      DEFAULT_LIGHT_AMBIENT_3,
      DEFAULT_LIGHT_DIFFUSE_3,
      DEFAULT_LIGHT_SPECULAR_3
      );

   _light_global_ambient   =  DEFAULT_LIGHT_GLOBAL_AMBIENT;
}

void 
VIEW::set_frame_time(double t) 
{
   if (_frame_time != t) {
      _frame_time = t;
      _frame_time_observers.frame_time_changed();
   }
}

void
VIEW::paint()
{ 
   if (dont_draw())     // don't bother drawing if we haven't 
      return;           // flushed the previous frame yet

   // do nothing if time since last draw < secs per frame:
   if (_spf_timer.elapsed_time() < _spf)
      return;
   _spf_timer.set();

   _stamp++; 

   if (!_antialias_init)
      if (_impl && !_impl->antialias_check())
         _antialias_enable = 0;

   _pix_to_ndc_scale = (_width > _height ? 2.0/_height : 2.0/_width);

   // Animation/Camera animation stuff...

   // Sanity check
   assert(!(_recorder->on() && _animator->on()));

   if (_animator->on()) {
      set_frame_time(_animator->pre_draw_CB());
   } else if (_recorder->on()) { 

      // _recorder->pre_draw_CB() MAY set the frame time for this view
      // but it MAY NOT, depending on some convoluted logic that only
      // it knows.  So pre-emptively set it here first. (Meaning frame
      // time observers may get a notification with a wrong time
      // followed by a notification with a correct time.)
      set_frame_time(stop_watch::sys_time());
      _recorder->pre_draw_CB();
   } else {
      // XXX - should scale by time scale for slow-mo effects:
      set_frame_time(stop_watch::sys_time());
   }

   // It's handy to be able to display only the opaque or transparent
   // objects en mass for compositing runs.  The _render_mode decides
   // what in the active list gets drawn...
   _drawn.clear();
   if (_render_mode == NORMAL_MODE)
   {
      for (int i = 0; i < _active.num(); i++)
         if (!_active[i]->cull(this))   // bcz: handle CAVE culling properly
            _drawn += _active[i];
   }
   else if (_render_mode == OPAQUE_MODE)
   {
      for (int i = 0; i < _active.num(); i++)
         if ((!_active[i]->cull(this)) && (!_active[i]->needs_blend()))
            _drawn += _active[i];
   }
   else if (_render_mode == TRANSPARENT_MODE)
   {
      for (int i = 0; i < _active.num(); i++)
         if ((!_active[i]->cull(this)) && (_active[i]->needs_blend()))
            _drawn += _active[i];
   }
   else
   {
      assert(0);
   }

   // configure camera to render for _screen
   if (_screen) _screen->config_cam(_cam);



   if (!multithread) push(this);

   if (_impl) _tris = _impl->paint();

   if (!multithread) pop();
   
   if (!dont_swap())
      swap();
}

void        
VIEW::swap()
{ 
   //Sanity check
   assert(!(_recorder->on() && _animator->on()));
   
   if ( _animator->on() )
   {
      _animator->post_draw_CB();
   }
   else if ( _recorder->on() ) 
   { 
      _recorder->post_draw_CB();
   }
   
   if (_impl) 
      _impl->swap_buffers(); 
}




RAYhit
VIEW::intersect(
   RAYhit            &r,
   const GELFILTlist &filt
   ) const
{
#ifdef USE_PTHREAD
   assert(ThreadToView::get() == 0);
#endif
   CVIEWptr this_view((VIEW *)this);

   push(this_view);

   GELFILTlist filter_list(filt);

   // Only add default GELFILTpickable filter if no such filter already exists
   bool has_pickable = false;
   for (int f = 0; !has_pickable && f < filter_list.num(); f++)
      has_pickable = 
         filter_list[f]->class_name() == GELFILTpickable::static_name();

   GELFILTpickable pick(0);  // The default filter
   if (!has_pickable)
      filter_list += &pick;

   for (int i = 0; i < _drawn.num(); i++ ) {
      if (filter_list.accept(_drawn[i]))
         _drawn[i]->intersect(r, Identity);
   }

   pop();
   return r;
}


//
// Intersect all objects displayed in this view other than a particular one,
// either interesecting text or not
//
RAYhit
VIEW::intersect_others(
   RAYhit   &r,
   CGELlist &exclude,
   filt      filter
   ) const
{
   GELFILTlist      filter_list;

   GELFILTothers          others(exclude);
   GELFILTpickable        pick  (1);

   // XXX -
   //   should use TEXT2D::static_name() below,
   //   but can't reference geom/text2d.H
   GELFILTclass_desc_excl no_text("TEXT2D");    

   filter_list += &others;

   if ((filter & H_TEXT) == 0)
      filter_list += &no_text; 
   if (filter & H_UNPICKABLE) 
      filter_list += &pick;
      
   return intersect(r, filter_list);
}

//
// Intersect all objects displayed in this view, either interesecting text or
// not
//
RAYhit
VIEW::intersect(
   CXYpt  &xy,
   filt    filter
   ) const
{
   RAYhit ray(xy);
   return intersect(ray,filter);
}

//
// Intersect all objects displayed in this view, either interesecting text or
// not
//
RAYhit
VIEW::intersect(
   RAYhit &r,
   filt    filter
   ) const
{
   GELFILTlist      filter_list;
   GELFILTpickable  pick(1);

   // XXX -
   //   should use TEXT2D::static_name() below,
   //   but can't reference geom/text2d.H
   GELFILTclass_desc_excl no_text("TEXT2D");    

   if ((filter & H_TEXT) == 0)
      filter_list += &no_text;
   if (filter & H_UNPICKABLE)
      filter_list += &pick;

   return intersect(r, filter_list);
}

//
// Intersect all objects displayed in this view, either interesecting text or
// not (set g parameter to be intersected GEOM)
//
RAYhit
VIEW::intersect(
   RAYhit  &r,
   GELptr  &g,
   filt     filter
   ) const
{
   intersect(r, filter);
   g = r.geom();

   return r;
}

//
// Intersect all objects displayed in this view with a particular class name,
// either interesecting text or not
//
RAYhit
VIEW::intersect(
   RAYhit   &r,
   Cstr_ptr &cn,
   filt      filter
   ) const
{
   for (int i = 0; i < _drawn.num(); i++)
      if (_drawn[i]->class_name() == cn && 
          (PICKABLE.get(_drawn[i]) || (filter & H_UNPICKABLE)))
         _drawn[i]->intersect(r, Identity);

   return r;
}


RAYnear
VIEW::nearest(
   RAYnear   &r,
   filt       filter
   ) const
{
   for (int i = 0; i < _drawn.num(); i++)
      if (PICKABLE.get(_drawn[i]) || (filter & H_UNPICKABLE))
         _drawn[i]->nearest(r, Identity);

   return r;
}

RAYnear
VIEW::nearest(
   RAYnear    &r,
   Cstr_ptr   &cn,
   filt        filter
   ) const
{
   for (int i = 0; i < _drawn.num(); i++)
      if (_drawn[i]->class_name() == cn &&
          (PICKABLE.get(_drawn[i]) || (filter & H_UNPICKABLE)))
         _drawn[i]->nearest(r, Identity);

   return r;
}


//
// Returns GEOM's that are within the lasso and can be copied
//
GELlist
VIEW::inside(
   CXYpt_list &lasso
   ) const
{
   GELlist objs;
   VIEWptr  v((VIEW *)this);
   for (int i = 0; i < _drawn.num(); i++)
      if (!NO_COPY.get(_drawn[i]) && _drawn[i]->inside(lasso))
         objs += _drawn[i];

   return objs;
}

void
VIEW::save_cam(CCAMptr c)
{
   /* delete rest of fwd history if we backed up */
   if (_cam_hist.valid_index(_cam_hist_cur ))
      _cam_hist.truncate(_cam_hist_cur );

   CAMptr cptr = new CAM("history");
   *cptr = (c ? *c : *_cam);
   _cam_hist += cptr;
   _cam_hist_cur++;

   err_mesg_cond(Config::get_var_bool("DEBUG_CAM_HISTORY",false,true), 
      ERR_LEV_ERROR, "VIEW::save_cam() - Saved camera. cam_hist = %d.", _cam_hist_cur);
}

void
VIEW::fwd_cam_hist()
{
  if (_cam_hist_cur >= (_cam_hist.num()-1) ){
    /* already at max */
  } else {
    new CamFocus(this, _cam_hist[++_cam_hist_cur]);
  }
}

void
VIEW::bk_cam_hist()
{
  if(_cam_hist_cur<=0) return;

  /* if we're at the end, make sure we've saved the current one */
  if(_cam_hist_cur >= _cam_hist.num()){
    save_cam();
    _cam_hist_cur--;
  }
  new CamFocus(this, _cam_hist[--_cam_hist_cur]);
}

void
VIEW::copy_cam(CCAMptr &c)
{
   assert(_cam != NULL);

   if (c != NULL)
      *_cam = *c;
   else
      err_msg("VIEW::copy_cam: given CAMptr is null");
}

void
VIEW::use_cam(CCAMptr &c)
{
   if (c == NULL) {
      err_msg("VIEW::use_cam: can't use null CAM");
      return;
   }

   _cam = c;
}

void
VIEW::set_size(
   int w, 
   int h, 
   int x, 
   int y
   )
{
   _width  = w;
   _height = h;

   _cam->data()->changed();

   if (_impl)
      _impl->set_size(w,h, x, y);

//XXX - Changing size (say during some AuxRefImage stuff)
//      mucks up some cached values, so we update the 
//      stamp, etc.
//      Some cached stuff may still be dangling...(?)

   _stamp++; 

   _pix_to_ndc_scale = (_width > _height ? 2.0/_height : 2.0/_width);

}


Wtransf   
VIEW::wpt_proj(SCREENptr s, CAMdata::eye e) const 
{
   return _jitter * _lens * _cam->projection_xform(s,e); 
}

Wtransf 
VIEW::xypt_proj() const 
{ 
   return _jitter * _lens; 
}

// returns projection matrix to use for
// drawing normalized device coords to the window
Wtransf     
VIEW::ndc_proj()  const
{
   Wtransf ret;

   if (_width > _height) ret(0,0) = double(_height)/_width;
   else                  ret(1,1) = double(_width)/_height;

   return _jitter * _lens * ret;
}

// returns projection matrix to use for
// drawing pixel coords to the window
Wtransf     
VIEW::pix_proj()  const
{
   Wtransf ret;

   ret(0,0) = 2.0/_width;
   ret(0,3) = -1;
   ret(1,1) = 2.0/_height;
   ret(1,3) = -1;

   return _jitter * _lens * ret;
}

Wtransf     
VIEW::eye_to_pix_proj(SCREENptr s, CAMdata::eye e)  const
{
  Wtransf eye, ndc, pix;

  eye = _cam->projection_xform(s,e);

  if (_width > _height) ndc(0,0) = double(_width)/_height;
  else                  ndc(1,1) = double(_height)/_width; 

  pix(0,0) = _width/2.0;
  pix(0,3) = _width/2.0;
  pix(1,1) = _height/2.0;
  pix(1,3) = _height/2.0;

  Wtransf ret = pix*ndc*eye;
  // TODO: why w=0 after cam xform ?
  return ret;
}

// returns projection matrix to use for
// converting Wpt to its pixel coordinates
Wtransf     
VIEW::wpt_to_pix_proj(SCREENptr s, CAMdata::eye e)  const
{
  Wtransf world, ndc, pix;

  world = _cam->projection_xform(s,e)*_cam->xform(s,e);

  if (_width > _height) ndc(0,0) = double(_width)/_height;
  else                  ndc(1,1) = double(_height)/_width; 

  pix(0,0) = _width/2.0;
  pix(0,3) = _width/2.0;
  pix(1,1) = _height/2.0;
  pix(1,3) = _height/2.0;

  Wtransf ret = pix*ndc*world;
  // TODO: why w=0 after cam xform ?
  return ret;
}

//
// Removes GEOM from the VIEW's drawn list - use WORLD::undisplay instead
//
int
VIEW::undisplay(
   CGELptr &o
   )
{ 
   for (int i=0; i < _active.num(); i++)
      if (_active[i] == o) {
         _active.rem(o);
         _drawn .rem(o);
         return 1;
      }
   return 0;
}

//
// Adds GEOM to the VIEW's drawn list - use WORLD::display instead
//
int
VIEW::display(
   CGELptr &o
   )
{ 
   if (o != GELptr(0)) {
      for (int i=0; i < _active.num(); i++)
         if (_active[i] == o)
            return 0;
      _active.add(o); 
      return 1;
   } 
   return 0;
}

// we're overriding the tick method on the GEL object.  In addition to
// allowing the app to associate some callback with this VIEW, we also let
// other objects register with us, and then we exec() them.
int
VIEW::tick(void)
{
   // for some reason this is sometimes called twice within the same frame:

   if (0) {
      cerr << "VIEW::tick: frame number " << stamp() << endl;

      static uint last_stamp = 0;
      if (last_stamp == stamp()) {
         cerr << "VIEW::tick: repeating frame: " << stamp() << endl;
      }
      last_stamp = stamp();
   }
   for (int i = 0; i < _scheduled.num(); i++)
      if (_scheduled[i]->tick() == -1) {
         _scheduled.rem(_scheduled[i]);
         i--; // cause the last object in the
              // list gets put where the object
              // that was deleted was
      }
   return 1;
}

void
VIEW::screen_grab(
   int       scale_factor, 
   Image    &output
   )
{
   bool alpha = Config::get_var_bool("GRAB_ALPHA",false);


   // sanity check:
   const int n = max(1,scale_factor);

   // make sure output is large output image (n times larger in each dimension)
   assert(int(output.width()) >= n * _width && int(output.height()) >= n * _height);

   // for drawing lines:
   _line_scale = n;

   _grabbing_screen = 1;

   // XXX - make sure silhouettes are up-to-date
   //       should do this right
   // _stamp++;

   // prepare "tile" image (same size as whole window),
	int a = (alpha)?(4):(3);
   Image    tile(_width,_height,a);             // format for RGB

   if (_impl)
      _impl->prepare_buf_read();

   // matrix _lens is the "lens" that selects a particular sub-region (or
   // "tile") of the screen to be rendered at full window-resolution. we simply
   // set _lens to select each tile of the window in turn, render it at full
   // resolution, then copy the data into the appropriate slot of the output
   // image.

   // _lens is initially the Identity matrix.
   _lens(0,0) = n;         // scale x and y by n
   _lens(1,1) = n;         // (translational component added below)
   
   // divide visible window into n^2 tiles (n x n).
   // render each one at the resolution of the window.
   // copy the pixels of each tile into the output image.
   // finally write the output image.
   //
   // i varies horizontally from 0 to n-1
   // j varies vertically from 0 to n-1
   // thus (i,j) designates a particular tile

   for (int j=0; j<n; j++) {
      // set vertical translation for lens:
      _lens(1,3) = n - 2*j - 1;
      for (int i=0; i<n; i++) {
         // set horizontal translation for lens:
         _lens(0,3) = n - 2*i - 1;

         if (_impl) // clear back buffer, set state, draw objects:
            _impl->read_pixels(tile.data(),alpha);

         // copy the pixels into the large output image:
         output.copy_tile(tile,i,j);
      }
   }
   
   // restore lens:
   _lens(0,0) = _lens(1,1) = 1;
   _lens(0,3) = _lens(1,3) = 0;

   // restore line scale:
   _line_scale = 1.0;

   _grabbing_screen = 0;

   if (_impl)
      _impl->end_buf_read();
}


int
VIEW::screen_grab(
   int       scale_factor, 
   Cstr_ptr& filename
   )
{
   bool alpha = Config::get_var_bool("GRAB_ALPHA",false);

   // Grabs current scene as a hi-res image --
   // scaled n times the size of the current window
   // in each dimension.

   // sanity check:
   const int n = max(1,scale_factor);
	int a = (alpha)?(4):(3);
   Image    output(n*_width, n*_height,a);      // format for RGB

   screen_grab(scale_factor, output);

   // write output. return 1 for success, 0 for failure:
   if (!output.write_png(**filename)) {
      // failed.  try writing as pnm (actually a ppm file)
      return output.write_pnm(**filename);
   } else {
      // success writing as png file
      return 1;
   }
}

void
VIEW::viewall()
{
   // Bring the camera back to where it can see everything
   // in the scene. (Useful when the camera gets lost in space):

   BBOX bb;
   for (int i=0; i<_active.num(); i++) {
      if (_active[i]->do_viewall())
         bb += _active[i]->bbox();
   }
   
   CAMdataptr camdata = cam()->data();
   double size = bb.dim().length();
   double dist = size * camdata->focal() / camdata->height();
   Wvec   dirvec = -Wvec::Z();

   Wpt from = bb.center() + dist * dirvec;
   new CamFocus(
      this,
      from,
      bb.center(),
      from + Wvec::Y(),
      bb.center(),
      camdata->width(),
      camdata->height()
      );
}

void
VIEW::set_color(CCOLOR &c) 
{
   if (c != _bkgnd_col) {
      _bkgnd_col = c;
      VIEWobs::notify_viewobs(this, COLOR_ALPHA_CHANGED);
   }
}

void
VIEW::set_alpha(double a)
{
   if (_alpha != a) {
      _alpha = a;
      VIEWobs::notify_viewobs(this, COLOR_ALPHA_CHANGED);
   }
}

void
VIEW::set_render_mode(render_mode_t r)
{
   if (_render_mode != r) {
      _render_mode = r;
      VIEWobs::notify_viewobs(this, UNKNOWN_CHANGED);
   }
}

Wpt
xy_to_w_1(CXYpt& x, CWpt& w) 
{
   return VIEW::peek_cam_const()->xy_to_w(x,w); 
}

Wpt
xy_to_w_2(CXYpt& x, double d) 
{
   return VIEW::peek_cam_const()->xy_to_w(x,d); 
}

Wpt
xy_to_w_3(CXYpt& x) 
{
   return VIEW::peek_cam_const()->xy_to_w(x); 
}

Wvec
xy_to_wvec(CXYpt& x) 
{
   return VIEW::peek_cam_const()->film_dir(x); 
}

XYpt 
w_to_xy(CWpt& w) 
{
   return VIEW::peek_cam_const()->w_to_xy(w); 
}

void 
view_size (int& w, int& h)
{
   VIEW::peek_size(w,h); 
}

double
view_aspect()
{
   return VIEW::peek_cam_const()->aspect(); 
}

void
view_pixels(double& z, NDCpt& p)
{
   z=VIEW::peek_cam_const()->zoom();
   p=VIEW::peek_cam_const()->min(); 
}

CWtransf& 
view_ndc_trans()
{
   return VIEW::peek_cam()->ndc_projection();
}

CWtransf& 
view_ndc_trans_inv()
{
   return VIEW::peek_cam()->ndc_projection_inv();
}

/*****************************************************************
 * Function pointers used in mlib for converting between
 * coordinate types:
 *****************************************************************/
Wpt     (*XYtoW_1       )(CXYpt &, CWpt &)   = xy_to_w_1;
Wpt     (*XYtoW_2       )(CXYpt &, double)   = xy_to_w_2;
Wpt     (*XYtoW_3       )(CXYpt &)           = xy_to_w_3;
Wvec    (*XYtoWvec      )(CXYpt &)           = xy_to_wvec;
XYpt    (*WtoXY         )(CWpt  &)           = w_to_xy;
void    (*VIEW_SIZE     )(int &, int &)      = view_size;
double  (*VIEW_ASPECT   )()                  = view_aspect;
void    (*VIEW_PIXELS   )(double &, NDCpt &) = view_pixels;
CWtransf& (*VIEW_NDC_TRANS)()                = view_ndc_trans;
CWtransf& (*VIEW_NDC_TRANS_INV)()            = view_ndc_trans_inv;

double at_length(CWpt& p, double length)
{   
   CWpt from = VIEW::peek_cam()->data()->from();
   Wvec at_v = VIEW::peek_cam()->data()->at_v();
   Wvec right= VIEW::peek_cam()->data()->right_v();
   
   double dist = from.dist(p);
   CWpt new_p = from + (at_v * dist);
   CWpt new_q = new_p + (right * length);
   return PIXEL(new_p).dist(new_q);
}

/* end of file view.C */
