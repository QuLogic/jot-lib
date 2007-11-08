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
#include "mesh/lmesh.H"
#include "mesh/patch.H"

using namespace mlib;

void 
Lface::color_changed()
{
   for (int i = 1; i <= 3; i++) {
      lv(i)->subdiv_color_changed();
      le(i)->subdiv_color_changed();
   }
}

void
Lface::delete_subdiv_elements()
{
   // Make this lightweight, so you can call
   // it when you're not sure if you need to:
   if (!is_set(SUBDIV_ALLOCATED_BIT))
      return;
   clear_bit(SUBDIV_ALLOCATED_BIT);

   // After this we need to get treated as a "dirty" face.
   // That means the vertices have to be dirty
   lv(1)->mark_dirty();
   lv(2)->mark_dirty();
   lv(3)->mark_dirty();

   LMESH* submesh = lmesh()->subdiv_mesh();
   assert(submesh);

   // Removing a face (or edge) also causes it to remove
   // *its* subdiv elements.  I.e., this is recursive.

   Lface* subface;
   if ((subface = subdiv_face_center()))
      submesh->remove_face(subface);
   if ((subface = subdiv_face1()))
      submesh->remove_face(subface);
   if ((subface = subdiv_face2()))
      submesh->remove_face(subface);
   if ((subface = subdiv_face3()))
      submesh->remove_face(subface);

   Ledge* subedge;
   if ((subedge = subdiv_edge1()))
      submesh->remove_edge(subedge);
   if ((subedge = subdiv_edge2()))
      submesh->remove_edge(subedge);
   if ((subedge = subdiv_edge3()))
      submesh->remove_edge(subedge);
}

inline void
reverse_face(Bface* f)
{
   if (f)
      f->reverse();
}

void 
Lface::reverse()
{
   if (is_set(SUBDIV_ALLOCATED_BIT)) {
      reverse_face(subdiv_face_center());
      reverse_face(subdiv_face1());
      reverse_face(subdiv_face2());
      reverse_face(subdiv_face3());
   }
   Bface::reverse();
}

int
Lface::detach()
{
   delete_subdiv_elements();
   return Bface::detach();
}

// Helper used in Lface::set_layer() below
inline void
set_layer(ushort l, Lface* f)
{
   if (f)
      f->set_layer(l);
}

void 
Lface::set_layer(ushort l)
{
   if (_layer == l)
      return;

   Bface::set_layer(l);

   // Pass the layer number down to any children that exist:
   if (is_set(SUBDIV_ALLOCATED_BIT)) {
      ::set_layer(_layer, subdiv_face1());
      ::set_layer(_layer, subdiv_face2());
      ::set_layer(_layer, subdiv_face3());
      ::set_layer(_layer, subdiv_face_center());
   }
}

// Helper used in Lface::make_secondary() below
inline void
push_secondary(Lface* f)
{
   if (f)
      f->make_secondary();
}

void 
Lface::make_secondary()
{
   // (For non-manifold meshes).
   // Label the face as "secondary." I.e.,
   // as not being in the primary layer

   if (is_secondary())
      return;

   Bface::make_secondary();
   
   // Pass the label down to any children that exist:
   if (is_set(SUBDIV_ALLOCATED_BIT)) {
      push_secondary(subdiv_face1());
      push_secondary(subdiv_face2());
      push_secondary(subdiv_face3());
      push_secondary(subdiv_face_center());
   }
}

// Helper used in Lface::make_primary() below
inline void
push_primary(Lface* f)
{
   if (f)
      f->make_primary();
}

void 
Lface::make_primary()
{
   // (For non-manifold meshes).
   // Label the face as "primary." I.e.,
   // as not being in the primary layer

   if (is_primary())
      return;

   Bface::make_primary();
   
   // Pass the label down to any children that exist:
   if (is_set(SUBDIV_ALLOCATED_BIT)) {
      push_primary(subdiv_face1());
      push_primary(subdiv_face2());
      push_primary(subdiv_face3());
      push_primary(subdiv_face_center());
   }
}

Lface* 
Lface::gen_child_face(
   Bvert* v1,
   Bvert* v2,
   Bvert* v3,
   Patch* p,
   LMESH* m,
   bool center_face
   )
{
   // Lface Internal method.

   if (!(v1 && v2 && v3 && m))
      return 0;

   // Create child of this face
   Lface* child = (Lface*)m->add_face(v1, v2, v3, p);

   // Record this face as its parent, and propagate any
   // attributes that it should inherit
   claim_child(child, center_face);

   return child;
}

