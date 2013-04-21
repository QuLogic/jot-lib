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
////////////////////////////////////////////
// HatchingGroupBase, ...LevelBase, ...HatchBase
////////////////////////////////////////////
//
// -Provides various methods useful to all
//  hatching group types
// -Generally a private parent class
// -Subclassed by fixed hatch groups and free
//  hatch group instances
// -HatchingLevel and HatchingHatch also here...
// -A hatch group is a set of Levels (LOD)
// -A level holds a set of Hatches
//
////////////////////////////////////////////


#include "std/config.H"
#include "geom/world.H"
#include "gtex/ref_image.H"
#include "npr/hatching_group_base.H"

using namespace mlib;

#define SELECTION_STROKE_TEXTURE "nprdata/system_textures/select_box.png"

#define ID_REF_RADIUS 2

#define HATCH_MAX_WINDING           0.5
#define HATCH_MAX_ABS_WINDING       1.5
#define HATCH_MIN_STRAIGHTNESS      0.6
#define HATCH_MIN_LENGTH            8.0
#define HATCH_SPLINE_MULTIPLE_LOW   1.5   //Mult. of ave. gest. spacing for control pts.
#define HATCH_SPLINE_MULTIPLE_HIGH  3.5   //Mult. of ave. gest. spacing for control pts.
#define HATCH_MIN_SPLINE_SAMPLES    5     //Fewest evenly space gest pts. to use
#define HATCH_SPLINE_SPACING        2.5   //Pixel resolution of spline sampling

/*****************************************************************
 * HatchingGroupBase
 *****************************************************************/


/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingGroupBase::HatchingGroupBase(HatchingGroup *hg) :
   _group(hg), _cam(Wpt(0,0,0)), _num_base_levels(0)
{
   _select = NULL;
   _selected = false;

   // Fill out by child class
   // Assumed to exist when group is 'complete'
   _backbone = NULL;

   _old_desired_level = 1.0;

   init();
}


/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingGroupBase::~HatchingGroupBase()
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupBase::~HatchingGroupBase()"); 

   clear();
}


/////////////////////////////////////
// init()
/////////////////////////////////////
//
// -In in case things need to be inited
//  differently in subclasses, we init
//  here in this subclassable method

// -e.g. _selected might take a subclass
//  of HatchingSelectBase in some other 
//  subclass of HathcingGroupBase
// -clear cleans up any stuff in init
//  and may also require subclassing
//
/////////////////////////////////////
void 
HatchingGroupBase::init() 
{
        
   // Add the base level


   assert(num_base_levels() == 0);

   add_base_level();

   //Make sure this base level is drawn!
   base_level(0)->set_desired_frac(1.0);

   _select = new HatchingSelectBase(this);
   assert(_select);

}


/////////////////////////////////////
// clear()
/////////////////////////////////////
void
HatchingGroupBase::clear()
{
   int i;

   for (i=0;i<_level.num();i++) delete _level[i];

   assert(_select);
   delete(_select);
   _select = NULL;
}

/////////////////////////////////////
// select()
/////////////////////////////////////
void    
HatchingGroupBase::select()
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupBase:select()"); 

   _selected = true;

   _select->clear();
}


/////////////////////////////////////
// deselect()
/////////////////////////////////////
void    
HatchingGroupBase::deselect()
{
   _selected = false;
}

/////////////////////////////////////
// kill_animation()
/////////////////////////////////////
void    
HatchingGroupBase::kill_animation()
{
   int l;

   for (l=0 ; l<_level.num(); l++)
      if (_level[l]->in_trans()) _level[l]->abort_transition();

}

/////////////////////////////////////
// draw()
/////////////////////////////////////
//
// -Just draws the levels (which draw hatches)
// -All the setups must be called...
// -See FixedHatchingGroup draw() for full drawing
//
/////////////////////////////////////
int     
HatchingGroupBase::draw(CVIEWptr &v)
{
   int i,num = 0;
        
   _cam = _group->patch()->inv_xform() * VIEW::peek_cam()->data()->from();

   if (_selected) _select->clear();

   for (i=0 ; i<_level.num() ; i++) 
      num += _level[i]->draw(v);

   return num;
}

/////////////////////////////////////
// draw_select()
/////////////////////////////////////
int     
HatchingGroupBase::draw_select(CVIEWptr &v)
{
   return (_selected)? _select->draw(v) : 0;
}

/////////////////////////////////////
// level_draw_setup()
/////////////////////////////////////
void    
HatchingGroupBase::level_draw_setup()
{
   //Advance level animations, etc.
   for (int i=0 ; i<_level.num() ; i++) 
      _level[i]->draw_setup();
}

/////////////////////////////////////
// hatch_draw_setup()
/////////////////////////////////////
void    
HatchingGroupBase::hatch_draw_setup()
{
   //Causes hatches to update (we do this outside of level_draw_setup
   //in hopes of conglomerating code to leverage the cache)
   for (int i=0 ; i<_level.num() ; i++) 
      _level[i]->hatch_draw_setup();
}

/////////////////////////////////////
// draw_setup()
/////////////////////////////////////
void    
HatchingGroupBase::draw_setup()
{
       
   //Advance level animations

   if (is_complete())
      update_levels();

}
/////////////////////////////////////
// add_base_level()
/////////////////////////////////////
void    
HatchingGroupBase::add_base_level()
{ 
   assert(_level.num() == _num_base_levels);
   HatchingLevelBase *hlb = new HatchingLevelBase(this); 
   assert(hlb); 
   _level.add(hlb); 
   _num_base_levels++; 
}

/////////////////////////////////////
// pop_base_level()
/////////////////////////////////////
HatchingLevelBase*
HatchingGroupBase::pop_base_level()
{ 
   assert(_num_base_levels == _level.num());
   assert(_level.last()->num() == 0);
   assert(_level.num() > 1);
   _num_base_levels--; 
   
   return _level.pop();
}

/////////////////////////////////////
// trash_upper_levels()
/////////////////////////////////////
void
HatchingGroupBase::trash_upper_levels()
{ 
   while (_level.num() > _num_base_levels)
   {
      HatchingLevelBase *hlb = _level.pop(); assert(hlb);
      
      err_mesg(ERR_LEV_SPAM, "HatchingGroupBase::trash_upper_levels() - Trashing level %d (%d hatches)",
            (_level.num() + 1), hlb->num() ); 
     
      delete hlb;
   }

}


