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
#include "lpatch.H"
#include "std/config.H"


static bool debug = Config::get_var_bool("DEBUG_LPATCH",false);
TAGlist* Lpatch::_lpatch_tags = NULL;

Lpatch::~Lpatch() 
{
   // Recurse down thru the subdivision hierarchy:
   delete_child();
}

void 
Lpatch::triangulation_changed()
{
   Patch::triangulation_changed();
}

Lpatch* 
Lpatch::sub_patch(int k)
{
   // Returns the child patch at the given subdivision level
   // relative to this patch. E.g.:
   //
   //     k | returned Patch
   // ---------------------
   //    -1 | _parent
   //     0 | this
   //     1 | _child
   //     2 | _child->_child
   //      ...
   //

   // If there is no parent, returns this Patch for k < 0:
   if (k < 0)
      return _parent ? _parent->sub_patch(k+1) : this;
   if (k == 0)
      return this;

   // Ensure child is allocated:
   if (!_child)
      get_child();

   // if for some reason the child could not be allocated, 
   // return this:
   return _child ? _child->sub_patch(k-1) : this;
}

Patch*
Lpatch::get_child()
{
   if (!_child) {

      // Have to generate the child patch

      if (!lmesh()) {
         err_msg("Lpatch::get_child: error: mesh is NULL");
         return 0;
      }

      LMESH* child_mesh = lmesh()->subdiv_mesh();
      if (!child_mesh) {
         err_msg("Lpatch::get_child: error: child mesh is NULL");
         return 0;
      }

      // The mesh creates the child and adds it to its list of
      // patches and also to its drawables list:
      _child = (Lpatch*)child_mesh->new_patch();

      _child->set_parent(this);
   }
   return _child;
}

bool 
Lpatch::set_parent(Patch* p)
{
   // enters into child relationship w/ given patch,
   // if it's legal. returns true on success
   // (asserts zero on failure!)

   // upcast to Lpatch
   if (!isa(p))
      return false;
   Lpatch* pa = (Lpatch*)p;
   assert(
      pa->lmesh()->subdiv_mesh() == lmesh() &&
      (!_parent || _parent == pa) &&
      (!pa->_child || pa->_child == this)
      );

   // accept this union
   pa->_child = this;
   _parent = pa;
   return true;
}

int
Lpatch::draw(CVIEWptr& v)
{
   // XXX -
   //   this is here more for the comments than the
   //   functionaliy!

   // Should only be called on the control patch
   assert(is_ctrl_patch());

   // The control patch chooses an appropriate GTexture,
   // which at some point in its procedure typically calls
   // _patch->draw_tri_strips() (on the control patch).
   // Lpatch::draw_tri_strips(), below, ensures that the
   // triangle strips of the currently displayed subdivision
   // patch are the ones that are drawn.
   return Patch::draw(v);
}

int
Lpatch::draw_tri_strips(StripCB* cb)
{
   return cur_patch()->Patch::draw_tri_strips(cb);
}

int
Lpatch::draw_sil_strips(StripCB* cb)
{
   return cur_patch()->Patch::draw_sil_strips(cb);
}

int 
Lpatch::num_faces() const 
{
   // Returns number of faces at current subdiv level:

   return ((Lpatch*)this)->cur_patch()->Patch::num_faces();
}

double 
Lpatch::tris_per_strip() const 
{
   // Diagnostic:
   return ((Lpatch*)this)->cur_patch()->Patch::tris_per_strip();
}

CBface_list& 
Lpatch::cur_faces() const
{
   return ((Lpatch*)this)->cur_patch()->Patch::cur_faces();
}

Bvert_list
Lpatch::cur_verts() const
{
   // Return vertices at current subdivision level

   assert(_mesh);

   // Short-cut if patch is whole mesh
   if (_mesh->nfaces() == _faces.num() &&
       !(_mesh->is_polylines() || _mesh->is_points()))
      return lmesh()->cur_mesh()->verts();

   return cur_faces().get_verts();
}

Bedge_list
Lpatch::cur_edges() const
{
   // Return edges at current subdivision level

   assert(_mesh);

   // Short-cut if patch is whole mesh
   if (_mesh->nfaces() == _faces.num() && !_mesh->is_polylines())
      return lmesh()->cur_mesh()->edges();

   if (debug)
      err_msg("Lpatch: level %d, faces %d, edges %d",
              subdiv_level(), _faces.num(), cur_faces().get_edges().num());

   return cur_faces().get_edges();
}

void 
Lpatch::clear_subdiv_strips(int /* level */)
{
   // XXX - being phased out... (not called)
   //
   // the problem is that when subdivision meshes get deleted, we also
   // have to delete the sub-strips that reference Lfaces in
   // them. this first implementation ignores the given level at which
   // subdivision faces were deleted, and just blows away subdiv
   // strips at all levels below the top.

   if (!_tri_strips_dirty) {
      for (int i=0; i<_tri_strips.num(); i++)
         lstrip(i)->delete_substrips();
   }
}


//******DATA ITEM METHODS**********************************************//

CTAGlist &
Lpatch::tags() const
{
   return Patch::tags();
   
   if (!_lpatch_tags) 
   {
      _lpatch_tags = new TAGlist;
	  *_lpatch_tags += Patch::tags();
      *_lpatch_tags += new TAG_meth<Lpatch> ("parent_patch",    
											&Lpatch::put_parent_patch, &Lpatch::get_parent_patch,  1);
   }
   return *_lpatch_tags;
}


DATA_ITEM*              
Lpatch::dup() const
{
   // to be filled in...
	
   return 0;
}

void
Lpatch::put_parent_patch(TAGformat &d) const
{

	if (!(Config::get_var_bool("WRITE_SUBDIV_MESHES",false)))
		return;
	
	if (is_ctrl_patch())
		return;

	int parent_index;

	int num_patches = lmesh()->parent_mesh()->npatches();

	for (parent_index = 0; parent_index < num_patches; parent_index++)
		if (lmesh()->parent_mesh()->patch(parent_index) == parent())
			break;

	if(parent_index == num_patches)   
	{	
		cerr << "Lpatch::put_parent_patch - error: found a noncontrol patch without a parent!\n";
		parent_index = -1;
	}

	d.id();
	*d << parent_index;
	d.end_id();


}

void
Lpatch::get_parent_patch(TAGformat &d) 
{

	int parent_index;
	*d >> parent_index;
	
	if (parent_index == -1)
		_parent = 0;
	else if(set_parent(lmesh()->parent_mesh()->patch(parent_index)) != true)
		_parent = 0;
	
}

/* end of file lpatch.C */
