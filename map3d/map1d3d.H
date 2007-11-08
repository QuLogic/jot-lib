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
 * map1d3d.H
 *****************************************************************/
#ifndef MAP1D3D_H_IS_INCLUDED
#define MAP1D3D_H_IS_INCLUDED

#include "map0d3d.H"
#include "param_list.H"

/*****************************************************************
 * Map1D3D:
 *
 *   Base class for a mapping from [0,1] to R^3,
 *   i.e. a curve in 3D.
 *
 *   We actually allow parameter values to lie outside the
 *   range [0,1]. For a closed curve, any parameter outside
 *   the range is just converted to the equivalent parameter
 *   inside the range. For a non-closed curve, we treat the
 *   curve as continuing forever in a straight line along
 *   the tangent direction at either end.
 *
 *****************************************************************/
class Map1D3D : public Map {
 public:

   //******** MANAGERS ********
   Map1D3D(Map0D3D* p0 = 0, Map0D3D* p1 = 0, mlib::CWvec& n = mlib::Wvec::null());

   virtual ~Map1D3D() { unhook(); } // clears dependencies

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Map1D3D", Map1D3D*, Map, CBnode*);

   //******** VIRTUAL METHODS ********

   // Point maps at parameters 0 and 1, respectively.
   //  Q: Why virtual?
   //  A: See ReverseMap1D3D, e.g.
   virtual Map0D3D* p0()        const   { return _p0; }
   virtual Map0D3D* p1()        const   { return _p1; }

   void replace_endpt(Map0D3D* old_pt,
                      Map0D3D* new_pt);

   virtual void set_p0(Map0D3D* p);
   virtual void set_p1(Map0D3D* p);

   // Is the curve a closed loop?
   virtual bool is_closed() const = 0;

   // Total length of curve:
   virtual double length()  const = 0;

   // Point on curve at parameter value t:
   virtual mlib::Wpt map(double t) const = 0;

   // Derivative at parameter value t.
   // Base class does numerical derivative.
   // l is a 3D length used to compute h = l/length().
   // h is the parameter-space displacement used
   // to compute numerical derivatives:
   //   f'(t) = (f(t+h) - f(t-h))/(2h)
   // (Uses hlen() if l == 0).
   virtual mlib::Wvec deriv(double t, double l=0) const;

   // The "planck length" for this curve; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const = 0;

   // Tells how many "samples" are used to describe the curve.
   // E.g., for a Wpt_listMap it's the number of points in the list;
   // for a RayMap it's 2
   virtual int nsamples() const = 0;

   // XXX - recent change (7/02):
   // The domain of the map should be an interval in t.
   // The following defines the bounds of the interval:
   virtual double t_min() const { return 0; }
   virtual double t_max() const { return 1; }

   // Return a Wpt_list describing the current shape of the map
   virtual mlib::Wpt_list get_wpts() const = 0;
   
   //******** LOCAL COORD FRAME ********

   // The local unit-length tangent vector:
   mlib::Wvec tan(double t) const { return deriv(t).normalized(); }

   // Subclasses that can provide a "normal" direction at each
   // parameter value can answer 'true' to the query is_oriented().
   // The base class offers the option of specifying a constant normal
   // direction. If you're using this, try not to let any tangents get
   // nearly equal to the constant normal direction.
   virtual bool is_oriented()   const { return !_norm.is_null(); }

   // The local unit-length normal:
   virtual mlib::Wvec norm(double t)  const {
      return _norm.orthogonalized(tan(t)).normalized();
   }
   virtual void set_norm(mlib::CWvec& n) { _norm = n.normalized(); invalidate(); }

   virtual mlib::CWvec& norm() const { return _norm; }

   // The local unit-length binormal:
   mlib::Wvec binorm(double t) const { return cross(norm(t),tan(t)); }

   // The local coordinate frame based on tangent, binormal and normal
   // vectors:
   mlib::Wtransf F(double t)  const {
      return mlib::Wtransf(map(t),tan(t),binorm(t),norm(t));
   }

   // Map to local coords of the frame (convenience):
   mlib::Wtransf Finv(double t)  const { return F(t).inverse(); }

   //******** COMPUTATION (NOT VIRTUAL) ********

   // Return an average parameter between the given parameters,
   // doing the right thing if this is a closed curve. The
   // weight is the fraction of the way from t1 to t2:
   // 
   // E.g., on a closed curve the half-way point between 
   // 0.8 and 0.1 is 0.95.
   double avg(double t1, double t2, double weight = 0.5) const;

