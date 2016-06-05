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
#include "base_jotapp/base_jotapp.H"
#include "disp/ref_img_client.H"
#include "mesh/lmesh.H"
#include "wnpr/line_pen.H"
#include "wnpr/sil_ui.H"
#include "npr/npr_view.H"
#include "wpath_stroke.H"

#include <fstream>

using namespace mlib;

extern "C" void HACK_mouse_right_button_up();
#define LINE_TEXTURE             "nprdata/stroke_textures/1D--dark-8.png"
#define LINE_TAPER               40.0f
#define LINE_FLARE               0.2f
#define LINE_FADE                3.0f
#define LINE_AFLARE              0.0f

/*****************************************************************
 * Wpath_stroke
 *****************************************************************/

/////////////////////////////////////////////////////////////////
// Sorting comparison functions
/////////////////////////////////////////////////////////////////

static bool
arclen_compare_votes(const LuboVote &a, const LuboVote &b)
{
   return a._s < b._s;
}

static bool
coverage_comp(const CoverageBoundary &a, const CoverageBoundary &b)
{
   if ( a._s == b._s ) {
      if      ( a._type == b._type )
         return false;
      else if ( a._type == COVERAGE_START )
         return true;
      else
         return false;
   }

   return a._s < b._s;
}

/* XXX: The begin() and confidence() functions are not marked const because
 * they return references that are use a *lot*. I don't really feel like going
 * and fixing that right now. So we'll have to const_cast instead. No need to
 * worry, since these functions don't modify VoteGroup as long as you don't mess
 * with the referenced variables.
 */

static bool
votegroups_comp_confidence(const VoteGroup &a, const VoteGroup &b)
{
   return const_cast<VoteGroup&>(a).confidence() > const_cast<VoteGroup&>(b).confidence();
}

static bool
votegroups_comp_begin(const VoteGroup &a, const VoteGroup &b)
{
   return const_cast<VoteGroup&>(a).begin() < const_cast<VoteGroup&>(b).begin();
}

/////////////////////////////////////////////////////////////////
// Wpath_stroke
/////////////////////////////////////////////////////////////////


/////////////////////////////////////
// Constructor
/////////////////////////////////////
WpathStroke::WpathStroke(CLMESHptr& mesh) :
      _inited(false),
      _paths_need_update(true),      
      _strokes_need_update(false)
      
{
      OutlineStroke *s = new OutlineStroke;
      assert(s);

      s->set_propagate_offsets(true);      
      s->set_width(8.0);
      s->set_alpha(1.0);

      s->set_texture(Config::JOT_ROOT() + LINE_TEXTURE);
      s->set_taper(LINE_TAPER);
      s->set_flare(LINE_FLARE);
      s->set_fade(LINE_FADE);
      s->set_aflare(LINE_AFLARE);
           
      _zx_edge_tex = new ZXedgeStrokeTexture((CLMESHptr&)mesh);
      _coher_stroke = new SilStrokePool(s);
      _polyline = new Wpt_list();   
}
/////////////////////////////////////
// Destructor
/////////////////////////////////////

WpathStroke::~WpathStroke()
{   
   //while (_coher_stroke_pools.num() > 0) {
   delete _coher_stroke;
   delete _zx_edge_tex;
   //}
   unobserve();
}

/////////////////////////////////////
// init()
/////////////////////////////////////

void WpathStroke::init()
{
   if(_inited) {
      cerr << "WpathStroke::init() called twice, returning" << endl;
      return;
   } 
   observe();
   _inited = true;
}

/////////////////////////////////////
// observe()
/////////////////////////////////////

void
WpathStroke::observe()
{
   // CAMobs:
   VIEW::peek()->cam()->data()->add_cb(this);
   // BMESHobs:
   subscribe_all_mesh_notifications();  
   // XFORMobs:
   every_xform_obs();
}

/////////////////////////////////////
// unobserve
/////////////////////////////////////

void
WpathStroke::unobserve()
{
   // CAMobs:
   VIEW::peek()->cam()->data()->rem_cb(this);
   // BMESHobs:
   unsubscribe_all_mesh_notifications();
   // XFORMobs:
   unobs_every_xform();
}


/////////////////////////////////////
// draw()
/////////////////////////////////////

int
WpathStroke::draw(CVIEWptr& v)
{
   if (!_inited)
      init();  
   
  

   //Branch off to update the paths
   //if necessary. because of view change,
   // or the always update flag...
   update_paths(v);

   //Create strokes from groups if necessary  
   generate_strokes_from_groups();

   //_zx_edge_tex->draw(v);
   _coher_stroke->draw_flat(v);      

   return 0;
}

int
WpathStroke::draw_id_ref()
{
_zx_edge_tex->set_seethru(1);    
_zx_edge_tex->set_new_branch(1);
return _zx_edge_tex->draw_id_ref();
}

int
WpathStroke::draw_id_ref_pre1()
{
_zx_edge_tex->set_seethru(1);
_zx_edge_tex->set_new_branch(1);
return _zx_edge_tex->draw_id_ref_pre1(); }

int
WpathStroke::draw_id_ref_pre2()
{ return _zx_edge_tex->draw_id_ref_pre2(); }

int
WpathStroke::draw_id_ref_pre3()
{ return _zx_edge_tex->draw_id_ref_pre3(); }

int
WpathStroke::draw_id_ref_pre4()
{ return _zx_edge_tex->draw_id_ref_pre4(); }

/////////////////////////////////////
// get_style()
/////////////////////////////////////
void
WpathStroke::get_style(const string path)
{
   string str;
   str = Config::JOT_ROOT() + path;
   fstream fin;
   fin.open(str.c_str(),ios::in);
   if (!fin){
      err_mesg(ERR_LEV_ERROR, "Wpath_stroke - Could not find the file %s", str.c_str());
   } else {
      STDdstream s(&fin);
      s >> str;
      if (str == "Bla") {         
         _coher_stroke->decode(s);
      }     
   }   
}


double
WpathStroke::get_cur_size() 
{
  /*if (_pix_size_stamp != VIEW::stamp()){
      _pix_size_stamp = VIEW::stamp();
      BBOX bb = (_mesh) ? _mesh->xform()*get_bb() : Identity*get_bb();
      double size = bb.dim().length();
      Wpt pos = bb.center();

      Wvec perp = VIEW::peek_cam()->data()->right_v();
      assert(!perp.is_null());
     _pix_size = VEXEL(pos, size * perp).length();
  }
  return _pix_size;
  */
  double pix_to_ndc_scale = _zx_edge_tex->pix_to_ndc_scale(); 
  if(_zx_edge_tex->_mesh){
      return pix_to_ndc_scale * _zx_edge_tex->_mesh->pix_size();
  } else {
      _pix_size_stamp = VIEW::stamp();
     
      double size = _polyline->length();
     
      Wpt pos = _polyline->average();
      Wvec perp = VIEW::peek_cam()->data()->right_v();
      assert(!perp.is_null());
      _pix_size = VEXEL(pos, size * perp).length();


      return _pix_size;

  }
}
/////////////////////////////////////
// mark_all_dirty()
/////////////////////////////////////

void
WpathStroke::mark_dirty()
{
   _paths_need_update = true;
   _strokes_need_update = true;
}

/////////////////////////////////////
// update_sil_paths()
/////////////////////////////////////

void
WpathStroke::update_paths(CVIEWptr &v)
{
   bool ALWAYS_UPDATE = SilUI::always_update(v);

   if ( ! ( ALWAYS_UPDATE || _paths_need_update) )
      return;

  
   //Create paths with samples
   _zx_edge_tex->create_paths();
   
   //Cache some values into the paths
   //(Such as pix_to_ndc scale, etc.)
   cache_per_path_values();
   
   //Create fitted groups of samples
   generate_groups();

   //Create new samples for next frame
   _zx_edge_tex->regen_group_samples();

   _paths_need_update = false;

   _strokes_need_update = true;
}

/////////////////////////////////////
// cache_per_path_values()
/////////////////////////////////////

