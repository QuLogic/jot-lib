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
// HatchingGroupFree
////////////////////////////////////////////
//
// -'Free' Hatching Group class
// -Implements HatchingGroup abstract class
// -Holds a collection of HatchingGroupFreeInst's
//  (also defined here) which subclass HatchingGroupBase
//
////////////////////////////////////////////

#define INTERPOLATION_RANDOM_FACTOR 0.15

#include "geom/world.H"
#include "mesh/uv_data.H"
#include "mesh/lmesh.H"
#include "npr/hatching_group_free.H"
#include "std/config.H"

using namespace mlib;

/*****************************************************************
 * HatchingGroupFree
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingGroupFree::_hgfr_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingGroupFree::tags() const
{
   if (!_hgfr_tags) {
      _hgfr_tags = new TAGlist;
      *_hgfr_tags += HatchingGroup::tags();
      *_hgfr_tags += new TAG_meth<HatchingGroupFree>(
         "mapping",
         &HatchingGroupFree::put_mapping,
         &HatchingGroupFree::get_mapping,
         0);
      *_hgfr_tags += new TAG_meth<HatchingGroupFree>(
         "position",
         &HatchingGroupFree::put_position,
         &HatchingGroupFree::get_position,
         0);
      *_hgfr_tags += new TAG_meth<HatchingGroupFree>(
         "backbone",
         &HatchingGroupFree::put_backbone,
         &HatchingGroupFree::get_backbone,
         0);
      *_hgfr_tags += new TAG_meth<HatchingGroupFree>(
         "level",
         &HatchingGroupFree::put_levels,
         &HatchingGroupFree::get_level,
         1);
      *_hgfr_tags += new TAG_meth<HatchingGroupFree>(
         "hatch",
         &HatchingGroupFree::put_hatchs,
         &HatchingGroupFree::get_hatch,
         1);
   }
   return *_hgfr_tags;
}

/////////////////////////////////////
// decode()
/////////////////////////////////////
STDdstream&
HatchingGroupFree::decode(STDdstream &ds)
{
   STDdstream &rds  = DATA_ITEM::decode(ds);

   assert(_position);
   assert(_instances[0]->position());

   assert(_instances[0]->num_base_levels());

   if (_instances[0]->base_level(0)->num() > 0)
   {
      if (_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT)
         assert(_instances[0]->backbone());

      _complete = true;
   }
   //else this was loading an old file and we can't
   //get the hatches, so we fail to complete the
   //group and hatching collection will punt us!

   return rds;
}

/**** Next two tags are only around so we gracefully die on old files ***/
/////////////////////////////////////
// put_hatchs()
/////////////////////////////////////
//
// -Actually dumps a tag for each one
//
//////////////////////////////////////
void
HatchingGroupFree::put_hatchs(TAGformat&) const
{
// *** LEGACY ***
}

/////////////////////////////////////
// get_hatch()
/////////////////////////////////////
void
HatchingGroupFree::get_hatch(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "HatchingGroupFree::get_hatch() - ****** OLD FILE FORMAT - DUMPING ******");

   //Grab the class name... should be HatchingHatchFree
   str_ptr str;
   *d >> str;      

   if ((str != HatchingHatchFree::static_name())) 
   {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFree::get_hatch() - Not 'HatchingHatchFree': '%s'!!", **str);
      return;
   }

   HatchingHatchFree *hhf = new HatchingHatchFree();   assert(hhf);
   hhf->decode(*d);

   //Toss this away
}

/////////////////////////////////////
// put_levels()
/////////////////////////////////////
//
// -Actually dumps a tag for each one
//
//////////////////////////////////////
void
HatchingGroupFree::put_levels(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::put_levels()");

   int i;
   assert(_instances[0]->num_base_levels());

   for (i=0; i<_instances[0]->num_base_levels(); i++)
   {
      HatchingLevelBase *hlb = _instances[0]->base_level(i);
      assert(hlb);
      d.id();
      hlb->format(*d);
      d.end_id();
   }
}

/////////////////////////////////////
// get_level()
/////////////////////////////////////
void
HatchingGroupFree::get_level(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::get_level()");

   assert(_instances[0]->num_base_levels() > 0);

   //Grab the class name... should be HatchingLevelBase
   str_ptr str;
   *d >> str;      

   if ((str != HatchingLevelBase::static_name())) {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFree::get_level() - Not 'HatchingLevelBase': '%s'!!", **str);
      return;
   }

   if (!(_instances[0]->base_level(_instances[0]->num_base_levels()-1)->pix_size()))
   {
      assert(_instances[0]->num_base_levels() == 1);
   }
   else
   {
      _instances[0]->add_base_level();
   }

   _instances[0]->base_level(_instances[0]->num_base_levels()-1)->decode(*d);

   assert(_instances[0]->base_level(_instances[0]->num_base_levels()-1)->pix_size() != 0.0);

}

/////////////////////////////////////
// put_mapping()
/////////////////////////////////////
void
HatchingGroupFree::put_mapping(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::put_mapping()");

   assert(_mapping);
   assert(_mapping->seed_face());

   d.id();
   *d << _mapping->seed_face()->index();
   d.end_id();
}

/////////////////////////////////////
// get_mapping()
/////////////////////////////////////
void
HatchingGroupFree::get_mapping(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::get_mapping()");

   Bface *f;
   UVdata *uvdata;
   int index;

   BMESH *mesh = _patch->mesh();
   if (LMESH::isa(mesh)) mesh = ((LMESH*)mesh)->cur_mesh();
   CBface_list& faces = mesh->faces();

   *d >> index;

   f = faces[index];

   uvdata = UVdata::lookup(f);
   if (uvdata)
      {
         if (uvdata->mapping())
         {
            err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::get_mapping()- The uv region is already mapped.");
            _mapping = uvdata->mapping();
            _mapping->register_usage();
         }
         else
         {
            err_mesg(ERR_LEV_INFO, "HatchingGroupFree::get_mapping() - The uv region is NOT mapped.  Generating new map...");
            _mapping = new UVMapping(f);
            assert(_mapping);
            _mapping->register_usage();
         }
      }
   else
   {
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFree::get_mapping() - NO UV DATA ON UV MAPPING REGION SEED FACE!!!!!");
   }

}

/////////////////////////////////////
// put_backbone()
/////////////////////////////////////
void
HatchingGroupFree::put_backbone(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::put_backbone()");

   if (_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT)
      assert(_instances[0]->backbone());
   else
      return;

   d.id();
   _instances[0]->backbone()->format(*d);
   d.end_id();

}

/////////////////////////////////////
// get_backbone()
/////////////////////////////////////
void
HatchingGroupFree::get_backbone(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::get_backbone()");

   assert(_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT);

   //Grab the class name... should be HatchingBackboneFree
   str_ptr str;
   *d >> str;      

   if ((str != HatchingBackboneFree::static_name())) 
   {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFree::get_backbone() - Not 'HatchingBackboneFree': '%s'!!", **str);
      return;
   }

   _instances[0]->set_backbone(new HatchingBackboneFree(_patch,_instances[0]));
   assert(_instances[0]->backbone());
   _instances[0]->backbone()->decode(*d);

}

/////////////////////////////////////
// put_position()
/////////////////////////////////////
void
HatchingGroupFree::put_position(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::put_position()");

   assert(_position);

   d.id();
   _position->format(*d);
   d.end_id();
}

/////////////////////////////////////
// get_position()
/////////////////////////////////////
void
HatchingGroupFree::get_position(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::get_position()");

   //Grab the class name... should be HatchingPositionFree
   str_ptr str;
   *d >> str;      

   if ((str != HatchingPositionFree::static_name())) {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFree::get_position() - Not 'HatchingPositionFree': '%s'!!", **str);
      return;
   }

   assert(_mapping);

   _position = new HatchingPositionFree(this,_mapping);
   assert(_position);
   _position->decode(*d);

   _instances[0]->set_position(new HatchingPositionFreeInst(_mapping,_position));
   assert(_instances[0]->position());
}


/////////////////////////////////////
// Constructor
/////////////////////////////////////

HatchingGroupFree::HatchingGroupFree(Patch *p)  : 
   HatchingGroup(p),
   _mapping(0),
   _selected(false),
   _position(0)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::HatchingGroupFree()");
   //Add the base instance
   _instances.clear();
   _instances.add(new HatchingGroupFreeInst(this));
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

HatchingGroupFree::~HatchingGroupFree()
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::~HatchingGroupFree()");

   int k;
        
   if (_complete)
   {
      assert(_position);
      delete _position;
      _position = NULL;
   }

   if (_mapping)
   {
      _mapping->unregister_usage();
      _mapping = NULL;
   }

   for (k=0;k<_instances.num();k++)  delete _instances[k];
   _instances.clear();

}

/////////////////////////////////////
// select()
/////////////////////////////////////
void    
HatchingGroupFree::select()
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::select()");

   _selected = true;

   int k;

   for (k=0; k<_instances.num(); k++)
      _instances[k]->select();

}
        
/////////////////////////////////////
// deselect()
/////////////////////////////////////
void
HatchingGroupFree::deselect()
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::deselect()");

   _selected = false;

   int k;

   for (k=0; k<_instances.num(); k++)
      _instances[k]->deselect();

}
/////////////////////////////////////
// level_sorting_comparison
/////////////////////////////////////
extern int compare_pix_size(const void *a, const void *b) ;

/////////////////////////////////////
// complete()
/////////////////////////////////////
bool
HatchingGroupFree::complete()
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::complete()");

   assert(!_complete);

   HatchingPositionFree       *hpfr;
   HatchingPositionFreeInst   *hpfri;
   HatchingBackboneFree       *hbfr;

   if (_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT)
   {
      assert(_instances[0]->num_base_levels() == 1);
      assert(_instances[0]->base_level(0)->pix_size() == 0.0);

      if (_instances[0]->base_level(0)->num() < 2)
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree::complete() - Not enough hatches (<2) to complete group.");
         return false;
      }
      else
      {
         err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::complete() - Completing group.");

         hpfr = new HatchingPositionFree(this,_mapping);          assert(hpfr);
         hpfri = new HatchingPositionFreeInst(_mapping,hpfr);     assert(hpfri);
         hbfr = new HatchingBackboneFree(_patch,_instances[0]);   assert(hbfr);

         //If we cannot compute any of these, we abort completion...
         if (hbfr->compute(_instances[0]->base_level(0)) 
             && hpfr->compute(_instances[0]))
         {
            _instances[0]->base_level(0)->set_pix_size(_patch->mesh()->pix_size());

            _position = hpfr;               
            _instances[0]->set_position(hpfri);             
            _instances[0]->set_backbone(hbfr);

            _complete = true;
            return true;
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::complete() - Failed to complete group.");
            delete hpfr;
            delete hpfri;
            delete hbfr;
            return false;
         }
      }
   }
   else //The two sloppy types
   {
      assert(_instances[0]->num_base_levels() > 0);
      
      if (_instances[0]->base_level(_instances[0]->num_base_levels()-1)->pix_size() > 0.0)
      {
         // If the last editted level is complete, complete the group
         err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::complete() - Completing group.");

         hpfr = new HatchingPositionFree(this,_mapping);          assert(hpfr);
         hpfri = new HatchingPositionFreeInst(_mapping,hpfr);     assert(hpfri);

         //If we cannot compute this, we abort completion...
         //XXX - Change to using all base levels
         if (hpfr->compute(_instances[0]))
         {
            _position = hpfr;               
            _instances[0]->set_position(hpfri);             

            _complete = true;
            return true;
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::complete() - Failed to complete group.");
            delete hpfr;
            delete hpfri;
            return false;
         }         
         
         //Sort levels by increasing mesh size
         _instances[0]->level_sort(compare_pix_size);

         _complete = true;
         return true;
      }
      else
      {
         // Else try to complete the last editted level
         if (_instances[0]->base_level(_instances[0]->num_base_levels()-1)->num() >= 1)
         {
            // XXX - Recompute ndc_length of each hatch?
            
            _instances[0]->base_level(_instances[0]->num_base_levels()-1)->
                              set_pix_size(_patch->mesh()->pix_size());
            return true;
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::complete() - Not enough hatches (<1) to complete level.");
            return false;
         }

      }
   }
}

/////////////////////////////////////
// undo_last()
/////////////////////////////////////
// Failure to undo means we ran
// out of things to undo. That
// is, there a single hatch left.
// The pen should notice this
// failure, and call delete 
/////////////////////////////////////
bool
HatchingGroupFree::undo_last()
{

   assert(!_complete);

   if (_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT)
   {
      assert(_instances[0]->num_base_levels() == 1);
      assert(_instances[0]->base_level(0)->pix_size() == 0.0);

      if (_instances[0]->base_level(0)->num() < 2)
      {
         assert(_instances[0]->base_level(0)->num() > 0);
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree::undo_last() - Only 1 hatch left... can't undo.");
         //Pen should notice this failure and delete the group...
         return false;
      }
      else
      {
         err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::undo_last() - Popping off hatch.");
         WORLD::message("Popped hatch stroke.");
         HatchingHatchBase *hhb = _instances[0]->base_level(0)->pop();
         assert(hhb);
         delete(hhb);
         return true;
      }
   }
   else //The two sloppy types
   {
      assert(_instances[0]->num_base_levels() > 0);
      
      if (_instances[0]->base_level(_instances[0]->num_base_levels()-1)->pix_size() > 0.0)
      {
         // If the last editted level is complete, un-complete it
         err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::undo_last() - Uncompleting level.");
         WORLD::message("Un-completed level.");
         _instances[0]->base_level(_instances[0]->num_base_levels()-1)->set_pix_size(0.0);
         return true;
      }
      else
      {
         if (_instances[0]->base_level(_instances[0]->num_base_levels()-1)->num() > 1)
         {
            err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::undo_last() - Popping off hatch.");
            WORLD::message("Popped hatch stroke.");
            HatchingHatchBase *hhb = _instances[0]->base_level(_instances[0]->num_base_levels()-1)->pop();
            assert(hhb);
            delete(hhb);
            return true;
         }
         else 
         {
            //If there 1 hatch, pop it and the level 
            //if this isn't the bottom level. Else
            //return failure so the pen deletes the
            //remainder.
            assert(_instances[0]->base_level(_instances[0]->num_base_levels()-1)->num() != 0);
            
            if (_instances[0]->num_base_levels() > 1)
            {
               err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::undo_last() - Popping off hatch and level.");
               WORLD::message("Popped hatch level of detail.");
               HatchingHatchBase *hhb = _instances[0]->base_level(_instances[0]->num_base_levels()-1)->pop();
               assert(hhb);
               delete(hhb);
               HatchingLevelBase *hlb = _instances[0]->pop_base_level();
               assert(hlb);
               delete(hlb);
               return true;
            }
            else
            {
               err_mesg(ERR_LEV_WARN, "HatchingGroupFree::undo_last() - Only 1 hatch left... can't undo.");
               //Pen should notice this failure and delete the group...
               return false;
            }
         }
      }
   }
}
/////////////////////////////////////
// draw()
/////////////////////////////////////

