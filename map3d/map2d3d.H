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
#ifndef MAP2D3D_H_IS_INCLUDED
#define MAP2D3D_H_IS_INCLUDED

#include "map1d3d.H"
#include "mlib/points.H"
/*****************************************************************
 * Map2D3D:
 *
 *   Base class for a mapping from [0,1] x [0,1] ---> R^3,
 *   i.e. a parameterized surface in 3D.
 *****************************************************************/
#define CMap2D3D const Map2D3D
class Map2D3D : public Map {
 public:
   //******** MANAGERS ********
   Map2D3D() {}
   virtual ~Map2D3D() {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Map2D3D", Map2D3D*, Map, CBnode*);

   //******** VIRTUAL METHODS ********

   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv) const  = 0;

   // Partial derivatives.
   // (By default, base class does numerical differentiation):
   virtual Wvec du(CUVpt& uv) const;
   virtual Wvec dv(CUVpt& uv) const;

   // The "planck length" for this surface; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const = 0;

   // Tell whether the surface wraps in either u or v:
   // (E.g. a cylinder wraps in one direction, a torus in two).
   virtual bool wrap_u() const { return false; }
   virtual bool wrap_v() const { return false; }

   // The domain of the map should be an axis-aligned
   // rectangle in UV-space. The following define the bounds
   // of the rectangle:
   virtual double u_min() const { return 0; }
   virtual double u_max() const { return 1; }
   virtual double v_min() const { return 0; }
   virtual double v_max() const { return 1; }

   virtual bool is_u_extreme(CUVpt& uv) const {
      return (uv[0] == u_min() || uv[0] == u_max());
   }
   virtual bool is_v_extreme(CUVpt& uv) const {
      return (uv[1] == v_min() || uv[1] == v_max());
   }

   // Convenience
   bool is_topological_sheet()    const { return !(wrap_u() || wrap_v()); }
   bool is_topological_cylinder() const { return XOR(wrap_u(), wrap_v()); }
   bool is_topological_torus()    const { return wrap_u() && wrap_v();    }

   // Returns indicated iso-parameter curve:
   Wpt_list ucurve(CARRAY<double>& uvals, double v) const;
   Wpt_list vcurve(double u, CARRAY<double>& vvals) const;

   // Given a set of u-values, returns a list of v-values which
   // approximately preserve the given aspect ratio of the quads
   // created when tessellating the surface in a regular grid:
   void get_min_distortion_v_vals(
      CARRAY<double>& uvals,    // given u-values
      ARRAY<double>& vvals,     // returned v-values
      double aspect = 1.0       // desired ratio of v-length / u-length
      ) const;

   //******** COMPUTATION (NOT VIRTUAL) ********

   // Return an average uv-coord between the given coordinates,
   // doing the right thing if the surface wraps around. The
   // weight is the fraction of the way from uv1 to uv2:
   UVpt avg(CUVpt& uv1, CUVpt& uv2, double weight = 0.5) const;

   UVpt add(CUVpt& uv, CUVvec& delt) const;

   // Surface normal:
   Wvec norm(CUVpt& uv) const { return cross(du(uv),dv(uv)).normalized(); }

   // "Derivative" at a given uv point.
   Wtransf deriv(CUVpt& uv) const {

      // By rights this should return a (3x2) matrix.  Partly
      // because we haven't implemented such a matrix class, we
      // now pretend this is really a mapping from R^3 to R^3,
      // where the u and v parameters map onto the surface, and
      // the third parameter w specifies a displacement from the
      // surface along the local normal. If the amount of
      // displacement in world space is w, then the partial
      // derivative of the map w/ respect to w is just the
      // unit-length surface normal.

      return Wtransf(du(uv), dv(uv), norm(uv));
   }

   // At parameter point uv, given a world displacement delt_p,
   // calculate the parameter space displacement delt_u that
   // corresponds most nearly to delt_p.
   bool solve(CUVpt& uv, CWvec& delt_p, UVvec& delt_u) const;

