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
 * sps.C
 *****************************************************************/
// comments to be added
#include "sps.H"
#include <queue>

static bool debug = Config::get_var_bool("DEBUG_SPS",false);

// see header file for explanation of parameters
void
generate_samples(
   BMESHptr mesh, 
   Bface_list& flist,   // output: faces that contain samples
   ARRAY<Wvec>& blist,  // output: sample barycentric coordinates WRT faces
   double& spacing,
   int height,
   double min_dist,
   double regularity
   )
{
   OctreeNode* n = sps(mesh, height, regularity, min_dist, flist, blist, spacing);
   delete n;
}

void
generate_samples(
   Patch* p,
   Bface_list& flist,   // output: faces that contain samples
   ARRAY<Wvec>& blist,  // output: sample barycentric coordinates WRT faces
   double& spacing,
   int height,
   double min_dist,
   double regularity
   )
{
   assert(p);
   OctreeNode* n =
      sps(p->faces(), height, regularity, min_dist, flist, blist, spacing);
   delete n;
}

OctreeNode*
sps_real(CBface_list& input_faces,
         CBBOX& box,
         int height,
         double regularity,
         double min_dist,
         Bface_list& flist,
         ARRAY<Wvec>& blist,
         double& spacing
   ) 
{
   clock_t start, end;
   double duration; 
   flist.clear();
   blist.clear();

   OctreeNode* root = new OctreeNode(box.min(), box.max(), 1, NULL);
   if (height == 1) {
      root->set_leaf(true);
      root->set_disp(true);
   }
   root->intersects() = input_faces;

   start = clock();
   root->build_octree(height);
   end = clock();   
  
   duration = (double)(end - start) / CLOCKS_PER_SEC;
   err_adv(debug, "step 1 time: %f", duration); 
  
   start = clock();
   visit(root, regularity, flist, blist);
   end = clock();
   duration = (double)(end - start) / CLOCKS_PER_SEC;
   err_adv(debug, "step 2 time: %f", duration);

   // remove bad samples
   start = clock();
   double dist = min_dist * box.dim().length() / (1<<(height-1));
   spacing = dist;
   root->set_neibors();

   root->set_terms();

   remove_nodes(flist, blist, dist, root->terms());
   end = clock();
   duration = (double)(end - start) / CLOCKS_PER_SEC;
   err_adv(debug, "step 3 time: %f", duration);   
   err_adv(debug, "no of points: %d", flist.num());

   return root;
}

inline
Wpt
center (Wpt_list& pts, ARRAY<int>& N)
{
   Wpt ret = Wpt(0);
   for (int i = 0; i < N.num(); i++) {
      ret += pts[N[i]];
   }
   return ret / N.num();
}

class Priority{
 public:
  
   bool operator< (const Priority& other) const { return _priority < other._priority;}; 
   int _index;
   double _priority;
   int _version;
};

inline Wpt_list
get_pts(Bface_list& flist, ARRAY<Wvec>& blist)
{
   assert(flist.num() == blist.num());
   Wpt_list pts;
   for (int i = 0; i < flist.num(); i++) {
      Wpt pt;
      flist[i]->bc2pos(blist[i], pt);
      pts += pt;
   }
   return pts;
}

void 
remove_nodes(Bface_list& flist, ARRAY<Wvec>& blist, double min_dist, ARRAY<OctreeNode*>& t)
{
//    if (flist.num() != blist.num()) {
//       return;
//    }
   assert(flist.num() == blist.num());
   Wpt_list pts = get_pts(flist, blist);
   ARRAY< ARRAY<int> > N;
   ARRAY<bool> to_remove;
   for (int i = 0; i < pts.num(); i++) {
      N += ARRAY<int>();
      to_remove += false;
   }

   clock_t start;

   for (int i = 0; i < pts.num(); i++) {
      for (int j = 0; j < t[i]->neibors().num(); j++) {
         int index = t[i]->neibors()[j]->get_term_index();
         
         if (index < pts.num())
         {
            if (pts[i].dist(pts[index]) < min_dist) {
               N[i] += index;
               N[index] += i;
            }
         }
         else
         {
            //cerr << "Sps Warning, index > pts.num()" << endl;
         }

      }
   }

   start = clock();
   priority_queue< Priority, vector<Priority> > queue;
   ARRAY<int> versions;
   for (int i = 0; i < pts.num(); i++) {
      if (!to_remove[i] && !N[i].empty()) {
         Priority p;
         p._priority = center(pts, N[i]).dist(pts[i]);
         p._index = i;
         p._version = 0;
         queue.push(p);
      }
      versions += 0;
   }

   while (!queue.empty()) {
      Priority p = queue.top();
      queue.pop();
      int r = p._index;
      if (p._version == versions[r]) {
         to_remove[r] = true;
         for (int i = 0; i < N[r].num(); i++) {
            N[N[r][i]] -= r;
            versions[N[r][i]]++;
            if (!N[N[r][i]].empty()) {
               Priority q;
               q._priority = center(pts, N[N[r][i]]).dist(pts[N[r][i]]);
               q._index = N[r][i];
               q._version = versions[N[r][i]];
               queue.push(q);
            }
         }
      }
   }
   versions.clear();

   Bface_list ftemp(flist);
   ARRAY<Wvec> btemp(blist);
   flist.clear();
   blist.clear();
   for (int i = 0; i < ftemp.num(); i++)
      if (!to_remove[i]) {
         flist += ftemp[i];
         blist += btemp[i];
      }
}

