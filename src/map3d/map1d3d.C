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
 * map1d3d.C:
 *****************************************************************/
#include "map2d3d.H"
#include "std/config.H"

using namespace mlib;

/*****************************************************************
 * Map1D3D:
 *****************************************************************/
Map1D3D::Map1D3D(Map0D3D* p0, Map0D3D* p1, CWvec& n) :
   _p0(p0),
   _p1(p1),
   _norm(n)
{
   if (_p0 || _p1) {
      assert(_p0 && _p1);
      hookup();         // set up dependencies
      invalidate();     // ensure we recompute to match endpoints
   }
}

Bnode_list 
Map1D3D::inputs() const 
{
   // The endpoints (if any) are the inputs:

   if (_p0 && _p1)
      return Bnode_list((Bnode*) _p0, (Bnode*) _p1);
   else {
      assert(!(_p0 || _p1));
      return Bnode_list();
   }
}

double 
Map1D3D::avg(double t1, double t2, double weight) const
{
   // Return an average parameter between the given parameters,
   // doing the right thing if this is a closed curve. The
   // weight is the fraction of the way from t1 to t2:
   // 
   // E.g., on a closed curve the half-way point between 
   // 0.8 and 0.1 is 0.95.

   WrapCoord w;
   return is_closed() ? w.interp(t1, t2, weight) : interp(t1, t2, weight);
}

double 
Map1D3D::add(double t, double d) const
{
   // Add the displacement d to the parameter t, handling
   // correctly the case that the curve wraps around.
   // E.g., 0.9 + .15 = 0.05

   WrapCoord w;
   return is_closed() ? w.add(t,d) : (t + d);
}

Wvec      
Map1D3D::deriv(double t, double l) const
{
   // Base class does numerical derivative.

   // See comment in map1d3d.H about how we deal w/
   // parameters outside the range [0,1).

   // Step size for numerical derivative.
   double h = (l == 0) ? hlen() : (l/length());

   // For closed curves normalize parameter to [0,1):
   if (is_closed())
      return (map(WrapCoord::N(t+h)) - map(WrapCoord::N(t-h)))/(2*h);

   // Derivative is constant outside [0,1].
   // Clamp parameter to [0,1]:
   t = clamp(t, 0.0, 1.0);

   // We'll try to be careful at the ends.
   if (t < h)
      return (map(t+h) - map(0))/(t+h);
   else if (t > 1-h)
      return (map(1) - map(t-h))/(1-t+h);
   else
      return (map(t+h) - map(t-h))/(2*h);
}

bool   
Map1D3D::solve(double t, CWvec& delt_p, double& delt_t, double l) const
{       
   // At parameter value t, given a world displacement
   // delt_p, calculate the parameter space displacement
   // delt_t that corresponds most nearly to delt_p.

   delt_t = 0;
   Wvec d = deriv(t,l);
   double dd = d*d;
   if (dd < gEpsAbsMath)
      return 0;
   delt_t = (d * delt_p) / dd;
   return 1;
}

bool
Map1D3D::invert(
   CWpt& p,
   double t,
   double &ret,
   int max_iters) const 
{
   // Given world-space point p and initial parameter t for
   // which map(t) approximates p, perform an iterative
   // optimization to find the t-value that locally maps
   // closest to p. Returns false if it fails to get "close"
   // after the given maximum number of iterations.

   double cur_t = t;
   for (int k=0; k<max_iters; k++) {
      double delt_t;
      if (!solve(cur_t, p - map(cur_t), delt_t))
         return 0;
      cur_t += delt_t;
      if (fabs(delt_t) < 1e-3) {
         ret = cur_t;     // doesn't get much better than this
         return 1;
      }
   }

   return 0;
}

void
Map1D3D::set_p0(Map0D3D* p)
{
  unhook();
  _p0 = p;
  hookup();
}

void
Map1D3D::set_p1(Map0D3D* p)
{
  unhook();
  _p1 = p;
  hookup();
}

void
Map1D3D::replace_endpt(Map0D3D* old_pt,
                       Map0D3D* new_pt)
{
  if (old_pt == p0())
    set_p0(new_pt);
  else if (old_pt == p1())
    set_p1(new_pt);
}

void 
Map1D3D::transform(CWtransf& xf, CMOD& m)  
{
   if (!can_transform())
      cerr << class_name() << "::transform: not implemented" << endl;
   else if (!_mod.current()) {
      _norm = xf * _norm;
      if (_p0) _p0->transform(xf, m);
      if (_p1) _p1->transform(xf, m);
      Map::transform(xf, m);
   }
}

