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
PatchBlendWeight::compute_all(CBMESHptr mesh, int num_passes)
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
PatchBlendWeight::clear(CBMESHptr mesh)
{
   // remove all weights stored on the mesh:
   if (mesh) {
      CBvert_list& verts = mesh->verts();
      for (Bvert_list::size_type i=0; i<verts.size(); i++)
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
   for (Bvert_list::size_type i=0; i<verts.size(); i++)
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
   double n = star.size();
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
   for (const auto & elem : _map)
      ret += elem.second;
   return ret;
}

void 
PatchBlendWeight::normalize(CBvert_list& verts)
{
   for (Bvert_list::size_type i=0; i<verts.size(); i++) {
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
      for (auto & elem : _map) {
         elem.second /= total;
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
   vector<double> new_weights;
   for (Bvert_list::size_type i=0; i<verts.size(); i++) {
      new_weights.push_back(get_smooth_weight(verts[i], p));
   }
   // now assign the smoothed weights:
   for (Bvert_list::size_type i=0; i<verts.size(); i++) {
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
   for (Bvert_list::size_type i=0; i<nbrs.size(); i++) {
      ret += get_weight(p, nbrs[i]);
   }
   return ret / nbrs.size();
}

void
PatchBlendWeight::set_weight(Bvert* v, double w, Patch* p)
{
   PatchBlendWeight* pbw = get_data(v);
   assert(pbw);
   pbw->set_weight(p, w);
}

// end of file patch_blend_weight.C
