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

/****************************************************************/
/*    NAME: J.D. Northrup                                       */
/*    ACCT: jdn                                                 */
/*    FILE: hspline.H                                           */
/*    DATE: Wed Jul 29 11:48:12 1998                            */
/****************************************************************/
#ifndef __HSPLINE_HEADER
#define __HSPLINE_HEADER

#include "mlib/points.H"

/****************************************************************/
/* CLASS NAME :  HSpline                                        */
/*                                                              */
/* A Hermite spline curve class. Set the constraints, and then  */
/* query for points along the curve by calling value_at where   */
/* 0 <= t <= 1. See Sec. 11.2.1 in the Bible for more info.     */
/*                                                              */
/****************************************************************/

class HSpline {
 public:
   // need following class to be public
   // to have a static instance below

   /*******************************************************
    * HermiteBasis:
    *
    *  transpose of the matrix Mh from "Bible" sec. 11.2.1:
    *******************************************************/
   class HermiteBasis : public mlib::Wtransf {
    public:
      HermiteBasis() {
         row[0][0] =  2; row[0][1] = -3; row[0][2] = 0; row[0][3] = 1; 
         row[1][0] = -2; row[1][1] =  3; row[1][2] = 0; row[1][3] = 0; 
         row[2][0] =  1; row[2][1] = -2; row[2][2] = 1; row[2][3] = 0; 
         row[3][0] =  1; row[3][1] = -1; row[3][2] = 0; row[3][3] = 0; 
      }
   };
 protected:
   /*******************************************************
    * HermiteGeometry:
    *
    *  transpose of the matrix Gh from "Bible" sec. 11.2.1:
    *******************************************************/
   class HermiteGeometry : public mlib::Wtransf {
    public:
      HermiteGeometry() {}
      HermiteGeometry(mlib::CWpt& p1, mlib::CWpt& p2, mlib::CWvec& v1, mlib::CWvec& v2) {
         set(p1, p2, v1, v2);
      }

      void set(mlib::CWpt& p1, mlib::CWpt& p2, mlib::CWvec& v1, mlib::CWvec& v2);
   };

   // HSpline member data:
   static const HermiteBasis _HemiteBasisConst;  // const thing
   mlib::Wtransf                      _M;     // our thing

public:
   HSpline() {}
   HSpline(mlib::CWpt& p1, mlib::CWpt& p2, mlib::CWvec& v1, mlib::CWvec& v2) {
      set_constraints(p1, p2, v1, v2);
   }
   virtual ~HSpline() {}

   void set_constraints(mlib::CWpt& p1, mlib::CWpt& p2, mlib::CWvec& v1, mlib::CWvec& v2) {
      _M = HermiteGeometry(p1, p2, v1, v2) * _HemiteBasisConst;
   }
   mlib::Wpt  value_at(double t) const {
      double t2 = t*t;
      return _M * mlib::Wpt(t2*t,t2,t);
   }
   mlib::Wvec  tan_at(double t) const { return _M * mlib::Wvec(3*sqr(t), 2*t, 1); }
};

/************************************************************
 * CRSpline:
 *
 *      Catmull-Rom spline
 *
 *      interpolates given points _pts and 
 *      associated parameter vals _u
 *      strives to be C1
 *
 *      Based on:
 *        Gerald Farin: Curves and Surfaces for CAGD,
 *        3rd Ed., section 8.3 (pp 126-128)
 ************************************************************/
class CRSpline {
 protected:
   ARRAY<mlib::Wpt>           _pts;   // interpolated points
   ARRAY<double>        _u;     // parameter values
   ARRAY<HSpline*>      _H;     // splines between adjacent points
   bool                 _valid; // tells if _H is up-to-date

   int _update();
   int update() const {
      return _valid ? 1 : ((CRSpline*)this)->_update();
   }

   // for internal convenience:
   int    num()       const { return _pts.num(); }
   double delt(int i) const { return _u[i+1] - _u[i]; }
   mlib::Wvec   m(int i)    const { return (_pts[i+1]-_pts[i-1])/(_u[i+1]-_u[i-1]); }

 public:
   CRSpline() : _pts(0), _u(0), _H(0), _valid(0) {}
   ~CRSpline() { clear(); }

   void clear(int all=1);
   void set(mlib::CWpt_list& p, CARRAY<double> &u);
   void add(mlib::CWpt& p, double u);

   ////////////////////////////////////////////////////
   // not for casual users:
   // 
   //   u is the global parameter over whole spline.
   //   t is a local parameter varying from 0 to 1
   //   between a given pair of interpolated points.
   //
   // regular users only care about u
   //////////////////////////////////////////////////
   void get_seg(double u, int& seg, double& t) const;

   mlib::Wpt pt(int k, double t) const {
      return update() ? _H[k]->value_at(t) : mlib::Wpt::Origin();
   }

   mlib::Wvec tan(int k, double t) const {
      return update() ? _H[k]->tan_at(t)/delt(k) : mlib::Wvec();
   }

   ////////////////////////////////////////////////////
   // for regular users:
   // 
   //   (see above)
   //////////////////////////////////////////////////
   mlib::Wpt pt (double u) const {
      // go out of our way not to segv
      // even for short splines
      int k; double t;
      switch (num()) {
         case 0: return mlib::Wpt::Origin();
         case 1: return _pts[0];
         case 2: get_seg(u,k,t);
                 return _pts[0] + (_pts[1]-_pts[0])*t;
      }
      get_seg(u,k,t);
      return pt(k,t);
   }
   mlib::Wvec tan(double u) const {
      // go out of our way not to segv
      // even for short splines
      switch (num()) {
         case   0: 
         case   1: return mlib::Wvec();
         case   2: return (_pts[1]-_pts[0])/delt(0);
      }
      int k; double t; get_seg(u,k,t);
      return tan(k,t);
   }
};

#define CCRSpline_list const CRSpline_list
typedef ARRAY<CRSpline*> CRSpline_list;

#endif // __HSPLINE_HEADER