/*****************************************************************
 * ReverseMap1D3D:
 *****************************************************************/
ReverseMap1D3D::ReverseMap1D3D(Map1D3D* m) :
  _map1d3d(m) // don't need to pass p0, p1, etc.
{
   // Set up a Bnode dependency relation on the internal map:
   assert(_map1d3d);
   hookup();
}

ReverseMap1D3D::~ReverseMap1D3D() 
{
   // Remove the Bnode dependency on the internal map:
   assert(_map1d3d);
   unhook();
}

void 
ReverseMap1D3D::transform(CWtransf& xf, CMOD& m) 
{
   if (!can_transform())
      cerr << class_name() << "::transform: not implemented" << endl;
   else if (!_mod.current()) {
      _map1d3d->transform(xf, m);
      Map1D3D::transform(xf, m);
   }
}

/*****************************************************************
 * Wpt_listMap:
 *****************************************************************/
Wpt_listMap::Wpt_listMap(
   CWpt_list& pts,
   Map0D3D*    p0,
   Map0D3D*    p1,
   CWvec&       n
   ) : Map1D3D(p0, p1, n),
       _pts(pts)
{
   assert(_pts.num() > 1);

   // XXX - why doesn't this happen in the constructor???
   //       the lines of code are there, what else can we do???
   _pts.update_length();

   if (_p0 || _p1)
      assert(_p0 && _p1 && !_pts.is_closed());
}

void
Wpt_listMap::recompute()
{
   // Translate, rotate and scale points to match endpoints.

   if (_p0 && _p1)
      _pts.fix_endpoints(_p0->map(), _p1->map());
}

Wpt
Wpt_listMap::map(double t) const
{
   // Point on curve at parameter value t.

   // See comment in map1d3d.H about how we deal w/
   // parameters outside the range [0,1).

   if (is_closed()) {
      return _pts.interpolate(WrapCoord::N(t));
   } else if (t < 0) {
      Wpt    p = _pts.first();
      Wvec   v = _pts.vec(0).normalized();
      double s = t*_pts.length();
      return p + (v*s);
   } else if (t > 1) {
      Wpt    p = _pts.last();
      Wvec   v = _pts.vec(_pts.num()-2).normalized();
      double s = (t - 1)*_pts.length();
      return p + (v*s);
   } else
      return _pts.interpolate(t);
}

void
Wpt_listMap::set_pts(CWpt_list& pts)
{
   _pts = pts; 
   _pts.update_length();
   if (pts.num() > 0 && _p0 != NULL && _p1 != NULL) {
      if (!_p0->set_pt(pts[0]))
         err_msg("Wpt_listMap::set_pts: error setting endpoint");
      if (!_p1->set_pt(pts.last()))
         err_msg("Wpt_listMap::set_pts: error setting endpoint");
   }
   // Notify dependents they're out of date and sign up to be
   // recomputed
   invalidate();
}

void 
Wpt_listMap::transform(CWtransf& xf, CMOD& m) 
{
   if (!_mod.current()) {
      _pts.xform(xf);
      Map1D3D::transform(xf, m);
   }
}

/*****************************************************************
 * SurfaceCurveMap
 *****************************************************************/
SurfaceCurveMap::SurfaceCurveMap(
   Map2D3D* surface,
   CUVpt_list& uvs,
   Map0D3D* p0,
   Map0D3D* p1
   ) : Map1D3D(p0, p1),
       _surf(surface),
       _uvs(uvs)
{
   assert(_uvs.num() > 1);

   if (_p0 || _p1) {
      assert(!_uvs.is_closed());
   } else
      assert(_uvs.is_closed());

   assert(_surf != NULL);
   hookup();
   
   if (Config::get_var_bool("DEBUG_SURF_CURVE_MAP_CONSTRUCTOR",false,true)) {
      cerr << " -- in SurfaceCurveMapconstructor with uvpts: " << endl;
      int i;
      for (i = 0; i < _uvs.num(); i++) {
         cerr << _uvs[i] << endl;
      }
      cerr << " -- end uv pts list" << endl;
   }

}

