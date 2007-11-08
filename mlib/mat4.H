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
#ifndef MAT4_H_IS_INCLUDED
#define MAT4_H_IS_INCLUDED

/*! \file Mat4.H
 *  \brief Contains the declaration of the Mat4 class, a 4x4 matrix class.
 *  \ingroup group_MLIB
 *
 */
 
#include "mlib/vec4.H"
#include "mlib/point3.H"
#include "mlib/line.H"

namespace mlib {

/*! \brief A 4x4 matrix class with double precision floating point elements.
 *  \ingroup gourp_MLIB
 *
 *  Mat4 is a 4x4 matrix class that can be used to perform general affine
 *  or projective transformations in 3D space.
 *
 *  To transform a point or a vector, PREmultiply it by a Mat4 instance.
 *
 *  \libgfx Mat4 uses some code from the Mat4 class in libgfx.
 *
 *  \todo Add eigenvector and eigenvalue finding functions.
 *
 */
template <typename M, typename P, typename V, typename L, typename Q>
class Mat4 {
   
   protected:
   
      Vec4 row[4];            //!< Rows of the matrix stored as 4D vectors.
      bool perspective;       //!< Is this matrix a perspective transform?

   public:
   
      //! \name Constructors
      //@{

      //! \brief Default constructor.  Creates an identity matrix.
      inline Mat4()
         : perspective(false)
      {
         row[0] = row[1] = row[2] = row[3] = Vec4(0.0, 0.0, 0.0, 0.0);
         row[0][0] = row[1][1] = row[2][2] = row[3][3] = 1.0;
      }
      
      //! \brief Constructor that creates a matrix with rows equal to the
      //! vectors givens as arguments.  Perspective can also be set.
      inline Mat4(Vec4 row0, Vec4 row1, Vec4 row2, Vec4 row3, bool perspec = false)
         : perspective(perspec)
      { row[0] = row0; row[1] = row1; row[2] = row2; row[3] = row3; }
   
      //! \brief  This constructor creates a rigid motion transform from an
      //! origin and directions of x, y and z axes.
      Mat4(const P &origin, const V &x_dir, const V &y_dir, const V &z_dir);
   
      //! \brief This constructor creates a linear map (no translation)
      //! whose columns are the given vectors.
      Mat4(const V &col0, const V &col1, const V &col2);
   
      //! \brief This constructor creates a rigid motion transform from an
      //! origin and direction of x and y axis. If yDir is not perpendicular to
      //! xDir, yDir will be adjusted.
      Mat4(const P &origin, const V &xDir, const V &yDir);
   
      //! \brief This constructor creates a rigid motion transform from an
      //! origin (axis.point) and the direction of the z axis (axis.vector). The
      //! directions of the x and y axes are determined according to an 
      //! arbitrary axis algorithm.
      Mat4(const L& axis);
   
      //! \brief The constructor creates a rigid motion transform from an origin.
      //! The x, y and z axis are aligned with the world x, y and z axis.
      Mat4(const P& origin);
      
      //@}
      
      //! \name Element Access Functions
      //! Functions that allow you to get or set the elements of the matrix in
      //! various different ways.
      //@{
   
      //! \brief Returns a reference to the cell (i1, i2) in the matrix.
      //! No bounds checking is performed in this function.
      double& operator ()(int i1, int i2)       { return row[i1][i2]; }
      //! \brief Returns the value in the cell (i1, i2) in the matrix.
      //! No bounds checking is performed in this function.
      double  operator ()(int i1, int i2) const { return row[i1][i2]; }
      
      //! \brief Returns a reference to row \p i as a Vec4.  No bounds checking
      //! is performed in this function.
      Vec4&   operator [](int i)                { return row[i]; }
      //! \brief Returns the value of row \p i as a Vec4.  No bounds checking
      //! is performed in this function.
      Vec4    operator [](int i) const          { return row[i]; }
   
      //! Returns a vector containing the lengths of the axes of the coordinate
      //! system defined by the matrix.
      V    get_scale() const
         { return V(X().length(),Y().length(),Z().length()); }
      
      //! Returns by reference the origin and axes of the coordinate system
      //! defined by the matrix.
      void get_coord_system(P& o, V& x, V& y, V& z) const
         { x = X(); y = Y(); z = Z(); o = origin(); }
   
      //! \brief Returns the x-axis of the coordinate system defined by the
      //! matrix as a vector.
      V    X()      const  { return V(row[0][0], row[1][0], row[2][0]); }
      //! \brief Returns the y-axis of the coordinate system defined by the
      //! matrix as a vector.
      V    Y()      const  { return V(row[0][1], row[1][1], row[2][1]); }
      //! \brief Returns the z-axis of the coordinate system defined by the
      //! matrix as a vector.
      V    Z()      const  { return V(row[0][2], row[1][2], row[2][2]); }
      //! \brief Returns the origin of the coordinate system defined by the
      //! matrix as a point.
      P    origin() const  { return P(row[0][3], row[1][3], row[2][3]); }
      
      
      //! \brief Returns the elements of the matrix as a 1D array (in row major
      //! layout).
      //! \libgfx This function is borrowed from the libgfx Mat4 class.
      const double  *matrix() const  { return row[0]; }
   
