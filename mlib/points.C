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
 *  \file points.C
 *  \brief Contains the definitions of various static constants for the
 *  coordinate system classes.  Also contains the implementation of various
 *  non-inlined functions for these classes.
 *
 */

#include "mlib/points.H"

using namespace mlib;

CWtransf mlib::Identity;

CWvec mlib::Wvec::_null_vec(0,0,0);
CWvec mlib::Wvec::_x_axis(1,0,0);
CWvec mlib::Wvec::_y_axis(0,1,0);
CWvec mlib::Wvec::_z_axis(0,0,1);

CWpt  mlib::Wpt::_origin;

CXYvec mlib::XYvec::_null_vec(0,0);
CXYvec mlib::XYvec::_x_axis(1,0);
CXYvec mlib::XYvec::_y_axis(0,1);

CXYpt mlib::XYpt::_origin;

CNDCZvec mlib::NDCZvec::_null_vec(0,0,0);
CNDCZvec mlib::NDCZvec::_x_axis(1,0,0);
CNDCZvec mlib::NDCZvec::_y_axis(0,1,0);
CNDCZvec mlib::NDCZvec::_z_axis(0,0,1);

CNDCZpt mlib::NDCZpt::_origin;

CNDCvec mlib::NDCvec::_null_vec(0,0);
CNDCvec mlib::NDCvec::_x_axis(1,0);
CNDCvec mlib::NDCvec::_y_axis(0,1);

CNDCpt mlib::NDCpt::_origin;

CVEXEL mlib::VEXEL::_null_vec(0,0);
CVEXEL mlib::VEXEL::_x_axis(1,0);
CVEXEL mlib::VEXEL::_y_axis(0,1);

CPIXEL mlib::PIXEL::_origin;

CUVvec mlib::UVvec::_null_vec(0,0);
CUVvec mlib::UVvec::_u_dir(1,0);
CUVvec mlib::UVvec::_v_dir(0,1);

CUVpt mlib::UVpt::_origin;

// -------- inlined conversion between coordinates ------ 

/* World */

mlib::Wpt::Wpt(
   CXYpt &x, 
   CWpt  &pt
   ) 
{
   *this = XYtoW_1(x, pt);
}

mlib::Wpt::Wpt(
   CXYpt  &x,
   double  d
   )
{
   *this = XYtoW_2(x, d);
}

mlib::Wpt::Wpt(CNDCZpt &p)
{ 
   Wpt ndc;
   ndc[0] = p[0];
   ndc[1] = p[1];
   ndc[2] = p[2];

   *this = VIEW_NDC_TRANS().inverse() * ndc;
}

mlib::Wpt::Wpt(CXYpt &x)
{
   *this = XYtoW_3(x);
}

mlib::Wvec::Wvec(CNDCZvec& v, CWtransf& obj_to_ndc_der_inv)
{
   // converts an ndcz vector to 
   // wvec using the supplied matrix
   // obj_to_ndc_der_inv = obj_to_ndc.derivative().inverse, where
   // obj_to_ndc is the object to ndcz space transform

   Wvec temp(v[0],v[1],v[2]); 
   *this = obj_to_ndc_der_inv * temp;

}

mlib::Wvec::Wvec(CXYpt &x)
{
   *this = XYtoWvec(x);
}

bool 
mlib::Wpt::in_frustum() const
{
   return NDCZpt(*this).in_frustum();
}

mlib::Wvec::Wvec(CWpt &p, CVEXEL &v)
{
   Wpt  p2(PIXEL(p)+v, p);
   *this = p2 - p;
}

/* XY */

mlib::XYpt::XYpt(CWpt &w) 
{
   *this = WtoXY(w);
}

mlib::XYpt::XYpt(CNDCpt &n) 
{
   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x =   n[0] / aspect;
      _y =   n[1];
   } else {
      _x =   n[0];
      _y =   n[1] * aspect;
   }
}

mlib::XYvec::XYvec(CNDCvec &n) 
{
   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x =   n[0] / aspect;
      _y =   n[1];
   } else {
      _x =   n[0];
      _y =   n[1] * aspect;
   }
}

mlib::XYpt::XYpt(CPIXEL &p) 
{
   NDCpt  corner;
   int    w, h; VIEW_SIZE  (w, h);
   double zoom; VIEW_PIXELS(zoom, corner);
   XYpt   botleft(corner);

   _x = 2 * (p[0]/w -1); // first map pixel to 
   _y = 2 * (p[1]/h -1); // (-1,1) range;
   // next we scale pt to the current film plane zoom range
   // and offset it by the current film plane offset.
   // (NOTE: we assume that the default camera displays 
   //        2.0 coordinate units ranging from -1,1.
   //        Thus, the zoom factor is 2.0/max(hei,wid).)
   _x = (1/zoom) *_x - botleft[0];
   _y = (1/zoom) *_y - botleft[1];
}

