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
#ifndef WPATH_STROKE_HEADER
#define WPATH_STROKE_HEADER

#include "gtex/ref_image.H"
#include "geom/text2d.H"   
#include "npr/zxedge_stroke_texture.H"
#include "std/config.H" 
#include "stroke/sil_stroke_pool.H"
#include "stroke/base_stroke.H"

/*****************************************************************
 * Wpath_stroke -
 * class that contains a ZXedgeStrokeTexture and SilStrokePool.
 * ZXedge stroke is used to create paths and SilStroke uses
 * those paths to put down strokes.
 *
 *
 * Here is what matters, all together for conviniance:
 *   Wpath_stroke:
 *      draw_id_ref_pre1
 *        ---process all paths to be in frustum, get partial lengths
 *        ---before we go to draw the ID ref image
 *        ---this allows us to do a more accurate lookup for our
 *        ZXedgeStrokeTexture::sil_path_preprocess_seethru();
 *
 *      draw_id_ref_pre2
 *       ---INVISIBLE PASS (pass2)
 *       ---Using a color that we've marked as invisible ( using the second bit of the alpha channel )
 *       ---we're going to draw all silhouettes into the idref image
 *       ---without checking or writing into the depth buffer
 *       ZXedgeStrokeTexture::draw_id_ref_param_invis_pass
 *
 *      draw_id_ref_pre3
 *       ---not sure
 *
 *      draw_id_ref_pre4
 *        ZXedgeStrokeTexture::draw_id_ref_param_vis_pass
 *
 *   Wpath_stroke::draw
 *      Wpath_stroke::update_paths
 *          ---Create paths with samples
 *          ZXedgeStrokeTexture::create_paths
 *
 *              ---the processing we did before drawing the IDref
 *              ---resample as needed, perform visibility checks
 *              ZXedgeStrokeTexture::resample_ndcz_seethru();
 *
 *              ---Build Parameterized Paths ( LuboPaths ) from the big ndc array
 *              ZXedgeStrokeTexture::add_paths_seethru();
 *
 *              ---Compute new parameterizations from the old:
 *              ZXedgeStrokeTexture::propagate_sil_parameterization_seethru();
 *
 *          ---Create fitted groups of samples
 *          Wpath_stroke::generate_groups
 *
 *          ---Create new samples for next frame
 *          ZXedgeStrokeTexture::regen_group_samples
 *
 *      ---Create strokes from groups if necessary
 *      Wpath_stroke::generate_strokes_from_groups
 *
 *
 *****************************************************************/
#define CWpathStroke const WpathStroke

class WpathStroke :
         public RefImageClient,
         protected CAMobs,
         protected BMESHobs,
         protected XFORMobs
{
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

protected:
   /******** MEMBER VARIABLES ********/   
   SilStrokePool*           _coher_stroke;         // for coherence 
   ZXedgeStrokeTexture*     _zx_edge_tex;          // for building paths     
   BaseStroke               _prototype;            // prototype stroke for bcurves to have
                                                   // individual styles
   bool                     _inited;

   bool                     _paths_need_update;              
   bool                     _strokes_need_update;
   double                   _pix_size;      // approx pix size
   uint                     _pix_size_stamp;// for caching per-frame
   mlib::CWpt_list*               _polyline;

public :
   /******** CONSTRUCTOR/DECONSTRUCTOR *******/  
   WpathStroke(CLMESHptr& mesh=0);
   virtual ~WpathStroke();

   /******** MEMBER METHODS ********/
   ZXedgeStrokeTexture*    zx_edge_tex()                { return _zx_edge_tex;  }
   virtual void   init();   
  
   /******** DATA_ITEM METHODS ********/
   DEFINE_RTTI_METHODS_BASE("WpathStroke", CWpathStroke*);
   //virtual DATA_ITEM*  dup()  const   { return new WpathStroke; }
  
   //******** OBSERVER METHODS ********
   void           observe  ();
   void           unobserve();
   // CAMobs:
   virtual void   notify(CCAMdataptr&)                   { mark_dirty(); }
  
   // XFORMobs
   virtual void   notify_xform(CGEOMptr&g, STATE){
      if (!TEXT2D::isa(g))
         mark_dirty();
   }


   void set_stroke_3d(CEdgeStrip* stroke_3d){
      mark_dirty();
      _zx_edge_tex->set_stroke_3d(stroke_3d);
   }
   void set_polyline(mlib::CWpt_list& polyline){
      _polyline = &polyline;
      _zx_edge_tex->set_polyline(polyline);
   }
   //******** RefImageClient VIRTUAL METHODS ********

   virtual int   draw(CVIEWptr& v);  
   virtual int   draw_id_ref();     
   virtual int   draw_id_ref_pre1(); 
   virtual int   draw_id_ref_pre2();
   virtual int   draw_id_ref_pre3();
   virtual int   draw_id_ref_pre4(); 

   //******** RefImageClient METHODS ********
   virtual void request_ref_imgs() {
      //if (_sil_paths_need_update)
      IDRefImage::schedule_update();
   }
   CBaseStroke& get_prototype()  const { return _zx_edge_tex->get_prototype(); }
   void set_prototype(CBaseStroke &s){ _zx_edge_tex->set_prototype(s); }
   void get_style(str_ptr path);
   double get_cur_size();
  
protected:
   /******** INTERNAL METHODS ********/
   void           mark_dirty();
   virtual void   update_paths(CVIEWptr& v);   
public:
   void           generate_groups();
protected:   
   void           lubksb(double *a, int n, int *indx, double b[]);
   bool           ludcmp(double *a, int n, int *indx, double *d);

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
   void           fit_initial_groups(LuboPath* p, void (WpathStroke::*fit_func)(VoteGroup&,double));
   void           cull_bad_fit_groups(LuboPath* p);
   void           coverage_manage_groups(LuboPath* p, void (WpathStroke::*cover_func)(LuboPath*));
   void           cull_outliers_in_groups(LuboPath* p);
   void           fit_final_groups(LuboPath* p,  void (WpathStroke::*fit_func)(VoteGroup&,double));
   void           heal_groups(LuboPath* p,  void (WpathStroke::*fit_func)(VoteGroup&,double));
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

   typedef void (WpathStroke::*fit_ptr)(class VoteGroup &,double);

   fit_ptr        fit_function(int fit_type)
   {
      switch(fit_type) {
      case SIL_FIT_RANDOM:
         return &WpathStroke::random_fit;
         break;
      case SIL_FIT_SIGMA:
         return &WpathStroke::sigma_fit;
         break;
      case SIL_FIT_PHASE:
         return &WpathStroke::phasing_fit;
         break;
      case SIL_FIT_INTERPOLATE:
         return &WpathStroke::interpolating_fit;
         break;
      case SIL_FIT_OPTIMIZE:
         return &WpathStroke::optimizing_fit;
         break;
      default:
         assert(0);
         return NULL;
         break;
      }
   }

   typedef void (WpathStroke::*cover_ptr)(class LuboPath *);

   cover_ptr      cover_function(int cover_type)
   {
      switch(cover_type) {
      case SIL_COVER_MAJORITY:
         return &WpathStroke::majority_cover;
         break;
      case SIL_COVER_ONE_TO_ONE:
         return &WpathStroke::one_to_one_cover;
         break;
      case SIL_COVER_TRIMMED:
         return &WpathStroke::hybrid_cover;
         break;
      default:
         assert(0);
         return NULL;
         break;
      }
   }
};

#endif
