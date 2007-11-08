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
#ifndef POINTS_H_IS_INCLUDED
#define POINTS_H_IS_INCLUDED

/*!
 *  \file points.H
 *  \brief Definitions of geometry classes for various coordinate systems.
 *  \ingroup group_MLIB
 *
 *  \remark This file should probably be renamed to something like
 *  coordinate_systems.H.
 *
 */

namespace mlib {

/*!
 *  \defgroup group_world_coordinate_system "World" Coordinate System
 *  \brief 3D coordinates used for "world space" and "object space".
 *  \ingroup group_MLIB
 *
 */
//@{
   
class Wpt;
class Wvec;
class Wtransf;  // 4x4 matrix
class WMat3;    // 3x3 matrix
class Wquat;    // quaternion representing a rotation
class Wplane;
class Wline;
class Wpt_list;

//@}

/*!
 *  \defgroup group_xy_coordinate_system "XY" Coordinate System
 *  \brief 2D screen coordinates running from [-1,-1] to [1,1] within window.
 *  \ingroup group_MLIB
 *
 */
//@{
   
class XYpt;
class XYvec;
class XYline;
class XYpt_list;

//@}

/*!
 *  \defgroup group_ndc_coordinate_system "NDC" Coordinate System
 *  \brief Normalized Device Coordinates.  2D screen coordinates running from
 *  -1 to 1 in shorter window dimension, and from -A to A in the longer window
 *  dimension, where A is the aspect ratio.
 *  \ingroup group_MLIB
 *
 */
//@{
   
class NDCpt;
class NDCvec;
class NDCline;
class NDCpt_list;

//@}

/*!
 *  \defgroup group_ndcz_coordinate_system "NDCZ" Coordinate System
 *  \brief Same as \ref group_ndc_coordinate_system "NDC" but with a 3rd
 *  coordinate z = depth.
 *  \ingroup group_MLIB
 *
 */
//@{

class NDCZvec;
class NDCZpt;
class NDCZpt_list;

//@}

/*!
 *  \defgroup group_pixel_coordinate_system "PIXEL" Coordinate System
 *  \brief 2D floating point screen coordinates representing pixel
 *  locations.  E.g. the lower left corner of the screen is (0,0),
 *  and the upper right is (w,h) (i.e. window width and height).
 *  \ingroup group_MLIB
 *
 */
//@{

class VEXEL;
class PIXEL;
class PIXELline;
class PIXEL_list;

//@}

/*!
 *  \defgroup group_uv_coordinate_system "UV" Coordinate System
 *  \brief 2D coordinates that can be used for texture mapping, e.g.
 *  \ingroup group_MLIB
 *
 */
//@{

class UVpt;
class UVvec;
class UVline;
class UVpt_list;

//@}

// The evil capital-C thing (shorthand for "const"):

//! \addtogroup group_world_coordinate_system
//@{

typedef const class Wvec     CWvec;
typedef const class Wpt      CWpt;
typedef const class Wpt_list CWpt_list;
typedef const class Wtransf  CWtransf;
typedef const class WMat3    CWMat3;
typedef const class Wplane   CWplane;
typedef const class Wline    CWline;
typedef const class Wquat    CWquat;

//@}

//! \addtogroup group_xy_coordinate_system
//@{

// Screen-space coordinates
// (ranges from [-1,1] in X and Y):
typedef const class XYvec               CXYvec;
typedef const class XYpt                CXYpt;
typedef const class XYpt_list           CXYpt_list;
typedef const class XYline              CXYline;

//@}

//! \addtogroup group_ndc_coordinate_system
//@{

// Aspect ratio-preserving screen-space coordinates
// (ranges from [-1,1] in shorter window dimension):
typedef const class NDCvec              CNDCvec;
typedef const class NDCpt               CNDCpt;
typedef const class NDCpt_list          CNDCpt_list;

//@}

//! \addtogroup group_ndcz_coordinate_system
//@{

// Same as NDC coordinates but with a depth coordinate:
typedef const class NDCZvec             CNDCZvec;
typedef const class NDCZvec_list        CNDCZvec_list;
typedef const class NDCZpt              CNDCZpt;
typedef const class NDCZpt_list         CNDCZpt_list;

//@}

//! \addtogroup group_pixel_coordinate_system
//@{

// PIXEL coordinates - 2D floating point coordinates
// with (0,0) = lower left corner, (w, h) = upper right:
typedef const class VEXEL               CVEXEL;
typedef const class PIXEL               CPIXEL;
typedef const class PIXEL_list          CPIXEL_list;

//@}

//! \addtogroup group_uv_coordinate_system
//@{

// UV coordinates (e.g. for texture mapping):
typedef const class UVvec               CUVvec;
typedef const class UVpt                CUVpt;
typedef const class UVpt_list           CUVpt_list;
typedef const class UVline              CUVline;

//@}

} // namespace mlib

#include "mlib/global.H"
#include "mlib/point2.H"
#include "mlib/point3.H"
#include "mlib/plane.H"
#include "mlib/mat3.H"
#include "mlib/mat4.H"
#include "mlib/quat.H"
#include "mlib/nearest_pt.H"

namespace mlib {

//! \addtogroup group_world_coordinate_system
//@{

/*!
 *  \brief A vector in World coordinates.
 *
 */
class Wvec : public Vec3<Wvec> {
   
   protected:
   
      static CWvec _null_vec;
      static CWvec _x_axis;
      static CWvec _y_axis;
      static CWvec _z_axis;

   public:

      //! \name Constructors
      //@{
         
