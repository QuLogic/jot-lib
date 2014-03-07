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
#include <list>

static bool debug = Config::get_var_bool("DEBUG_SPS",false);

// see header file for explanation of parameters
void
generate_samples(
   BMESHptr mesh, 
   Bface_list& flist,   // output: faces that contain samples
   vector<Wvec>& blist,  // output: sample barycentric coordinates WRT faces
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
   vector<Wvec>& blist,  // output: sample barycentric coordinates WRT faces
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
         vector<Wvec>& blist,
         double& spacing
   ) 
{
   clock_t start, end;
   double duration; 
   flist.clear();
   blist.clear();

   OctreeNode* root = new OctreeNode(box.min(), box.max(), 1, nullptr);
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
   err_adv(debug, "no of points: %d", flist.size());

   return root;
}

inline
Wpt
center (Wpt_list& pts, list<int>& N)
{
   Wpt ret = Wpt(0);
   list<int>::iterator i;
   for (i = N.begin(); i != N.end(); ++i) {
      ret += pts[*i];
   }
   return ret / N.size();
}

class Priority{
 public:
  
   bool operator< (const Priority& other) const { return _priority < other._priority;}; 
   int _index;
   double _priority;
   int _version;
};

inline Wpt_list
get_pts(Bface_list& flist, vector<Wvec>& blist)
{
   assert(flist.size() == blist.size());
   Wpt_list pts;
   for (Bface_list::size_type i = 0; i < flist.size(); i++) {
      Wpt pt;
      flist[i]->bc2pos(blist[i], pt);
      pts.push_back(pt);
   }
   return pts;
}

