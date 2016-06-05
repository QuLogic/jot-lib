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
#ifndef MAT3_H_IS_INCLUDED
#define MAT3_H_IS_INCLUDED

/*! \file Mat3.H
 *  \brief Contains the declaration of the Mat3 class, a 3x3 matrix class.
 *  \ingroup group_MLIB
 *
 */
 
#include "mlib/vec3.H"
#include "mlib/point3.H"
#include "mlib/line.H"

namespace mlib {

/*! \brief A 3x3 matrix class with double precision floating point elements.
 *  \ingroup gourp_MLIB
 *
 *  Mat3 is a 3x3 matrix class that can be used to perform linear
 *  transformations in 3D space.
 *
 *  To transform a point or a vector, PREmultiply it by a Mat3 instance.
 *
 *  M is the derived 3x3 matrix type, P is a 3D point, V is a 3D vector.
 */
template <typename M, typename P, typename V>
class Mat3 {
 protected:
   
   V _row[3]; //!< Rows of the matrix stored as 3D vectors.

 public:
   
   //! \name Constructors
   //@{

   //! \brief Default constructor.  Creates an identity matrix.
   Mat3() {
      // row vectors are initialized to all 0's;
      // add 1's in diagonal slots:
      _row[0][0] = _row[1][1] = _row[2][2] = 1.0;
   }
      
   //! \brief If rows = true, creates a matrix with rows equal to the
   //! given vectors; otherwise set the columns to the given vectors.
   Mat3(const V& v0, const V& v1, const V& v2, bool rows=true) {
      if (rows) {
         _row[0] = v0;
         _row[1] = v1;
         _row[2] = v2;
      } else {
         _row[0] = V(v0[0],v1[0],v2[0]);
         _row[1] = V(v0[1],v1[1],v2[1]);
         _row[2] = V(v0[2],v1[2],v2[2]);
      }
   }

   //! \brief Constructor that takes 9 elements organized by rows
   Mat3(double m00, double m01, double m02,
        double m10, double m11, double m12,
        double m20, double m21, double m22) {
      _row[0] = V(m00,m01,m02);
      _row[1] = V(m10,m11,m12);
      _row[2] = V(m20,m21,m22);
   }

   //! \brief Copy constructor 
   Mat3(const M& m) {
      _row[0] = m.row(0);
      _row[1] = m.row(1);
      _row[2] = m.row(2);
   }

   //! Assignment operator
   M& operator=(const M& m) {
      if (this != &m) {
         _row[0] = m.row(0);
         _row[1] = m.row(1);
         _row[2] = m.row(2);
      }
      return *this;
   }
   //@}
      
   //! \name Element Access Functions
   //! Functions that allow you to get or set the elements of the matrix in
   //! various different ways.
   //@{
   
   //! \brief Returns a reference to the cell (i1, i2) in the matrix.
   //! No bounds checking is performed in this function.
   double& operator ()(int i1, int i2) { return _row[i1][i2]; }
   //! \brief Returns the value in the cell (i1, i2) in the matrix.
   //! No bounds checking is performed in this function.
   double  operator ()(int i1, int i2) const { return _row[i1][i2]; }
      
   //! \brief Returns a reference to row \p i as a V.  No bounds checking
   //! is performed in this function.
   V&   operator [](int i)                { return _row[i]; }
   //! \brief Returns the value of row \p i as a V.  No bounds checking
   //! is performed in this function.
   V    operator [](int i) const          { return _row[i]; }

   //! \brief Returns row number \p i (no bounds checking)
   V row(int i) const { return _row[i]; }
   //! \brief Returns column number \p j (no bounds checking)
   V col(int j) const { return V(_row[0][j], _row[1][j], _row[2][j]); }

   //! \brief Returns the first column
   V    X()      const  { return col(0); }
   //! \brief Returns the 2nd column
   V    Y()      const  { return col(1); }
   //! \brief Returns the 3rd column
   V    Z()      const  { return col(2); }

   //! Returns a vector containing the lengths of the 3 columns
   V    get_scale() const {
      return V(X().length(),Y().length(),Z().length());
   }
      
   //! \brief Returns the elements of the matrix as a 1D array (in row major
   //! layout).
   const double *matrix() const  { return _row[0]; }
   
   //! \brief Set the 1st column
   void set_X(const V& x) {
      _row[0][0] = x[0]; _row[1][0] = x[1]; _row[2][0] = x[2];
   }
   //! \brief Set the 2nd column
   void set_Y(const V& y) {
      _row[0][1] = y[0]; _row[1][1] = y[1]; _row[2][1] = y[2];
   }
   //! \brief Set the 3rd column
   void set_Z(const V& z) {
      _row[0][2] = z[0]; _row[1][2] = z[1]; _row[2][2] = z[2];
   }
      
   //@}
       
   //! \name Matrix Operations
   //@{
   
   //! \brief Return the transpose of the matrix.
   M transpose() const { return M(row(0),row(1),row(2),false); }

   //! \brief Return the determinant of the matrix.
   double det() const { return cross(_row[0],_row[1])*_row[2]; }

