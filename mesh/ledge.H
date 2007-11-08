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
#ifndef LEDGE_H_HAS_BEEN_INCLUDED
#define LEDGE_H_HAS_BEEN_INCLUDED

#include <climits>
#include "bmesh.H"

class Lvert;
class Lface;
class LMESH;

#define CLvert const Lvert
#define CLedge const Ledge
#define CLface const Lface
#define CLMESH const LMESH
/**********************************************************************
 * Ledge:
 *
 *    An edge of an LMESH (subdivision mesh)
 **********************************************************************/
class Ledge : public Bedge {
   friend class REPARENT_CMD;
 public:

   //******** ENUMS ********

   enum mask_t {
      REGULAR_SMOOTH_EDGE = 0,
      REGULAR_CREASE_EDGE,
      NON_REGULAR_CREASE_EDGE
   };

   enum {
      SUBDIV_LOC_VALID_BIT = Bedge::NEXT_AVAILABLE_BIT,
      SUBDIV_COLOR_VALID_BIT,
      SUBDIV_CREASE_VALID_BIT,
      MASK_VALID_BIT,
      SUBDIV_ALLOCATED_BIT,
      NEXT_AVAILABLE_BIT
   };


   //******** MANAGERS ********

   Ledge(Lvert* u, Lvert* v) :
      Bedge((Bvert*)u,(Bvert*)v),
      _subdiv_vertex(0),
      _crease(0),
      _mask(REGULAR_SMOOTH_EDGE),
      _parent(0) {}
   
   virtual ~Ledge();

   //******** ACCESSORS ********

   LMESH*  lmesh()      const   { return (LMESH*)mesh(); }
   Lvert*  lv(int k)    const   { return (Lvert*) v(k); }
   Lface*  lf(int k)    const   { return (Lface*) f(k); }

   //******** SUBDIVISION ELEMENTS ********

   Lvert*  subdiv_vertex()const   { return _subdiv_vertex; }

   // child edges
   Ledge*  subdiv_edge1() const;
   Ledge*  subdiv_edge2() const;

   // child edge of adjacent face (1 or 2) that is parallel to this:
   Ledge*  parallel_sub_edge(int k) const;

   void    allocate_subdiv_elements();
   void    set_subdiv_elements(Lvert* subv);
   void    delete_subdiv_elements();
   void    update_subdivision();

   // Return an ordered list of edges at the given subdiv level
   // *relative* to the level of this edge. (Level must be >= 0).
   // E.g. level 1 means one level down the subdivision hierarchy.
   // The returned edges run in the direction of v1 to v2.
   bool append_subdiv_edges(int lev, ARRAY<Bedge *> &edges); 

   // Similar to append_subdiv_edges(), but returns the
   // ordered list of subdiv verts at the given level lev >= 0:
   bool get_subdiv_verts(int lev, Bvert_list& ret);

   // not for public use, to be called by the child
   void    subdiv_vert_deleted();

   //******** NON-MANIFOLD MESHES ********

 protected:
   // Add the face as a secondary (or "multi") face.
   // Then do the same for the appropriate child faces.
   virtual bool add_multi(Bface* f); 

   // Add the face as a primary face (in slot _f1 or _f2).
   // In Ledge, do the same for the appropriate child faces.
   virtual bool add_primary(Bface* f); 

 public:

   // Bedge::can_promote() returns true if there is at least one
   // available primary slot (_f1 or _f2).  In Ledge, all child
   // edges must also have available slots.
   virtual bool can_promote() const {
      Ledge* s = 0;
      return (Bedge::can_promote() &&
              (!(s = subdiv_edge1()) || s->can_promote()) &&
              (!(s = subdiv_edge2()) || s->can_promote()));
   }

   // The following is used in Lface::allocate_subdiv_elements()
   // and also in Ledge::add_multi().  If the given face is a
   // secondary face WRT this edge, then ensure its children have
   // the same relationship WRT the child edges of this edge.
   void push_multi(Bface* f);

   // Used in Ledge::add_primary(); similar to above.
   void push_primary(Bface* f);

   //******** CONTROL ELEMENTS ********

