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
/*************************************************************************
 *    NAME: J.D. Northrup
 *    USER: jdn
 *    FILE: hspline.C
 *    DATE: Wed Jul 29 12:58:54 US/Eastern 1998
 *************************************************************************/
#include "hspline.H"

using mlib::CWpt;
using mlib::CWpt_list;
using mlib::Wvec;
using mlib::CWvec;

const HSpline::HermiteBasis HSpline::_HemiteBasisConst;

void
HSpline::HermiteGeometry::set(CWpt& p1, CWpt& p2, CWvec& v1, CWvec& v2)
{
   for (int i = 0; i < 3; i++) {
      row[i][0] = p1[i];
      row[i][1] = p2[i];
      row[i][2] = v1[i];
      row[i][3] = v2[i];
   }
   row[3][0] = 0;
   row[3][1] = 0;
   row[3][2] = 0;
   row[3][3] = 0;
}

void
CRSpline::clear(int all) 
{
   if (all) {
      _pts.clear();
      _u.clear();
   }
   while (!_H.empty())
      delete _H.pop();
   _valid = 0;
}

void 
CRSpline::set(CWpt_list& p, CARRAY<double> &u)
{
   clear();

   if (p.num() != u.num()) {
      err_msg("CRSpline::set: points and parameter vals don't match");
      return;
   }
   _pts = p;
   _u   = u;
}

void 
CRSpline::add(CWpt& p, double u)
{
   _pts += p;
   _u   += u;

   clear(0);
}

int
CRSpline::_update()
{
   // see Farin: curves and surfaces for CAGD,
   // 3rd ed., section 8.3 (pp 126-128)

   if (_valid)
      return 1;

   if (num() < 3) {
      err_msg("CRSpline::_update: too few points (%d)", num());
      return 0;
   }

   // first piece
   Wvec   m1 = m(1);
   double di = delt(0);
   Wvec   m0 = (_pts[1] - _pts[0])*(2/di) - m1;
   _H += new HSpline(_pts[0], _pts[1], di*m0, di*m1);

   // middle pieces
   int i;
   for (i=1; i<num()-2; i++) {
      m0 = m1;
      m1 = m(i+1);
      di = delt(i);
      _H += new HSpline(_pts[i], _pts[i+1], di*m0, di*m1);
   }

   // last piece
   m0 = m1;
   di = delt(i);
   m1 = (_pts[i+1] - _pts[i])*(2/di) - m0;
   _H += new HSpline(_pts[i], _pts[i+1], di*m0, di*m1);

   _valid = 1;
   return 1;
}

void  
CRSpline::get_seg(double u, int& seg, double& t) const
{
   // copied from _point3d_list::interpolate_length()

   assert(num() > 1);

   if (u <= _u[0]) { 
      seg = 0; 
      t   = 0; 
      return; 
   }
   if (u >= _u.last()) {
      seg = num()-2; 
      t = 1; 
      return; 
   }

   int l=0, r=num()-1, m;
   while ((m = (l+r) >> 1) != l) {
      if (u < _u[m])       
         r = m;
      else l = m;
   }
   seg = m;
   t = (u-_u[m])/delt(m);
}

/* end of file hspline.C */