int
HatchingGroupFree::draw(CVIEWptr &v)
{
   int num=0, k;

   static int debug_mapping = Config::get_var_bool("HATCHING_DEBUG_MAPPING",false,true)?true:false;

   if (VIEW::stamp() != _stamp)
   {
      if (debug_mapping)
         if (_mapping && _selected)
            _mapping->clear_debug_image();
        
      //Update instances
      draw_setup();

      //Update levels
      for (k=0; k<_instances.num(); k++)
         _instances[k]->draw_setup();

      //Advance level animations, etc.
      for (k=0; k<_instances.num(); k++)
         _instances[k]->level_draw_setup();
        
      //Causes hatches to update 
      for (k=0; k<_instances.num(); k++)
         _instances[k]->hatch_draw_setup();

      _stamp = VIEW::stamp();
   }

   //Draw all the hatches
   _prototype.draw_start();
   for (k=0; k<_instances.num(); k++)
      num += _instances[k]->draw(v);
   _prototype.draw_end();

   //XXX - Could query _selected here
   for (k=0; k<_instances.num(); k++)
      num += _instances[k]->draw_select(v);

   if (debug_mapping)
      if (_mapping && _selected)
         _mapping->draw_debug();

   return num;
}

/////////////////////////////////////
// draw_setup()
/////////////////////////////////////
void
HatchingGroupFree::draw_setup()
{
   //Figure out where to stick instances
   //Instantiate them and set their transforms
   //Handle instance fading stuff

   if (!_complete) return;

        //XXX - Like this?
   _position->update();

}


/////////////////////////////////////
// update_prototype()
/////////////////////////////////////
void    
HatchingGroupFree::update_prototype()
{
   HatchingGroup::update_prototype();

   for (int k=0; k<_instances.num(); k++)
      _instances[k]->update_prototype();

}

/////////////////////////////////////
// query_pick()
/////////////////////////////////////
bool
HatchingGroupFree::query_pick(CNDCpt &pt)
{
   int k;

   for (k=0; k<_instances.num(); k++)
      if (_instances[k]->query_pick(pt))
         return true;
        
   return false;
}

/////////////////////////////////////
// add()
/////////////////////////////////////
bool
HatchingGroupFree::add(CNDCpt_list &pl, const ARRAY<double>&prl, int curve_type)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add()");

   int k;
   double a,b;
   Bface *f;
   UVMapping *m;

   if (pl.num() != prl.num())
   {
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFree::add() - Gesture pixel list and pressure list are not same length.");
      return false;
   }

   //Smooth the input gesture
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Smoothing gesture.");
   NDCpt_list        smoothpts;
   ARRAY<double>     smoothprl;
   HatchingGroupBase::smooth_gesture(pl,smoothpts,prl,smoothprl,_params.anim_style());
   smoothpts.update_length();

   //If _mapping=0, the first hatch defines the uv region 
   //else, we just check that stroke falls in right mapping
   m = find_uv_mapping(smoothpts);
   if (!_mapping)
   {
      if (m)
      {
         err_mesg(ERR_LEV_INFO, "HatchingGroupFree::add() - No associated uv mapping yet, so we use the one we hit.");
         _mapping = m;
         _mapping->register_usage();
      }
      else
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - No associated uv mapping yet, and we missed a uv region, so punt this stroke.");
         return false;
      }
   }
   else
   {
      if (_mapping == m)
      {
         err_mesg(ERR_LEV_INFO, "HatchingGroupFree::add() - Stroke lands in right uv mapping. Good.");
      }
      else
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Stroke lands in WRONG/NO uv mapping. Punting stroke...");
         return false;
      }
   }

   //Clip gesture to the uv region
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Clipping gesture to uv region.");
   NDCpt_list     ndcpts;
   ARRAY<double>  finalprl;
   clip_to_uv_region(smoothpts,ndcpts,smoothprl,finalprl);
   ndcpts.update_length();
   assert(ndcpts.num()>0);

   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Checking gesture silliness.");

   if (HatchingGroupBase::is_gesture_silly(ndcpts,_params.anim_style()))
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Punting silly gesture...");
      return false;
   }

   //Even if the user wants to project to create the
   //hatch, we continue with plane cutting to
   //generate a curve we can use to estimate
   //the mesh spacing so that the final projected
   //hatch is sampled evenly on the level of the mesh
   //spacing

   //Use the result to define a cutting plane
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Fitting line.");
   if (!HatchingGroupBase::fit_line(ndcpts,a,b)) return false;

   //Slide to midpoint if desired
   if (Config::get_var_bool("HATCHING_GROUP_SLIDE_FIT",false,true)) 
      b = ndcpts.interpolate(0.5)[1] - (a*ndcpts.interpolate(0.5)[0]);

   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Computing plane.");
   //Find the cutting plane
   Wplane wpPlane;
   f = HatchingGroupBase::compute_cutting_plane(_patch,a, b, ndcpts, wpPlane);
   if (!f) return false;
   else
   {
      if (!f->front_facing())
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Nearest pt. on fit line hit backfacing surface.");     
         return false;
      }
      UVdata *uvdata = UVdata::lookup(f);
      if (!uvdata)
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Nearest pt. on interp. line misses uv region.");     
         return false;
      }
      else if (uvdata->mapping() != _mapping)
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Nearest pt. on interp. line misses correct uv region.");     
         return false;
      }
   }

   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Slicing mesh.");     
   //Intersect the mesh to get a 3D curve
   Wpt_list wlList;
   slice_mesh_with_plane(f,wpPlane,wlList);

   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Cliping curve to gesture."); 
   //Clip end of 3D curve to match gesture
   Wpt_list wlClipList;
   HatchingGroupBase::clip_curve_to_stroke(_patch,ndcpts, wlList, wlClipList);
   wlClipList.update_length();

   Wpt_list wlScaledList;

   if (curve_type == HatchingGroup::CURVE_MODE_PROJECT)
   {
      err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Projecting to surface."); 
      
      //Okay, the user wants to get literal, projected
      //points, so lets do it.  We're careful to
      //toss points that hit the no/wrong mesh, no/wrong uv region.
      Wpt_list wlProjList;
      Wpt wloc;
      UVdata *uvdata;

      for (k=0; k<ndcpts.num(); k++)
      {
         f = HatchingGroupBase::find_face_vis(NDCpt(ndcpts[k]),wloc);
         if ((f) && (f->patch() == _patch) && (f->front_facing()) &&
             (uvdata = UVdata::lookup(f)) && (uvdata->mapping() == _mapping))
         {
            wlProjList += wloc;
         }
         else 
         {
            if (!f) 
               err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed while projecting: No hit on a mesh!");
            else if (!(f->patch() == _patch)) 
               err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed while projecting: Hit wrong patch.");
            else if (!f->front_facing())
               err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed while projecting: Hit backfacing tri.");
            else if (!(uvdata = UVdata::lookup(f))) 
               err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed while projecting: Hit region w/o uv.");
            else if (!(uvdata->mapping() == _mapping)) 
               err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed while projecting: Wrong uv region (or unmapped.)");
            else 
               err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed while projecting: WHAT?!?!?!?!");
         }
      }
      if (wlProjList.num()<2)
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree:add() - Nothing left after projection failures. Punting...");
         return false;
      }
      wlProjList.update_length();

      err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Resampling curve."); 
      //Resample to even spacing in world space. Sample
      //at a world distance similar to wlClipList, which
      //will be on the order of the mesh resolution
      //unless the gesture fits into one triangle,
      //in which case we ensure a minimum sampling
      int guess = (int)ceil(((double)wlClipList.num()*
                             (double)wlProjList.length())/(double)wlClipList.length());
      int num = max(guess,5);
      double step = 1.0/((double)(num-1));
      for (k=0 ; k<num ; k++) wlScaledList += wlProjList.interpolate((double)k*step);
   }
   else //CURVE_MODE_PLANE
   {
      assert(curve_type == HatchingGroup::CURVE_MODE_PLANE);

      err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Resampling curve."); 
      //Resample to even spacing in world space. This curve will
      //be sampled on the order of the mesh spacing but we'll
      //not allow the num of samples to drop too low in case
      //the gesture's on the scale of one triangle
      int num = max(wlClipList.num(),5);
      double step = 1.0/((double)(num-1));
      for (k=0 ; k<num ; k++) wlScaledList += wlClipList.interpolate((double)k*step);
   }

   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Converting to 2D."); 

   NDCZpt_list ndczlScaledList;
   for (k=0;k<wlScaledList.num();k++) ndczlScaledList += NDCZpt(_patch->xform()*wlScaledList[k]);
   ndczlScaledList.update_length();

   // Calculate pixel length of hatch
   double pix_len = ndczlScaledList.length() * VIEW::peek()->ndc2pix_scale();
  
   if (pix_len < 8.0)
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Stroke only %f pixels. Probably an accident. Punting...", pix_len);
      return false;
   }

   UVpt_list            uvs;
   Wpt_list             pts;
   ARRAY<Wvec>          norms;

   err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::add() - Final sampling."); 
   for (k=0; k<ndczlScaledList.num(); k++) {

      Wpt wloc;
      UVdata *uvdata;
      f = HatchingGroupBase::find_face_vis(NDCpt(ndczlScaledList[k]),wloc);

      if ((f) && (f->patch() == _patch) && (f->front_facing()) && 
          (uvdata = UVdata::lookup(f)) && (uvdata->mapping() == _mapping))
      {
         Wvec bc;
         Wvec norm;
         UVpt uv;

         //f->project_barycentric(wloc,bc);
         f->project_barycentric_ndc(NDCpt(ndczlScaledList[k]),bc);

         Wvec bc_old = bc;
         Bsimplex::clamp_barycentric(bc);
         double dL = fabs(bc.length() - bc_old.length());

         if (bc != bc_old)
         {
            err_mesg(ERR_LEV_INFO, 
               "HatchingGroupFree::add() - Baycentric clamp modified result: (%f,%f,%f) --> (%f,%f,%f) Length Change: %f", 
                  bc_old[0], bc_old[1], bc_old[2], bc[0], bc[1], bc[2], dL);
         }
         if (dL < 1e-3)
         {
            //uv
            uvdata->bc2uv(bc,uv);
            //norm
            f->bc2norm_blend(bc,norm);

            uvs += uv;
            pts += wloc;
            norms += norm;
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Change too large due to error in projection. Dumping point...");
         }
      }
      else 
      {
         if (!f) 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed in final lookup: No hit on a mesh!");
         else if (!(f->patch() == _patch)) 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed in final lookup: Hit wrong patch.");
         else if (!(f->front_facing())) 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed in final lookup: Hit backfracing tri.");
         else if (!(uvdata = UVdata::lookup(f))) 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed in final lookup: Hit region w/o uv.");
         else if (!(uvdata->mapping() == _mapping)) 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed in final lookup: Wrong uv region (or unmapped.)");
         else 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFree::add() - Missed in final lookup: WHAT?!?!?!?!");
      }
   }

   if (uvs.num()>1)
   {
      //XXX  - Okay, using the gesture pressure, but no offsets.
      //Need to go back and add offset generation...

      BaseStrokeOffsetLISTptr ol = new BaseStrokeOffsetLIST;

      ol->set_replicate(0);
      ol->set_hangover(1);
      ol->set_pix_len(pix_len);

      ol->add(BaseStrokeOffset( 0.0, 0.0, 
            finalprl[0],                  BaseStrokeOffset::OFFSET_TYPE_BEGIN));
      for (k=1; k< finalprl.num(); k++)
      ol->add(BaseStrokeOffset( (double)k/(double)(finalprl.num()-1), 0.0, 
            finalprl[k],                  BaseStrokeOffset::OFFSET_TYPE_MIDDLE));
      ol->add(BaseStrokeOffset( 1.0, 0.0, 
            finalprl[finalprl.num()-1],   BaseStrokeOffset::OFFSET_TYPE_END));


      if (_instances[0]->base_level(_instances[0]->num_base_levels()-1)->pix_size() > 0)
      {
         assert(_params.anim_style() != HatchingGroup::STYLE_MODE_NEAT);

         _instances[0]->add_base_level();

         // Make sure we can see it while we're editing!
         _instances[0]->base_level(_instances[0]->num_base_levels()-1)->set_desired_frac(1.0);

      }

      _instances[0]->base_level(_instances[0]->num_base_levels()-1)->add_hatch(
         new HatchingHatchFree( 
            _instances[0]->base_level(_instances[0]->num_base_levels()-1),
                  _patch->mesh()->pix_size(),uvs,pts,norms,ol) );
      return true;
   }
   else
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFree:add() - All lookups are bad. Punting...");
      return false;
   }

   return true; 
}