      //! \brief Set the x-axis of the coordinate system defined by the matrix.
      void set_X(const V& x)
         { row[0][0] = x[0]; row[1][0] = x[1]; row[2][0] = x[2]; }
      //! \brief Set the y-axis of the coordinate system defined by the matrix.
      void set_Y(const V& y)
         { row[0][1] = y[0]; row[1][1] = y[1]; row[2][1] = y[2]; }
      //! \brief Set the z-axis of the coordinate system defined by the matrix.
      void set_Z(const V& z)
         { row[0][2] = z[0]; row[1][2] = z[1]; row[2][2] = z[2]; }
      //! \brief Set the origin of the coordinate system defined by the matrix.
      void set_origin(const P& o)
         { row[0][3] = o[0]; row[1][3] = o[1]; row[2][3] = o[2]; }
   
      //! \brief Returns the rotation part of the transform.
      M    rotation() const { return M(P(0,0,0), X(), Y(), Z()); }
      
      //@}
       
      //! \name Special Transform Constructors
      //! Static functions to create matrices that perform special
      //! transformations.
      //@{
         
      //! \brief Create a matrix that does the rotation described by the
      //! quaternion \a quat.
      static M rotation   (const  Q& quat);
      //! \brief Create a matrix that rotates \a angle radians about the axis
      //! described by line \a axis.
      static M rotation   (const  L& axis,    double   angle);
      //! \brief Create a matrix that rotates \a angle radians about the axis
      //! described by vector \a axis.
      static M rotation   (const  V& axis,    double   angle);
      
      //! \brief Create a matrix that performs a shearing transform.
      static M shear      (const  V& normal,  const V& shear_vec);
      
      //! \brief Create a matrix that does a uniform scale by \a factor along
      //! the x-, y- and z-directions, centered at the point fixed_pt.
      static M scaling    (const  P& fixed_pt, double   factor);
      //! \brief Create a matrix that scales along x-, y- and z-axes by the
      //! amount in the corresponding component of \a xyz_factors and that
      //! leaves the point fixed_pt unmoved.
      static M scaling    (const  P& fixed_pt, const V& xyz_factors);
      //! \brief Create a matrix that scales along the x-, y- and z-axes by the
      //! amount in the corresponding component of \a xyz_factors.
      static M scaling    (const  V& xyz_factors);
      //! \brief Create a matrix that scales by \a x along the x-axis, \a y
      //! along the y-axis and \a z along the z-axis.
      static M scaling    (double x, double y, double z)
         { return scaling(V(x,y,z)); }
      //! \brief Create a matrix that does a uniform scale by \a factor.
      static M scaling    (double    factor);
      
      //! \brief Create a matrix that scales along the direction of \a axis.vector
      //! leaving point \a axis.point unmoved.
      static M stretching (const  L& axis);
      
      //! \brief Create a matrix that does a translation along the vector \a vec.
      static M translation(const  V& vec);

      //! \brief Create a matrix that translates the origin to the point \a p.
      static M translation(const P& p) {
         return translation(p - P(0,0,0));
      }
   
      //! \brief Contruct a rotation / non-uniform scale matrix that maps the
      //! line segment joining \a anchor and \a old_pt to the segment joining
      //! \a anchor and \a new-pt.
      static M anchor_scale_rot(const P& anchor,
                                const P& old_pt,
                                const P& new_pt);
   
      //! \brief Map a triple of points (src1,src2,src3) to a triple of points 
      //! (dst1,dst2,dst3).  Point src1 maps to dst1, line (src1,src2) maps to
      //! line (dst1,dst2) and plane (src1,src2,src3) maps to plane
      //! (dst1,dst2,dst3).
      static M align(const P& src1, const P& src2, const P& src3,
                     const P& dst1, const P& dst2, const P& dst3);
      
      static M align(const P&  src1, const V& src2, const V& src3,
                     const P&  dst1, const V& dst2, const V& dst3);
   
      static M align(const P&  src1, const V&    src2,
                     const P&  dst1, const V&    dst2);
   
      static M align_and_scale(const P& o, const V& x, const V& y, const V& z);
      
      //! \brief Creates a matrix that performs a perspective transformation
      //! exactly like gluPerspective.
      static M glu_perspective(double fovy, double aspect,
             double zmin=0.0, double zmax=0.0);

      //! \brief Creates a matrix that performs a transformation exactly like
      //! gluLookAt.
      static M glu_lookat(const V& from, const V& at, const V& up);

      //! \brief Creates a matrix that performs the OpenGL viewport
      //! transformation.
      static M gl_viewport(double w, double h);
      
      //@}
      
      //! \name Matrix Operations
      //@{
   