/////////////////////////////////////
// update_levels()
/////////////////////////////////////
void    
HatchingGroupBase::update_levels()
{
   //Compute LOD measure
   //Update animation paramaters

   int l;

   HatchingGroupParams *p = _group->get_params();

   int    style         = p->anim_style();
   double lo_thresh     = p->anim_lo_thresh();
   double hi_thresh     = p->anim_hi_thresh();
   double lo_width      = p->anim_lo_width();
   double hi_width      = p->anim_hi_width();
   double master_scale  = p->anim_global_scale();
   double limit_scale   = p->anim_limit_scale();
   double trans_time    = p->anim_trans_time();
   double max_level     = p->anim_max_lod();


   double ratio_adjust;    //Overall scale of original size to base size
   double limit_adjust;    //Scaling when above/below the max/min level
   double desired_level;   //Desired fractional level

   int base_level;         //Level upto which we should be visible
   int trans_level;        //Level that's fractionally approached
        
   // Setup the desired levels, and scaling ratios
   if (style == HatchingGroup::STYLE_MODE_NEAT)
   {
      assert(_backbone);
      ratio_adjust = _backbone->get_ratio();

      // Smaller than min level
      if (ratio_adjust < 1.0)
      {
         desired_level = 0.0;
         limit_adjust = ratio_adjust;
      }
      else
      {
         desired_level = log(ratio_adjust)/log(2.);
     
         // Within allowed levels
         if (desired_level < max_level)
         {
            limit_adjust = 1.0;
         }
         // Above max level
         else
         {
            limit_adjust = 1.0 + (desired_level - max_level);
            desired_level = max_level;
         }
      }
      base_level = (int)floor(desired_level);
      trans_level = base_level + 1;
        
      if (_level.num() < trans_level+1) generate_interpolated_level(trans_level);
   }
        //(style == HatchingGroup::STYLE_MODE_SLOPPY_REP) ||
   else //(style == HatchingGroup::STYLE_MODE_SLOPPY_ADD)
   {
      double pix_size = _group->patch()->mesh()->pix_size();
      
      ratio_adjust = pix_size/_level[0]->pix_size();

      // Smaller than min level
      if (ratio_adjust < 1.0)
      {
         desired_level = 0.0;
         limit_adjust = ratio_adjust;
      }
      else
      {
         // Look for first larger level
         for( l=0; l < _num_base_levels; l++ )
         {
            if (_level[l]->pix_size() > pix_size) 
            {
               l++;
               break;
            }
         }
         l--;

         // Within allowed levels
         if (_level[l]->pix_size() > pix_size)
         {
            assert(l>0);

            desired_level = (double)(l-1) + 
                  (pix_size - _level[l-1]->pix_size()) /
                             (_level[ l ]->pix_size() - _level[l-1]->pix_size());
            limit_adjust = 1.0;
         }
         // Above max level
         else
         {
            desired_level = (double)(_num_base_levels-1);

            // If only one level use the pix ratio
            if (_num_base_levels == 1)
            {
               limit_adjust = ratio_adjust;
            }
            // Otherwise use the min to max in ratio
            else
            {
               limit_adjust =  (pix_size - _level[0]->pix_size()) /
                      (_level[_num_base_levels-1]->pix_size() - _level[0]->pix_size());

            }
         }
      }
      base_level = (int)floor(desired_level);
      trans_level = base_level + 1;

      //If were showing all levels, there's no transition level!
      if (trans_level == _num_base_levels) 
         trans_level = 0;
   }

   //Adjust widths and schedule transitions

   //Base level is set of strokes (including lower levels for additive hatching)  
   //that should be visible.
   //Transition level is the the level that's only fractionally visible.
   //Its fractionality decides the width of all strokes and it's state 
   //(and the base state) decide whether a scheduled transition state is necessary

   // The fractionality of the transition level (if there is one)
   double frac = desired_level - base_level;

   // The global width multiplier
   double desired_frac;

   if ((frac < lo_thresh) || 
      ((frac < hi_thresh) && (trans_level) && (!_level[trans_level]->non_zero()) ))
      desired_frac = 1.0 + (hi_width - 1.0) * (frac/hi_thresh);
   else 
      desired_frac = lo_width + (1 - lo_width) * ((frac - lo_thresh)/(1 - lo_thresh));

   desired_frac *=   (1.0 + (ratio_adjust-1.0) * master_scale) * 
                     (1.0 + (limit_adjust-1.0) * limit_scale);
   
   //Clear extinction flags from non transitioning levels
   for (l=0 ; l<_level.num(); l++)
      if (!_level[l]->in_trans()) _level[l]->clear_ext();

   //Set the new width targets for all levels below the base level
   for (l=0; l<base_level; l++)
   {
      if (style == HatchingGroup::STYLE_MODE_SLOPPY_REP)
      {  //These levels should be invisible in replacement mode

         //Start transitions for visible levels that aren't already transitioning
         if ( (_level[l]->non_zero()) && !(_level[l]->in_trans()) )
         {
            _level[l]->start_transition(trans_time);
            _level[l]->set_ext(HatchingLevelBase::EXT_DIE);
         }
         _level[l]->set_desired_frac(0.0);
      }
      else //These levels should be visible in additive mode
      {
         //Start transitions for invisible levels that aren't already transitioning
         if ( !(_level[l]->non_zero()) && !(_level[l]->in_trans()) )
         {
            _level[l]->start_transition(trans_time);
            _level[l]->set_ext(HatchingLevelBase::EXT_GROW);
         }
         _level[l]->set_desired_frac(desired_frac);
      }
   }

   //If we're at the max level, there's no transition level. 
   //Just set the target width and fire up a transition if req'd
   if (!trans_level)
   {
      if (!_level[base_level]->non_zero())
      {
         _level[base_level]->set_ext(HatchingLevelBase::EXT_GROW);
         for (l=0; l<base_level; l++) _level[l]->start_transition(trans_time);
      }
      _level[base_level]->set_desired_frac(desired_frac);
   }
   //Otherwise there is a transition level and both the
   //base and trans levels must be examined for transition scheduling
   else 
   {
      //If we're below threshold then set the desired
      //widths; checking that the base and trans levels
      //are doing the right thing
      if (frac < lo_thresh) 
      {
         if (!_level[base_level]->non_zero() || _level[trans_level]->non_zero())
         {
            if (!_level[base_level]->non_zero()) 
               _level[base_level]->set_ext(HatchingLevelBase::EXT_GROW);
            if (_level[trans_level]->non_zero()) 
               _level[trans_level]->set_ext(HatchingLevelBase::EXT_DIE);
            for (l=0; l<=trans_level; l++)
               _level[l]->start_transition(trans_time);
         }
         _level[base_level]->set_desired_frac(desired_frac);
         _level[trans_level]->set_desired_frac(0.0);
      } 
      //If we're above threshold then set the desired widths;
      //checking that the base and trans levels are doing right
      else if (frac >=hi_thresh)
      {
         if (style == HatchingGroup::STYLE_MODE_SLOPPY_REP)
         {
            if (_level[base_level]->non_zero() || !_level[trans_level]->non_zero())
            {
               if (_level[base_level]->non_zero()) 
                  _level[base_level]->set_ext(HatchingLevelBase::EXT_DIE);
               if (!_level[trans_level]->non_zero()) 
                  _level[trans_level]->set_ext(HatchingLevelBase::EXT_GROW);
               for (l=0; l<=trans_level; l++)
                  _level[l]->start_transition(trans_time);
            }
            _level[base_level]->set_desired_frac(0.0);
            _level[trans_level]->set_desired_frac(desired_frac);
         }
         else
         {
            if (!_level[base_level]->non_zero() || !_level[trans_level]->non_zero())
            {
               if (!_level[base_level]->non_zero()) 
                  _level[base_level]->set_ext(HatchingLevelBase::EXT_GROW);
               if (!_level[trans_level]->non_zero()) 
                  _level[trans_level]->set_ext(HatchingLevelBase::EXT_GROW);
               for (l=0; l<=trans_level; l++)
                  _level[l]->start_transition(trans_time);
            }
            _level[base_level]->set_desired_frac(desired_frac);
            _level[trans_level]->set_desired_frac(desired_frac);
         }
      }
      //Otherwise, were in the hysteresis zone 
      else
      {
         //In replacement mode, only one of the base or
         //trans levels should be visible. If this isn't
         //true, fire up a transition and default to
         //setting the base level visible
         if (style == HatchingGroup::STYLE_MODE_SLOPPY_REP)
         {
            if (_level[base_level]->non_zero() == _level[trans_level]->non_zero())
            {
               if (!_level[base_level]->non_zero()) 
                  _level[base_level]->set_ext(HatchingLevelBase::EXT_GROW);
               _level[trans_level]->set_ext(HatchingLevelBase::EXT_DIE);

               for (l=0; l<=trans_level; l++)
                  _level[l]->start_transition(trans_time);

               _level[base_level]->set_desired_frac(desired_frac);
               _level[trans_level]->set_desired_frac(0.0);
            }
            else
            {
               if (_level[base_level]->non_zero())
                  _level[base_level]->set_desired_frac(desired_frac);
               if (_level[trans_level]->non_zero())
                  _level[trans_level]->set_desired_frac(desired_frac);
            }
         }
         //In additive mode, just check that the base level
         //is visible and transition if not.  Then set both
         //level widths correctly.
         else
         {
            if (!_level[base_level]->non_zero())
            {
               _level[base_level]->set_ext(HatchingLevelBase::EXT_GROW);
               for (l=0; l<=trans_level; l++)
                  _level[l]->start_transition(trans_time);
            }
            _level[base_level]->set_desired_frac(desired_frac);
            if (_level[trans_level]->non_zero())
               _level[trans_level]->set_desired_frac(desired_frac);
         }
      }
   }


   //Make sure extra levels are zeroed out
   if (trans_level)
   {
      for (l=trans_level+1 ; l<_level.num(); l++)
      {
         if (_level[l]->non_zero()) 
         {
            _level[l]->set_desired_frac(0);
            if (!_level[l]->in_trans()) 
            {
               _level[l]->set_ext(HatchingLevelBase::EXT_DIE);
               _level[l]->start_transition(trans_time);    
            }
         }
      }
   }

   static bool keep_old_trans = Config::get_var_bool("HATCHING_KEEP_OLD_TRANSITIONS",false,true);

   if (!keep_old_trans)
   {
      if (fabs(desired_level - _old_desired_level) > 1.0)
      {
         err_mesg(ERR_LEV_SPAM, "HatchingGroupBase::update_level() - Aborting out-of-date level transitions.");
         for (l=0 ; l<_level.num(); l++)
            if (_level[l]->in_trans()) 
            {
               _level[l]->abort_transition();
               _level[l]->clear_ext();
            }
      }
   }

   _old_desired_level = desired_level;


}

