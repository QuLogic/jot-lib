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
 * map2d3d.C:
 *****************************************************************/
#include "map2d3d.H"
#include "std/config.H"

using namespace mlib;

/*****************************************************************
 * Map2D3D
 *****************************************************************/
UVpt 
Map2D3D::add(CUVpt& uv, CUVvec& delt) const
{
   if (!(wrap_u() || wrap_v()))
      return uv + delt;

   // XXX -
   //   implementation below expects u and v to range
   //   over [0,1]. slightly harder to fix than to add
   //   these debug lines...
   static bool debug = Config::get_var_bool("DEBUG_MAP2D3D",false,true);
   static bool warned = false;
   if (debug && !warned && !(u_min() == 0 && v_min() == 0 &&
                             u_max() == 1 && v_max() == 1)) {
      err_msg("Map2D3D::add: error: uv bounds not in [0,1] square!");
      warned = 1;
   }

   WrapCoord w;

   return UVpt(
      wrap_u() ? w.add(uv[0],delt[0]) : (uv[0]+delt[0]),
      wrap_v() ? w.add(uv[1],delt[1]) : (uv[1]+delt[1])
      );
}

UVpt
Map2D3D::avg(CUVpt& uv1, CUVpt& uv2, double weight) const
{
   // Return an average uv-coord between the given coordinates,
   // doing the right thing if the surface wraps around. The
   // weight is the fraction of the way from uv1 to uv2:

   if (!(wrap_u() || wrap_v()))
      return interp(uv1, uv2, weight);

   // XXX -
   //  initial implementation: may need to run solve
   //  to get something closer to the midpoint along the
   //  geodesic curve.

   // XXX -
   //   implementation below expects u and v to range
   //   over [0,1]. slightly harder to fix than to add
   //   these debug lines...
   static bool debug = Config::get_var_bool("DEBUG_MAP2D3D",false,true);
   static bool warned = false;
   if (debug && !warned && !(u_min() == 0 && v_min() == 0 &&
                             u_max() == 1 && v_max() == 1)) {
      err_msg("Map2D3D::avg: error: uv bounds not in [0,1] square!");
      warned = 1;
   }

   WrapCoord w;
   return UVpt(
      wrap_u() ? w.interp(uv1[0],uv2[0],weight) : interp(uv1[0],uv2[0],weight),
      wrap_v() ? w.interp(uv1[1],uv2[1],weight) : interp(uv1[1],uv2[1],weight)
      );
}

Wvec 
Map2D3D::du(CUVpt& uv) const
{
   // Base class does numerical derivative
   const double h = hlen();

   // XXX - maps that wrap should handle that in their map()
   //       (e.g. see TubeMap)
   return (map(uv + UVvec(h,0)) - map(uv - UVvec(h,0))) / (2*h);
}

Wvec 
Map2D3D::dv(CUVpt& uv) const
{
   // Base class does numerical derivative
   const double h = hlen();
   return (map(uv + UVvec(0,h)) - map(uv - UVvec(0,h))) / (2*h);
}

bool
Map2D3D::solve(CUVpt& uv, CWvec& delt_p, UVvec& delt_uv) const
{
   // At parameter point uv, given a world displacement
   // delt_p, calculate the parameter space displacement
   // delt_uv that corresponds most nearly to delt_p.

   // We're solving this system to find delt_uv:
   //
   //  (3x2)  (2x1)     (3x1)
   //    D * delt_uv = delt_p,
   //
   // where D is the (3x2) derivative matrix.
   // The system is over-determined, so we take the
   // least-squares solution.
   //
   // We could also invert the (3x3) deriv() matrix,
   // multiply it by delt_p, and take the 1st two
   // components... but this is fewer computations and
   // gives the same answer.

   // Columns of D:
   Wvec   Du  = du(uv);
   Wvec   Dv  = dv(uv);

   // Entries of (2x2) symmetric matrix: D.transpose() * D:
   double suu = Du*Du;
   double suv = Du*Dv;
   double svv = Dv*Dv;

   // Determinant of the (2x2) matrix:
   double det = suu*svv - sqr(suv);

   // Avoid division by zero:
   if (fabs(det) < gEpsAbsMath)
      return 0;

   // Entries of (2x1) matrix: D.transpose() * delt_p:
   double sup = Du*delt_p;
   double svp = Dv*delt_p;

   // Least-squares solution:
   delt_uv[0] = (sup*svv - svp*suv)/det;
   delt_uv[1] = (suu*svp - suv*sup)/det;

   // XXX - overly strict?
   //   If current uv coord is on the boundary, remove the
   //   cross-boundary component of the displacement.
   if (uv[0] == u_min() || uv[0] == u_max())
      delt_uv[0] = 0;
   if (uv[1] == v_min() || uv[1] == v_max())
      delt_uv[1] = 0;

   return 1;
}

