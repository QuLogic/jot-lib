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
#ifndef SIL_AND_CREASE_TEXTURE_HEADER
#define SIL_AND_CREASE_TEXTURE_HEADER

#include "gtex/basic_texture.H"
#include "gtex/ref_image.H"

#include "geom/text2d.H"

#include "stroke/sil_stroke_pool.H"
#include "stroke/edge_stroke_pool.H"

#include "std/config.H"

#include "npr/zxedge_stroke_texture.H"


/*****************************************************************
 * SilAndCreaseTexture
 *****************************************************************/
#define CSilAndCreaseTexture const SilAndCreaseTexture

class SilAndCreaseTexture :
         public OGLTexture,
         protected CAMobs,
         protected BMESHobs,
         protected XFORMobs
{
   /******** STATIC MEMBER VARIABLES ********/
private:
   static TAGlist          *_sct_tags;
public:
   /******** PUBLIC MEMBERS TYPES ********/
   enum sil_fit_t {
      SIL_FIT_RANDOM = 0,
      SIL_FIT_SIGMA,
      SIL_FIT_PHASE,
      SIL_FIT_INTERPOLATE,
      SIL_FIT_OPTIMIZE
   };

   enum sil_cover_t {
      SIL_COVER_MAJORITY = 0,
      SIL_COVER_ONE_TO_ONE,
      SIL_COVER_TRIMMED
   };

   enum sil_stroke_pool_t {
      SIL_VIS = 0,
      SIL_HIDDEN,
      SIL_OCCLUDED,
      SILBACK_VIS,
      SILBACK_HIDDEN,
      SILBACK_OCCLUDED,
      CREASE_VIS,
      CREASE_HIDDEN,
      CREASE_OCCLUDED,
      BORDER_VIS,
      BORDER_HIDDEN,
      BORDER_OCCLUDED,
      SUGLINE_VIS,
      SUGLINE_HIDDEN,
      SUGLINE_OCCLUDED,
      SIL_STROKE_POOL_NUM
   };

   /******** STATIC MEMBER METHODS ********/
   static const char * sil_stroke_pool(sil_stroke_pool_t t)
   {
      switch(t) {
      case SIL_VIS:
         return "SIL_VIS";
         break;
      case SIL_HIDDEN:
         return "SIL_HID";
         break;
      case SIL_OCCLUDED:
         return "SIL_OCC";
         break;
      case SILBACK_VIS:
         return "SILB_VIS";
         break;
      case SILBACK_HIDDEN:
         return "SILB_HID";
         break;
      case SILBACK_OCCLUDED:
         return "SILB_OCC";
         break;
      case CREASE_VIS:
         return "CRE_VIS";
         break;
      case CREASE_HIDDEN:
         return "CRE_HID";
         break;
      case CREASE_OCCLUDED:
         return "CRE_OCC";
         break;
      case BORDER_VIS:
         return "BOR_VIS";
         break;
      case BORDER_HIDDEN:
         return "BOR_HID";
         break;
      case BORDER_OCCLUDED:
         return "BOR_OCC";
         break;
      case SUGLINE_VIS:
         return "SUG_VIS";
         break;
      case SUGLINE_HIDDEN:
         return "SUG_HID";
         break;
      case SUGLINE_OCCLUDED:
         return "SUG_OCC";
         break;
      default:
         return "";
      }
   }

   static sil_stroke_pool_t type_and_vis_to_sil_stroke_pool(int t, int v)
   {
      int i;
      switch(t) {
      case STYPE_SIL:
         i = SIL_VIS;
         break;
      case STYPE_BF_SIL:
         i = SILBACK_VIS;
         break;
      case STYPE_CREASE:
         i = CREASE_VIS;
         break;
      case STYPE_BORDER:
         i = BORDER_VIS;
         break;
      case STYPE_SUGLINE:
         i = SUGLINE_VIS;
         break;
      default:
         assert(0);
         break;
      }

      switch(v) {
      case SIL_VISIBLE:
         i += 0;
         break;
      case SIL_HIDDEN:
         i += 1;
         break;
      case SIL_OCCLUDED:
         i += 2;
         break;
      default:
         assert(0);
         break;
      }
      return (sil_stroke_pool_t)i;
   }

protected:
   /******** MEMBER VARIABLES ********/

   ARRAY<SilStrokePool*>    _sil_stroke_pools;

   ARRAY<EdgeStrokePool*>   _crease_stroke_pools;

   bool                     _inited;

   bool                     _sil_paths_need_update;
   bool                     _sil_strokes_need_update;
   bool                     _crease_strokes_need_update;

   double                   _crease_max_bend_angle;
   double                   _crease_vis_step_size;
   double                   _crease_thresh;
   bool                     _crease_hide;

   bool                     _noise_motion;
   float                    _noise_frequency;
   float                    _noise_order;
   float                    _noise_duration;

   double                   _noise_time_stamp;

   ZXedgeStrokeTexture      _zx_edge_tex;    // for building sil paths

public :
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/

   SilAndCreaseTexture(Patch* patch = 0);

   virtual ~SilAndCreaseTexture();

   /******** MEMBER METHODS ********/

   BStrokePool* get_sil_stroke_pool(sil_stroke_pool_t i)
   {
      assert(i< SIL_STROKE_POOL_NUM);
      return _sil_stroke_pools[(int)i];
   }

   ARRAY<EdgeStrokePool*>* get_crease_stroke_pools()    { return &_crease_stroke_pools; }

   ZXedgeStrokeTexture*    zx_edge_tex()                { return &_zx_edge_tex;  }


   void           recreate_creases();

   void           set_crease_vis_step_size(double s);
   double         get_crease_vis_step_size()    { return _crease_vis_step_size;  }

   void           set_crease_max_bend_angle(double a) { _crease_max_bend_angle = a; }
   double         get_crease_max_bend_angle() const   { return _crease_max_bend_angle; }

   void           set_crease_thresh(double a)   { _crease_thresh = a;      }
   double         get_crease_thresh() const     { return _crease_thresh;   }

   void           set_hide_crease(bool h)       { _crease_hide = h;    }
   bool           get_hide_crease() const       { return _crease_hide; }

   void           set_noise_motion(bool m)      { _noise_motion = m;    }
   bool           get_noise_motion() const      { return _noise_motion; }

   void           set_noise_frequency(float f)  { _noise_frequency = f;    }
   float          get_noise_frequency() const   { return _noise_frequency; }

   void           set_noise_order(float o)      { _noise_order = o;    }
   float          get_noise_order() const       { return _noise_order; }

   void           set_noise_duration(float d)   { _noise_duration = d;    }
   float          get_noise_duration() const    { return _noise_duration;  }

   virtual void   init();
   void           mark_all_dirty();
   void           mark_sils_dirty();
   void           mark_creases_dirty();

   virtual void set_seethru(int s)     { _zx_edge_tex.set_seethru(s); }
   virtual void set_new_branch(int b)    { _zx_edge_tex.set_new_branch(b); }

   virtual void set_zxsil_render_flag(int type, int vis, bool val ) {  _zx_edge_tex.set_render_flag(type, vis, val);  }
   virtual void set_zxsil_render_flag(int i, bool val )     { _zx_edge_tex.set_render_flag(i,val);     }

   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS2("SilAndCreaseTexture", OGLTexture, CDATA_ITEM *);
   virtual DATA_ITEM*   dup()  const         { return new SilAndCreaseTexture; }
   virtual CTAGlist&    tags() const;

   /******** I/O functions ********/
   double&        crease_max_bend_angle_()   { return _crease_max_bend_angle; }
   double&        crease_thresh_()           { return _crease_thresh;         }
   float&         noise_frequency_()         { return _noise_frequency;       }
   float&         noise_order_()             { return _noise_order;           }
   float&         noise_duration_()          { return _noise_duration;        }
   bool&          noise_motion_()            { return _noise_motion;          }

   void           get_sil_pool (TAGformat &d);
   void           put_sil_pools (TAGformat &d) const;

   void           get_crease_pool (TAGformat &d);
   void           put_crease_pools (TAGformat &d) const;

   //******** OBSERVER METHODS ********
   void           observe  ();
   void           unobserve();
   // CAMobs:
   virtual void   notify(CCAMdataptr&)                   { mark_all_dirty(); }
   // BMESHobs
   virtual void   notify_xform (BMESH*, mlib::CWtransf&, CMOD&){ mark_all_dirty(); }
   virtual void   notify_change(BMESH*, BMESH::change_t) { mark_all_dirty(); }
   // XFORMobs
   virtual void   notify_xform(CGEOMptr&g, STATE)
   {
      if (!TEXT2D::isa(g))
         mark_all_dirty();
   }

   //******** GTexture VIRTUAL METHODS ********

   virtual int       draw(CVIEWptr& v);
   virtual void      set_patch(Patch* p);
   virtual void request_ref_imgs();
   virtual int   draw_id_ref()      { return _zx_edge_tex.draw_id_ref();  }
   virtual int   draw_id_ref_pre1() { return _zx_edge_tex.draw_id_ref_pre1(); }
   virtual int   draw_id_ref_pre2() { return _zx_edge_tex.draw_id_ref_pre2(); }
   virtual int   draw_id_ref_pre3() { return _zx_edge_tex.draw_id_ref_pre3(); }
   virtual int   draw_id_ref_pre4() { return _zx_edge_tex.draw_id_ref_pre4(); }

   /******** DATA_ITEM METHODS ********/

   virtual int       write_stream(ostream &os) const;
   virtual int       read_stream (istream &, str_list &leftover);

protected:
   /******** INTERNAL METHODS ********/
   virtual void   update_sil_paths(CVIEWptr& v);

   void           create_crease_strokes();
   double         mesh_screen_size()
   {
      assert(_patch && _patch->mesh());
      return _patch->mesh()->pix_size();
   }

   // XXX - The new world order...
public:
   void           generate_sil_groups();
protected:
   void           cache_per_path_values();

   void           build_groups(LuboPath* p);

   void           cull_small_groups(LuboPath* p);
   void           cull_short_groups(LuboPath* p);
   void           cull_sparse_groups(LuboPath* p);
   void           split_looped_groups(LuboPath* p);
   void           split_large_delta_groups(LuboPath* p);
   void           split_gapped_groups(LuboPath* p);
   void           cull_backwards_groups(LuboPath* p);
   void           split_all_backtracking_groups(LuboPath* p);
   void           fit_initial_groups(LuboPath* p, void (SilAndCreaseTexture::*fit_func)(VoteGroup&,double));
   void           cull_bad_fit_groups(LuboPath* p);
   void           coverage_manage_groups(LuboPath* p, void (SilAndCreaseTexture::*cover_func)(LuboPath*));
   void           cull_outliers_in_groups(LuboPath* p);
   void           fit_final_groups(LuboPath* p,  void (SilAndCreaseTexture::*fit_func)(VoteGroup&,double));
   void           heal_groups(LuboPath* p,  void (SilAndCreaseTexture::*fit_func)(VoteGroup&,double));
   void           refit_backward_fit_groups(LuboPath* p);

   void           arclength_fit(VoteGroup& g, double stretch);
   void           random_fit(VoteGroup& g, double stretch); //phase=rand
   void           sigma_fit(VoteGroup& g, double stretch); //phase=0
   void           phasing_fit(VoteGroup& g, double stretch);
   void           interpolating_fit(VoteGroup& g, double stretch);
   void           optimizing_fit(VoteGroup& g, double stretch);

   void           majority_cover(LuboPath *p);
   void           one_to_one_cover(LuboPath *p);
   void           hybrid_cover(LuboPath *p);

   void           generate_strokes_from_groups();

   void           update_squiggle_vision();

   typedef void (SilAndCreaseTexture::*fit_ptr)(class VoteGroup &,double);

   fit_ptr        fit_function(int fit_type)
   {
      switch(fit_type) {
      case SIL_FIT_RANDOM:
         return &SilAndCreaseTexture::random_fit;
         break;
      case SIL_FIT_SIGMA:
         return &SilAndCreaseTexture::sigma_fit;
         break;
      case SIL_FIT_PHASE:
         return &SilAndCreaseTexture::phasing_fit;
         break;
      case SIL_FIT_INTERPOLATE:
         return &SilAndCreaseTexture::interpolating_fit;
         break;
      case SIL_FIT_OPTIMIZE:
         return &SilAndCreaseTexture::optimizing_fit;
         break;
      default:
         assert(0);
         return NULL;
         break;
      }
   }

   typedef void (SilAndCreaseTexture::*cover_ptr)(class LuboPath *);

   cover_ptr      cover_function(int cover_type)
   {
      switch(cover_type) {
      case SIL_COVER_MAJORITY:
         return &SilAndCreaseTexture::majority_cover;
         break;
      case SIL_COVER_ONE_TO_ONE:
         return &SilAndCreaseTexture::one_to_one_cover;
         break;
      case SIL_COVER_TRIMMED:
         return &SilAndCreaseTexture::hybrid_cover;
         break;
      default:
         assert(0);
         return NULL;
         break;
      }
   }
};

enum coverage_t {
   COVERAGE_START,
   COVERAGE_END,
   COVERAGE_BAD
};

class CoverageBoundary
{
public:

   int            _vg;
   double         _s;
   int            _type;

   CoverageBoundary (int v , double s, int t ) :  _vg(v),   _s(s),  _type(t) {}
   CoverageBoundary () :                         _vg(-1),   _s(0),  _type(COVERAGE_START)  {}

   bool operator == ( CoverageBoundary m ) { return (_vg == m._vg) && (_s == m._s) && (_type == m._type); }
} ;

#endif