void
SurfaceCurveMap::recompute()
{
   // Translate, rotate and scale uv points to match endpoints.

   if (_p0 && _p1) {
      assert(!_uvs.is_closed());
      // If we can figure out the uv-coords of the endpoints,
      // warp the UVpt_list to match:
      UVpt uv0, uv1;    // uv coords for endpoints
      Wpt p0, p1; // wpts for endpoints
      Wpt op0, op1; // old wpts for endpoints (according to the currently cached uv points)
      p0 = _p0->map();
      p1 = _p1->map();
      op0 = _surf->map(_uvs.first());
      op1 = _surf->map(_uvs.last());
      bool needfix = false;

      static bool debug =
         Config::get_var_bool("DEBUG_SURF_CURVE_MAP_RECOMPUTE",false,true);

      if (debug) {
         cerr << "**** In surface curve map recompute\n";
         cerr << "p0 p1 op0 op1 uv(0) uv(1)" << endl;
         cerr << p0 << p1 << op0 <<  op1 << uv(0) << uv(1) << endl;
      }

      if (p0 != op0) {
         if(!_surf->invert(p0, uv(0), uv0))
            cerr << "SurfaceCurveMap::recompute(): warning.. could not invert endpoint" << endl;
         needfix = true;
      } else {
         uv0 = _uvs.first();
      }

      if (p1 != op1) {
         if (!_surf->invert(p1, uv(1), uv1))
            cerr << "SurfaceCurveMap::recompute(): warning.. could not invert endpoint" << endl;
         needfix = true;
      } else {
         uv1 = _uvs.last();
      }

      if (needfix) {
         if (RuledSurfCurveVec::isa(_surf)) {
            // we do a special case if we are in a ruled surface
            // defined by a curve

            if(debug) cerr << "Special case recompute for RuledSurfCurveVec" << endl;
            // in particular this is to fix the problem with curve sketch behavior
            UVvec delta0 = uv0 - uv(0); // the distance in uv the first endpoint moved
            UVvec delta1 = uv1 - uv(1); // the distance in uv the second endpoint moved
            delta0[0] = 0.0; // we only care about the second component
            delta1[0] = 0.0; 
            int i;
            for (i = 0; i < _uvs.num(); i++) {
               double t = (_uvs[i])[0]; // this t should be 0 <= t <= 1
               UVvec delt = (1.0 - t) * delta0 + t * delta1; // linearly interpolate
               if (debug) cerr << "applying delta " << delta0 << delta1 << " t = " << t << endl;
               _uvs[i] = _uvs[i] + delt;
            }
            _uvs.update_length();
         } else {
            if (debug) {
               cerr << "calling fix endpionts with" << uv0 << uv1 << endl;
            }
            _uvs.fix_endpoints(uv0, uv1);
         }
      }
   }
}

Wpt
SurfaceCurveMap::map(double t) const
{
   // Point on curve at parameter value t.

   return _surf->map(uv(t));
}

Wpt_list 
SurfaceCurveMap::get_wpts() const 
{ 
  // Return a Wpt_list describing the current shape of the map
  Wpt_list ret(_uvs.num());
  for ( int i=0; i<_uvs.num(); i++ ) {
    ret += _surf->map(_uvs[i]);
  }
  return ret;
}

Wvec 
SurfaceCurveMap::norm(double t) const
{
   // The local unit-length normal:

   return _surf->norm(uv(t));
}

double 
SurfaceCurveMap::length() const
{
   // Total length of curve:

   // XXX - if this is called often should cache the info

   double len = 0;
   for (int k=1; k<_uvs.num(); k++)
      len += _surf->map(_uvs[k]).dist(_surf->map(_uvs[k-1]));
   return len;
}

double
SurfaceCurveMap::hlen() const
{
   // The "planck length" for this curve; i.e., the step size
   // below which you'd just be sampling the noise:

   // XXX - this isn't a good way to calculate this...
   //       but it's okay "for now."

   double h = ((_uvs.num()  < 2) ?  1.0 :
               (_uvs.num() == 2) ? 1e-9 :
               1.0/_uvs.num());
   return max(h, _surf->hlen())/10.0;
}

void
SurfaceCurveMap::set_uvs(CUVpt_list& uvs)
{
   _uvs = uvs; // calls update_length
   
   // does it really?
   _uvs.update_length();

   static bool debug = Config::get_var_bool("DEBUG_SURF_CURVE_SETUV",false,true);

   if (debug) {
      cerr << "-- In SurfaceCurveMap::set_uvs() got hte following uv list" << endl;
      int i;
      for (i = 0; i < _uvs.num(); i++) {
         cerr << i << " " << _uvs[i] << endl;
      }
      cerr << "-- End list" << endl;
   }

   if (_p0 || _p1)
      assert(!_uvs.is_closed());

   // Notify dependents they're out of date and sign up to be
   // recomputed
   invalidate();
}

/* end of file curve.C */