bool
Map2D3D::invert(CWpt& p, CUVpt& uv, UVpt &ret, int max_iters) const
{
   // Given world-space point p and initial parameter
   // uv for which map(uv) approximates p, perform an
   // iterative optimization to find the uv-point that
   // locally maps closest to p:

   UVpt cur_uv = uv;
   for (int k=0; k<max_iters; k++) {
      UVvec delt_uv;
      if (!solve(cur_uv, p, delt_uv))
         return 0;
      cur_uv = add(cur_uv, delt_uv);
      if (delt_uv.length_sqrd() < 1e-4) { // XXX - how close is close?
         ret = cur_uv;
         return 1;
      }
   }

   return 0;
}

extern CWtransf& (*VIEW_NDC_TRANS)();

bool
Map2D3D::solve_ndc(CUVpt& uv, CNDCvec& delt_ndc, UVvec& delt_uv) const
{
   // We're solving this system to find delt_uv:
   //
   //  (2x2)  (2x1)     (2x1)
   //    D * delt_uv = delt_ndc,
   //
   // where D is the (2x2) derivative at uv of PM
   // (the NDC projection P composed w/ this map M).

   // derivative of the NDC projection at uv:
   Wtransf DP = VIEW_NDC_TRANS().derivative(map(uv));

   // the 2 columns of the 2x2 matrix D
   NDCvec D0 = NDCZvec(du(uv), DP);
   NDCvec D1 = NDCZvec(dv(uv), DP);

   // Determinant of the (2x2) matrix:
   double det = D0[0]*D1[1] - D0[1]*D1[0];

   // Avoid division by zero:
   if (isZero(det))
      return 0;

   // multiply D inverse by delt_ndc
   delt_uv[0] = (D1[1]*delt_ndc[0] - D1[0]*delt_ndc[1])/det;
   delt_uv[1] = (D0[0]*delt_ndc[1] - D0[1]*delt_ndc[0])/det;

   return 1;
}

bool 
Map2D3D::invert_ndc(CNDCpt& p, CUVpt& uv, UVpt &ret, int max_iters) const
{
   // Given NDC point p and initial parameter uv for which
   // map(uv) approximates p, perform an iterative
   // optimization to find the uv-point that locally maps
   // closest to p:

   bool debug = Config::get_var_bool("DEBUG_UV_INTERSECT",false,true);

   err_adv(debug, "Map2D3D::invert_ndc: starting %d iterations", max_iters);
   UVpt cur_uv = uv;
   for (int k=0; k<max_iters; k++) {
      UVvec  delt_uv;
      if (!solve_ndc(cur_uv, p, delt_uv)) {
         err_adv(debug, "solve failed, iteration %d", k);
         return 0;
      }
      cur_uv = add(cur_uv, delt_uv);
      // within a quarter pixel is good
      err_adv(debug, "%d: distance: %f", k, PIXEL(map(cur_uv)).dist(PIXEL(p)));
      if (PIXEL(map(cur_uv)).dist(PIXEL(p)) < 0.25) {
         err_adv(debug, "success on iteration %d", k);
         ret = cur_uv;
         return 1;
      }
   }
   err_adv(debug, "Map2D3D::invert_ndc: failed");
   return 0;
}

