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
#include "mlib/mat4.H"
#include "mlib/vec3.H"
#include "mlib/global.H"

/*!
 *  \file Mat4.C
 *  \brief Contains the definitions of member functions for the Mat4 class.
 *  \ingroup group_MLIB
 *
 */

/*!
 *  Construct a rigid motion transformation from given coordinate
 *  system (origin,x_dir,y_dir,z_dir). This transformation will
 *  transform points from (origin,x_dir,y_dir,z_dir) to world.
 *
 *  \param[in] origin The origin of the coordinate system that will be defined
 *  by the new matrix.
 *  \param[in] x_dir The direction of the x-axis of the coordinate system that
 *  will be defined by the new matrix.
 *  \param[in] y_dir The direction of the y-axis of the coordinate system that
 *  will be defined by the new matrix.
 *  \param[in] z_dir The direction of the z-axis of the coordinate system that
 *  will be defined by the new matrix.
 *
 *  \todo Change use of temp matrix to direct access to the this pointer.
 *
 */
template <typename M, typename P, typename V, typename L, typename Q>
MLIB_INLINE
mlib::Mat4<M,P,V,L,Q>::Mat4(
   const P& origin, 
   const V& x_dir, 
   const V& y_dir, 
   const V& z_dir
   )
{
   const V xx = x_dir.normalized();
   const V yy = y_dir.normalized();
   const V zz = z_dir.normalized();

   assert(!xx.is_null());
   assert(!yy.is_null());
   assert(!zz.is_null());

   Mat4<M,P,V,L,Q> t;

   for (int i = 0; i < 3; i++) {
      t(i,0) = xx    [i];
      t(i,1) = yy    [i];
      t(i,2) = zz    [i];
      t(i,3) = origin[i];
      t(3,i) = 0.0;
   }
   t(3,3) = 1.0;

   (*this) = t;
} 

