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
/*!
 *  \file bmesh_curvature.C
 *  \brief Contains the implementation of the curvature computation classes
 *  for the BMESH class.
 *
 *  \note This code is taken almost verbatim from the TriMesh2 library written by
 *  Szymon Rusinkiewicz of Princeton University
 *  (http://www.cs.princeton.edu/gfx/proj/trimesh2/).
 *
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

#include "mlib/points.H"
#include "mlib/linear_sys.H"

using namespace mlib;

#include "mesh/bvert.H"
#include "mesh/bface.H"
#include "mesh/bmesh.H"

#include "mesh/bmesh_curvature.H"

/* Helper Functions for Computing Curvature */

void rot_coord_sys(const Wvec &old_u, const Wvec &old_v,
                   const Wvec &new_norm, Wvec &new_u, Wvec &new_v);
void proj_curv(const Wvec &old_u, const Wvec &old_v,
               double old_ku, double old_kuv, double old_kv,
               const Wvec &new_u, const Wvec &new_v,
               double &new_ku, double &new_kuv, double &new_kv);
void proj_dcurv(const Wvec &old_u, const Wvec &old_v,
                const BMESHcurvature_data::dcurv_tensor_t &old_dcurv,
                const Wvec &new_u, const Wvec &new_v,
                BMESHcurvature_data::dcurv_tensor_t &new_dcurv);
void diagonalize_curv(const Wvec &old_u, const Wvec &old_v,
                      double ku, double kuv, double kv,
                      const Wvec &new_norm,
                      Wvec &pdir1, Wvec &pdir2, double &k1, double &k2);

inline void compute_edge_vectors(const BMESH *mesh, int face_idx, Wvec edges[3]);
inline void compute_ntb_coord_sys(const Wvec edges[3], Wvec &n, Wvec &t, Wvec &b);
inline void compute_initial_vertex_coord_sys(const BMESH *mesh,
                                             vector<Wvec> &pdir1, vector<Wvec> &pdir2);

/* BMESHcurvature_data Constructors */

/*!
 *  Computes curvature and curvature derivatives for the given BMESH.
 *
 */
BMESHcurvature_data::BMESHcurvature_data(const BMESH *mesh_in)
   : mesh(mesh_in), mesh_feature_size(0.0)
{
   
   compute_corner_areas();
   compute_vertex_areas();
   compute_face_curvatures();
   compute_vertex_curvatures();
   diagonalize_vertex_curvatures();
   compute_face_dcurv();
   compute_vertex_dcurv();
   compute_feature_size();
   
}

/* BMESHcurvature_data Curvature Data Queries */

BMESHcurvature_data::curv_tensor_t
BMESHcurvature_data::curv_tensor(const Bvert *v)
{
   
   return vertex_curv[v->index()];
   
}
      
BMESHcurvature_data::diag_curv_t
BMESHcurvature_data::diag_curv(const Bvert *v)
{
   
   return diag_vertex_curv[v->index()];
   
}

double
BMESHcurvature_data::k1(const Bvert *v)
{
   
   return diag_vertex_curv[v->index()].k1();
   
}

double
BMESHcurvature_data::k2(const Bvert *v)
{
   
   return diag_vertex_curv[v->index()].k2();
   
}

Wvec
BMESHcurvature_data::pdir1(const Bvert *v)
{
   
   return diag_vertex_curv[v->index()].pdir1();
   
}

Wvec
BMESHcurvature_data::pdir2(const Bvert *v)
{
   
   return diag_vertex_curv[v->index()].pdir2();
   
}

BMESHcurvature_data::dcurv_tensor_t
BMESHcurvature_data::dcurv_tensor(const Bvert *v)
{
   
   return vertex_dcurv[v->index()];
   
}

/* BMESHcurvature_data Curvature Computation Functions */

/*!
 *  Computes the corner areas for all faces in the given BMESH.  These are used
 *  for weighting purposes when computing the curvatures for the BMESH's vertices.
 *
 */