/////////////////////////////////////
// update_prototype()
/////////////////////////////////////
void    
HatchingGroupBase::update_prototype()
{
   int i;

   for (i=0; i<_level.num() ; i++)
      _level[i]->update_prototype();
        
}

/////////////////////////////////////
// smooth_gesture()
/////////////////////////////////////
bool    
HatchingGroupBase::smooth_gesture(
   CNDCpt_list&         pts,    // input points
   NDCpt_list&          spts,   // smoothed output points
   const ARRAY<double>& prl,    // input pressures
   ARRAY<double>&       sprl,   // smoothed output pressures
   int                  style   // hatching style (affects how much smoothing)
   )
{
   // Q: Would they ever call this on an empty list?
   // A: Yes.
   if (pts.empty()) 
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::smooth_gesture() - Error: empty point list.");
      return false;
   }
   if (prl.empty()) 
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::smooth_gesture() - Error: empty pressure list.");
      return false;
   }

   // multiple of ave. gesture spacing:
   double spline_spacing;

   double sample_spacing = HATCH_SPLINE_SPACING;  //in fractional pixels

   if (style == HatchingGroup::STYLE_MODE_NEAT)
      spline_spacing = HATCH_SPLINE_MULTIPLE_HIGH; 
   else
      spline_spacing = HATCH_SPLINE_MULTIPLE_LOW; 

   int k, num;
   double dlen,dave;

   int seg;
   double frac;

   //***assume pts length is updated***

   //Resample to shortest spacing

   dlen = pts.segment_length(0);
   dave = pts.segment_length(0);
   for (k=1; k<pts.num()-1; k++)
   {
      dave += pts.segment_length(k);
      if (pts.segment_length(k) < dlen) dlen = pts.segment_length(k);
   }
   dave /= (double)pts.num();

   num = (int)ceil(pts.length()/dlen);

   dlen = 1.0/(double)num;

   for (k=0; k<=num; k++) 
   {
      spts += pts.interpolate((double)k*dlen,0,&seg,&frac);
      sprl += prl[seg]*(1.0-frac) + prl[seg+1]*frac;
   }
   spts.update_length();

   if (spts.empty()) 
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::smooth_gesture() - Warning: Smoothed point list is empty.");
      return false;
   }

   CRSpline spline;
   CRSpline pspline;

   //Input gesture is now equally spaced, we could
   //just pick off multiples, but we'll spline instead
   //using a desired subsampling as control pts

   //int cnt = (int)ceil(VIEW::peek()->ndc2pix_scale() * spts.length() / spline_spacing);
   int cnt = (int)ceil((spts.length() / dave)/spline_spacing);
   
   // But we'll not sample fewer than some arbitrary
   // amount in case the fps is low and we've few pts.
   cnt = max(cnt, HATCH_MIN_SPLINE_SAMPLES);

   dlen = 1.0/(double)(cnt-1);
   
   for (k=0 ; k<cnt ; k++) 
   {
      spline.add(Wpt(XYpt(spts.interpolate((double)k*dlen,0,&seg,&frac))),k);
      double p = sprl[seg]*(1.0-frac) + sprl[seg+1]*frac;
      pspline.add(Wpt(p,p,p),k);
   }
        
   int pix_len = (int)ceil(spts.length() *
                           VIEW::peek()->ndc2pix_scale() / sample_spacing);
   if (pix_len < 2) pix_len = 2;

   dlen = (double)(cnt-1)/(double)(pix_len - 1);

   spts.clear();
   sprl.clear();
   for (k=0 ; k<pix_len ; k++) 
   {
      spts += NDCpt(spline.pt((double)k*dlen));
      sprl += (pspline.pt((double)k*dlen))[0];
   }
   spts.update_length();

   return true;
}
/////////////////////////////////////
// is_gesture_silly()
/////////////////////////////////////
bool    
HatchingGroupBase::is_gesture_silly(
   CNDCpt_list &pts,
   int style)
{
   //Assume length are up to date

   double scale = VIEW::peek()->ndc2pix_scale();

   if (pts.length()*scale < HATCH_MIN_LENGTH)
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::is_gesture_silly() - Too short: %f (Actual Length: %f, Scaling Factor: %f)", (float)(pts.length()*scale), (float)pts.length(), (float)scale);
      return true;
   }
      
   if (pts.num() < 3)
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::is_gesture_silly() - Too few points: %d", pts.num());
      return true;
   }
   
   //If the hatch group isn't neat, this hatch is
   //not silly...
   if (style != HatchingGroup::STYLE_MODE_NEAT)
      return false;

   if (pts.self_intersects())
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::is_gesture_silly() - Self-intersects!");
      return true;
   }

   double wind=0.0, awind = 0.0;
   int k;
   for (k = 1; k < pts.num()-1; k++) {
      NDCvec v1 = (pts[k]   - pts[k-1]).normalized();
      NDCvec v2 = (pts[k+1] - pts[k]).normalized();
      double  a = Sign(det(v1,v2)) * acos(max(-0.999,min(v1*v2, 0.999)));
      wind += a;
      awind += fabs(a);
   }

   wind /= (M_PI*2);   
   wind = fabs(wind);
   awind /= (M_PI*2);   

   double straight = pts[pts.num()-1].dist(pts[0]) / pts.length();

   if (wind > HATCH_MAX_WINDING)
   { 
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::is_gesture_silly() - Winding # too high: %f", wind);
      return true;
   }

   if (awind > HATCH_MAX_ABS_WINDING)
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::is_gesture_silly() - Absolute Winding # too high: %f", awind);
      return true;
   }

   if (straight < HATCH_MIN_STRAIGHTNESS)
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::is_gesture_silly() - Straightness too low: %f", straight);
      return true;
   }

   double minx, maxx, miny, maxy;

   minx = maxx = pts[0][0];
   miny = maxy = pts[0][1];

   for (k=1 ; k<pts.num(); k++)
   {
      if (pts[k][0] > maxx) maxx = pts[k][0];
      if (pts[k][0] < minx) minx = pts[k][0];
      if (pts[k][1] > maxy) maxy = pts[k][1];
      if (pts[k][1] < miny) miny = pts[k][1];
   }

   //Check that the stroke's extremals are at
   //the endpoints, and 

   if (!((pts[0][0] == minx) || (pts[0][0] == maxx) ||
         (pts[0][1] == miny) || (pts[0][1] == maxy) ))
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::is_gesture_silly() - First point not on stroke bounding box.");
      return true;
   }

   if (!((pts[pts.num()-1][0] == minx) || (pts[pts.num()-1][0] == maxx) ||
         (pts[pts.num()-1][1] == miny) || (pts[pts.num()-1][1] == maxy) ))
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::is_gesture_silly() - Last point not on stroke bounding box.");
      return true;
   }


   return false;

}