   // proper parent
   // (it's null for edges of the control mesh):
   Bsimplex* parent() const { return _parent; }

   // Get parent edge (it it exists) at the given relative
   // level up from this edge
   Ledge* parent_edge(int rel_level) const;

   // control element
   // (it's the edge itself in the control mesh):
   Bsimplex* ctrl_element() const;

   // convenience method - casts control element to Ledge if valid:
   Ledge* ctrl_edge() const {
      Bsimplex* sim = ctrl_element();
      return is_edge(sim) ? (Ledge*)sim : (Ledge*)0;
   }
   Lface* ctrl_face() const;    // defined in bface.H

   void set_parent(Bsimplex* p) { _parent = p; }

   // Difference between subdiv level of this edge and its control element
   // (See Ledge):
   // XXX - move to Bsimplex?
   virtual uint rel_level() const;

   //******** SUBDIVISION MASK ********

   void set_mask();
   unsigned short subdiv_mask() const;
   int is_smooth() const { return !is_crease(); }

   //******** CACHED DATA ********

   void    subdiv_loc_changed()    { clear_bit(SUBDIV_LOC_VALID_BIT);   }
   void    subdiv_color_changed()  { clear_bit(SUBDIV_COLOR_VALID_BIT); }

   //******** Bedge VIRTUAL METHODS ********

   // crease management
   virtual void set_crease(unsigned short c = USHRT_MAX);
   virtual unsigned short crease_val() const { return _crease; }

   // cached data management
   virtual void normal_changed();
   virtual void geometry_changed();
   virtual void color_changed();
   virtual void crease_changed();
   virtual void faces_changed();    // called when faces are added or deleted

   virtual void mask_changed();

   // building/redefining
   virtual int  redefine(Bvert *v, Bvert *u);
   virtual void set_new_vertices(Bvert *v1, Bvert *v2);

   // Carry out swap if it is legal:
   virtual bool do_swap();

 protected:
   Lvert*         _subdiv_vertex; // vertex generated by subdivision
   unsigned short _crease;        // variable sharpness crease
   unsigned short _mask;          // subdivision mask id
   Bsimplex*      _parent;        // face or an edge that created this edge

   //******** PROTECTED METHODS ********

   // Helper functions used in Ledge::allocate_subdiv_elements() and
   // Ledge::set_subdiv_elements().
   //
   // Record this edge as parent of given child element, and propagate
   // attributes to the child.
   //
   // It's protected because we assume child legitimately
   // belongs to this edge.
   void claim_child(Ledge* child);
   void claim_child(Lvert* child);

   // Used in push_multi() and push_primary().
   // Full comments in ledge.C:
   void get_sub_faces(Bface* f, Bedge* &e1, Bface* &sf1, Bedge* &e2, Bface* &sf2);

 private:
   // In rare cases, certain people can do the following.
   // However, you are not one of them.
   void assign_subdiv_vert(Lvert* v) { _subdiv_vertex = v; }
};

/*****************************************************************
 *  get_subdiv_chain:
 *
 *     Given vertices v1 and v2 joined by an Ledge, recursively
 *     extract chain of subdivision vertices generated along the
 *     edge at the given level (relative to the original edge):
 *****************************************************************/
bool get_subdiv_chain(Bvert* v1, Bvert* v2, int level, Bvert_list& ret);

/*****************************************************************
 *  get_subdiv_chain:
 *
 *     Given a connected chain of vertices, recursively extract
 *     the corresponding chain of subdivision vertices at the
 *     given level (relative to the original chain). On failure
 *     returns the empty list.
 *****************************************************************/
bool get_subdiv_chain(CBvert_list& chain, int level, Bvert_list& ret);


/*****************************************************************
 * EdgeChildFilter:
 *
 *   Accepts an edge that was generated in subdivision by an edge
 *   in the next coarser-level mesh.
 *
 *   NB: Only use this with class Ledge.
 *****************************************************************/
class EdgeChildFilter : public SimplexFilter {
 public:
   virtual bool accept(CBsimplex* s) const {
      return is_edge(s) && is_edge(((Ledge*)s)->parent());
   }
};

#endif  // LEDGE_H_HAS_BEEN_INCLUDED

// end of file ledge.H