/*!
 *  Construct a matrix whose columns are the given vectors.
 *
 *  \param[in] col0 Becomes the first 3 rows of the first column of the matrix.
 *  \param[in] col1 Becomes the first 3 rows of the second column of the matrix.
 *  \param[in] col2 Becomes the first 3 rows of the third column of the matrix.
 *
 *  \todo Change use of temp matrix to direct access to the this pointer.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
mlib::Mat4<M,P,V,L,Q>::Mat4(const V& col0, const V& col1, const V& col2)
{
   Mat4<M,P,V,L,Q> t;

   for (int i = 0; i < 3; i++) {
      t(i,0) = col0  [i];
      t(i,1) = col1  [i];
      t(i,2) = col2  [i];
      t(i,3) = t(3,i) = 0.0;
   }
   t(3,3) = 1.0;

   (*this) = t;
} 

/*!
 *  Construct a rigid motion transformation from the coordinate
 *  system (origin,x_dir,y_dir,cross(x_dir,y_dir)). This transformation will
 *  transform points from (origin,x_dir,y_dir,cross(x_dir,y_dir)) to world.
 *
 *  \param[in] origin The origin of the coordinate system that will be defined
 *  by the new matrix.
 *  \param[in] x_dir The direction of the x-axis of the coordinate system that
 *  will be defined by the new matrix.
 *  \param[in] y_dir The direction of the y-axis of the coordinate system that
 *  will be defined by the new matrix.
 *
 *  \note \a y_dir is recomputed to be cross(cross(x_dir,y_dir), x_dir).
 *
 *  \note If the result of cross(x_dir,y_dir) is a null vector, then the
 *  constructed matrix will be the identity matrix.
 *
 *  \todo Change use of temp matrix to direct access to the this pointer.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
mlib::Mat4<M,P,V,L,Q>::Mat4(const P& origin, const V& x_dir, const V& y_dir)
{
   // isError = false;

   const V xx = x_dir.normalized();
   const V zz = cross(xx, y_dir).normalized();

   if (zz.is_null()) {
      *this = M();
      // isError = true;
      return;
   }

   const V yy = cross(zz, xx).normalized();
   *this = M(origin, xx, yy, zz);
}



/*!
 *  Construct a transform that moves origin to axis.point and
 *  z-axis to axis.vector. The x and y axes are constructed
 *  according to an arbitrary axis algorithm.
 *
 *  \param[in] axis A line specifying the origin and z-axis of the coordinate
 *  system that will be defined by the new matrix.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
mlib::Mat4<M,P,V,L,Q>::Mat4(const L& axis)
{
   // isError = false;

   const V zDir = axis.vector().normalized();

   if (zDir.is_null())
   {
       *this = M(axis.point(), V(1,0,0), V(0,1,0), V(0,0,1));
       // isError = true;
   } else {
      const V xDir = zDir.perpend();
      const V yDir = cross(zDir, xDir);
      *this = M(axis.point(), xDir, yDir, zDir);
   }
}

/*!
 *  \param[in] origin The origin of the rigid motion transform.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
mlib::Mat4<M,P,V,L,Q>::Mat4(const P& origin) 
{
   M t;

   t(0,3) = origin[0];
   t(1,3) = origin[1];
   t(2,3) = origin[2];

   (*this) = t;
} 


/*!
 *  \param[in] vec A vector describing the length and direction of the
 *  translation.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::translation(const V& vec)
{
   M t;

   t(0,3) = vec[0]; 
   t(1,3) = vec[1];
   t(2,3) = vec[2];

   return t;
}



/*!
 *  \param[in] axis A vector describing the axis to rotate about.
 *  \param[in] angle The angle to rotate through (in radians).
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::rotation(const V& axis, double angle)
{
   return  rotation(L(P(), axis), angle); 
}

/*!
 *  \param[in] axis A line describing the axis to rotate about.
 *  \param[in] angle The angle to rotate through (in radians).
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::rotation(const L& axis, double angle)
{
   const V      v  = axis.vector().normalized();
   const P      p  = axis.point();
   const double sa = sin(angle);
   const double ca = cos(angle);

   M t;

   t(0,0) = v[0]*v[0]  + ca*(1.0-v[0]*v[0]);
   t(0,1) = v[0]*v[1]*(1.0-ca) - v[2]*sa;
   t(0,2) = v[2]*v[0]*(1.0-ca) + v[1]*sa;

   t(1,0) = v[0]*v[1]*(1.0-ca) + v[2]*sa;
   t(1,1) = v[1]*v[1] + ca*(1.0-v[1]*v[1]);
   t(1,2) = v[1]*v[2]*(1.0-ca) - v[0]*sa;

   t(2,0) = v[0]*v[2]*(1.0-ca) - v[1]*sa;
   t(2,1) = v[1]*v[2]*(1.0-ca) + v[0]*sa;
   t(2,2) = v[2]*v[2] + ca*(1.0-v[2]*v[2]);

   t(0,3) = p[0] - (t(0,0)*p[0] + t(0,1)*p[1] + t(0,2)*p[2]);
   t(1,3) = p[1] - (t(1,0)*p[0] + t(1,1)*p[1] + t(1,2)*p[2]);
   t(2,3) = p[2] - (t(2,0)*p[0] + t(2,1)*p[1] + t(2,2)*p[2]);

   t(3,0) = t(3,1) = t(3,2) = 0;
   t(3,3) = 1.0;

   return t;
} 


/*!
 *  \param[in] quat A quaternion describing the rotation.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::rotation(const Q& quat)
{
   double angle = 2.0 * Acos(quat.w());
   const V axis = (angle != 0.0) ? quat.v()/sin(angle/2.0) : V(0,1,0);
   return rotation(axis, angle);
}

/*!
 *  \param[in] fixed_pt The point that should remain fixed after the scaling.
 *  \param[in] factor The amount to scale by in the x-, y- and z-directions.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::scaling(const P& fixed_pt, double factor)
{
   return scaling(fixed_pt, V(factor, factor, factor));
}

/*!
 *  \param[in] factor The amount to scale by along the x-, y- and z-axes.
 *
 */ 
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::scaling(double factor)
{
   return scaling(P(), V(factor, factor, factor));
}