/////////////////////////////////////
// fit_line() - y=a*x+b 
/////////////////////////////////////
bool 
HatchingGroupBase::fit_line(CNDCpt_list &pts, double &a, double &b ) {


   double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0, denom;
   int i;


   if (pts.num() < 2) 
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::fit_line() - Can't do linear fit to < 2 points...");
      return false;
   }


   for (i=0;i<pts.num();i++) {
      sumX += pts[i][0];
      sumY += pts[i][1];
      sumXY += pts[i][0]*pts[i][1];
      sumX2 += pts[i][0]*pts[i][0];
   }


   denom = pts.num()*sumX2 - sumX*sumX;


   if (denom == 0.0) 
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::fit_line() - Can't do linear fit with denom=0...");
      return false;
   }


   a = ( pts.num()*sumXY - sumX*sumY )/denom;
   b = ( sumX2*sumY - sumXY*sumX )/denom;


   return true;
}


/////////////////////////////////////
// compute_cutting_plane()
/////////////////////////////////////
Bface *         
HatchingGroupBase::compute_cutting_plane(
   Patch * pat,
   double a,          double b, 
   CNDCpt_list &pts, Wplane &p ) 
{


   NDCpt ndcLeft;  
   NDCpt ndcRight;
   Bface *f;


   //Want to keep the 2 endpoints on the screen
   NDCpt ndclim(XYpt(1.0,1.0));


   double x = ndclim[0];
   double y = ndclim[1];


   if           (( a*(-x)+b) >  y) ndcLeft =       NDCpt(( y-b)/a,          y);
   else if (( a*(-x)+b) < -y)      ndcLeft =       NDCpt((-y-b)/a,        - y);    
   else                                                            ndcLeft =       NDCpt(      -x,  a*(-x)+ b);
   if           (( a*  x +b) >  y) ndcRight =      NDCpt(( y-b)/a,          y);
   else if (( a*  x +b) < -y)      ndcRight =      NDCpt((-y-b)/a,         -y);    
   else                                                            ndcRight =      NDCpt(       x,  a * x + b);


   Wpt wCam = VIEW::peek_cam()->data()->from();
   Wvec wvLeft = Wpt(XYpt(ndcLeft)) - wCam;
   Wvec wvRight = Wpt(XYpt(ndcRight)) - wCam;
   Wvec wvLineNorm = cross(wvRight,wvLeft).normalized();


   NDCpt ndcMidLine;
   NDCpt ndcMidDraw = pts.interpolate(0.5);


   //Point on line closest to midpoint of gesture stroke


   ndcMidLine[0] = (ndcMidDraw[0] + a*ndcMidDraw[1] - a*b)/(1 + a*a);
   ndcMidLine[1] = a*ndcMidLine[0] + b;


   Wpt wIsect;
   f = find_face_vis(ndcMidLine,wIsect);
   if (!f) 
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupBase::compute_cutting_plane() - Nearest pt. on interp. line misses all meshes!"); 
      return 0;
   }
   else
   {
      if (f->patch() != pat)
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupBase::compute_cutting_plane() - Nearest pt. on interp. line misses right patch!"); 
         return 0;
      }
   }


   Wvec wvFaceNorm = f->mesh()->xform()*f->norm();
   Wvec wvPlaneNorm = cross(cross(wvFaceNorm,wvLineNorm),wvFaceNorm).normalized();
   //p = Wplane(f->mesh()->inv_xform()*wIsect,f->mesh()->inv_xform()*wvPlaneNorm);
   p = Wplane(wIsect,f->mesh()->inv_xform()*wvPlaneNorm);

   return f;

}

/////////////////////////////////////
// clip_curve_to_stroke()
/////////////////////////////////////
void
HatchingGroupBase::clip_curve_to_stroke(
   Patch *pat,
   CNDCpt_list &gestpts, 
   CWpt_list &allpts, 
   Wpt_list &clippts)
{
   int k, i1, i2;


   NDCpt_list ndclList;
   for (k=0;k<allpts.num();k++){
      ndclList += NDCpt(pat->xform()*allpts[k]);
   }


   NDCpt ndcEnd1;
   NDCpt ndcEnd2;
   
   double foo;
   ndclList.closest(gestpts[0],              ndcEnd1, foo, i1);
   ndclList.closest(gestpts[gestpts.num()-1],ndcEnd2, foo, i2);

   Wpt wEnd1;
   Wpt wEnd2;


   if (i1 < allpts.num() -1) {
      double del = (ndclList[i1+1] - ndclList[i1]).length();
      double d =          (ndcEnd1 - ndclList[i1]).length();
      if (del>0)
      {
         double frac = d/del;
         if (frac>1) frac=1;
         wEnd1 = allpts[i1]*(1.0-frac) + allpts[i1+1]*frac;
      }
      else
         wEnd1 = allpts[i1];
   }
   else {
      wEnd1 = allpts[allpts.num()-1];
   }
   if (i2 < allpts.num() -1) {
      double del = (ndclList[i2+1] - ndclList[i2]).length();
      double d =          (ndcEnd2 - ndclList[i2]).length();
      if (del>0)
      {
         double frac = d/del;
         if (frac>1) frac=1;
         wEnd2 = allpts[i2]*(1.0-frac) + allpts[i2+1]*frac;
      }
      else
         wEnd2 = allpts[i2];
   }
   else {
      wEnd2 = allpts[allpts.num()-1];
   }


   if (i1 <= i2) {
      clippts += wEnd1;
      for (k=i1+1 ; k<=i2 ; k++) {
         clippts += allpts[k];
      }
      clippts += wEnd2;
   }
   else {
      clippts += wEnd1;
      for (k=i1 ; k>i2 ; k--) {
         clippts += allpts[k];
      }
      clippts += wEnd2;
   }
}