      Wvec() {}
      Wvec(CXYpt&);
      Wvec(CNDCZvec& , CWtransf& );
      Wvec(double x, double y, double z) : Vec3<Wvec>(x,y,z) {}
      Wvec(CWpt& , CVEXEL&);
      
      //@}
      
      static CWvec& null() { return Wvec::_null_vec; }
      static CWvec&    X() { return Wvec::_x_axis; }
      static CWvec&    Y() { return Wvec::_y_axis; }
      static CWvec&    Z() { return Wvec::_z_axis; }
      
};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
// Should be able to define this once for Vec3, but that
// doesn't work (then again, we have to do this because of
// template problems in the first place)
inline Wvec operator *(double s, const Wvec& p) { return p * s;}
#endif

/*!
 *  \brief A point in World coordinates.
 *
 */
class Wpt : public Point3<Wpt, Wvec> {
   
   protected:
   
      static CWpt _origin;

   public:
   
      //! \name Constructors
      //@{
   
      Wpt() { }
      explicit Wpt(double s) : Point3<Wpt,Wvec>(s) { }
      Wpt(double x, double y, double z):Point3<Wpt,Wvec>(x,y,z) { }
      Wpt(CXYpt& );
      Wpt(CNDCZpt& );
      Wpt(CXYpt& , double d);
      Wpt(CXYpt& , CWpt& );
   
      inline Wpt(CWtransf& );
      inline Wpt(CWline& , CWline& );
      inline Wpt(CWline& , CWpt  & );
      inline Wpt(CWplane&, CWline& );
      
      //@}
      
      //! \name Misc. Functions
      //@{
   
      bool in_frustum() const;
      
      //@}
   
      static CWpt& Origin() { return Wpt::_origin; }
      
};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline Wpt operator *(double s, const Wpt& p) { return p * s;}
#endif

/*!
 *  \brief A line in World coordinates.
 *
 */
class Wline : public Line<Wline, Wpt, Wvec> {
   
   public:
   
      //! \name Constructors
      //@{
   
      Wline() { }
      Wline(CWpt& p,  CWvec& v) : Line<Wline,Wpt,Wvec>(p,v)  {}
      Wline(CWpt& p1, CWpt & p2): Line<Wline,Wpt,Wvec>(p1,p2) {}
      Wline(CXYpt&x)            : Line<Wline,Wpt,Wvec>(Wpt(x),Wvec(x)) {}
      
      //@}
   
};

/*!
 *  \brief A tranform (or matrix) in World coordinates.
 *
 *  4x4 matrix for transforming Wpts and Wvecs
 *
 */
class Wtransf : public Mat4<Wtransf,Wpt,Wvec,Wline,Wquat> {
   
   public:
   
      //! \name Constructors
      //@{
   
      Wtransf()   { }
      Wtransf(Vec4 row0, Vec4 row1, Vec4 row2, Vec4 row3, bool perspec = false)
         : Mat4<Wtransf,Wpt,Wvec,Wline,Wquat>(row0, row1, row2, row3, perspec) { }
      Wtransf(CWpt  &   origin, 
              CWvec &     xDir, 
              CWvec &     yDir, 
              CWvec &     zDir) 
              : Mat4<Wtransf,Wpt,Wvec,Wline,Wquat>(origin,xDir,yDir,zDir) { }
      Wtransf(CWvec &     col0, 
              CWvec &     col1, 
              CWvec &     col2) 
              : Mat4<Wtransf,Wpt,Wvec,Wline,Wquat>(col0,col1,col2) { }
      Wtransf(CWpt  &   origin, 
              CWvec &     xDir, 
              CWvec &     yDir) 
              : Mat4<Wtransf,Wpt,Wvec,Wline,Wquat>(origin,xDir,yDir) { }
      Wtransf(CWline&   axis)   : Mat4<Wtransf,Wpt,Wvec,Wline,Wquat>(axis) { }
      Wtransf(CWpt  &   origin) : Mat4<Wtransf,Wpt,Wvec,Wline,Wquat>(origin) { }
      
      //@}
      
};

/*!
 *  \brief A 3x3 matrix for transforming Wpts and Wvecs
 *
 */
class WMat3 : public Mat3<WMat3,Wpt,Wvec> {
 public:
   
   //! \name Constructors
   //@{
   
   WMat3() {}
   WMat3(CWvec& row0, CWvec& row1, CWvec& row2, bool rows=false) :
      Mat3<WMat3,Wpt,Wvec>(row0, row1, row2, rows) {}
   WMat3(double m00, double m01, double m02,
         double m10, double m11, double m12,
         double m20, double m21, double m22) :
      Mat3<WMat3,Wpt,Wvec>(m00,m01,m02,m10,m11,m12,m20,m21,m22) {}
   WMat3(const Mat3<WMat3,Wpt,Wvec>& m) : Mat3<WMat3,Wpt,Wvec>(m) {}

   //@}
};

/*!
 *  \brief A plane in World coordinates.
 *
 */
class Wplane : public Plane<Wplane,Wpt,Wvec,Wline> {
   
   public:
   
      //! \name Constructors
      //@{
   
      Wplane()                     { }
      Wplane(CWvec & n, double d)             : Plane<Wplane,Wpt,Wvec,Wline>(n,d) { }
      Wplane(CWpt&n, CWvec&v)                 : Plane<Wplane,Wpt,Wvec,Wline>(n,v) { }
      Wplane(CWpt&n, CWpt&p1, CWpt&p2)        : Plane<Wplane,Wpt,Wvec,Wline>(n,p1,p2) { }
      Wplane(CWpt&p, CWvec&v1, CWvec&v2)      : Plane<Wplane,Wpt,Wvec,Wline>(p,v1,v2) { }
      Wplane(CWpt plg[], int n)               : Plane<Wplane,Wpt,Wvec,Wline>(plg,n) { }
      Wplane(CWpt plg[], int n, CWvec& normal): Plane<Wplane,Wpt,Wvec,Wline>(plg,n,normal) { }
      