/*!
 *  \param[in] xyz_factors A vector whose components are the amount to scale by
 *  along their corresponding directions.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::scaling(const V& xyz_factors)
{
   return scaling(P(), xyz_factors);
} 

/*!
 *  \param[in] fixed_pt The point that should remain fixed after the scaling.
 *  \param[in] xyz_factors A vector whose components are the amount to scale by
 *  along their corresponding directions.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::scaling(const P& fixed_pt, const V& xyz_factors)
{
   M t;

   t(0,0) = xyz_factors[0];
   t(1,1) = xyz_factors[1];
   t(2,2) = xyz_factors[2];

   t(0,3) = fixed_pt[0] * (1.0-xyz_factors[0]);
   t(1,3) = fixed_pt[1] * (1.0-xyz_factors[1]);
   t(2,3) = fixed_pt[2] * (1.0-xyz_factors[2]);

   return t;
} 

/*!
 *  \param[in] normal Some sort of normal vector.  Not sure exactly what this is
 *  (perhaps the normal vector to the plane that will be sheared?).
 *  \param[in] shear_vec A vector describing the shear.  Not sure exactly how
 *  this is interpreted.
 *
 *  \todo Document this function more completely.
 *
 *  \question What are the exact meanings of the parameters?
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::shear(const V& normal, const V& shear_vec)
{
   M t;

   V      realShear = shear_vec;
   double dot       = normal * shear_vec;

   // Shear vec must be perpendicular to normal
   if (fabs(dot) > epsAbsSqrdMath()) 
      realShear = shear_vec - normal * dot;

   // Set rows to coordinate axes transformed by shear
   for (int j = 0; j < 3; j++) { 
      t(j, 0) += normal[0] * realShear[j];
      t(j, 1) += normal[1] * realShear[j];
      t(j, 2) += normal[2] * realShear[j];
   }

   return t;
}



/*!
 *  \param[in] axis A line describing the direction to stretch along and the
 *  amount to stretch as well as the point that should remain fixed after the
 *  stretching.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::stretching(const L& axis)
{
   const M t    = M(axis);
   const M invt = t.inverse();

   P q = invt * axis.point();  
   M mat = scaling(q, V(1,1,axis.vector().length()));

   return t * mat * invt;
}


/*!
 *  Contruct a rotation / non-uniform scale matrix that maps
 *  the line segment joining anchor -- old_pt to the segment
 *  joining anchor -- new_pt:
 *  
 *  <tt>                                                
 *                                                  
 *               new point                          
 *                 /                                
 *                /                                 
 *               /                                  
 *              /                                   
 *             /                                    
 *            /                                     
 *           /                                      
 *     anchor ------------------- old point         
 *                                                  
 *  </tt>
 *
 */ 
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::anchor_scale_rot(
   const P& anchor,
   const P& old_pt,
   const P& new_pt)
{                                                

   V   old_vec = (old_pt - anchor).normalized();
   V   new_vec = (new_pt - anchor).normalized();
   double old_len = anchor.dist(old_pt);
   double new_len = anchor.dist(new_pt);

   // Avoid division by zero:
   if (old_len < gEpsAbsMath) {
      err_msg("Mat4::anchor_scale_rot: Original segment too short");
      return M();
   }

   // First non-uniformly scale the old segment to the length of
   // the new one:
   M z2old = M(L(anchor, old_vec));     // rotates z-axis to old seg
   M ret = (
      z2old *                                   // z-axis to old seg
      M::scaling(V(1,1,new_len/old_len)) *      // scale z-axis
      z2old.inverse()                        // old seg to z-axis
      );

   // now rotate the old segment to the new one:
   V n = cross(new_vec, old_vec).normalized();
   if (n.is_null())
      return ret;       // Rotation not necessary

   return M::rotation(L(anchor,n), -Acos(new_vec*old_vec)) * ret;
}


