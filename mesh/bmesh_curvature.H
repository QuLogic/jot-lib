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
#ifndef BMESH_CURVATURE_H_IS_INCLUDED
#define BMESH_CURVATURE_H_IS_INCLUDED

/*!
 *  \file bmesh_curvature.H
 *  \brief Contains the declaration of the curvature computation classes
 *  for the BMESH class.
 *
 *  \note This uses code from the TriMesh2 library written by Szymon Rusinkiewicz
 *  of Princeton University (http://www.cs.princeton.edu/gfx/proj/trimesh2/).
 *
 */

#include <vector>

#include "mlib/points.H"
using namespace mlib;

class Bvert;
class BMESH;

/*!
 *  \brief Computes curvature data for a BMESH object.
 *
 */
class BMESHcurvature_data {
   
   public:
   
      //! \name Constructors
      //@{
   
      //! \brief Computes all curvature data for the given BMESH.
      BMESHcurvature_data(const BMESH *mesh_in);
      
      //@}
   
      //! \name Curvature Data Structures
      //! These structures store various different types of data needed for
      //! computing the curvature of a BMESH.
      //@{
   
      /*!
       *  \brief Stores "Voronoi" corner area values for a single face in a mesh.
       *
       */
      struct corner_areas_t {
      
         corner_areas_t(double val = 0.0) { areas[0] = areas[1] = areas[2] = val; }
         
         double areas[3];
         
         double &operator[](int idx) { return areas[idx]; }
         double operator[](int idx) const { return areas[idx]; }
         
      };
   
      /*!
       *  \brief Stores the three unique values of a 2x2 curvature tensor.
       *  
       *  This is used to store curvature tensors for both faces and vertices.
       *
       */
      struct curv_tensor_t {
         
         curv_tensor_t(double val = 0.0)
            { curv[0] = curv[1] = curv[2] = val; }
            
         curv_tensor_t(double val0, double val1, double val2)
            { curv[0] = val0; curv[1] = val1; curv[2] = val2; }
         
         double curv[3];
         
         double &operator[](int idx) { return curv[idx]; }
         double operator[](int idx) const { return curv[idx]; }
         
         curv_tensor_t operator+(const curv_tensor_t &other) const
            { return curv_tensor_t(curv[0] + other[0],
                                   curv[1] + other[1],
                                   curv[2] + other[2]); }
         
         curv_tensor_t &operator+=(const curv_tensor_t &other)
            { *this = curv_tensor_t(curv[0] + other[0], curv[1] + other[1],
                                    curv[2] + other[2]);
              return *this; }
         
         curv_tensor_t operator-(const curv_tensor_t &other) const
            { return curv_tensor_t(curv[0] - other[0],
                                   curv[1] - other[1],
                                   curv[2] - other[2]); }
         
         curv_tensor_t operator*(double s) const
            { return curv_tensor_t(curv[0]*s, curv[1]*s, curv[2]*s); }
            
         friend curv_tensor_t operator*(double s, const curv_tensor_t &curv)
            { return curv*s; }
         
      };
   
      /*!
       *  \brief Stores the four unique values of a 2x2x2 curvature derivative
       *  tensor.
       *  
       *  This is used to store curvature derivative tensors for both faces and
       *  vertices.
       *
       */
      struct dcurv_tensor_t {
         
         dcurv_tensor_t(double val = 0.0)
            { dcurv[0] = dcurv[1] = dcurv[2] = dcurv[3] = val; }
         
         dcurv_tensor_t(double val0, double val1, double val2, double val3)
            { dcurv[0] = val0; dcurv[1] = val1; dcurv[2] = val2; dcurv[3] = val3; }
         
         double dcurv[4];
         
         double &operator[](int idx) { return dcurv[idx]; }
         double operator[](int idx) const { return dcurv[idx]; }
         
         dcurv_tensor_t operator+(const dcurv_tensor_t &other) const
            { return dcurv_tensor_t(dcurv[0] + other[0], dcurv[1] + other[1],
                                    dcurv[2] + other[2], dcurv[3] + other[3]); }
         
         dcurv_tensor_t &operator+=(const dcurv_tensor_t &other)
            { *this = dcurv_tensor_t(dcurv[0] + other[0], dcurv[1] + other[1],
                                     dcurv[2] + other[2], dcurv[3] + other[3]);
              return *this; }
         
         dcurv_tensor_t operator-(const dcurv_tensor_t &other) const
            { return dcurv_tensor_t(dcurv[0] - other[0], dcurv[1] - other[1],
                                    dcurv[2] - other[2], dcurv[3] - other[3]); }
         
         dcurv_tensor_t operator*(double s) const
            { return dcurv_tensor_t(dcurv[0]*s, dcurv[1]*s, dcurv[2]*s, dcurv[3]*s); }
            
         friend dcurv_tensor_t operator*(double s, const dcurv_tensor_t &dcurv)
            { return dcurv*s; }
         
      };
   
      /*!
       *  \brief Stores the results of diagonalizing a curvature tensor for a
       *  vertex.  Specifically it stores the principal curvatures and directions
       *  for a vertex.
       *
       */
      struct diag_curv_t {
         
//          Wvec curv1, curv2;
//          
//          double k1() const { return curv1.length(); }
//          double k2() const { return curv2.length(); }
//          
//          Wvec pdir1() const { return curv1.normalized(); }
//          Wvec pdir2() const { return curv2.normalized(); }
         
         double _k1, _k2;
         Wvec _pdir1, _pdir2;
         
         double k1() const { return _k1; }
         double k2() const { return _k2; }
         
         Wvec pdir1() const { return _pdir1; }
         Wvec pdir2() const { return _pdir2; }
         
      };
      
      //@}
      
      //! \name Curvature Data Queries
      //@{
      
      curv_tensor_t curv_tensor(const Bvert *v);
      diag_curv_t diag_curv(const Bvert *v);
      double k1(const Bvert *v);
      double k2(const Bvert *v);
      Wvec pdir1(const Bvert *v);
      Wvec pdir2(const Bvert *v);
      dcurv_tensor_t dcurv_tensor(const Bvert *v);
      double feature_size()
         { return mesh_feature_size; }
      
      //@}
   
   private:
   
      const BMESH *mesh;
      
      std::vector<corner_areas_t> corner_areas;   //!< "Voronoi" corner areas for all faces.
      std::vector<double> vertex_areas;           //!< "Voronoi" areas for all vertices.
      std::vector<curv_tensor_t> face_curv;       //!< Per-face curvature tensors.
      std::vector<curv_tensor_t> vertex_curv;     //!< Per-vertex curvature tensors.
      std::vector<diag_curv_t> diag_vertex_curv;  //!< Diagonolized per-vertex curvature tensors.
      std::vector<dcurv_tensor_t> face_dcurv;     //!< Per-face curvature derivative tensors.
      std::vector<dcurv_tensor_t> vertex_dcurv;   //!< Per-vertex curvature derivative tensors.
      double mesh_feature_size;                   //!< Feature size of the mesh.
      
      //! \name Curvature Computation Functions
      //@{
      
      void compute_corner_areas();
      void compute_vertex_areas();
      void compute_face_curvatures();
      void compute_vertex_curvatures();
      void diagonalize_vertex_curvatures();
      void compute_face_dcurv();
      void compute_vertex_dcurv();
      void compute_feature_size();
      
      //@}

};

#endif