      //@}
   
};

inline Wpt::Wpt(CWtransf& t)           { (*this) = t.origin(); }
inline Wpt::Wpt(CWline & a, CWline& b) { (*this) = a.intersect(b); }
inline Wpt::Wpt(CWline & a, CWpt  & b) { (*this) = a.project(b); }
inline Wpt::Wpt(CWplane& a, CWline& b) { (*this) = a.intersect(b); }

/*!
 *  \brief A list of points in World coordinates.
 *
 */
class Wpt_list : public Point3list<Wpt_list,Wtransf,Wpt,Wvec,Wline> {
   
   public:
   
      //! \name Constructors
      //@{
   
      Wpt_list(int max=0)
         : Point3list<Wpt_list,Wtransf,Wpt,Wvec,Wline>(max) {}
      
      Wpt_list(const Point3list<Wpt_list,Wtransf,Wpt,Wvec,Wline>& p)
         : Point3list<Wpt_list,Wtransf,Wpt,Wvec,Wline>(p)
      {
         // XXX - base class Point3list updates length, but
         //       the compiler (linux anyway) ignores it.
         //       So now we have to do it again (?):
         update_length();
      }
      
      Wpt_list(const Point3list<Wpt_list,Wtransf,Wpt,Wvec,Wline>& p, CWtransf& t)
         : Point3list<Wpt_list,Wtransf,Wpt,Wvec,Wline>(p,t)
      {
         // XXX - stupid compiler?
         update_length();
      }
      
      //@}
      
      //! \name Projection Functions
      //@{
   
      //! \brief Project to screen coords, provided all Wpts lie in
      //! the view frustum.
      bool project(XYpt_list&  ret) const;
      //! \brief Project to screen coords, provided all Wpts lie in
      //! the view frustum.
      bool project(PIXEL_list& ret) const;
      
      //@}
      
      //! \name Nearest Point Functions
      //@{
   
      //! \brief Returns index of the *vertex* of the polyline
      //! that is closest to the given screen-space point
      //! (distance measured in PIXELs). Skips vertices
      //! that lie outside the view frustum. Returns -1 if
      //! no vertices lie in the frustum.
      int closest_vertex(CPIXEL& p) const;
      
      //@}
      
      //! \name Best-Fit Plane Functions
      //@{
   
      //! \brief Return the best-fit plane.
      bool get_best_fit_plane(Wplane& P) const;
      
      //! \brief Return the best-fit plane if the fit is good.  Parameter
      //! 'len_scale' is multiplied by the length of the polyline
      //! to yield a threshold. The fit is considered good if
      //! every point lies within the threshold distance from the
      //! plane.
      bool get_plane(Wplane& P, double len_scale=1e-3) const;
   
      //! \brief Return true if get_plane succeeds with the given threshold.
      bool is_planar(double len_scale=1e-3) const {
         Wplane P;
         get_plane(P, len_scale);
         return P.is_valid();
      }
   
      //! \brief Convenience: compute the best-fit plane and return its normal.
      bool get_plane_normal(Wvec& n) {
         Wplane P;
         if (!get_plane(P))
            return false;
         n = P.normal();
         return true;   
      }
      
      //@}
      
};

//! \name Overloaded Arithmetic Operators
//@{

//! \brief Multiplies a transformation matrix by every point in the list.
//! \relates Wpt_list
inline Wpt_list 
operator *(CWtransf& x, CWpt_list& pts) {
   Wpt_list ret;
   for (int i=0; i<pts.num(); i++)
      ret += (x * pts[i]);
   return ret;
}

//! \brief Multiplies all the points in the list by a scalar constant.
//! \relates Wpt_list
inline Wpt_list
operator *(CWpt_list& pts, double t) {
   Wpt_list ret;
   for (int i = 0; i < pts.num(); i++)
      ret += (pts[i]*t);
   return ret;
}

//! \brief Adds all the points in one list to the corresponding points in another
//! list.  The two lists must contain the same number of points.
//! \relates Wpt_list
inline Wpt_list
operator +(CWpt_list& pts1, CWpt_list& pts2) {
   assert(pts1.num() == pts2.num());
   Wpt_list ret;
   for (int i=0; i<pts1.num(); i++)
      ret += (pts1[i] + pts2[i]);
   return ret;
}

//@}

/*!
 *  \brief A quaternion in World coordinates.
 *
 */
class Wquat : public Quat<Wquat, Wtransf, Wpt, Wvec, Wline>
{
   
   public:
   
      //! \name Constructors
      //@{
   
      Wquat() {}
      Wquat(CWvec& v, double w)
         : Quat<Wquat, Wtransf, Wpt, Wvec, Wline>(v,w) {}
      Wquat(double s)
         : Quat<Wquat, Wtransf, Wpt, Wvec, Wline>(Wvec(),s) {}
      Wquat(CWtransf& t)
         : Quat<Wquat, Wtransf, Wpt, Wvec, Wline>(t) {}
      Wquat(CWvec& v1, CWvec& v2)
         : Quat<Wquat, Wtransf, Wpt, Wvec, Wline>(v1,v2) {}
      
      //@}

};

//@}

} // namespace mlib