template <typename M, typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::operator*(const Mat4<M,P,V,L,Q> &m, const Mat4<M,P,V,L,Q>& m2)
{
   M        t;

   t(0,0) = m(0,0) * m2(0,0) +
            m(0,1) * m2(1,0) +
            m(0,2) * m2(2,0) +
            m(0,3) * m2(3,0) ;

   t(0,1) = m(0,0) * m2(0,1) +
            m(0,1) * m2(1,1) +
            m(0,2) * m2(2,1) +
            m(0,3) * m2(3,1) ;

   t(0,2) = m(0,0) * m2(0,2) +
            m(0,1) * m2(1,2) +
            m(0,2) * m2(2,2) +
            m(0,3) * m2(3,2) ;

   t(0,3) = m(0,0) * m2(0,3) +
            m(0,1) * m2(1,3) +
            m(0,2) * m2(2,3) +
            m(0,3) * m2(3,3) ;

   t(1,0) = m(1,0) * m2(0,0) +
            m(1,1) * m2(1,0) +
            m(1,2) * m2(2,0) +
            m(1,3) * m2(3,0) ;

   t(1,1) = m(1,0) * m2(0,1) +
            m(1,1) * m2(1,1) +
            m(1,2) * m2(2,1) +
            m(1,3) * m2(3,1) ;

   t(1,2) = m(1,0) * m2(0,2) +
            m(1,1) * m2(1,2) +
            m(1,2) * m2(2,2) +
            m(1,3) * m2(3,2) ;

   t(1,3) = m(1,0) * m2(0,3) +
            m(1,1) * m2(1,3) +
            m(1,2) * m2(2,3) +
            m(1,3) * m2(3,3) ;

   t(2,0) = m(2,0) * m2(0,0) +
            m(2,1) * m2(1,0) +
            m(2,2) * m2(2,0) +
            m(2,3) * m2(3,0) ;

   t(2,1) = m(2,0) * m2(0,1) +
            m(2,1) * m2(1,1) +
            m(2,2) * m2(2,1) +
            m(2,3) * m2(3,1) ;

   t(2,2) = m(2,0) * m2(0,2) +
            m(2,1) * m2(1,2) +
            m(2,2) * m2(2,2) +
            m(2,3) * m2(3,2) ;

   t(2,3) = m(2,0) * m2(0,3) +
            m(2,1) * m2(1,3) +
            m(2,2) * m2(2,3) +
            m(2,3) * m2(3,3) ;

   t(3,0) = m(3,0) * m2(0,0) +
            m(3,1) * m2(1,0) +
            m(3,2) * m2(2,0) +
            m(3,3) * m2(3,0) ;

   t(3,1) = m(3,0) * m2(0,1) +
            m(3,1) * m2(1,1) +
            m(3,2) * m2(2,1) +
            m(3,3) * m2(3,1) ;

   t(3,2) = m(3,0) * m2(0,2) +
            m(3,1) * m2(1,2) +
            m(3,2) * m2(2,2) +
            m(3,3) * m2(3,2) ;

   t(3,3) = m(3,0) * m2(0,3) +
            m(3,1) * m2(1,3) +
            m(3,2) * m2(2,3) +
            m(3,3) * m2(3,3) ;

   t.set_perspective(m.is_perspective() || m2.is_perspective());

   return t;

} // operator *

