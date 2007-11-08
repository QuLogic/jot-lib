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
// HatchingGroupFixed
////////////////////////////////////////////
//
// -'Fixed' Hatching Group class
// -Implements HatchingGroup abstract class
// -Subclasses HatchingGroupBase which provides
//  common hatch group functionality
//
////////////////////////////////////////////



#define INTERPOLATION_RANDOM_FACTOR 0.15

#include "geom/world.H"
#include "mesh/lmesh.H"
#include "npr/hatching_group_fixed.H"
#include "std/config.H"

using namespace mlib;

static bool debug = Config::get_var_bool("DEBUG_HATCH_FIXED",false,true);

/*****************************************************************
 * HatchingGroupFixed
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingGroupFixed::_hgf_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingGroupFixed::tags() const
{
   if (!_hgf_tags) {
      _hgf_tags = new TAGlist;
      *_hgf_tags += HatchingGroup::tags();
      *_hgf_tags += new TAG_meth<HatchingGroupFixed>(
         "level",
         &HatchingGroupFixed::put_levels,
         &HatchingGroupFixed::get_level,
         1);
      *_hgf_tags += new TAG_meth<HatchingGroupFixed>(
         "visibility",
         &HatchingGroupFixed::put_visibility,
         &HatchingGroupFixed::get_visibility,
         0);
      *_hgf_tags += new TAG_meth<HatchingGroupFixed>(
         "backbone",
         &HatchingGroupFixed::put_backbone,
         &HatchingGroupFixed::get_backbone,
         1);
      *_hgf_tags += new TAG_meth<HatchingGroupFixed>(
         "hatch",
         &HatchingGroupFixed::put_hatchs,
         &HatchingGroupFixed::get_hatch,
         1);


   }
   return *_hgf_tags;
}

/////////////////////////////////////
// decode()
/////////////////////////////////////
STDdstream&
HatchingGroupFixed::decode(STDdstream &ds)
{
   STDdstream &rds  = DATA_ITEM::decode(ds);
   
   assert(_num_base_levels);

   if (_level[0]->num() > 0)
   {
      if (_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT)
         assert(_backbone);

      _complete = true;
   }
   //else this was loading an old file and we can't
   //get the hatches, so we fail to complete the
   //gruop and hatching collection will punt us!

   return rds;
}

/**** Next two tags are only around so we gracefully die on old files ***/
/////////////////////////////////////
// put_hatchs()
/////////////////////////////////////
void
HatchingGroupFixed::put_hatchs(TAGformat&) const
{
// *** LEGACY ***
}