namespace mlib {

//! \addtogroup group_xy_coordinate_system
//! XY coordinates : -1 to 1 normalized coordinates
//@{

/*!
 *  \brief A vector in XY coordinates.
 *
 */
class XYvec : public Vec2<XYvec> {

   protected:
   
      static CXYvec _null_vec;
      static CXYvec _x_axis;
      static CXYvec _y_axis;

   public:
   
      //! \name Constructors
      //@{
         
      XYvec() { }
      XYvec(double x, double y):Vec2<XYvec>(x,y) { }
      XYvec(CNDCvec& );
      XYvec(CVEXEL& );
      
      //@}
      
      static CXYvec& null() { return XYvec::_null_vec; }
      static CXYvec&    X() { return XYvec::_x_axis; }
      static CXYvec&    Y() { return XYvec::_y_axis; }

};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline XYvec operator *(double s, const XYvec& p) { return p * s;}
#endif

/*!
 *  \brief A point in XY coordinates.
 *
 */
class XYpt : public Point2<XYpt, XYvec> {
   
   protected:
   
      static CXYpt _origin;

   public:
   
      //! \name Constructors
      //@{
         
      XYpt() { }
      explicit XYpt(double s) : Point2<XYpt,XYvec>(s) { }
      XYpt(double x, double y):Point2<XYpt,XYvec>(x,y) { }
      XYpt(CNDCpt& );
      XYpt(CPIXEL& );
      XYpt(CWpt  & );
      inline XYpt(CXYline& , CXYline& );
      inline XYpt(CXYline& , CXYpt  & );
      
      //@}
      
      static CXYpt& Origin() { return XYpt::_origin; }
   
};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline XYpt operator *(double s, const XYpt& p) { return p * s;}
#endif

/*!
 *  \brief A line in XY coordinates.
 *
 */
class XYline : public Line<XYline, XYpt, XYvec> {
   
   public:
   
      //! \name Constructors
      //@{
   
      XYline() { }
      XYline(CXYpt& p,  CXYvec& v) : Line<XYline,XYpt,XYvec>(p,v)  {}
      XYline(CXYpt& p1, CXYpt & p2): Line<XYline,XYpt,XYvec>(p1,p2) {}
      
      //@}

};

/*!
 *  \brief A list of points in XY coordinates.
 *
 */
class XYpt_list : public Point2list<XYpt_list,XYpt,XYvec,XYline> {
   
   public:
   
      //! \name Constructors
      //@{
   
      XYpt_list(int m=0)
         : Point2list<XYpt_list,XYpt,XYvec,XYline>(m) {}
      XYpt_list(const Point2list<XYpt_list,XYpt,XYvec,XYline>& p)
         : Point2list<XYpt_list,XYpt,XYvec,XYline>(p) {}
      
      //@}
      
      //! \name Projection Functions
      //@{
      
      void project_to_plane(CWplane& P, Wpt_list& pts) const {
         pts.clear();
         if (!empty()) pts.realloc(num());
         for (int i=0; i<_num; i++)
            pts += Wpt(P, Wline(_array[i]));
         pts.update_length();
      }
      
      //@}
   
};

inline XYpt::XYpt(CXYline& a, CXYline& b) {(*this) = a.intersect(b);}
inline XYpt::XYpt(CXYline& a, CXYpt  & b) {(*this) = a.project(b);}

//@}

} // namespace mlib

namespace mlib {

//! \addtogroup group_ndcz_coordinate_system
//! NDCZ coordinates : 3D normalized device coordinates
//!
//! Normalized device coordinates that range
//! from -1 to 1 in the shorter window dimension,
//! and from -a to a in the longer dimension,
//! where 'a' is the aspect ratio, i.e. the ratio
//! of the window's longer side to its shortest one.
//! The 3rd coordinate gives 'depth', as computed
//! by the camera's projection matrix.
//@{

/*!
 *  \brief A vector in NDCZ coordinates.
 *
 */
class NDCZvec : public Vec3<NDCZvec> {
   
   protected:
   
      static CNDCZvec _null_vec;
      static CNDCZvec _x_axis;
      static CNDCZvec _y_axis;
      static CNDCZvec _z_axis;

   public:
   
      //! \name Constructors
      //@{
   
      NDCZvec() { }
      NDCZvec(double x, double y, double z):Vec3<NDCZvec>(x,y,z) { }
      NDCZvec(CNDCvec& );
      NDCZvec(CXYvec& );
      NDCZvec(CWvec&v, CWtransf& obj_to_ndc_der);
      NDCZvec(CVEXEL& );
      
      //@}
      
      //! \name Vector Operations
      //@{
      
      //! \brief Computes an arbitrary vector that is perpendicular to this one
      //! and that lies in the xy-plane.
      //! 
      //! Differs from the perpend() function becayse it only takes the x and y
      //! coordinates into account (i.e. it ignores depth).
      NDCZvec perpendicular() const { return NDCZvec(-_y, _x, 0); }
      
      //@}
      
      //! \name Vector Property Queries
      //@{
      
      //! \brief Computes the length of the vector in the xy-plane (i.e. ignores
      //! depth).
      double  planar_length() const { return sqrt(_x*_x+_y*_y); }
      
      //@}
      
      static CNDCZvec& null() { return NDCZvec::_null_vec; }
      static CNDCZvec&    X() { return NDCZvec::_x_axis; }
      static CNDCZvec&    Y() { return NDCZvec::_y_axis; }
      static CNDCZvec&    Z() { return NDCZvec::_z_axis; }

};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline NDCZvec operator *(double s, const NDCZvec& p) { return p * s;}
#endif

/*!
 *  \brief A point in NDCZ coordinates.
 *
 */
class NDCZpt : public Point3<NDCZpt, NDCZvec> {
   