void
BMESHcurvature_data::compute_corner_areas()
{

   int nf = mesh->nfaces();
   
   corner_areas.clear();
   corner_areas.resize(nf);

   for (int i = 0; i < nf; i++) {
      
      Wvec e[3];
      compute_edge_vectors(mesh, i, e);

      // Compute corner weights
      double area = mesh->bf(i)->area();// 0.5 * cross(e[0],e[1]).length();
      double l2[3] = { e[0].length_sqrd(), e[1].length_sqrd(), e[2].length_sqrd() };
      double ew[3] = { l2[0] * (l2[1] + l2[2] - l2[0]),
                       l2[1] * (l2[2] + l2[0] - l2[1]),
                       l2[2] * (l2[0] + l2[1] - l2[2]) };
                       
      if (ew[0] <= 0.0f) {
         
         corner_areas[i][1] = -0.25f * l2[2] * area / (e[0] * e[2]);
         corner_areas[i][2] = -0.25f * l2[1] * area / (e[0] * e[1]);
         corner_areas[i][0] = area - corner_areas[i][1] - corner_areas[i][2];
         
      } else if (ew[1] <= 0.0f) {
         
         corner_areas[i][2] = -0.25f * l2[0] * area / (e[1] * e[0]);
         corner_areas[i][0] = -0.25f * l2[2] * area / (e[1] * e[2]);
         corner_areas[i][1] = area - corner_areas[i][2] - corner_areas[i][0];
         
      } else if (ew[2] <= 0.0f) {
         
         corner_areas[i][0] = -0.25f * l2[1] * area / (e[2] * e[1]);
         corner_areas[i][1] = -0.25f * l2[0] * area / (e[2] * e[0]);
         corner_areas[i][2] = area - corner_areas[i][0] - corner_areas[i][1];
         
      } else {
         
         double ewscale = 0.5f * area / (ew[0] + ew[1] + ew[2]);
         for (int j = 0; j < 3; j++)
            corner_areas[i][j] = ewscale * (ew[(j+1)%3] + ew[(j+2)%3]);
            
      }
      
   }
   
}

/*!
 *  Computes vertex areas for all vertices in the given BMESH.  These are used for
 *  weighting purposes when computing the curvatures for the BMESH's vertices.
 *
 */
void
BMESHcurvature_data::compute_vertex_areas()
{
   
   int nf = mesh->nfaces();
   
   vertex_areas.clear();
   vertex_areas.resize(mesh->nverts());

   for (int i = 0; i < nf; i++) {
      
      vertex_areas[mesh->bf(i)->v1()->index()] += corner_areas[i][0];
      vertex_areas[mesh->bf(i)->v2()->index()] += corner_areas[i][1];
      vertex_areas[mesh->bf(i)->v3()->index()] += corner_areas[i][2];
      
   }
   
}

/*!
 *  Computes the curvatures per face for all faces in the given BMESH.  Each
 *  face's curvature is estimated based on the variation of normals along its
 *  edges.
 *
 */
void
BMESHcurvature_data::compute_face_curvatures()
{
   
   int nf = mesh->nfaces();
   
   face_curv.clear();
   face_curv.resize(nf);
   
   // Compute curvature per-face
   for (int i = 0; i < nf; ++i) {
      
      Wvec e[3];
      compute_edge_vectors(mesh, i, e);

      Wvec n, t, b;
      compute_ntb_coord_sys(e, n, t, b);

      // Estimate curvature based on variation of normals
      // along edges
      double m[3] = { 0, 0, 0 };
      double w[3][3] = { {0,0,0}, {0,0,0}, {0,0,0} };
      for (int j = 0; j < 3; ++j) {
         
         double u = e[j] * t;
         double v = e[j] * b;
         w[0][0] += u*u;
         w[0][1] += u*v;
         //w[1][1] += v*v + u*u; 
         //w[1][2] += u*v; 
         w[2][2] += v*v;
         Wvec dn = mesh->bf(i)->v(((j+2)%3)+1)->norm()
                 - mesh->bf(i)->v(((j+1)%3)+1)->norm();
         double dnu = dn * t;
         double dnv = dn * b;
         m[0] += dnu*u;
         m[1] += dnu*v + dnv*u;
         m[2] += dnv*v;
         
      }
      
      w[1][1] = w[0][0] + w[2][2];
      w[1][2] = w[0][1];

      // Least squares solution
      double diag[3];
      if (!ldltdc<double,3>(w, diag)) {
         //fprintf(stderr, "ldltdc failed!\n");
         continue;
      }
      ldltsl<double,3>(w, diag, m, m);
      
      face_curv[i][0] = m[0];
      face_curv[i][1] = m[1];
      face_curv[i][2] = m[2];

   }
   
}