/////////////////////////////////////
// clip_to_uv_region()
/////////////////////////////////////
void    
HatchingGroupFree::clip_to_uv_region(
   CNDCpt_list &pts, 
   NDCpt_list &cpts,
   CARRAY<double>&prl, 
   ARRAY<double>&cprl ) 
{
   int k;
   Bface *f;
   UVdata *uvdata;
   Wpt foo;

   int buf=0;     //1      //2
   NDCpt_list     tmppts1, tmppts2;
   ARRAY<double>  tmpprl1, tmpprl2;


   // Will find the longest contiguous piece that
   // falls in the right uv region. 

   for (k=0; k<pts.num(); k++)
   {
      f = HatchingGroupBase::find_face_vis(pts[k], foo);
      if (f)
      {
         uvdata = UVdata::lookup(f);
         if ((uvdata)&&(uvdata->mapping()==_mapping))
         {
            if (buf == 0) buf = 1;

            if (buf == 1)
            {
               tmppts1 += pts[k];
               tmpprl1 += prl[k];
            }
            else //buf == 2
            {
               tmppts2 += pts[k];
               tmpprl2 += prl[k];
            }
         }
      }
      else
      {
         //if (buf == 0) do nothing

         if (buf == 1) buf = 2;

         else if (buf == 2)
         {
            if (tmppts2.num()>0)
            {
               if (tmppts2.num() > tmppts1.num())
               {
                  tmppts1.clear();
                  tmpprl1.clear();
                  tmppts1.operator+=(tmppts2);
                  tmpprl1.operator+=(tmpprl2);

                  err_mesg(ERR_LEV_INFO, 
                              "HatchingGroupFree::clip_to_uv_region() - Keeping longer of %d and %d vertex regions (2nd).", 
                                    tmppts1.num(), tmppts2.num());
               }
               else
               {
                  err_mesg(ERR_LEV_INFO, 
                              "HatchingGroupFree::clip_to_uv_region() - Keeping longer of %d and %d vertex regions (1st).", 
                                    tmppts1.num(), tmppts2.num());
               }
               tmppts2.clear();
               tmpprl2.clear();
            }
         }
      }
   }
   cpts.operator+=(tmppts1);
   cprl += tmpprl1;
}

/////////////////////////////////////
// slice_mesh_with_plane()
/////////////////////////////////////
void
HatchingGroupFree::slice_mesh_with_plane(
   Bface *f, 
   CWplane &wpPlane, 
   Wpt_list &wlList)
{
   Bface *nxtFace1, *nxtFace2;
   Bedge *nxtEdge1, *nxtEdge2;

   assert(f->front_facing());

   nxtFace1 = f->plane_walk(0,wpPlane,nxtEdge1);
   assert(nxtEdge1);
   nxtFace2 = f->plane_walk(nxtEdge1,wpPlane,nxtEdge2);
   assert(nxtEdge2);

   Wpt_list wlList1;
   Wpt_list wlList2;

   wlList1 += wpPlane.intersect(nxtEdge1->line());
   wlList2 += wpPlane.intersect(nxtEdge2->line());

   //So theres at least 2 pts in the final list...

   UVdata *uvdata;

   while (nxtFace1 && (nxtFace1->front_facing()) &&
          (uvdata = UVdata::lookup(nxtFace1)) && (uvdata->mapping() == _mapping) ) {
      nxtFace1 = nxtFace1->plane_walk(nxtEdge1,wpPlane,nxtEdge1);
      if (nxtEdge1) {
         wlList1 += wpPlane.intersect(nxtEdge1->line());
      }
      else {
         assert(!nxtFace1);
      }
   }

   while (nxtFace2 && (nxtFace2->front_facing()) &&
          (uvdata = UVdata::lookup(nxtFace2)) && (uvdata->mapping() == _mapping) ) {
      nxtFace2 = nxtFace2->plane_walk(nxtEdge2,wpPlane,nxtEdge2);
      if (nxtEdge2) {
         wlList2 += wpPlane.intersect(nxtEdge2->line());
      }
      else {
         assert(!nxtFace2);
      }
   }

   int k;
   for (k = wlList1.num()-1; k >= 0; k--) {
      wlList += wlList1[k];
   }
   //wlList += wIsect;
   for (k=0; k < wlList2.num(); k++) {
      wlList += wlList2[k];
   }


}

/////////////////////////////////////
// kill_animation()
/////////////////////////////////////
void
HatchingGroupFree::kill_animation()
{
   int k;

   for (k=0; k<_instances.num(); k++)
      _instances[k]->kill_animation();

}

/////////////////////////////////////
// find_uv_mapping()
/////////////////////////////////////
UVMapping*
HatchingGroupFree::find_uv_mapping(CNDCpt_list &pl)
{
   NDCpt mid = pl.interpolate(0.5);
   Wpt foo;

   Bface *f;
   UVdata *uvdata;

   f = HatchingGroupBase::find_face_vis(mid,foo);
   if (f)
   {
      uvdata = UVdata::lookup(f);
      if (uvdata)
      {
         err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::find_uv_mapping() - Found a uv region.");
         if (uvdata->mapping())
         {
            err_mesg(ERR_LEV_SPAM, "HatchingGroupFree::find_uv_mapping() - The uv region is already mapped.");
            return uvdata->mapping();
         }
         else
         {
            err_mesg(ERR_LEV_INFO, "HatchingGroupFree::find_uv_mapping() - The uv region is NOT mapped.  Generating new map...");
            return new UVMapping(f);
         }
      }
      else
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFree::find_uv_mapping() - Stroke mid. pt. missed uv region!");
      }
   }
   else
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFree::find_uv_mapping() - Stroke mid. pt. missed mesh!!");
   }

   return 0;
}


////////////////////////////////////////////
// HatchingGroupFreeInst
////////////////////////////////////////////
//
// -'Free' Hatching Group Instance
// -Multiple instances held by Free Hatching Groups
// -Similar to a fixed hatching group except that 
//  support for uv and scaling
// -Derived from HatchingGroupBase which provides
//  common hatching functionality
//
////////////////////////////////////////////


/*****************************************************************
 * HatchingGroupFreeInst
 *****************************************************************/

/////////////////////////////////////
// Constructor
/////////////////////////////////////

HatchingGroupFreeInst::HatchingGroupFreeInst(HatchingGroup *hg) :
   HatchingGroupBase(hg),
   _position(0)
{


}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

HatchingGroupFreeInst::~HatchingGroupFreeInst()
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFreeInst::~HatchingGroupFreeInst()");

   if (is_complete()) 
   {
      assert(_position);
      delete(_position);
      _position = NULL;

      if (_backbone)
      {
         delete(_backbone);
         _backbone = NULL;
      }
   }
}

/////////////////////////////////////
// clone()
/////////////////////////////////////
HatchingGroupFreeInst*
HatchingGroupFreeInst::clone()
{
   int l, h;

   //Assume we'll only do this when the group's complete...
   assert(_group->is_complete());
   assert(base_level(0)->num() > 0);
        
   HatchingGroupFreeInst *hgfri;

   hgfri = new HatchingGroupFreeInst(_group);
   assert(hgfri);

   HatchingHatchFree *hhf;

   for (l=0; l<num_base_levels(); l++)
   {
      if (l != 0) hgfri->add_base_level();

      for (h=0; h<base_level(l)->num(); h++)
      {
         hhf= new HatchingHatchFree(hgfri->base_level(l),
                                    (HatchingHatchFree*)(*base_level(l))[h]);
         assert(hhf);
         hgfri->base_level(l)->add_hatch(hhf);
      }
   }

   HatchingPositionFreeInst  *hpfri;
   HatchingBackboneFree  *hbfr;

   //The base instance's version
   hpfri = new HatchingPositionFreeInst(
      ((HatchingGroupFree*)_group)->mapping(),
      ((HatchingGroupFree*)_group)->position());
   assert(hpfri);
   hgfri->set_position(hpfri);

   if (_backbone)
   {
      assert(_group->get_params()->anim_style() == HatchingGroup::STYLE_MODE_NEAT);
      //The base instance's backbone
      hbfr = new HatchingBackboneFree(_group->patch(),hgfri, (HatchingBackboneFree*)_backbone);
      assert(hbfr);
      hgfri->set_backbone(hbfr);
   }

   if (_selected) hgfri->select();

   return hgfri;

   /*

   //Assume we'll only do this when the group's complete...
   assert(_group->is_complete());
   assert(base_level()->num() > 0);
        
   HatchingGroupFreeInst *hgfri;

   hgfri = new HatchingGroupFreeInst(_group);
   assert(hgfri);

   HatchingHatchFree *hhf;
   for (int k=0; k<base_level()->num(); k++)
   {
      hhf= new HatchingHatchFree(hgfri->base_level(),
                                 (HatchingHatchFree*)(*base_level())[k]);
      assert(hhf);
      hgfri->base_level()->add_hatch(hhf);
   }

   HatchingPositionFreeInst  *hpfri;
   HatchingBackboneFree  *hbfr;

   //The base instance's version
   hpfri = new HatchingPositionFreeInst(
      ((HatchingGroupFree*)_group)->mapping(),
      ((HatchingGroupFree*)_group)->position());
   assert(hpfri);

   //The base instance's backbone
   hbfr = new HatchingBackboneFree(_group->patch(),hgfri, (HatchingBackboneFree*)_backbone);
   assert(hbfr);

   hgfri->set_position(hpfri);             
   hgfri->set_backbone(hbfr);              

   if (_selected) hgfri->select();

   return hgfri;
   */
}

/////////////////////////////////////
// query_pick()
/////////////////////////////////////
bool
HatchingGroupFreeInst::query_pick(CNDCpt &pt)
{
   if (_position && (_position->curr_weight() == 0)) return false;

   Wpt foo;
   Bface *f;
   UVdata *uvdata;

   if (!is_complete()) return false;

   assert(((HatchingGroupFree*)_group)->mapping());

   f = find_face_vis(pt,foo);

   if (f)
   {
      uvdata = UVdata::lookup(f);
      if (uvdata)
      {
         if (uvdata->mapping() == ((HatchingGroupFree*)_group)->mapping())
         {
            Wvec bc;
            UVpt uv;

            f->project_barycentric_ndc(pt,bc);
                       //XXX - Some barycentric clamping?
            uvdata->bc2uv(bc,uv);

            assert(_position);

            return _position->query(uv);
         }
      }
   }

   return false;

}

/////////////////////////////////////
// SimplexFilterFree
/////////////////////////////////////
//
// -A simplex filter that checks vis
// for fixed hatch group vertices.
//
/////////////////////////////////////
class SimplexFilterFree : public SimplexFilter {
 private:
   HatchingGroupFreeInst *    _hgfri;
   CHatchingVertexData *      _hsvd;
   NDCpt                      _pt;
 public:
   SimplexFilterFree(HatchingGroupFreeInst *hgfri, 
                     CHatchingVertexData *hsvd, CNDCpt &pt) : 
      _hgfri(hgfri), _hsvd(hsvd), _pt(pt) {}
        
   virtual bool accept(CBsimplex* s) const 
      { 
         if (!is_face(s)) return false;

         UVdata *uvdata = UVdata::lookup((Bface *)s);

         if (uvdata)
         {
            if (uvdata->mapping() == ((HatchingGroupFree *)_hgfri->group())->mapping())
            {
               Wvec bc;
               UVpt uv;

                          //Note, _pt may not fall in the face,
                          //and so the bc's may not all lie in (0,1)
                          //but that should be cool...
               ((Bface *)s)->project_barycentric_ndc(_pt,bc);
               uvdata->bc2uv(bc,uv);
               //Bsimplex::clamp_barycentric(bc);

               assert(_hsvd);

               return _hgfri->position()->query(uv,_hsvd->_uv);
            }
         }
         return false;
      }
};


/////////////////////////////////////
// query_visibility()
/////////////////////////////////////
bool
HatchingGroupFreeInst::query_visibility(CNDCpt &pt, CHatchingVertexData *hsvd)
{
   SimplexFilterFree filt(this,hsvd,pt);

   return query_filter_id(pt,filt);
}