Wpt_list
Map2D3D::ucurve(CARRAY<double>& uvals, double v) const
{
   Wpt_list ret(uvals.num());
   for (int i=0; i<uvals.num(); i++)
      ret += map(UVpt(uvals[i], v));
   ret.update_length();
   return ret;
}

Wpt_list
Map2D3D::vcurve(double u, CARRAY<double>& vvals) const
{
   Wpt_list ret(vvals.num());
   for (int j=0; j<vvals.num(); j++)
      ret += map(UVpt(u, vvals[j]));
   ret.update_length();
   return ret;
}

inline double
dist(CWpt_list& p, CWpt_list& q)
{
   // return average distance between the polylines
   // (that must have same number of points)

   int n = p.num();
   assert(n == q.num());
   double ret = 0;
   for (int i=0; i<n; i++)
      ret += p[i].dist(q[i]);
   return ret / max(1,n);
}

void
Map2D3D::get_min_distortion_v_vals(
   CARRAY<double>& uvals,       // input u-vals
   ARRAY<double>& vvals,        // output v-vals
   double aspect                // desired ratio of v-length / u-length
   ) const
{
   vvals.clear();

   int n = uvals.num();

   static bool debug = Config::get_var_bool("DEBUG_MIN_DISTORT",false,true);
   if (n < 2) {
      if (debug)
         err_msg("Map2D3D::min_distortion_v_vals: Error: %d u-vals", n);
      return;
   }

   // tiny step size in v:
   double delt_v = max(1e-3, hlen());

   if (debug)
      err_msg("delt_v: %f", delt_v);

   // For cone tips, the smaller the 'factor', the greater number
   // of teensy slices that are generated in approaching the tip:
   double factor = Config::get_var_dbl("ANTI_DISTORT_TIP_FACTOR", 4.0,true);
   double max_v = 1 - factor*delt_v;

   // Curves with constant v, allocated here, used below:
   Wpt_list ucurve1, ucurve2;

   double r = 0; // aspect ratio, used below

   // Generate the desired v params, one at a time:
   double cur_v=0;
   while (cur_v < max_v) {
      
      vvals += cur_v;                           // record the current v
      ucurve1 = ucurve(uvals, cur_v);           // u-curve at current v
      double avg_u_len = ucurve1.avg_len();     // its avg length:

      // Find the next v:
      //
      // We'll take little steps in v, checking each ucurve until
      // the ratio:
      //
      //   (average v length) / (running avg u-curve length)
      //
      // exceeds the desired aspect ratio:
      //
      cur_v += delt_v;
      for (int j=1; cur_v < max_v; cur_v += delt_v) {
         ucurve2 = ucurve(uvals, cur_v);
         avg_u_len = interp(avg_u_len, ucurve2.avg_len(), 1.0/++j);
         r = dist(ucurve1, ucurve2) / avg_u_len;        // get the ratio
         if (r >= aspect)
            break;
      }
   }

   // Do correction so it ends at 1.0.
   if (vvals.num() < 2) {
      vvals += 1.0;
   } else {
      // If we were getting close to matching the aspect ratio,
      // add a last v-param past 1.0. Then, in any case, rescale
      // all the v-params to end at 1.0:

      if (r >= 1.0)
         vvals += cur_v;        // doh! it can happen
      else if (r > 0.5)
         vvals += interp(vvals.last(), 1.0, 1/r);

      if (debug) {
         err_msg("last ratio: %f", r);
         err_msg("v-params before correction:");
         cerr << vvals << endl;
      }

      vvals /= vvals.last();
   }

   if (debug) {
      cerr << "Output v-params:" << endl << vvals << endl;
   }
}

/*****************************************************************
 * RuledSurfCurveVec:
 *****************************************************************/
void 
RuledSurfCurveVec::transform(CWtransf& xf, CMOD& m)
{
   if (!can_transform())
      cerr << class_name() << "::transform: not implemented" << endl;
   else if (!_mod.current()) {
      _c->transform(xf, m);
      Map2D3D::transform(xf, m);
   }
}