void
WpathStroke::cache_per_path_values()
{
   LuboPathList::size_type k, num;

   LuboPathList& paths = _zx_edge_tex->paths();
   num = paths.size();

   vector<double> offset_pix_lens;
   vector<double> stretch_factors;

   double pix_to_ndc_scale = _zx_edge_tex->pix_to_ndc_scale();
  // double cur_size = (_zx_edge_tex->_mesh) ? pix_to_ndc_scale * _zx_edge_tex->_mesh->pix_size()
 //                                          : pix_to_ndc_scale * get_pix_size();

      OutlineStroke *s = _coher_stroke->get_active_prototype();
      assert(s);

      // If no offsets, use the 'angle' as the pixel period of
      // the stylizations (in this case, a texture map.)
      // Always push params around, even if period=0 (by using 1)

      if (s->get_offsets())
         offset_pix_lens.push_back(s->get_offsets()->get_pix_len());
      else
         offset_pix_lens.push_back(max(1.0f,(s->get_angle())));

      // First time round, might need to set this...

      if (s->get_original_mesh_size() == 0.0) {
         assert(_coher_stroke->get_edit_proto() == 0);
         assert(_coher_stroke->get_num_protos() == 1);
         s->set_original_mesh_size(get_cur_size());
         _coher_stroke->set_prototype(s);
      }

      if ((_coher_stroke->get_coher_global())?
          (SilUI::sigma_one(VIEW::peek())):
          (_coher_stroke->get_coher_sigma_one()))
         stretch_factors.push_back(1.0);
      else
         stretch_factors.push_back((get_cur_size() < epsAbsMath()) ? 1.0 : (s->get_original_mesh_size() / get_cur_size()));

   for (k=0; k<num; k++) {
      LuboPath *p = paths[k];

      p->set_pix_to_ndc_scale(pix_to_ndc_scale);

      int pool_index = 0;//type_and_vis_to_sil_stroke_pool(p->type(), p->vis());

      p->line_type() = pool_index;
      p->set_offset_pix_len(offset_pix_lens[pool_index]);
      p->set_stretch(stretch_factors[pool_index]);
   }
}


/////////////////////////////////////
// generate_groups()
/////////////////////////////////////

void
WpathStroke::generate_groups()
{
   static double MIN_PATH_PIX = Config::get_var_dbl("MIN_PATH_PIX",2.0,true);

   LuboPathList::size_type i;

   const int   global_fit_type =   SilUI::fit_type(VIEW::peek());
   const int   global_cover_type = SilUI::cover_type(VIEW::peek());
   const bool  global_do_heal =   (global_cover_type == SIL_COVER_TRIMMED) &&
                                  (SilUI::weight_heal(VIEW::peek()) > 0.0);

   LuboPathList &paths = _zx_edge_tex->paths();

   double min_ndc = (!paths.empty()) ? (paths[0]->pix_to_ndc_scale() * MIN_PATH_PIX) : 0;

   //Build and fit groups
   for (i=0; i < paths.size(); i++) {
      LuboPath* p = paths[i];
      
      if (p->length() < min_ndc )
         continue;

      //cerr << "wpath: " << paths.size() << " and l " << p->length() << endl;
      bool  do_heal;                    
      int   fit_type, cover_type;

      SilStrokePool* pool = _coher_stroke;

      if (pool->get_coher_global()) {
         fit_type = global_fit_type;
         cover_type = global_cover_type;
         do_heal = global_do_heal;
      } else {
         fit_type = pool->get_coher_fit_type();
         cover_type = pool->get_coher_cover_type();
         do_heal = (cover_type == SIL_COVER_TRIMMED) && (pool->get_coher_wh() > 0.0);
      }

      fit_ptr     fit_func = fit_function(fit_type);
      cover_ptr   cover_func = cover_function(cover_type);

      build_groups(p);

      cull_small_groups(p);
      cull_short_groups(p);

      split_looped_groups(p);

      split_gapped_groups(p);
      split_large_delta_groups(p);

      cull_backwards_groups(p);

      if (fit_type == SIL_FIT_INTERPOLATE)
         split_all_backtracking_groups(p);

      cull_small_groups(p);
      cull_short_groups(p);

      cull_sparse_groups(p);

      if (fit_type != SIL_FIT_INTERPOLATE) {
         fit_initial_groups(p, fit_func);

         if (fit_type == SIL_FIT_OPTIMIZE)
            cull_bad_fit_groups(p);

         coverage_manage_groups(p, cover_func);

         cull_outliers_in_groups(p);

         fit_final_groups(p, fit_func);

         if (do_heal)
            heal_groups(p, fit_func);

         refit_backward_fit_groups(p);

      } else {
         coverage_manage_groups(p, cover_func);

         fit_final_groups(p, fit_func);
      }

   }

   // Notify that groups have been built and fitted
   _zx_edge_tex->regen_group_notify();

}

/////////////////////////////////////
// build_groups()
/////////////////////////////////////
void
WpathStroke::build_groups ( LuboPath * p )
{
   size_t i,j,n;
   uint last_id=0, last_ind=0;

   vector<LuboVote>& votes = p->votes();
   vector<VoteGroup>& groups = p->groups();

   std::sort(votes.begin(), votes.end(), arclen_compare_votes);

   n = votes.size();

   groups.clear();
   //cerr << "Num Votes: " << n << "\n";
   for ( i=0 ; i < n ; i++ ) {
      bool added = false;        
      //Consecutive votes often fall in the same group.
      //We accelerate this case explicitly...
      if ( last_id == votes[i]._stroke_id ) {
         groups[last_ind].add(votes[i]);
         added = true;
      } else {
         for (j=groups.size()-1; !added; j--) {
            if ( groups[j].id() == votes[i]._stroke_id ) {
               groups[j].add(votes[i]);
               last_id = groups[j].id();
               last_ind = j;
               added = true;
            }
            if (j==0) break;
         }
      }

      if ( !added ) {
         groups.push_back(VoteGroup(votes[i]._stroke_id, p));
         last_id = votes[i]._stroke_id;
         last_ind = groups.size()-1;
         groups[last_ind].add( votes[i] );
      }
   }

   //Sort (also sets begin/end), and set unique id's
   for (i = 0; i < groups.size(); i++) {
      groups[i].sort();
      groups[i].id() = p->gen_stroke_id();
   }
}

/////////////////////////////////////
// cull_small_groups()
/////////////////////////////////////
void
WpathStroke::cull_small_groups(LuboPath* p)
{
   static int GLOBAL_MIN_VOTES_PER_GROUP = Config::get_var_int("MIN_VOTES_PER_GROUP", 2,true);

   SilStrokePool* pool = _coher_stroke;
   int MIN_VOTES_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_VOTES_PER_GROUP):(pool->get_coher_mv()));

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];
      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;
      if (g.num() < MIN_VOTES_PER_GROUP)
         g.status() = VoteGroup::VOTE_GROUP_LOW_VOTES;
   }
}

/////////////////////////////////////
// cull_short_groups()
/////////////////////////////////////
void
WpathStroke::cull_short_groups(LuboPath* p)
{
   static double GLOBAL_MIN_PIX_PER_GROUP = Config::get_var_dbl("MIN_PIX_PER_GROUP", 5.0,true);
   static double GLOBAL_MIN_FRAC_PER_GROUP = Config::get_var_dbl("MIN_FRAC_PER_GROUP", 0.05,true);

   SilStrokePool* pool = _coher_stroke;
   double MIN_PIX_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_PIX_PER_GROUP):(pool->get_coher_mp()));
   double MIN_FRAC_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_FRAC_PER_GROUP):(pool->get_coher_m5()));

   double min_length = min( MIN_PIX_PER_GROUP * p->pix_to_ndc_scale(),
                            MIN_FRAC_PER_GROUP * p->length());

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];
      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;
      if ((g.end()- g.begin()) < min_length)
         g.status() = VoteGroup::VOTE_GROUP_LOW_LENGTH;
   }
}

/////////////////////////////////////
// cull_sparse_groups()
/////////////////////////////////////
void
WpathStroke::cull_sparse_groups(LuboPath* p)
{
   static double SPARSE_FACTOR = Config::get_var_dbl("SPARSE_FACTOR", 3.0,true);

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();

   double sample_spacing = _zx_edge_tex->get_sampling_dist() * p->pix_to_ndc_scale();

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];
      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (g.num()  < 2) )
         continue;

      if ( ((g.end()- g.begin()) / (g.num()-1)) > SPARSE_FACTOR * sample_spacing)
         g.status() = VoteGroup::VOTE_GROUP_BAD_DENSITY;
   }
}