/////////////////////////////////////
// interpolate()
/////////////////////////////////////
HatchingHatchBase*      
HatchingGroupFreeInst::interpolate(
   int lev,
   HatchingLevelBase *hlb,
   HatchingHatchBase *hhb1, 
   HatchingHatchBase *hhb2)
{
   int k;
   int seg1, seg2;
   double frac1, frac2;
   double ifrac;
   int num;
   double dlen;

   //Random seed issues. 
   //To make life easy, we insure each instance
   //uses the same random coefficients... To do this,
   //we seed the generator, and extract lev values...

   srand48(_group->random_seed());
   for (k=0; k< lev; k++) drand48();

   HatchingHatchFree *             h1 = (HatchingHatchFree*)hhb1;
   HatchingHatchFree *             h2 = (HatchingHatchFree*)hhb2;

   CUVpt_list                    &uvl1 = h1->get_uvs();
   CUVpt_list                    &uvl2 = h2->get_uvs();
   CWpt_list                     &ptl1 = h1->get_pts();
   CWpt_list                     &ptl2 = h2->get_pts();
   const ARRAY<Wvec>             &nl1  = h1->get_norms();
   const ARRAY<Wvec>             &nl2  = h2->get_norms();
   const BaseStrokeOffsetLISTptr &ol1  = h1->get_offsets();
   const BaseStrokeOffsetLISTptr &ol2  = h2->get_offsets();

   //XXX - Interpolated hatch will have num vertices
   //equal to greater of two interpolating hatches?

   num = max(uvl1.num(),uvl2.num());
   dlen = 1.0/((double)num-1.0);

   UVpt_list   uvpts;
   Wpt_list    pts;
   ARRAY<Wvec> norms;
   BaseStrokeOffsetLISTptr offsets = new BaseStrokeOffsetLIST;

   UVpt uv;
   UVpt uv1;
   UVpt uv2;

   UVMapping *m = ((HatchingGroupFree*)_group)->mapping();
   assert(m);

   ifrac = 0.5 + (drand48() - 0.5) * INTERPOLATION_RANDOM_FACTOR;

   for (k=0; k<num; k++) {
           
      pts +=
         ptl1.interpolate((double) k*dlen, 0, &seg1, &frac1)*ifrac +
         ptl2.interpolate((double) k*dlen, 0, &seg2, &frac2)*(1.0-ifrac);

      m->interpolate( uvl1[seg1], (1.0-frac1), uvl1[seg1+1], frac1, uv1 );
      m->interpolate( uvl2[seg2], (1.0-frac2), uvl2[seg2+1], frac2, uv2 );
      m->interpolate( uv1, ifrac, uv2, (1.0-ifrac), uv );
      uvpts += uv;

      norms +=
         (nl1[seg1]*(1.0-frac1) + nl1[seg1+1]*frac1)*ifrac  +
         (nl2[seg2]*(1.0-frac2) + nl2[seg2+1]*frac2)*(1.0-ifrac);

   }

   double pix_size = h1->get_pix_size() * ifrac + h2->get_pix_size() * (1.0 - ifrac);

   // XXX - Here's a policy for offset interp, maybe it sucks
   // but I'll revisit...

   num = (ol1->num() + ol2->num())/2;
   assert(num>1);
   for (k=0; k < num; k++)
   {
      BaseStrokeOffset o, o1, o2;
      double t = (double)k/(double)(num-1);

      ol1->get_at_t(t, o1);
      ol2->get_at_t(t, o2);

      o._pos   = t;
      o._len   = o1._len   * ifrac    +  o2._len     * (1.0 - ifrac);
      o._press = o1._press * ifrac    +  o2._press   * (1.0 - ifrac);
      o._type =   (k==0)?     (BaseStrokeOffset::OFFSET_TYPE_BEGIN):
                  ((k==num-1)?(BaseStrokeOffset::OFFSET_TYPE_END):
                              (BaseStrokeOffset::OFFSET_TYPE_MIDDLE));
      offsets->add(o);
   }

   offsets->set_replicate(0);
   offsets->set_hangover(1);
   offsets->set_pix_len(ol1->get_pix_len() * ifrac + ol2->get_pix_len() * (1.0 - ifrac));

   HatchingHatchFree *hhfr;

   hhfr = new HatchingHatchFree(hlb,pix_size,uvpts,pts,norms,offsets);
        
   return hhfr;
}

/////////////////////////////////////
// update_prototype()
/////////////////////////////////////
void    
HatchingGroupFreeInst::update_prototype()
{
   HatchingGroupBase::update_prototype();
}

/////////////////////////////////////
// draw_setup()
/////////////////////////////////////
void
HatchingGroupFreeInst::draw_setup()
{

   if (!is_complete()) return;

   assert(_position);
   _position->update();

   if (_position && (_position->curr_weight() == 0)) return;

   //Checks LOD, gets missing levels, 
   //and sets their animation settings

   update_levels();

}

/////////////////////////////////////
// level_draw_setup()
/////////////////////////////////////
void
HatchingGroupFreeInst::level_draw_setup()
{
   if (_position && (_position->curr_weight() == 0)) return;

   HatchingGroupBase::level_draw_setup();
}

/////////////////////////////////////
// hatch_draw_setup()
/////////////////////////////////////
void
HatchingGroupFreeInst::hatch_draw_setup()
{
   if (_position && (_position->curr_weight() == 0)) return;

   HatchingGroupBase::hatch_draw_setup();
}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int
HatchingGroupFreeInst::draw(CVIEWptr &v)
{
   if (_position && (_position->curr_weight() == 0)) return 0;

   return HatchingGroupBase::draw(v);
}

/////////////////////////////////////
// draw_select()
/////////////////////////////////////
int
HatchingGroupFreeInst::draw_select(CVIEWptr &v)
{
   if (_position && (_position->curr_weight() == 0)) return 0;

   return HatchingGroupBase::draw_select(v);
}

/*****************************************************************
 * HatchingHatchFree
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingHatchFree::_hhfr_tags = 0;

static int foobar = DECODER_ADD(HatchingHatchFree);

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingHatchFree::tags() const
{
   if (!_hhfr_tags) {
      _hhfr_tags = new TAGlist;
      *_hhfr_tags += HatchingHatchBase::tags();
      *_hhfr_tags += new TAG_val<HatchingHatchFree,UVpt_list>(
         "uvpts",
         &HatchingHatchFree::uvs_);

   }
   return *_hhfr_tags;
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingHatchFree::HatchingHatchFree(
   HatchingLevelBase *hlb, double len, CUVpt_list &uvl, 
   CWpt_list &pl,  const ARRAY<Wvec> &nl,
   CBaseStrokeOffsetLISTptr &ol) :
      HatchingHatchBase(hlb,len,pl,nl,ol)
{
   _uvs.clear();   
   _uvs.operator+=(uvl);

   //XXX - Fix this... init() get called twice (in base class, too)

   init();
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingHatchFree::HatchingHatchFree(
   HatchingLevelBase *hlb, 
   HatchingHatchFree *hhf):
   HatchingHatchBase(hlb)
{
   assert(hhf->get_uvs().num() == hhf->get_pts().num());
   assert(hhf->get_uvs().num() == hhf->get_norms().num());

   _uvs.clear();   
   _uvs.operator+=(hhf->get_uvs());

   _pts.clear();   
   _pts.operator+=(hhf->get_pts());

   _norms.clear();   
   _norms.operator+=(hhf->get_norms());

   _offsets = new BaseStrokeOffsetLIST;
   
   //XXX - Copy this or just use the ptr?!?
   _offsets->copy(*(hhf->get_offsets()));

   _pix_size = hhf->get_pix_size();

   init();
}

/////////////////////////////////////
// init()
/////////////////////////////////////
void
HatchingHatchFree::init()
{
   HatchingHatchBase::init();

   assert(_pts.num() == _uvs.num());       

}


/////////////////////////////////////
// draw_setup()
/////////////////////////////////////
void    
HatchingHatchFree::draw_setup()
{
   // XXX - Cache this better...
   // Modify stroke alpha to include dot product weight

   HatchingPositionFreeInst *p = ((HatchingGroupFreeInst*)_level->group())->position();
   if (p) _stroke->set_alpha( (float)p->curr_weight() );

   HatchingHatchBase::draw_setup();
        
}
/////////////////////////////////////
// stroke_real_setup()
/////////////////////////////////////
void    
HatchingHatchFree::stroke_real_setup()
{
   int i, good_num=0;

   //First time round, copy in the original stuff to
   //alloc the arrays
   if (_real_pts.num() == 0)
   {
      assert(_pts.num()   ==  _uvs.num());
      assert(_norms.num() ==  _uvs.num());

      for (i=0; i<_uvs.num(); i++)
      {
         _real_uvs.add(_uvs[i]);
         _real_pts.add(_pts[i]);
         _real_norms.add(_norms[i]);
         _real_good.add(true);
      }

      _real_pix_size = _pix_size;
   }

   //XXX - Should cache this or something
   HatchingPositionFreeInst *p = ((HatchingGroupFreeInst*)_level->group())->position();
   //XXX - Should cache this or something
   UVMapping *m = ((HatchingGroupFree*)_level->group()->group())->mapping();
   assert(m);

   UVpt uv;
   Bface *f;
   Wvec bc;

   for (i=0; i<_uvs.num(); i++)
   {
      if (p)
         p->transform(_uvs[i],uv);
      else
         uv = _uvs[i];
             
      f = m->find_face(uv, bc);

      if (f)
      {
         //Store uv for vis checking
         _real_uvs[i] = uv;
         
         //Life is good, we found a pt/norm for this uv
         f->bc2pos(bc,_real_pts[i]);     
                  
         //Use interpolated norm to smoth vis clipping at ends
         f->bc2norm_blend(bc,_real_norms[i]);


         //Store uv for vis checking
         _real_good[i] = true;

         good_num++;
      }
      else
      {
         //Store uv for vis checking (used for interpolation
         //at good to bad transition by HatchingStroke)
         _real_uvs[i] = uv;
         
         //No need to store the pt/norm   

         //Store uv for vis checking
         _real_good[i] = false;
      }

   }
   
   if (good_num)
      _visible = true;
   else
      _visible = false;

}

/////////////////////////////////////
// stroke_pts_setup()
/////////////////////////////////////
void    
HatchingHatchFree::stroke_pts_setup()
{
   if (!_visible) return;

   //Use _real_* vars to get 2D pts for stroke
   //and do visiblity

   HatchingSelectBase * select = (_level->group()->selected())?
      (_level->group()->selection()):(NULL);

   NDCZpt pt;

   CWtransf &tran = _level->group()->patch()->inv_xform();
   
   _stroke->clear();               


   static double pix_spacing = max(Config::get_var_dbl("HATCHING_PIX_SAMPLING",5,true),0.000001);

   //XXX - Account for scaling

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

      _stroke->add(pt, tran * _real_norms[j], _real_uvs[j], _real_good[j]);
      i += gap;
      j = (int)i;
   }
   pt = NDCZpt(_real_pts[n],_level->group()->patch()->obj_to_ndc());
   if (select) select->update(pt);
   _stroke->add(pt, tran * _real_norms[n], _real_uvs[n], _real_good[n]);


/*
   int n = _real_uvs.num();
   for (int i=0; i< num;i++) 
   {
      pt = NDCZpt(_real_pts[i],_level->group()->patch()->obj_to_ndc());
      if (select) select->update(pt);
      _stroke->add(pt, tran * _real_norms[i], _real_uvs[i], _real_good[i]);
   }   
*/

}