/*!
 *  Computes the curvatures per vertex for all vertices in the given BMESH.  The
 *  curvature of each vertex is estimated based on the weighted average of
 *  curvatures for each face surrounding that vertex.
 *
 */
void
BMESHcurvature_data::compute_vertex_curvatures()
{
   
   int nv = mesh->nverts();
   
   vertex_curv.clear();
   vertex_curv.resize(nv);
   
   // Set up an initial coordinate system per vertex
   vector<Wvec> pdir1(nv), pdir2(nv);
   
   compute_initial_vertex_coord_sys(mesh, pdir1, pdir2);
   
   for(int i = 0; i < mesh->nfaces(); ++i) {
      
      Wvec e[3];
      compute_edge_vectors(mesh, i, e);

      Wvec n, t, b;
      compute_ntb_coord_sys(e, n, t, b);

      // Compute curvature per vertex:
      for(int j = 0; j < 3; ++j){
         
         int vj = mesh->bf(i)->v(j+1)->index();
         double c1, c12, c2;
         proj_curv(t, b, face_curv[i][0], face_curv[i][1], face_curv[i][2],
                   pdir1[vj], pdir2[vj], c1, c12, c2);
         double wt = corner_areas[i][j] / vertex_areas[vj];
         vertex_curv[vj][0] += wt * c1;
         vertex_curv[vj][1] += wt * c12;
         vertex_curv[vj][2] += wt * c2;
         
      }
      
   }
   
}

/*!
 *  Computes the principal directions and curvatures per vertex for the given
 *  BMESH.  These values are computed based on precomputed per vertex curvature
 *  tensors from the compute_vertex_curvature() function.
 *
 */
void
BMESHcurvature_data::diagonalize_vertex_curvatures()
{
   
   int nv = mesh->nverts();
   
   diag_vertex_curv.clear();
   diag_vertex_curv.resize(nv);
   
   vector<Wvec> pdir1(nv), pdir2(nv);
   compute_initial_vertex_coord_sys(mesh, pdir1, pdir2);
   
   // Compute principal directions and curvatures at each vertex
   vector<double> k1(nv), k2(nv);
   
   for (int i = 0; i < nv; i++){
      
      diagonalize_curv(pdir1[i], pdir2[i],
                       vertex_curv[i][0], vertex_curv[i][1], vertex_curv[i][2],
                       mesh->bv(i)->norm(), pdir1[i], pdir2[i],
                       k1[i], k2[i]);
      
      diag_vertex_curv[i]._k1 = k1[i];
      diag_vertex_curv[i]._k2 = k2[i];
      diag_vertex_curv[i]._pdir1 = pdir1[i];
      diag_vertex_curv[i]._pdir2 = pdir2[i];
                       
//       diag_vertex_curv[i].curv1 = pdir1[i]*k1[i];
//       diag_vertex_curv[i].curv2 = pdir2[i]*k2[i];
                       
   }
   
}

/*!
 *  Computes the derivative of curvature per face for all faces in the given
 *  BMESH.  Each face's curvature derivative is estimated based on the variation
 *  of the per vertex curvatures along its edges.
 *
 */