   // Similar to above, but instead of taking a displacement to a
   // desired target location, it takes the target location
   // directly (for convenience):
   bool solve(CUVpt& uv, CWpt& target, UVvec& delt_u) const {
      return solve(uv, target - map(uv), delt_u);
   }

   // Given world-space point p and initial parameter uv
   // for which map(uv) approximates p, perform an iterative
   // optimization to find the uv-point that locally maps
   // closest to p. Returns false if it fails to get "close"
   // after the given maximum number of iterations.
   bool invert(CWpt& p, CUVpt& uv, UVpt& ret, int max_iters=15) const;

   //******** SOLVING FOR NDC ********

   // At parameter point uv, given an NDC displacement delt_ndc,
   // calculate the parameter space displacement delt_u that
   // corresponds most nearly to delt_ndc.
   bool solve_ndc(CUVpt& uv, CNDCvec& delt_ndc, UVvec& delt_uv) const;

   // Similar to above, but instead of taking a displacement to a
   // desired target location, it takes the target location
   // directly (for convenience):
   bool solve_ndc(CUVpt& uv, CNDCpt& target, UVvec& delt_u) const {
      return solve_ndc(uv, target - NDCpt(map(uv)), delt_u);
   }

   // Given NDC point p and initial parameter uv
   // for which map(uv) approximates p, perform an iterative
   // optimization to find the uv-point that locally maps
   // closest to p. Returns false if it fails to get "close"
   // after the given maximum number of iterations.
   bool invert_ndc(CNDCpt& p, CUVpt& uv, UVpt &ret, int max_iters) const;
};

/*****************************************************************
 * RuledSurfCurveVec:
 *
 *   Ruled surface based on a Map1D3D and its unit "normal"
 *   direction.
 *****************************************************************/
#define CRuledSurfCurveVec const RuledSurfCurveVec
class RuledSurfCurveVec : public Map2D3D {
 public:
   //******** MANAGERS ********

   RuledSurfCurveVec(Map1D3D* c) : _c(c) {
      assert(_c != NULL);
      hookup();
   }

   virtual ~RuledSurfCurveVec() { unhook(); } // undoes dependencies

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3(
      "RuledSurfCurveVec", RuledSurfCurveVec*, Map2D3D, CBnode*
      );

   //******** Map2D3D VIRTUAL METHODS ********
 
   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv) const {
      return _c->map(uv[0]) + _c->norm(uv[0])*uv[1];
   }

   virtual void set_curve(Map1D3D* c) { _c = c; }
   virtual Map1D3D* get_curve() { return _c; }

   // The "planck length" for this surface; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const { return _c->hlen(); }

   // Partial derivatives:
   virtual Wvec du(CUVpt& uv) const { return Map2D3D::du(uv); } // numerical
   virtual Wvec dv(CUVpt& uv) const { return _c->norm(uv[0]); }    // "exact"

   virtual double u_min() const { return _c->t_min(); }
   virtual double u_max() const { return _c->t_max(); }
   virtual double v_min() const { return -DBL_MAX;    }
   virtual double v_max() const { return  DBL_MAX;    }

   //******** MAP VIRTUAL METHODS ********

   virtual bool can_transform() const { return _c->can_transform(); }
   virtual void transform(CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // The defining curve is the input:
   virtual Bnode_list inputs()  const { return Bnode_list(_c); }

 protected:

   // The curve:
   Map1D3D* _c;    
};

/*****************************************************************
 * RuledSurf2Curves:
 *
 *      Based on: Gerald Farin: Curves and Surfaces for CAGD,
 *                3rd Ed., section 20.1 (pp. 364-365).
 *****************************************************************/
class RuledSurf2Curves : public Map2D3D {
 public:
   //******** MANAGERS ********

   // Define a ruled surface from two given curves, which should
   // be oriented similarly. The u parameter will map along the
   // curves. The v parameter will map from a given u-point on c1
   // to the correspnding u-point on c2:
   RuledSurf2Curves(Map1D3D* c1, Map1D3D* c2) : _c1(c1), _c2(c2) {
      // require compatible parameterizations:
      assert(_c1 && _c2 &&
             _c1->t_min() == _c2->t_min() &&
             _c1->t_max() == _c2->t_max());

      hookup(); // set up dependencies
   }