/*****************************************************************
 * HatchingBackboneFree
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingBackboneFree::_hbfr_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingBackboneFree::tags() const 
{
   if (!_hbfr_tags) {
      _hbfr_tags = new TAGlist;
      *_hbfr_tags += HatchingBackboneBase::tags();
      *_hbfr_tags += new TAG_meth<HatchingBackboneFree>( 
         "uvpts1",
         &HatchingBackboneFree::put_uvpts1,
         &HatchingBackboneFree::get_uvpts1,
         0);
      *_hbfr_tags += new TAG_meth<HatchingBackboneFree>(
         "uvpts2",
         &HatchingBackboneFree::put_uvpts2,
         &HatchingBackboneFree::get_uvpts2,
         0);

   }
   return *_hbfr_tags;
}

/////////////////////////////////////
// get_num()
/////////////////////////////////////
void
HatchingBackboneFree::get_num(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneFree::get_num()");
   int num, i;

   *d >> num;

   for (i=0;i<num;i++) 
      _vertebrae.add(new VertebraeFree);

}

/////////////////////////////////////
// put_uvpts1()
/////////////////////////////////////
void
HatchingBackboneFree::put_uvpts1(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneFree::put_uvpts1()");

   int i;
   ARRAY<UVpt> uvpts1;

   for (i=0; i<_vertebrae.num(); i++) 
      uvpts1.add(((VertebraeFree*)_vertebrae[i])->uvpt1);

   d.id();
   *d << uvpts1;
   d.end_id();
}

/////////////////////////////////////
// get_uvpts1()
/////////////////////////////////////
void
HatchingBackboneFree::get_uvpts1(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneFree::get_uvpts1()");

   int i;
   ARRAY<UVpt> uvpts1;

   *d >> uvpts1;

   assert(_vertebrae.num() == uvpts1.num());

   for (i=0; i<_vertebrae.num(); i++) 
      ((VertebraeFree*)_vertebrae[i])->uvpt1 = uvpts1[i];

}

/////////////////////////////////////
// put_uvpts2()
/////////////////////////////////////
void
HatchingBackboneFree::put_uvpts2(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneFree::put_uvpts2()");

   int i;
   ARRAY<UVpt> uvpts2;

   for (i=0; i<_vertebrae.num(); i++) 
      uvpts2.add(((VertebraeFree*)_vertebrae[i])->uvpt2);

   d.id();
   *d << uvpts2;
   d.end_id();
}

/////////////////////////////////////
// get_uvpts2()
/////////////////////////////////////
void
HatchingBackboneFree::get_uvpts2(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingBackboneFree::get_uvpts2()");

   int i;
   ARRAY<UVpt> uvpts2;

   *d >> uvpts2;

   assert(_vertebrae.num() == uvpts2.num());

   for (i=0; i<_vertebrae.num(); i++) 
      {
         ((VertebraeFree*)_vertebrae[i])->uvpt2[0] = uvpts2[i][0];
         ((VertebraeFree*)_vertebrae[i])->uvpt2[1] = uvpts2[i][1];
      }

}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingBackboneFree::HatchingBackboneFree(
   Patch *p,
   HatchingGroupFreeInst *i) :
   HatchingBackboneBase(p),
   _inst(i)
{

   _use_exist = true;

   //Do nothing to generate backbone
   //Wait for a call to build from
   //scratch or from disk

}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingBackboneFree::HatchingBackboneFree(
   Patch *p,
   HatchingGroupFreeInst *i,
   HatchingBackboneFree *b) :
   HatchingBackboneBase(p),
   _inst(i)
{

   _use_exist = true;

   //Clone the given backbone

   _len = b->_len;

   VertebraeFree *vf;
   for (int k=0; k<b->_vertebrae.num(); k++)
      {
         vf = new VertebraeFree;
         assert(vf);
         *vf = *((VertebraeFree*)b->_vertebrae[k]);
         _vertebrae.add(vf);
      }

}


/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingBackboneFree::~HatchingBackboneFree()
{
   //We own nothing that the base class doesn't already destroy
}

/////////////////////////////////////
// get_ratio()
/////////////////////////////////////
double
HatchingBackboneFree::get_ratio()
{
        
   int i;
//   NDCpt   n1, n2;
   Bface *f1, *f2;
   UVMapping *m;
   VertebraeFree *vf;
   Wvec bc1, bc2; 
   UVpt    uv1, uv2;

   assert(_inst);
   m = ((HatchingGroupFree*)_inst->group())->mapping();
   assert(m);

   HatchingPositionFreeInst *p = _inst->position();

   for (i=0;i<_vertebrae.num();i++)
      {
         vf = (VertebraeFree*)_vertebrae[i];

         if (p)
            {
               _inst->position()->transform(vf->uvpt1,uv1);
               _inst->position()->transform(vf->uvpt2,uv2);
            }
         else
            {
               uv1 = vf->uvpt1;
               uv2 = vf->uvpt2;
            }

         f1 = m->find_face(uv1, bc1);
         f2 = m->find_face(uv2, bc2);

         if ( (!f1) || (!f2) )
            {
               vf->exist = false;
            }
         else
            {
               f1->bc2pos(bc1,vf->pt1);        
               f2->bc2pos(bc2,vf->pt2);
               vf->exist = true;
            }
      }
   return HatchingBackboneBase::get_ratio();
}

/////////////////////////////////////
// compute()
/////////////////////////////////////
//
// -To compute on the fly, step through
//  each hatching pair, and find closest
//  point on longer hatch to mid of other
// -Set all pairs to exist, but who cares
//  as this is set every frame during uv lookup
// -Also fill in all segment lengths
//
/////////////////////////////////////
bool
HatchingBackboneFree::compute(
   HatchingLevelBase *hlb)
{
   int i;
   UVMapping *m;

   assert(hlb);
   assert(_inst);

   m = ((HatchingGroupFree*)_inst->group())->mapping();

   assert(m);

   if (hlb->num() < 2)
   {
      err_mesg(ERR_LEV_WARN, "HatchingBackboneFixed::compute() - Can't get backbone from less that 2 hatches!");
      return false;
   }
                
   _len = 0;

   for (i=0;i<hlb->num()-1;i++)
   {
      int ind;
      double frac;
      double foo;
      Wpt p;
      UVpt puv;
      Wpt_list l;
      Wpt loc;
      UVpt locuv;

      if ((*hlb)[i]->get_pts().length() < (*hlb)[i+1]->get_pts().length())
      {
         p =     (*hlb)[i  ]->get_pts().interpolate(0.5,0,&ind,&frac);
         m->interpolate(((HatchingHatchFree*)(*hlb)[i  ])->get_uvs()[ind  ], (1.0-frac),
                        ((HatchingHatchFree*)(*hlb)[i  ])->get_uvs()[ind+1], (    frac),
                        puv);

         l =     (*hlb)[i+1]->get_pts();
         l.closest(p,loc,foo,ind);
         frac = (loc-l[ind]).length()/(l[ind+1]-l[ind]).length();
         m->interpolate(((HatchingHatchFree*)(*hlb)[i+1])->get_uvs()[ind  ], (1.0-frac),
                        ((HatchingHatchFree*)(*hlb)[i+1])->get_uvs()[ind+1], (    frac),
                        locuv);
      }
      else
      {
         p =   (*hlb)[i+1]->get_pts().interpolate(0.5,0,&ind,&frac);
         m->interpolate(((HatchingHatchFree*)(*hlb)[i+1])->get_uvs()[ind  ], (1.0-frac),
                        ((HatchingHatchFree*)(*hlb)[i+1])->get_uvs()[ind+1], (    frac),
                        puv);

         l =   (*hlb)[i  ]->get_pts();
         l.closest(p,loc,foo,ind);
         frac = (loc-l[ind]).length()/(l[ind+1]-l[ind]).length();
         m->interpolate(((HatchingHatchFree*)(*hlb)[i  ])->get_uvs()[ind  ], (1.0-frac),
                        ((HatchingHatchFree*)(*hlb)[i  ])->get_uvs()[ind+1], (    frac),
                        locuv);
      }
             
      //XXX - We used real points the lie on lines between
      //the real points, but the linearly interpolated uv
      //may not exactly correspond to these real points.
      //The difference isn't likely significant, and
      //by not looking up the real points from the uv,
      //we avoid the miniscule possibility that we fail...

      NDCpt n1 = NDCZpt( p,   _patch->obj_to_ndc() );
      NDCpt n2 = NDCZpt( loc, _patch->obj_to_ndc() );

      double len = (n2 - n1).length() * VIEW::peek()->ndc2pix_scale();

      VertebraeFree *vf = new VertebraeFree();
      assert(vf);

      vf->exist = true;
      vf->uvpt1 = puv;
      vf->uvpt2 = locuv;
      vf->pt1 = p;
      vf->pt2 = loc;
      vf->len = len;

      _len += len;

      _vertebrae.add(vf);
   }

   err_mesg(ERR_LEV_INFO, 
      "HatchingBackboneFree::compute() - Backbone is %f pixels in %d vertebrae.",
         _len, _vertebrae.num());

   return true;
}

/*****************************************************************
 * HatchingPositionFree
 *****************************************************************/