void
Lface::claim_child(Lface* child, bool center_face)
{
   // Protected helper function used in Lface::gen_child_face()
   // and Lface::set_subdiv_elements().
   //
   // Record this face as parent of given child face, and
   // propagate attributes to the child.

   // 'center_face' is true for the center face, whose edges
   // consider this face to be their parent.

   // It's protected because we assume child legitimately
   // belongs to this face.

   if (!child)
      return;

   // Record this face as the child's parent
   assert(child->parent() == NULL);
   child->set_parent(this);
   if (center_face) {
	  child->le(1)->set_parent(this);
      child->le(2)->set_parent(this);
      child->le(3)->set_parent(this);
   }

   // Propagate attributes to child:
   if (is_secondary())
      child->make_secondary();
   child->set_layer(_layer);

   // Add more attributes here... (when needed).
}

void 
Lface::allocate_subdiv_elements()
{
   // Generate 4 faces and 3 internal edges
   // in the subdivision mesh next level down.

   // NOTE: The specific order that sub-faces are created,
   // and the order of the vertices used in creating them,
   // should not be changed. The barycentric coordinate
   // conversion routines (below) depend on these orderings.

   // Make this lightweight, so you can call
   // it when you're not sure if you need to:
   if (is_set(SUBDIV_ALLOCATED_BIT))
      return;
   set_bit(SUBDIV_ALLOCATED_BIT);

   assert(lmesh() != NULL);
   lmesh()->allocate_subdiv_mesh();
   LMESH* submesh = lmesh()->subdiv_mesh();
   assert(submesh != NULL);


   //                       lv3                                       #
   //                        /\                                       #
   //                       /3 \                                      #
   //                      /    \                                     #
   //                     /      \                                    #
   //                    / child3 \                                   #
   //                   /          \                                  #
   //                  / 1        2 \                                 #
   //            le3  /______________\ le2                            #
   //                /\ 2          1 /\                               #
   //               /3 \            /3 \                              #
   //              /    \  center  /    \                             #
   //             /      \  child /      \                            #
   //            /        \      /        \                           #
   //           /  child1  \    /  child2  \                          #
   //          /            \3 /            \                         #
   //         / 1          2 \/ 1          2 \                        #
   //     lv1 -------------------------------- lv2                    #
   //                       le1                                       #

   // Make sure subdiv elements have
   // been allocated around face boundary:
   lv(1)->allocate_subdiv_vert();
   lv(2)->allocate_subdiv_vert();
   lv(3)->allocate_subdiv_vert();

   le(1)->allocate_subdiv_elements();
   le(2)->allocate_subdiv_elements();
   le(3)->allocate_subdiv_elements();

   Patch* child_patch = _patch ? _patch->get_child() : 0;

   // hook up 4 faces (verifying they're not already there):
   if (!subdiv_face1())
      gen_child_face(
         lv(1)->subdiv_vertex(),
         le(1)->subdiv_vertex(),
         le(3)->subdiv_vertex(),
         child_patch, submesh);

   if (!subdiv_face2())
      gen_child_face(
         le(1)->subdiv_vertex(),
         lv(2)->subdiv_vertex(),
         le(2)->subdiv_vertex(),
         child_patch, submesh);

   if (!subdiv_face3())
      gen_child_face(
         le(3)->subdiv_vertex(),
         le(2)->subdiv_vertex(),
         lv(3)->subdiv_vertex(),
         child_patch, submesh);

   if (!subdiv_face_center())
      gen_child_face(
         le(2)->subdiv_vertex(),
         le(3)->subdiv_vertex(),
         le(1)->subdiv_vertex(),
         child_patch, submesh, true);   // true: face is at center

   if (is_quad()) {
      //
      //       o                                         
      //       | .                                       
      //       |   .                                     
      //       |     .  w                                
      //       |       .                                 
      //  sub1 o .----- o.                               
      //       |   .    |  .                             
      //       |     .  |    .                           
      //       |       .|      .                         
      //       o ------ o ----- o                        
      //      v       sub2                               
      //                                                 
      // A quad face is one that has a "weak" edge
      // (shown as the dotted edge labelled 'w' above).
      // A weak edge is considered to be the internal
      // diagonal edge of a quad, with this face making
      // up one half of the quad and the face on the
      // other side (not shown) making up the other
      // half. In subdivision we label the subdivision
      // edges accordingly. I.e. the edge connecting
      // subdivision vertices sub1 and sub2 should be
      // labelled weak.

      Bedge* w = weak_edge();
      Bvert* v = other_vertex(w);
      Bvert* sub1 = ((Ledge*)v->lookup_edge(w->v1()))->subdiv_vertex();
      Bvert* sub2 = ((Ledge*)v->lookup_edge(w->v2()))->subdiv_vertex();
      sub1->lookup_edge(sub2)->set_bit(Bedge::WEAK_BIT);
   }

   // Now that child faces are generated, propagate multi
   // status (if any) to children
   le(1)->push_multi(this);
   le(2)->push_multi(this);
   le(3)->push_multi(this);

   // Notify observers:
   if (_data_list)
      _data_list->notify_subdiv_gen();
}