/*!
 *  Convenience version of inverse that returns the inverse rather than passing
 *  it back by reference.  This version also prints an error message if a
 *  singular matrix is detected (in which case the returned matrix is undefined).
 *
 *  \return The inverse of the matrix.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::inverse(bool debug) const
{
   // Compute the inverse, store in ret, return determinant
   M ret;
   double det = inverse(ret); 
   if (debug && isZero(det)) {
      // Complain if result is singular:
      cerr << "Mat4::inverse: error: singular matrix" << endl;
      // gdb won't recognize line numbers in templated functions
      // like this, but it does recognize the following function in
      // case you want to set a breakpoint there to find out who is
      // trying to compute the inverse of a singular matrix:
      fn_gdb_will_recognize_so_i_can_set_a_fuggin_breakpoint();
   }
   return ret;
}

/*!
 *  \brief Matrix inversion code for 4x4 matrices using Gaussian elimination
 *  with partial pivoting.  This is a specialized version of a
 *  procedure originally due to Paul Heckbert <ph@cs.cmu.edu>.
 *
 *  If the matrix is singular, returns 0 and leaves trash in \p inv.
 *
 *  \param[out] inv The result of inverting the matrix.  If the matrix is
 *  singular, the value of \p inv is undefined.
 *  
 *  \return Determinant of the matrix.  If the matrix is singular, the
 *  determinant will be 0 (or very close to 0).
 *
 *  \libgfx This function was taken directly from libgfx.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
double
mlib::Mat4<M,P,V,L,Q>::inverse(Mat4<M,P,V,L,Q> &inv) const
{
   Mat4<M,P,V,L,Q> A = *this;
   int i, j, k;
   double maxval, t, detr, pivot;

   /*---------- forward elimination ----------*/

   for (i=0; i<4; i++)                /* put identity matrix in inv */
      for (j=0; j<4; j++)
         inv(i, j) = (double)(i==j);

   detr = 1.0;
   for (i=0; i<4; i++) {              /* eliminate in column i, below diag */
      maxval = -1.;
      for (k=i; k<4; k++)             /* find pivot for column i */
         if (fabs(A(k, i)) > maxval) {
            maxval = fabs(A(k, i));
            j = k;
         }
      if (maxval<=0.) return 0.;      /* if no nonzero pivot, PUNT */
      if (j!=i) {                     /* swap rows i and j */
         for (k=i; k<4; k++)
            { t = A(i,k); A(i,k) = A(j,k); A(j,k) = t; }
         for (k=0; k<4; k++)
            { t = inv(i,k); inv(i,k) = inv(j,k); inv(j,k) = t; }
         detr = -detr;
      }
      pivot = A(i, i);
      detr *= pivot;
      for (k=i+1; k<4; k++)           /* only do elems to right of pivot */
         A(i, k) /= pivot;
      for (k=0; k<4; k++)
         inv(i, k) /= pivot;
      /* we know that A(i, i) will be set to 1, so don't bother to do it */

      for (j=i+1; j<4; j++) {         /* eliminate in rows below i */
         t = A(j, i);                 /* we're gonna zero this guy */
         for (k=i+1; k<4; k++)        /* subtract scaled row i from row j */
            A(j, k) -= A(i, k)*t;     /* (ignore k<=i, we know they're 0) */
         for (k=0; k<4; k++)
            inv(j, k) -= inv(i, k)*t;
      }
   }

   /*---------- backward elimination ----------*/

   for (i=4-1; i>0; i--) {            /* eliminate in column i, above diag */
      for (j=0; j<i; j++) {           /* eliminate in rows above i */
         t = A(j, i);                 /* we're gonna zero this guy */
         for (k=0; k<4; k++)          /* subtract scaled row i from row j */
             inv(j, k) -= inv(i, k)*t;
      }
   }

   inv.perspective = perspective;
   
   return detr;

}