/////////////////////////////////////
// find_face_vis()
/////////////////////////////////////
Bface *
HatchingGroupBase::find_face_vis(
   CNDCpt& pt,
   Wpt &p
   )
{
   static VisRefImage *vis_ref = VisRefImage::lookup(VIEW::peek());

   if (vis_ref == NULL) 
   {
      err_mesg(ERR_LEV_ERROR, "HatchingGroupBase::find_face_vis() - Error: Can't get visibility reference image!"); 
      return 0;
   }

   // Does a thing using the faces attached to
   // edges we hit... not good...
   // vis_ref->vis_update();
   // ret = vis_ref->vis_intersect(XYpt(pt),p);

   Bsimplex *s = vis_ref->intersect_sim(pt, p);

   if (is_face(s))
      return (Bface*)s;
   else
      return 0;
}
/////////////////////////////////////
// find_face_id()
/////////////////////////////////////
Bface *
HatchingGroupBase::find_face_id(CNDCpt& pt)
{
   static BfaceFilter filt;

   static IDRefImage* id_ref = IDRefImage::lookup(VIEW::peek());

   if (id_ref == NULL) 
   {
      err_mesg(ERR_LEV_ERROR, "HatchingGroupBase::find_face_id() - Error: Can't get ID reference image!"); 
      return 0;
   } 

   Bsimplex *s = id_ref->simplex(pt);

   if (is_face(s))
      return (Bface*)s;
   else
      return (Bface*)id_ref->find_near_simplex(pt,ID_REF_RADIUS,filt);
}

/////////////////////////////////////
// query_filter_id()
/////////////////////////////////////
bool
HatchingGroupBase::query_filter_id(CNDCpt& pt, CSimplexFilter& filt)
{

   //Tries the simplex at the given pixel
   //and applies the filter if it's a face.
   //If it's not a face (an edge, say), then
   //we try in a region about the pixel until
   //we hit, or else fail on all of them.
   //The filter should check is it's a face,
   //and then test visibility if it is.

   static IDRefImage* id_ref = IDRefImage::lookup(VIEW::peek());

   if (id_ref == NULL) 
   {
      err_mesg(ERR_LEV_ERROR, "HatchingGroupBase::query_filter_id() - Error: Can't get ID reference image!"); 
      return 0;
   } 

   return (id_ref->find_near_simplex(pt,ID_REF_RADIUS,filt) != 0);
}


/////////////////////////////////////
// compute_hatch_indices()
/////////////////////////////////////
//
// -If there are num hatches in the base level
//  then assuming we have all the hatches up to the
//  k-1'st level already generated, this function
//  will return the level and hatch index (l,i)
//  of the k'th hatch
//
/////////////////////////////////////
void
HatchingGroupBase::compute_hatch_indices(int &level,int &index, int i, int depth)
{
   int factor = 1 << depth;

   int base = i/factor;
   int mod =  i%factor;

   int temp = 0;

   level = 0;

   while (mod != 0)
      {
         level++;
         factor >>= 1;
         temp <<= 1;

         temp += mod/factor;
         mod %= factor;
      }

   if (level==0) index = base;
   else
   {
      index = (base << (level-1)) + (temp >> 1);
   }
}

/////////////////////////////////////
// generate_interpolated_level()
/////////////////////////////////////
//
// -To compute the lev'th level we recursively
//  call this on the lev-1'st level until lev-1 is
//  an existing level.
// -We can then generate the (lev-1)*(num-1)
//  hatches in the lev level (given there are
//  num hatches in the base level)
//
/////////////////////////////////////
void
HatchingGroupBase::generate_interpolated_level(int lev)
{
   assert(_level.num() < lev + 1);
   assert(lev>0);

   //Make sure previous level of interpolation exists (recursive)

   int k;
   int l1,i1,l2,i2;
//   HatchingHatchBase *hhb1, *hhb2;
   int num;

   if (_level.num() < lev) generate_interpolated_level(lev-1);

   _level.add(new HatchingLevelBase(this));

   num = _level[0]->num();
   for (k=0; k<((1<<(lev-1))*(num-1)); k++) 
      {
         compute_hatch_indices(l1, i1, k, lev-1);
         compute_hatch_indices(l2, i2, k+1, lev-1);
         _level[lev]->add_hatch(
            interpolate(lev,_level[lev],(*_level[l1])[i1],(*_level[l2])[i2]));
      }

   err_mesg(ERR_LEV_INFO, "HatchingGroupBase::generate_interpolated_level() - Created level #%d.", lev); 
}