   // Add the displacement d to the parameter t, handling
   // correctly the case that the curve wraps around.
   // E.g., 0.9 + .15 = 0.05
   double add(double t, double d) const;

   // At parameter value t, given a world displacement
   // delt_p, calculate the parameter space displacement
   // delt_t that corresponds most nearly to delt_p.
   // l is the paramter passed to deriv() used in computing
   // numerical derivatives.
   bool solve(double t, mlib::CWvec& delt_p, double& delt_t, double l=0) const;

   // Similar to above, but instead of taking a displacement to a
   // desired target location, it takes the target location
   // directly (for convenience):
   bool solve(double t, mlib::CWpt& target, double& delt_t) const {
      return solve(t, target - map(t), delt_t);
   }

   // Given world-space point p and initial parameter t for
   // which map(t) approximates p, perform an iterative
   // optimization to find the t-value that locally maps
   // closest to p. Returns false if it fails to get "close"
   // after the given maximum number of iterations.
   bool invert(mlib::CWpt &p, double t, double &ret, int max_iters=15) const;

   mlib::Wpt_list map_all(CARRAY<double>& tvals) {
      mlib::Wpt_list ret(tvals.num());
      for (int k=0; k<tvals.num(); k++)
         ret += map(tvals[k]);
      ret.update_length();
      return ret;
   }

   //******** Map VIRTUAL METHODS ********

   virtual void transform(mlib::CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // The endpoints (if any) are the inputs:
   virtual Bnode_list inputs()  const;

 protected:
   Map0D3D*     _p0;    // optional endpoints
   Map0D3D*     _p1;    //   (both null or both non-null)

   mlib::Wvec _norm;  // optional normal direction to provide orientation
};

/*****************************************************************
 * ReverseMap1D3D:
 *
 *   A Map1d3D that reverses the direction of a given Map1D3D.
 *****************************************************************/
class ReverseMap1D3D : public Map1D3D {
 public:
   //******** MANAGERS ********

   // Create a Map1D3D that reverses the direction of a given one:
   ReverseMap1D3D(Map1D3D* m);

   virtual ~ReverseMap1D3D();

   static Map1D3D* get_reverse_map(Map1D3D* m) {
      if (!m)
         return 0;
      if (isa(m))
         return ((ReverseMap1D3D*)m)->_map1d3d;
      else
         return new ReverseMap1D3D(m);
   }

   static void reverse_tvals(ARRAY<double>& tvals) {
      tvals.reverse();
      for (int k=0; k<tvals.num(); k++)
         tvals[k] = (1 - tvals[k]);
   }

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("ReverseMap1D3D", ReverseMap1D3D*, Map1D3D, CBnode*);

   //******** Map1D3D VIRTUAL METHODS ********

   // Points at parameters 0 and 1, respectively
   virtual Map0D3D* p0() const { return _map1d3d->p1(); }
   virtual Map0D3D* p1() const { return _map1d3d->p0(); }

   virtual void set_p0(Map0D3D* p) { _map1d3d->set_p1(p); }
   virtual void set_p1(Map0D3D* p) { _map1d3d->set_p0(p); }

   // Is the curve a closed loop?
   virtual bool is_closed()     const { return _map1d3d->is_closed(); }

   // Total length of curve:
   virtual double length()      const { return _map1d3d->length(); }

   // Point on curve at parameter value t:
   // XXX - should fix: assumes t ranges from 0 to 1:
   virtual mlib::Wpt map(double t)    const { return _map1d3d->map(1-t); }

   // Derivative at parameter value t:
   // XXX - should fix: assumes t ranges from 0 to 1:
   virtual mlib::Wvec deriv(double t) const { return -_map1d3d->deriv(1-t); }

   // The "planck length" for this curve; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen()        const { return _map1d3d->hlen(); }

   // Tells how many "samples" are used to describe the curve.
   virtual int nsamples()       const { return _map1d3d->nsamples(); }

   // The domain of the map should be an interval in t.
   // The following defines the bounds of the interval:
   // XXX - should fix: assumes t ranges from 0 to 1:
   virtual double t_min() const { return 0; }
   virtual double t_max() const { return 1; }

   // Return a Wpt_list describing the current shape of the map
   virtual mlib::Wpt_list get_wpts() const { 
     mlib::Wpt_list ret = _map1d3d->get_wpts();
     ret.reverse();
     return ret;
   }

   //******** LOCAL COORD FRAME ********