/////////////////////////////////////
// get_hatch()
/////////////////////////////////////
void
HatchingGroupFixed::get_hatch(TAGformat &d)
{
   err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::get_hatch() - ****** OLD FILE FORMAT - DUMPING ******"); 

   str_ptr str;
   *d >> str;      

   if ((str != HatchingHatchFixed::static_name())) 
   {
      // XXX - should throw away stuff from unknown obj
	   err_mesg(ERR_LEV_ERROR, "HatchingGroupFixed::get_hatch() - Not 'HatchingHatchFixed': '%s'!!", **str); 
		return;
   }

	HatchingHatchFixed *hhf = new HatchingHatchFixed(); assert(hhf);
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
HatchingGroupFixed::put_levels(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::put_levels()"); 

   int i;
   assert(_num_base_levels);
   assert(_level.num() >= _num_base_levels);

   for (i=0; i<num_base_levels(); i++)
   {
      d.id();
      base_level(i)->format(*d);
      d.end_id();
   }
}

/////////////////////////////////////
// get_level()
/////////////////////////////////////
void
HatchingGroupFixed::get_level(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::get_level()"); 

   assert(_level.num() > 0);

   //Grab the class name... should be HatchingLevelBase
   str_ptr str;
   *d >> str;      

   if ((str != HatchingLevelBase::static_name())) {
      // XXX - should throw away stuff from unknown obj

      err_mesg(ERR_LEV_ERROR, "HatchingGroupFixed::get_level() - 'Not HatchingLevelBase': '%s'!!", **str); 
      return;
   }

   if (!(base_level(num_base_levels()-1)->pix_size()))
   {
      assert(_level.num() == 1);
      assert(num_base_levels() == 1);
   }
   else
   {
      add_base_level();
   }

   base_level(num_base_levels()-1)->decode(*d);

   assert(base_level(num_base_levels()-1)->pix_size() != 0.0);

}

/////////////////////////////////////
// put_visibility()
/////////////////////////////////////
void
HatchingGroupFixed::put_visibility(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::put_visibility()"); 

   BMESH *m = _patch->mesh();
   if (LMESH::isa(m))
      m = ((LMESH*)m)->cur_mesh();

   int k;
   ARRAY<int> indices;
   CBface_list& faces = m->faces();
        
   for (k=0; k< faces.num(); k++)
      {
         HatchingSimplexDataFixed *hsdf = HatchingSimplexDataFixed::find(faces[k]);
         if (hsdf) 
            {
               if(hsdf->hack_exists(this))
                  indices += faces[k]->index();
            }
      }
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::put_visibility() - Stored %d tri indices.", indices.num()); 

   d.id();
   *d << indices;
   d.end_id();
}

/////////////////////////////////////
// get_visibility()
/////////////////////////////////////
void
HatchingGroupFixed::get_visibility(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::get_visibility()"); 

   BMESH *m = _patch->mesh();
   if (LMESH::isa(m))
      m = ((LMESH*)m)->cur_mesh();

   int k, ctr=0;
   ARRAY<int> indices;
   CBface_list& faces = m->faces();

   *d >> indices;

   for (k=0; k<indices.num(); k++)
      {
         HatchingSimplexDataFixed *hsdf =
            HatchingSimplexDataFixed::find(faces[indices[k]]);
         if (!hsdf) 
            {
               hsdf = new HatchingSimplexDataFixed(faces[indices[k]]);
               ctr++;
            }
         hsdf->add(this);
      }

   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::get_visibility() - Flagged %d tris and added %d new simplex data.", indices.num(), ctr); 


}

/////////////////////////////////////
// put_backbone()
/////////////////////////////////////
void
HatchingGroupFixed::put_backbone(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::put_backbone()"); 

   if (_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT)
      assert(_backbone);
   else
      return;

   d.id();
   _backbone->format(*d);
   d.end_id();
}

/////////////////////////////////////
// get_backbone()
/////////////////////////////////////
void
HatchingGroupFixed::get_backbone(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::get_backbone()"); 

   assert(_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT);

   //Grab the class name... should be HatchingBackboneFixed
   str_ptr str;
   *d >> str;      

   if ((str != HatchingBackboneFixed::static_name())) 
   {
      // XXX - should throw away stuff from unknown obj

      err_mesg(ERR_LEV_ERROR, "HatchingGroupFixed::get_backbone() - Not 'HatchingBackboneFixed': '%s'!!", **str); 
   }

   _backbone = new HatchingBackboneFixed(_patch);
   assert(_backbone);
   _backbone->decode(*d);

}


/////////////////////////////////////
// Constructor
/////////////////////////////////////

HatchingGroupFixed::HatchingGroupFixed(Patch *p) : 
   HatchingGroup(p), HatchingGroupBase(NULL) //NULL should be 'this'
{
   _group = this;

}

/////////////////////////////////////
// Destructor
/////////////////////////////////////

HatchingGroupFixed::~HatchingGroupFixed()
{
   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed::get_backbone()"); 

   if (_complete) 
   {
      clear_visibility();

      if (_backbone)
         delete(_backbone);
      _backbone = NULL;
   }

}
/////////////////////////////////////
// notify_change()
/////////////////////////////////////
void
HatchingGroupFixed::notify_change(BMESH *m, BMESH::change_t chg)
{
   assert(m == _patch->mesh());
      
   int l,h;

   if (chg == BMESH::VERT_POSITIONS_CHANGED)
   {
      //Kills of interpolated LODs
      trash_upper_levels();

      //Recompute hatch verts from the index/barycentric data
      for (l=0; l<num_base_levels(); l++)
         for (h=0; h<_level[l]->num(); h++)
            ((HatchingHatchFixed*)(*_level[l])[h])->notify_change(m,chg);

      //Recompute backbone
      if (_backbone)
         ((HatchingBackboneFixed*)_backbone)->notify_change(m,chg);

   }
   else
   {
      err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::notify_change() - Don't care."); 
   }

}
/////////////////////////////////////
// notify_xform()
/////////////////////////////////////
void
HatchingGroupFixed::notify_xform(BMESH *m, CWtransf &t, CMOD& mod)
{
   err_mesg(ERR_LEV_SPAM, "HatchingGroupFixed::notify_xform()"); 

   assert(m == _patch->mesh());

   int l,h;

   for (l=0; l<_level.num(); l++)
      for (h=0; h<_level[l]->num(); h++)
         ((HatchingHatchFixed*)(*_level[l])[h])->notify_xform(m,t, mod);

   if (_backbone)
      ((HatchingBackboneFixed*)_backbone)->notify_xform(m,t, mod);
   

}

/////////////////////////////////////
// select()
/////////////////////////////////////
void    
HatchingGroupFixed::select()
{
   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed::select()"); 

   HatchingGroupBase::select();
}
        
/////////////////////////////////////
// deselect()
/////////////////////////////////////
void
HatchingGroupFixed::deselect()
{
      err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed::deselect()"); 
   HatchingGroupBase::deselect();
}

/////////////////////////////////////
// level_sorting_comparison
/////////////////////////////////////
int compare_pix_size(const void *a, const void *b) 
{
   HatchingLevelBase **pta = (HatchingLevelBase **)a;   
   HatchingLevelBase **ptb = (HatchingLevelBase **)b;
   double diff = (**pta).pix_size() - (**ptb).pix_size();  
   return (diff>0)?(1):((diff<0)?(-1):(0));
}

/////////////////////////////////////
// complete()
/////////////////////////////////////
bool
HatchingGroupFixed::complete()
{
   int i;

   assert(!_complete);

   if (_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT)
   {
      assert(num_base_levels() == 1);
      assert(base_level(0)->pix_size() == 0.0);

      if (base_level(0)->num() < 2)
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::complete() - Not enough hatches (<2) to complete group."); 
         return false;
      }
      else if (store_visibility(base_level(0)))
      {
         HatchingBackboneFixed  *hbf = new HatchingBackboneFixed(_patch);
         assert(hbf);
         
         if (hbf->compute(base_level(0)))
         {
            _backbone = hbf;

            base_level(0)->set_pix_size(_group->patch()->mesh()->pix_size());

            _complete = true;

            return true;
         }
         else
         {
            clear_visibility();
            delete hbf;

            return false;
         }
      }
      else
      {
         return false;
      }
   }
   else //The two sloppy types
   {
      assert(num_base_levels() > 0);
      
      if (base_level(num_base_levels()-1)->pix_size() > 0.0)
      {
         // If the last editted level is complete, complete the group
         err_mesg(ERR_LEV_INFO, "HatchingGroupFixed::complete() - Completing group."); 
         for (i=0; i<num_base_levels(); i++)
         {
            if (!store_visibility(base_level(i)))
            {
               err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::complete() - Visibility failed in a level. Failing out.");
               clear_visibility();
               return false;
            }
         }
         //Sort levels by increasing mesh size
         level_sort(compare_pix_size);

         _complete = true;
         return true;
      }
      else
      {
         // Else try to complete the last editted level
         if (base_level(num_base_levels()-1)->num() >= 1)
         {
            // XXX - Recompute ndc_length of each hatch?
            
            base_level(num_base_levels()-1)->set_pix_size(_group->patch()->mesh()->pix_size());
            return true;
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::complete() - Not enough hatches (<1) to complete level.");
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
HatchingGroupFixed::undo_last()
{

   assert(!_complete);

   if (_params.anim_style() == HatchingGroup::STYLE_MODE_NEAT)
   {
      assert(num_base_levels() == 1);
      assert(base_level(0)->pix_size() == 0.0);

      if (base_level(0)->num() < 2)
      {
         assert(base_level(0)->num() > 0);
         err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::undo_last() - Only 1 hatch left... can't undo.");
         //Pen should notice this failure and delete the group...
         return false;
      }
      else
      {
         err_mesg(ERR_LEV_INFO, "HatchingGroupFixed::undo_last() - Popping off hatch.");
         WORLD::message("Popped hatch stroke.");
         HatchingHatchBase *hhb = base_level(0)->pop();
         assert(hhb);
         delete(hhb);
         return true;
      }
   }
   else //The two sloppy types
   {
      assert(num_base_levels() > 0);
      
      if (base_level(num_base_levels()-1)->pix_size() > 0.0)
      {
         // If the last editted level is complete, un-complete it
         err_mesg(ERR_LEV_INFO, "HatchingGroupFixed::undo_last() - Uncompleting level.");
         WORLD::message("Un-completed level.");
         base_level(num_base_levels()-1)->set_pix_size(0.0);
         return true;
      }
      else
      {
         if (base_level(num_base_levels()-1)->num() > 1)
         {
            err_mesg(ERR_LEV_INFO, "HatchingGroupFixed::undo_last() - Popping off hatch.");
            WORLD::message("Popped hatch stroke.");
            HatchingHatchBase *hhb = base_level(num_base_levels()-1)->pop();
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
            assert(base_level(num_base_levels()-1)->num() != 0);
            
            if (num_base_levels() > 1)
            {
               err_mesg(ERR_LEV_INFO, "HatchingGroupFixed::undo_last() - Popping off hatch and level.");
               WORLD::message("Popped hatch level of detail.");
               HatchingHatchBase *hhb = base_level(num_base_levels()-1)->pop();
               assert(hhb);
               delete(hhb);
               HatchingLevelBase *hlb = pop_base_level();
               assert(hlb);
               delete(hlb);
               return true;
            }
            else
            {
               err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::undo_last() - Only 1 hatch left... can't undo.");
               //Pen should notice this failure and delete the group...
               return false;
            }
         }
      }
   }
}

/////////////////////////////////////
// compute_convex_hull()
//  -supporting functions
/////////////////////////////////////
int compare_xinc_ydec(const void *a, const void *b) 
{
   double diff; 
   NDCZpt **pta = (NDCZpt **)a;   NDCZpt **ptb = (NDCZpt **)b;
   diff = (**pta)[0] - (**ptb)[0];  if (diff>0) return 1; if (diff<0) return -1;
   diff = (**ptb)[1] - (**pta)[1];  if (diff>0) return 1; if (diff<0) return -1;
	return 0;
}

int compare_xdec_yinc(const void *a, const void *b) { return compare_xinc_ydec(b,a); }

int make_chain(ARRAY<NDCZpt*>& V, int (*cmp)(const void*, const void*)) 
{
	int i, j, s = 1; 	NDCZpt *tmp;
	V.sort(cmp);
	for (i=2; i<V.num(); i++) 
   {
      for (j=s; j>=1 ; j--)
         if ((det(NDCvec(*(V[i]) - *(V[j])),	NDCvec(*(V[j-1]) - *(V[j]))) > 0)) break;
		s = j+1;
		tmp = V[s]; V[s] = V[i]; V[i] = tmp;
	}
	return s;
}
/////////////////////////////////////
// compute_convex_hull()
/////////////////////////////////////
void 
HatchingGroupFixed::compute_convex_hull(
   CNDCZpt_list &pts, 
   NDCZpt_list &hull) 
{
   int i, num_upper, num_lower;
   ARRAY<NDCZpt*> P, Q;

   hull.clear();

   for (i=0; i<pts.num(); i++)  P.add(&(pts[i]));

   num_lower = make_chain(P, compare_xinc_ydec);

   if (!num_lower) return;

   for (i=0; i<num_lower; i++) hull.add(*(P[i]));

   for (i=num_lower; i<P.num(); i++) Q.add(P[i]);
   Q.add(P[0]);

   num_upper = make_chain(Q, compare_xdec_yinc);

   for (i=0; i<num_upper; i++) hull.add(*(Q[i]));

}

/////////////////////////////////////
// store_visibility()
/////////////////////////////////////
bool
HatchingGroupFixed::store_visibility(HatchingLevelBase *hlb)
{
   assert(!_complete);

   int k,j;

   NDCZpt_list pts, hull;

   //One side
   for (k=0; k<hlb->num(); k++)
      for (j=0; j<(*hlb)[k]->get_pts().num(); j++)
         pts += NDCZpt( _patch->xform() * ((*hlb)[k]->get_pts()[j]) );

   compute_convex_hull(pts,hull);

   if (hull.num() == 0)
   {
      err_mesg(ERR_LEV_WARN, 
         "HatchingGroupFixed:store_visibility() - Error!! There were %d convex hull verts found from %d hatch verts.",
         hull.num(), pts.num());
      return false;
   }
   else
   {
      err_mesg(ERR_LEV_INFO, 
         "HatchingGroupFixed:store_visibility() - There were %d convex hull verts found from %d hatch verts.",
         hull.num(), pts.num());
   }

   Bface *f = 0;
   XYpt center = NDCpt(hull.average());
   BMESHray r(center);
   VIEW::peek()->intersect(r);
   f = r.face();

   if (!f) 
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFixed:store_visibility() - Seed face intersect failed (hull center)!");
   }
        
   if (!(_patch == f->patch())) 
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFixed:store_visibility() - Seed face intersected wrong patch (hull center)!!");
      return false;
   }

   CBface_list & faces = f->mesh()->faces();
   for (k=0; k< faces.num(); k++) faces[k]->clear_bit(1);

   int ctr = 0;
        
   ctr = recurse_visibility(f,hull);

   err_mesg(ERR_LEV_INFO, "HatchingGroupFixed:store_visibility() - There were %d faces in the visible region (from %d).", ctr, faces.num());

   _group->patch()->changed();

   if (ctr>0) return true;
   else return false;

}

/////////////////////////////////////
// intersect()
/////////////////////////////////////
bool
isect(CNDCpt &pt1a, CNDCpt &pt1b, CNDCpt &pt2a, CNDCpt &pt2b)
{

   if ((pt1a==pt2a)||(pt1a==pt2b)||(pt1b==pt2a)||(pt1b==pt2b)) return true;

   double c1a = det(pt1b - pt1a, pt2a - pt1a);
   double c1b = det(pt1b - pt1a, pt2b - pt1a);
   double c2a = det(pt2b - pt2a, pt1a - pt2a);
   double c2b = det(pt2b - pt2a, pt1b - pt2a);

   if  ( ( ((c1a>=-gEpsZeroMath)&&(c1b<=gEpsZeroMath)) || ((c1a<=gEpsZeroMath)&&(c1b>=-gEpsZeroMath)) ) &&
         ( ((c2a>=-gEpsZeroMath)&&(c2b<=gEpsZeroMath)) || ((c2a<=gEpsZeroMath)&&(c2b>=-gEpsZeroMath)) )    )
      return true;

   return false;
}

/////////////////////////////////////
// recurse_visibility()
/////////////////////////////////////
int             
HatchingGroupFixed::recurse_visibility(Bface *f, CNDCpt_list &poly)
{           
   //Stop if no face, or if already seen
   if (!f) return 0;
   if (f->is_set(1)) return 0;

   f->set_bit(1);

   int i, ctr = 0, added = 0;

   bool visible = false;

   //If any vertex fits in polygon, we're visible and want to recurse neighbors
   if (f->front_facing())
   {
      NDCpt_list tri(3);
      tri += f->v1()->wloc(); tri += f->v2()->wloc(); tri += f->v3()->wloc();

      //See if any tri vertex lands in the hull
      for (i=0; (i<tri.num()) && (!visible); i++) 
         if (poly.contains(tri[i])) visible = true;

      if (!visible)
      {
         //See if any hull vertex lands in the tri
         for (i=0; (i<poly.num()) && (!visible); i++) 
            if (tri.contains(poly[i])) visible = true;

         if (!visible)
         {
            //See if any edges cross
            int num = poly.num();
            for (i=0; (i<poly.num()) && (!visible); i++)
            {
               if      (isect(poly[i],poly[(i+1)%num], tri[0],tri[1])) visible = true;
               else if (isect(poly[i],poly[(i+1)%num], tri[1],tri[2])) visible = true;            
               else if (isect(poly[i],poly[(i+1)%num], tri[2],tri[0])) visible = true;            
            }
         }
      }
   }

   if (visible)
   {
      //Add group to face
      HatchingSimplexDataFixed *hsdf = HatchingSimplexDataFixed::find(f);

      if (!hsdf) hsdf = new HatchingSimplexDataFixed(f);

      hsdf->add(this);
      added = 1;
             
      //Now check the adjacent faces
      ctr += recurse_visibility(f->e1()->other_face(f),poly);
      ctr += recurse_visibility(f->e2()->other_face(f),poly);
      ctr += recurse_visibility(f->e3()->other_face(f),poly);
   }


/*
   //Even if not in the region, we add this face, 
   //since it neighbors vis face
   //This gives us a 1 polygon border which 
   //helps defeat vis glitches near the border
   HatchingSimplexDataFixed *hsdf = HatchingSimplexDataFixed::find(f);
   if (!hsdf) 
   {
      hsdf = new HatchingSimplexDataFixed(f);
   }

   hsdf->add(this);
   added = 1;

   //If any vertex fits in polygon, we're visible and want to recurse neighbors
   if ( f->front_facing() &&
        (poly.contains(_patch->xform() * f->v1()->loc()) ||
         poly.contains(_patch->xform() * f->v2()->loc()) ||
         poly.contains(_patch->xform() * f->v3()->loc()) )  )
   {
             
      //Now check the adjacent faces
      ctr += recurse_visibility(f->e1()->other_face(f),poly);
      ctr += recurse_visibility(f->e2()->other_face(f),poly);
      ctr += recurse_visibility(f->e3()->other_face(f),poly);
   }
*/

   return added+ctr;
}

/////////////////////////////////////
// clear_visibility()
/////////////////////////////////////
void
HatchingGroupFixed::clear_visibility(void)
{
   int k, ctr=0, kctr=0;
        
   CARRAY<Bface*>& faces = _patch->cur_faces();

   for (k=0; k< faces.num(); k++)
   {
      HatchingSimplexDataFixed *hsdf = HatchingSimplexDataFixed::find(faces[k]);
      if (hsdf) 
      {
         if(hsdf->exists(this))
         {
            hsdf->remove(this);
            ctr++;
            if (hsdf->num()==0)
            {
               faces[k]->rem_simplex_data(hsdf);
               kctr++;
               delete hsdf;
            }
         }
      }
   }
   err_mesg(ERR_LEV_INFO, "HatchingGroupFixed:clear_visibility - Removed from %d faces.", ctr);     
   err_mesg_cond((kctr>0), ERR_LEV_INFO, "HatchingGroupFixed:clear_visibility - Removed %d empty HatchingSimplexDataFixed's.", kctr);     

   _complete = false;
   
   _group->patch()->changed();
}

/////////////////////////////////////
// draw()
/////////////////////////////////////
int
HatchingGroupFixed::draw(CVIEWptr &v)
{
   int num;

   if (VIEW::stamp() != _stamp)
   {

      //if (_complete) __asm int 3;
      //Update prototypes, and levels
      draw_setup();

      //Advance level animations, etc.
      level_draw_setup();
     
      //Causes hatches to update (we do this outside of level_draw_setup
      //in hopes of conglomerating code to leverage the cache)
      hatch_draw_setup();

      _stamp = VIEW::stamp();
   }

   //Draw all the hatches
   _prototype.draw_start();
   num = HatchingGroupBase::draw(v);
   _prototype.draw_end();

   //XXX - Could query _selected here
   num += draw_select(v);

   return num;
}

/////////////////////////////////////
// update_prototype()
/////////////////////////////////////
void    
HatchingGroupFixed::update_prototype()
{
   //Updates the group's proto
   HatchingGroup::update_prototype();
   //Bubbles down to hatches 
   HatchingGroupBase::update_prototype();
}

/////////////////////////////////////
// kill_animation()
/////////////////////////////////////
void
HatchingGroupFixed::kill_animation()
{
   HatchingGroupBase::kill_animation();
}

/////////////////////////////////////
// query_pick()
/////////////////////////////////////
bool
HatchingGroupFixed::query_pick(CNDCpt &pt)
{
   Wpt foo;
   Bface *f;
   HatchingSimplexDataFixed *hsdf;

   if (!_complete) return false;

   f = find_face_vis(pt,foo);
   if (f)
      {
         hsdf = HatchingSimplexDataFixed::find(f);
         if (hsdf)
            {
               if (hsdf->exists(this))
                  {
                     return true;
                  }
            }
      }

   return false;

}

/////////////////////////////////////
// SimplexFilterFixed
/////////////////////////////////////
//
// -A simplex filter that checks vis
// for fixed hatch group vertices.
//
/////////////////////////////////////
class SimplexFilterFixed : public SimplexFilter {
 private:
   HatchingGroupFixed *     _hgf;
 public:
   SimplexFilterFixed(HatchingGroupFixed *hgf) : _hgf(hgf) {}
        
   virtual bool accept(CBsimplex* s) const 
   { 
      if (!is_face(s)) return false;
      HatchingSimplexDataFixed *hsdf = HatchingSimplexDataFixed::find((Bface *)s);
      if ((hsdf) && (hsdf->exists(_hgf))) return true;
      return false;
   }
};


/////////////////////////////////////
// query_visibility()
/////////////////////////////////////
bool
HatchingGroupFixed::query_visibility(
   CNDCpt &pt,
   CHatchingVertexData* /* hsvd */)
{

   SimplexFilterFixed filt(this);

   return query_filter_id(pt,filt);

}

/////////////////////////////////////
// add()
/////////////////////////////////////
bool
HatchingGroupFixed::add(
   CNDCpt_list &pl,
   const ARRAY<double>&prl,
   int curve_type
   )
{
   int k;
   double a,b;
   Bface *f;

   // It happens:
   if (pl.empty()) 
   {
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFixed:add() - Error: point list is empty!");
      return false;
   }
   if (prl.empty()) 
   {
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFixed:add() - Error: pressure list is empty!");
      return false;
   }

   if (pl.num() != prl.num())
   {
      err_mesg(ERR_LEV_ERROR, "HatchingGroupFixed:add() - gesture pixel list and pressure list are not same length.");
      return false;
   }

   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed:add() - smoothing gesture.");

   //Smooth the input gesture
   NDCpt_list              smoothpts;
   ARRAY<double>           smoothprl;
   if (!smooth_gesture(pl, smoothpts, prl, smoothprl, _params.anim_style()))
      return false;

   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed:add() - clipping gesture to model.");

   NDCpt_list              ndcpts;
   ARRAY<double>           finalprl;
   clip_to_patch(smoothpts,ndcpts,smoothprl,finalprl);
   ndcpts.update_length();

   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed::add() - Checking gesture silliness.");

   if (HatchingGroupBase::is_gesture_silly(ndcpts,_params.anim_style()))
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Punting silly gesture...");
      return false;
   }

   //Even if the user wants to project to create the
   //hatch, we continue with plane cutting to
   //generate a curve we can use to estimate
   //the mesh spacing so that the final projected
   //hatch is sampled evenly on the level of the mesh
   //spacing

   //Get the cutting line
   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed:add() - fitting line.");
   if (!fit_line(ndcpts,a,b)) return false;

   //Slide to midpoint if desired
   if (Config::get_var_bool("HATCHING_GROUP_SLIDE_FIT",false,true)) 
      b = ndcpts.interpolate(0.5)[1] - (a*ndcpts.interpolate(0.5)[0]);

   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed:add() - computing plane.");

   //Find the cutting plane
   Wplane wpPlane;
   f = compute_cutting_plane(_patch, a, b, ndcpts, wpPlane);
   if (!f) return false;
   else
   {
      if (!f->front_facing())
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Nearest pt. on fit line hit backfacing surface.");
         return false;
      }
   }

   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed:add() - slicing mesh.");
   
   //Intersect the mesh to get a 3D curve
   Wpt_list wlList;
   slice_mesh_with_plane(f,wpPlane,wlList);

   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed:add() - cliping curve to gesture.");

   //Clip end of 3D curve to match gesture
   Wpt_list wlClipList;
   clip_curve_to_stroke(_patch, ndcpts, wlList, wlClipList);
   wlClipList.update_length();

   Wpt_list wlScaledList;

   if (curve_type == HatchingGroup::CURVE_MODE_PROJECT)
   {
      err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed::add() - Projecting to surface.");
     
      //Okay, the user wants to get literal, projected
      //points, so lets do it.  We're careful to
      //toss points that hit the no/wrong mesh
      Wpt_list wlProjList;
      Wpt wloc;

      for (k=0; k<ndcpts.num(); k++)
      {
         f = HatchingGroupBase::find_face_vis(NDCpt(ndcpts[k]),wloc);
         if ((f) && (f->patch() == _patch) && (f->front_facing()))
         {
            wlProjList += wloc;
         }
         else 
         {
            if (!f) 
               err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Missed while projecting: No hit on a mesh!");
            else if (!(f->patch() == _patch)) 
               err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Missed while projecting: Hit wrong patch.");
            else if (!f->front_facing())
               err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Missed while projecting: Hit backfacing tri.");
            else 
               err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Missed while projecting: WHAT?!?!?!?!");
         }
      }
      if (wlProjList.num()<2)
      {
         err_mesg(ERR_LEV_WARN, "HatchingGroupFixed:add() - Nothing left after projection failures. Punting...");
         return false;
      }
      wlProjList.update_length();

      err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed::add() - Resampling curve.");

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

      err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed::add() - Resampling curve.");

      //Resample to even spacing in world space. This curve will
      //be sampled on the order of the mesh spacing but we'll
      //not allow the num of samples to drop too low in case
      //the gesture's on the scale of one triangle
      int num = max(wlClipList.num(),5);
      double step = 1.0/((double)(num-1));
      for (k=0 ; k<num ; k++) wlScaledList += wlClipList.interpolate((double)k*step);
   }

   // Convert back to 2D
   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed:add() - converting to 2D.");
   NDCZpt_list ndczlScaledList;
   for (k=0;k<wlScaledList.num();k++) ndczlScaledList += NDCZpt(_patch->xform()*wlScaledList[k]);
   ndczlScaledList.update_length();

   // Calculate pixel length of hatch
   double pix_len = ndczlScaledList.length() * VIEW::peek()->ndc2pix_scale();

   if (pix_len < 8.0)
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Stroke only %f pixels. Probably an accident. Punting...", pix_len);
      return false;
   }

   ARRAY<HatchingFixedVertex>    verts;
   Wpt_list                      pts;
   ARRAY<Wvec>                   norms;

   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingGroupFixed::add() - Final sampling.");

   for (k=0; k<ndczlScaledList.num(); k++) {

      Wpt wloc;
      f = HatchingGroupBase::find_face_vis(NDCpt(ndczlScaledList[k]),wloc);

      if ((f) && (f->patch() == _patch) && (f->front_facing()))
      {
         Wvec bc;
         Wvec norm;

         //f->project_barycentric(wloc,bc);
         f->project_barycentric_ndc(NDCpt(ndczlScaledList[k]),bc);

         Wvec bc_old = bc;
         Bsimplex::clamp_barycentric(bc);
         double dL = fabs(bc.length() - bc_old.length());

         if (bc != bc_old)
         {
            err_mesg(ERR_LEV_INFO, 
               "HatchingGroupFixed::add() - Baycentric clamp modified result: (%f,%f,%f) --> (%f,%f,%f) Length Change: %f", 
                  bc_old[0], bc_old[1], bc_old[2], bc[0], bc[1], bc[2], dL);
         }
         if (dL < 1e-3)
         {
            verts += HatchingFixedVertex(f->index(),bc);
            f->bc2norm_blend(bc,norm);
            pts += wloc;
            norms += norm;
         }
         else
         {
            err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Change too large due to error in projection. Dumping point...");
         }
      }
      else 
      {
         if (!f) 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Missed in final lookup: No hit on a mesh!");
         else if (!(f->patch() == _patch)) 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Missed in final lookup: Hit wrong patch.");
         else if (!(f->front_facing())) 
            err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Missed in final lookup: Hit backfracing tri.");
         else
            err_mesg(ERR_LEV_WARN, "HatchingGroupFixed::add() - Missed in final lookup: WHAT?!?!?!?!");
      }
   }

   if (pts.num()>1)
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
            ol->add(BaseStrokeOffset( (double)k/(double)(finalprl.num()-1), 0.0, finalprl[k], BaseStrokeOffset::OFFSET_TYPE_MIDDLE));

      ol->add(BaseStrokeOffset( 1.0, 0.0, finalprl[finalprl.num()-1],   BaseStrokeOffset::OFFSET_TYPE_END));

      if (base_level(num_base_levels()-1)->pix_size() > 0)
      {
         assert(_params.anim_style() != HatchingGroup::STYLE_MODE_NEAT);

         add_base_level();

         // Make sure we can see it whil we're editing!
         base_level(num_base_levels()-1)->set_desired_frac(1.0);

      }

      base_level(num_base_levels()-1)->add_hatch(
         new HatchingHatchFixed(
            base_level(num_base_levels()-1),_patch->mesh()->pix_size(),verts,pts,norms,ol) );

      return true;
   }
   else
   {
      err_mesg(ERR_LEV_WARN, "HatchingGroupFixed:add() - All lookups are bad. Punting...");
      return false;
   }



   return true;

}

