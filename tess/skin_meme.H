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
#ifndef SKIN_MEME_H_IS_INCLUDED
#define SKIN_MEME_H_IS_INCLUDED

#include "tess/meme.H"

class Skin;
/*****************************************************************
 * SkinMeme:
 *
 *   Vertex meme that moves toward centroid of neighboring
 *   vertices while sticking to some surface.
 *****************************************************************/
class SkinMeme : public VertMeme {
 public:

   //******** MANAGERS ********

   SkinMeme(Skin* skin, Lvert* v, Bsimplex* t=0);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("SkinMeme", SkinMeme*, VertMeme, CSimplexData*);

   //******** ACCESSORS ********

   // Return the "parent" meme if it exists:
   SkinMeme* parent()   const { return (SkinMeme*)VertMeme::parent(); }

   // Return the "child" meme if it exists:
   SkinMeme* child()    const { return (SkinMeme*)VertMeme::child(); }

   Bsimplex* track_simplex() const {
      return _trackers.empty() ? 0 : _trackers.first();
   }

   void set_track_simplex(Bsimplex* s);
   void add_track_simplex(Bsimplex* s);

   Wvec get_bc()                const   { return _bc;}
   void set_bc(CWvec &bc)               { _bc = bc;}

   void set(Bsimplex* t, CWvec& bc);

   Wpt  track_pt()      const;
   Wvec track_norm()    const;

   // multi-tracking is not used
   bool is_tracking()           const { return _trackers.num() > 0; }
   bool is_multi_tracking()     const { return _trackers.num() > 1; }

   void set_track_filter(SimplexFilter* f) { _track_filter = f; }
   bool is_restricted()         const;

   // sticky means it is actively sticking to the skeleton
   bool is_sticky()             const { return _is_sticky; }
   void set_sticky(bool b=true)       { _is_sticky = b; }
   void clear_sticky()                { _is_sticky = false; }

   // whether to use non-penetration policy, and if so
   // which side of the skeleton surface should be avoided
   void set_non_penetrate(bool b=true){ _non_penetrate = b; }
   void set_stay_outside(bool b=true) { _stay_out = b; }

   // A frozen meme won't slide over the tracked surface
   // during the smoothing process:
   void freeze()                { _is_frozen = true; }
   void unfreeze()              { _is_frozen = false; }
   bool is_frozen()     const   { return _is_frozen; }

   bool   has_offset()          const   { return _h != 0; }
   double offset()              const   { return _h; }
   void   add_offset(double d)          { set_offset(_h + d); }
   void   scale_offset(double s)        { set_offset(_h * s); }
   void   clear_offset()                { set_offset(0); }
   void   set_offset(double h);

   //******** VertMeme METHODS ********

   // Compute where to move to a more "relaxed" position:
   virtual bool compute_delt();
   virtual bool apply_delt();

   // Compute 3D vertex location WRT the track simplex
   virtual CWpt& compute_update();

   //******** SimplexData NOTIFICATION METHODS ********

 protected:
   Bsimplex_list  _trackers;      // track simplices
   Wvec     _bc;            // barycentric coords on given simplex
   Wvec     _delt;          // displacement computed in compute_delt()
   double         _h;             // displacement along normal
   bool           _is_sticky;     // sticks to skeleton surface
   bool           _non_penetrate; // true means exercise non-penetration policy
   bool           _stay_out;      // for non-penetration: stay on outside
   bool           _is_frozen;     // flag used to hold some memes in place
   SimplexFilter* _track_filter;  // filter used in updating track point

   //******** PROTECTED METHODS ********

   // Do a local search to find the closest point to
   // 'target' on the tracked surface:
   bool track_to_target(CWpt& target, mlib::Wpt& near_pt);
   bool track_to_target(CWpt& target) {
      Wpt p; return track_to_target(target, p);
   }

   Bsimplex_list get_local_trackers() const;

   Bsimplex* update_tracker(
      CWpt& target,             // target point to try to reach
      CBsimplex_list& trackers, // simplices from which to start a local search
      Wpt& near_pt,             // resulting nearest point on tracked surface
      Wvec& near_bc             // corresponding barycentric coords
      );

};

#endif // SKIN_MEME_H_IS_INCLUDED

// end of file skin_meme.H
