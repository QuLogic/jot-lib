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
 * lvert.C
 **********************************************************************/
#include "std/config.H"
#include "mesh/mi.H"
#include "mesh/lmesh.H"

using namespace mlib;

double 
loop_alpha(int n)
{
   // do_warren means use a weight of 5/8 at each vertex instead of the
   // more complex computation proposed by Loop:
   static bool do_warren = Config::get_var_bool("DO_WARREN",false);
   if (do_warren)
      return 5.0/3*n;

   // Function alpha (which depends on a vertex's degree), from
   // section 2.1 of Hoppe et al. Siggraph 94 (see subdiv_calc.H).
   //
   // This was supposed to go in subdiv_calc.H but the sun compiler
   // has been taking prissiness to an extreme.
   //
   // return values are cached for vertex degrees up to 31

   static double _alpha[] = {
      1,         1,         1.282051,  2.333333,
      4.258065,  6.891565, 10.000000, 13.397789,
      16.957691, 20.598919, 24.272687, 27.950704,
      31.617293, 35.264345, 38.888210, 42.487807,
      46.063495, 49.616397, 53.148004, 56.659946,
      60.153856, 63.631303, 67.093752, 70.542553,
      73.978934, 77.404006, 80.818773, 84.224136,
      87.620905, 91.009809, 94.391503, 97.766576
   };
   if (n<2) {
      err_msg("loop_alpha: bad index %d", n);
      return 1;
   } else if (n >= 32) {
      // actual formula encoded in _alpha table:
      double b = 5.0/8.0 - sqr(3.0 + 2.0*cos(2*M_PI/n))/64.0;
      return n*(1-b)/b;
   } else {
      return _alpha[n];
   }
}

Lvert::~Lvert()
{
  if (is_edge(_parent)) {
    ((Ledge*)_parent)->subdiv_vert_deleted();
  } else if (is_vert(_parent)) {
    ((Lvert*)_parent)->subdiv_vert_deleted();
  } 
   // subdiv vert must die with us
   delete_subdiv_vert();

   // one other thing... we have to repeat code here that's
   // from the Bvert destructor, since it will trigger
   // callbacks that come back this way, calling Lvert
   // methods, which won't be available once the Lvert
   // destructor is finished:

   // if the vertex dies, it takes neighboring edges and
   // faces with it:
   if (_mesh) {
      while (degree() > 0)
         _mesh->remove_edge(_adj.last());
   }
}

void    
Lvert::subdiv_vert_deleted() 
{ 
  // not for public use, to be called by the child
  _subdiv_vertex = 0; 

  clear_bit(SUBDIV_LOC_VALID_BIT);
  clear_bit(SUBDIV_COLOR_VALID_BIT);
  clear_bit(SUBDIV_CORNER_VALID_BIT);
}

Bsimplex* 
Lvert::ctrl_element() const 
{
   if (!_parent)
      return (Bsimplex*)this;
   if (is_vert(_parent))
      return ((Lvert*)_parent)->ctrl_element();
   if (is_edge(_parent))
      return ((Ledge*)_parent)->ctrl_element();
   if (is_face(_parent))
      return ((Lface*)_parent)->control_face();
   return 0; // should never happen
}

Lvert* 
Lvert::subdiv_vert(int level)
{
   // Return subdiv vertex at given level down,
   // relative to this:

   if (level < 0)
      return 0;
   if (level == 0)
      return this;
   return _subdiv_vertex ? _subdiv_vertex->subdiv_vert(level-1) : 0;
}

Lvert* 
Lvert::parent_vert(int rel_level) const 
{ 
   // Get parent vert (it it exists) at the given relative
   // level up from this vert

   Lvert* v = (Lvert*)this; 
   for (int i=0; v && i<rel_level; i++) {
      Bsimplex* p = v->parent();
      if (!is_vert(p))
         return 0;
      v = (Lvert*)p;
   }

   return v;
}

Lvert* 
Lvert::cur_subdiv_vert()
{
   // Return subdiv vertex at current subdiv level of the mesh:

   return _mesh ? subdiv_vert(_mesh->rel_cur_level()) : 0;
}

/*****************************************************************
 * managing the subdivision vertex
 *****************************************************************/