/////////////////////////////////////
// split_looped_groups()
/////////////////////////////////////
void
WpathStroke::split_looped_groups(LuboPath* p)
{
   if (!p->is_closed())
      return;

   const double GAP_FRACTION = 0.75;
   const double GAP_FACTOR = 3.0;

   const double DELTA_FRACTION = 0.75;
   const double DELTA_FACTOR = 4.0;

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();
   vector<double> gaps, sorted_gaps, deltas, sorted_deltas;

   int j, nv, j0, j1;
   double gap_thresh, delta_thresh, cnt;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 5) )
         continue;

      gaps.clear();
      sorted_gaps.clear();
      deltas.clear();
      sorted_deltas.clear();

      cnt = 0.0;
      for (j=0; j<nv-1; j++) {
         gaps.push_back(g.vote(j+1)._s - g.vote(j)._s);
         double d = (g.vote(j+1)._t - g.vote(j)._t);
         deltas.push_back(d);
         cnt += (d<0.0) ? -1.0 : +1.0;
      }

      // Bail out if < 75% of the
      // deltas have the same sign
      if (fabs(cnt)/(nv-1.0) < 0.5) {
         err_mesg(ERR_LEV_INFO, "WpathStroke::split_looped_groups() - Noisy data...");
         continue;
      }

      sorted_gaps.insert(sorted_gaps.end(), gaps.begin(), gaps.end());
      std::sort(sorted_gaps.begin(), sorted_gaps.end());

      sorted_deltas.insert(sorted_deltas.end(), deltas.begin(), deltas.end());
      if (cnt<0.0)
         std::sort(sorted_deltas.begin(), sorted_deltas.end(), std::greater<double>());
      else
         std::sort(sorted_deltas.begin(), sorted_deltas.end());

      // XXX - Hacky, but it might work:
      // Loop crossing found if there's
      // any reverse delta > delta_thresh
      // AND votes exists within gap_thresh
      // of boths ends of the path.
      // Toss votes between the first and
      // last large reverse delta and join
      // the two groups, shifting one of them.

      gap_thresh   = GAP_FACTOR *   sorted_gaps  [(int)(  GAP_FRACTION * (nv-2.0))];
      delta_thresh = -1.0 * DELTA_FACTOR * sorted_deltas[(int)(DELTA_FRACTION * (nv-2.0))];

      if ( sorted_deltas[0]/delta_thresh > 1.0) {
         //cerr << "WpathStroke::split_looped_groups() - Splitting...\n";

         j = 0;
         j0 = nv;
         j1 = -1;
         while (sorted_deltas[j]/delta_thresh > 1.0) {
            vector<double>::iterator it;
            it = std::find(deltas.begin(), deltas.end(), sorted_deltas[j]);
            int ind = it - deltas.begin();
            if (ind < j0)
               j0 = ind;
            if (ind > j1)
               j1 = ind;
            j++;
            assert(j < (nv-1));
         }

         if ( j0 != j1 ) {
            cerr << "WpathStroke::split_looped_groups() - Hey! I found >1 large delta...\n";
         }

         if (g.votes().front()._s > gap_thresh) {
            cerr << "WpathStroke::split_looped_groups() - No votes near start of path...\n";
            continue;
         }
         if (g.votes().back()._s < (p->length()-gap_thresh)) {
            cerr << "WpathStroke::split_looped_groups() - No votes near end of path...\n";
            continue;
         }

         // If we make it past these checks, then
         // we break the group into [0,j0] [j1+1,num-1]
         // One day we'll try rejoining them
         // into a continguous group...

         groups.push_back(VoteGroup(p->gen_stroke_id(), p));
         groups.push_back(VoteGroup(p->gen_stroke_id(), p));

         VoteGroup& ng1 = groups[groups.size()-2];
         VoteGroup& ng2 = groups[groups.size()-1];

         VoteGroup& g = groups[i];

         g.status() = VoteGroup::VOTE_GROUP_SPLIT_LOOP;

         for (j=0; j<=j0; j++)
            ng1.add(g.vote(j));
         ng1.begin() = g.vote(0)._s;
         ng1.end() = g.vote(j0)._s;

         for (j=j1+1; j<nv; j++)
            ng2.add(g.vote(j));
         ng2.begin() = g.vote(j1+1)._s;
         ng2.end() = g.vote(nv-1)._s;
      }
   }
}

/////////////////////////////////////
// split_large_delta_groups()
/////////////////////////////////////
void
WpathStroke::split_large_delta_groups(LuboPath* p)
{
   const double DELTA_FRACTION = 0.75;
   const double DELTA_FORWARD_FACTOR = 6.0;
   const double DELTA_REVERSE_FACTOR = 3.0;

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();
   vector<double> deltas, sorted_deltas;

   int k, j, j0, nv;
   double cnt, delta_forward_thresh, delta_reverse_thresh;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 5) )
         continue;

      deltas.clear();
      sorted_deltas.clear();

      cnt = 0.0;
      for (j=0; j<nv-1; j++) {
         double d = (g.vote(j+1)._t - g.vote(j)._t);
         deltas.push_back(d);
         cnt += ((d<0.0)?(-1.0):(+1.0));
      }

      // Bail out if < 75% of the
      // deltas have the same sign
      if (fabs(cnt)/(nv-1.0) < 0.5) {
         cerr << "WpathStroke::split_large_delta_groups() - Noisy data...\n";
         continue;
      }

      sorted_deltas.insert(sorted_deltas.end(), deltas.begin(), deltas.end());
      if (cnt<0.0)
         std::sort(sorted_deltas.begin(), sorted_deltas.end(), std::greater<double>());
      else
         std::sort(sorted_deltas.begin(), sorted_deltas.end());

      delta_reverse_thresh = -1.0 * DELTA_REVERSE_FACTOR * sorted_deltas[(int)(DELTA_FRACTION * (nv-2.0))];
      delta_forward_thresh =  1.0 * DELTA_FORWARD_FACTOR * sorted_deltas[(int)(DELTA_FRACTION * (nv-2.0))];

      //if (sorted_deltas[0]/delta_thresh > 1.0)
      //   cerr << "WpathStroke::split_large_delta_groups() - Splitting...\n";

      j0 = 0;
      for (j=0; j<nv-1; j++) {
         if ( (deltas[j]/delta_reverse_thresh > 1.0) || (deltas[j]/delta_forward_thresh > 1.0) ) {
            groups.push_back(VoteGroup(p->gen_stroke_id(), p));
            VoteGroup& ng = groups.back();
            VoteGroup& g  = groups[i];

            for (k=j0; k<=j; k++)
               ng.add(g.vote(k));
            ng.begin() = g.vote(j0)._s;
            ng.end() = g.vote(j)._s;

            j0 = j+1;
         }
      }

      if (j0 != 0) {
         groups.push_back(VoteGroup(p->gen_stroke_id(), p));
         VoteGroup& ng = groups.back();
         VoteGroup& g  = groups[i];
         g.status() = VoteGroup::VOTE_GROUP_SPLIT_LARGE_DELTA;

         for (k=j0; k<=j; k++)
            ng.add(g.vote(k));
         ng.begin() = g.vote(j0)._s;
         ng.end() = g.vote(j)._s;
      }
   }
}

/////////////////////////////////////
// split_gapped_groups()
/////////////////////////////////////
void
WpathStroke::split_gapped_groups(LuboPath* p)
{
   const double THRESH_FRACTION = 0.75;
   const double THRESH_FACTOR = 4.0;

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();
   vector<double> gaps, sorted_gaps;

   int k, j, j0, nv;
   double thresh;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 5) )
         continue;

      gaps.clear();
      sorted_gaps.clear();

      for (j=0; j<nv-1; j++)
         gaps.push_back(g.vote(j+1)._s - g.vote(j)._s);

      sorted_gaps.insert(sorted_gaps.end(), gaps.begin(), gaps.end());
      std::sort(sorted_gaps.begin(), sorted_gaps.end());

      thresh = THRESH_FACTOR * sorted_gaps[(int)(THRESH_FRACTION * (nv-2.0))];

      j0 = 0;
      for (j=0; j<nv-1; j++) {
         if (gaps[j] > thresh) {
            groups.push_back(VoteGroup(p->gen_stroke_id(), p));
            VoteGroup& ng = groups.back();
            VoteGroup& g  = groups[i];

            for (k=j0; k<=j; k++)
               ng.add(g.vote(k));
            ng.begin() = g.vote(j0)._s;
            ng.end() = g.vote(j)._s;

            j0 = j+1;
         }
      }

      if (j0 != 0) {
         groups.push_back(VoteGroup(p->gen_stroke_id(), p));
         VoteGroup& ng = groups.back();
         VoteGroup& g  = groups[i];
         g.status() = VoteGroup::VOTE_GROUP_SPLIT_GAP;

         for (k=j0; k<=j; k++)
            ng.add(g.vote(k));
         ng.begin() = g.vote(j0)._s;
         ng.end() = g.vote(j)._s;
      }
   }
}

