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
 * sps.H
 *****************************************************************/
// comments to be added
#ifndef SPS_H_IS_INCLUDED
#define SPS_H_IS_INCLUDED
#include "mesh/bmesh.H"


class OctreeNode : public BBOX {
 public:

   //******** MANAGERS *******/
   OctreeNode(Wpt min, Wpt max, int height, OctreeNode* p) :
      _height(height), _parent(p) {
      _max = max;
      _min = min;
      double a = max[0]-min[0], b = max[1]-min[1],
         c = max[2]-min[2];
      _area = 2 * (a * b + b * c + c * a);
      _valid = true;
      _leaf = false;
      _display = false;
   }

   ~OctreeNode() {
      _terms.clear();
      _neibors.clear();
      _intersects.clear();
      if (!_leaf) {
         for (int i = 0; i < 8; i++) {
            delete _children[i];
         }
      }
   }

   //******** OctreeNode METHODS ********
   void build_octree(int height);
   void set_leaf(bool leaf) {_leaf = leaf; }
   bool get_leaf() { return _leaf; }
   void set_disp(bool disp) { _display = disp; }
   bool get_disp() { return _display; }
   int  get_height() { return _height; }
   double get_area() { return _area; }
   void set_face(Bface_list& f) { _face = f;}
   void set_point(Wvec& p) { _pt = p;}
   Bface_list& get_face()       { return _face; }
   Wvec get_point()       { return _pt; }
   OctreeNode** get_children() { return _children; }
   OctreeNode* parent() { return _parent; }
   ARRAY<OctreeNode*>& neibors() { return _neibors; }
   ARRAY<OctreeNode*>& terms() { return _terms; };
   void set_neibors();
   void set_terms();
   int get_term_index() { return _term_index; }
   Bface_list& intersects() { return _intersects; }

 protected:
   void set_terms(ARRAY<OctreeNode*>& terms, int& count);

   //******** MEMBER DATA ********

   int _height, _term_index;
   double _area;
   bool _leaf;
   bool _display;
   Bface_list _face;
   Wvec		  _pt;
   OctreeNode* _parent;
   ARRAY<OctreeNode*> _neibors;
   ARRAY<OctreeNode*> _terms;
   OctreeNode* _children[8];
   Bface_list _intersects;
};

class QuadtreeNode {
 public:

   //******** MANAGERS *******/
   QuadtreeNode(Wpt p1, Wpt p2, Wpt p3)
   {
      _v1 = p1;
      _v2 = p2;
      _v3 = p3;
      _leaf = false;
      _in_cell = true;
      _weight = 0;
   }

   ~QuadtreeNode() { 
      _terms.clear();
      if (!_leaf) {
         for (int i = 0; i < 4; i++) {
            delete _children[i];
         }
      }
   }

   Wpt centroid()
   {
      return (_v1 + _v2 + _v3) / 3;
   }

   Wvec normal()
   {
      return cross(_v1-_v2,_v3-_v2).normalized();
   }

   double area()
   {
      double a = _v1.dist(_v2);
      double b = _v1.dist(_v3);
      double c = _v2.dist(_v3);
      double cosv = (a * a + b * b - c * c) / (2 * a * b);
      double sinv = sqrt(1-cosv);
      return a * b * sinv / 2;
   }

   Wpt v1() { return _v1; }
   Wpt v2() { return _v2; }
   Wpt v3() { return _v3; }

   bool
   contains(CWpt& pt,double threshold)  
   {
      // NOTE: Normalize behave in the way that if v is zero, it
      // normalize to zero vector
      const double dist_threshold=0.00001;
      if (Wplane(_v1, _v2, _v3).dist(pt) > dist_threshold)
         return false;
    
      Wvec vec1 = (pt-v1()).normalized();
      Wvec vec2 = (pt-v2()).normalized();
      Wvec vec3 = (pt-v3()).normalized();
      Wvec ab = (v2()-v1()).normalized();
      Wvec bc = (v3()-v2()).normalized();
      Wvec ca = (v1()-v3()).normalized();
    
      Wvec c1 = cross(ab,vec1).normalized();
      Wvec c2 = cross(bc,vec2).normalized();
      Wvec c3 = cross(ca,vec3).normalized();
    
      Wvec n = cross(ab,bc).normalized();
    
      if(c1*n<-threshold) return false;
      if(c2*n<-threshold) return false;
      if(c3*n<-threshold) return false;
    
      return true;
   }