   protected:
   
      static CNDCZpt _origin;

   public:
   
      //! \name Constructors
      //@{
   
      NDCZpt() { }
      explicit NDCZpt(double s) : Point3<NDCZpt,NDCZvec>(s) { }
      NDCZpt(double x, double y, double z):Point3<NDCZpt,NDCZvec>(x,y,z) { }
      NDCZpt(CWpt& w);
      NDCZpt(CWpt& w, CWtransf& PM);
      NDCZpt(CXYpt& );
      NDCZpt(CNDCpt& );
      NDCZpt(CPIXEL& );
      
      //@}
      
      //! \name Two Point Operations
      //@{
      
      //! \brief Return the distance between NDCZpts, ignoring z-coord.
      double planar_length(CNDCZpt& p) const {
         double dx = _x - p._x, dy = _y - p._y;
         return sqrt(dx*dx + dy*dy);
      }
      
      //@}
      
      //! \name Misc. Functions
      //@{
      
      //! \brief Project to an XYpt_list, provided all points lie in
      //! the view frustum. Assumes Wpts are not in "object
      //! space."
      bool in_frustum() const;
      
      //@}
      
      static CNDCZpt& Origin() { return NDCZpt::_origin; }

};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline NDCZpt operator *(double s, const NDCZpt& p) { return p * s;}
#endif

/*!
 *  \brief A list of points in NDCZ coordinates.
 *
 *  \remark This class is very similar to the Pointlist class.  However, there
 *  are some fundamental differences in how it handles distances.  Specifically,
 *  it measures distances in 2D (XY) coordinates, rather than 3D (XY + depth)
 *  coordinates.  Because NDCZpt is based on the Point3 class, the Pointlist
 *  class can't be used as a base because that would force distances to be
 *  computed in 3D rather than the desired 2D.  Hence, a good chunk of the
 *  functional from Pointlist is duplicates here.
 *
 */
class NDCZpt_list : public ARRAY<NDCZpt> {
   
   protected:
   
      ARRAY<double>    _partial_length;

   public:
   
      //! \name Constructors
      //@{
   
      NDCZpt_list(int max=0)              : ARRAY<NDCZpt>(max) {}
      NDCZpt_list(const ARRAY<NDCZpt>& p) : ARRAY<NDCZpt>(p)   {}
      
      //@}
      
      //! \name Segment Accessor Functions
      //@{
   
      //! Gets a vector pointing from the start point to the end point of the
      //! \p i<sup>th</sup> segment.
      NDCZvec segment       (int i) const { return _array[i+1]-_array[i];}
      //! Gets the length of the \p i<sup>th</sup> segment.
      double  segment_length(int i) const { return segment(i).length(); }
   
      //! Return segment \p i with 0 depth component:
      NDCZvec xy_seg(int i) const {
         NDCZvec ret = segment(i);
         ret[2] = 0;
         return ret;
      }
      double xy_len(int i) const { return xy_seg(i).length(); }
      
      //@}
      
      //! \name Interpolation Functions
      //@{
   
      //! \brief Parameter s should lie in the range [0,1]. Returns the
      //! corresponding segment index, and the paramter t varying
      //! from 0 to 1 along that segment.
      //!
      //! \note Progress along the polyline is measured just in X and Y, not Z.
      void   interpolate_length(double s, int& seg, double& t)              const;
   
      //! \brief Given interpolation parameter s varying from 0 to 1
      //! along the polyline, return the corresponding point,
      //! and optionally the tangent, segment index, and segment
      //! paramter there:
      //!
      //! \note Progress along the polyline is measured just in X and Y, not Z.
      NDCZpt interpolate(double s, NDCZvec *tan=0, int*segp=0, double*tp=0) const;
      
      //@}
      
      //! \name Length Functions
      //@{
   
      //! \brief This function must be called for interpolation routines to work.
      //! \note Policy on measuring length: we ignore the z component.
      void update_length() {
         double len = 0;
         _partial_length.clear();
         if (_num > 0)
            _partial_length.realloc(_num);
         _partial_length += 0;
         for ( int i=1; i< _num; i++ ) {
            len += xy_len(i-1);
            _partial_length += len;
         }
      }
      double partial_length(int i) const {
         return _partial_length.valid_index(i) ? _partial_length[i] : 0;
      }
   
      double length() const {
         return _partial_length.empty() ? 0 : _partial_length.last();
      }
      
      //@}
   
      //! \name Misc. Functions
      //@{
         
      NDCZvec  tan(int i)      const;
   
      NDCZpt   average () const {
         NDCZpt sum; 
         for(int i=0;i<num();i++) sum=sum+(*this)[i];
         return sum/(num() ? num():1); 
      }
      
      //@}

};

/*!
 *  \brief A list of vectors in NDCZ coordinates.
 *
 *  \question Is this class done?
 *  \question Is _partial_length even necessary?
 *
 */
class NDCZvec_list : public ARRAY<NDCZvec> {
   
   protected:
   
      ARRAY<double>    _partial_length;

   public:
   
      //! \name Constructors
      //@{
   
      NDCZvec_list(int max=0)             :ARRAY<NDCZvec>(max) { }
      NDCZvec_list(const ARRAY<NDCZvec>& p) :ARRAY<NDCZvec>(p) { }
      
      //@}
      
      //! \name Misc. Functions
      //@{
      
      NDCZvec   average () const {
         NDCZvec sum;
         for(int i=0;i<num();i++) sum=sum + (*this)[i];
         return sum/(num() ? num():1); 
      }
      
