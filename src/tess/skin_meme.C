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
#include "tess/skin.H"
#include "tess/skin_meme.H"

static bool debug = Config::get_var_bool("DEBUG_SKIN",false);

static SimplexFilter default_filter;

/************************************************************
 * SkinMeme
 ************************************************************/
SkinMeme::SkinMeme(Skin* skin, Lvert* v, Bsimplex* t) :
   VertMeme(skin, v),
   _bc(1,0,0),
   _h(0),
   _is_sticky(false),
   _non_penetrate(true),
   _stay_out(true),
   _is_frozen(false),
   _track_filter(&default_filter)
{
   _trackers.set_unique();
   add_track_simplex(t);
}

void 
SkinMeme::set(Bsimplex* t, CWvec& bc) 
{
   set_track_simplex(t);
   set_bc(bc);
}

void
SkinMeme::set_offset(double h)
{
   _h = h;
   assert(bbase());
   bbase()->invalidate();
}

void 
SkinMeme::set_track_simplex(Bsimplex* s)  
{
   assert(_track_filter);
   if (!s) {
      _trackers.clear();
   } else if (!_track_filter->accept(s)) {
      err_adv(debug, "SkinMeme::set_track_simplex: error: filter rejects simplex");
   } else if (is_tracking()) {
      _trackers[0] = s;
   } else {
      add_track_simplex(s);
   }
}

void 
SkinMeme::add_track_simplex(Bsimplex* s) 
{
   assert(_track_filter);
   if (s && _track_filter->accept(s)) {
      _trackers.add_uniquely(s);
   }
}

class InflateBaseFilter : public SimplexFilter {
   public:
   //******** SOLE JOB ********
   virtual bool accept(CBsimplex* s) const {
      return Bbase::find_controller(s) != 0;
   }
};

inline Wvec
bc2norm(Bsimplex* s, CWvec& bc)
{
   Wvec ret;
   if (is_face(s)) {
      ((Bface*)s)->bc2norm_blend(bc, ret);
   } else if (is_edge(s)) {
      ret = ((Bedge*)s)->norm();
   } else if (is_vert(s)) {
      ret = ((Bvert*)s)->norm();
   }
   return ret;
}

inline Wvec
penetration_correction(CWpt& p, Bsimplex* s, CWvec& bc, bool stay_out)
{
   // used for non-sticky memes, that compute their position thru
   // smooth subdivision. returns a correction they can use to get
   // outside of the surface, in case the smooth position has moved
   // inside it.

   // p is current position (smooth position)
   // s is the track simplex, with barycentric coords bc
   // t is track point on the surface (s->bc2pos(bc))
   // n is surface normal at track point

   if (!s)
      return Wvec::null();

   // get track point
   Wpt t;       
   s->bc2pos(bc, t);

   // get surface normal at track point
   Wvec n = bc2norm(s, bc);

   double h = (p - t)*n;
   if (stay_out)
      return (h >= 0) ? Wvec::null() : (t - p);
   else
      return (h <= 0) ? Wvec::null() : (t - p);

   return n * h;
}

inline Wpt
skin_loc(Bsimplex* track_sim, CWvec& bc, double h)
{
   assert(track_sim);
   Wpt ret = track_sim->bc2pos(bc);
   if (h != 0) {
      // add offset along skel normal:
      ret += (bc2norm(track_sim, bc)*h);
   }
   return ret;
}

bool 
SkinMeme::is_restricted() const
{
   // XXX - was going to use; changed my mind
   //       leaving here temporarily in case it is useful after all

   return _track_filter != &default_filter;
}

Wpt
SkinMeme::track_pt() const
{
   return is_tracking() ? track_simplex()->bc2pos(_bc) : loc();
}

Wvec
SkinMeme::track_norm() const
{
   return is_tracking() ? bc2norm(track_simplex(), _bc) : norm();
}

CWpt& 
SkinMeme::compute_update()
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_SKIN_UPDATE",false);

   // compute 3D vertex location WRT track simplex

   if (_is_sticky) {
      // this meme is supposed to follow the skeleton surface
      if (is_tracking()) {
         // it actually is following it
         return _update = skin_loc(track_simplex(), _bc, _h);
      }
      // supposed to follow, but has no track point: do nothing
      return _update = loc();
   }

   // this meme is not following the skeleton surface;
   // it computes its location via smooth subdivision.
   // but it may still track the closest point on the skeleton
   // surface to avoid penetrating inside the skeleton surface.

   if (vert()->parent() == 0)
      _update = loc();
   else
      _update = vert()->detail_loc_from_parent();
   track_to_target(_update);
   if (_non_penetrate && is_tracking()) {
      Wvec d = penetration_correction(_update, track_simplex(), _bc, _stay_out);
      if (debug && !d.is_null())
         err_msg("SkinMeme::compute_update: correcting penetration, level %d",
                 bbase()->subdiv_level());
      _update += d;
               
   }
   return _update;
}

inline bool
is_border(Bsimplex* s)
{
   if (is_face(s)) return false;
   if (is_edge(s)) return ((Bedge*)s)->is_border();
   if (is_vert(s)) return ((Bvert*)s)->is_border();
   return false;
}

