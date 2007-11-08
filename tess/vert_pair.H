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
 * vert_pair.H
 *****************************************************************/
#ifndef VERT_PAIR_H_IS_INCLUDED
#define VERT_PAIR_H_IS_INCLUDED

#include "mesh/bvert.H"
#include "mesh/edge_strip.H"
#include "face_pair.H"

/*****************************************************************
 * VertPair:
 *
 *  Simplex data stored on a given vertex with an associated key,
 *  records a "partner" vertex.
 *
 *  Used by Rmemes (see Rsurf.H) to represent a surface that is
 *  "inflated" from a given surface region (as a Bface_list).
 *  VertPair is a SimplexData stored on vertices of the original
 *  faces via a known lookup key that is unique to those faces.
 *  Given the key, you can check any vertex in the original
 *  faces and find the corresponding "inflated" vertex. Can also
 *  find the normal (WRT the original face set) at any vertex of
 *  the original faces.
 *   
 *****************************************************************/
class VertPair : public SimplexData {
 public:
   //******** MANAGERS ********

   // using key, record data on v, associating to partner p
   VertPair(uint key, Bvert* v, Bvert* p) : SimplexData(key, v), _p(p) {}

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("VertPair", VertPair*, SimplexData, CSimplexData*);

   //******** ACCESSORS ********

   Bvert* partner() const { return _p; }

   // not sure if we want to be able to set the partner
   // outside the destructor

   //******** LOOKUPS ********

   // static methods useful when you know the key and want to
   // find the partner of a given vertex v

   // get the VertPair on v
   static VertPair* find_pair(uint key, CBvert* v) {
      return v ? upcast(v->find_data(key)) : 0;
   }

   // find the partner of v
   static Bvert* map_vert(uint key, CBvert* v) {
      VertPair* pair = find_pair(key, v);
      return pair ? pair->partner() : 0;
   }

   // return a normal WRT the original face set:
   static Wvec vert_normal(uint key, CBvert* v) {
      return v ? v->norm(FacePairFilter(key)) : Wvec::null();
   }

   // find the partners of a whole set of verts
   static bool map_verts(uint key, CBvert_list& verts, Bvert_list& ret) {
      ret.clear();
      for (int i=0; i<verts.num(); i++) {
         Bvert* v = map_vert(key, verts[i]);
         if (v)
            ret += v;
      }
      return (ret.num() == verts.num());
   }

   // map an edge strip to corresponding strip made up of partners
   // (returns matching strip on success, or empty strip on failure):
   static bool map_strip(uint key, CEdgeStrip& strip, EdgeStrip& ret) {
      ret.reset();
      for (int i=0; i<strip.num(); i++) {
         Bvert* u1 = map_vert(key, strip.vert(i));
         Bvert* u2 = map_vert(key, strip.next_vert(i));
         Bedge* e  = lookup_edge(u1, u2);
         if (!(u1 && u2 && e)) {
            ret.reset();
            return false;
         }
         ret.add(u1, e);
      }
      return true;
   }

 protected:
   Bvert* _p; // associated partner vertex
};

typedef const VertPair CVertPair;

#endif // VERT_PAIR_H_IS_INCLUDED

/* end of file vert_pair.H */