/////////////////////////////////////
// cull_backwards_groups()
/////////////////////////////////////
void
WpathStroke::cull_backwards_groups(LuboPath* p)
{
   const double BACK_THRESH = 0.0;

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();

   int j, nv;
   double cnt;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 2) )
         continue;

      cnt = 0.0;
      for (j=0; j<nv-1; j++) {
         cnt += ((g.vote(j+1)._t < g.vote(j)._t)?(-1.0):(+1.0));
      }

      if (cnt/(nv-1.0) <= BACK_THRESH) {
         g.status() = VoteGroup::VOTE_GROUP_CULL_BACKWARDS;
      }
   }
}

/////////////////////////////////////
// split_all_backtracking_groups()
/////////////////////////////////////
void
WpathStroke::split_all_backtracking_groups(LuboPath* p)
{
   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();
   vector<double> deltas;

   int k, j, j0, nv;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 2) )
         continue;

      deltas.clear();

      for (j=0; j<nv-1; j++)
         deltas.push_back(g.vote(j+1)._t - g.vote(j)._t);

      j0 = 0;
      for (j=0; j<nv-1; j++) {
         if (deltas[j] < 0.0) {
            groups.push_back(VoteGroup(p->gen_stroke_id(), p));
            VoteGroup& ng = groups.back();

            VoteGroup& g = groups[i];

            for (k=j0; k<=j; k++)
               ng.add(g.vote(k));
            ng.begin() = g.vote(j0)._s;
            ng.end() = g.vote(j)._s;

            j0 = j+1;
         }
      }

      if (j0 != 0) {
         groups.push_back(VoteGroup(p->gen_stroke_id(), p));
         VoteGroup& ng = groups.back();

         VoteGroup& g = groups[i];

         g.status() = VoteGroup::VOTE_GROUP_SPLIT_ALL_BACKTRACK;

         for (k=j0; k<=j; k++)
            ng.add(g.vote(k));
         ng.begin() = g.vote(j0)._s;
         ng.end() = g.vote(j)._s;
      }
   }
}

/////////////////////////////////////
// fit_intial_groups()
/////////////////////////////////////
void
WpathStroke::fit_initial_groups(
   LuboPath* p,
   void (WpathStroke::*fit_func)(VoteGroup&,double))
{
   // Note: p->stretch() is the inverse of the expected ratio

   double freq = p->stretch() * 1.0 / ( p->pix_to_ndc_scale() * p->offset_pix_len() );

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = p->groups()[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      assert(g.fstatus() == VoteGroup::FIT_NONE);

      if ( g.num() == 0 )
         arclength_fit(g,freq);
      else
         (*this.*fit_func)(g,freq);
   }

}
/////////////////////////////////////
// cull_bad_fit_groups()
/////////////////////////////////////
void
WpathStroke::cull_bad_fit_groups(LuboPath* p)
{
   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = p->groups()[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      assert(g.fstatus() != VoteGroup::FIT_NONE);

      if (g.fstatus() == VoteGroup::FIT_BACKWARDS)
         g.status() = VoteGroup::VOTE_GROUP_FIT_BACKWARDS;
      else
         //XXX - Not thinkning about any other badness... yet...
         assert(g.fstatus() == VoteGroup::FIT_GOOD);
   }

}
/////////////////////////////////////
// coverage_manage_groups()
/////////////////////////////////////
void
WpathStroke::coverage_manage_groups(LuboPath* p, void (WpathStroke::*cover_func)(LuboPath*))
{

   //Do common stuff...

   (*this.*cover_func)(p);

}
/////////////////////////////////////
// cull_outliers_in_groups()
/////////////////////////////////////
void
WpathStroke::cull_outliers_in_groups(LuboPath* p)
{

   const double THRESH_FRACTION = 0.75;
   const double THRESH_FACTOR = 20.0;

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();
   vector<double> errs, sorted_errs;

   int j, cnt, nv;
   double thresh;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];
      nv = g.num();

      if ( (g.status() != VoteGroup::VOTE_GROUP_GOOD ) || (nv < 5) )
         continue;

      //XXX - Presently allow FIT_OLD since changing begin/end
      //invalidates the fit during coverage... however the
      //fits good enough for outlier analysis
      assert(g.fstatus() != VoteGroup::FIT_NONE);

      errs.clear();
      sorted_errs.clear();

      for (j=0; j<nv; j++) {
         double err = (g.vote(j)._t - g.get_t(g.vote(j)._s));
         errs.push_back(fabs(err));
      }

      sorted_errs.insert(sorted_errs.end(), errs.begin(), errs.end());
      std::sort(sorted_errs.begin(), sorted_errs.end());

      thresh = THRESH_FACTOR * sorted_errs[(int)(THRESH_FRACTION * (nv-2.0))];

      cnt = 0;
      for (j=0; j<nv; j++) {
         if (errs[j] > thresh) {
            g.vote(j)._status = LuboVote::VOTE_OUTLIER;
            cnt++;
         }
      }

      if (cnt) {
         g.fstatus() = VoteGroup::FIT_OLD;
      }
   }

}


/////////////////////////////////////
// fit_final_groups()
/////////////////////////////////////
void
WpathStroke::fit_final_groups(
   LuboPath* p,
   void (WpathStroke::*fit_func)(VoteGroup&,double))
{
   // Note: p->stretch() is the inverse of the expected ratio

   double freq = p->stretch() * 1.0 / ( p->pix_to_ndc_scale() * p->offset_pix_len() );

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = p->groups()[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      if (g.fstatus() != VoteGroup::FIT_GOOD) {
         g.fstatus() = VoteGroup::FIT_NONE;
         g.fits().clear();

         if ( g.num() == 0 )
            arclength_fit(g,freq);
         else
            (*this.*fit_func)(g,freq);
      }
   }

}
/////////////////////////////////////
// heal_groups()
/////////////////////////////////////
void
WpathStroke::heal_groups(
   LuboPath* p,
   void (WpathStroke::*fit_func)(VoteGroup&,double))
{

   static double GLOBAL_HEAL_JOIN_PIX_THRESH = Config::get_var_dbl("HEAL_JOIN_PIX_THRESH",3.0,true);
   static double GLOBAL_HEAL_DRAG_PIX_THRESH = Config::get_var_dbl("HEAL_DRAG_PIX_THRESH",15.0,true);

   SilStrokePool* pool = _coher_stroke;
   double HEAL_JOIN_PIX_THRESH = ((pool->get_coher_global())?(GLOBAL_HEAL_JOIN_PIX_THRESH):(pool->get_coher_hj()));
   double HEAL_DRAG_PIX_THRESH = ((pool->get_coher_global())?(GLOBAL_HEAL_DRAG_PIX_THRESH):(pool->get_coher_ht()));

   double pix_to_ndc = p->pix_to_ndc_scale();
   double freq = p->stretch() * 1.0 / ( p->pix_to_ndc_scale() * p->offset_pix_len() );

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, j, n = groups.size();
   vector<int> final_groups;

   size_t i0;
   int pi, pi_1;

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = groups[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      final_groups.push_back(i);
   }

   if (final_groups.size()<2)
      return;

   //We're assuming that the groups provide total coverage
   //of the path, and abutt one another perfectly...
   std::sort(final_groups.begin(), final_groups.end(), votegroups_comp_begin);

   i0 = (size_t)-1;
   pi = 0;

   for (i=0; i < final_groups.size(); i++) {
      VoteGroup &gi   = groups[final_groups[i]];

      bool attach = false;

      if (i<(final_groups.size()-1)) {
         VoteGroup &gi_1 = groups[final_groups[i+1]];
         assert(gi.end() == gi_1.begin());

         double ti   =   gi.get_t(  gi.end()  );
         double ti_1 = gi_1.get_t(gi_1.begin());

         double floori   = floor(ti);
         double floori_1 = floor(ti_1);

         double d = (ti_1 - floori_1) - (ti - floori);

         if      (d >  0.5) { floori_1++; d -= 1.0; } else if (d < -0.5) { floori_1--; d += 1.0; }

         //Pixel difference in phase
         double dpix = fabs((d/freq)/pix_to_ndc);



         if (dpix < HEAL_JOIN_PIX_THRESH) {
            attach = true;
         } else if (dpix < HEAL_DRAG_PIX_THRESH) {
            //Add healing verts
            gi.votes().push_back(LuboVote());
            gi_1.votes().push_back(LuboVote());

            LuboVote  &vi   =   gi.votes().front();
            LuboVote &nvi   =   gi.votes().back();
            LuboVote  &vi_1 = gi_1.votes().front();
            LuboVote &nvi_1 = gi_1.votes().back();

            nvi._path_id   = vi._path_id;
            nvi._stroke_id = vi._stroke_id;
            nvi._s =         gi.end();
            nvi._t =         ti + d/2.0;
            nvi._status =    LuboVote::VOTE_HEALER;

            nvi_1._path_id   = vi_1._path_id;
            nvi_1._stroke_id = vi_1._stroke_id;
            nvi_1._s =         gi_1.begin();
            nvi_1._t =         ti_1 - d/2.0;
            nvi_1._status = LuboVote::VOTE_HEALER;

            //Invalidate fits
            gi.fstatus() = VoteGroup::FIT_OLD;
            std::sort(gi.votes().begin(), gi.votes().end(), arclen_compare_votes);

            gi_1.fstatus() = VoteGroup::FIT_OLD;
            std::sort(gi_1.votes().begin(), gi_1.votes().end(), arclen_compare_votes);

         } else {}

         pi_1 = pi + (int)(floori - floori_1);
      }

      //If we can attach with the next group
      if (attach) {
         //Record this as the starting group if
         //we're not already in a chain
         if (i0 == (size_t)-1) {
            i0 = i;

            //Make a new VoteGroup
            groups.push_back(VoteGroup(p->gen_stroke_id(), p));
            VoteGroup &ng   = groups.back();
            VoteGroup &gi   = groups[final_groups[i]];

            //Add the votes (p0 should be 0)
            assert (pi == 0);
            ng.votes().insert(ng.votes().end(), gi.votes().begin(), gi.votes().end());
            ng.begin() = gi.begin();

         }
         //Otherwise, just add the votes to
         //the current chain
         else {
            //Add votes from gi (pi = ?)
            VoteGroup& ng = groups.back();
            n = gi.num();
            for (j=0; j<n; j++) { ng.add(gi.vote(j)); ng.votes().back()._t += pi; }
         }
         pi = pi_1;
      }
      //If we can't attach with next group...
      else {
         //Then add the votes if necessary
         if (i0 != (size_t)-1) {
            //Add votes from gi (pi = ?)
            VoteGroup& ng = groups.back();
            n = gi.num();
            for (j=0; j<n; j++) { ng.add(gi.vote(j)); ng.votes().back()._t += pi; }
            ng.end() = gi.end();

            //Flag [i0,i] as healed, remove (i0,i]
            while (i > i0) {
               VoteGroup &gi = groups[final_groups[i]];
               gi.status() = VoteGroup::VOTE_GROUP_HEALED;
               final_groups.erase(final_groups.begin() + i);
               i--;
            }
            VoteGroup &gi = groups[final_groups[i]];
            gi.status() = VoteGroup::VOTE_GROUP_HEALED;

            //Replace i0 with new group
            final_groups[i] = groups.size()-1;

            //Resort, since remove() mangles order
            std::sort(final_groups.begin(), final_groups.end(), votegroups_comp_begin);

            //Finally, fit the new group, but sort first!!
            std::sort(ng.votes().begin(), ng.votes().end(), arclen_compare_votes);
            if ( ng.num() == 0 )
               arclength_fit(ng,freq);
            else
               (*this.*fit_func)(ng,freq);

            i0 = (size_t)-1;
            pi = 0;
         }
      }

   }

   //Refit groups that got healer votes
   for (i=0; i < final_groups.size(); i++) {
      VoteGroup &gi   = groups[final_groups[i]];

      if (gi.fstatus() == VoteGroup::FIT_OLD) {
         gi.fits().clear();

         if ( gi.num() == 0 )
            arclength_fit(gi,freq);
         else
            (*this.*fit_func)(gi,freq);
      }
   }
}