/////////////////////////////////////
// clip_to_patch()
/////////////////////////////////////
void    
HatchingGroupFixed::clip_to_patch(
   CNDCpt_list &pts,        NDCpt_list &cpts,
   const ARRAY<double>&prl, ARRAY<double>&cprl ) 
{
   int k, started = 0;
   Bface *f;
   Wpt foo;

   for (k=0; k<pts.num(); k++)
   {
      f = find_face_vis(pts[k], foo);
      if ((f) && (f->patch() == _patch))
      {
         started = 1;
         cpts += pts[k];
         cprl += prl[k];
      }
      else
      {
         if (started)
         {
            k=pts.num();
         }
      }
   }
}

/////////////////////////////////////
// slice_mesh_with_plane()
/////////////////////////////////////
void
HatchingGroupFixed::slice_mesh_with_plane(
   Bface *f, CWplane &wpPlane, Wpt_list &wlList)
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

	//sky box fix prevents infinite loops for sky box type geometry
    //where the camera is contained inside the object
    //points are checked against view volume

   while (nxtFace1 && (nxtFace1->front_facing())) {
      nxtFace1 = nxtFace1->plane_walk(nxtEdge1,wpPlane,nxtEdge1);

      if (nxtEdge1) {
		Wpt temp_pt = wpPlane.intersect(nxtEdge1->line()); //sky box fix
		if (!temp_pt.in_frustum())
		{
			break;
		}
         wlList1 += temp_pt;
      }
      else {
		  cerr << "Slice Mesh :: assert(!nxtFace1)" << endl;
         assert(!nxtFace1);
      }
   }

   while (nxtFace2 && (nxtFace2->front_facing())) {
      nxtFace2 = nxtFace2->plane_walk(nxtEdge2,wpPlane,nxtEdge2);

      if (nxtEdge2) {
		  Wpt temp_pt =wpPlane.intersect(nxtEdge2->line()); //sky box fix
		if (!temp_pt.in_frustum())
		{
			break;
		}

         wlList2 += temp_pt;
      }
      else {
		 cerr << "Slice Mesh :: assert(!nxtFace2)" << endl;
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
// interpolate()
/////////////////////////////////////
HatchingHatchBase *
HatchingGroupFixed::interpolate(
   int /* lev */,
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

   HatchingHatchFixed *h1 = (HatchingHatchFixed*)hhb1;
   HatchingHatchFixed *h2 = (HatchingHatchFixed*)hhb2;

   CWpt_list                     &ptl1 = h1->get_pts();
   CWpt_list                     &ptl2 = h2->get_pts();

   const ARRAY<Wvec>             &nl1  = h1->get_norms();
   const ARRAY<Wvec>             &nl2  = h2->get_norms();

   const BaseStrokeOffsetLISTptr &ol1 = h1->get_offsets();
   const BaseStrokeOffsetLISTptr &ol2 = h2->get_offsets();
        
   num = max(ptl1.num(),ptl2.num());
   dlen = 1.0/((double)num-1.0);

   Wpt_list pts;
   ARRAY<Wvec> norms;
   BaseStrokeOffsetLISTptr offsets = new BaseStrokeOffsetLIST;

   ifrac = 0.5 + (drand48() - 0.5) * INTERPOLATION_RANDOM_FACTOR;

   for (k=0; k<num; k++) {
      pts +=
         ptl1.interpolate((double) k*dlen, 0, &seg1, &frac1)*ifrac +
         ptl2.interpolate((double) k*dlen, 0, &seg2, &frac2)*(1.0-ifrac);

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

   HatchingHatchFixed *hhf;
   hhf = new HatchingHatchFixed(hlb,pix_size,pts,norms,offsets);
        
   return hhf;
}

/*****************************************************************
 * HatchingHatchFixed
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingHatchFixed::_hhf_tags = 0;

static int foo = DECODER_ADD(HatchingHatchFixed);

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingHatchFixed::tags() const
{
   if (!_hhf_tags) {
      _hhf_tags = new TAGlist;
      *_hhf_tags += HatchingHatchBase::tags();
      *_hhf_tags += new TAG_meth<HatchingHatchFixed>(
         "verts",
         &HatchingHatchFixed::put_verts,
         &HatchingHatchFixed::get_verts,
         1);

   }
   return *_hhf_tags;
}
/////////////////////////////////////
// put_verts()
/////////////////////////////////////
void
HatchingHatchFixed::put_verts(TAGformat &d) const
{
   err_mesg(ERR_LEV_SPAM, "HatchingHatchFixed::put_verts()"); 
   d.id();
   *d << _verts;
   d.end_id();
}

/////////////////////////////////////
// get_verts()
/////////////////////////////////////
void
HatchingHatchFixed::get_verts(TAGformat &d)
{
   err_mesg(ERR_LEV_SPAM, "HatchingHatchFixed::get_verts()"); 
   *d >> _verts;

}
/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingHatchFixed::HatchingHatchFixed(
   HatchingLevelBase *hlb, double len, 
   const ARRAY<HatchingFixedVertex> &vl,
   CWpt_list &pl, const ARRAY<Wvec> &nl, 
   CBaseStrokeOffsetLISTptr &ol) : 
      HatchingHatchBase(hlb,len,pl,nl,ol)
{
   _verts.clear();   
   _verts.operator+=(vl);

   //XXX - Fix this... init() get called twice (in base class, too)

   init();
}

/////////////////////////////////////
// notify_xform()
/////////////////////////////////////
void
HatchingHatchFixed::notify_xform(BMESH *, CWtransf& t, CMOD&)
{
   int k;

   for (k=0; k< _pts.num(); k++)
      _pts[k] = t * _pts[k];
   for (k=0; k< _norms.num(); k++)
      _norms[k] = t.inverse().transpose() * _norms[k];

   //Clear these so they regenerate
   _real_pts.clear();      
   _real_norms.clear();
   _real_good.clear();
}

/////////////////////////////////////
// notify_change()
/////////////////////////////////////
void
HatchingHatchFixed::notify_change(BMESH *m, BMESH::change_t chg)
{
   assert(chg == BMESH::VERT_POSITIONS_CHANGED);

   int k;

   if (_verts.num() > 0)
   {
      //Sanity check
      assert(_verts.num() == _pts.num());
      assert(_verts.num() == _norms.num());

      for (k=0; k < _verts.num(); k++)
      {
         Bface *f = m->bf(_verts[k].ind);
         assert(f);
         f->bc2pos(_verts[k].bar,_pts[k]);
         f->bc2norm_blend(_verts[k].bar,_norms[k]);
      }

      //Clear these cached values so they regenerate
      _real_pts.clear();      
      _real_norms.clear();
      _real_good.clear();

   }
   else
   {
      err_mesg(ERR_LEV_WARN, "HatchingHatchFixed::notify_change() - Verts changed, but we can't update fixed hatches!!!"); 
   }

}


/////////////////////////////////////
// init()
/////////////////////////////////////
void
HatchingHatchFixed::init()
{
   HatchingHatchBase::init();

   //If there are _verts (tri index/bary) then
   //make sure we have the right number of them
   assert((_verts.num() == 0) || (_pts.num() == _verts.num()));       

}

/////////////////////////////////////
// draw_setup()
/////////////////////////////////////
void    
HatchingHatchFixed::draw_setup()
{
   // XXX - Cache this better...
   // Reset the alpha cause it changes due to animation extinctions

   _stroke->set_alpha( _level->group()->group()->prototype()->get_alpha());

   HatchingHatchBase::draw_setup();
        
}
/*****************************************************************
 * HatchingBackboneFixed
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////
TAGlist *       HatchingBackboneFixed::_hbf_tags = 0;

/////////////////////////////////////
// tags()
/////////////////////////////////////
CTAGlist &
HatchingBackboneFixed::tags() const
{
   if (!_hbf_tags) {
      _hbf_tags = new TAGlist;
      *_hbf_tags += HatchingBackboneBase::tags();
   }
   return *_hbf_tags;
}

/////////////////////////////////////
// Constructor
/////////////////////////////////////
HatchingBackboneFixed::HatchingBackboneFixed(
   Patch *p) :
   HatchingBackboneBase(p)
{

   _use_exist = false;

   //Do nothing to generate backbone
   //Wait for a call to build from
   //scratch or from disk

}

/////////////////////////////////////
// Destructor
/////////////////////////////////////
HatchingBackboneFixed::~HatchingBackboneFixed()
{
   //We own nothing that the base class doesn't already destroy
}

/////////////////////////////////////
// get_ratio()
/////////////////////////////////////
double
HatchingBackboneFixed::get_ratio()
{
   //This is good enough
   return HatchingBackboneBase::get_ratio();
}

/////////////////////////////////////
// compute()
/////////////////////////////////////
//
// -To compute on the fly, step through
//  each hatching pair, and find closest
//  point on longer hatch to mid of other
// -Set all pairs to exist, since we'll never
//  fail to present the wpts
// -Also fill in all segment lengths
//
/////////////////////////////////////
bool
HatchingBackboneFixed::compute(
   HatchingLevelBase *hlb)
{
   int i;

   assert(hlb);

   if (hlb->num() < 2)
   {
      err_mesg(ERR_LEV_WARN, "HatchingBackboneFixed::compute() - Can't get backbone from less that 2 hatches!"); 
      return false;
   }

   for (i=0;i<hlb->num()-1;i++)
      {
         Wpt p;
         Wpt_list l;

         if ((*hlb)[i]->get_pts().length() < (*hlb)[i+1]->get_pts().length())
            {
               p = (*hlb)[i  ]->get_pts().interpolate(0.5);
               l = (*hlb)[i+1]->get_pts();
            }
         else
            {
               p = (*hlb)[i+1]->get_pts().interpolate(0.5);
               l = (*hlb)[i  ]->get_pts();
            }
                
         Wpt loc;
         loc = l.closest(p);

         NDCpt n1 = NDCZpt( p,   _patch->obj_to_ndc() );
         NDCpt n2 = NDCZpt( loc, _patch->obj_to_ndc() );

         double len = (n2 - n1).length() * VIEW::peek()->ndc2pix_scale();

         Vertebrae *v = new Vertebrae();
         assert(v);

         v->exist = true;
         v->pt1 = p;
         v->pt2 = loc;
         v->len = len;

         _vertebrae.add(v);
      }

   _len = find_len();

   err_mesg_cond(debug, ERR_LEV_SPAM, "HatchingBackboneFixed::compute() - Backbone is %f pixels in %d  vertebrae.", _len, _vertebrae.num()); 

   return true;
}

/////////////////////////////////////
// notify_xform()
/////////////////////////////////////
void
HatchingBackboneFixed::notify_xform(BMESH *, CWtransf& t, CMOD&)
{
   int k;

   for (k=0; k< _vertebrae.num(); k++)
   {
      _vertebrae[k]->pt1 = t * _vertebrae[k]->pt1;
      _vertebrae[k]->pt2 = t * _vertebrae[k]->pt2;
   }
  
}


/////////////////////////////////////
// notify_change()
/////////////////////////////////////
void
HatchingBackboneFixed::notify_change(BMESH *, BMESH::change_t chg)
{
   assert(chg == BMESH::VERT_POSITIONS_CHANGED);


   //XXX - Not implemented yet...
/*
   for (k=0; k< _vertebrae.num(); k++)
   {
      _vertebrae[k]->pt1 = t * _vertebrae[k]->pt1;
      _vertebrae[k]->pt2 = t * _vertebrae[k]->pt2;
   }
*/
}