      //@}

};

NDCZvec normal_to_ndcz(CWpt& p, CWvec& world_normal);

//@}

} // namespace mlib

namespace mlib {

//! \addtogroup group_ndc_coordinate_system
//! Normalized device coordinates that range
//! from -1 to 1 in the shorter window dimension,
//! and from -a to a in the longer dimension,
//! where 'a' is the aspect ratio, i.e. the ratio
//! of the window's longer side to its shortest one.
//@{

/*!
 *  \brief A vector in NDC coordinates.
 *
 */
class NDCvec : public Vec2<NDCvec> {
   
   protected:
   
      static CNDCvec _null_vec;
      static CNDCvec _x_axis;
      static CNDCvec _y_axis;

   public:
   
      //! \name Constructors
      //@{
   
      NDCvec() { }
      NDCvec(double x, double y):Vec2<NDCvec>(x,y) { }
      NDCvec(CXYvec& );
      NDCvec(CVEXEL& );
      NDCvec(CNDCZvec& v): Vec2<NDCvec>(v[0], v[1]) { };
      
      //@}
      
      //! \name Vector Operations
      //@{
      
      //! \question Isn't this the same as the Vec2 perpend() function?
      NDCvec perpendicular() const { return NDCvec(-_y, _x); }
      
      //@}
      
      static CNDCvec& null() { return NDCvec::_null_vec; }
      static CNDCvec&    X() { return NDCvec::_x_axis; }
      static CNDCvec&    Y() { return NDCvec::_y_axis; }

};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline NDCvec operator *(double s, const NDCvec& p) { return p * s;}
#endif

/*!
 *  \brief A point in NDC coordinates.
 *
 */
class NDCpt : public Point2<NDCpt, NDCvec> {
   
   protected:
   
      static CNDCpt _origin;

   public:
   
      //! \name Constructors
      //@{
   
      NDCpt() { }
      explicit NDCpt(double s) : Point2<NDCpt,NDCvec>(s) { }
      NDCpt(double x, double y):Point2<NDCpt,NDCvec>(x,y) { }
      NDCpt(CNDCZpt& n):Point2<NDCpt,NDCvec>(n[0],n[1]) { }
      NDCpt(CWpt& w);
      NDCpt(CXYpt& );
      NDCpt(CPIXEL& p) { *this = XYpt(p); }
      
      //@}
      
      static CNDCpt& Origin() { return NDCpt::_origin; }
      
};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline NDCpt operator *(double s, const NDCpt& p) { return p * s;}
#endif

/*!
 *  \brief A line in NDC coordinates.
 *
 */
class NDCline : public Line<NDCline, NDCpt, NDCvec> {
   
   public:
   
      //! \name Constructors
      //@{
 
      NDCline() {}
      NDCline(CNDCpt& p,  CNDCvec& v) : Line<NDCline,NDCpt,NDCvec>(p,v)   {}
      NDCline(CNDCpt& p1, CNDCpt& p2) : Line<NDCline,NDCpt,NDCvec>(p1,p2) {}
      
      //@}

};

/*!
 *  \brief A list of points in NDC coordinates.
 *
 */
class NDCpt_list : public Point2list<NDCpt_list,NDCpt,NDCvec,NDCline> {
   
   public:
   
      //! \name Constructors
      //@{
   
      NDCpt_list(int m=0)
         : Point2list<NDCpt_list,NDCpt,NDCvec,NDCline>(m) {}
      NDCpt_list(const Point2list<NDCpt_list,NDCpt,NDCvec,NDCline>& N)
         : Point2list<NDCpt_list,NDCpt,NDCvec,NDCline>(N) { update_length(); }
      NDCpt_list(CXYpt_list&   X)
         : Point2list<NDCpt_list,NDCpt,NDCvec,NDCline>(0) { *this = X; }
      NDCpt_list(CNDCZpt_list& N)
         : Point2list<NDCpt_list,NDCpt,NDCvec,NDCline>(0) { *this = N; }
      NDCpt_list(CPIXEL_list&  P)
         : Point2list<NDCpt_list,NDCpt,NDCvec,NDCline>(0) { *this = P; }
         
      //@}
      
      //! \name Overloaded Assignment Operators
      //@{
      
      NDCpt_list& operator=(CXYpt_list&   X);
      NDCpt_list& operator=(CNDCZpt_list& N);
      NDCpt_list& operator=(CPIXEL_list&  P);
      
      //@}
      
      //! \name Projection Functions
      //@{
      
      void project_to_plane(CWplane& P, Wpt_list& pts) const {
         pts.clear();
         if (!empty()) pts.realloc(num());
         for (int i=0; i<_num; i++)
            pts += Wpt(P, Wline(_array[i]));
         pts.update_length();
      }
      
      //@}

};

//@}

} // namespace mlib

namespace mlib {

//! \addtogroup group_pixel_coordinate_system
//! Corresponds to GL-style pixels coords:
//! lower left corner is (0,0)
//! upper right is (width, height)
//@{

/*!
 *  \brief A vector in pixel coordinates.
 *
 */
class VEXEL : public Vec2<VEXEL> {
   
   protected:
   
      static CVEXEL _null_vec;
      static CVEXEL _x_axis;
      static CVEXEL _y_axis;

   public:
   
      //! \name Constructors
      //@{
   
      VEXEL() { }
      VEXEL(double x, double y):Vec2<VEXEL>(x,y) { }
      VEXEL(CXYvec& );
      VEXEL(CNDCvec& );
      VEXEL(CWpt& , CWvec& );
      
