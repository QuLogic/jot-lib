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
/**********************************************************************
 * decorator_stroke.C:
 **********************************************************************/


#include "decorator_stroke.H"
#include "mesh/patch.H"

using mlib::Wpt;
using mlib::NDCZpt;

static int f1 = DECODER_ADD(DecoratorStroke);

DecoratorStroke::DecoratorStroke(Patch* p, double width) : 
   ViewStroke(width)
{
   if (p)
      _patch = p;
}


int
DecoratorStroke::draw(CVIEWptr& view)
{
   if (_surface_pts.num() < 2)       
      return 0;

   ViewStroke::clear();  // clears all the vertices

   // recompute the vertex ndc locations for the current frame
   for ( int i=0; i<_surface_pts.num(); i++) {
      assert(_patch);
      NDCZpt p(_surface_pts[i],_patch->mesh()->obj_to_ndc());
      add(p, _default_width);  // add vert with location p
   }

   // actually draw the stroke
   ViewStroke::draw(view);  

   return 0;
}

NDCZpt
DecoratorStroke::getNDCZpt(int i)
{
  assert(i < _surface_pts.num());
  assert(_patch);
  NDCZpt p(_surface_pts[i],_patch->mesh()->obj_to_ndc());
  return p;
}

void
DecoratorStroke::clone_attribs(ViewStroke& v) const 
{
   ViewStroke::clone_attribs(v);
}

ViewStroke*
DecoratorStroke::dup_stroke() const
{
   DecoratorStroke* s = new DecoratorStroke(_patch, _default_width);
   clone_attribs(*s);
   return s;
}

void
DecoratorStroke::clear()
{
   _surface_pts.clear();
   _edges.clear();      
   _t_vals.clear();
   ViewStroke::clear();
}

void
DecoratorStroke::add_vert_loc(CBedge* e, double t)
{
   assert(e);
   assert( t >= 0 && t <= 1.0 );
   _edges += e;
   _t_vals += t;

   // compute and cache the object space location (convenient for drawing)
   Wpt loc = e->v1()->loc() + ((e->v2()->loc() - e->v1()->loc()) * t);
   _surface_pts += loc;
   _dirty = true;
}

/* end of file feature_stroke.C */
