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
 * lvert.H
 **********************************************************************/
#ifndef LVERT_H_HAS_BEEN_INCLUDED
#define LVERT_H_HAS_BEEN_INCLUDED

#include "ledge.H"

template <class T> class SubdivCalc;
typedef SubdivCalc<Wpt> LocCalc;

/**********************************************************************
 * Lvert:
 *
 *      Vertex class for LMESH -- subdivision meshes implementing the
 *      scheme of Charles Loop, with special rules for creases as
 *      described in:
 *
 *        Hugues Hoppe, Tony DeRose, Tom Duchamp, Mark Halstead,
 *        Hubert Jin, John McDonald, Jean Schweitzer, and Werner
 *        Stuetzle. Piecewise Smooth Surface Reconstruction,
 *        Proceedings of SIGGRAPH 94, Computer Graphics Proceedings,
 *        Annual Conference Series, pp. 295-302 (July 1994, Orlando,
 *        Florida). ACM Press. Edited by Andrew Glassner. ISBN
 *        0-89791-667-0.
 **********************************************************************/
class Lvert : public Bvert {
   friend class REPARENT_CMD;
   friend class Ledge;
 public:

   //******** MASK VALUES ********

   enum mask_t {
      SMOOTH_VERTEX = 0,
      DART_VERTEX,
      REGULAR_CREASE_VERTEX,
      NON_REGULAR_CREASE_VERTEX,
      CORNER_VERTEX,
      CUSP_VERTEX
   };

   //******** BITS ********

   enum {
      SUBDIV_LOC_VALID_BIT = Bvert::NEXT_AVAILABLE_BIT,
      SUBDIV_COLOR_VALID_BIT,
      SUBDIV_CORNER_VALID_BIT,
      DIRTY_VERT_LIST_BIT,
      MASK_VALID_BIT,
      DEAD_BIT,
      DISPLACED_LOC_VALID,
      SUBDIV_ALLOCATED_BIT,
      NEXT_AVAILABLE_BIT
   };

   //******** MANAGERS ********

   Lvert(CWpt& p=mlib::Wpt::Origin()) :
      Bvert(p),
      _subdiv_vertex(0),
      _corner(0),
      _mask(SMOOTH_VERTEX),
      _parent(0),
      _offset(0) {}

   virtual ~Lvert();

   //******** ACCESSORS ********

   LMESH* lmesh()                 const   { return (LMESH*)mesh(); }
   Ledge* le(int k)               const   { return (Ledge*)_adj[k]; }
   Lvert* lv(int k)               const   { return (Lvert*)nbr(k); }
   Lvert* subdiv_vertex()         const   { return _subdiv_vertex; }

 public:

   //******** CONTROL ELEMENTS ********

   bool is_control() const { return _parent == 0; }

   // proper parent
   // (it's null for vertices of the control mesh):
   Bsimplex* parent() const { return _parent; }

   // control element
   // (it's the vertex itself in the control mesh):
   Bsimplex* ctrl_element() const;

   // convenience method - casts control element to Lvert if valid:
   Lvert* ctrl_vert() const {
      Bsimplex* sim = ctrl_element();
      return is_vert(sim) ? (Lvert*)sim : (Lvert*)0;
   }
   
   //******** SUBDIV VERTS ********

   // Return subdiv vertex at given level down,
   // relative to this:
   Lvert* subdiv_vert(int level=1);

   // Get parent vert (it it exists) at the given relative
   // level up from this vert
   Lvert* parent_vert(int rel_level) const;

   // Return subdiv vertex at current subdiv level of the mesh:
   Lvert* cur_subdiv_vert();

   void set_parent(Bsimplex* p) { _parent=p; }

   //******** SUBDIVISION MASKS ********

   unsigned short subdiv_mask() const {
      if (!is_set(MASK_VALID_BIT))
         ((Lvert*)this)->set_mask();
      return _mask;
   }

   void set_corner(unsigned short c = USHRT_MAX);
   unsigned short corner_value(void) const {return _corner;}
   bool is_smooth()         const { return subdiv_mask() <= DART_VERTEX; }
   bool is_regular_crease() const {
      return subdiv_mask() == REGULAR_CREASE_VERTEX;
   }

   //******** SUBDIVISION COMPUTATION ********

   Lvert* update_subdivision();
   Lvert* allocate_subdiv_vert();
   void   set_subdiv_vert(Lvert* subv);
   void   delete_subdiv_vert();

   // not for public use, to be called by the child
   void    subdiv_vert_deleted();

   Wpt&   limit_loc(mlib::Wpt&)       const;
   Wvec&  loop_normal(mlib::Wvec&)    const;

   // Used in volume-preserving subdivision:
   CWpt&  displaced_loc(LocCalc*);

   bool   has_offset()          const   { return _offset != 0; }
   double offset()              const   { return _offset; }
   void   add_offset(double d)          { set_offset(_offset + d); }
   void   clear_offset()                { set_offset(0); }
   void   set_offset(double d);

   // Return the smooth subdiv location (without the "detail")
   // that would be assigned to this vertex using the SubdivCalc
   // of the parent LMESH:
   Wpt smooth_loc_from_parent() const;

   // Return smooth_loc_from_parent() + added "detail" offset:
   Wpt detail_loc_from_parent() const;

   // Compute and store the offset needed to approximate the
   // given location via the smooth subdiv loc + detail offset.
   void fit_subdiv_offset(CWpt& detail_loc);

 protected:
   // Protected method called by the parent of this vertex
   // to set the smooth subdiv location.
   //
   // The given value base_loc is the smooth subdiv location
   // computed from the parent level; this vertex should now set
   // its location = base_loc + n*offset, where n is the normal
   // from the parent and offset is the subdiv offset ("detail").
   void set_subdiv_base_loc(CWpt& base_loc);

 public:

   //******** VALIDITY OF CACHED VALUES ********

   void mark_dirty(int bit = SUBDIV_LOC_VALID_BIT);
   void subdiv_color_changed()          { mark_dirty(SUBDIV_COLOR_VALID_BIT); }
   void subdiv_loc_changed() {
      clear_bit(DISPLACED_LOC_VALID);
      mark_dirty(SUBDIV_LOC_VALID_BIT);
   }

   //******** BVERT VIRTUAL METHODS ********

   virtual void geometry_changed();
   virtual void normal_changed();
   virtual void crease_changed();
   virtual void degree_changed();
   virtual void color_changed();
   virtual void mask_changed();

 //*******************************************************
 // PROTECTED
 //*******************************************************
 protected:
   Lvert*          _subdiv_vertex; // optional vertex generated by subdivision
   unsigned short  _corner;        // "variable sharpness" corner
   unsigned short  _mask;          // subdivision mask
   Bsimplex*       _parent;        // edge or vertex that has created this one
   double          _offset;        // subdivision "detail"
   Wpt             _displaced_loc; // x-perimental

   //******** PROTECTED METHODS ********

   void set_mask();

 private:
   // In rare cases, certain people can do the following.
   // However, you are not one of them.
   void assign_subdiv_vert(Lvert* v) { _subdiv_vertex = v; }
};

#endif  // LVERT_H_HAS_BEEN_INCLUDED

// end of file lvert.H