   //******** QuadtreeNode METHODS ********
   void build_quadtree(OctreeNode* o, double regularity);
   void set_leaf(bool leaf) {_leaf = leaf; }
   bool get_leaf() { return _leaf; }
   bool is_in_cell() { return _in_cell; }
   QuadtreeNode** get_children() { return _children; }
   Wpt farthest_pt(Wpt& p);
   Wpt nearest_pt(Wpt& p);
   void set_terms(); // find all leaves in cell for root node
   ARRAY<QuadtreeNode*>& terms() { return _terms; } // get leaves in cell
   double get_weight() { return _weight; };
   void set_weight(double weight) { _weight = weight; }
   Wpt urand_pick();

 protected:
   void subdivide(OctreeNode* o, double regularity);
   void set_terms(ARRAY<QuadtreeNode*>& terms);

   //******** MEMBER DATA ********

   bool _leaf;
   bool _in_cell;
   double _weight;
   Wpt _v1, _v2, _v3;
   QuadtreeNode* _children[4];
   ARRAY<QuadtreeNode*> _terms;
};

// Recommended method to call for sampling points on a surface
//
// Input: mesh is the surface to be sampled
//
//        height is the height of the octree you want to build (which
//        means the resolution for sampling can only be controlled
//        exponentially). Default to be 6.
//
//        min_dist is a double between 0 and 1, proportional to the
//        diagonal length of the smallest cells in the octree. (You
//        probably want min_dist to be less than 0.5). Default is
//        0.35.
//
//        regularity is a density parameter discussed in the paper:
//        Stratified Point Sampling. The larger this parameter is, the
//        more the samples will be squeezed toward the center of the
//        octree node.  Default is 20. Changing this would not effect
//        the result much.
//
//        spacing: approximate distance between generated samples
//
//  Output: a Bface_list and a list of barycentric coordinates
void
generate_samples(
   BMESHptr mesh, 
   Bface_list& flist, // faces
   ARRAY<Wvec>& blist, // barycentric coordinates
   double& spacing,
   int height = 6,
   double min_dist = 0.35,
   double regularity = 20
   );

// same as above, but operates on a patch instead of a whole mesh:
void
generate_samples(
   Patch* p,
   Bface_list& flist, // faces
   ARRAY<Wvec>& blist, // barycentric coordinates
   double& spacing,
   int height = 6,
   double min_dist = 0.35,
   double regularity = 20
   );

// build samples over the given input faces,
// which should be contained in the given bounding box:
// ("real" refers to the "real version" that does all the work)
OctreeNode*
sps_real(CBface_list& input_faces,
         CBBOX& box,
         int height,
         double regularity,
         double min_dist,
         Bface_list& flist,
         ARRAY<Wvec>& blist,
         double& spacing
   );

// build samples for a set of faces:
inline OctreeNode*
sps(CBface_list& input_faces,
    int height,
    double regularity,
    double min_dist,
    Bface_list& flist, // faces
    ARRAY<Wvec>& blist, // barycentric coordinates
    double& spacing
   )
{
   return sps_real(
      input_faces,
      input_faces.get_verts().get_bbox_obj(),
      height,
      regularity,
      min_dist,
      flist,
      blist,
      spacing);
}

// build samples for a whole mesh:
// (it gets the bounding box more efficiently than the above method):
inline OctreeNode*
sps(BMESHptr mesh,
    int height,
    double regularity,
    double min_dist,
    Bface_list& flist, // faces
    ARRAY<Wvec>& blist, // barycentric coordinates
    double& spacing
   )
{
   assert(mesh);
   return sps_real(
      mesh->faces(),
      mesh->get_bb(),
      height,
      regularity,
      min_dist,
      flist,
      blist,
      spacing);
}

void 
visit(OctreeNode* node,  
   double regularity, Bface_list& flist, ARRAY<Wvec>& blist);

void 
remove_nodes(Bface_list& flist, ARRAY<Wvec>& blist, double min_dist, ARRAY<OctreeNode*>& t);

void
assign_weights(ARRAY<QuadtreeNode*>& fs, double regularity, Wpt& pt);

int
pick_triangle(ARRAY<QuadtreeNode*>& fs);

#endif // SPS_H_IS_INCLUDED
// end of file sps.H
