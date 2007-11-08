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
/*****************************************************************
 * vert_mapper.H
 *
 *   Class that tracks correspondence between two sets of vertices.
 *
 *****************************************************************/
#ifndef VERT_MAPPER_H_IS_INCLUDED
#define VERT_MAPPER_H_IS_INCLUDED

#include "mesh/edge_strip.H"
#include "mesh/bface.H"

/*****************************************************************
 * VertMapper
 *
 *    Maps a set of vertices (A) to another set (B).
 *    Requires that A and B have the same size, and that A
 *    does not contain any duplicates.  Can also map B to A,
 *    but that requires that B contains no duplicates.
 *
 *****************************************************************/
class VertMapper;
typedef const VertMapper CVertMapper;
class VertMapper {
 public:
   //******** MANAGERS ********

   VertMapper(bool enable_b_to_a=false);
   VertMapper(CBvert_list& A, CBvert_list& B, bool enable_b_to_a=false);

   VertMapper(CVertMapper& mapper);
   VertMapper& operator=(CVertMapper& mapper);

   virtual ~VertMapper() { clear(); }

   // Clear the lists, removing simplex data
   void clear() { _a_verts.clear(); _b_verts.clear(); }

   // Create a mapper for two vertex lists of equal size.
   // Returns true if successful (A and B are valid).  A and
   // B are valid if they are the same size and A contains
   // no duplicates. If the map is dual, then B must also
   // contain no duplicates.  Does nothing if A and B are
   // invalid; returns false.
   bool set(CBvert_list& A, CBvert_list& B);

   // Create a mapper for two vertex lists of equal size.
   // Fails if A and B are invalid.
   bool add(CBvert_list& A, CBvert_list& B);

   // Add a new pair of corresponding vertices
   // (if a and b are valid):
   bool add(Bvert* a, Bvert* b);

   bool is_valid() const { 
      return ((_a_verts.num() == _b_verts.num()) &&
              !_a_verts.has_duplicates() &&
              !(_dual && _b_verts.has_duplicates()));
   }

   //******** ACCESSORS ********

   CBvert_list& A() const { return _a_verts; }
   CBvert_list& B() const { return _b_verts; }

   // Inner edges of A verts, excluding those edges
   // edges that cannot be mapped to B:
   Bedge_list a_edges() const;

   int num_verts() const { return is_valid() ? _a_verts.num() : 0; }
   bool is_empty() const { return num_verts() == 0; }

   //******** A --> B MAPPING ********

   // Given side A element, return corresponding side B element:
   Bvert* a_to_b(Bvert* a) const {
      int i = _a_verts.get_index(a);
      return _b_verts.valid_index(i) ? _b_verts[i] : 0;
   }
   Bedge* a_to_b(Bedge* a) const {
      if (!a) return 0;
      Bvert* v1 = a_to_b(a->v1());
      if (v1->get_all_faces().empty())
         v1 = a->v1();
      Bvert* v2 = a_to_b(a->v2());
      if (v2->get_all_faces().empty())
         v2 = a->v2();
      return lookup_edge(v1, v2);
   }
   Bface* a_to_b(Bface* a) const {
      if (!a) return 0;
      Bvert* v1 = a_to_b(a->v1());
      if (v1->get_all_faces().empty())
         v1 = a->v1();
      Bvert* v2 = a_to_b(a->v2());
      if (v2->get_all_faces().empty())
         v2 = a->v2();
      Bvert* v3 = a_to_b(a->v3());
      if (v3->get_all_faces().empty())
         v3 = a->v3();
      return lookup_face(v1, v2, v3);
   }

   // Map side-A elements to corresponding side-B elements,
   // but if any element can't be mapped returns the empty list:
   Bvert_list a_to_b(CBvert_list& A) const {
      return map_verts(A, &VertMapper::a_to_b);
   }
   Bedge_list a_to_b(CBedge_list& A) const {
      return map_edges(A, &VertMapper::a_to_b);
   }
   Bface_list a_to_b(CBface_list& A) const {
      return map_faces(A, &VertMapper::a_to_b);
   }
   EdgeStrip a_to_b(CEdgeStrip& A) const {
      return map_strip(A, &VertMapper::a_to_b, &VertMapper::a_to_b);
   }

   //******** B --> A MAPPING ********

   // Given side B element, return corresponding side A element:
   Bvert* b_to_a(Bvert* b) const {
      if (!_dual) return 0;
      int i = _b_verts.get_index(b);
      return _a_verts.valid_index(i) ? _a_verts[i] : 0;
   }
   Bedge* b_to_a(Bedge* b) const {
      if (!b) return 0;
      return lookup_edge(b_to_a(b->v1()), b_to_a(b->v2()));
   }
   Bface* b_to_a(Bface* b) const {
      if (!b) return 0;
      return lookup_face(b_to_a(b->v1()), b_to_a(b->v2()), b_to_a(b->v3()));
   }

   // Map side-B elements to corresponding side-A elements,
   // but if any element can't be mapped returns the empty list:
   Bvert_list b_to_a(CBvert_list& B) const {
      return _dual ? map_verts(B, &VertMapper::b_to_a) : Bvert_list();
   }
   Bedge_list b_to_a(CBedge_list& B) const {
      return _dual ? map_edges(B, &VertMapper::b_to_a) : Bedge_list();
   }
   Bface_list b_to_a(CBface_list& B) const {
      return _dual ? map_faces(B, &VertMapper::b_to_a) : Bface_list();
   }
   EdgeStrip b_to_a(CEdgeStrip& B) const {
      return _dual ? map_strip(B, &VertMapper::b_to_a, &VertMapper::b_to_a) :
         EdgeStrip();
   }

 protected:
   Bvert_list   _a_verts;     // side A verts
   Bvert_list   _b_verts;     // side B verts
   bool         _dual;        // true means can map b to a

   typedef Bvert* (VertMapper::*map_vert_meth_t)(Bvert*) const;
   typedef Bedge* (VertMapper::*map_edge_meth_t)(Bedge*) const;
   typedef Bface* (VertMapper::*map_face_meth_t)(Bface*) const;

   // allows us to map A --> B or vice versa
   Bvert_list map_verts(CBvert_list& in, map_vert_meth_t) const;
   Bedge_list map_edges(CBedge_list& in, map_edge_meth_t) const;
   Bface_list map_faces(CBface_list& in, map_face_meth_t) const;
   EdgeStrip  map_strip(CEdgeStrip& in, map_vert_meth_t, map_edge_meth_t) const;
};

#endif // VERT_MAPPER_H_IS_INCLUDED

// end of file vert_mapper.H