void
BMESHcurvature_data::compute_face_dcurv()
{
   
   int nf = mesh->nfaces();
   
   face_dcurv.clear();
   face_dcurv.resize(nf);
   
   // Compute dcurv per-face
   for(int i = 0; i < nf; ++i){
      
      Wvec e[3];
      compute_edge_vectors(mesh, i, e);

      Wvec n, t, b;
      compute_ntb_coord_sys(e, n, t, b);

      // Project curvature tensor from each vertex into this
      // face's coordinate system
      curv_tensor_t fcurv[3];
      for(int j = 0; j < 3; ++j){
         int vj = mesh->bf(i)->v(j+1)->index();
         proj_curv(diag_vertex_curv[vj].pdir1(), diag_vertex_curv[vj].pdir2(),
                   diag_vertex_curv[vj].k1(), 0, diag_vertex_curv[vj].k2(),
                   t, b, fcurv[j][0], fcurv[j][1], fcurv[j][2]);

      }

      // Estimate dcurv based on variation of curvature along edges
      double m[4] = { 0, 0, 0, 0 };
      double w[4][4] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
      for(int j = 0; j < 3; ++j){
         // Variation of curvature along each edge
         curv_tensor_t dfcurv = fcurv[(j+2)%3] - fcurv[(j+1)%3];
         double u = e[j] * t;
         double v = e[j] * b;
         double u2 = u*u, v2 = v*v, uv = u*v;
         w[0][0] += u2;
         w[0][1] += uv;
         //w[1][1] += 2.0f*u2 + v2;
         //w[1][2] += 2.0f*uv;
         //w[2][2] += u2 + 2.0f*v2;
         //w[2][3] += uv;
         w[3][3] += v2;
         m[0] += u*dfcurv[0];
         m[1] += v*dfcurv[0] + 2.0f*u*dfcurv[1];
         m[2] += 2.0f*v*dfcurv[1] + u*dfcurv[2];
         m[3] += v*dfcurv[2];
      }
      w[1][1] = 2.0f * w[0][0] + w[3][3];
      w[1][2] = 2.0f * w[0][1];
      w[2][2] = w[0][0] + 2.0f * w[3][3];
      w[2][3] = w[0][1];

      // Least squares solution
      double d[4];
      if(!ldltdc<double,4>(w, d)){
         //fprintf(stderr, "ldltdc failed!\n");
         continue;
      }
      ldltsl<double,4>(w, d, m, m);
      
      face_dcurv[i][0] = m[0];
      face_dcurv[i][1] = m[1];
      face_dcurv[i][2] = m[2];
      face_dcurv[i][3] = m[3];

   }
   
}

/*!
 *  Computes the derivative of curvature per vertex for all vertices in the given
 *  BMESH.  The curvature derivative of each vertex is estimated based on the
 *  weighted average of curvature derivatives for each face surrounding that vertex.
 *
 */
void
BMESHcurvature_data::compute_vertex_dcurv()
{
   
   int nv = mesh->nverts();
   int nf = mesh->nfaces();
   
   vertex_dcurv.clear();
   vertex_dcurv.resize(nv);
   
   for(int i = 0; i < nf; ++i){
      
      Wvec e[3];
      compute_edge_vectors(mesh, i, e);

      Wvec n, t, b;
      compute_ntb_coord_sys(e, n, t, b);

      // Compute dcurv per vertex:
      for(int j = 0; j < 3; ++j){
         
         int vj = mesh->bf(i)->v(j+1)->index();
         dcurv_tensor_t this_vert_dcurv;
         proj_dcurv(t, b, face_dcurv[i],
                    diag_vertex_curv[vj].pdir1(), diag_vertex_curv[vj].pdir2(),
                    this_vert_dcurv);
         double wt = corner_areas[i][j] / vertex_areas[vj];
         vertex_dcurv[vj] += wt * this_vert_dcurv;
         
      }
      
   }
   
}

/*!
 *  \brief Compute a "feature size" for the mesh: computed as 1% of
 *  the reciprocal of the 10-th percentile curvature.
 *
 */