inline
double
distr_func (double r, double d) 
{
   const float E = 2.71828183;
   return r * powf(E, -(float)r * (float)d);
}

/// An auxilliary function that produces a pseudo-random floating
/// point number between 0 and 1
inline
float dorand() {
   return double(rand()) / (RAND_MAX);
}

inline
int
pick (ARRAY<QuadtreeNode*>& l)
{
   int ret = -1;
   double total_w = 0.0;
   for (int i = 0; i < l.num(); i++) {
      total_w += l[i]->get_weight();
   }
   double r = dorand() * total_w;
   
   total_w = 0.0;
   for (int i = 0; i < l.num(); i++) {
      total_w += l[i]->get_weight();
      if (total_w >= r) {
         ret = i;
         break;
      }
   }
   return ret;
}

void
assign_weights(ARRAY<QuadtreeNode*>& fs, double regularity, CWpt& pt)
{
   double weight, d;
   QuadtreeNode* leaf;
   for (int i = 0; i < fs.num(); i++) {
      weight = 0.0;
      for (int j = 0; j < fs[i]->terms().num(); j++) {
         leaf = fs[i]->terms()[j];
         d = pt.dist(leaf->centroid());
         leaf->set_weight(distr_func(regularity, d) * leaf->area());
         weight += leaf->get_weight();
      }
      fs[i]->set_weight(weight);
   }
}

void 
visit(OctreeNode* node,  
      double regularity, Bface_list& flist, ARRAY<Wvec>& blist)
{
   if (node->get_leaf()) {
      if (node->get_disp()) {
         // subdivision
         ARRAY<QuadtreeNode*> fs;
         Bface_list temp;
         for (int i = 0; i < node->intersects().num(); i++) {
            Bface* f = node->intersects()[i];
            temp += f;
            fs += new QuadtreeNode(f->v1()->loc(), f->v2()->loc(), f->v3()->loc());
            fs.last()->build_quadtree(node, regularity);
            fs.last()->set_terms();
         }

         // assign weights
         assign_weights(fs, regularity, node->center());
         
         // pick a triangle
         int t = pick(fs);

         // moved below; want to ensure flist and blist stay in sync:
//         flist += temp[t];

         //set node face
         Bface_list ftemp;
         ftemp += temp[t];
         node->set_face(ftemp);

         // pick a point
         int p = pick(fs[t]->terms());
         if (p != -1) {
            Wvec bc;
            temp[t]->project_barycentric(fs[t]->terms()[p]->urand_pick(), bc);
            blist += bc;
            flist += temp[t]; // moved from above
            node->set_point(bc);
         }

         for (int i = 0; i < fs.num(); i++)
            delete fs[i];
         fs.clear();
      }
   } else {
      for (int i = 0; i < 8; i++)
         visit(node->get_children()[i], regularity, flist, blist);
   }
}

void
OctreeNode::set_neibors()
{
   OctreeNode* n;
   BBOX test_box = BBOX(min()-0.5*dim(), max()+0.5*dim());

   if ((!_leaf || _display) && _height != 1) {
      for (int i = 0; i < 8; i++) {
         n = _parent->get_children()[i];
         if (n != this && (!n->get_leaf() || n->get_disp())) {
            _neibors += n;
         }
      }

      _parent->neibors().num();

      for (int i = 0; i < _parent->neibors().num(); i++) {
         n = _parent->neibors()[i];
         if (!n->get_leaf()) {
            for (int j = 0; j < 8; j++) {
               if (n->get_children()[j]->overlaps(test_box) &&
                   (!n->get_children()[j]->get_leaf() || 
                    n->get_children()[j]->get_disp())) {
                  //XXX - crashing here...
                  _neibors += n->get_children()[j];
               }
            }
         }
      }
        
   }

   if (!_leaf) {
      for (int i = 0; i < 8; i++) {
         _children[i]->set_neibors();
      }
   }
}

void 
OctreeNode::set_terms(ARRAY<OctreeNode*>& terms, int& count)
{
   if (_leaf) {
      if (_display) {
         terms += this;
         _term_index = count;
         count++;
      }
   } else {
      for (int i = 0; i < 8; i++)
         _children[i]->set_terms(terms, count);
   }
}

void 
OctreeNode::set_terms()
{
   _terms.clear();
   int count = 0;
   set_terms(_terms, count);
}

void 
QuadtreeNode::set_terms(ARRAY<QuadtreeNode*>& terms)
{
   if (_leaf) {
      if (_in_cell) {
         terms += this;
      }
   } else {
      for (int i = 0; i < 4; i++)
         _children[i]->set_terms(terms);
   }
}