/////////////////////////////////////
// refit_backward_fit_groups()
/////////////////////////////////////
void
WpathStroke::refit_backward_fit_groups(
   LuboPath* p)
{
   // Note: p->stretch() is the inverse of the expected ratio

   double freq = p->stretch() * 1.0 / ( p->pix_to_ndc_scale() * p->offset_pix_len() );

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();

   for ( i=0; i<n; i++ ) {
      VoteGroup& g = p->groups()[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      if (g.fstatus() != VoteGroup::FIT_GOOD) {
         assert(g.fstatus() == VoteGroup::FIT_BACKWARDS);

         groups.push_back(VoteGroup(p->gen_stroke_id(), p));
         VoteGroup& ng = groups.back();
         VoteGroup& gi = p->groups()[i];

         ng.votes().insert(ng.votes().end(), gi.votes().begin(), gi.votes().end());
         ng.begin() = gi.begin();
         ng.end() = gi.end();

         gi.status() = VoteGroup::VOTE_GROUP_FINAL_FIT_BACKWARDS;

         if ( ng.num() == 0 )
            arclength_fit(ng,freq);
         else
            phasing_fit(ng,freq);

         assert(ng.fstatus() == VoteGroup::FIT_GOOD);

         cerr << "WpathStroke::refit_backward_fit_groups - <<<BACKWARDS FIT>>>\n";
         //HACK_mouse_right_button_up();
      }
   }
}

/////////////////////////////////////
// random_fit()
/////////////////////////////////////
void
WpathStroke::random_fit(VoteGroup& g, double freq)
{
   double phase = drand48();
   g.fits().push_back(XYpt(g.begin(), phase + 0.0));
   g.fits().push_back(XYpt(g.end(),   phase + (g.end() - g.begin()) * freq));
   g.fstatus() = VoteGroup::FIT_GOOD;
}

/////////////////////////////////////
// sigma_fit()
/////////////////////////////////////
void
WpathStroke::sigma_fit(VoteGroup& g, double freq)
{
   g.fits().push_back(XYpt(g.begin(), 0.0));
   g.fits().push_back(XYpt(g.end(),   (g.end() - g.begin())   * freq));
   g.fstatus() = VoteGroup::FIT_GOOD;
}

/////////////////////////////////////
// arclength_fit()
/////////////////////////////////////
void
WpathStroke::arclength_fit(VoteGroup& g, double freq)
{
   g.fits().push_back(XYpt(g.begin(), g.begin() * freq));
   g.fits().push_back(XYpt(g.end(),   g.end()   * freq));
   g.fstatus() = VoteGroup::FIT_GOOD;
}

/////////////////////////////////////
// phasing_fit()
/////////////////////////////////////
void
WpathStroke::phasing_fit(VoteGroup& g, double freq)
{
   //XXX - Fix this up to respect begin/end better

   int vote_count = 0;
   Wvec phase_vec;
   double phase, phase_i, phase_ave = 0.0;
   double t_begin, t_end, s_begin, s_end;

   for ( int i=0; i < g.num(); i++) {
      if (g.vote(i)._status == LuboVote::VOTE_OUTLIER)
         continue;

      phase_i = g.vote(i)._t - g.vote(i)._s*freq;

      phase_vec[1] += sin ( phase_i * TWO_PI );
      phase_vec[0] += cos ( phase_i * TWO_PI );

      phase_ave += phase_i;

      vote_count++;
   }

   phase_vec /= vote_count;
   phase_ave /= vote_count;

   phase = atan2 ( phase_vec[1] , phase_vec[0] );
   if ( phase < 0 )
      phase += TWO_PI;
   phase /= TWO_PI;

   //XXX - The phase is only unique to an integer,
   //but when plotting the vote vs. the fit,
   //we should try to get the right integer to
   //make the fit look like it actually 'fits'
   //the data...

   phase += floor(phase_ave);

   //Set the fit to cover all the votes
   //or more if begin/end demand it

   s_begin = min(g.begin(),g.first_vote()._s);
   s_end =   max(g.end(),  g.last_vote()._s);

   t_begin = s_begin * freq + phase;
   t_end   = s_end   * freq + phase;

   g.fits().push_back(XYpt(s_begin, t_begin));
   g.fits().push_back(XYpt(s_end  , t_end  ));

   g.fstatus() = VoteGroup::FIT_GOOD;
}

/////////////////////////////////////
// interpolating_fit()
/////////////////////////////////////
void
WpathStroke::interpolating_fit(VoteGroup& g, double freq)
{
   bool bad = false;
   double t_begin, t_end, t_last, t;

   t_begin = g.first_vote()._t  + freq * ( g.begin() - g.first_vote()._s );
   t_end =    g.last_vote()._t  + freq * (   g.end() -  g.last_vote()._s );

   t_last = -DBL_MAX;
   if (g.begin() < g.first_vote()._s) {
      g.fits().push_back(XYpt(g.begin(), t_begin));
   }
   for ( int i =0 ; i < g.num() ; i++ ) {
      assert(g.vote(i)._status == LuboVote::VOTE_GOOD);
      t = g.vote(i)._t;
      g.fits().push_back(XYpt(g.vote(i)._s, t));
      if (t<t_last)
         bad = true;
      t_last = t;
   }
   if (g.end() > g.vote(g.num()-1)._s) {
      g.fits().push_back(XYpt(g.end(), t_end));
   }

   if (bad)
      g.fstatus() = VoteGroup::FIT_BACKWARDS;
   else
      g.fstatus() = VoteGroup::FIT_GOOD;
}

#define REALLY_TINY 1.0e-20;

void
WpathStroke::lubksb(double *a, int n, int *indx, double b[])
{
   double sum;
   int i,ip,j;
   int ii=-1;
   for (i=0;i<n;i++) {
      ip=indx[i];
      sum=b[ip];
      b[ip]=b[i];
      if (ii>-1)
         for (j=ii;j<=i-1;j++)
            sum -= a[n*i+j]*b[j];
      else if (sum)
         ii=i;
      b[i]=sum;
   }
   for (i=n-1;i>=0;i--) {
      sum=b[i];
      for (j=i+1;j<n;j++)
         sum -= a[n*i+j]*b[j];
      b[i]=sum/a[n*i+i];
   }
}


bool
WpathStroke::ludcmp(double *a, int n, int *indx, double *d)
{
   int    i,imax,j,k;
   double big,dum,sum,temp;
   double *vv;

   vv = new double[n];
   assert(vv);

   *d=1.0;
   for (i=0;i<n;i++) {
      big=0.0;
      for (j=0;j<n;j++)
         if ((temp=fabs(a[n*i+j])) > big)
            big=temp;
   if (big == 0.0) { cerr << "ludcmp() - Singular!\n"; return false; }
      vv[i]=1.0/big;
   }
   for (j=0;j<n;j++) {
      for (i=0;i<j;i++) {
         sum=a[n*i+j];
         for (k=0;k<i;k++)
            sum -= a[n*i+k]*a[n*k+j];
         a[n*i+j]=sum;
      }
      big=0.0;
      for (i=j;i<n;i++) {
         sum=a[n*i+j];
         for (k=0;k<j;k++)
            sum -= a[n*i+k]*a[n*k+j];
         a[n*i+j]=sum;
         if ( (dum=vv[i]*fabs(sum)) >= big) {
            big=dum;
            imax=i;
         }
      }
      if (j != imax) {
         for (k=0;k<n;k++) {
            dum=a[n*imax+k];
            a[n*imax+k]=a[n*j+k];
            a[n*j+k]=dum;
         }
         *d = -(*d);
         vv[imax]=vv[j];
      }
      indx[j]=imax;
      if (a[n*j+j] == 0.0)
         a[n*j+j]=REALLY_TINY;
      if (j != n) {
         dum=1.0/(a[n*j+j]);
         for (i=j+1;i<n;i++)
            a[n*i+j] *= dum;
      }
   }
   delete[] vv;

   return true;
}   
/////////////////////////////////////
// optimizing_fit()
/////////////////////////////////////
void
WpathStroke::optimizing_fit(VoteGroup& g, double freq)
{

   double fit_pix_spacing, weight_fit, weight_scale, weight_distort, weight_heal;

   SilStrokePool* pool = _coher_stroke;

   if (pool->get_coher_global()) {
      fit_pix_spacing = SilUI::fit_pix(VIEW::peek());
      weight_fit      = SilUI::weight_fit(VIEW::peek());
      weight_scale    = SilUI::weight_scale(VIEW::peek());
      weight_distort  = SilUI::weight_distort(VIEW::peek());
      weight_heal     = SilUI::weight_heal(VIEW::peek());
   } else {
      fit_pix_spacing = pool->get_coher_pix();
      weight_fit      = pool->get_coher_wf();
      weight_scale    = pool->get_coher_ws();
      weight_distort  = pool->get_coher_wb();
      weight_heal     = pool->get_coher_wh();
   }

   int i0, i, j, k, n;
   double yij,   wij,   tij,   xj,   xjd,   ndc_delta, factor;
   double yij_1, wij_1, tij_1, xj_1, xj_1d, foo;

   double begin = min(g.begin(),g.first_vote()._s);
   double end =   max(g.end(),  g.last_vote()._s);

   ndc_delta = (end - begin);

   n = max(2, (int)ceil( ndc_delta / g.lubo_path()->pix_to_ndc_scale() / (double)fit_pix_spacing ));

   ndc_delta /= (double)(n-1);

   // QQQ - Avoid using matrix class
   //   GXMatrixMNd A,d;
   //   A.SetDim(n,n,0.0);
   //   d.SetDim(n,1,0.0);

   double *A = new double[n*n];
   assert(A);
   double *d = new double[n];
   assert(d);

   for (i=0; i<n*n; i++)
      A[i]=0;
   for (i=0; i<n;   i++)
      d[i]=0;

   i = 0;

   assert(g.num()>0);

   // First, walk up to first vote within [begin,end]
   // (Should always stop immediately now...)
   while ((i < g.num()) && (g.vote(i)._s < begin))
      i++;

   for (j=0; j<n; j++) {
      //Fit - 1st term
      if (j>0) {
         while(i < g.num()) {
            if (g.vote(i)._s <= xj_1d) {
               assert(g.vote(i)._s >= xj_1);

               if (g.vote(i)._status != LuboVote::VOTE_OUTLIER) {
                  yij_1 = g.vote(i)._t - g.vote(i)._s * freq;

                  tij_1 = (g.vote(i)._s - xj_1)/ndc_delta;

                  if (g.vote(i)._status == LuboVote::VOTE_HEALER) {
                     wij_1 = 1.0;
                     factor = weight_heal * 2.0 * wij_1 * tij_1;
                  } else {
                     wij_1 = 1.0/g.num();
                     factor = weight_fit * 2.0 * wij_1 * tij_1;
                  }

                  A[n*j + j-1] += factor * (1.0 - tij_1);
                  A[n*j + j  ] += factor * (      tij_1);
                  d[j]         += factor * (      yij_1);
               }
               i++;
            } else {
               break;
            }
         }
      }

      //Fit - 2nd term

      if (j<(n-1)) {
         i0 = i;

         xj  = begin + j * ndc_delta;
         xjd = xj + ndc_delta;

         while(i < g.num()) {
            if (g.vote(i)._s <= xjd) {
               assert(g.vote(i)._s >= xj);

               if (g.vote(i)._status != LuboVote::VOTE_OUTLIER) {
                  yij = g.vote(i)._t  - g.vote(i)._s * freq;

                  tij = (g.vote(i)._s - xj)/ndc_delta;

                  if (g.vote(i)._status == LuboVote::VOTE_HEALER) {
                     wij = 1.0;
                     factor = weight_heal * 2.0 * wij * (1.0 - tij);
                  } else {
                     wij = 1.0/g.num();
                     factor = weight_fit * 2.0 * wij * (1.0 - tij);
                  }

                  A[n*j + j  ] += factor * (1.0 - tij);
                  A[n*j + j+1] += factor * (      tij);
                  d[j]         += factor * (      yij);
               }
               i++;
            } else {
               break;
            }
         }

         i = i0;
      }

      xj_1  = xj;
      xj_1d = xjd;

      factor = 2.0 * weight_scale / (double)(n*n);

      //Stretch
      for (k=0; k<n; k++) {
         A[n*j + k] -=  factor;
      }
      A[n*j + j] += n * factor;

      factor = 2.0 * weight_distort / (double) n;

      //Distort - 1st term
      if (j>1) {
         A[n*j + j-2] +=        factor;
         A[n*j + j-1] += -2.0 * factor;
         A[n*j + j  ] +=        factor;
      }
      //Distort - 2nd term
      if ((j>0)&&(j<(n-1))) {
         A[n*j + j-1] += -2.0 * factor;
         A[n*j + j  ] +=  4.0 * factor;
         A[n*j + j+1] += -2.0 * factor;
      }
      //Distort - 3rd term
      if (j<(n-2)) {
         A[n*j + j  ] +=        factor;
         A[n*j + j+1] += -2.0 * factor;
         A[n*j + j+2] +=        factor;
      }
   }

   int *IDX = new int[n];
   assert(IDX);

   if (ludcmp(A,n,IDX,&foo)) {
      lubksb(A,n,IDX,d);

      bool bad = false;

      for (j=0; j<n; j++) {
         double fj,fj_1 = 0.0;

         xj = begin + (double)j*ndc_delta;

         fj = d[j] + xj * freq;

         if ((j>0) && (fj < fj_1))
            bad = true;

         g.fits().push_back(XYpt(xj, fj));

         fj_1 = fj;
      }

      if (bad)
         g.fstatus() = VoteGroup::FIT_BACKWARDS;
      else
         g.fstatus() = VoteGroup::FIT_GOOD;
   } else {
      arclength_fit(g,freq);
   }

   delete[] IDX;
   delete[] A;
   delete[] d;
}



/////////////////////////////////////
// majority_cover()
/////////////////////////////////////
void
WpathStroke::majority_cover(LuboPath *p)
{
   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();
   int max_ind=-1, max_votes=0;

   for ( i=0; i<n ; i++ ) {
      VoteGroup& g = groups[i];
      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      g.status() = VoteGroup::VOTE_GROUP_NOT_MAJORITY;

      if (g.num() > max_votes) {
         max_ind = i;
         max_votes = g.num();
      }
   }

   if (max_ind != -1) {
      VoteGroup& g = groups[max_ind];
      g.status() = VoteGroup::VOTE_GROUP_GOOD;

      if ( (0.0 < g.begin()) && (0.0 < g.first_vote()._s)
           && (g.fstatus() != VoteGroup::FIT_NONE))
         g.fstatus() = VoteGroup::FIT_OLD;
      g.begin() = 0.0;

      if ( (p->length() > g.end())  && (p->length() > g.last_vote()._s)
           && (g.fstatus() != VoteGroup::FIT_NONE))
         g.fstatus() = VoteGroup::FIT_OLD;
      g.end() = p->length();
   } else {
      //Deal with no groups
      groups.push_back(VoteGroup(p->gen_stroke_id(), p));
      VoteGroup& ng = groups.back();
      ng.begin() = 0;
      ng.end()   = p->length();
   }
}

/////////////////////////////////////
// one_to_one_cover()
/////////////////////////////////////
void
WpathStroke::one_to_one_cover(LuboPath *p)
{
   static double GLOBAL_MIN_PIX_PER_GROUP = Config::get_var_dbl("MIN_PIX_PER_GROUP", 5.0,true);
   static double GLOBAL_MIN_FRAC_PER_GROUP = Config::get_var_dbl("MIN_FRAC_PER_GROUP", 0.05,true);

   SilStrokePool* pool = _coher_stroke;
   double MIN_PIX_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_PIX_PER_GROUP):(pool->get_coher_mp()));
   double MIN_FRAC_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_FRAC_PER_GROUP):(pool->get_coher_m5()));

   double min_length = min( MIN_PIX_PER_GROUP * p->pix_to_ndc_scale(),
                            MIN_FRAC_PER_GROUP * p->length());

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, n = groups.size();
   double i_1_num, i_num, del, cnt;

   vector<CoverageBoundary> final_boundary;
   final_boundary.reserve(2*n);

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      if ((g.end() - g.begin()) < min_length) {
         g.status() = VoteGroup::VOTE_GROUP_NOT_ONE_TO_ONE;
      } else {
         final_boundary.push_back(CoverageBoundary(i, g.begin(), COVERAGE_START));
         final_boundary.push_back(CoverageBoundary(i,   g.end(),   COVERAGE_END));
      }
   }

   n = final_boundary.size();

   if (n>0) {
      std::sort(final_boundary.begin(), final_boundary.end(), coverage_comp);
      assert(final_boundary[0]._type == COVERAGE_START);
      //Now fall through
   }

   //Now fill the holes in the actual VoteGroups

   cnt = 1;
   for (i=1; i<n; i++) {
      if (cnt == 0) {
         assert(final_boundary[i-1]._type == COVERAGE_END);
         assert(  final_boundary[i]._type == COVERAGE_START);

         del = final_boundary[i]._s - final_boundary[i-1]._s;
         if (del>0.0) {
            VoteGroup &gi_1 = groups[final_boundary[i-1]._vg];
            VoteGroup &gi   = groups[final_boundary[i  ]._vg];

            i_1_num = gi_1.num();
            i_num =   gi.num();

            double s = gi_1.end() + del*((double)i_1_num)/((double)(i_1_num + i_num));

            if ((s < gi.begin()) && (s < gi.first_vote()._s) &&
                (gi.fstatus() != VoteGroup::FIT_NONE) )
               gi.fstatus()   = VoteGroup::FIT_OLD;
            gi.begin() = s;

            if ((gi_1.end() < s) && (gi_1.last_vote()._s < s) &&
                (gi_1.fstatus() != VoteGroup::FIT_NONE) )
               gi_1.fstatus() = VoteGroup::FIT_OLD;
            gi_1.end() = s;

         }
         cnt++;
      } else {
         if (final_boundary[i]._type == COVERAGE_START) {
            cnt++;
         } else {
            assert(final_boundary[i]._type == COVERAGE_END);
            cnt--;
         }

      }
   }

   //We found groups and filled the holes, now just tidy up the ends...
   if (cnt == 0) {
      assert(final_boundary.front()._type == COVERAGE_START);
      assert( final_boundary.back()._type == COVERAGE_END);

      VoteGroup &gf = groups[final_boundary.front()._vg];
      VoteGroup &gl = groups[ final_boundary.back()._vg];

      if ((0.0 < gf.begin()) && (0.0 < gf.first_vote()._s) &&
          (gf.fstatus() != VoteGroup::FIT_NONE) )
         gf.fstatus() = VoteGroup::FIT_OLD;
      gf.begin() = 0.0;

      if ((gl.end() < p->length()) && (gl.last_vote()._s < p->length()) &&
          (gl.fstatus() != VoteGroup::FIT_NONE) )
         gl.fstatus() = VoteGroup::FIT_OLD;
      gl.end() = p->length();

   }
   //Otherwise, just put a fresh-voteless group in...
   else {
      assert(cnt == 1);
      groups.push_back(VoteGroup(p->gen_stroke_id(), p));
      VoteGroup &ng = groups.back();
      ng.begin() = 0;
      ng.end()   = p->length();
   }


}

