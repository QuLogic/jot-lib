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
// HatchingGroup
////////////////////////////////////////////
//
// -Virtual base class of hatching groups
// -Bare essentials of the interface
// -Actual derived classes also subclass
//  HatchingGroupBase which supplies
//  additional common internal methods
// -Provides the constructor via type index
//
////////////////////////////////////////////



#include "npr/hatching_group.H"

/*****************************************************************
 * HatchingGroupParams
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingGroupParams::_hp_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingGroupParams::tags() const
{
   if (!_hp_tags) {
      _hp_tags = new TAGlist;
   
      *_hp_tags += new TAG_val<HatchingGroupParams,float>(
         "anim_lo_thresh",
         &HatchingGroupParams::anim_lo_thresh_);
      *_hp_tags += new TAG_val<HatchingGroupParams,float>(
         "anim_hi_thresh",
         &HatchingGroupParams::anim_hi_thresh_);
      *_hp_tags += new TAG_val<HatchingGroupParams,float>(
         "anim_lo_width",
         &HatchingGroupParams::anim_lo_width_);
      *_hp_tags += new TAG_val<HatchingGroupParams,float>(
         "anim_hi_width",
         &HatchingGroupParams::anim_hi_width_);
      *_hp_tags += new TAG_val<HatchingGroupParams,float>(
         "anim_global_scale",
         &HatchingGroupParams::anim_global_scale_);
      *_hp_tags += new TAG_val<HatchingGroupParams,float>(
         "anim_limit_scale",
         &HatchingGroupParams::anim_limit_scale_);
      *_hp_tags += new TAG_val<HatchingGroupParams,float>(
         "anim_trans_time",
         &HatchingGroupParams::anim_trans_time_);
      *_hp_tags += new TAG_val<HatchingGroupParams,float>(
         "anim_max_lod",
         &HatchingGroupParams::anim_max_lod_);
      *_hp_tags += new TAG_val<HatchingGroupParams,int>(
         "anim_style",
         &HatchingGroupParams::anim_style_);

      *_hp_tags += new TAG_meth<HatchingGroupParams>(
         "hatching_stroke",
         &HatchingGroupParams::put_base_stroke,
         &HatchingGroupParams::get_base_stroke,
         1);
      
        
   }
   return *_hp_tags;
}

/////////////////////////////////////
// put_base_stroke()
/////////////////////////////////////
void
HatchingGroupParams::put_base_stroke(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroup::put_base_stroke()"); 

   d.id();
   _stroke.format(*d);
   d.end_id();

}

/////////////////////////////////////
// get_base_stroke()
/////////////////////////////////////
void
HatchingGroupParams::get_base_stroke(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroup::get_base_stroke()"); 

   //Grab the class name... should be HatchingStroke
   str_ptr str;
   *d >> str;      

   if ((str != BaseStroke::static_name())) 
   {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "HatchingGroupParams::get_base_stroke() - Not 'BaseStroke': '%s'!", **str); 
      return;
   }

   _stroke.decode(*d);

}

/*****************************************************************
 * HatchingGroup
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingGroup::_hg_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
//
// -The base class handles IO for params
// -Subclasses take care of the rest
//
////////////////////////////////////
CTAGlist &
HatchingGroup::tags() const
{
   if (!_hg_tags) {
      _hg_tags = new TAGlist;
      *_hg_tags += new TAG_meth<HatchingGroup>(
         "params",
         &HatchingGroup::put_hatching_group_params,
         &HatchingGroup::get_hatching_group_params,
         0);
   }

   return *_hg_tags;
}

/////////////////////////////////////
// put_hatching_group_params()
/////////////////////////////////////
void
HatchingGroup::put_hatching_group_params(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroup::put_hatching_group_params()"); 

   d.id();
   _params.format(*d);
   d.end_id();
}

/////////////////////////////////////
// get_hatching_group_params()
/////////////////////////////////////
void
HatchingGroup::get_hatching_group_params(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroup::get_hatching_group_params()"); 

   //Grab the class name... should be HatchingCollection
   str_ptr str;
   *d >> str;      

   if ((str != HatchingGroupParams::static_name())) {
      // XXX - should throw away stuff from unknown obj
      err_mesg(ERR_LEV_ERROR, "HatchingGroupParams::get_hatching_group_params() - Not 'HatchingGroupParams': '%s'!", **str); 
      return;
   }

   _params.decode(*d);

   update_prototype();

}

/////////////////////////////////////
// update_prototype()
/////////////////////////////////////
void    
HatchingGroup::update_prototype()
{
   _prototype.copy(_params.stroke());
}






 