void 
remove_nodes(Bface_list& flist, vector<Wvec>& blist, double min_dist, vector<OctreeNode*>& t)
{
   assert(flist.size() == blist.size());
   Wpt_list pts = get_pts(flist, blist);
   vector< list<int> > N;
   vector<bool> to_remove;

   N.resize(pts.size());
   to_remove.resize(pts.size(), false);

   for (Wpt_list::size_type i = 0; i < pts.size(); i++) {
      for (vector<OctreeNode*>::size_type j = 0; j < t[i]->neibors().size(); j++) {
         int index = t[i]->neibors()[j]->get_term_index();
         
         if (index < (int)pts.size()) {
            if (pts[i].dist(pts[index]) < min_dist) {
               N[i].push_back(index);
               N[index].push_back(i);
            }
         } else {
            //cerr << "Sps Warning, index > pts.num()" << endl;
         }
      }
   }

   priority_queue< Priority, vector<Priority> > queue;
   vector<int> versions;
   for (Wpt_list::size_type i = 0; i < pts.size(); i++) {
      if (!to_remove[i] && !N[i].empty()) {
         Priority p;
         p._priority = center(pts, N[i]).dist(pts[i]);
         p._index = i;
         p._version = 0;
         queue.push(p);
      }
      versions.push_back(0);
   }

   while (!queue.empty()) {
      Priority p = queue.top();
      queue.pop();
      int r = p._index;
      if (p._version == versions[r]) {
         to_remove[r] = true;
         list<int>::iterator i;
         for (i = N[r].begin(); i != N[r].end(); ++i) {
            int ind = *i;
            N[ind].remove(r);
            versions[ind]++;
            if (!N[ind].empty()) {
               Priority q;
               q._priority = center(pts, N[ind]).dist(pts[ind]);
               q._index = ind;
               q._version = versions[ind];
               queue.push(q);
            }
         }
      }
   }
   versions.clear();

   Bface_list ftemp(flist);
   vector<Wvec> btemp(blist);
   flist.clear();
   blist.clear();
   for (Bface_list::size_type i = 0; i < ftemp.size(); i++) {
      if (!to_remove[i]) {
         flist.push_back(ftemp[i]);
         blist.push_back(btemp[i]);
      }
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
pick (vector<QuadtreeNode*>& l)
{
   int ret = -1;
   double total_w = 0.0;
   vector<QuadtreeNode*>::size_type i;
   for (i = 0; i < l.size(); i++) {
      total_w += l[i]->get_weight();
   }
   double r = dorand() * total_w;
   
   total_w = 0.0;
   for (i = 0; i < l.size(); i++) {
      total_w += l[i]->get_weight();
      if (total_w >= r) {
         ret = i;
         break;
      }
   }
   return ret;
}

void
assign_weights(vector<QuadtreeNode*>& fs, double regularity, CWpt& pt)
{
   double weight, d;
   QuadtreeNode* leaf;
   vector<QuadtreeNode*>::size_type i;
   for (i = 0; i < fs.size(); i++) {
      weight = 0.0;
      vector<QuadtreeNode*>::iterator j;
      for (j = fs[i]->terms().begin(); j != fs[i]->terms().end(); ++j) {
         leaf = *j;
         d = pt.dist(leaf->centroid());
         leaf->set_weight(distr_func(regularity, d) * leaf->area());
         weight += leaf->get_weight();
      }
      fs[i]->set_weight(weight);
   }
}

void 
visit(OctreeNode* node,  
      double regularity, Bface_list& flist, vector<Wvec>& blist)
{
   if (node->get_leaf()) {
      if (node->get_disp()) {
         // subdivision
         vector<QuadtreeNode*> fs;
         Bface_list temp;
         for (Bface_list::size_type i = 0; i < node->intersects().size(); i++) {
            Bface* f = node->intersects()[i];
            temp.push_back(f);
            fs.push_back(new QuadtreeNode(f->v1()->loc(), f->v2()->loc(), f->v3()->loc()));
            fs.back()->build_quadtree(node, regularity);
            fs.back()->set_terms();
         }

         // assign weights
         assign_weights(fs, regularity, node->center());
         
         // pick a triangle
         int t = pick(fs);

         //set node face
         Bface_list ftemp;
         ftemp.push_back(temp[t]);
         node->set_face(ftemp);

         // pick a point
         int p = pick(fs[t]->terms());
         if (p != -1) {
            Wvec bc;
            temp[t]->project_barycentric(fs[t]->terms()[p]->urand_pick(), bc);
            blist.push_back(bc);
            flist.push_back(temp[t]);
            node->set_point(bc);
         }

         for (vector<QuadtreeNode*>::size_type i = 0; i < fs.size(); i++)
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
            _neibors.push_back(n);
         }
      }

      for (vector<OctreeNode*>::iterator i = _parent->neibors().begin(); i != _parent->neibors().end(); ++i) {
         n = *i;
         if (!n->get_leaf()) {
            for (int j = 0; j < 8; j++) {
               if (n->get_children()[j]->overlaps(test_box) &&
                   (!n->get_children()[j]->get_leaf() || 
                    n->get_children()[j]->get_disp())) {
                  //XXX - crashing here...
                  _neibors.push_back(n->get_children()[j]);
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
OctreeNode::set_terms(vector<OctreeNode*>& terms, int& count)
{
   if (_leaf) {
      if (_display) {
         terms.push_back(this);
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
QuadtreeNode::set_terms(vector<QuadtreeNode*>& terms)
{
   if (_leaf) {
      if (_in_cell) {
         terms.push_back(this);
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

   int i;
   Bface_list::size_type  j;
   Wpt_list pts;
   points(pts);

   for (i = 0; i < 8; i++) {
      _children[i] = new OctreeNode((pts[0]+pts[i])/2, (pts[i]+pts[7])/2, 
                                    _height+1, this);
   }

   for (j = 0; j < _intersects.size(); j++) {
      Bface* f = _intersects[j];

      for (i = 0; i < 8; i++) {
         OctreeNode* n = _children[i];
         if (n->contains(f->v1()->loc()) && n->contains(f->v2()->loc())
             && n->contains(f->v3()->loc())) {
            n->intersects().push_back(f);
            break;
         } else if (n->overlaps(bface_bbox(f))) {
            n->intersects().push_back(f);
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