/////////////////////////////////////
// hybrid_cover()
/////////////////////////////////////
void
WpathStroke::hybrid_cover(LuboPath *p)
{
   static double GLOBAL_MIN_PIX_PER_GROUP = Config::get_var_dbl("MIN_PIX_PER_GROUP", 5.0, true);
   static double GLOBAL_MIN_FRAC_PER_GROUP = Config::get_var_dbl("MIN_FRAC_PER_GROUP", 0.05,true);

   SilStrokePool* pool = _coher_stroke;
   double MIN_PIX_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_PIX_PER_GROUP):(pool->get_coher_mp()));
   double MIN_FRAC_PER_GROUP = ((pool->get_coher_global())?(GLOBAL_MIN_FRAC_PER_GROUP):(pool->get_coher_m5()));

   double min_length = min( MIN_PIX_PER_GROUP * p->pix_to_ndc_scale(),
                            MIN_FRAC_PER_GROUP * p->length());

   vector<VoteGroup>& groups = p->groups();
   vector<VoteGroup>::size_type i, i0, n = groups.size();
   double i0_len, i_len, del;

   vector<CoverageBoundary> boundary;
   vector<CoverageBoundary> final_boundary;
   vector<int> confidence_groups;
   boundary.reserve(2*n);
   final_boundary.reserve(2*n);

   for ( i=0; i < n ; i++ ) {
      VoteGroup& g = groups[i];

      if (g.status() != VoteGroup::VOTE_GROUP_GOOD )
         continue;

      boundary.push_back(CoverageBoundary(i, g.begin(), COVERAGE_START));
      boundary.push_back(CoverageBoundary(i,   g.end(),   COVERAGE_END));

      g.status() = VoteGroup::VOTE_GROUP_NOT_HYBRID;
   }

   n = boundary.size()/2;

   if (n>0) {
      //Assign confidences
      for (i=0; i<n; i++) {
         VoteGroup &g = groups[boundary[2*i]._vg];

         //XXX - Just look at length right now...
         g.confidence() = (g.end() - g.begin())/p->length();
      }

      std::sort(boundary.begin(), boundary.end(), coverage_comp);

      assert(boundary[0]._type == COVERAGE_START);

      final_boundary.push_back(boundary[0]);
      confidence_groups.push_back(boundary[0]._vg);

      n = n * 2;
      for (i=1; i<n; i++) {
         CoverageBoundary &cb = boundary[i];

         //Between covereage regions
         if (final_boundary.back()._type == COVERAGE_END) {
            assert(confidence_groups.empty());
            assert(cb._type == COVERAGE_START);

            final_boundary.push_back(cb);
            confidence_groups.push_back(cb._vg);
         }
         //In the midst of a coverage region
         else {
            //Found the end of a group
            if (cb._type == COVERAGE_END) {
               //If its not the current covering region...
               if (cb._vg != confidence_groups[0]) {
                  //Just drop it from the array of current groups
                  vector<int>::iterator it;
                  it = std::find(confidence_groups.begin(), confidence_groups.end(), cb._vg);
                  confidence_groups.erase(it);
               }
               //Otherwise, complete this group...
               else {
                  final_boundary.push_back(cb);
                  vector<int>::iterator it;
                  it = std::find(confidence_groups.begin(), confidence_groups.end(), cb._vg);
                  confidence_groups.erase(it);

                  //And begin the next group
                  if (confidence_groups.size()>0) {
                     std::sort(confidence_groups.begin(), confidence_groups.end(), votegroups_comp_confidence);
                     final_boundary.push_back(CoverageBoundary(confidence_groups[0], cb._s, COVERAGE_START));
                  }
               }
            }
            //Otherwise, found the start of a group
            else {
               //If it's less confident...
               if (groups[cb._vg].confidence() <= groups[confidence_groups[0]].confidence()) {
                  //Just tuck it into the list of current groups
                  confidence_groups.push_back(cb._vg);
               }
               //If it's more confident
               else {
                  //Complete the current group
                  final_boundary.push_back(CoverageBoundary(confidence_groups[0], cb._s, COVERAGE_END));

                  //Replace the maximum with the new groups
                  confidence_groups[0] = cb._vg;

                  //And begin the next group
                  final_boundary.push_back(cb);
               }
            }
         }
      }

      //Now kill off any coverages that are too narrow
      n = final_boundary.size()/2;
      for (i=0; i<n; i++) {
         if ((final_boundary[2*i+1]._s - final_boundary[2*i]._s) < min_length) {
            final_boundary[2*i]._type = final_boundary[2*i+1]._type = COVERAGE_BAD;
         }
      }

      //Now fall through
   }

   //Now fill the holes...

   //XXX - Maybe fitting should use all the votes, and we just use a window of that,
   //      so window (begin/end) modifications don't invalidate the fits...

   i0 = (size_t)-1;
   n = final_boundary.size()/2;
   for (i=0; i<n; i++) {
      if (final_boundary[2*i]._type != COVERAGE_BAD) {
         //First good coverage should cover start of path
         if (i0 == (size_t)-1) {
            i_len = final_boundary[2*i+1]._s - final_boundary[2*i]._s;
            final_boundary[2*i]._s = 0.0;
         } else {
            i_len = final_boundary[2*i+1]._s - final_boundary[2*i]._s;
            del = final_boundary[2*i]._s - final_boundary[2*i0+1]._s;
            if (del>0.0)
               final_boundary[2*i]._s = (final_boundary[2*i0+1]._s += del*i0_len/(i0_len + i_len));
         }
         i0 = i;
         i0_len = i_len;
      }
   }

   //If any coverage was found, set up the groups...
   if (i0 != (size_t)-1) {
      //First stretch last coverage to end of path
      final_boundary[2*i0+1]._s = p->length();

      //Now setup the corresponding groups...
      for (i=0; i<n; i++) {
         if (final_boundary[2*i]._type != COVERAGE_BAD) {
            VoteGroup &vg = groups[final_boundary[2*i]._vg];

            //If the group's not yet used just adjust it's begin/end
            if (vg.status() == VoteGroup::VOTE_GROUP_NOT_HYBRID) {
               double s;

               s = final_boundary[2*i]._s;
               if ((s < vg.begin()) && (s < vg.first_vote()._s) &&
                   (vg.fstatus() != VoteGroup::FIT_NONE) )
                  vg.fstatus()   = VoteGroup::FIT_OLD;
               vg.begin() = s;

               s = final_boundary[2*i+1]._s;
               if ((vg.end() < s) && (vg.last_vote()._s < s) &&
                   (vg.fstatus() != VoteGroup::FIT_NONE) )
                  vg.fstatus() = VoteGroup::FIT_OLD;
               vg.end() = s;

               vg.status() = VoteGroup::VOTE_GROUP_GOOD;
            }

            //Otherwise, its been segmented, so introduce a new group for this segment
            else {
               groups.push_back(VoteGroup(p->gen_stroke_id(), p));
               VoteGroup& ng = groups.back();

               VoteGroup &vg = groups[final_boundary[2*i]._vg];

               ng.votes().insert(ng.votes().end(), vg.votes().begin(), vg.votes().end());
               ng.begin() = final_boundary[2*i]._s;
               ng.end()   = final_boundary[2*i+1]._s;
            }
         }
      }


   }
   //Otherwise, just put a fresh-voteless group in...
   else {
      groups.push_back(VoteGroup(p->gen_stroke_id(), p));
      VoteGroup &ng = groups.back();
      ng.begin() = 0;
      ng.end()   = p->length();
   }
}

