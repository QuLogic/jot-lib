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
// HatchingStroke
////////////////////////////////////////////
//
// -Stroke used to render hatching group strokes
// -Supports pressures, offsets and textures
//
////////////////////////////////////////////



#include "hatching_stroke.H"
#include "geom/gl_util.H"
#include "geom/texturegl.H"
#include "disp/view.H"
#include <cmath>
#include "npr/hatching_group_free.H"
#include "npr/hatching_group_base.H"
#include "mesh/uv_mapping.H"

#include "base_stroke.H"

using mlib::Wpt;
using mlib::Wvec;
using mlib::UVpt;

////////////////////////////////////////////////////////////////////////////////////

/*****************************************************************
 * HatchingStroke
 *****************************************************************/

/////////////////////////////////////
// Static Variable Initialization
/////////////////////////////////////

static int foo = DECODER_ADD(HatchingStroke);

TAGlist*			HatchingStroke::_hs_tags = 0;

////////////////////////////////////////////////////////////////////////////////
// HatchingStroke Methods
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////
// Constructor
/////////////////////////////////////

HatchingStroke::HatchingStroke() : _group(NULL), _transf(NULL), _free(false)
{

   _vis_type =		VIS_TYPE_SCREEN | VIS_TYPE_NORMAL | VIS_TYPE_SUBCLASS;

  	_draw_verts.set_proto(new HatchingVertexData);
	_refine_verts.set_proto(new HatchingVertexData);
	_verts.set_proto(new HatchingVertexData);


}


/////////////////////////////////////
// Destructor
/////////////////////////////////////

HatchingStroke::~HatchingStroke()
{

}

/////////////////////////////////////
// tags()
/////////////////////////////////////

CTAGlist &
HatchingStroke::tags() const
{
   if (!_hs_tags) {
      _hs_tags = new TAGlist;
      *_hs_tags += BaseStroke::tags();      
	}
   return *_hs_tags;
}

/////////////////////////////////////
// get_group()
/////////////////////////////////////
void
HatchingStroke::set_group(HatchingGroupBase *hgb)
{ 
   _group = hgb; 

   if ((_group)&&(_group->group()->is_of_type(HatchingGroupFree::static_name())))
      _free = true;
   else
      _free= false;

}

/////////////////////////////////////
// copy() 
/////////////////////////////////////
BaseStroke*	
HatchingStroke::copy() const
{

	HatchingStroke *s =  new HatchingStroke;
	assert(s);
	s->copy(*this);
	return s;

}

/////////////////////////////////////
// copy()
/////////////////////////////////////
void			
HatchingStroke::copy(CHatchingStroke& s)
{
   BaseStroke::copy(s);
}

/////////////////////////////////////
// update()
/////////////////////////////////////
void
HatchingStroke::update()
{
   if (_group)
      _transf = &_group->group()->patch()->xform();
   BaseStroke::update();
}


/////////////////////////////////////
// check_vert_visibility()
/////////////////////////////////////
bool	
HatchingStroke::check_vert_visibility(CBaseStrokeVertex &v)
{
   assert(_group);

   if (!_group->is_complete()) return true;

   // XXX - Should inline this
   // XXX - The EVIL casting is temporary...

   return ( _group->query_visibility(v._base_loc,(CHatchingVertexData *)v._data));

}

////////////////////////////////////
// interpolate_refinement_vert
/////////////////////////////////////

// This version is called by refine_vertex().
// In the base class, it just calls to
// interpolate_vert(), but here we interpolate
// free hatch stroke leveraging uv.

void
HatchingStroke::interpolate_refinement_vert(
	BaseStrokeVertex *v, 
	BaseStrokeVertex **vl,
	double u)
{
   
   // For non-free hatches, just use the usual
   // screen space splining, but for free hatches
   // linearly interpolate the uv and sample
   // a point from the uv-mapping
   if (!_free)
	   interpolate_vert(v,vl,u);
   else
   {
      UVMapping *m;
      UVpt uv;
      Bface *f;
      Wvec bc, norm;
      Wpt pt;

      m = ((HatchingGroupFree*)_group->group())->mapping();
      /*XXX*/assert(m);

      m->interpolate( ((HatchingVertexData *)(vl[1]->_data))->_uv, 1.0-u, 
                        ((HatchingVertexData *)(vl[2]->_data))->_uv, u, uv);
      
      f = m->find_face(uv, bc);

      if (f)
      {
         f->bc2pos(bc, pt);       
         f->bc2norm_blend(bc, norm);

         v->_good = true;
         v->_norm = *_transf * norm;
         v->_base_loc = *_transf * pt;
         ((HatchingVertexData *)(v->_data))->_uv = uv;

      }
      else
      {
         // A failed uv lookup mean we hit a hole in the
         // uv map.  We call this a 'bad' vertex, which
         // will fail visibility...
         v->_good = false;
      }
   }
}