   //! \brief Return the trace of the matrix.
   double trace() const {
      return (*this)(0,0) + (*this)(1,1) + (*this)(2,2);
   }
   //! \brief Return the adjoint of the matrix.
   M adjoint() const {
      return M(cross((*this)[1],(*this)[2]),
               cross((*this)[2],(*this)[0]),
               cross((*this)[0],(*this)[1]));
   }
   //! \brief Return a copy of the matrix with columns normalized
   M unscaled() const {
      return M(col(0).normalized(),
               col(1).normalized(),
               col(2).normalized(),false);
   }

   //! \brief Returns the matrix inverse; if \p debug is true,
   //!        warns of zero determinant:
   M inverse(bool debug = false) const {
      double d = det();
      if (isZero(d)) {
         if (debug) {
            cerr << "Mat3::inverse: warning: determinant is zero"
                 << endl;
         }
         return M(*this);
      }
      return adjoint().transpose()/d;
   }

   //@}
      
   //! \name Overloaded Operators
   //! Overloaded mathematical and logical operators.
   //@{
   
   //! \brief Are matrices exactly equal?
   bool operator==(const M &m) const {
      return (row(0) == m.row(0) &&
              row(1) == m.row(1) &&
              row(2) == m.row(2));
   }

   //! \brief Are matrices not exactly equal?
   bool operator!=(const M &m) const { return !((*this) == m); }
      
   //! \brief Are the matrices equal (within a small tolerance)?
   bool is_equal(const M& m) const {
      return (row(0).is_equal(m.row(0)) &&
              row(1).is_equal(m.row(1)) &&
              row(2).is_equal(m.row(2)));
   }

   //! \brief Returns the identity matrix
   static M Identity() { return M(); }

   //! \brief Is the matrix equal to the identity matrix?
   //!        (within a small tolerance)
   bool is_identity() const { return is_equal(Identity()); }

   //! \brief Returns true if the matrix is symmetric:
   bool is_symmetric() const { return (*this) == transpose(); }

   //! \brief Tells if the matrix is singular (determinant is zero)
   bool is_singular() const { return isZero(det()); }

   //! \brief Tells if the matrix is all zeros (or nearly):
   bool is_null() const {
      return _row[0].is_null() && _row[1].is_null() && _row[2].is_null();
   }

   //@}
      
}; // class Mat3

//! \brief Mat3 by Mat3 multiplication.
//! \relates Mat3
template <typename M, typename P, typename V>
M operator*(const Mat3<M,P,V> &m1, const Mat3<M,P,V> &m2) {
   V c0 = m2.col(0), c1 = m2.col(1), c2 = m2.col(2);
   return M(m1.row(0)*c0,m1.row(0)*c1,m1.row(0)*c2,
            m1.row(1)*c0,m1.row(1)*c1,m1.row(1)*c2,
            m1.row(2)*c0,m1.row(2)*c1,m1.row(2)*c2);
}

//! \brief Mat3 by 3D point multiplication.
//! \relates Mat3
template <typename M, typename P, typename V>
P operator*(const Mat3<M,P,V> &m, const P &p) {
   V v(p[0],p[1],p[2]);
   return P(m.row(0)*v,m.row(1)*v,m.row(2)*v);
}

//! \brief Mat3 by 3D vector multiplication.
//! \relates Mat3
template <typename M, typename P, typename V>
V operator*(const Mat3<M,P,V> &m, const Vec3<V> &v) {
   return V(m.row(0)*v,m.row(1)*v,m.row(2)*v);
}

//! \brief Component wise matrix addition.
//! \relates Mat3
template <typename M, typename P, typename V>
inline M operator+(const Mat3<M,P,V>& n, const Mat3<M,P,V>& m) {
   return M(n[0]+m[0], n[1]+m[1], n[2]+m[2]);
}

//! \brief Component wise matrix subtraction.
//! \relates Mat3
template <typename M, typename P, typename V>
inline M operator-(const Mat3<M,P,V>& n, const Mat3<M,P,V>& m) {
   return M(n[0]-m[0], n[1]-m[1], n[2]-m[2]);
}

//! \brief Component wise matrix negation.
//! \relates Mat3
template <typename M, typename P, typename V>
inline M operator-(const Mat3<M,P,V>& n) {
   return M(-n[0], -n[1], -n[2]);
}

//! \brief Scalar by matrix multiplication.
//! \relates Mat3
template <typename M, typename P, typename V>
inline M operator*(double s, const Mat3<M,P,V>& m) {
   return M(m[0]*s, m[1]*s, m[2]*s);
}

//! \brief Matrix by scalar multiplication.
//! \relates Mat3
template <typename M, typename P, typename V>
inline M operator*(const Mat3<M,P,V>& m, double s) { return s*m; }

//! \brief Matrix by scalar division.
//! \relates Mat3
template <typename M, typename P, typename V>
inline M operator/(const Mat3<M,P,V>& m, double s) {
   return (1/s)*m;
}

//! \brief Stream insertion operator for Mat3 class.
//! \relates Mat3
template <typename M,typename P,typename V>
std::ostream &operator <<(std::ostream &os, const Mat3<M,P,V> &x) {
   return os << "{" << x[0] << "\n " << x[1] << "\n " << x[2] << "}";
}

//! \brief Stream insertion operator for Mat3 class.
//! \relates Mat3
template <typename M,typename P,typename V>
std::istream &operator >>(std::istream &is, const Mat3<M,P,V> &x) {
   char brace;
   return is >> brace >> x[0] >> x[1] >> x[2] >> brace;
}

} // namespace mlib

#endif // MAT3_H_IS_INCLUDED