      //! \brief Return the transpose of the matrix.
      M        transpose       ()                    const;
      //! \brief Return the determinant of the matrix.
      //! \libgfx This function was take directly from libgfx (it was only
      //! modified to make it into a member function).
      double   det             ()                    const
         { return row[0] * -cross(row[1], row[2], row[3]); }
      //! \brief Return the trace of the matrix.
      //! \libgfx This function was take directly from libgfx (it was only
      //! modified to make it into a member function).
      double   trace           ()                    const
         { return (*this)(0,0)+(*this)(1,1)+(*this)(2,2)+(*this)(3,3); }
      //! \brief Return the adjoint of the matrix.
      M        adjoint         ()                    const;
      //! \brief Return a copy of the matrix with the axes of the coordinate
      //! system it defines normalized.
      M        unscaled        ()                    const;
      //! \brief Returns a normalized basis version of the matrix.
      M        normalized_basis()                    const;
      //! \brief Returns a orthogonalized version of the matrix.
      M        orthogonalized  ()                    const;
      //! \brief Returns the inverse of the matrix.
      //         If debug is true, warns of near-zero determinant.
      M        inverse(bool debug = false) const;
      double   inverse(Mat4<M,P,V,L,Q> &inv) const;
      //! \brief Returns the derivative of the matrix at point \a p.
      M        derivative(const P& p)                const;
      
      //@}
      
      //! \name Matrix Property Queries
      //! Functions that check various properties of the matrix.
      //@{
         
      //! \brief Set whether this matrix represents a perspective transform or
      //! not.
      void set_perspective(bool p)  { perspective = p;   }
      //! \brief Does this matrix represent a perspective transform?
      bool is_perspective()  const  { return perspective;}
   
      //! \brief Is the matrix valid?
      bool     is_valid                   () const;
      //! \brief Is the matrix equal to the identity matrix?
      bool     is_identity                () const;
      //! \brief Is the matrix orthogonal (no shearing)?
      bool     is_orthogonal              () const;
      //! \brief Is the matrix equal_scaling orthogonal (no shearing, no
      //! nonequal scaling)?
      bool     is_equal_scaling_orthogonal() const;
      //! \brief Is the matrix orthonormal (orthogonal, no scaling)?
      bool     is_orthonormal() const
         { return (is_orthogonal() && get_scale().is_equal(V(1,1,1))); }
        
      //@}
      
      //! \name Overloaded Operators
      //! Overloaded mathematical and logical operators.
      //@{
   
      //! \brief Overloaded equality operator.  Only checks to see if top 3 rows
      //! are equal.
      int operator == (const M &m) const
         { return origin() == m.origin() && 
                  X() == m.X() && Y() == m.Y() && Z() == m.Z(); }
      
      //@}

}; // class Mat4

} // namespace mlib

namespace mlib {

//! \brief Mat4 by Mat4 multiplication.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
M operator*(const Mat4<M,P,V,L,Q> &m, const Mat4<M,P,V,L,Q> &m2);

//! \brief Mat4 by 3D point multiplication.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
P operator*(const Mat4<M,P,V,L,Q> &m, const P &p);

//! \brief Mat4 by 3D vector multiplication.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
V operator*(const Mat4<M,P,V,L,Q> &m, const Vec3<V> &v);

//! \brief Mat4 by 3D line multiplication.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
L operator*(const Mat4<M,P,V,L,Q> &m, const Line<L,P,V> &l);

//! \brief Component wise matrix addition.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
inline M operator+(const Mat4<M,P,V,L,Q>& n, const Mat4<M,P,V,L,Q>& m)
   { return M(n[0]+m[0], n[1]+m[1], n[2]+m[2], n[3]+m[3],
              n.is_perspective() || m.is_perspective()); }

//! \brief Component wise matrix subtraction.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
inline M operator-(const Mat4<M,P,V,L,Q>& n, const Mat4<M,P,V,L,Q>& m)
   { return M(n[0]-m[0], n[1]-m[1], n[2]-m[2], n[3]-m[3],
              n.is_perspective() || m.is_perspective()); }

//! \brief Component wise matrix negation.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
inline M operator-(const Mat4<M,P,V,L,Q>& n)
   { return M(-n[0], -n[1], -n[2], -n[3], n.is_perspective()); }

//! \brief Scalar by matrix multiplication.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
inline M operator*(double s, const Mat4<M,P,V,L,Q>& m)
   { return M(m[0]*s, m[1]*s, m[2]*s, m[3]*s, m.is_perspective()); }

//! \brief Matrix by scalar multiplication.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
inline M operator*(const Mat4<M,P,V,L,Q>& m, double s)
   { return s*m; }

//! \brief Matrix by scalara division.
//! \relates Mat4
template <typename M, typename P, typename V, typename L, typename Q>
inline M operator/(const Mat4<M,P,V,L,Q>& m, double s)
   { return M(m[0]/s, m[1]/s, m[2]/s, m[3]/s, m.is_perspective()); }

//! \brief Stream insertion operator for Mat4 class.
//! \relates Mat4
template <typename M,typename P,typename V, typename L, typename Q>
std::ostream &operator <<(std::ostream &os, const Mat4<M,P,V,L,Q> &x);

} // namespace mlib

#ifdef JOT_NEEDS_TEMPLATES_IN_H_FILE
#include "mat4.C"
#endif

#endif // MAT4_H_IS_INCLUDED