/*****************************************************************
 * RuledSurf2Curves:
 *****************************************************************/
void 
RuledSurf2Curves::transform(CWtransf& xf, CMOD& m)
{
   if (!can_transform())
      cerr << class_name() << "::transform: not implemented" << endl;
   else if (!_mod.current()) {
      _c1->transform(xf, m);
      _c2->transform(xf, m);
      Map2D3D::transform(xf, m);
   }
}

/*****************************************************************
 * RuledSurfCurvePt:
 *****************************************************************/
void 
RuledSurfCurvePt::transform(CWtransf& xf, CMOD& m)
{
   if (!can_transform())
      cerr << class_name() << "::transform: not implemented" << endl;
   else if (!_mod.current()) {
      _c->transform(xf, m);
      _p->transform(xf, m);
      Map2D3D::transform(xf, m);
   }
}

/*****************************************************************
 * CoonsPatchMap:
 *
 *    Curves c1 and c2 are opposite, oriented the same way, 
 *    as are d1 and d2.
 *           
 *             c2
 *         ---------> 
 *         ^        ^      ^
 *      d1 |        | d2 v |
 *         --------->       u ->
 *             c1         
 *            
 *      (Surface normal faces you.)
 *  
 *****************************************************************/
CoonsPatchMap::CoonsPatchMap(
   Map1D3D* c1,
   Map1D3D* c2,
   Map1D3D* d1,
   Map1D3D* d2) :
   _rc(c1,c2),
   _rd(d1,d2) 
{
   // we require that u and v range in [0,1]x[0,1]:

   assert(c1 && c2 && d1 && d2 &&
          c1->t_min() == 0 &&
          c2->t_min() == 0 &&
          d1->t_min() == 0 &&
          d2->t_min() == 0 &&
          c1->t_max() == 1 &&
          c2->t_max() == 1 &&
          d1->t_max() == 1 &&
          d2->t_max() == 1);

   hookup(); // set up dependencies
}

Wpt    
CoonsPatchMap::map(CUVpt& uv) const
{
   Wpt a = _rc.map(UVpt(0,0));
   Wpt b = _rc.map(UVpt(1,0));
   Wpt c = _rc.map(UVpt(1,1));
   Wpt d = _rc.map(UVpt(0,1));
   
   double u = uv[0];
   double v = uv[1];

   Wpt rcd = interp(interp(a,d,v), interp(b,c,v), u);

   return _rc.map(uv) + (_rd.map(UVpt(v,u)) - rcd);
}

void 
CoonsPatchMap::transform(CWtransf& xf, CMOD& m)
{
   if (!can_transform())
      cerr << class_name() << "::transform: not implemented" << endl;
   else if (!_mod.current()) {
      _rc.transform(xf, m);
      _rd.transform(xf, m);
      Map2D3D::transform(xf, m);
   }
}

/*****************************************************************
 * XFormedMap2D3D
 *****************************************************************/
void 
XFormedMap2D3D::transform(CWtransf& xf, CMOD& m)
{
   // XXX -
   //
   //    Potential problem: our internal _map2d3d might get
   //    the same transform applied elsewhere. If that happens
   //    we are effetively applying it twice (below).
   //
   //    We could pass the transform on to the internal map,
   //    but then we'd have to alter it to deal w/ matrix
   //    non-commutativity, which again is incompatible with
   //    the same transform being applied elsewhere.

   if (!_mod.current()) {
      _xf = xf * _xf;
      Map2D3D::transform(xf, m);
   }
}

/*****************************************************************
 * PlaneMap
 *****************************************************************/
void 
PlaneMap::transform(CWtransf& xf, CMOD& m)
{
   if (!_mod.current()) {
      _origin = xf * _origin;
      _u = xf * _u;
      _v = xf * _v;
      Map2D3D::transform(xf, m);
   }
}

/*****************************************************************
 * TubeMap
 *****************************************************************/