      //@}
      
      static CVEXEL& null() { return VEXEL::_null_vec; }
      static CVEXEL&    X() { return VEXEL::_x_axis; }
      static CVEXEL&    Y() { return VEXEL::_y_axis; }

};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline VEXEL operator *(double s, const VEXEL& p) { return p * s;}
#endif

/*!
 *  \brief A point in pixel coordinates.
 *
 */
class PIXEL : public Point2<PIXEL, VEXEL> {
   
   protected:
   
      static CPIXEL _origin;

   public:
   
      //! \name Constructors
      //@{
 
      PIXEL() { }
      explicit PIXEL(double s) : Point2<PIXEL,VEXEL>(s) { }
      PIXEL(double x, double y):Point2<PIXEL,VEXEL>(x,y) { }
      PIXEL(CXYpt& );
      PIXEL(CWpt& w) { *this = XYpt(w); }
      
      //@}
      
      static CPIXEL& Origin() { return PIXEL::_origin; }

};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline PIXEL operator *(double s, const PIXEL& p) { return p * s;}
#endif

/*!
 *  \brief A line in pixel coordinates.
 *
 */
class PIXELline : public Line<PIXELline, PIXEL, VEXEL> {
   
   public:
   
      //! \name Constructors
      //@{
   
      PIXELline() {}
      PIXELline(CPIXEL& p,  CVEXEL&  v) : Line<PIXELline,PIXEL,VEXEL>(p,v)   {}
      PIXELline(CPIXEL& p1, CPIXEL& p2) : Line<PIXELline,PIXEL,VEXEL>(p1,p2) {}
      
      //@}

};

/*!
 *  \brief A list of points in pixel coordinates.
 *
 */
class PIXEL_list : public Point2list<PIXEL_list,PIXEL,VEXEL,PIXELline> {
   
   public:
   
      //! \name Constructors
      //@{
      
      PIXEL_list(int n=0)
         : Point2list<PIXEL_list,PIXEL,VEXEL,PIXELline>(n) {}
      PIXEL_list(const Point2list<PIXEL_list,PIXEL,VEXEL,PIXELline>& P)
         : Point2list<PIXEL_list,PIXEL,VEXEL,PIXELline>(P) { update_length(); }
      PIXEL_list(CWpt_list&    W)
         : Point2list<PIXEL_list,PIXEL,VEXEL,PIXELline>(0) { *this = W; }
      PIXEL_list(CXYpt_list&   X)
         : Point2list<PIXEL_list,PIXEL,VEXEL,PIXELline>(0) { *this = X; }
      PIXEL_list(CNDCpt_list&  N)
         : Point2list<PIXEL_list,PIXEL,VEXEL,PIXELline>(0) { *this = N; }
      PIXEL_list(CNDCZpt_list& N)
         : Point2list<PIXEL_list,PIXEL,VEXEL,PIXELline>(0) { *this = N; }
         
      //@}
      
      //! \name Overloaded Assignment Operators
      //@{
      
      PIXEL_list& operator=(CWpt_list&    X);
      PIXEL_list& operator=(CXYpt_list&   X);
      PIXEL_list& operator=(CNDCpt_list&  N);
      PIXEL_list& operator=(CNDCZpt_list& N);
      
      //@}
      
      //! \name Projection Functions
      //@{
      
      void project_to_plane(CWplane& P, Wpt_list& pts) const {
         pts.clear();
         if (!empty()) pts.realloc(num());
         for (int i=0; i<_num; i++)
            pts += Wpt(P, Wline(_array[i]));
         pts.update_length();
      }
      
      //@}

};

//@}

} // namespace mlib

namespace mlib {

//! \addtogroup group_uv_coordinate_system
//! UV coordinates : -1 to 1 normalized coordinates
//@{

/*!
 *  \brief A vector in UV coordinates.
 *
 */
class UVvec : public Vec2<UVvec> {
   
   protected:
   
      static CUVvec _null_vec;
      static CUVvec _u_dir;
      static CUVvec _v_dir;

   public:
   
      //! \name Constructors
      //@{
      
      UVvec() {}
      UVvec(double u, double v) : Vec2<UVvec>(u,v) {}
      
      //@}
      
      static CUVvec& null() { return UVvec::_null_vec; }
      static CUVvec&    U() { return UVvec::_u_dir; }
      static CUVvec&    V() { return UVvec::_v_dir; }

};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline UVvec operator*(double s, const UVvec& p) { return p * s;}
#endif

/*!
 *  \brief A point in UV coordinates.
 *
 */
class UVpt : public Point2<UVpt, UVvec> {
   
   protected:
   
      static CUVpt _origin;

   public:
   
      //! \name Constructors
      //@{
      
      UVpt() {}
      explicit UVpt(double s) : Point2<UVpt,UVvec>(s) { }
      UVpt(double u, double v) : Point2<UVpt,UVvec>(u,v) {}
      inline UVpt(CUVline&, CUVline&);
      inline UVpt(CUVline&, CUVpt&);
      
      //@}
      
      static CUVpt& Origin() { return UVpt::_origin; }

};

#ifdef JOT_NEEDS_DOUBLE_STAR_EXPLICIT
inline UVpt operator *(double s, const UVpt& p) { return p * s;}
#endif

/*!
 *  \brief A line in UV coordinates.
 *
 */
class UVline : public Line<UVline, UVpt, UVvec> {
   
   public:
   
      //! \name Constructors
      //@{
   
      UVline() {}
      UVline(CUVpt& p,  CUVvec& v)  : Line<UVline,UVpt,UVvec>(p,v)   {}
      UVline(CUVpt& p1, CUVpt & p2) : Line<UVline,UVpt,UVvec>(p1,p2) {}
      