mlib::XYvec::XYvec(CVEXEL &v) 
{
   int w, h;
   VIEW_SIZE(w, h);
   _x = 2*v[0]/w;
   _y = 2*v[1]/h;
}

/* NDC */

bool
mlib::NDCZpt::in_frustum() const
{
   // z-coord has to be in [0,1]:
   // XXX - Actually, not sure if this accounts for the
   //       clipping planes.
  if (!in_interval(_z, 0.0, 1.0)) { 
    return 0;
  }
  
  // double a = VIEW_ASPECT(); // swine!

  int w,h; VIEW_SIZE(w,h);
  double a = (double)h/(double)w;
   if (a > 1) {
      // Tall window
      return  (in_interval(_y, -a, a) && in_interval(_x, -1.0, 1.0));
   } else {
      // Wide window
      assert(a > 1e-6);
      a = 1/a;
      return (in_interval(_x, -a, a) && in_interval(_y, -1.0, 1.0));
      
   }
}


mlib::NDCpt::NDCpt(CXYpt &x) 
{
   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x =   x[0] * aspect;
      _y =   x[1];
   } else {
      assert(aspect > 1e-6);
      _x =   x[0];
      _y =   x[1] / aspect;
   }
}

mlib::NDCvec::NDCvec(CXYvec &x) 
{
   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x =   x[0] * aspect;
      _y =   x[1];
   } else {
      _x =   x[0];
      _y =   x[1] / aspect;
   }
}

mlib::NDCpt::NDCpt(CWpt &w) 
{
   *this = NDCZpt(w);
}

mlib::NDCvec::NDCvec(CVEXEL &v) 
{
   int w, h;
   VIEW_SIZE(w, h);

   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x = 2*v[0]*aspect/w;
      _y = 2*v[1]/h;
   } else {
      _x = 2*v[0]/w;
      _y = 2*v[1]/(aspect*h);
   }
}

/* NDCZ */

mlib::NDCZpt::NDCZpt(CWpt &p)
{
   Wpt ndc = VIEW_NDC_TRANS() * p;

   _x = ndc[0];
   _y = ndc[1];
   _z = ndc[2];
}

mlib::NDCZpt::NDCZpt(CWpt& o, CWtransf& PM)
{
   // converts an object space point to 
   // NDCZ using the supplied matrix
   // PM = P * M, where
   //
   //   P is the camera NDC projection matrix,
   //   M is the object to world transform.

   Wpt ndc = PM * o;

   _x = ndc[0];
   _y = ndc[1];
   _z = ndc[2];
}

mlib::NDCZpt::NDCZpt(CXYpt &x) 
{
   _z = 0;
   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x = x[0]*aspect;
      _y = x[1];
   } else {
      _x = x[0];
      _y = x[1]/aspect;
   }
}

mlib::NDCZpt::NDCZpt(CNDCpt &p)
{
  _x = p[0];
  _y = p[1];
  _z = 0;
}

mlib::NDCZpt::NDCZpt(CPIXEL &p) 
{
   int w, h;
   _z = 0;
   VIEW_SIZE(w, h);
   double aspect = VIEW_ASPECT();
   if (aspect > 1 ) {
      _x = (2*p[0]/w - 1) * aspect;
      _y = (2*p[1]/h - 1);
   } else {
      _x = (2*p[0]/w - 1);
      _y = (2*p[1]/h - 1) / aspect;
   }
}

mlib::NDCZvec::NDCZvec(CNDCvec &v)
{
  _x = v[0];
  _y = v[1];
  _z = 0;
}

mlib::NDCZvec::NDCZvec(CXYvec &x) 
{
   _z = 0;
   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x = x[0]*aspect;
      _y = x[1];
   } else {
      _x = x[0];
      _y = x[1]/aspect;
   }
}

mlib::NDCZvec::NDCZvec(CVEXEL &v) 
{
   int w, h;
   _z = 0;
   VIEW_SIZE(w, h);
   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x = 2*v[0]*aspect/w;
      _y = 2*v[1]/h;
   } else {
      _x = 2*v[0]/w;
      _y = 2*v[1]/(aspect*h);
   }
}

mlib::NDCZvec::NDCZvec(CWvec&v, CWtransf& obj_to_ndc_der)
{
   // converts an object space vector to 
   // NDCZ using the supplied matrix
   // obj_to_ndc_der = obj_to_ndc.derivative(), where
   // obj_to_ndc is the object to ndcz space transform

   Wvec temp = obj_to_ndc_der * v;
   _x = temp[0]; _y = temp[1]; _z = temp[2];
}