TubeMap::TubeMap(
   Map1D3D* axis,
   Map1D3D* c0,
   CWpt_list& spts
   ) : _a(axis),
       _c0(c0),
       _s(spts)
{
   assert(_a && _c0);
   assert(_c0->is_closed());

   // For good measure:
   // XXX - needed?
   _s.update_length();

   // Create the top curve as a copy of the bottom one,
   // but moved to the top and rescaled to match the sweep curve.

   // Scale factor:
   double s = spts.last()[2]/spts.first()[2];

   // Combined xform:
   Wtransf M = _a->F(1) * Wtransf::scaling(0,s,s) * _a->Finv(0);

   // map for the top cross-section:
   _c1 = new Wpt_listMap(M *_c0->get_wpts(), 0, 0, _a->tan(1));

   hookup();
}

Wpt 
TubeMap::map(CUVpt& uv) const 
{
   // Point on surface at parameter uv

   double u = WrapCoord().N(uv[0]); // normalize to [0,1)
   double v = uv[1];

   // v varies along the sweep curve:
   Wpt sv = _s.interpolate(v);

   // sv is expressed in t, b, n coordinate frame:
   //
   //    sv[0] : t value along axis
   //    sv[1] : always 0, not used
   //    sv[2] : scale factor applied to both b, n coords

   // The story of a bug.
   // 
   //   We used to pre-apply the following scalings, until it
   //   finally dawned on us that it was messing up the
   //   parameterization, which in Wpt_list is done by arc
   //   length. If you go around scaling the points
   //   non-uniformly, then you need a length metric that undoes
   //   the non-uniform scale. Rather than go there, we just
   //   apply the scaling to each point after we get it out of
   //   the Wpt_list.

   // Choose scale factor to apply to top and bottom curves:
   double scale0 = sv[2] / _s.first()[2];
   double scale1 = sv[2] / _s.last ()[2];
   Wtransf S0 = Wtransf::scaling(0, scale0, scale0);
   Wtransf S1 = Wtransf::scaling(0, scale1, scale1);

   // normalize t coord to end at 1.0
   double t = sv[0] / _s.last()[0];

   // Get axis frame F at t, apply to interpolated, scaled
   // cross-sections in local coordinates of F:
   // XXX - should cache _a->Finv(0), _a->Finv(1)
   return _a->F(t) * interp(S0 * (_a->Finv(0) * _c0->map(u)),
                            S1 * (_a->Finv(1) * _c1->map(u)), v);
}

void
TubeMap::set_pts(CWpt_list &pts)
{
   _s = pts; 
   _s.update_length();
   if (false && pts.num() > 0 && _c1 != NULL) {

      //Wpt_listMap* axis = Wpt_listMap::upcast(_a);
      //Wpt_list apts = axis->pts();
      //assert(apts.num() == 2);
      //axis->p1()->set_pt(Wline(apts[0], apts[1]).project(pts.last()));

      // Scale factor:
      double s = pts.last()[2]/pts.first()[2];

      // Combined xform:
      Wtransf M = _a->F(1) * Wtransf::scaling(0,s,s) * _a->Finv(0);

      // map for the top cross-section:
      //_c1 = new Wpt_listMap(M *_c0->get_wpts(), 0, 0, _a->tan(1));
      ((Wpt_listMap*)_c1)->set_pts(M *_c0->get_wpts());
   }
   // Notify dependents they're out of date and sign up to be
   // recomputed
   invalidate();
}

bool
TubeMap::can_transform() const
{
   return _c0->can_transform() && _c1->can_transform() && _a->can_transform();
}

void 
TubeMap::transform(CWtransf& xf, CMOD& m)
{
   if (!can_transform())
      cerr << class_name() << "::transform: not implemented" << endl;
   else if (can_transform() && !_mod.current()) {
      _c0->transform(xf, m);
      _c1->transform(xf, m);
      _a ->transform(xf, m);
      Map2D3D::transform(xf, m);
   }
}

/* end of file mapping2d3d.C */