/////////////////////////////////////
// generate_strokes_from_groups()
/////////////////////////////////////

void
WpathStroke::generate_strokes_from_groups()
{
   static double SIL_TO_STROKE_PIX_SAMPLING = Config::get_var_dbl("SIL_TO_STROKE_PIX_SAMPLING",6.0,true);

   size_t n, i, j, k, num;
   double sbegin, send, sdelta, ubegin, uend, udelta, length;
   OutlineStroke* stroke;

   if (!_strokes_need_update)
      return;

   LuboPathList& paths = _zx_edge_tex->paths();

   double step_size = (paths.size()) ? (paths[0]->pix_to_ndc_scale() * SIL_TO_STROKE_PIX_SAMPLING) : 0;
  
   _coher_stroke->blank();
   _coher_stroke->set_path_index_stamp(_zx_edge_tex->path_stamp());
   _coher_stroke->set_group_index_stamp(_zx_edge_tex->group_stamp());

   for (k=0; k<paths.size(); k++) {
      LuboPath *p = paths[k];

      n = p->groups().size();
      length = p->length();

      for ( i = 0 ; i < n ; i++ ) {
         VoteGroup &g = p->groups()[i];

         if (g.status() != VoteGroup::VOTE_GROUP_GOOD)
            continue;

         assert(g.fstatus() == VoteGroup::FIT_GOOD) ;

         stroke = _coher_stroke->get_stroke();
         assert(stroke);

         stroke->set_path_index(k);
         stroke->set_group_index(g.id());

         sbegin = g.begin();
         ubegin = sbegin / length;

         send = g.end();
         uend = send / length;

         sdelta = send - sbegin;
         udelta = uend - ubegin;

         assert(sdelta>=0);

         if (sdelta > 0) {
            num =(int)max(2.0, ceil(sdelta/step_size));

            sdelta /= (double)(num-1);
            udelta /= (double)(num-1);

            if ( stroke->get_offsets())
               stroke->get_offsets()->set_manual_t(true);
            stroke->set_offset_stretch((float)(1.0/p->stretch()));
            stroke->set_gen_t(false);

            stroke->set_offset_lower_t(g.get_t(sbegin));
            stroke->set_min_t         (g.get_t(sbegin));

            stroke->set_offset_upper_t(g.get_t(send));
            stroke->set_max_t         (g.get_t(send));

            NDCZpt beg = p->pt(ubegin);
            NDCZpt end = p->pt(uend);

            if ((num == 2) && (end-beg).planar_length() < gEpsZeroMath ) {
               //cerr << "WpathStroke::generate_strokes_from_groups() - Ah-ha!! Culled a 2 vertex, gEpsZeroMath length group!\n";
               _coher_stroke->remove_stroke(stroke);
            } else {
               stroke->add
               (g.get_t(sbegin), beg);
               for (j=1; j < num-1; j++)
                  stroke->add
                  ( g.get_t(sbegin + j * sdelta), p->pt(ubegin + j * udelta) );
               stroke->add
               (g.get_t(send), end);
            }
         } else {
            cerr << "WpathStroke::generate_strokes_from_groups() - ERROR!! Culled a ZERO length group!\n";
            _coher_stroke->remove_stroke(stroke);
         }

      }
   }
   
   bool selection_changed = false;
  
   selection_changed = _coher_stroke->update_selection(paths);
  
   if (selection_changed) {
      LinePen *line_pen = nullptr;
      if (BaseJOTapp::instance() &&
          (line_pen = dynamic_cast<LinePen*>(BaseJOTapp::instance()->cur_pen()))){
         line_pen->selection_changed(LinePen::LINE_PEN_SELECTION_CHANGED__SIL_TRACKING);
      }
   }
   _strokes_need_update = false;
}