NDCZvec
mlib::normal_to_ndcz(CWpt& p, CWvec& world_normal)
{
   Wvec temp = VIEW_NDC_TRANS().derivative(p) * world_normal;
   return NDCZvec(temp[0], temp[1], temp[2]);
}

/* NDCZpt_list */

NDCZvec
mlib::NDCZpt_list::tan(int i) const 
{
   NDCZpt *ar = _array;
   const int n=num()-1;
   if (i<0 || i>n || n<1) return NDCZvec();
   if (i==0) return (ar[1]-ar[  0]).normalized();
   if (i==n) return (ar[n]-ar[n-1]).normalized();
   return ((ar[i+1]-ar[i]).normalized() + (ar[i]-ar[i-1]).normalized()).normalized();
}


NDCZpt       
mlib::NDCZpt_list::interpolate(double s, NDCZvec *tan, int*segp, double*tp) const
{
   int seg;
   double t;
   interpolate_length(s, seg, t);
   const NDCZvec v = _array[seg+1]-_array[seg];
   if (tan)  *tan  = v.normalized();
   if (segp) *segp = seg;
   if (tp)   *tp   = t;

   return _array[seg]+v*t;
}

void         
mlib::NDCZpt_list::interpolate_length(double s, int &seg, double &t) const
{
   // Assume update_length() has been called.  Note if
   // there are 0 or 1 points, _length will be 0, we
   // return early.  Note also, generally returns 0<=t<1,
   // except at the extreme right, returns t=1;

   // It turns out that when we ASSUME we make an ass of
   // everybody.  Not updating the partial length array
   // remains a common mistake... so we check for it
   // here. This check will not catch all cases where the
   // partial length array is out of date, but it will catch
   // many of them
   if (_partial_length.num() != _num) {
      cerr << "NDCZpt_list::interpolate_length: "
           << "Warning: partial lengths are out of date."
           << endl;
      // You know you're bad when you cast away the const:
      ((NDCZpt_list*)this)->update_length();
   }

   const double val = s*length();
   if (val <= 0) { 
      seg = 0; 
      t   = 0; 
      return; 
   }
   if (val >= length()) { 
      seg = _num-2; 
      t = 1; 
      return; 
   }

   int l=0, r=_num-1, m;
   while ((m = (l+r) >> 1) != l) {
      if (val < _partial_length[m])       
         r = m;
      else l = m;
   }
   seg = m;
   t = (val-_partial_length[m])/(_partial_length[m+1]-_partial_length[m]);
}

NDCpt_list& 
mlib::NDCpt_list::operator=(CPIXEL_list& P) 
{
   clear();
   for (int i=0; i<P.num(); i++)
      add(NDCpt(P[i]));
   update_length();
   return *this;
}

NDCpt_list& 
mlib::NDCpt_list::operator=(CXYpt_list& X) 
{
   clear();
   for (int i=0; i<X.num(); i++)
      add(NDCpt(X[i]));
   update_length();
   return *this;
}

NDCpt_list& 
mlib::NDCpt_list::operator=(CNDCZpt_list& N) 
{
   clear();
   for (int i=0; i<N.num(); i++)
      add(NDCpt(N[i]));
   update_length();
   return *this;
}

/* Pixel */
mlib::VEXEL::VEXEL(CNDCvec &n) 
{
   int w, h;
   VIEW_SIZE(w, h);
   double aspect = VIEW_ASPECT();
   if (aspect > 1) {
      _x = n[0]/(aspect/w)/2;
      _y = n[1]*h/2;
   } else {
      _x = n[0]*w/2;
      _y = n[1]*(aspect*h)/2;
   }
}


mlib::VEXEL::VEXEL(CWpt &wp, CWvec &wv) 
{
   PIXEL p1(wp), p2(wp + wv);
   *this = p2 - p1;
}

mlib::PIXEL::PIXEL(CXYpt &x) 
{
   NDCpt  corner;
   int    w,h;  VIEW_SIZE  (w, h);
   double zoom; VIEW_PIXELS(zoom, corner);
   XYpt   botleft(corner);

   // First map XY coordinate to (-w/2,-h/2) -> (w/2,h/2)
   _x = w/2*(x[0] + botleft[0]);
   _y = h/2*(x[1] + botleft[1]);

   // then scale by zoom factor and offset to be 0->(w,h)
   _x = _x*zoom + w;
   _y = _y*zoom + h;
}


mlib::VEXEL::VEXEL(CXYvec &x) 
{
   int w, h; VIEW_SIZE(w, h);

   _x = x[0]*w/2.;
   _y = x[1]*h/2.;
}

