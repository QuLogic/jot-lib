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
// HatchingCollection
////////////////////////////////////////////
//
// -Holds a collection of hatching groups
// -HatchingPen talks to this object
// -Is held by the texture
//
////////////////////////////////////////////



#include "npr/hatching_collection.H"
#include "npr/hatching_group_free.H"
#include "npr/hatching_group_fixed.H"

#include "base_jotapp/base_jotapp.H"
#include "manip/cam_pz.H"
#include "manip/cam_fp.H"
#include "manip/cam_edit.H"

using mlib::CWtransf;
using mlib::CNDCpt;

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingCollection::_hc_tags = 0;


/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingCollection::tags() const
{
   if (!_hc_tags) {
      _hc_tags = new TAGlist;
      *_hc_tags += new TAG_meth<HatchingCollection>(
         "hatching_group",
         &HatchingCollection::put_hatching_groups,
         &HatchingCollection::get_hatching_group,
         1);
   }
   return *_hc_tags;
}

/////////////////////////////////////
// put_hatching_group()
/////////////////////////////////////
//
// -Actually writes out all groups
//
/////////////////////////////////////
void
HatchingCollection::put_hatching_groups(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingCollection::put_hatching_groups()"); 

   int i;
        
   for (i=0;i<num();i++)
      {
         if ((*this)[i]->is_complete())
            {
               d.id();
               *d << (*this)[i]->type();
               (*this)[i]->format(*d);
               d.end_id();
            }
      }
}

/////////////////////////////////////
// get_hatching_group()
/////////////////////////////////////
void
HatchingCollection::get_hatching_group(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingCollection::get_hatching_group()"); 

   int type_id;
   *d >> type_id;

   str_ptr str;
   *d >> str;      

   HatchingGroup *hg = add_group(type_id);

   if (!hg) 
   {
      err_mesg(ERR_LEV_ERROR, "HatchingCollection::get_hatching_group() - Not valid HatchingGroup id# (or failed to create): '%d'!!", type_id); 
      return;
   }

   if ((str != hg->class_name())) 
   {
      // XXX - should throw away stuff from unknown obj?
      err_mesg(ERR_LEV_ERROR, 
         "HatchingCollection::get_hatching_group() - Not valid class name: '%s' for given id #: '%d' which is a '%s'!", 
         **str, type_id, **hg->class_name()); 
      delete_group(hg);
      return;
   }

   hg->decode(*d);

   if (!hg->is_complete())
   {
      err_mesg(ERR_LEV_ERROR, "HatchingCollection::get_hatching_group() - ****Not complete. Destroying!****"); 
      delete_group(hg);
   }

}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingCollection::HatchingCollection(Patch *p) : 
   _xform_flag(false)
{
   set_patch(p);
}

/////////////////////////////////////
// set_patch()
/////////////////////////////////////
void
HatchingCollection::set_patch(Patch *p) 
{
   _patch = p;

   if (p)
   {
      assert(p->mesh());
      subscribe_mesh_notifications(p->mesh());
   }
}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

HatchingCollection::~HatchingCollection()
{

   int k;

   if (_patch)
   {
      unsubscribe_mesh_notifications(_patch->mesh());
   }

   for (k=0; k<num(); k++)
   {
      delete array()[k];
   }
   
}

/////////////////////////////////////
// add_group()
/////////////////////////////////////

bool
HatchingCollection::add_group(HatchingGroup* g)
{
   if (!(g && g->patch() == _patch))
      return false;
   add(g);
   return true;
}

HatchingGroup * 
HatchingCollection::add_group(int t)
{

   HatchingGroup *hg;

   switch(t)
      {
       case HatchingGroup::FIXED_HATCHING_GROUP: 
         hg = new HatchingGroupFixed(_patch); 
         break;
       case HatchingGroup::FREE_HATCHING_GROUP: 
         hg = new HatchingGroupFree(_patch); 
         break;
       default: 
         hg = 0;
      }

   if (hg)
      add(hg);

   return hg;

}

void
HatchingCollection::remove_group(HatchingGroup* g)
{
   rem(g);
}

/////////////////////////////////////
// next_group()
/////////////////////////////////////

