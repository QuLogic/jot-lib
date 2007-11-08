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
#ifndef PATCH_BLEND_WEIGHT_IS_INCLUDED
#define PATCH_BLEND_WEIGHT_IS_INCLUDED

#include <map>

#include "bmesh.H"

/*****************************************************************
 * PatchBlendWeight:
 *
 *   assigns weights to vertices to represent the strength
 *   of various patches at the given vertex.
 *
 *   used in dynamic 2D patterns for blending patterns near
 *   patch boundaries.
 *****************************************************************/
class PatchBlendWeight : public SimplexData {
 public:
   // no public constructor: all static interface

   //******** STATICS ********

   // reset and then compute all weights, using
   // the specified number of smoothing steps:
   static void compute_all(CBMESH* mesh, int num_passes);

   // remove all weights stored on the mesh:
   static void clear(CBMESH* mesh);

   // get the weight of the given patch at the given vertex:
   static double get_weight(Patch* p, CBvert* v);

 protected:
   //******** MEMBER DATA ********
   map<Patch*,double>   _map; // associates a patch with a weight

   typedef map<Patch*,double>::const_iterator citer_t;
   typedef map<Patch*,double>::      iterator  iter_t;

   //******** MANAGERS ********
   PatchBlendWeight(Bvert* v) : SimplexData(key(), v) {}

   //******** RUN TIME TYPE ID ********
   DEFINE_RTTI_METHODS3(
      "PatchBlendWeight", PatchBlendWeight*, SimplexData, CSimplexData*
      );

   //******** ACCESSORS ********
   Bvert* vert() const { return (Bvert*)simplex(); }

   // once weights have all been computed, look them up:
   double get_weight(Patch* p) const {
      if (!p) return 0;
      citer_t pos = _map.find(p);
      return (pos == _map.end()) ? 0 : pos->second;
   }
   void set_weight(Patch* p, double w) { _map[p] = w; }

   //******** LOOKUP ********

   // the unique ID used to lookup PatchBlendWeight:
   static uint key() {
      // XXX - not 64-bit safe:
      return uint(**static_name());
   }

   // Lookup a PatchBlendWeight* from a Bvert
   static PatchBlendWeight* lookup(CBvert* v) {
      return v ? dynamic_cast<PatchBlendWeight*>(v->find_data(key())) : 0;
   }
   // Similar to lookup, but creates a PatchBlendWeight if it wasn't found
   static PatchBlendWeight* get_data(Bvert* v) {
      PatchBlendWeight* ret = lookup(v);
      return ret ? ret : v ? new PatchBlendWeight(v) : 0;
   }

   //******** COMPUTING WEIGHTS ********
   // initialize a set of vertices:
   static void init(CBvert_list verts);
   // initialize a single vertex:
   static void init(Bvert* v);
   // initialize the weight based on 1-ring around the vertex:
   void init();

   // sum of weights at the vertex
   double total_weight() const;

   // make the weights sum to 1:
   void normalize();

   static void   normalize(CBvert_list& verts);
   static void   smooth_weights(Patch* p, int n);
   static double get_smooth_weight(Bvert* v, Patch* p);
   static void   set_weight(Bvert* v, double w, Patch* p);
};

#endif // PATCH_BLEND_WEIGHT_IS_INCLUDED