   // Does it have orientation?
   virtual bool is_oriented()   const { return _map1d3d->is_oriented(); }

   // The local unit-length normal orthogonal to the tangent
   // direction:
   virtual mlib::Wvec norm(double t)  const { return _map1d3d->norm(1-t); }
   virtual void set_norm(mlib::CWvec& n)    { _map1d3d->set_norm(n); }

   //******** Map VIRTUAL METHODS ********

   virtual bool can_transform() const   { return _map1d3d->can_transform(); }
   virtual void transform(mlib::CWtransf& xf, CMOD& m);

   //******** Bnode VIRTUAL METHODS ********

   // The internal map1d3d is the only input
   virtual Bnode_list inputs() const { return Bnode_list((Bnode*) _map1d3d); }

 protected:
   Map1D3D*     _map1d3d; // The internal map1d3d
};

/*****************************************************************
 * Wpt_listMap:
 *
 *   Map1D3D based on a Wpt_list and optionally two map0D3Ds 
 *   at the endpoints.
 *****************************************************************/
class Wpt_listMap : public Map1D3D {
 public:
   //******** MANAGERS ********

   // Constructor takes optional endpoints.
   // If they are given the Wpt_list cannot be closed.
   Wpt_listMap(mlib::CWpt_list& pts,
               Map0D3D* p0 = 0,
               Map0D3D* p1 = 0,
               mlib::CWvec&    n = mlib::Wvec::null()
      );

   virtual ~Wpt_listMap() {}

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("Wpt_listMap", Wpt_listMap*, Map1D3D, CBnode*);

   //******** ACCESSORS ********
   
   mlib::CWpt_list&   pts()                   const   { return _pts; }
   void         set_pts(mlib::CWpt_list& pts);

   //******** Map1D3D VIRTUAL METHODS ********

   // Is the curve a closed loop?
   virtual bool is_closed()  const { return _pts.is_closed(); }

   // Total length of curve:
   virtual double length()   const { return _pts.length(); }

   // Point on curve at parameter value t:
   virtual mlib::Wpt map(double t) const;

   // The "planck length" for this curve; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const {
      return ((_pts.num()  < 2) ?  1.0 :
              (_pts.num() == 2) ? 1e-9 :
              1.0/_pts.num()
         );
   }

   // Tells how many "samples" are used to describe the curve.
   virtual int nsamples()       const { return _pts.num(); }

   // Return a Wpt_list describing the current shape of the map
   virtual mlib::Wpt_list get_wpts() const { return _pts; }

   //******** Map VIRTUAL METHODS ********

   virtual bool can_transform() const   { return true; }
   virtual void transform(mlib::CWtransf& xf, CMOD& m);

 protected:
   mlib::Wpt_list     _pts;   // The body of the curve

   //******** Bnode VIRTUAL METHODS ********

 public:
   // After endpoints change, re-adjust curve to match endpoints:
   virtual void recompute();
};

/*****************************************************************
 * SurfaceCurveMap:
 *
 *   Represents a curve embedded in a surface. Defined by a
 *   UVpt_list and a Map2D3D. 
 *
 *   Different policy than base class: the paramter is clamped to
 *   [0,1] for non-closed curves, since we don't know how to
 *   extend the curve in a "straight line" past the endpoints.
 *****************************************************************/
class SurfaceCurveMap : public Map1D3D {
 public:
   //******** MANAGERS ********

   // Constructor takes optional endpoints, but if they are
   // null the UVpt_list has to be closed.
   SurfaceCurveMap(Map2D3D* surface,
                   mlib::CUVpt_list& uvs,
                   Map0D3D* p0 = 0,
                   Map0D3D* p1 = 0);

   virtual ~SurfaceCurveMap() { unhook(); } // undo dependencies

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("SurfaceCurveMap", SurfaceCurveMap*, Map1D3D, CBnode*);

   //******** ACCESSORS ********
   
   mlib::CUVpt_list&   uvs()                          const   { return _uvs; }
   void          set_uvs(mlib::CUVpt_list& uvs);
   Map2D3D*      surface()                      const { return _surf; }

   //******** Map1D3D VIRTUAL METHODS ********

   // Is the curve a closed loop?
   virtual bool is_closed()  const { return _uvs.is_closed(); }

   // Total length of curve:
   virtual double length()   const;

   // Point on curve at parameter value t:
   virtual mlib::Wpt map(double t) const;