HatchingGroup * 
HatchingCollection::next_group(CNDCpt &pt, HatchingGroup *hg)
{

   ARRAY<HatchingGroup*> hit;
   int k;

   for (k=0; k<num(); k++)
      {
         if (array()[k]->query_pick(pt)) hit += array()[k];
      }

   if (hit.num()==0) return 0;
   else if (hit.num()==1) return hit.first();
   else if (hit.contains(hg)) return hit[(hit.get_index(hg)+1)%hit.num()];
   else return hit.first();


}

/////////////////////////////////////
// delete_group()
/////////////////////////////////////

bool 
HatchingCollection::delete_group(HatchingGroup *hg)
{
   if (rem(hg))
   {
      delete hg;
      return true;
   }
   return false;

}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int 
HatchingCollection::draw(CVIEWptr& v)
{
   int k,cnt=0;

   static bool init = false;

   if (!init) 
   {
      err_mesg(ERR_LEV_SPAM, "HatchingCollection::draw() - Adding collection to draw_int UPobs list."); 
      BaseJOTapp::instance()->window(0)->_cam1->add_up_obs(this);
	  BaseJOTapp::instance()->window(0)->_cam2->add_up_obs(this);
      init = true;
   }

   for (k=0;k<num();k++) 
   {
      cnt += array()[k]->draw(v);
   }

   return cnt;

}
/////////////////////////////////////
// reset()
/////////////////////////////////////
void
HatchingCollection::reset(int /* is_reset */)
{
   int k;
        
   // XXX Env. var. for this
   for (k=0;k<num();k++) 
   {
      array()[k]->kill_animation();
   }

}

/////////////////////////////////////
// notify_xform()
/////////////////////////////////////
void
HatchingCollection::notify_xform(BMESH *m, CWtransf &t, CMOD& mod)
{
   err_mesg(ERR_LEV_SPAM, "HatchingCollection::notify_xform"); 

   assert(m == _patch->mesh());

   assert(!_xform_flag);

   _xform_flag = true;

   for (int k=0;k<num();k++) 
      array()[k]->notify_xform(m,t, mod);

}

/////////////////////////////////////
// notify_change()
/////////////////////////////////////
void
HatchingCollection::notify_change(BMESH *m, BMESH::change_t chg)
{
   assert(m == _patch->mesh());

   switch(chg)
   {
      case BMESH::VERT_POSITIONS_CHANGED:
         if (_xform_flag)
         {
            //Skip this notificiation if it follows
            //an xform notification, as they're redundant...
            err_mesg(ERR_LEV_INFO, "HatchingCollection::notify_change - VERT_POSITIONS_CHANGED - Ignored..."); 
            _xform_flag = false;
            return;
         }
         else
         {
            err_mesg(ERR_LEV_SPAM, "HatchingCollection::notify_change - VERT_POSITIONS_CHANGED"); 
         }
      break;
      case BMESH::TOPOLOGY_CHANGED:
         err_mesg(ERR_LEV_INFO, "HatchingCollection::notify_change - TOPOLOGY_CHANGED");
      break;
      case BMESH::PATCHES_CHANGED:
         err_mesg(ERR_LEV_INFO, "HatchingCollection::notify_change - PATCHES_CHANGED");
      break;
      case BMESH::TRIANGULATION_CHANGED:
         err_mesg(ERR_LEV_INFO, "HatchingCollection::notify_change - TRIANGULATION_CHANGED");
      break;
      case BMESH::VERT_COLORS_CHANGED:
         err_mesg(ERR_LEV_INFO, "HatchingCollection::notify_change - VERT_COLORS_CHANGED");
      break;
      case BMESH::RENDERING_CHANGED:
         err_mesg(ERR_LEV_INFO, "HatchingCollection::notify_change - RENDERING_CHANGED");
      break;
      case BMESH::NO_CHANGE:
         err_mesg(ERR_LEV_INFO, "HatchingCollection::notify_change - NO_CHANGE");
      break;
      default:
         err_mesg(ERR_LEV_WARN, "HatchingCollection::notify_change - UNKNOWN TYPE!!!!!!!");
   };

   for (int k=0;k<num();k++) 
      array()[k]->notify_change(m,chg);

}