void 
Lface::set_subdiv_elements()
{
   // Claim parental ownership of 4 faces and 3 internal
   // edges in the subdivision mesh next level down.

   if (is_set(SUBDIV_ALLOCATED_BIT)) {
      err_msg("Lface::set_subdiv_elements: elements already set");
      return;
   }
   set_bit(SUBDIV_ALLOCATED_BIT);

   // hook up the 4 faces 
   claim_child(subdiv_face1());
   claim_child(subdiv_face2());
   claim_child(subdiv_face3());
   claim_child(subdiv_face_center(), true);     // true: face is at center

   // Notify observers:
   if (_data_list)
      _data_list->notify_subdiv_gen();
}

//
// Helper function used in Lface::set_patch(), below:
//
void
Lface::set_child_patch(Lface* f, Patch*& child)
{
   // Do nothing for Mr. No-face:
   if (!f)
      return;

   if (_patch && (child || (child = _patch->get_child())))
      child->add(f);
   else
      f->set_patch(0);
}

void
Lface::set_patch(Patch* p)
{
   Bface::set_patch(p);

   // In case subfaces already exist, set the patch on each
   Patch* child_patch = 0;
   set_child_patch(subdiv_face1(),       child_patch);
   set_child_patch(subdiv_face2(),       child_patch);
   set_child_patch(subdiv_face3(),       child_patch);
   set_child_patch(subdiv_face_center(), child_patch);
}

inline void
get_subdiv_faces(Lface* f, int lev, ARRAY<Bface*>& faces)
{
   if (f) 
      f->append_subdiv_faces(lev, faces);
}

void 
Lface::append_subdiv_faces(int lev, ARRAY<Bface*>& faces)
{
   if (lev < 0) {
      err_msg("Lface::append_subdiv_faces: error: bad level: %d", lev);
      return;
   } else if (lev == 0) {
      faces += this;
   } else {
      get_subdiv_faces(subdiv_face1(),       lev-1, faces);
      get_subdiv_faces(subdiv_face2(),       lev-1, faces);
      get_subdiv_faces(subdiv_face3(),       lev-1, faces);
      get_subdiv_faces(subdiv_face_center(), lev-1, faces);
   }
}

////////////////////////////////////////////////////////////
//
//      Barycentric coordinate conversions
//
//        These rely on the following relationship
//        between a given Lface and its 4 child
//        sub-faces:
//                                                                 #
//                        v3                                       #
//                                                                 #
//                        /\                                       #
//                       /3 \                                      #
//                      /    \                                     #
//                     /      \                                    #
//                    / child3 \                                   #
//                   /          \                                  #
//                  / 1        2 \                                 #
//            e3   /______________\ e2                             #
//                /\ 2          1 /\                               #
//               /3 \            /3 \                              #
//              /    \  center  /    \                             #
//             /      \  child /      \                            #
//            /        \      /        \                           #
//           /  child1  \    /  child2  \                          #
//          /            \3 /            \                         #
//         / 1          2 \/ 1          2 \                        #
//      v1 -------------------------------- v2                     #
//                       e1                                        #
//                                                                 #
//   The following matrices convert between barycentric
//   coordinates with respect to a parent triangle and its
//   children.
//
static Wtransf c1_to_p(Wvec(1.0, 0.0, 0.0),     // child 1 to parent
                       Wvec(0.5, 0.5, 0.0),
                       Wvec(0.5, 0.0, 0.5));

static Wtransf c2_to_p(Wvec(0.5, 0.5, 0.0),     // child 2 to parent
                       Wvec(0.0, 1.0, 0.0),
                       Wvec(0.0, 0.5, 0.5));

static Wtransf c3_to_p(Wvec(0.5, 0.0, 0.5),     // child 3 to parent
                       Wvec(0.0, 0.5, 0.5),
                       Wvec(0.0, 0.0, 1.0));

static Wtransf cc_to_p(Wvec(0.0, 0.5, 0.5),     // center child to parent
                       Wvec(0.5, 0.0, 0.5),
                       Wvec(0.5, 0.5, 0.0));

static Wtransf p_to_c1 = c1_to_p.inverse();     // parent to child 1
static Wtransf p_to_c2 = c2_to_p.inverse();     // parent to child 2
static Wtransf p_to_c3 = c3_to_p.inverse();     // parent to child 3
static Wtransf p_to_cc = cc_to_p.inverse();     // parent to center child