/*!
 *  Returns the derivative of the map "mult by this matrix".  Of
 *  course, this is only interesting for perspective matrices, since
 *  otherwise the derivative is the matrix itself. (We can ignore the
 *  translational component of the matrix since the derivative should
 *  only be multiplied with vectors, and the translational component
 *  doesn't affect them).
 *
 *  \param[in] p The point at which the derivative is computed.
 *
 *  \note The derivative is only computed for matrices with
 *  perspective set to true.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::derivative(const P& p) const
{
   const M &t = (const M&)(*this);
   
   if (!perspective)
      return t;

   double h = t(3,0)*p[0] + t(3,1)*p[1] + t(3,2)*p[2] + t(3,3);
   if (fabs(h) < 1e-6) {
      cerr << "Mat4::derivative: bad homogeneous coordinate" << endl;
      return t;
   }
   P q = t * p;
   V px(q[0], q[1], q[2]);

   return M(
      (t.X() - t(3,0)*px)/h,
      (t.Y() - t(3,1)*px)/h,
      (t.Z() - t(3,2)*px)/h
      );
}

template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::unscaled() const
{
   P o;
   V x,y,z;
   get_coord_system(o,x,y,z);

   return M(o,x,y,z);
}

/*!
 *  Return a copy of the matrix with the axes of the coordinate
 *  system it defines normalized and the origin of that coordinate system
 *  replaced with the origin of the world coordinate system, (0, 0, 0).
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::normalized_basis() const
{
   P o;
   V x,y,z;
   get_coord_system(o,x,y,z);

   return M(P(),x,y,z);
}

/*!
 *  Creates a new matrix that defines a coordinate system with the
 *  same origin as the coordinate system defined by the original matrix,
 *  but x-axis normalized, the y-axis redefined to be perpendicular to the
 *  x-axis (and also unit length), and the z-axis redefined to be
 *  perpendicular to both (but the same length as before).
 *
 *  \todo Come up with a better description of this function.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::orthogonalized() const
{
   P o;
   V x,y,z;
   get_coord_system(o,x,y,z);

   V xn(x.normalized());
   y = (y - (xn * (xn*y))).normalized() * y.length();
   V yn(y.normalized());
   z = cross(xn,yn).normalized() * z.length();

   return align_and_scale(o,x,y,z);
}

template <typename M, typename P, typename V, typename L, typename Q>
MLIB_INLINE
P
mlib::operator*(const Mat4<M,P,V,L,Q> &m, const P& p)
{  
   Vec4 p4(p, 1.0);
   if (m.is_perspective()) {
      double hcoord = m(3,0)*p[0] + m(3,1)*p[1]
                    + m(3,2)*p[2] + m(3,3);
      if (hcoord != 0.0) {
         hcoord = 1.0/hcoord;
      } else {
         cerr << "Bad homogeneous coordinate - divide by zero" << endl;
         // isError = true;
      }
      //return P(t(0,0)*p[0] + t(0,1)*p[1] + t(0,2)*p[2] + t(0,3),
      //         t(1,0)*p[0] + t(1,1)*p[1] + t(1,2)*p[2] + t(1,3),
      //         t(2,0)*p[0] + t(2,1)*p[1] + t(2,2)*p[2] + t(2,3))
      //         * hcoord;
      return P(m[0] * p4 * hcoord, m[1] * p4 * hcoord, m[2] * p4 * hcoord);
   } else {
      return P(m[0] * p4, m[1] * p4, m[2] * p4);
   }
}


template <typename M, typename P, typename V, typename L, typename Q>
MLIB_INLINE
V
mlib::operator*(const Mat4<M,P,V,L,Q> &m, const Vec3<V>& v)
{     
   Vec4 v4(v, 0.0);
   return V(m[0] * v4, m[1] * v4, m[2] * v4);
}


template <typename M, typename P, typename V, typename L, typename Q>
MLIB_INLINE
L
mlib::operator*(const Mat4<M,P,V,L,Q> &m, const Line<L,P,V>& l)
{
   return L(m * l.point(), m * l.vector());
}


template <typename M, typename P, typename V, typename L, typename Q>
MLIB_INLINE
bool
mlib::Mat4<M,P,V,L,Q>::is_identity() const
{
   static const double EPS = gEpsAbsMath*1e2;
   return fabs(row[0][0]-1)<EPS && fabs(row[1][1]-1)<EPS && fabs(row[2][2]-1)<EPS&&
          fabs(row[3][3]-1)<EPS && fabs(row[0][1])<EPS && fabs(row[0][2])<EPS &&
          fabs(row[0][3])<EPS && fabs(row[1][0])<EPS && fabs(row[1][2])<EPS &&
          fabs(row[1][3])<EPS && fabs(row[2][0])<EPS && fabs(row[2][1])<EPS &&
          fabs(row[2][3])<EPS && fabs(row[3][0])<EPS && fabs(row[3][1])<EPS &&
          (row[3][2]==0);
}


template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
bool 
mlib::Mat4<M,P,V,L,Q>::is_orthogonal() const
{
   P  origin;
   V x;
   V y;
   V z;

   get_coord_system(origin, x, y, z);

   return x.is_perpend(y) && y.is_perpend(z) && z.is_perpend(x);
}

template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
bool 
mlib::Mat4<M,P,V,L,Q>::is_equal_scaling_orthogonal() const
{
   if (!is_orthogonal())
      return false;

   V s = get_scale();

   return fabs(s[0]/s[1] - 1) < epsNorMath() && 
          fabs(s[1]/s[2] - 1) < epsNorMath() &&
          fabs(s[2]/s[0] - 1) < epsNorMath();
}


template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::transpose() const
{
   M  ret;

   for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
         ret(i,j) = row[j][i];

   return ret;
}

/*!
 *  \libgfx This function was taken directly from libgfx (it was only modified
 *  to make it into a member function).
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::adjoint() const
{
    M A;

    A[0] = -cross( row[1], row[2], row[3]);
    A[1] = -cross(-row[0], row[2], row[3]);
    A[2] = -cross( row[0], row[1], row[3]);
    A[3] = -cross(-row[0], row[1], row[2]);
        
    return A;
}

/*!
 *  As align(src1, src2, src3, dst1, dst2, dst3), but src3 and dst3 are equal
 *  and computed to be cross(src2, dst2).  If cross(src2, dst2) is the null
 *  vector, then src3 and dst3 are perpendicular to src2 (computed using an
 *  arbitrary axis algorithm) is src2 and dst2 point away from each other.
 *  Otherwise, a translation matrix from sr1 to dst1 is returned.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::align(
   const P&  src1,
   const V&  src2,
   const P&  dst1,
   const V&  dst2
   )
{
   V planeNormal = cross(src2, dst2);

   if (planeNormal.is_null()) {
      if (src2 * dst2 < 0) {
         planeNormal = src2.perpend();
      } else {
         return translation(dst1 - src1);
      }
   }

   return align(src1, src2, planeNormal, dst1, dst2, planeNormal);
}



/*!
 *  The transformation maps points and vectors as follows:
 *  
 *  <tt>
 *  point src1            --> point  dst1
 *  vec src2              --> vector dst2
 *  plane(src1,src2,src3) --> plane(dst1,dst2,dst3)
 *  </tt>
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::align(
   const P& src1,
   const V& src2,
   const V& src3,
   const P& dst1,
   const V& dst2,
   const V& dst3
   )
{
    // isError = false;

   const M t1 = M(src1, src2, src3);

//    checkError(!isError, eSourceArgumentsAreColinearOrCoincident);

   const M t2 = M(dst1, dst2, dst3);

//    checkError(!isError, eDestinationArgumentsAreColinearOrCoincident);

   return t2 * t1.inverse();
}

/*!
 *  The transformation maps points as follows:
 *  
 *  point  src1             maps to point  dst1
 *  vec    (src1,src2)      maps to vector (dst1,dst2)
 *  plane  (src1,src2,src3) maps to plane  (dst1,dst2,dst3)
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M 
mlib::Mat4<M,P,V,L,Q>::align(
   const P& src1,
   const P& src2,
   const P& src3,
   const P& dst1,
   const P& dst2,
   const P& dst3
   )
{
   return align(src1, src2-src1, src3-src1, dst1, dst2-dst1, dst3-dst1);
}

/*!
 *  Construct a rigid motion transformation from given coordinate
 *  system (origin,xx,yy,zz). This transformation will
 *  transform points from (origin,xx,yy,zz) to world.
 *  The axes xx, yy, and zz are not normalized before the matrix is constructed.
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::align_and_scale(
   const P& origin,
   const V& xx,
   const V& yy,
   const V& zz
   )
{
   assert(!xx.is_null());
   assert(!yy.is_null());
   assert(!zz.is_null());

   M t;

   for (int i = 0; i < 3; i++)       {
      t(i,0) = xx    [i];
      t(i,1) = yy    [i];
      t(i,2) = zz    [i];
      t(i,3) = origin[i];
      t(3,i) = 0.0;
   }
   t(3,3) = 1.0;

   return t;
}

/*!
 *  \libgfx This function was take directly from libgfx (it was only
 *  modified to make it into a static member function).
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::glu_perspective(double fovy, double aspect, 
                                       double zmin, double zmax)
{
   double A, B;
   M t;
   
   if( zmax==0.0 ){
      A = B = 1.0;
   } else {
      A = (zmax+zmin)/(zmin-zmax);
      B = (2*zmax*zmin)/(zmin-zmax);
   }
   
   double f = 1.0/tan(fovy*M_PI/180.0/2.0);
   t(0,0) = f/aspect;
   t(1,1) = f;
   t(2,2) = A;
   t(2,3) = B;
   t(3,2) = -1;
   t(3,3) = 0;
   
   t.perspective = true;
   
   return t;
}

/*!
 *  \libgfx This function was take directly from libgfx (it was only
 *  modified to make it into a static member function).
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::glu_lookat(const V& from, const V& at, const V& v_up)
{
    V up = v_up.normalized();
    V f = (at - from).normalized();

    // NOTE: Normalization in these two steps is left out of the GL man page!!
    V s = cross(f, up).normalized();
    V u = cross(s, f).normalized();

    M t(Vec4(s, 0), Vec4(u, 0), Vec4(-f, 0), Vec4(0, 0, 0, 1));

    return t * translation(-from);
}

/*!
 *  \libgfx This function was take directly from libgfx (it was only
 *  modified to make it into a static member function).
 *
 */