void
BMESHcurvature_data::compute_feature_size()
{
   
   int nv = mesh->nverts();
   int nsamp = min(nv, 500);

   vector<double> samples;
   samples.reserve(nsamp * 2);

   for(int i = 0; i < nsamp; ++i){
      
      // Quick 'n dirty portable random number generator 
      static unsigned randq = 0;
      randq = unsigned(1664525) * randq + unsigned(1013904223);

      int ind = randq % nv;
      samples.push_back(fabs(diag_vertex_curv[ind].k1()));
      samples.push_back(fabs(diag_vertex_curv[ind].k2()));
      
   }
   
   const double frac = 0.1f;
   const double mult = 0.01f;
   int which = int(frac * samples.size());
   nth_element(samples.begin(), samples.begin() + which, samples.end());
   
   mesh_feature_size = mult / samples[which];
   
}

/* Helper Functions for Computing Curvature */

/*!
 *  \brief  Rotate a coordinate system to be perpendicular to the given normal.
 *
 */
void
rot_coord_sys(const Wvec &old_u, const Wvec &old_v,
              const Wvec &new_norm, Wvec &new_u, Wvec &new_v)
{
   new_u = old_u;
   new_v = old_v;
   Wvec old_norm = cross(old_u, old_v);
   double ndot = old_norm * new_norm;
   if (ndot <= -1.0f) {
      new_u = -new_u;
      new_v = -new_v;
      return;
   }
   Wvec perp_old = new_norm - ndot * old_norm;
   Wvec dperp = 1.0f / (1 + ndot) * (old_norm + new_norm);
   new_u -= dperp * (new_u * perp_old);
   new_v -= dperp * (new_v * perp_old);
}


/*!
 *  \brief Reproject a curvature tensor from the basis spanned by old_u and
 *  old_v (which are assumed to be unit-length and perpendicular) to the new_u,
 *  new_v basis.
 *
 */
void
proj_curv(const Wvec &old_u, const Wvec &old_v,
          double old_ku, double old_kuv, double old_kv,
          const Wvec &new_u, const Wvec &new_v,
          double &new_ku, double &new_kuv, double &new_kv)
{
   Wvec r_new_u, r_new_v;
   rot_coord_sys(new_u, new_v, cross(old_u, old_v), r_new_u, r_new_v);

   double u1 = r_new_u * old_u;
   double v1 = r_new_u * old_v;
   double u2 = r_new_v * old_u;
   double v2 = r_new_v * old_v;
   new_ku  = old_ku * u1*u1 + old_kuv * (2.0f  * u1*v1) + old_kv * v1*v1;
   new_kuv = old_ku * u1*u2 + old_kuv * (u1*v2 + u2*v1) + old_kv * v1*v2;
   new_kv  = old_ku * u2*u2 + old_kuv * (2.0f  * u2*v2) + old_kv * v2*v2;
}


/*!
 * \brief Like proj_curv(), but for the derivative of curvature.
 *
 */
void
proj_dcurv(const Wvec &old_u, const Wvec &old_v,
           const BMESHcurvature_data::dcurv_tensor_t &old_dcurv,
           const Wvec &new_u, const Wvec &new_v,
           BMESHcurvature_data::dcurv_tensor_t &new_dcurv)
{
   Wvec r_new_u, r_new_v;
   rot_coord_sys(new_u, new_v, cross(old_u, old_v), r_new_u, r_new_v);

   double u1 = r_new_u * old_u;
   double v1 = r_new_u * old_v;
   double u2 = r_new_v * old_u;
   double v2 = r_new_v * old_v;

   new_dcurv[0] = old_dcurv[0]*u1*u1*u1 +
             old_dcurv[1]*3.0f*u1*u1*v1 +
             old_dcurv[2]*3.0f*u1*v1*v1 +
             old_dcurv[3]*v1*v1*v1;
   new_dcurv[1] = old_dcurv[0]*u1*u1*u2 +
             old_dcurv[1]*(u1*u1*v2 + 2.0f*u2*u1*v1) +
             old_dcurv[2]*(u2*v1*v1 + 2.0f*u1*v1*v2) +
             old_dcurv[3]*v1*v1*v2;
   new_dcurv[2] = old_dcurv[0]*u1*u2*u2 +
             old_dcurv[1]*(u2*u2*v1 + 2.0f*u1*u2*v2) +
             old_dcurv[2]*(u1*v2*v2 + 2.0f*u2*v2*v1) +
             old_dcurv[3]*v1*v2*v2;
   new_dcurv[3] = old_dcurv[0]*u2*u2*u2 +
             old_dcurv[1]*3.0f*u2*u2*v2 +
             old_dcurv[2]*3.0f*u2*v2*v2 +
             old_dcurv[3]*v2*v2*v2;
}