   // Derivative at parameter value t.  Base class does
   // numerical derivative and treates the parameter as
   // valid outside [0,1].  Since we clamp the parameter to
   // [0,1], the derivative is the null vector when t is
   // outside [0,1] (for non-closed curves).
   virtual mlib::Wvec deriv(double t) const {
      if (!is_closed() && (t < 0 || t > 1))
          return mlib::Wvec::null();
      return Map1D3D::deriv(t);
   }

   // The "planck length" for this curve; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const;

   // Tells how many "samples" are used to describe the curve.
   virtual int nsamples()       const { return _uvs.num(); }

   // Return a Wpt_list describing the current shape of the map
   virtual mlib::Wpt_list get_wpts() const;

   //******** LOCAL COORD FRAME ********

   // The "normal" direction at each parameter value is the
   // surface normal.
   virtual bool is_oriented()   const { return true; }

   // The local unit-length normal:
   virtual mlib::Wvec norm(double t) const;

   virtual void set_norm(mlib::CWvec&) {}     // ignore

   //******** Bnode VIRTUAL METHODS ********

   // The endpoints are the surface, and endpoints if any:
   virtual Bnode_list inputs()  const {
      return Bnode_list((Bnode*)_surf) + Map1D3D::inputs();
   }

 protected:
   Map2D3D*     _surf;  // surface that the curve lives in
   mlib::UVpt_list    _uvs;   // uv coords of the curve WRT the surface

   //******** PROTECTED METHODS ********

   // The base class allows parameters outside the range [0,1],
   // but not us. If the parameter is outside get it back in:
   double fix(double t) const {
      return is_closed() ? WrapCoord::N(t) : clamp(t, 0.0, 1.0);
   }
   // get uv corresponding to given parameter t
   mlib::UVpt uv(double t) const { return _uvs.interpolate(fix(t)); }

   //******** Bnode VIRTUAL METHODS ********

   // After endpoints change, re-adjust curve to match endpoints:
   virtual void recompute();
};

/*****************************************************************
 * RayMap:
 *
 *   A "curve" defined from a base point (Map0D3D) and a ray
 *   direction. The curve is a straight line.
 *
 *   XXX - PARADIGM BREAK ALERT!!! This curve has no
 *         endpoints, but it isn't closed. Plus which,
 *         it goes on forever.
 *****************************************************************/
class RayMap : public Map1D3D {
 public:
   //******** MANAGERS ********

   // Constructor takes a base point and a direction.
   RayMap(Map0D3D* base_pt, mlib::CWvec& dir) :
      _base(base_pt),
      _dir(dir.normalized()) {
      assert(_base != NULL);
      assert(!_dir.is_null());
      hookup(); // create Bnode dependency on base point
   }

   virtual ~RayMap() { unhook(); } // undo dependency

   //******** RUN-TIME TYPE ID ********
   DEFINE_RTTI_METHODS3("RayMap", RayMap*, Map1D3D, CBnode*);

   //******** Map1D3D VIRTUAL METHODS ********

   // Is the curve a closed loop?
   virtual bool is_closed()  const { return false; }

   // Total length of curve:
   virtual double length()   const { return 1.0; }

   // Point on curve at parameter value t:
   virtual mlib::Wpt map(double t) const { return _base->map() + _dir*t; }

   // Derivative at parameter value t:
   virtual mlib::Wvec deriv(double) const { return _dir; }

   // The "planck length" for this curve; i.e., the step size
   // below which you'd just be sampling the noise:
   virtual double hlen() const { return 1e-9; }

   // Tells how many "samples" are used to describe the curve.
   virtual int nsamples()       const { return 2; }

   // The domain of the map should be an interval in t.
   // The following defines the bounds of the interval:
   virtual double t_min() const { return -DBL_MAX; }
   virtual double t_max() const { return  DBL_MAX; }

   // Return a Wpt_list describing the current shape of the map
   virtual mlib::Wpt_list get_wpts() const {
     mlib::Wpt_list ret(2);
     ret += map(0);
     ret += map(1);
     return ret;
   }

   //******** Bnode VIRTUAL METHODS ********

   // The input is the base point
   virtual Bnode_list inputs()  const { return Bnode_list((Bnode*)_base); }

 protected:
   Map0D3D*     _base;  // base point
   mlib::Wvec         _dir;   // ray direction

   // We don't override Map::recompute() because no cached
   // info needs to be recomputed when the base point
   // changes.
};

#endif // MAP1D3D_H_IS_INCLUDED

/* end of file map1d3d.H */