#define HATCHING_POSITION_FREE_TOL 0.05

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingPositionFree::_hpfr_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingPositionFree::tags() const
{
   if (!_hpfr_tags) {
      _hpfr_tags = new TAGlist;
      *_hpfr_tags += new TAG_val<HatchingPositionFree,UVpt>(
         "lower_left",
         &HatchingPositionFree::lower_left_);
      *_hpfr_tags += new TAG_val<HatchingPositionFree,UVpt>(
         "upper_right",
         &HatchingPositionFree::upper_right_);
      *_hpfr_tags += new TAG_val<HatchingPositionFree,UVpt>(
         "center",
         &HatchingPositionFree::center_);
      *_hpfr_tags += new TAG_val<HatchingPositionFree,Wvec>(
         "direction",
         &HatchingPositionFree::direction_);
      *_hpfr_tags += new TAG_val<HatchingPositionFree,double>(
         "left_dot",
         &HatchingPositionFree::left_dot_);
      *_hpfr_tags += new TAG_val<HatchingPositionFree,double>(
         "right_dot",
         &HatchingPositionFree::right_dot_);
   }
   return *_hpfr_tags;
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingPositionFree::HatchingPositionFree(
   HatchingGroupFree* hg,
   UVMapping *m) :
   _group(hg),
   _mapping(m),
   _lower_left(UVpt(0,0)),
   _upper_right(UVpt(0,0)),
   _center(UVpt(0,0)),
   _direction(Wvec(0,0,0)),
   _left_dot(0.0),
   _right_dot(0.0),
   _curr_placements(NULL),
   _old_placements(NULL)
{
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingPositionFree::~HatchingPositionFree()
{
   while (_geoms.num()>0)
      WORLD::destroy(_geoms.pop());

   if (_curr_placements) 
      delete _curr_placements;

   if (_old_placements) 
      delete _old_placements;
}

/////////////////////////////////////
// compute()
/////////////////////////////////////
bool
HatchingPositionFree::compute(
   HatchingGroupFreeInst *hgfri)
{
   assert(_group);
   assert(_mapping);
   assert(hgfri);

   if (!compute_bounds(hgfri))
      return false;

   if (!compute_location())
      return false;

   return true;

}

/////////////////////////////////////
// compute_location()
/////////////////////////////////////
//
// -Finds the center of the group
//  (as bounding box center)
// -Find surface normal there and
//  stores are a screen space direction
// -Stores dot product between this dir
//  and normals at min and max
//  u edges of bounding box (a measure
//  of the extent of the group)
//
/////////////////////////////////////
bool
HatchingPositionFree::compute_location()
{
   UVpt query;
   UVpt offset(0,0);
   Wvec surface_norm;
   Wvec test_norm;
   Wvec bc;
   Bface *f;

   //So we can average over the seam,
   //we offset the respective coordinate(s) 
   //in the upper right by the respective span(s)
   //if the bounding box crosses seam(s)
   //(i.e. unwrap the box)
   //Then we apply wrapping to the result

   if (_lower_left[0] > _upper_right[0])
      offset[0] += _mapping->span_u();
   if (_lower_left[1] > _upper_right[1])
      offset[1] += _mapping->span_v();

   _center = _lower_left;
   _center += _upper_right;
   _center += offset;
   _center /= 2.0;
   _mapping->apply_wrap(_center);

   err_mesg(ERR_LEV_INFO, 
      "HatchingPositionFree::compute_location() - Center of hatch group: (%f,%f)",
         _center[0], _center[1]);

   //The lighting direction is taken as the
   //surface normal at the center, transformed
   //into view space

   //XXX - Something more representative of 
   //the whole group might be better...

   f = _mapping->find_face(_center,bc);
   if (!f)
   {
      err_mesg(ERR_LEV_WARN, "HatchingPositionFree::compute_location() - Failed!! Center of hatch group fails uv lookup!!");
      return false;
   }
   f->bc2norm_blend(bc,surface_norm);
   _direction = (VIEW::peek_cam()->data()->xform(0) * _group->patch()->xform() * surface_norm).normalized();

   //For now, hatch groups only slide in the u direction.
   //To figure out their u scaling, we record the following.
   //The min and max dots are the dot products of the
   //surface normals (with the light direction) at the 
   //points on the surface where a line of constant v
   //passing through the center intersects the
   //bounding box

   query[1] = _center[1];

   query[0] = _lower_left[0];
   f = _mapping->find_face(query,bc);
   if (!f)
   {
      err_mesg(ERR_LEV_WARN, "HatchingPositionFree::compute_location() - Failed!! Midpoint of left bounding box edge fails uv lookup.");
      return false;
   }
   f->bc2norm_blend(bc,test_norm);
   _left_dot = surface_norm * test_norm;

   query[0] = _upper_right[0];
   f = _mapping->find_face(query,bc);
   if (!f)
   {
      err_mesg(ERR_LEV_WARN, "HatchingPositionFree::compute_location() - Failed!! Midpoint of right bounding box edge fails uv lookup.");
      return false;
   }
   f->bc2norm_blend(bc,test_norm);
   _right_dot = surface_norm * test_norm;

   err_mesg(ERR_LEV_INFO, 
      "HatchingPositionFree::compute_location() - Dot products at left/right: (%f,%f)",
         _left_dot, _right_dot);

   return true;
}

/////////////////////////////////////
// compute_bounds()
/////////////////////////////////////
//
// -Computes lower left and upper right
//  vertices of uv bounding box
//
/////////////////////////////////////
bool
HatchingPositionFree::compute_bounds(
   HatchingGroupFreeInst *hgfri)
{
   assert(_mapping);
   assert(hgfri);

   int l,h,k,k1;
   double umin, umax;
   double vmin, vmax;

   //We save a uv bounding box (lower left/upper right) 
   //and take care to account for seams in u or v

   UVpt_list pts;

   for (l=0; l< hgfri->num_base_levels(); l++)
   {
      for (h=0; h<hgfri->base_level(l)->num(); h++)
      {
         pts.operator+=(
            ((HatchingHatchFree*)(*hgfri->base_level(l))[h])->get_uvs()
            );
      }
   }

   //Consider each point for blindly deciding the extremals.
   //Also consider every pair of points.  If they lie
   //on opposite sides of a discontinuity then we'll
   //come back and be careful at the seam.

   umin = _mapping->max_u();
   umax = _mapping->min_u();
   vmin = _mapping->max_v();
   vmax = _mapping->min_v();

   bool discont_u = false;
   bool discont_v = false;

   for (k=0; k<pts.num(); k++)
   {
      UVpt uv = pts[k];

      //Do usual min/max finding
      if (uv[0]>umax) umax=uv[0];
      if (uv[0]<umin) umin=uv[0];
      if (uv[1]>vmax) vmax=uv[1];
      if (uv[1]<vmin) vmin=uv[1];

      //Check seam crossings in u
      if ((!discont_u)&&(_mapping->wrap_u()))
      {
         for (k1=k+1; k1<pts.num(); k1++)
         {
            UVpt uv1 = pts[k1];
                  
            //Assume a seam is crossed if delta u is > half the region's span
            if (fabs(uv[0] - uv1[0]) > 0.5 * _mapping->span_u()) 
            {
               discont_u = true;
               break;
            }
         }

      }

      //Check seam crossings in v
      if ((!discont_v)&&(_mapping->wrap_v()))
      {
         for (k1=k+1; k1<pts.num(); k1++)
         {
            UVpt uv1 = pts[k1];
                  
            //Assume a seam is crossed if delta v is > half the region's span
            if (fabs(uv[1] - uv1[1]) > 0.5 * _mapping->span_v()) 
            {
               discont_v = true;
               break;
            }
         }

      }
   }

   //Now we go back and fix the min/max if any seam
   //crossings were found
   if (discont_u)
   {
      err_mesg(ERR_LEV_INFO, "HatchingPositionFree::compute_bounds() - Hatch group crosses a u seam. Fixing bounding box...");
       
      double mid = _mapping->min_u() + (0.5 * _mapping->span_u());
             
      umin = _mapping->max_u();
      umax = _mapping->min_u();

      for (k=0; k< pts.num(); k++)
      {
         if (pts[k][0] > mid)
         {
            if (pts[k][0]<umin) umin = pts[k][0];
         }
         else //right of seam
         {
            if (pts[k][0]>umax) umax = pts[k][0];
         }
      }
   }

   if (discont_v)
   {
      err_mesg(ERR_LEV_INFO, "HatchingPositionFree::compute_bounds() - Hatch group crosses a v seam. Fixing bounding box...");
       
      double mid = _mapping->min_v() + (0.5 * _mapping->span_v());
             
      vmin = _mapping->max_v();
      vmax = _mapping->min_v();

      for (k=0; k< pts.num(); k++)
      {
         if (pts[k][1] > mid)
         {
            if (pts[k][1]<vmin) vmin = pts[k][1];
         }
         else //right of seam
         {
            if (pts[k][1]>vmax) vmax = pts[k][1];
         }
      }
   }

   //Expand the box slightly to avoid vis glitches

   double uwid, vwid, du, dv;

   uwid = fabs(umax - umin);
   vwid = fabs(vmax - vmin);

   du = uwid * HATCHING_POSITION_FREE_TOL / 2.0;
   dv = vwid * HATCHING_POSITION_FREE_TOL / 2.0;

   umin -= du;
   umax += du;
        
   vmin -= dv;
   vmax += dv;

   _lower_left = UVpt(umin,vmin);
   _upper_right = UVpt(umax,vmax);

   _mapping->apply_wrap(_lower_left);
   _mapping->apply_wrap(_upper_right);

   err_mesg(ERR_LEV_INFO, 
      "HatchingPositionFree::compute_bounds() - Min/max u: (%f,%f) and min/max v: (%f,%f)",
         umin, umax, vmin, vmax);

   return true;
}

/////////////////////////////////////
// cache_search_normals()
/////////////////////////////////////
void
HatchingPositionFree::cache_search_normals()
{
   //XXX - This is the number of divisions in the coarse
   //search.  It should be dynanically computed from some
   //measure of the number of faces crossed by a line
   //in uv space

   const int num = 100;

   if (_search_normals.num() == 0)
   {
      err_mesg(ERR_LEV_SPAM, "HatchingPositionFree::cache_search_normals() - Caching search normals...");
             
      Bface *f;
      Wvec bc;
      UVpt query = _center;           
      double du = _mapping->span_u()/((double)(num-1));
      int count=0;

      for (int k=0;k<num;k++)
      {
         query[0] = _mapping->min_u() + ((double)k)*du;

         f = _mapping->find_face(query,bc);
         _search_normals += Wvec(0,0,0);
         _search_dots += 0.0;
         if (f)
            {
               f->bc2norm_blend(bc,_search_normals[k]);
               count++;
            }
         else
            {
               _search_normals[k] = Wvec(666,666,666);
               _search_dots[k] = 666;
            }
      }
      
      //err_mesg(ERR_LEV_SPAM, "Done.");

      //Let's assert that atleast SOME good normals are found
      if (count==0)
      {
         err_mesg(ERR_LEV_WARN, "HatchingPositionFree::cache_search_normals() - Found ZERO good normals. Bailing out.");
         assert(0);
      }
      else if (count < num)
      {
         err_mesg(ERR_LEV_INFO, 
            "HatchingPositionFree::cache_search_normals() - WARNING!!! Only %d of the possible %d good normals was/were found.",
               count, num);
      }
      //XXX - Maybe temporary
      smooth_search_normals();
   }

}

/////////////////////////////////////
// smooth_search_normals()
/////////////////////////////////////
void
HatchingPositionFree::smooth_search_normals()
{
   ARRAY<Wvec> smooth;
   ARRAY<Wvec> swap;

   bool okay;
   int i, iters;
   int k, k_minus, k_plus;
   int num = _search_normals.num();

   smooth = _search_normals;

   double du = _mapping->span_u()/((double)(num-1));

   double width = _upper_right[0] - _lower_left[0];

   if (_upper_right[0] < _lower_left[0])
      {
         assert(_mapping->wrap_u());
         width += _mapping->span_u();
      }

   iters = int(ceil(width / du));

   err_mesg(ERR_LEV_INFO, "HatchingPositionFree::smooth_search_normals() - Smoothing %d times.", iters);

   for (i=0; i<iters; i++)
   {
      swap = smooth;

      //We don't smooth a normal if either
      //neighbour is bad (a hole in uv map)
      //or the end point for non-wrapping
      //regions.  We set okay to false when
      //we don't want to smooth
      //Note: k-1 is same as zero for wraps, but
      //we just use 0 in that case. Soooo
      //k-1 isn't ever smoothed, and should be
      //avoided...

      for (k=0; k<(num-1); k++)
      {
         if (k==0)
         {
            if (_mapping->wrap_u())
            {
               okay = true;
               k_minus = num-2;
               k_plus = k+1;
            }
            else
            {
               okay = false;
            }
         }
         else if (k==(num-2))
         {
            if (_mapping->wrap_u())
            {
               okay = true;
               k_minus = k-1;
               k_plus = 0;
            }
            else
            {
               okay = true;
               k_minus = k-1;
               k_plus = k+1;
            }
         }
         else
         {
            okay = true;
            k_minus = k-1;
            k_plus = k+1;
         }
                  
         if ((okay)&&(swap[k_minus][0]==666 || swap[k][0]==666 || swap[k_plus][0]==666 ))
         {
            okay = false;
         }

         if (okay)
         {
            smooth[k] = (swap[k_minus] + swap[k] + swap[k_plus]).normalized();
         }
      }
      //The first and last point are same.
      //We never use [num-1] but it's shown in
      //the plot of the graph, so let's tidy
      //it up.
      if (_mapping->wrap_u())
         smooth[num-1] = smooth[0];
   }

   _old_search_normals = _search_normals;
   _search_normals = smooth;

   generate_normal_spline();

}

/////////////////////////////////////
// generate_normal_spline()
/////////////////////////////////////
void
HatchingPositionFree::generate_normal_spline()
{

   Wpt zip(0,0,0);

   int k;

   int num = _search_normals.num();

   double du = _mapping->span_u()/((double)(num-1));

   //Setup the spline to take the u value and
   //return the normal (which but get cast from a Wpt
   //back to a Wvec and normalized).
   //Wrapped regions add a wrapped point at the start
   //and end to allow the spline to handle the seam.
   //For now, missing normals (due to holes) are 
   //just skipped, and we hope the holes are small
   //so that the spline can interp the across the hole.
   //When querying this spline, we should consult
   //the _search_normals to see if we're in a hole
   //and then return a failure, rather than the interped value.

   //Wrap point for wrapping regions
   if (_mapping->span_u())
      {
         if (_search_normals[num-2][0] != 666)
            {
               _normal_spline.add(zip+_search_normals[num-2],_mapping->min_u()-du);
            }
      }

   //Middle bit
   for (k=0; k<=(num-2); k++)
      {
         if (_search_normals[k][0] != 666)
            {
               _normal_spline.add(zip+_search_normals[k],_mapping->min_u() + ((double)k)*du);
            }
      }

   //End bit
   if (!_mapping->span_u())
      {
         if (_search_normals[num-1][0] != 666)
            {
               _normal_spline.add(zip+_search_normals[num-1],_mapping->max_u());
            }
      }
   else
      {
         if (_search_normals[0][0] != 666)
            {
               _normal_spline.add(zip+_search_normals[0],_mapping->max_u());
            }
         if (_search_normals[1][0] != 666)
            {
               _normal_spline.add(zip+_search_normals[1],_mapping->max_u()+du);
            }
      }


}

/////////////////////////////////////
// query_normal_spline()
/////////////////////////////////////
bool
HatchingPositionFree::query_normal_spline(double u, Wvec &n)
{
   assert(u>=_mapping->min_u());
   assert(u<=_mapping->max_u());

   int num = _search_normals.num();
   double du = _mapping->span_u()/((double)(num-1));

   // XXX - two identical numbers?
   int k_minus = (int)floor((u - _mapping->min_u())/du);
   int k_plus =  (int)floor((u - _mapping->min_u())/du);

   //Sanity check
   assert(k_minus>=0);
   assert(k_plus>=0);
   assert(k_minus<num);
   assert(k_plus<num);

   //If the control normal at either
   //side of u is bad, then this normal
   //query is bad

   if ((_search_normals[k_minus][0] == 666) || (_search_normals[k_plus][0] == 666))
      {
         n = Wvec(666,666,666);
         return false;
      }

   n = (_normal_spline.pt(u) - Wpt(0,0,0)).normalized();

   return true;

}

/////////////////////////////////////
// cache_search_dots()
/////////////////////////////////////
void
HatchingPositionFree::cache_search_dots()
{
   int num = _search_normals.num();

   static int debug_mapping = Config::get_var_bool("HATCHING_DEBUG_MAPPING",false,true)?true:false;

   if (!debug_mapping)
      {
         for (int k=0; k<num; k++)
            {
               if (_search_normals[k][0] != 666)
                  _search_dots[k] = _search_normals[k] * _curr_direction;
               //else 
               //              the dot's already 666
            }
      }
   else
      {
         double du = _mapping->span_u()/((double)(num-1));
        
         double u;

         for (int k=0; k<num; k++)
            {
               u = _mapping->min_u() + du*k;

               if (_search_normals[k][0] != 666)
                  {
                     //XXXX - Draw the unsmoothed too just for kicks
                     if (_old_search_normals[k][0] != 666)
                        _mapping->debug_dot(u,_old_search_normals[k]*_curr_direction,0x99, 0x55, 0x55);

                     _search_dots[k] = _search_normals[k] * _curr_direction;
                     _mapping->debug_dot(u,_search_dots[k],0xFF, 0xFF, 0xFF);

                  }
               else 
                  {
                                //the dot's already 666
                     _mapping->debug_dot(u,0.0,0xFF, 0x00, 0x00);
                  }
            }

      }
}

/////////////////////////////////////
// refine_placement_maximum()
/////////////////////////////////////
bool
HatchingPositionFree::refine_placement_maximum(
   int maxk, 
   double& maxu,
   double& maxdot)
{

//   int k;

   int num = _search_normals.num();
   double du = _mapping->span_u()/((double)(num-1));

   //Location of coarse maximum
   maxu = _mapping->min_u() + du*maxk;

   //u values of neighbouring points bounding extremal
   //They depend upon wrapping...
   double uplus;
   double uminus;

   if (_mapping->wrap_u())
      {
         uminus = _mapping->min_u() + du*(double)((num-1 + maxk-1)%(num-1));
         uplus = _mapping->min_u() + du*(double)((maxk+1)%(num-1));
      }
   else
      {
         if (maxk == (num-1))
            {
               uminus = _mapping->min_u() + du*(double)(maxk - 1);
               uplus = _mapping->min_u() + du*(double)(maxk);
            }
         else if (maxk == 0)
            {
               uminus = _mapping->min_u() + du*(double)(maxk);
               uplus = _mapping->min_u() + du*(double)(maxk+1);
            }
         else
            {
               uminus = _mapping->min_u() + du*(double)(maxk-1);
               uplus = _mapping->min_u() + du*(double)(maxk+1);
            }
      }

   maxu = refine(  maxu,
                   uminus, 
                   uplus, 
                   1.0,
                   0.0001,
                   _curr_direction);

   Wvec maxnorm;

   // QQQ - XXX - Change to use spline
   //query[0] = maxu;
   //f = _mapping->find_face(query,bc);
   //assert(f);
   //f->bc2norm_blend(bc,maxnorm);
   // QQQ - XXX //
   bool ret = query_normal_spline(maxu,maxnorm);
   assert(ret);
   // QQQ - XXX //

   maxdot = maxnorm * _curr_direction;

   return (maxdot > 0.0);
}


/////////////////////////////////////
// refine_placement_extent()
/////////////////////////////////////
bool
HatchingPositionFree::refine_placement_extent(
   double /* absmaxu */,
   double maxu,
   double maxdot,
   double& leftu, 
   double& rightu)
{
   assert(maxdot>0.0);

   int k;

   int num = _search_normals.num();
   double du = _mapping->span_u()/((double)(num-1));

   //u values of neighbouring points bounding extremal
   //They depend upon wrapping...
   double uplus;
   double uminus;

   //double leftdot = min(sqrt(maxdot)*_left_dot, maxdot - 0.01);
   //double rightdot = min(sqrt(maxdot)*_right_dot,maxdot - 0.01);
   double leftdot = maxdot*_left_dot; 
   double rightdot = maxdot*_right_dot;

   //We used to use the surf. norm at maxu...
   Wvec maxnorm = _curr_direction;

   double lastdot;
   bool trough;
   int rightk, leftk;
   int startk, endk;
   int lastk;

   assert(maxdot > 0);

   //Step out right first using the cached normals
   rightk = -1;
   lastk = -1;
   lastdot = 2;
   trough = false;
   if (_mapping->wrap_u())
   {
      startk = ((int)ceil(maxu/du))%(num-1);
      endk = (num-1 + startk-1)%(num-1);
   }
   else
   {
      startk = ((int)ceil(maxu/du));
      endk = num-1;
   }

   for (k =  startk; ; k++)
   {

      if (_mapping->wrap_u()) 
         k = (k<(num-1))?(k):(0);

      if (_search_normals[k][0] == 666)
      {
         //XXX - laziness
         if (rightk == -1)
         {
            err_mesg(ERR_LEV_WARN, "HatchingPositionFree::refine_placement() - 1st norm right of max is bad. Giving up.");
            k = endk; //stop
         }
         //else keep going to see if we pass this hole
         //and find we're still short of the goal
      }
      else
      {
         double dot = _search_normals[k]*maxnorm;
         if ( dot >= rightdot)
         {
                       //Abort out if we're climbing again
            if (dot > lastdot)
            {
               trough = true;
               k = endk;
            }
            else
            {
               rightk = k;
               lastdot=dot;
            }
         }
         else
         {
            if ( (lastk==-1) || (lastk == rightk))
            {
               //The last norm was good (or this is the 1st)
               //so this point and the previous bracket the goal
               rightk = k;
               k = endk; //stop
            }
            else
            {
               //The last norm was bad (and perhaps many
               //before it.  For now, let's just assume
               //that the we should bound at the last good pt
               k = endk; //stop
            }
         }
      }
      lastk = k;
      if (k==endk) break;
   }

   //Now refine
   rightu = _mapping->min_u() + du*rightk;

   if (rightk != -1)
   {
      if (!trough)
      {
         if (_mapping->wrap_u())
         {
            uplus = rightu;
                       //uminus = _mapping->min_u() + du*(double)((num-1 + rightk-1)%(num-1));
            uminus = _mapping->min_u() + du*(double)(rightk-1);
                       //Shouldn't bound refinement below the center
                       //Assure that the region to search for the right
                       //extent is bounded on the left by a position
                       //to the right of the max (or =)
            double mu = (maxu <= rightu)?(maxu):(maxu - _mapping->span_u());
            uminus = max(uminus,mu);
            if (uminus < _mapping->min_u())
               uminus += _mapping->span_u();
         }
         else
         {
            uplus = rightu;
            uminus = _mapping->min_u() + du*(double)((rightk)?(rightk-1):(0));
                       //Shouldn't bound refinement below the center
            uminus = max(maxu,uminus);
         }
      }
      else
      {
         if (_mapping->wrap_u())
         {
            uplus = _mapping->min_u() + du*(double)((rightk+1)%(num-1));
                       //uminus = _mapping->min_u() + du*(double)((num-1 + rightk-1)%(num-1));
            uminus = _mapping->min_u() + du*(double)(rightk-1);
                       //Shouldn't bound refinement below the center
                       //Assure that the region to search for the right
                       //extent is bounded on the left by a position
                       //to the right of the max (or =)
            double mu = (maxu <= rightu)?(maxu):(maxu - _mapping->span_u());
            uminus = max(uminus,mu);
            if (uminus < _mapping->min_u())
               uminus += _mapping->span_u();
         }
         else
         {
            if (rightk == (num-1))
               {
                  uminus = _mapping->min_u() + du*(double)(rightk - 1);
                  uplus = _mapping->min_u() + du*(double)(rightk);
               }
            else if (rightk == 0)
               {
                  uminus = _mapping->min_u() + du*(double)(rightk);
                  uplus = _mapping->min_u() + du*(double)(rightk+1);
               }
            else
               {
                  uminus = _mapping->min_u() + du*(double)(rightk-1);
                  uplus = _mapping->min_u() + du*(double)(rightk+1);
               }
         }
      }
      rightu = refine(        rightu,
                              uminus, 
                              uplus,
                              (!trough)?rightdot:(-2.0),
                              0.0001,
                              maxnorm);
   }
   else
   {
      //We'll just call this placement bad for now... 
      //Don't think this happens... but maybe one day...
   }

   //Now track out left using cached norms
   leftk = -1;
   lastk = -1;
   lastdot = 2;
   trough = false;
   if (_mapping->wrap_u())
   {
      startk = ((int)floor(maxu/du))%(num-1);
      endk = (startk+1)%(num-1);
   }
   else
   {
      startk = ((int)floor(maxu/du));
      endk = 0;
   }
        
   for (k = startk; ; k-- )
   {
      if (_mapping->wrap_u())
         k = (k>=0)?(k):(num-2);

      if (_search_normals[k][0] == 666)
      {
         if (leftk == -1)
         {
            err_mesg(ERR_LEV_WARN, "HatchingPositionFree::refine_placement() - 1st norm left of max is bad. Giving up.");
            k = endk; //stop
         }
         //else keep going to see if we pass this hole
         //and find we're still short of the goal
      }
      else
      {
         double dot = _search_normals[k]*maxnorm;
         if (dot >= leftdot)
         {
                       //Abort out if we're climbing again
            if (dot > lastdot)
            {
               trough = true;
               k = endk;
            }
            else
            {
               leftk = k;
               lastdot=dot;
            }
         }
         else
         {
            if ( (lastk==-1) || (lastk == leftk) )
            {
               //The last norm was good (or this is the 1st)
               //so this point and the previous braket the goal
               leftk = k;
               k = endk; //stop
            }
            else
            {
               //The last norm was bad (and perhaps many
               //before it.  For now, let's just assume
               //that the we should bound at the last good pt
               k = endk; //stop
            }
         }
      }
      lastk = k;
      if (k==endk) break;
   }

   leftu = _mapping->min_u() + du*leftk;

   //Refine
   if (leftk != -1)
   {
      if (!trough)
      {
         if (_mapping->wrap_u())
         {
            uminus = leftu;
                       //uplus = _mapping->min_u() + du*(double)((leftk+1)%(num-1));
            uplus = _mapping->min_u() + du*(double)(leftk+1);
                       //Shouldn't bound refinement above the center
                       //Assure that the region to search for the left
                       //extent is bounded on the right by a position
                       //to the left of the max (or =)
            double mu = (maxu >= leftu)?(maxu):(maxu + _mapping->span_u());
            uplus = min(uplus,mu);
            if (uplus > _mapping->max_u())
               uplus -= _mapping->span_u();
         }
         else
         {
            uminus = leftu;
            uplus = _mapping->min_u() + du*(double)((leftk < (num-1))?(leftk + 1):leftk);
                       //Shouldn't bound refinement above the center
            uplus = min(maxu,uplus);
         }
      }
      else
      {
         if (_mapping->wrap_u())
         {
            uminus = _mapping->min_u() + du*(double)((num-1 + leftk-1)%(num-1));
                       //uplus = _mapping->min_u() + du*(double)((leftk+1)%(num-1));
            uplus = _mapping->min_u() + du*(double)(leftk+1);
                       //Shouldn't bound refinement above the center
                       //Assure that the region to search for the left
                       //extent is bounded on the right by a position
                       //to the left of the max (or =)
            double mu = (maxu >= leftu)?(maxu):(maxu + _mapping->span_u());
            uplus = min(uplus,mu);
            if (uplus > _mapping->max_u())
               uplus -= _mapping->span_u();

         }
         else
         {
            if (leftk == (num-1))
            {
               uminus = _mapping->min_u() + du*(double)(leftk - 1);
               uplus = _mapping->min_u() + du*(double)(leftk);
            }
            else if (leftk == 0)
            {
               uminus = _mapping->min_u() + du*(double)(leftk);
               uplus = _mapping->min_u() + du*(double)(leftk+1);
            }
            else
            {
               uminus = _mapping->min_u() + du*(double)(leftk-1);
               uplus = _mapping->min_u() + du*(double)(leftk+1);
            }
         }
      }

      leftu = refine( leftu,
                      uminus, 
                      uplus, 
                      (!trough)?leftdot:(-2.0),
                      0.0001,
                      maxnorm);
   }
   else
   {
      //We'll just call this placement bad for now... 
      //Don't think this happens... but maybe one day...
   }


   static bool debug = Config::get_var_bool("HATCHING_DEBUG_PLACEMENT",false,true);
   static bool debug_spread =
      Config::get_var_bool("HATCHING_DEBUG_PLACEMENT_SPREAD",false,true);

   if (debug)
   {
      Wvec n;
      Wpt  p;

      Bface *f;
      UVpt query = _center;
      Wvec bc;
             
      query[0] = maxu;
      f = _mapping->find_face(query,bc);
      assert(f);
      f->bc2pos(bc,p);
      p = _group->patch()->xform() * p;
      // QQQ - XXX - Change to use spline
      //f->bc2norm_blend(bc,n);
      // QQQ - XXX //
      query_normal_spline(maxu,n);
      // QQQ - XXX
      n = _group->patch()->xform() * n;
//       _geoms.add(WORLD::show(p,n,2.5,10));
//       _geoms.add(WORLD::show(p,_curr_direction,1.1,20));

      if (debug_spread)
      {
         if (leftk != -1)
         {
            query[0] = leftu;
            f = _mapping->find_face(query,bc);
            assert(f);
            f->bc2pos(bc,p);
            p = _group->patch()->xform() * p;
                       // QQQ - XXX - Change to use spline
            //f->bc2norm_blend(bc,n);
                       // QQQ - XXX //
            query_normal_spline(leftu,n);
                       // QQQ - XXX
            n = _group->patch()->xform() * n;
//             _geoms.add(WORLD::show(p,n,1.1,6));
         }

         if (rightk != -1)
         {
            query[0] = rightu;
            f = _mapping->find_face(query,bc);
            assert(f);
            f->bc2pos(bc,p);
            p = _group->patch()->xform() * p;
                       // QQQ - XXX - Change to use spline
            //f->bc2norm_blend(bc,n);
                       // QQQ - XXX //
            query_normal_spline(rightu,n);
                       // QQQ - XXX
            n = _group->patch()->xform() * n;
//             _geoms.add(WORLD::show(p,n,1.1,6));
         }
      }
      // QQQ - XXX - Change to use spline
   }

   if ( (leftk==-1) || (rightk==-1) )
      return false;
   else return true;

}


/////////////////////////////////////
// refine()
/////////////////////////////////////
double
HatchingPositionFree::refine(
   double  mid,  
   double  left, 
   double  right,
   double  offset,
   double  tol,
   CWvec&  dir)
{
//   UVpt query = _center;
   Wvec norm;
//   Bface *f;

   double leftval, midval, rightval;

   double leftmid,         rightmid;
   double leftmidval,      rightmidval;

   //Remap the dot products to the form
   //used in this refining algorithm
   //so that we're always looking to 
   //minimize

   // QQQ - XXX - Change to use spline
   //query[0] = mid;
   //f = _mapping->find_face(query,bc);
   //assert(f);
   //f->bc2norm_blend(bc,norm);
   //midval = fabs(offset - norm*dir);

   //query[0] = left;
   //f = _mapping->find_face(query,bc);
   //if (f)
   //{
   //   f->bc2norm_blend(bc,norm);
   //   leftval = fabs(offset - norm*dir);
   //}
   //else leftval = 666;

   //query[0] = right;
   //f = _mapping->find_face(query,bc);
   //if (f)
   //{
   //   f->bc2norm_blend(bc,norm);
   //   rightval = fabs(offset - norm*dir);
   //}
   //else rightval = 666;
   // QQQ - XXX //
   bool ret = query_normal_spline(mid,norm);
   assert(ret);
   midval = fabs(offset - norm*dir);

   if (query_normal_spline(left,norm))
      leftval = fabs(offset - norm*dir);
   else leftval = 666;

   if (query_normal_spline(right,norm))
      rightval = fabs(offset - norm*dir);
   else rightval = 666;
   // QQQ - XXX //

   while (fabs(right-left)>tol)
      {
         //Get the midpoints of the left and right regions

         if (left<=mid)
            leftmid = (left + mid)/2.0;
         else
            {
               leftmid = (left + mid + _mapping->span_u())/2.0;
               if (leftmid>=_mapping->max_u())
                  leftmid -= _mapping->span_u();
            }

         if (mid<=right)
            rightmid = (right + mid)/2.0;
         else
            {
               rightmid = (right + mid + _mapping->span_u())/2.0;
               if (rightmid>=_mapping->max_u())
                  rightmid -= _mapping->span_u();
            }

         //Query the objective function at both midpoints

         // QQQ - XXX - Change to use spline
         //query[0] = leftmid;
         //f = _mapping->find_face(query,bc);
         //if (f)
         //{
         //   f->bc2norm_blend(bc,norm);
         //   leftmidval = fabs(offset - norm*dir);
         //}
         //else leftmidval = 666;

         //query[0] = rightmid;
         //f = _mapping->find_face(query,bc);
         //if(f)
         //{
                                //f->bc2norm_blend(bc,norm);
                                //rightmidval = fabs(offset - norm*dir);
         //}
         //else rightmidval = 666;
         // QQQ - XXX //
         if (query_normal_spline(leftmid,norm))
            leftmidval = fabs(offset - norm*dir);
         else leftmidval = 666;

         if (query_normal_spline(rightmid,norm))
            rightmidval = fabs(offset - norm*dir);
         else rightmidval = 666;
         // QQQ - XXX //

         //Update the region

         if (leftmidval < midval)
            {
               right           = mid;
               rightval = midval;

               mid             = leftmid;
               midval  = leftmidval;
            }
         else if (rightmidval < midval)
            {
               left            = mid;
               leftval = midval;

               mid             = rightmid;
               midval  = rightmidval;
            }
         else
            {
               bool bad = true;

               if (!((leftval==666) && (leftmidval!=666)))
                  {
                     left            = leftmid;
                     leftval = leftmidval;
                     bad = false;
                  }

               if (!((rightval==666) && (rightmidval!=666)))
                  {
                     right           = rightmid;
                     rightval        = rightmidval;
                     bad = false;
                  }
               err_mesg_cond(bad, ERR_LEV_ERROR, "HatchingPositionFree::refine() - The bad thing happened...");
            }
      }

   return mid;

}

/////////////////////////////////////
// update()
/////////////////////////////////////
void
HatchingPositionFree::update()
{
   int k;

   cache_search_normals();

   //The 'dark light' direction in model space
   //must be updates each frame

   _curr_direction = (_group->patch()->inv_xform() * 
                      VIEW::peek_cam()->data()->xform(0).inverse() * _direction).normalized();

   cache_search_dots();

   int num = _search_normals.num();

   //Swap placement array to keep old ones
   //and overwrite the old, old ones
   ARRAY<HatchingPlacement>*       foo = _old_placements;
   _old_placements = _curr_placements;
   _curr_placements = foo;

   if (!_curr_placements)
      _curr_placements = new ARRAY<HatchingPlacement>;
   else
      _curr_placements->clear();

   HatchingPlacement placement;

   //XXX - Check for holes!!!!!!!!!
   if (_mapping->wrap_u())
      {
         for (k=0; k<(num-1); k++)
            {
               if (k<2)
                  {
                     if ( 
                        (_search_dots[(num-1 + k-1)%(num-1)]    <= _search_dots[k]) &&
                        (_search_dots[k]                >= _search_dots[k+1])  )
                        {
                           if (    (_search_dots[(num-1 + k-1)%(num-1)]!=666) &&
                                   (_search_dots[k] != 666) &&
                                   (_search_dots[k+1] != 666) )
                              {
                                 placement.maxk = k;
                                 _curr_placements->add(placement);
                              }
                        }
                  }
               else if (k>(num-4))
                  {
                     if (    (_search_dots[k-1]      <= _search_dots[k]) &&
                             (_search_dots[k]                >= _search_dots[(k+1)%(num-1)]) )
                        {
                           if (    (_search_dots[(k+1)%(num-1)]!=666) &&
                                   (_search_dots[k] != 666) &&
                                   (_search_dots[k-1] != 666) )
                              {
                                 placement.maxk = k;
                                 _curr_placements->add(placement);
                              }
                        }
                  }
               else 
                  {
                     if (    (_search_dots[k-1]      <= _search_dots[k]) &&
                             (_search_dots[k]                >= _search_dots[k+1]) )
                        {
                           if (    (_search_dots[k-1] !=666) &&
                                   (_search_dots[k] != 666) &&
                                   (_search_dots[k+1] != 666) )
                              {
                                 placement.maxk = k;
                                 _curr_placements->add(placement);
                              }
                        }
                  }
            }
      }
   else
      {
         for (k=0; k<=(num-1); k++)
            {
               if ( (k>1) && (k<(num-2)) )
                  {
                     if (    (_search_dots[k-2] <= _search_dots[k-1]) && 
                             (_search_dots[k-1] <= _search_dots[k]) &&
                             (_search_dots[k] >= _search_dots[k+1]) && 
                             (_search_dots[k+1] >= _search_dots[k+2]))
                        {
                           if (    (_search_dots[k-2]!=666) &&
                                   (_search_dots[k-1] != 666) &&
                                   (_search_dots[k] != 666) &&
                                   (_search_dots[k+1] != 666) &&
                                   (_search_dots[k+2] != 666) )
                              {
                                 placement.maxk = k;
                                 _curr_placements->add(placement);
                              }
                        }
                  }
               else 
                  {
                     if (  ((k<1) || ( (_search_dots[k-2] <= _search_dots[k-1]) &&
                                       (_search_dots[k-2] != 666) && 
                                       (_search_dots[k-1] != 666) ) ) &&
                           ((k<2) || ( (_search_dots[k-1] <= _search_dots[k]) &&
                                       (_search_dots[k-1] != 666) && 
                                       (_search_dots[k] != 666) ) ) &&
                           ((k>(num-2)) || ( (_search_dots[k] >= _search_dots[k+1])        &&
                                             (_search_dots[k] != 666) && 
                                             (_search_dots[k+1] != 666) ) )  &&
                           ((k>(num-3)) || ( (_search_dots[k+1] >= _search_dots[k+2])      &&
                                             (_search_dots[k+1] != 666) && 
                                             (_search_dots[k+2] != 666) ) ) )
                        {
                           placement.maxk = k;
                           _curr_placements->add(placement);
                        }
                  }
            }
      }

   //Wipe any debug objects
   while (_geoms.num()>0)
      WORLD::destroy(_geoms.pop());

   //Keep track of the highest max, so it
   //can be used to deterime how relavent
   //other maxima are...
   double absmax = -2.0;

   //Find the refined maxima
   for (k=0; k<_curr_placements->num(); k++)
      {
         (*_curr_placements)[k].good = refine_placement_maximum((*_curr_placements)[k].maxk,
                                                                (*_curr_placements)[k].maxu,(*_curr_placements)[k].maxdot);
         if (((*_curr_placements)[k].good) && 
             ((*_curr_placements)[k].maxdot > absmax))
            absmax = (*_curr_placements)[k].maxdot;
      }

   //Find the extents
   for (k=0; k<_curr_placements->num(); k++)
      {
         if ((*_curr_placements)[k].good)
            (*_curr_placements)[k].good = refine_placement_extent(absmax,
                                                                  (*_curr_placements)[k].maxu,(*_curr_placements)[k].maxdot,
                                                                  (*_curr_placements)[k].leftu,(*_curr_placements)[k].rightu);
      }
/*
  //Merge them
  for (k=1; k<_curr_placements->num(); k++)
  {
  if ((*_curr_placements)[k].good && (*_curr_placements)[k-1].good)
  {
  double diff = fabs((*_curr_placements)[k].leftu - (*_curr_placements)[k-1].rightu);
  if (diff < 1e-4)
  {
                                //Change leftu to leftu of group on left that we're merging with
                                (*_curr_placements)[k].leftu = (*_curr_placements)[k-1].leftu;
                                (*_curr_placements)[k].maxdot = 
                                max( (*_curr_placements)[k-1].maxdot, (*_curr_placements)[k].maxdot);
                                //Invalidate the other placement
                                (*_curr_placements)[k-1].good = false;
                                }
                                }
                                }
                                //If the region wraps, we need to check if there's
                                //a group to the right of [num-1] that's should absorb [num-1]
                                if ((*_curr_placements).last().good)
                                {
                                for (k=0; k<_curr_placements->num()-1; k++)
                                {
                                if ((*_curr_placements)[k].good)
                                {
                                double diff = fabs((*_curr_placements)[k].leftu - (*_curr_placements).last().rightu);
                                if (diff < 1e-4)
                                {
                                //Change leftu to leftu of group on left that we're merging with
                                (*_curr_placements)[k].leftu = (*_curr_placements).last().leftu;
                                (*_curr_placements)[k].maxdot = 
                                max( (*_curr_placements).last().maxdot, (*_curr_placements)[k].maxdot);
                                //Invalidate the other placement
                                (*_curr_placements).last().good = false;
                                }
                                break;
                                }
                                }
                                }
*/
   while (_curr_placements->num() > _group->instances().num())
      _group->instances().add(_group->base_instance()->clone());

   double maxu;
   double leftu, rightu;
   double maxdot;

   for (k=0; k < _group->instances().num(); k++)   
      {
         if (k >= _curr_placements->num())
            {
               _group->instances()[k]->position()->_weight = 0;
            }
         else
            {
               if (!(*_curr_placements)[k].good)
                  {
                     _group->instances()[k]->position()->_weight = 0;
                  }
               else
                  {
                     maxu = (*_curr_placements)[k].maxu;
                     leftu = (*_curr_placements)[k].leftu;
                     rightu = (*_curr_placements)[k].rightu;
                     maxdot = (*_curr_placements)[k].maxdot;
                                
                     double newmid;
                     double newwid;
                     double oldwid;

                     if (leftu<rightu)
                        {
                           newmid = ((leftu+rightu)/2.0);
                           newwid = (rightu - leftu);
                        }
                     else
                        {
                           assert (_mapping->wrap_u());
                           newmid = ((leftu + rightu + _mapping->span_u())/2.0);
                           if (newmid > _mapping->span_u())
                              newmid -= _mapping->span_u();
                           newwid = rightu + _mapping->span_u() - leftu;
                        }
                
                     assert(newwid>0);

                     _group->instances()[k]->position()->_shift[0] = newmid - _center[0];

                     if (_lower_left[0] <= _upper_right[0])
                        oldwid = _upper_right[0] - _lower_left[0];
                     else
                        {
                           assert(_mapping->wrap_u());
                           oldwid = _upper_right[0] + _mapping->span_u() - _lower_left[0];
                        }

                     double weight = (maxdot/absmax);
                     if (weight < 0.5) 
                        weight = 0;
                     else if (weight < 0.75)
                        weight = pow((weight-0.5)/0.25,0.33);
                     else
                        weight = 1;

                     _group->instances()[k]->position()->_weight = weight;

                     assert(oldwid>0);
                     _group->instances()[k]->position()->_scale[0] = weight*newwid/oldwid;
                     assert(maxdot > 0);
                     assert(absmax > 0);

                  }
            }
      }
        
}

/*****************************************************************
 * HatchingPositionFreeInst
 *****************************************************************/

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingPositionFreeInst::HatchingPositionFreeInst(
   UVMapping *m,
   HatchingPositionFree *p) :
   _mapping(m),
   _position(p),
   _curr_weight(1.0),
   _scale(UVpt(1,1)),
   _shift(UVvec(0,0)),
   _weight(1.0)
{
   assert(m);
   assert(p);
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingPositionFreeInst::~HatchingPositionFreeInst()
{
        
}

/////////////////////////////////////
// query()
/////////////////////////////////////
bool
HatchingPositionFreeInst::query(CUVpt &pt)
{

   if (_curr_upper_right[0] > _curr_lower_left[0])
      if (_curr_upper_right[1] > _curr_lower_left[1])
         return ((pt[0]>=_curr_lower_left[0]) && (pt[0]<=_curr_upper_right[0]) 
                 && (pt[1]>=_curr_lower_left[1]) && (pt[1]<=_curr_upper_right[1]));
      else
         return ((pt[0]>=_curr_lower_left[0]) && (pt[0]<=_curr_upper_right[0]) 
                 && !((pt[1]<_curr_lower_left[1]) && (pt[1]>_curr_upper_right[1])));
   else
      if (_curr_upper_right[1] > _curr_lower_left[1])
         return (!((pt[0]<_curr_lower_left[0]) && (pt[0]>_curr_upper_right[0]))
                 && (pt[1]>=_curr_lower_left[1]) && (pt[1]<=_curr_upper_right[1]));
      else
         return (!((pt[0]<_curr_lower_left[0]) && (pt[0]>_curr_upper_right[0]))
                 && !((pt[1]<_curr_lower_left[1]) && (pt[1]>_curr_upper_right[1])));

}

/////////////////////////////////////
// query()
/////////////////////////////////////
bool
HatchingPositionFreeInst::query(CUVpt &pt, CUVpt &tpt )
{

   UVpt query = pt;
   UVpt target = tpt;

   if (_curr_upper_right[0] > _curr_lower_left[0])
      {
         if (_curr_upper_right[1] > _curr_lower_left[1])
            {
               if ((pt[0]>=_curr_lower_left[0]) && (pt[0]<=_curr_upper_right[0]) 
                   && (pt[1]>=_curr_lower_left[1]) && (pt[1]<=_curr_upper_right[1]))
                  {
                                //query = pt;
                                //target = dpt;
                  }
               else return false;

            }
         else
            {
               if ((pt[0]>=_curr_lower_left[0]) && (pt[0]<=_curr_upper_right[0]) 
                   && !((pt[1]<_curr_lower_left[1]) && (pt[1]>_curr_upper_right[1])))
                  {
                     if (query[1] <= _curr_upper_right[1]) query[1] += _mapping->span_v();
                     if (target[1] <= _curr_upper_right[1]) target[1] += _mapping->span_v();
                  }
               else return false;

            }
      }
   else
      {
         if (_curr_upper_right[1] > _curr_lower_left[1])
            {
               if (!((pt[0]<_curr_lower_left[0]) && (pt[0]>_curr_upper_right[0]))
                   && (pt[1]>=_curr_lower_left[1]) && (pt[1]<=_curr_upper_right[1]))
                  {
                     if (query[0] <= _curr_upper_right[0]) query[0] += _mapping->span_u();
                     if (target[0] <= _curr_upper_right[0]) target[0] += _mapping->span_u();
                  }
               else return false;

            }
         else
            {
               if (!((pt[0]<_curr_lower_left[0]) && (pt[0]>_curr_upper_right[0]))
                   && !((pt[1]<_curr_lower_left[1]) && (pt[1]>_curr_upper_right[1])))
                  {
                     if (query[1] <= _curr_upper_right[1]) query[1] += _mapping->span_v();
                     if (target[1] <= _curr_upper_right[1]) target[1] += _mapping->span_v();
                     if (query[0] <= _curr_upper_right[0]) query[0] += _mapping->span_u();
                     if (target[0] <= _curr_upper_right[0]) target[0] += _mapping->span_u();
                  }
               else return false;

            }
      }

   double h = _curr_upper_right[1] - _curr_lower_left[1];
   double w = _curr_upper_right[0] - _curr_lower_left[0];
   if (_curr_upper_right[1] < _curr_lower_left[1]) h += _mapping->span_v();
   if (_curr_upper_right[0] < _curr_lower_left[0]) w += _mapping->span_u();

   double d = (h*h+w*w);

   double dist_ratio = (query-target).length_sqrd()/d;

   //Ratio is the square of percentage of
   //the bounding box's diagonal
   if (dist_ratio > 0.005)
      {

         return false;
      }
   else
      return true;
}

/////////////////////////////////////
// update()
/////////////////////////////////////
//
// - Updates anything that needs maintenance
//   when the transform is changed
//
/////////////////////////////////////
void
HatchingPositionFreeInst::update()
{

   //XXX - Likely to change
   _curr_weight = _weight * _position->group()->prototype()->get_alpha();

   //XXX - Likely to change
   transform(_position->lower_left(),_curr_lower_left);
   transform(_position->upper_right(),_curr_upper_right);


}

/////////////////////////////////////
// transform()
/////////////////////////////////////
//
// - Applies current transform to the
//   given point and stores in other
//   variable
// - Source and destination can be the same
//
/////////////////////////////////////
void
HatchingPositionFreeInst::transform(CUVpt& pt, UVpt& transpt)
{

   //XXX - The transform is likely to change

   //If the hatches cross a wrapping, then we must be careful
   //with the scaling...

   if (_position->upper_right()[0] < _position->lower_left()[0])
      {
         assert(_mapping->wrap_u());

         transpt =  pt;

         if (transpt[0] <= _position->upper_right()[0])
            transpt[0] += _mapping->span_u();

         transpt -= _position->center();

         if (_position->center()[0] <= _position->upper_right()[0])
            transpt[0] -= _mapping->span_u();

         transpt = transpt * _scale;

         transpt += _position->center();

         if (_position->center()[0] <= _position->upper_right()[0])
            transpt[0] += _mapping->span_u();

      }
   else
      {
         transpt =  pt;
         transpt -= _position->center();
         transpt = transpt * _scale;
         transpt += _position->center();
      }
   transpt += _shift;

   _mapping->apply_wrap(transpt);
}



// End of HatchingGroupFree.C