void 
QuadtreeNode::set_terms()
{
   _terms.clear();
   set_terms(_terms);
}

inline
BBOX
bface_bbox(QuadtreeNode* face) 
{
   BBOX ret = BBOX();
   ret.update(face->v1());
   ret.update(face->v2());
   ret.update(face->v3());
   return ret;
}

inline
BBOX
bface_bbox(Bface* face) 
{
   BBOX ret = BBOX();
   ret.update(face->v1()->loc());
   ret.update(face->v2()->loc());
   ret.update(face->v3()->loc());
   return ret;
}

void 
OctreeNode::build_octree(int height) 
{

   if (_leaf || _height == height) 
      return;

   int i, j;
   Wpt_list pts;
   points(pts);

   for (i = 0; i < 8; i++) {
      _children[i] = new OctreeNode((pts[0]+pts[i])/2, (pts[i]+pts[7])/2, 
                                    _height+1, this);
   }

   for (j = 0; j < _intersects.num(); j++) {
      Bface* f = _intersects[j];

      for (i = 0; i < 8; i++) {
         OctreeNode* n = _children[i];
         if (n->contains(f->v1()->loc()) && n->contains(f->v2()->loc())
             && n->contains(f->v3()->loc())) {
            n->intersects() += f;
            break;
         } else if (n->overlaps(bface_bbox(f))) {
            n->intersects() += f;
         }
      }
   }

   for (i = 0; i < 8; i++) {
      if (_height+1 == height) {
         _children[i]->set_leaf(true);
         _children[i]->set_disp(true);
      }
      if (_children[i]->intersects().empty()) {
         _children[i]->set_leaf(true);
         _children[i]->set_disp(false);
      }
      _children[i]->build_octree(height);
   }

}

void
QuadtreeNode::subdivide(OctreeNode* o, double regularity)
{
   // subdivide 
   Wpt v4 = (_v1+_v2)/2;
   Wpt v5 = (_v2+_v3)/2;
   Wpt v6 = (_v3+_v1)/2;
   _children[0] = new QuadtreeNode(_v1, v4, v6);
   _children[0]->build_quadtree(o, regularity);
   _children[1] = new QuadtreeNode(v4, _v2, v5);
   _children[1]->build_quadtree(o, regularity);
   _children[2] = new QuadtreeNode(v6, v5, _v3);
   _children[2]->build_quadtree(o, regularity);
   _children[3] = new QuadtreeNode(v4, v5, v6);
   _children[3]->build_quadtree(o, regularity);
}

void 
QuadtreeNode::build_quadtree(OctreeNode* o, double regularity)
{
   const double THRED_RATIO = 0.3;
   const double THRED_EDGE = THRED_RATIO * o->dim().length();
   /*
   if (!(o->contains(_v1) && o->contains(_v2))
       && o->contains(_v3)) {
       */

   //I've replaced that conditional with this one:
   // -Karol

 if (!(o->contains(_v1) && o->contains(_v2)
       && o->contains(_v3))) {

      if (max(max(_v1.dist(_v2), _v2.dist(_v3)), _v3.dist(_v1))
          < THRED_EDGE || !o->overlaps(bface_bbox(this))) {
         _leaf = true;
         _in_cell = false;
         return;
      }
   }

   if (area() < o->get_area()/20) {
      _leaf = true;
      return;
   }

   //Wpt nearest = nearest_pt(o->center());
   //Wpt farthest = farthest_pt(o->center());
   //double smallest = distr_func(regularity, farthest.dist(o->center()));
   //double largest = distr_func(regularity, nearest.dist(o->center()));
   //double ratio = (largest - smallest) / smallest;
   //if (ratio < THRED_RATIO) {
   //   _leaf = true;
   //   return;
   //}
   subdivide(o, regularity);
}

Wpt 
QuadtreeNode::nearest_pt(Wpt& p)
{
   Wvec bc; 
   bool on;
   Bvert* v1 = new Bvert(_v1);
   Bvert* v2 = new Bvert(_v2);
   Bvert* v3 = new Bvert(_v3);
   Bface f(v1, v2, v3, new Bedge(v1, v2), new Bedge(v2, v3), new Bedge(v3, v1));
   return f.nearest_pt(p, bc, on);
}

Wpt 
QuadtreeNode::farthest_pt(Wpt& p)
{
   Wpt ret = _v1;
   double max_dist = ret.dist(p);
   if (_v2.dist(p) > max_dist) {
      ret = _v2;
      max_dist = ret.dist(p);
   }
   if (_v3.dist(p) > max_dist) {
      ret = _v3;
   }
   return ret;
}

Wpt 
QuadtreeNode::urand_pick()
{
   Wvec vec1 = _v2 - _v1;
   Wvec vec2 = _v3 - _v1;
   double d1 = dorand(), d2 = dorand();
   Wpt pt = _v1 + d1 * vec1 + d2 * vec2;
   if (!contains(pt, 0.1))
      pt = (_v2 + _v3) + (-pt);
   return pt;
}

// end of file sps.C