/*****************************************************************
 * HatchingLevelBase
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingLevelBase::_hlb_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingLevelBase::tags() const
{
   if (!_hlb_tags) {
      _hlb_tags = new TAGlist;
      *_hlb_tags += new TAG_val<HatchingLevelBase,double>(
         "pix_size",
         &HatchingLevelBase::pix_size_);
      *_hlb_tags += new TAG_meth<HatchingLevelBase>(
         "hatch",
         &HatchingLevelBase::put_hatchs,
         &HatchingLevelBase::get_hatch,
         1);

   }
   return *_hlb_tags;
}

/////////////////////////////////////
// put_hatchs()
/////////////////////////////////////
//
// -Actually dumps a tag for each one
//
//////////////////////////////////////
void
HatchingLevelBase::put_hatchs(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingLevelBase::put_hatchs()");

   int i;
   assert(num() > 0);
   for (i=0; i<num(); i++)
   {
      d.id();
      (*this)[i]->format(*d);
      d.end_id();
   }
}

/////////////////////////////////////
// get_hatch()
/////////////////////////////////////
void
HatchingLevelBase::get_hatch(TAGformat &d)
{
   //Grab the class name... should be HatchingHatchFixed, or HatchingHatchFree
   str_ptr str;
   *d >> str;      

   err_mesg(ERR_LEV_SPAM, "HatchingLevelBase::get_hatch() - Found: '%s'", **str);

   DATA_ITEM *data_item = DATA_ITEM::lookup(str);

   if (!data_item) 
   {
      err_mesg(ERR_LEV_ERROR, "HatchingLevelBase::get_hatch() - Class: '%s' could not be found!", **str);
      return;
   }
   if (!HatchingHatchBase::isa(data_item)) 
   {
      err_mesg(ERR_LEV_ERROR, "HatchingLevelBase::get_hatch() - Class: '%s' not derived from 'HatchingHatchBase'!", **str);
      return;
   }

   HatchingHatchBase *hhb = (HatchingHatchBase*) data_item->dup();
   assert(hhb);
   
   hhb->set_level(this);
   hhb->decode(*d);

   add_hatch(hhb);


}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingLevelBase::HatchingLevelBase(HatchingGroupBase *hgb) :
   _group(hgb)
{
   _desired_frac = 0.0;
   _current_frac = 0.0;
   _trans_time = 0.0;

   //If this is 0, then this level is not 'complete'
   _pix_size = 0.0;

   _extinction = EXT_NONE;
}


/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingLevelBase::~HatchingLevelBase()
{


   while (num()>0)
   {
      assert(first());
      delete first();
      remove(0);
   }
}


/////////////////////////////////////
// draw()
/////////////////////////////////////
int     
HatchingLevelBase::draw(CVIEWptr &v)
{
        
   int i, n=0;

   // Bail out if width is zero
   if (_current_width == 0.0)
      return 0;

   for (i=0; i<num() ; i++)
      n += array()[i]->draw(v);


   return n;
}


/////////////////////////////////////
// draw_setup()
/////////////////////////////////////
void    
HatchingLevelBase::draw_setup()
{
   update_transition();
}


/////////////////////////////////////
// draw_hatch_setup()
/////////////////////////////////////
void    
HatchingLevelBase::hatch_draw_setup()
{
   int i;

   for (i=0; i<num() ; i++)
      array()[i]->draw_setup();

}

/////////////////////////////////////
// update_prototype()
/////////////////////////////////////
void    
HatchingLevelBase::update_prototype()
{
   int i;

   for (i=0; i<num() ; i++)
      array()[i]->update_prototype();
        
}

/////////////////////////////////////
// add_hatch()
/////////////////////////////////////
void
HatchingLevelBase::add_hatch(HatchingHatchBase *hhb)
{
   assert(hhb);
   add(hhb);
}

/////////////////////////////////////
// start_transition()
/////////////////////////////////////
void                                    
HatchingLevelBase::start_transition(double t)
{
   _trans_time = t;
   _trans_begin_frac = _current_frac;
   _watch = VIEW::peek()->frame_time();
}

/////////////////////////////////////
// update_transitions()
/////////////////////////////////////
void
HatchingLevelBase::update_transition()
{
//   int k;

   //Use scheduling to get new width fraction

   if (in_trans()) 
   {
      double elapsed = VIEW::peek()->frame_time() - _watch;
      if (elapsed > _trans_time)
      {
         _trans_time = 0;
         _current_frac = _desired_frac;
      }
      else 
      {
         _current_frac = _trans_begin_frac + 
            (_desired_frac - _trans_begin_frac) * (elapsed/_trans_time);
      }
      //Keeps BufferRefImage from updating until the animation's done
      VIEWobs::notify_viewobs(VIEW::peek(),VIEW::UNKNOWN_CHANGED);
   }
   else 
      _current_frac = _desired_frac;

   _current_width = _group->group()->prototype()->get_width() * _current_frac;

   if ((_group->group()->prototype()->get_alpha() < 1.0) ||
      (_group->group()->prototype()->get_texture() != NULL))
         _current_alpha_frac = (_extinction == EXT_NONE)?(1.0):(min(_current_frac,1.0));
   else
      _current_alpha_frac = 1.0;

}

/*****************************************************************
 * HatchingHatchBase
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingHatchBase::_hhb_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingHatchBase::tags() const
{
   if (!_hhb_tags) {
      _hhb_tags = new TAGlist;
      *_hhb_tags += new TAG_val<HatchingHatchBase,double>(
         "screen_length",
         &HatchingHatchBase::pix_size_);
      *_hhb_tags += new TAG_meth<HatchingHatchBase>(
         "wpts",
         &HatchingHatchBase::put_pts,
         &HatchingHatchBase::get_pts,
         0);
      *_hhb_tags += new TAG_meth<HatchingHatchBase>(
         "norms",
         &HatchingHatchBase::put_norms,
         &HatchingHatchBase::get_norms,
         0);
      *_hhb_tags += new TAG_meth<HatchingHatchBase>(
         "offsetlist",
         &HatchingHatchBase::put_offsetlist,
         &HatchingHatchBase::get_offsetlist,
         0);
      //Legacy stuff
      *_hhb_tags += new TAG_meth<HatchingHatchBase>(
         "pressures",
         &HatchingHatchBase::put_pressures,
         &HatchingHatchBase::get_pressures,
         0);
      *_hhb_tags += new TAG_meth<HatchingHatchBase>(
         "offsets",
         &HatchingHatchBase::put_offsets,
         &HatchingHatchBase::get_offsets,
         0);

   }
   return *_hhb_tags;
}
/////////////////////////////////////
// decode()
/////////////////////////////////////
STDdstream&
HatchingHatchBase::decode(STDdstream &ds)
{
   STDdstream &rds  = DATA_ITEM::decode(ds);
   if (_level) init();
   else
   {
      err_mesg(ERR_LEV_WARN, "HatchingHatchBase::decode() - Warning!! Level is NULL, this better be a legacy file!");
   }
   return rds;
}


/////////////////////////////////////
// put_pts()
/////////////////////////////////////
void
HatchingHatchBase::put_pts(TAGformat &d) const
{
   d.id();
   *d << _pts;
   d.end_id();
}

/////////////////////////////////////
// get_pts()
/////////////////////////////////////
void
HatchingHatchBase::get_pts(TAGformat &d)
{
   *d >> _pts;
}


/////////////////////////////////////
// put_norms()
/////////////////////////////////////
void
HatchingHatchBase::put_norms(TAGformat &d) const
{
   d.id();
   *d <<  _norms;
   d.end_id();
}

/////////////////////////////////////
// get_norms()
/////////////////////////////////////
void
HatchingHatchBase::get_norms(TAGformat &d)
{
   *d >> _norms;

   assert(_pts.num() == _norms.num());

}

/////////////////////////////////////
// put_offsets()
/////////////////////////////////////
void
HatchingHatchBase::put_offsetlist(TAGformat &d) const
{
   d.id();
   _offsets->format(*d);
   d.end_id();

}

/////////////////////////////////////
// get_offsets()
/////////////////////////////////////
void
HatchingHatchBase::get_offsetlist(TAGformat &d)
{

   //Grab the class name... should be BaseStrokeOffset
   str_ptr str;
   *d >> str;      

   if ((str != BaseStrokeOffsetLIST::static_name())) {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "HatchingHatchBase::get_offsets() - Not BaseStrokeOffsetLIST: '%s'!", **str);
      return;
   }

   BaseStrokeOffsetLISTptr o = new BaseStrokeOffsetLIST;
   assert(o);
   o->decode(*d);

   _offsets = o;


}
/**** Next four tags are only around so we gracefully die on old files ***/
/////////////////////////////////////
// put_pressures()
/////////////////////////////////////
void
HatchingHatchBase::put_pressures(TAGformat &d) const
{
//Legacy
}

/////////////////////////////////////
// get_pressures()
/////////////////////////////////////
void
HatchingHatchBase::get_pressures(TAGformat &d)
{
	ARRAY<double> presses;
	*d >> presses;
   //Toss this away
}

/////////////////////////////////////
// put_offsets()
/////////////////////////////////////
void
HatchingHatchBase::put_offsets(TAGformat &d) const
{
//Legacy
}

/////////////////////////////////////
// get_offsets()
/////////////////////////////////////
void
HatchingHatchBase::get_offsets(TAGformat &d)
{
	ARRAY<NDCvec> offsets;
	*d >> offsets;
   //Toss this away
}
/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingHatchBase::HatchingHatchBase(
   HatchingLevelBase *hlb, 
   double len, 
   CWpt_list &pl,  
   const ARRAY<Wvec> &nl,
   CBaseStrokeOffsetLISTptr &ol) :
      _level(hlb)
{
   assert(_level);

   assert(pl.num() == nl.num());

   _pts.clear();   
   _pts.operator+=(pl);

   _norms.clear();   
   _norms.operator+=(nl);

   _offsets = ol;

   _pix_size = len;

   init();
}