Lface*
Lface::child_bc(
   CWvec& bc,   // Barycentric coordinates of a point inside this face
   Wvec& ret    // Corresponding barycentric coords WRT corresponding sub-face
   ) const
{
   // For the given barycentric coordinates 'bc' WRT this
   // face, convert to barycentric coords WRT the
   // corresponding child face (i.e. subdivision face).
   //
   // Returns the sub-face, and sets 'ret' to the computed
   // barycentric coords WRT that face.
   //
   // Note: it's okay for 'ret' to actually be a reference to
   // 'bc'.

   // Reject the input if bc has any negative coordinates.
   // Negative coordinates occur when the point is outside the
   // triangle, and so does not correspond to any child face.
   //
   // XXX - we're not checking that the coordinates sum to 1,
   // which is a prerequisite for this whole thing to make
   // sense.
   if ((bc[0] < 0) || (bc[1] < 0) || (bc[2] < 0)) {
      err_msg("Lface::child_bc: barycentric coords outside face (%f,%f,%f)",
              bc[0], bc[1], bc[2]);
      return 0;
   }

   if (bc[0] >= 0.5) {
      // Child 1
      ret = p_to_c1 * bc;
      return subdiv_face1();
   } else if (bc[1] >= 0.5) {
      // Child 2
      ret = p_to_c2 * bc;
      return subdiv_face2();
   } else if (bc[2] >= 0.5) {
      // Child 3
      ret = p_to_c3 * bc;
      return subdiv_face3();
   } else {
      // Center child
      ret = p_to_cc * bc;
      return subdiv_face_center();
   }
}

Lface*
Lface::parent_bc(CWvec& bc, Wvec& ret) const
{
   // Converts barycentric coordinates WRT this Lface to
   // barycentric coords WRT its parent.
   //
   // See child_bc() above for more discussion.

   if (!_parent) {
      err_msg("Lface::parent_bc: Error: no parent");
      return 0;
   }

   // Get parent simplex of v1:
   Bsimplex* v1p = lv(1)->parent();
   if (!v1p) {
      err_msg("Lface::parent_bc: Error: can't get v1 parent");
      return 0;
   }

   if (v1p == _parent->v1()) {
      // We are child 1
      ret = c1_to_p * bc;
   } else if (v1p == _parent->e1()) {
      // We are child 2
      ret = c2_to_p * bc;
   } else if (v1p == _parent->e3()) {
      // We are child 3
      ret = c3_to_p * bc;
   } else if (v1p == _parent->e2()) {
      // We are the center child 
      ret = cc_to_p * bc;
   } else {
      // We are screwed
      err_msg("Lface::parent_bc: Error: can't determine child");
      return 0;
   }

   return _parent;
}

Lface*                  // Corresponding Lface at desired mesh level
Lface::bc_to_level(
   int level,           // Desired mesh level to convert to
   CWvec& bc,           // Given barycentric coords WRT this face
   Wvec& ret_bc         // Corresponding barycentric coords WRT returned face
   ) const
{
   // Find the parent or child Lface at the desired mesh level, return
   // that face, and convert the given barycentric coords WRT this
   // face to corresponding coords WRT the returned face.
   //
   // If the desired level isn't available, returns the Lface* and
   // barycentric coordinates of the closest level to it we can reach.

   // Get the difference between the desired level and the
   // mesh level of this Lface:
   int delta_level = level - lmesh()->subdiv_level();

   CLface*     f = this, *p = 0;
   Wvec   tmp_bc = bc;

   ret_bc = bc;

   if (delta_level < 0) {
      // We have to go coarser
      for (int i = -delta_level; i>0; i--) {
         p = f->parent_bc(tmp_bc);
         if (p) {
            ret_bc = tmp_bc;
            f = p;
         } else
            break;
      }
   } else if (delta_level > 0) {
      // We have to go finer
      for (int i = delta_level; i>0; i--) {
         p = f->child_bc(tmp_bc);
         if (p) {
            ret_bc = tmp_bc;
            f = p;
         } else
            break;
      }
   } else {
      // They are toying with us...
      // We're at the correct level already
   }

   // XXX -- if we couldn't reach the requested level, this returns
   // the last reachable face at some coarser level
   return (Lface*)f;    // corresponding face at desired level
}

Lface* 
Lface::parent(int rel_level)
{
   // Return the parent face at the given subdivision level
   // relative to this face.
   //   rel_level <= 0: this face
   //   rel_level == 1: immediate parent
   //   rel_level == 2: parent of parent,
   //   etc.
   //
   // If there is no parent at the requested level,
   // return the parent at the highest available level.

   Lface* ret = (Lface*) this;
   for ( ; rel_level > 0 && ret->parent() != NULL; rel_level--)
      ret = ret->parent();

   return ret;
}

/* end of file lface.C */