/*!
 *  \brief Given a curvature tensor, find principal directions and curvatures.
 *
 *  Makes sure that pdir1 and pdir2 are perpendicular to normal.
 *
 */
void
diagonalize_curv(const Wvec &old_u, const Wvec &old_v,
                 double ku, double kuv, double kv,
                 const Wvec &new_norm,
                 Wvec &pdir1, Wvec &pdir2, double &k1, double &k2)
{
   Wvec r_old_u, r_old_v;
   rot_coord_sys(old_u, old_v, new_norm, r_old_u, r_old_v);

   double c = 1, s = 0, tt = 0;
   if (kuv != 0.0f) {
      // Jacobi rotation to diagonalize
      double h = 0.5f * (kv - ku) / kuv;
      tt = (h < 0.0f) ?
         1.0f / (h - sqrt(1.0f + h*h)) :
         1.0f / (h + sqrt(1.0f + h*h));
      c = 1.0f / sqrt(1.0f + tt*tt);
      s = tt * c;
   }

   k1 = ku - tt * kuv;
   k2 = kv + tt * kuv;

   if (fabs(k1) >= fabs(k2)) {
      pdir1 = c*r_old_u - s*r_old_v;
   } else {
      swap(k1, k2);
      pdir1 = s*r_old_u + c*r_old_v;
   }
   pdir2 = cross(new_norm, pdir1);
}

inline
void
compute_edge_vectors(const BMESH *mesh, int face_idx, Wvec edges[3])
{
   
   // Edges
   edges[0] = mesh->bf(face_idx)->v3()->loc() - mesh->bf(face_idx)->v2()->loc();
   edges[1] = mesh->bf(face_idx)->v1()->loc() - mesh->bf(face_idx)->v3()->loc();
   edges[2] = mesh->bf(face_idx)->v2()->loc() - mesh->bf(face_idx)->v1()->loc();
   
}

inline
void
compute_ntb_coord_sys(const Wvec edges[3], Wvec &n, Wvec &t, Wvec &b)
{
   
   // N-T-B coordinate system per face
   t = edges[0].normalized();
   n = cross(edges[0], edges[1]);
   b = cross(n, t).normalized();
   
}

inline
void
compute_initial_vertex_coord_sys(const BMESH *mesh,
                                 vector<Wvec> &pdir1, vector<Wvec> &pdir2)
{
   
   // Set up an initial coordinate system per vertex
      
   for(int i = 0; i < mesh->nfaces(); ++i){
      
      pdir1[mesh->bf(i)->v1()->index()] = mesh->bf(i)->v2()->loc()
                                        - mesh->bf(i)->v1()->loc();
      pdir1[mesh->bf(i)->v2()->index()] = mesh->bf(i)->v3()->loc()
                                        - mesh->bf(i)->v2()->loc();
      pdir1[mesh->bf(i)->v3()->index()] = mesh->bf(i)->v1()->loc()
                                        - mesh->bf(i)->v3()->loc();
      
   }
   
   for(int i = 0; i < mesh->nverts(); i++){
      
      pdir1[i] = cross(pdir1[i], mesh->bv(i)->norm()).normalized();
      pdir2[i] = cross(mesh->bv(i)->norm(), pdir1[i]);
      
   }
   
}