   virtual ~RuledSurf2Curves() { unhook(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("RuledSurf2Curves", Map2D3D, CBnode*);

   //******** Map2D3D VIRTUAL METHODS ********
 
   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv) const {
      return interp(_c1->map(uv[0]), _c2->map(uv[0]), uv[1]);
   }

   // Partial derivatives:
   virtual Wvec du(CUVpt& uv) const {
      return interp(_c1->deriv(uv[0]), _c2->deriv(uv[0]), uv[1]);
   }
   virtual Wvec dv(CUVpt& uv) const {
      return _c2->map(uv[0]) - _c1->map(uv[0]);
   }

   // The "planck length" for this surface; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const { return max(_c1->hlen(), _c2->hlen()); }

   virtual double u_min() const { return _c1->t_min(); }
   virtual double u_max() const { return _c1->t_max(); }

   // Don't need to override v_min() and v_max()
   // because v ranges from 0 to 1

   //******** MAP VIRTUAL METHODS ********

   virtual bool can_transform() const {
      return _c1->can_transform() && _c2->can_transform();
   }
   virtual void transform(CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // The two defining curves are the inputs:
   virtual Bnode_list inputs()  const { return Bnode_list(_c1, _c2); }

 protected:

   // The two curves:
   Map1D3D*     _c1;    
   Map1D3D*     _c2;
};

/*****************************************************************
 * RuledSurfCurvePt:
 *
 *  A ruled surface that is defined by a curve and rays emanating *
 *  from one point to all points on the curve.
 *
 *  Note: parameterization is strange and should probably be fixed.
 *
 *   u varies along the curve.
 *
 *   v measures distance from the curve along the ray FROM
 *   the point. I.e., v == 0 corresponds to a point on the curve,
 *   and v > 0 maps to the ray from the point.
 *
 *****************************************************************/

class RuledSurfCurvePt : public Map2D3D {

 public:
   //******** MANAGERS ********

   RuledSurfCurvePt(Map1D3D* c, Map0D3D* p) : _c(c), _p(p) {
      assert(_c && _p);
      hookup(); // set up dependencies
   }

   virtual ~RuledSurfCurvePt() { unhook(); }

   //******** ACCESSORS ********
   Map1D3D*   curve()   const { return _c; }
   Map0D3D*   pt()      const { return _p; }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("RuledSurfCurvePt", Map2D3D, CBnode*);

   //******** Map2D3D VIRTUAL METHODS ********
 
   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv) const {
      Wpt o = _c->map(uv[0]);
      return o + ((o - _p->map()).normalized() * uv[1]);
   }

   // Partial derivatives:
   virtual Wvec du(CUVpt& uv) const {
      return _c->deriv(uv[0]);
   }
   virtual Wvec dv(CUVpt& uv) const {
      return (_c->map(uv[0]) - _p->map()).normalized();
   }

   // The "planck length" for this surface; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const { return _c->hlen(); }

   virtual double u_min() const { return _c->t_min(); }
   virtual double u_max() const { return _c->t_max(); }
   virtual double v_min() const { return -DBL_MAX;    }
   virtual double v_max() const { return  DBL_MAX;    }

   //******** MAP VIRTUAL METHODS ********

   virtual bool can_transform() const {
      return _c->can_transform() && _p->can_transform();
   }
   virtual void transform(CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // The two defining curves are the inputs:
   virtual Bnode_list inputs()  const { return Bnode_list(_c, _p); }

 protected:

   Map1D3D*     _c;     // the curve
   Map0D3D*     _p;     // the point
};

/*****************************************************************
 * CoonsPatchMap:
 *
 *      Based on: Gerald Farin: Curves and Surfaces for CAGD,
 *                3rd Ed., section 20.2 (pp. 365-368).
 *
 * Note: we require that u and v range in [0,1]x[0,1].
 *
 *****************************************************************/
class CoonsPatchMap : public Map2D3D {
 public:

   //******** MANAGERS ********

   //  Curves c1 and c2 are opposite, oriented the same way, 
   //  as are d1 and d2.                                     
   //                                                        
   //           c2                                           
   //       --------->                                       
   //       ^        ^      ^                                
   //    d1 |        | d2 v |                                
   //       --------->       u ->                            
   //           c1                                           
   //                                                        
   //    (Surface normal faces you.)                         
   //                                                        
   CoonsPatchMap(Map1D3D* c1, Map1D3D* c2, Map1D3D* d1, Map1D3D* d2);

   virtual ~CoonsPatchMap() { unhook(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("CoonsPatchMap", Map2D3D, CBnode*);

   //******** Map2D3D VIRTUAL METHODS ********
 
   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv) const;

   // XXX - for now, going with numerical partial derivatives...

   // The "planck length" for this surface; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const { return max(_rc.hlen(), _rd.hlen()); }

   //******** MAP VIRTUAL METHODS ********

   virtual bool can_transform() const {
      return _rc.can_transform() && _rd.can_transform();
   }
   virtual void transform(CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // The two ruled surfaces are the inputs:
   virtual Bnode_list inputs()  const {
      return Bnode_list((Bnode*) &_rc, (Bnode*) &_rd);
   }

 protected:

   // Ruled surfaces used internally in computing the Coons patch:
   RuledSurf2Curves  _rc;
   RuledSurf2Curves  _rd;  // have to reverse u and v
};

/*****************************************************************
 * XFormedMap2D3D:
 *
 *      Applies an affine transform to an internal Map2D3D.
 *****************************************************************/
class XFormedMap2D3D : public Map2D3D {
 public:
   //******** MANAGERS ********

   // You're on your honor to supply an affine (not perspective)
   // transform:
   XFormedMap2D3D(Map2D3D* m, CWtransf& xf = Identity) :
      _map2d3d(0),
      _xf(xf) {
      set_map(m);
   }

   virtual ~XFormedMap2D3D() { clear_map(); }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("XFormedMap2D3D", Map2D3D, CBnode*);

   //******** ACCESSORS ********

   CWtransf& xf()                       const   { return _xf; }
   void      set_xf(CWtransf& xf)               { _xf = xf; invalidate(); }

   void clear_map() {
      if (_map2d3d) {
         _map2d3d->rem_output(this);
         _map2d3d = 0;
      }
   }
   void set_map(Map2D3D* m) {
      clear_map();
      if ((_map2d3d = m) != NULL) {
         _map2d3d->add_output(this);
         invalidate();
      }
   }
   void delete_map() {
      Map2D3D* m = _map2d3d;
      clear_map();
      delete m;
   }

   //******** Map2D3D VIRTUAL METHODS ********

   // XXX - should test below if pointer _map2d3d is null

   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv) const { return _xf * _map2d3d->map(uv); }

   // Partial derivatives:
   virtual Wvec du(CUVpt& uv)   const   { return _xf * _map2d3d->du(uv); }
   virtual Wvec dv(CUVpt& uv)   const   { return _xf * _map2d3d->dv(uv); }

   // The "planck length" for this surface; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen()        const   { return _map2d3d->hlen(); }

   virtual bool wrap_u()        const   { return _map2d3d->wrap_u(); }
   virtual bool wrap_v()        const   { return _map2d3d->wrap_v(); }

   virtual double u_min() const { return _map2d3d->u_min(); }
   virtual double u_max() const { return _map2d3d->u_max(); }
   virtual double v_min() const { return _map2d3d->v_min(); }
   virtual double v_max() const { return _map2d3d->v_max(); }

   //******** MAP VIRTUAL METHODS ********

   virtual bool can_transform()         const   { return true; }
   virtual void transform(CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // The internal map2D3D is the input:
   virtual Bnode_list inputs() const {
      return _map2d3d ? Bnode_list((Bnode*)_map2d3d) : Bnode_list();
   }

 protected:
   Map2D3D*     _map2d3d;       // the internal map2D3D:
   Wtransf      _xf;            // the affine transform
};

/*****************************************************************
 * UnitSphereMap:
 *
 *      Standard unit sphere mapped from [0,1]x[0,1]
 *        - radius 1
 *        - center at origin
 *        - equator lies in the XZ plane
 *        - constant v: lines of latitude 
 *        - constant u: lines of longitude
 *        - v is 0 at south pole (0,-1,0)
 *        - v is 1 at north pole (0, 1,0)
 *        - u is 0 at z-axis, increasing CCW around y-axis.
  *****************************************************************/
class UnitSphereMap : public Map2D3D {
 public:

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("UnitSphereMap", Map2D3D, CBnode*);

   //******** Map2D3D VIRTUAL METHODS ********

   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv_raw) const {
      UVpt uv = rescale(uv_raw);
      return Wpt(cos(uv[1])*sin(uv[0]),
                 sin(uv[1]),
                 cos(uv[1])*cos(uv[0]));
   }

   // Partial derivatives:
   virtual Wvec du(CUVpt& uv_raw) const {
      UVpt uv = rescale(uv_raw);
      return Wvec( cos(uv[1])*cos(uv[0]),
                   0,
                  -cos(uv[1])*sin(uv[0])) * TWO_PI;
   }
   virtual Wvec dv(CUVpt& uv_raw) const {
      UVpt uv = rescale(uv_raw);
      return Wvec(-sin(uv[1])*sin(uv[0]),
                   cos(uv[1]),
                  -sin(uv[1])*cos(uv[0])) * M_PI;
   }

   // The "planck length" for this surface; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen()        const   { return 1e-9; }

   // The u-parameter traces a path that returns to itself:
   virtual bool wrap_u() const { return true; }

   //******** Bnode VIRTUAL METHODS ********

   // No inputs
   virtual Bnode_list inputs()  const { return Bnode_list(); }

 protected:

   // Rescale u from [0,1] to [0, TWO_PI].
   // Rescale v from [0,1] to [-PI/2, PI/2].
   UVpt rescale(CUVpt& uv) const {
      return UVpt(interp(    0.0, TWO_PI, uv[0]),
                  interp(-M_PI_2, M_PI_2, uv[1]));
   }
};

/*****************************************************************
 * EllipsoidMap:
 *
 *      Ellipsoid with given origin and radius,
 *      or <x,y,z> scale factors.
  *****************************************************************/
class EllipsoidMap : public XFormedMap2D3D {
 protected:
   //******** PROTECTED METHODS ********
   Wtransf build_xf(CWpt& o, CWvec& factors) const {
      return Wtransf::translation(o - Wpt::Origin())*Wtransf::scaling(factors);
   }
   Wtransf build_xf(CWpt& o, double rad) const {
      return build_xf(o, Wvec(rad,rad,rad));
   }

 public:

   //******** MANAGERS ********
   EllipsoidMap(CWpt& o, double rad)     : XFormedMap2D3D(new UnitSphereMap) {
      set_xf(build_xf(o, rad));
   }
   EllipsoidMap(CWpt& o, CWvec& factors) : XFormedMap2D3D(new UnitSphereMap) {
      set_xf(build_xf(o, factors));
   }

   ~EllipsoidMap() { delete_map(); }

   //******** ORIGIN AND SCALE FACTORS ********
   void set_origin(CWpt& o)       { set_xf(build_xf(o, xf().get_scale())); }
   void set_scale(double rad)     { set_xf(build_xf(xf().origin(), rad)); }
   void set_scale(CWvec& factors) { set_xf(build_xf(xf().origin(), factors)); }

   //******** XFormedMap2D3D VIRTUAL METHODS ********

   // XXX - should use protected inheritance or otherwise prevent
   //       someone messing with the xform
/*
   // Just bring these into public scope:
   XFormedMap2D3D::map;
   XFormedMap2D3D::du;
   XFormedMap2D3D::dv;
   XFormedMap2D3D::wrap_u;
   XFormedMap2D3D::wrap_v;
*/

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS2("EllipsoidMap", XFormedMap2D3D, CBnode*);
};


/*****************************************************************
 * PlaneMap:
 *
 *   Map2D3D defining a plane. Stores an origin and 2 vectors.
 *****************************************************************/
#define CPlaneMap const PlaneMap
class PlaneMap : public Map2D3D {
 public:
   //******** MANAGERS ********
   PlaneMap(CWpt& origin, CWvec& u, CWvec& v) :
      _origin(origin) {
      set_uv(u, v);
      hookup();
   }
   
   virtual ~PlaneMap() { unhook(); } // undoes dependencies

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("PlaneMap", PlaneMap*, Map2D3D, CBnode*);

   //******** ACCESORS ********

   CWpt& origin() const { return _origin;}
   
   void set_origin(CWpt& o) { _origin = o; }
   void set_uv(CWvec& u, CWvec& v) {
      _u = u;
      _v = v;
      assert(!(_u.is_null() || _v.is_null()));
   }

   Wplane plane() const { return Wplane(_origin, _u, _v); }
   
   //******** Map2D3D VIRTUAL METHODS ********
 
   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv) const {
      return _origin + (_u * uv[0]) + (_v * uv[1]);
   }

   // Partial derivative
   virtual Wvec du(CUVpt&)      const   { return _u; }
   virtual Wvec dv(CUVpt&)      const   { return _v; }

   virtual double hlen() const { return 1.0e-9; }

   virtual double u_min() const { return -DBL_MAX; }
   virtual double u_max() const { return  DBL_MAX; }
   virtual double v_min() const { return -DBL_MAX; }
   virtual double v_max() const { return  DBL_MAX; }

   //******** MAP VIRTUAL METHODS ********

   // Rigid-body transforms are supported:
   virtual bool can_transform()         const   { return true; }
   virtual void transform(CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // No inputs
   virtual Bnode_list inputs()  const { return Bnode_list(); }

 protected:

   Wpt _origin;

   // _u and _v are vectors that span the plane
   Wvec _u;
   Wvec _v;

};

/*****************************************************************
 * TubeMap:
 *
 *  Generalized cylinder based on "axis," start and end
 *  cross sections, and sweep curve.
 *
 *  u varies along "cross section" curve.
 *  v varies along "sweep curve".
 *****************************************************************/
class TubeMap : public Map2D3D {
 public:
   //******** MANAGERS ********

   TubeMap(Map1D3D* axis, Map1D3D* cross_section, CWpt_list& spts);

   virtual ~TubeMap() { unhook(); } // undoes dependencies

   //******** ACCESSORS ********

   Map1D3D* axis()      const { return _a; }
   Map1D3D* c0()        const { return _c0; }   // lower cross section
   Map1D3D* c1()        const { return _c1; }   // upper cross section

   // should be able to get and set the sweep points
   Wpt_list get_wpts()  const { return _s; }
   void set_pts(CWpt_list& pts);

   //******** RUN-TIME TYPE ID ********

   DEFINE_RTTI_METHODS3("TubeMap", TubeMap*, Map2D3D, CBnode*);

   //******** Map2D3D VIRTUAL METHODS ********
 
   virtual bool wrap_u() const { return true; }

   // Point on surface at parameter uv
   virtual Wpt map(CUVpt& uv) const;

   // Partial derivatives: numerical implementation (base class)

   // The "planck length" for this surface; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const {
      return max(max(_a->hlen(), _c0->hlen()), _c1->hlen());
   }

   virtual double u_min() const { return _c0->t_min(); }
   virtual double u_max() const { return _c0->t_max(); }

   // v ranges from 0 to 1 along the sweep curve (Wpt_list)

   //******** MAP VIRTUAL METHODS ********

   virtual bool can_transform()         const;
   virtual void transform(CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // The defining curves are the input
   virtual Bnode_list inputs()  const { return Bnode_list(_a, _c0, _c1); }

 protected:

   Map1D3D* _a;  // axis
   Map1D3D* _c0; // cross section at v = 0 (bottom)
   Map1D3D* _c1; // cross section at v = 1 (top)
   Wpt_list _s;  // sweep points
};


#endif // MAP2D3D_H_IS_INCLUDED

/* end of file mapping2d3d.H */