Lvert*       
Lvert::allocate_subdiv_vert()
{
   // Make this lightweight, so you can call
   // it when you're not sure if you need to:
   if (is_set(SUBDIV_ALLOCATED_BIT))
      return _subdiv_vertex;
   set_bit(SUBDIV_ALLOCATED_BIT);

   assert(!_subdiv_vertex);

   LMESH* submesh = lmesh()->subdiv_mesh();
   assert(submesh);
   _subdiv_vertex = (Lvert*)submesh->add_vertex(_loc);   assert(_subdiv_vertex);

   _subdiv_vertex->set_parent(this);

   // Notify observers:
   if (_data_list)
      _data_list->notify_subdiv_gen();

   return _subdiv_vertex;
}

void
Lvert::set_subdiv_vert(Lvert* subv)
{
   if (is_set(SUBDIV_ALLOCATED_BIT)) {
      err_msg("Lvert::set_subdiv_vert: already set");
      return;
   }
   assert(!_subdiv_vertex);

   set_bit(SUBDIV_ALLOCATED_BIT);
   _subdiv_vertex = subv;
   if (subv) {
      assert(!subv->parent());
      assert(lmesh()->subdiv_mesh() == subv->mesh());
      subv->set_parent(this);
   }

   // Notify observers:
   if (_data_list)
      _data_list->notify_subdiv_gen();
}

void               
Lvert::delete_subdiv_vert()
{
   // Make this lightweight, so you can call
   // it when you're not sure if you need to:
   if (!is_set(SUBDIV_ALLOCATED_BIT))
      return;
   clear_bit(SUBDIV_ALLOCATED_BIT);

   if (_subdiv_vertex) {
      _subdiv_vertex->mesh()->remove_vertex(_subdiv_vertex);
      mark_dirty();
   }
}

Lvert*
Lvert::update_subdivision()
{
   // make sure subdiv vertex is allocated
   allocate_subdiv_vert();
   if (!_subdiv_vertex) {
     // this vertex generated a child at one time, but someone else
     // removed it at the next level down
      return 0;
   }

   // keep track of color, position, and corner separately
   // since any one can be changed independently

   // propagate variable sharpness corner:
   if (is_clear(SUBDIV_CORNER_VALID_BIT)) {
      set_bit(SUBDIV_CORNER_VALID_BIT);
      _subdiv_vertex->set_corner((_corner > 0) ? _corner - 1 : 0);
   }

   // calculate subdiv color (if needed):
   if (is_clear(SUBDIV_COLOR_VALID_BIT)) {
      set_bit(SUBDIV_COLOR_VALID_BIT);
      if (has_color())
         _subdiv_vertex->set_color(lmesh()->subdiv_color(this));
   }

   // calculate subdiv loc:
   if (is_clear(SUBDIV_LOC_VALID_BIT)) {
      set_bit(SUBDIV_LOC_VALID_BIT);

      // Assign subdiv loc, but defer to observers:
      if (!(_data_list && _data_list->handle_subdiv_calc())) {
        if (corner_value() > 0) 
          _subdiv_vertex->set_loc(loc());
        else
          _subdiv_vertex->set_subdiv_base_loc(lmesh()->subdiv_loc(this));
      }
   }

   // return
   return _subdiv_vertex;
}

void
Lvert::set_offset(double d)
{
   if (!_parent) {
      err_msg("Lvert::set_offset: error: called on control vert");
   } else if (_offset != d) {
      _offset = d;
      set_loc(detail_loc_from_parent());
   }
}

Wpt 
Lvert::smooth_loc_from_parent() const
{
   // Return the smooth subdiv location (without the "detail")
   // that would be assigned to this vertex using the SubdivCalc
   // of the parent LMESH:

   if (!_parent)
      return loc();
   assert(lmesh() && lmesh()->parent_mesh());
   if (is_vert(_parent))
      return lmesh()->parent_mesh()->subdiv_loc((Lvert*)_parent);
   if (is_edge(_parent))
      return lmesh()->parent_mesh()->subdiv_loc((Ledge*)_parent);
   assert(0);
   return Wpt::Origin();
}

Wpt 
Lvert::detail_loc_from_parent() const
{
   // Return smooth_loc_from_parent() + added "detail" offset:

   assert(_parent);

   return smooth_loc_from_parent() + get_norm(_parent)*_offset;
}

void 
Lvert::set_subdiv_base_loc(CWpt& base_loc)
{
   // Protected method called by the parent of this vertex
   // to set the smooth subdiv location.
   //
   // The given value base_loc is the smooth subdiv location
   // computed from the parent level; this vertex should now set
   // its location = base_loc + n*offset, where n is the normal
   // from the parent and offset is the subdiv offset ("detail").

   if (_offset == 0 || !_parent) {
      set_loc(base_loc);
   } else {
      set_loc(base_loc + get_norm(_parent)*_offset);
   }
}