bool
SkinMeme::compute_delt()
{
   static bool debug = ::debug || Config::get_var_bool("DEBUG_SKIN_MEMES",false);

   _delt = Wvec::null();

   if (is_frozen())
      return false;

   if (!is_tracking()) {
      err_msg("SkinMeme::compute_delt: not tracking, can't proceed");
      return false;
   }

   if (!is_boss()) {
      err_adv(debug, "SkinMeme::compute_delt: non-boss; bailing...");
      track_to_target(loc());
      return false;
   }

   Wpt cur = loc();
   Wpt centroid = vert()->qr_centroid();

   static const double bw = Config::get_var_dbl("SKIN_SMOOTH_CENTROID_WEIGHT",0.5);
   Wpt target = interp(cur, centroid, bw);

   // Target is where we want to be, but we'll settle for
   // somewhere on the skeleton surface that is as close as
   // possible.
   Wpt tp;
   if (track_to_target(target, tp)) {
//      double d = target.dist(tp);
//      Wpt new_loc = target + norm() * (fabs(_h) - d);
//      Wpt new_loc = tp + ((target - tp).normalized() * fabs(_h));

      // XXX - hack
      // can we figure this out, one day, soon??
      Wpt new_loc;
      if (is_border(track_simplex())) {
         new_loc = tp + track_norm()*_h;
      } else {
         new_loc = tp + ((target - tp).normalized() * fabs(_h));
      }

      _delt = new_loc - cur;
      return true;
   }

   err_adv(debug, "SkinMeme::compute_delt: track to target failed");
   return false;
}

bool
SkinMeme::apply_delt()
{
   // Nothing to do:
   if (_delt.is_null())
      return false;

   // Compute displaced location
//    _update = loc() + _delt;
//    _delt = Wvec::null();

   // trying this out: ignore computed _delta,
   // instead base new location on computed track point
   // (which was recomputed in compute_delt())
   return do_update();
//    return apply_update();
}

inline void
add_tracker(Bsimplex* s, CSimplexFilter& f, Bsimplex_list& ret)
{
   if (s && f.accept(s))
      ret.add_uniquely(s);
}

Bsimplex_list
SkinMeme::get_local_trackers() const
{
   // return a list of track simplices ("trackers") from
   // this skin meme and its immediate neighbors

   Bsimplex_list ret(vert()->degree() + 1);

   // add our tracker
   add_tracker(track_simplex(), *_track_filter, ret);

   // add neighbors' trackers
   static VertMemeList nbrs;
   get_nbrs(nbrs);
   for (int i=0; i<0; i++) {
      assert(isa(nbrs[i]));
      add_tracker(((SkinMeme*)nbrs[i])->track_simplex(), *_track_filter, ret);
   }
   return ret;
}

Bsimplex*
SkinMeme::update_tracker(
   CWpt& target,                // target point to try to reach
   CBsimplex_list& trackers,    // simplices from which to start a local search
   Wpt& near_pt,                // resulting nearest point on tracked surface
   Wvec& near_bc                // corresponding barycentric coords
   )
{
   // Do a local search to find the closest point to 'target' on the
   // tracked surface. Do the search starting from each "starter"
   // simplex (which should all be elements of the tracked surface
   // satisfied by our track filter). In the end take the one that
   // yielded the closest result, and return the corresponding track
   // simplex.  Also return point 'near_pt' on the simplex that is
   // nearest to the target, and the corresponding barycentric coords
   // in 'near_bc'.

   // XXX - assume all meshes have Identity transform. We
   // don't deal w/ translating between local coord systems.

   Bsimplex* best_simplex = 0;
   double min_dist = 0; // (initial value is irrelevant)

   assert(_track_filter && trackers.all_satisfy(*_track_filter));

   for (int i=0; i<trackers.num(); i++) {
      Bsimplex* tracker = trackers[i];
      assert(tracker);
      if (tracker == best_simplex)
         continue;
      // do a mesh walk from the tracker to a new, closer simplex
      Wpt  closest_pt;  // closest point on new_sim
      Wvec bc;          // barycentric coords of closest pt on new sim
      Bsimplex* new_sim = tracker->walk_to_target(
         target, closest_pt, bc, *_track_filter
         );
      assert(new_sim && _track_filter->accept(new_sim));
      if (new_sim == best_simplex)
         continue;
      double dist = target.dist(closest_pt);
      if (!best_simplex || dist < min_dist) {
         // This is the new best
         best_simplex = new_sim;
         min_dist     = dist;
         near_pt      = closest_pt;
         near_bc      = bc;
      }
   }
   return best_simplex;
}

bool
SkinMeme::track_to_target(CWpt& target, Wpt& near_pt)
{
   // Do a local search to find the closest point to 'target' on the
   // tracked surface. Do the search starting from our own track
   // simplex, and then repeat the search starting from each track
   // simplex that can be borrowed from our SkinMeme neighbors. In the
   // end take the one that yielded the closest result, and store the
   // corresponding track simplex.

   if (is_frozen())
      return is_tracking();

   Wvec near_bc;
   Bsimplex* track_sim = update_tracker(
      target, get_local_trackers(), near_pt, near_bc
      );

   // Record the result, even if unsuccessful:
   set(track_sim, near_bc);

   return (track_simplex() != 0);
}

// end of file skin_meme.C