/////////////////////////////////////
// init()
/////////////////////////////////////
//
// -Called at end of full constructor
// -Called at end of decode to finish 
//  construction
//
/////////////////////////////////////
void
HatchingHatchBase::init()
{

   assert(_level);

   assert(_pts.num() == _norms.num());
        
   _pts.update_length();

   _stroke = (HatchingStroke*)_level->group()->group()->prototype()->copy();

   assert(_stroke);

   _stroke->set_group(_level->group());

   _real_pts.clear();      
   _real_norms.clear();
   _real_good.clear();

   _visible = true;
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingHatchBase::~HatchingHatchBase()
{
        
   assert(_stroke);
   delete _stroke;
   _stroke = 0;

}


/////////////////////////////////////
// draw()
/////////////////////////////////////
int     
HatchingHatchBase::draw(CVIEWptr &v)
{

   if (_visible)
      return _stroke->draw(v);
   else 
      return 0;

}

/////////////////////////////////////
// update_prototype()
/////////////////////////////////////
void    
HatchingHatchBase::update_prototype()
{
   _stroke->copy(*_level->group()->group()->prototype());

}
/////////////////////////////////////
// draw_setup()
/////////////////////////////////////
void    
HatchingHatchBase::draw_setup()
{
   // Modify stroke width to include level's LOD modifier

   _stroke->set_width((float)_level->current_width());

   //Bail out if zero width (no need to setup drawing)
   //UNLESS we're in transition -- in which case we
   //need to setup for the sake of updating the select window
   if ((_level->current_width() == 0.0) && (!_level->in_trans())) return;

   _stroke->set_alpha(_stroke->get_alpha()*(float)_level->current_alpha_frac());

   //Then regenerate the pts
   stroke_real_setup();


   //And fill out the stroke
   stroke_pts_setup();

}


/////////////////////////////////////
// real_setup()
/////////////////////////////////////
void    
HatchingHatchBase::stroke_real_setup()
{
   //Generate the _real_pts and _real_data
   //and the _real_num 

   //These are the points that are used to
   //create stroke vertices.  For fixed groups,
   //we use a copy of the given points as
   //already set in init.  For free groups, these
   //are generated per frame by uv lookup.
   //The _real_num delimit how many points in
   //_real_* are valid, so that
   //pts_setup only looks at the valid points.

   //By default, these are inited to the given
   //set, and this is in fact sufficient for 
   //fixed hatches...

   int i;

   //XXX - Might make this always happen and
   //apply the current obj->world xform...

   if (_real_pts.num() == 0)
   {
      assert(_real_pts.num() == _real_norms.num());

      for (i=0; i<_pts.num(); i++)
      {
         _real_pts.add(_pts[i]);
         _real_norms.add(_norms[i]);
         _real_good.add(true);
      }

      _real_pix_size = _pix_size;
   }

}


/////////////////////////////////////
// stroke_pts_setup()
/////////////////////////////////////
void    
HatchingHatchBase::stroke_pts_setup()
{
   if (!_visible) return;

   //Use _real_* vars to get 2D pts for stroke

   HatchingSelectBase * select = (_level->group()->selected())?
      (_level->group()->selection()):(NULL);

   NDCZpt pt;
        
   _stroke->clear();
   
   CWtransf &inv_tran = _level->group()->patch()->inv_xform();
//   CWtransf &tran = _level->group()->patch()->xform();

   static double pix_spacing = max(Config::get_var_dbl("HATCHING_PIX_SAMPLING",5,true),0.000001);
   //Estimated pixel length of hatch
   double pix_len = _offsets->get_pix_len() * 
                        _level->group()->patch()->mesh()->pix_size()/_real_pix_size;
   //Desired samples
   double num = pix_len/pix_spacing;

   //Spacing
   double gap = max((double)_real_pts.num() / num, 1.0);

   double i=0;  int j = 0;

   int n = _real_pts.num()-1;
   while (j<n)
   {
      pt = NDCZpt(_real_pts[j],_level->group()->patch()->obj_to_ndc());
      if (select) select->update(pt);

      _stroke->add(pt, inv_tran.transpose() * _real_norms[j], _real_good[j]);
      i += gap;
      j = (int)i;
   }
   pt = NDCZpt(_real_pts[n],_level->group()->patch()->obj_to_ndc());
   if (select) select->update(pt);
   _stroke->add(pt, inv_tran.transpose() * _real_norms[n], _real_good[n]);


/*
   int n = _real_pts.num()-1;
   for (int i=0; i<n; i++) 
   {
      pt = NDCZpt(_real_pts[i],_level->group()->patch()->obj_to_ndc());
      if (select) select->update(pt);
      //XXX - inv tran...
      _stroke->add(pt, tran * _real_norms[i], _real_good[i]);
   }
*/   
}



/*****************************************************************
 * HatchingSelectBase
 *****************************************************************/

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingSelectBase::HatchingSelectBase(
   HatchingGroupBase *hgb) :
   _group(hgb)
{
   _cleared = true;

   _left_stroke = new BaseStroke();
   assert(_left_stroke);

   //Configure the stroke
   _left_stroke->set_texture(str_ptr(Config::JOT_ROOT() + SELECTION_STROKE_TEXTURE));
   _left_stroke->set_width(4);
   _left_stroke->set_alpha(1);
   _left_stroke->set_flare(1.0);
   _left_stroke->set_halo(0);
   _left_stroke->set_fade(0);
   _left_stroke->set_color( COLOR(1.0,0.5,0.01) ); 

   // Dup it 
   _right_stroke =   (HatchingStroke*)_left_stroke->copy();
   assert(_right_stroke);

   _top_stroke =     (HatchingStroke*)_left_stroke->copy();
   assert(_top_stroke);

   _bottom_stroke =   (HatchingStroke*)_left_stroke->copy();
   assert(_bottom_stroke);


}
/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingSelectBase::~HatchingSelectBase()
{
   delete _left_stroke;
   delete _right_stroke;
   delete _top_stroke;
   delete _bottom_stroke;
}


/////////////////////////////////////
// clear();
/////////////////////////////////////
void
HatchingSelectBase::clear()
{
   _cleared = true;
}


/////////////////////////////////////
// update()
/////////////////////////////////////
void
HatchingSelectBase::update(CNDCZpt &pt)
{
   if (_cleared)
      {
         _cleared = false;

         _left = pt;
         _right = pt;
         _top = pt;
         _bottom = pt;
      }
   else
      {
         if (pt[0] < _left[0]) _left = pt;
         if (pt[0] > _right[0]) _right = pt;


         if (pt[1] < _bottom[1]) _bottom = pt;
         if (pt[1] > _top[1]) _top = pt;
      }


}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int
HatchingSelectBase::draw(CVIEWptr &v)
{
   int n = 0;

   _left_stroke->clear();
   _right_stroke->clear();
   _top_stroke->clear();
   _bottom_stroke->clear();
        
   double z = (_left[2] + _right[2] + _top[2] + _bottom[2]) / 4.0;


   NDCZpt _top_left(_left[0],_top[1],z);
   NDCZpt _top_right(_right[0],_top[1],z);
   NDCZpt _bottom_left(_left[0],_bottom[1],z);
   NDCZpt _bottom_right(_right[0],_bottom[1],z);

   _left_stroke->add(_bottom_left);
   _left_stroke->add(_top_left);


   _top_stroke->add(_top_left);
   _top_stroke->add(_top_right);


   _right_stroke->add(_top_right);
   _right_stroke->add(_bottom_right);


   _bottom_stroke->add(_bottom_right);
   _bottom_stroke->add(_bottom_left);

   _left_stroke->draw_start();
   n += _left_stroke->draw(v);
   n += _top_stroke->draw(v);
   n += _right_stroke->draw(v);
   n += _bottom_stroke->draw(v);
   _left_stroke->draw_end();

   return n;
}

/*****************************************************************
 * HatchingBackboneBase
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingBackboneBase::_hbb_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
//
// -The base class handles wpts and lens
// -Subclass for fixed adds nothing
// -Subclass for free adds uv handling
//
////////////////////////////////////
CTAGlist &
HatchingBackboneBase::tags() const
{
   if (!_hbb_tags) {
      _hbb_tags = new TAGlist;
      *_hbb_tags += new TAG_meth<HatchingBackboneBase>(
         "num",
         &HatchingBackboneBase::put_num,
         &HatchingBackboneBase::get_num,
         0);
      *_hbb_tags += new TAG_val<HatchingBackboneBase,double>(
         "length",
         &HatchingBackboneBase::len_);
      *_hbb_tags += new TAG_meth<HatchingBackboneBase>(
         "wpts1",
         &HatchingBackboneBase::put_wpts1,
         &HatchingBackboneBase::get_wpts1,
         0);
      *_hbb_tags += new TAG_meth<HatchingBackboneBase>(
         "wpts2",
         &HatchingBackboneBase::put_wpts2,
         &HatchingBackboneBase::get_wpts2,
         0);
      *_hbb_tags += new TAG_meth<HatchingBackboneBase>(
         "lengths",
         &HatchingBackboneBase::put_lengths,
         &HatchingBackboneBase::get_lengths,
         0);
   }
   return *_hbb_tags;
}

/////////////////////////////////////
// put_num()
/////////////////////////////////////
void
HatchingBackboneBase::put_num(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneBase::put_num()");

   d.id();
   *d << _vertebrae.num();
   d.end_id();
}

/////////////////////////////////////
// get_num()
/////////////////////////////////////
void
HatchingBackboneBase::get_num(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneBase::get_num()");

   int num, i;

   *d >> num;

   for (i=0;i<num;i++) 
      _vertebrae.add(new Vertebrae);

}

/////////////////////////////////////
// put_wpts1()
/////////////////////////////////////
void
HatchingBackboneBase::put_wpts1(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneBase::put_wpts1()");

   int i;
   ARRAY<Wpt> pts1;

   for (i=0; i<_vertebrae.num(); i++) 
      pts1.add(_vertebrae[i]->pt1);

   d.id();
   *d << pts1;
   d.end_id();
}

/////////////////////////////////////
// get_wpts1()
/////////////////////////////////////
void
HatchingBackboneBase::get_wpts1(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneBase::get_wpts1()");

   int i;
   ARRAY<Wpt> pts1;

   *d >> pts1;

   assert(_vertebrae.num() == pts1.num());

   for (i=0; i<_vertebrae.num(); i++) 
      _vertebrae[i]->pt1 = pts1[i];

}

/////////////////////////////////////
// put_wpts2()
/////////////////////////////////////
void
HatchingBackboneBase::put_wpts2(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneBase::put_wpts2()");

   int i;
   ARRAY<Wpt> pts2;

   for (i=0; i<_vertebrae.num(); i++) 
      pts2.add(_vertebrae[i]->pt2);

   d.id();
   *d << pts2;
   d.end_id();
}

/////////////////////////////////////
// get_wpts2()
/////////////////////////////////////
void
HatchingBackboneBase::get_wpts2(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneBase::get_wpts2()");

   int i;
   ARRAY<Wpt> pts2;

   *d >> pts2;

   assert(_vertebrae.num() == pts2.num());

   for (i=0; i<_vertebrae.num(); i++) 
      _vertebrae[i]->pt2 = pts2[i];

}
/////////////////////////////////////
// put_lengths()
/////////////////////////////////////
void
HatchingBackboneBase::put_lengths(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneBase::put_lengths()");

   int i;
   ARRAY<double> lens;

   for (i=0; i<_vertebrae.num(); i++) 
      lens.add(_vertebrae[i]->len);

   d.id();
   *d << lens;
   d.end_id();
}

/////////////////////////////////////
// get_lengths()
/////////////////////////////////////
void
HatchingBackboneBase::get_lengths(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneBase::get_lengths()");

   int i;
   ARRAY<double> lens;

   *d >> lens;

   assert(_vertebrae.num() == lens.num());

   for (i=0; i<_vertebrae.num(); i++) 
      _vertebrae[i]->len = lens[i];

}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingBackboneBase::~HatchingBackboneBase() 
{
   for(int i=0;i<_vertebrae.num();i++) 
      delete _vertebrae[i];

   while (_geoms.num()>0)
      WORLD::destroy(_geoms.pop());

}

/////////////////////////////////////
// get_ratio()
/////////////////////////////////////
//
// -Uses the vertebrae list created by subclass
//  and converts segments to screen length
// -Totals the length of 'existing' segments
//  and makes ratio with original segment length
// -Some segments might not exist for free hatched
//  because uv -> world could fail
// -_use_exist dictates whether to keep running
//  total of orignal lengths, or just use the full total
// -free hatches would overide this to first get
//  wpt's (and exists) from uv before calling this 
//
//////////////////////////////////////
double
HatchingBackboneBase::get_ratio()
{
   int i;
   double original_length;

   // XXX - Support for hatch groups with only *one* hatch??
   // So far, we can't get here, as trying to compute backbone
   // will abort...

   assert(_vertebrae.num() > 0);

   if (_use_exist)
   {
      original_length = 0.0;
      for (i=0; i<_vertebrae.num(); i++)
         if (_vertebrae[i]->exist) 
            original_length += _vertebrae[i]->len;
      if(!(original_length > 0.0))
      {
         err_mesg(ERR_LEV_WARN, "HatchingBaseboneBase::get_ratio() - Entire backbone is undefined!");
         return 1.0;
      }
   }
   else
   {
      assert(_len);
      original_length = _len;
   }

   return find_len() / original_length;

}

/////////////////////////////////////
// find_len()
/////////////////////////////////////
double
HatchingBackboneBase::find_len()
{
   int i;
   double len = 0.0;
   NDCpt   n1, n2;

   static bool debug = Config::get_var_bool("HATCHING_DEBUG_BACKBONE",false,true);

   while (_geoms.num()>0)
      WORLD::destroy(_geoms.pop());

   if (_use_exist)
      {
         for (i=0;i<_vertebrae.num();i++)
            {
               if (_vertebrae[i]->exist)
                  {
                     n1 = NDCZpt( _vertebrae[i]->pt1, _patch->obj_to_ndc() );
                     n2 = NDCZpt( _vertebrae[i]->pt2, _patch->obj_to_ndc() );
                     len += (n2 - n1).length();

                     if (debug)
                        _geoms.add(WORLD::show(
                           _patch->xform() * _vertebrae[i]->pt1, 
                           _patch->xform() * _vertebrae[i]->pt2, 2.5));
                  }
            }
      }
   else
      {
         for (i=0;i<_vertebrae.num();i++)
            {
               n1 = NDCZpt( _vertebrae[i]->pt1, _patch->obj_to_ndc() );
               n2 = NDCZpt( _vertebrae[i]->pt2, _patch->obj_to_ndc() );
               len += (n2 - n1).length();
               if (debug)
                  _geoms.add(WORLD::show(
                     _patch->xform() * _vertebrae[i]->pt1, 
                     _patch->xform() * _vertebrae[i]->pt2 , 2.5));
            }
      }

   len *= VIEW::peek()->ndc2pix_scale();

   return len;
}