      //@}

};

/*!
 *  \brief A list of points in UV coordinates.
 *
 */
class UVpt_list : public Point2list<UVpt_list,UVpt,UVvec,UVline> {
   
   public:
   
      //! \name Constructors
      //@{
   
      UVpt_list(int m=0)
         : Point2list<UVpt_list,UVpt,UVvec,UVline>(m) {}
      UVpt_list(const Point2list<UVpt_list,UVpt,UVvec,UVline>& p)
         : Point2list<UVpt_list,UVpt,UVvec,UVline>(p) {}
      
      //@}

};

inline UVpt::UVpt(CUVline& a, CUVline& b) {(*this) = a.intersect(b);}
inline UVpt::UVpt(CUVline& a, CUVpt  & b) {(*this) = a.project(b);}

//@}

} // namespace mlib

namespace mlib {

class EDGElist;

//! \brief Shortcut for const EDGElist.
//! \relates EDGElist
typedef const EDGElist CEDGElist;

/*!
 *  \brief A list of edges in World coordinates.
 *
 *  Stores a list of vertices as points and a list of pairs of indices into the
 *  vertex list (representing edges).
 *
 */
class EDGElist {
   
   protected:
   
      Wpt_list      _verts;
      ARRAY<int>    _start;
      ARRAY<int>    _end;
      
   public:
   
      //! \name Constructors
      //@{
   
      EDGElist(CWpt_list& verts) :_verts(verts)    {}
      EDGElist() : _verts(0), _start(0), _end(0)   {}
      
      //@}
      
      //! \name Edge Accessor Functions
      //@{
      
      void add_edge(int i, int j)      { _start += i; _end += j;}
      
      void edge(int edge, Wpt& s, Wpt& e) const
         { s = _verts[_start[edge]]; e= _verts[_end[edge]]; }
      
      CWpt& operator()(int edge, int num) const 
         { return num == 0 ? _verts[_start[edge]] : _verts[_end[edge]];}
      
      void reset   (CWpt_list& verts)  { reset(); _verts = verts; }
      void reset   ()                  { _verts.clear(); _start.clear(); _end.clear();}
      
      //@}
      
      //! \name EDGElist Operations
      //@{
      
      void xform   (CWtransf& t)       { _verts.xform(t);}
      
      //@}
      
      //! \name EDGElist Property Queries
      //@{
         
      int       num     () const       { return _start.num(); }
      
      //@}

};

} // namespace mlib

namespace mlib {

//---------------------------------------------------
//  Well-known items like 4x4 identity transform.
//---------------------------------------------------
extern CWtransf Identity;

} // namespace mlib

//---------------------------------------------------
//  Conversion functions.
//---------------------------------------------------
extern mlib::Wpt  (*XYtoW_1 )(mlib::CXYpt& , mlib::CWpt& ) ;
extern mlib::Wpt  (*XYtoW_2 )(mlib::CXYpt& , double) ;
extern mlib::Wpt  (*XYtoW_3 )(mlib::CXYpt& ) ;
extern mlib::Wvec (*XYtoWvec)(mlib::CXYpt& ) ;
extern mlib::XYpt (*WtoXY   )(mlib::CWpt & ) ;
extern void (*VIEW_SIZE)(int& , int& ) ;
extern void (*VIEW_PIXELS)(double& , mlib::NDCpt& ) ;
extern double (*VIEW_ASPECT)() ;
extern mlib::CWtransf& (*VIEW_NDC_TRANS)() ;
extern mlib::CWtransf& (*VIEW_NDC_TRANS_INV)() ;
#if defined(MLIB_STANDALONE) || defined(DEV_STANDALONE)
mlib::Wpt  (*XYtoW_1 )(mlib::CXYpt& , mlib::CWpt& ) = 0;
mlib::Wpt  (*XYtoW_2 )(mlib::CXYpt& , double) = 0;
mlib::Wpt  (*XYtoW_3 )(mlib::CXYpt& ) = 0;
mlib::Wvec (*XYtoWvec)(mlib::CXYpt& ) = 0;
mlib::XYpt (*WtoXY   )(mlib::CWpt & ) = 0;
void (*VIEW_SIZE)(int& , int& ) = 0;
double (*VIEW_ASPECT)() = 0;
mlib::CWtransf& (*VIEW_NDC_TRANS)() = 0;
mlib::CWtransf& (*VIEW_NDC_TRANS_INV)() = 0;
void (*VIEW_PIXELS)(double& , mlib::NDCpt& ) = 0;
inline mlib::Wtransf RET_IDENTITY() { return mlib::Identity; }
#endif

namespace mlib {

//---------------------------------------------------
//  Convenience functions.
//---------------------------------------------------

//! \brief Given point p in world space and pixel length r,
//! return the world space length of a segment passing
//! through p, parallel to the film plane, with pixel
//! length r.
inline double
world_length(CWpt& p, double r)
{
   return 2*(p - Wpt(XYpt(PIXEL(p) + VEXEL(r/2,0)), p)).length();
}

//! \brief Same as world_length(CWpt& p, double r), but for object-space
//! point o, with transform M from object to world and its inverse I.
inline double
obj_length(CWpt& o, double r, CWtransf& M, CWtransf& I)
{
   Wpt w = M*o;
   return (o - I*Wpt(XYpt(PIXEL(w)+VEXEL(r,0)), w)).length();
}

} // namespace mlib

#endif // POINTS_H_IS_INCLUDED

/* end of file points.H */