void 
Lvert::fit_subdiv_offset(CWpt& detail_loc)
{
   // Compute and store the offset needed to approximate the
   // given location via the smooth subdiv loc + detail offset.

   if (!_parent) {
      set_loc(detail_loc);
   } else {
      set_offset( (detail_loc - smooth_loc_from_parent()) * get_norm(_parent) );
   }
}

Wpt&
Lvert::limit_loc(Wpt& ret) const
{
   return (ret = lmesh()->limit_loc(this));
}

Wvec&
Lvert::loop_normal(Wvec& ret) const
{
   return (ret = LoopLoc().limit_normal(this));
}

CWpt&
Lvert::displaced_loc(LocCalc* c)
{
   static double a = Config::get_var_dbl("VOL_PRESERVE_ALPHA", 0.5,true);

   if (!is_set(DISPLACED_LOC_VALID)) {
      // It's the scaled Laplacian added to the base location
      _displaced_loc = interp(c->subdiv_val(this), _loc, 1 + a);
      set_bit(DISPLACED_LOC_VALID);
   }
   return _displaced_loc;
}

void
Lvert::mark_dirty(int bit)
{
   if (!is_set(DEAD_BIT)) {
      clear_bit(bit);
      lmesh()->add_dirty_vert(this);
   }
}

void
Lvert::crease_changed()
{
   Bvert::crease_changed();

   mask_changed();
}

void 
Lvert::degree_changed()
{
   Bvert::degree_changed();

   mask_changed();
}

void 
Lvert::mask_changed()
{
   clear_bit(MASK_VALID_BIT);
   subdiv_loc_changed();
   subdiv_color_changed();

   // Edge masks depend on vertex masks.
   //
   // In the loop below, if we try to upcast an
   // adjacent Bedge* to Ledge* when the Bedge
   // constructor is not finished, the upcast is
   // invalid, causing a seg fault.  The following
   // 'if' statement is a work-around:
   for (int k = 0; k < degree(); k++) {
      if (e(k)->is_set(Ledge::MASK_VALID_BIT))
         le(k)->mask_changed();
   }
}

void 
Lvert::geometry_changed()
{ 
   subdiv_loc_changed(); 
   Bvert::geometry_changed(); 
}

void 
Lvert::normal_changed()
{
   subdiv_loc_changed();
   Bvert::normal_changed();
}

void 
Lvert::color_changed()
{
   subdiv_color_changed();
   Bvert::color_changed();

   for (int k = 0; k < degree(); k++)
      le(k)->color_changed();
}

void  
Lvert::set_corner(unsigned short c) 
{
   if (_corner != c) {
      _corner = c;                              // set the new value
      clear_bit(SUBDIV_CORNER_VALID_BIT);       // subdiv vert is now invalid
      mask_changed();                           // recompute mask
   }
}

void
Lvert::set_mask()
{
   set_bit(MASK_VALID_BIT);

   static Bedge_list pedges;
   get_manifold_edges(pedges);

   // vertex can be explicitly set to be a "corner."  i.e., it is not
   // smoothed in subdivision. also applies to isolated vertices, or
   // vertices adjacent to just one edge.
   if (_corner || pedges.num() < 2) {
      _mask = CORNER_VERTEX;
      return;
   }

   // deal with the polyline case first --
   // a polyline edge is one w/ no adjacent faces.
   // polyline degree is number of such edges
   // connected to this vertex.
   int s = pedges.num_satisfy(PolylineEdgeFilter());
   if (s > 0) {
      _mask = ((s == 2) ? REGULAR_CREASE_VERTEX : CORNER_VERTEX);
      return;
   }

   // normal case (vertex is part of a surface).
   // count adjacent crease or border edges:
   for (int k=0; k<pedges.num(); k++)
      if (pedges[k]->is_crease() || pedges[k]->is_border())
         s++;

   // set mask value
   _mask = ((s == 0) ? SMOOTH_VERTEX             :      // no creases
            (s == 1) ? DART_VERTEX               :      // just one
            (s == 2) ? REGULAR_CREASE_VERTEX     :      // two
            CORNER_VERTEX);                             // wow! a whole bunch
}       
 
// end of file lvert.C