PIXEL_list&
mlib::PIXEL_list::operator=(CWpt_list& X) 
{
   clear();
   for (int i=0; i<X.num(); i++)
      add(PIXEL(X[i]));
   update_length();
   return *this;
}

PIXEL_list&
mlib::PIXEL_list::operator=(CXYpt_list& X) 
{
   clear();
   for (int i=0; i<X.num(); i++)
      add(PIXEL(X[i]));
   update_length();
   return *this;
}

PIXEL_list&
mlib::PIXEL_list::operator=(CNDCpt_list& N) 
{
   clear();
   for (int i=0; i<N.num(); i++)
      add(PIXEL(N[i]));
   update_length();
   return *this;
}

PIXEL_list&
mlib::PIXEL_list::operator=(CNDCZpt_list& N) 
{
   clear();
   for (int i=0; i<N.num(); i++)
      add(PIXEL(N[i]));
   update_length();
   return *this;
}

bool 
mlib::Wpt_list::project(XYpt_list& ret) const
{
   // Project to an XYpt_list, provided all points lie in the
   // view frustum. Assumes Wpts are not in "object space."

   // Clear it
   ret.clear();

   // If there are no points,
   // there's no point in continuing...
   if (empty())
      return 1; // "success"

   // Resize the output list now (Note: _num != 0):
   ret.realloc(_num);

   // Fill it up w/ projected points, 
   // ensuring all points lie in the frustum:
   for (int k=0; k<_num; k++) {
      NDCZpt ndcz = NDCZpt(_array[k]);
      if (!ndcz.in_frustum()) {
         ret.clear();
         return 0;      // failure
      }
      ret.add(NDCpt(ndcz));
   }
   return 1;    // success
}

bool 
mlib::Wpt_list::project(PIXEL_list& ret) const
{
   // Project to a PIXEL_list, provided all points lie in the
   // view frustum. Assumes Wpts are not in "object space."

   XYpt_list xy_list;
   if (!project(xy_list))
      return 0;
   ret = xy_list;
   return 1;
}

int
mlib::Wpt_list::closest_vertex(CPIXEL& p) const
{
   // Returns index of the *vertex* of the polyline
   // that is closest to the given screen-space point
   // (distance measured in PIXELs). Skips vertices
   // that lie outside the view frustum. Returns -1 if
   // no vertices lie in the frustum.

   int ret = -1; // return value: index of closest vertex
   double min_d=0;
   for (int i=0; i<_num; i++) {
      // skip points not in the view frustum
      if (_array[i].in_frustum()) {
         double d = PIXEL(_array[i]).dist(p);
         if (ret < 0 || d < min_d) {
            min_d = d;
            ret = i;
         }
      }
   }
   return ret;
}

bool 
mlib::Wpt_list::get_best_fit_plane(Wplane& P) const
{
   // Return the best-fit plane.
   // 
   //    Newell's algorithm,
   //    from Graphics Gems III,
   //    Vol. 5, pp 231-232

   // If less than 3 points let the caller deal with it:
   if (_num < 3)
      return 0;

   // Find vector N, normal of "best-fit" plane
   Wvec N;
   for (int k=0; k<_num; k++) {
      Wpt i = _array[k];
      Wpt j = _array[(k+1)%_num];
      N += Wvec(
         (i[1] - j[1])*(i[2] + j[2]),
         (i[2] - j[2])*(i[0] + j[0]),
         (i[0] - j[0])*(i[1] + j[1])
         );
   }
   N = N.normalized();

   // If it's not working out let the caller deal w/ it:
   if (N.is_null())
      return 0;

   // The best-fit plane:
   P = Wplane(average(), N);

   return 1;
}


bool
mlib::Wpt_list::get_plane(Wplane &P, double len_scale) const
{
   // Return the best-fit plane if the fit is good.
   // Parameter 'len_scale' is multiplied by the length
   // of the polyline to yield a threshold. The fit is
   // considered good if every point lies within the
   // threshold distance from the plane.

   P = Wplane(); // clear it

   // Try to compute the best-fit plane
   Wplane ret;
   if (!get_best_fit_plane(ret))
      return 0;

   // We now return 'true' if the points lie close to the plane.
   //
   // "Close" is defined relative to the length of the
   // polyline. We need the partial lengths to be updated.
   if (_num != _partial_length.num()) {
      cerr << "Wpt_list::get_plane: Error: lengths are not updated" << endl;
      return 0;
   }
   double thresh = length()*len_scale;
   for (int k=0; k<_num; k++)
      if (ret.dist(_array[k]) > thresh)
         return 0;

   // It's good. Now record the plane:
   P = ret;

   return 1;
}

/* end of file points.C */
