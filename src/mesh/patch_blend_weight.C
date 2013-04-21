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
#include "patch_blend_weight.H"

void 
PatchBlendWeight::compute_all(CBMESH* mesh, int num_passes) 
{
   assert(mesh && num_passes >= 0);
   clear(mesh);
   init(mesh->verts());
   CPatch_list& patches = mesh->patches();
   if (patches.num() < 2)
      return;
   for (int i=1; i<=num_passes; i++) {
      // compute smoothed weights for each patch in turn:
      for (int j=0; j<patches.num(); j++) {
         smooth_weights(patches[j], i);
      }
      // now normalize the weights so at each vertex,
      // the weights for all the patches sum to 1:
      normalize(mesh->verts());
   }
}

void 
PatchBlendWeight::clear(CBMESH* mesh) 
{
   // remove all weights stored on the mesh:
   if (mesh) {
      CBvert_list& verts = mesh->verts();
      for (int i=0; i<verts.num(); i++)
         delete lookup(verts[i]);
   }
}

double 
PatchBlendWeight::get_weight(Patch* p, CBvert* v) 
{
   // get the weight of the given patch at the given vertex:
   PatchBlendWeight* pbw = lookup(v);
   assert(pbw);  // make sure mesh has been initialized
   return pbw->get_weight(p);
}

void 
PatchBlendWeight::init(CBvert_list verts) 
{
   // initialize a set of vertices:
   for (int i=0; i<verts.num(); i++)
      init(verts[i]);
}

void 
PatchBlendWeight::init(Bvert* v) 
{
   // initialize a single vertex:

   // create (if needed) and initialize 
   // the PatchBlendWeight of the vertex
   assert(v);
   PatchBlendWeight* pbw = get_data(v);
   assert(pbw);
   pbw->init();
}

inline bool
is_equal(double a, double b)
{
   return fabs(a-b) < mlib::epsAbsMath();
}

void 
PatchBlendWeight::init() 
{
   // initialize the weight based on 1-ring around the vertex:

   // set up the map by recording a weight for each patch.
   // the weight is the number of faces owned by that patch
   // divided by the total number of faces in the 1-ring.
   _map.clear();
   assert(vert());
   Bface_list star = vert()->get_faces();
   Patch_list patches = Patch_list::get_unique_patches(star);
   double n = star.num();
   for (int i=0; i<patches.num(); i++) {
      _map[patches[i]] = star.num_satisfy(PatchFaceFilter(patches[i]))/n;
   }
   assert(is_equal(total_weight(), 1.0));
}

double 
PatchBlendWeight::total_weight() const 
{
   // sum of weights at the vertex
   double ret = 0;
   for (citer_t i=_map.begin(); i != _map.end(); ++i)
      ret += i->second;
   return ret;
}

void 
PatchBlendWeight::normalize(CBvert_list& verts)
{
   for (int i=0; i<verts.num(); i++) {
      PatchBlendWeight* pbw = get_data(verts[i]);
      assert(pbw);
      pbw->normalize();
   }
}

void 
PatchBlendWeight::normalize()
{
   // make the weights sum to 1:
   double total = total_weight();
   assert(total >= 0);
   if (total == 0) {
      cerr << "PatchBlendWeight::normalize: error: total weight is 0"
           << endl;
   } else {
      for (iter_t i=_map.begin(); i != _map.end(); ++i) {
         i->second /= total;
      }
      assert(is_equal(total_weight(), 1.0));
   }
}

void 
PatchBlendWeight::smooth_weights(Patch* p, int n)
{
   assert(p);
   assert(n >= 0);
   Bvert_list verts = p->faces().n_ring_faces(n).get_verts();
   if (verts.empty())
      return;

   // compute smoothed weights first...
   ARRAY<double> new_weights;
   for (int i=0; i<verts.num(); i++) {
      new_weights += get_smooth_weight(verts[i], p);
   }
   // now assign the smoothed weights:
   for (int i=0; i<verts.num(); i++) {
      set_weight(verts[i], new_weights[i], p);
   }
}

double
PatchBlendWeight::get_smooth_weight(Bvert* v, Patch* p)
{
   assert(v);
   Bvert_list nbrs;
   v->get_p_nbrs(nbrs);
   if (nbrs.empty()) {
      return get_weight(p, v);
   }
   double ret = 0;
   for (int i=0; i<nbrs.num(); i++) {
      ret += get_weight(p, nbrs[i]);
   }
   return ret / nbrs.num();
}

void
PatchBlendWeight::set_weight(Bvert* v, double w, Patch* p)
{
   PatchBlendWeight* pbw = get_data(v);
   assert(pbw);
   pbw->set_weight(p, w);
}

// end of file patch_blend_weight.C
