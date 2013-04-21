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
#ifndef LFACE_H_HAS_BEEN_INCLUDED
#define LFACE_H_HAS_BEEN_INCLUDED

#include "lvert.H"
class LMESH;
/**********************************************************************
 * Lface:
 *    A face for LMESH, a subdivision mesh
 **********************************************************************/
class Lface : public Bface {
 public:
   //******** ENUMS ********
   enum {
      SUBDIV_ALLOCATED_BIT = Bface::NEXT_AVAILABLE_BIT,
      NEXT_AVAILABLE_BIT
   };


   //******** MANAGERS ********
   Lface(Lvert* u, Lvert* v, Lvert* w, Ledge* e, Ledge* f, Ledge* g) :
      Bface(u,v,w,e,f,g), _parent(0) {}

   virtual ~Lface() { delete_subdiv_elements(); }

   //******** ACCESSORS ********
   LMESH* lmesh()       const   { return (LMESH*)mesh(); }
   Lvert* lv(int k)     const   { return (Lvert*) v(k); }
   Ledge* le(int k)     const   { return (Ledge*) e(k); }
   

   //******** CONTROL ELEMENTS ********
   Lface* parent() const { return _parent; }

   void   set_parent(Lface* f) { _parent = f; }

   // Return the parent face at the given subdivision level
   // relative to this face.
   //   rel_level <= 0: this face
   //   rel_level == 1: immediate parent
   //   rel_level == 2: parent of parent,
   //   etc.
   //
   // If there is no parent at the requested level,
   // return the parent at the highest available level.
   Lface* parent(int rel_level);

   Lface* control_face() const {
      return _parent ? _parent->control_face() : (Lface*)this;
   }

   //******** BARYCENTRIC COORDINATE CONVERSION ********
   // Convert barycentric coordinates WRT this face to
   // corresponding coords WRT parent or child face:
   Lface* parent_bc(CWvec& bc, mlib::Wvec& ret) const;
   Lface* parent_bc(Wvec& bc) const { return parent_bc(bc,bc); }

   Lface* child_bc (CWvec& bc, mlib::Wvec& ret) const;
   Lface* child_bc (Wvec& bc) const { return child_bc(bc,bc); }

   // Find the parent or child Lface at the desired mesh level, return
   // that face, and convert the given barycentric coords WRT this
   // face to corresponding coords WRT the returned face.
   //
   // If the desired level isn't available, returns the Lface* and
   // barycentric coordinates of the closest level to it we can reach.
   //
   Lface* bc_to_level(int level, CWvec& bc, mlib::Wvec& ret)  const;
   Lface* bc_to_level(int level, Wvec& bc)  const {
      return bc_to_level(level, bc, bc);
   }

   // Similar to above, using the mesh edit level:
   Lface* bc_to_edit_level(Wvec& bc)  const {
      return bc_to_level(_mesh->edit_level(), bc);
   }
   
   //******** SUBDIVISION ELEMENTS ********
   bool subdiv_dirty()  const   { return is_clear(SUBDIV_ALLOCATED_BIT); }
   void allocate_subdiv_elements();
   void set_subdiv_elements();
   void delete_subdiv_elements();

   // subdiv edges
   Ledge *subdiv_edge1() const {
      return (Ledge*)lookup_edge(le(1)->subdiv_vertex(),
                                 le(2)->subdiv_vertex());
   }
   Ledge *subdiv_edge2() const {
      return (Ledge*)lookup_edge(le(2)->subdiv_vertex(),
                                 le(3)->subdiv_vertex());
   }
   Ledge *subdiv_edge3() const {
      return (Ledge*)lookup_edge(le(3)->subdiv_vertex(),
                                 le(1)->subdiv_vertex());
   }

   // subdiv faces
   Lface *subdiv_face_center() const {
      return (Lface*) lookup_face(subdiv_edge1(), subdiv_edge2());
   }
   Lface *subdiv_face1() const {
      return (Lface*) lookup_face(subdiv_edge3(),lv(1)->subdiv_vertex());
   }
   Lface *subdiv_face2() const {
      return (Lface*) lookup_face(subdiv_edge1(),lv(2)->subdiv_vertex());
   }
   Lface *subdiv_face3() const {
      return (Lface*) lookup_face(subdiv_edge2(),lv(3)->subdiv_vertex());
   }
   
   void append_subdiv_faces(int lev, ARRAY<Bface *> &faces);

   virtual void color_changed();

   //******** NON-MANIFOLD MESHES ********

   // Label the face as "primary" or "secondary." I.e., as being
   // in the primary layer or not, respectively. In Lface, the
   // same action is passed down to all child faces.
   virtual void make_primary();
   virtual void make_secondary();

   // Set the layer number and pass it down to any children that exist:
   virtual void set_layer(ushort l);

   //******** BUILDING/REDEFINING ********

   virtual int  detach();
   virtual void reverse();

 protected:
   Lface* _parent;

   //******** PROTECTED METHODS ********
   virtual void set_patch(Patch* p);

   // Helper function used in Lface::set_patch():
   void set_child_patch(Lface* subface, Patch*& child);

   // Protected helper function used in Lface::gen_child_face()
   // and Lface::set_subdiv_elements().
   //
   // Record this face as parent of given child face, and
   // propagate attributes to the child.
   //
   // 'center_face' is true for the center face, whose edges
   // consider this face to be their parent.
   //
   // It's protected because we assume child legitimately
   // belongs to this face.
   void claim_child(Lface* child, bool center_face=false);

   // Helper function used in Lface::allocate_subdiv_elements().
   // Creat a child face and copy selected attributes from the
   // parent to the child.
   Lface* gen_child_face(Bvert* v1, Bvert* v2, Bvert* v3,
                         Patch* p, LMESH* m, bool center_face=false);
};

/*****************************************************************
 * inlines
 *****************************************************************/
inline Lface*
get_child(Lface* f, int level)
{
   // Return f's child face at the given level
   // (relative to the level of f).
   //
   // XXX - chooses the center child at each step.

   while (f && level > 0) {
      f->allocate_subdiv_elements();
      f = f->subdiv_face_center();
      level--;
   }
   return f;
}

inline Lface*
get_parent(Lface* f, int level)
{   
   // Return f's parent face at the given level
   // (relative to the level of f).

   return f ? f->parent(level) : 0;
}

inline Lface* 
remap(Lface* f, int level)
{
   // Given a face, remap to a corresponding face at a
   // given subdivision level RELATIVE TO the given face.
   //
   // E.g. remap(f, 1) yields the center child face one
   // subdivision level finer than the given face.
   // remap(f, -1) yields the immediate parent, and
   // remap(f, -2) yields the parent of the parent, etc.
   // However, if the face is already at the control
   // level, and 'level' < 0, then remap(f, level) just
   // returns f.

   if (!f)
      return 0;                         // bad input
   if (level == 0)
      return f;                         // nothing to do
   if (level > 0)
      return get_child(f, level);       // go to finer level

   return get_parent(f, -level);        // go to coarser level:
}

inline Lface* 
Ledge::ctrl_face() const 
{
   Bsimplex* sim = ctrl_element();
   return is_face(sim) ? (Lface*)sim : (Lface*)0;
}

#endif  // LFACE_H_HAS_BEEN_INCLUDED

// end of file lface.H