template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
M
mlib::Mat4<M,P,V,L,Q>::gl_viewport(double w, double h)
{
    return scaling(V(w/2.0, -h/2.0, 1)) * translation(V(1, -1, 0));
}

template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
bool 
mlib::Mat4<M,P,V,L,Q>::is_valid() const
{
   const V x = V(row[0][0], row[1][0], row[2][0]).normalized();
   const V y = V(row[0][1], row[1][1], row[2][1]).normalized();
   const V z = V(row[0][2], row[1][2], row[2][2]).normalized();

   return (fabs(mlib::det(x, y, z)) > epsNorMath());
}




template <typename M,typename P, typename V, typename L, typename Q>
MLIB_INLINE
ostream &
mlib::operator << (ostream &os, const Mat4<M,P,V,L,Q> &x) 
{
   os<<"["<<
       "["<<x(0,0)<<","<<x(0,1)<<","<<x(0,2)<<","<<x(0,3)<<"]"<<endl<<
       "["<<x(1,0)<<","<<x(1,1)<<","<<x(1,2)<<","<<x(1,3)<<"]"<<endl<<
       "["<<x(2,0)<<","<<x(2,1)<<","<<x(2,2)<<","<<x(2,3)<<"]"<<endl<<
       "["<<x(3,0)<<","<<x(3,1)<<","<<x(3,2)<<","<<x(3,3)<<"]"<<endl<<
       "]"<<endl;
   return os; 
}
